﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* VariableUnitTest.cc                                         (C) 2000-2021 */
/*                                                                           */
/* Service de test des variables.                                            */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/ValueChecker.h"
#include "arcane/utils/Event.h"
#include "arcane/utils/ITraceMng.h"

#include "arcane/BasicUnitTest.h"
#include "arcane/IMesh.h"
#include "arcane/IVariableMng.h"
#include "arcane/VariableStatusChangedEventArgs.h"
#include "arcane/VariableView.h"

#include "arcane/tests/ArcaneTestGlobal.h"
#include "arcane/tests/StdScalarMeshVariables.h"
#include "arcane/tests/StdArrayMeshVariables.h"
#include "arcane/tests/StdScalarVariables.h"
#include "arcane/tests/VariableUnitTest_axl.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANETEST_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

using namespace Arcane;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Module de test des variables
 */
class VariableUnitTest
: public ArcaneVariableUnitTestObject
{
  class EventTester
  {
   public:
    EventTester(ISubDomain* sd)
    : m_trace_mng(sd->traceMng()), m_nb_added(0), m_nb_removed(0)
    {
      // TODO: Ajouter EventObserverPool pour gerer la destruction des evenements
      IVariableMng* vm = sd->variableMng();
      auto f1 = [&](const VariableStatusChangedEventArgs& e){
        m_trace_mng->debug() << "** ** ADD VARIABLE name=" << e.variable()->fullName();
        ++m_nb_added;
      };
      auto f2 = [&](const VariableStatusChangedEventArgs& e){
        m_trace_mng->debug() << "** ** REMOVE VARIABLE name=" << e.variable()->fullName();
        ++m_nb_removed;
      };

      vm->onVariableAdded().attach(m_observer_pool,f1);
      vm->onVariableRemoved().attach(m_observer_pool,f2);
    }
    ~EventTester()
    {
      m_trace_mng->info() << "** ** NB_ADDED=" << m_nb_added << " NB_REMOVED=" << m_nb_removed;
    }
   private:
    ITraceMng* m_trace_mng;
    Integer m_nb_added;
    Integer m_nb_removed;
    EventObserverPool m_observer_pool;
  };
 public:

  VariableUnitTest(const ServiceBuildInfo& cb);
  ~VariableUnitTest();

 public:

  virtual void initializeTest();
  virtual void executeTest();

 private:

  EventTester m_event_tester;
  StdScalarVariables m_scalars;
  StdScalarMeshVariables<Cell> m_cells;
  StdArrayMeshVariables<Cell> m_array_cells;

 private:

  void _testReferences(Integer nb_ref);
  void _testUsed();
  void _testRefersTo();
  void _testSimpleView();
  void _checkException(Integer i);
  void _testAlignment();
  void _testSwap();

  template<typename MeshVarType>
  void _testSwapHelper(MeshVarType& cells);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_REGISTER_SERVICE_VARIABLEUNITTEST(VariableUnitTest,VariableUnitTest);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

VariableUnitTest::
VariableUnitTest(const ServiceBuildInfo& mb)
: ArcaneVariableUnitTestObject(mb)
, m_event_tester(mb.subDomain())
, m_scalars(mb.meshHandle(),"TestParallelScalars")
, m_cells(mb.meshHandle(),"TestParallelCells")
, m_array_cells(mb.meshHandle(),"TestArrayCells")
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

VariableUnitTest::
~VariableUnitTest()
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void VariableUnitTest::
executeTest()
{
  _testRefersTo();
  _testSimpleView();
  _testUsed();
  _testSwap();
  _testAlignment();
  _testReferences(options()->nbReference());
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void VariableUnitTest::
initializeTest()
{
  info() << "INITIALIZE TEST";
  m_array_cells.initialize();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void VariableUnitTest::
_checkException(Integer i)
{
  if (i>15)
    throw Exception("MyException",A_FUNCINFO,StackTrace("NoStack"));
  if (i>10)
    throw FatalErrorException(A_FUNCINFO,"Bad value");
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void VariableUnitTest::
_testReferences(Integer nb_ref)
{
  {
    Integer nb_exception = 0;
    for(Integer i=0; i<2500; ++i ){
      try{
        _checkException(i);
      }
      catch(Exception & e){
        ++nb_exception;
      }
    }
    info() << "NB_EXCEPTION=" << nb_exception;
  }
  Integer nb_var = 150;
  UniqueArray<VariableRef*>  vars;
  vars.reserve(nb_ref*nb_var);
  IMesh* mesh = subDomain()->defaultMesh();
  info() << "TEST VARIABLE REFERENCE NB=" << nb_ref;
  UniqueArray<String> vars_name(nb_var);
  for( Integer z=0; z<nb_var; ++z )
    vars_name[z] = String("TestVar")+z;

  for( Integer i=0; i<nb_ref; ++i ){
    for( Integer z=0; z<nb_var; ++z )
      vars.add(new VariableCellReal(VariableBuildInfo(mesh,vars_name[z])));
  }

  for( Integer i=0, is=vars.size(); i<is; ++i )
    delete vars[i];
  vars.clear();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*
 * \brief Teste la methode setUsed sur une variable.
 */
void VariableUnitTest::
_testUsed()
{
  info() << "Test VariableRef::setUsed()";
  VariableCellReal tmp_var(VariableBuildInfo(mesh(),"CellRealTest"));
  IVariable* ivar = tmp_var.variable();
  info() << "family=" << ivar->itemFamilyName() << " group=" << ivar->itemGroupName();
  for( Integer i=0; i<10; ++i ){
    ENUMERATE_CELL(icell,allCells()){
      tmp_var[icell] = 1.0;
    }

    tmp_var.setUsed(false);

    tmp_var.setUsed(true);
    
    ENUMERATE_CELL(icell,allCells()){
      tmp_var[icell] = 2.0;
    }
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*
 * \brief Teste la méthode VariableRef::refersTo().
 */
void VariableUnitTest::
_testRefersTo()
{
  info() << "Test " << A_FUNCINFO;

  ValueChecker vc(A_FUNCINFO);

  // Teste refersTo() pour les variables 0D sur les entités du maillage
  {
    VariableCellReal var1(VariableBuildInfo(mesh(),"CellRealTest1"));
    VariableCellReal var2(VariableBuildInfo(mesh(),"CellRealTest2"));
    var2.fill(3.0);

    var1.refersTo(var2);

    // Vérifie que ce sont les mêmes variables avec les mêmes valeurs.
    vc.areEqual(var1.variable(),var2.variable(),"Bad refersTo()");
    vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values");
  }

  // Teste refersTo() pour les variables 1D sur les entités du maillage
  {
    VariableCellArrayReal var1(VariableBuildInfo(mesh(),"CellRealTest1"));
    VariableCellArrayReal var1_bis(VariableBuildInfo(mesh(),"CellRealTest1"));
    var1.resize(5);
    var1.fill(4.2);
    VariableCellArrayReal var2(VariableBuildInfo(mesh(),"CellRealTest2"));
    var2.resize(3);
    var1.fill(7.5);

    var1.refersTo(var2);

    // Vérifie que ce sont les mêmes variables.
    vc.areEqual(var1.variable(),var2.variable(),"Bad refersTo() for Array");
    vc.areEqual(var1.arraySize(),3,"Bad size (2)");
    //vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values");
  }

  // Teste refersTo() pour les variables 2D
  {
    const Real fill_value = 4.2;
    const Real fill_value2 = fill_value+1.0;
    VariableArray2Real var1(VariableBuildInfo(mesh(),"Array2RealTest1"));
    VariableArray2Real var1_bis(VariableBuildInfo(mesh(),"Array2RealTest1"));
    var1.resize(2,5);
    var1.fill(fill_value);
    VariableArray2Real var2(VariableBuildInfo(mesh(),"Array2RealTest2"));
    var2.resize(12,3);
    var1.fill(fill_value2);

    for( Integer i=0, n1=var1.dim1Size(); i<n1; ++i )
      for( Integer j=0, n2=var1.dim2Size(); j<n2; ++j )
        vc.areEqual(var1[i][j],fill_value2,"Array2RealCompare");

    var1.refersTo(var2);

    // Vérifie que ce sont les mêmes variables.
    vc.areEqual(var1.variable(),var2.variable(),"Bad refersTo() for VariableArray2Real");
    vc.areEqual(var1.arraySize(),12,"Bad size (3)");
    Span2<const Real> var1_view(var1);
    Span2<const Real> var2_view(var2);
    vc.areEqualArray(var1_view,var2_view,"Bad values");
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*
 * \brief Teste un accès simple aux vues.
 */
void VariableUnitTest::
_testSimpleView()
{
  ValueChecker vc(A_FUNCINFO);

  VariableCellReal var1(VariableBuildInfo(mesh(),"CellRealTest1"));
  VariableCellReal var2(VariableBuildInfo(mesh(),"CellRealTest2"));
  {
    info() << A_FUNCINFO << " Test simple view In/Out";
    ENUMERATE_CELL(icell,allCells()){
      var1[icell] = icell.itemLocalId()+1;
    }
    auto v1 = viewIn(var1);
    // TODO: pouvoir tester que le code suivante ne compile pas
    // auto v1 = viewOut(var1);
    auto v2 = viewOut(var2);
    ENUMERATE_CELL(icell,allCells()){
      v2[icell] = v1[icell];
    }
    vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values (1)");
  }

  {
    info() << A_FUNCINFO << " Test simple view InOut/Out";
    ENUMERATE_CELL(icell,allCells()){
      var1[icell] = icell.itemLocalId()+2;
    }
    auto v1 = viewInOut(var1);
    auto v2 = viewOut(var2);
    ENUMERATE_CELL(icell,allCells()){
      v2[icell] = v1[icell];
    }
    vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values (2)");
  }

  {
    info() << A_FUNCINFO << " Test simple view InOut/InOut";
    ENUMERATE_CELL(icell,allCells()){
      var1[icell] = icell.itemLocalId()+3;
    }
    auto v1 = viewInOut(var1);
    auto v2 = viewInOut(var2);
    ENUMERATE_CELL(icell,allCells()){
      v2[icell] = v1[icell];
    }
    vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values (3)");
  }

  {
    info() << A_FUNCINFO << " Test simple view InOut/InOut";
    ENUMERATE_CELL(icell,allCells()){
      var1[icell] = icell.itemLocalId()+4;
    }
    auto v1 = viewInOut(var1);
    auto v2 = viewOut(var2);
    ENUMERATE_CELL(icell,allCells()){
      v2[icell] = v1[icell];
    }
    vc.areEqualArray(var1.asArray().constView(),var2.asArray().constView(),"Bad values (4)");
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename MeshVarType> void
VariableUnitTest::
_testSwapHelper(MeshVarType& cells)
{
  ItemGroup all_cells = mesh()->allCells();

  cells.initialize();
  cells.setValues(25,all_cells);
  {
    MeshVarType cells2(mesh()->handle(),"TestParallelCells2");
    cells2.initialize();
    cells2.setValues(37,all_cells);

    cells.m_real.swapValues(cells2.m_real);
    cells.m_byte.swapValues(cells2.m_byte);
    cells.m_int16.swapValues(cells2.m_int16);
    cells.m_int32.swapValues(cells2.m_int32);
    cells.m_int64.swapValues(cells2.m_int64);
    cells.m_real2.swapValues(cells2.m_real2);
    cells.m_real2x2.swapValues(cells2.m_real2x2);
    cells.m_real3.swapValues(cells2.m_real3);
    cells.m_real3x3.swapValues(cells2.m_real3x3);

    // Vérifie les valeurs
    {
      Integer nb_error = 0;

      nb_error += cells.checkValues(37,all_cells);
      info() << "NB ERROR1 = " << nb_error;

      nb_error += cells2.checkValues(25,all_cells);
      info() << "NB ERROR2 = " << nb_error;

      if (nb_error!=0)
        fatal() << "Error in variable swapping (1) n=" << nb_error;
    }
  }
  // Vérifie que si la variable de recopie est libérée tout est encore OK.
  {
    Integer nb_error = 0;

    nb_error += cells.checkValues(37,all_cells);
    info() << "NB ERROR3 = " << nb_error;

    if (nb_error!=0)
      fatal() << "Error in variable swapping (2) n=" << nb_error;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void VariableUnitTest::
_testSwap()
{
  info() << "Test VariableRef::swap()";
  m_array_cells.initialize();
  VariableCellReal v2(VariableBuildInfo(mesh(),"T1"));

  ItemGroup all_cells = mesh()->allCells();
  info() << "Test mesh scalar";
  _testSwapHelper(m_cells);
  info() << "Test mesh array";
  _testSwapHelper(m_array_cells);

  info() << "Test scalar values";
  m_scalars.setValues(25);
  {
    StdScalarVariables scalars2(mesh()->handle(),"TestParallelScalars2");
    scalars2.setValues(37);

    m_scalars.m_real.swapValues(scalars2.m_real);
    m_scalars.m_byte.swapValues(scalars2.m_byte);
    m_scalars.m_int16.swapValues(scalars2.m_int16);
    m_scalars.m_int32.swapValues(scalars2.m_int32);
    m_scalars.m_int64.swapValues(scalars2.m_int64);
    m_scalars.m_real2.swapValues(scalars2.m_real2);
    m_scalars.m_real2x2.swapValues(scalars2.m_real2x2);
    m_scalars.m_real3.swapValues(scalars2.m_real3);
    m_scalars.m_real3x3.swapValues(scalars2.m_real3x3);

    // Vérifie les valeurs
    {
      Integer nb_error = 0;

      nb_error += m_scalars.checkValues(37);
      info() << "NB ERROR1 = " << nb_error;

      nb_error += scalars2.checkValues(25);
      info() << "NB ERROR2 = " << nb_error;

      if (nb_error!=0)
        fatal() << "Error in variable swapping (1) n=" << nb_error;
    }
  }
  // Vérifie que si la variable de recopie est libérée tout est encore OK.
  {
    Integer nb_error = 0;

    nb_error += m_scalars.checkValues(37);
    info() << "NB ERROR3 = " << nb_error;

    if (nb_error!=0)
      fatal() << "Error in variable swapping (2) n=" << nb_error;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class AlignmentChecker
: public TraceAccessor
{
 public:
  AlignmentChecker(ITraceMng* tm)
  : TraceAccessor(tm), nb_error(0),
    align_size(AlignedMemoryAllocator::simdAlignment())
  {
  }
 public:
  // Pour les variables 1D
  template<typename ItemType,typename DataType>
  void operator()(MeshVariableScalarRefT<ItemType,DataType>& var)
  {
    info() << "VAR1D=" << var.name() << " sizeofArray=" << sizeof(ArrayImplT<DataType>);
    void* ptr = var.asArray().unguardedBasePointer();
    intptr_t int_ptr = (intptr_t)ptr;
    intptr_t modulo = int_ptr % align_size;
    if (modulo != 0){
      ++nb_error;
      info() << "ERROR: Invalid alignment for variable '" << var.name() << "'"
             << " ptr=" << int_ptr << " modulo=" << modulo;
    }
  }
  // Pour les variables 2D
  template<typename ItemType,typename DataType>
  void operator()(MeshVariableArrayRefT<ItemType,DataType>& var)
  {
    info() << "VAR2D=" << var.name();
    void* ptr = var.asArray().unguardedBasePointer();
    intptr_t int_ptr = (intptr_t)ptr;
    intptr_t modulo = int_ptr % align_size;
    if (modulo != 0){
      ++nb_error;
      info() << "ERROR: Invalid alignment for variable '" << var.name() << "'"
             << " ptr=" << int_ptr << " modulo=" << modulo;
    }
  }
 public:
  Integer nb_error;
  const Integer align_size;
};

void VariableUnitTest::
_testAlignment()
{
  // Vérifie que les variables scalaires et tableaux sur les maillages.

  info() << "Testing alignment";
  m_array_cells.initialize();

  AlignmentChecker xad(traceMng());
  m_cells.applyFunctor(xad);
  m_array_cells.applyFunctor(xad);

  if (xad.nb_error!=0)
    fatal() << "Invalid alignment for variables nb_error=" << xad.nb_error;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANETEST_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
