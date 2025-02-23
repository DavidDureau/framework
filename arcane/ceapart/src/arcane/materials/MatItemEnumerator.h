﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* MatItemEnumerator.h                                         (C) 2000-2016 */
/*                                                                           */
/* Enumérateurs sur les mailles materiaux.                                   */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_MATERIALS_MATITEMENUMERATOR_H
#define ARCANE_MATERIALS_MATITEMENUMERATOR_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \file MatItemEnumerator.h
 *
 * Ce fichier contient les différents types d'itérateur et macros
 * pour itérer sur les mailles matériaux et milieux.
 */
#include "arcane/utils/FatalErrorException.h"

#include "arcane/EnumeratorTraceWrapper.h"

#include "arcane/materials/MatItem.h"
#include "arcane/materials/MeshMaterialVariableIndexer.h"
#include "arcane/materials/IEnumeratorTracer.h"

// Laisse ces 2 includes pour des raisons de compatibilité
// mais il faudrait les supprimer
#include "arcane/materials/IMeshMaterialMng.h"
#include "arcane/materials/IMeshBlock.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
//! Active toujours les traces dans les parties Arcane concernant les matériaux.
#ifdef ARCANE_COMPONENT_arcane_materials
#ifndef ARCANE_TRACE_ENUMERATOR
#define ARCANE_TRACE_ENUMERATOR
#endif
#endif

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE
MATERIALS_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class MatCellVector;
class EnvCellVector;
class ComponentItemVector;
class ComponentItemVectorView;
class ComponentPartItemVectorView;
class MatItemVectorView;
class EnvItemVectorView;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Vue sur une liste de mailles avec infos sur les milieux.
 *
 * Comme toute vue, cet objet n'est valide que tant que le conteneur
 * associé (en général un CellGroup) n'est pas modifié.
 */
class ARCANE_MATERIALS_EXPORT AllEnvCellVectorView
{
 public:

  AllEnvCellVectorView(Int32ConstArrayView local_ids,ArrayView<ComponentItemInternal> items_internal)
  : m_local_ids(local_ids), m_items_internal(items_internal)
  {
  }

 public:

  //! Nombre d'éléments.
  Integer size() const { return m_local_ids.size(); }

  // \i ème maille du vecteur
  AllEnvCell operator[](Integer index)
  {
    Int32 lid = m_local_ids[index];
    return AllEnvCell(&m_items_internal[lid]);
  }

  // localId() de la \i ème maille du vecteur
  Int32 localId(Integer index)
  {
    return m_local_ids[index];
  }

 private:

  Int32ConstArrayView m_local_ids;
  ArrayView<ComponentItemInternal> m_items_internal;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur une partie des mailles d'un composant (matériau ou milieu)
 */
class ARCANE_MATERIALS_EXPORT ComponentCellEnumerator
{
  friend class EnumeratorTracer;
 protected:
  ComponentCellEnumerator(ConstArrayView<ComponentItemInternal*> items,
                          ConstArrayView<MatVarIndex> matvar_indexes,
                          IMeshComponent* component)
  : m_index(0), m_size(items.size()), m_items(items)
  , m_matvar_indexes(matvar_indexes), m_component(component)
  {
#ifdef ARCANE_CHECK
    if (m_index<m_size)
      _check();
#endif
  }
 public:
  static ComponentCellEnumerator create(IMeshComponent* component);
  static ComponentCellEnumerator create(const ComponentItemVector& v);
  static ComponentCellEnumerator create(ComponentItemVectorView v);
 public:
  void operator++()
  {
    ++m_index;
#ifdef ARCANE_CHECK
    if (m_index<m_size)
      _check();
#endif
  }
  bool hasNext() const { return m_index<m_size; }

  ComponentCell operator*() const
  {
    return ComponentCell(m_items[m_index]);
  }

  Integer index() const { return m_index; }
  MatVarIndex _varIndex() const { return m_matvar_indexes[m_index]; }

  operator ComponentItemLocalId() const
  {
    return ComponentItemLocalId(m_matvar_indexes[m_index]);
  }

 protected:

  Int32 _varArrayIndex() const { return m_matvar_indexes[m_index].arrayIndex(); }
  Int32 _varValueIndex() const { return m_matvar_indexes[m_index].valueIndex(); }

 protected:
  void _check() const
  {
    ComponentItemInternal* ii = m_items[m_index];
    MatVarIndex mvi = ii->variableIndex();
    Int32 i_var_array_index = mvi.arrayIndex();
    Int32 mv_array_index = _varArrayIndex();
    if (i_var_array_index!=mv_array_index)
      ARCANE_FATAL("Bad 'var_array_index' in ComponentCell matvar='{0}' registered='{1}' index={2}",
                   mvi,m_matvar_indexes[m_index],m_index);
    Int32 i_var_value_index = mvi.valueIndex();
    Int32 mv_value_index = _varValueIndex();
    if (i_var_value_index!=mv_value_index)
      ARCANE_FATAL("Bad 'var_value_index' for ComponentCell matvar='{0}' registered='{1}' index={2}",
                   mvi,m_matvar_indexes[m_index],m_index);
  }
 protected:
  Integer m_index;
  Integer m_size;
  ConstArrayView<ComponentItemInternal*> m_items;
  ConstArrayView<MatVarIndex> m_matvar_indexes;
  IMeshComponent* m_component;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur une partie des mailles d'un seul matériau.
 */
class ARCANE_MATERIALS_EXPORT MatCellEnumerator
: public ComponentCellEnumerator
{
 protected:
  MatCellEnumerator(ConstArrayView<ComponentItemInternal*> items,
                    ConstArrayView<MatVarIndex> matvar_indexes,
                    IMeshComponent* component)
  : ComponentCellEnumerator(items,matvar_indexes,component)
  {
  }
 public:
  static MatCellEnumerator create(IMeshMaterial* mat);
  static MatCellEnumerator create(const MatCellVector& miv);
  static MatCellEnumerator create(MatItemVectorView v);
 public:

  MatCell operator*() const
  {
#ifdef ARCANE_CHECK
    _check();
#endif
    return MatCell(m_items[m_index]);
  }

 private:
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les mailles d'un milieu
 */
class ARCANE_MATERIALS_EXPORT EnvCellEnumerator
: public ComponentCellEnumerator
{
 protected:
  EnvCellEnumerator(ConstArrayView<ComponentItemInternal*> items,
                    ConstArrayView<MatVarIndex> matvar_indexes,
                    IMeshComponent* component)
  : ComponentCellEnumerator(items,matvar_indexes,component)
  {
  }
 public:
  static EnvCellEnumerator create(IMeshEnvironment* mat);
  static EnvCellEnumerator create(const EnvCellVector& miv);
  static EnvCellEnumerator create(EnvItemVectorView v);
 public:
  EnvCell operator*() const
  {
#ifdef ARCANE_CHECK
    _check();
#endif
    return EnvCell(m_items[m_index]);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur une sous-partie (pure ou partielle) d'un
 * sous-ensemble des mailles d'un composant (matériau ou milieu)
 */
class ARCANE_MATERIALS_EXPORT ComponentPartCellEnumerator
{
 protected:
  ComponentPartCellEnumerator(IMeshComponent* component,Int32 var_idx,
                              Int32ConstArrayView value_indexes,
                              Int32ConstArrayView item_indexes,
                              ConstArrayView<ComponentItemInternal*> items_internal,
                              Int32 base_index)
  : m_index(0), m_size(item_indexes.size()), m_var_idx(var_idx), m_base_index(base_index),
    m_value_indexes(value_indexes), m_item_indexes(item_indexes),
    m_items_internal(items_internal), m_component(component)
  {
  }
  ComponentPartCellEnumerator(const ComponentPartItemVectorView& view,Integer base_index);
 public:
  static ComponentPartCellEnumerator create(ComponentPartItemVectorView v);
  static ComponentPartCellEnumerator create(IMeshComponent* c,eMatPart part);
 public:
  void operator++()
  {
    ++m_index;
  }
  bool hasNext() const { return m_index<m_size; }

  MatVarIndex _varIndex() const { return MatVarIndex(m_var_idx,m_value_indexes[m_index]); }

  operator ComponentItemLocalId() const
  {
    return ComponentItemLocalId(_varIndex());
  }

  ComponentCell operator*() const
  {
    return ComponentCell(m_items_internal[m_item_indexes[m_index]]);
  }

 protected:

  Integer m_index;
  Integer m_size;
  Integer m_var_idx;
  Integer m_base_index;
  Int32ConstArrayView m_value_indexes;
  Int32ConstArrayView m_item_indexes;
  ConstArrayView<ComponentItemInternal*> m_items_internal;
  IMeshComponent* m_component;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur les entités pures ou impures d'un matériau.
 */
class ARCANE_MATERIALS_EXPORT MatPartCellEnumerator
: public ComponentPartCellEnumerator
{
 public:
  MatPartCellEnumerator(const MatPartItemVectorView& v);
 public:
  static MatPartCellEnumerator create(MatPartItemVectorView v);
  static MatPartCellEnumerator create(IMeshMaterial* mat,eMatPart part);
 public:

  MatCell operator*() const
  {
    return MatCell(m_items_internal[m_item_indexes[m_index]]);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les entités pures ou impures d'un milieu.
 */
class ARCANE_MATERIALS_EXPORT  EnvPartCellEnumerator
: public ComponentPartCellEnumerator
{
 public:
  EnvPartCellEnumerator(const EnvPartItemVectorView& v);
 public:
  static EnvPartCellEnumerator create(EnvPartItemVectorView v);
  static EnvPartCellEnumerator create(IMeshEnvironment* env,eMatPart part);
 public:

  EnvCell operator*() const
  {
    return EnvCell(m_items_internal[m_item_indexes[m_index]]);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les matériaux d'une maille.
 */
class ARCANE_MATERIALS_EXPORT CellComponentCellEnumerator
{
  friend class EnumeratorTracer;
 public:
  CellComponentCellEnumerator(ComponentItemInternal* items_begin,Integer nb_mat)
  : m_index(0), m_size(nb_mat), m_items_begin(items_begin)
  {
  }
  CellComponentCellEnumerator(ComponentItemInternal* super_item)
  : m_index(0), m_size(super_item->nbSubItem()), m_items_begin(super_item->firstSubItem())
  {
  }
 public:
  void operator++() { ++m_index; }
  bool hasNext() const { return m_index<m_size; }

  ComponentCell operator*() const
  {
    ARCANE_CHECK_AT(m_index,m_size);
    return ComponentCell(m_items_begin+m_index);
  }
  MatVarIndex _varIndex() const { return m_items_begin[m_index].variableIndex(); }
  Integer index() const { return m_index; }
  operator ComponentItemLocalId() const
  {
    return ComponentItemLocalId(m_items_begin[m_index].variableIndex());
  }
 protected:
  Integer m_index;
  Integer m_size;
  ComponentItemInternal* m_items_begin;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur typés sur les mailles composants d'une maille.
 */
template <typename ComponentCellType> class CellComponentCellEnumeratorT
: public CellComponentCellEnumerator
{
 public:

  CellComponentCellEnumeratorT(ComponentItemInternal* items_begin,Integer nb_mat)
  : CellComponentCellEnumerator(items_begin,nb_mat){}
  CellComponentCellEnumeratorT(ComponentItemInternal* super_item)
  : CellComponentCellEnumerator(super_item){}

 public:

  ComponentCellType operator*() const
  {
    ARCANE_CHECK_AT(m_index,m_size);
    return ComponentCellType(m_items_begin+m_index);
  }
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les mailles d'un milieu
 */
class ARCANE_MATERIALS_EXPORT CellGenericEnumerator
{
 public:
  static EnvCellEnumerator create(IMeshEnvironment* env);
  static EnvCellEnumerator create(const EnvCellVector& ecv);
  static EnvCellEnumerator create(EnvItemVectorView v);

  static MatCellEnumerator create(IMeshMaterial* mat);
  static MatCellEnumerator create(const MatCellVector& miv);
  static MatCellEnumerator create(MatItemVectorView v);

  static CellEnumerator create(CellVectorView v);
  static CellEnumerator create(const CellGroup& v);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les mailles matériaux d'une maille.
 */
typedef CellComponentCellEnumeratorT<MatCell> CellMatCellEnumerator;
/*!
 * \brief Enumérateur sur les mailles milieux d'une maille.
 */
typedef CellComponentCellEnumeratorT<EnvCell> CellEnvCellEnumerator;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur les mailles milieux
 */
class ARCANE_MATERIALS_EXPORT AllEnvCellEnumerator
{
  friend class EnumeratorTracer;
 protected:
  AllEnvCellEnumerator(AllEnvCellVectorView items)
  : m_index(0), m_size(items.size()), m_items(items) { }
 public:
  static AllEnvCellEnumerator create(AllEnvCellVectorView items);
  static AllEnvCellEnumerator create(IMeshMaterialMng* mng,const CellGroup& group);
  static AllEnvCellEnumerator create(IMeshMaterialMng* mng,const CellVectorView& view);
  static AllEnvCellEnumerator create(IMeshBlock* block);
 public:
  void operator++() { ++m_index; }
  bool hasNext() const { return m_index<m_size; }
  AllEnvCell operator*() { return m_items[m_index]; }
  Integer index() const { return m_index; }
 public:
  Integer m_index;
  Integer m_size;
  AllEnvCellVectorView m_items;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur des composants
 */
class ARCANE_MATERIALS_EXPORT ComponentEnumerator
{
  friend class EnumeratorTracer;
 public:

  ComponentEnumerator(ConstArrayView<IMeshComponent*> components);

 public:

  bool hasNext() const { return m_index<m_size; }
  void operator++() { ++m_index; }
  IMeshComponent* operator*() const { return m_components[m_index]; }

 private:

  ConstArrayView<IMeshComponent*> m_components;
  Integer m_index;
  Integer m_size;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur des matériaux
 */
class ARCANE_MATERIALS_EXPORT MatEnumerator
{
 public:

  MatEnumerator(IMeshMaterialMng* mng);
  MatEnumerator(IMeshEnvironment* env);
  MatEnumerator(ConstArrayView<IMeshMaterial*> mats);

 public:

  bool hasNext() const { return m_index<m_size; }
  void operator++() { ++m_index; }
  IMeshMaterial* operator*() const { return m_mats[m_index]; }

 private:

  ConstArrayView<IMeshMaterial*> m_mats;
  Integer m_index;
  Integer m_size;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Enumérateur sur des milieux
 */
class ARCANE_MATERIALS_EXPORT EnvEnumerator
{
 public:

  EnvEnumerator(IMeshMaterialMng* mng);
  EnvEnumerator(IMeshBlock* block);
  EnvEnumerator(ConstArrayView<IMeshEnvironment*> envs);

 public:

  bool hasNext() const { return m_index<m_size; }
  void operator++() { ++m_index; }
  IMeshEnvironment* operator*() const { return m_envs[m_index]; }

 private:

  ConstArrayView<IMeshEnvironment*> m_envs;
  Integer m_index;
  Integer m_size;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#if defined(ARCANE_TRACE_ENUMERATOR)
#define A_TRACE_COMPONENT(_EnumeratorClassName) \
  ::Arcane::EnumeratorTraceWrapper< ::Arcane::Materials::_EnumeratorClassName, ::Arcane::Materials::IEnumeratorTracer >
#else
#define A_TRACE_COMPONENT(_EnumeratorClassName) \
  ::Arcane::Materials::_EnumeratorClassName
#endif

#define A_ENUMERATE_COMPONENTCELL(_EnumeratorClassName,iname,...) \
  for( A_TRACE_COMPONENT(_EnumeratorClassName) iname(Arcane::Materials::_EnumeratorClassName::create(__VA_ARGS__) A_TRACE_ENUMERATOR_WHERE); iname.hasNext(); ++iname )

#define A_ENUMERATE_COMPONENT(_EnumeratorClassName,iname,container) \
  for( A_TRACE_COMPONENT(_EnumeratorClassName) iname((::Arcane::Materials::_EnumeratorClassName)(container) A_TRACE_ENUMERATOR_WHERE); iname.hasNext(); ++iname )

#define A_ENUMERATE_CELL_COMPONENTCELL(_EnumeratorClassName,iname,component_cell) \
  for( A_TRACE_COMPONENT(_EnumeratorClassName) iname(component_cell.internal() A_TRACE_ENUMERATOR_WHERE); iname.hasNext(); ++iname )

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*!
 * \brief Macro générique pour itérer sur les entités d'un matériau ou d'un milieu.
 *
 * \param enumerate_class_name nom de la classe de l'énumérateur
 * \param iname nom de la variable contenant l'itérateur
 * \param ...  Les arguments supplémentaires sont passés à la méthode statique create() de
 * la classe de l'énumérateur.
 */
#define ENUMERATE_COMPONENTITEM(enumerator_class_name,iname,...)  \
  A_ENUMERATE_COMPONENTCELL(enumerator_class_name##Enumerator,iname,__VA_ARGS__)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*!
 * \brief Macro pour itérer sur toutes les mailles AllEnvCell d'un groupe.
 *
 * Il existe deux possibilités pour utiliser cette macro. La première
 * est obsolète et utilise la méthode IMeshMaterialMng::view(). La seconde
 * utilise trois arguments:
 * \param iname nom de l'itérateur, de type \a AllEnvCellEnumerator.
 * \param matmng gestionnaire de materiaux de type \a IMeshMaterialMng.
 * \param igroup groupe de maille, de type \a CellGroup.
 */
#define ENUMERATE_ALLENVCELL(iname,...) \
  for( A_TRACE_COMPONENT(AllEnvCellEnumerator) iname( ::Arcane::Materials::AllEnvCellEnumerator::create(__VA_ARGS__) ); iname.hasNext(); ++iname )

/*!
 * \brief Macro pour itérer sur toutes les mailles d'un matériau.
 *
 * \param iname nom de l'itérateur, de type MatCellEnumerator.
 * \param mat matériau, de type IMeshMaterial*
 */
#define ENUMERATE_MATCELL(iname,mat) \
  A_ENUMERATE_COMPONENTCELL(MatCellEnumerator,iname,mat)

/*!
 * \brief Macro pour itérer sur toutes les mailles d'un milieu.
 *
 * \param iname nom de l'itérateur, de type EnvCellEnumerator.
 * \param env milieu, de type IMeshEnvironment*
 */
#define ENUMERATE_ENVCELL(iname,env) \
  A_ENUMERATE_COMPONENTCELL(EnvCellEnumerator,iname,env)

/*!
 * \brief Macro pour itérer sur toutes les mailles d'un composant.
 *
 * \param iname nom de l'itérateur, de type EnvCellEnumerator.
 * \param component composant, de type IMeshComponent*
 */
#define ENUMERATE_COMPONENTCELL(iname,component) \
  A_ENUMERATE_COMPONENTCELL(ComponentCellEnumerator,iname,component)

/*!
 * \brief Macro pour itérer sur une liste de composants
 *
 * \a icomponent nom de l'itérateur.
 * \a container peut être un objet du type suivant:
 * - ConstArrayView<IMeshComponent*> pour itérer sur une liste spécifique de composants
 */
#define ENUMERATE_COMPONENT(icomponent,container) \
  A_ENUMERATE_COMPONENT(ComponentEnumerator,icomponent,container)

/*!
 * \brief Macro pour itérer sur une liste de matériaux
 *
 * \a imat nom de l'itérateur.
 * \a container peut être un objet du type suivant:
 * - IMeshMaterialMng* pour itérer sur tous les matériaux
 * - IMeshEnvironment* pour itérer sur tous les matériaux d'un milieu
 * - ConstArrayView<IMeshMaterial*> pour itérer sur une liste spécifique de matériaux
 */
#define ENUMERATE_MAT(imat,container) \
  A_ENUMERATE_COMPONENT(MatEnumerator,imat,container)

/*!
 * \brief Macro pour itérer sur une liste de milieux.
 *
 * \a ienv est le nom de l'énumérateur.
 * \a container peut être un objet du type suivant:
 * - IMeshMaterialMng* pour itérer sur tous les milieux
 * - IMeshBlock* pour itérer sur tous les milieux du bloc
 * - ConstArrayView<IMeshEnvironment*> pour itérer sur une liste spécifique de milieux.
 */
#define ENUMERATE_ENV(ienv,container) \
  A_ENUMERATE_COMPONENT(EnvEnumerator,ienv,container)

/*!
 * \brief Macro pour itérer sur tous les composants d'une maille.
 *
 * \param iname nom de l'itérateur, de type CellComponentCellEnumerator.
 * \param component_cell maille composant, de type ComponentCell.
 */
#define ENUMERATE_CELL_COMPONENTCELL(iname,component_cell) \
  A_ENUMERATE_CELL_COMPONENTCELL(CellComponentCellEnumerator,iname,component_cell)

/*!
 * \brief Macro pour itérer sur tous les matériaux d'une maille.
 *
 * \param iname nom de l'itérateur, de type CellMatCellEnumerator.
 * \param env_cell maille milieu, de type EnvCell.
 */
#define ENUMERATE_CELL_MATCELL(iname,env_cell) \
  A_ENUMERATE_CELL_COMPONENTCELL(CellMatCellEnumerator,iname,env_cell)

/*!
 * \brief Macro pour itérer sur tous les milieux d'une maille.
 *
 * \param iname nom de l'itérateur, de type CellEnvCellEnumerator.
 * \param all_env_cell maille avec infos sur les milieux, de type AllEnvCell.
 */
#define ENUMERATE_CELL_ENVCELL(iname,all_env_cell) \
  A_ENUMERATE_CELL_COMPONENTCELL(CellEnvCellEnumerator,iname,all_env_cell)

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*!
 * \brief Macro pour itérer de manière générique sur les matériaux,
 * milieux ou les mailles
 *
 * \param iname nom de l'itérateur, de type CellEnvCellEnumerator.
 * \param mat_or_env_or_group un objet qui peut être passé en argument de
 * ENUMERATE_CELL, ENUMERATE_MATCELL ou ENUMERATE_ENVCELL.
 */
#define ENUMERATE_GENERIC_CELL(iname,mat_or_env_or_group) \
  for( auto iname = ::Arcane::Materials::CellGenericEnumerator::create(mat_or_env_or_group); iname.hasNext(); ++iname )

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MATERIALS_END_NAMESPACE
ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
