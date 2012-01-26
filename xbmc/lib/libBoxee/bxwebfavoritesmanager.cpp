/*
 * bxWebfavoritesmanager.cpp
 *
 *  Created on: Feb 10, 2011
 *      Author: shayyizhak
 */


#include "bxwebfavoritesmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"
#include "bxmessages.h"
#include "../../Util.h"

namespace BOXEE
{

BXWebFavoritesManager::BXWebFavoritesManager()
{
  m_webFavoritesListGuard = SDL_CreateMutex();
  m_webFavoritesList.Clear();
}

BXWebFavoritesManager::~BXWebFavoritesManager()
{
  SDL_DestroyMutex(m_webFavoritesListGuard);

}

bool BXWebFavoritesManager::Initialize()
{
  m_webFavoritesList.Clear();

  return true;
}

bool BXWebFavoritesManager::UpdateWebFavoritesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestWebFavoritesListFromServerTask* reqWebFavoritesListTask = new RequestWebFavoritesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqWebFavoritesListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order
      // for get function to wait for update

      LockWebFavoritesList();

      m_webFavoritesList.SetLoaded(false);

      UnLockWebFavoritesList();
    }

    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqWebFavoritesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXWebFavoritesManager::UpdateWebFavoritesList - FAILED to allocate RequestWebFavoritesListFromServerTask (subs)");
    return false;
  }
}

void BXWebFavoritesManager::LockWebFavoritesList()
{
  SDL_LockMutex(m_webFavoritesListGuard);
}

void BXWebFavoritesManager::UnLockWebFavoritesList()
{
  SDL_UnlockMutex(m_webFavoritesListGuard);
}

void BXWebFavoritesManager::CopyWebFavoritesList(const BXBoxeeWebFavorites& webFavoritesList)
{
  LockWebFavoritesList();

  m_webFavoritesList = webFavoritesList;
  m_webFavoritesList.SetLoaded(true);

  UnLockWebFavoritesList();
}

void BXWebFavoritesManager::SetWebFavoritesListIsLoaded(bool isLoaded)
{
  LockWebFavoritesList();

  m_webFavoritesList.SetLoaded(isLoaded);

  UnLockWebFavoritesList();
}

bool BXWebFavoritesManager::GetWebFavorites(BXBoxeeWebFavorites& webFavoritesList)
{
  LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavoritesList - Enter function (subs)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavoritesList - In offline mode. Going to return FALSE (subs)");
    return false;
  }

  bool webFavoritesListWasLoaded = false;

  LockWebFavoritesList();

  webFavoritesListWasLoaded = m_webFavoritesList.IsLoaded();

  while (!webFavoritesListWasLoaded)
  {
    // webFavoritesList ISN'T loaded yet -> UnLock the webFavoritesList and wait for it to load

    UnLockWebFavoritesList();

    LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavoritesList - webFavoritesList is not loaded yet. Going to try again in [%dms] (subs)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockWebFavoritesList();

    webFavoritesListWasLoaded = m_webFavoritesList.IsLoaded();
  }

  webFavoritesList = m_webFavoritesList;

  UnLockWebFavoritesList();

  //LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavoritesList - Exit function. After set [webFavoritesListSize=%d] (subs)",webFavoritesList);

  return true;
}

bool BXWebFavoritesManager::GetWebFavorites(std::vector<BXObject>& webFavoritesVec)
{
  BXBoxeeWebFavorites webFavorites;

  GetWebFavorites(webFavorites);

  int numOfWebFavorites = webFavorites.GetNumOfWebFavorites();

  if(numOfWebFavorites < 1)
  {
    LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavorites - Call to GetWebFavoritesList returned [webFavoritesListSize=%d] (subs)",numOfWebFavorites);
    return false;
  }

  for(int i=0; i<numOfWebFavorites; i++)
  {
    BXObject obj = webFavorites.GetWebFavorites(i);
    webFavoritesVec.push_back(obj);
  }

  LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::GetWebFavorites - Going to return  [webFavoritesVecSize=%d] (subs)",(int)webFavoritesVec.size());

  return true;
}

bool BXWebFavoritesManager::IsInWebFavorites(const std::string& src)
{
  bool webFavoritesListWasLoaded = false;

  LockWebFavoritesList();

  webFavoritesListWasLoaded = m_webFavoritesList.IsLoaded();

  while (!webFavoritesListWasLoaded)
  {
    // webFavoritesList ISN'T loaded yet -> UnLock the webFavoritesList and wait for it to load

    UnLockWebFavoritesList();

    LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::IsInWebFavorites - webFavoritesList is not loaded yet. Going to try again in [%dms] (subs)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockWebFavoritesList();

    webFavoritesListWasLoaded = m_webFavoritesList.IsLoaded();
  }

  int numOfWebFavorites = m_webFavoritesList.GetNumOfWebFavorites();

  LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::IsInWebFavorites - Enter function with [src=%s][webFavoritesListSize=%d] (subs)",src.c_str(),numOfWebFavorites);

  if(numOfWebFavorites < 1)
  {
    UnLockWebFavoritesList();
    LOG(LOG_LEVEL_WARNING,"BXWebFavoritesManager::IsInWebFavorites - [webFavoritesListSize=%d] therefore going to return FALSE (subs)",numOfWebFavorites);
    return false;
  }

  for (int i=0; i < numOfWebFavorites; i++)
  {
    BXObject favorite = m_webFavoritesList.GetWebFavorites(i);
    CStdString decodedFavoritePath = favorite.GetValue(MSG_KEY_URL);
    CUtil::UrlDecode(decodedFavoritePath);

    if(decodedFavoritePath.c_str() == src)
    {
      UnLockWebFavoritesList();
      LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::IsInWebFavorites - [src=%s] WAS found in the webFavoritesList. Exit function and return TRUE (subs)",src.c_str());
      return true;
    }
  }

  UnLockWebFavoritesList();
  LOG(LOG_LEVEL_DEBUG,"BXWebFavoritesManager::IsInWebFavorites - [src=%s] WASN'T found in the webFavoritesList. Exit function and return FALSE (subs)",src.c_str());
  return false;
}


//////////////////////////////////////////////////////
// RequestwebFavoritesListFromServerTask functions //
//////////////////////////////////////////////////////

BXWebFavoritesManager::RequestWebFavoritesListFromServerTask::RequestWebFavoritesListFromServerTask(BXWebFavoritesManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestWebFavoritesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXWebFavoritesManager::RequestWebFavoritesListFromServerTask::~RequestWebFavoritesListFromServerTask()
{

}

void BXWebFavoritesManager::RequestWebFavoritesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestWebFavoritesListFromServerTask::DoWork - Enter function (subs)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetWebFavoritesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestWebFavoritesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (subs)");
    return;
  }

  BXBoxeeWebFavorites webFavoritesList;

  webFavoritesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  webFavoritesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.webFavoritesListUrl","http://app.boxee.tv/api/bookmarks");

  ListHttpHeaders headers;
  headers.push_back("Content-Type: text/xml");

  webFavoritesList.LoadFromURL(strUrl, headers, "");
  m_taskHandler->CopyWebFavoritesList(webFavoritesList);

  LOG(LOG_LEVEL_DEBUG,"RequestWebFavoritesListFromServerTask::DoWork - Exit function (subs)");

  return;
}

//////////////////////////////////
// BXWebFavoriteItem functions //
//////////////////////////////////

BXWebFavoritesItem::BXWebFavoritesItem(const std::string& id, const std::string& name, const std::string src) : BXItem("WebFavorites",id,name)
{
  m_src = src;
}

BXWebFavoritesItem::BXWebFavoritesItem(const std::string& type, const std::string& id, const std::string& name, const std::string src) : BXItem("WebFavorites",id,name)
{
  m_src = src;
}

std::string BXWebFavoritesItem::GetSrc()
{
  return m_src;
}

}

