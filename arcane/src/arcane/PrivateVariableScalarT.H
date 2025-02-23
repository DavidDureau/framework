﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* PrivateVariableScalarT.H                                    (C) 2000-2020 */
/*                                                                           */
/* Factorisation de variable scalaire du maillage.                           */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/PrivateVariableScalar.h"
#include "arcane/IMesh.h"
#include "arcane/ItemGroup.h"
#include "arcane/Variable.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> 
PrivateVariableScalarT<DataType>::
PrivateVariableScalarT(const VariableBuildInfo& vbi, const VariableInfo& vi)
: MeshVariableRef(vbi)
, m_private_part(PrivatePartType::getReference(vbi,vi))
, m_memory_trace(nullptr)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> 
PrivateVariableScalarT<DataType>::
PrivateVariableScalarT(IVariable* var)
: MeshVariableRef(var)
, m_private_part(PrivatePartType::getReference(var))
, m_memory_trace(nullptr)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> 
PrivateVariableScalarT<DataType>::
PrivateVariableScalarT(const PrivateVariableScalarT& rhs)
: MeshVariableRef(rhs)
, m_private_part(rhs.m_private_part)
, m_memory_trace(rhs.m_memory_trace)
{
  updateFromInternal();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> 
PrivateVariableScalarT<DataType>::
PrivateVariableScalarT()
: m_private_part(nullptr)
, m_memory_trace(nullptr)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> void 
PrivateVariableScalarT<DataType>::
operator=(const PrivateVariableScalarT& rhs)
{
  VariableRef::operator=(rhs);
  m_private_part = rhs.m_private_part;
  updateFromInternal();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> void 
PrivateVariableScalarT<DataType>::
updateFromInternal()
{
  arcaneCheckNull(m_private_part);

  m_view.setArray(m_private_part->valueView());
  m_memory_trace = m_private_part->memoryAccessTrace();
#ifdef ARCANE_TRACE 
  m_access_infos = m_private_part->memoryAccessInfos(); 
  m_trace_infos = m_private_part->traceInfos();
#endif
  MeshVariableRef::updateFromInternal();

	//cout << "** UPDATE FROM INTERNAL name=" << name()
  //     << " this=" << this
  //     << '\n';
  //cout.flush();

  _executeUpdateFunctors();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class DataType> ItemGroup
PrivateVariableScalarT<DataType>::
itemGroup() const
{
  return m_private_part->itemGroup();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
