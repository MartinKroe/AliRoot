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
//  Hits class for PHOS     
//  A hit in PHOS is the sum of all hits in a single crystal
//               
//*-- Author: Maxime Volkov (RRC KI) & Yves Schutz (SUBATECH)

// --- ROOT system ---

// --- Standard library ---
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <strstream>

// --- AliRoot header files ---
#include "AliPHOSHit.h"
#include "AliRun.h"
#include "AliConst.h"


ClassImp(AliPHOSHit)

//____________________________________________________________________________
AliPHOSHit::AliPHOSHit(Int_t primary, Int_t id, Float_t *hits)
{
  // ctor
  
   fId         = id ;
   fX          = hits[0] ;
   fY          = hits[1] ;
   fZ          = hits[2] ;
   fELOS       = hits[3] ;
   fPrimary    = primary ;
}

//____________________________________________________________________________
Bool_t AliPHOSHit::operator==(AliPHOSHit const &rValue) const
{ 
  // Two hits are identical if they have the same Id and originate from the same primary

  Bool_t rv = kFALSE ; 

  if ( fId == rValue.GetId() && fPrimary == rValue.GetPrimary() ) 
    rv = kTRUE;
  
  return rv;
}

//____________________________________________________________________________
AliPHOSHit AliPHOSHit::operator+(const AliPHOSHit &rValue) const
{
  // Add the energy of the hit
  
  AliPHOSHit added(*this);

   added.fX    = rValue.fX  ;
   added.fY    = rValue.fY ;
   added.fZ    = rValue.fZ ;

   added.fELOS += rValue.GetEnergy() ;
    
   return added;

}

//____________________________________________________________________________
ostream& operator << (ostream& out, const AliPHOSHit& hit) 
{
  // Print out Id and energy 
  
  out << "AliPHOSHit = " << hit.GetId() << " " << hit.GetEnergy() << endl ;
  return out ;
}



