/*---------------------------------------------------------------------------*/
/* MpiAdapter.cc                                               (C) 2000-2018 */
/*                                                                           */
/* Gestionnaire de parallÚlisme utilisant MPI.                               */
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/base/TraceInfo.h"
#include "arccore/base/String.h"
#include "arccore/base/FatalErrorException.h"

#include "arccore/messagepassingmpi/MpiDatatype.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{
namespace MessagePassing
{
namespace Mpi
{

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MpiDatatype::
MpiDatatype(MPI_Datatype datatype)
: m_datatype(datatype)
, m_reduce_operator(new BuiltInMpiReduceOperator())
, m_is_built_in(true)
{
}

MpiDatatype::
MpiDatatype(MPI_Datatype datatype,bool is_built_in,IMpiReduceOperator* reduce_operator)
: m_datatype(datatype)
, m_reduce_operator(reduce_operator)
, m_is_built_in(is_built_in)
{
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MpiDatatype::
~MpiDatatype()
{
  if (!m_is_built_in){
    if (m_datatype!=MPI_DATATYPE_NULL)
      MPI_Type_free(&m_datatype);
  }
  m_datatype = MPI_DATATYPE_NULL;
  delete m_reduce_operator;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

MPI_Op BuiltInMpiReduceOperator::
reduceOperator(eReduceType rt)
{
  // TODO: a fusionner avec reduceOperator de StdMpiReduceOperator.
  MPI_Op op = MPI_OP_NULL;
  switch(rt){
  case ReduceMax: op = MPI_MAX; break;
  case ReduceMin: op = MPI_MIN; break;
  case ReduceSum: op = MPI_SUM; break;
  }
  if (op==MPI_OP_NULL)
    ARCCORE_FATAL("Reduce operation unknown or not implemented");
  return op;
}

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

template class StdMpiReduceOperator<float>;
template class StdMpiReduceOperator<double>;
template class StdMpiReduceOperator<long double>;

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Mpi
} // End namespace MessagePassing
} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
