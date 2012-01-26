#include "GUIDialogBoxeeChapters.h"
#include "GUIWindowManager.h"
#include "GUIBaseContainer.h"
#include "utils/log.h"
#include "FileItem.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"

#define ITEMS_LIST 500

CGUIDialogBoxeeChapters::CGUIDialogBoxeeChapters()
: CGUIDialog(WINDOW_DIALOG_BOXEE_CHAPTERS, "boxee_chapters.xml")
{
  m_bConfirmed = true;
  m_initSelection = -1;
}


CGUIDialogBoxeeChapters::~CGUIDialogBoxeeChapters()
{
}

void CGUIDialogBoxeeChapters::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  LoadItems();
  m_bConfirmed = true;
}

bool CGUIDialogBoxeeChapters::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    m_bConfirmed = false;
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeChapters::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {
      return OnClick(message);
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeChapters::OnClick(CGUIMessage& message)
{
  int iControl = message.GetSenderId();
  switch(iControl)
  {
    case ITEMS_LIST:
    {
      CGUIBaseContainer* subMenuList = (CGUIBaseContainer*) GetControl(ITEMS_LIST);
      CGUIListItemPtr selectedListItem = subMenuList->GetSelectedItemPtr();
      if (selectedListItem.get())
      {
        m_selectionId = selectedListItem->GetPropertyInt("value");
      }
    }
    break;
  }

  Close();

  return true;
}

//static
bool CGUIDialogBoxeeChapters::Show(const std::vector<DemuxChapterInfo>& chapters, int& selectedChapterId, int currentSelection)
{
  CGUIDialogBoxeeChapters *dialog = (CGUIDialogBoxeeChapters*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_CHAPTERS);
  if (!dialog)
  {
    return false;
  }

  dialog->m_chapters = chapters;
  dialog->m_titles.clear();
  dialog->m_bAddBrowse = false;
  dialog->m_initSelection = currentSelection;
  dialog->DoModal();

  if( dialog->m_bConfirmed )
    selectedChapterId = dialog->m_selectionId;

  return dialog->m_bConfirmed;
}
//static
bool CGUIDialogBoxeeChapters::Show(const std::vector<DemuxTitleInfo>& titles, int& selectedTitleId, bool bCanBrowse /*= false*/)
{
  CGUIDialogBoxeeChapters *dialog = (CGUIDialogBoxeeChapters*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_CHAPTERS);
  if (!dialog)
  {
    return false;
  }

  dialog->m_titles = titles;
  dialog->m_chapters.clear();
  dialog->m_bAddBrowse = bCanBrowse;
  dialog->m_initSelection = 0;
  dialog->DoModal();
  
  if( dialog->m_bConfirmed )
    selectedTitleId = dialog->m_selectionId;
  
  return dialog->m_bConfirmed;
}

void CGUIDialogBoxeeChapters::LoadItems()
{
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), ITEMS_LIST, 0);
  OnMessage(msg);

  if( m_chapters.size() > 0 )
  {
    CGUIMessage label(GUI_MSG_LABEL_SET, GetID(), 610);
    label.SetLabel(21419);
    OnMessage(label);
    
    // We are in chapter display mode
    for (size_t i = 0; i< m_chapters.size(); i++)
    {
      CStdString title;
      bool showStartTime = true;

      if( m_chapters[i].startSecond == 0 && m_chapters[i].endSecond == 0)
        showStartTime = false;

      CStdString startTimeStr;
      StringUtils::SecondsToTimeString(m_chapters[i].startSecond, startTimeStr, TIME_FORMAT_HH_MM_SS);

      if (!m_chapters[i].title.IsEmpty())
      {
        if(showStartTime)
          title.Format("%d  %s  (%s)", i + 1, m_chapters[i].title, startTimeStr.c_str());
        else
          title.Format("%d  %s", i + 1, m_chapters[i].title);
      }
      else
      {
        if(showStartTime)
          title.Format("%d  %s  %d  (%s)", i + 1, g_localizeStrings.Get(21396).c_str(), i + 1, startTimeStr.c_str());
        else
          title.Format("%d  %s  %d", i + 1, g_localizeStrings.Get(21396).c_str(), i + 1);
      }

      CFileItemPtr item(new CFileItem(title));
      item->SetProperty("value", m_chapters[i].id);

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST, 0, 0, item);
      OnMessage(winmsg);
    }
  }
  else
  {
    CGUIMessage label(GUI_MSG_LABEL_SET, GetID(), 610);
    label.SetLabel(21450);
    OnMessage(label);
    
    // We are in title display mode
    CStdString line;
    
    // Add a 'browse...' item at the top for browsing the folder/disc
    if( m_bAddBrowse )
    {
      line.Format("%s", g_localizeStrings.Get(20153));
      CFileItemPtr item(new CFileItem(line));
      item->SetProperty("value", -1);

      CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST, 0, 0, item);
      OnMessage(winmsg);
    }
    
    // add the titles
    for (size_t i = 0; i < m_titles.size(); i++)
    {
      CStdString durStr;
      StringUtils::SecondsToTimeString(m_titles[i].duration, durStr, TIME_FORMAT_HH_MM_SS);
      
      if( !m_titles[i].title.IsEmpty() )
      {
        line.Format("%d. %s (%s)", i+1, m_titles[i].title, durStr.c_str());
      }
      else
      {
        line.Format("%d. %s %d (%s)", i+1, g_localizeStrings.Get(369).c_str(), i+1, durStr.c_str());
      }

      CFileItemPtr item(new CFileItem(line));
      item->SetProperty("value",m_titles[i].id);
      
      CGUIMessage wmsg(GUI_MSG_LABEL_ADD, GetID(), ITEMS_LIST, 0, 0, item);
      OnMessage(wmsg);
    }

    if(m_bAddBrowse)
      m_initSelection = 1;
  }

  if(m_initSelection != -1)
    CONTROL_SELECT_ITEM(ITEMS_LIST, m_initSelection);
}
