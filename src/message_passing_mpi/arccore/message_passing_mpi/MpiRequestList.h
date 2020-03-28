﻿// -*- tab-width: 2; indent-tabs-mode: nil; coding: utf-8-with-signature -*-
/*---------------------------------------------------------------------------*/
/* MpiRequestList.h                                            (C) 2000-2020 */
/*                                                                           */
/* Liste de requêtes MPI.                                                    */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_MESSAGEPASSINGMPI_MPIREQUESTLIST_H
#define ARCCORE_MESSAGEPASSINGMPI_MPIREQUESTLIST_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/message_passing/RequestListBase.h"
#include "arccore/message_passing_mpi/MessagePassingMpiGlobal.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore::MessagePassing::Mpi
{
/*!
 * \brief Liste de requêtes MPI.
 */
class ARCCORE_MESSAGEPASSINGMPI_EXPORT MpiRequestList
: public internal::RequestListBase
{
 public:

  MpiRequestList(MpiAdapter* adapter) : m_adapter(adapter){}

 public:

  Integer wait(eWaitType wait_type) override;

 private:

  MpiAdapter* m_adapter;
  UniqueArray<MPI_Status> m_requests_status;

 private:

  Integer _doWaitSome(bool is_non_blocking);
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore::MessagePassing

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  

