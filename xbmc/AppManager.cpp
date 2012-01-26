/*
 *      Copyright (C) 2005-2009 Team Boxee
 *      http://www.boxee.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */


#include "AppDescriptor.h"
#include "AppManager.h"
#include "Util.h"
#include "File.h"
#include "SkinInfo.h"
#include "Directory.h"
#include "FileItem.h"
#include "GUIWindowApp.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "TextureManager.h"
#include "bxcurl.h"
#include "Settings.h"
#include "bxversion.h"
#include "bxcurl.h"
#ifdef __APPLE__
#include "SystemInfo.h"
#endif
#include "ZipManager.h"
#include "GUIWindowBoxeeBrowse.h"
#include "lib/libPython/XBPython.h"
#include "File.h"
#include "IPlayer.h"
#include "SpecialProtocol.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "utils/SingleLock.h"
#include "VideoInfoTag.h"
#include "lib/libBoxee/bxutils.h"
#include "RenderSystem.h"

CAppManager::CAppManager()
{
  m_nLastWinId = WINDOW_APPS_START + 500;
  m_iWindowIdCounter = WINDOW_APPS_START + 501;
}

CAppManager::~CAppManager()
{
}

CAppManager& CAppManager::GetInstance()
{
  static CAppManager instance;
  return instance;
}

bool CAppManager::Launch(const CFileItem& item, bool bReportInstall)
{
  m_launchedItem = item;
  return Launch(item.m_strPath, bReportInstall);
}

bool CAppManager::Launch(const CStdString& urlStr, bool bReportInstall)
{
  CLog::Log(LOGDEBUG,"CAppManager::Launch - Enter function with [url=%s][bReportInstall=%d]. App [label=%s][path=%s] (bapps)(erez)",urlStr.c_str(),bReportInstall,m_launchedItem.GetLabel().c_str(),m_launchedItem.m_strPath.c_str());
  CLog::Log(LOGINFO, "Launching app: %s (applaunch)", urlStr.c_str());
  //GetAppsStats();
  // Loading the skin of the app
  if (!CAppDescriptor::Exists(urlStr))
  {
    CLog::Log(LOGINFO, "Unable to load descriptor for app: %s, trying to install", urlStr.c_str());

    CURL url(urlStr);
    
    InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), true, false, bReportInstall);
    if (!CUtil::RunInBG(job))
    {
      return false;
    }
  }

  // Load the descriptor
  CAppDescriptor desc;
  desc.Load(urlStr);
  if (!desc.IsLoaded())
  {
    CLog::Log(LOGERROR, "Unable to load descriptor for app: %s", urlStr.c_str());
    return false;
  }

  // Only check upgrade here for boxee apps (which is not TestApp)
  if (!desc.IsTestApp()) // removed: desc.IsBoxeeApp() && 
  {
    InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(desc.GetId(), true, true, bReportInstall);
    if (CUtil::RunInBG(job))
    {
      // Reload the descriptor after upgrade
      desc.Load(urlStr);
    }
    else
    {
      return false;
    }
  }

  //soon to be unleashed
  UpdateAppStat(desc.GetId());
#ifdef APPS_DIR_MAX_SIZE
  LimitAppsDirSize(APPS_DIR_MAX_SIZE);
#endif


  // Report to the server about the launch app
  BoxeeUtils::ReportLaunchApp(desc.GetId(), &m_launchedItem);

  if (desc.GetType() == "skin")
  {
    LaunchSkinApp(desc, urlStr);
  }
  else if (desc.GetType() == "url")
  {
    LaunchUrlApp(desc);
  }
  else if (desc.GetType() == "html")
  {
    LaunchHtmlApp(desc);
  }
  else if (desc.GetType() == "plugin")
  {
    LaunchPluginApp(desc);
  }
#ifdef HAS_NATIVE_APPS
  else if (desc.GetType() == "native")
  {
    LaunchNativeApp(desc, urlStr);
  }
#endif
  
  return true;
}

void CAppManager::LaunchSkinApp(const CAppDescriptor& desc, const CStdString& urlStr)
{
  CLog::Log(LOGDEBUG, "CAppManager::LaunchSkinApp, id = %s, url = %s (applaunch)", desc.GetId().c_str(), urlStr.c_str());

  CURL url(urlStr);
  if (desc.GetId() != m_lastLaunchedAppId)
  {
    CStdString mediaPath = desc.GetMediaPath();

    CStdString xmlFile = "";
    CStdString skinPath = desc.GetSkinPath(xmlFile);

    CFileItemList skinItems;
    DIRECTORY::CDirectory::GetDirectory(skinPath, skinItems);
    for (int i = 0; i < skinItems.Size(); i++)
    {
      CFileItemPtr pItem = skinItems[i];
      CLog::Log(LOGDEBUG, "Loading skin %s for app %s",  pItem->m_strPath.c_str(), desc.GetId().c_str());

      // Load the skin file to check it's a window and get the id
      TiXmlDocument xmlDoc;
      if (!xmlDoc.LoadFile(pItem->m_strPath))
      {
        CLog::Log(LOGERROR, "Unable to load skin file %s", pItem->m_strPath.c_str());
        continue;
      }

      TiXmlElement* rootElement = xmlDoc.RootElement();
      if (strcmpi(rootElement->Value(), "window") != 0)
      {
        CLog::Log(LOGERROR, "Invalid skin file %s. No window root element", pItem->m_strPath.c_str());
        continue;
      }

      const char* windowIdStr = rootElement->Attribute("id");
      if (!windowIdStr)
      {
        CLog::Log(LOGERROR, "Invalid skin file %s. No window id in root element", pItem->m_strPath.c_str());
        continue;
      }

      // Create a new window from the skin file
      int windowId = atoi(windowIdStr);

      // APPWINDOW: Translate window id to correct window
      int realWindowId = CreateWindowId(desc.GetId(), windowId);

      CLog::Log(LOGDEBUG, "Get window for id = %d, translated id = %d, app id = %s (applaunch)", windowId, realWindowId, desc.GetId().c_str());
      CGUIWindow* newWindow = g_windowManager.GetWindow(realWindowId);

      if (newWindow == NULL)
      {
        CLog::Log(LOGDEBUG, "Create new window for id = %d, translated id = %d, app id = %s (applaunch)", windowId, realWindowId, desc.GetId().c_str());
        if (rootElement->Attribute("type") && strcmp(rootElement->Attribute("type"), "dialog") == 0)
        {
          newWindow = new CGUIDialog(realWindowId, pItem->m_strPath);
        }
        else
        {
          newWindow = new CGUIWindowApp(realWindowId, pItem->m_strPath, desc);
        }

        // Add the custom window to the window manager
        g_windowManager.AddCustomWindow(newWindow);
      }
      else
      {
        CLog::Log(LOGDEBUG, "Found existing window for id = %d, translated id = %d, app id = %s (applaunch)", windowId, realWindowId, desc.GetId().c_str());
      }

//      // If a window with the same id exists, delete it
//      if (g_windowManager.GetWindow(realWindowId) != NULL)
//      {
//        // we can do this because this is for sure a window of this specific application
//        g_windowManager.Delete(realWindowId);
//      }

    }

    // Load the app strings      
    ClearPluginStrings();
    LoadPluginStrings(desc);
    
    // Load the registry of the application
    m_registry.Load(desc);

    // Clear all the parameters of the previous windows
    m_params.clear();

    // Remember the last running application
    m_lastLaunchedAppId = desc.GetId();
    m_lastLaunchedDescriptor = desc;
  }

  int windowId = 0;
  int realWindowId = 0;
  if (url.GetFileName() == "")
  {
    windowId = atoi(desc.GetStartWindow().c_str());
    realWindowId = GetWindowId(desc.GetId(), windowId);
    m_params[realWindowId].clear();
  }
  else
  {
    windowId = atoi(url.GetFileName().c_str());
    if (windowId != 0)
    {
      realWindowId = GetWindowId(desc.GetId(), windowId);
      m_params[realWindowId].clear();
      m_params[realWindowId] = url.GetOptionsAsMap();
      CLog::Log(LOGDEBUG, "Add params for window %d (applaunch)", windowId);
    }
    else
    {
      // I am not sure this is necessary but I put it here to preserve previous functionality
      m_params[windowId].clear();
      m_params[windowId] = url.GetOptionsAsMap();
    }
  }
  
  if (windowId != 0)
  {
    // Activate the window of the application... we're good to go!
    CGUIWindowApp* pAppWindow = (CGUIWindowApp*) g_windowManager.GetWindow(realWindowId);
    if (pAppWindow) 
    {
        pAppWindow->ClearStateStack();
        g_windowManager.ActivateWindow(GetWindowId(desc.GetId(), windowId));
    }
    else
    {
      CLog::Log(LOGERROR, "Unable to retrieve window with id = %d", windowId);
    }
  }
  else
  {
    CStdString python = desc.GetLocalPath();
    if (url.GetFileName().size() > 0)
      python = CUtil::AddFileToFolder(python, url.GetFileName());
    else
      python = CUtil::AddFileToFolder(python, desc.GetStartWindow());
    python += ".py";
    
    CStdString actualPython = python;
    if (!XFILE::CFile::Exists(actualPython))
    {
      actualPython = python;
      actualPython += "o";
      if (!XFILE::CFile::Exists(actualPython))
      {
        actualPython = python;
        actualPython += "c";
        if (!XFILE::CFile::Exists(actualPython))
        {
          CLog::Log(LOGERROR, "Cannot find pythong file (tried also pyc, pyo): %s (python)", python.c_str());
        }
      }
    }
      
    // Remove ? at the beginning of the options, if it exists
    CStdString options = url.GetOptions();
    if (options.Left(1) == "?")
    {
      options = options.Right(options.size() - 1);
    }
    
    const char* args[2];
    args[0] = python.c_str();
    args[1] = options.c_str();
    
    CLog::Log(LOGINFO, "Launch python file: %s (python)", python.c_str());
    CStdString strAppId = url.GetHostName();
    CLog::Log(LOGINFO, "Application id is: %s (python)", strAppId.c_str());
    g_pythonParser.evalFileInContext(actualPython, strAppId, 2, args);
  } 
}

void CAppManager::LoadPluginStrings(const CAppDescriptor& desc)
{
  // Path where the language strings reside
  CStdString pathToLanguageFile = desc.GetLocalPath();
  CStdString pathToFallbackLanguageFile = desc.GetLocalPath();
  CUtil::AddFileToFolder(pathToLanguageFile, "language", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "language", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, g_guiSettings.GetString("locale.language"), pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "english", pathToFallbackLanguageFile);
  CUtil::AddFileToFolder(pathToLanguageFile, "strings.xml", pathToLanguageFile);
  CUtil::AddFileToFolder(pathToFallbackLanguageFile, "strings.xml", pathToFallbackLanguageFile);

  // Load language strings temporarily
  g_localizeStringsTemp.Load(pathToLanguageFile, pathToFallbackLanguageFile);
}

void CAppManager::ClearPluginStrings()
{
  // Unload temporary language strings
  g_localizeStringsTemp.Clear();
}

bool CAppManager::IsPlayable(const CStdString& urlStr)
{
  CURL url(urlStr);
  if (url.GetFileName() == "")
    return false;
  
  int windowId = atoi(url.GetFileName().c_str());
  if (windowId != 0)
    return false;
  
  return true;
}

const CStdString& CAppManager::GetLauchedAppParameter(const DWORD windowId, const CStdString& key)
{
  if (m_params.find(windowId) == m_params.end())
  {
    return StringUtils::EmptyString;
  }

  if (m_params[windowId].find(key) == m_params[windowId].end())
  {
    return StringUtils::EmptyString;
  }

  return m_params[windowId][key];
}

const std::map<CStdString, CStdString>& CAppManager::GetLauchedParameters(const DWORD windowId)
{
  return m_params[windowId];
}

void CAppManager::SetLauchedAppParameter(const DWORD windowId, const CStdString& key, const CStdString& value)
{
  CLog::Log(LOGDEBUG, "CAppManager::SetLauchedAppParameter, window id = %d, key = %s, value = %s (applaunch)", windowId, key.c_str(), value.c_str());
  m_params[windowId][key] = value;
}

void CAppManager::LaunchUrlApp(const CAppDescriptor& desc)
{
  CLog::Log(LOGDEBUG, "CAppManager::LaunchUrlApp, id = %s (applaunch)", desc.GetId().c_str());
  if (g_windowManager.GetActiveWindow() != WINDOW_BOXEE_BROWSE)
  {
    CGUIWindowBoxeeBrowse::Show(desc.GetURL(), "other", desc.GetName(), desc.GetBackgroundImageURL(), true, desc.GetId());
  }
  else
  {
    CFileItem* pItem = new CFileItem();
    pItem->m_strPath = desc.GetURL();
    pItem->SetLabel(desc.GetName());
    pItem->SetProperty("isrss", true);
    pItem->SetProperty("appid", desc.GetId());
    pItem->SetProperty("BrowseBackgroundImage", desc.GetBackgroundImageURL().c_str());
    CGUIMessage message(GUI_MSG_SET_CONTAINER_PATH, WINDOW_BOXEE_BROWSE, 0);
    message.SetPointer(pItem);
    g_windowManager.SendMessage(message);    
  }
}

void CAppManager::LaunchHtmlApp(const CAppDescriptor& desc)
{
  CLog::Log(LOGDEBUG, "CAppManager::LaunchHtmlApp, id = %s (applaunch)", desc.GetId().c_str());
  CStdString url = desc.GetURL();
  CUtil::URLEncode(url);
  CStdString path = "flash://"+desc.GetId()+"/?src="+url;
  CStdString controller = desc.GetController();
  if (!controller.IsEmpty())
  {
    CUtil::URLEncode(controller);
    path += "&bx-jsactions=" + controller;
  }
  
  CFileItem item;
  item.m_strPath = path;
  item.SetLabel(desc.GetName());
  item.GetVideoInfoTag()->m_strPlot = desc.GetDescription();
  item.SetProperty("appid", desc.GetId());
  g_application.PlayFile(item);
}

void CAppManager::LaunchPluginApp(const CAppDescriptor& desc)
{
  CLog::Log(LOGDEBUG, "CAppManager::LaunchPluginApp, id = %s (applaunch)", desc.GetId().c_str());
  m_lastLaunchedAppId = desc.GetId();
  m_lastLaunchedDescriptor = desc;

  CStdString url = "plugin://";
  url += desc.GetMediaType() + "/" + desc.GetId(); // We pass here the id and not the fq id since PluginDirectory will fix
  CGUIWindowBoxeeBrowse::Show(url,"other", desc.GetName(), desc.GetBackgroundImageURL(), false);
}

void CAppManager::LaunchNativeApp(const CAppDescriptor& desc, const CStdString &path)
{
  CLog::Log(LOGDEBUG, "CAppManager::LaunchNativeApp, id = %s (applaunch)", desc.GetId().c_str());
#ifdef HAS_NATIVE_APPS
  g_application.getApplicationMessenger().MediaStop();
  
  CSingleLock lock(m_lock);
  m_lastLaunchedAppId = desc.GetId();
  m_lastLaunchedDescriptor = desc;
  
  // must unlock for the duration of the "launch" 
  lock.Leave();
  BOXEE::NativeApplication *app = new BOXEE::NativeApplication;
  if (app->Launch(desc, path))
  {
    lock.Enter();
    m_mapNativeApps[desc.GetId()] = app;
  }
  else
    delete app;
#endif
}

void CAppManager::RemoveNativeApp(CStdString id)
{
#ifdef HAS_NATIVE_APPS
  CSingleLock lock(m_lock);
  delete m_mapNativeApps[id];
  m_mapNativeApps.erase(id);
#endif
}

void CAppManager::PingNativeApps()
{
#ifdef HAS_NATIVE_APPS
  CSingleLock lock(m_lock);
  std::map<CStdString, BOXEE::NativeApplication *>::iterator i = m_mapNativeApps.begin();
  while (i != m_mapNativeApps.end())
  {
    i->second->ExecuteRenderOperations();
    i++;
  }
#endif  
}

CAppRegistry& CAppManager::GetRegistry()
{
  return m_registry;
}

CAppDescriptor::AppDescriptorsMap CAppManager::GetInstalledApps()
{
  m_installedApplications.clear();
  CStdString appsPath = "special://home/apps/";
  appsPath = _P(appsPath);

  GetInstalledAppsInternal(m_installedApplications, appsPath, "", false);
  RefreshAppsStats();

  return m_installedApplications;
}

void CAppManager::RegisterPlayerCallback(const CStdString& appId, IPlayerCallback* playerCallback) 
{
  // Add callback to the map of apps registered to receive playback callbacks
  CLog::Log(LOGDEBUG, "CAppManager::RegisterPlayerCallback, register callback for id = %s (callback)", appId.c_str());
  m_playerCallbacks[appId] = playerCallback;
}

void CAppManager::OnPlayBackStarted()
{
  CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackStarted (flow)");
  /*
  // Run over all registered applications and call their functions in matching context
  std::map<CStdString, IPlayerCallback* >::iterator it = m_playerCallbacks.begin();
  while (it != m_playerCallbacks.end()) {
    CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackStarted call python parser with id %s (callback)", it->first.c_str());
    g_pythonParser.OnPlayBackStarted(it->first, it->second);
    it++;
  }
  */

  std::vector<IEventCallback* >::iterator it1 = m_eventCallbacks.begin();
  while (it1 != m_eventCallbacks.end()) {
    (*it1)->OnPlayBackStarted();
    it1++;
  }
  
}

void CAppManager::OnPlayBackEnded(bool bError, const CStdString& error)
{
  CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackEnded (flow)");
  
  /*
  // Run over all registered applications and call their functions in matching context
  std::map<CStdString, IPlayerCallback* >::iterator it = m_playerCallbacks.begin();
  
  while (it != m_playerCallbacks.end()) {
    CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackEnded call python parser with id %s (callback)", it->first.c_str());
    g_pythonParser.OnPlayBackEnded(it->first, it->second);
    it++;
  }
  */
  
  std::vector<IEventCallback* >::iterator it1 = m_eventCallbacks.begin();
  while (it1 != m_eventCallbacks.end()) {
    (*it1)->OnPlayBackEnded(bError, error);
    it1++;
  }
  
}

void CAppManager::OnPlayBackStopped()
{
  CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackStopped (flow)");
  
  /*
  // Run over all registered applications and call their functions in matching context
  std::map<CStdString, IPlayerCallback* >::iterator it = m_playerCallbacks.begin();
  
  while (it != m_playerCallbacks.end()) {
    CLog::Log(LOGDEBUG, "CAppManager::OnPlayBackStopped call python parser with id %s (callback)", it->first.c_str());
    g_pythonParser.OnPlayBackStopped(it->first, it->second);
    it++;
  }
  */
  
  std::vector<IEventCallback* >::iterator it1 = m_eventCallbacks.begin();
  while (it1 != m_eventCallbacks.end()) {
    (*it1)->OnPlayBackStopped();
    it1++;
  }
}

void CAppManager::OnQueueNextItem()
{
  CLog::Log(LOGDEBUG, "CAppManager::OnQueueNextItem (flow)");
  
  /*
  // Run over all registered applications and call their functions in matching context
  std::map<CStdString, IPlayerCallback* >::iterator it = m_playerCallbacks.begin();
  while (it != m_playerCallbacks.end()) {
    CLog::Log(LOGDEBUG, "CAppManager::OnQueueNextItem call python parser with id %s (callback)", it->first.c_str());
    g_pythonParser.OnQueueNextItem(it->first, it->second);
    it++;
  }
  */
  std::vector<IEventCallback* >::iterator it1 = m_eventCallbacks.begin();
  while (it1 != m_eventCallbacks.end()) {
    (*it1)->OnQueueNextItem();
    it1++;
  }
}

void CAppManager::RegisterEventCallback(IEventCallback* eventCallback) 
{
  CLog::Log(LOGDEBUG, "CAppManager::RegisterEventCallback (flow)");
  m_eventCallbacks.push_back(eventCallback);
}

void CAppManager::RemoveEventCallback(IEventCallback* eventCallback) 
{
  CLog::Log(LOGDEBUG, "CAppManager::RemoveEventCallback (flow)");
  std::vector<IEventCallback* >::iterator it = m_eventCallbacks.begin();
    while (it != m_eventCallbacks.end()) {
      if ((*it) == eventCallback) {
        m_eventCallbacks.erase(it);
        return;
      }
      it++;
    }
}

void CAppManager::RegisterActionCallback(IActionCallback* actionCallback) 
{
  CLog::Log(LOGDEBUG, "CAppManager::RegisterActionCallback (flow)");
  m_actionCallbacks.push_back(actionCallback);
}

void CAppManager::RemoveActionCallback(IActionCallback* actionCallback) 
{
  CLog::Log(LOGDEBUG, "CAppManager::RemoveEventCallback (flow)");
  std::vector<IActionCallback* >::iterator it = m_actionCallbacks.begin();
    while (it != m_actionCallbacks.end()) {
      if ((*it) == actionCallback) {
        m_actionCallbacks.erase(it);
        return;
      }
      it++;
    }
}

void CAppManager::OnActionNext()
{
  // Run over all registered applications and call their functions in matching context
  std::vector<IActionCallback* >::iterator it = m_actionCallbacks.begin();
  while (it != m_actionCallbacks.end()) {
    (*it)->OnActionNextItem();
    it++;
  }
}

void CAppManager::OnActionStop()
{
  // Run over all registered applications and call their functions in matching context
  std::vector<IActionCallback* >::iterator it = m_actionCallbacks.begin();
  while (it != m_actionCallbacks.end()) {
    (*it)->OnActionStop();
    it++;
  }
}

void CAppManager::GetInstalledAppsInternal(CAppDescriptor::AppDescriptorsMap& result, CStdString startPath, CStdString repository, bool recurse)
{
  CFileItemList appItems;
  DIRECTORY::CDirectory::GetDirectory(startPath, appItems);
  for (int i = 0; i < appItems.Size(); i++)
  {
    CFileItemPtr pItem = appItems[i];
    if (pItem->m_bIsFolder)
    {
      CStdString descriptorFile = pItem->m_strPath;
      CUtil::AddFileToFolder(descriptorFile, "descriptor.xml", descriptorFile);
      
      if (XFILE::CFile::Exists(descriptorFile))
      {      
        CStdString appName = pItem->m_strPath;
        CUtil::RemoveSlashAtEnd(appName);
        appName = CUtil::GetFileName(appName);
  
        CStdString path = "app://";
        path += appName;
  
        CAppDescriptor desc(path);
        if (desc.IsLoaded())
        {
          result[desc.GetId()] = desc;
        }
      }
      else if (recurse)
      {
        // If descriptor.xml not, found, search child directories (only first level)
        CStdString pathWitoutSlash = pItem->m_strPath;
        CUtil::RemoveSlashAtEnd(pathWitoutSlash);
        GetInstalledAppsInternal(result, pItem->m_strPath, CUtil::GetFileName(pathWitoutSlash), false); 
      }
    }
  }  
}

bool CAppManager::UninstallApp(const CStdString& appId)
{
  CAppDescriptor::AppDescriptorsMap installedAppsDesc = GetInstalledApps();

  if (installedAppsDesc.find(appId) == installedAppsDesc.end())
  {
    CLog::Log(LOGDEBUG, "Request to uninstall non existnt app: %s", appId.c_str());
    return true;
  }

  CLog::Log(LOGINFO, "Uninstalling app. Deleting directory: %s", appId.c_str());
#ifdef _LINUX
  std::string cmd = "/bin/rm -rf ";
  cmd += "'";
  cmd += installedAppsDesc[appId].GetLocalPath();
  cmd += "'";
  system(cmd.c_str());
#else
  CUtil::WipeDir(installedAppsDesc[appId].GetLocalPath());
#endif
  
  // Report to the server about the uninstall app
  BoxeeUtils::ReportRemoveApp(appId);

  return true;
}

bool CAppManager::InstallApp(const CStdString& appId, bool reportInstall)
{
  CAppDescriptor::AppDescriptorsMap availableAppsDesc;
  if (appId.Find(".") > 0)
  {
    availableAppsDesc = GetRepositories().GetAvailableApps(false, false);
  }
  else
  {
    availableAppsDesc = GetRepositories().GetAvailableApps(false, true);
  }
  
  if (availableAppsDesc.find(appId) == availableAppsDesc.end())
  {
    CLog::Log(LOGERROR, "Request to install non existant app: %s", appId.c_str());
    return false;
  }

  CAppDescriptor& appDesc = availableAppsDesc[appId];

  CStdString tempFile = "special://temp/";
  tempFile += appDesc.GetId();
  tempFile += "-";
  tempFile += appDesc.GetVersion();
  tempFile += ".zip";
  tempFile = _P(tempFile);

  CStdString downloadURL = appDesc.GetRepository();
  downloadURL += "/download/";
  downloadURL += appDesc.GetId();
  downloadURL += "-";
  downloadURL += appDesc.GetVersion();
  
  if (appDesc.GetType() == "native")
  {
    downloadURL += "-";
    #if defined(BOXEE_DEVICE)
      downloadURL += BOXEE_DEVICE;
    #elif defined(__APPLE__)
      if (CSysInfo::IsAppleTV())
        downloadURL += "atv";
      else
        downloadURL += "mac";
    #elif defined(_LINUX)
      downloadURL += "linux";
    #else
      downloadURL += "win";
    #endif
  }
  
  downloadURL += ".zip";

  CLog::Log(LOGINFO, "Installing app. Downloading from URL: %s.", downloadURL.c_str());

  CStdString dlMessage;
  dlMessage.Format(g_localizeStrings.Get(53840), appDesc.GetName().c_str());
  g_application.m_guiDialogKaiToast.QueueNotification("", "", dlMessage, 10000);  

  BOXEE::BXCurl curl;
  try
  {
    if (!curl.HttpDownloadFile(downloadURL, tempFile, ""))
    {
      CLog::Log(LOGERROR, "Error downloading application.");
      g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(53841));  
      return false;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error downloading application.");
    g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(53841));
    return false;
  }

  CStdString targetDir = _P("special://home/apps/");
  
  CZipManager zip;
  try
  {
    g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(53842));
    if (!zip.ExtractArchive(tempFile, targetDir, false))
    {
      g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(53843));
      CLog::Log(LOGERROR, "Error extracting application.");
      return false;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "Error extracting application.");
    return false;
  }
  ::DeleteFile(tempFile.c_str());

  // Create directory to store registry.xml
  CStdString fileName = _P(g_settings.GetProfileUserDataFolder());
  CUtil::AddFileToFolder(fileName, "apps", fileName);
  ::CreateDirectory(fileName, NULL);
  CUtil::AddFileToFolder(fileName, appDesc.GetId(), fileName);
  ::CreateDirectory(fileName, NULL);
  
  // If we are upgrading, delete the source from sources.xml as it 
  // may no longer be accurate
  CBoxeeMediaSourceList sourceList;

  CBoxeeMediaSource newSource;

  CAppManager::AppDescriptorToBoxeeMediaSource(appDesc,newSource);
  
  //sourceList.deleteSourceByAppId(appDesc.GetId());
  //sourceList.addSource(newSource);

  if(reportInstall)
  {
    // Report to the server about the install app
    BoxeeUtils::ReportInstallApp(appDesc);
  }

  g_application.m_guiDialogKaiToast.QueueNotification("", g_localizeStrings.Get(53844));

  return true;
}

void InstallOrUpgradeAppBG::Run()
{
  if (m_bInstall && m_bUpgrade)
  {
    m_bJobResult = CAppManager::GetInstance().InstallOrUpgradeAppIfNeeded(m_strAppId,m_bReportInstall);
  }
  else if (m_bInstall) {
    m_bJobResult = CAppManager::GetInstance().InstallApp(m_strAppId,m_bReportInstall);
  }
  else if (m_bUpgrade) {
    m_bJobResult = CAppManager::GetInstance().UpgradeApp(m_strAppId);
  }
}

bool CAppManager::InstallOrUpgradeAppIfNeeded(const CStdString& appId, bool bReportInstall)
{
  CAppDescriptor::AppDescriptorsMap installedAppsDesc = GetInstalledApps();
  // If the the application is not found -- install it
  if (installedAppsDesc.find(appId) == installedAppsDesc.end())
  {
    InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(appId, true, false, bReportInstall);
    return CUtil::RunInBG(job);
  }

  CAppDescriptor& appDesc = installedAppsDesc[appId];
  
  // Do not automatically upgrade non boxee apps;
  /*if (!appDesc.IsBoxeeApp())
  {
    return true;
  }*/
  
  CAppDescriptor::AppDescriptorsMap availableAppsDesc = GetRepositories().GetAvailableApps(false, appDesc.IsBoxeeApp());
  if (CUtil::VersionCompare(installedAppsDesc[appId].GetVersion(), availableAppsDesc[appId].GetVersion()) < 0)
  {
    InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(appId, false, true, bReportInstall);
    return CUtil::RunInBG(job);
  }
  
	return true;
}

bool CAppManager::UpgradeApp(const CStdString& appId)
{
  if (!UninstallApp(appId))
  {
    return false;
  }
  
  return InstallApp(appId,true);
}

const CStdString& CAppManager::GetLastLaunchedId()
{
  return m_lastLaunchedAppId;
}

CAppDescriptor& CAppManager::GetLastLaunchedDescriptor()
{
  return m_lastLaunchedDescriptor;
}

CAppRepositories& CAppManager::GetRepositories()
{
  return m_repositories;
}

int CAppManager::GetRepositoriesSize()
{
  return m_repositories.Size();
}

CFileItem& CAppManager::GetLastLaunchedItem()
{
  return m_launchedItem;
}

void CAppManager::AppDescriptorToBoxeeMediaSource(const CAppDescriptor& appDesc,CBoxeeMediaSource& boxeeMediaSource)
{
  boxeeMediaSource.name = appDesc.GetName();
  boxeeMediaSource.thumbPath = appDesc.GetThumb();
  boxeeMediaSource.path = "app://";
  boxeeMediaSource.path += appDesc.GetId();
  boxeeMediaSource.path += "/";
  boxeeMediaSource.isVideo = (appDesc.GetMediaType() == "video");
  boxeeMediaSource.isMusic = (appDesc.GetMediaType() == "music");
  boxeeMediaSource.isPicture = (appDesc.GetMediaType()  == "pictures");
  boxeeMediaSource.isPrivate = false;
  boxeeMediaSource.isAdult = appDesc.IsAdult();
  appDesc.GetCountryRestrictions(boxeeMediaSource.country, boxeeMediaSource.countryAllow);
}

int CAppManager::CreateWindowId(const CStdString& strAppId, int iWindowId)
{
  int windowId = GetWindowId(strAppId, iWindowId);
  if (windowId != WINDOW_INVALID)
  {
    return windowId;
  }

  m_iWindowIdCounter++;
  std::map<int,int> windowMap;
  windowMap[iWindowId] = m_iWindowIdCounter;
  m_mapAppWindows[strAppId] = windowMap;

  return m_iWindowIdCounter;
}

int CAppManager::GetWindowId(const CStdString& strAppId, int iWindowId)
{
  CSingleLock lock(m_lock);
  CLog::Log(LOGDEBUG,"%s - requested window id  = %d, for app id = %s (applaunch)", __FUNCTION__, iWindowId, strAppId.c_str());
  
  if (iWindowId < WINDOW_APPS_START)
  {
    CLog::Log(LOGDEBUG,"%s - requested regular window. returning unchanged.", __FUNCTION__);
    return iWindowId;
  }
  
  std::map<CStdString, std::map<int,int> >::iterator it = m_mapAppWindows.find(strAppId);
  if (it != m_mapAppWindows.end())
  {
    std::map<int,int> windowMap = it->second;

    std::map<int,int>::iterator it2 = windowMap.find(iWindowId);
    if (it2 != windowMap.end())
    {
      CLog::Log(LOGDEBUG,"%s - requested window id  = %d, for app id = %s, returning = %d (applaunch)", __FUNCTION__, iWindowId, strAppId.c_str(), it2->second);
      return it2->second;
    }
    else
    {
      m_iWindowIdCounter++;
      windowMap[iWindowId] = m_iWindowIdCounter;
      m_mapAppWindows[strAppId] = windowMap;

      return m_iWindowIdCounter;
    }
  }
  
  return WINDOW_INVALID;
}

bool CAppManager::GetAppByWindowId(int iWindowId, CStdString& strAppId)
{
  // Perform reverse lookup to get application id from provided window id

  std::map<CStdString, std::map<int,int> >::iterator it = m_mapAppWindows.begin();
  while(it != m_mapAppWindows.end())
  {
    CStdString strCurrentApp = it->first;

    std::map<int,int> windowMap = it->second;

    std::map<int,int>::iterator it2 = windowMap.begin();
    while (it2 != windowMap.end())
    {
      if (it2->second == iWindowId)
      {
        strAppId = strCurrentApp;
        return true;
      }
      ++it2;
    }
    ++it;
  }
  return false;
}

void CAppManager::GetAppWindows(const CStdString& strAppId, std::vector<int> vecAppWindows)
{
  vecAppWindows.clear();
  std::map<int,int> windowMap = m_mapAppWindows[strAppId];

  std::map<int,int>::iterator it = windowMap.begin();
  while(it != windowMap.end())
  {
    int realId = it->first;
    int windowId = it->second;

    CLog::Log(LOGERROR,"%s - window for app id = %s, real id = %d, mapped id = %d", __FUNCTION__, strAppId.c_str(), realId, windowId);
    vecAppWindows.push_back(windowId);
    it++;
  }
}

int  CAppManager::AcquireWindowID()
{
  CSingleLock lock(m_lock);
  int n = -1;
  if (m_windowIdPool.size() == 0)
  {
    if (m_nLastWinId < WINDOW_APPS_END)
      n = m_nLastWinId++;
    else
    {
      CLog::Log(LOGERROR,"%s - too many windows allocated!", __FUNCTION__);
    }
  }
  else
  {
    n = m_windowIdPool[0];
    m_windowIdPool.erase(m_windowIdPool.begin());
  }
  
  return n;
}

void CAppManager::ReleaseWindowID(int nID)
{
  CSingleLock lock(m_lock);
  m_windowIdPool.push_back(nID);
}

class CloseAppJob : public IGUIThreadTask
{
public:
  CloseAppJob(const CStdString& strAppId) : m_strAppId(strAppId) { }
  virtual void DoWork()
  {
    // Activate home window
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS,"apps://all");

    g_pythonParser.RemoveContext(m_strAppId);

    // Get all windows that belong to the applicaton
    std::vector<int> vecAppWindows;
    CAppManager::GetInstance().GetAppWindows(m_strAppId, vecAppWindows);

    // Delete all windows
    for (size_t i = 0; i < vecAppWindows.size(); i++)
    {
      g_windowManager.Delete(vecAppWindows[i]);
    }
  }

  CStdString m_strAppId;
};

void CAppManager::Close(const CStdString& strAppId)
{
  CLog::Log(LOGERROR,"%s - close application, id = %s", __FUNCTION__, strAppId.c_str());

  CloseAppJob* j = new CloseAppJob(strAppId);
  g_application.getApplicationMessenger().ExecuteOnMainThread(j, false, true);

}

void CAppManager::LimitAppsDirSize(uint32_t appsDirMAxSize)
{
  typedef std::multimap<int, CStdString> appsMmapT;
  appsMmapT appsMmap;

  CStdString fileName = _P("special://profile/apps/apps.xml");
  CStdString strValue, appName;
  std::map<CStdString, TiXmlNode *> appIdToXmlNodeMap;
  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;

  //load xml file
  if ( xmlDoc.LoadFile( fileName) )
  {
    pRootElement = xmlDoc.RootElement();
    if (pRootElement)
    {
      strValue = pRootElement->Value();
      if ( strValue != "apps")
      {
        CLog::Log(LOGERROR, "failed to load apps.xml file");
        return;
      }
    }
  }

  TiXmlNode *pAppNode = pRootElement->FirstChild("app");
  TiXmlNode *pIdNode = NULL, *pLastOpenedDateNode = NULL;

  CLog::Log(LOGDEBUG, "reading apps.xml file");
  // read the content of app.xml file
  while (pAppNode > 0)
  {
    pIdNode = pAppNode->FirstChild("id");
    appName = pIdNode->FirstChild()->Value();
    pLastOpenedDateNode = pAppNode->FirstChild("lastopeneddate");
    int lastOpenedTime = atoi(pLastOpenedDateNode->FirstChild()->Value());
    appsMmap.insert(std::make_pair(lastOpenedTime, appName));
    appIdToXmlNodeMap.insert(std::make_pair(appName, pAppNode));
    pAppNode = pAppNode->NextSiblingElement("app");
  }

  CStdString appsDir = _P("special://home/apps/");

  CLog::Log(LOGDEBUG, "check apps dir size and change it if needed");
  //delete oldest applications if size of special://home/apps/ is bigger than allowed
  appsMmapT::iterator it = appsMmap.begin();
  for (;it != appsMmap.end(); it++)
  {
    // if dir is small enough, break
    if (CUtil::GetDirSize(appsDir) < appsDirMAxSize)
    {
      break;
    }

    // get the least recently used app
    CStdString appName = (*it).second;
    CLog::Log(LOGINFO, "delete application %s", appName.c_str());
    pRootElement->RemoveChild(appIdToXmlNodeMap[appName]);
    CAppManager::UninstallApp(appName);
  }

  //file have been changed, save
  xmlDoc.SaveFile();
}

void CAppManager::UpdateAppStat(const std::string& AppId)
{
  CStdString fileName = _P("special://profile/apps/apps.xml");
  CStdString strValue, currAppStr;
  CStdString tmp;
  CStdString appsPath = _P("special://home/apps/");
  CAppDescriptor::AppDescriptorsMap installedAppsDesc;

  GetInstalledAppsInternal(installedAppsDesc, appsPath, "", false);

  TiXmlDocument xmlDoc;
  TiXmlElement *pRootElement = NULL;
  TiXmlNode *pTempNode = NULL;
  bool fixDoc = true, deleteNode = false;

  CLog::Log(LOGINFO, "updating %s's information in apps.xml", AppId.c_str());

  if ( xmlDoc.LoadFile( fileName) )
  {
    pRootElement = xmlDoc.RootElement();
    if (pRootElement)
    {
      strValue = pRootElement->Value();
      if ( strValue == "apps")
      {
        fixDoc = false;
      }
    }
  }

  if (fixDoc)
  {
    if (pRootElement)
    {
      xmlDoc.RemoveChild(pRootElement);
    }
    else
    {
      pTempNode = xmlDoc.FirstChild();
      if (pTempNode > 0)
        xmlDoc.RemoveChild(pTempNode);
    }

    pRootElement = new TiXmlElement( "apps" );
    pRootElement->SetAttribute("version", "1.0");
    xmlDoc.LinkEndChild(pRootElement);
  }

  TiXmlNode *pAppNode = pRootElement->FirstChild("app");
  TiXmlNode *pOpenedCntNode = NULL, *pIdNode = NULL, *pLastOpenedDateNode = NULL;

  while (pAppNode > 0)
  {
    pIdNode = pAppNode->FirstChild("id");
    if (pIdNode && pIdNode->FirstChild())
    {
      currAppStr = pIdNode->FirstChild()->Value();
      if (currAppStr == AppId)
      {
        pLastOpenedDateNode = pAppNode->FirstChild("lastopeneddate");
        pOpenedCntNode = pAppNode->FirstChild("timesopened");
        if (pOpenedCntNode && pOpenedCntNode->FirstChild())
        {

          int openedCnt = atoi (pOpenedCntNode->FirstChild()->Value());
          openedCnt++;
          tmp = BOXEE::BXUtils::IntToString(openedCnt);

          pOpenedCntNode->FirstChild()->SetValue(tmp.c_str());
          //CLog::Log(LOGDEBUG,"    Found name: %s", strName.c_str());
        }
        else
        {
          if (pOpenedCntNode)
          {
            pAppNode->RemoveChild(pOpenedCntNode);
          }
          TiXmlElement * timesOpenedElement = new TiXmlElement( "timesopened" );
          TiXmlText * timesOpenedText = new TiXmlText( "1" );
          timesOpenedElement->LinkEndChild( timesOpenedText );
          pAppNode->LinkEndChild(timesOpenedElement);
        }
        if (pLastOpenedDateNode)
        {
          pAppNode->RemoveChild(pLastOpenedDateNode);
        }

        tmp = BOXEE::BXUtils::IntToString(std::time(NULL));
        TiXmlElement * lastOpenedElement = new TiXmlElement( "lastopeneddate" );
        TiXmlText * lastOpenedText = new TiXmlText(tmp.c_str());
        lastOpenedElement->LinkEndChild( lastOpenedText );
        pAppNode->LinkEndChild(lastOpenedElement);
      }

      //intersect installed applications and the ones that apps.xml knows about

      if (installedAppsDesc.find(currAppStr) == installedAppsDesc.end())
      {
        CLog::Log(LOGINFO, "deleting %s from app.xml file\n", currAppStr.c_str());
        pTempNode = pAppNode;
        deleteNode = true;
      }
      else
      {
        deleteNode = false;
      }


      CLog::Log(LOGDEBUG, "deleting %s from apps map\n", currAppStr.c_str());
      installedAppsDesc.erase(currAppStr);
    }
    pAppNode = pAppNode->NextSiblingElement("app");

    if (deleteNode)
    {
      pRootElement->RemoveChild(pTempNode);
    }

  }

  tmp = BOXEE::BXUtils::IntToString(std::time(NULL));

  CAppDescriptor::AppDescriptorsMap::iterator it = installedAppsDesc.begin();
  for (; it != installedAppsDesc.end(); it++)
  {
    TiXmlElement * appElement = new TiXmlElement( "app" );
    pRootElement->LinkEndChild(appElement);

    TiXmlElement * appIdElement = new TiXmlElement( "id" );
    TiXmlText * appIdText = new TiXmlText(it->first);
    appIdElement->LinkEndChild( appIdText );
    appElement->LinkEndChild(appIdElement);

    TiXmlElement * timesOpenedElement = new TiXmlElement( "timesopened" );
    TiXmlText * timesOpenedText = new TiXmlText( "1" );
    timesOpenedElement->LinkEndChild( timesOpenedText );
    appElement->LinkEndChild(timesOpenedElement);

    TiXmlElement * lastOpenedElement = new TiXmlElement( "lastopeneddate" );
    TiXmlText * lastOpenedText = new TiXmlText(tmp.c_str());
    lastOpenedElement->LinkEndChild( lastOpenedText );
    appElement->LinkEndChild(lastOpenedElement);

    CLog::Log(LOGINFO, "adding %s to app.xml file\n", it->first.c_str());

  }

  xmlDoc.SaveFile();

  m_mapAppNameToStat.clear();
  pRootElement = xmlDoc.RootElement();
  pAppNode = pRootElement->FirstChild("app");

  CLog::Log(LOGDEBUG, "reading apps.xml file");
  // read the content of app.xml file
  while (pAppNode > 0)
  {
    pIdNode = pAppNode->FirstChild("id");
    currAppStr = pIdNode->FirstChild()->Value();
    pLastOpenedDateNode = pAppNode->FirstChild("lastopeneddate");
    pOpenedCntNode = pAppNode->FirstChild("timesopened");
    int lastOpenedTime = 1;
    int timesOpened = 1;
    if (pOpenedCntNode && pLastOpenedDateNode->FirstChild())
    {
      lastOpenedTime = BOXEE::BXUtils::StringToInt(pLastOpenedDateNode->FirstChild()->Value());
    }
    if (pOpenedCntNode && pOpenedCntNode->FirstChild())
    {
      timesOpened = BOXEE::BXUtils::StringToInt(pOpenedCntNode->FirstChild()->Value());
    }
    std::map<CStdString, int> statsMap;
    statsMap["lastopeneddate"] = lastOpenedTime;
    statsMap["timesopened"] = timesOpened;
    m_mapAppNameToStat[currAppStr] = statsMap;
    pAppNode = pAppNode->NextSiblingElement("app");
  }
}

void CAppManager::RefreshAppsStats()
{
  UpdateAppStat("");
}

int CAppManager::GetAppLastOpenedDate(const std::string& AppId)
{

  int lastOpenedDateNode = 0;

  std::map<CStdString, std::map<CStdString, int> >::iterator it = m_mapAppNameToStat.find(AppId);

  if (it != m_mapAppNameToStat.end())
  {
    lastOpenedDateNode = m_mapAppNameToStat[AppId]["lastopeneddate"];
  }

  return lastOpenedDateNode;

}

int CAppManager::GetAppTimesOpened(const std::string& AppId)
{

  int openedTimes = 0;

  std::map<CStdString, std::map<CStdString, int> >::iterator it = m_mapAppNameToStat.find(AppId);

  if (it != m_mapAppNameToStat.end())
  {
    openedTimes = m_mapAppNameToStat[AppId]["timesopened"];
  }

  return openedTimes;

}

void CAppManager::StopAllApps()
{
  if(m_lastLaunchedAppId != "") 
  {
    g_pythonParser.RemoveContextAll();
    m_lastLaunchedAppId = "";
  }
}