
#include "GUIDialogBoxeeManualResolveAddFiles.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "FileSystem/Directory.h"
#include "GUIDialogBoxeeManualResolvePartAction.h"
#include "SpecialProtocol.h"

using namespace BOXEE;

#define FILE_LIST 5020
#define PART_LIST 5010

#define BUTTON_DONE 9010
#define BUTTON_PREV 9020

using namespace DIRECTORY;

CGUIDialogBoxeeManualResolveAddFiles::CGUIDialogBoxeeManualResolveAddFiles() :
  CGUIDialog(WINDOW_DIALOG_BOXEE_MANUAL_ADD_FILES, "boxee_manual_resolve_add_files.xml")
  {
    m_moveShortcut = false;
  }

CGUIDialogBoxeeManualResolveAddFiles::~CGUIDialogBoxeeManualResolveAddFiles()
{
}

void CGUIDialogBoxeeManualResolveAddFiles::Show(CFileItemPtr pItem, CFileItemList& items)
{
  CGUIDialogBoxeeManualResolveAddFiles *pDialog = (CGUIDialogBoxeeManualResolveAddFiles*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_ADD_FILES);
  if (pDialog)
  {
    // Copy the item into the dialog
    pDialog->m_videoItem = pItem;
    pDialog->m_partItems = items;

    pDialog->DoModal();

    pDialog->GetParts(items);
  }
}

void CGUIDialogBoxeeManualResolveAddFiles::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  // Send the item to the special container to allow skin acceess 
  CFileItemPtr itemPtr(new CFileItem(*m_videoItem.get()));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);

  // Read contents of the folder and populate the file list
  CStdString strFilePath = m_videoItem->m_strPath;
  CStdString strDirectory;
  CUtil::GetParentPath(strFilePath, strDirectory);

  //translate the direcotory 
  strDirectory = _P(strDirectory);
  GetDirectory(strDirectory);

  CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), PART_LIST);
  OnMessage(message);

  CGUIMessage message3(GUI_MSG_LABEL_BIND, GetID(), PART_LIST, 0, 0, &m_partItems);
  OnMessage(message3);



}

void CGUIDialogBoxeeManualResolveAddFiles::OnDeinitWindow(int nextWindowID)
  {
  CGUIDialog::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeManualResolveAddFiles::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (m_moveShortcut)
    {
      m_moveShortcut = false;
      SetProperty("manage-set",false);
    }
    else
    {
      Close();
    }

    return true;
  }
  break;
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    if (GetFocusedControlID() == PART_LIST && m_moveShortcut)
    {
      CGUIBaseContainer* pControl = (CGUIBaseContainer*) GetFocusedControl();

      int previousItem = pControl->GetSelectedItem();

      // Handle the move event
      CGUIDialog::OnAction(action);

      int currentItem = pControl->GetSelectedItem();

      if (currentItem != previousItem)
      {
        m_partItems.Swap(previousItem, currentItem);

        // Do the same to the screen list items
        std::vector< CGUIListItemPtr >& listItems = pControl->GetItemsByRef();
        CGUIListItemPtr theScreenItem = listItems[previousItem];
        listItems.erase(listItems.begin() + previousItem);
        listItems.insert(listItems.begin() + currentItem, theScreenItem);
      }
      return true;
    }
  }
  break;
  default:
  {
      // do nothing
  }
  break;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeManualResolveAddFiles::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == FILE_LIST)
    {
      // Get the selected item
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), FILE_LIST);
      OnMessage(msg);
      int iSelectedItem = msg.GetParam1();

      CFileItemPtr selectedItem = m_fileItems.Get(iSelectedItem);

      if (selectedItem->m_bIsFolder)
      {
        GetDirectory(selectedItem->m_strPath);
      }
      else
      {
        // Add selected video file to the list of parts, unless it is already there
        CFileItemPtr itemPtr(new CFileItem(*selectedItem.get()));
        for (int i = 0; i < m_partItems.Size(); i++)
        {
          if (selectedItem->m_strPath == m_partItems.Get(i)->m_strPath)
          {
            return true;
          }
        }

        selectedItem->Select(true); //set the selected item as on
        m_partItems.Add(itemPtr);


        CGUIMessage message3(GUI_MSG_LABEL_BIND, GetID(), PART_LIST, 0, 0, &m_partItems); //for the new added item
        OnMessage(message3);

        CGUIMessage message2(GUI_MSG_MARK_ITEM, GetID(), FILE_LIST, iSelectedItem, 0); //for the disabled item
        OnMessage(message2);
      }

    }
    else if (iControl == PART_LIST)
    {
      if (m_moveShortcut)
      {
        m_moveShortcut = false;
        SetProperty("manage-set",false);
      }
      else
      {
        OnPartListClick();
      }
    }
    else if (iControl == BUTTON_DONE)
    {
      Close();
    }
    else if (iControl == BUTTON_PREV)
    {
      // Clear the list of parts and activate the previous dialog
      m_partItems.Clear();
      Close();
    }
  }
  break;
  } // switch
  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeeManualResolveAddFiles::GetDirectory(const CStdString& strFolderPath)
{
  m_fileItems.Clear();
  CDirectory::GetDirectory(strFolderPath, m_fileItems);

  CStdString strParentDirectory;
  CUtil::GetParentPath(strFolderPath, strParentDirectory);

  CLog::Log(LOGERROR,"CGUIDialogBoxeeManualResolveAddFiles::GetDirectory, parent folder, path = %s (manual)", strParentDirectory.c_str());

  if (!strParentDirectory.IsEmpty())
  { //add the ability to go to the parent folder
    CFileItemPtr parentItemPtr(new CFileItem(".."));
    parentItemPtr->m_strPath = strParentDirectory;
    parentItemPtr->m_bIsFolder = true;
    m_fileItems.AddFront(parentItemPtr,0);
  }

  // Go over the files and filter out non relevant ones
  for (int i = 0; i < m_fileItems.Size();)
  {
    if (m_fileItems.Get(i)->IsVideo() || m_fileItems.Get(i)->m_bIsFolder)
    {

      //set the initial part of the selected item
      if (m_partItems.Size() == 0 && m_fileItems.Get(i)->m_strPath == m_videoItem->m_strPath)
      {
        CFileItemPtr initialItem(new CFileItem(*m_fileItems.Get(i).get()));
        m_partItems.Add(initialItem);
      }

      //remove items from m_fileItems that exist in the m_partItems
      for (int j = 0 ; j < m_partItems.Size() ; j++)
      {
        CStdString currentFileItemPath = m_fileItems.Get(i)->m_strPath;
        if (currentFileItemPath == m_partItems.Get(j)->m_strPath)
        {
          m_fileItems.Get(i)->Select(true);
          break; //move on to the next one
        }
      }

      i++;
    }
    else
    { //if its not a video or not a folder, don't show it
      m_fileItems.Remove(i);
    }
  }

  m_fileItems.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

  CGUIMessage message1(GUI_MSG_LABEL_RESET, GetID(), FILE_LIST);
  OnMessage(message1);

  CGUIMessage message2(GUI_MSG_LABEL_BIND, GetID(), FILE_LIST, 0, 0, &m_fileItems);
  OnMessage(message2);
}

void CGUIDialogBoxeeManualResolveAddFiles::OnPartListClick()
{
  CGUIBaseContainer* pContainer = (CGUIBaseContainer*)GetControl(PART_LIST);
  CGUIBaseContainer* pFileContainer = (CGUIBaseContainer*)GetControl(FILE_LIST);

  if (!pContainer)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeMainMenu::HandleShortcutListClick - FAILED to get container [%d=PART_LIST] container (mainmenu)(shortcut)",PART_LIST);
    return;
  }

  CGUIDialogBoxeeManualResolvePartAction* partActionDialog = (CGUIDialogBoxeeManualResolvePartAction*) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_MANUAL_PART_ACTION);
  // The following is required so that SetPosition will actually work, otherwise during GUI_MSG_INIT, the XML is reloaded
  // and the position is reset to whatever is written there
  partActionDialog->AllocResources();
  partActionDialog->SetAllowRemove(true);
  partActionDialog->SetPosition(pContainer->GetFocusedPositionX()+20,pContainer->GetFocusedPositionY()+140);

  partActionDialog->DoModal();

  if (!partActionDialog->IsCancelled())
  {
    // In case remove was selected
    if (partActionDialog->IsActionRemove())
    {
      int selectedItem = pContainer->GetSelectedItem();

      CFileItemPtr item = m_partItems.Get(selectedItem);

      std::vector < CGUIListItemPtr > fileitems = pFileContainer->GetItems();

      int i = 0;
      for (std::vector< CGUIListItemPtr >::iterator it = fileitems.begin(); it != fileitems.end(); it++)
      {
        CFileItemPtr itemUI(new CFileItem(*((CFileItem*) it->get())));
        if (item->m_strPath == itemUI->m_strPath)
        {
          it->get()->Select(false);
          item->Select(false);
          break;
        }
        i++;
      }

      m_partItems.Remove(selectedItem);

      CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), PART_LIST);
      OnMessage(message);

      CGUIMessage message3(GUI_MSG_LABEL_BIND, GetID(), PART_LIST, 0, 0, &m_partItems);
      OnMessage(message3);

    }
    // In case move was selected
    else if (partActionDialog->IsActionMove())
    {
      m_moveShortcut = true;
      SetProperty("manage-set",true);
    }
  }
}

