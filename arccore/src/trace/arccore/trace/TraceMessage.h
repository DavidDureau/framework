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
/* TraceMessage.h                                              (C) 2000-2018 */
/*                                                                           */
/* Message de trace.                                                         */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_TRACE_TRACEMESSAGE_H
#define ARCCORE_TRACE_TRACEMESSAGE_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/trace/Trace.h"

#include <iostream>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Gestion d'un message.
 *
 Cette classe se gère comme un flot de sortie standard (ostream&) et
 permet d'envoyer un message du type spécifié par #eTraceMessageClass.
 
 \warning Les instances de cette classe sont normalement créées par
 un gestionnaire de message ITraceMng.
*/
class ARCCORE_TRACE_EXPORT TraceMessage
{
 public:
  static const int DEFAULT_LEVEL = Trace::DEFAULT_VERBOSITY_LEVEL;
 public:
  TraceMessage(std::ostream*,ITraceMng*,Trace::eMessageType,int level =DEFAULT_LEVEL);
  TraceMessage(const TraceMessage& from);
  const TraceMessage& operator=(const TraceMessage& from);
  ~TraceMessage() ARCCORE_NOEXCEPT_FALSE;

 public:
  std::ostream& file() const;
  const TraceMessage& width(Integer v) const;
  ITraceMng* parent() const { return m_parent; }
  Trace::eMessageType type() const { return m_type; }
  int level() const { return m_level; }
  int color() const { return m_color; }
 private:
  std::ostream* m_stream; //!< Flot sur lequel le message est envoyé
  ITraceMng* m_parent; //!< Gestionnaire de message parent
  Trace::eMessageType m_type; //!< Type de message
  int m_level; //!< Niveau du message
 public:
  mutable int m_color; //!< Couleur du message.
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Trace
{
ARCCORE_TRACE_EXPORT std::ostream&
operator<< (std::ostream& o,const Width& w);

ARCCORE_TRACE_EXPORT std::ostream&
operator<< (std::ostream& o,const Precision& w);
}

#ifndef ARCCORE_DEBUG
class ARCCORE_TRACE_EXPORT TraceMessageDbg
{
};
template<class T> inline const TraceMessageDbg&
operator<<(const TraceMessageDbg& o,const T&)
{
  return o;
}
#endif

ARCCORE_TRACE_EXPORT const TraceMessage&
operator<<(const TraceMessage& o,const Trace::Color& c);

template<class T> inline const TraceMessage&
operator<<(const TraceMessage& o,const T& v)
{
  o.file() << v;
  return o;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif

