﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* MpiParallelMng.cc                                           (C) 2000-2020 */
/*                                                                           */
/* Gestionnaire de parallélisme utilisant MPI.                               */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/String.h"
#include "arcane/utils/Collection.h"
#include "arcane/utils/Enumerator.h"
#include "arcane/utils/ScopedPtr.h"
#include "arcane/utils/PlatformUtils.h"
#include "arcane/utils/TimeoutException.h"
#include "arcane/utils/NotImplementedException.h"
#include "arcane/utils/OStringStream.h"
#include "arcane/utils/ArgumentException.h"
#include "arcane/utils/ITraceMng.h"
#include "arcane/utils/HPReal.h"

#include "arcane/IMesh.h"
#include "arcane/IIOMng.h"
#include "arcane/IVariable.h"
#include "arcane/VariableTypes.h"
#include "arcane/SerializeBuffer.h"
#include "arcane/Timer.h"
#include "arcane/IItemFamily.h"

#include "arcane/parallel/IStat.h"

#include "arcane/parallel/mpi/MpiParallelMng.h"
#include "arcane/parallel/mpi/MpiAdapter.h"
#include "arcane/parallel/mpi/MpiParallelDispatch.h"
#include "arcane/parallel/mpi/MpiSerializeMessageList.h"
#include "arcane/parallel/mpi/MpiTimerMng.h"
#include "arcane/parallel/mpi/MpiLock.h"
#include "arcane/parallel/mpi/MpiSerializeMessage.h"
#include "arcane/parallel/mpi/MpiParallelNonBlockingCollective.h"
#include "arcane/parallel/mpi/MpiVariableSynchronizeDispatcher.h"
#include "arcane/parallel/mpi/MpiDatatype.h"

#include "arcane/SerializeMessage.h"

#include "arcane/impl/GetVariablesValuesParallelOperation.h"
#include "arcane/impl/TransferValuesParallelOperation.h"
#include "arcane/impl/ParallelExchanger.h"
#include "arcane/impl/VariableSynchronizer.h"
#include "arcane/impl/ParallelTopology.h"
#include "arcane/impl/ParallelReplication.h"
#include "arcane/impl/SequentialParallelMng.h"

#include "arccore/message_passing_mpi/MpiMessagePassingMng.h"
#include "arccore/message_passing_mpi/MpiRequestList.h"
#include "arccore/message_passing_mpi/MpiSerializeDispatcher.h"
#include "arccore/message_passing/Dispatchers.h"
#include "arccore/message_passing/Messages.h"
#include "arccore/message_passing/SerializeMessageList.h"

//#define ARCANE_TRACE_MPI

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane
{
using namespace Arccore::MessagePassing::Mpi;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

extern "C++" IIOMng*
arcaneCreateIOMng(IParallelMng* psm);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MpiParallelMngBuildInfo::
MpiParallelMngBuildInfo(MPI_Comm comm)
: is_parallel(false)
, comm_rank(A_NULL_RANK)
, comm_nb_rank(0)
, stat(nullptr)
, trace_mng(nullptr)
, timer_mng(nullptr)
, thread_mng(nullptr)
, mpi_comm(comm)
, is_mpi_comm_owned(true)
, mpi_lock(nullptr)
, m_dispatchers(nullptr)
, m_message_passing_mng(nullptr)
{
  ::MPI_Comm_rank(comm,&comm_rank);
  ::MPI_Comm_size(comm,&comm_nb_rank);

  m_dispatchers = new MP::Dispatchers();
  MP::Mpi::MpiMessagePassingMng::BuildInfo bi(comm_rank,comm_nb_rank,m_dispatchers,mpi_comm);
  m_message_passing_mng = new MP::Mpi::MpiMessagePassingMng(bi);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MpiParallelMng::
MpiParallelMng(const MpiParallelMngBuildInfo& bi)
: ParallelMngDispatcher(ParallelMngDispatcherBuildInfo(bi.dispatchers(),bi.messagePassingMng()))
, m_trace(bi.trace_mng)
, m_thread_mng(bi.thread_mng)
, m_world_parallel_mng(bi.world_parallel_mng)
, m_io_mng(nullptr)
, m_timer_mng(bi.timer_mng)
, m_replication(new ParallelReplication())
, m_is_timer_owned(false)
, m_datatype_list(nullptr)
, m_adapter(nullptr)
, m_is_parallel(bi.is_parallel)
, m_comm_rank(bi.commRank())
, m_comm_size(bi.commSize())
, m_is_initialized(false)
, m_stat(bi.stat)
, m_communicator(bi.mpiComm())
, m_is_communicator_owned(bi.is_mpi_comm_owned)
, m_mpi_lock(bi.mpi_lock)
, m_non_blocking_collective(nullptr)
{
  if (!m_world_parallel_mng){
    m_trace->debug()<<"[MpiParallelMng] No m_world_parallel_mng found, reverting to ourselves!";
    m_world_parallel_mng = this;
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MpiParallelMng::
~MpiParallelMng()
{
  delete m_non_blocking_collective;
  m_sequential_parallel_mng.reset();
  if (m_is_communicator_owned){
    MpiLock::Section ls(m_mpi_lock);
    MPI_Comm_free(&m_communicator);
  }
  delete m_replication;
  delete m_io_mng;
  if (m_is_timer_owned)
    delete m_timer_mng;
  m_adapter->destroy();
  delete m_datatype_list;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
// Classe pour créer les différents dispatchers
class DispatchCreator
{
 public:
  DispatchCreator(ITraceMng* tm,IMessagePassingMng* mpm,MpiAdapter* adapter,MpiDatatypeList* datatype_list)
  : m_tm(tm), m_mpm(mpm), m_adapter(adapter), m_datatype_list(datatype_list){}
 public:
  template<typename DataType> MpiParallelDispatchT<DataType>*
  create()
  {
    MpiDatatype* dt = m_datatype_list->datatype(DataType());
    return new MpiParallelDispatchT<DataType>(m_tm,m_mpm,m_adapter,dt);
  }

  ITraceMng* m_tm;
  IMessagePassingMng* m_mpm;
  MpiAdapter* m_adapter;
  MpiDatatypeList* m_datatype_list;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

class ControlDispatcherDecorator
: public ParallelMngDispatcher::DefaultControlDispatcher
{
 public:

  ControlDispatcherDecorator(IParallelMng* pm, MpiAdapter* adapter)
  : ParallelMngDispatcher::DefaultControlDispatcher(pm), m_adapter(adapter) {}

  IMessagePassingMng* commSplit(bool keep) override
  {
    return m_adapter->commSplit(keep);
  }
  MP::IProfiler* profiler() const override { return m_adapter->profiler(); }
  void setProfiler(MP::IProfiler* p) override { m_adapter->setProfiler(p); }

 private:
  MpiAdapter* m_adapter;
};
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
build()
{
  ITraceMng* tm = traceMng();
  if (!m_timer_mng){
    m_timer_mng = new MpiTimerMng(tm);
    m_is_timer_owned = true;
  }

  // Créé le gestionnaire séquentiel associé.
  {
    SequentialParallelMngBuildInfo bi(timerMng(),worldParallelMng());
    bi.setTraceMng(traceMng());
    bi.setCommunicator(communicator());
    bi.setThreadMng(threadMng());
    m_sequential_parallel_mng = arcaneCreateSequentialParallelMngRef(bi);
  }

  // Indique que les reduces doivent être fait dans l'ordre des processeurs
  // afin de garantir une exécution déterministe
  bool is_ordered_reduce = false;
  if (platform::getEnvironmentVariable("ARCANE_ORDERED_REDUCE")=="TRUE")
    is_ordered_reduce = true;
  m_datatype_list = new MpiDatatypeList(is_ordered_reduce);

  ARCANE_CHECK_POINTER(m_stat);

  MpiAdapter* adapter = new MpiAdapter(m_trace,m_stat->toArccoreStat(),m_communicator,m_mpi_lock);
  m_adapter = adapter;
  auto mpm = _messagePassingMng();

  // NOTE: cette instance sera détruite par le ParallelMngDispatcher
  auto* control_dispatcher = new ControlDispatcherDecorator(this,m_adapter);
  _setControlDispatcher(control_dispatcher);

  // NOTE: cette instance sera détruite par le ParallelMngDispatcher
  auto* serialize_dispatcher = new MpiSerializeDispatcher(m_adapter);
  m_mpi_serialize_dispatcher = serialize_dispatcher;
  _setSerializeDispatcher(serialize_dispatcher);

  DispatchCreator creator(m_trace,mpm,m_adapter,m_datatype_list);
  this->createDispatchers(creator);

  m_io_mng = arcaneCreateIOMng(this);

  m_non_blocking_collective = new MpiParallelNonBlockingCollective(tm,this,adapter);
  m_non_blocking_collective->build();
  if (m_mpi_lock)
    m_trace->info() << "Using mpi with locks.";
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
_checkInit()
{
  if (m_is_initialized)
    return;
  ARCANE_FATAL("Trying to use unitialized parallel mng");
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
initialize()
{
  Trace::Setter mci(m_trace,"Mpi");
  if (m_is_initialized){
    m_trace->warning() << "MpiParallelMng already initialized";
    return;
  }
	
  m_trace->info() << "Initialisation de MpiParallelMng";
  m_sequential_parallel_mng->initialize();

  m_adapter->setTimeMetricCollector(timeMetricCollector());

  m_is_initialized = true;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

SerializeBuffer* MpiParallelMng::
_castSerializer(ISerializer* serializer)
{
  SerializeBuffer* sbuf = dynamic_cast<SerializeBuffer*>(serializer);
  if (!sbuf)
    ARCANE_THROW(ArgumentException,"Can not cast 'ISerializer' to 'SerializeBuffer'");
  return sbuf;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IGetVariablesValuesParallelOperation* MpiParallelMng::
createGetVariablesValuesOperation()
{
  return new GetVariablesValuesParallelOperation(this);
}

ITransferValuesParallelOperation* MpiParallelMng::
createTransferValuesOperation()
{
  return new TransferValuesParallelOperation(this);
}

IParallelExchanger* MpiParallelMng::
createExchanger()
{
  return new ParallelExchanger(this);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
sendSerializer(ISerializer* s,Int32 rank)
{
  Trace::Setter mci(m_trace,"Mpi");
  Timer::Phase tphase(timeStats(),TP_Communication);
  MessageTag mpi_tag = BasicSerializeMessage::defaultTag();
  m_mpi_serialize_dispatcher->legacySendSerializer(s,{MessageRank(rank),mpi_tag,Blocking});
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ISerializeMessage* MpiParallelMng::
createSendSerializer(Int32 rank)
{
  auto x = new SerializeMessage(m_comm_rank,rank,ISerializeMessage::MT_Send);
  return x;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Request MpiParallelMng::
sendSerializer(ISerializer* s,Int32 rank,[[maybe_unused]] ByteArray& bytes)
{
  Trace::Setter mci(m_trace,"Mpi");
  Timer::Phase tphase(timeStats(),TP_Communication);
  MessageTag mpi_tag = BasicSerializeMessage::defaultTag();
  return m_mpi_serialize_dispatcher->legacySendSerializer(s,{MessageRank(rank),mpi_tag,NonBlocking});
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
allGatherSerializer(ISerializer* send_serializer,ISerializer* recv_serializer)
{
  Timer::Phase tphase(timeStats(),TP_Communication);
  SerializeBuffer* sbuf = _castSerializer(send_serializer);
  SerializeBuffer* recv_buf = _castSerializer(recv_serializer);
  recv_buf->allGather(this,*sbuf);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
broadcastSerializer(ISerializer* values,Int32 rank)
{
  Timer::Phase tphase(timeStats(),TP_Communication);
  m_mpi_serialize_dispatcher->broadcastSerializer(values,MessageRank(rank));
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
recvSerializer(ISerializer* values,Int32 rank)
{
  Trace::Setter mci(m_trace,"Mpi");
  Timer::Phase tphase(timeStats(),TP_Communication);
  MessageTag mpi_tag = BasicSerializeMessage::defaultTag();
  m_mpi_serialize_dispatcher->legacyReceiveSerializer(values,MessageRank(rank),mpi_tag);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ISerializeMessage* MpiParallelMng::
createReceiveSerializer(Int32 rank)
{
  auto x = new SerializeMessage(m_comm_rank,rank,ISerializeMessage::MT_Recv);
  return x;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

auto MpiParallelMng::
probe(const PointToPointMessageInfo& message) -> MessageId
{
  return m_adapter->probeMessage(message);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Request MpiParallelMng::
sendSerializer(const ISerializer* s,const PointToPointMessageInfo& message)
{
  return m_mpi_serialize_dispatcher->sendSerializer(s,message);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Request MpiParallelMng::
receiveSerializer(ISerializer* s,const PointToPointMessageInfo& message)
{
  return m_mpi_serialize_dispatcher->receiveSerializer(s,message);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
freeRequests(ArrayView<Parallel::Request> requests)
{
  for( Integer i=0, is=requests.size(); i<is; ++i )
    m_adapter->freeRequest(requests[i]);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
_checkFinishedSubRequests()
{
  m_mpi_serialize_dispatcher->checkFinishedSubRequests();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Ref<IParallelMng> MpiParallelMng::
sequentialParallelMngRef()
{
  return m_sequential_parallel_mng;
}

IParallelMng* MpiParallelMng::
sequentialParallelMng()
{
  return m_sequential_parallel_mng.get();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
printStats()
{
  if (m_stat)
    m_stat->print(m_trace);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
barrier()
{
  traceMng()->flush();
  m_adapter->barrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
waitAllRequests(ArrayView<Request> requests)
{
  m_adapter->waitAllRequests(requests);
  _checkFinishedSubRequests();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

UniqueArray<Integer> MpiParallelMng::
waitSomeRequests(ArrayView<Request> requests)
{
  return _waitSomeRequests(requests, false);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

UniqueArray<Integer> MpiParallelMng::
testSomeRequests(ArrayView<Request> requests)
{
  return _waitSomeRequests(requests, true);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

UniqueArray<Integer> MpiParallelMng::
_waitSomeRequests(ArrayView<Request> requests, bool is_non_blocking)
{
  UniqueArray<Integer> results;
  UniqueArray<bool> done_indexes(requests.size());

  m_adapter->waitSomeRequests(requests, done_indexes, is_non_blocking);
  for (int i = 0 ; i < requests.size() ; i++) {
    if (done_indexes[i])
      results.add(i);
  }
  return results;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ISerializeMessageList* MpiParallelMng::
_createSerializeMessageList()
{
  // Pour l'instant (avril 2020) on laisse l'implémentation historique le
  // temps de valider l'ancienne.
  bool do_new = false;
  if (do_new)
    return new MP::internal::SerializeMessageList(messagePassingMng());
  return new MpiSerializeMessageList(serializeDispatcher());
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IVariableSynchronizer* MpiParallelMng::
createSynchronizer(IItemFamily* family)
{
  typedef DataTypeDispatchingDataVisitor<IVariableSynchronizeDispatcher> DispatcherType;
  MpiVariableSynchronizeDispatcherBuildInfo bi(this,nullptr);
  auto vd = new VariableSynchronizerDispatcher(this,DispatcherType::create<MpiVariableSynchronizeDispatcher>(bi));
  return new VariableSynchronizer(this,family->allItems(),vd);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IVariableSynchronizer* MpiParallelMng::
createSynchronizer(const ItemGroup& group)
{
  typedef DataTypeDispatchingDataVisitor<IVariableSynchronizeDispatcher> DispatcherType;
  SharedPtrT<GroupIndexTable> table = group.localIdToIndex();
  MpiVariableSynchronizeDispatcherBuildInfo bi(this,table.get());
  auto vd = new VariableSynchronizerDispatcher(this,DispatcherType::create<MpiVariableSynchronizeDispatcher>(bi));
  return new VariableSynchronizer(this,group,vd);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IParallelTopology* MpiParallelMng::
createTopology()
{
  ParallelTopology* t = new ParallelTopology(this);
  t->initialize();
  return t;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IParallelReplication* MpiParallelMng::
replication() const
{
  return m_replication;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void MpiParallelMng::
setReplication(IParallelReplication* v)
{
  delete m_replication;
  m_replication = v;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

IParallelMng* MpiParallelMng::
_createSubParallelMng(Int32ConstArrayView kept_ranks)
{
  MPI_Group mpi_group = MPI_GROUP_NULL;
  MPI_Comm_group(m_communicator,&mpi_group);
  Integer nb_sub_rank = kept_ranks.size();
  UniqueArray<int> mpi_kept_ranks(nb_sub_rank);
  for( Integer i=0; i<nb_sub_rank; ++i )
    mpi_kept_ranks[i] = (int)kept_ranks[i];

  MPI_Group final_group = MPI_GROUP_NULL;
  MPI_Group_incl(mpi_group,nb_sub_rank,mpi_kept_ranks.data(),&final_group);
  MPI_Comm sub_communicator = MPI_COMM_NULL;

  MPI_Comm_create(m_communicator,final_group, &sub_communicator);
  // Si nul, ce rang ne fait pas partie du sous-communicateur
  if (sub_communicator==MPI_COMM_NULL)
    return nullptr;

  int sub_rank = -1;
  MPI_Comm_rank(sub_communicator,&sub_rank);

  MpiParallelMngBuildInfo bi(sub_communicator);
  bi.is_parallel = isParallel();
  bi.stat = m_stat;
  bi.timer_mng = m_timer_mng;
  bi.thread_mng = m_thread_mng;
  bi.trace_mng = m_trace;
  bi.world_parallel_mng = m_world_parallel_mng;
  bi.mpi_lock = m_mpi_lock;

  IParallelMng* sub_pm = new MpiParallelMng(bi);
  sub_pm->build();
  return sub_pm;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \brief Spécialisation de MpiRequestList pour MpiParallelMng.
 *
 * Cette classe fait juste en sorte d'appeler _checkFinishedSubRequests();
 * après les wait. Elle ne sera plus utile lorsqu'on utilisera l'implémentation
 * 'SerializeMessageList' de message_passing.
 */
class MpiParallelMng::RequestList
: public MpiRequestList
{
  using Base = MpiRequestList;
 public:
  RequestList(MpiParallelMng* pm)
  : Base(pm->m_adapter), m_parallel_mng(pm){}
 public:
  void _wait(Parallel::eWaitType wait_type) override
  {
    Base::_wait(wait_type);
    m_parallel_mng->_checkFinishedSubRequests();
  };
 private:
  MpiParallelMng* m_parallel_mng;
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

Ref<IRequestList> MpiParallelMng::
createRequestListRef()
{
  IRequestList* x = new RequestList(this);
  return makeRef(x);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
