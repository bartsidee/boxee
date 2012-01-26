
#include "BoxeeSocialUtilsManager.h"
#include "utils/log.h"
#include "BoxeeUtils.h"

using namespace BOXEE;
using namespace DIRECTORY;

CBoxeeSocialUtilsManager::CBoxeeSocialUtilsManager()
{
  Reset();
}

CBoxeeSocialUtilsManager::~CBoxeeSocialUtilsManager()
{
}

bool CBoxeeSocialUtilsManager::Initialize()
{
  if (m_bIsInit)
  {
    CLog::Log(LOGWARNING,"CBoxeeSocialUtilsManager::InitBoxeeSocialUtilsManager - reset BoxeeSocialUtilsManager didn't occur");
  }

  Reset();

  if (!GetStatusFromServer())
  {
    CLog::Log(LOGERROR,"CBoxeeSocialUtilsManager::InitBoxeeSocialUtilsManager() - FAILED to get social services status from server.");
    return false;
  }

  m_bIsInit = true;

  return true;
}

void CBoxeeSocialUtilsManager::Reset()
{
  m_bIsInit = false;

  m_serviceMap.clear();
  m_serviceMap[FACEBOOK_SERVICE_ID] = SocialServiceInfo();
  m_serviceMap[TWITTER_SERVICE_ID] = SocialServiceInfo();
  m_serviceMap[TUMBLR_SERVICE_ID] = SocialServiceInfo();
}

bool CBoxeeSocialUtilsManager::IsInitialized()
{
  return m_bIsInit;
}

CStdString CBoxeeSocialUtilsManager::GetConnectLink(const CStdString& serviceId)
{
#ifdef CANMORE
  return m_serviceMap[serviceId].link;
#else
  return m_serviceMap[serviceId].externalLink;
#endif
}

CStdString CBoxeeSocialUtilsManager::GetDisconnectLink(const CStdString& serviceId)
{
  return m_serviceMap[serviceId].disconnectLink;
}

bool CBoxeeSocialUtilsManager::GetStatusFromServer()
{
  int retCode;
  Job_Result jobResult = BoxeeUtils::GetShareServicesJson(m_jsonServiceList, retCode);

  CLog::Log(LOGDEBUG,"CBoxeeSocialUtilsManager::GetSocialServicesStatus - call to get SocialServices status from server returned [jobResult=%d] ",jobResult);

  if (jobResult != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CBoxeeSocialUtilsManager::GetSocialServicesStatus - FAILED to get SocialServices status from server. [jobResult=%d]",jobResult);
    return false;
  }

  m_servicesList.Clear();
  m_serviceMap.clear();

  BoxeeUtils::ParseJsonShareServicesToFileItems(m_jsonServiceList,m_servicesList);

  CLog::Log(LOGDEBUG,"CBoxeeSocialUtilsManager::GetSocialServicesStatus - after parse SocialServices to FIleItemList. [NumOfSocialServices=%d]",m_servicesList.Size());

  for (int i = 0; i < m_servicesList.Size(); i++)
  {
    CFileItemPtr item = m_servicesList.Get(i);

    CStdString currentServiceId = item->GetProperty("serviceId");

    if (currentServiceId == FACEBOOK_SERVICE_ID ||
        currentServiceId == TWITTER_SERVICE_ID ||
        currentServiceId == TUMBLR_SERVICE_ID)
    {
      m_serviceMap[currentServiceId].isConnected = item->GetPropertyBOOL("enable");
      m_serviceMap[currentServiceId].requiresUpdate = item->GetPropertyBOOL("refresh");
      m_serviceMap[currentServiceId].link = item->GetProperty("connect");
      m_serviceMap[currentServiceId].disconnectLink = item->GetProperty("disconnect");
      m_serviceMap[currentServiceId].externalLink = item->GetProperty("link");
    }
  }

  return true;
}

bool CBoxeeSocialUtilsManager::IsConnected(const CStdString& serviceId, bool getDataFromServer)
{
  if (getDataFromServer)
  {
    GetStatusFromServer();
  }

  return m_serviceMap[serviceId].isConnected;
}

bool CBoxeeSocialUtilsManager::RequiresReconnect(const CStdString& serviceId, bool getDataFromServer)
{
  if (getDataFromServer)
  {
    GetStatusFromServer();
  }

  return m_serviceMap[serviceId].requiresUpdate;
}

bool CBoxeeSocialUtilsManager::IsAnyConnected(bool getDataFromServer)
{
  bool connected = true;

  if(!IsConnected(FACEBOOK_SERVICE_ID, getDataFromServer) &&
     !IsConnected(TWITTER_SERVICE_ID, getDataFromServer) &&
     !IsConnected(TUMBLR_SERVICE_ID, getDataFromServer))
  {
    connected = false;
  }

  return connected;
}

void CBoxeeSocialUtilsManager::SetIsConnected(const CStdString& serviceId, bool isConnected)
{
  m_serviceMap[serviceId].isConnected = isConnected;
}

