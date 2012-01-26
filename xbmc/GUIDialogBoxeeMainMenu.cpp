
#include "GUIDialogBoxeeMainMenu.h"
#include "Profile.h"
#include "lib/libBoxee/bxobject.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeBrowse.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "FileSystem/cdioSupport.h"
#include "DetectDVDType.h"
#include "utils/log.h"
#include "GUIDialogBoxeeShortcutAction.h"
#include "GUIDialogYesNo2.h"
#include "BoxeeShortcut.h"
#include "GUIDialogBoxeeGlobalSearch.h"

#define BTN_PHOTOS  111
#define BTN_MUSIC   112
#define BTN_MOVIES  113
#define BTN_HOME    114
#define BTN_TVSHOWS 115
#define BTN_APPS    116
#define BTN_FILES   117

#define BTN_LOGOUT    121
#define BTN_SETTINGS  122
#define BTN_SEARCH    123
#define BTN_QUEUE     124
#define BTN_FEED      125
#define BTN_HISTORY   126
#define BTN_PLAYING_VIDEO  127
#define BTN_PLAYING_AUDIO  128
#define BTN_DVD            129

#define BTN_DOWNLOADS     8106
#define BTN_DOWNLOADS_GROUP 81061

#define UPPER_MENU_LIST  3210
#define MID_MENU_LIST    3010
#define SHORTCUT_LIST    3110

#define BTN_MANAGE       3120

CGUIDialogBoxeeMainMenu::CGUIDialogBoxeeMainMenu(void) : CGUILoaderDialog(WINDOW_BOXEE_DIALOG_MAIN_MENU, "boxee_main_menu.xml")
{
  m_manageButtonOn = false;
  m_moveShortcut = false;
}

CGUIDialogBoxeeMainMenu::~CGUIDialogBoxeeMainMenu()
{

}

bool CGUIDialogBoxeeMainMenu::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (m_manageButtonOn)
    {
      HandleShortcutManageButtonClick();
    }
    else
    {
      Close();
    }

    return true;
  }
  break;
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    if (GetFocusedControlID() == SHORTCUT_LIST && m_moveShortcut)
    {
      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetFocusedControl();

      std::vector<CBoxeeShortcut>& shortcutList = g_settings.GetShortcuts().GetItems();
      int previousItem = pControl->GetSelectedItem();

      // Handle the move event
      CGUILoaderDialog::OnAction(action);

      int currentItem = pControl->GetSelectedItem();

      if (currentItem != previousItem)
      {
        // Delete the current shortcut and insert a new one
        CBoxeeShortcut theShortcut = shortcutList[previousItem];
        shortcutList.erase(shortcutList.begin() + previousItem);
        shortcutList.insert(shortcutList.begin() + currentItem, theShortcut);
        g_settings.GetShortcuts().Save();

        // Do the same to the screen list items
        std::vector< CGUIListItemPtr >& listItems = pControl->GetItemsByRef();
        CGUIListItemPtr theScreenItem = listItems[previousItem];
        listItems.erase(listItems.begin() + previousItem);
        listItems.insert(listItems.begin() + currentItem, theScreenItem);
      }

      return true;
    }
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUILoaderDialog::OnAction(action);
}

void CGUIDialogBoxeeMainMenu::OnInitWindow()
{
  // DISABLE DOWNLOAD
#ifdef _WIN32
  SET_CONTROL_HIDDEN(BTN_DOWNLOADS_GROUP);
#else
  SET_CONTROL_HIDDEN(8111);
#endif

  m_manageButtonOn = false;
  SetProperty("manage-set",(int)m_manageButtonOn);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMainMenu::OnInitWindow - After set [m_manageButtonOn=%d]. [manage-set=%d] (mainmenu)(shortcut)",m_manageButtonOn,GetPropertyInt("manage-set"));

  m_moveShortcut = false;

  CGUILoaderDialog::OnInitWindow();

  CONTROL_SELECT_ITEM(UPPER_MENU_LIST,2);
  SET_CONTROL_FOCUS(MID_MENU_LIST,3);
}

bool CGUIDialogBoxeeMainMenu::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_INIT:
  {
	  m_manageButtonOn = false;
    m_moveShortcut = false;
	}
  break;
  case GUI_MSG_WINDOW_DEINIT:
  {
    CONTROL_SELECT_ITEM(UPPER_MENU_LIST,2);
    CONTROL_SELECT_ITEM(MID_MENU_LIST,3);
  }
  break;
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    // Close all previous dialogs first

    switch (iControl)
    {
    case BTN_PHOTOS:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_PHOTOS, "boxeedb://pictures/");
    }
    break;
    case BTN_MUSIC:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_ALBUMS);
    }
    break;
    case BTN_MOVIES:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_MOVIES);
    }
    break;
    case BTN_HOME:
    {
      ActivateWindow(WINDOW_HOME);
    }
    break;
    case BTN_TVSHOWS:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_TVSHOWS);
    }
    break;
    case BTN_APPS:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_APPS,"apps://all");
    }
    break;
    case BTN_FILES:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL,"sources://all/");
    }
    break;
    case BTN_LOGOUT:
    {
      ActivateWindow(WINDOW_DIALOG_BUTTON_MENU);
    }
    break;
    case BTN_SETTINGS:
    {
      ActivateWindow(WINDOW_SETTINGS_MENU);
    }
    break;
    case BTN_SEARCH:
    {
      CStdString searchString;
      CGUIDialogBoxeeGlobalSearch::ShowAndGetInput(searchString, "Search", true, false);
    }
    break;
    case BTN_QUEUE:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_QUEUE);
    }
    break;
    case BTN_FEED:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_DISCOVER);
    }
    break;
    case BTN_HISTORY:
    {
      ActivateWindow(WINDOW_BOXEE_BROWSE_HISTORY);
    }
    break;
    case BTN_PLAYING_VIDEO:
    {
      ActivateWindow(WINDOW_FULLSCREEN_VIDEO,"");
    }
    break;
    case BTN_PLAYING_AUDIO:
    {
      ActivateWindow(WINDOW_VISUALISATION,"");
    }
    break;
#ifdef HAS_DVD_DRIVE
    case BTN_DVD:
    {
  	  CGUIDialogBoxeeMediaAction::OpenDvdDialog();
    }
    break;
#endif
    case BTN_MANAGE:
    {
      HandleShortcutManageButtonClick();
    }
    break;
    case SHORTCUT_LIST:
    {
      HandleShortcutListClick();
    }
    break;
    }

    return true;

    //    if ((iControl == BTN_VIDEO_MOVIES) ||
    //        (iControl == BTN_VIDEO_TVSHOWS) ||
    //        (iControl == BTN_VIDEO_INTERNET) ||
    //        (iControl == BTN_VIDEO_SOURCES) ||
    //        (iControl == BTN_MUSIC_ARTISTS) ||
    //        (iControl == BTN_MUSIC_ALBUMS) ||
    //        (iControl == BTN_MUSIC_INTERNET) ||
    //        (iControl == BTN_MUSIC_SOURCES) ||
    //        (iControl == BTN_PICTURES_MYPICTURES) ||
    //        (iControl == BTN_PICTURES_INTERNET) ||
    //        (iControl == BTN_PICTURES_SOURCES) ||
    //        (iControl == BTN_DOWNLOADS) ||
    //        (iControl == BTN_SETTINGS) ||
    //        (iControl == BTN_PROFILE))
    //    {
    //      g_windowManager.CloseDialogs(true);
    //    }
    //    else if (iControl == BTN_DVD)
    //    {
    //      CFileItem dvdItem;
    //      dvdItem.m_strPath = "iso9660://";
    //      dvdItem.SetProperty("isdvd", true);
    //      dvdItem.SetThumbnailImage("defaultmusicalbum.png");
    //
    //      CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    //
    //      CStdString strDiscLabel = g_application.CurrentFileItem().GetLabel();
    //      if (pInfo)
    //        strDiscLabel = pInfo->GetDiscLabel();
    //
    //      strDiscLabel.Trim();
    //
    //      if (strDiscLabel == "")
    //        strDiscLabel = "DVD";
    //
    //      dvdItem.SetLabel(strDiscLabel);
    //
    //      Close();
    //      CGUIDialogBoxeeMediaAction::ShowAndGetInput(&dvdItem);
    //      return true;
    //    }
    //    else
    //    {
    //      // We pass the previous if's -> We are going to open the dialog SubMenu
    //      // and m_inDialogSubMenu need to be set to TRUE
    //      m_inDialogSubMenu = true;
    //    }
  }
  break;
  case GUI_MSG_FOCUSED:
  {
    int controlId = message.GetControlId();

    if (controlId == MID_MENU_LIST)
    {
      // Not in the SHORTCUT_LIST -> reset the BTN_MANAGE
      m_manageButtonOn = false;
      SetProperty("manage-set",(int)m_manageButtonOn);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMainMenu::OnMessage - GUI_MSG_FOCUSED - After set [m_manageButtonOn=%d]. [manage-set=%d] (mainmenu)(shortcut)",m_manageButtonOn,GetPropertyInt("manage-set"));
    }
  }
  break;
  }

  return CGUILoaderDialog::OnMessage(message);
}

void CGUIDialogBoxeeMainMenu::ActivateWindow(unsigned int windowId, const CStdString& path, bool closeDialog)
{
  if (closeDialog)
  {
    g_windowManager.CloseDialogs(true);
  }

  g_windowManager.ActivateWindow(windowId,path);
}

bool CGUIDialogBoxeeMainMenu::HandleShortcutListClick()
{
  if(!m_manageButtonOn)
  {
    //////////////////////////////////////////////////////////////////////////////
    // case of click on item with m_manageButtonOn=FALSE -> Launch the shortcut //
    //////////////////////////////////////////////////////////////////////////////

    CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(SHORTCUT_LIST);

    if (!pContainer)
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - FAILED to get container [%d=SHORTCUT_LIST] container (mainmenu)(shortcut)",SHORTCUT_LIST);
      return false;
    }

    CGUIListItemPtr pSelectedShortcut = pContainer->GetSelectedItemPtr();
    if (!pSelectedShortcut.get())
    {
      CLog::Log(LOGERROR,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - FAILED to get the SelectedItem from container [%d=SHORTCUT_LIST] container (mainmenu)(shortcut)",SHORTCUT_LIST);
      return false;
    }

    CFileItem* pShortcutItem = (CFileItem*)pSelectedShortcut.get();

    // need to close the dialog before launch the shortcut
    Close();

    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - [m_manageButtonOn=%d] -> Going to launch shortcut [label=%s][path=%s][shortcut-command=%s] (mainmenu)(shortcut)",m_manageButtonOn,pShortcutItem->GetLabel().c_str(),pShortcutItem->m_strPath.c_str(),pShortcutItem->GetProperty("shortcut-command").c_str());

    CBoxeeShortcut shortcut(*pShortcutItem);
    shortcut.Launch();
  }
  else
  {
    if (m_moveShortcut)
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // case of click on item with m_manageButtonOn=TRUE and m_moveShortcut=TRUE (we are in move) -> Release the item //
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

      m_moveShortcut = false;
      SetProperty("manage-set",(int)m_manageButtonOn);

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnClick - Finish moving the shortcut. [m_moveShortcut=%d][m_manageButtonOn=%d][manage-set=%d] (mainmenu)(shortcut)",m_moveShortcut,m_manageButtonOn,GetPropertyInt("manage-set"));
    }
    else
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // case of click on item with m_manageButtonOn=TRUE -> Open WINDOW_BOXEE_SHORTCUT_DIALOG around the shortcut //
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

      CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(SHORTCUT_LIST);

      if (!pContainer)
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - FAILED to get container [%d=SHORTCUT_LIST] container (mainmenu)(shortcut)",SHORTCUT_LIST);
        return false;
      }

      CGUIListItemPtr selectedShortcut = pContainer->GetSelectedItemPtr();
      if (!selectedShortcut.get())
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - FAILED to get the SelectedItem from container [%d=SHORTCUT_LIST] container (mainmenu)(shortcut)",SHORTCUT_LIST);
        return false;
      }

      CFileItem* pShortcutItem = (CFileItem*)selectedShortcut.get();

      CGUIDialogBoxeeShortcutAction* shortcutActionDialog = (CGUIDialogBoxeeShortcutAction*) g_windowManager.GetWindow(WINDOW_BOXEE_SHORTCUT_DIALOG);
      // The following is required so that SetPosition will actually work, otherwise during GUI_MSG_INIT, the XML is reloaded
      // and the position is reset to whatever is written there
      shortcutActionDialog->AllocResources();
      shortcutActionDialog->SetAllowRemove(!pShortcutItem->HasProperty("shortcut-readonly"));
      shortcutActionDialog->SetPosition(pContainer->GetFocusedPositionX()+20,pContainer->GetFocusedPositionY()+140);

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - Open SHORTCUT_DIALOG [X=%f][%f]. [m_manageButtonOn=%d] (mainmenu)(shortcut)",shortcutActionDialog->GetXPosition(),shortcutActionDialog->GetYPosition(),m_manageButtonOn);

      shortcutActionDialog->DoModal();

      if (!shortcutActionDialog->IsCancelled())
      {
        // In case remove was selected
        if (shortcutActionDialog->IsActionRemove())
        {
          // Prompt the user for confirmation
          if (CGUIDialogYesNo2::ShowAndGetInput(51925, 53310))
          {
            // Delete the shortcut
            std::vector<CBoxeeShortcut>& shortcutList = g_settings.GetShortcuts().GetItems();

            shortcutList.erase(shortcutList.begin() + pContainer->GetSelectedItem());
            g_settings.GetShortcuts().Save();

            // Delete the list item
            pContainer->GetItemsByRef().erase(pContainer->GetItemsByRef().begin() + pContainer->GetSelectedItem());

            // Update the selection if we deleted the last item
            if (pContainer->GetSelectedItem() >= (int) pContainer->GetNumItems())
            {
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), SHORTCUT_LIST, pContainer->GetNumItems() - 1);
              OnMessage(msg);
            }

            CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
            OnMessage(msg);
          }
        }
        // In case move was selected
        else if (shortcutActionDialog->IsActionMove())
        {
          m_moveShortcut = true;
          SetProperty("manage-set",2);

          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - Move shortcut was chose [m_moveShortcut=%d]. [m_manageButtonOn=%d][manage-set=%d] (mainmenu)(shortcut)",m_moveShortcut,m_manageButtonOn,GetPropertyInt("manage-set"));
        }
      }
    }
  }

  return true;
}

void CGUIDialogBoxeeMainMenu::HandleShortcutManageButtonClick()
{
  m_manageButtonOn = !m_manageButtonOn;
  SetProperty("manage-set",(int)m_manageButtonOn);

  SET_CONTROL_FOCUS(SHORTCUT_LIST,0);
}

