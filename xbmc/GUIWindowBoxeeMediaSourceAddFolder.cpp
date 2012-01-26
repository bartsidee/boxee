
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

#include "GUIDialogOK.h"
#include "Util.h"

#define CONTROL_TITLE_LABEL     40
#define CONTROL_PATH_LABEL      51
#define CONTROL_FOLDERS_LIST    52
#define CONTROL_ADD_BUTTON      53

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
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnAction - Hit on ACTION_PREVIOUS_MENU (msmk)");

    Close();
    
    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnAction - Hit on ACTION_PARENT_DIR (msmk)");

    Close();

    return true;
  }
  
  return CGUIWindow::OnAction(action);
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

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_FOLDERS_LIST]. Going to call ProccessItemSelectedInControlFoldersList() (msmk)");

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
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowBoxeeMediaSourceAddFolder::OnInitWindow()
{ 
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
  // Get all the folders
  DIRECTORY::IDirectory* dir = DIRECTORY::CFactoryDirectory::Create(m_folderPath);
  
  // We need to enable prompting in order to allow requests for passwords where
  // necessary
  dir->SetAllowPrompting(true);
  m_bJobResult = dir->GetDirectory(m_folderPath, *m_folders);

  // in case that the job was canceled it is his responsibility to
  // release the m_folders parameters
  if ((m_IsCanceled) && (m_folders != NULL))
	  delete m_folders;

  delete dir;
}

void CGUIWindowBoxeeMediaSourceAddFolder::BrowseDirectory()
{
  
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_FOLDERS_LIST);
   OnMessage(msgReset);   
   m_folders.Clear();
   
   SET_CONTROL_HIDDEN(CONTROL_ADD_BUTTON);
   
   // if BrowseDirectoryJob was canceled it is his responsibility to
   // delete the *folders, otherwise it is our responsibility
   CFileItemList *folders = new CFileItemList;
   bool bResult = CUtil::RunInBG(new BrowseDirectoryJob(m_currentFolder, folders));

   if (!bResult)
   {
     Close(true);
     return;
   }
   
   SET_CONTROL_VISIBLE(CONTROL_ADD_BUTTON);
  
   // Make sure there's at least one item to view, otherwise show an error message 
   if (m_currentFolder == m_rootFolder && folders->Size() == 0)
   {
      CGUIDialogOK::ShowAndGetInput(257, 50006, 0, 0);
	  Close(true);

	  if (folders != NULL)
		  delete folders;
	  return;
      //CONTROL_DISABLE(CONTROL_ADD_BUTTON);
   }
   
   // Add the previous folder
   if (m_currentFolder != m_rootFolder)
   {
      CFileItemPtr folder (new CFileItem(".."));      
      folder->SetProperty("parentFolder", true);
      folder->SetIconImage("wizard_up_folder_icon.png");
      CUtil::GetParentPath(m_currentFolder, folder->m_strPath);
      m_folders.Add(folder);
      CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FOLDERS_LIST, 0, 0, folder);
      OnMessage(msg);
   }
   
   // Sort folders by name
   folders->Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);
   
   for (int i = 0; i < folders->Size(); i++)
   {
      CFileItemPtr folder ( new CFileItem(*(folders->Get(i))));
      if (folder->m_bIsFolder)
      {
         folder->SetIconImage("wizard_folder_icon.png");
         m_folders.Add(folder);
         CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_FOLDERS_LIST, 0, 0, folder);
         OnMessage(msg);
      }
   }
   
   CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_FOLDERS_LIST, m_currentPosition);
   OnMessage(msg);
     
   // Set the path label
   // hack to remove passwords
   CStdString strData = m_currentFolder;
   CUtil::RemovePasswordFromPath(strData);
   
   CGUILabelControl* pathLabel = (CGUILabelControl*) GetControl(CONTROL_PATH_LABEL);   
   pathLabel->SetLabel(strData);

   if (folders != NULL)
 	delete folders;
}   

CStdString CGUIWindowBoxeeMediaSourceAddFolder::GetSelectedFolderThumbPath()
{
  return m_currentFolderThumbPath;
}

CStdString CGUIWindowBoxeeMediaSourceAddFolder::GetSelectedEncodedPath()
{
  CStdString strPath = m_currentFolder;
  
  // Get the password used to browse this share and store it as well
  CURL url(m_currentFolder);
  
  if (url.GetProtocol() == "smb") 
  {
    /* must url encode this as, auth code will look for the encoded value */
    CStdString strShare;
	
	CStdString hostName = url.GetHostName();
	CUtil::URLEncode(hostName);
	CStdString shareName = url.GetShareName();
	CUtil::URLEncode(shareName);

	strShare  = hostName;
    strShare += "/";
    strShare += shareName;
  
    IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);
    if (it != g_passwordManager.m_mapSMBPasswordCache.end())
    {
      // if share found in cache use it to supply username and password
      CURL url2(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
      CStdString strPassword = url2.GetPassWord();
      CStdString strUserName = url2.GetUserName();
      url.SetPassword(strPassword);
      url.SetUserName(strUserName);
    }    
  
    // first we encode the url to get the full path including the user/pass.
    // then we decode it again to get it in human readable format, which is what we store in sources.xml
    //strPath = smb.URLDecode(smb.URLEncode(url));
    url.GetURL(strPath);
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
    pDlgSourceInfo->SetAddSource(GetSelectedEncodedPath());
    pDlgSourceInfo->SetSourceThumbPath(GetSelectedFolderThumbPath());
  }
  
  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);  
}

