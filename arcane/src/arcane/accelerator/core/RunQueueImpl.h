﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* RunQueueImpl.h                                              (C) 2000-2021 */
/*                                                                           */
/* Implémentation d'une 'RunQueue'.                                          */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_ACCELERATOR_CORE_RUNQUEUEIMPL_H
#define ARCANE_ACCELERATOR_CORE_RUNQUEUEIMPL_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/accelerator/core/AcceleratorCoreGlobal.h"

#include <stack>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane::Accelerator
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief File d'exécution  pour accélérateur.
 * \warning API en cours de définition.
 */
class ARCANE_ACCELERATOR_CORE_EXPORT RunQueueImpl
{
  friend class Runner;
  friend class RunQueue;
  friend class RunCommandImpl;
 private:
  RunQueueImpl(Runner* runner,Int32 id,IRunQueueRuntime* runtime);
  ~RunQueueImpl();
  RunQueueImpl(const RunQueueImpl&) = delete;
  RunQueueImpl& operator=(const RunQueueImpl&) = delete;
 public:
  static RunQueueImpl* create(Runner* r,eExecutionPolicy exec_policy);
 public:
  eExecutionPolicy executionPolicy() const { return m_execution_policy; }
 public:
  void release();
  void reset();
 private:
  RunCommandImpl* _internalCreateOrGetRunCommandImpl();
  void _internalFreeRunCommandImpl(RunCommandImpl*);
  IRunQueueRuntime* _internalRuntime() const { return m_runtime; }
  IRunQueueStream* _internalStream() const { return m_queue_stream; }
 private:
  Runner* m_runner;
  eExecutionPolicy m_execution_policy;
  IRunQueueRuntime* m_runtime;
  IRunQueueStream* m_queue_stream;
  std::stack<RunCommandImpl*> m_run_command_pool;
  Int32 m_id = 0;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane::Accelerator

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
