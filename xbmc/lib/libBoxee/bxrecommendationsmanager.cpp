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
#include "logger.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIWindowBoxeeMain.h"
#include "../../Application.h"

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
    LOG(LOG_LEVEL_ERROR,"CBoxeeClientServerComManager::UpdateRecommendationsList - FAILED to allocate RequestRecommendationsListFromServerTask (rec)");
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
  LOG(LOG_LEVEL_DEBUG,"BXRecommendationsManager::GetRecommendationsList - Enter function (shares)");

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

  recommendationsList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_recommendationsList.IsLoaded();
  long lastRetCode = recommendationsList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d] (rec)",lastRetCode,isLoaded);

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a)  recommendationsList isn't loaded   //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopyRecommendationsList(recommendationsList);

    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::DoWork - After copy going to send GUI_MSG_UPDATE to LIST_RECOMMEND and WINDOW_BOXEE_BROWSE_DISCOVER (rec)");

    int activeWindow = g_windowManager.GetActiveWindow();

    if (activeWindow == WINDOW_HOME)
    {
      CGUIMessage refreshHomeRecommendList(GUI_MSG_UPDATE, WINDOW_HOME, LIST_RECOMMEND);
      g_windowManager.SendThreadMessage(refreshHomeRecommendList);
    }

    if (activeWindow == WINDOW_BOXEE_BROWSE_DISCOVER)
    {
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
  bool IsConnectedToInternet = g_application.IsConnectedToInternet();

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - Enter function. [activeWindowId=%d][isLoaded=%d][IsConnectedToInternet=%d] (rec)",activeWindowId,isLoaded,IsConnectedToInternet);

  if (!IsConnectedToInternet)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - [hasInternetConnection=%d] -> return FALSE (rec)",IsConnectedToInternet);
    return false;
  }

  if (!isLoaded)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - [isLoaded=%d] -> return TRUE (rec)",isLoaded);
    return true;
  }

  if ((activeWindowId == WINDOW_HOME) || (activeWindowId == WINDOW_BOXEE_BROWSE_DISCOVER))
  {
    LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - [activeWindowId=%d] -> return TRUE (rec)",activeWindowId);
    return true;
  }

  LOG(LOG_LEVEL_DEBUG,"RequestRecommendationsListFromServerTask::CanExecute - return FALSE (rec)");
  return false;
}

}

