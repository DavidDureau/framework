﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* AnyItemVariable.h                                           (C) 2000-2012 */
/*                                                                           */
/* Variable aggrégée de types quelconques.                                   */
/*---------------------------------------------------------------------------*/

#ifndef ARCANE_ANYITEMS_ANYITEMVARIABLE_H 
#define ARCANE_ANYITEMS_ANYITEMVARIABLE_H

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/anyitem/AnyItemGlobal.h"
#include "arcane/anyitem/AnyItemGroup.h"
#include "arcane/anyitem/AnyItemLinkFamily.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ANYITEM_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*!
 * \brief Variable aggrégée de types quelconques (pas de variables tableaux)
 *
 * Par exemple :
 *
 * AnyItem::Family family;
 *
 * family << AnyItem::GroupBuilder( allFaces() ) 
 *        << AnyItem::GroupBuilder( allCells() );
 *
 * AnyItem::Variable<Real> variable(family);
 *
 * variable[allFaces()] << m_face_variable;
 * variable[allCells()] << m_cell_variable;
 *
 * Real value = 0.;
 * ENUMERATE_ANY_ITEM(iitem, family.allItems()) {
 *   value += variable[iitem];
 * }
 *
 */
template<typename DataType>
class Variable : public IFamilyObserver
{
public:
  
  /*!
   * \brief Outil pour l'ajout de variable à un groupe
   */
  class VariableAdder
  {
  public:
    
    VariableAdder(Variable<DataType>& variable, const ItemGroup& group) 
      : m_variable(variable)
      , m_group(group)
      , m_used(false) {}
    
    ~VariableAdder() 
    {
      ARCANE_ASSERT((m_used == true),("VariableAdder never used"));
    }
    
    //! Liaison d'un variable 
    template<typename K, typename T>  
    inline void operator<<(MeshVariableScalarRefT<K,T>& v) 
    {
      ARCANE_ASSERT((m_used == false),("VariableAdder already used"));
      m_variable._insertVariable(m_group,v.asArray());
      m_variable._insertInternalVariable(m_group,v.variable());
      m_used = true;
    }
    
    //! Liaison d'une variable partielle
    template<typename K, typename T> 
    inline void operator<<(MeshPartialVariableScalarRefT<K,T>& v) 
    {
      ARCANE_ASSERT((m_used == false),("VariableAdder already used"));
      m_variable._insertPartialVariable(m_group,v.asArray());
      m_variable._insertInternalVariable(m_group,v.variable());
      m_used = true;
    }
  private:
    
    //! Variable AnyItem
    Variable<DataType>& m_variable;
    
    //! Groupe de la variable
    const ItemGroup& m_group;
    
    //! Indicateur sur l'utilisation du Adder 
    bool m_used;
  };
  
public:
  
  Variable(const Family& family) 
    : m_family(family)
    , m_values(m_family.groupSize())
    , m_variables(m_family.groupSize()) 
  {
    // la famille enregistre les variables portées
    m_family.registerObserver(*this);
    for(Integer i = 0; i < m_family.groupSize(); ++i)
      m_variables[i] = NULL;
  }
  
  Variable(const Variable& v) 
    : m_family(v.m_family)
    , m_values(v.m_values)
    , m_variables(v.m_variables) 
  {
    // la famille enregistre les variables portées
    m_family.registerObserver(*this);
  }
  
  ~Variable() 
  {
    // la famille désenregistre la variable
    m_family.removeObserver(*this);
  }
  
  //! Accesseur direct par un enumerateur AnyItem
  inline DataType& operator[](const Group::BlockItemEnumerator& item) { 
    return m_values[item.groupIndex()][item.varIndex()];
  }
  
  //! Accesseur direct par un enumerateur AnyItem
  inline const DataType& operator[](const Group::BlockItemEnumerator & item) const { 
    return m_values[item.groupIndex()][item.varIndex()];
  }
  
  //! Accesseur direct par un élément de LinkFamily (LinkData)
  inline DataType& operator[](const LinkFamily::LinkData & item) { 
    return m_values[item.groupIndex()][item.varIndex()];
  }
  
  //! Accesseur direct par un élément de LinkFamily (LinkData)
  inline const DataType& operator[](const LinkFamily::LinkData & item) const { 
    return m_values[item.groupIndex()][item.varIndex()];
  }

  //! Ajout d'une variable pour un groupe
  inline VariableAdder operator[](const ItemGroup& group) 
  {
    return VariableAdder(*this,group);
  }

  template<typename T>
  inline VariableAdder operator[](const ItemGroupT<T>& group) 
  {
    return VariableAdder(*this,group);
  }

  //! Accesseur à la famille
  inline const Family& family() const { return m_family; }

  //! Tableau des variables
  inline ConstArrayView< IVariable* > variables() const 
  {
    return m_variables;
  } 
  
  //! Doonnées brutes associées à un groupe identifié relativement à sa famille
  inline ArrayView<DataType> valuesAtGroup(const Integer igrp) 
  {
    return m_values[igrp];
  }

  //! Doonnées brutes associées à un groupe identifié relativement à sa famille
  inline ConstArrayView<DataType> valuesAtGroup(const Integer igrp) const
  {
    return m_values[igrp];
  }

  //! Notification d'invalidation de la famille
  inline void notifyFamilyIsInvalidate() {
    // Si la famille change, on invalide les variables et on retaille
    m_values.resize(m_family.groupSize());
    m_variables.resize(m_family.groupSize());
    for(Integer i = 0; i < m_family.groupSize(); ++i)
      m_variables[i] = NULL; 
  }

  //! Notification d'aggrandissement de la famille
  inline void notifyFamilyIsIncreased() {
    // Si la famille est agrandie, on retaille simplement
    const Integer old_size = m_values.size();
    ARCANE_ASSERT((old_size < m_family.groupSize()),("Old size greater than new size!"));
    m_values.resize(m_family.groupSize());
    m_variables.resize(m_family.groupSize());
    for(Integer i = old_size; i < m_family.groupSize(); ++i)
      m_variables[i] = NULL; 
  }
  
private:
  
  inline void _insertVariable(ItemGroup group, ArrayView<DataType> v) 
  {
    if(m_family.isPartial(group))
      throw FatalErrorException(String::format("Group '{0}' defined partial",group.name())); 
    m_values[m_family.groupIndex(group)] = v;
  }

  inline void _insertPartialVariable(ItemGroup group, ArrayView<DataType> v) 
  {
    if(not m_family.isPartial(group))
      throw FatalErrorException(String::format("Group '{0}' not defined partial",group.name())); 
    m_values[m_family.groupIndex(group)] = v;
  }
  
  inline void _insertInternalVariable(ItemGroup group, IVariable* v) 
  {
    m_variables[m_family.groupIndex(group)] = v;
  }

private:
  
  //! Famille AnyItem des groupes
  const Family m_family;

  //! Conteneur des variables génériques
  Arcane::UniqueArray< ArrayView<DataType> > m_values;

  //! Conteneur des variables
  Arcane::UniqueArray< IVariable* > m_variables;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ANYITEM_END_NAMESPACE
ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif /* ARCANE_ANYITEMS_ANYITEMVARIABLE_H */
