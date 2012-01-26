
#include "GUIWindowBoxeeBrowseAppBox.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "AppManager.h"
#include "BoxeeUtils.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogBoxeeDropdown.h"
#include "boxee.h"
#include "GUIDialogBoxeeSearch.h"
#include "GUIDialogBoxeeMainMenu.h"

using namespace std;
using namespace BOXEE;

#define BTN_APPLICATION_TYPE  170
#define MENU_MY_APPS          111
#define MENU_SORT             110
#define MENU_ALL_APPS         130
#define MENU_REPOSITORIES     150
#define MENU_SEARCH           160

// STATE IMPLEMENTATION

CAppBoxWindowState::CAppBoxWindowState(CGUIWindowBoxeeBrowse* pWindow) : CAppsWindowState(pWindow)
{
  m_strSearchType = "appbox";
}

CBrowseWindowState* CAppBoxWindowState::Clone()
{
  return new CAppBoxWindowState(m_pWindow);
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseAppBox::CGUIWindowBoxeeBrowseAppBox() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_APPBOX, "boxee_browse_appbox.xml")
{
  SetWindowState(new CAppBoxWindowState(this));
}

CGUIWindowBoxeeBrowseAppBox::~CGUIWindowBoxeeBrowseAppBox()
{
}



bool CGUIWindowBoxeeBrowseAppBox::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (((CAppBoxWindowState*)m_windowState)->OnBack())
    {
      EnableAllControls();
      Refresh();
      return true;
    }
    else
    {
      // HHH: Should it be called here?
      //((CAppBoxWindowState*)m_windowState)->SaveWindowStateToDB();

      // Open a main menu
      CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);

      if (pMenu)
      {
        pMenu->DoModal();
        return true;
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseAppBox::OnAction - FAILED to get WINDOW_BOXEE_DIALOG_MAIN_MENU. [ActionId=%d] (bapps)",action.id);
      }
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

void CGUIWindowBoxeeBrowseAppBox::OnInitWindow()
{
  m_windowState->ResetDefaults();

  CGUIWindowBoxeeBrowse::OnInitWindow();
  SetProperty("appbox",true);
}

void CGUIWindowBoxeeBrowseAppBox::EnableAllControls()
{
  CONTROL_ENABLE(MENU_MY_APPS);
  CONTROL_ENABLE(MENU_ALL_APPS);
  CONTROL_ENABLE(MENU_SORT);
  CONTROL_ENABLE(MENU_REPOSITORIES);
  CONTROL_ENABLE(BTN_APPLICATION_TYPE);
}

void CGUIWindowBoxeeBrowseAppBox::DisableAllControls()
{
  CONTROL_DISABLE(MENU_MY_APPS);
  CONTROL_DISABLE(MENU_ALL_APPS);
  CONTROL_DISABLE(MENU_SORT);
  CONTROL_DISABLE(MENU_REPOSITORIES);
  CONTROL_DISABLE(BTN_APPLICATION_TYPE);
}

bool CGUIWindowBoxeeBrowseAppBox::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_WINDOW_INIT:
  {
    CStdString path = message.GetStringParam();

    CStdString windowTitle = g_localizeStrings.Get(53830);

    if (path.IsEmpty())
    {
      windowTitle += "Boxee";
    }
    else
    {
      CURI url(path);
      windowTitle += url.GetShareName();
    }

    SetProperty("windowtitle", windowTitle);
  }
  break;
  }// switch

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

/*
bool CGUIWindowBoxeeBrowseAppBox::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {

  int iControl = message.GetSenderId();


  switch (iControl)
  {
  case BTN_APPLICATION_TYPE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::ProcessMenuClick - Handling click on [%s=%d=BTN_APPLICATION_TYPE] (bapps)",CGUIWindowBoxeeBrowseAppBox::ControlIdAsString(iControl),iControl);

    return HandleBtnApplicationType();
  }
  break;
  case MENU_REPOSITORIES:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::ProcessMenuClick - Handling click on [%s=%d=MENU_REPOSITORIES] (bapps)",CGUIWindowBoxeeBrowseAppBox::ControlIdAsString(iControl),iControl);

    return HandleMenuRepositories();
  }
  break;
  case MENU_MY_APPS:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::ProcessMenuClick - Handling click on [%s=%d=MENU_MY_APPS] (bapps)",CGUIWindowBoxeeBrowseAppBox::ControlIdAsString(iControl),iControl);

    return HandleMenuMyApps();
  }
  break;
  case MENU_SEARCH:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::ProcessMenuClick - Handling click on [%s=%d=MENU_SEARCH] (bapps)",CGUIWindowBoxeeBrowseAppBox::ControlIdAsString(iControl),iControl);

    return HandleMenuSearch();
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }// switch

}
  }

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}
*/

bool CGUIWindowBoxeeBrowseAppBox::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::OnClick - Enter function with [iItem=%d] (bapps)",iItem);

  CFileItem item;

  if (!GetClickedItem(iItem,item))
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseAppBox::OnClick - FAILED to get FileItem for [iItem=%d] (bapps)",iItem);
    return false;
  }

  bool isInUserApplications = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInUserApplications(item.GetProperty("appid"));

  if (isInUserApplications)
  {
    CStdString message;
    CStdString str = g_localizeStrings.Get(52090);
    message.Format(str.c_str(), item.GetLabel());

    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(52039), message);
  }
  else
  {
  CStdString heading = g_localizeStrings.Get(52039);
  CStdString line;
  CStdString str = g_localizeStrings.Get(52038);
  line.Format(str.c_str(), item.GetLabel());

  if (CGUIDialogYesNo2::ShowAndGetInput(heading, line))
  {
    bool succeeded = false;
    CStdString message;
    CURI url(item.m_strPath);
    if (url.GetProtocol() == "app")
    {
      InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), true, false);
      if (CUtil::RunInBG(job) == JOB_SUCCEEDED)
      {
        message = g_localizeStrings.Get(52016);
        succeeded = true;
      }
      else
      {
        CStdString errorStr = g_localizeStrings.Get(52017);
        message.Format(errorStr.c_str(), item.GetLabel());
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseAppBox::OnClick - FAILED to install app [label=%s]. Invalid path [path=%s] (bapps)",item.GetLabel().c_str(),item.m_strPath.c_str());

      CStdString errorStr = g_localizeStrings.Get(52017);
      message.Format(errorStr.c_str(), item.GetLabel());
    }

      //Refresh();

    if (succeeded)
    {
      g_application.m_guiDialogKaiToast.QueueNotification("", "","Application was installed", 5000);
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(52039), message);
    }
  }
  }

  return true;
}

bool CGUIWindowBoxeeBrowseAppBox::HandleBtnApplicationType()
{
  CFileItemList applicationTypes;
  FillDropdownWithApplicationTypes(applicationTypes);

  CStdString value;
  CGUIDialogBoxeeDropdown dialog; // TODO: Is some sort of initialization is needed here?
  if (dialog.Show(applicationTypes, g_localizeStrings.Get(53563), value))
  {
    //((CAppBoxWindowState*)m_windowState)->SetApplicationType(value);

    Refresh();
    return true;
    }

  return false;
}

bool CGUIWindowBoxeeBrowseAppBox::FillDropdownWithApplicationTypes(CFileItemList& applicationTypes)
{
  CFileItemPtr appTypeAll (new CFileItem(g_localizeStrings.Get(53800)));
  appTypeAll->SetProperty("type", "apps-type");
  appTypeAll->SetProperty("value", g_localizeStrings.Get(53800));
  applicationTypes.Add(appTypeAll);

  CFileItemPtr appTypeVideo (new CFileItem(g_localizeStrings.Get(53801)));
  appTypeVideo->SetProperty("type", "apps-type");
  appTypeVideo->SetProperty("value", g_localizeStrings.Get(53801));
  applicationTypes.Add(appTypeVideo);

  CFileItemPtr appTypeMusic (new CFileItem(g_localizeStrings.Get(53802)));
  appTypeMusic->SetProperty("type", "apps-type");
  appTypeMusic->SetProperty("value", g_localizeStrings.Get(53802));
  applicationTypes.Add(appTypeMusic);

  CFileItemPtr appTypePictures (new CFileItem(g_localizeStrings.Get(53803)));
  appTypePictures->SetProperty("type", "apps-type");
  appTypePictures->SetProperty("value", g_localizeStrings.Get(53803));
  applicationTypes.Add(appTypePictures);

  CFileItemPtr appTypeGeneral (new CFileItem(g_localizeStrings.Get(53804)));
  appTypeGeneral->SetProperty("type", "apps-type");
  appTypeGeneral->SetProperty("value", g_localizeStrings.Get(53804));
  applicationTypes.Add(appTypeGeneral);

  return true;
}

//bool CGUIWindowBoxeeBrowseAppBox::HandleMenuSort()
//{
//  CFileItemList sorts;
//
//  CFileItemPtr popularSortItem (new CFileItem(g_localizeStrings.Get(53504)));
//  popularSortItem->SetProperty("type", "sort");
//  popularSortItem->SetProperty("value", SORT_POPULAR);
//  sorts.Add(popularSortItem);
//
//  CFileItemPtr atozSortItem (new CFileItem(g_localizeStrings.Get(53505)));
//  atozSortItem->SetProperty("type", "sort");
//  atozSortItem->SetProperty("value", SORT_ATOZ);
//  sorts.Add(atozSortItem);
//
//  CFileItemPtr dateSortItem (new CFileItem(g_localizeStrings.Get(53506)));
//  dateSortItem->SetProperty("type", "sort");
//  dateSortItem->SetProperty("value", SORT_APP_RELEASE_DATE);
//  sorts.Add(dateSortItem);
//
//  CStdString value;
//  if (CGUIDialogBoxeeDropdown::Show(sorts, g_localizeStrings.Get(53560), value))
//  {
//    ((CAppBoxWindowState*)m_windowState)->SetSort(atoi(value));
//    Refresh(true);
//  }
//
//  return true;
//}

bool CGUIWindowBoxeeBrowseAppBox::HandleMenuRepositories()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::HandleMenuRepositories - Enter function. Going to open WINDOW_BOXEE_BROWSE_REPOSITORIES (bapps)");

  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_REPOSITORIES);

  return true;
}

bool CGUIWindowBoxeeBrowseAppBox::HandleMenuMyApps()
{
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS, "apps://all");

  return true;
}

bool CGUIWindowBoxeeBrowseAppBox::HandleMenuSearch()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAppBox::HandleMenuSearch - Enter function. [VecViewItemsSize=%d] (bapps)",m_vecViewItems.Size());

  SetProperty("searching", true);

  /*
  if (((CAppBoxWindowState*)m_windowState)->OnSearch())
  {
    ClearView();
    SET_CONTROL_FOCUS(50,0);
    DisableAllControls();
    Refresh(true);
    SetProperty("searching", false);
    SetProperty("search-set", true);
  }
  else
  {
    SetProperty("searching", false);
  }
  */

  return true;
  }

const char* CGUIWindowBoxeeBrowseAppBox::ControlIdAsString(int controlId)
{
  switch(controlId)
  {
  case BTN_APPLICATION_TYPE:
    return "BTN_APPLICATION_TYPE";
  case MENU_SORT:
    return "MENU_SORT";
  case MENU_REPOSITORIES:
    return "MENU_REPOSITORIES";
  case MENU_MY_APPS:
    return "MENU_MY_APPS";
  case MENU_ALL_APPS:
    return "MENU_ALL_APPS";
  case MENU_SEARCH:
    return "MENU_SEARCH";
  default:
    return "UNKNOWN";
  }
}

