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

/*
$Log$
Revision 1.1  2006/02/13 16:10:48  arcelli
Add classes for TOF Calibration (C.Zampolli)

author: Chiara Zampolli, zampolli@bo.infn.it
*/  

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// class for TOF calibration : PlateA                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "TObject.h"
#include "TROOT.h"
#include "TBrowser.h"
#include "TClass.h"
#include "AliTOFGeometryV4.h"
#include "AliTOFCalStrip.h"
#include "AliTOFCalPlateA.h"
#include "AliTOFChannel.h"

ClassImp(AliTOFCalPlateA)

//________________________________________________________________

AliTOFCalPlateA::AliTOFCalPlateA(){
  fCh = 0;
  fNSector = AliTOFGeometryV4::NSectors();
  fNPlate = AliTOFGeometryV4::NPlates();
  fNStripA = AliTOFGeometryV4::NStripA();
  fNStripB = AliTOFGeometryV4::NStripB();
  fNStripC = 20;
  //  fNStripC = AliTOFGeometryV4::NStripC();
  fNpadZ = AliTOFGeometryV4::NpadZ();
  fNpadX = AliTOFGeometryV4::NpadX();
}
//________________________________________________________________

AliTOFCalPlateA::AliTOFCalPlateA(AliTOFChannel *ch):
  fCh(ch)
{
  fNSector = AliTOFGeometryV4::NSectors();
  fNPlate = AliTOFGeometryV4::NPlates();
  fNStripA = AliTOFGeometryV4::NStripA();
  fNStripB = AliTOFGeometryV4::NStripB();
  fNStripC = 20;
  //  fNStripC = AliTOFGeometryV4::NStripC();
  fNpadZ = AliTOFGeometryV4::NpadZ();
  fNpadX = AliTOFGeometryV4::NpadX();
}
//________________________________________________________________

AliTOFCalPlateA::~AliTOFCalPlateA()
{
  delete[] fCh;
}

//________________________________________________________________

AliTOFCalPlateA::AliTOFCalPlateA(const AliTOFCalPlateA& pl):
  TObject(pl)
  {
    fCh = pl.fCh;
    fCh = pl.fCh;
    fNSector = pl.fNSector;
    fNPlate = pl.fNPlate;
    fNStripA = pl.fNStripA;
    fNStripB = pl.fNStripB;
    fNStripC = pl.fNStripC;
    fNpadZ = pl.fNpadZ;
    fNpadX = pl.fNpadX;

  }
//________________________________________________________________

void AliTOFCalPlateA::Browse(TBrowser *b){

  char name[10];
  for(Int_t i=0; i<fNStripA; ++i) {
    snprintf(name,sizeof(name),"Strip %2.2d",i);
    b->Add(new AliTOFCalStrip(&fCh[i*fNpadZ*fNpadX]),name);
  }
}
