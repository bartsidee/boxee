#ifndef GUI_WINDOW_APPLICATION_SETTINGS
#define GUI_WINDOW_APPLICATION_SETTINGS

#pragma once

#include "GUIDialog.h"
#include "FileItem.h"
#include "FileSystem/FactoryDirectory.h"
#include "AppDescriptor.h"
#include "Thread.h"

class CGUIWindowBoxeeApplicationSettings : public CGUIWindow
{
public:
  CGUIWindowBoxeeApplicationSettings(void);
  virtual ~CGUIWindowBoxeeApplicationSettings(void);
  virtual void OnInitWindow();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  void RefreshLists(bool forceReload);

private:
  void RefreshListsBG();
  void CreateListNewApps();
  void CreateListExistingApps();
  void CreateListRepositories();
  void GetUnusedPlugins(CFileItemList& unusedPluginsList);
  void GetUnusedLastfm(CFileItemList& unusedLastfmList);
  void GetUnusedShoutcast(CFileItemList& unusedLastfmList);
  void GetUnusedApps(CFileItemList& unusedAppsList);  
  bool IsSourceInUse(CStdString sourcePath,VECSOURCES* pSharesList);

  bool AddAppToList(CFileItemList& appsList,const CFileItemPtr app);

  CFileItemList m_availableAppsFileItems;
  CAppDescriptor::AppDescriptorsMap m_availableAppsDesc;
  CAppDescriptor::AppDescriptorsMap m_installedAppsDesc;
  int m_availableAppsDescLoadTime;
  
  CFileItemList m_listNewApps;
  CFileItemList m_listExistingApps;
  CFileItemList m_listExistingFeeds;
  CFileItemList m_listRepositories;
};

class RefreshAppListBG : public IRunnable
{
public:
  RefreshAppListBG(CGUIWindowBoxeeApplicationSettings* theDialog, bool forceReload) : m_theDialog(theDialog)
  {
    m_bJobResult = false;
    m_theDialog = theDialog;
    m_forceReload = forceReload;
  }
  
  virtual void Run();
  
private:
  CGUIWindowBoxeeApplicationSettings* m_theDialog;
  bool m_forceReload;
};

#endif
