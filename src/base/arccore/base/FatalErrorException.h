/*---------------------------------------------------------------------------*/
/* FatalErrorException.h                                       (C) 2000-2018 */
/*                                                                           */
/* Exception lorsqu'une erreur fatale est survenue.                          */
/*---------------------------------------------------------------------------*/
#ifndef ARCCORE_BASE_FATALERROREXCEPTION_H
#define ARCCORE_BASE_FATALERROREXCEPTION_H
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#include "arccore/base/Exception.h"

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

namespace Arccore
{
/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/
/*!
 * \ingroup Core
 * \brief Exception lorsqu'une erreur fatale est survenue.
 */
class ARCCORE_BASE_EXPORT FatalErrorException
: public Exception
{
 public:
	
  explicit FatalErrorException(const String& where);
  explicit FatalErrorException(const TraceInfo& where);
  FatalErrorException(const String& where,const String& message);
  FatalErrorException(const TraceInfo& where,const String& message);
  FatalErrorException(const FatalErrorException& rhs) ARCCORE_NOEXCEPT;
  ~FatalErrorException() ARCCORE_NOEXCEPT {}

 public:
	
  void explain(std::ostream& m) const override;

 private:
};

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

} // End namespace Arccore

/*---------------------------------------------------------------------------*/
/*---------------------------------------------------------------------------*/

#endif  
