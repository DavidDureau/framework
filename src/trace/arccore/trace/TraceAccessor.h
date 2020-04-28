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
/* TraceAccessor.h                                             (C) 2000-2018 */
/*                                                                           */
/* Accès aux traces.                                                         */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_TRACE_TRACEACCESSOR_H
#define ARCCORE_TRACE_TRACEACCESSOR_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/trace/TraceMessage.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class ITraceMng;
class TraceMessage;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Classe d'accès aux traces.
 * \ingroup Core
 */
class ARCCORE_TRACE_EXPORT TraceAccessor
{
 public:

  // Construit un accesseur via le gestionnaire de trace \a m.
  explicit TraceAccessor(ITraceMng* m);
  TraceAccessor(const TraceAccessor& rhs) = default;
  virtual ~TraceAccessor(); //!< Libère les ressources

 public:

  //! Gestionnaire de trace
  ITraceMng* traceMng() const;

  //! Flot pour un message d'information
  TraceMessage info() const;

  /*! \brief Flot pour un message d'information en parallèle.
   *
   * A la difference de info(), tous les processeurs écrivent ce
   * message sur la sortie standard.
   */
  TraceMessage pinfo() const;

  //! Flot pour un message d'information d'une catégorie donnée
  TraceMessage info(char category) const;

  //! Flot pour un message d'information parallèle d'une catégorie donnée
  TraceMessage pinfo(char category) const;

  /*!
   * \brief Flot pour un message d'information.
   *
   * Si \a v est \a false, le message ne sera pas affiché.
   */
  TraceMessage info(bool v) const;

  //! Flot pour un message d'avertissement
  TraceMessage warning() const;

  /*! Flot pour un message d'avertissement parallèle
   *
   * A la difference de warning(), seul le processeur maître écrit ce message.
   */
  TraceMessage pwarning() const;

  //! Flot pour un message d'erreur
  TraceMessage error() const;

  /*! Flot pour un message d'erreur parallèle
   *
   * A la difference de error(), seul le processeur maître écrit ce message.
   */
  TraceMessage perror() const;

  //! Flot pour un message de log
  TraceMessage log() const;

  //! Flot pour un message de log
  TraceMessage plog() const;

  //! Flot pour un message de log précédé de la date
  TraceMessage logdate() const;

  //! Flot pour un message d'erreur fatale
  TraceMessage fatal() const;

  //! Flot pour un message d'erreur fatale en parallèle
  TraceMessage pfatal() const;

#ifdef ARCCORE_DEBUG
  //! Flot pour un message de debug
  TraceMessageDbg debug(Trace::eDebugLevel =Trace::Medium) const;
#else
  //! Flot pour un message de debug
  TraceMessageDbg debug(Trace::eDebugLevel =Trace::Medium) const
  { return TraceMessageDbg(); }
#endif

  //! Niveau debug du fichier de configuration
  Trace::eDebugLevel configDbgLevel() const;

  //! Flot pour un message d'information d'un niveau donné
  TraceMessage info(Int32 verbose_level) const;

  //! Flot pour un message d'information avec le niveau d'information local à cette instance.
  TraceMessage linfo() const
  {
    return info(m_local_verbose_level);
  }

  //! Flot pour un message d'information avec le niveau d'information local à cette instance.
  TraceMessage linfo(Int32 relative_level) const
  {
    return info(m_local_verbose_level+relative_level);
  }

 protected:
  
  void _setLocalVerboseLevel(Int32 v) { m_local_verbose_level = v; }
  Int32 _localVerboseLevel() const { return m_local_verbose_level; }

 private:

  ITraceMng* m_trace;
  Int32 m_local_verbose_level;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  

