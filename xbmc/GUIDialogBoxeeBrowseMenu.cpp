
#include "GUIDialogBoxeeBrowseMenu.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "utils/log.h"
#include "GUIDialogBoxeeShortcutAction.h"
#include "GUIDialogYesNo2.h"
#include "BoxeeShortcut.h"
#include "GUIDialogBoxeeGlobalSearch.h"
#include "ButtonTranslator.h"
#include "LocalizeStrings.h"
#include "guilib/GUIButtonControl.h"
#include "guilib/GUIToggleButtonControl.h"
#include "guilib/GUIControlGroupList.h"
#include "guilib/GUIEditControl.h"
#include "GUIFontManager.h"
#include "SkinInfo.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIControlFactory.h"
#include "BoxeeUtils.h"
#include "URL.h"
#include "Builtins.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "FileSystem/Directory.h"
#include "FileSystem/DirectoryCache.h"
#include "GUIDialogButtonMenu.h"
#include "GUIDialogBoxeeSearch.h"
#include "GUIUserMessages.h"
#include "BoxeeVersionUpdateManager.h"
#include "GUILabelControl.h"
#include "utils/GUIInfoManager.h"
#include "GUIWindowBoxeeMain.h"
#include "GUIWindowBoxeeBrowseQueue.h"
#include "GUIWindowBoxeeBrowseDiscover.h"

#ifdef _WIN32
#include "GUISettings.h"
#endif

using namespace BOXEE;

// Static buttons for the top level of the browse menu
#define MENU_BUTTONS_LIST                         1823

#define BROWSE_MENU_BUTTON_LIVETV                 1831
#define BROWSE_MENU_BUTTON_FRIENDS                1832
#define BROWSE_MENU_BUTTON_WATCH_LATER            1833
#define BROWSE_MENU_BUTTON_TV                     1834
#define BROWSE_MENU_BUTTON_MOVIES                 1835
#define BROWSE_MENU_BUTTON_FILES                  1836
#define BROWSE_MENU_BUTTON_APPS                   1837
#define BROWSE_MENU_BUTTON_WEB                    1838

#define BTN_LOGOUT                                121
#define BTN_SETTINGS                              122
#define BTN_NOW_PLAYING                           102
#define BTN_DVB                                   1831
#define BTN_HOME                                  120
#define BTN_HISTORY                               123
#define BTN_PLAYBACK                              125

#define DOWNLOADING_LABEL                         7111

CGUIDialogBoxeeBrowseMenu::CGUIDialogBoxeeBrowseMenu(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_BROWSE_MENU, "boxee_browse_menu.xml")
{
  m_bInitialized = false;

  ResetCurrentParameters();

  ResetAction();
}

CGUIDialogBoxeeBrowseMenu::~CGUIDialogBoxeeBrowseMenu()
{
}

void CGUIDialogBoxeeBrowseMenu::OnInitWindow()
{
  if (!CanOpenBrowseMenu())
  {
    Close(true);
    return;
  }

  CGUIDialog::OnInitWindow();

  m_downloadCounter = 0;

  //CLog::Log(LOGDEBUG, "CGUIDialogBoxeeBrowseMenu::OnInitWindow - [CurrentRow=%d][CurrentButton=%d][StartActionId=%d] (browsemenu)", m_iCurrentRowIndex, m_iCurrentButtonId,m_startAction.id);

//  if (!g_application.IsPlaying() && m_iCurrentButtonId == 102)
//  {
//    m_iCurrentButtonId = BROWSE_MENU_BUTTON_HOME;
//    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), m_iCurrentButtonId);
//    OnMessage(msg);
//    m_iCurrentRowIndex = 0;
//    SetSelectedButtonInRow(0);
//  }

  if (m_openInSearch)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), BROWSE_MENU_BUTTON_SEARCH);
    OnMessage(msg);

    m_iCurrentButtonId = BROWSE_MENU_BUTTON_SEARCH;
    return;
  }

  if (m_startAction.id != 0)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), BROWSE_MENU_BUTTON_SEARCH);
    OnMessage(msg);

    m_iCurrentButtonId = BROWSE_MENU_BUTTON_SEARCH;

    CGUIEditControl* pSearchControl = (CGUIEditControl*)GetControl(BROWSE_MENU_BUTTON_SEARCH);
    if (pSearchControl)
    {
      pSearchControl->OnAction(m_startAction);
    }

    ResetAction();
  }

  int iCurrentWindow = g_windowManager.GetActiveWindow();
  bool setFocusOnButton = false;
  if ((m_iCurrentWindowId != iCurrentWindow) || (!g_application.IsPlaying() && m_iCurrentButtonId == BTN_NOW_PLAYING))
  {
    m_iCurrentWindowId = iCurrentWindow;
    setFocusOnButton = true;
    switch(iCurrentWindow)
    {
    case WINDOW_BOXEE_BROWSE_MOVIES:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_MOVIES;
    }
    break;
    case WINDOW_BOXEE_BROWSE_TVSHOWS:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_TV;
    }
    break;
    case WINDOW_BOXEE_BROWSE_APPS:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_APPS;
    }
    break;
    case WINDOW_BOXEE_BROWSE_LOCAL:
    case WINDOW_BOXEE_BROWSE_ALBUMS:
    case WINDOW_BOXEE_BROWSE_PHOTOS:
    case WINDOW_BOXEE_BROWSE_TRACKS:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_FILES;
    }
    break;
    case WINDOW_BOXEE_BROWSE_DISCOVER:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_FRIENDS;
    }
    break;
    case WINDOW_BOXEE_BROWSE_QUEUE:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_WATCH_LATER;
    }
    break;
    case WINDOW_BOXEE_LIVETV:
    {
      m_iCurrentButtonId = BROWSE_MENU_BUTTON_LIVETV;
    }
    break;
    default:
    {
      setFocusOnButton = false;
    }
    break;
    }

    if (setFocusOnButton)
    {
      CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), MENU_BUTTONS_LIST,m_iCurrentButtonId);
      OnMessage(msg);
    }
  }

}

bool CGUIDialogBoxeeBrowseMenu::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    Close();
    return true;
  }
  break;
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    // Call parent to perform the actual action
    bool bResult = CGUIDialog::OnAction(action);

    return bResult;
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  // Pass along all key presses to the edit control
  if (action.id >= KEY_ASCII)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), BROWSE_MENU_BUTTON_SEARCH);
    OnMessage(msg);

    m_iCurrentButtonId = BROWSE_MENU_BUTTON_SEARCH;

    CGUIEditControl* pSearchControl = (CGUIEditControl*)GetControl(BROWSE_MENU_BUTTON_SEARCH);
    if (pSearchControl)
    {
      pSearchControl->OnAction(action);
    }

    return true;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogBoxeeBrowseMenu::Render()
{
#ifdef HAS_EMBEDDED

  if (m_downloadCounter % 60 == 0)
  {
    m_downloadCounter = 0;

    if (g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob().GetVersionUpdateDownloadStatus() == VUDS_DOWNLOADING)
    {
      CDownloadInfo downloadnfo;
      g_boxeeVersionUpdateManager.GetDownloadInfo(downloadnfo);

      double percent = downloadnfo.m_CurrentDownloadProgress;

      CGUILabelControl* pControl = (CGUILabelControl*) GetControl(DOWNLOADING_LABEL);
      if (pControl)
      {
        CStdString percentStr = "%d%%";
        CStdString percentLabel;
        percentLabel.Format(percentStr.c_str(),(int)percent);
        pControl->SetLabel(percentLabel);
      }
    }
  }
  m_downloadCounter++;
#endif

  CGUIDialog::Render();
}

void CGUIDialogBoxeeBrowseMenu::OnClick(int iClickedButton)
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeBrowseMenu::OnClick - Enter function with [ClickedButton=%d] (browsemenu)",iClickedButton);

  switch(iClickedButton)
  {
  case BTN_LOGOUT:
  {
    CGUIDialogButtonMenu* pDialog = (CGUIDialogButtonMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_BUTTON_MENU);
    if (pDialog)
    {
      pDialog->DoModal();

      if (!pDialog->IsCanceled())
      {
        ResetCurrentParameters();
        m_iCurrentWindowId = WINDOW_LOGIN_SCREEN;
      }
    }

    return;
  }
  break;
  case BTN_SETTINGS:
  {
    ActivateWindow(WINDOW_SETTINGS_MENU);
    ResetCurrentParameters();
    return;
  }
  break;
  case BTN_NOW_PLAYING:
  {
    return;
  }
  break;
  case BROWSE_MENU_BUTTON_WEB:
  {
    BoxeeUtils::LaunchBrowser();
  }
  break;
  case BROWSE_MENU_BUTTON_FRIENDS:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_DISCOVER;
    CGUIWindowBoxeeBrowseDiscover::LaunchFriends();
  }
  break;
  case BROWSE_MENU_BUTTON_WATCH_LATER:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_QUEUE;
    CGUIWindowBoxeeBrowseQueue::LaunchWatchLater();
  }
  break;
  case BROWSE_MENU_BUTTON_TV:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_TVSHOWS;
  }
  break;

  case BROWSE_MENU_BUTTON_MOVIES:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_MOVIES;
  }
  break;
  case BROWSE_MENU_BUTTON_FILES:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_LOCAL;
    g_windowManager.ActivateWindow(m_iCurrentWindowId,BoxeeUtils::GetFilesButtonPathToExecute());
  }
  break;
  case BROWSE_MENU_BUTTON_APPS:
  {
    m_iCurrentWindowId = WINDOW_BOXEE_BROWSE_APPS;
  }
  break;
  case BTN_HOME:
  {
    m_iCurrentWindowId = WINDOW_HOME;
  }
  break;
#ifdef HAS_DVB
  case BTN_DVB:
  {
    if (CGUIWindowBoxeeMain::RunOnBoardingWizardIfNeeded(false))
    {
      m_iCurrentWindowId = WINDOW_BOXEE_LIVETV;
      g_windowManager.ActivateWindow(WINDOW_BOXEE_LIVETV);
    }
    else
    {
      CLog::Log(LOGERROR, "CGUIWindowBoxeeMain::OnMessage - GUI_MSG_CLICKED - BTN_DVB - RunOnBoardingWizard FAILED");
    }
  }
  break;
#endif
  default:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeBrowseMenu::OnClick - No case for [ClickedButton=%d] -> continue (browsemenu)",iClickedButton);
  }
  break;
  }
}

bool CGUIDialogBoxeeBrowseMenu::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  {
    CStdString param = message.GetStringParam(0);
    m_openInSearch = (param == "openinsearch");
  }
  break;
  case GUI_MSG_LABEL2_SET:
  {
    CGUIDialogBoxeeSearch *pSearchDialog = (CGUIDialogBoxeeSearch*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SEARCH);
    if (pSearchDialog && pSearchDialog->ClosedByMovingRightFromTextBox())
    {
      CAction action;
      action.id = ACTION_MOVE_RIGHT;
      OnAction(action);
    }
  }
  break;
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BROWSE_MENU_BUTTON_SEARCH)
    {
      // Get the keystroke that was pressed
      bool bResult = CGUIDialog::OnMessage(message);

      SET_CONTROL_FOCUS(BROWSE_MENU_BUTTON_SEARCH,0);

      CGUIEditControl* pSearchControl = (CGUIEditControl*)GetControl(BROWSE_MENU_BUTTON_SEARCH);
      if (pSearchControl)
      {
        CStdString strSearchTerm = pSearchControl->GetLabel2();
        //pSearchControl->SetLabel2("");

        CLog::Log(LOGDEBUG,"CGUIDialogBoxeeBrowseMenu::OnMessage - [term=%s] (search)",  strSearchTerm.c_str());

        ThreadMessage tMsg(TMSG_GUI_ACTIVATE_WINDOW , WINDOW_DIALOG_BOXEE_SEARCH , false);
        std::vector<CStdString> params;
        params.push_back(strSearchTerm);
        tMsg.params = params;
        g_application.getApplicationMessenger().SendMessage(tMsg,false);
      }

      return bResult;
    }

    OnClick(iControl);
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

CStdString CGUIDialogBoxeeBrowseMenu::GetSearchTerm()
{
  CGUIEditControl* pSearchControl = (CGUIEditControl*)GetControl(BROWSE_MENU_BUTTON_SEARCH);
  return pSearchControl->GetLabel2();
}

void CGUIDialogBoxeeBrowseMenu::ActivateWindow(unsigned int windowId, const CStdString& path, bool closeDialog)
{
  if (closeDialog)
  {
    g_windowManager.CloseDialogs(true);
  }

  m_iCurrentWindowId = windowId;
  g_windowManager.ActivateWindow(windowId,path);
}

void CGUIDialogBoxeeBrowseMenu::ResetCurrentParameters()
{
  m_iCurrentButtonId = BROWSE_MENU_BUTTON_TV;
}

void CGUIDialogBoxeeBrowseMenu::ResetAction()
{
  m_startAction.id = 0;
}

void CGUIDialogBoxeeBrowseMenu::SetAction(const CAction &action)
{
  m_startAction = action;
}

void CGUIDialogBoxeeBrowseMenu::OpenSearchWithAction(const CAction &action)
{
  CGUIDialogBoxeeBrowseMenu *pWindow = (CGUIDialogBoxeeBrowseMenu*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU);
  if (pWindow)
  {
    pWindow->SetAction(action);
    pWindow->DoModal();
  }
}

void CGUIDialogBoxeeBrowseMenu::Close(bool forceClose)
{
  return CGUIDialog::Close(forceClose);
}

bool CGUIDialogBoxeeBrowseMenu::CanOpenBrowseMenu()
{
  int activeWindow = g_windowManager.GetActiveWindow();

  if ((activeWindow == WINDOW_FULLSCREEN_VIDEO)/*during video*/ || (activeWindow == WINDOW_VISUALISATION)/*during music*/ || (activeWindow >= 10436 && activeWindow <= 10449)/*during FTU*/)
  {
    CLog::Log(LOGERROR, "CGUIDialogBoxeeBrowseMenu::CanOpenBrowseMenu - return FALSE since [activeWindow=%d] (browsemenu)",activeWindow);
    return false;
  }

  return true;
}
