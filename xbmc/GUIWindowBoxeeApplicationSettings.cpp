
#include <vector>
#include "GUIWindowBoxeeApplicationSettings.h"
#include "Application.h"
#include "GUIDialogProgress.h"
#include "GUIWindowManager.h"
#include "GUIDialogPluginSettings.h"
#include "FileSystem/FactoryDirectory.h"
#include "FileSystem/File.h"
#include "GUIDialogOK.h"
#include "Util.h"
#include "BoxeeMediaSourceList.h"
#include "Settings.h"
#include "MediaManager.h"
#include "FileSystem/Directory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "AppManager.h"
#include "AppDescriptor.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogOK2.h"
#include "GUIDialogProgress.h"
#include "GUIDialogYesNo2.h"
#include "GUIDialogBoxeeApplicationAction.h"
#include "URL.h"
#include "AppDescriptor.h"
#include "GUIDialogBoxeeRssFeedInfo.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "ItemLoader.h"

#define CONTROL_TITLE_LABEL            40
// lists
#define CONTROL_LIST_NEW_APPS          52
#define CONTROL_LIST_FEEDS             152
#define CONTROL_LIST_REMOVE_APPS       252
#define CONTROL_LIST_REPOSITORIES      352
// buttons
#define CONTROL_BUTTON_ADD_FEED        151
#define CONTROL_BUTTON_ADD_REPOSITORY  351

#define AVAILABLE_APPS_INDEX_CACHE_TIME (60 * 60)

CGUIWindowBoxeeApplicationSettings::CGUIWindowBoxeeApplicationSettings(void)
    : CGUIWindow(WINDOW_BOXEE_APPLICATION_SETTINGS, "boxee_application_list.xml")
{
  m_availableAppsDescLoadTime = 0;
}

CGUIWindowBoxeeApplicationSettings::~CGUIWindowBoxeeApplicationSettings(void)
{}

bool CGUIWindowBoxeeApplicationSettings::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
    
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeApplicationSettings::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {    
      DWORD senderId = message.GetSenderId();

      if (senderId == CONTROL_LIST_REMOVE_APPS)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_REMOVE_APPS);
        OnMessage(msg);
        int iItem = msg.GetParam1();

        CGUIDialogBoxeeApplicationAction *pDlgAction = (CGUIDialogBoxeeApplicationAction*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_APPLICATION_ACTION);
        if (pDlgAction)
        {
          pDlgAction->SetAppItem(m_listExistingApps[iItem]);
          pDlgAction->DoModal();
        }
        
        RefreshListsBG();
        
        return true;
      }
      else if (senderId == CONTROL_LIST_NEW_APPS)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_NEW_APPS);
        OnMessage(msg);
        int iItem = msg.GetParam1();

        bool response;
        if (CGUIDialogYesNo2::ShowAndGetInput(52039, 52038, response))
        {
          CStdString message;
          CURI url(m_listNewApps[iItem]->m_strPath);
          if (url.GetProtocol() == "app")
          {
            InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), true, false);
            if (CUtil::RunInBG(job) == JOB_SUCCEEDED)
            {
              message = g_localizeStrings.Get(52016);
            }
            else
            {
              message = g_localizeStrings.Get(52017);
            }
          }
          
          RefreshListsBG();

          CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(52039), message);          
        }
        
        return true;
      }  
      else if (senderId == CONTROL_LIST_FEEDS)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_FEEDS);
        OnMessage(msg);
        int iItem = msg.GetParam1();

        bool response;
        if (CGUIDialogYesNo2::ShowAndGetInput(52039, 52019, response))
        {
          CBoxeeMediaSourceList sourceList;
          sourceList.deleteSource(m_listExistingFeeds[iItem]->GetLabel());    
          g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_MINUS, "", g_localizeStrings.Get(51039), 5000, KAI_GREEN_COLOR, KAI_GREEN_COLOR);
        }

        RefreshListsBG();

        return true;
      }
      else if (senderId == CONTROL_LIST_REPOSITORIES)
      {
        CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_REPOSITORIES);
        OnMessage(msg);
        int iItem = msg.GetParam1();
        
        if (iItem != 0)
        {
          
          bool response;
          if (CGUIDialogYesNo2::ShowAndGetInput(117, 52037, response))
          {
            CAppManager::GetInstance().GetRepositories().Delete(m_listRepositories[iItem]->GetProperty("id"));
            CAppManager::GetInstance().GetRepositories().Save();
            RefreshListsBG();
          }
        }
        
        return true;
      }    
      else if (senderId == CONTROL_BUTTON_ADD_REPOSITORY) 
      {
        CStdString repositoryUrl;
        if (CGUIDialogKeyboard::ShowAndGetInput(repositoryUrl, g_localizeStrings.Get(52035), false))
        {
          CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
          if (progress) 
          {
            progress->StartModal();
            progress->Progress();
          }
          
          CAppRepository repo(repositoryUrl);
  
          progress->Close();
          
          if (repo.IsValid())
          {
            CAppManager::GetInstance().GetRepositories().Add(repo);
            CAppManager::GetInstance().GetRepositories().Save();
            RefreshListsBG();
          }
          else
          {
            CGUIDialogOK2::ShowAndGetInput(257, 52036);
          }
          return true;
        }
      }
      else if (senderId == CONTROL_BUTTON_ADD_FEED) 
      {
        CGUIDialogBoxeeRssFeedInfo* addFeedDialog = (CGUIDialogBoxeeRssFeedInfo *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_RSS_FEED_INFO);
        if (addFeedDialog) 
        {
          addFeedDialog->DoModal();
          RefreshListsBG();          
        } 
        return true;
      }
    }
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeApplicationSettings::OnInitWindow()
{
  CGUIWindow::OnInitWindow();  
  RefreshListsBG();
}

void RefreshAppListBG::Run()
{
  m_theDialog->RefreshLists(m_forceReload);
  m_bJobResult = true;
}

void CGUIWindowBoxeeApplicationSettings::RefreshListsBG()
{
  CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
  if (progress) 
  {
    progress->StartModal();
    progress->Progress();
  }
  
  bool forceReload = false;
  if (m_availableAppsDescLoadTime == 0 || m_availableAppsDescLoadTime + AVAILABLE_APPS_INDEX_CACHE_TIME < time(NULL))
  {
    m_availableAppsDescLoadTime = time(NULL);
    forceReload = true;
  }
  
  m_availableAppsFileItems.Clear();
  
  RefreshAppListBG* job = new RefreshAppListBG(this, forceReload);
  CUtil::RunInBG(job);
  
  CreateListNewApps();
  CreateListExistingApps();
  CreateListRepositories();
  
  g_application.GetItemLoader().AddControl(GetID(), CONTROL_LIST_REMOVE_APPS, m_listExistingApps);
  g_application.GetItemLoader().AddControl(GetID(), CONTROL_LIST_FEEDS, m_listExistingFeeds);
  g_application.GetItemLoader().AddControl(GetID(), CONTROL_LIST_NEW_APPS, m_listNewApps);
  g_application.GetItemLoader().AddControl(GetID(), CONTROL_LIST_REPOSITORIES, m_listRepositories);
  
  progress->Close();
  
  SET_CONTROL_FOCUS(9000, 0);  
}

void CGUIWindowBoxeeApplicationSettings::RefreshLists(bool forceReload)
{    
  m_installedAppsDesc.clear();
  m_installedAppsDesc = (CAppManager::GetInstance().GetInstalledApps());
  m_availableAppsDesc.clear();
  m_availableAppsDesc = (CAppManager::GetInstance().GetRepositories().GetAvailableApps(forceReload));

  CAppManager::GetInstance().GetRepositories().GetAvailableApps(m_availableAppsDesc, m_availableAppsFileItems);
}

void CGUIWindowBoxeeApplicationSettings::CreateListNewApps()
{
  // Clear the list first
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_NEW_APPS);
  OnMessage(msgReset);

  m_listNewApps.Clear();
  
  CFileItemList unusedPluginsList;
  
  GetUnusedPlugins(unusedPluginsList);

  for (int i=0; i<unusedPluginsList.Size(); i++)
  {    
    CFileItemPtr unusedPlugin (new CFileItem(*(unusedPluginsList[i])));
    m_listNewApps.Add(unusedPlugin);
    
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_NEW_APPS, 0, 0, unusedPlugin);
    OnMessage(msg);
  }

  // Get unused lastFM
  CFileItemList unusedLastfmList;
  GetUnusedLastfm(unusedLastfmList);

  for (int i=0; i<unusedLastfmList.Size(); i++)
  {    
    CFileItemPtr unusedLastfm (new CFileItem(*(unusedLastfmList[i])));
    m_listNewApps.Add(unusedLastfm);
    
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_NEW_APPS, 0, 0, unusedLastfm);
    OnMessage(msg);
  }
  
  // Get unused Shoutcast
  CFileItemList unusedShoutcastList;
  GetUnusedShoutcast(unusedShoutcastList);

  for (int i=0; i<unusedShoutcastList.Size(); i++)
  {    
    CFileItemPtr unusedShoutcast (new CFileItem(*(unusedShoutcastList[i])));
    m_listNewApps.Add(unusedShoutcast);
    
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_NEW_APPS, 0, 0, unusedShoutcast);
    OnMessage(msg);
  }
  
  // Get unused apps
  CFileItemList unusedAppsList;
  GetUnusedApps(unusedAppsList);

  for (int i=0; i<unusedAppsList.Size(); i++)
  {    
    CFileItemPtr unusedApp (new CFileItem(*(unusedAppsList[i])));
    m_listNewApps.Add(unusedApp);
    
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_NEW_APPS, 0, 0, unusedApp);
    OnMessage(msg);
  }
  
  m_listNewApps.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);  
}

void CGUIWindowBoxeeApplicationSettings::CreateListExistingApps()
{
  // Clear the list of sources
  CGUIMessage msgResetApps(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_REMOVE_APPS);
  OnMessage(msgResetApps);
  m_listExistingApps.Clear();
  
  CGUIMessage msgResetFeeds(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_FEEDS);
  OnMessage(msgResetFeeds);
  m_listExistingFeeds.Clear();

  CBoxeeMediaSourceList sourceList;
  BoxeeMediaSourceMap::iterator sourcesIterator;
    
  for (sourcesIterator = sourceList.getMap().begin(); sourcesIterator != sourceList.getMap().end(); sourcesIterator++)
  {    
    CBoxeeMediaSource& source = (*sourcesIterator).second;
    if (!(CUtil::IsPlugin(source.path))    && 
        !(CUtil::IsLastFM(source.path))    &&
        !(CUtil::IsShoutCast(source.path)) &&
        !(CUtil::IsRSS(source.path)) &&
        !(CUtil::IsApp(source.path)))
    {
      continue;
    }

    // Create new share FileItem
    CFileItemPtr share ( new CFileItem(source.name) );
    share->m_strPath = source.path;
    share->SetThumbnailImage(source.thumbPath);
    if (source.isVideo) share->SetProperty("app-media", "video");
    if (source.isMusic) share->SetProperty("app-media", "music");
    if (source.isPicture) share->SetProperty("app-media", "pictures");
    
    if (!(CUtil::IsRSS(source.path))) 
    {
      share->SetProperty("app-hassettings", false);
      share->SetProperty("app-hasupgrade", false);
    }

    if (CUtil::IsLastFM(source.path))
    {
      share->SetProperty("app-hassettings", true);
    } 
    
    if (CUtil::IsApp(source.path))
    {
      CURI appUrl(source.path);
      CStdString id = appUrl.GetHostName();
      
      CAppDescriptor::AppDescriptorsMap& availableAppsDesc = m_availableAppsDesc;
      CAppDescriptor::AppDescriptorsMap& installedAppsDesc = m_installedAppsDesc;
      
      if (installedAppsDesc.find(id) != installedAppsDesc.end())
      {      
        CAppDescriptor& desc = installedAppsDesc[id];
        
        if (!desc.IsAllowed())
        {
          continue;
        }
        
        share->SetProperty("description", desc.GetDescription());
        share->SetProperty("app-author", desc.GetAuthor());
        share->SetProperty("app-version", desc.GetVersion());
        
        if (desc.GetType() == "plugin")
        {
          CStdString path = desc.GetLocalPath();
          CUtil::AddFileToFolder(path, "resources", path);
          CUtil::AddFileToFolder(path, "settings.xml", path);
          
          if (XFILE::CFile::Exists(path))
          {
            share->SetProperty("app-hassettings", true);
          }
        }
        
        if (!desc.IsBoxeeApp() && availableAppsDesc.find(desc.GetId()) != availableAppsDesc.end())
        {
          CAppDescriptor& newApp = availableAppsDesc[desc.GetId()];
          if (newApp.GetVersion() != "" && desc.GetVersion() != "" && CUtil::VersionCompare(desc.GetVersion(), newApp.GetVersion()) < 0)
          {
            share->SetProperty("app-hasupgrade", true);
          }
        }
      }
    }
    
    if (CUtil::IsPlugin(source.path))
    {
      CStdString path = source.path;
      path.Replace("plugin://", "special://xbmc/plugins/");
      CUtil::AddFileToFolder(path, "resources", path);
      CUtil::AddFileToFolder(path, "settings.xml", path);
      if (XFILE::CFile::Exists(path))
      {
        share->SetProperty("app-hassettings", true);
      }      
    }
    
    if (CUtil::IsRSS(source.path))
    {
      m_listExistingFeeds.Add(share);      
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_FEEDS, 0, 0, share);
      OnMessage(msg);
    }
    else
    {
      m_listExistingApps.Add(share);      
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_REMOVE_APPS, 0, 0, share);
      OnMessage(msg);
    }
  }
  
  m_listExistingApps.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  m_listExistingFeeds.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
}
  
void CGUIWindowBoxeeApplicationSettings::CreateListRepositories()
{
  // Clear the list of sources
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_REPOSITORIES);
  OnMessage(msgReset);
  m_listRepositories.Clear();

  CAppManager::GetInstance().GetRepositories().Load();
  const std::vector<CAppRepository> repositories = CAppManager::GetInstance().GetRepositories().Get();
  for (size_t i = 0; i < repositories.size(); i++)
  {    
    const CAppRepository& repository = repositories[i];
    
    // Create new share FileItem
    CFileItemPtr share ( new CFileItem(repository.GetName()) );
    share->m_strPath = repository.GetName();
    share->SetThumbnailImage(repository.GetThumbnail());
    share->SetProperty("description", repository.GetDescription());
    share->SetProperty("id", repository.GetID());    
    m_listRepositories.Add(share);    
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_REPOSITORIES, 0, 0, share);
    OnMessage(msg);
  }
}

void CGUIWindowBoxeeApplicationSettings::GetUnusedLastfm(CFileItemList& unusedLastfmList)
{
  // For now going to check only under the current music internet applications
  VECSOURCES* currentMusicShares = NULL;
  currentMusicShares = g_settings.GetSourcesFromType("music");

  bool isLastfmInUse = IsSourceInUse("lastfm://",currentMusicShares);
  
  if(isLastfmInUse == false)
  {
    CFileItemPtr lastfm(new CFileItem("lastfm://", false));
    lastfm->SetLabel("Last.fm");        
    lastfm->SetThumbnailImage("special://xbmc/skin/boxee/media/lastfm.png");
    lastfm->SetProperty("app-media", "music");         
    unusedLastfmList.Add(lastfm); 
  }
}

void CGUIWindowBoxeeApplicationSettings::GetUnusedShoutcast(CFileItemList& unusedLastfmList)
{
  // For now going to check only under the current music internet applications
  VECSOURCES* currentMusicShares = NULL;
  currentMusicShares = g_settings.GetSourcesFromType("music");

  bool isShoutInUse = IsSourceInUse("shout://www.shoutcast.com/sbin/newxml.phtml/",currentMusicShares);
  isShoutInUse |= IsSourceInUse("shout://classic.shoutcast.com/sbin/newxml.phtml/",currentMusicShares);
  isShoutInUse |= IsSourceInUse("shout://",currentMusicShares);
 
  if(isShoutInUse == false)
  {
    CFileItemPtr shout(new CFileItem("shout://", false));
    shout->SetLabel("SHOUTcast Radio");       
    shout->SetThumbnailImage("special://xbmc/skin/boxee/media/winamp_logo.png");
    shout->SetProperty("app-media", "music");         
    unusedLastfmList.Add(shout); 
  }
}

void CGUIWindowBoxeeApplicationSettings::GetUnusedApps(CFileItemList& unusedAppsList)
{
  VECSOURCES* pSharesVideo = g_settings.GetSourcesFromType("video");
  VECSOURCES* pSharesMusic = g_settings.GetSourcesFromType("music");
  VECSOURCES* pSharesPicture = g_settings.GetSourcesFromType("pictures");

  for (int i = 0; i< m_availableAppsFileItems.Size(); i++)
  {
    CFileItemPtr app = m_availableAppsFileItems[i];    
    bool alreadyInUse = IsSourceInUse(app->m_strPath, pSharesVideo) ||
                        IsSourceInUse(app->m_strPath, pSharesMusic) ||
                        IsSourceInUse(app->m_strPath, pSharesPicture);
     
    if (!alreadyInUse && app->IsAllowed())
    {
      app->m_bIsFolder = false;
      unusedAppsList.Add(app);
    }
  }
}


void CGUIWindowBoxeeApplicationSettings::GetUnusedPlugins(CFileItemList& unusedPluginsList)
{
  //CLog::Log(LOGDEBUG,"In CGUIWindowBoxeeMediaSourceAddShare::GetUnusedPluginsByType - Enter function with [mediaType=%s] (aup)",mediaType.c_str());

  VECSOURCES* pSharesVideo = g_settings.GetSourcesFromType("video");
  VECSOURCES* pSharesMusic = g_settings.GetSourcesFromType("music");
  VECSOURCES* pSharesPicture = g_settings.GetSourcesFromType("pictures");
 
  CFileItemList directoryPluginListVideo;
  CFileItemList directoryPluginListMusic;
  CFileItemList directoryPluginListPicture;
   
  DIRECTORY::CDirectory::GetDirectory("special://xbmc/plugins/video", directoryPluginListVideo);
  for (int i = 0; i < directoryPluginListVideo.Size(); i++)
  {
    directoryPluginListVideo[i]->SetProperty("app-media", "video");  
  }
  DIRECTORY::CDirectory::GetDirectory("special://xbmc/plugins/music", directoryPluginListMusic);
  for (int i = 0; i < directoryPluginListMusic.Size(); i++)
  {
    directoryPluginListMusic[i]->SetProperty("app-media", "music");  
  }
  DIRECTORY::CDirectory::GetDirectory("special://xbmc/plugins/pictures", directoryPluginListPicture);
  for (int i = 0; i < directoryPluginListPicture.Size(); i++)
  {
    directoryPluginListPicture[i]->SetProperty("app-media", "pictures");  
  }

  CFileItemList directoryPluginListAll;
  directoryPluginListAll.Append(directoryPluginListVideo);
  directoryPluginListAll.Append(directoryPluginListMusic);
  directoryPluginListAll.Append(directoryPluginListPicture);
 
  for (int i=0; i<directoryPluginListAll.Size(); i++)
  {
    CFileItemPtr plugin = directoryPluginListAll[i];
    CStdString path = directoryPluginListAll[i]->m_strPath;

    // Make sure that this is actually a plugin
    CStdString default_py = path;
    CUtil::AddFileToFolder(default_py, "default.py", default_py);
    if (!XFILE::CFile::Exists(default_py))
    {
      continue;
    }
    
    // Change the path to plugin://
    CUtil::RemoveSlashAtEnd(directoryPluginListAll[i]->m_strPath);      
    CStdString url = "plugin://";
    url += directoryPluginListAll[i]->GetProperty("app-media");
    url += "/";
    url += CUtil::GetFileName(directoryPluginListAll[i]->m_strPath);
    url += "/";
    directoryPluginListAll[i]->m_strPath = url;    
    
    if (IsSourceInUse(((directoryPluginListAll[i])->m_strPath), pSharesVideo) ||
        IsSourceInUse(((directoryPluginListAll[i])->m_strPath), pSharesMusic) ||
        IsSourceInUse(((directoryPluginListAll[i])->m_strPath), pSharesPicture))
    {
      continue;
    }
    
    CStdString thumbnail = path;
    CUtil::AddFileToFolder(thumbnail, "default.tbn", thumbnail);
    directoryPluginListAll[i]->SetThumbnailImage(thumbnail);  
    directoryPluginListAll[i]->m_bIsFolder = false;
    unusedPluginsList.Add(directoryPluginListAll[i]);
  }
 
}

bool CGUIWindowBoxeeApplicationSettings::IsSourceInUse(CStdString sourcePath,VECSOURCES* pSharesList)
{
  // Check if plug-in is already in use.
  for (size_t k = 0; k < pSharesList->size(); k++)
  {
    CStdString sharePath1 = (*pSharesList)[k].strPath;
    CStdString sharePath2 = (*pSharesList)[k].strPath;
    
    CUtil::RemoveSlashAtEnd(sharePath1);
    CUtil::AddSlashAtEnd(sharePath2);

    if(sourcePath.Equals(sharePath1) || sourcePath.Equals(sharePath2))
    {
      return true;
    }
  }

  return false;
}
