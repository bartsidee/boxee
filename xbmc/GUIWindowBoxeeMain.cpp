
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
#include "GUIWindowBoxeeBrowseWithPanel.h"
#include "GUIDialogBoxeeGlobalSearch.h"
#include "URL.h"
#include "BoxeeUtils.h"
#include "bxutils.h"
#include "BoxeeItemLauncher.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIUserMessages.h"

#include "GUIListContainer.h"

#define BTN_LIST    9000

#define BTN_PHOTOS  111
#define BTN_MUSIC   112
#define BTN_MOVIES  113
#define BTN_SEARCH  114
#define BTN_TVSHOWS 115
#define BTN_APPS    116
#define BTN_FILES   117
#define BTN_DVD     118

//#define LIST_SHORTCUTS        9110
//#define IMG_OVERLAY_SHORTCUTS 9120
//#define BTN_MANAGE_SHORTCUTS  9130
//#define BTN_MANAGE_EXIT       9131
#define BTN_NEW_VERSION       2410
//#define LIST_SIDE_MENU        9000
//#define EDIT_SEARCH           9140

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

  SET_CONTROL_FOCUS(BTN_LIST,3);
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
    CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);

    if (pMenu)
    {
      pMenu->DoModal();
      return true;
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeMain::OnAction - FAILED to get WINDOW_BOXEE_DIALOG_MAIN_MENU. [ActionId=%d] (home)",action.id);
      return true;
    }
  }
  break;
  }

  return CGUILoaderWindow::OnAction(action);
}

bool CGUIWindowBoxeeMain::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() ) 
  {
  case GUI_MSG_WINDOW_DEINIT:
  {
    CONTROL_SELECT_ITEM(BTN_LIST,3);
  }
  break;
  case GUI_MSG_UPDATE:
  {
    int iControl = message.GetSenderId();

    if (iControl == 0)
    {
      CGUIMessage winmsg1(GUI_MSG_UPDATE, GetID(), LIST_RECOMMEND);
      OnMessage(winmsg1);

      CGUIMessage winmsg2(GUI_MSG_UPDATE, GetID(), LIST_FEATURES);
      OnMessage(winmsg2);

      CGUIMessage winmsg3(GUI_MSG_UPDATE, GetID(), LIST_QUEUE);
      OnMessage(winmsg3);

      return true;
    }
    else
    {
      switch(message.GetControlId())
      {
      case LIST_RECOMMEND:
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_UPDATE - Going to reload LIST_RECOMMEND (home)(rec)");
        ReloadContainer(LIST_RECOMMEND,true);
        return true;
      }
      break;
      case LIST_FEATURES:
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_UPDATE - Going to reload LIST_FEATURES (home)(feat)");
        ReloadContainer(LIST_FEATURES,true);
        return true;
      }
      break;
      case LIST_QUEUE:
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_UPDATE - Going to reload LIST_QUEUE (home)(queue)");
        ReloadContainer(LIST_QUEUE,true);
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
      switch (iControl)
      {
      case BTN_PHOTOS:
      {
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_PHOTOS, "boxeedb://pictures/");
      }
      break;
      case BTN_MUSIC:
      {
        ((CGUIWindowBoxeeBrowseWithPanel*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_ALBUMS))->ShowPanel();
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_ALBUMS);
      }
      break;
      case BTN_MOVIES:
      {
        ((CGUIWindowBoxeeBrowseWithPanel*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_TVSHOWS))->ShowPanel();
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_MOVIES);
      }
      break;
      case BTN_SEARCH:
      {
        CStdString searchString;
        CGUIDialogBoxeeGlobalSearch::ShowAndGetInput(searchString, "Search", true, false);
      }
      break;
      case BTN_TVSHOWS:
      {
        ((CGUIWindowBoxeeBrowseWithPanel*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_TVSHOWS))->ShowPanel();
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVSHOWS);
      }
      break;
      case BTN_APPS:
      {
        ((CGUIWindowBoxeeBrowseWithPanel*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_APPS))->ShowPanel();
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS,"apps://all");
      }
      break;
      case BTN_FILES:
      {
        g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_LOCAL, "sources://all/");
      }
      break;
#ifdef HAS_DVD_DRIVE
      case BTN_DVD:
      {
    	  CGUIDialogBoxeeMediaAction::OpenDvdDialog();
      }
      break;
#endif
      case LIST_RECOMMEND:
      case LIST_FEATURES:
      case LIST_QUEUE:
      {
        HandleClickInList(iControl);
      }
      break;
      }
    }
  }
  break;
  case GUI_MSG_LABEL_BIND:
  {
    CFileItemList* pList = (CFileItemList*)message.GetPointer();

    if (pList && (pList->Size() > 0))
    {
      CStdString strParams;
      CStdString strDir;
      CStdString strFile;
      std::map<std::string, std::string> mapParams;

      BoxeeUtils::ParseBoxeeDbUrl( pList->m_strPath, strDir, strFile, mapParams );

      CStdString limitStr = mapParams["limit"];
      CStdString countStr = mapParams["count"];

      if (!limitStr.IsEmpty() || !countStr.IsEmpty())
      {
        ////////////////////////////////////////////////////////////////////////////////////////
        // in case there is a "limit" option -> add a "View More" item at the end of the list //
        ////////////////////////////////////////////////////////////////////////////////////////

        CFileItem* viewMoreItem = NULL;

        if( strDir == "queue" )
        {
          CGUIListContainer* pControl = (CGUIListContainer*)GetControl(LIST_QUEUE);
          if (pControl)
          {
            if (pList->Size() > pControl->GetItemsPerPage())
            {
              viewMoreItem = new CFileItem();

              viewMoreItem->SetLabel(g_localizeStrings.Get(53900));
              //viewMoreItem->m_strPath "";                            // TODO: need to be set from skin
              viewMoreItem->SetThumbnailImage("defaultfolder.png");    // TODO: need to be set from skin
              viewMoreItem->SetProperty("external-browse-all",true);
            }
            else
            {
              CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_LABEL_BIND - Not adding [%s] to LIST_QUEUE because [pControl=%p][ControlItemsPerPage=%d][ListItemSize=%d] (home)",g_localizeStrings.Get(53900).c_str(),pControl,pControl->GetItemsPerPage(),pList->Size());
            }
          }
        }
        else if( strDir == "recommend" )
        {
          CGUIListContainer* pControl = (CGUIListContainer*)GetControl(LIST_RECOMMEND);
          if (pControl)
          {
            if (pList->Size() > pControl->GetItemsPerPage())
            {
              viewMoreItem = new CFileItem();

              viewMoreItem->SetLabel(g_localizeStrings.Get(53905));
              //viewMoreItem->m_strPath = "";                          // TODO: need to be set from skin
              viewMoreItem->SetThumbnailImage("defaultfolder.png");    // TODO: need to be set from skin
              viewMoreItem->SetProperty("external-browse-all",true);
            }
            else
            {
              CLog::Log(LOGDEBUG,"CGUIWindowBoxeeMain::OnMessage - GUI_MSG_LABEL_BIND - Not adding [%s] to LIST_RECOMMEND because [pControl=%p][ControlItemsPerPage=%d][ListItemSize=%d] (home)",g_localizeStrings.Get(53905).c_str(),pControl,pControl->GetItemsPerPage(),pList->Size());
            }
          }
        }

        if (viewMoreItem)
        {
          pList->Add(CFileItemPtr(viewMoreItem));
        }
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
      case LIST_RECOMMEND:
      {
        HandleClickOnRecommendationItem(*pSelectedItem);
      }
      break;
      case LIST_FEATURES:
      {
        HandleClickOnFeatureItem(*pSelectedItem);
      }
      break;
      case LIST_QUEUE:
      {
        HandleClickOnQueueItem(*pSelectedItem);
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

void CGUIWindowBoxeeMain::HandleClickOnRecommendationItem(const CFileItem& item)
{
  if (item.GetPropertyBOOL("external-browse-all"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_DISCOVER,"feed://share");
  }
  else
  {
    CFileItem recommendationItem(item);
    recommendationItem.SetProperty("isFeed",true);
    recommendationItem.Dump();
    CBoxeeItemLauncher::Launch(recommendationItem);
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

void CGUIWindowBoxeeMain::HandleClickOnQueueItem(const CFileItem& item)
{
  if (item.GetPropertyBOOL("external-browse-all"))
  {
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_QUEUE,"feed://queue");
  }
  else
  {
    CFileItem queueItem(item);
    queueItem.SetProperty("isQueue",true);
    queueItem.Dump();
    CBoxeeItemLauncher::Launch(queueItem);
  }
}

