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
#include "../../LangInfo.h"

#include "GUIUserMessages.h"
#include "../../guilib/GUIWindowManager.h"

#ifdef HAS_EMBEDDED
#include "HalServices.h"
#endif

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

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBoxBoxeeApplicationsList - going to copy appBoxBoxeeApplicationsList [size=%d]. [currSize=%d] (appbox)",appBoxBoxeeApplicationsList.GetNumOfApplication(),m_appBoxBoxeeApplicationsList.GetNumOfApplication());

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

bool BXAppBoxManager::IsAppIdInAppBoxApplicationsList(const std::string& id)
{
  LockAppBoxBoxeeApplicationsList();

  bool boxeeApplicationsListWasLoaded = m_appBoxBoxeeApplicationsList.IsLoaded();

  if (!boxeeApplicationsListWasLoaded)
  {
    UnLockAppBoxBoxeeApplicationsList();
    return false;
  }

  bool isInAppBoxApplicationsList = m_appBoxBoxeeApplicationsList.IsApplicationIdExist(id);

  UnLockAppBoxBoxeeApplicationsList();

  return isInAppBoxApplicationsList;
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

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBoxPopularitiesList - going to copy appBoxPopularitiesList [size=%d]. [currSize=%d] (appbox)(popu)",appBoxPopularitiesList.GetNumOfPopularities(),m_appBoxPopularitiesList.GetNumOfPopularities());

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
  //LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - Enter function with [id=%s] (appbox)",id.c_str());

  std::string popularity = "0";

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    //LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - In offline mode. Going to return [0] (appbox)");
    return popularity;
  }

  bool popularitiesListWasLoaded = false;

  LockAppBoxPopularitiesList();

  popularitiesListWasLoaded = m_appBoxPopularitiesList.IsLoaded();

  while (!popularitiesListWasLoaded)
  {
    // AppBoxPopularitiesList ISN'T loaded yet -> UnLock the AppBoxPopularitiesList and wait for it to load

    UnLockAppBoxPopularitiesList();

    //LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - AppBoxPopularitiesList is not loaded yet. Going to try again in [%dms] (appbox)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockAppBoxPopularitiesList();

    popularitiesListWasLoaded = m_appBoxPopularitiesList.IsLoaded();
  }

  popularity = m_appBoxPopularitiesList.GetPopularityById(id);

  UnLockAppBoxPopularitiesList();

  //LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::GetAppBoxPopularitiesById - Exit function (appbox)");

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

  LOG(LOG_LEVEL_DEBUG,"BXAppBoxManager::CopyAppBoxRepositoriesList - going to copy appBoxRepositoriesList [size=%d]. [currSize=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories(),m_appBoxRepositoriesList.GetNumOfRepositories());

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
  

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxBoxeeApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE]-> Exit function (appbox)");
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

  if (g_guiSettings.GetBool("filelists.filtergeoip2"))
  {
    CStdString countryCode = g_application.GetCountryCode();

    if (!countryCode.IsEmpty())
    {
      mapRemoteOptions["geo"] = countryCode;
    }
  }

#ifdef HAS_EMBEDDED
  CHalHardwareInfo hwInfo;
  CHalSoftwareInfo swInfo;
  IHalServices& client = CHalServicesFactory::GetInstance();

  if(client.GetHardwareInfo(hwInfo))
  {
    mapRemoteOptions["serialnumber"] = hwInfo.serialNumber;
  }

  if (client.GetSoftwareInfo(swInfo))
  {
    mapRemoteOptions["regionsku"] = swInfo.regionSKU;
  }
#endif

  if (mapRemoteOptions.size() > 0)
  {
    strUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);
  }

  appBoxBoxeeApplicationsList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_appBoxBoxeeApplicationsList.IsLoaded();
  long lastRetCode = appBoxBoxeeApplicationsList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxBoxeeApplicationsListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (appbox)",lastRetCode,isLoaded,appBoxBoxeeApplicationsList.GetNumOfApplication(),m_taskHandler->m_appBoxBoxeeApplicationsList.GetNumOfApplication());

  if (!isLoaded || lastRetCode == 200)
  {
    /////////////////////////////////////////////////
    // copy return result from the server if:      //
    // a) appBoxBoxeeApplicationsList isn't loaded //
    // b) the server returned 200                  //
    /////////////////////////////////////////////////

    m_taskHandler->CopyAppBoxBoxeeApplicationsList(appBoxBoxeeApplicationsList);
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

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxPopularitiesApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxPopularitiesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (appbox)(popu)");
    return;
  }

  BXAppBoxPopularities appBoxPopularitiesList;

  appBoxPopularitiesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxPopularitiesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxPopularitiesUrl","http://app.boxee.tv/applications/popularity");

  appBoxPopularitiesList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_appBoxPopularitiesList.IsLoaded();
  long lastRetCode = appBoxPopularitiesList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxPopularitiesListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (appbox)(popu)",lastRetCode,isLoaded,appBoxPopularitiesList.GetNumOfPopularities(),m_taskHandler->m_appBoxPopularitiesList.GetNumOfPopularities());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////////
    // copy return result from the server if:     //
    // a) appBoxPopularitiesList isn't loaded     //
    // b) the server returned 200                 //
    ////////////////////////////////////////////////

    m_taskHandler->CopyAppBoxPopularitiesList(appBoxPopularitiesList);
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

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetAppBoxRepositoriesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (repos)");
    return;
  }

  BXAppBoxRepositories appBoxRepositoriesList;

  appBoxRepositoriesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  appBoxRepositoriesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.AppBoxRepositoriesUrl","http://app.boxee.tv/api/get_repositories");

  appBoxRepositoriesList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_appBoxRepositoriesList.IsLoaded();
  long lastRetCode = appBoxRepositoriesList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (repos)",lastRetCode,isLoaded,appBoxRepositoriesList.GetNumOfRepositories(),m_taskHandler->m_appBoxRepositoriesList.GetNumOfRepositories());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////////
    // copy return result from the server if:     //
    // a) appBoxRepositoriesList isn't loaded     //
    // b) the server returned 200                 //
    ////////////////////////////////////////////////

    m_taskHandler->CopyAppBoxRepositoriesList(appBoxRepositoriesList);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Going to call BuildRepositoriesApplicationsList. [NumOfRepositories=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories());

    std::map<std::string, TiXmlDocument> appBox3rdPartyRepositoriesApplicationsMap;

    BuildRepositoriesApplicationsList(appBoxRepositoriesList, appBox3rdPartyRepositoriesApplicationsMap);

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::DoWork - Call to BuildRepositoriesApplicationsList for [NumOfRepositories=%d] returned [appBox3rdPartyRepositoriesApplicationsMapSize=%d] (repos)",appBoxRepositoriesList.GetNumOfRepositories(),(int)appBox3rdPartyRepositoriesApplicationsMap.size());

    m_taskHandler->CopyAppBox3rdPartyRepositoriesApplicationsList(appBox3rdPartyRepositoriesApplicationsMap);
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

    BOXEE::BXXMLDocument document;
    if (!document.LoadFromURL(strUrl.c_str()))
    {
      LOG(LOG_LEVEL_ERROR,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - FAILED to parse index file for [repoId=%s][repoName=%s]. [ErrorDesc=%s] at [row=%d][col=%d]. Continue to next repository (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),repoId.c_str(),repoName.c_str(),document.GetDocument().ErrorDesc(),document.GetDocument().ErrorRow(),document.GetDocument().ErrorCol());
      continue;
    }

    appBox3rdPartyRepositoriesApplicationsMap[repoId] = document.GetDocument();

    LOG(LOG_LEVEL_DEBUG,"RequestAppBoxRepositoriesListFromServerTask::BuildRepositoriesApplicationsList - [%d/%d] - After add RepositoryApplicationXml for [strUrl=%s][repoId=%s][repoName=%s]. [3rdPartyRepositoriesApplicationsMapSize=%d] (repos)",i+1,appBoxRepositoriesList.GetNumOfRepositories(),strUrl.c_str(),repoId.c_str(),repoName.c_str(),(int)appBox3rdPartyRepositoriesApplicationsMap.size());
  }
}

//////////////////////
// Apps Categories  //
//////////////////////

BXAppBoxManager::RequestAppsCategoriesListFromServerTask::RequestAppsCategoriesListFromServerTask(BXAppBoxManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestRepositoriesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXAppBoxManager::RequestAppsCategoriesListFromServerTask::~RequestAppsCategoriesListFromServerTask()
{

}

void BXAppBoxManager::RequestAppsCategoriesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestAppsCategoriesListFromServerTask::DoWork - Enter function (categories)");

  if (!g_application.ShouldConnectToInternet())
  {
    LOG(LOG_LEVEL_DEBUG,"RequestAppsCategoriesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (categories)");
    return;
  }

  std::string strLink = BXConfiguration::GetInstance().GetStringParam("Boxee.ApiGetAppCategories","http://app.boxee.tv/applications/categories");

  bool bHasParameters = false;
  if (!g_langInfo.GetLanguageCode().IsEmpty())
  {
    strLink += "?lang=";
    strLink += g_langInfo.GetLanguageCode().c_str();
    bHasParameters = true;
  }

  if (CUtil::IsAdultAllowed())
  {
    strLink += bHasParameters?"&":"?";
    strLink += "adult=yes";
  }

  BXXMLDocument xmlDoc;

  xmlDoc.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  xmlDoc.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  if (!xmlDoc.LoadFromURL(strLink))
  {
    LOG(LOG_LEVEL_ERROR,"RequestAppsCategoriesListFromServerTask::DoWork - FAILED to get application categories from [strLink=%s] (categories)",strLink.c_str());
    return;
  }

  TiXmlElement *pRootElement = xmlDoc.GetDocument().RootElement();
  if (!pRootElement || strcmpi(pRootElement->Value(), "categories") != 0)
  {
    LOG(LOG_LEVEL_ERROR,"RequestAppsCategoriesListFromServerTask::DoWork - could not parse application categories. [pRootElement=%p] (categories)",pRootElement);
    return;
  }

  std::vector<AppCategoryItem> appCategoryVec;

  const TiXmlNode* pTag = 0;
  while ((pTag = pRootElement->IterateChildren(pTag)))
  {
    if (pTag && pTag->ValueStr() == "category")
    {
      const TiXmlNode* pValue = pTag->FirstChild();

      if (pValue)
      {
        AppCategoryItem categoryItem;
        const char* attribute = ((TiXmlElement*)pTag)->Attribute("id");

        if (attribute)
          categoryItem.m_Id = std::string(attribute);

        categoryItem.m_Text = pValue->ValueStr();

        appCategoryVec.push_back(categoryItem);
      }
    }
  }

  m_taskHandler->CopyAppsCategoriesList(appCategoryVec);

  return;
}

bool BXAppBoxManager::UpdateAppsCategoriesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestAppsCategoriesListFromServerTask* reqAppCategoriesListTask = new RequestAppsCategoriesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqAppCategoriesListTask)
  {
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqAppCategoriesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::UpdateAppsCategoriesList - FAILED to allocate RequestAppsCategoriesListFromServerTask (appscategories)");
    return false;
  }
}

bool BXAppBoxManager::GetAppsCategories(std::vector<AppCategoryItem>& categoryList)
{
  LockAppsCategoriesList();

  categoryList = m_appCategoryList;

  UnLockAppsCategoriesList();

  return true;
}

std::string BXAppBoxManager::GetAppsCategoryLabel(const std::string& id)
{
  std::string output;
  LockAppsCategoriesList();

  for (size_t i = 0; i<m_appCategoryList.size(); i++)
  {
    if (m_appCategoryList[i].m_Id == id)
    {
      output = m_appCategoryList[i].m_Text;
    }
  }

  UnLockAppsCategoriesList();

  return output;
}

void BXAppBoxManager::LockAppsCategoriesList()
{
  SDL_LockMutex(m_appsCategoriesListGuard);
}

void BXAppBoxManager::UnLockAppsCategoriesList()
{
  SDL_UnlockMutex(m_appsCategoriesListGuard);
}

void BXAppBoxManager::CopyAppsCategoriesList(const std::vector<AppCategoryItem>& appCategoriesList)
{
  LockAppsCategoriesList();

  m_appCategoryList = appCategoriesList;

  UnLockAppsCategoriesList();

  CGUIMessage refreshMenuMsg(GUI_MSG_UPDATE, WINDOW_INVALID, GUI_MSG_APPS_CATEGORIES_UPDATE);
  g_windowManager.SendThreadMessage(refreshMenuMsg,WINDOW_BOXEE_BROWSE_APPS);
}

}

