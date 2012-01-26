//
// C++ Implementation: bxappboxsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxappboxmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "bxversion.h"

#include "../../Util.h"
#include "../../GUISettings.h"
#include "../../Application.h"
#include "../../BoxeeUtils.h"

#define NUM_OF_ITER_TO_WAIT_FOR_REPO_MAP_LOAD 1

namespace BOXEE
{

BXAppBoxManager::BXAppBoxManager()
{
  m_appBoxBoxeeApplicationsListGuard = SDL_CreateMutex();
  m_appBox3rdPartyRepositoriesApplicationsListGuard = SDL_CreateMutex();
  m_appBoxPopularitiesListGuard = SDL_CreateMutex();
  m_appBoxRepositoriesListGuard = SDL_CreateMutex();

  m_is3rdPartyRepositoriesMapLoaded = false;
}

BXAppBoxManager::~BXAppBoxManager()
{
  SDL_DestroyMutex(m_appBoxBoxeeApplicationsListGuard);
  SDL_DestroyMutex(m_appBox3rdPartyRepositoriesApplicationsListGuard);
  SDL_DestroyMutex(m_appBoxPopularitiesListGuard);
  SDL_DestroyMutex(m_appBoxRepositoriesListGuard);
}

bool BXAppBoxManager::Initialize()
{
  return true;
}

///////////////////////////
// BoxeeApplicationsList //
///////////////////////////

void BXAppBoxManager::LockAppBoxBoxeeApplicationsList()
{
  SDL_LockMutex(m_appBoxBoxeeApplicationsListGuard);
}

void BXAppBoxManager::UnLockAppBoxBoxeeApplicationsList()
{
  SDL_UnlockMutex(m_appBoxBoxeeApplicationsListGuard);
}

void BXAppBoxManager::CopyAppBoxBoxeeApplicationsList(const BXAppBoxApplications& appBoxBoxeeApplicationsList)
{
  LockAppBoxBoxeeApplicationsList();

  m_appBoxBoxeeApplicationsList = appBoxBoxeeApplicationsList;
  m_appBoxBoxeeApplicationsList.SetLoaded(true);

  UnLockAppBoxBoxeeApplicationsList();
}

void BXAppBoxManager::SetAppBoxBoxeeApplicationsListIsLoaded(bool isLoaded)
{
  LockAppBoxBoxeeApplicationsList();

  m_appBoxBoxeeApplicationsList.SetLoaded(isLoaded);

  UnLockAppBoxBoxeeApplicationsList();
}

bool BXAppBoxManager::UpdateAppBoxBoxeeApplicationsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestAppBoxBoxeeApplicationsListFromServerTask* reqAppBoxBoxeeApplicationsListTask = new RequestAppBoxBoxeeApplicationsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqAppBoxBoxeeApplicationsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockAppBoxBoxeeApplicationsList();

      m_appBoxBoxeeApplicationsList.SetLoaded(false);
      
      UnLockAppBoxBoxeeApplicationsList();
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqAppBoxBoxeeApplicationsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::UpdateAppBoxBoxeeApplicationsList - FAILED to allocate RequestAppBoxBoxeeApplicationsListFromServerTask (appbox)");
    return false;
  }
}

bool BXAppBoxManager::GetAppBoxBoxeeApplications(TiXmlDocument& appBoxBoxeeApplicationsList)
{
  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxBoxeeApplications - Enter function (appbox)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxBoxeeApplications - In offline mode. Going to return FALSE (appbox)");
    return false;
  }

  bool boxeeApplicationsListWasLoaded = false;
  
  LockAppBoxBoxeeApplicationsList();

  boxeeApplicationsListWasLoaded = m_appBoxBoxeeApplicationsList.IsLoaded();
  
  while (!boxeeApplicationsListWasLoaded)
  {
    // AppBoxApplicationsList ISN'T loaded yet -> UnLock the AppBoxApplicationsList and wait for it to load
    
    UnLockAppBoxBoxeeApplicationsList();

    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxBoxeeApplications - AppBoxBoxeeApplicationsList is not loaded yet. Going to try again in [%dms] (appbox)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockAppBoxBoxeeApplicationsList();
      
    boxeeApplicationsListWasLoaded = m_appBoxBoxeeApplicationsList.IsLoaded();
  }

  appBoxBoxeeApplicationsList = m_appBoxBoxeeApplicationsList.GetDocument();

  UnLockAppBoxBoxeeApplicationsList();

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxBoxeeApplications - Exit function (appbox)");

  return true;
}

///////////////////////////
// BoxeeApplicationsList //
///////////////////////////

void BXAppBoxManager::LockAppBox3rdPartyRepositoriesApplicationsList()
{
  SDL_LockMutex(m_appBox3rdPartyRepositoriesApplicationsListGuard);
}

void BXAppBoxManager::UnLockAppBox3rdPartyRepositoriesApplicationsList()
{
  SDL_UnlockMutex(m_appBox3rdPartyRepositoriesApplicationsListGuard);
}

void BXAppBoxManager::CopyAppBox3rdPartyRepositoriesApplicationsList(std::map<std::string, TiXmlDocument>& appBox3rdPartyApplicationsMap)
{
  LockAppBox3rdPartyRepositoriesApplicationsList();

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBox3rdPartyRepositoriesApplicationsList - Enter function. [NumOf3rdPartysList=%d][IsRepositoriesMapLoaded=%d] (repos)",(int)appBox3rdPartyApplicationsMap.size(),m_is3rdPartyRepositoriesMapLoaded);

  m_appBox3rdPartyRepositoriesApplicationsMap.clear();

  std::map<std::string, TiXmlDocument>::iterator it = appBox3rdPartyApplicationsMap.begin();

  int counter = 0;

  while (it != appBox3rdPartyApplicationsMap.end())
  {
    std::string repositoryId = (*it).first;
    TiXmlDocument repositoryApplicationListXml = (*it).second;

    m_appBox3rdPartyRepositoriesApplicationsMap[repositoryId] = repositoryApplicationListXml;

    counter++;
    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBox3rdPartyRepositoriesApplicationsList - [%d/%d] - After copy [repositoryId=%s]. [NumOf3rdPartysMap=%d] (repos)",counter,(int)appBox3rdPartyApplicationsMap.size(),repositoryId.c_str(),(int)m_appBox3rdPartyRepositoriesApplicationsMap.size());

    it++;
  }

  m_is3rdPartyRepositoriesMapLoaded = true;

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBox3rdPartyRepositoriesApplicationsList - Exit function. [NumOf3rdPartysList=%d][IsRepositoriesMapLoaded=%d] (repos)",(int)m_appBox3rdPartyRepositoriesApplicationsMap.size(),m_is3rdPartyRepositoriesMapLoaded);

  UnLockAppBox3rdPartyRepositoriesApplicationsList();
}

void BXAppBoxManager::SetAppBox3rdPartyRepositoriesApplicationsListIsLoaded(bool isLoaded)
{
  LockAppBox3rdPartyRepositoriesApplicationsList();

  m_is3rdPartyRepositoriesMapLoaded = isLoaded;

  UnLockAppBox3rdPartyRepositoriesApplicationsList();
}

bool BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications(const std::string& repositoryId, TiXmlDocument& appBox3rdPartyRepositoryApplicationsList, bool bDontWait)
{
  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications - Enter function. [repositoryId=%s][IsRepositoriesMapLoaded=%d] (repos)",repositoryId.c_str(),m_is3rdPartyRepositoriesMapLoaded);

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications - In offline mode. Going to return FALSE. [repositoryId=%s][IsRepositoriesMapLoaded=%d] (repos)",repositoryId.c_str(),m_is3rdPartyRepositoriesMapLoaded);
    return false;
  }

  int numOfIterToWaitForRepoMapLoad = bDontWait ? NUM_OF_ITER_TO_WAIT_FOR_REPO_MAP_LOAD : NUM_OF_ITER_TO_WAIT_FOR_REPO_MAP_LOAD * 10;

  LockAppBox3rdPartyRepositoriesApplicationsList();

  while (!m_is3rdPartyRepositoriesMapLoaded && numOfIterToWaitForRepoMapLoad)
  {
    // AppBox3rdPartyApplicationsMap ISN'T loaded yet -> UnLock the AppBox3rdPartyApplicationsMap and wait for it to load

    UnLockAppBox3rdPartyRepositoriesApplicationsList();

    numOfIterToWaitForRepoMapLoad--;

    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications - AppBox3rdPartyApplicationsMap is not loaded yet. Going to try again in [%dms]. [repositoryId=%s][IsRepositoriesMapLoaded=%d][numOfIterToWaitForRepoMapLoad=%d] (repos)",DELAY_FOR_CHECK_FEED_LOADED,repositoryId.c_str(),m_is3rdPartyRepositoriesMapLoaded,numOfIterToWaitForRepoMapLoad);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockAppBox3rdPartyRepositoriesApplicationsList();
  }

  std::map<std::string, TiXmlDocument>::iterator it = m_appBox3rdPartyRepositoriesApplicationsMap.find(repositoryId);

  if (it == m_appBox3rdPartyRepositoriesApplicationsMap.end())
  {
    UnLockAppBox3rdPartyRepositoriesApplicationsList();

    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications - FAILED to find applications for [repositoryId=%s] (repos)",repositoryId.c_str());

    return false;
  }

  appBox3rdPartyRepositoryApplicationsList = (*it).second;

  UnLockAppBox3rdPartyRepositoriesApplicationsList();

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBox3rdPartyRepositoryApplications - Exit function. [repositoryId=%s] (repos)",repositoryId.c_str());

  return true;
}

//////////////////////
// PopularitiesList //
//////////////////////

void BXAppBoxManager::LockAppBoxPopularitiesList()
{
  SDL_LockMutex(m_appBoxPopularitiesListGuard);
}

void BXAppBoxManager::UnLockAppBoxPopularitiesList()
{
  SDL_UnlockMutex(m_appBoxPopularitiesListGuard);
}

void BXAppBoxManager::CopyAppBoxPopularitiesList(const BXAppBoxPopularities& appBoxPopularitiesList)
{
  LockAppBoxPopularitiesList();

  m_appBoxPopularitiesList = appBoxPopularitiesList;
  m_appBoxPopularitiesList.SetLoaded(true);

  UnLockAppBoxPopularitiesList();
}

void BXAppBoxManager::SetAppBoxPopularitiesApplicationsListIsLoaded(bool isLoaded)
{
  LockAppBoxPopularitiesList();

  m_appBoxPopularitiesList.SetLoaded(isLoaded);

  UnLockAppBoxPopularitiesList();
}

bool BXAppBoxManager::UpdateAppBoxPopularitiesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestAppBoxPopularitiesListFromServerTask* reqAppBoxPopularitiesListTask = new RequestAppBoxPopularitiesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqAppBoxPopularitiesListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order
      // for get function to wait for update

      LockAppBoxPopularitiesList();

      m_appBoxPopularitiesList.SetLoaded(false);

      UnLockAppBoxPopularitiesList();
    }

    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqAppBoxPopularitiesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::UpdateAppBoxPopularitiesList - FAILED to allocate RequestAppBoxPopularitiesListFromServerTask (appbox)");
    return false;
  }
}

std::string BXAppBoxManager::GetAppBoxPopularityById(const std::string& id)
{
  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - Enter function with [id=%s] (appbox)",id.c_str());

  std::string popularity = "0";

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - In offline mode. Going to return [0] (appbox)");
    return popularity;
  }

  bool popularitiesListWasLoaded = false;

  LockAppBoxPopularitiesList();

  popularitiesListWasLoaded = m_appBoxPopularitiesList.IsLoaded();

  while (!popularitiesListWasLoaded)
  {
    // AppBoxPopularitiesList ISN'T loaded yet -> UnLock the AppBoxPopularitiesList and wait for it to load

    UnLockAppBoxPopularitiesList();

    LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - AppBoxPopularitiesList is not loaded yet. Going to try again in [%dms] (appbox)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockAppBoxPopularitiesList();

    popularitiesListWasLoaded = m_appBoxPopularitiesList.IsLoaded();
  }

  popularity = m_appBoxPopularitiesList.GetPopularityById(id);

  UnLockAppBoxPopularitiesList();

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - Exit function (appbox)");

  return popularity;
}

//////////////////////
// RepositoriesList //
//////////////////////

void BXAppBoxManager::LockAppBoxRepositoriesList()
{
  SDL_LockMutex(m_appBoxRepositoriesListGuard);
}

void BXAppBoxManager::UnLockAppBoxRepositoriesList()
{
  SDL_UnlockMutex(m_appBoxRepositoriesListGuard);
}

void BXAppBoxManager::CopyAppBoxRepositoriesList(const BXAppBoxRepositories& appBoxRepositoriesList)
{
  LockAppBoxRepositoriesList();

  m_appBoxRepositoriesList = appBoxRepositoriesList;
  m_appBoxRepositoriesList.SetLoaded(true);

  UnLockAppBoxRepositoriesList();
}

void BXAppBoxManager::SetAppBoxRepositoriesListIsLoaded(bool isLoaded)
{
  LockAppBoxRepositoriesList();

  m_appBoxRepositoriesList.SetLoaded(isLoaded);

  UnLockAppBoxRepositoriesList();
}

bool BXAppBoxManager::UpdateAppBoxRepositoriesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestAppBoxRepositoriesListFromServerTask* reqAppBoxRepositoriesListTask = new RequestAppBoxRepositoriesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqAppBoxRepositoriesListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order
      // for get function to wait for update

      LockAppBoxRepositoriesList();

      m_appBoxRepositoriesList.SetLoaded(false);
      m_is3rdPartyRepositoriesMapLoaded = false;

      UnLockAppBoxRepositoriesList();
    }

    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqAppBoxRepositoriesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::UpdateAppBoxRepositoriesList - FAILED to allocate RequestAppBoxRepositoriesListFromServerTask (repos)");
    return false;
  }
}

bool BXAppBoxManager::GetRepositories(BXAppBoxRepositories& repositoriesList)
{
  LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetRepositories - Enter function (repos)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetRepositories - In offline mode. Going to return FALSE (repos)");
    return false;
  }

  bool repositoriesListWasLoaded = false;

  LockAppBoxRepositoriesList();

  repositoriesListWasLoaded = m_appBoxRepositoriesList.IsLoaded();

  while (!repositoriesListWasLoaded)
  {
    // ApplicationsList ISN'T loaded yet -> UnLock the ApplicationsList and wait for it to load

    UnLockAppBoxRepositoriesList();

    LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetRepositories - ApplicationsList is not loaded yet. Going to try again in [%dms] (repos)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockAppBoxRepositoriesList();

    repositoriesListWasLoaded = m_appBoxRepositoriesList.IsLoaded();
  }

  repositoriesList = m_appBoxRepositoriesList;

  UnLockAppBoxRepositoriesList();

  LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetRepositories - Exit function. After set [RepositoriesListSize=%d] (repos)",repositoriesList.GetNumOfRepositories());

  return true;
}

////////////////////////////////////////////////////////////////
// RequestAppBoxBoxeeApplicationsListFromServerTask functions //
////////////////////////////////////////////////////////////////

BXAppBoxManager::RequestAppBoxBoxeeApplicationsListFromServerTask::RequestAppBoxBoxeeApplicationsListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestApplicationsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXAppBoxManager::RequestAppBoxBoxeeApplicationsListFromServerTask::~RequestAppBoxBoxeeApplicationsListFromServerTask()
{
  
}

void BXAppBoxManager::RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - Enter function (appbox)");
  
  if (!g_application.IsConnectedToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxBoxeeApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (appbox)");
    return;
  }

  BXAppBoxApplications appBoxBoxeeApplicationsList;
  
  appBoxBoxeeApplicationsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxBoxeeApplicationsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxApplicationsPrefixUrl","http://app.boxee.tv/appindex");

  strUrl += "/";
  strUrl += BOXEE::Boxee::GetInstance().GetPlatform();
  strUrl += "/";
  strUrl += BOXEE_VERSION;

  std::map<CStdString, CStdString> mapRemoteOptions;

  if (!CUtil::IsAdultAllowed())
  {
    mapRemoteOptions["adult"] = "no";
  }

  if (g_guiSettings.GetBool("filelists.filtergeoip"))
  {
    CStdString countryCode = g_application.GetCountryCode();

    if (!countryCode.IsEmpty())
    {
      mapRemoteOptions["geo"] = countryCode;
    }
  }

  if (mapRemoteOptions.size() > 0)
  {
    strUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);
  }

  bool succeeded = appBoxBoxeeApplicationsList.LoadFromURL(strUrl);

  if (succeeded)
  {
    m_taskHandler->CopyAppBoxBoxeeApplicationsList(appBoxBoxeeApplicationsList);
  }
  else
  {
    LOG(LOG_LEVEL_WARNING,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - Call to LoadFromURL for [strUrl=%s] returned [%d] (appbox)",strUrl.c_str(),succeeded);

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxBoxeeApplicationsListIsLoaded(true);
  }
  
  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - Exit function (appbox)");

  return;
}

///////////////////////////////////////////////////////////
// RequestAppBoxPopularitiesListFromServerTask functions //
///////////////////////////////////////////////////////////

BXAppBoxManager::RequestAppBoxPopularitiesListFromServerTask::RequestAppBoxPopularitiesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestPopularitiesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXAppBoxManager::RequestAppBoxPopularitiesListFromServerTask::~RequestAppBoxPopularitiesListFromServerTask()
{

}

void BXAppBoxManager::RequestAppBoxPopularitiesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxPopularitiesListFromServerTask::DoWork - Enter function (appbox)(popu)");

  if (!g_application.IsConnectedToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxPopularitiesApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxPopularitiesListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (appbox)(popu)");
    return;
  }

  BXAppBoxPopularities appBoxPopularitiesList;

  appBoxPopularitiesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxPopularitiesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxPopularitiesUrl","http://app.boxee.tv/applications/popularity");

  bool succeeded = appBoxPopularitiesList.LoadFromURL(strUrl);

  if (succeeded)
  {
    m_taskHandler->CopyAppBoxPopularitiesList(appBoxPopularitiesList);
  }
  else
  {
    LOG(LOG_LEVEL_WARNING,"RequestAppBoxPopularitiesListFromServerTask::DoWork - Call to LoadFromURL for [strUrl=%s] returned [%d] (appbox)(popu)",strUrl.c_str(),succeeded);

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxPopularitiesApplicationsListIsLoaded(true);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxPopularitiesListFromServerTask::DoWork - Exit function (appbox)(popu)");

  return;
}

///////////////////////////////////////////////////////////
// RequestAppBoxRepositoriesListFromServerTask functions //
///////////////////////////////////////////////////////////

BXAppBoxManager::RequestAppBoxRepositoriesListFromServerTask::RequestAppBoxRepositoriesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestRepositoriesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXAppBoxManager::RequestAppBoxRepositoriesListFromServerTask::~RequestAppBoxRepositoriesListFromServerTask()
{

}

void BXAppBoxManager::RequestAppBoxRepositoriesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Enter function (repos)");

  if (!g_application.IsConnectedToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxRepositoriesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - [IsConnectedToInternet=FALSE] -> Exit function (repos)");
    return;
  }

  BXAppBoxRepositories appBoxRepositoriesList;

  appBoxRepositoriesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxRepositoriesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxRepositoriesUrl","http://app.boxee.tv/api/get_repositories");

  bool succeeded = appBoxRepositoriesList.LoadFromURL(strUrl);

  if (succeeded)
  {
    m_taskHandler->CopyAppBoxRepositoriesList(appBoxRepositoriesList);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Going to call BuildRepositoriesApplicationsList. [NumOfRepositories=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories());

    std::map<std::string, TiXmlDocument> appBox3rdPartyRepositoriesApplicationsMap;

    BuildRepositoriesApplicationsList(appBoxRepositoriesList, appBox3rdPartyRepositoriesApplicationsMap);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Call to BuildRepositoriesApplicationsList for [NumOfRepositories=%d] returned [appBox3rdPartyRepositoriesApplicationsMapSize=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories(),(int)appBox3rdPartyRepositoriesApplicationsMap.size());

    m_taskHandler->CopyAppBox3rdPartyRepositoriesApplicationsList(appBox3rdPartyRepositoriesApplicationsMap);
  }
  else
  {
    LOG(LOG_LEVEL_WARNING,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Call to LoadFromURL for [strUrl=%s] returned [%d] (repos)",strUrl.c_str(),succeeded);

    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxRepositoriesListIsLoaded(true);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Exit function (repos)");

  return;
}

void BXAppBoxManager::RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList(const BXAppBoxRepositories& appBoxRepositoriesList, std::map<std::string, TiXmlDocument>& appBox3rdPartyRepositoriesApplicationsMap)
{
  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - Enter function. [NumOfRepositories=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories());

  for (int i=0; i < appBoxRepositoriesList.GetNumOfRepositories(); i++)
  {
    BXObject repo = appBoxRepositoriesList.GetRepository(i);

    std::string repoId = repo.GetID();
    std::string repoName = repo.GetName();
    std::string repoUrl = repo.GetValue("url");

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - Going to get ApplicationList for [repoId=%s][repoName=%s] (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),repoId.c_str(),repoName.c_str());

    if (repoUrl.empty())
    {
      LOG(LOG_LEVEL_ERROR,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - FAILED to get application list for [RepoId=%s][repoName=%d] because url is EMPTY [url=%s]. Continue to next repository (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),repoId.c_str(),repoName.c_str(),repoUrl.c_str());
      continue;
    }

    std::string strUrl = repoUrl;
    if (!CUtil::HasSlashAtEnd(strUrl))
    {
      strUrl += "/";
    }

    strUrl += "index.xml";

    // Retrieve the file
    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - Try to get [strUrl=%s] for [repoId=%s][repoName=%s]  (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),strUrl.c_str(),repoId.c_str(),repoName.c_str());

    if (!g_application.IsConnectedToInternet())
    {
      LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - [IsConnectedToInternet=FALSE]. Continue to next repository (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories());
      continue;
    }

    std::string appsXml;
    BOXEE::BXCurl curl;
    try
    {
      appsXml = curl.HttpGetString(strUrl.c_str());
    }
    catch(...)
    {
      LOG(LOG_LEVEL_ERROR,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - Exception when try to get [strUrl=%s] (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),strUrl.c_str());
    }

    if (appsXml.empty())
    {
      LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - Try to get [strUrl=%s] return an EMPTY answer. Continue to next repository (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),strUrl.c_str());
      continue;
    }

    // Parse it
    TiXmlDocument xmlDoc;
    if (!xmlDoc.Parse(appsXml.c_str()))
    {
      LOG(LOG_LEVEL_ERROR,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - FAILED to parse index file for [repoId=%s][repoName=%s]. [ErrorDesc=%s] at [row=%d][col=%d]. Continue to next repository (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),repoId.c_str(),repoName.c_str(),xmlDoc.ErrorDesc(),xmlDoc.ErrorRow(),xmlDoc.ErrorCol());
      continue;
    }

    appBox3rdPartyRepositoriesApplicationsMap[repoId] = xmlDoc;

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - After add RepositoryApplicationXml for [strUrl=%s][repoId=%s][repoName=%s]. [3rdPartyRepositoriesApplicationsMapSize=%d] (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),strUrl.c_str(),repoId.c_str(),repoName.c_str(),(int)appBox3rdPartyRepositoriesApplicationsMap.size());
  }
}

}

