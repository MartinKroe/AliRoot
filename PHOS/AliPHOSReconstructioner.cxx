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

/* $Id$ */

//_________________________________________________________________________
//  Algorithm class for the reconstruction: clusterizer
//                                          track segment maker
//                                          particle identifier   
//                  
//*-- Author: Gines Martinez & Yves Schutz (SUBATECH)


// --- ROOT system ---

#include "TClonesArray.h"

// --- Standard library ---

// --- AliRoot header files ---

#include "AliPHOSReconstructioner.h"
#include "AliPHOSClusterizer.h"

ClassImp(AliPHOSReconstructioner)

//____________________________________________________________________________
AliPHOSReconstructioner::AliPHOSReconstructioner(AliPHOSClusterizer * Clusterizer, 
						 AliPHOSTrackSegmentMaker * Tracker,
						 AliPHOSPID * Pid)
{
  // ctor
  
  fClusterizer        = Clusterizer ;
  fTrackSegmentMaker  = Tracker ;
  fPID                = Pid ; 
} 


//____________________________________________________________________________
 void AliPHOSReconstructioner::Init(AliPHOSClusterizer * Clusterizer, 
						 AliPHOSTrackSegmentMaker * Tracker,
						 AliPHOSPID * Pid)
{
  fClusterizer        = Clusterizer ;
  fTrackSegmentMaker  = Tracker ;
  fPID                = Pid ; 
} 

//____________________________________________________________________________
 void AliPHOSReconstructioner::Make(TClonesArray * dl, RecPointsList * emccl, RecPointsList * ppsdl, 
				     TrackSegmentsList * trsl, RecParticlesList * rpl)
{
  // Launches the Reconstruction process in the sequence: Make the reconstructed poins (clusterize)
  //                                                      Make the track segments 
  //                                                      Make the reconstructed particles

  cout << "Start making reconstructed points (clusterizing)" << endl;
  fClusterizer->MakeClusters(dl, emccl, ppsdl);

  cout << "Start making track segments" << endl;
  fTrackSegmentMaker->MakeTrackSegments(dl, emccl, ppsdl, trsl) ;

  cout << "Start making reconstructed particles" << endl;
  fPID->MakeParticles(trsl, rpl) ; 
}
