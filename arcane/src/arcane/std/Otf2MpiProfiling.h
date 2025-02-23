﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
//-----------------------------------------------------------------------------
// Copyright 2000-2021 CEA (www.cea.fr) IFPEN (www.ifpenergiesnouvelles.com)
// See the top-level COPYRIGHT file for details.
// SPDX-License-Identifier: Apache-2.0
//-----------------------------------------------------------------------------
/*---------------------------------------------------------------------------*/
/* Otf2MpiProfiling.h                                          (C) 2000-2018 */
/*                                                                           */
/* Implementation de l'interface IMpiProfiling permettant l'instrumentation  */
/* au format OTF2                              .                             */
/*---------------------------------------------------------------------------*/
#ifndef ARCANE_STD_OTF2MPIPROFILING_H
#define ARCANE_STD_OTF2MPIPROFILING_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/base/BaseTypes.h"
#include "arccore/collections/CollectionsGlobal.h"
#include "arccore/message_passing/Request.h"
#include "arccore/message_passing_mpi/MessagePassingMpiGlobal.h"
#include "arccore/message_passing_mpi/IMpiProfiling.h"
#include "arccore/message_passing_mpi/MessagePassingMpiEnum.h"
#include "arcane/std/Otf2LibWrapper.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/


namespace Arcane
{
  using namespace Arccore::MessagePassing::Mpi;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \internal
 * \brief Implementation de l'interface des operations MPI.
 * Decore chacun des appels MPI avec les fonctions de la librairie
 * Otf2 pour faire du profiling.
 */
 class Otf2MpiProfiling
 : public IMpiProfiling
{
 public:
  explicit Otf2MpiProfiling(Otf2LibWrapper* otf2_wrapper);
  ~Otf2MpiProfiling() override = default;

 public:
	// Bcast
	void broadcast(void *buffer, int count, MPI_Datatype datatype, int root, MPI_Comm comm) final;
	// Gather
	void gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
	            int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm) final;
	// Gatherv
	void gatherVariable(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                      const int *recvcounts, const int *displs, MPI_Datatype recvtype, int root, MPI_Comm comm) final;
	// Allgather
	void allGather(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                 int recvcount, MPI_Datatype recvtype, MPI_Comm comm) final;
	// Allgatherv
	void allGatherVariable(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                         const int *recvcounts, const int *displs, MPI_Datatype recvtype, MPI_Comm comm) final;
	// Scatterv
	void scatterVariable(const void *sendbuf, const int *sendcounts, const int *displs,
	                     MPI_Datatype sendtype, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                       int root, MPI_Comm comm) final;
	// Alltoall
	void allToAll(const void *sendbuf, int sendcount, MPI_Datatype sendtype, void *recvbuf,
                int recvcount, MPI_Datatype recvtype, MPI_Comm comm) final;
	// Alltoallv
	void allToAllVariable(const void *sendbuf, const int *sendcounts, const int *sdispls,
	                      MPI_Datatype sendtype, void *recvbuf, const int *recvcounts,
	                      const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm) final;
	// Barrier
	void barrier(MPI_Comm comm) final;
	// Reduce
	void reduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	            MPI_Op op, int root, MPI_Comm comm) final;
	// Allreduce
	void allReduce(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype,
	               MPI_Op op, MPI_Comm comm) final;
	// Scan
	void scan(const void *sendbuf, void *recvbuf, int count, MPI_Datatype datatype, MPI_Op op, MPI_Comm comm) final;
	// Sendrecv
	void sendRecv(const void *sendbuf, int sendcount, MPI_Datatype sendtype, int dest,
                int sendtag, void *recvbuf, int recvcount, MPI_Datatype recvtype,
                int source, int recvtag, MPI_Comm comm, MPI_Status *status) final;
	// Isend
	void iSend(const void *buf, int count, MPI_Datatype datatype, int dest, int tag,
	           MPI_Comm comm, MPI_Request *request) final;
	// Send
	void send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) final;
  // Irecv
  void iRecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Request *request) final;
	// recv
	void recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) final;
	// Test
	void test(MPI_Request *request, int *flag, MPI_Status *status) final;
	// Probe
	void probe(int source, int tag, MPI_Comm comm, MPI_Status *status) final;
	// Get_count
	void getCount(const MPI_Status *status, MPI_Datatype datatype, int *count) final;
	// Wait
	void wait(MPI_Request *request, MPI_Status *status) final;
	// Waitall
	void waitAll(int count, MPI_Request array_of_requests[], MPI_Status array_of_statuses[]) final;
	// Testsome
  void testSome(int incount, MPI_Request array_of_requests[], int *outcount,
	              int array_of_indices[], MPI_Status array_of_statuses[]) final;
	// Waitsome
	void waitSome(int incount, MPI_Request array_of_requests[], int *outcount,
	              int array_of_indices[], MPI_Status array_of_statuses[]) final;

 private:
  Otf2LibWrapper* m_otf2_wrapper;
 private:
  void _doEventEnter(eMpiName event_name);
  void _doEventLeave(eMpiName event_name);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

}  // namespace Arcane

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
