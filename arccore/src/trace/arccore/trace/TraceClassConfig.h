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
/* TraceClassConfig.h                                          (C) 2000-2018 */
/*                                                                           */
/* Configuration d'une classe de messages.                                   */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_TRACE_TRACECLASSCONFIG_H
#define ARCCORE_TRACE_TRACECLASSCONFIG_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/trace/Trace.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Configuration associée à une classe de trace.
 */
class TraceClassConfig
{
 public:
  TraceClassConfig()
  : m_is_activated(true), m_is_parallel_activated(true),
    m_debug_level(Trace::Medium), m_verbose_level(Trace::UNSPECIFIED_VERBOSITY_LEVEL),
    m_flags(_defaultFlags())
  {
  }
  TraceClassConfig(bool activated,bool parallel_activated,
                   Trace::eDebugLevel dl,Int32 aflags=_defaultFlags())
  : m_is_activated(activated), m_is_parallel_activated(parallel_activated),
    m_debug_level(dl), m_verbose_level(Trace::UNSPECIFIED_VERBOSITY_LEVEL),
    m_flags(aflags)
  {
  }
 public:
  void setActivated(bool v) { m_is_activated = v; }
  bool isActivated() const { return m_is_activated; }
  void setParallelActivated(bool v) { m_is_parallel_activated = v; }
  bool isParallelActivated() const { return m_is_parallel_activated; }
  void setDebugLevel(Trace::eDebugLevel v) { m_debug_level = v; }
  Trace::eDebugLevel debugLevel() const { return m_debug_level; }
  void setVerboseLevel(Int32 v) { m_verbose_level = v; }
  Int32 verboseLevel() const { return m_verbose_level; }
  void setFlags(Int32 aflags) { m_flags = aflags; }
  Int32 flags() const { return m_flags; }
 private:
  bool m_is_activated;
  bool m_is_parallel_activated;
  Trace::eDebugLevel m_debug_level;
  Int32 m_verbose_level;
  Int32 m_flags;
 private:
  static Int32 _defaultFlags() { return Trace::PF_Default; }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif
