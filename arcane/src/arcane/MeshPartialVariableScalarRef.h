﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* MeshPartialVariableScalarRef.h                              (C) 2000-2020 */
/*                                                                           */
/* Classe gérant une variable partielle scalaire sur une entité du maillage. */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_MESHPARTIALVARIABLESCALARREF_H
#define ARCANE_MESHPARTIALVARIABLESCALARREF_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/FatalErrorException.h"

#include "arcane/MeshVariableRef.h"
#include "arcane/PrivateVariableScalar.h"
#include "arcane/ItemEnumerator.h"
#include "arcane/ItemGroupRangeIterator.h"
#include "arcane/ItemPairEnumerator.h"
#include "arcane/GroupIndexTable.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Variable
 * \brief Variable partielle scalaire sur un type d'entité du maillage.
 */
template<typename DataTypeT>
class ItemPartialVariableScalarRefT
: public PrivateVariableScalarT<DataTypeT>
{
 public:

  typedef DataTypeT DataType;
  typedef UniqueArray<DataType> ValueType;
  typedef const DataType& ConstReturnReferenceType;
  typedef DataType& ReturnReferenceType;

 protected:

  typedef PrivateVariableScalarT<DataType> BaseClass;
  typedef typename BaseClass::PrivatePartType PrivatePartType;
  typedef typename BaseClass::DataTypeReturnReference DataTypeReturnReference;
#ifdef ARCANE_PROXY
  typedef typename BaseClass::ProxyType ProxyType;
#endif
  
 public:

  //! Construit une référence à la variable spécifiée dans \a vb
  ARCANE_CORE_EXPORT ItemPartialVariableScalarRefT(const VariableBuildInfo& vb,eItemKind ik);
  //! Construit une référence à partir de \a var
  explicit ARCANE_CORE_EXPORT ItemPartialVariableScalarRefT(IVariable* var);  
  //! Construit une référence à partir de \a rhs
  ARCANE_CORE_EXPORT ItemPartialVariableScalarRefT(const ItemPartialVariableScalarRefT<DataType>& rhs);
  
 protected:
  
  //! Opérateur de recopie
  ARCANE_CORE_EXPORT void operator=(const ItemPartialVariableScalarRefT<DataType>& rhs);
  
 public:
	
  ARCANE_CORE_EXPORT void fill(const DataType& value);  
  ARCANE_CORE_EXPORT void copy(const ItemPartialVariableScalarRefT<DataType>& v);
  ARCANE_CORE_EXPORT void internalSetUsed(bool v);

 public:
  
  const DataType& operator[](const Item& i) const
  {
    ARCANE_ASSERT((i.kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_ASSERT((m_table.isUsed()),("GroupIndexTable expired"));
    const GroupIndexTable& table = *m_table;
#ifdef ARCANE_PROXY
    this->_getMemoryInfo(table[i.localId()]).setRead();
#endif
    return this->_value(table[i.localId()]);
  }
#ifdef ARCANE_PROXY
  ProxyType operator[](const Item& i)
  {
    ARCANE_ASSERT((i.kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_ASSERT((m_table.isUsed()),("GroupIndexTable expired"));
    const GroupIndexTable& table = *m_table;
    return this->_getProxy(table[i.localId()]);
  }
#else
  DataTypeReturnReference operator[](const Item& i)
  {
    ARCANE_ASSERT((i.kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_ASSERT((m_table.isUsed()),("GroupIndexTable expired"));
    const GroupIndexTable& table = *m_table;
    return this->_value(table[i.localId()]);
  }
#endif

  const DataType& operator[](const ItemGroupRangeIterator& i) const
  {
    ARCANE_ASSERT((i.kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
#ifdef ARCANE_PROXY
    _getMemoryInfo(i.index()).setRead();
#endif
    return this->_value(i.index());
  }
  const DataType& operator[](const ItemEnumerator& i) const
  {
    ARCANE_ASSERT((i->kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_CHECK_ENUMERATOR(i,this->itemGroup());
#ifdef ARCANE_PROXY
    _getMemoryInfo(i.index()).setRead();
#endif
    return this->_value(i.index());
  }
  const DataType& operator[](const ItemPairEnumerator& i) const
  {
    ARCANE_ASSERT(((*i).kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
#ifdef ARCANE_PROXY
    _getMemoryInfo(i.index()).setRead();
#endif
    return this->_value(i.index());
  }
  
#ifdef ARCANE_PROXY
  ProxyType operator[](const ItemGroupRangeIterator& i)
  {
    ARCANE_ASSERT((i->kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    return this->_getProxy(i.index());
  }
  ProxyType operator[](const ItemEnumerator& i)
  {
    ARCANE_ASSERT((i->kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_CHECK_ENUMERATOR(i,this->itemGroup());
    return this->_getProxy(i.index());
  }
  ProxyType operator[](const ItemPairEnumerator& i)
  {
    ARCANE_ASSERT((i->kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    return this->_getProxy(i.index());
  }
#else
  DataTypeReturnReference operator[](const ItemGroupRangeIterator& i)
  {
    ARCANE_ASSERT((i.kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    return this->_value(i.index());
  }
  DataTypeReturnReference operator[](const ItemEnumerator& i)
  {
    ARCANE_ASSERT((i->kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    ARCANE_CHECK_ENUMERATOR(i,this->itemGroup());
    return this->_value(i.index());
  }
  DataTypeReturnReference operator[](const ItemPairEnumerator& i)
  {
    ARCANE_ASSERT(((*i).kind() == this->itemGroup().itemKind()),("Item and group kind not same"));
    return this->_value(i.index());
  }
#endif

 protected:
  
  SharedPtrT<GroupIndexTable> m_table;

 protected:

  static VariableInfo _buildVariableInfo(const VariableBuildInfo& vbi,eItemKind ik);
  static VariableTypeInfo _buildVariableTypeInfo(eItemKind ik);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Variable
 * \brief Variable scalaire sur un type d'entité du maillage.
 */
template<typename ItemTypeT,typename DataTypeT>
class MeshPartialVariableScalarRefT
: public ItemPartialVariableScalarRefT<DataTypeT>
{
 public:
 
  typedef DataTypeT DataType;
  typedef ItemTypeT ItemType;
  typedef UniqueArray<DataType> ValueType;
  typedef const DataType& ConstReturnReferenceType;
  typedef DataType& ReturnReferenceType;

  typedef ItemPartialVariableScalarRefT<DataType> BaseClass;

  typedef typename ItemTraitsT<ItemType>::ItemGroupType GroupType;

  typedef MeshPartialVariableScalarRefT<ItemType,DataType> ThatClass;
  typedef typename BaseClass::DataTypeReturnReference DataTypeReturnReference;

 public:

  //! Construit une référence à la variable spécifiée dans \a vb
  ARCANE_CORE_EXPORT MeshPartialVariableScalarRefT(const VariableBuildInfo& vb);
  //! Construit une référence à partir de \a rhs
  ARCANE_CORE_EXPORT MeshPartialVariableScalarRefT(const MeshPartialVariableScalarRefT<ItemType,DataType>& rhs);
  //! Positionne la référence de l'instance à la variable \a rhs.
  ARCANE_CORE_EXPORT void refersTo(const MeshPartialVariableScalarRefT<ItemType,DataType>& rhs);
  
 public:
 
  const DataType& operator[](const ItemType& i) const
  {
    ARCANE_ASSERT((this->m_table.isUsed()),("GroupIndexTable expired"));
    const GroupIndexTable& table = *this->m_table;
#ifdef ARCANE_PROXY
    this->_getMemoryInfo(table[i.localId()]).setRead();
#endif
    return this->_value(table[i.localId()]);
  }
#ifdef ARCANE_PROXY
  ProxyType operator[](const ItemType& i)
  {
    ARCANE_ASSERT((this->m_table.isUsed()),("GroupIndexTable expired")); 
    const GroupIndexTable& table = *this->m_table;
    return this->_getProxy(table[i.localId()]);
  }
#else
  DataTypeReturnReference operator[](const ItemType& i)
  {
    ARCANE_ASSERT((this->m_table.isUsed()),("GroupIndexTable expired"));
    const GroupIndexTable& table = *this->m_table;
    return this->_value(table[i.localId()]);
  }
#endif

  const DataType& operator[](const ItemGroupRangeIteratorT<ItemType>& i) const
  {
    return this->_value(i.index());
  }
  DataTypeReturnReference operator[](const ItemGroupRangeIteratorT<ItemType>& i)
  {
    return this->_value(i.index());
  }
  const DataType& operator[](const ItemEnumeratorT<ItemType>& i) const
  {
    ARCANE_CHECK_ENUMERATOR(i,this->itemGroup());
    return this->_value(i.index());
  }
  DataTypeReturnReference operator[](const ItemEnumeratorT<ItemType>& i)
  {
    ARCANE_CHECK_ENUMERATOR(i,this->itemGroup());
    return this->_value(i.index());
  }
  const DataType& operator[](const ItemPairEnumeratorSubT<ItemType>& i) const
  {
    return this->_value(i.index());
  }
  DataTypeReturnReference operator[](const ItemPairEnumeratorSubT<ItemType>& i)
  {
    return this->_value(i.index());
  }

  //! Groupe associé à la grandeur
  ARCANE_CORE_EXPORT GroupType itemGroup() const;
  
 private:

  static VariableFactoryRegisterer m_auto_registerer;
  static VariableInfo _buildVariableInfo(const VariableBuildInfo& vbi);
  static VariableTypeInfo _buildVariableTypeInfo();
  static VariableRef* _autoCreate(const VariableBuildInfo& vb);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
