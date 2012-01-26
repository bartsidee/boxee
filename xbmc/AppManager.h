#ifndef APP_MANAGER_H_
#define APP_MANAGER_H_

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

#include <map>
#include <vector>
#include "FileItem.h"
#include "AppRegistry.h"
#include "Thread.h"
#include "AppRepositories.h"
#include "BoxeeMediaSourceList.h"
#include "nativeapp/NativeApplication.h"
#include "utils/CriticalSection.h"
#include "app/DllSkinNativeApp.h"
#include "app/XAPP_Native.h"

class IPlayerCallback;

class InstallOrUpgradeAppBG : public IRunnable
{
public:
  InstallOrUpgradeAppBG(const CStdString& strAppId, bool bInstall, bool bUpgrade, bool bReportInstall = true)
  {
    m_strAppId = strAppId;
    m_bInstall = bInstall;
    m_bUpgrade = bUpgrade;
    m_bReportInstall = bReportInstall;
    m_bJobResult = false;
  }
  
  virtual ~InstallOrUpgradeAppBG() { }
  
  virtual void Run();
  CStdString m_strAppId;
  bool m_bInstall;
  bool m_bUpgrade;
  bool m_bReportInstall;
};

class InstallAppBG : public IRunnable
{
public:
  virtual ~InstallAppBG() {}

  virtual void Run();
  bool m_bResult;
};

class IActionCallback
{
public:
  virtual ~IActionCallback() {}
  virtual void OnActionNextItem() = 0;
  virtual void OnActionPrevItem() = 0;
  virtual void OnActionStop() = 0;
  virtual void OnActionSeek(int iTime) = 0;
  virtual void OnActionOsdExt(int amount) = 0;
};

class IEventCallback
{
public:
  virtual ~IEventCallback() {}  
  virtual void OnPlayBackEnded(bool bError = false, const CStdString& error = "") = 0;
  virtual void OnPlayBackStarted() = 0;
  virtual void OnPlayBackStopped() = 0;
  virtual void OnQueueNextItem() = 0;
};

class CAppManager
{
public:
  virtual ~CAppManager();

  static CAppManager& GetInstance();
  
  bool Launch(const CStdString& urlStr, bool bReportInstall = true);
  bool Launch(const CFileItem& item, bool bReportInstall = true);
  CAppRegistry& GetRegistry();
  CAppDescriptor::AppDescriptorsMap GetInstalledApps();
  bool UpgradeApp(const CStdString& appId);
  bool UninstallApp(const CStdString& appId);
  bool InstallApp(const CStdString& appId, bool reportInstall);
  bool InstallOrUpgradeAppIfNeeded(const CStdString& appId, bool bReportInstall = true);
  const CStdString& GetLauchedAppParameter(const DWORD windowId, const CStdString& key);
  const std::map<CStdString, CStdString>& GetLauchedParameters(const DWORD windowId);  
  bool IsPlayable(const CStdString& urlStr);
  void SetLauchedAppParameter(const DWORD windowId, const CStdString& key, const CStdString& value);
  const CStdString& GetLastLaunchedId();
  CAppDescriptor& GetLastLaunchedDescriptor();
  CAppDescriptor& GetDescriptor(const CStdString& strAppId);
  CFileItem& GetLastLaunchedItem();
  CAppRepositories& GetRepositories();
  int GetRepositoriesSize();
  
  void RegisterPlayerCallback(const CStdString& appId, IPlayerCallback* playerCallback);
  void RegisterEventCallback(IEventCallback* eventCallback);
  void RemoveEventCallback(IEventCallback* eventCallback); 
  void OnPlayBackStarted();
  void OnPlayBackEnded(bool bError = false, const CStdString& error = "");
  void OnPlayBackStopped();
  void OnQueueNextItem();
  void OnPlaybackSeek(int iTime);
  
  void RegisterActionCallback(IActionCallback* actionCallback);
  void RemoveActionCallback(IActionCallback* actionCallback); 
  void OnActionNext();
  void OnActionPrev();
  void OnActionStop();
  void OnOsdExt(int amount);

  void ClearPluginStrings();
  void LoadPluginStrings(const CAppDescriptor& desc);

  void AppDescriptorToBoxeeMediaSource(const CAppDescriptor& appDesc,CBoxeeMediaSource& boxeeMediaSource);

  int CreateWindowId(const CStdString& strAppId, int iWindowId);
  int GetWindowId(const CStdString& strAppId, int iWindowId);
  bool GetAppByWindowId(int iWindowId, CStdString& strAppId);
  int  AcquireWindowID();
  void ReleaseWindowID(int nID);
  void GetAppWindows(const CStdString& strAppId, std::vector<int>& vecAppWindows);
  void ReleaseAppWindows(const CStdString& strAppId);
  void Close(const CStdString& strAppId);
  void RemoveNativeApp(CStdString id); // the parameter is not a const reference on purpose to avoid someone calling it with a reference
                                       // to an item in the app which is about to be destroyed. safety measure.
  void PingNativeApps();
  void RefreshAppsStats();
  int GetAppTimesOpened(const std::string& AppId);
  int GetAppLastOpenedDate(const std::string& AppId);
  void StopAllApps();
  
  const char* GetCurrentContextAppId();


  bool HasPartnerThreadRunning(const CStdString& partnerId, const ThreadIdentifier& threadId);
private:
  CAppManager();
  
  void LaunchSkinApp(const CAppDescriptor& appDesc, const CStdString& url, bool isNative);
  void LaunchUrlApp(const CAppDescriptor& desc);
  void LaunchHtmlApp(const CAppDescriptor& desc);
  void LaunchPluginApp(const CAppDescriptor& desc);
  void LaunchNativeApp(const CAppDescriptor& desc, const CStdString &path);
  
  void GetInstalledAppsInternal(CAppDescriptor::AppDescriptorsMap& result, CStdString startPath, CStdString repository, bool recurse);
  //void GetAppsStats();
  void UpdateAppStat(const std::string& AppId);
  void LimitAppsDirSize(uint32_t appsDirMAxSize);
  
  CAppDescriptor::AppDescriptorsMap m_installedApplications;
  CStdString m_lastLaunchedAppId;
  CAppDescriptor m_lastLaunchedDescriptor;
  std::map<DWORD, std::map<CStdString, CStdString> > m_params;
  std::map<CStdString, IPlayerCallback* > m_playerCallbacks;
  std::vector<IActionCallback* > m_actionCallbacks;
  std::vector<IEventCallback* > m_eventCallbacks;
  CAppRegistry m_registry;
  CAppRepositories m_repositories;
  CFileItem m_launchedItem;
  
  std::map<CStdString, BOXEE::NativeApplication *> m_mapNativeApps;
  
  std::map<CStdString, std::map<CStdString, int> > m_mapAppNameToStat;

  std::map<CStdString, std::map<int,int> > m_mapAppWindows;
  int m_iWindowIdCounter;

  std::vector<int> m_windowIdPool;
  int              m_nLastWinId;
  bool             m_bVerifyAppStatus;
  CCriticalSection m_lock;

  DllSkinNativeApp  m_dll;
  XAPP::NativeApp*  m_nativeApp;
  bool              m_bNativeAppStarted;

  std::vector<LibraryLoader*> m_sharedLibs;
};

bool BoxeeValidatePartner(const char* partnerId, const int developer_indication);

#endif /*APP_MANAGER_H_*/
