/*
 * GUIDialogBoxeeSortDropdown.cpp
 *
 *  Created on: Nov 24, 2010
 *      Author: shayyizhak
 */

#include "GUIDialogBoxeeSortDropdown.h"
#include "GUIWindowManager.h"
#include "GUIBaseContainer.h"
#include "utils/log.h"
#include "GUIListContainer.h"
#include "GUIControlGroupList.h"
#include "GUIToggleButtonControl.h"
#include "GUILabelControl.h"

#define ITEMS_LIST                        600
#define GROUP_LIST                        500

#define TOGGLE_BUTTON_CONTROL             8020
#define TOGGLE_BUTTON_POSX_OFFSET         0
#define TOGGLE_BUTTON_POSY_OFFSET         0

#define BACKGROUND_IMAGE_CONTROL          8021
#define BACKGROUND_IMAGE_POSX_OFFSET      -158
#define BACKGROUND_IMAGE_POSY_OFFSET      0

#define CONTROL_SORT_LABEL                9001
#define LABEL_POSX_OFFSET                 -228
#define LABEL_POSY_OFFSET                 0

#define LIST_ITEM_HEIGHT                  54
#define LIST_HEIGHT_OFFSET                0
#define LIST_POSX_OFFSET                  -240
#define LIST_POSY_OFFSET                  -3

CGUIDialogBoxeeSortDropdown::CGUIDialogBoxeeSortDropdown() : CGUIDialog(WINDOW_DIALOG_BOXEE_SORT_DROPDOWN, "boxee_sort_dropdown.xml")
{
  m_bConfirmed = true;
}


CGUIDialogBoxeeSortDropdown::~CGUIDialogBoxeeSortDropdown()
{

}

void CGUIDialogBoxeeSortDropdown::OnInitWindow()
{
  if (!LoadItems())
  {
    Close();
    return;    
  }

  CGUIDialog::OnInitWindow();

  m_bConfirmed = true;

  SET_CONTROL_FOCUS(ITEMS_LIST,m_selectedSort);
}

bool CGUIDialogBoxeeSortDropdown::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeSortDropdown::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {
      return OnClick(message);
    }
  } //switch

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeSortDropdown::OnClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
    case ITEMS_LIST:
    {
      CGUIBaseContainer* subMenuList = (CGUIBaseContainer*)GetControl(ITEMS_LIST);
      GetClickedLabel(subMenuList);
    }
    break;
  } // switch

  Close();

  return true;
}

bool CGUIDialogBoxeeSortDropdown::Show(CFileItemList& items, CStdString& value, float posX, float posY)
{
  CGUIDialogBoxeeSortDropdown* dialog = (CGUIDialogBoxeeSortDropdown *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SORT_DROPDOWN);

  if (!dialog)
  {
    return false;
  }

  dialog->m_selectedSort = items.GetPropertyInt("selectedSort");

  dialog->m_items.Clear();
  dialog->m_items.Append(items);
  dialog->m_value = value;

  dialog->AllocResources();

  dialog->SetControlPos((CGUIControl*)dialog->GetControl(GROUP_LIST),(posX + LIST_POSX_OFFSET),(posY + LIST_POSY_OFFSET));
  dialog->SetControlPos((CGUIControl*)dialog->GetControl(TOGGLE_BUTTON_CONTROL),(posX + TOGGLE_BUTTON_POSX_OFFSET),(posY + TOGGLE_BUTTON_POSY_OFFSET));
  dialog->SetControlPos((CGUIControl*)dialog->GetControl(CONTROL_SORT_LABEL),(posX + LABEL_POSX_OFFSET),(posY + LABEL_POSY_OFFSET));
  dialog->SetControlPos((CGUIControl*)dialog->GetControl(BACKGROUND_IMAGE_CONTROL),(posX + BACKGROUND_IMAGE_POSX_OFFSET),(posY + BACKGROUND_IMAGE_POSY_OFFSET));

  dialog->DoModal();

  value =  dialog->m_value;

  return dialog->m_bConfirmed;
}


bool CGUIDialogBoxeeSortDropdown::Show(CFileItemList& items, CStdString& value)
{
  CGUIDialogBoxeeSortDropdown *dialog = (CGUIDialogBoxeeSortDropdown *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SORT_DROPDOWN);

  if (!dialog)
  {
    return false;
  }

  dialog->m_items.Clear();
  dialog->m_items.Append(items);
  dialog->m_value = value;

  dialog->DoModal();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeSortDropdown::Show, set genre =  %s (browse)", dialog->m_value.c_str());
  value =  dialog->m_value;

  return dialog->m_bConfirmed;
}

bool CGUIDialogBoxeeSortDropdown::LoadItems()
{
  bool succeeded = false;

  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), ITEMS_LIST, 0);
  OnMessage(msg);

  for (int i=0; i< m_items.Size(); i++)
  {
    CFileItemPtr item = m_items.Get(i);
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST, 0, 0, m_items.Get(i));
    OnMessage(winmsg);
  }

  CGUIListContainer* pList = (CGUIListContainer*)GetControl(ITEMS_LIST);
  float listHeight = (pList->GetNumItems() * LIST_ITEM_HEIGHT) + LIST_HEIGHT_OFFSET;
  if (pList)
  { 
    pList->SetHeight(listHeight);
    AllocResources();
    succeeded = true;
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSortDropdown::LoadItems - FAILED to get ListContainer for [%d]",ITEMS_LIST);
  }

  return succeeded;
}

bool CGUIDialogBoxeeSortDropdown::GetClickedLabel(CGUIBaseContainer* subMenuList)
{
  if (!subMenuList)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSortDropdown::GetClickedLabel - Enter function with a NULL CGUIBaseContainer object");
    return false;
  }

  CGUIListItemPtr selectedListItem = subMenuList->GetSelectedItemPtr();
  if (selectedListItem.get())
  {
    m_value = selectedListItem->GetProperty("value");
  }

  return true;
}

bool CGUIDialogBoxeeSortDropdown::SetControlPos(CGUIControl* pControl, float posX, float posY)
{
  if (!pControl)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeSortDropdown::SetControlPos - enter function with NULL control (sdd)");
    return false;
  }

  pControl->SetPosition(posX,posY);
  pControl->AllocResources();

  return true;
}

