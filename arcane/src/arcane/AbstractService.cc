﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* AbstractService.cc                                          (C) 2000-2017 */
/*                                                                           */
/* Classe de base d'un service.                                              */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/ArcanePrecomp.h"

#include "arcane/AbstractService.h"
#include "arcane/ServiceBuildInfo.h"
#include "arcane/IBase.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

AbstractService::
AbstractService(const ServiceBuildInfo& sbi)
: TraceAccessor(sbi.serviceParent()->traceMng())
, m_service_info(sbi.serviceInfo())
, m_parent(sbi.serviceParent())
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

AbstractService::
~AbstractService()
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

