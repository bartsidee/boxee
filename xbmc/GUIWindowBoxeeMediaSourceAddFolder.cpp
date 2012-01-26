
#include <vector>
#include "GUIWindowBoxeeMediaSources.h"
#include "GUIWindowBoxeeMediaSourceAddShare.h"
#include "GUIWindowBoxeeMediaSourceAddFolder.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "Application.h"
#include "GUILabelControl.h"
#include "URL.h"
#include "GUIWindowBoxeeWizardAddSourceManual.h"
#include "GUIWindowManager.h"
#include "FileSystem/FactoryDirectory.h"
#include "FileSystem/FileSmb.h"
#include "RegExp.h"
#include "UPnPDirectory.h" // for making upnp friendly names
#include "GUIUserMessages.h"
#include "Directory.h"
#include "BoxeeUtils.h"
#include "LocalizeStrings.h"
#include "AfpDirectory.h"

#include "GUIDialogOK.h"
#include "Util.h"
#ifdef HAS_EMBEDDED
#include "HalServices.h"
#endif

using namespace DIRECTORY;

#define CONTROL_TITLE_LABEL     40
#define CONTROL_PATH_LABEL      51
#define CONTROL_FOLDERS_LIST    42
#define CONTROL_ADD_BUTTON      53
#define CONTROL_COUNT_LABEL     54
#define CONTROL_BUTTON_EJECT    55

CGUIWindowBoxeeMediaSourceAddFolder::CGUIWindowBoxeeMediaSourceAddFolder(void)
    : CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCE_ADD_FOLDER, "boxee_media_source_add_folder.xml")
{
  m_currentFolderThumbPath = "";
}

CGUIWindowBoxeeMediaSourceAddFolder::~CGUIWindowBoxeeMediaSourceAddFolder(void)
{
  
}

bool CGUIWindowBoxeeMediaSourceAddFolder::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnAction - Hit on ACTION_PREVIOUS_MENU (msmk)");

    if(!OnBack())
    {
      m_previousFoldervec.clear();
      Close();
    }
    return true;
  }
  
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeMediaSourceAddFolder::OnBack()
{
  if(m_previousFoldervec.size() == 1)
  {
    return false;
  }
  m_currentFolder = m_previousFoldervec[m_previousFoldervec.size()-1];
  m_previousFoldervec.pop_back();

  m_currentPosition = m_LastPosition[m_LastPosition.size()-1];
  m_LastPosition.pop_back();

  BrowseDirectory();
  return true;
}

bool CGUIWindowBoxeeMediaSourceAddFolder::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {    
    DWORD senderId = message.GetSenderId();
    
    if(senderId == CONTROL_FOLDERS_LIST)
    {
      if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
      {
        // Handle only GUI_MSG_CLICKED on CONTROL_FOLDERS_LIST that origin from navigation

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_FOLDERS_LIST]. Going to call onback() (msmk)");

        ProccessItemSelectedInControlFoldersList();
      
        return true;
      }
    }
    else if (senderId == CONTROL_ADD_BUTTON)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_BUTTON]. Going to call ProccessHitOnAddButton() (msmk)");

      ProccessHitOnAddButton();
      
      return true;
    }
    else if (senderId == CONTROL_BUTTON_EJECT)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_ADD_BUTTON]. Going to call ProccessHitOnAddButton() (msmk)");
#ifdef HAS_AFP
      if(m_currentFolder.Find("afp://") != -1)
      {

        CAfpDirectory::EjectUser(m_currentFolder);
        Close();
      }
#endif
      return true;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowBoxeeMediaSourceAddFolder::UpdateEjectButtonState()
{
#ifndef HAS_EMBEDDED
  SET_CONTROL_HIDDEN(CONTROL_BUTTON_EJECT);
#else

  if(!m_currentFolder.Equals("afp://") && !m_currentFolder.Equals("afp://all") && m_currentFolder.Find("afp://") != -1)
  {
    SET_CONTROL_VISIBLE(CONTROL_BUTTON_EJECT);
    return;
  }

  SET_CONTROL_HIDDEN(CONTROL_BUTTON_EJECT);
#endif
}

void CGUIWindowBoxeeMediaSourceAddFolder::OnInitWindow()
{ 
  SET_CONTROL_HIDDEN(CONTROL_BUTTON_EJECT);
  m_currentFolderThumbPath = "";
  m_currentPosition = 0;

  CGUIWindow::OnInitWindow();

  bool retVal = InitBrowseDirectory();

  if(retVal == true)
  {
    BrowseDirectory();
  }
    
  CGUIWindowBoxeeMediaSources *pDlgSources = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (!pDlgSources)
    return;
  int m_selectedSource = pDlgSources->getSelectedSource();
    
  if (m_selectedSource == SOURCE_LOCAL)  
  {
    SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Local / Add New Source");
  }
  else if (m_selectedSource == SOURCE_NETWORK)  
  {
    SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Network / Add New Source");
  }
  else if (m_selectedSource == SOURCE_NETWORK_APPLICATIONS)  
  {
    SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Network Application / Add New Source");
  }  
  else if (m_selectedSource == MANUALLY_ADD_SOURCE)  
  {
    SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, "Settings / Media Sources / Manually Add Source / Add New Source");
  }
}

bool CGUIWindowBoxeeMediaSourceAddFolder::InitBrowseDirectory()
{
  CGUIWindowBoxeeMediaSourceAddShare *pDlgAddShare= (CGUIWindowBoxeeMediaSourceAddShare*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_ADD_SHARE);
  if (!pDlgAddShare)
  {
    return false;
  }

  CFileItemPtr device = pDlgAddShare->GetSelectedFileItem();

  if (!device)
  {
    return false;
  }
  else
  {
    m_rootFolder = device->m_strPath;
    CUtil::AddSlashAtEnd(m_rootFolder);
    m_currentFolder = m_rootFolder;
    m_previousFoldervec.clear();
    m_previousFoldervec.push_back(m_currentFolder);
    m_currentFolderThumbPath = device->GetThumbnailImage();

    if((device->IsPlugin()) || (device->IsRSS()) || (device->IsApp()))
    {
      CFileItemPtr folder (device);
      m_folders.Add(folder);
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FOLDERS_LIST, 0, 0, folder);
      OnMessage(msg);

      return false;
    }
    else
    {
      return true;
    }
  }
}

void CGUIWindowBoxeeMediaSourceAddFolder::BrowseDirectoryJob::Run()
{
  DIRECTORY::CDirectory dir;
  CStdString newPath;

  m_folders->SetProperty("dontUseGuestCredentials",true);
  m_bJobResult = dir.GetDirectory(m_folderPath, *m_folders);
  if(m_folders->GetPropertyBOOL("getcredentials"))
  {
    g_passwordManager.SetSMBShare(m_folders->m_strPath);
    if (g_passwordManager.GetSMBShareUserPassword())
    {
      newPath = g_passwordManager.GetSMBShare();
    }
#ifndef HAS_CIFS
    CURI urlnew( newPath );
    CStdString u = urlnew.GetUserName(), p = urlnew.GetPassWord();
    CUtil::UrlDecode(u);
    CUtil::UrlDecode(p);
    urlnew.SetUserName(u);
    urlnew.SetPassword(p);
    newPath = urlnew.Get();
#endif

    m_bJobResult = dir.GetDirectory(newPath, *m_folders);
  }

  // in case that the job was canceled it is his responsibility to
  // release the m_folders parameters
  if ((m_IsCanceled) && (m_folders != NULL))
    delete m_folders;

}

void CGUIWindowBoxeeMediaSourceAddFolder::BrowseDirectory()
{
  CONTROL_ENABLE(CONTROL_FOLDERS_LIST);
  UpdateEjectButtonState();

  // if BrowseDirectoryJob was canceled it is his responsibility to
  // delete the *folders, otherwise it is our responsibility
  CFileItemList *folders = new CFileItemList;
  bool bResult = (CUtil::RunInBG(new BrowseDirectoryJob(m_currentFolder,folders)) == JOB_SUCCEEDED);

  if (!bResult)
  {
    Close(true);
    return;
  }

 // UpdateEjectButtonState();
  SET_CONTROL_HIDDEN(CONTROL_ADD_BUTTON);
  // Clear the list first
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_FOLDERS_LIST);
  OnMessage(msgReset);
  m_folders.Clear();

  SET_CONTROL_VISIBLE(CONTROL_ADD_BUTTON);

  // Make sure there's at least one item to view, otherwise show an error message
  // UPnP folder might load the items on the fly when the window is already opened
  if (m_currentFolder == m_rootFolder && folders->Size() == 0
      && m_currentFolder != "upnp://all/")
  {
    CGUIDialogOK::ShowAndGetInput(257, 50006, 0, 0);
    Close(true);

    if (folders != NULL)
      delete folders;
    return;

    //CONTROL_DISABLE(CONTROL_ADD_BUTTON);
  }

  // Sort folders by name
  folders->Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);

  for (int i = 0; i < folders->Size(); i++)
  {
    CFileItemPtr folder(new CFileItem(*(folders->Get(i))));
    if (folder->m_bIsFolder)
    {
      folder->SetIconImage("wizard_folder_icon.png");
      folder->SetProperty("foldericon","network_folder");
      m_folders.Add(folder);
    }
  }

  SET_CONTROL_LABEL(CONTROL_COUNT_LABEL , GetItemDescription().ToLower());
  if(m_folders.Size() > 0)
  {
    CGUIMessage msgBind(GUI_MSG_LABEL_BIND, GetID(), CONTROL_FOLDERS_LIST, 0, 0, &m_folders);
    OnMessage(msgBind);
    CGUIMessage msgSelect(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_FOLDERS_LIST,m_currentPosition);
    OnMessage(msgSelect);
  }
  else
  {
    SET_CONTROL_FOCUS(CONTROL_ADD_BUTTON,0);
    CONTROL_DISABLE(CONTROL_FOLDERS_LIST);
  }

  // Set the path label
  // Do this consistently with GUIWindowBoxeeBrowseLocal and BrowseWindowState...
  // Should probably call MakeShortenPath to deal with egregiously long path names
  CStdString strData = m_currentFolder;
  if (CUtil::IsUPnP(strData))
    DIRECTORY::CUPnPDirectory::GetFriendlyPath(strData);
  else
  {
    if (CUtil::IsHD(strData))
      CUtil::HideExternalHDPath(strData, strData);

    CUtil::RemovePasswordFromPath(strData);
  }

  if (folders != NULL)
    delete folders;
}   

CStdString CGUIWindowBoxeeMediaSourceAddFolder::GetItemDescription()
{
  CStdString label = m_currentFolder;
  if (!label.IsEmpty() && !CUtil::IsUPnP(label))
  {
    CUtil::RemovePasswordFromPath(label);
  }
  if (!label.IsEmpty() && CUtil::IsUPnP(label))
  {
    CUtil::UrlDecode(label);
  }
  if (label.substr(0,3) == "bms")
  {
    label[2] = 'm';
  }
  CStdString shortPath = label;
  CUtil::MakeShortenPath(label,shortPath,60);
  label = shortPath;

  return label;
}

CStdString CGUIWindowBoxeeMediaSourceAddFolder::GetSelectedFolderThumbPath()
{
  return m_currentFolderThumbPath;
}

CStdString CGUIWindowBoxeeMediaSourceAddFolder::GetSelectedEncodedPath(const CStdString& _strPath)
{
  CStdString strPath =_strPath;
  
  // Get the password used to browse this share and store it as well
  CURI url(strPath);
  
  if (url.GetProtocol() == "smb" || url.GetProtocol() == "afp")
  {
    CStdString strPassword;
    CStdString strUserName;

    if(url.GetProtocol() == "smb")
    {
      CStdString strPath = url.GetHostName();
      if(url.GetFileName() != "")
      {
        strPath  += "/" +url.GetFileName();
      }

      bool bFound = false;
      CStdString strKey = strPath;
      while(!bFound)
      {
        IMAPUSERNAMEPASSWORDS it = g_passwordManager.m_mapCIFSPasswordCache.find(strKey);

        if (it != g_passwordManager.m_mapCIFSPasswordCache.end())
        {
          strPassword = g_passwordManager.m_mapCIFSPasswordCache[strKey].second;
          strUserName =  g_passwordManager.m_mapCIFSPasswordCache[strKey].first;
          url.SetPassword(strPassword);
          url.SetUserName(strUserName);
          bFound = true;
        }
        else
        {
          CStdString strParent;

          CUtil::RemoveSlashAtEnd(strKey);
          CUtil::GetDirectory(strKey, strParent);

          if(strParent == strKey)
          {
            break;
          }

          strKey = strParent;
        }
      }

    }
    else if(url.GetProtocol() == "afp")
    {
      IMAPUSERNAMEPASSWORDS it = g_passwordManager.m_mapAFPPasswordCache.find(url.GetHostName());
      if (it != g_passwordManager.m_mapAFPPasswordCache.end())
      {
        // if share found in cache use it to supply username and password
        CURI afpUrl(strPath); // map value contains the full url of the originally authenticated share. map key is just the share
        strPassword = g_passwordManager.m_mapAFPPasswordCache[afpUrl.GetHostName()].second;
        strUserName =  g_passwordManager.m_mapAFPPasswordCache[afpUrl.GetHostName()].first;

        if(!strUserName.Equals("guest"))
        {
          url.SetPassword(strPassword);
          url.SetUserName(strUserName);
        }

      }
    }

    // first we encode the url to get the full path including the user/pass.
    // then we decode it again to get it in human readable format, which is what we store in sources.xml
    //strPath = smb.URLDecode(smb.URLEncode(url));
    strPath = url.Get();
  }
	
  // Since this is a folder we always add slash at the end
	CUtil::AddSlashAtEnd(strPath);	
   
  return strPath;
}         

void CGUIWindowBoxeeMediaSourceAddFolder::ProccessItemSelectedInControlFoldersList()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_FOLDERS_LIST);
  OnMessage(msg);
  int iFolder = msg.GetParam1();

  if (iFolder < m_folders.Size())
  {
    m_previousFoldervec.push_back(m_currentFolder);
    m_currentFolder = m_folders[iFolder]->m_strPath;
    if(m_folders[iFolder]->GetPropertyBOOL("parentFolder"))
    {
      m_currentPosition = m_LastPosition[m_LastPosition.size()-1]; 
      m_LastPosition.pop_back();
    }
    else
    {
      m_currentPosition = 0;
      m_LastPosition.push_back(iFolder);
    }

    BrowseDirectory();
  }  
}

void CGUIWindowBoxeeMediaSourceAddFolder::ProccessHitOnAddButton()
{
  CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  if (pDlgSourceInfo)
  {
    pDlgSourceInfo->SetAddSource(GetSelectedEncodedPath(m_currentFolder));
    pDlgSourceInfo->SetSourceThumbPath(GetSelectedFolderThumbPath());
  }
  
  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);  
}

