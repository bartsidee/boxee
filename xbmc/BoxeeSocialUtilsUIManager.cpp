
#include "BoxeeSocialUtilsUIManager.h"
#include "utils/log.h"
#include "BoxeeUtils.h"
#include "GUIWebDialog.h"
#include "Application.h"
#include "GUIDialogOK2.h"
#include "LocalizeStrings.h"
#include "lib/libBoxee/bxutils.h"

using namespace BOXEE;
using namespace DIRECTORY;

CBoxeeSocialUtilsUIManager::CBoxeeSocialUtilsUIManager()
{
}

CBoxeeSocialUtilsUIManager::~CBoxeeSocialUtilsUIManager()
{
}

bool CBoxeeSocialUtilsUIManager::ConnectUISocialService(const CStdString& serviceId)
{
  CStdString url =  g_application.GetBoxeeSocialUtilsManager().GetConnectLink(serviceId);
#ifdef CANMORE
  if (url == "")
  {
    CLog::Log(LOGERROR,"CBoxeeSocialUtilsUIManager::ConnectUISocialService - url is empty return FALSE");
    return false;
  }

  CLog::Log(LOGDEBUG,"CBoxeeSocialUtilsUIManager::ConnectUISocialService - open GUIWebDialog with [url=%s]", url.c_str());

  return CGUIWebDialog::ShowAndGetInput(url);
#else
  CStdString text = g_localizeStrings.Get(80001);
  text += " " + url;
  CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(10014),text);
  return false;
#endif
}

bool CBoxeeSocialUtilsUIManager::HandleUISocialUtilDisconnect(const CStdString& serviceId)
{
  CLog::Log(LOGDEBUG,"CBoxeeSocialUtilsUIManager::HandleUISocialUtilDisconnect - disconnecting [%s]", serviceId.c_str());

  Json::Value jResponse;
  int returnCode;
  CStdString url = g_application.GetBoxeeSocialUtilsManager().GetDisconnectLink(serviceId);

  if (BOXEE::BXUtils::PerformJSONGetRequestInBG(url, jResponse, returnCode) != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CBoxeeSocialUtilsUIManager::HandleUISocialUtilDisconnect: failed calling the server. serviceId=%s RC=%d", serviceId.c_str(), returnCode);
    return false;
  }

  return true;
}

bool CBoxeeSocialUtilsUIManager::HandleUISocialUtilConnect(const CStdString& serviceId)
{
  if (g_application.GetBoxeeSocialUtilsManager().IsConnected(serviceId, true) &&
      !g_application.GetBoxeeSocialUtilsManager().RequiresReconnect(serviceId, true))
  {
    CLog::Log(LOGDEBUG,"CBoxeeSocialUtilsUIManager::HandleUISocialUtilConnect - [%s] is already enable ",serviceId.c_str());
    return true;
  }

  // The server instructed us that a refresh is needed, so we need to disconnect before we connect
  if (g_application.GetBoxeeSocialUtilsManager().RequiresReconnect(serviceId))
  {
    HandleUISocialUtilDisconnect(serviceId);
  }

  if (!ConnectUISocialService(serviceId))
  {
    CLog::Log(LOGERROR,"CBoxeeSocialUtilsUIManager::HandleUISocialUtilConnect - FAILED to connect [%s]",serviceId.c_str());
    return false;
  }

  return true;
}
