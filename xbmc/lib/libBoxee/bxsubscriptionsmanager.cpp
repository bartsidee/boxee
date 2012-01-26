//
// C++ Implementation: bxsubscriptionsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxsubscriptionsmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"

namespace BOXEE
{

BXSubscriptionsManager::BXSubscriptionsManager()
{
  m_subscriptionsListGuard = SDL_CreateMutex();
  m_subscriptionsList.Clear();
}

BXSubscriptionsManager::~BXSubscriptionsManager()
{
  SDL_DestroyMutex(m_subscriptionsListGuard);
  
}

bool BXSubscriptionsManager::Initialize()
{
  m_subscriptionsList.Clear();
  
  return true;
}

bool BXSubscriptionsManager::UpdateSubscriptionsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestSubscriptionsListFromServerTask* reqSubscriptionsListTask = new RequestSubscriptionsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqSubscriptionsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockSubscriptionsList();

      m_subscriptionsList.SetLoaded(false);
      
      UnLockSubscriptionsList();      
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqSubscriptionsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXSubscriptionsManager::UpdateSubscriptionsList - FAILED to allocate RequestSubscriptionsListFromServerTask (subs)");
    return false;
  }
}

void BXSubscriptionsManager::LockSubscriptionsList()
{
  SDL_LockMutex(m_subscriptionsListGuard);
}

void BXSubscriptionsManager::UnLockSubscriptionsList()
{
  SDL_UnlockMutex(m_subscriptionsListGuard);
}

void BXSubscriptionsManager::CopySubscriptionsList(const BXBoxeeSubscriptions& subscriptionsList)
{
  LockSubscriptionsList();

  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::CopySubscriptionsList - going to copy subscriptionsList [size=%d]. [currSize=%d] (subs)",subscriptionsList.GetNumOfSubscriptions(),m_subscriptionsList.GetNumOfSubscriptions());

  m_subscriptionsList = subscriptionsList;
  m_subscriptionsList.SetLoaded(true);
  
  UnLockSubscriptionsList();
}

void BXSubscriptionsManager::SetSubscriptionsListIsLoaded(bool isLoaded)
{
  LockSubscriptionsList();

  m_subscriptionsList.SetLoaded(isLoaded);

  UnLockSubscriptionsList();
}

bool BXSubscriptionsManager::GetSubscriptions(BXBoxeeSubscriptions& subscriptionsList)
{
  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionsList - Enter function (subs)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionsList - In offline mode. Going to return FALSE (subs)");
    return false;
  }

  bool subscriptionsListWasLoaded = false;
  
  LockSubscriptionsList();

  subscriptionsListWasLoaded = m_subscriptionsList.IsLoaded();
  
  while (!subscriptionsListWasLoaded)
  {
    // SubscriptionsList ISN'T loaded yet -> UnLock the SubscriptionsList and wait for it to load
    
    UnLockSubscriptionsList();

    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionsList - SubscriptionsList is not loaded yet. Going to try again in [%dms] (subs)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockSubscriptionsList();
      
    subscriptionsListWasLoaded = m_subscriptionsList.IsLoaded();
  }

  subscriptionsList = m_subscriptionsList;

  UnLockSubscriptionsList();

  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionsList - Exit function. After set [SubscriptionsListSize=%d] (subs)",subscriptionsList.GetNumOfSubscriptions());

  return true;
}

bool BXSubscriptionsManager::GetSubscriptions(CSubscriptionType::SubscriptionTypeEnums subscriptionType, std::vector<BXSubscriptionItem>& subscriptionsVec)
{
  BXBoxeeSubscriptions subscriptions;
  
  GetSubscriptions(subscriptions);
  
  int numOfSubscriptions = subscriptions.GetNumOfSubscriptions();
  
  if(numOfSubscriptions < 1)
  {
    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptions - Call to GetSubscriptionsList returned [SubscriptionsListSize=%d] (subs)",numOfSubscriptions);
    return false;
  }
  
  for(int i=0; i<numOfSubscriptions; i++)
  {
    BXObject obj = subscriptions.GetSubscription(i);
    
    if(IsSubscriptionMatchType(subscriptionType,obj))
    {
      std::string type = obj.GetType();
      std::string id = obj.GetID();
      std::string name = obj.GetValue(MSG_KEY_NAME);
      std::string src = obj.GetValue(MSG_KEY_SRC);
    
      BXSubscriptionItem subscriptionItem(type,id,name,src);
    
      subscriptionsVec.push_back(subscriptionItem);
    }
  }

  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptions - Going to return  [SubscriptionsVecSize=%d] (subs)",(int)subscriptionsVec.size());

  return true;
}

bool BXSubscriptionsManager::GetSubscriptionIds(CSubscriptionType::SubscriptionTypeEnums subscriptionType, std::vector<std::string>& subscriptionsIdsVec)
{
  BXBoxeeSubscriptions subscriptions;

  GetSubscriptions(subscriptions);

  int numOfSubscriptions = subscriptions.GetNumOfSubscriptions();

  if(numOfSubscriptions < 1)
  {
    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionIds - Call to GetSubscriptionsList returned [SubscriptionsListSize=%d] (subs)",numOfSubscriptions);
    return false;
  }

  for(int i=0; i<numOfSubscriptions; i++)
  {
    BXObject obj = subscriptions.GetSubscription(i);

    if(IsSubscriptionMatchType(subscriptionType,obj))
    {
      subscriptionsIdsVec.push_back(obj.GetID());
    }
  }

  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::GetSubscriptionIds - Going to return  [SubscriptionsIdVecSize=%d] (subs)",(int)subscriptionsIdsVec.size());

  return true;
}

bool BXSubscriptionsManager::IsInSubscriptions(const std::string& src)
{
  bool subscriptionsListWasLoaded = false;

  LockSubscriptionsList();

  subscriptionsListWasLoaded = m_subscriptionsList.IsLoaded();

  while (!subscriptionsListWasLoaded)
  {
    // SubscriptionsList ISN'T loaded yet -> UnLock the SubscriptionsList and wait for it to load

    UnLockSubscriptionsList();

    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::IsInSubscriptions - SubscriptionsList is not loaded yet. Going to try again in [%dms] (subs)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockSubscriptionsList();

    subscriptionsListWasLoaded = m_subscriptionsList.IsLoaded();
  }

  int numOfSubscriptions = m_subscriptionsList.GetNumOfSubscriptions();

  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::IsInSubscriptions - Enter function with [src=%s][SubscriptionsListSize=%d] (subs)",src.c_str(),numOfSubscriptions);

  if(numOfSubscriptions < 1)
  {
    UnLockSubscriptionsList();
    LOG(LOG_LEVEL_WARNING,"BXSubscriptionsManager::IsInSubscriptions - [SubscriptionsListSize=%d] therefore going to return FALSE (subs)",numOfSubscriptions);
    return false;
  }

  for (int i=0; i < numOfSubscriptions; i++)
  {
    BXObject app = m_subscriptionsList.GetSubscription(i);

    if(app.GetID() == src)
    {
      UnLockSubscriptionsList();
      LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::IsInSubscriptions - [src=%s] WAS found in the SubscriptionsList. Exit function and return TRUE (subs)",src.c_str());
      return true;
    }
  }

  UnLockSubscriptionsList();
  LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::IsInSubscriptions - [src=%s] WASN'T found in the SubscriptionsList. Exit function and return FALSE (subs)",src.c_str());
  return false;
}

bool BXSubscriptionsManager::IsSubscriptionMatchType(CSubscriptionType::SubscriptionTypeEnums subscriptionType,BXObject& obj)
{
  bool match = false;

  switch(subscriptionType)
  {
  case CSubscriptionType::ALL:
    match = true;
    break;
  case CSubscriptionType::TVSHOW_SUBSCRIPTION:
    if(obj.GetType() == "tvshow-subscription")
    {
      match = true;
    }
    break;
  case CSubscriptionType::RSS_SUBSCRIPTION:
    if(obj.GetType() == "rss-subscription")
    {
      match = true;
    }
    break;
  default:
    LOG(LOG_LEVEL_DEBUG,"BXSubscriptionsManager::IsSubscriptionMatchType - Type [%d] is unknown (subs)",subscriptionType);
    break;
  }

  return match;
}

std::string BXSubscriptionsManager::GetSubscriptionTypeAsStr(CSubscriptionType::SubscriptionTypeEnums subscriptionType)
{
  std::string subscriptionTypeStr = "";

  switch(subscriptionType)
  {
  case CSubscriptionType::TVSHOW_SUBSCRIPTION:
    subscriptionTypeStr = "tvshow-subscription";
    break;
  case CSubscriptionType::RSS_SUBSCRIPTION:
    subscriptionTypeStr = "rss-subscription";
    break;
  default:
    break;
  }

  return subscriptionTypeStr;
}

CSubscriptionType::SubscriptionTypeEnums BXSubscriptionsManager::GetSubscriptionTypeAsEnum(const std::string& subscriptionTypeStr)
{
  if(subscriptionTypeStr == "tvshow-subscription")
  {
    return CSubscriptionType::TVSHOW_SUBSCRIPTION;
  }
  else if (subscriptionTypeStr == "rss-subscription")
  {
    return CSubscriptionType::RSS_SUBSCRIPTION;
  }
  else
  {
    return CSubscriptionType::NONE;
  }
}

//////////////////////////////////////////////////////
// RequestSubscriptionsListFromServerTask functions //
//////////////////////////////////////////////////////

BXSubscriptionsManager::RequestSubscriptionsListFromServerTask::RequestSubscriptionsListFromServerTask(BXSubscriptionsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestSubscriptionsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXSubscriptionsManager::RequestSubscriptionsListFromServerTask::~RequestSubscriptionsListFromServerTask()
{
  
}

void BXSubscriptionsManager::RequestSubscriptionsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestSubscriptionsListFromServerTask::DoWork - Enter function (subs)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetSubscriptionsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestSubscriptionsListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (subs)");
    return;
  }

  BXBoxeeSubscriptions subscriptionsList;
  
  subscriptionsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  subscriptionsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.SubscriptionsListUrl","http://app.boxee.tv/api/get_subscriptions");

  subscriptionsList.LoadFromURL(strUrl);
  
  bool isLoaded = m_taskHandler->m_subscriptionsList.IsLoaded();
  long lastRetCode = subscriptionsList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestSubscriptionsListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (subs)",lastRetCode,isLoaded,subscriptionsList.GetNumOfSubscriptions(),m_taskHandler->m_subscriptionsList.GetNumOfSubscriptions());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) recommendationsList isn't loaded    //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopySubscriptionsList(subscriptionsList);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestSubscriptionsListFromServerTask::DoWork - Exit function (subs)");

  return;
}

//////////////////////////////////
// BXSubscriptionItem functions //
//////////////////////////////////

BXSubscriptionItem::BXSubscriptionItem(const std::string& type, const std::string& id, const std::string& name, const std::string src) : BXItem(type,id,name)
{
  m_src = src;
}

std::string BXSubscriptionItem::GetSrc()
{
  return m_src;
}

}

