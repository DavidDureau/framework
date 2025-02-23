﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* ParallelExchanger.cc                                        (C) 2000-2012 */
/*                                                                           */
/* Echange d'informations entre processeurs.                                 */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arcane/utils/ArcanePrecomp.h"

#include "arcane/utils/NotSupportedException.h"

#include "arcane/impl/ParallelExchanger.h"

#include "arcane/IParallelMng.h"
#include "arcane/IItemFamily.h"
#include "arcane/SerializeBuffer.h"
#include "arcane/SerializeMessage.h"
#include "arcane/Timer.h"

#include <algorithm>

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_BEGIN_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ParallelExchanger::
ParallelExchanger(IParallelMng* pm)
: TraceAccessor(pm->traceMng())
, m_parallel_mng(pm)
, m_own_send_message(0)
, m_own_recv_message(0)
, m_exchange_mode(EM_Independant)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ParallelExchanger::
~ParallelExchanger()
{
  for( Integer i=0, is=m_comms_buf.size(); i<is; ++i )
    delete m_comms_buf[i];
  m_comms_buf.clear();
  delete m_own_send_message;
  delete m_own_recv_message;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

bool ParallelExchanger::
initializeCommunicationsMessages()
{
  Integer nb_send_rank = m_send_ranks.size();
  IntegerUniqueArray gather_input_send_ranks(nb_send_rank+1);
  gather_input_send_ranks[0] = nb_send_rank;
  std::copy(std::begin(m_send_ranks),std::end(m_send_ranks),
            std::begin(gather_input_send_ranks)+1);

  debug() << "Number of subdomain to communicate with: "
          << nb_send_rank;

  IntegerUniqueArray gather_output_send_ranks;
  Integer nb_rank = m_parallel_mng->commSize();
  m_parallel_mng->allGatherVariable(gather_input_send_ranks,
                                    gather_output_send_ranks);
  
  m_recv_ranks.clear();
  Integer total_comm_rank = 0;
  Int32 my_rank = m_parallel_mng->commRank();
  {
    Integer gather_index = 0;
    for( Integer i=0; i<nb_rank; ++i ){
      Integer nb_comm = gather_output_send_ranks[gather_index];
      total_comm_rank += nb_comm;
      ++gather_index;
      for( Integer z=0; z<nb_comm; ++z ){
        Integer current_rank = gather_output_send_ranks[gather_index+z];
        if (current_rank==my_rank)
          m_recv_ranks.add(i);
      }
      gather_index += nb_comm;
    }
  }

  if (total_comm_rank==0)
    return true;
  
  _initializeCommunicationsMessages();

  return false;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void ParallelExchanger::
initializeCommunicationsMessages(Int32ConstArrayView recv_ranks)
{
  m_recv_ranks.resize(recv_ranks.size());
  m_recv_ranks.copy(recv_ranks);
  _initializeCommunicationsMessages();
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void ParallelExchanger::
_initializeCommunicationsMessages()
{
  debug() << "Has to send information to " << (m_send_ranks.size())
          << " subdomains: ";
  for( Integer i=0; i<m_send_ranks.size(); ++i ){
    debug() << "==> subdomain " << m_send_ranks[i];
  }
  debug() << "Has to receive information from " << m_recv_ranks.size()
                << " subdomains: ";
  for( Integer i=0; i<m_recv_ranks.size(); ++i ){
    debug() << "==> subdomain " << m_recv_ranks[i];
  }

  Int32 my_rank = m_parallel_mng->commRank();

  for( Integer i=0, is=m_send_ranks.size(); i<is; ++i ){
    Integer dest_rank = m_send_ranks[i];
    SerializeMessage* comm = new SerializeMessage(my_rank,dest_rank,
                                                  ISerializeMessage::MT_Send);
    // Il ne sert à rien de s'envoyer des messages.
    // (En plus ca fait planter certaines versions de MPI...)
    if (my_rank==dest_rank){
      m_own_send_message = comm;
    }
    else{
      m_comms_buf.add(comm);
    }
    m_send_serialize_infos.add(comm);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void ParallelExchanger::
processExchange()
{
  bool use_all_to_all = false;
  if (m_exchange_mode==EM_Collective)
    use_all_to_all = true;
  // TODO: traiter le cas EM_Auto

  // Génère les infos pour chaque processeur de qui on va recevoir
  // des entités
  Int32 my_rank = m_parallel_mng->commRank();
  for( Integer i=0, is=m_recv_ranks.size(); i<is; ++i ){
    Integer dest_rank = m_recv_ranks[i];
    SerializeMessage* comm = new SerializeMessage(my_rank,dest_rank,
                                                  ISerializeMessage::MT_Recv);
    // Il ne sert à rien de s'envoyer des messages.
    // (En plus ca fait planter certaines versions de MPI...)
    if (my_rank==dest_rank){
      m_own_recv_message = comm;
    }
    else
      m_comms_buf.add(comm);
    m_recv_serialize_infos.add(comm);
  }

  if (use_all_to_all){
    _processExchangeCollective();
  }
  else{
    m_parallel_mng->processMessages(m_comms_buf);

    if (m_own_send_message && m_own_recv_message){
      m_own_recv_message->buffer().copy(m_own_send_message->buffer());
    }
  }

  // Récupère les infos de chaque receveur
  for( Integer i=0, is=m_recv_serialize_infos.size(); i<is; ++i ){
    SerializeMessage* comm = m_recv_serialize_infos[i];
    SerializeBuffer& sbuf = comm->buffer();
    sbuf.setMode(ISerializer::ModeGet);
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

void ParallelExchanger::
_processExchangeCollective()
{
  info() << "Using collective exchange in ParallelExchanger";

  IParallelMng* pm = m_parallel_mng;
  Int32 nb_rank = pm->commSize();

  Int32UniqueArray send_counts(nb_rank,0);
  Int32UniqueArray send_indexes(nb_rank,0);
  Int32UniqueArray recv_counts(nb_rank,0);
  Int32UniqueArray recv_indexes(nb_rank,0);
 
  // D'abord, détermine pour chaque proc le nombre d'octets à envoyer
  for( Integer i=0, n=m_send_serialize_infos.size(); i<n; ++i ){
    SerializeMessage* comm = m_send_serialize_infos[i];
    SerializeBuffer& sbuf = comm->buffer();
    Span<Byte> val_buf = sbuf.globalBuffer();
    Int32 rank = comm->destRank();
    send_counts[rank] = arcaneCheckArraySize(val_buf.size());
  }

  // Fait un AllToAll pour connaitre combien de valeurs je dois recevoir des autres.
  {
    Timer::SimplePrinter sp(traceMng(),"ParallelExchanger: sending sizes with AllToAll");
    pm->allToAll(send_counts,recv_counts,1);
  }

  // Détermine le nombre total d'infos à envoyer et recevoir
  //TODO: Vérifier débordement.

  //TODO: En cas débordement, il faudrait le faire en plusieurs morceaux
  // ou alors revenir aux échanges point à point.
  Int32 total_send = 0;
  Int32 total_recv = 0;
  for( Integer i=0; i<nb_rank; ++i ){
    send_indexes[i] = total_send;
    recv_indexes[i] = total_recv;
    total_send += send_counts[i];
    total_recv += recv_counts[i];
  }

  ByteUniqueArray send_buf(total_send);
  ByteUniqueArray recv_buf(total_recv);
  bool is_verbose = false;
  if (is_verbose){
    for( Integer i=0; i<nb_rank; ++i ){
      info() << "INFOS: rank=" << i << " send_count=" << send_counts[i]
             << " send_idx=" << send_indexes[i]
             << " recv_count=" << recv_counts[i]
             << " recv_idx=" << recv_indexes[i];
    }
  }

  // Copie dans send_buf les infos des sérialisers.
  for( Integer i=0, n=m_send_serialize_infos.size(); i<n; ++i ){
    SerializeMessage* comm = m_send_serialize_infos[i];
    SerializeBuffer& sbuf = comm->buffer();
    Span<Byte> val_buf = sbuf.globalBuffer();
    Int32 rank = comm->destRank();
    if (is_verbose)
      info() << "SEND rank=" << rank << " size=" << send_counts[rank]
             << " idx=" << send_indexes[rank]
             << " buf_size=" << val_buf.size();
    ByteArrayView dest_buf(send_counts[rank],&send_buf[send_indexes[rank]]);
    dest_buf.copy(val_buf);
  }

  info() << "AllToAllVariable total_send=" << total_send
         << " total_recv=" << total_recv;

  {
    Timer::SimplePrinter sp(traceMng(),"ParallelExchanger: sending values with AllToAll");
    pm->allToAllVariable(send_buf,send_counts,send_indexes,recv_buf,recv_counts,recv_indexes);
  }
  // Recopie les données reçues dans le message correspondant.
  for( Integer i=0, n=m_recv_serialize_infos.size(); i<n; ++i ){
    SerializeMessage* comm = m_recv_serialize_infos[i];
    SerializeBuffer& sbuf = comm->buffer();
    Int32 rank = comm->destRank();
    if (is_verbose)
      info() << "RECV rank=" << rank << " size=" << recv_counts[rank]
             << " idx=" << recv_indexes[rank];
    ByteArrayView orig_buf(recv_counts[rank],&recv_buf[recv_indexes[rank]]);

    sbuf.preallocate(orig_buf.size());
    sbuf.globalBuffer().copy(orig_buf);
    sbuf.setFromSizes();
  }
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ISerializeMessage* ParallelExchanger::
messageToSend(Integer i)
{
  return m_send_serialize_infos[i];
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ISerializeMessage* ParallelExchanger::
messageToReceive(Integer i)
{
  return m_recv_serialize_infos[i];
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

ARCANE_END_NAMESPACE

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

