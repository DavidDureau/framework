﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* SharedMemoryParallelDispatch.cc                             (C) 2000-2019 */
/*                                                                           */
/* Implémentation des messages en mémoire partagée.                          */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/ArcanePrecomp.h"

#include "arcane/utils/Array.h"
#include "arcane/utils/PlatformUtils.h"
#include "arcane/utils/String.h"
#include "arcane/utils/ITraceMng.h"
#include "arcane/utils/Real2.h"
#include "arcane/utils/Real3.h"
#include "arcane/utils/Real2x2.h"
#include "arcane/utils/Real3x3.h"
#include "arcane/utils/APReal.h"
#include "arcane/utils/NotImplementedException.h"

#include "arcane/MeshVariable.h"
#include "arcane/IParallelMng.h"
#include "arcane/ItemGroup.h"
#include "arcane/IMesh.h"
#include "arcane/IBase.h"

#include "arcane/parallel/thread/SharedMemoryParallelDispatch.h"
#include "arcane/parallel/thread/SharedMemoryParallelMng.h"
#include "arcane/parallel/thread/ISharedMemoryMessageQueue.h"

#include "arccore/message_passing/PointToPointMessageInfo.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arcane::MessagePassing
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*
 * TODO: pour simplifier le debug lorsqu'il y a un décalage des appels
 * collectifs entre les threads, il faudrait faire un type de barrière
 * par type d'appel collectif alors qu'actuellement tous les appels
 * collectifs utilisent la même barrière (via _collectiveBarrier()).
 * A cause de cela, des problèmes peuvent survenir qui ne sont pas
 * facilement détectable. Par exemple:
 *
 *  Thread1:
 * allGather();
 * barrier();
 * allReduce();
 *  Thread2:
 * barrier();
 * allGather();
 * allReduce();
 *
 * Dans ce cas, le code ne plantera pas mais les valeurs des collectives ne
 * seront pas bonnes.
 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> SharedMemoryParallelDispatch<Type>::
SharedMemoryParallelDispatch(ITraceMng* tm,SharedMemoryParallelMng* parallel_mng,
                             ISharedMemoryMessageQueue* message_queue,
                             ArrayView<SharedMemoryParallelDispatch<Type>*> all_dispatchs)
: TraceAccessor(tm)
, m_parallel_mng(parallel_mng)
, m_rank(parallel_mng->commRank())
, m_nb_rank(parallel_mng->commSize())
, m_all_dispatchs(all_dispatchs)
, m_message_queue(message_queue)
{
  m_reduce_infos.m_index = 0;
  m_all_dispatchs[m_rank] = this;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> SharedMemoryParallelDispatch<Type>::
~SharedMemoryParallelDispatch()
{
  finalize();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/ 

template<class Type> void SharedMemoryParallelDispatch<Type>::
finalize()
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<typename T>
class _ThreadIntegralType
{
 public:
  typedef FalseType IsIntegral;
};

#define ARCANE_DEFINE_INTEGRAL_TYPE(datatype)\
template<>\
class _ThreadIntegralType<datatype>\
{\
 public:\
  typedef TrueType IsIntegral;\
}

ARCANE_DEFINE_INTEGRAL_TYPE(long long);
ARCANE_DEFINE_INTEGRAL_TYPE(long);
ARCANE_DEFINE_INTEGRAL_TYPE(int);
ARCANE_DEFINE_INTEGRAL_TYPE(short);
ARCANE_DEFINE_INTEGRAL_TYPE(unsigned long long);
ARCANE_DEFINE_INTEGRAL_TYPE(unsigned long);
ARCANE_DEFINE_INTEGRAL_TYPE(unsigned int);
ARCANE_DEFINE_INTEGRAL_TYPE(unsigned short);
ARCANE_DEFINE_INTEGRAL_TYPE(double);
ARCANE_DEFINE_INTEGRAL_TYPE(float);
ARCANE_DEFINE_INTEGRAL_TYPE(HPReal);

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace
{

template<class Type> void
_computeMinMaxSum2(ArrayView<SharedMemoryParallelDispatch<Type>*> all_dispatchs,
                   Type& min_val,Type& max_val,Type& sum_val,
                   Int32& min_rank,Int32& max_rank,Int32 nb_rank,FalseType)
{
  ARCANE_UNUSED(all_dispatchs);
  ARCANE_UNUSED(min_val);
  ARCANE_UNUSED(max_val);
  ARCANE_UNUSED(sum_val);
  ARCANE_UNUSED(min_rank);
  ARCANE_UNUSED(max_rank);
  ARCANE_UNUSED(nb_rank);

  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void 
_computeMinMaxSum2(ArrayView<SharedMemoryParallelDispatch<Type>*> all_dispatchs,
                   Type& min_val,Type& max_val,Type& sum_val,
                   Int32& min_rank,Int32& max_rank,Int32 nb_rank,TrueType)
{
  Type _min_val = all_dispatchs[0]->m_reduce_infos.reduce_value;
  Type _max_val = _min_val;
  Type _sum_val = _min_val;
  Integer _min_rank = 0;
  Integer _max_rank = 0;
  for( Integer i=1; i<nb_rank; ++i ){
    Type cval = all_dispatchs[i]->m_reduce_infos.reduce_value;
    if (cval<_min_val){
      _min_val = cval;
      _min_rank = i;
    }
    if (cval>_max_val){
      _max_val = cval;
      _max_rank = i;
    }
    _sum_val = (Type)(_sum_val + cval);
  }
  min_val = _min_val;
  max_val = _max_val;
  sum_val = _sum_val;
  min_rank = _min_rank;
  max_rank = _max_rank;
}

}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
computeMinMaxSum(Type val,Type& min_val,Type& max_val,Type& sum_val,
                 Int32& min_rank,Int32& max_rank)
{
  typedef typename _ThreadIntegralType<Type>::IsIntegral IntegralType;
  m_reduce_infos.reduce_value = val;
  _collectiveBarrier();
  _computeMinMaxSum2(m_all_dispatchs,min_val,max_val,sum_val,min_rank,max_rank,m_nb_rank,IntegralType());
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
computeMinMaxSum(ConstArrayView<Type> values,
                 ArrayView<Type> min_values,
                 ArrayView<Type> max_values,
                 ArrayView<Type> sum_values,
                 ArrayView<Int32> min_ranks,
                 ArrayView<Int32> max_ranks)
{
  // Implémentation sous-optimale qui ne vectorise pas le calcul 
  // (c'est actuellement un copier-coller d'au-dessus mis dans une boucle)
  typedef typename _ThreadIntegralType<Type>::IsIntegral IntegralType;
  Integer n = values.size();
  for(Integer i=0;i<n;++i) {
    m_reduce_infos.reduce_value = values[i];
    _collectiveBarrier();  
    _computeMinMaxSum2(m_all_dispatchs,min_values[i],max_values[i],sum_values[i],
                       min_ranks[i],max_ranks[i],m_nb_rank,IntegralType());
    _collectiveBarrier();
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
broadcast(Span<Type> send_buf,Int32 rank)
{
  m_broadcast_view = send_buf;
  _collectiveBarrier();
  send_buf.copy(m_all_dispatchs[rank]->m_broadcast_view);
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
allGather(Span<const Type> send_buf,Span<Type> recv_buf)
{
  m_const_view = send_buf;
  _collectiveBarrier();
  Int64 total_size = 0;
  for( Integer i=0; i<m_nb_rank; ++i ){
    total_size += m_all_dispatchs[i]->m_const_view.size();
  }
  //recv_buf.resize(total_size);
  Int64 index = 0;
  for( Int32 i=0; i<m_nb_rank; ++i ){
    Span<const Type> view = m_all_dispatchs[i]->m_const_view;
    Int64 size = view.size();
    for( Int64 j=0; j<size; ++j )
      recv_buf[j+index] = view[j];
    index += size;
  }
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
gather(Span<const Type> send_buf,Span<Type> recv_buf,Int32 rank)
{
  ARCANE_UNUSED(send_buf);
  ARCANE_UNUSED(recv_buf);
  ARCANE_UNUSED(rank);
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
allGatherVariable(Span<const Type> send_buf,Array<Type>& recv_buf)
{
  m_const_view = send_buf;
  _collectiveBarrier();
  Int64 total_size = 0;
  for( Integer i=0; i<m_nb_rank; ++i ){
    total_size += m_all_dispatchs[i]->m_const_view.size();
  }
  recv_buf.resize(total_size);
  Int64 index = 0;
  for( Integer i=0; i<m_nb_rank; ++i ){
    Span<const Type> view = m_all_dispatchs[i]->m_const_view;
    Int64 size = view.size();
    for( Int64 j=0; j<size; ++j )
      recv_buf[j+index] = view[j];
    index += size;
  }
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
gatherVariable(Span<const Type> send_buf,Array<Type>& recv_buf,Int32 rank)
{
  ARCANE_UNUSED(send_buf);
  ARCANE_UNUSED(recv_buf);
  ARCANE_UNUSED(rank);
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
scatterVariable(Span<const Type> send_buf,Span<Type> recv_buf,Int32 root)
{
  m_const_view = send_buf;
  m_recv_view = recv_buf;
  _collectiveBarrier();
  if (m_rank==root){
    Int64 index = 0;
    for( Integer i=0; i<m_nb_rank; ++i ){
      Span<Type> view = m_all_dispatchs[i]->m_recv_view;
      Int64 size = view.size();
      for( Int64 j=0; j<size; ++j )
        view[j] = m_const_view[index+j];
      index += size;
    }
  }
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
allToAll(Span<const Type> send_buf,Span<Type> recv_buf,Int32 count)
{
  Int32 nb_rank = m_nb_rank;

  //TODO: Faire une version sans allocation
  Int32UniqueArray send_count(nb_rank,count);
  Int32UniqueArray recv_count(nb_rank,count);

  Int32UniqueArray send_indexes(nb_rank);
  Int32UniqueArray recv_indexes(nb_rank);
  for( Integer i=0; i<nb_rank; ++i ){
    send_indexes[i] = count * i;
    recv_indexes[i] = count * i;
  }
  this->allToAllVariable(send_buf,send_count,send_indexes,recv_buf,recv_count,recv_indexes);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
allToAllVariable(Span<const Type> send_buf,
                 Int32ConstArrayView send_count,
                 Int32ConstArrayView send_index,
                 Span<Type> recv_buf,
                 Int32ConstArrayView recv_count,
                 Int32ConstArrayView recv_index
                 )
{
  m_alltoallv_infos.send_buf = send_buf;
  m_alltoallv_infos.send_count = send_count;
  m_alltoallv_infos.send_index = send_index;
  m_alltoallv_infos.recv_buf = recv_buf;
  m_alltoallv_infos.recv_count = recv_count;
  m_alltoallv_infos.recv_index = recv_index;
  _collectiveBarrier();
  Integer global_index = 0;
  Int32 my_rank = m_rank;
  for( Integer i=0; i<m_nb_rank; ++i ){
    AllToAllVariableInfo ainfo = m_all_dispatchs[i]->m_alltoallv_infos;
    Span<const Type> view = ainfo.send_buf;
    Integer index = ainfo.send_index[my_rank];
    Integer count = ainfo.send_count[my_rank];
    //Integer size = view.size();
    for( Integer j=0; j<count; ++j )
      recv_buf[global_index+j] = view[index+j];
    global_index += count;
  }
  _collectiveBarrier();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> auto SharedMemoryParallelDispatch<Type>::
send(Span<const Type> send_buffer,Int32 rank,bool is_blocking) -> Request
{
  auto block_mode = (is_blocking) ? Parallel::Blocking : Parallel::NonBlocking;
  auto p2p_message = m_parallel_mng->buildMessage(rank,block_mode);
  return send(send_buffer,p2p_message);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
send(ConstArrayView<Type> send_buf,Int32 rank)
{
  send(send_buf,rank,true);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> Parallel::Request SharedMemoryParallelDispatch<Type>::
receive(Span<Type> recv_buffer,Int32 rank,bool is_blocking)
{
  auto block_mode = (is_blocking) ? Parallel::Blocking : Parallel::NonBlocking;
  auto p2p_message = m_parallel_mng->buildMessage(rank,block_mode);
  return receive(recv_buffer,p2p_message);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> Request SharedMemoryParallelDispatch<Type>::
send(Span<const Type> send_buffer,const PointToPointMessageInfo& message2)
{
  PointToPointMessageInfo message(message2);
  message.setSourceRank(MessageRank(m_rank));
  bool is_blocking = message.isBlocking();
  if (message.isRankTag()){
    Request r = m_message_queue->addSend(message,SendBufferInfo(asBytes(send_buffer)));
    if (is_blocking){
      m_message_queue->waitAll(ArrayView<Request>(1,&r));
      return Request();
    }
    return r;
  }
  if (message.isMessageId()){
    // Le send avec un MessageId n'existe pas.
    ARCCORE_THROW(NotSupportedException,"Invalid generic send with MessageId");
  }
  ARCCORE_THROW(NotSupportedException,"Invalid message_info");
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> Request SharedMemoryParallelDispatch<Type>::
receive(Span<Type> recv_buffer,const PointToPointMessageInfo& message2)
{
  PointToPointMessageInfo message(message2);
  bool is_blocking = message.isBlocking();
  message.setSourceRank(MessageRank(m_rank));
  ReceiveBufferInfo buf(asWritableBytes(recv_buffer));
  Request r = m_message_queue->addReceive(message,buf);
  if (is_blocking){
    m_message_queue->waitAll(ArrayView<Request>(1,&r));
    return MP::Request();
  }
  return r;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
recv(ArrayView<Type> recv_buffer,Integer rank)
{
  recv(recv_buffer,rank,true);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
sendRecv(ConstArrayView<Type> send_buffer,ArrayView<Type> recv_buffer,Integer proc)
{
  ARCANE_UNUSED(send_buffer);
  ARCANE_UNUSED(recv_buffer);
  ARCANE_UNUSED(proc);
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> Type SharedMemoryParallelDispatch<Type>::
allReduce(eReduceType op,Type send_buf)
{
  m_reduce_infos.reduce_value = send_buf;
  //cout << "ALL REDUCE BEGIN RANk=" << m_rank << " TYPE=" << (int)op << " MY=" << send_buf << '\n';
  cout.flush();
  _collectiveBarrier();
  Type ret = m_all_dispatchs[0]->m_reduce_infos.reduce_value;
  switch(op){
  case Parallel::ReduceMin:
    for( Integer i=1; i<m_nb_rank; ++i )
      ret = math::min(ret,m_all_dispatchs[i]->m_reduce_infos.reduce_value);
    break;
  case Parallel::ReduceMax:
    for( Integer i=1; i<m_nb_rank; ++i )
      ret = math::max(ret,m_all_dispatchs[i]->m_reduce_infos.reduce_value);
    break;
  case Parallel::ReduceSum:
    for( Integer i=1; i<m_nb_rank; ++i )
      ret = (Type)(ret + m_all_dispatchs[i]->m_reduce_infos.reduce_value);
    break;
  default:
    ARCANE_FATAL("Bad reduce type {0}",(int)op);
  }
  //cout << "ALL REDUCE RANK=" << m_rank << " TYPE=" << (int)op << " MY=" << send_buf << " GLOBAL=" << ret << '\n';
  _collectiveBarrier();
  return ret;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
allReduce(eReduceType op,Span<Type> send_buf)
{
  m_reduce_infos.reduce_buf = send_buf;
  ++m_reduce_infos.m_index;
  Int64 buf_size = send_buf.size();
  UniqueArray<Type> ret(buf_size);
  //cout << "ALL REDUCE BEGIN RANk=" << m_rank << " TYPE=" << (int)op << " MY=" << send_buf << '\n';
  //cout.flush();
  _collectiveBarrier();
  {
    Integer index0 = m_all_dispatchs[0]->m_reduce_infos.m_index;
    for( Integer i=0; i<m_nb_rank; ++i ){
      Integer indexi = m_all_dispatchs[i]->m_reduce_infos.m_index;
      if (index0!=m_all_dispatchs[i]->m_reduce_infos.m_index){
        ARCANE_FATAL("INTERNAL: incoherent all reduce i0={0} in={1} n={2}",
                     index0,indexi,i);
      }
    }
  }

  for( Integer j=0; j<buf_size; ++j )
    ret[j] = m_all_dispatchs[0]->m_reduce_infos.reduce_buf[j];
  switch(op){
  case Parallel::ReduceMin:
    for( Integer i=1; i<m_nb_rank; ++i )
      for( Integer j=0; j<buf_size; ++j )
        ret[j] = math::min(ret[j],m_all_dispatchs[i]->m_reduce_infos.reduce_buf[j]);
    break;
  case Parallel::ReduceMax:
    for( Integer i=1; i<m_nb_rank; ++i )
      for( Integer j=0; j<buf_size; ++j )
        ret[j] = math::max(ret[j],m_all_dispatchs[i]->m_reduce_infos.reduce_buf[j]);
    break;
  case Parallel::ReduceSum:
    for( Integer i=1; i<m_nb_rank; ++i )
      for( Integer j=0; j<buf_size; ++j )
        ret[j] = (Type)(ret[j] + m_all_dispatchs[i]->m_reduce_infos.reduce_buf[j]);
    break;
  default:
    ARCANE_FATAL("Bad reduce type");
  }
  //cout << "ALL REDUCE RANK=" << m_rank << " TYPE=" << (int)op << " MY=" << send_buf << " GLOBAL=" << ret << '\n';
  _collectiveBarrier();
  for( Integer j=0; j<buf_size; ++j )
    send_buf[j] = ret[j];
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> Type SharedMemoryParallelDispatch<Type>::
scan(eReduceType op,Type send_buf)
{
  ARCANE_UNUSED(op);
  ARCANE_UNUSED(send_buf);
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
scan(eReduceType op,ArrayView<Type> send_buf)
{
  ARCANE_UNUSED(op);
  ARCANE_UNUSED(send_buf);
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
_collectiveBarrier()
{
  m_parallel_mng->getThreadBarrier()->wait();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template<class Type> void SharedMemoryParallelDispatch<Type>::
waitAll()
{
  // TEMPORAIRE: a priori pas utilisé
  throw NotImplementedException(A_FUNCINFO);
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template class SharedMemoryParallelDispatch<char>;
template class SharedMemoryParallelDispatch<signed char>;
template class SharedMemoryParallelDispatch<unsigned char>;
template class SharedMemoryParallelDispatch<short>;
template class SharedMemoryParallelDispatch<unsigned short>;
template class SharedMemoryParallelDispatch<int>;
template class SharedMemoryParallelDispatch<unsigned int>;
template class SharedMemoryParallelDispatch<long>;
template class SharedMemoryParallelDispatch<unsigned long>;
template class SharedMemoryParallelDispatch<long long>;
template class SharedMemoryParallelDispatch<unsigned long long>;
template class SharedMemoryParallelDispatch<float>;
template class SharedMemoryParallelDispatch<double>;
template class SharedMemoryParallelDispatch<long double>;
template class SharedMemoryParallelDispatch<APReal>;
template class SharedMemoryParallelDispatch<Real2>;
template class SharedMemoryParallelDispatch<Real3>;
template class SharedMemoryParallelDispatch<Real2x2>;
template class SharedMemoryParallelDispatch<Real3x3>;
template class SharedMemoryParallelDispatch<HPReal>;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arcane::MessagePassing

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
