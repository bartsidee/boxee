
#include "FileItem.h"
#include "StdString.h"

#include "XAPP_List.h"
#include "GUIBaseContainer.h"
#include "GUIMessage.h"
#include "GUIWindowManager.h"
#include "GUIWindowApp.h"
#include "Application.h"

#include "utils/log.h"
#include "utils/SingleLock.h"

namespace XAPP
{

List::List(int windowId, int controlId) throw (AppException) : Control(windowId, controlId)
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (!(type == CGUIControl::GUICONTAINER_LIST || type == CGUIControl::GUICONTAINER_FIXEDLIST || type == CGUIControl::GUICONTAINER_WRAPLIST || type == CGUIControl::GUICONTAINER_PANEL))
        throw AppException("Control is not a List");
      }

void List::ScrollPageUp()
{
  CGUIMessage message(GUI_MSG_PAGE_UP, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
}

void List::ScrollPageDown()
{
  CGUIMessage message(GUI_MSG_PAGE_DOWN, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
}

void List::SetContentURL(const std::string& url)
{
  CStdString urlStr = url;
  urlStr.Replace("http://", "rss://");
  
  CGUIWindowApp* pWindow = (CGUIWindowApp*) g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (pWindow)
  {
    pWindow->SetContainerPath(m_controlId, urlStr);    
  }
}

void List::SetFocusedItem(int item)
{
  CGUIMessage message(GUI_MSG_ITEM_SELECT, m_windowId, m_controlId, item);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
}

void List::SetSelected(int item, bool selected)
{
  if (selected)
  {
    CGUIMessage message(GUI_MSG_MARK_ITEM, m_windowId, m_controlId, item);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
  }      
  else
  {
    CGUIMessage message(GUI_MSG_UNMARK_ITEM, m_windowId, m_controlId, item);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
  }
}

void List::SelectAll()
{
  CGUIMessage message(GUI_MSG_MARK_ALL_ITEMS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
}

void List::UnselectAll()
{
  CGUIMessage message(GUI_MSG_UNMARK_ALL_ITEMS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
}

void List::JumpToLetter(char letter)
{
  CGUIMessage message(GUI_MSG_JUMPTOLETTER, m_windowId, m_controlId);
  CStdString strLabel;
  strLabel.Format("%c", letter);
  message.SetLabel(strLabel);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, false);
    }

ListItems List::GetSelected()
{
  ListItems result;

  CGUIMessage msg(GUI_MSG_GET_MARKED_ITEMS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  std::vector<CGUIListItemPtr>* guiListItems = (std::vector<CGUIListItemPtr>*)msg.GetPointer();

  if (guiListItems != NULL)
  {
    for (size_t i = 0; i < guiListItems->size(); i++)
    {
      result.push_back(ListItem(boost::static_pointer_cast<CFileItem>((*guiListItems)[i])));
      }
    delete guiListItems;
    }

  return result;
}

bool List::IsSelected(int itemIndex)
{
  ListItems result;

  bool bIsSelected = false;

  CGUIMessage msg(GUI_MSG_GET_ITEMS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  std::vector<CGUIListItemPtr>* guiListItems = (std::vector<CGUIListItemPtr>*)msg.GetPointer();

  if (guiListItems != NULL)
  {
    CFileItemPtr pItem = boost::static_pointer_cast<CFileItem>((*guiListItems)[itemIndex]);
    if (pItem)
    {
      bIsSelected = pItem->IsSelected();
    }
    delete guiListItems;
  }

  return bIsSelected;
}

int List::GetFocusedItem() throw (XAPP::AppException)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
  return msg.GetParam1();
    }

ListItems List::GetItems()
{
  ListItems result;

  CGUIMessage msg(GUI_MSG_GET_ITEMS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  std::vector<CGUIListItemPtr>* guiListItems = (std::vector<CGUIListItemPtr>*)msg.GetPointer();

  if (guiListItems != NULL)
  {
    for (size_t i = 0; i < guiListItems->size(); i++)
    {
      result.push_back(ListItem(boost::static_pointer_cast<CFileItem>((*guiListItems)[i])));
      }
  delete guiListItems;
  }
  return result;	
}

ListItem List::GetItem(int item) throw (XAPP::AppException)
{
  CGUIMessage msg(GUI_MSG_GET_ITEM, m_windowId, m_controlId, item);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  ListItem result = ListItem(boost::static_pointer_cast<CFileItem>(msg.GetItem()));
        return result;

  throw AppException("List::GetItem unable to get item from list");
}

void List::SetItems(ListItems list, int iSelectedItem)
{
  CFileItemList fileItems;

  {
    CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, m_windowId, m_controlId, 0, 0);
    g_windowManager.SendThreadMessage(clearmsg);
  }

  for (size_t i = 0; i < list.size(); i++)
  {
    CFileItem* pItem =  list[i].GetFileItem().get();
    pItem->Select(i == (size_t) iSelectedItem);
    CFileItemPtr fileItem(new CFileItem(*pItem));
    fileItems.Add(fileItem);

    {
      CGUIMessage addMsg(GUI_MSG_LABEL_ADD, m_windowId, m_controlId, 0, 0, fileItem);
      g_windowManager.SendThreadMessage(addMsg);
    }
  }
  
  if (fileItems.Size() > 0)
  {
    g_application.GetItemLoader().AddControl(m_windowId, m_controlId, fileItems, SORT_METHOD_NONE, SORT_ORDER_NONE, iSelectedItem);
  }
  else
  {
    CGUIMessage clearmsg(GUI_MSG_LABEL_RESET, m_windowId, m_controlId, 0, 0);
    g_windowManager.SendThreadMessage(clearmsg);
  }
}

void List::Refresh()
{
  CGUIMessage clearmsg(GUI_MSG_RELOAD_CONTENT, m_windowId, m_controlId, 0, 0);
  g_windowManager.SendThreadMessage(clearmsg);
}

void List::UpdateItem(int index, ListItem item)
{
  CFileItem* pItem =  item.GetFileItem().get();
  CFileItemPtr fileItem(new CFileItem(*pItem));

  CGUIMessage updatemsg(GUI_MSG_LABEL_SET, m_windowId, m_controlId, index, 0, fileItem);
  g_windowManager.SendThreadMessage(updatemsg);
}

}
