#ifndef ALIPHOSTRACKSEGMENTMAKERV1_H
#define ALIPHOSTRACKSEGMENTMAKERV1_H
/* Copyright(c) 1998-1999, ALICE Experiment at CERN, All rights reserved. *
 * See cxx source for full Copyright notice                               */

/* $Id$ */

//_________________________________________________________________________
// Implementation version 1 of algorithm class to construct PHOS track segments
// Associates EMC and PPSD clusters
// Unfolds the EMC cluster   
//                  
//*-- Author: Dmitri Peressounko (RRC Ki & SUBATECH)

// --- ROOT system ---

// --- Standard library ---

// --- AliRoot header files ---

#include "TObjArray.h"
#include "AliPHOSClusterizer.h"
#include "AliPHOSEmcRecPoint.h"
#include "AliPHOSPpsdRecPoint.h"
#include "AliPHOSTrackSegmentMaker.h"
#include "TMinuit.h" 

class  AliPHOSTrackSegmentMakerv1 : public AliPHOSTrackSegmentMaker {

public:

  AliPHOSTrackSegmentMakerv1() ;                     
  virtual ~ AliPHOSTrackSegmentMakerv1() ; // dtor
  
  Bool_t  FindFit(AliPHOSEmcRecPoint * emcRP, int * MaxAt, Float_t * maxAtEnergy, 
		  Int_t NPar, Float_t * FitParametres) ; //Used in UnfoldClusters, calls TMinuit
  void    FillOneModule(DigitsList * Dl, RecPointsList * emcIn, TObjArray * emcOut, RecPointsList * ppsdIn, 
			TObjArray * ppsdOutUp, TObjArray * ppsdOutLow, Int_t &PHOSModule, Int_t & emcStopedAt, 
			Int_t & ppsdStopedAt) ; // Unfolds clusters and fills temporary arrais   
  Float_t GetDistanceInPHOSPlane(AliPHOSEmcRecPoint * EmcClu , AliPHOSPpsdRecPoint * Ppsd , Bool_t & TooFar ) ; // see R0

  void    MakeLinks(TObjArray * EmcRecPoints, TObjArray * PpsdRecPointsUp, TObjArray * PpsdRecPointsLow, 
		    TClonesArray * LinkLowArray, TClonesArray *LinkUpArray) ; //Evaluates distances(links) between EMC and PPSD
  void    MakePairs(TObjArray * EmcRecPoints, TObjArray * PpsdRecPointsUp, TObjArray * PpsdRecPointsLow, 
		    TClonesArray * LinkLowArray, TClonesArray * LinkUpArray, TrackSegmentsList * trsl) ; 
                    //Finds pairs(triplets) with smallest link
  void    MakeTrackSegments(DigitsList * DL, RecPointsList * emcl, RecPointsList * ppsdl, TrackSegmentsList * trsl ) ; // does the job
  virtual void SetMaxEmcPpsdDistance(Float_t r){ fR0 = r ;}
  virtual void    SetUnfoldFlag() { fUnfoldFlag = kTRUE ; } ; 
  static Double_t ShowerShape(Double_t r) ; // Shape of shower used in unfolding; class member function (not object member function)
  void  UnfoldClusters(DigitsList * DL, RecPointsList * emcIn, AliPHOSEmcRecPoint * iniEmc, Int_t Nmax, 
		         int * maxAt, Float_t * maxAtEnergy, TObjArray * emclist) ; //Unfolds overlaping clusters using TMinuit package
  virtual void UnsetUnfoldFlag() { fUnfoldFlag = kFALSE ; } 

private:

  Float_t fDelta ;     // parameter used for sorting
  TMinuit * fMinuit ;  // Minuit object needed by cluster unfolding
  Float_t fR0 ;        // Maximum distance between a EMC RecPoint and a PPSD RecPoint   
  Bool_t fUnfoldFlag ; // Directive to unfold or not the clusters in case of multiple maxima

  ClassDef( AliPHOSTrackSegmentMakerv1,1)  // Implementation version 1 of algorithm class to make PHOS track segments 

};

#endif // AliPHOSTRACKSEGMENTMAKERV1_H
