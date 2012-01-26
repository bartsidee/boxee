
#include "GUIWindowBoxeeBrowseRepositories.h"
#include "GUIDialogKeyboard.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "GUIWindowManager.h"
#include "AppManager.h"
#include "BoxeeUtils.h"
#include "GUIWindowBoxeeBrowseApps.h"

#include "utils/log.h"
#include "LocalizeStrings.h"
#include "Util.h"

using namespace std;
using namespace BOXEE;

#define MENU_LIST      100
#define MENU_ALL       571
#define MENU_ADD       120
#define MENU_MANAGE    130
#define LIST_CONTROL_ID    56

#define ADD_REPOSITORIES 7092

#define SORT_ATOZ 1

#define INIT_SELECT_POS_IN_BROWSE_MENU 5

CRepositoriesWindowState::CRepositoriesWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.SetNewSource(new CBrowseWindowSource("repositoriessource", "repositories://external", pWindow->GetID()));
}

CGUIWindowBoxeeBrowseRepositories::CGUIWindowBoxeeBrowseRepositories() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_REPOSITORIES,"boxee_browse_repositories.xml")
{
  SetWindowState(new CRepositoriesWindowState(this));
  m_manageButtonOn = false;
}

CGUIWindowBoxeeBrowseRepositories::~CGUIWindowBoxeeBrowseRepositories()
{
}

bool CGUIWindowBoxeeBrowseRepositories::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  break;
  case GUI_MSG_CLICKED:
  {
    if (ProcessMenuClick(message))
    {
      // in case we handle the message -> return
      return true;
    }
  }
  break;

  }// switch

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

void CGUIWindowBoxeeBrowseRepositories::OnInitWindow()
{
  m_manageButtonOn = false;
  CGUIWindowBoxeeBrowse::OnInitWindow();
}

bool CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick - Enter function with [iControl=%d] (bapps)",iControl);

  switch (iControl)
  {
  case MENU_ALL:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick - Handling click on [%s=%d=MENU_ALL] (bapps)",CGUIWindowBoxeeBrowseRepositories::ControlIdAsString(iControl),iControl);

    return HandleMenuAll();
  }
  break;
  case MENU_ADD:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick - Handling click on [%s=%d=MENU_ADD] (bapps)",CGUIWindowBoxeeBrowseRepositories::ControlIdAsString(iControl),iControl);

    return HandleMenuAdd();
  }
  break;
  case ADD_REPOSITORIES:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick - Handling click on [%s=%d=MENU_ADD] (bapps)",CGUIWindowBoxeeBrowseRepositories::ControlIdAsString(iControl),iControl);

    return HandleMenuAdd();
  }
  break;

  case MENU_MANAGE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::ProcessMenuClick - Handling click on [%s=%d=MENU_MANAGE] (bapps)",CGUIWindowBoxeeBrowseRepositories::ControlIdAsString(iControl),iControl);

    return HandleMenuManage();
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }// switch

  return false;
}

bool CGUIWindowBoxeeBrowseRepositories::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::OnClick - Enter function with [iItem=%d]. [m_manageButtonOn=%d] (bapps)",iItem,m_manageButtonOn);

  CFileItem item;

  if (!GetClickedItem(iItem,item))
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseRepositories::OnClick - FAILED to get FileItem for [iItem=%d] (bapps)",iItem);
    return false;
  }

  if(!m_manageButtonOn)
  {
    if (item.GetProperty("id") == "tv.boxee")
    {
      CGUIDialogOK2::ShowAndGetInput(53600, 53611);
      return true;
    }
    else
    {
      if (item.GetProperty("id").IsEmpty())
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseRepositories::OnClick - FAILED to get RepositoryId in order to browse it (bapps)");
        return false;
      }

      CStdString strId = item.GetProperty("id");
      CStdString strLabel = item.GetLabel();

      CUtil::URLEncode(strId);
      CUtil::URLEncode(strLabel);

      CStdString path = "boxeeui://apps/?";
      path += "repository=";
      path += strId;
      path += "&label=";
      path += strLabel;
      path += "&sort=title";

      //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::OnClick - [m_manageButtonOn=%d] -> Going to ActivateWindow WINDOW_BOXEE_BROWSE_APPBOX with [strPath=%s]. RepositoryItem [label=%s][id=%s][repo-url=%s] (bapps)",m_manageButtonOn,path.c_str(),item.GetLabel().c_str(),item.GetProperty("id").c_str(),item.GetProperty("repo-url").c_str());

      //CGUIWindowBoxeeBrowseApps* pWindow = (CGUIWindowBoxeeBrowseApps*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_APPS);
      //pWindow->FromRepositories();

      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS, path);

      return true;
    }
  }
  else
  {
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::OnClick - Click on repository [name=%s][id=%s] (bapps)",item.GetLabel().c_str(),item.GetProperty("id").c_str());

  if (item.GetProperty("id") == "tv.boxee")
  {
    CGUIDialogOK2::ShowAndGetInput(53600, 53610);
    return true;
  }

  CStdString heading = g_localizeStrings.Get(117);
  CStdString line;
  CStdString str = g_localizeStrings.Get(52037);
  line.Format(str.c_str(), item.GetLabel());

  if (CGUIDialogYesNo2::ShowAndGetInput(heading, line))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::OnClick - Going to remove repository [name=%s][id=%s] (bapps)",item.GetLabel().c_str(),item.GetProperty("id").c_str());

      CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      if (progress)
      {
        progress->StartModal();
        progress->Progress();
      }

      CAppManager::GetInstance().GetRepositories().Delete(item.GetProperty("id"));
      Refresh();

      if (progress)
      {
        progress->Close();
      }
  }

  return true;
}
}

bool CGUIWindowBoxeeBrowseRepositories::HandleMenuAll()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::HandleMenuAll - Going to set FILTER_ALL (bapps)");

  Refresh();

  return true;
}

bool CGUIWindowBoxeeBrowseRepositories::HandleMenuAdd()
{
  CStdString repositoryUrl;
  m_manageButtonOn = false;

  if (CGUIDialogKeyboard::ShowAndGetInput(repositoryUrl, g_localizeStrings.Get(52035), false))
  {
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    CAppRepository repo(repositoryUrl);

    if (progress)
    {
      progress->Close();
    }

    if (repo.IsValid())
    {
      bool succeeded = CAppManager::GetInstance().GetRepositories().Add(repo);

      if (succeeded)
      {
        // Report to the server about the added repository
        BoxeeUtils::ReportInstallRepository(repo);

        // update the screen
        Refresh();
      }
    }
    else
    {
      CGUIDialogOK2::ShowAndGetInput(257, 52036);
    }

    return true;
  }

  return true;
}

void CGUIWindowBoxeeBrowseRepositories::ShowItems(CFileItemList& list, bool append)
{
  CGUIWindowBoxeeBrowse::ShowItems(list,append);

  if (m_vecViewItems.Size() == 1)
  {
    CGUIMessage msg(GUI_MSG_SETFOCUS, GetID(), LIST_CONTROL_ID, 0);
    g_windowManager.SendMessage(msg);
  }
}

bool CGUIWindowBoxeeBrowseRepositories::HandleMenuManage()
{
  m_manageButtonOn = !m_manageButtonOn;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::HandleMenuManage - After set [m_manageButtonOn=%d] (bapps)",m_manageButtonOn);

  return true;
}

const char* CGUIWindowBoxeeBrowseRepositories::ControlIdAsString(int controlId)
{
  switch(controlId)
  {
  case MENU_ALL:
    return "MENU_ALL";
  case MENU_ADD:
    return "MENU_ADD";
  case MENU_MANAGE:
    return "MENU_MANAGE";
  default:
    return "UNKNOWN";
  }
}

void CGUIWindowBoxeeBrowseRepositories::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_library_repository_apps",browseMenuLevelList);

  m_initSelectPosInBrowseMenu = INIT_SELECT_POS_IN_BROWSE_MENU;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseRepositories::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu][m_initSelectPosInBrowseMenu=%d]. [category=%s] (bm)",browseMenuLevelList.size(),m_initSelectPosInBrowseMenu,category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);
}

