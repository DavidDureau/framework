﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* MeshPartInfo.cc                                             (C) 2000-2018 */
/*                                                                           */
/* Informations sur la partie d'un maillage.                                 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/MeshPartInfo.h"
#include "arcane/IParallelMng.h"
#include "arcane/IParallelReplication.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern "C++" MeshPartInfo
makeMeshPartInfoFromParallelMng(IParallelMng* pm)
{
  return MeshPartInfo(pm->commRank(),pm->commSize(),
                      pm->replication()->replicationRank(),
                      pm->replication()->nbReplication());
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

