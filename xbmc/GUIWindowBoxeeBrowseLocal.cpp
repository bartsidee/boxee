
#include "GUIWindowBoxeeBrowseLocal.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "SpecialProtocol.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIDialogBoxeeDropdown.h"
#include "Application.h"
#include "MediaSource.h"
#include "lib/libBoxee/boxee.h"
#include "GUIWindowBoxeeMediaSourceInfo.h"
#include "MediaManager.h"
#include "HalServices.h"
#include "BoxeeUtils.h"
#include "GUIButtonControl.h"
#include "UPnPDirectory.h" // for directory renaming
#include "AfpDirectory.h"
#include "GUIWindowBoxeeMediaSourceAddFolder.h"
#include "BoxeeBrowseMenuManager.h"
#include "GUIControl.h"
#include "SMBUtils.h"
#include "GUIDialogBoxeeEject.h"

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

#define BUTTON_ADD_SOURCE 130 // implemented from skin
#define BUTTON_SCAN 131 // implemented from skin
#define BUTTON_EJECT 132
#define BUTTON_EJECT_USER 133
#define SORT_DROPDOWN_BUTTON 8014
#define CONTROL_LIST   59
#define EMPTY_STATES_LABEL  7094

#define FILES_TITLE_LABEL  9019
#define ITEM_SUMMARY       9018
#define ITEM_SUMMARY_FLAG "item-summary"
#define ITEM_COUNT_LABEL "item-summary-count"

CLocalFilesSource::CLocalFilesSource(int iWindowID) : CBrowseWindowSource("localsource", "sources://all/", iWindowID)
{

}

CLocalFilesSource::~CLocalFilesSource()
{

}

void CLocalFilesSource::RequestItems(bool bUseCurPath)
{
  CBoxeeSort previousSort = m_sourceSort;
  CStdString strPath = GetBasePath();


  if (strPath.Find("sources://all") >= 0 || strPath.CompareNoCase("network://protocols") == 0)
  {
    m_sourceSort.Reset();
  }
  else if (strPath.IsEmpty() ||
      strPath == "network:/" ||
      strPath.Equals("afp:/") ||
      (strPath.Left(6).Equals("afp://") && strPath.Right(6).Equals(".local")) ||
      strPath.Left(15).Equals("smb://computers") ||
      strPath == "nfs:/" ||
      (strPath.Left(6).Equals("nfs://") && strPath.Right(6).Equals(".local")) ||
      strPath.Left(6).Equals("upnp:/") ||
      strPath.Left(5).Equals("bms:/")
      )
  {
    CBoxeeSort specialCasesSort(VIEW_SORT_METHOD_ATOZ, SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), "start");
    m_sourceSort = specialCasesSort;
  }  

  CBrowseWindowSource::RequestItems(bUseCurPath);
  m_sourceSort = previousSort;
}

void CLocalFilesSource::BindItems(CFileItemList& items)
{
  // Perform post processing here
  // Need to default video content to help with the UPnP case where we don't really know the content type
  bool bDefaultVideo = false;

  if( items.m_strPath.Equals("boxeedb://unresolvedVideoFiles") )
    bDefaultVideo = true;

  for (int i = 0 ; i < items.Size() ; i++)
  {
    CFileItemPtr pItem = items[i];

    if (pItem->IsVideo() || bDefaultVideo)
    {
      pItem->SetProperty("IsVideo", true);
    }
    else if (pItem->IsAudio())
    {
      pItem->SetProperty("IsMusic", true);
    }
    else if (pItem->IsPicture())
    {
      pItem->SetProperty("IsPhotos", true);
    }
    else if (!pItem->m_bIsFolder)
    {
      items.Remove(i);
      i--;
    }
  }

  CBrowseWindowSource::BindItems(items);
}

CLocalBrowseWindowState::CLocalBrowseWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowStateWithHistory(pWindow)
{
  m_sourceController.SetNewSource(new CLocalFilesSource(m_pWindow->GetID()));
}

void CLocalBrowseWindowState::InitState()
{
  m_strTitle = g_localizeStrings.Get(52044);
  m_strLabel = g_localizeStrings.Get(52044);

  // Initialize sort vector
  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), "start"));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_DATE, SORT_METHOD_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53534), "start"));

  CBrowseWindowStateWithHistory::InitState();
}

void CLocalBrowseWindowState::Refresh(bool bResetSeleted)
{
  CBrowseWindowStateWithHistory::Refresh(bResetSeleted);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());

  ((CGUIWindowBoxeeBrowseLocal*) m_pWindow)->UpdateEjectButtonState();

  m_pWindow->SetProperty("is-browse-root", (GetCurrentPath().Find("sources://all") != -1) ? true : false);
}

CStdString CLocalBrowseWindowState::GetItemSummary()
{
  CStdString itemSummary = "";
  std::map<CStdString , CStdString> mapTitleItemValue;

  if (GetCurrentPath().Find("network://protocols") >= 0)
  {
    return g_localizeStrings.Get(51712);
  }

  if (GetCurrentPath().Find("sources://all") >= 0)
  {
    return g_localizeStrings.Get(51406);
  }

  else
  {
    CStdString currentPath = GetCurrentPath();
    CStdString currentTitle = g_localizeStrings.Get(744);;
    if(currentPath == "afp://")
    {
      currentTitle = g_localizeStrings.Get(51253); //Apple Filing Protocol (AFP)
    }
    else if(currentPath == "bms://")
    {
      currentTitle = g_localizeStrings.Get(51255); //Boxee Media Manager
    }
    else if(currentPath == "nfs://")
    {
      currentTitle = g_localizeStrings.Get(51251); //Network File System (NFS)
    }
    else if(currentPath == "upnp://all")
    {
      currentTitle = g_localizeStrings.Get(51252); //Universal Plug and Play (UPnP)
    }
    else if(currentPath == "smb://computers")
    {
      currentTitle = g_localizeStrings.Get(51250); //Windows Network (SMB)
    }
    else if(currentPath.Find("smb://computers/?name=") != -1)
    {
      CStdString strComp;
      CStdString strDevices;
      std::map<std::string, std::string> mapParams;

      // Parse boxeedb url
      if (BoxeeUtils::ParseBoxeeDbUrl(currentPath, strComp, strDevices, mapParams))
      {
        if(!mapParams["name"].empty())
        {
          currentTitle = mapParams["name"];
        }
      }
    }
    else if(currentPath.Find("upnp://") != -1)
    {
      currentTitle = currentPath;
      DIRECTORY::CUPnPDirectory::GetFriendlyPath(currentTitle);
      CURI url(currentTitle);
      currentTitle = url.GetHostName();
    }
    else if(currentPath.Find("smb://") != -1 || currentPath.Find("afp://") != -1 || currentPath.Find("bms://") != -1 || currentPath.Find("nfs://") != -1 )
    {
      CURI url(currentPath);
      currentTitle = url.GetHostName();
    }
    mapTitleItemValue["media"] = currentTitle;
  }

  if (!m_sort.m_sortName.empty() && m_sort.m_id != VIEW_SORT_METHOD_ATOZ )
  {
    //show the sort only if its not A TO Z
    mapTitleItemValue["sort"] = m_sort.m_sortName;
  }

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90005), mapTitleItemValue,itemSummary))
  {
    itemSummary = g_localizeStrings.Get(744);
    CLog::Log(LOGERROR,"CLocalBrowseWindowState::GetItemSummary - Error with template [stringId=90005] (browse)");
  }

  return itemSummary;
}

void CLocalBrowseWindowState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  CLog::Log(LOGDEBUG,"CLocalBrowseWindowState::OnPathChanged - path has changed [path=%s][bResetSelected=%s] (browse)", strPath.c_str(), bResetSelected ? "true" : "false");

  CLocalFilesSource* source = new CLocalFilesSource(m_pWindow->GetID());

  source->SetBasePath(strPath);

  source->SetSortMethod(m_sourceController.GetSortMethod());

  m_sourceController.SetNewSource(source);

  CBrowseWindowStateWithHistory::OnPathChanged(strPath,bResetSelected);

  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateScanningButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateEjectButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateSortButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateEmptyLabelText();
}

void CLocalBrowseWindowState::FromHistory(CBrowseStateHistoryItem* historyItem)
{
  CLocalFilesSource* source;
  if (!historyItem) return;

  m_sourceController.RemoveAllSources();

  for (size_t i = 0; i < historyItem->m_vecSources.size(); i++)
  {
    CStdString strSourceId = historyItem->m_vecSources[i].m_strSourceId;
    CStdString strBasePath = historyItem->m_vecSources[i].m_strBasePath;

    source = new CLocalFilesSource( m_pWindow->GetID() );

    source->SetBasePath(strBasePath);
    source->SetSourceId(strSourceId);
    source->SetSortMethod(m_sourceController.GetSortMethod());

    m_sourceController.AddSource(source);

    source = NULL;
  }

  m_iSelectedItem = historyItem->m_iSelectedItem;
}


bool CLocalBrowseWindowState::OnBack()
{
  if (!CBrowseWindowStateWithHistory::OnBack())
  {
    CBrowseWindowState::OnBack();
  }

  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateScanningButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateEjectButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateSortButtonState();
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateEmptyLabelText();

  return false;
}

void CLocalBrowseWindowState::SetCurrentPath(const CStdString& path)
{
  // Return current path under assumption that Local browse screen has only one source
  SourcesMap mapSources = m_sourceController.GetSources();

  if (CUtil::IsBlurayFolder(path))
    UpdateHistory();

  if (!mapSources.empty())
  {
    CBrowseWindowSource* firstSource = mapSources.begin()->second;
    if (firstSource)
    {
      firstSource->SetBasePath(path);
    }
    else
    {
      CLog::Log(LOGWARNING, "CLocalBrowseWindowState::SetCurrentPath - first source is null (browse)");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CLocalBrowseWindowState::SetCurrentPath - no sources found (browse)");
  }
}

CStdString CLocalBrowseWindowState::GetCurrentPath()
{
  // Return current path under assumption that Local browse screen has only one source
  SourcesMap mapSources = m_sourceController.GetSources();

  if (!mapSources.empty())
  {
    CBrowseWindowSource* firstSource = mapSources.begin()->second;
    if (firstSource)
    {
      return firstSource->GetBasePath();
    }
    else
    {
      CLog::Log(LOGWARNING, "CLocalBrowseWindowState::GetCurrentPath - first source is null (browse)");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CLocalBrowseWindowState::GetCurrentPath - no sources found (browse)");
  }

  return "";
}


// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseLocal::CGUIWindowBoxeeBrowseLocal() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_LOCAL, "boxee_browse_local.xml")
{
  SetWindowState(new CLocalBrowseWindowState(this));
}

CGUIWindowBoxeeBrowseLocal::~CGUIWindowBoxeeBrowseLocal()
{
}

CStdString CGUIWindowBoxeeBrowseLocal::GetItemDescription()
{
  CStdString strPath = ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();
  CStdString translatedPath;

  if (!strPath.IsEmpty())
  {
    if (strPath.Find("sources://all") >= 0 )
    {
      translatedPath = " ";
    }
    else if (CUtil::IsHD(strPath) || CUtil::IsSmb(strPath) || CUtil::IsUPnP(strPath) || CUtil::IsAfp(strPath) || CUtil::IsNfs(strPath) || CUtil::IsBms(strPath))
    {
      // Translate the path
      translatedPath = _P(strPath);

      if( CUtil::IsBms(translatedPath) )
      {
        translatedPath[2] = 'm';
      }

      if( CUtil::IsUPnP(translatedPath) )
      {
        DIRECTORY::CUPnPDirectory::GetFriendlyPath(translatedPath);
        if(translatedPath.Find("upnp://all") != -1)
        {
          translatedPath = "upnp://";
        }
      }

      if (CUtil::IsSmb(translatedPath))
      {
        translatedPath = SMBUtils::TranslatePath(translatedPath);
      }

      if (CUtil::IsHD(strPath))
      {
        CUtil::HideExternalHDPath(translatedPath, translatedPath);
      }

      CStdString shortPath = translatedPath;
      CUtil::MakeShortenPath(translatedPath,shortPath,60);
      translatedPath = shortPath;

      if (!translatedPath.IsEmpty() && !CUtil::IsUPnP(translatedPath))
      {
        CUtil::RemovePasswordFromPath(translatedPath);
        CUtil::RemoveSlashAtEnd(translatedPath);
        CUtil::UrlDecode(translatedPath);
      }
    }
    else if (strPath == "boxeedb://unresolvedVideoFiles")
    {
      translatedPath.Format("%d %s",m_windowState->GetTotalItemCount(), g_localizeStrings.Get(90049).c_str());
    }
  }

  return translatedPath;
}

void CGUIWindowBoxeeBrowseLocal::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();

  UpdateScanningButtonState();
  UpdateEjectButtonState();
  UpdateSortButtonState();
  UpdateEmptyLabelText();

  m_windowState->SetPageSize(DISABLE_PAGING);
}

void CGUIWindowBoxeeBrowseLocal::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.find("path") != optionsMap.end())
    {
      CStdString path = optionsMap["path"];

      ((CLocalBrowseWindowState*)m_windowState)->SetCurrentPath(path);

      UpdateScanningButtonState();
      UpdateEjectButtonState();
      UpdateSortButtonState();
    }
  }
}

bool CGUIWindowBoxeeBrowseLocal::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (GetPropertyBOOL("empty") && !GetPropertyBOOL("is-browse-root"))
    {
      OnBack();
      return true;
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseLocal::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_NOTIFY_ALL:
    {
      if (message.GetParam1() == GUI_MSG_FILE_SCANNER_UPDATE)
      {
        CStdString strPath = message.GetStringParam();
        if (strPath == ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath())
        {
          SET_CONTROL_VISIBLE(BUTTON_SCAN);
          SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
          SET_CONTROL_HIDDEN(BUTTON_EJECT);
          CONTROL_ENABLE(BUTTON_SCAN);
        }
      }
      else if (message.GetParam1() == GUI_MSG_REMOVED_MEDIA)
      {
        Refresh();
      }
    }
    break;
    case GUI_MSG_MANAGE_ITEM:
    {
      if (message.GetParam1() == GUI_MSG_ADD_ITEM)
      {
        if (((CLocalBrowseWindowState*) m_windowState)->GetCurrentPath() == "upnp://all")
        {
          CStdString strPath = message.GetStringParam(0);
          CStdString strFriendlyName = message.GetStringParam(1);
          CStdString strIconURL = message.GetStringParam(2);

          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage - adding new UPnP source. [strPath=%s][strFriendlyName=%s][strIconURL=%s] (browse)",strPath.c_str(), strFriendlyName.c_str(), strIconURL.c_str());

          CFileItemPtr pItem(new CFileItem(strFriendlyName));
          pItem->m_strPath = strPath;
          pItem->m_bIsFolder = true;
          pItem->SetThumbnailImage(strIconURL);

          CFileItemList pList;
          pList.Add(pItem);
          ShowItems(pList, true);
        }
      }
    }
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage - GUI_MSG_CLICKED - [controlId=%d] (browse)", iControl);

      if (iControl == BUTTON_ADD_SOURCE)
      {
        CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
        if (pDlgSourceInfo)
        {
          CStdString path = CGUIWindowBoxeeMediaSourceAddFolder::GetSelectedEncodedPath(((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath());
          pDlgSourceInfo->SetAddSource(path);
        }

        g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);

        Refresh();
        return true;
      }
      else if (iControl == BUTTON_SCAN)
      {
        CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

        if (BoxeeUtils::DoYouWantToScan(strPath))
        {
          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage - GUI_MSG_CLICKED - BUTTON_SCAN - [path=%s] (checkpath)", strPath.c_str());
          g_application.GetBoxeeFileScanner().AddUserPath(_P(strPath));
          //BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderTreeNew(_P(strPath));

          UpdateScanningButtonState();
        }
      }
      else if(iControl == BUTTON_EJECT_USER)
      {
        CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage - GUI_MSG_CLICKED - BUTTON_EJECT_USER - [strPath=%s] (ejc)",strPath.c_str());
        EjectUserButtonPressed();
      }
      else if (iControl == BUTTON_EJECT)
      {
        // Get the list of available USB devices
        CFileItemList devices;
        VECSOURCES removableDrives;
        g_mediaManager.GetLocalDrives(removableDrives);

        CGUIDialogBoxeeEject *dialog = (CGUIDialogBoxeeEject *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_EJECT);

        dialog->Reset();
        for (size_t i = 0; i < removableDrives.size(); i++)
        {
          CMediaSource removableDrive = removableDrives[i];
          CFileItemPtr share ( new CFileItem(removableDrive.strName)  );
          share->SetProperty("IsShare","1");
          share->m_strPath = removableDrive.strPath;
          share->SetProperty("value", removableDrive.strPath);
          share->m_bIsFolder = true;
          share->SetProperty("IsRemovable",true);

          if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_USB)
            share->SetProperty("IsUSB",true);
          else if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_SD)
            share->SetProperty("IsSD",true);
          else if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_INTERNAL_HD)
            share->SetProperty("IsInternalHD",true);
          else
            share->SetProperty("IsUSB",true);

          devices.Add(share);
        }

        if(devices.Size() > 0)
        {
          dialog->Add(devices);
          dialog->DoModal();

          if(dialog->GetWasItemSelected())
          {
            CStdString value = dialog->GetSelectedItem().m_strPath;
#ifdef HAS_BOXEE_HAL
            CHalServicesFactory::GetInstance().EjectStorage(value);
#endif
          }
        }
      }
    } // case GUI_MSG_CLICKED
    break;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

void CGUIWindowBoxeeBrowseLocal::EjectUserButtonPressed()
{
#ifdef HAS_AFP
  CStdString ejectUserPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  while(!strPath.Equals("afp://all") && !strPath.Equals("afp://") && strPath.Left(13) != "sources://all")
  {
    OnBack();
    strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();
  }

  CAfpDirectory::EjectUser(ejectUserPath);
  SET_CONTROL_FOCUS(CONTROL_LIST,0);
#endif
  return;
}
void CGUIWindowBoxeeBrowseLocal::UpdateSortButtonState()
{
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  if (CUtil::HasSlashAtEnd(strPath))
  {
    // remove slash at the end
    strPath.Delete(strPath.size() - 1);
  }

  if (strPath.IsEmpty() ||
      strPath == "sources://all" ||
      strPath == "network:/" ||
      strPath == "network://protocols" ||
      strPath.Equals("afp:/") ||
      (strPath.Left(6).Equals("afp://") && strPath.Right(6).Equals(".local")) ||
      strPath.Left(15).Equals("smb://computers") ||
      strPath == "nfs:/" ||
      (strPath.Left(6).Equals("nfs://") && strPath.Right(6).Equals(".local")) ||
      strPath.Left(6).Equals("upnp:/") ||
      strPath.Left(5).Equals("bms:/")
      )
  {
    SET_CONTROL_HIDDEN(SORT_DROPDOWN_BUTTON);
  }
  else
  {
    SET_CONTROL_VISIBLE(SORT_DROPDOWN_BUTTON);
  }

  return;
}

void CGUIWindowBoxeeBrowseLocal::UpdateEmptyLabelText()
{
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  if(strPath == "afp://" || strPath == "bms://" || strPath == "nfs://" || strPath == "upnp://all" ||strPath == "smb://computers")
  {
    SET_CONTROL_LABEL(EMPTY_STATES_LABEL,g_localizeStrings.Get(50007));
    return;
  }

  SET_CONTROL_LABEL(EMPTY_STATES_LABEL,g_localizeStrings.Get(56061));
}

void CGUIWindowBoxeeBrowseLocal::UpdateEjectButtonState()
{
#ifndef HAS_EMBEDDED
  SET_CONTROL_HIDDEN(BUTTON_EJECT);
  SET_CONTROL_HIDDEN(BUTTON_EJECT_USER);
#else
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  if (CUtil::HasSlashAtEnd(strPath))
  {
    // remove slash at the end
    strPath.Delete(strPath.size() - 1);
  }

  if(!strPath.Equals("afp://") && !strPath.Equals("afp://all") && strPath.Find("afp://") != -1)
  {
    SET_CONTROL_VISIBLE(BUTTON_EJECT_USER);
    SET_CONTROL_HIDDEN(BUTTON_EJECT);
    return;
  }

  if (strPath != "network://protocols" && strPath != "")
  {
    SET_CONTROL_HIDDEN(BUTTON_EJECT);
    SET_CONTROL_HIDDEN(BUTTON_EJECT_USER);
    return;
  }


  VECSOURCES removableDrives;
  g_mediaManager.GetRemovableDrives(removableDrives);
  if (removableDrives.size() == 0)
  {
    SET_CONTROL_HIDDEN(BUTTON_EJECT);
    SET_CONTROL_HIDDEN(BUTTON_EJECT_USER);
    return;
  }

  SET_CONTROL_HIDDEN(BUTTON_EJECT_USER);
  SET_CONTROL_VISIBLE(BUTTON_EJECT);
#endif
}

void CGUIWindowBoxeeBrowseLocal::UpdateScanningButtonState(int iStatus)
{
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  if (iStatus == 0)
  {
    std::vector<CStdString> vecTypes;
    iStatus = g_application.GetBoxeeFileScanner().GetFolderStatus(_P(strPath), vecTypes);
  }

  switch (iStatus)
  {
  case FOLDER_STATUS_NONE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState - [path=%s][state=FOLDER_STATUS_NONE] (checkpath)", strPath.c_str());
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    CONTROL_DISABLE(BUTTON_ADD_SOURCE);

    SET_CONTROL_HIDDEN(BUTTON_SCAN);
    CONTROL_DISABLE(BUTTON_SCAN);
  }
  break;
  case FOLDER_STATUS_NOT_SHARE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState - [path=%s][state=FOLDER_STATUS_NOT_SHARE] (checkpath)", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_ADD_SOURCE);
    SET_CONTROL_HIDDEN(BUTTON_SCAN);
    CONTROL_ENABLE(BUTTON_ADD_SOURCE);
  }
  break;
  case FOLDER_STATUS_ON_SHARE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState - [path=%s][state=FOLDER_STATUS_ON_SHARE] (checkpath)", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_EJECT);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    CONTROL_ENABLE(BUTTON_SCAN);
  }
  break;
  case FOLDER_STATUS_PRIVATE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState - [path=%s][state=FOLDER_STATUS_PRIVATE] (checkpath)", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    CONTROL_DISABLE(BUTTON_SCAN);
  }
  break;
  case FOLDER_STATUS_SCANNING:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState - [path=%s][state=FOLDER_STATUS_SCANNING] (checkpath)", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_EJECT);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    //SET_CONTROL_LABEL(BUTTON_SCAN, "SCANNING...");
    CONTROL_ENABLE(BUTTON_SCAN);
  }
  break;
  }
  if(strPath.Equals("afp://") || strPath.Equals("bms://") || strPath.Equals("nfs://") || strPath.Equals("upnp://all") ||strPath.Equals("smb://computers") )
  {
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    CONTROL_DISABLE(BUTTON_ADD_SOURCE);
  }
  CURI uri(strPath);
  if(strPath.Find("smb://computers/?name=") != -1 || uri.GetFileName() == "")
  {
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    CONTROL_DISABLE(BUTTON_ADD_SOURCE);
  }
}

bool CGUIWindowBoxeeBrowseLocal::HandleEmptyState()
{
  int saveFocusedControl = GetFocusedControlID();
  bool retVal = CGUIWindowBoxeeBrowse::HandleEmptyState();

  if (GetPropertyBOOL("empty") && !GetPropertyBOOL("is-browse-root"))
  {
    SET_CONTROL_FOCUS(saveFocusedControl,0);
  }

  return retVal;
}

void CGUIWindowBoxeeBrowseLocal::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();
  CStdString strPath =  ((CLocalBrowseWindowState*)m_windowState)->GetCurrentPath();

  CURI path(strPath);
  CStdString protocol = path.GetProtocol();

  if (protocol == "network")
  {
    m_initSelectPosInBrowseMenu = 0;
  }
  else if (protocol == "sources")
  {
    m_initSelectPosInBrowseMenu = 1;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::GetStartMenusStructure - enter function [category=%s][path=%s] (bm)",category.c_str(),strPath.c_str());

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_local_files",browseMenuLevelList);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu]. [category=%s][path=%s] (bm)",browseMenuLevelList.size(),category.c_str(),strPath.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);
}

