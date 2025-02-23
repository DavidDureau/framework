﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
// Wrapper pour les classes ItemPairEnumerator.

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
 
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

%typemap(csinterfaces) Arcane::ItemPairEnumerator "";
%typemap(csclassmodifiers) Arcane::ItemPairEnumerator "public unsafe struct"
%typemap(csattributes) Arcane::ItemPairEnumerator "[StructLayout(LayoutKind.Sequential)]"
%typemap(csbody) Arcane::ItemPairEnumerator %{ %}
%typemap(SWIG_DISPOSING, methodname="Dispose", methodmodifiers="private") Arcane::ItemPairEnumerator ""
%typemap(SWIG_DISPOSE, methodname="Dispose", methodmodifiers="private") Arcane::ItemPairEnumerator ""
%typemap(ctype) Arcane::ItemPairEnumerator %{Arcane::ItemPairEnumerator%}
%typemap(csin) Arcane::ItemPairEnumerator "$csinput"
%typemap(csout) Arcane::ItemPairEnumerator { return $imcall;  }
%typemap(imtype) Arcane::ItemPairEnumerator "Arcane.ItemPairEnumerator"
%typemap(out) Arcane::ItemPairEnumerator %{ $result = ($1); %}
%typemap(in) Arcane::ItemPairEnumerator %{$1 = $input; %}

%typemap(cscode) Arcane::ItemPairEnumerator
%{
  internal Integer m_current;
  internal Integer m_end;
  internal IntegerConstArrayView m_indexes;
  internal Int32ConstArrayView m_items_local_id;
  internal Int32ConstArrayView m_sub_items_local_id;
  internal ItemInternalList m_items_internal;
  internal ItemInternalList m_sub_items_internal;
  
  public Item Current
  {
    get { return new Item(m_items_internal.m_ptr[m_current]); }
  }
  public void Reset()
  {
    m_current = -1;
  }
  public bool MoveNext()
  {
    ++m_current;
    return m_current < m_end;
  }
%}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

%typemap(cscode) Arcane::ItemPairGroup %{
  public ItemPairEnumerator GetEnumerator()
  {
    ItemPairEnumerator e = _enumerator();
    e.Reset();
    return e;
  }
%}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{
  class ItemPairEnumerator
  {
    ItemPairEnumerator(const ItemPairGroup&);
  };
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

%include arcane/ItemPairGroup.h
%include arcane/ItemPairGroupBuilder.h

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

%template(ItemPairGroupCustomFunctor) Arcane::IFunctorWithArgumentT<ItemPairGroupBuilder&>;
