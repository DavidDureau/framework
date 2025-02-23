﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* Observer.cc                                                 (C) 2000-2021 */
/*                                                                           */
/* Observateur.                                                              */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/Observer.h"
#include "arcane/utils/FatalErrorException.h"
#include "arcane/utils/TraceInfo.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

AbstractObserver::
~AbstractObserver()
{
  if (m_observable)
    m_observable->detachObserver(this);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void AbstractObserver::
attachToObservable(IObservable* obs)
{
  if (m_observable)
    ARCANE_FATAL("Observer is already attached");
  m_observable = obs;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void AbstractObserver::
detach()
{
  m_observable = 0;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

