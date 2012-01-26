//
// C++ Implementation: bxapplicationsmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxapplicationsmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"

namespace BOXEE
{

BXApplicationsManager::BXApplicationsManager()
{
  m_applicationsListGuard = SDL_CreateMutex();
  m_applicationsList.Clear();
}

BXApplicationsManager::~BXApplicationsManager()
{
  SDL_DestroyMutex(m_applicationsListGuard);
}

bool BXApplicationsManager::Initialize()
{
  m_applicationsList.Clear();
  
  return true;
}

bool BXApplicationsManager::UpdateApplicationsList(unsigned long executionDelayInMS, bool repeat)
{
  RequestApplicationsListFromServerTask* reqApplicationsListTask = new RequestApplicationsListFromServerTask(this,executionDelayInMS,repeat);

  if(reqApplicationsListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockApplicationsList();

      m_applicationsList.SetLoaded(false);

      UnLockApplicationsList();      
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqApplicationsListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::UpdateApplicationsList - FAILED to allocate RequestApplicationsListFromServerTask (apps)");
    return false;
  }
}

void BXApplicationsManager::LockApplicationsList()
{
  SDL_LockMutex(m_applicationsListGuard);
}

void BXApplicationsManager::UnLockApplicationsList()
{
  SDL_UnlockMutex(m_applicationsListGuard);
}

void BXApplicationsManager::CopyApplicationsList(const BXBoxeeApplications& applicationsList)
{
  LockApplicationsList();
  
  m_applicationsList = applicationsList;
  m_applicationsList.SetLoaded(true);
  
  UnLockApplicationsList();
}

void BXApplicationsManager::SetApplicationsListIsLoaded(bool isLoaded)
{
  LockApplicationsList();

  m_applicationsList.SetLoaded(isLoaded);

  UnLockApplicationsList();
}

bool BXApplicationsManager::GetApplications(BXBoxeeApplications& applicationsList)
{
  LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetApplicationsList - Enter function (apps)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetApplicationsList - In offline mode. Going to return FALSE (apps)");
    return false;
  }

  bool applicationsListWasLoaded = false;
  
  LockApplicationsList();

  applicationsListWasLoaded = m_applicationsList.IsLoaded();
  
  while (!applicationsListWasLoaded)
  {
    // ApplicationsList ISN'T loaded yet -> UnLock the ApplicationsList and wait for it to load
    
    UnLockApplicationsList();

    LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetApplicationsList - ApplicationsList is not loaded yet. Going to try again in [%dms] (apps)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockApplicationsList();
      
    applicationsListWasLoaded = m_applicationsList.IsLoaded();
  }

  applicationsList = m_applicationsList;

  UnLockApplicationsList();

  LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetApplicationsList - Exit function. After set [ApplicationsListSize=%d] (apps)",applicationsList.GetNumOfApplications());

  return true;
}

bool BXApplicationsManager::GetApplications(std::vector<BXApplicationItem>& applicationsVec)
{
  BXBoxeeApplications applications;
  
  GetApplications(applications);
  
  int numOfApplications = applications.GetNumOfApplications();
  
  if(numOfApplications < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXApplicationsManager::GetApplications - Call to GetApplications returned [ApplicationsListSize=%d] (apps)",numOfApplications);
    return false;
  }
  
  for(int i=0; i<numOfApplications; i++)
  {
    BXObject obj = applications.GetApplication(i);
    
    std::string type = obj.GetType();
    std::string id = obj.GetID();
    std::string name = obj.GetValue(MSG_KEY_NAME);
    
    BXApplicationItem applicationItem(type,id,name);
    
    applicationsVec.push_back(applicationItem);
  }

  LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::GetApplications - Going to return  [ApplicationsListSize=%d] (apps)",applicationsVec.size());

  return true;
}

bool BXApplicationsManager::IsInApplications(const std::string& id)
{
  bool applicationsListWasLoaded = false;

  LockApplicationsList();

  applicationsListWasLoaded = m_applicationsList.IsLoaded();
  
  while (!applicationsListWasLoaded)
  {
    // ApplicationsList ISN'T loaded yet -> UnLock the ApplicationsList and wait for it to load

    UnLockApplicationsList();

    LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplications - ApplicationsList is not loaded yet. Going to try again in [%dms] (apps)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockApplicationsList();

    applicationsListWasLoaded = m_applicationsList.IsLoaded();
  }

  int numOfApplications = m_applicationsList.GetNumOfApplications();
  
  //LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplicationsList - Enter function with [id=%s][ApplicationsListSize=%d] (apps)",id.c_str(),numOfApplications);
  
  if(numOfApplications < 1)
  {
    UnLockApplicationsList();
    LOG(LOG_LEVEL_WARNING,"BXApplicationsManager::IsInApplicationsList - [ApplicationsListSize=%d] therefore going to return FALSE (apps)",numOfApplications);
    return false;    
  }

  /*
  for (int i=0; i < numOfApplications; i++)
  {
    if (m_applicationsList.IsApplicationIdExist(id))
    {
      UnLockApplicationsList();
      LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplicationsList - [id=%s] WAS found in the ApplicationsList. Exit function and return TRUE (apps)",id.c_str());
      return true;
    }

    BXObject app = m_applicationsList.GetApplication(i);
    
    if(app.GetID() == id)
    {
      UnLockApplicationsList();
      LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplicationsList - [is=%s] WAS found in the ApplicationsList. Exit function and return TRUE (apps)",id.c_str());
      return true;      
    }
  }
  */
  
  if (m_applicationsList.IsApplicationIdExist(id))
  {
    UnLockApplicationsList();
    //LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplicationsList - [id=%s] WAS found in the ApplicationsList. Exit function and return TRUE (apps)",id.c_str());
    return true;
  }
  else
  {
    UnLockApplicationsList();
    //LOG(LOG_LEVEL_DEBUG,"BXApplicationsManager::IsInApplicationsList - [id=%s] WASN'T found in the ApplicationsList. Exit function and return FALSE (apps)",id.c_str());
    return false;
  }
}

/////////////////////////////////////////////////////
// RequestApplicationsListFromServerTask functions //
/////////////////////////////////////////////////////

BXApplicationsManager::RequestApplicationsListFromServerTask::RequestApplicationsListFromServerTask(BXApplicationsManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestApplicationsList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXApplicationsManager::RequestApplicationsListFromServerTask::~RequestApplicationsListFromServerTask()
{
  
}

void BXApplicationsManager::RequestApplicationsListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestApplicationsListFromServerTask::DoWork - Enter function (apps)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestApplicationsListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (apps)");
    return;
  }

  BXBoxeeApplications applicationsList;
  
  applicationsList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  applicationsList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ApplicationsListUrl","http://app.boxee.tv/api/get_applications");

  bool retVal = false;

  try
  {
    LOG(LOG_LEVEL_DEBUG,"RequestApplicationsListFromServerTask::DoWork - Going to call LoadFromURL with [%s] (apps)",strUrl.c_str());

    retVal = applicationsList.LoadFromURL(strUrl);

    bool isLoaded = m_taskHandler->m_applicationsList.IsLoaded();
    long lastRetCode = applicationsList.GetLastRetCode();

    LOG(LOG_LEVEL_DEBUG,"RequestApplicationsListFromServerTask::DoWork - Call LoadFromURL with [%s] returned [ApplicationsListSize=%d][lastRetCode=%d][timestamp=%lu]. [isLoaded=%d] (apps)(schedule)",strUrl.c_str(),applicationsList.GetNumOfApplications(),lastRetCode,applicationsList.GetTimeStamp(),isLoaded);

    if (!isLoaded || lastRetCode == 200)
    {
      ////////////////////////////////////////////
      // copy return result from the server if: //
      // a)  applicationsList isn't loaded      //
      // b) the server returned 200             //
      ////////////////////////////////////////////

      m_taskHandler->CopyApplicationsList(applicationsList);
    }
  }
  catch (BXNetworkException* e)
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_ERROR,"RequestApplicationsListFromServerTask::DoWork - Call LoadFromURL with [%s] FAILED [BXNetworkException] (apps)",strUrl.c_str());
    delete e;
  }
  catch (BXCredentialsException* e)
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetApplicationsListIsLoaded(true);

    LOG(LOG_LEVEL_ERROR,"RequestApplicationsListFromServerTask::DoWork - Call LoadFromURL with [%s] FAILED [BXCredentialsException] (apps)",strUrl.c_str());
    delete e;
  }

  LOG(LOG_LEVEL_DEBUG,"RequestApplicationsListFromServerTask::DoWork - Exit function (apps)");

  return;
}

//////////////////////////////////
// BXApplicationItem functions //
//////////////////////////////////

BXApplicationItem::BXApplicationItem(const std::string& type, const std::string& id, const std::string& name) : BXItem(type,id,name)
{

}

}

