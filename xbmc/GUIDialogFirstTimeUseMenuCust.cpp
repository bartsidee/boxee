/*
 * GUIDialogFirstTimeUseMenuCust.cpp
 *
 *  Created on: Nov 29, 2010
 *      Author: shayyizhak
 */

#include "GUIDialogFirstTimeUseMenuCust.h"
#include "log.h"
#include "GUISettings.h"
#include "Key.h"
#include "GUIWindowStateDatabase.h"

#define BUTTON_FILES 210
#define BUTTON_WEB 220
#define BUTTON_NOT_SURE 230
#define CATEGORY_LOCAL "local"
#define CATEGORY_FAVORITES "favorite"
#define CATEGORY_ALL "all"


CGUIDialogFirstTimeUseMenuCust::CGUIDialogFirstTimeUseMenuCust() : CGUIDialog(WINDOW_DIALOG_MENU_CUSTOMIZATION,"ftu_menu_cust.xml")
{

}

CGUIDialogFirstTimeUseMenuCust::~CGUIDialogFirstTimeUseMenuCust()
{

}

void CGUIDialogFirstTimeUseMenuCust::OnInitWindow()
{
  return CGUIDialog::OnInitWindow();
}

bool CGUIDialogFirstTimeUseMenuCust::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    //User must choose a default, so he can't close the dialog using "back".
    return true;
  }
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogFirstTimeUseMenuCust::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_CLICKED:
      {
        int senderId = message.GetSenderId();

        CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseMenuCust::OnMessage - GUI_MSG_CLICKED - [buttonId=%d]",senderId);

        switch (senderId)
        {
          case BUTTON_FILES:
          {
            CGUIWindowStateDatabase stateDB;
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, CATEGORY_LOCAL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, CATEGORY_LOCAL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, CATEGORY_FAVORITES);
          }
          break;
          case BUTTON_WEB:
          {
            CGUIWindowStateDatabase stateDB;
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, CATEGORY_ALL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, CATEGORY_ALL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, CATEGORY_ALL);
          }
          break;
          case BUTTON_NOT_SURE:
          {
            CGUIWindowStateDatabase stateDB;
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_TVSHOWS, CATEGORY_ALL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_MOVIES, CATEGORY_ALL);
            stateDB.SetDefaultCategory(WINDOW_BOXEE_BROWSE_APPS, CATEGORY_ALL);
          }
          break;
        }

        this->Close(true);
      }
  }


  return CGUIDialog::OnMessage(message);
}
