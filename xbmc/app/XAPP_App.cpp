
#include "XAPP_App.h"
#include "AppManager.h"
#include "Util.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "ThreadPolicy.h"

namespace XAPP
{

const std::string Parameters::toQueryString() const
{
  std::string url;
  
  for (Parameters::const_iterator it = begin(); it != end(); it++)
  {
    if (it != begin())
    {
      url += "&";
    }
    
    url += it->first;
    url += "=";
    CStdString value = it->second;
    CUtil::URLEncode(value);
    url += value;
  }
  
  return url;
}

LocalConfig& App::GetLocalConfig()
{
  return s_localConfig;
}

void App::ActivateWindow(int windowId, const Parameters& parameters)
{
#if defined(HAS_EMBEDDED) && !defined(__APPLE__) 
  TPDisablePolicy(gettid(), false);
#endif
  CStdString url;
  url.Format("app://%s/%d?", CAppManager::GetInstance().GetLastLaunchedId(), windowId); 
  url += parameters.toQueryString();
  CAppManager::GetInstance().Launch(url);
#if defined(HAS_EMBEDDED) && !defined(__APPLE__) 
  TPDisablePolicy(gettid(), true);
#endif

}

void App::Close()
{
  CAppManager::GetInstance().Close(GetId());
}

void App::RunScript(const std::string& scriptName, const Parameters& parameters)
{
  CStdString url;
  url.Format("app://%s/%s?", CAppManager::GetInstance().GetLastLaunchedId(), scriptName.c_str());
  url += parameters.toQueryString();
  CAppManager::GetInstance().Launch(url);  
}

void App::SendMessage(const std::string& handler, const std::string& parameter)
{
  std::vector<CStdString> vecParams;
  vecParams.push_back(GetId());
  vecParams.push_back(handler);
  vecParams.push_back(parameter);

  ThreadMessage tMsg ( TMSG_APP_HANDLE, 0, 0, "", vecParams, "", NULL, NULL );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

}

std::string App::GetId()
{
  return CAppManager::GetInstance().GetLastLaunchedId();
}

std::string App::GetAppDir()
{
  return CAppManager::GetInstance().GetLastLaunchedDescriptor().GetLocalPath();
}

std::string App::GetAppMediaDir()
{
  return CAppManager::GetInstance().GetLastLaunchedDescriptor().GetMediaPath();
}

std::string App::GetAppSkinDir()
{
  CStdString fileName = "";
  return CAppManager::GetInstance().GetLastLaunchedDescriptor().GetSkinPath(fileName);
}

XAPP::Parameters App::GetLaunchedWindowParameters()
{
  Parameters result;
  const std::map<CStdString, CStdString>& params = CAppManager::GetInstance().GetLauchedParameters(g_windowManager.GetActiveWindow());
  
  std::map<CStdString, CStdString>::const_iterator iter;
  for (iter = params.begin(); iter != params.end(); ++iter) 
  {
    result[iter->first] = iter->second;
  }  

  return result;
}

XAPP::Parameters App::GetLaunchedScriptParameters()
{
  Parameters result;
  const std::map<CStdString, CStdString>& params = CAppManager::GetInstance().GetLauchedParameters(0);
  
  std::map<CStdString, CStdString>::const_iterator iter;
  for (iter = params.begin(); iter != params.end(); ++iter) 
  {
    result[iter->first] = iter->second;
  }  
  
  return result;
}

std::string App::GetAuthenticationToken()
{
  CStdString result = g_application.GetBoxeeAuthenticator().AuthenticateApp(CAppManager::GetInstance().GetLastLaunchedId());
  return result;
}

XAPP::ListItem App::GetLaunchedListItem()
{
  CFileItemPtr itemPtr(new CFileItem(CAppManager::GetInstance().GetLastLaunchedItem()));
  ListItem item(itemPtr);
  return item;
}

LocalConfig App::s_localConfig;

}
