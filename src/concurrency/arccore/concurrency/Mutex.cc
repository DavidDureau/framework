﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2020 IFPEN-CEA
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* Mutex.cc                                                    (C) 2000-2012 */
/*                                                                           */
/* Mutex pour le multi-threading.                                            */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/concurrency/Mutex.h"
#include "arccore/concurrency/IThreadImplementation.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Mutex::
Mutex()
{
  m_thread_impl = Concurrency::getThreadImplementation();
  m_p = m_thread_impl->createMutex();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Mutex::
~Mutex()
{
  m_thread_impl->destroyMutex(m_p);
}

void Mutex::
lock()
{
  m_thread_impl->lockMutex(m_p);
}

void Mutex::
unlock()
{
  m_thread_impl->unlockMutex(m_p);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MutexImpl* GlobalMutex::m_p = 0;

void GlobalMutex::
init(MutexImpl* p)
{
  m_p = p;
}

void GlobalMutex::
destroy()
{
  m_p = 0;
}

void GlobalMutex::
lock()
{
  if (m_p)
    Concurrency::getThreadImplementation()->lockMutex(m_p);
}

void GlobalMutex::
unlock()
{
  if (m_p)
    Concurrency::getThreadImplementation()->unlockMutex(m_p);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
