#include "stdafx.h"
#include "GUIWindowBoxeeWizardAddSource.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "FileSystem/FactoryDirectory.h"
#include "FileSystem/FileSmb.h"
#include "GUILabelControl.h"
#include "Util.h"
#ifdef _LINUX
#include "LinuxFileSystem.h"
#endif
#include "GUIDialogOK.h"
#include "GUILabelControl.h"
#include "GUIWindowBoxeeWizardAddSourceManual.h"
#include "GUIWindowBoxeeWizardSourceName.h"
#include "GUIListContainer.h"
#include "GUIPassword.h"
#include "FileSystem/PluginDirectory.h"
#include "GUIDialogProgress.h"

using namespace std;
using namespace DIRECTORY;

#define CONTROL_CATEGORY_LABEL        10
#define CONTROL_NETWORK_LOCAL_LIST    50
#define CONTROL_SEP1                  54
#define CONTROL_DEVICES_LIST          55
#define CONTROL_SEP2                  61
#define CONTROL_PATH                  62
#define CONTROL_FOLDERS_LIST          65
#define CONTROL_FOLDERS_LIST_SCROLL   70 
#define CONTROL_BACK                  98
#define CONTROL_NEXT                  99
   
#define ITEM_NETWORK     0
#define ITEM_LOCAL       1
#define ITEM_INTERNET    2

CGUIWindowBoxeeWizardAddSource::CGUIWindowBoxeeWizardAddSource(void)
    : CGUIDialog(WINDOW_BOXEE_WIZARD_ADD_SOURCE, "boxee_wizard_add_source.xml")
{
}

CGUIWindowBoxeeWizardAddSource::~CGUIWindowBoxeeWizardAddSource(void)
{}

void CGUIWindowBoxeeWizardAddSource::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  
  CGUILabelControl* categoryLabel = (CGUILabelControl*) GetControl(CONTROL_CATEGORY_LABEL);
  if (categoryLabel)
  {
    if (m_category == "video")
      categoryLabel->SetLabel(g_localizeStrings.Get(51022));
    else if (m_category == "music")
      categoryLabel->SetLabel(g_localizeStrings.Get(51023));
    else if (m_category == "pictures")
      categoryLabel->SetLabel(g_localizeStrings.Get(51024));
  }

  SET_CONTROL_HIDDEN(CONTROL_SEP1);
  SET_CONTROL_HIDDEN(CONTROL_DEVICES_LIST);
  SET_CONTROL_HIDDEN(CONTROL_SEP2);
  SET_CONTROL_HIDDEN(CONTROL_FOLDERS_LIST);
  SET_CONTROL_HIDDEN(CONTROL_FOLDERS_LIST_SCROLL);
  CONTROL_DISABLE(CONTROL_NEXT);
  SET_CONTROL_FOCUS(CONTROL_NETWORK_LOCAL_LIST, 0);  
}  

bool CGUIWindowBoxeeWizardAddSource::OnAction(const CAction &action)
{
   int iControl = GetFocusedControlID();

   bool bSelectAction = ((action.wID == ACTION_SELECT_ITEM) || (action.wID == ACTION_MOUSE_LEFT_CLICK));

   if (bSelectAction && iControl == CONTROL_NETWORK_LOCAL_LIST)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_NETWORK_LOCAL_LIST);
      OnMessage(msg);
      int iItem = msg.GetParam1();
      
      if (iItem == ITEM_NETWORK)
         ShowNetworkDevices();
      else if (iItem == ITEM_LOCAL)
         ShowLocalDevices();
      else
         ShowInternetApps();
            
      CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_NETWORK_LOCAL_LIST);
      if (pList)
        pList->SetSingleSelectedItem();

      return true;
   }
   else if (bSelectAction && iControl == CONTROL_DEVICES_LIST)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_DEVICES_LIST);
      OnMessage(msg);
      int iDevice = msg.GetParam1();
      
      CGUIMessage msg2(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_NETWORK_LOCAL_LIST);
      OnMessage(msg2);
      int iItem = msg2.GetParam1();
      
      if (iItem == ITEM_NETWORK && iDevice == m_devices.Size()-1)
      {
         CGUIWindowBoxeeWizardAddSourceManual* pDlgAddSourceManual = 
            (CGUIWindowBoxeeWizardAddSourceManual*)m_gWindowManager.GetWindow(WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL);
         pDlgAddSourceManual->DoModal();
         if (!pDlgAddSourceManual->IsConfirmed())
            return true;
            
         m_devices[iDevice]->m_strPath = pDlgAddSourceManual->GetURL();
      }
      
      InitBrowseDirectory();
      BrowseDirectory();
       
      CONTROL_ENABLE(CONTROL_NEXT);
              
      CGUIListContainer *pList = (CGUIListContainer *)GetControl(CONTROL_DEVICES_LIST);
      if (pList)
        pList->SetSingleSelectedItem();

      return true;
   }
   else if (action.wID == ACTION_MOVE_LEFT && iControl == CONTROL_DEVICES_LIST)
   {   
      SET_CONTROL_HIDDEN(CONTROL_DEVICES_LIST);
      SET_CONTROL_HIDDEN(CONTROL_SEP1);
      CONTROL_DISABLE(CONTROL_NEXT);
   }      
   else if (bSelectAction && iControl == CONTROL_FOLDERS_LIST)
   {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FOLDERS_LIST);
      OnMessage(msg);
      int iFolder = msg.GetParam1();
      m_upFolder = m_currentFolder;
      m_currentFolder = m_folders[iFolder]->m_strPath;
      
      BrowseDirectory();
      return true;
   }
   else if (action.wID == ACTION_MOVE_LEFT && iControl == CONTROL_FOLDERS_LIST)
   {
      SET_CONTROL_HIDDEN(CONTROL_FOLDERS_LIST);
      SET_CONTROL_HIDDEN(CONTROL_PATH);
      SET_CONTROL_HIDDEN(CONTROL_SEP2);
      CONTROL_DISABLE(CONTROL_NEXT);      
   }      
   else if (action.wID == ACTION_MOVE_RIGHT && iControl == CONTROL_FOLDERS_LIST)
   {
      SET_CONTROL_FOCUS(CONTROL_NEXT, 0);      
   }
   else if (action.wID == ACTION_MOVE_UP && (iControl == CONTROL_NEXT || iControl == CONTROL_BACK))
   {
      if (GetControl(CONTROL_FOLDERS_LIST)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_FOLDERS_LIST, 0);
      }      
      else if (GetControl(CONTROL_DEVICES_LIST)->IsVisible())
      {
         SET_CONTROL_FOCUS(CONTROL_DEVICES_LIST, 0);
      }      
      else
      {
         SET_CONTROL_FOCUS(CONTROL_NETWORK_LOCAL_LIST, 0);
      }               
   }
   else if (bSelectAction && iControl == CONTROL_NEXT)
   {
      CGUIWindowBoxeeWizardSourceName *sourceNameDlg = (CGUIWindowBoxeeWizardSourceName *)m_gWindowManager.GetWindow(WINDOW_BOXEE_WIZARD_SOURCE_NAME);
      sourceNameDlg->SetSourcePath(m_currentFolder);
      sourceNameDlg->DoModal();
      if (sourceNameDlg->IsConfirmed())
      {
         CStdString strPath = m_currentFolder;

         // Get the password used to browse this share and store it as well
         CURI url(m_currentFolder);

         if (url.GetProtocol() == "smb") 
         {
           CStdString strPassword;
           CStdString strUserName ;
           CStdString strShare;

           //go over cifs map
           strShare = url.GetHostName();
           if(url.GetFileName() != "")
           {
             strShare  += "/" +url.GetFileName();
           }
           IMAPUSERNAMEPASSWORDS it = g_passwordManager.m_mapCIFSPasswordCache.find(strShare);

           if (it != g_passwordManager.m_mapCIFSPasswordCache.end())
           {
             strPassword = g_passwordManager.m_mapCIFSPasswordCache[strShare].second;
             strUserName =  g_passwordManager.m_mapCIFSPasswordCache[strShare].first;
             url.SetPassword(strPassword);
             url.SetUserName(strUserName);

             strPath = url.Get();
           }
           else
           {
             /* must url encode this as, auth code will look for the encoded value */
             strShare  = smb.URLEncode(url.GetHostName());
             strShare += "/";
             strShare += smb.URLEncode(url.GetShareName());

             IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);
             if (it != g_passwordManager.m_mapSMBPasswordCache.end())
             {
               // if share found in cache use it to supply username and password
               CURL url2(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
               strPassword = url2.GetPassWord();
               strUserName = url2.GetUserName();
               url.SetPassword(strPassword);
               url.SetUserName(strUserName);
             }

             // first we encode the url to get the full path including the user/pass.
             // then we decode it again to get it in human readable format, which is what we store in sources.xml
             strPath = smb.URLDecode(smb.URLEncode(url));
           }
         }

         CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_DEVICES_LIST);
         OnMessage(msg);
         int iDevice = msg.GetParam1();

         CMediaSource share;
         vector<CStdString> paths;
         paths.push_back(strPath);
         share.FromNameAndPaths(m_category, sourceNameDlg->GetSourceName(), paths);

         if (iDevice >= 0 && iDevice < m_devices.Size())
           share.m_strThumbnailImage = m_devices[iDevice]->GetThumbnailImage();

         g_settings.AddShare(m_category, share);
         g_settings.SaveSources();
         Close();
      }
      
      return true;
   }   
   else if (action.wID == ACTION_PREVIOUS_MENU || (bSelectAction && iControl == CONTROL_BACK))
   {
      Close();
      return true;
   }
   
   return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeWizardAddSource::ShowNetworkDevices()
{
   SET_CONTROL_VISIBLE(CONTROL_DEVICES_LIST);
   SET_CONTROL_VISIBLE(CONTROL_SEP1);

   CGUIDialogProgress* pDlgProgress = (CGUIDialogProgress*)m_gWindowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
   pDlgProgress->SetHeading("");
   pDlgProgress->SetLine(0, "Searching for network devices...");
   pDlgProgress->SetLine(1, "");
   pDlgProgress->SetLine(2, "");
   pDlgProgress->StartModal();
   pDlgProgress->Progress();
      
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DEVICES_LIST);
   OnMessage(msgReset);
   
   m_devices.Clear();
   
   CFileItemList workgroups;
   
   IDirectory* dir = CFactoryDirectory::Create("smb://");
   dir->SetAllowPrompting(true);
   dir->GetDirectory("smb://", workgroups);
   
   bool prependWorkgroup = true;
   if (workgroups.Size() == 1)
      prependWorkgroup = false;
   
   for (int w = 0; w < workgroups.Size(); w++)
   {
      CFileItemList computers;
      dir->GetDirectory(workgroups[w]->m_strPath, computers);
      
      for (int c = 0; c < computers.Size(); c++)
      {
         CFileItemPtr computer (new CFileItem(*computers[c]));
         m_devices.Add(computer);
         CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DEVICES_LIST, 0, 0, computer);
         OnMessage(msg);
      }
   }
   
   CFileItemPtr manual ( new CFileItem("Manually Enter IP / Name...") );
   m_devices.Add(manual);
   CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DEVICES_LIST, 0, 0, manual);
   OnMessage(msg);
      
   delete dir;
   
   pDlgProgress->Close();
      
   SET_CONTROL_FOCUS(CONTROL_DEVICES_LIST, 0);
}

void CGUIWindowBoxeeWizardAddSource::InitBrowseDirectory()
{
   SET_CONTROL_VISIBLE(CONTROL_FOLDERS_LIST);
   SET_CONTROL_VISIBLE(CONTROL_PATH);
   SET_CONTROL_VISIBLE(CONTROL_SEP2);

   CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_DEVICES_LIST);
   OnMessage(msg);
   int iDevice = msg.GetParam1();

   CFileItemPtr device = m_devices[iDevice];
   
   m_rootFolder = device->m_strPath;
   CUtil::AddSlashAtEnd(m_rootFolder);
   m_currentFolder = m_rootFolder;
   
   SET_CONTROL_FOCUS(CONTROL_FOLDERS_LIST, 0);      
}

void CGUIWindowBoxeeWizardAddSource::BrowseDirectory()
{      
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_FOLDERS_LIST);
   OnMessage(msgReset);   
   m_folders.Clear();

   // Get all the folders
   IDirectory* dir = CFactoryDirectory::Create(m_currentFolder);
   CFileItemList folders;
   dir->SetAllowPrompting(true);   
   dir->GetDirectory(m_currentFolder, folders);
  
   // Make sure there's at least one item to view, otherwise show an error message 
   if (m_currentFolder == m_rootFolder && folders.Size() == 0)
   {
      CGUIDialogOK::ShowAndGetInput(0, 50006, 0, 0);
      SET_CONTROL_HIDDEN(CONTROL_FOLDERS_LIST);
      SET_CONTROL_HIDDEN(CONTROL_PATH);
      SET_CONTROL_HIDDEN(CONTROL_SEP2);
      CONTROL_DISABLE(CONTROL_NEXT);
      SET_CONTROL_FOCUS(CONTROL_DEVICES_LIST, 0);      
      return;
   }
   
   // Add the previous folder
   if (m_currentFolder != m_rootFolder)
   {
      CFileItemPtr folder ( new CFileItem(".."));      
      folder->SetIconImage("wizard_up_folder_icon.png");
      CUtil::GetParentPath(m_currentFolder, folder->m_strPath);
      m_folders.Add(folder);
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FOLDERS_LIST, 0, 0, folder);
      OnMessage(msg);
   }
   
   // Sort folders by name
   folders.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
   
   for (int i = 0; i < folders.Size(); i++)
   {
      CFileItemPtr folder ( new CFileItem(*folders[i]) );      
      if (folder->m_bIsFolder)
      {
         folder->SetIconImage("wizard_folder_icon.png");
         m_folders.Add(folder);
         CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FOLDERS_LIST, 0, 0, folder);
         OnMessage(msg);
      }
   }
   
   delete dir;
   
   
   
   // Set the path label
   CURI url(m_currentFolder);
   CStdString path = "//" + url.GetHostName() + "/" + url.GetFileName();
   CGUILabelControl* pathLabel = (CGUILabelControl*) GetControl(CONTROL_PATH);
   pathLabel->SetLabel(path);
}   

void CGUIWindowBoxeeWizardAddSource::ShowLocalDevices()
{
#ifdef _LINUX
  VECSOURCES removableDrives;
  CLinuxFileSystem::GetRemovableDrives(removableDrives);
  
  if (removableDrives.size() == 0)
  {
    CGUIDialogOK::ShowAndGetInput(0, 50007, 0, 0);
    return;
  }
  
  // Clear the list first
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DEVICES_LIST);
  OnMessage(msgReset);
  m_devices.Clear();
  
  for (size_t i = 0; i < removableDrives.size(); i++)
  {
    CMediaSource removableDrive = removableDrives[i]; 
    CFileItemPtr share ( new CFileItem(removableDrive.strName)  );      
    share->m_strPath = removableDrive.strPath;
    m_devices.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DEVICES_LIST, 0, 0, share);
    OnMessage(msg);
  }
  
  
  CFileItemPtr share ( new CFileItem("Local") );      
  share->m_strPath = "/";
  m_devices.Add(share);
  CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DEVICES_LIST, 0, 0, share);
  OnMessage(msg);
#endif
  
  SET_CONTROL_VISIBLE(CONTROL_DEVICES_LIST);
  SET_CONTROL_VISIBLE(CONTROL_SEP1);
  SET_CONTROL_FOCUS(CONTROL_DEVICES_LIST, 0);
}

void CGUIWindowBoxeeWizardAddSource::AddPlugins(const CStdString &strType, vector<ShareData> &sharesVec)
{
   CFileItemList listPlugins;
   DIRECTORY::CPluginDirectory dir;
   if (dir.GetPluginsDirectory(strType, listPlugins))
   {
     for (int i=0; i<listPlugins.Size(); i++)
     {
       CFileItemPtr pItem = listPlugins[i];
       sharesVec.push_back(ShareData(pItem->GetLabel(), pItem->m_strPath, pItem->GetThumbnailImage()));
     }
   }
}

void CGUIWindowBoxeeWizardAddSource::ShowInternetApps()
{
   vector<ShareData> shares;
   if (m_category == "video")
   {
     shares.push_back(ShareData("Video Channels", "rss://rss.boxee.tv/video.xml", "Q:/media/defaultrss.png"));
     shares.push_back(ShareData("Movie Trailers", "rss://rss.boxee.tv/movie_trailers/index.xml","Q:/plugins/video/Apple Movie Trailers/default.tbn"));
     AddPlugins("video",shares);
   }
   else if (m_category == "music")
   {
     shares.push_back(ShareData("Last.fm", "lastfm://", "special://xbmc/skin/boxee/media/lastfm.png"));
     shares.push_back(ShareData("SHOUTcast Radio", "shout://","special://xbmc/skin/boxee/media/winamp_logo.png"));
     shares.push_back(ShareData("Audio Channels", "rss://rss.boxee.tv/audio.xml", "Q:/media/defaultrss.png"));
     AddPlugins("music",shares);
   }
   else if (m_category == "pictures")
   {
     AddPlugins("pictures",shares);
   }
   
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_DEVICES_LIST);
   OnMessage(msgReset);
   m_devices.Clear();
   
   for (size_t i = 0; i < shares.size(); i++)
   {
      CFileItemPtr share ( new CFileItem(shares[i].strLabel) );      
      share->m_strPath = shares[i].strPath;
      share->SetThumbnailImage(shares[i].strThumb);
      m_devices.Add(share);
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_DEVICES_LIST, 0, 0, share);
      OnMessage(msg);
   }
   
   SET_CONTROL_VISIBLE(CONTROL_DEVICES_LIST);
   SET_CONTROL_VISIBLE(CONTROL_SEP1);
   SET_CONTROL_FOCUS(CONTROL_DEVICES_LIST, 0);
}

void CGUIWindowBoxeeWizardAddSource::SetCategory(CStdString category)
{
   m_category = category;
}
