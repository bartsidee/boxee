
#include "FileItem.h"
#include "GUIWindowManager.h"
#include "GUIWindowBoxeeMediaMain.h"
#include "GUIDialogMediaSource.h"
#include "GUIDialogBoxeeAppCtx.h"
#include "utils/PerformanceSample.h"
#include "GUIDialogYesNo.h"
#include "Util.h"

#define MEDIA_ITEMS_LIST 7102
#define MEDIA_ITEMS_LIST_SMALL 7103
#define APPS_ITEMS_LIST 7202
#define APPS_ITEMS_LIST_SMALL 7203

#define SEE_MORE_MEDIA_BUTTON 7101
#define SEE_MORE_APPS_BUTTON 7201

#define NEXT_MEDIA_BUTTON 7105
#define NEXT_APPS_BUTTON 7205

#define TAB_BUTTON_BROWSE 8002
#define TAB_BUTTON_APPS 8004

CGUIWindowBoxeeMediaMain::CGUIWindowBoxeeMediaMain(DWORD id, const char *xmlFile) : CGUIWindow(id, xmlFile)
{
  m_Loader = NULL;
}

CGUIWindowBoxeeMediaMain::~CGUIWindowBoxeeMediaMain(void)
{
  if (m_Loader)
    m_Loader->StopThread(true);
}

bool CGUIWindowBoxeeMediaMain::OnClick(int iControl, int iAction)
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
  OnMessage(msg);
  int iItem = msg.GetParam1();
  if (iItem < 0) 
    return false;

  CFileItem *pItem = NULL;
  if (iControl == MEDIA_ITEMS_LIST || iControl == MEDIA_ITEMS_LIST_SMALL)
  {
    pItem = m_listMediaItems[iItem].get();
    if (pItem && pItem->m_strPath != "add")
    {
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        OnMediaItemClick(pItem);
      else if (iAction == ACTION_PLAYER_PLAY)
        OnMediaItemPlay(pItem);
      else if (iAction == ACTION_SHOW_INFO || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        OnMediaItemPopup(pItem);
      return true;
    }
  }
  else if (iControl == APPS_ITEMS_LIST || iControl == APPS_ITEMS_LIST_SMALL)
  {
    pItem = m_listApps[iItem].get();
    if (pItem && pItem->m_strPath != "add")
    {
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
        OnAppsClick(pItem);
      else if (iAction == ACTION_SHOW_INFO || iAction == ACTION_CONTEXT_MENU || iAction == ACTION_MOUSE_RIGHT_CLICK)
        OnAppsPopup(pItem);
      return true;
    }
  }
  else if (iControl == SEE_MORE_APPS_BUTTON || iControl == NEXT_APPS_BUTTON)
    OnMoreApps();
  else if (iControl == SEE_MORE_MEDIA_BUTTON || iControl == NEXT_MEDIA_BUTTON)
    OnMoreMedia();

  if (pItem && pItem->m_strPath == "add" && (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK))
  {
    if (CGUIDialogMediaSource::ShowAndAddMediaSource(GetType()))
    {
      LoadMediaItems();
      return true;
    }
  }

  return false;
}

void CGUIWindowBoxeeMediaMain::UpdateItemInList(const CFileItem *pItem, CFileItemList &list)
{
  for (int i=0; i<list.Size(); i++)
  {
    CFileItemPtr pListItem = list[i];
    if (pListItem && pListItem->m_strPath == pItem->m_strPath)
    {
      *pListItem = *pItem;
      break;
    }
  }
}

bool CGUIWindowBoxeeMediaMain::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_WINDOW_DEINIT:
    Cleanup();
    CGUIWindow::OnMessage(message);
    return true;
  case MSG_ITEM_LOADED:
    {
      CFileItem *pItem = (CFileItem *)message.GetPointer();
      message.SetPointer(NULL);

      if (!pItem)
        return true;

      UpdateItemInList(pItem, m_listMediaItems);
      UpdateItemInList(pItem, m_listMediaItemsSmall);
      UpdateItemInList(pItem, m_listApps);
      UpdateItemInList(pItem, m_listAppsSmall);

      delete pItem;
    }
    return true;
  case GUI_MSG_CLICKED:
    {
      CGUIWindow::OnMessage(message);
      int iControl = message.GetSenderId();
      int iAction = message.GetParam1();
      
      if (iControl == TAB_BUTTON_BROWSE)
        OnMoreMedia();
      else if (iControl == TAB_BUTTON_APPS)
        OnMoreApps();
      else
        OnClick(iControl, iAction);
      return true;
    }
    break;
  case GUI_MSG_REFRESH_APPS:
    {
      LoadMediaItems();
      return true;
    }
    break;
  default:
    break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeMediaMain::OnInitWindow()
{
/*
  // if no media sources defined (except local ones) - ask the user if she wants to add them
  int nMediaShares = 0;
  const VECSOURCES &shares = GetShares();
  for (size_t i=0; i<shares.size(); i++)
  {
    const CMediaSource &share = shares[i];
    if (!CUtil::IsPlugin(share.strPath) && (CUtil::IsSmb(share.strPath) || CUtil::IsOnLAN(share.strPath) || CUtil::IsHD(share.strPath)))
      nMediaShares++;
  }

  if (nMediaShares <= 1) // 1 because there is a default source (/content)
  {
    if (CGUIDialogYesNo::ShowAndGetInput(51004, 0, 51005, 51006, 12018, 12019))
      g_windowManager.ActivateWindow(WINDOW_BOXEE_WIZARD_SOURCE_MANAGER);
  }
*/
  LoadMediaItems();
  CGUIWindow::OnInitWindow();
}

bool CGUIWindowBoxeeMediaMain::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow(); 
    return true;
  }
  else if (action.id == ACTION_PLAYER_PLAY)
  {
    if (GetFocusedControl())
      return OnClick(GetFocusedControl()->GetID(), ACTION_PLAYER_PLAY);
  }
  else if (action.id == ACTION_SHOW_INFO)
  {
    if (GetFocusedControl())
      return OnClick(GetFocusedControl()->GetID(), ACTION_SHOW_INFO);
  }

  return CGUIWindow::OnAction(action);
}

void CGUIWindowBoxeeMediaMain::Cleanup()
{
  //CPerformanceSample aSample("CGUIWindowBoxeeMediaMain::Cleanup",true);

  if (m_Loader)
    m_Loader->StopThread(true);
  m_Loader = NULL;
  
  int arrLabels[] = { MEDIA_ITEMS_LIST, MEDIA_ITEMS_LIST_SMALL, APPS_ITEMS_LIST, APPS_ITEMS_LIST_SMALL };
  for (int nLabel = 0; nLabel < 4; nLabel++)
  {
    CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), arrLabels[nLabel]);
    OnMessage(msg);
  }

  m_listMediaItems.Clear();
  m_listMediaItemsSmall.Clear();
  m_listApps.Clear();
  m_listAppsSmall.Clear();
}

CBackgroundInfoLoader *CGUIWindowBoxeeMediaMain::GetLoader()
{
  return new CPictureThumbLoader;
}

void CGUIWindowBoxeeMediaMain::LoadMediaItems()
{
  Cleanup();

  LoadMediaItems(m_listMediaItems);
  LoadAppItems(m_listApps);
  
  m_listMediaItemsSmall.Append(m_listMediaItems);
  m_listAppsSmall.Append(m_listApps);

  CLog::Log(LOGDEBUG,"%s - loaded %d media items and %d applications", __FUNCTION__, m_listMediaItems.Size(), m_listApps.Size());

  int i;
  for (i=0; i<m_listMediaItems.Size(); i++) 
  {
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), MEDIA_ITEMS_LIST, 0, 0, m_listMediaItems[i]);
    OnMessage(winmsg);
    CGUIMessage winmsg2(GUI_MSG_LABEL_ADD, GetID(), MEDIA_ITEMS_LIST_SMALL, 0, 0, m_listMediaItemsSmall[i]);
    OnMessage(winmsg2);
  }

  for (i=0; i<m_listApps.Size(); i++) 
  {
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), APPS_ITEMS_LIST, 0, 0, m_listApps[i]);
    OnMessage(winmsg);
    CGUIMessage winmsg2(GUI_MSG_LABEL_ADD, GetID(), APPS_ITEMS_LIST_SMALL, 0, 0, m_listAppsSmall[i]);
    OnMessage(winmsg2);
  }
  
  // Load thumbnails for the items
  CLog::Log(LOGDEBUG,"Loading thumbnail for %d items", m_listMediaItems.Size()); 
  m_listMediaItems.FillInDefaultIcons();


  m_Loader = GetLoader();
  CFileItemList *pList = new CFileItemList;
  pList->Append(m_listMediaItems);
  pList->Append(m_listApps);
  m_Loader->SetObserver(this);
  m_Loader->Load(*pList);
}

void CGUIWindowBoxeeMediaMain::OnItemLoaded(CFileItem* pItem) 
{
  if (!pItem)
    return;

  CGUIMessage winmsg(MSG_ITEM_LOADED, GetID(), 0);
  winmsg.SetPointer(new CFileItem(*pItem));
  g_windowManager.SendThreadMessage(winmsg, GetID());  
}

void CGUIWindowBoxeeMediaMain::OnMediaItemPlay(CFileItem *pItem) 
{
  OnMediaItemClick(pItem);
}

void CGUIWindowBoxeeMediaMain::OnAppsPopup(CFileItem *pItem) 
{
  CGUIDialogBoxeeAppCtx* pDlgInfo = (CGUIDialogBoxeeAppCtx*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_APP_CTX);
  if (!pDlgInfo || !pItem)
    return ;

  pDlgInfo->SetItem(*pItem);
  pDlgInfo->DoModal();
}

