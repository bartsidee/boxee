
#include "GUIWindowBoxeeBrowseDiscover.h"
#include "GUIDialogBoxeeMakeBoxeeSocial.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowStateDatabase.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "DirectoryCache.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define THUMB_VIEW_LIST       50
#define LINE_VIEW_LIST        51

#define EMPTY_STATE_LABEL    7094


CDiscoverWindowState::CDiscoverWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.SetNewSource(new CBrowseWindowSource("discoversource", "feed://share", pWindow->GetID()));
}

void CDiscoverWindowState::SetDefaultView()
{
  m_iCurrentView = THUMB_VIEW_LIST;
}

CGUIWindowBoxeeBrowseDiscover::CGUIWindowBoxeeBrowseDiscover() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_DISCOVER, "boxee_browse_discover.xml")
{
  SetWindowState(new CDiscoverWindowState(this));

  m_hasBrowseMenu = false;
}


CGUIWindowBoxeeBrowseDiscover::~CGUIWindowBoxeeBrowseDiscover()
{
  
}

void CGUIWindowBoxeeBrowseDiscover::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();
}

void CGUIWindowBoxeeBrowseDiscover::OnDeinitWindow(int nextWindowID)
{
  CGUIWindowBoxeeBrowse::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowBoxeeBrowseDiscover::FriendsMakeBoxeeSocial()
{
  CGUIDialogBoxeeMakeBoxeeSocial* pMakeBoxeeSocial = (CGUIDialogBoxeeMakeBoxeeSocial*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MAKE_SOCIAL);
  if(pMakeBoxeeSocial)
  {
    pMakeBoxeeSocial->DoModal();

    if(pMakeBoxeeSocial->m_bIsConfirmed || !g_application.IsConnectedToInternet())
    {
      return true;
    }
  }
  return false;
}

void CGUIWindowBoxeeBrowseDiscover::LaunchFriends()
{
  int size = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetRecommendationsSize();

  if(size == 0)
  {
    CGUIWindowStateDatabase stateDB;
    CStdString isFirstTimeUser;
    bool settingExists = stateDB.GetUserSetting("firstTimeFriendsRequest", isFirstTimeUser);
    if(!settingExists)
    {
      stateDB.SetUserSetting("firstTimeFriendsRequest","true");
      isFirstTimeUser = "true";
    }
    if(isFirstTimeUser == "true")
    {
      bool isConnected = g_application.GetBoxeeSocialUtilsManager().IsAnyConnected();

      if(!isConnected)
      {
        if(!FriendsMakeBoxeeSocial())
        {
          return;
        }
      }
      stateDB.SetUserSetting("firstTimeFriendsRequest","false");
    }
  }
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_DISCOVER);
}

bool CGUIWindowBoxeeBrowseDiscover::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    OnBack();
    return true;
  }
  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseDiscover::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_UPDATE)
  {
    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseDiscover::OnMessage - GUI_MSG_UPDATE - Enter function with [SenderId=%d] (rec)(browse)",message.GetSenderId());

    if (message.GetSenderId() != GetID())
    {
      return true;
    }

    bool clearCache = message.GetParam1();

    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseDiscover::OnMessage - GUI_MSG_UPDATE - Enter function with [clearCache=%d] (rec)(browse)",clearCache);

    if (clearCache)
    {
      g_directoryCache.ClearSubPaths("feed://share/");
      g_directoryCache.ClearSubPaths("feed://recommend/");
    }
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseDiscover::HandleEmptyState()
{
  if(g_application.GetBoxeeSocialUtilsManager().IsAnyConnected())
  {
    SetProperty("paired",true);
  }
  else
  {
    SetProperty("paired",false);
  }
  return CGUIWindowBoxeeBrowse::HandleEmptyState();
}
