// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxboxeesubscriptionsmanager
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEESUBSCRIPTIONSMANAGER_H
#define BOXEESUBSCRIPTIONSMANAGER_H

#include "bxscheduletaskmanager.h"
#include "bxboxeesubscriptions.h"
#include <SDL/SDL.h>

namespace BOXEE
{

class CSubscriptionType
{
public:
  enum SubscriptionTypeEnums
  {
    TVSHOW_SUBSCRIPTION=0,
    RSS_SUBSCRIPTION=1,
    NUM_OF_REPORT_TO_SERVER_ACTIONS=2,
    ALL=3,
    NONE=4
  };
};

class BXSubscriptionItem : public BXItem
{
public:
  BXSubscriptionItem(const std::string& type, const std::string& id, const std::string& name, const std::string src);
  virtual ~BXSubscriptionItem(){};

  std::string GetSrc();

private:

  std::string m_src;
};

class BXSubscriptionsManager
{
public:
  BXSubscriptionsManager();
  virtual ~BXSubscriptionsManager();

  bool Initialize();
  
  bool UpdateSubscriptionsList(unsigned long executionDelayInMS, bool repeat);
  bool GetSubscriptions(BXBoxeeSubscriptions& subscriptionsList);
  bool GetSubscriptions(CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<BXSubscriptionItem>& subscriptionsVec);
  bool GetSubscriptionIds(CSubscriptionType::SubscriptionTypeEnums subscriptionType,std::vector<std::string>& subscriptionsIdsVec);

  bool IsInSubscriptions(const std::string& src);

  static std::string GetSubscriptionTypeAsStr(CSubscriptionType::SubscriptionTypeEnums subscriptionType);
  static CSubscriptionType::SubscriptionTypeEnums GetSubscriptionTypeAsEnum(const std::string& subscriptionTypeStr);

private:

  void LockSubscriptionsList();
  void UnLockSubscriptionsList();
  void CopySubscriptionsList(const BXBoxeeSubscriptions& subscriptionsList);
  void SetSubscriptionsListIsLoaded(bool isLoaded);

  bool IsSubscriptionMatchType(CSubscriptionType::SubscriptionTypeEnums subscriptionType,BXObject& obj);

  SDL_mutex* m_subscriptionsListGuard;
  BXBoxeeSubscriptions m_subscriptionsList;
  
  class RequestSubscriptionsListFromServerTask : public BoxeeScheduleTask
  {
  public:
    
    RequestSubscriptionsListFromServerTask(BXSubscriptionsManager* taskHandler, unsigned long executionDelayInMS, bool repeat);
    virtual ~RequestSubscriptionsListFromServerTask();
    virtual void DoWork();
    
  private:
    
    BXSubscriptionsManager* m_taskHandler;
  };
};

}

#endif
