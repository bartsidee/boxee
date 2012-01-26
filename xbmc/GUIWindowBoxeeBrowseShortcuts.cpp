
#include "GUIWindowBoxeeBrowseShortcuts.h"
#include "BoxeeShortcut.h"
#include "GUIDialogBoxeeShortcutAction.h"
#include "GUIDialogYesNo2.h"
#include "Settings.h"
#include "GUILoaderWindow.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeApplicationAction.h"
#include "lib/libBoxee/boxee.h"
#include "utils/log.h"

using namespace std;
using namespace BOXEE;

#define CONTROL_SHORTCUT_LIST     57
#define MENU_MANAGE               120

CGUIWindowBoxeeBrowseShortcuts::CGUIWindowBoxeeBrowseShortcuts() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_SHORTCUTS,"boxee_browse_shortcuts.xml")
{
  m_strPath = "shortcuts://all";
  m_manageButtonOn = false;
  //m_removeShortcut = false;
  m_moveShortcut = false;
}

CGUIWindowBoxeeBrowseShortcuts::~CGUIWindowBoxeeBrowseShortcuts()
{
  
}

bool CGUIWindowBoxeeBrowseShortcuts::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    if (GetFocusedControlID() == CONTROL_SHORTCUT_LIST && m_moveShortcut)
    {
      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetFocusedControl();

      std::vector<CBoxeeShortcut>& shortcutList = g_settings.GetShortcuts().GetItems();
      int previousItem = pControl->GetSelectedItem();

      // Handle the move event
      CGUIWindowBoxeeBrowse::OnAction(action);

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

        // Do the same to the browse list items
        const CFileItemPtr theBrowseItem = m_vecModelItems.Get(previousItem);
        m_vecModelItems.Remove(previousItem);
        m_vecModelItems.AddFront(theBrowseItem, currentItem);
      }

      return true;
    }
  }
  default:
  {
    // do nothing
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseShortcuts::OnMessage(CGUIMessage& message)
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

void CGUIWindowBoxeeBrowseShortcuts::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnInitWindow - Enter function (bshort)");

  m_strPath = "shortcuts://all";

  m_windowState->InitState(m_strPath);

//  m_configuration.ClearActiveFilters();
//  m_configuration.ClearSortMethods();

  m_manageButtonOn = false;
  SetProperty("button-manage-on",(int)m_manageButtonOn);

  m_moveShortcut = false;

  CGUIWindowBoxeeBrowse::OnInitWindow();
}

CStdString CGUIWindowBoxeeBrowseShortcuts::CreatePath()
{
  CStdString strPath = m_strPath;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::CreatePath - Going to return [strPath=%s] (bshort)",strPath.c_str());

  return strPath;
}

bool CGUIWindowBoxeeBrowseShortcuts::ProcessMenuClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::ProcessMenuClick - Enter function with [iControl=%d] (bshort)",iControl);

  switch (iControl)
  {
  case MENU_MANAGE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::ProcessMenuClick - Handling click on [%s=%d=MENU_MANAGE] (bshort)",CGUIWindowBoxeeBrowseShortcuts::ControlIdAsString(iControl),iControl);

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

bool CGUIWindowBoxeeBrowseShortcuts::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnClick - Enter function with [iItem=%d] (bshort)",iItem);

  if(!m_manageButtonOn)
  {
    //////////////////////////////////////////////////////////////////////////////
    // case of click on item with m_manageButtonOn=FALSE -> Launch the shortcut //
    //////////////////////////////////////////////////////////////////////////////

    CFileItem pSelectedShortcut;

    if (!GetClickedItem(iItem, pSelectedShortcut))
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseShortcuts::OnClick - FAILED to get clicked item [iItem=%d]. [m_manageButtonOn=%d] (bshort)",iItem,m_manageButtonOn);
      return true;
    }

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnClick - [m_manageButtonOn=%d][iItem=%d] -> Going to launch shortcut [label=%s][path=%s][shortcut-command=%s] (bshort)",m_manageButtonOn,iItem,pSelectedShortcut.GetLabel().c_str(),pSelectedShortcut.m_strPath.c_str(),pSelectedShortcut.GetProperty("shortcut-command").c_str());

    pSelectedShortcut.Dump();
    CBoxeeShortcut shortcut(pSelectedShortcut);
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
    }
    else
    {
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////
      // case of click on item with m_manageButtonOn=TRUE -> Open WINDOW_BOXEE_SHORTCUT_DIALOG around the shortcut //
      ///////////////////////////////////////////////////////////////////////////////////////////////////////////////

      CFileItem pSelectedShortcut;

      if (!GetClickedItem(iItem, pSelectedShortcut))
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseShortcuts::OnClick - FAILED to get clicked item [iItem=%d]. [m_manageButtonOn=%d] (bshort)",iItem,m_manageButtonOn);
        return true;
      }

      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetControl(CONTROL_SHORTCUT_LIST);

      if (!pControl)
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseShortcuts::OnClick - FAILED to get the items control. [iItem=%d][m_manageButtonOn=%d] (bshort)",iItem,m_manageButtonOn);
        return true;
      }

      CGUIDialogBoxeeShortcutAction* shortcutActionDialog = (CGUIDialogBoxeeShortcutAction*) g_windowManager.GetWindow(WINDOW_BOXEE_SHORTCUT_DIALOG);
      // The following is required so that SetPosition will actually work, otherwise during GUI_MSG_INIT, the XML is reloaded
      // and the position is reset to whatever is written there
      shortcutActionDialog->AllocResources();
      shortcutActionDialog->SetAllowRemove(!pSelectedShortcut.HasProperty("shortcut-readonly"));
      shortcutActionDialog->SetPosition(pControl->GetFocusedPositionX(),pControl->GetFocusedPositionY());

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnClick - Open SHORTCUT_DIALOG [X=%f][%f]. [iItem=%d][m_manageButtonOn=%d] (bshort)",shortcutActionDialog->GetXPosition(),shortcutActionDialog->GetYPosition(),iItem,m_manageButtonOn);

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

            shortcutList.erase(shortcutList.begin() + pControl->GetSelectedItem());
            g_settings.GetShortcuts().Save();

            // Delete the list item
            pControl->GetItemsByRef().erase(pControl->GetItemsByRef().begin() + pControl->GetSelectedItem());

            // Update the selection if we deleted the last item
            if (pControl->GetSelectedItem() >= (int) pControl->GetNumItems())
            {
              CGUIMessage msg(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_SHORTCUT_LIST, pControl->GetNumItems() - 1);
              OnMessage(msg);
            }

            Refresh();
          }
        }
        // In case move was selected
        else if (shortcutActionDialog->IsActionMove())
        {
          m_moveShortcut = true;

          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::OnClick - Move shortcut was chose [m_moveShortcut=%d]. [m_manageButtonOn=%d] (bshort)",m_moveShortcut,m_manageButtonOn);
        }
      }
    }
  }

  return true;
}

bool CGUIWindowBoxeeBrowseShortcuts::HandleMenuManage()
{
  m_manageButtonOn = !m_manageButtonOn;

  SetProperty("button-manage-on",(int)m_manageButtonOn);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseShortcuts::HandleMenuManage - After set [m_manageButtonOn=%d] (bshort)",m_manageButtonOn);

  return true;
}

void CGUIWindowBoxeeBrowseShortcuts::SortItems(CFileItemList &items)
{
  // Don't allow sort in this screen
}

const char* CGUIWindowBoxeeBrowseShortcuts::ControlIdAsString(int controlId)
{
  switch(controlId)
  {
  case CONTROL_SHORTCUT_LIST:
    return "CONTROL_SHORTCUT_LIST";
  case MENU_MANAGE:
    return "MENU_MANAGE";
  default:
    return "UNKNOWN";
  }
}
