﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* DataVector1D.cc                                             (C) 2000-2004 */
/*                                                                           */
/* Vecteur de données 1D.                                                    */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


#include "arcane/utils/ArcanePrecomp.h"

#include "arcane/utils/DataVector1D.h"
#include "arcane/utils/Iostream.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern "C++"
void DataVector1D_RegressionTests()
{
  {
    DataVector1D<double> vec;
  }
  {
    DataVector1D<int> vec(5);
    vec.add(5);
    vec.add(7);
  }
  {
    DataVector1D<int> vec_int;
    {
      DataVector1D<int> vec;
      vec.add(3);
      vec.add(7);
      vec[1] = 1;
      DataVector1D<int> new_vec(vec);
      new_vec[1] = 3;
      vec_int = new_vec;
    }
  }

  cerr << "** END OF NON REGRESSION\n";
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

