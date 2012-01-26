//
// C++ Implementation: bxservicesmanager
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxservicesmanager.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxexceptions.h"
#include "logger.h"
#include "../../Application.h"
#include "../../BoxeeUtils.h"
#include "lib/libjson/include/json/reader.h"
#include "../../GUILargeTextureManager.h"

namespace BOXEE
{

BXServicesManager::BXServicesManager()
{
  m_servicesListGuard = SDL_CreateMutex();

  m_servicesList.Clear();
}

BXServicesManager::~BXServicesManager()
{
  SDL_DestroyMutex(m_servicesListGuard);
}

bool BXServicesManager::Initialize()
{
  m_servicesList.Clear();
  
  return true;
}

bool BXServicesManager::UpdateServicesList(unsigned long executionDelayInMS, bool repeat)
{
  RequestServicesListFromServerTask* reqServicesListTask = new RequestServicesListFromServerTask(this,executionDelayInMS,repeat);

  if(reqServicesListTask)
  {
    if(executionDelayInMS == 0)
    {
      // In case the request is for immediate execution -> Set the status of the list to NOT LOADED in order 
      // for get function to wait for update

      LockServicesList();

      m_servicesList.SetLoaded(false);
      
      UnLockServicesList();      
    }
    
    Boxee::GetInstance().GetBoxeeScheduleTaskManager().AddScheduleTask(reqServicesListTask);
    return true;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR,"BXServicesManager::UpdateServicesList - FAILED to allocate RequestServicesListFromServerTask (serv)");
    return false;
  }
}


void BXServicesManager::LockServicesList()
{
  SDL_LockMutex(m_servicesListGuard);
}

void BXServicesManager::UnLockServicesList()
{
  SDL_UnlockMutex(m_servicesListGuard);
}


void BXServicesManager::CopyServicesList(const BXBoxeeServices& servicesList)
{
  LockServicesList();

  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::CopyServicesList - going to copy servicesList [size=%d]. [currSize=%d] (serv)",servicesList.GetNumOfServices(),m_servicesList.GetNumOfServices());

  m_servicesList = servicesList;
  m_servicesList.SetLoaded(true);
  
  UnLockServicesList();
}

void BXServicesManager::SetServicesListIsLoaded(bool isLoaded)
{
  LockServicesList();

  m_servicesList.SetLoaded(isLoaded);

  UnLockServicesList();
}

bool BXServicesManager::GetServices(BXBoxeeServices& servicesList)
{
  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServicesList - Enter function (serv)");

  if(BOXEE::Boxee::GetInstance().IsInOfflineMode())
  {
    LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServicesList - In offline mode. Going to return FALSE (serv)");
    return false;
  }

  bool servicesListWasLoaded = false;
  
  LockServicesList();

  servicesListWasLoaded = m_servicesList.IsLoaded();
  
  while (!servicesListWasLoaded)
  {
    // ServicesList ISN'T loaded yet -> UnLock the ServicesList and wait for it to load
    
    UnLockServicesList();

    LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServicesList - ServicesList is not loaded yet. Going to try again in [%dms] (serv)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockServicesList();
      
    servicesListWasLoaded = m_servicesList.IsLoaded();
  }

  servicesList = m_servicesList;

  UnLockServicesList();

  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServicesList - Exit function. After set [ServicesListSize=%d] (serv)",servicesList.GetNumOfServices());

  return true;
}

bool BXServicesManager::GetServices(std::vector<BXServicesItem>& servicesVec)
{
  BXBoxeeServices services;
  
  GetServices(services);
  
  int numOfServices = services.GetNumOfServices();
  
  if(numOfServices < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXServicesManager::GetServices - Call to GetServices returned [ServicesListSize=%d] (serv)",numOfServices);
    return false;
  }
  
  for(int i=0; i<numOfServices; i++)
  {
    BXObject obj = services.GetService(i);
    
    std::string type = obj.GetType();
    std::string id = obj.GetID();
    std::string name = obj.GetValue(MSG_KEY_NAME);
    
    BXServicesItem servicesItem(type,id,name);
    
    servicesVec.push_back(servicesItem);
  }

  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServices - Going to return  [ServicesListSize=%d] (serv)",servicesVec.size());

  return true;
}

bool BXServicesManager::GetServicesIds(std::vector<std::string>& servicesIdsVec)
{
  BXBoxeeServices services;

  GetServices(services);

  int numOfServices = services.GetNumOfServices();

  if(numOfServices < 1)
  {
    LOG(LOG_LEVEL_ERROR,"BXServicesManager::GetServicesIds - Call to GetServices returned [ServicesListSize=%d] (serv)",numOfServices);
    return false;
  }

  for(int i=0; i<numOfServices; i++)
  {
    BXObject obj = services.GetService(i);

    servicesIdsVec.push_back(obj.GetID());
  }

  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::GetServicesIds - Going to return  [servicesIdsVecSize=%d] (serv)",servicesIdsVec.size());

  return true;
}

bool BXServicesManager::IsRegisterToServices(const std::string& serviceIdentifier, CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum)
{
  if (identifierTypeEnum >= CServiceIdentifierType::NUM_OF_SERVICE_IDENTIFIER_TYPES)
  {
    LOG(LOG_LEVEL_ERROR,"BXServicesManager::IsRegisterToServices - Enter function with invalid IdentifierType [identifierTypeEnum=%d=%s]. Going to return FALSE for [service=%s] (serv)",identifierTypeEnum,BXServicesManager::GetServiceIdentifierTypeEnumAsString(identifierTypeEnum),serviceIdentifier.c_str());
    return false;
  }

  bool servicesListWasLoaded = false;

  LockServicesList();

  servicesListWasLoaded = m_servicesList.IsLoaded();

  while (!servicesListWasLoaded)
  {
    // ServicesList ISN'T loaded yet -> UnLock the ServicesList and wait for it to load

    UnLockServicesList();

    LOG(LOG_LEVEL_DEBUG,"BXServicesManager::IsRegisterToServices - ServicesList is not loaded yet. Going to try again in [%dms] (serv)",DELAY_FOR_CHECK_FEED_LOADED);

    SDL_Delay(DELAY_FOR_CHECK_FEED_LOADED);

    LockServicesList();

    servicesListWasLoaded = m_servicesList.IsLoaded();
  }

  int numOfServices = m_servicesList.GetNumOfServices();

  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::IsRegisterToServices - Enter function with [serviceIdentifier=%s][identifierTypeEnum=%d=%s][ServicesListSize=%d] (serv)",serviceIdentifier.c_str(),identifierTypeEnum,BXServicesManager::GetServiceIdentifierTypeEnumAsString(identifierTypeEnum),numOfServices);

  if(numOfServices < 1)
  {
    UnLockServicesList();
    LOG(LOG_LEVEL_WARNING,"BXServicesManager::IsRegisterToServices - [ServicesListSize=%d] therefore going to return FALSE for [serviceIdentifier=%s] (serv)",numOfServices,serviceIdentifier.c_str());
    return false;
  }

  for (int i=0; i < numOfServices; i++)
  {
    BXObject service = m_servicesList.GetService(i);

    switch (identifierTypeEnum)
    {
    case CServiceIdentifierType::ID:
    {
      if(service.GetID() == serviceIdentifier)
      {
        UnLockServicesList();
        LOG(LOG_LEVEL_DEBUG,"BXServicesManager::IsRegisterToServices - [serviceIdentifier=%s] from type [identifierTypeEnum=%d=%s=ID] WAS found in the ServicesList. Exit function and return TRUE (serv)",serviceIdentifier.c_str(),identifierTypeEnum,BXServicesManager::GetServiceIdentifierTypeEnumAsString(identifierTypeEnum));
        return true;
      }
    }
    break;
    case CServiceIdentifierType::NAME:
    {
      if(service.GetValue("app") == serviceIdentifier)
      {
        UnLockServicesList();
        LOG(LOG_LEVEL_DEBUG,"BXServicesManager::IsRegisterToServices - [serviceIdentifier=%s] from type [identifierTypeEnum=%d=%s=NAME] WAS found in the ServicesList. Exit function and return TRUE (serv)",serviceIdentifier.c_str(),identifierTypeEnum,BXServicesManager::GetServiceIdentifierTypeEnumAsString(identifierTypeEnum));
        return true;
      }
    }
    break;
    default:
    {

    }
    break;
    }
  }

  UnLockServicesList();
  LOG(LOG_LEVEL_DEBUG,"BXServicesManager::IsRegisterToServices - [serviceIdentifier=%s] from type [%d=%s] WASN'T found in the ServicesList. Exit function and return FALSE (serv)",serviceIdentifier.c_str(),identifierTypeEnum,BXServicesManager::GetServiceIdentifierTypeEnumAsString(identifierTypeEnum));
  return false;
}

const char* BXServicesManager::GetServiceIdentifierTypeEnumAsString(CServiceIdentifierType::ServiceIdentifierTypeEnums identifierTypeEnum)
{
  switch(identifierTypeEnum)
  {
  case CServiceIdentifierType::ID:
    return "ID";
  case CServiceIdentifierType::NAME:
    return "NAME";
  default:
    LOG(LOG_LEVEL_DEBUG,"Failed to convert enum [%d] to string of ServiceIdentifierTypeEnums. Return UNKNOWN (serv)",(int)identifierTypeEnum);
    return "UNKNOWN";
  }
}

/////////////////////////////////////////////////
// RequestServicesListFromServerTask functions //
/////////////////////////////////////////////////

BXServicesManager::RequestServicesListFromServerTask::RequestServicesListFromServerTask(BXServicesManager* taskHandler, unsigned long executionDelayInMS, bool repeat):BoxeeScheduleTask("RequestServicesList",executionDelayInMS,repeat)
{
  m_taskHandler = taskHandler;
}

BXServicesManager::RequestServicesListFromServerTask::~RequestServicesListFromServerTask()
{
  
}

void BXServicesManager::RequestServicesListFromServerTask::DoWork()
{
  LOG(LOG_LEVEL_DEBUG,"RequestServicesListFromServerTask::DoWork - Enter function (serv)");

  if (!g_application.ShouldConnectToInternet())
  {
    // set loaded to true so Get() functions won't wait forever
    m_taskHandler->SetServicesListIsLoaded(true);

    LOG(LOG_LEVEL_DEBUG,"RequestServicesListFromServerTask::DoWork - [ShouldConnectToInternet=FALSE] -> Exit function (serv)");
    return;
  }

  BXBoxeeServices servicesList;
  
  servicesList.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
  servicesList.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ServicesListUrl","http://app.boxee.tv/api/get_services");

  servicesList.LoadFromURL(strUrl);

  bool isLoaded = m_taskHandler->m_servicesList.IsLoaded();
  long lastRetCode = servicesList.GetLastRetCode();

  LOG(LOG_LEVEL_DEBUG,"RequestServicesListFromServerTask::DoWork - After LoadFromURL. [lastRetCode=%d][isLoaded=%d][size=%d][currSize=%d] (serv)",lastRetCode,isLoaded,servicesList.GetNumOfServices(),m_taskHandler->m_servicesList.GetNumOfServices());

  if (!isLoaded || lastRetCode == 200)
  {
    ////////////////////////////////////////////
    // copy return result from the server if: //
    // a) servicesList isn't loaded           //
    // b) the server returned 200             //
    ////////////////////////////////////////////

    m_taskHandler->CopyServicesList(servicesList);
  }

  LOG(LOG_LEVEL_DEBUG,"RequestServicesListFromServerTask::DoWork - Exit function (serv)");

  return;
}
//////////////////////////////
// BXServicesItem functions //
//////////////////////////////

BXServicesItem::BXServicesItem(const std::string& type, const std::string& id, const std::string& name) : BXItem(type,id,name)
{

}

}

