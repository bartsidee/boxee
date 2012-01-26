/*
 * CGUIDialogBoxeeSelectionList.cpp
 *
 */
#include "GUIDialogBoxeeSelectionList.h"
#include "GUIRadioButtonControl.h"
#include "GUIListContainer.h"
#include "GUIWindowManager.h"
#include "GUIImage.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

#define CONTROL_LIST_ACTION_DIALOG     11
#define DIALOG_TITLE  9001

CGUIDialogBoxeeSelectionList::CGUIDialogBoxeeSelectionList(void) : CGUIDialog(WINDOW_DIALOG_BOXEE_SELECTION_LIST, "boxee_selection_list.xml"), m_focusedItemPos (0)
{

}

CGUIDialogBoxeeSelectionList::~CGUIDialogBoxeeSelectionList(void)
{
}

bool CGUIDialogBoxeeSelectionList::OnMessage(CGUIMessage &message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (message.GetParam1() != ACTION_BUILT_IN_FUNCTION)
    {
      ProccessItemSelected();
      Close();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeSelectionList::OnInitWindow()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSelectionList::OnInitWindow - Enter function (cvq)");

  SET_CONTROL_LABEL(DIALOG_TITLE, m_strDialogTitle.c_str());
  m_canceled = true;
  CGUIDialog::OnInitWindow();
  LoadList();
}

void CGUIDialogBoxeeSelectionList::Close(bool forceClose)
{
  CGUIDialog::Close(forceClose);
}

void CGUIDialogBoxeeSelectionList::Reset()
{
  m_vecList.Clear();
  m_focusedItemPos = 0;
  m_strDialogTitle = "";
}

void CGUIDialogBoxeeSelectionList::LoadList()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSelectionList::LoadList");

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST_ACTION_DIALOG);
  OnMessage(msgReset);

  for (int i = 0 ; i < m_vecList.Size();  i++ )
  {
    CFileItemPtr listNode (new CFileItem((*m_vecList[i])));
    listNode->SetLabel(listNode->GetProperty("link-title"));

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_LIST_ACTION_DIALOG, 0, 0, listNode);
    OnMessage(msg);
  }

  if (m_focusedItemPos < m_vecList.Size())
  {
    SET_CONTROL_FOCUS(CONTROL_LIST_ACTION_DIALOG, m_focusedItemPos);
  }
  else
  {
    SET_CONTROL_FOCUS(CONTROL_LIST_ACTION_DIALOG,0);
  }
}

void CGUIDialogBoxeeSelectionList::ProccessItemSelected()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSelectionList::ProccessItemSelected");

  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST_ACTION_DIALOG);
  OnMessage(msg);

  m_selectedItemPos = msg.GetParam1();
  m_canceled = false;
}

int CGUIDialogBoxeeSelectionList::GetSelectedItemPos()
{
  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSelectionList::GetItemSelected");
  return m_selectedItemPos;
}
