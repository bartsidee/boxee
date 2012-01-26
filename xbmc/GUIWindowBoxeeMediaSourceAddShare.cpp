
#include <vector>
#include "GUIWindowBoxeeMediaSources.h"
#include "GUIWindowBoxeeMediaSourceAddShare.h"
#include "Application.h"
#include "GUIDialogProgress.h"
#include "GUIWindowBoxeeWizardAddSourceManual.h"
#include "GUIWindowManager.h"
#include "FileSystem/FactoryDirectory.h"
#include "GUIDialogOK.h"
#include "Util.h"
#include "BoxeeMediaSourceList.h"
#include "Settings.h"
#include "MediaManager.h"
#include "FileSystem/Directory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "AppManager.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "utils/log.h"

#define CONTROL_TITLE_LABEL    40
#define CONTROL_SHARES_LIST     52
#define CONTROL_REFRESH_BUTTON  53

CGUIWindowBoxeeMediaSourceAddShare::CGUIWindowBoxeeMediaSourceAddShare(void)
    : CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCE_ADD_SHARE, "boxee_media_source_add_share.xml")
{
}

CGUIWindowBoxeeMediaSourceAddShare::~CGUIWindowBoxeeMediaSourceAddShare(void)
{}


bool CGUIWindowBoxeeMediaSourceAddShare::OnAction(const CAction &action)
{
  int iControl = GetFocusedControlID();

  bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));

  if (bSelectAction && iControl == CONTROL_SHARES_LIST)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit in CONTROL_SHARES_LIST. Going to call ProccessItemSelectedInControlShareList() (msmk)");

    ProccessItemSelectedInControlShareList();
    
    return true;
  }
  else if (bSelectAction && iControl == CONTROL_REFRESH_BUTTON)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit in CONTROL_REFRESH_BUTTON. Going to call ProccessHitOnRefreshButton() (msmk)");

    ProccessHitOnRefreshButton();

    return true;
  }
  else if ((action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP) && iControl == CONTROL_REFRESH_BUTTON && !m_devicesFound)
  {
    return true;
  }
  else if (action.id == ACTION_PREVIOUS_MENU)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit on ACTION_PREVIOUS_MENU (msmk)");

    Close();

    return true;
   }
  else if (action.id == ACTION_PARENT_DIR)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit on ACTION_PARENT_DIR (msmk)");

    Close();

    return true;
   }

   return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeMediaSourceAddShare::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {    
    DWORD senderId = message.GetSenderId();
    
    if(senderId == CONTROL_SHARES_LIST)
    {
      if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
      {
        // Handle only GUI_MSG_CLICKED on CONTROL_SHARES_LIST that origin from navigation

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_SOURCE_LIST]. Going to call ProccessItemSelectedInControlShareList() (msmk)");

        ProccessItemSelectedInControlShareList();
      
        return true;
      }
    }
    else if(senderId == CONTROL_REFRESH_BUTTON)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnMessage - Enter GUI_MSG_CLICKED case with [SenderId=CONTROL_REFRESH_BUTTON]. Going to call ProccessHitOnRefreshButton() (msmk)");

      ProccessHitOnRefreshButton();
      
      return true;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowBoxeeMediaSourceAddShare::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  Refresh();

  SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, g_localizeStrings.Get(51300));

/*  CGUIWindowBoxeeMediaSources *pDlgSources = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (!pDlgSources)
    return;
  int selectedSource = pDlgSources->getSelectedSource();

  if (selectedSource == SOURCE_LOCAL)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, g_localizeStrings.Get(51300));
  }
  else if (selectedSource == SOURCE_NETWORK)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, g_localizeStrings.Get(51301));
  }
  else if (selectedSource == MANUALLY_ADD_SOURCE)
  {
     SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, g_localizeStrings.Get(51304));
  } */
}

void CGUIWindowBoxeeMediaSourceAddShare::Refresh()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::Refresh - Enter function (msmk)");

/*  CGUIWindowBoxeeMediaSources *pDlgSources = (CGUIWindowBoxeeMediaSources*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCES);
  if (!pDlgSources)
    return;
  m_selectedSource = pDlgSources->getSelectedSource();

  if (m_selectedSource == SOURCE_LOCAL)
  {
     ShowLocalDevices();
  }
  else if (m_selectedSource == SOURCE_NETWORK)
  {
     ShowNetworkDevices();
  }
  else if (m_selectedSource == MANUALLY_ADD_SOURCE)
  {
     
  } */
  ClearDevicesList();
  ShowLocalDevices();
  ShowNetworkDevices();
}

void CGUIWindowBoxeeMediaSourceAddShare::BrowseWorkgroupsJob::Run()
{
  DIRECTORY::CDirectory dir;
  m_bJobResult = dir.GetDirectory(m_sharePath, m_folders, "", true, true);
}

void CGUIWindowBoxeeMediaSourceAddShare::BrowseComputersJob::Run()
{
  DIRECTORY::CDirectory dir;
  m_bJobResult = dir.GetDirectory(m_sharePath, m_folders);
}

void CGUIWindowBoxeeMediaSourceAddShare::ClearDevicesList()
{
  // Clear the list first
  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msgReset);

  m_devices.Clear();
}
void CGUIWindowBoxeeMediaSourceAddShare::ShowNetworkDevices()
{

   CFileItemList computers;

   std::vector<CStdString> vecShares;
   CStdString strPath = "network://";
   
   BrowseWorkgroupsJob* job = new BrowseWorkgroupsJob(strPath, computers);
   CUtil::RunInBG(job);

   for (int w = 0; w < computers.Size(); w++)
   {
       CFileItemPtr computer = computers[w] ;
       computer->SetProperty("foldericon","network_folder");
       m_devices.Add(computer);
       CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, computer);
       OnMessage(msg);
   }

   CFileItemPtr manual ( new CFileItem(g_localizeStrings.Get(51044)) );
   m_devices.Add(manual);
   CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, manual);
   OnMessage(msg);

   m_devicesFound = true;
}

void CGUIWindowBoxeeMediaSourceAddShare::ShowLocalDevices()
{
  m_devicesFound = false;

#ifndef _WIN32
  VECSOURCES removableDrives;
  g_mediaManager.GetRemovableDrives(removableDrives);
  
  for (size_t i = 0; i < removableDrives.size(); i++)
  {
    CMediaSource removableDrive = removableDrives[i];
    CFileItemPtr share ( new CFileItem(removableDrive.strName)  );      
    share->m_strPath = removableDrive.strPath;
    
    share->SetProperty("foldericon","folder");
    m_devices.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, share);
    OnMessage(msg);
    m_devicesFound = true;
  }
#endif
  CFileItemList sharesToAddList;
  CStdString devicePath;
  
  
#if (defined(__APPLE__) || defined(_LINUX)) && defined(HAS_LOCAL_MEDIA)
  // Add user home directory
  devicePath = getenv("HOME");
  CFileItemPtr shareUserHome ( new CFileItem(CUtil::GetTitleFromPath(devicePath, true)) );
  shareUserHome->m_strPath = devicePath;
  shareUserHome->SetProperty("IsHomeDirectory",true);	
  shareUserHome->SetProperty("foldericon","folder");
  sharesToAddList.Add(shareUserHome);
  
  // Add root directory
  devicePath = "/";
  CFileItemPtr shareRootDirectory ( new CFileItem(devicePath));
  shareRootDirectory->m_strPath = (devicePath);
  sharesToAddList.Add(shareRootDirectory);
#endif	
  
#ifdef _WIN32
  // Add windows drives as sources
  VECSOURCES drives;
  g_mediaManager.GetLocalDrives(drives);
  for (size_t i = 0; i < drives.size(); i++)
  {
    CMediaSource source = drives[i];
    devicePath = source.strPath;
    
    //printf("Add win share: name = %s, path = %s, status = %s, type = %d", 
    //       source.strName.c_str(), source.strPath.c_str(), source.strStatus.c_str(), source.m_iDriveType);
    
    CFileItemPtr shareRootDirectory(new CFileItem(source.strName));
    shareRootDirectory->m_strPath = (devicePath);
    shareRootDirectory->SetProperty("foldericon","folder");
    //shareRootDirectory->SetProperty("IsHomeDirectory",true); 
    sharesToAddList.Add(shareRootDirectory);
  }
  
#endif
  
  CFileItemPtr share;
  for(int i=0; i<sharesToAddList.Size(); i++)
  {
    CFileItemPtr share = sharesToAddList.Get(i);
    share->SetProperty("foldericon","folder");
    m_devices.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, share);
    OnMessage(msg);
    m_devicesFound = true;	  
  }
  
  if (!m_devicesFound)
  {
    CFileItemPtr share ( new CFileItem(g_localizeStrings.Get(53742)));
    share->SetProperty("foldericon","folder");
    m_devices.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, share);
    OnMessage(msg);
    
    SET_CONTROL_FOCUS(CONTROL_REFRESH_BUTTON, 0);
  }
  else
  {
    SET_CONTROL_FOCUS(CONTROL_SHARES_LIST, 0);
  }
}

CFileItemPtr CGUIWindowBoxeeMediaSourceAddShare::GetSelectedFileItem()
{
	CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
	OnMessage(msg);
	int iDevice = msg.GetParam1();

	return m_devices[iDevice];
}

void CGUIWindowBoxeeMediaSourceAddShare::ProccessItemSelectedInControlShareList()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::ProccessItemSelectedInControlShareList - Enter function (msmk)");

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msg);
  int iDevice = msg.GetParam1();

  if (iDevice == m_devices.Size()-1 && m_devices[iDevice]->GetLabel().Equals(g_localizeStrings.Get(51044)))
  {
    CGUIWindowBoxeeWizardAddSourceManual* pDlgAddSourceManual = (CGUIWindowBoxeeWizardAddSourceManual*)g_windowManager.GetWindow(WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL);
    
    pDlgAddSourceManual->DoModal();
    
    if (!pDlgAddSourceManual->IsConfirmed())
    {
      return;
    }

    m_devices[iDevice]->m_strPath = pDlgAddSourceManual->GetURL();
  }

  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_ADD_FOLDER);
}

void CGUIWindowBoxeeMediaSourceAddShare::ProccessHitOnRefreshButton()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::ProccessHitOnRefreshButton - Enter function (msmk)");

  Refresh();  
}


