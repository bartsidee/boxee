#include "GUIDialogBoxeeDropdown.h"
#include "GUIWindowManager.h"
#include "GUIBaseContainer.h"
#include "utils/log.h"

#define ITEMS_LIST 500
#define ITEMS_LIST2 600
#define ITEMS_LIST2_LABEL 610

#define TITLE_PROPERTY "title"
#define CUSTOM_LABEL "CUSTOM_LABEL"

CGUIDialogBoxeeDropdown::CGUIDialogBoxeeDropdown(int id, const CStdString &xmlFile)
: CGUIDialog(id, xmlFile)
{
  m_bConfirmed = true;
  m_getDropdownLabelCallback = NULL;
}


CGUIDialogBoxeeDropdown::~CGUIDialogBoxeeDropdown()
{
}

void CGUIDialogBoxeeDropdown::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (m_type == 1)
  {
    SET_CONTROL_VISIBLE(ITEMS_LIST);
    SET_CONTROL_HIDDEN(ITEMS_LIST2);
    SET_CONTROL_FOCUS(ITEMS_LIST,0);
  }
  else
  {
    SET_CONTROL_VISIBLE(ITEMS_LIST2);
    SET_CONTROL_HIDDEN(ITEMS_LIST);
    SET_CONTROL_FOCUS(ITEMS_LIST2,0);
  }

  LoadItems();
  m_bConfirmed = true;

  SET_CONTROL_LABEL(ITEMS_LIST2_LABEL,m_title);
}

bool CGUIDialogBoxeeDropdown::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeDropdown::OnMessage(CGUIMessage& message)
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

bool CGUIDialogBoxeeDropdown::OnClick(CGUIMessage& message)
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
  case ITEMS_LIST2:
  {
    CGUIBaseContainer* subMenuList = (CGUIBaseContainer*)GetControl(ITEMS_LIST2);
    GetClickedLabel(subMenuList);
    }
  break;
  } // switch

  Close();

  return true;
}

bool CGUIDialogBoxeeDropdown::Show(CFileItemList& items, const CStdString& title, CStdString& value, float posX, float posY, int type, ICustomDropdownLabelCallback* cdlCallback)
{
  // save original position
  AllocResources();
  SetPosition(posX, posY);

  bool retVal = Show(items, title, value, type, cdlCallback);

  // reset original position
  //dialog->AllocResources();
  //dialog->SetPosition(orgPosX, orgPosY);

  return retVal;
}

bool CGUIDialogBoxeeDropdown::Show(CFileItemList& items, const CStdString& title, CStdString& value, int type, ICustomDropdownLabelCallback* cdlCallback)
{
  m_items.Clear();
  m_items.Append(items);
  m_type = type;
  m_value = value;

  if (cdlCallback)
  {
    CFileItemPtr customLabelItem(new CFileItem(cdlCallback->m_customTitle));
    customLabelItem->SetProperty("value",CUSTOM_LABEL);
    m_items.Add(customLabelItem);

    m_getDropdownLabelCallback = cdlCallback;
  }
  else
  {
    m_getDropdownLabelCallback = NULL;
  }

  SetTitle(title);

  DoModal();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeDropdown::Show, set m_value =  %s (browse)", m_value.c_str());
  value = m_value;

  return m_bConfirmed;
}

void CGUIDialogBoxeeDropdown::LoadItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), ITEMS_LIST, 0);
  OnMessage(msg);

  CGUIMessage msg2(GUI_MSG_LABEL_RESET, GetID(), ITEMS_LIST2, 0);
  OnMessage(msg2);

  int iSelectedItem = 0;

  if (m_type == 1)
  {
    for (int i=0; i< m_items.Size(); i++)
    {
      CFileItemPtr item = m_items.Get(i);
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST, 0, 0, m_items.Get(i));
      OnMessage(winmsg);

      CStdString value = item->GetProperty("value");
      if (stricmp(value.c_str(), m_value.c_str()) == 0)
      {
        iSelectedItem = i;
    }
  }

    CGUIMessage winmsg1(GUI_MSG_ITEM_SELECT, GetID(), ITEMS_LIST, iSelectedItem);
    OnMessage(winmsg1);
  }
  else
  {
    for (int i=0; i< m_items.Size(); i++)
    {
      CFileItemPtr item = m_items.Get(i);

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST2, 0, 0, m_items.Get(i));
      OnMessage(winmsg);

      CStdString value = item->GetProperty("value");
      if (stricmp(value.c_str(), m_value.c_str()) == 0)
      {
        iSelectedItem = i;
    }
  }
    CGUIMessage winmsg1(GUI_MSG_ITEM_SELECT, GetID(), ITEMS_LIST2, iSelectedItem);
    OnMessage(winmsg1);
}
}

void CGUIDialogBoxeeDropdown::SetTitle(const CStdString& title)
{
  m_title = title;
}

bool CGUIDialogBoxeeDropdown::GetClickedLabel(CGUIBaseContainer* subMenuList)
{
  if (!subMenuList)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeDropdown::GetClickedLabel - Enter function with a NULL CGUIBaseContainer object");
    return false;
  }

  CGUIListItemPtr selectedListItem = subMenuList->GetSelectedItemPtr();
  if (selectedListItem.get())
  {
    if (selectedListItem->GetProperty("value") == CUSTOM_LABEL)
    {
      if (m_getDropdownLabelCallback)
      {
        m_getDropdownLabelCallback->OnGetDropdownLabel(m_value);
      }
      else
      {
        CLog::Log(LOGERROR,"CGUIDialogBoxeeDropdown::GetClickedLabel - NO callback object to handle click on custom label");
        return false;
      }
    }
    else
    {
      m_value = selectedListItem->GetProperty("value");
    }
  }

  return true;
}

CGUIDialogBoxeeBrowserDropdown::CGUIDialogBoxeeBrowserDropdown()
: CGUIDialogBoxeeDropdown(WINDOW_DIALOG_BOXEE_BROWSER_DROPDOWN , "boxee_dropdown_browser.xml")
{
}


CGUIDialogBoxeeBrowserDropdown::~CGUIDialogBoxeeBrowserDropdown()
{
}
