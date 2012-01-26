
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

using namespace std;
using namespace BOXEE;

#define BUTTON_ADD_SOURCE 130 // implemented from skin
#define BUTTON_SCAN 131 // implemented from skin

#define BUTTON_SHORTCUT 160

#define LOCAL_WINDOW_SHORTCUT_COMMAND   "ActivateWindow(10479,%s)"

// STATE IMPLEMENTATION

CLocalBrowseWindowState::CLocalBrowseWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort("1", SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), "start"));
  m_vecSortMethods.push_back(CBoxeeSort("2", SORT_METHOD_DATE, SORT_ORDER_ASC, g_localizeStrings.Get(53530), "start"));

  SetSort(m_vecSortMethods[0]);
}

void CLocalBrowseWindowState::SetHasShortcut(bool hasShortcut)
{
  m_bHasShortcut = hasShortcut;
}

bool CLocalBrowseWindowState::UpdateHasShortcut(const CStdString& strCommand)
{
  // check if item has shortcut
  CBoxeeShortcut cut;
  CStdString command;
  CStdString str = strCommand;
  command.Format(str.c_str(), _P(GetCurrentPath()));
  cut.SetCommand(command);

  if (g_settings.GetShortcuts().HasShortcutByCommand(command))
  {
    SetHasShortcut(true);
    return true;
  }
  else
  {
    SetHasShortcut(false);
    return false;
  }
}

CStdString CLocalBrowseWindowState::CreatePath()
{
  return CBrowseWindowState::CreatePath();
}

void CLocalBrowseWindowState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  CBrowseWindowState::OnPathChanged(strPath, bResetSelected);

  CLog::Log(LOGDEBUG,"CLocalBrowseWindowState::OnPathChanged, path = %s (checkpath)", strPath.c_str());
  ((CGUIWindowBoxeeBrowseLocal*)m_pWindow)->UpdateButtonState();
}

bool CLocalBrowseWindowState::HasShortcut()
{
  return m_bHasShortcut;
}

bool CLocalBrowseWindowState::OnShortcut(const CStdString& strCommand)
{
  CStdString currentPath = _P(GetCurrentPath());
  CBoxeeShortcut cut;
  cut.SetName(currentPath);

  CStdString command;
  CStdString str = strCommand;
  command.Format(str.c_str(), currentPath);
  cut.SetCommand(command);

  cut.SetThumbPath("defaultfolder.png");
  cut.SetCountry("all");
  cut.SetCountryAllow(true);
  cut.SetReadOnly(false);

  if (!HasShortcut())
  {
    // add to shortcut
    if (g_settings.GetShortcuts().AddShortcut(cut))
    {
      SetHasShortcut(true);
    }
  }
  else
  {
    // remove from shortcut
    if (g_settings.GetShortcuts().RemoveShortcut(cut))
    {
      SetHasShortcut(false);
    }
  }

  return HasShortcut();
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseLocal::CGUIWindowBoxeeBrowseLocal()
: CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_LOCAL, "boxee_browse_local.xml")
{
  SetWindowState(new CLocalBrowseWindowState(this));
}

CGUIWindowBoxeeBrowseLocal::~CGUIWindowBoxeeBrowseLocal()
{

}

void CGUIWindowBoxeeBrowseLocal::OnInitWindow()
{
  m_iLastControl = 59;
  CGUIWindowBoxeeBrowse::OnInitWindow();

  UpdateShortcutButton();
  UpdateButtonState();

  CONTROL_DISABLE(BUTTON_SHORTCUT);
}

bool CGUIWindowBoxeeBrowseLocal::OnBind(CGUIMessage& message)
{
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    UpdateShortcutButton();
    CONTROL_ENABLE(BUTTON_SHORTCUT);
  }

  return CGUIWindowBoxeeBrowse::OnBind(message);
}

void CGUIWindowBoxeeBrowseLocal::UpdateShortcutButton()
{
  CStdString label = "";

  bool bHasShortcut = ((CLocalBrowseWindowState*)m_windowState)->UpdateHasShortcut(LOCAL_WINDOW_SHORTCUT_COMMAND);

  if (bHasShortcut)
  {
    label = g_localizeStrings.Get(53716);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
  else
  {
    label = g_localizeStrings.Get(53715);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
}

bool CGUIWindowBoxeeBrowseLocal::OnMessage(CGUIMessage& message)
{
  if (ProcessPanelMessages(message)) {
    return true;
  }

  if (message.GetMessage() == GUI_MSG_NOTIFY_ALL)
  {
    if (message.GetParam1() == GUI_MSG_FILE_SCANNER_UPDATE)
    {
      CStdString strPath = message.GetStringParam();
      if (strPath == m_windowState->GetCurrentPath())
      {
        SET_CONTROL_VISIBLE(BUTTON_SCAN);
        SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
        SET_CONTROL_LABEL(BUTTON_SCAN, "SCAN FOR MEDIA");
        CONTROL_ENABLE(BUTTON_SCAN);
      }
    }

  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseLocal::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage, GUI_MSG_CLICKED, control = %d (browse)", iControl);

    if (iControl == BUTTON_ADD_SOURCE)
    {
      CGUIWindowBoxeeMediaSourceInfo *pDlgSourceInfo = (CGUIWindowBoxeeMediaSourceInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);
      if (pDlgSourceInfo)
      {
        pDlgSourceInfo->SetAddSource(m_windowState->GetCurrentPath());
      }

      g_windowManager.ActivateWindow(WINDOW_BOXEE_MEDIA_SOURCE_INFO);

      Refresh();
      return true;
    }
    else if (iControl == BUTTON_SCAN)
    {
      CStdString strPath = m_windowState->GetCurrentPath();

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::OnMessage, BUTTON_SCAN, path = %s (checkpath)", strPath.c_str());
      g_application.GetBoxeeFileScanner().AddUserPath(_P(strPath));
//      BOXEE::Boxee::GetInstance().GetMetadataEngine().MarkFolderTreeNew(_P(strPath));

      UpdateButtonState();
    }
    else if (iControl == BUTTON_SHORTCUT)
    {
      if (((CLocalBrowseWindowState*)m_windowState)->OnShortcut(LOCAL_WINDOW_SHORTCUT_COMMAND))
      {
        // shortcut was added
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53716));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53737), 5000);
      }
      else
      {
        // shortcut was removed
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53715));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53739), 5000);
      }

      return true;
    }
  } // case GUI_MSG_CLICKED
  // else - break from switch and return false
  break;
  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}

void CGUIWindowBoxeeBrowseLocal::UpdateButtonState(int iStatus)
{
  CStdString strPath = m_windowState->GetCurrentPath();

  if (iStatus == 0)
  {
    std::vector<CStdString> vecTypes;
    iStatus = g_application.GetBoxeeFileScanner().GetFolderStatus(_P(strPath), vecTypes);
  }

  switch (iStatus)
  {
  case FOLDER_STATUS_NONE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState, path = %s state = FOLDER_STATUS_NONE (checkpath) ", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_ADD_SOURCE);
    SET_CONTROL_HIDDEN(BUTTON_SCAN);
    CONTROL_DISABLE(BUTTON_ADD_SOURCE);
  }
  break;
  case FOLDER_STATUS_NOT_SHARE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState, path = %s state = FOLDER_STATUS_NOT_SHARE (checkpath) ", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_ADD_SOURCE);
    SET_CONTROL_HIDDEN(BUTTON_SCAN);
    CONTROL_ENABLE(BUTTON_ADD_SOURCE);
  }
  break;
  case FOLDER_STATUS_ON_SHARE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState, path = %s state = FOLDER_STATUS_ON_SHARE (checkpath) ", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    SET_CONTROL_LABEL(BUTTON_SCAN, "SCAN FOR MEDIA");
    CONTROL_ENABLE(BUTTON_SCAN);
  }
  break;
  case FOLDER_STATUS_PRIVATE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState, path = %s state = FOLDER_STATUS_PRIVATE (checkpath) ", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    SET_CONTROL_LABEL(BUTTON_SCAN, "SCAN FOR MEDIA");
    CONTROL_DISABLE(BUTTON_SCAN);
  }
  break;
  case FOLDER_STATUS_SCANNING:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseLocal::UpdateButtonState, path = %s state = FOLDER_STATUS_SCANNING (checkpath) ", strPath.c_str());
    SET_CONTROL_VISIBLE(BUTTON_SCAN);
    SET_CONTROL_HIDDEN(BUTTON_ADD_SOURCE);
    SET_CONTROL_LABEL(BUTTON_SCAN, "SCANNING...");
    CONTROL_DISABLE(BUTTON_SCAN);
  }
  break;
  }
}
