//
// C++ Implementation: bxrecommendationsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxrecommendationsmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "bxutils.h"
#include "logger.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "../../Application.h"

#define MAX_NUMBER_OF_ITEMS_TO_REQ              200

namespace BOXEE
{

BXRecommendationsManager::BXRecommendationsManager()
{
  m_recommendationsListGuard = SDL_CreateMutex();
  m_recommendationsList.Clear();
}

BXRecommendationsManager::~BXRecommendationsManager()
{
  SDL_DestroyMutex(m_recommendationsListGuard);
}

bool BXRecommendationsManager::Initialize()
{
  m_recommendationsList.Clear();
  
  return true;
}

bool BXRecommendationsManager::UpdateRecommendationsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestRecommendationsListFromServerTask* reqRecommendationsListTask = new RequestRecommendationsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqRecommendationsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockRecommendationsList();

      m_recommendationsList.SetLoaded(false);
      
      UnLockRecommendationsList();      
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqRecommendationsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXRecommendationsManager::UpdateRecommendationsList - FAILED to allocate RequestRecommendationsListFromServerTask (rec)");
    return false;
  }
}

void BXRecommendationsManager::LockRecommendationsList()
{
  SDL_LockMutex(m_recommendationsListGuard);
}

void BXRecommendationsManager::UnLockRecommendationsList()
{
  SDL_UnlockMutex(m_recommendationsListGuard);
}

void BXRecommendationsManager::CopyRecommendationsList(const BXBoxeeFeed& recommendationsList)
{
  LockRecommendationsList();
  
  LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::CopyRecommendationsList - going to copy recommendationsList [size=%d]. [currSize=%d] (rec)",recommendationsList.GetNumOfActions(),m_recommendationsList.GetNumOfActions());

  m_recommendationsList = recommendationsList;
  m_recommendationsList.SetLoaded(true);
  
  UnLockRecommendationsList();
}

void BXRecommendationsManager::SetRecommendationsListIsLoaded(bool isLoaded)
{
  LockRecommendationsList();

  m_recommendationsList.SetLoaded(isLoaded);

  UnLockRecommendationsList();
}

bool BXRecommendationsManager::GetRecommendationsList(BXBoxeeFeed& recommendationsList)
{
  LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsList - Enter function (rec)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsList - In offline mode. Going to return FALSE (rec)");
    return false;
  }

  bool recommendationsListWasLoaded = false;
  
  LockRecommendationsList();

  recommendationsListWasLoaded = m_recommendationsList.IsLoaded();
  
  while (!recommendationsListWasLoaded)
  {
    // RecommendationsList ISN'T loaded yet -> UnLock the RecommendationsList and wait for it to load
    
    UnLockRecommendationsList();

    LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsList - RecommendationsList is not loaded yet. Going to try again in [%dms] (rec)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockRecommendationsList();
      
    recommendationsListWasLoaded = m_recommendationsList.IsLoaded();      
  }

  recommendationsList = m_recommendationsList;

  UnLockRecommendationsList();

  LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsList - Exit function. After set [RecommendationsListSize=%d] (rec)",recommendationsList.GetNumOfActions());

  return true;
}

int BXRecommendationsManager::GetRecommendationsListSize()
{
  int recommendationsSize = 0;

  LockRecommendationsList();

  BXBoxeeFeed recommendationsList;

  if (GetRecommendationsList(recommendationsList))
  {
    recommendationsSize = recommendationsList.GetNumOfActions();

    LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsListSize - return [size=%d] (recommendations)",recommendationsSize);
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXRecommendationsManager::GetRecommendationsListSize - FAILED to get size(recommendations)");
  }

  UnLockRecommendationsList();

  return recommendationsSize;
}

////////////////////////////////////////////////////////
// RequestRecommendationsListFromServerTask functions //
////////////////////////////////////////////////////////

BXRecommendationsManager::RequestRecommendationsListFromServerTask::RequestRecommendationsListFromServerTask(BXRecommendationsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestRecommendationsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXRecommendationsManager::RequestRecommendationsListFromServerTask::~RequestRecommendationsListFromServerTask()
{
  
}

void BXRecommendationsManager::RequestRecommendationsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - Enter function (rec)");

  if (!CanExecute())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetRecommendationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - CanExecute() returned FALSE -> Exit function (rec)");
    return;
  }

  BXBoxeeFeed recommendationsList;

  recommendationsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  recommendationsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.RecommendationsListUrl","http://app.boxee.tv/api/get_recommendations");

  strUrl += "?last=0&count=";
  strUrl += BXUtils::IntToString(MAX_NUMBER_OF_ITEMS_TO_REQ);

  recommendationsList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_recommendationsList.IsLoaded();
  long lastRetCode = recommendationsList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (rec)",lastRetCode,isLoaded,recommendationsList.GetNumOfActions(),m_taskHandler->m_recommendationsList.GetNumOfActions());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) recommendationsList isn't loaded    //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopyRecommendationsList(recommendationsList);

    int activeWindowId = g_windowManager.GetActiveWindow();
    if (!isLoaded && activeWindowId == WINDOW_BOXEE_BROWSE_DISCOVER)
    {
      LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - since [isLoaded=%d=FALSE] and [activeWindowId=%d=WINDOW_BOXEE_BROWSE_DISCOVER] sending GUI_MSG_UPDATE to WINDOW_BOXEE_BROWSE_DISCOVER (rec)",isLoaded,activeWindowId);

      CGUIMessage refreshDiscoverWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_DISCOVER, 0, 1);
      g_windowManager.SendThreadMessage(refreshDiscoverWinMsg);
    }
  }

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - Exit function (rec)");

  return;
}

bool BXRecommendationsManager::RequestRecommendationsListFromServerTask::CanExecute()
{
  int activeWindowId = g_windowManager.GetActiveWindow();
  bool isLoaded = m_taskHandler->m_recommendationsList.IsLoaded();

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - Enter function. [activeWindowId=%d][isLoaded=%d] (rec)",activeWindowId,isLoaded);

  if (!g_application.ShouldConnectToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - [ShouldConnectToInternet=FALSE] -> return FALSE (rec)");
    return false;
  }

  if (!isLoaded)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - [isLoaded=%d] -> return TRUE (rec)",isLoaded);
    return true;
  }

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - return TRUE. [ActiveWindowId=%d] (rec)",activeWindowId);
  return true;
}

}

