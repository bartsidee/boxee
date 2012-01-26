// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeservicesmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEESERVICESMANAGER_H
#define BOXEESERVICESMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeeservices.h"
#include <SDL/SDL.h>
#include "../libjson/include/json/value.h"

namespace BOXEE
{

class CServiceIdentifierType
{
public:
  enum ServiceIdentifierTypeEnums
  {
    ID=0,
    NAME=1,
    NUM_OF_SERVICE_IDENTIFIER_TYPES=2
  };
};

class BXServicesItem : public BXItem
{
public:
  BXServicesItem(const std::string& type, const std::string& id, const std::string& name);
  virtual ~BXServicesItem(){};

private:

};

class BXServicesManager
{
public:
  BXServicesManager();
  virtual ~BXServicesManager();

  bool Initialize();
  
  bool UpdateServicesList(unsigned long executionDelayInMS, bool repeat);
  bool GetServices(BXBoxeeServices& servicesList);
  bool GetServices(std::vector<BXServicesItem>& servicesVec);
  bool GetServicesIds(std::vector<std::string>& servicesIdsVec);

  bool IsRegisterToServices(const std::string& serviceIdentifier, CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum);

  static const char* GetServiceIdentifierTypeEnumAsString(CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum);

private:

  void LockServicesList();
  void UnLockServicesList();
  void CopyServicesList(const BXBoxeeServices& servicesList);
  void SetServicesListIsLoaded(bool isLoaded);

  SDL_mutex* m_servicesListGuard;
  BXBoxeeServices m_servicesList;

  class RequestServicesListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestServicesListFromServerTask(BXServicesManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestServicesListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXServicesManager* m_taskHandler;
  };
};

}

#endif
