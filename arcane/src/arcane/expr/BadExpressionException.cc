﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* BadExpressionException.cc                                   (C) 2000-2018 */
/*                                                                           */
/* Exception lorsqu'une expression n'est pas valide.                         */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


#include "arcane/utils/ArcanePrecomp.h"

#include "arcane/utils/Iostream.h"

#include "arcane/expr/BadExpressionException.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

BadExpressionException::
BadExpressionException(const String& where)
: Exception("BadExpression",where)
, m_msg("")
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

BadExpressionException::
BadExpressionException(const String& where,const String& msg)
: Exception("BadExpression",where)
, m_msg(msg)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void BadExpressionException::
explain(std::ostream& m) const
{
  if (m_msg.empty())
    m << "Expression invalide.\n";
  else
    m << m_msg << '\n';
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


