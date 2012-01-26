//
// C++ Implementation: bxqueuesmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxqueuemanager.h"
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

BXQueueManager::BXQueueManager()
{
  m_queueListGuard = SDL_CreateMutex();
  m_queueList.Clear();
  m_validQueueSize = 0;
}

BXQueueManager::~BXQueueManager()
{
  SDL_DestroyMutex(m_queueListGuard);
}

bool BXQueueManager::Initialize()
{
  m_queueList.Clear();
  m_validQueueSize = 0;
  
  return true;
}

bool BXQueueManager::UpdateQueueList(unsigned long executionDelayInMS, bool repeat)
{
  RequestQueueListFromServerTask* reqQueueListTask = new RequestQueueListFromServerTask(this,executionDelayInMS,repeat);

  if(reqQueueListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockQueueList();

      m_queueList.SetLoaded(false);
      
      UnLockQueueList();
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqQueueListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::UpdateQueueList - FAILED to allocate RequestQueueListFromServerTask (queue)");
    return false;
  }
}

void BXQueueManager::LockQueueList()
{
  SDL_LockMutex(m_queueListGuard);
}

void BXQueueManager::UnLockQueueList()
{
  SDL_UnlockMutex(m_queueListGuard);
}

void BXQueueManager::CopyQueueList(const BXBoxeeFeed& queueList)
{
  LockQueueList();
  
  m_queueList = queueList;
  m_queueList.SetLoaded(true);
  
  UnLockQueueList();
}

void BXQueueManager::SetQueueListIsLoaded(bool isLoaded)
{
  LockQueueList();

  m_queueList.SetLoaded(isLoaded);

  UnLockQueueList();
}

bool BXQueueManager::GetQueueList(BXBoxeeFeed& queueList)
{
  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - Enter function (queue)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - In offline mode. Going to return FALSE (queue)");
    return false;
  }

  bool queueListWasLoaded = false;
  
  LockQueueList();

  queueListWasLoaded = m_queueList.IsLoaded();
  
  while (!queueListWasLoaded)
  {
    // QueueList ISN'T loaded yet -> UnLock the QueueList and wait for it to load
    
    UnLockQueueList();

    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - QueueList is not loaded yet. Going to try again in [%dms] (queue)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockQueueList();
      
    queueListWasLoaded = m_queueList.IsLoaded();
  }

  queueList = m_queueList;

  UnLockQueueList();

  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - Exit function. After set [QueueListSize=%d] (queue)",queueList.GetNumOfActions());

  return true;
}

int BXQueueManager::GetQueueSize()
{
  int queueSize;

  LockQueueList();

  queueSize = m_queueList.GetNumOfActions();

  UnLockQueueList();

  return queueSize;
}

void BXQueueManager::SetValidQueueSize(int validQueueSize)
{
  m_validQueueSize = validQueueSize;
}

int BXQueueManager::GetValidQueueSize()
{
  return m_validQueueSize;
}

bool BXQueueManager::IsInQueueList(const std::string& boxeeId, const std::string& path, std::string& referral)
{
  if (boxeeId.empty() && path.empty())
  {
    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - Both [boxeeId=%s][path=%s] are empty. Exit function and return FALSE (queue)",boxeeId.c_str(),path.c_str());
    return false;
  }

  bool queueListWasLoaded = false;

  LockQueueList();

  queueListWasLoaded = m_queueList.IsLoaded();

  while (!queueListWasLoaded)
  {
    // QueueList ISN'T loaded yet -> UnLock the QueueList and wait for it to load

    UnLockQueueList();

    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - QueueList is not loaded yet. Going to try again in [%dms] (queue)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockQueueList();

    queueListWasLoaded = m_queueList.IsLoaded();
  }
  
  int numOfActions = m_queueList.GetNumOfActions();
  
  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - Enter function with [boxeeId=%s][path=%s][QueueListSize=%d] (queue)",boxeeId.c_str(),path.c_str(),numOfActions);
  
  if(numOfActions < 1)
  {
    UnLockQueueList();
    LOG(LOG_LEVEL_WARNING,"BXQueueManager::IsInQueueList - [QueueListSize=%d] therefore going to return FALSE (queue)",numOfActions);
    return false;    
  }

  for (int i=0; i < numOfActions; i++)
  {
    BOXEE::BXGeneralMessage msg = m_queueList.GetAction(i);
    
    // Go over all object in message
    for (int k = 0; k < msg.GetObjectCount();k++) 
    {
      BXObject obj = msg.GetObject(k);

      if(obj.HasValue(MSG_KEY_BOXEE_ID))
      {
        if(boxeeId == obj.GetValue(MSG_KEY_BOXEE_ID))
        {
          referral = msg.GetReferral();

          UnLockQueueList();
          LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - [boxeeId=%s] WAS found in the QueueList. Exit function and return TRUE (queue)",boxeeId.c_str());
          return true;
        }
      }

      if(!path.empty() && obj.HasValue(MSG_KEY_URL))
      {
        if(path == obj.GetValue(MSG_KEY_URL))
        {
          referral = msg.GetReferral();

          UnLockQueueList();
          LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - [path=%s] WAS found in the QueueList. Exit function and return TRUE (queue)",path.c_str());
          return true;
        }
      }

      if(obj.HasValue(MSG_KEY_CLIENT_ID))
      {
        std::string strClientId = obj.GetValue(MSG_KEY_CLIENT_ID);
        std::string strLabel;
        std::string strPath;
        std::string strThumb;

        BXMetadataEngine& MDE = Boxee::GetInstance().GetMetadataEngine();
        bool succeeded = MDE.GetQueueItem(strClientId, strLabel, strPath, strThumb);

        if (succeeded)
        {
          if(path == strPath)
          {
            referral = msg.GetReferral();

            UnLockQueueList();
            LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - [path=%s] WAS found in the QueueList. [strClientId=%s]. Exit function and return TRUE (queue)",path.c_str(), strClientId.c_str());
            return true;
          }
        }
      }
    }
  }
  
  UnLockQueueList();
  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::IsInQueueList - [boxeeId=%s] or [path=%s] WASN'T found in the QueueList. Exit function and return FALSE (queue)",boxeeId.c_str(),path.c_str());

  return false;
}

bool BXQueueManager::IsInQueueList(const std::string& boxeeId, const std::string& path)
{
  std::string tmpReferral;

  return IsInQueueList(boxeeId, path, tmpReferral);
}

//////////////////////////////////////////////
// RequestQueueListFromServerTask functions //
//////////////////////////////////////////////

BXQueueManager::RequestQueueListFromServerTask::RequestQueueListFromServerTask(BXQueueManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestQueueList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXQueueManager::RequestQueueListFromServerTask::~RequestQueueListFromServerTask()
{
  
}

void BXQueueManager::RequestQueueListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - Enter function (queue)");
  
  if (!CanExecute())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetQueueListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - CanExecute() returned FALSE -> Exit function (queue)");

    return;
  }

  BXBoxeeFeed queueList;
  
  queueList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  queueList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.QueueListUrl","http://app.boxee.tv/api/get_queue");

  queueList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_queueList.IsLoaded();
  long lastRetCode = queueList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d] (queue)",lastRetCode,isLoaded);

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) queueList isn't loaded              //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopyQueueList(queueList);

    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - After copy going to send GUI_MSG_UPDATE to LIST_QUEUE and WINDOW_BOXEE_BROWSE_QUEUE (queue)");

    int activeWindow = g_windowManager.GetActiveWindow();

    if (activeWindow == WINDOW_HOME)
    {
      CGUIMessage refreshHomeQueueList(GUI_MSG_UPDATE, WINDOW_HOME, LIST_QUEUE);
      g_windowManager.SendThreadMessage(refreshHomeQueueList);
    }

    if (activeWindow == WINDOW_BOXEE_BROWSE_QUEUE)
    {
      CGUIMessage refreshQueueWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_BROWSE_QUEUE, 0, 1);
      g_windowManager.SendThreadMessage(refreshQueueWinMsg);
    }
  }

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - Exit function (queue)");

  return;
}

bool BXQueueManager::RequestQueueListFromServerTask::CanExecute()
{
  int activeWindowId = g_windowManager.GetActiveWindow();
  bool isLoaded = m_taskHandler->m_queueList.IsLoaded();
  bool IsConnectedToInternet = g_application.IsConnectedToInternet();

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - Enter function. [activeWindowId=%d][isLoaded=%d][IsConnectedToInternet=%d] (queue)",activeWindowId,isLoaded,IsConnectedToInternet);

  if (!IsConnectedToInternet)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - [hasInternetConnection=%d] -> return FALSE (queue)",IsConnectedToInternet);
    return false;
  }

  if (!isLoaded)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - [isLoaded=%d] -> return TRUE (queue)",isLoaded);
    return true;
  }

  if ((activeWindowId == WINDOW_HOME) || (activeWindowId == WINDOW_BOXEE_BROWSE_QUEUE))
  {
    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - [activeWindowId=%d] -> return TRUE (queue)",activeWindowId);
    return true;
  }

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - return FALSE (queue)");
  return false;
}

}

