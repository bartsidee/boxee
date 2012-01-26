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
#include "bxutils.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "../../Application.h"

namespace BOXEE
{

BXQueueManager::BXQueueManager()
{
  m_queueListGuard = SDL_CreateMutex();

  m_queueList.Clear();
  m_queueClipList.Clear();
  m_queueMovieList.Clear();
  m_queueTvList.Clear();

  m_validQueueSize = 0;
}

BXQueueManager::~BXQueueManager()
{
  SDL_DestroyMutex(m_queueListGuard);
}

bool BXQueueManager::Initialize()
{
  m_queueList.Clear();
  m_queueClipList.Clear();
  m_queueMovieList.Clear();
  m_queueTvList.Clear();

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
      m_queueClipList.SetLoaded(false);
      m_queueMovieList.SetLoaded(false);
      m_queueTvList.SetLoaded(false);
      
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

void BXQueueManager::CopyQueueList(const BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType)
{
  LockQueueList();
  
  BXBoxeeFeed* qList = GetQueueListByType(queueType);

  if (qList)
  {
    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::CopyQueueList - going to copy queueList [queueType=%d=%s][size=%d]. [currSize=%d] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str(),queueList.GetNumOfActions(),GetQueueSize(queueType));

    *qList = queueList;
    qList->SetLoaded(true);
  }
  
  UnLockQueueList();
}

BXBoxeeFeed* BXQueueManager::GetQueueListByType(CQueueItemsType::QueueItemsTypeEnums queueType)
{
  BXBoxeeFeed* qList = NULL;

  switch (queueType)
  {
  case CQueueItemsType::QIT_ALL:
  {
    qList = &m_queueList;
  }
  break;
  case CQueueItemsType::QIT_CLIP:
  {
    qList = &m_queueClipList;
  }
  break;
  case CQueueItemsType::QIT_MOVIE:
  {
    qList = &m_queueMovieList;
  }
  break;
  case CQueueItemsType::QIT_TVSHOW:
  {
    qList = &m_queueTvList;
  }
  break;
  default:
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::GetQueueListByType - FAILED to set queueList for [queueType=%d] (queue)",queueType);
  }
  break;
  }

  return qList;
}

void BXQueueManager::SetQueueListIsLoaded(CQueueItemsType::QueueItemsTypeEnums queueType, bool isLoaded)
{
  LockQueueList();

  BXBoxeeFeed* qList = GetQueueListByType(queueType);

  if (qList)
  {
    qList->SetLoaded(isLoaded);
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::SetQueueListIsLoaded - FAILED to set Loaded for queueList [queueType=%d] (queue)",queueType);
  }

  UnLockQueueList();
}

bool BXQueueManager::GetQueueList(BXBoxeeFeed& queueList, CQueueItemsType::QueueItemsTypeEnums queueType)
{
  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - Enter function. [queueType=%d=%s] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - In offline mode. Going to return FALSE. [queueType=%d=%s] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());
    return false;
  }

  bool queueListWasLoaded = false;
  
  LockQueueList();

  queueListWasLoaded = IsLoaded(queueType);
  
  while (!queueListWasLoaded)
  {
    // QueueList ISN'T loaded yet -> UnLock the QueueList and wait for it to load
    
    UnLockQueueList();

    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - QueueList [queueType=%d=%s] is not loaded yet. Going to try again in [%dms] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str(),DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockQueueList();
      
    queueListWasLoaded = IsLoaded(queueType);
  }

  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - going to copy queue [queueType=%d=%s] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());

  BXBoxeeFeed* qList = GetQueueListByType(queueType);

  if (qList)
  {
    queueList = *qList;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::GetQueueList - FAILED to get queueList [queueType=%d=%s] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());
  }

  UnLockQueueList();

  LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueList - Exit function. After copy [QueueListSize=%d]. [queueType=%d=%s] (queue)",queueList.GetNumOfActions(),queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());

  return true;
}

int BXQueueManager::GetQueueSize(CQueueItemsType::QueueItemsTypeEnums queueType)
{
  int queueSize = 0;

  LockQueueList();

  BXBoxeeFeed* qList = GetQueueListByType(queueType);

  if (qList)
  {
    queueSize = qList->GetNumOfActions();

    LOG(LOG_LEVEL_DEBUG,"BXQueueManager::GetQueueSize - for queueList [queueType=%d=%s] return [size=%d] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str(),queueSize);
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::GetQueueSize - FAILED to get queueList [queueType=%d=%s] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId(queueType).c_str());
  }

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

      if(!path.empty() && obj.HasValue(MSG_KEY_CLIENT_ID))
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

bool BXQueueManager::IsLoaded(CQueueItemsType::QueueItemsTypeEnums queueType)
{
  bool isLoaded = false;

  switch (queueType)
  {
  case CQueueItemsType::QIT_ALL:
  {
    isLoaded = m_queueList.IsLoaded();
  }
  break;
  case CQueueItemsType::QIT_CLIP:
  {
    isLoaded = m_queueClipList.IsLoaded();
  }
  break;
  case CQueueItemsType::QIT_MOVIE:
  {
    isLoaded = m_queueMovieList.IsLoaded();
  }
  break;
  case CQueueItemsType::QIT_TVSHOW:
  {
    isLoaded = m_queueTvList.IsLoaded();
  }
  break;
  default:
  {
    LOG(LOG_LEVEL_ERROR,"BXQueueManager::IsLoaded - FAILED to get IsLoaded for [queueType=%d] (queue)",queueType);
  }
  break;
  }

  return isLoaded;
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
    for (int queueType = CQueueItemsType::QIT_ALL; queueType < CQueueItemsType::NUM_OF_QUEUE_ITEMS_TYPE; queueType++)
    {
      m_taskHandler->SetQueueListIsLoaded((CQueueItemsType::QueueItemsTypeEnums)queueType,true);
    }

    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - CanExecute() returned FALSE -> Exit function (queue)");

    return;
  }

  for (int queueType = CQueueItemsType::QIT_CLIP; queueType < CQueueItemsType::NUM_OF_QUEUE_ITEMS_TYPE; queueType++)
  {
    BXBoxeeFeed queueList;
    queueList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
    queueList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

    std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.QueueListUrl","http://app.boxee.tv/api/get_queue");

    std::string typeId = CQueueItemsType::GetQueueItemTypeAsStringId((CQueueItemsType::QueueItemsTypeEnums)queueType);

    if (!typeId.empty())
    {
      strUrl += "?ot=";
      strUrl += typeId;
    }

    queueList.LoadFromURL(strUrl);

    bool isLoaded = m_taskHandler->IsLoaded((CQueueItemsType::QueueItemsTypeEnums)queueType);
    long lastRetCode = queueList.GetLastRetCode();

    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - for [typeId=%d=%s]. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (queue)",queueType,CQueueItemsType::GetQueueItemTypeAsStringId((CQueueItemsType::QueueItemsTypeEnums)queueType).c_str(),lastRetCode,isLoaded,queueList.GetNumOfActions(),m_taskHandler->GetQueueSize((CQueueItemsType::QueueItemsTypeEnums)queueType));

    if (!isLoaded || lastRetCode == 200)
    {
      ////////////////////////////////////////////
      // copy return result from the server if: //
      // a) queueList isn't loaded              //
      // b) the server returned 200             //
      ////////////////////////////////////////////

      m_taskHandler->CopyQueueList(queueList,(CQueueItemsType::QueueItemsTypeEnums)queueType);
    }
  }

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::DoWork - Exit function (queue)");

  return;
}

bool BXQueueManager::RequestQueueListFromServerTask::CanExecute()
{
  int activeWindowId = g_windowManager.GetActiveWindow();
  bool isLoaded = m_taskHandler->m_queueList.IsLoaded();

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - Enter function. [activeWindowId=%d][isLoaded=%d] (queue)",activeWindowId,isLoaded);

  if (!g_application.ShouldConnectToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - [ShouldConnectToInternet=FALSE] -> return FALSE (queue)");
    return false;
  }

  if (!isLoaded)
  {
    LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - [isLoaded=%d] -> return TRUE (queue)",isLoaded);
    return true;
  }

  LOG(LOG_LEVEL_DEBUG,"RequestQueueListFromServerTask::CanExecute - return TRUE. [ActiveWindowId=%d] (queue)",activeWindowId);
  return true;
}

std::string CQueueItemsType::GetQueueItemTypeAsStringId(CQueueItemsType::QueueItemsTypeEnums queueItemType)
{
  std::string id = "";

  switch (queueItemType)
  {
  //server is not hanlding this, keep id empty to get all the items
  case CQueueItemsType::QIT_ALL:
  {
    id = "all";
  }
  break;
  case CQueueItemsType::QIT_CLIP:
  {
    id = "clip";
  }
  break;
  case CQueueItemsType::QIT_MOVIE:
  {
    id = "movie";
  }
  break;
  case CQueueItemsType::QIT_TVSHOW:
  {
    id = "tv_show";
  }
  break;
  default:
  {
    LOG(LOG_LEVEL_ERROR,"CQueueItemsType::GetQueueItemTypeAsStringId - FAILED to get string id for [queueItemType=%d] (queue)",queueItemType);
  }
  break;
  }

  return id;
}

CQueueItemsType::QueueItemsTypeEnums CQueueItemsType::GetQueueItemTypeAsEnum(const std::string& _queueItemStr)
{
  std::string queueItemStr = BXUtils::URLDecode(_queueItemStr);

  if (queueItemStr.empty() || (queueItemStr == "all"))
  {
    return CQueueItemsType::QIT_ALL;
  }
  else if (queueItemStr == "clip")
  {
    return CQueueItemsType::QIT_CLIP;
  }
  else if (queueItemStr == "movie")
  {
    return CQueueItemsType::QIT_MOVIE;
  }
  else if (queueItemStr == "tv_show")
  {
    return CQueueItemsType::QIT_TVSHOW;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"CQueueItemsType::GetQueueItemTypeAsEnum - FAILED to get id for [queueItemStr=%s] (queue)",queueItemStr.c_str());
  }

  return CQueueItemsType::QIT_ALL;
}

}

