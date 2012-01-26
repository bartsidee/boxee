
#include "GUIWindowBoxeeBrowseApps.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIDialogBoxeeMediaAction.h"

using namespace std;
using namespace BOXEE;

#define MY_APPS_WINDOW_LABEL       9001
#define APPS_LIBRARY_WINDOW_LABEL  9002
#define MY_APPS_WINDOW_LABEL       9001


#define MENU_LIST             100
#define BTN_MY_APPS           120
#define BTN_ALL_APPS          130
#define BTN_MY_REPOSITORIES   200
#define BTN_SEARCH            160
#define BTN_APPLICATION_TYPE  170

#define STATE_SHOW_MY_APPS    1
#define STATE_SHOW_ALL_APPS   2
#define STATE_SHOW_MY_REPOSITORIES 3

// STATE IMPLEMENTATION

CAppsWindowState::CAppsWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
  m_strApplicationType = g_localizeStrings.Get(53800);
  m_iState = STATE_SHOW_MY_APPS;
  m_iPreviousState = STATE_SHOW_MY_APPS;

  SetSearchType("appbox");

  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort("last-used", SORT_METHOD_APP_LAST_USED_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(54250), ""));
  m_vecSortMethods.push_back(CBoxeeSort("usage", SORT_METHOD_APP_USAGE, SORT_ORDER_DESC, g_localizeStrings.Get(53507), ""));
  m_vecSortMethods.push_back(CBoxeeSort("title", SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), ""));

  SetSort(m_vecSortMethods[0]);
  SetApplicationType(g_localizeStrings.Get(53800));
}

void CAppsWindowState::Reset()
{
  CBrowseWindowState::Reset();

  // Reset view labels
  switch (m_iState)
  {
  case STATE_SHOW_MY_APPS:
  {
    m_pWindow->SetProperty("my-set", true);
    m_pWindow->SetProperty("all-set", false);
    m_pWindow->SetProperty("repository-set", false);
  }
  break;
  case STATE_SHOW_ALL_APPS:
  {
    m_pWindow->SetProperty("my-set", false);
    m_pWindow->SetProperty("all-set", true);
    m_pWindow->SetProperty("repository-set", false);
  }
  break;
  case STATE_SHOW_MY_REPOSITORIES:
  {
    m_pWindow->SetProperty("my-set", false);
    m_pWindow->SetProperty("all-set", false);
    m_pWindow->SetProperty("repository-set", true);
  }
  break;
  }

  CLog::Log(LOGDEBUG,"CAppsWindowState::Reset - Afer set [my-set=%s][all-set=%s][repository-set=%s] (bapps)(browse)",m_pWindow->GetProperty("my-set").c_str(),m_pWindow->GetProperty("all-set").c_str(),m_pWindow->GetProperty("repository-set").c_str());

  SetApplicationType(g_localizeStrings.Get(53800));
}

CStdString CAppsWindowState::CreatePath()
{
  CLog::Log(LOGDEBUG,"CAppsWindowState::CreatePath - Enter function. [InSearchMode=%d][m_iState=%d] (bapps)(browse)",InSearchMode(),m_iState);

  CStdString strPath;

  if (InSearchMode())
  {
    if (!m_strSearchString.IsEmpty())
    {
      strPath = "appbox://all/?search=";
      strPath += GetSearchString();
    }
  }
  else
  {
    CStdString windowLabel = "";

    switch (m_iState)
    {
    case STATE_SHOW_MY_APPS:
    {
      strPath = "apps://all/";
      ((CGUIWindowBoxeeBrowseApps*)m_pWindow)->SetWindowLabel(MY_APPS_WINDOW_LABEL,g_localizeStrings.Get(53830));
    }
    break;
    case STATE_SHOW_ALL_APPS:
    {
      strPath = "appbox://all/";
      ((CGUIWindowBoxeeBrowseApps*)m_pWindow)->SetWindowLabel(MY_APPS_WINDOW_LABEL,g_localizeStrings.Get(53831));
    }
    break;
    case STATE_SHOW_MY_REPOSITORIES:
    {
      bool ok = false;
      if (!m_configuration.m_strPath.IsEmpty())
      {
        strPath = m_configuration.m_strPath;

        CURL url(m_configuration.m_strPath);

        if (url.GetProtocol() == "appbox")
        {
          CStdString repositoryLabel = "";
          std::map<CStdString, CStdString> optionsMap = url.GetOptionsAsMap();
          std::map<CStdString, CStdString>::iterator it = optionsMap.find("label");
          if (it != optionsMap.end())
          {
            repositoryLabel = it->second;
            ((CGUIWindowBoxeeBrowseApps*)m_pWindow)->SetWindowLabel(MY_APPS_WINDOW_LABEL,repositoryLabel);
            ok = true;
          }
        }
      }

      if (!ok)
      {
        m_iState = m_iPreviousState;
        m_sort = m_savedSort;

        if (m_iState == STATE_SHOW_ALL_APPS)
        {
          strPath = "appbox://all/";
          ((CGUIWindowBoxeeBrowseApps*)m_pWindow)->SetWindowLabel(MY_APPS_WINDOW_LABEL,g_localizeStrings.Get(53831));
        }
        else
        {
          strPath = "apps://all/";
          ((CGUIWindowBoxeeBrowseApps*)m_pWindow)->SetWindowLabel(MY_APPS_WINDOW_LABEL,g_localizeStrings.Get(53830));
        }
      }
    }
    break;
    }
  }

  CLog::Log(LOGDEBUG,"CAppsWindowState::CreatePath - Going to return [path=%s]. [m_iState=%d] (bapps)(browse)", strPath.c_str(),m_iState);

  Reset();

  return strPath;
}

void CAppsWindowState::SortItems(CFileItemList &items)
{
  if (InSearchMode())
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::SortItems - Enter function [InSearchMode=TRUE]. [itemsListSize=%d] (bapps)(browse)",items.Size());

    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::SortItems - Enter function [InSearchMode=FALSE]. Going to call CBrowseWindowState::SortItems(). [itemsListSize=%d] (bapps)(browse)",items.Size());

    CBrowseWindowState::SortItems(items);
  }
}

bool CAppsWindowState::OnBack()
{
  CLog::Log(LOGDEBUG,"CAppsWindowState::OnBack - Enter function Going to call OnSearchEnd()  (bapps)(browse)");

  if (OnSearchEnd())
  {
    return true;
  }
  else
  {
    if (m_iState == STATE_SHOW_MY_REPOSITORIES)
    {
      // Activate the repositories window
      OnRepositories();
      Reset();

      m_iState = m_iPreviousState;
      m_sort = m_savedSort;

      return true;
    }
  }

  return false;
}

bool CAppsWindowState::OnMyApps()
{
  if (m_iState == STATE_SHOW_MY_APPS)
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::OnMyApps - [m_iState=%d] -> Going to return FALSE (bapps)(browse)",m_iState);
    return false;
  }

  m_iPreviousState = m_iState;
  m_iState = STATE_SHOW_MY_APPS;

  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort("last-used", SORT_METHOD_APP_LAST_USED_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(54250), ""));
  m_vecSortMethods.push_back(CBoxeeSort("usage", SORT_METHOD_APP_USAGE, SORT_ORDER_DESC, g_localizeStrings.Get(53507), ""));
  m_vecSortMethods.push_back(CBoxeeSort("title", SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), ""));
  SetSort(m_vecSortMethods[0]);

  CLog::Log(LOGDEBUG,"CAppsWindowState::OnMyApps - After set [m_iState=%d] (bapps)(browse)",m_iState);

  Reset();

  return true;
}

bool CAppsWindowState::OnAllApps()
{
  if (m_iState == STATE_SHOW_ALL_APPS)
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::OnAllApps - [m_iState=%d] -> Going to return FALSE (bapps)(browse)",m_iState);
    return false;
  }

  m_iPreviousState = m_iState;
  m_iState = STATE_SHOW_ALL_APPS;

  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort("popular", SORT_METHOD_APP_POPULARITY, SORT_ORDER_DESC, g_localizeStrings.Get(53504), ""));
  m_vecSortMethods.push_back(CBoxeeSort("release", SORT_METHOD_APP_RELEASE_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53506), ""));
  m_vecSortMethods.push_back(CBoxeeSort("title", SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), ""));
  SetSort(m_vecSortMethods[0]);

  CLog::Log(LOGDEBUG,"CAppsWindowState::OnAllApps - After set [m_iState=%d] (bapps)(browse)",m_iState);

  Reset();

  return true;
}

bool CAppsWindowState::OnRepositories()
{
  CLog::Log(LOGDEBUG,"CAppsWindowState::OnRepositories - Enter function. Going to open WINDOW_BOXEE_BROWSE_REPOSITORIES (bapps)");

  if (m_iState != STATE_SHOW_MY_REPOSITORIES)
  {
    m_iPreviousState = m_iState;
    m_savedSort = m_sort;
  }

  m_iState = STATE_SHOW_MY_REPOSITORIES;

  SetSort(m_vecSortMethods[2]);

  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_REPOSITORIES);

  return true;
}

void CAppsWindowState::SetApplicationType(const CStdString& strType)
{
  m_configuration.ClearActiveFilters();
  m_pWindow->SetProperty("type-label", strType);
  m_strApplicationType = strType;

  if (strType == g_localizeStrings.Get(53800))
  {
    // Show all applications
    CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=ALL] (bapps)",strType.c_str());

    m_configuration.AddActiveFilter(FILTER_ALL);
    m_pWindow->SetProperty("type-set", false);
  }
  else
  {
    // Specific type was selected
    m_pWindow->SetProperty("type-set", true);
    if (strType == g_localizeStrings.Get(53801))
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=VIDEO] (bapps)",strType.c_str());

      m_configuration.AddActiveFilter(FILTER_APPS_VIDEO);
    }
    else if (strType == g_localizeStrings.Get(53802))
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=MUSIC] (bapps)",strType.c_str());

      m_configuration.AddActiveFilter(FILTER_APPS_MUSIC);
    }
    else if (strType == g_localizeStrings.Get(53803))
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=PICTURES] (bapps)",strType.c_str());

      m_configuration.AddActiveFilter(FILTER_APPS_PICTURES);
    }
    else if (strType == g_localizeStrings.Get(53804))
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=GENERAL] (bapps)",strType.c_str());

      m_configuration.AddActiveFilter(FILTER_APPS_GENERAL);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s] which is UNKNOWN (bapps)",strType.c_str());
    }
  }
}

void CAppsWindowState::SetState(int state)
{
  m_iState = state;
}

int CAppsWindowState::GetState()
{
  return m_iState;
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseApps::CGUIWindowBoxeeBrowseApps() : CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_APPS,"boxee_browse_apps.xml")
{
  SetWindowState(new CAppsWindowState(this));
}

CGUIWindowBoxeeBrowseApps::~CGUIWindowBoxeeBrowseApps()
{

}

void CGUIWindowBoxeeBrowseApps::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnInitWindow - Enter function (bapps)(browse)");

  ((CAppsWindowState*)m_windowState)->Reset();

  CGUIWindowBoxeeBrowseWithPanel::OnInitWindow();

  m_itemWasLaunch = false;
}

void CGUIWindowBoxeeBrowseApps::OnDeinitWindow(int nextWindowID)
{
  CGUIWindowBoxeeBrowseWithPanel::OnDeinitWindow(nextWindowID);

  if (!m_itemWasLaunch && (((CAppsWindowState*)m_windowState)->GetState() != STATE_SHOW_MY_REPOSITORIES))
  {
    ((CAppsWindowState*)m_windowState)->SetState(STATE_SHOW_MY_APPS);
  }
}

bool CGUIWindowBoxeeBrowseApps::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnMessage - GUI_MSG_CLICKED - [control=%d] (bapps)(browse)", iControl);

    if (iControl == BTN_MY_APPS)
    {
      ResetHistory();

      if (((CAppsWindowState*)m_windowState)->OnMyApps())
      {
        Refresh();
      }

      return true;
    }
    else if (iControl == BTN_ALL_APPS)
    {
      ResetHistory();

      if (((CAppsWindowState*)m_windowState)->OnAllApps())
      {
        Refresh();
      }

      return true;
    }
    else if (iControl == BTN_MY_REPOSITORIES)
    {
      ResetHistory();

      ((CAppsWindowState*)m_windowState)->OnRepositories();

      return true;
    }
    else if (iControl == BTN_SEARCH)
    {
      if (m_windowState->OnSearchStart())
      {
        ClearView();
        SET_CONTROL_FOCUS(9000,0);

        Refresh(true);
      }

      return true;
    }
    else if (iControl == BTN_APPLICATION_TYPE)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnMessage - GUI_MSG_CLICKED - [control=%d=BTN_APPLICATION_TYPE] - Going to call HandleBtnApplicationType (bapps)(browse)", iControl);

      return HandleBtnApplicationType();
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED

  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}

bool CGUIWindowBoxeeBrowseApps::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (m_windowState->OnBack())
    {
      Refresh();
      return true;
    }
    else
    {
      CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);
      pMenu->DoModal();
      return true;
    }
  }
  };

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseApps::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnClick - Enter function with [item=%d] (bapps)(browse)", iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
  {
    return true;
  }

  item.Dump();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnClick - Going to open MediaAction with item [label=%s][path=%s] (bapps)(browse)", item.GetLabel().c_str(),item.m_strPath.c_str());

  m_itemWasLaunch = true;

  return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
}

bool CGUIWindowBoxeeBrowseApps::HandleBtnApplicationType()
{
  CFileItemList applicationTypes;
  FillDropdownWithApplicationTypes(applicationTypes);

  CStdString value;
  if (CGUIDialogBoxeeDropdown::Show(applicationTypes, g_localizeStrings.Get(53563), value))
  {
    ((CAppsWindowState*)m_windowState)->SetApplicationType(value);

    UpdateFileList();
    return true;
  }

  return false;
}

bool CGUIWindowBoxeeBrowseApps::FillDropdownWithApplicationTypes(CFileItemList& applicationTypes)
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

void CGUIWindowBoxeeBrowseApps::SetWindowLabel(int controlId, const CStdString windowLabel)
{
  SET_CONTROL_LABEL(controlId,windowLabel);
}

bool CGUIWindowBoxeeBrowseApps::IsInMyAppsState()
{
  return (((CAppsWindowState*)m_windowState)->GetState() == STATE_SHOW_MY_APPS);
}

void CGUIWindowBoxeeBrowseApps::FromRepositories()
{
  (((CAppsWindowState*)m_windowState)->SetState(STATE_SHOW_MY_REPOSITORIES));
}

