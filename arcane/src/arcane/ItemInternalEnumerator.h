﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* ItemInternalEnumerator.h                                    (C) 2000-2018 */
/*                                                                           */
/* Enumérateur sur une liste de ItemInternal.                                */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_ITEMINTERNALENUMERATOR_H
#define ARCANE_ITEMINTERNALENUMERATOR_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/ItemInternalVectorView.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class ItemInternal;
class ItemEnumerator;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Enumérateur sur une liste d'entités.
 */
class ItemInternalEnumerator
{
 private:

  typedef ItemInternal* ItemInternalPtr;
  friend class ItemEnumerator;

 public:

  ItemInternalEnumerator(const ItemInternalPtr* items,const Int32* local_ids,Integer n)
  : m_items(items), m_local_ids(local_ids), m_index(0), m_count(n) {}
  ItemInternalEnumerator(const ItemInternalVectorView& view)
  : m_items(view.items().data()), m_local_ids(view.localIds().data()), m_index(0), m_count(view.size()) {}
  ItemInternalEnumerator(const ItemInternalArrayView& items,const Int32ConstArrayView& local_ids)
  : m_items(items.data()), m_local_ids(local_ids.data()), m_index(0), m_count(local_ids.size()) {}

 public:

  ItemInternal* operator*() const { return m_items[ m_local_ids[m_index] ]; }
  ItemInternal* operator->() const { return m_items[ m_local_ids[m_index] ]; }
  inline void operator++() { ++m_index; }
  inline bool operator()() { return m_index<m_count; }
  inline bool hasNext() { return m_index<m_count; }

  //! Nombre d'éléments de l'énumérateur
  inline Integer count() const { return m_count; }

  //! Indice courant de l'énumérateur
  inline Integer index() const { return m_index; }

  //! localId() de l'entité courante.
  inline Integer localId() const { return m_local_ids[m_index]; }

 protected:

  const ItemInternalPtr* m_items;
  const Int32* ARCANE_RESTRICT m_local_ids;
  Integer m_index;
  Integer m_count;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif
