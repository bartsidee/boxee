
#include "utils/log.h"
#include "GUIWindowBoxeeMain.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "BoxeeVersionUpdateManager.h"
#include "GUIDialogBoxeeShortcutAction.h"
#include "GUILoaderWindow.h"
#include "GUIDialogYesNo2.h"
#include "GUIWindowManager.h"
#include "LocalizeStrings.h"
#include "GUIDialogBoxeeGlobalSearch.h"
#include "URL.h"
#include "BoxeeUtils.h"
#include "bxutils.h"
#include "BoxeeItemLauncher.h"
#include "GUIDialogBoxeeBrowseMenu.h"
#include "GUIUserMessages.h"
#include "GUIDialogYesNo.h"
#include "Util.h"
#include "GUIListContainer.h"
#include "GUIDialogFirstTimeUseMenuCust.h"
#include "xbmc/cores/dvb/dvbmanager.h"
#include "GUISettings.h"
#include "BoxeeOTAConfigurationManager.h"
#include "GUIWebDialog.h"
#include "GUIDialogOK2.h"
#include "GUIDialogBoxeeLiveTvScan.h"
#include "GUIWindowBoxeeBrowseQueue.h"
#include "GUIWindowBoxeeBrowseDiscover.h"
#include "GUIDialogBoxeeOTANoChannels.h"
#include "GUIDialogBoxeeOTAConfiguration.h"

#ifndef _WIN32
#include <boost/foreach.hpp>
#endif

#define BTN_LIST    7000

#define BTN_QUEUE     111
#define BTN_FRIENDS   112
#define BTN_TVSHOWS   113
#define BTN_LIVETV    114
#define BTN_MOVIES    115
#define BTN_APPS      116
#define BTN_FILES     117
#define BTN_DVD       118
#define BTN_DVB       119
#define BTN_WEB       120
#define TEMP_BTN_WEB_DLG 150

#define BTN_NEW_VERSION       2410

using namespace BOXEE;

CGUIWindowBoxeeMain::CGUIWindowBoxeeMain(void) : CGUILoaderWindow(WINDOW_HOME, "Home.xml")
{
  ResetControlStates();
}

CGUIWindowBoxeeMain::~CGUIWindowBoxeeMain(void)
{
}

void CGUIWindowBoxeeMain::OnInitWindow()
{
  CGUILoaderWindow::OnInitWindow();

  SetProperty("item-summary", g_localizeStrings.Get(10000));
  SetUserName(true);

  if (GetFocusedControlID() == 0)
  {
    SET_CONTROL_FOCUS(BTN_LIST, 3 /* Shows */);
  }
}

void CGUIWindowBoxeeMain::OnDeinitWindow(int nextWindowID)
{
  CGUILoaderWindow::OnDeinitWindow(nextWindowID);

  SetProperty("user-name", "");
}

bool CGUIWindowBoxeeMain::OnAction(const CAction& action)
{
  switch (action.id)
  {
    case ACTION_MOUSE:
    case ACTION_MOVE_LEFT:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_UP:
    case ACTION_MOVE_DOWN:
    {
      // In case of move in WINDOW_HOME we want to cancel the HomeScreenTimer
      g_application.SetHomeScreenTimerOnStatus(false);
    }
    break;
    
    case ACTION_SHOW_GUI:
    {
      return true;
    }
    break;
    
    case ACTION_PREVIOUS_MENU:
    case ACTION_PARENT_DIR:
    {
      // Open a main menu
      CGUIDialogBoxeeBrowseMenu* pMenu = (CGUIDialogBoxeeBrowseMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU);
      if (pMenu)
      {
        pMenu->DoModal();
        return true;
      }
    }
    break;
  }
  
  if (action.id >= KEY_ASCII)
  {
    CGUIDialogBoxeeBrowseMenu::OpenSearchWithAction(action);
    return true;
  }

  return CGUILoaderWindow::OnAction(action);
}

#ifdef HAS_DVB
bool CGUIWindowBoxeeMain::RunOnBoardingWizardIfNeeded(bool rescan)
{
  // Run the OTA wizard
  if (!g_guiSettings.GetBool("ota.run") || rescan)
  {
    ((CGUIDialogBoxeeOTAWelcome*)g_windowManager.GetWindow(WINDOW_OTA_WELCOME_CONFIGURATION))->setRescan(rescan);

    if (!CBoxeeOTAConfigurationManager::GetInstance().RunWizard(rescan))
    {
      return false;
    }

    g_guiSettings.SetBool("ota.scanned", false);
    g_settings.Save();
  }

  if (!g_guiSettings.GetBool("ota.scanned"))
  {
    CGUIDialogBoxeeLiveTvScan* pScanDialog;
    bool rescan = true;
    while (rescan)
    {
      pScanDialog = (CGUIDialogBoxeeLiveTvScan*) g_windowManager.GetWindow(WINDOW_OTA_SCANNING);
      pScanDialog->DoModal();
      if (pScanDialog->NoChannelsFound())
      {
        CGUIDialogBoxeeOTANoChannels*pNoChannelsDialog = (CGUIDialogBoxeeOTANoChannels*) g_windowManager.GetWindow(WINDOW_OTA_NO_CHANNELS);
        pNoChannelsDialog->DoModal();

        if (pNoChannelsDialog->IsSwitchConnection())
        {
          g_guiSettings.SetBool("ota.selectedcable", !g_guiSettings.GetBool("ota.selectedcable"));
          g_settings.Save();
        }
        else if (!pNoChannelsDialog->IsRescan())
        {
          rescan = false;
        }
      }
      else
      {
        rescan = false;
      }
    }

    return pScanDialog->RequestedGoToLiveTv();
  }

  return true;
}
#endif

bool CGUIWindowBoxeeMain::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() ) 
  {      
    case GUI_MSG_UPDATE:
    {
      int iControl = message.GetSenderId();

      if (iControl == 0)
      {
        CGUIMessage winmsg2(GUI_MSG_UPDATE, GetID(), LIST_FEATURES);
        OnMessage(winmsg2);
        return true;
      }
      else
      {
        switch(message.GetControlId())
        {
          case LIST_FEATURES:
          {
            CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_UPDATE - Going to reload LIST_FEATURES (home)(feat)");
            ReloadContainer(LIST_FEATURES,true);
            return true;
          }
          break;
        }
      }
    }
    break;

    case GUI_MSG_CLICKED:
    {
      // In case of click in WINDOW_HOME we want to cancel the HomeScreenTimer
      g_application.SetHomeScreenTimerOnStatus(false);
      
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      
      if (iControl == BTN_NEW_VERSION)
      {
        // If the user clicked on the new version notification
        CBoxeeVersionUpdateManager::HandleUpdateVersionButton();
      }
      else if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        switch(iControl)
        {
          case BTN_MOVIES:
          {
            g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_MOVIES,"boxeeui://movies/");
          }
          break;
          case BTN_TVSHOWS:
          {
            g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVSHOWS,"boxeeui://shows/");
          }
          break;
          case BTN_FRIENDS:
          {
            CGUIWindowBoxeeBrowseDiscover::LaunchFriends();
          }
          break;
          case BTN_QUEUE:
          {
            CGUIWindowBoxeeBrowseQueue::LaunchWatchLater();
          }
          break;
          case BTN_APPS:
          {
            g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS,"boxeeui://apps/");
          }
          break;
          case BTN_FILES:
          {
            g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL,BoxeeUtils::GetFilesButtonPathToExecute());
          }
          break;
          case BTN_WEB:
          {
            BoxeeUtils::LaunchBrowser();
          }
          break;
          case TEMP_BTN_WEB_DLG:
          {
            CGUIWebDialog::ShowAndGetInput("http://10.0.0.140/test2");
          }
          break;
#ifdef HAS_DVB
          case BTN_DVB:
          {
            if (RunOnBoardingWizardIfNeeded(false))
            {
              g_windowManager.ActivateWindow(WINDOW_BOXEE_LIVETV);
            }
            else
            {
              CLog::Log(LOGERROR, "CGUIWindowBoxeeMain::OnMessage - GUI_MSG_CLICKED - BTN_DVB - RunOnBoardingWizard FAILED");
            }
          }
          break;
#endif
#ifdef HAS_DVD_DRIVE
          case BTN_DVD:
          {
            CGUIDialogBoxeeMediaAction::OpenDvdDialog();
          }
          break;
#endif
          case LIST_FEATURES:
          {
            HandleClickInList(iControl);
          }
          break;
        }
      }
    }
    break;
    
    default:
    {
      // do nothing
    }
  } // end switch

  return CGUILoaderWindow::OnMessage(message);
}

void CGUIWindowBoxeeMain::HandleClickInList(int listType)
{
  const CGUIControl* pControl = GetControl(listType);
  if (pControl && pControl->IsContainer())
  {
    const CFileItem* pSelectedItem = (CFileItem*)((CGUIBaseContainer*)pControl)->GetListItem(0).get();

    if (pSelectedItem)
    {
      SaveControlStates();

      switch(listType)
      {
        case LIST_FEATURES:
        {
          HandleClickOnFeatureItem(*pSelectedItem);
        }
        break;
        default:
        {
          CLog::Log(LOGWARNING,"CGUIWindowBoxeeMain::HandleClickInList - No entry for [Control=%d], so item [label=%s] WON'T be handle  (home)",listType,pSelectedItem->GetLabel().c_str());
        }
        break;
      }
    }
    else
    {
      CLog::Log(LOGWARNING,"CGUIWindowBoxeeMain::HandleClickInList -  FAILED to get selected item from list [%d] (home)",listType);
    }
  }
  else
  {
    CLog::Log(LOGWARNING,"CGUIWindowBoxeeMain::HandleClickInList -  FAILED to get control for list [%d] (home)",listType);
  }
}

void CGUIWindowBoxeeMain::HandleClickOnFeatureItem(const CFileItem& item)
{
  if (item.GetPropertyBOOL("external-browse-all"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE,"feed://featured");
  }
  else
  {
    CFileItem featureItem(item);
    featureItem.Dump();
    CBoxeeItemLauncher::Launch(featureItem);
  }
}

void CGUIWindowBoxeeMain::Render()
{
  SetUserName(false);
  
  CGUILoaderWindow::Render();
}

void CGUIWindowBoxeeMain::SetUserName(bool force)
{
  if (force || (!force && g_application.IsConnectedToInternet(false) != m_lastConnectedStatus))
  {
    m_lastConnectedStatus = g_application.IsConnectedToInternet(false);
    
    CStdString userName = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getName();
    SetProperty("user-name", userName);
  }
}

