#include "HalServices.h"

#ifdef HAS_BOXEE_HAL

#ifndef _WIN32
#include "BoxeeHalServices.h"
#endif
#include "NullHalServices.h"

IHalServices& CHalServicesFactory::GetInstance()
{
#ifdef _WIN32
  return CNullHalServices::GetInstance();
#else
  return CBoxeeHalServices::GetInstance();
#endif
};

#endif
