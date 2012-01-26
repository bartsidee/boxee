
#include "GUIWindowBoxeeBrowseHistory.h"
#include "utils/log.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIFixedListContainer.h"
#include "GUIDialogOK2.h"
#include "Application.h"
#include "GUIDialogYesNo2.h"
#include "Util.h"
#include "BoxeeItemLauncher.h"

#define CONTROL_LIST          52
#define CLEAR_HISTORY_BUTTON  9015

using namespace std;
using namespace BOXEE;

CHistoryWindowState::CHistoryWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.SetNewSource(new CBrowseWindowSource("historysource", "history://all", pWindow->GetID()));
}

void CHistoryWindowState::OnBind(CFileItemList& items)
{
  bool isEmpty = items.GetPropertyBOOL("empty");

  CBrowseWindowState::OnBind(items);
  m_pWindow->SetProperty("empty",isEmpty);
}

CGUIWindowBoxeeBrowseHistory::CGUIWindowBoxeeBrowseHistory() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_HISTORY, "boxee_browse_history.xml")
{
  SetWindowState(new CHistoryWindowState(this));
}

CGUIWindowBoxeeBrowseHistory::~CGUIWindowBoxeeBrowseHistory()
{
  
}

bool CGUIWindowBoxeeBrowseHistory::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    OnBack();
    return true;
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseHistory::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED && message.GetSenderId() == CLEAR_HISTORY_BUTTON)
  {
    //////////////////////////////////////
    // case of click on ClearAllHistory //
    //////////////////////////////////////

    if (CGUIDialogYesNo2::ShowAndGetInput(53702, 53776))
    {
      bool succeeded = g_application.GetBoxeeItemsHistoryList().ClearAllHistory();

      if (!succeeded)
      {
        CGUIDialogOK2::ShowAndGetInput(53701, 53775);
      }

      SET_CONTROL_FOCUS(CLEAR_HISTORY_BUTTON,0);
    }

    return true;
  }

  bool retVal = CGUIWindowBoxeeBrowse::OnMessage(message);

  if (message.GetMessage() == GUI_MSG_LABEL_BIND && message.GetControlId() == 0)
  {
    if (!GetPropertyBOOL("empty"))
    {
      CONTROL_ENABLE(CONTROL_LIST);
      SET_CONTROL_FOCUS(CONTROL_LIST,0);
      CONTROL_SELECT_ITEM(CONTROL_LIST,0);
    }
    else
    {
      SET_CONTROL_FOCUS(CLEAR_HISTORY_BUTTON,0);
    }
  }

  return retVal;
}

bool CGUIWindowBoxeeBrowseHistory::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseHistory::OnClick - Enter function with [iItem=%d] (browse)",iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
  {
    return true;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseHistory::OnClick - For [iItem=%d] going to handle item [label=%s][path=%s] (browse)",iItem,item.GetLabel().c_str(),item.m_strPath.c_str());

  item.Dump();

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // for all history items we want to open DialogBoxeeMediaAction for the RemoveFromHistory button //
  ///////////////////////////////////////////////////////////////////////////////////////////////////

  if (!CBoxeeItemLauncher::ExecutePlayableFolder(item))
  {
    return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
  }

  return true;
}
