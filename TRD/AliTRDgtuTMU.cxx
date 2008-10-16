/**************************************************************************
 * Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 *                                                                        *
 * Author: The ALICE Off-line Project.                                    *
 * Contributors are mentioned in the code where appropriate.              *
 *                                                                        *
 * Permission to use, copy, modify and distribute this software and its   *
 * documentation strictly for non-commercial purposes is hereby granted   *
 * without fee, provided that the above copyright notice appears in all   *
 * copies and that both the copyright notice and this permission notice   *
 * appear in the supporting documentation. The authors make no claims     *
 * about the suitability of this software for any purpose. It is          *
 * provided "as is" without express or implied warranty.                  *
 **************************************************************************/

/* $Id: AliTRDgtuTMU.cxx 28397 2008-09-02 09:33:00Z cblume $ */

////////////////////////////////////////////////////////////////////////////
//                                                                        //
//  Track Matching Unit (TMU) simulation                                  //
//                                                                        //
//  Author: J. Klein (Jochen.Klein@cern.ch)                               //
//                                                                        //
////////////////////////////////////////////////////////////////////////////

#include "TTree.h"
#include "TList.h"
#include "TVectorD.h"
#include "TMath.h"

#include "AliESDEvent.h"
#include "AliESDTrdTrack.h"

#include "AliLog.h"
#include "AliTRDgeometry.h"
#include "AliTRDpadPlane.h"

#include "AliTRDgtuParam.h"
#include "AliTRDgtuTMU.h"
#include "AliTRDtrackGTU.h"

ClassImp(AliTRDgtuTMU)

AliTRDgtuTMU::AliTRDgtuTMU(Int_t stack, Int_t sector) :
  TObject(),
  fTracklets(0x0),
  fZChannelTracklets(0x0),
  fTracks(0x0),
  fGtuParam(0x0),
  fStack(-1),
  fSector(-1)
{
  fGtuParam = AliTRDgtuParam::Instance();
  fTracklets = new TObjArray*[fGtuParam->GetNLayers()];
  fZChannelTracklets = new TList*[fGtuParam->GetNLayers()];
  for (Int_t layer = 0;  layer <  fGtuParam->GetNLayers(); layer++) {
    fTracklets[layer] = new TObjArray();
    fZChannelTracklets[layer] = new TList[fGtuParam->GetNZChannels()];
  }
  fTracks = new TList*[fGtuParam->GetNZChannels()];
  for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
      fTracks[zch] = new TList[fGtuParam->GetNRefLayers()];
  }
  if (stack > -1) 
      SetStack(stack);
  if (sector > -1)
      SetSector(sector);
}

AliTRDgtuTMU::~AliTRDgtuTMU() 
{
  for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
    delete [] fTracks[zch];
  }
  delete [] fTracks;
  for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
    fTracklets[layer]->Delete(); 
    delete [] fZChannelTracklets[layer];
    delete fTracklets[layer];
  }
  delete [] fZChannelTracklets;
  delete [] fTracklets;
}

Bool_t AliTRDgtuTMU::SetSector(Int_t sector)
{
  // set the sector

  if (sector > -1 && sector < fGtuParam->GetGeo()->Nsector() ) {
    fSector = sector; 
    return kTRUE;
  }

  AliError(Form("Invalid sector given: %i", sector));
  return kFALSE;
}

Bool_t AliTRDgtuTMU::SetStack(Int_t stack) 
{
  // Set the stack (necessary for tracking)

  if (stack > -1 && stack < fGtuParam->GetGeo()->Nstack() ) {
    fStack = stack;
    return kTRUE;
  }

  AliError(Form("Invalid stack given: %i", stack));
  return kFALSE;
}

Bool_t AliTRDgtuTMU::Reset() 
{
  // delete all tracks

  for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
    for (Int_t reflayeridx = 0; reflayeridx < fGtuParam->GetNRefLayers(); reflayeridx++) {
      fTracks[zch][reflayeridx].Clear();
    }
  }

  // delete all added tracklets
  for (Int_t layer = 0; layer < fGtuParam->GetNLinks()/2; layer++) {
    for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++)
      fZChannelTracklets[layer][zch].Clear();
    fTracklets[layer]->Delete();
  }

  fSector = -1;
  fStack = -1;

  return kTRUE;
}

Bool_t AliTRDgtuTMU::AddTracklet(AliTRDtrackletBase *tracklet, Int_t link) 
{
  // add a tracklet on the given link

  AliTRDtrackletGTU *trkl = new AliTRDtrackletGTU(tracklet); 
  fTracklets[(Int_t) link/2]->Add(trkl);
  return kTRUE;
}


Bool_t AliTRDgtuTMU::RunTMU(TList *ListOfTracks, AliESDEvent *esd) 
{
  // performs the analysis as in a TMU module of the GTU, i. e.
  // track matching
  // calculation of track parameteres (pt, deflection, ???)

  if (fStack < 0 || fSector < 0) {
    AliError("No valid stack/sector set for this TMU! Tracking aborted!");
    return kFALSE;
  }

  // ----- Input units -----
  AliInfo("--------- Running Input units ----------");
  for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
    if (!RunInputUnit(layer)) {
      AliError(Form("Input unit in layer %i failed", layer));
      return kFALSE;
    }
  }

  // ----- Z-channel units -----
  AliInfo("--------- Running Z-channel units ----------");
  for (Int_t layer = 0;  layer <  fGtuParam->GetNLayers(); layer++) {
    fZChannelTracklets[layer] = new TList[fGtuParam->GetNZChannels()];
    if (!RunZChannelUnit(layer)) {
      AliError(Form("Z-Channel unit in layer %i failed", layer));
      return kFALSE;
    }
  }

  // ----- track finding -----
  AliInfo("--------- Running tracking units ----------");
  for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
    AliInfo(Form("Track finder for Zchannel: %i", zch));
    if (!RunTrackFinder(zch, ListOfTracks)) {
      AliError(Form("Track Finder in z-channel %i failed", zch));
      return kFALSE;
    }
  } 

  // ----- Track Merging -----
  if (!RunTrackMerging(ListOfTracks)) {
    AliError("Track merging failed");
    return kFALSE;
  }

  // ----- track reconstruction -----
  if (!RunTrackReconstruction(ListOfTracks)) {
    AliError("Track reconstruction failed");
    return kFALSE;
  }

  if (esd) {
      TIter next(ListOfTracks);
      while (AliTRDtrackGTU *trk = (AliTRDtrackGTU*) next()) {
	  AliESDTrdTrack *trdtrack = trk->CreateTrdTrack();
	  esd->AddTrdTrack(trdtrack);
	  delete trdtrack;
      }
  }

  return kTRUE;
}

Bool_t AliTRDgtuTMU::RunInputUnit(Int_t layer) 
{
  // precalculations for the tracking and reconstruction

  fTracklets[layer]->Sort();
  TIter next(fTracklets[layer]);

  while ( AliTRDtrackletGTU *trk = (AliTRDtrackletGTU*) next() ) {
    trk->SetIndex(fTracklets[layer]->IndexOf(trk));

    Int_t alpha = (trk->GetYbin() >> fGtuParam->GetBitExcessY()) * fGtuParam->GetCiAlpha(layer); 
    alpha = ( 2 * trk->GetdY() - (alpha >> fGtuParam->GetBitExcessAlpha()) + 1 ) >> 1;
    trk->SetAlpha(alpha);

    Int_t yproj = trk->GetdY() * (fGtuParam->GetCiYProj(layer)); 
    yproj = ( ( ( (yproj >> fGtuParam->GetBitExcessYProj()) + trk->GetYbin() ) >> 2) + 1) >> 1;
    trk->SetYProj(yproj);

    trk->SetYPrime(trk->GetYbin() + fGtuParam->GetYt(fStack, layer, trk->GetZbin()));

//    printf("InputUnit : GetIndex(): %3i, GetZbin(): %2i, GetY() : %5i, GetdY() : %3i, GetYPrime() : %5i, GetYProj() : %5i, GetAlpha() : %3i \n", 
//	   trk->GetIndex(), trk->GetZbin(), trk->GetYbin(), trk->GetdY(), trk->GetYPrime(), trk->GetYProj(), trk->GetAlpha() );
  }
  return kTRUE;
}

Bool_t AliTRDgtuTMU::RunZChannelUnit(Int_t layer) 
{
  // run the z-channel unit

  TIter next(fTracklets[layer]);

  while (AliTRDtrackletGTU *trk = (AliTRDtrackletGTU*) next()) {
    printf("*TMU* Tracklet in stack %d, layer %2d: 0x%08x ", fStack, layer, trk->GetTrackletWord());
    for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
      if (fGtuParam->IsInZChannel(fStack, layer, zch, trk->GetZbin()) ) {
	trk->SetSubChannel(zch, fGtuParam->GetZSubchannel(fStack, layer, zch, trk->GetZbin()) );
	printf("Z%i(%i) ", zch, trk->GetSubChannel(zch));

	TIter nexttrkl(&fZChannelTracklets[layer][zch], kIterBackward);
	AliTRDtrackletGTU *t = 0x0;
	while (t = (AliTRDtrackletGTU*) nexttrkl.Next()) {
	  if (t->GetSubChannel(zch) < trk->GetSubChannel(zch) || 
	      (t->GetSubChannel(zch) == trk->GetSubChannel(zch) && t->GetYProj() < trk->GetYProj()) )
	    break;
	}
	fZChannelTracklets[layer][zch].AddAfter(t, trk);
      }
      else 
	  printf("      ");
    }
    printf("\n");
  }
  return kTRUE;
}

Bool_t AliTRDgtuTMU::RunTrackFinder(Int_t zch, TList *ListOfTracks) 
{
  // run the track finding

   Int_t 	*notr = new Int_t[fGtuParam->GetNLayers()];
   Int_t 	*ptrA = new Int_t[fGtuParam->GetNLayers()];
   Int_t 	*ptrB = new Int_t[fGtuParam->GetNLayers()];

   Bool_t 	*bHitA 	   = new Bool_t[fGtuParam->GetNLayers()]; 
   Bool_t 	*bHitB 	   = new Bool_t[fGtuParam->GetNLayers()];
   Bool_t 	*bAligned  = new Bool_t[fGtuParam->GetNLayers()];
   Bool_t 	*bAlignedA = new Bool_t[fGtuParam->GetNLayers()];
   Bool_t 	*bAlignedB = new Bool_t[fGtuParam->GetNLayers()];
   Bool_t 	*bDone 	   = new Bool_t[fGtuParam->GetNLayers()];
   Bool_t 	 ready;

   Int_t 	*inc 	  = new Int_t[fGtuParam->GetNLayers()];
   Int_t 	*incprime = new Int_t[fGtuParam->GetNLayers()];

// ----- signals within current layer -----
   Int_t 	Yplus;
   Int_t 	Yminus; 	   
   Int_t 	YBplus;	   
   Int_t 	YBminus;
   Int_t 	Alphaplus;
   Int_t 	Alphaminus; 
   Int_t 	NHits;
   Int_t 	NUnc;   
   Int_t 	NWayBeyond;

   AliTRDtrackletGTU 	*trkRA 	= 0x0;	// reference tracklet A
   AliTRDtrackletGTU 	*trkRB 	= 0x0;	// reference tracklet B
   AliTRDtrackletGTU 	*trkA  	= 0x0;	// tracklet A in current layer
   AliTRDtrackletGTU 	*trkB  	= 0x0;	// tracklet B in current layer
/*
   AliTRDtrackletGTU 	*trkEnd = new AliTRDtrackletGTU();
   for (Int_t i = 0; i < fGtuParam->GetNZChannels(); i++) 
       trkEnd->SetSubChannel(i, 7);
   trkEnd->SetYProj(0);
   trkEnd->SetAlpha(0);
*/

   for (Int_t refLayerIdx = 0; refLayerIdx < fGtuParam->GetNRefLayers(); refLayerIdx++) {
     Int_t reflayer = fGtuParam->GetRefLayer(refLayerIdx);
     AliInfo(Form("~~~~~ Reflayer: %i", reflayer));

     ready = kFALSE; // ready if all channels done

     for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
       notr[layer] = fZChannelTracklets[layer][zch].GetEntries();
       ptrA[layer] = 0; // notr[layer] > 0 ? 0 : -1;
       ptrB[layer] = 1; // notr[layer] > 1 ? 1 : -1;

// not necessary here
//       bDone[layer] = (ptrB[layer] >= notr[layer] - 1); // trkB is last one
//       bDone[layer] = (notr[layer] <= 0);
//       ready = ready && bDone[layer];

       if (reflayer == 1)
	 AliInfo(Form("in layer: %i (zchannel = %i) there are: %i tracklets", layer, zch, notr[layer]));
     }
     
     if (ptrA[reflayer] < 0 && ptrB[reflayer] < 0) 
       continue;

     while (!ready) {
       // ----- reference tracklets -----
       trkRA = 0x0;
       trkRB = 0x0;
       if (0 <= ptrA[reflayer] && ptrA[reflayer] < notr[reflayer])
	 trkRA = (AliTRDtrackletGTU*) fZChannelTracklets[reflayer][zch].At(ptrA[reflayer]);
       else  {
	 AliInfo(Form("No valid tracklet in the reference at ptr: %i! Aborting!", ptrA[reflayer]));
	 break; 
       }

       if (0 <= ptrB[reflayer] && ptrB[reflayer] < notr[reflayer])
	 trkRB = (AliTRDtrackletGTU*) fZChannelTracklets[reflayer][zch].At(ptrB[reflayer]);

       AliInfo(Form("ptrRA: %i, ptrRB: %i", ptrA[reflayer], ptrB[reflayer]));
       Yplus  	  = trkRA->GetYProj() + fGtuParam->GetDeltaY();
       Yminus 	  = trkRA->GetYProj() - fGtuParam->GetDeltaY();
       Alphaplus  = trkRA->GetAlpha() + fGtuParam->GetDeltaAlpha();
       Alphaminus = trkRA->GetAlpha() - fGtuParam->GetDeltaAlpha();
       if (trkRB) {
	 YBplus 	  = trkRB->GetYProj() + fGtuParam->GetDeltaY();
	 YBminus 	  = trkRB->GetYProj() - fGtuParam->GetDeltaY();
       }
       else { // irrelevant (should be, is it?)
	 YBplus 	  = trkRA->GetYProj() + fGtuParam->GetDeltaY();
	 YBminus 	  = trkRA->GetYProj() - fGtuParam->GetDeltaY();
       }

       NHits 	  = 0;
       NUnc 	  = 0;
       NWayBeyond = 0;
       
       for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
	 bHitA[layer] = bHitB[layer] = bAligned[layer] = kFALSE;
	 inc[layer] = incprime[layer] = 0;

	 if (layer == reflayer) {
	   bHitA[layer] = kTRUE;
	   bAligned[layer] = kTRUE;
	   NHits++;
	   continue; 
	 }

	 trkA = 0x0;
	 trkB = 0x0;
	 if (0 <= ptrA[layer] && ptrA[layer] < notr[layer])
	   trkA = (AliTRDtrackletGTU*) fZChannelTracklets[layer][zch].At(ptrA[layer]);
	 if (0 <= ptrB[layer] && ptrB[layer] < notr[layer])
	   trkB = (AliTRDtrackletGTU*) fZChannelTracklets[layer][zch].At(ptrB[layer]);

	 bAlignedA[layer] = kFALSE;
	 bAlignedB[layer] = kFALSE;

	 if (trkA) { 
	   bHitA[layer] = ( !(trkA->GetSubChannel(zch) < trkRA->GetSubChannel(zch) || (trkA->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkA->GetYProj() < Yminus) ) &&
			    !(trkA->GetSubChannel(zch) > trkRA->GetSubChannel(zch) || (trkA->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkA->GetYProj() > Yplus)  ) &&
			    !(trkA->GetAlpha() < Alphaminus) &&
			    !(trkA->GetAlpha() > Alphaplus) );
	   bAlignedA[layer] = !(trkA->GetSubChannel(zch) < trkRA->GetSubChannel(zch) || (trkA->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkA->GetYProj() < Yminus) ); 
	 }
	 else {
	   bHitA[layer] = 0;
	   bAlignedA[layer] = kTRUE;
	 }

	 if (trkB) {
	   bHitB[layer] = ( !(trkB->GetSubChannel(zch) < trkRA->GetSubChannel(zch) || (trkB->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkB->GetYProj() < Yminus) ) &&
			    !(trkB->GetSubChannel(zch) > trkRA->GetSubChannel(zch) || (trkB->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkB->GetYProj() > Yplus) ) &&
			    !(Alphaminus > trkB->GetAlpha()) &&
			    !(Alphaplus  > trkB->GetAlpha()) );
	   bAlignedB[layer] = (trkB->GetSubChannel(zch) > trkRA->GetSubChannel(zch) || (trkB->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkB->GetYProj() > Yplus) );
	 } 
	 else {
	   bHitB[layer] = 0;
	   bAlignedB[layer] = kTRUE;
	 }
	  
	 bAligned[layer] = bAlignedA[layer] || bAlignedB[layer]; //???
//	 bAligned[layer] = bAlignedA[layer]; //???
	  
	 if (bAligned[layer] && (bHitA[layer] || bHitB[layer]) )
	   NHits++;
	 else if (!bAligned[layer] )
	   NUnc++;
	 if (trkRB) {
	   if (trkA) {
	     if ((trkA->GetSubChannel(zch) > trkRB->GetSubChannel(zch)) || (trkA->GetSubChannel(zch) == trkRB->GetSubChannel(zch) && trkA->GetYProj() > YBplus) )
	       NWayBeyond++;
	   }
	   else 
	     NWayBeyond++;
	 }

	 //  pre-calculation for the layer shifting (alignment w. r. t. trkRB)
	 if (trkA) {
	     if(trkRB) {
		 if ((trkA->GetSubChannel(zch) < trkRB->GetSubChannel(zch)) || (trkA->GetSubChannel(zch) == trkRB->GetSubChannel(zch) && trkA->GetYProj() < YBminus )) // could trkA be aligned for trkRB
		     incprime[layer] = 1;
		 else 
		     incprime[layer] = 0;
	     }
	     else 
		 incprime[layer] = 1;

	     if (trkB) { 
		 if (trkRB) {
		     if ((trkB->GetSubChannel(zch) < trkRB->GetSubChannel(zch)) || (trkB->GetSubChannel(zch) == trkRB->GetSubChannel(zch) && trkB->GetYProj() < YBminus )) // could trkB be aligned for trkRB
			 incprime[layer] = 2;
		 }
		 else 
		     incprime[layer] = 2;
	     }
	 }
       } // end of loop over layers

       AliInfo(Form("logic calculation finished, Nhits: %i", NHits));

       if (NHits >= 4) {
	 // ----- track registration -----
	 AliInfo("***** TMU: Track found *****");
	 AliTRDtrackGTU *track = new AliTRDtrackGTU();
	 track->SetSector(fSector);
	 track->SetStack(fStack);
	 for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++ ) {
	   if (bHitA[layer] || layer == reflayer) 
	     track->AddTracklet((AliTRDtrackletGTU* ) fZChannelTracklets[layer][zch].At(ptrA[layer]), layer );
	   else if (bHitB[layer]) 
	     track->AddTracklet((AliTRDtrackletGTU* ) fZChannelTracklets[layer][zch].At(ptrB[layer]), layer );
	 }

	 Bool_t registerTrack = kTRUE;
	 for (Int_t layerIdx = refLayerIdx; layerIdx > 0; layerIdx--) {
	     if (track->IsTrackletInLayer(fGtuParam->GetRefLayer(layerIdx)))
		 registerTrack = kFALSE;
	 }
	 if (registerTrack) {
	     track->SetZChannel(zch);
	     track->SetRefLayerIdx(refLayerIdx);
	     fTracks[zch][refLayerIdx].Add(track);
	 }
       }
	
       if ( (NUnc != 0) && (NUnc + NHits >= 4) ) // could this position of the reference layer give some track //??? special check in case of hit?
	  inc[reflayer] = 0;
       else if (NWayBeyond > 2) // no track possible for both reference tracklets
	 inc[reflayer] = 2;
       else 
	 inc[reflayer] = 1;
	
       if (inc[reflayer] != 0) // reflayer is shifted
	 for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
	   if (layer == reflayer)
	     continue;
	   inc[layer] = incprime[layer]; 
	 }
       else { // reflayer is not shifted
	 for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
	   if (layer == reflayer || notr[layer] == 0)
	     continue;
	   inc[layer] = 0;
	   trkA = 0x0;
	   trkB = 0x0;
	   if (0 <= ptrA[layer] && ptrA[layer] < notr[layer])
	     trkA = (AliTRDtrackletGTU*) fZChannelTracklets[layer][zch].At(ptrA[layer]);

	   if (0 <= ptrB[layer] && ptrB[layer] < notr[layer])
	     trkB = (AliTRDtrackletGTU*) fZChannelTracklets[layer][zch].At(ptrB[layer]);

	   if (trkA) {
	       if ( !(trkA->GetSubChannel(zch) < trkRA->GetSubChannel(zch) || (trkA->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkA->GetYProj() < Yminus) ) && 
		    !(trkA->GetSubChannel(zch) > trkRA->GetSubChannel(zch) || (trkA->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkA->GetYProj() > Yplus ) ) )  // trkA could hit trkRA
		   inc[layer] = 0;
	       else if (trkB) {
		   if ( trkB->GetSubChannel(zch) < trkRA->GetSubChannel(zch) || (trkB->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkB->GetYProj() < Yminus) )
		       inc[layer] = 2;
		   else if ( !(trkB->GetSubChannel(zch) > trkRA->GetSubChannel(zch) || (trkB->GetSubChannel(zch) == trkRA->GetSubChannel(zch) && trkB->GetYProj() > Yplus) ) )
		       inc[layer] = 1;
		   else 
		       inc[layer] = incprime[layer];
	       }
	       else 
		   inc[layer] = incprime[layer];
	   }
	 }
       }
       
       ready = kTRUE;
       for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {

	 bDone[layer] = ptrB[layer] < 0 || ptrB[layer] >= notr[layer];
	 ready = ready && bDone[layer];

	 if (inc[layer] != 0 && ptrA[layer] >= notr[layer])
	   AliError(Form("Invalid increment: %i at ptrA: %i, notr: %i", inc[layer], ptrA[layer], notr[layer]));

//	 AliInfo(Form("Shifting layer: %i, notr: %i, ptrA: %i, ptrB: %i, inc: %i", layer, notr[layer], ptrA[layer], ptrB[layer], inc[layer]));
	 AliInfo(Form(" -- Layer: %i   %i   %i   +%i   %s  (no: %i)", layer, ptrA[layer], ptrB[layer], inc[layer], bDone[layer] ? "done" : "    ", notr[layer]));
 	 ptrA[layer] += inc[layer];
	 ptrB[layer] += inc[layer];
       }

     } // end of while
   } // end of loop over reflayer

   delete [] bHitA;
   delete [] bHitB;
   delete [] bAligned;
   delete [] bDone;
   delete [] inc;
   delete [] incprime;
   delete [] bAlignedA;
   delete [] bAlignedB;
   delete [] notr;
   delete [] ptrA;
   delete [] ptrB;

   return kTRUE;
}

Bool_t AliTRDgtuTMU::RunTrackMerging(TList* ListOfTracks) 
{
    TList **tracksRefMerged = new TList*[fGtuParam->GetNZChannels()];
    TList **tracksRefUnique = new TList*[fGtuParam->GetNZChannels()];

    for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
	tracksRefMerged[zch] = new TList;
	tracksRefUnique[zch] = new TList;
    }

    TList *tracksZMergedStage0 = new TList;
    TList *tracksZUniqueStage0 = new TList;

    TList **tracksZSplitted = new TList*[2];
    for (Int_t i = 0; i < 2; i++)
	tracksZSplitted[i] = new TList;

    TList *tracksZMergedStage1 = new TList;

    AliTRDtrackGTU **trk = new AliTRDtrackGTU*[fGtuParam->GetNRefLayers()];

    Bool_t done = kFALSE;
    Int_t minIdx = 0;
    AliTRDtrackGTU *trkStage0 = 0x0;

    for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
        // ----- Merging and Unification in Reflayers -----
	do {
	    done = kTRUE;
	    trkStage0 = 0x0;
	    for (Int_t refLayerIdx = 0; refLayerIdx < fGtuParam->GetNRefLayers(); refLayerIdx++) {
		trk[refLayerIdx] = (AliTRDtrackGTU*) fTracks[zch][refLayerIdx].First();
		if (trk[refLayerIdx] == 0) {
		    continue;
		}
		else if (trkStage0 == 0x0 ) {
		    trkStage0 = trk[refLayerIdx];
		    minIdx = refLayerIdx;
		    done = kFALSE;
		}
		else if (trk[refLayerIdx]->GetZSubChannel() < trkStage0->GetZSubChannel() || 
			 (trk[refLayerIdx]->GetZSubChannel() == trkStage0->GetZSubChannel() && trk[refLayerIdx]->GetYapprox() < trkStage0->GetYapprox()) ) {
		    minIdx = refLayerIdx;
		    trkStage0 = trk[refLayerIdx];
		    done = kFALSE;
		}
	    }

	    tracksRefMerged[zch]->Add(trkStage0);
	    fTracks[zch][minIdx].RemoveFirst();
	} while (trkStage0 != 0);

	Uniquifier(tracksRefMerged[zch], tracksRefUnique[zch]);
    }

// ----- Merging in zchannels - 1st stage -----
    
    do {
	done = kTRUE;
	trkStage0 = 0x0;
	for (Int_t zch = 0; zch < fGtuParam->GetNZChannels(); zch++) {
	    AliTRDtrackGTU *trk = (AliTRDtrackGTU*) tracksRefUnique[zch]->First();
	    if (trk == 0) {
		continue;
	    }
	    else if (trkStage0 == 0x0 ) {
		trkStage0 = trk;
		minIdx = zch;
		done = kFALSE;
	    }
	    else if ( ((trk->GetZChannel() + 3 * trk->GetZSubChannel()) / 2 - 1) < ((trkStage0->GetZChannel() + 3 * trkStage0->GetZSubChannel()) / 2 -1 ) ) {
		minIdx = zch;
		trkStage0 = trk;
		done = kFALSE;
	    }
	}
	
	
	tracksZMergedStage0->Add(trkStage0);
	tracksRefUnique[minIdx]->RemoveFirst();
    } while (trkStage0 != 0);
    
    Uniquifier(tracksZMergedStage0, tracksZUniqueStage0);
    
// ----- Splitting in z -----
    
    TIter next(tracksZUniqueStage0);
    while (AliTRDtrackGTU *trk = (AliTRDtrackGTU*) next()) {
	tracksZSplitted[(trk->GetZChannel() + 3 * (trk->GetZSubChannel() - 1)) % 2]->Add(trk);
    }
    
// ----- Merging in zchanels - 2nd stage -----
    
    do {
	done = kTRUE;
	trkStage0 = 0x0;
	for (Int_t i = 0; i < 2; i++) {
	    AliTRDtrackGTU *trk = (AliTRDtrackGTU*) tracksZSplitted[i]->First();
	    if (trk == 0) {
		continue;
	    }
	    else if (trkStage0 == 0x0 ) {
		trkStage0 = trk;
		minIdx = i;
		done = kFALSE;
	    }
	    else if ( ((trk->GetZChannel() + 3 * (trk->GetZSubChannel() - 1)) / 2) < ((trkStage0->GetZChannel() + 3 * (trkStage0->GetZSubChannel() - 1)) / 2) ) {
		minIdx = i;
		trkStage0 = trk;
		done = kFALSE;
	    }
	}
	
	tracksZMergedStage1->Add(trkStage0);
	tracksZSplitted[minIdx]->RemoveFirst();
    } while (trkStage0 != 0);
    
    Uniquifier(tracksZMergedStage1, ListOfTracks);
    
    return kTRUE;
}

Bool_t AliTRDgtuTMU::RunTrackReconstruction(TList* ListOfTracks) 
{
  TIter next(ListOfTracks);
  while (AliTRDtrackGTU *track = (AliTRDtrackGTU*) next()) {
    CalculateTrackParams(track);
  }
  return kTRUE;
}

Bool_t AliTRDgtuTMU::CalculateTrackParams(AliTRDtrackGTU *track) 
{
  // calculate the track parameters

  if (!track) {
    AliError("No track to calculate!");
    return kFALSE;
  }

  Int_t a = 0;
  Float_t b = 0;
  Float_t c = 0;
  Float_t x1;
  Float_t x2;

  AliInfo(Form("There are %i tracklets in this track.", track->GetNTracklets()));

  for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
    if (!track->IsTrackletInLayer(layer)) {
      continue;
    }
    AliTRDtrackletGTU *trk = track->GetTracklet(layer);
    if (!trk) {
      AliError(Form("Could not get tracklet in layer %i\n", layer));
      continue;
    }
    AliInfo(Form("trk yprime: %i", trk->GetYPrime()));
    a += (((Int_t) (2048 * fGtuParam->GetAki(track->GetTrackletMask(), layer))) * trk->GetYPrime() + 1) >> 8; 
    b += fGtuParam->GetBki(track->GetTrackletMask(), layer) * trk->GetYPrime() * fGtuParam->GetBinWidthY();
    c += fGtuParam->GetCki(track->GetTrackletMask(), layer) * trk->GetYPrime() * fGtuParam->GetBinWidthY();
  }
  if (a < 0)
      a += 3;
  a = a >> 2;

  fGtuParam->GetIntersectionPoints(track->GetTrackletMask(), x1, x2);
  AliInfo(Form("Intersection points: %f, %f", x1, x2));
  AliInfo(Form("Sum: a = %5i, b = %9.2f, c = %9.2f\n", a, b, c));
  track->SetFitParams(a, b, c);

  Float_t r = fGtuParam->GetRadius(a, b, x1, x2);
  Int_t pt = (Int_t) (2 * r);
  if (pt >= 0) 
      pt += 32;
  else
      pt -= 29;
  pt /= 2;
  track->SetPtInt(pt);
  AliInfo(Form("Track parameters: a = %i, b = %f, c = %f, x1 = %f, x2 = %f, r = %f, pt = %f (trkl mask: %i)", a, b, c, x1, x2, r, track->GetPt(), track->GetTrackletMask()));
  return kTRUE;
}

Bool_t AliTRDgtuTMU::WriteTrackletsToTree(TTree *trklTree)
{
  if (!trklTree) {
    AliError("No tree given");
    return kFALSE;
  }
  AliTRDtrackletGTU *trkl = 0x0;
  TBranch *branch = trklTree->GetBranch("gtutracklets");
  if (!branch) {
      branch = trklTree->Branch("gtutracklets", "AliTRDtrackletGTU", &trkl, 32000, 99);
  }

  AliInfo(Form("---------- Writing tracklets to tree (not yet) ----------"));
  for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
    TIter next(fTracklets[layer]);
    while (trkl = (AliTRDtrackletGTU*) next()) {
	AliInfo(Form("InputUnit : GetIndex(): %3i, GetZbin(): %2i, GetY() : %5i, GetdY() : %3i, GetYPrime() : %5i, GetYProj() : %5i, GetAlpha() : %3i, Zidx(2..0): %i  %i  %i", trkl->GetIndex(), trkl->GetZbin(), trkl->GetYbin(), trkl->GetdY(), trkl->GetYPrime(), trkl->GetYProj(), trkl->GetAlpha(), trkl->GetSubChannel(2), trkl->GetSubChannel(1), trkl->GetSubChannel(0) ));
	branch->SetAddress(&trkl);
	trklTree->Fill();
    }
  }
  return kTRUE;
}

Bool_t AliTRDgtuTMU::Uniquifier(TList *inlist, TList *outlist)
{
    TIter next(inlist);
    AliTRDtrackGTU *trkStage0 = 0x0;
    AliTRDtrackGTU *trkStage1 = 0x0;

    do {
	trkStage0 = (AliTRDtrackGTU*) next();

	Bool_t tracks_equal = kFALSE;
	if (trkStage0 != 0 && trkStage1 != 0) {
	    for (Int_t layer = 0; layer < fGtuParam->GetNLayers(); layer++) {
		if (trkStage0->GetTrackletIndex(layer) != -1 && trkStage0->GetTrackletIndex(layer) == trkStage1->GetTrackletIndex(layer)) {
		    tracks_equal = kTRUE;
		    break;
		}
	    }
	}

	if (tracks_equal) {
	    if (trkStage0->GetNTracklets() > trkStage1->GetNTracklets())
		trkStage1 = trkStage0;
	} 
	else {
	    if (trkStage1 != 0x0) 
		outlist->Add(trkStage1);
	    trkStage1 = trkStage0;
	} 
	
    } while (trkStage1 != 0x0);
    return kTRUE;
}
