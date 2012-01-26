
#include <vector>
#include <set>
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
#include "GUIDialogOK2.h"

#define CONTROL_TITLE_LABEL              40
#define CONTROL_SHARES_LIST              42
#define CONTROL_REFRESH_BUTTON           53
#define CONTROL_COUNT_LABEL              54

CGUIWindowBoxeeMediaSourceAddShare::CGUIWindowBoxeeMediaSourceAddShare(void) : CGUIDialog(WINDOW_BOXEE_MEDIA_SOURCE_ADD_SHARE, "boxee_media_source_add_share.xml")
{

}

CGUIWindowBoxeeMediaSourceAddShare::~CGUIWindowBoxeeMediaSourceAddShare(void)
{

}

bool CGUIWindowBoxeeMediaSourceAddShare::OnAction(const CAction &action)
{
  int iControl = GetFocusedControlID();

  bool bSelectAction = ((action.id == ACTION_SELECT_ITEM) || (action.id == ACTION_MOUSE_LEFT_CLICK));

  if (bSelectAction && iControl == CONTROL_SHARES_LIST)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit in CONTROL_SHARES_LIST. Going to call ControlListSelectAction(); (msmk)");

    return ControlListSelectAction();
  }

  else if (bSelectAction && iControl == CONTROL_REFRESH_BUTTON && !m_addButton)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit in CONTROL_REFRESH_BUTTON. Going to call ProccessHitOnRefreshButton() (msmk)");

    ProccessHitOnRefreshButton();

    return true;
  }

  else if (bSelectAction && iControl == CONTROL_REFRESH_BUTTON && m_addButton)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit in CONTROL_REFRESH_BUTTON. Going to call ProccessHitOnAddButton() (msmk)");

      ProccessHitOnAddButton();

      return true;
    }

  else if ((action.id == ACTION_MOVE_LEFT || action.id == ACTION_MOVE_UP) && iControl == CONTROL_REFRESH_BUTTON && !m_devicesFound)
  {
    return true;
  }

  else if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::OnAction - Hit on ACTION_PREVIOUS_MENU (msmk)");

    return PreviousMenuAction();
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
  case GUI_MSG_MANAGE_ITEM:
  {
    if (message.GetParam1() == GUI_MSG_ADD_ITEM && (m_listState == CControlListState::CLS_DEVICES))
    {
      if (m_currentProtocol == CProtocols::PTCL_UPNP)
      {
        CStdString strPath = message.GetStringParam(0);
        CStdString strFriendlyName = message.GetStringParam(1);
        CStdString strIconURL = message.GetStringParam(2);

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddFolder::OnMessage, adding new UPnP source, strPath=[%s],strFriendlyName=[%s],strIconURL=[%s](browse)",
            strPath.c_str(), strFriendlyName.c_str(), strIconURL.c_str());

        CFileItemPtr pItem(new CFileItem(strFriendlyName));
        pItem->m_strPath = strPath;
        pItem->m_bIsFolder = true;
        pItem->SetThumbnailImage(strIconURL);
        pItem->SetProperty("foldericon","folder");
        pItem->SetIconImage("wizard_folder_icon.png");

        m_devices.Add(pItem);
        m_devices.ClearSortState();
        m_devices.Sort(SORT_METHOD_LABEL,SORT_ORDER_ASC);

        CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SHARES_LIST, 0, 0, &m_devices);
        OnMessage(msg);
        return true;
      }
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIWindowBoxeeMediaSourceAddShare::PreviousMenuAction()
{
  CONTROL_ENABLE(CONTROL_SHARES_LIST);
  bool res = false;
  if (m_listState == CControlListState::CLS_DEVICES)
  {
    if(m_currentProtocol == CProtocols::PTCL_SAMBA)
    {
      if(ShowNetworkDevices())
      {
        m_listState = CControlListState::CLS_PCOMPUTERS;
      }
    }
    else
    {
      if(ShowProtocols())
      {
        m_listState = CControlListState::CLS_PROTOCOLS;
      }
    }
    res = true;
    m_addButton = false;
    SET_CONTROL_LABEL(CONTROL_REFRESH_BUTTON,"[UPPERCASE][B]" + g_localizeStrings.Get(51842) + "[/B][/UPPERCASE]");
  }
  else if (m_listState == CControlListState::CLS_PCOMPUTERS)
  {
    if(ShowProtocols())
    {
      m_listState = CControlListState::CLS_PROTOCOLS;
    }
    res = true;
  }
  else if (m_listState == CControlListState::CLS_PROTOCOLS)
  {
    Close();
    res = true;
  }
  else if (m_listState == CControlListState::CLS_PCOMPUTERS_NO_RESULTS)
  {
    if(ShowProtocols())
    {
      m_listState = CControlListState::CLS_PROTOCOLS;
    }
    res = true;
  }
  else if (m_listState == CControlListState::CLS_DEVICES_NO_RESULTS)
  {
    if(m_currentProtocol == CProtocols::PTCL_SAMBA)
    {
      if(ShowNetworkDevices())
      {
        m_listState = CControlListState::CLS_PCOMPUTERS;
      }
    }
    else
    {
      if(ShowProtocols())
      {
        m_listState = CControlListState::CLS_PROTOCOLS;
      }
    }
    res = true;
  }

  SET_CONTROL_LABEL(CONTROL_COUNT_LABEL , GetItemDescription().ToLower());
  return res;
}

bool CGUIWindowBoxeeMediaSourceAddShare::ControlListSelectAction()
{
  bool succeed = false;
  bool res = false;

  if(m_listState == CControlListState::CLS_PROTOCOLS)
  {
    if (!GetSelectedProtocol())
    {
      res = false;
    }

    if(m_currentProtocol == CProtocols::PTCL_LOCAL)
    {
      succeed = ShowLocalDevices();
    }
    else
    {
      succeed = ShowNetworkDevices();
    }

    if(succeed)
    {
      //update state
      if(m_currentProtocol == CProtocols::PTCL_SAMBA)
      {
        m_listState = CControlListState::CLS_PCOMPUTERS;
      }
      else
      {
        m_listState = CControlListState::CLS_DEVICES;
      }
    }

    res = true;
  }
  else if(m_listState == CControlListState::CLS_PCOMPUTERS)
  {
    if (!SetSelectedComputer())
    {
      res = false;
    }

    if(g_localizeStrings.Get(51044).Find(m_currentComputer) != -1)
    {
      succeed = ProccessItemSelectedInControlShareList();
      if(m_currentComputer.Find(g_localizeStrings.Get(51044)) != -1 && m_currentProtocol == CProtocols::PTCL_SAMBA)
      {
        m_currentComputer = "";
        succeed = false;
      }
    }
    else
    {
      succeed = ShowComputerDevices();
    }

    if(succeed)
    {
      m_listState = CControlListState::CLS_DEVICES;
    }
    res = true;
  }
  else if (m_listState == CControlListState::CLS_DEVICES)
  {
    ProccessItemSelectedInControlShareList();
    res = true;
  }

  SET_CONTROL_LABEL(CONTROL_COUNT_LABEL , GetItemDescription().ToLower());
  return res;
}

void CGUIWindowBoxeeMediaSourceAddShare::ProccessHitOnAddButton()
{
  CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
  if (pDlgSourceInfo)
  {
    CStdString path = "smb://" + m_currentComputer;
    pDlgSourceInfo->SetAddSource(path);
  }
  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
}

void CGUIWindowBoxeeMediaSourceAddShare::OnInitWindow()
{
  CGUIWindow::OnInitWindow();
  m_listState = CControlListState::CLS_PROTOCOLS;
  Refresh();

  SET_CONTROL_LABEL(CONTROL_TITLE_LABEL, g_localizeStrings.Get(51300));
  m_addButton = false;

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
  if( m_listState == CControlListState::CLS_PROTOCOLS)
  {
    ShowProtocols();
  }
  else if(m_listState == CControlListState::CLS_PCOMPUTERS)
  {
    ShowNetworkDevices();
  }
  else if(m_listState == CControlListState::CLS_DEVICES)
  {
    if(m_currentProtocol == CProtocols::PTCL_SAMBA)
    {
      ShowComputerDevices();
    }
    if(m_currentProtocol == CProtocols::PTCL_LOCAL)
    {
      ShowLocalDevices();
    }
    else
    {
      ShowNetworkDevices();
    }
  }
}

void CGUIWindowBoxeeMediaSourceAddShare::BrowseWorkgroupsJob::Run()
{
  if(m_folders == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::BrowseWorkgroupsJob::Run() - enter with a NULL pointer (msmk)");
    return;
  }

  DIRECTORY::CDirectory dir;
  m_bJobResult = dir.GetDirectory(m_sharePath, *m_folders, "", true, true);

  // in case that the job was canceled it is his responsibility to
  // release the m_folders parameters
  if ((m_IsCanceled) && (m_folders != NULL))
    delete m_folders;
}

void CGUIWindowBoxeeMediaSourceAddShare::BrowseComputersJob::Run()
{
  if(m_folders == NULL)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::BrowseComputersJob::Run() - enter with a NULL pointer (msmk)");
    return;
  }

  DIRECTORY::CDirectory dir;
  m_bJobResult = dir.GetDirectory(m_sharePath, *m_folders);

  // in case that the job was canceled it is his responsibility to
  // release the m_folders parameters
  if ((m_IsCanceled) && (m_folders != NULL))
    delete m_folders;
}

void CGUIWindowBoxeeMediaSourceAddShare::ClearDevicesList()
{
   // Clear the list first
   CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_SHARES_LIST);
   OnMessage(msgReset);

   m_devices.Clear();
}

bool CGUIWindowBoxeeMediaSourceAddShare::ShowProtocols()
{
  CFileItemList *devicesTmp = new CFileItemList;

  bool bResult = (CUtil::RunInBG(new BrowseWorkgroupsJob("network://protocols",devicesTmp)) == JOB_SUCCEEDED);

  if(!bResult)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::ShowNetworkDevices - Unable found network devices (msmk)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(257),g_localizeStrings.Get(50006));
    m_devicesFound = false;
    return false;
  }

  ClearDevicesList();
  m_devices = *devicesTmp;

  for(int i = 0;i < m_devices.Size(); i++)
  {
    m_devices[i]->SetProperty("foldericon","network_folder");
    if(m_devices[i]->GetPropertyBOOL("IsRemovable"))
    {
      m_devices.Remove(i);
      i--;
    }
  }

  m_devices.Sort(SORT_METHOD_LABEL,SORT_ORDER_ASC);

  CFileItemPtr localprotocol(new CFileItem ( g_localizeStrings.Get(51254)));
  localprotocol->m_bIsFolder = true;
  localprotocol->SetProperty("foldericon","network_folder");
  m_devices.Add(localprotocol);

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SHARES_LIST, 0, 0, &m_devices);
  OnMessage(msg);

  m_devicesFound = true;

  if (devicesTmp != NULL)
    delete devicesTmp;
  return true;
}

bool CGUIWindowBoxeeMediaSourceAddShare::ShowNetworkDevices()
{
  m_devicesFound = false;

  CFileItemList *devices = new CFileItemList;
  CStdString strPath = GetProtocolFromEnum(m_currentProtocol) + "://";

  if(m_currentProtocol != CProtocols::PTCL_SAMBA)
  {
    strPath += "all";
  }
  else
  {
    strPath += "computers";
  }

  bool bResult = (CUtil::RunInBG(new BrowseWorkgroupsJob(strPath,devices)) == JOB_SUCCEEDED);

  if(!bResult)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::ShowNetworkDevices - Unable found network devices (msmk)");
    return false;
  }

  ClearDevicesList();

  for(int c = 0; c < devices->Size(); c++)
  {
    CFileItemPtr computer = (*devices)[c];
    computer->SetProperty("foldericon","network_folder");
    m_devices.Add(computer);
  }

  m_devices.Sort(SORT_METHOD_LABEL,SORT_ORDER_ASC);

  CFileItemPtr manual ( new CFileItem (g_localizeStrings.Get(51044)));
  manual->SetProperty("foldericon","network_folder");
  m_devices.Add(manual);

  if(m_devices.Size() > 0)
  {
    m_devicesFound = true;
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SHARES_LIST, 0, 0, &m_devices);
    OnMessage(msg);
  }
  // No results found
  else
  {
    m_devicesFound = false;
    if(m_currentProtocol == CProtocols::PTCL_SAMBA)
    {
      m_listState = CControlListState::CLS_PCOMPUTERS_NO_RESULTS;
    }
    else
    {
      m_listState = CControlListState::CLS_DEVICES_NO_RESULTS;
    }
    HandleNoResults();
  }

  if (devices != NULL)
    delete devices;
  return true;
}

CStdString CGUIWindowBoxeeMediaSourceAddShare::GetItemDescription()
{
  CStdString label = "";
  if(m_listState == CControlListState::CLS_PCOMPUTERS)
  {
    label = "smb://";
  }
  else if(m_listState == CControlListState::CLS_DEVICES)
  {
    if(m_currentProtocol == CProtocols::PTCL_SAMBA)
    {
      label = "smb://" + m_currentComputer;
    }
    else if(m_currentProtocol == CProtocols::PTCL_BMS)
    {
      label = "bmm://";
    }
    else
    {
      label = GetProtocolFromEnum(m_currentProtocol) + "://";
    }
  }
  return label;
}

CStdString CGUIWindowBoxeeMediaSourceAddShare::GetProtocolFromEnum(CProtocols::ProtocolsEnums protocolEnum)
{
  CStdString protocol = "smb";

  if(protocolEnum == CProtocols::PTCL_UPNP)
  {
    protocol = "upnp";
  }
  else if(protocolEnum == CProtocols::PTCL_NFS)
  {
    protocol = "nfs";
  }
  else if(protocolEnum == CProtocols::PTCL_AFP)
  {
    protocol = "afp";
  }
  else if(protocolEnum == CProtocols::PTCL_LOCAL)
  {
    protocol = "local";
  }
  else if(protocolEnum == CProtocols::PTCL_BMS)
  {
    protocol = "bms";
  }

  return protocol;
}

bool CGUIWindowBoxeeMediaSourceAddShare::ShowComputerDevices()
{
  m_devicesFound = false;

  CFileItemList *devices = new CFileItemList;
  CStdString strPath = "smb://computers/?name=" + m_currentComputer;

  bool bResult = (CUtil::RunInBG(new BrowseWorkgroupsJob(strPath,devices)) == JOB_SUCCEEDED);

  if(!bResult)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::ShowComputerDevices - Unable found network devices (msmk)");
    return false;
  }

  ClearDevicesList();

  for(int c = 0; c < devices->Size(); c++)
  {
    CFileItemPtr computer = (*devices)[c];
    computer->SetProperty("foldericon","network_folder");
    m_devices.Add(computer);
  }

  if(m_devices.Size() > 0)
  {
    m_devicesFound = true;
    m_devices.Sort(SORT_METHOD_LABEL,SORT_ORDER_ASC);
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_SHARES_LIST, 0, 0, &m_devices);
    OnMessage(msg);
  }
  // No results found
  else
  {
    m_devicesFound = false;
    m_listState = CControlListState::CLS_DEVICES_NO_RESULTS;
    HandleNoResults();
  }

  if (devices != NULL)
    delete devices;
  return true;
}

bool CGUIWindowBoxeeMediaSourceAddShare::ShowLocalDevices()
{
  ClearDevicesList();
  m_devicesFound = false;

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

  CFileItemList sharesToAddList;
  CStdString devicePath;


#if (defined(__APPLE__) || defined(_LINUX)) && defined(HAS_LOCAL_MEDIA)
#ifndef HAS_EMBEDDED
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
#ifndef HAS_EMBEDDED
    CFileItemPtr share ( new CFileItem(g_localizeStrings.Get(53742)));
    share->SetProperty("foldericon","folder");
    m_devices.Add(share);
    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, share);
    OnMessage(msg);

    SET_CONTROL_FOCUS(CONTROL_REFRESH_BUTTON, 0);
#endif
  }
  else
  {
    SET_CONTROL_FOCUS(CONTROL_SHARES_LIST, 0);
  }

  if(m_devices.Size() == 0)
  {
    HandleNoResults();
  }
  return true;
}

bool CGUIWindowBoxeeMediaSourceAddShare::SetSelectedComputer()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msg);
  int iComputer = msg.GetParam1();

  if (iComputer < 0 || iComputer > m_devices.Size())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::SetSelectedComputer - FAILED to get [iComputer=%d] (msmk)",iComputer);
    return false;
  }

  m_currentComputer = m_devices[iComputer]->GetLabel();
  return true;
}

bool CGUIWindowBoxeeMediaSourceAddShare::SetProtocolFromLabel(int selectedLabel)
{
  CStdString label = m_devices[selectedLabel]->m_strPath;
  bool res = false;

  if(label.Find("smb://") != -1)
  {
    m_currentProtocol = CProtocols::PTCL_SAMBA;
    res = true;
  }
  else if(label.Find("nfs://") != -1)
  {
    m_currentProtocol = CProtocols::PTCL_NFS;
    res = true;
  }
  else if(label.Find("afp://") != -1)
  {
    m_currentProtocol = CProtocols::PTCL_AFP;
    res = true;
  }
  else if(label.Find("upnp://") != -1)
  {
    m_currentProtocol = CProtocols::PTCL_UPNP;
    res = true;
  }
  else if(label.Find("bms://") != -1)
  {
    m_currentProtocol = CProtocols::PTCL_BMS;
    res = true;
  }
  else if(m_devices[selectedLabel]->GetLabel().Equals(g_localizeStrings.Get(51254)))
  {
    m_currentProtocol = CProtocols::PTCL_LOCAL;
    res = true;
  }
  return res;
}

bool CGUIWindowBoxeeMediaSourceAddShare::GetSelectedProtocol()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msg);
  int iProtocol = msg.GetParam1();

  if (iProtocol < 0 || iProtocol > m_devices.Size())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::GetSelectedProtocol - FAILED to convert [iProtocol=%d] to ProtocolsEnums (msmk)",iProtocol);
    return false;
  }

  if(!SetProtocolFromLabel(iProtocol))
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaSourceAddShare::GetSelectedProtocol - FAILED selected protocol not recognized [iProtocol=%d] (msmk)",iProtocol);
    return false;
  }

  return true;
}

CFileItemPtr CGUIWindowBoxeeMediaSourceAddShare::GetSelectedFileItem()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msg);
  int iDevice = msg.GetParam1();

  return m_devices[iDevice];
}

bool CGUIWindowBoxeeMediaSourceAddShare::ProccessItemSelectedInControlShareList()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::ProccessItemSelectedInControlShareList - Enter function (msmk)");

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_SHARES_LIST);
  OnMessage(msg);
  int iDevice = msg.GetParam1();

  if (m_devices[iDevice]->GetLabel().Equals(g_localizeStrings.Get(51044)))
  {
    CGUIWindowBoxeeWizardAddSourceManual* pDlgAddSourceManual = (CGUIWindowBoxeeWizardAddSourceManual*)g_windowManager.GetWindow(WINDOW_BOXEE_WIZARD_ADD_SOURCE_MANUAL);
    pDlgAddSourceManual->DoModal();
    if (!pDlgAddSourceManual->IsConfirmed())
    {
      return false;
    }

    pDlgAddSourceManual->SetProtocol(GetProtocolFromEnum(m_currentProtocol));
    m_devices[iDevice]->m_strPath = pDlgAddSourceManual->GetURL();
  }

  g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_ADD_FOLDER);
  return true;
}

void CGUIWindowBoxeeMediaSourceAddShare::HandleNoResults()
{
  CFileItemPtr noResults(new CFileItem ( g_localizeStrings.Get(51929)));
  CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_SHARES_LIST, 0, 0, noResults);
  OnMessage(msg);
  SET_CONTROL_FOCUS(CONTROL_REFRESH_BUTTON,0);
  CONTROL_DISABLE(CONTROL_SHARES_LIST);
}

void CGUIWindowBoxeeMediaSourceAddShare::ProccessHitOnRefreshButton()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMediaSourceAddShare::ProccessHitOnRefreshButton - Enter function (msmk)");
  Refresh();
}
