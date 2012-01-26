// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeeentitlementsmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEENTITLEMENTSMANAGER_H
#define BOXEEENTITLEMENTSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeeentitlements.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class BXEntitlementsItem : public BXItem
{
public:
  BXEntitlementsItem(const std::string& type, const std::string& id, const std::string& name, const std::string& entitlementId, const std::string& userIndex, const std::string& productId);
  virtual ~BXEntitlementsItem(){};

  std::string GetEntitlementId(){return m_entitlementId;};
  std::string GetUserIndex(){return m_userIndex;};
  std::string GetProductId(){return m_productId;};

private:

  std::string m_entitlementId;
  std::string m_userIndex;
  std::string m_productId;
};

class BXEntitlementsManager
{
public:
  BXEntitlementsManager();
  virtual ~BXEntitlementsManager();

  bool Initialize();
  
  bool UpdateEntitlementsList(unsigned long executionDelayInMS, bool repeat);
  bool GetEntitlements(BXBoxeeEntitlements& entitlementsList);
  bool GetEntitlements(std::vector<BXEntitlementsItem>& entitlementsVec);
  bool GetEntitlementsIds(std::vector<std::string>& entitlementsIdsVec);

  bool IsInEntitlements(const std::string& productsList);

private:

  void LockEntitlementsList();
  void UnLockEntitlementsList();
  void CopyEntitlementsList(const BXBoxeeEntitlements& entitlementsList);
  void SetEntitlementsListIsLoaded(bool isLoaded);

  SDL_mutex* m_entitlementsListGuard;
  BXBoxeeEntitlements m_entitlementsList;
  
  class RequestEntitlementsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestEntitlementsListFromServerTask(BXEntitlementsManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestEntitlementsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXEntitlementsManager* m_taskHandler;
  };
};

}

#endif
