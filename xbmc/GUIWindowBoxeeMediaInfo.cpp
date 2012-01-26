#include "GUIWindowBoxeeMediaInfo.h"
#include "GUIWindowManager.h"
#include "log.h"

CGUIWindowBoxeeMediaInfo::CGUIWindowBoxeeMediaInfo() : CGUIDialog(WINDOW_BOXEE_MEDIA_INFO, "boxee_media_info.xml")
{

}

CGUIWindowBoxeeMediaInfo::~CGUIWindowBoxeeMediaInfo()
{

}

void CGUIWindowBoxeeMediaInfo::Show(CFileItem* pItem)
{
  if (!pItem)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaInfo::Show - FAILED to open dialog. Enter with a NULL item (mip)");
    return;
  }

  CGUIWindowBoxeeMediaInfo* pDialog = (CGUIWindowBoxeeMediaInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_MEDIA_INFO);
  if (!pDialog)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeMediaInfo::Show - FAILED to open dialog. Got NULL for [id=%d] (mip)",WINDOW_BOXEE_MEDIA_INFO);
    return;
  }

  pDialog->m_item = *pItem;
  pDialog->DoModal();
}

void CGUIWindowBoxeeMediaInfo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(m_item));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);
}

bool CGUIWindowBoxeeMediaInfo::OnAction(const CAction &action)
{

  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIWindowBoxeeMediaInfo::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_ITEM_LOADED:
  {
    CFileItem *pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);
    if (pItem)
    {
      m_item = *pItem;
      CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
      g_windowManager.SendThreadMessage(winmsg1);
      CFileItemPtr itemPtr(new CFileItem(*pItem));
      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
      g_windowManager.SendThreadMessage(winmsg);
      delete pItem;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

