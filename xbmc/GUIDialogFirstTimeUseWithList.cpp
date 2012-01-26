#include "GUIDialogFirstTimeUseWithList.h"

#ifdef HAS_EMBEDDED

#include "log.h"

CGUIDialogFirstTimeUseWithList::CGUIDialogFirstTimeUseWithList(int id, const CStdString &xmlFile, const CStdString& name) : CGUIDialogFirstTimeUseBase(id,xmlFile,name)
{
  m_selectedIndex = -1;
}

CGUIDialogFirstTimeUseWithList::~CGUIDialogFirstTimeUseWithList()
{

}

void CGUIDialogFirstTimeUseWithList::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), LIST_CTRL);
  OnMessage(message);

  m_selectedItem.reset();
  m_listItems.Clear();

  if (FillListOnInit())
  {
    CLog::Log(LOGDEBUG,"%s::OnInitWindow - After adding [NumOfItems=%d] to list (initbox)",m_name.c_str(),m_listItems.Size());
  }
  else
  {
    // TODO: handle this case !!!
  }

  if (m_selectedIndex >= 0 && m_selectedIndex < m_listItems.Size())
  {
    m_selectedItem = m_listItems.Get(m_selectedIndex);
    m_selectedItem->Select(true);
  }
}

bool CGUIDialogFirstTimeUseWithList::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"%s::OnMessage - GUI_MSG_CLICKED - [iControl=%d] (initbox)",m_name.c_str(),iControl);

    switch (iControl)
    {
    case LIST_CTRL:
    {
      // Get selected index from the list
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), LIST_CTRL);
      OnMessage(msg);

      m_selectedIndex = msg.GetParam1();

      CLog::Log(LOGDEBUG,"%s::OnMessage - GUI_MSG_CLICKED - [ChoiceIndex=%d] (initbox)",m_name.c_str(),m_selectedIndex);

      if (m_selectedIndex < 0 || m_selectedIndex > m_listItems.Size() - 1)
      {
        CLog::Log(LOGERROR,"%s::OnMessage - GUI_MSG_CLICKED - FAILED to get [ChoiceIndex=%d]. [ListSize=%d] (initbox)",m_name.c_str(),m_selectedIndex,m_listItems.Size());
        return true;
      }

      if (m_selectedItem.get())
      {
        m_selectedItem->Select(false);
      }

      m_selectedItem = m_listItems.Get(m_selectedIndex);
      m_selectedItem->Select(true);

      if (!HandleListChoice())
      {
        CLog::Log(LOGERROR,"%s::OnMessage - GUI_MSG_CLICKED - FAILED to handle list choice [ChoiceIndex=%d]. (initbox)",m_name.c_str(),m_selectedIndex);
        return true;
      }

      m_actionChoseEnum = CActionChose::NEXT;
      Close();
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseWithList::HasSelectedItem()
{
  if (m_selectedItem.get())
  {
    return true;
  }

  return false;
}

CFileItemPtr CGUIDialogFirstTimeUseWithList::GetSelectedItem()
{
  return m_selectedItem;
}

int CGUIDialogFirstTimeUseWithList::GetSelectedIndex()
{
  return m_selectedIndex;
}

#endif

