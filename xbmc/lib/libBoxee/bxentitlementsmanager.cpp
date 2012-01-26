//
// C++ Implementation: bxentitlementsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxentitlementsmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"
#include "../../BoxeeUtils.h"

#define ENTITLEMENTS_DELIMITER ";"

namespace BOXEE
{

BXEntitlementsManager::BXEntitlementsManager()
{
  m_entitlementsListGuard = SDL_CreateMutex();
  m_entitlementsList.Clear();
}

BXEntitlementsManager::~BXEntitlementsManager()
{
  SDL_DestroyMutex(m_entitlementsListGuard);
  
}

bool BXEntitlementsManager::Initialize()
{
  m_entitlementsList.Clear();
  
  return true;
}

bool BXEntitlementsManager::UpdateEntitlementsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestEntitlementsListFromServerTask* reqEntitlementsListTask = new RequestEntitlementsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqEntitlementsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockEntitlementsList();

      m_entitlementsList.SetLoaded(false);
      
      UnLockEntitlementsList();
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqEntitlementsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXEntitlementsManager::UpdateEntitlementsList - FAILED to allocate RequestEntitlementsListFromServerTask (entitle)");
    return false;
  }
}

void BXEntitlementsManager::LockEntitlementsList()
{
  SDL_LockMutex(m_entitlementsListGuard);
}

void BXEntitlementsManager::UnLockEntitlementsList()
{
  SDL_UnlockMutex(m_entitlementsListGuard);
}

void BXEntitlementsManager::CopyEntitlementsList(const BXBoxeeEntitlements& entitlementsList)
{
  LockEntitlementsList();
  
  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::CopyRecommendationsList - going to copy entitlementsList [size=%d]. [currentSize=%d] (entitle)",entitlementsList.GetNumOfEntitlements(),m_entitlementsList.GetNumOfEntitlements());

  m_entitlementsList = entitlementsList;
  m_entitlementsList.SetLoaded(true);
  
  UnLockEntitlementsList();
}

void BXEntitlementsManager::SetEntitlementsListIsLoaded(bool isLoaded)
{
  LockEntitlementsList();

  m_entitlementsList.SetLoaded(isLoaded);

  UnLockEntitlementsList();
}

bool BXEntitlementsManager::GetEntitlements(BXBoxeeEntitlements& entitlementsList)
{
  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlementsList - Enter function (entitle)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlementsList - In offline mode. Going to return FALSE (entitle)");
    return false;
  }

  bool entitlementsListWasLoaded = false;
  
  LockEntitlementsList();

  entitlementsListWasLoaded = m_entitlementsList.IsLoaded();
  
  while (!entitlementsListWasLoaded)
  {
    // EntitlementsList ISN'T loaded yet -> UnLock the EntitlementsList and wait for it to load
    
    UnLockEntitlementsList();

    LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlementsList - EntitlementsList is not loaded yet. Going to try again in [%dms] (entitle)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockEntitlementsList();
      
    entitlementsListWasLoaded = m_entitlementsList.IsLoaded();
  }

  entitlementsList = m_entitlementsList;

  UnLockEntitlementsList();

  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlementsList - Exit function. After set [EntitlementsListSize=%d] (entitle)",entitlementsList.GetNumOfEntitlements());

  return true;
}

bool BXEntitlementsManager::GetEntitlements(std::vector<BXEntitlementsItem>& entitlementsVec)
{
  BXBoxeeEntitlements entitlements;
  
  GetEntitlements(entitlements);
  
  int numOfEntitlements = entitlements.GetNumOfEntitlements();
  
  if(numOfEntitlements < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXEntitlementsManager::GetEntitlements - Call to GetEntitlements returned [EntitlementsListSize=%d] (entitle)",numOfEntitlements);
    return false;
  }
  
  for(int i=0; i<numOfEntitlements; i++)
  {
    BXObject obj = entitlements.GetEntitlement(i);
    
    std::string type = obj.GetType();
    std::string id = obj.GetID();
    std::string name = obj.GetValue(MSG_KEY_NAME);
    std::string entitlementId = obj.GetValue("entitlement_id");
    std::string userIndex = obj.GetValue("user_index");
    std::string productId = obj.GetValue("product_id");

    BXEntitlementsItem entitlementsItem(type,id,name,entitlementId,userIndex,productId);
    
    entitlementsVec.push_back(entitlementsItem);
  }

  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlements - Going to return  [EntitlementsListSize=%d] (entitle)",entitlementsVec.size());

  return true;
}

bool BXEntitlementsManager::GetEntitlementsIds(std::vector<std::string>& entitlementsIdsVec)
{
  BXBoxeeEntitlements entitlements;

  GetEntitlements(entitlements);

  int numOfEntitlements = entitlements.GetNumOfEntitlements();

  if(numOfEntitlements < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXEntitlementsManager::GetEntitlementsIds - Call to GetEntitlements returned [EntitlementsListSize=%d] (entitle)",numOfEntitlements);
    return false;
  }

  for(int i=0; i<numOfEntitlements; i++)
  {
    BXObject obj = entitlements.GetEntitlement(i);

    entitlementsIdsVec.push_back(obj.GetID());
  }

  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::GetEntitlementsIds - Going to return  [entitlementsIdsVecSize=%d] (entitle)",entitlementsIdsVec.size());

  return true;
}

bool BXEntitlementsManager::IsInEntitlements(const std::string& productsList)
{
  if (productsList.empty())
  {
    LOG(LOG_LEVEL_WARNING,"BXEntitlementsManager::IsInEntitlements - Enter with an EMPTY productsList therefore going to return FALSE. [productsList=%s] (entitle)",productsList.c_str());
    return false;
  }

  bool entitlementsListWasLoaded = false;

  LockEntitlementsList();

  entitlementsListWasLoaded = m_entitlementsList.IsLoaded();

  while (!entitlementsListWasLoaded)
  {
    // EntitlementsList ISN'T loaded yet -> UnLock the EntitlementsList and wait for it to load

    UnLockEntitlementsList();

    LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::IsInEntitlements - EntitlementsList is not loaded yet. Going to try again in [%dms] (entitle)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockEntitlementsList();

    entitlementsListWasLoaded = m_entitlementsList.IsLoaded();
  }

  int numOfEntitlements = m_entitlementsList.GetNumOfEntitlements();

  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::IsInEntitlements - Enter function with [productsList=%s][EntitlementsListSize=%d] (entitle)",productsList.c_str(),numOfEntitlements);

  if(numOfEntitlements < 1)
  {
    UnLockEntitlementsList();
    LOG(LOG_LEVEL_WARNING,"BXEntitlementsManager::IsInEntitlements - [EntitlementsListSize=%d] therefore going to return FALSE (entitle)",numOfEntitlements);
    return false;
  }

  std::set<CStdString> productsSet;
  int numOfProducts = BoxeeUtils::StringTokenize(productsList,productsSet,ENTITLEMENTS_DELIMITER,true,true);
  if (numOfProducts < 1)
  {
    UnLockEntitlementsList();
    LOG(LOG_LEVEL_WARNING,"BXEntitlementsManager::IsInEntitlements - Tokenize of [productsList=%s] return [size=%d] therefore going to return FALSE (entitle)",productsList.c_str(),numOfProducts);
    return false;
  }

  for (int i=0; i < numOfEntitlements; i++)
  {
    BXObject product = m_entitlementsList.GetEntitlement(i);

    std::string productId = product.GetValue("product_id");
    if(productsSet.find(productId) != productsSet.end())
    {
      UnLockEntitlementsList();
      LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::IsInEntitlements - [productId=%s] WAS found in the EntitlementsList. Exit function and return TRUE (entitle)",productId.c_str());
      return true;
    }
  }

  UnLockEntitlementsList();
  LOG(LOG_LEVEL_DEBUG,"BXEntitlementsManager::IsInEntitlements - None of [productsList=%s] was found in the EntitlementsList. Exit function and return FALSE (entitle)",productsList.c_str());
  return false;
}

/////////////////////////////////////////////////
// RequestEntitlementsListFromServerTask functions //
/////////////////////////////////////////////////

BXEntitlementsManager::RequestEntitlementsListFromServerTask::RequestEntitlementsListFromServerTask(BXEntitlementsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestEntitlementsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXEntitlementsManager::RequestEntitlementsListFromServerTask::~RequestEntitlementsListFromServerTask()
{
  
}

void BXEntitlementsManager::RequestEntitlementsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestEntitlementsListFromServerTask::DoWork - Enter function (entitle)");
  
  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetEntitlementsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestEntitlementsListFromServerTask::DoWork - [IsConnectedToInternet=%d=FALSE] -> Exit function (entitle)");
    return;
  }

  BXBoxeeEntitlements entitlementsList;
  
  entitlementsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  entitlementsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.EntitlementsListUrl","http://app.boxee.tv/api/getentitlements");

  entitlementsList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_entitlementsList.IsLoaded();
  long lastRetCode = entitlementsList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestEntitlementsListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currentSize=%d] (entitle)",lastRetCode,isLoaded,entitlementsList.GetNumOfEntitlements(),m_taskHandler->m_entitlementsList.GetNumOfEntitlements());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) recommendationsList isn't loaded    //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopyEntitlementsList(entitlementsList);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestEntitlementsListFromServerTask::DoWork - Exit function (entitle)");

  return;
}

//////////////////////////////////
// BXEntitlementsItem functions //
//////////////////////////////////

BXEntitlementsItem::BXEntitlementsItem(const std::string& type, const std::string& id, const std::string& name, const std::string& entitlementId, const std::string& userIndex, const std::string& productId) : BXItem(type,id,name)
{
  m_entitlementId = entitlementId;
  m_userIndex = userIndex;
  m_productId = productId;
}

}

