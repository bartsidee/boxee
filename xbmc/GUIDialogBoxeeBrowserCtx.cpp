/*
* All Rights Reserved, Boxee.tv
*/

#include "GUIDialogBoxeeBrowserCtx.h"
#include "GUIWindowBoxeeMediaInfo.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "MouseStat.h"
#include "GUIEditControl.h"
#include "GUIWindowPointer.h"
#include "GUIListContainer.h"
#include "FileItem.h"
#include "GUIPanelContainer.h"
#include "utils/log.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogYesNo2.h"
#include "ItemLoader.h"
#include "lib/libBoxee/bxwebfavoritesmanager.h"
#include "lib/libBoxee/boxee.h"
#include "LocalizeStrings.h"
#include "GUIDialogOK.h"
#include "GUIDialogOK2.h"
#include "SpecialProtocol.h"

#define NOTIFICATION_APPEARANCE_IN_SEC   5000

#define HIDDEN_ITEM_ID 5000

#define CONTROLS_LIST 9000

#define BTN_PAUSE_ID 9105
#define BTN_STOP_ID  9104

#define ADDRESS_BAR_EDIT 9004
#define ADDRESS_BAR_EDIT_FAVORITES     19004
#define ADDRESS_BAR_EDIT_HISTORY       29004

#define LOCK_ICON                      9009
#define BTN_BACK_ID                    9005
#define BTN_FORWARD_ID                 9006

#define BTN_VOLUME_DOWN                9119
#define BTN_HISTORY                    9203
#define BTN_HISTORY_FAVORITES          19203
#define BTN_RELOAD                     9024
#define BTN_ADD_FAVORITE               9025
#define BTN_STOP                       9026
#define BTN_FAVORITE_ON                9027
#define BTN_FAVORITES                  9028
#define BTN_FAVORITES_HISTORY          29028
#define BTN_EXIT                       9029

#define FAVORITES_INFO                 9060
#define REMOVE_FAVORITES_GROUP         8010
#define BTN_REMOVE_FAVORITES           8011
#define BTN_DONE                       8012
#define BTN_CLEAR_HISTORY              8013
#define HOME_SCREEN                    7500
#define FAVORITES_LABEL                7501
#define HISTORY_LABEL                  7502
#define TOP_OSD                        7000
#define PAGE_LOAD_FAILURE              7600

#define BTN_EXIT                       9029

#define FAVORITES_CONTAINER            50
#define HISTORY_CONTAINER              51
#define FAVORITES_BG_CONTAINER         52

#define MAX_FAVORITES                  12

#define CONTROL_VIEW_START             50
#define CONTROL_VIEW_END               59

#define REMOVE_FAVORITES_MODE          "remove-favorites"
#define IS_FAVORITE_PROPERTY           "is-favorite"
#define HISTORY_EMPTY                  "history-empty"

void WebFavoriteJob::Run()
{
  if (m_bWebFavorite)
  {
    m_bJobResult = BoxeeUtils::AddWebFavorite(m_url, m_urlTitle, true);
    if (!m_bJobResult)
      CGUIDialogOK2::ShowAndGetInput(257, 55195);
  }
  else
  {
    m_bJobResult = BoxeeUtils::RemoveWebFavorite(m_id);
    if (!m_bJobResult)
      CGUIDialogOK2::ShowAndGetInput(257, 55195);
  }
}

CGUIDialogBoxeeBrowserCtx::CGUIDialogBoxeeBrowserCtx() : CGUIDialogBoxeeCtx(WINDOW_DIALOG_BOXEE_BROWSER_CTX, "boxee_browser_context.xml")
{
  m_itemsFavorites = new CFileItemList();
  m_itemsHistory = new CFileItemList();
  m_bMouseEnabled = true;
  m_bBrowserClosed = false;
}

void CGUIDialogBoxeeBrowserCtx::Update()
{
  CGUIDialogBoxeeCtx::Update();
}

bool CGUIDialogBoxeeBrowserCtx::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_SELECT_ITEM:
  {
    if (BrowseToAddressBarUrl())
    {
      return true;
    }
  }
  break;
  case ACTION_MOVE_DOWN:
  {
    CGUIControlGroup *c = (CGUIControlGroup *)GetControl(TOP_OSD);
    if (!m_item.GetPropertyBOOL("load-failed") && c && c->IsVisible())
    {
      Close();
    }
  }
  break;
  case ACTION_PREVIOUS_MENU:
  {

    CGUIControlGroup *c = (CGUIControlGroup*) GetControl(TOP_OSD);
    if (c->IsVisible())
    {
      bool cancelled = false;
      if (CGUIDialogYesNo2::ShowAndGetInput(54897,55404,54775,55406,cancelled,0,1))
      {
        g_application.StopPlaying();
        m_bBrowserClosed = true;
        return true;
      }

      if (m_item.GetPropertyBOOL("load-failed"))
      {
        return true;
      }

      Close();
    }
    else
    {
      CGUIControlGroup *ctrlHome = (CGUIControlGroup*) GetControl(HOME_SCREEN);
      if (ctrlHome->IsVisible())
      {
        SET_CONTROL_HIDDEN(HOME_SCREEN);
        SET_CONTROL_VISIBLE(TOP_OSD);
        SET_CONTROL_FOCUS(TOP_OSD, 0);
      }

      return true;
    }
  }
  break;
  }

  return CGUIDialogBoxeeCtx::OnAction(action);
}

bool CGUIDialogBoxeeBrowserCtx::OnMessage(CGUIMessage &message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_VISIBLE:
    {
      if (message.GetControlId() == TOP_OSD)
      {
        ManipulateAddressBar(m_item.m_strPath, true);
      }
    }
    break;
    case GUI_MSG_SETFOCUS:
    {
      if (message.GetControlId() == ADDRESS_BAR_EDIT)
      {
        CGUIEditControl *c = (CGUIEditControl *)GetControl(ADDRESS_BAR_EDIT);
        if (c)
        {
          c->SetHighlighted(true);
        }
      }
    }
    break;
    case GUI_MSG_CLICKED:
    {
      if (message.GetControlId() == WINDOW_DIALOG_BOXEE_BROWSER_CTX)
      {
        switch(message.GetSenderId())
        {
          case ADDRESS_BAR_EDIT:
          {
            // place holder
          }
          break;
          case BTN_EXIT:
          {
            g_application.StopPlaying();
            m_bBrowserClosed = true;
            return true;
          }
          break;
          case FAVORITES_CONTAINER:
          {
            CGUIListItemPtr ptrListItem = ((CGUIPanelContainer*) GetControl(FAVORITES_CONTAINER))->GetSelectedItemPtr();

            if (g_windowManager.GetWindow(g_windowManager.GetActiveWindow())->GetPropertyBOOL(REMOVE_FAVORITES_MODE))
            {
              OnRemoveWebFavorite(ptrListItem);
              SetEmptyForFavorites();
              SetFavoriteProperty();
            }
            else
            {
              OnGoToURL(ptrListItem);
            }
          }
          break;
          case HISTORY_CONTAINER:
          {
            CGUIListItemPtr ptrListItem = ((CGUIPanelContainer*) GetControl(HISTORY_CONTAINER))->GetSelectedItemPtr();
            OnGoToURL(ptrListItem);
          }
          break;
          case BTN_FAVORITES:
          case BTN_FAVORITES_HISTORY:
          {
            OnGoToFavorites();
          }
          break;
          case BTN_HISTORY:
          case BTN_HISTORY_FAVORITES:
          {
            OnGoToHistory();
          }
          break;
          case BTN_REMOVE_FAVORITES:
          {
            g_windowManager.GetWindow(g_windowManager.GetActiveWindow())->SetProperty(REMOVE_FAVORITES_MODE, true);
            SET_CONTROL_HIDDEN(BTN_REMOVE_FAVORITES);
            SET_CONTROL_VISIBLE(BTN_DONE);
            SET_CONTROL_FOCUS(FAVORITES_CONTAINER,0);
          }
          break;
          case BTN_DONE:
          {
            g_windowManager.GetWindow(g_windowManager.GetActiveWindow())->SetProperty(REMOVE_FAVORITES_MODE, false);
            SET_CONTROL_HIDDEN(BTN_DONE);
            SET_CONTROL_VISIBLE(BTN_REMOVE_FAVORITES);
            SET_CONTROL_FOCUS(BTN_REMOVE_FAVORITES,0);
          }
          break;
          case BTN_ADD_FAVORITE:
          {
            OnAddWebFavorite();
          }
          break;
          case BTN_RELOAD:
          {
            CAction action;
            action.id = ACTION_BROWSER_RELOAD;

            if (g_application.m_pPlayer)
            {
              g_application.m_pPlayer->OnAction(action);
              Close();
            }
          }
          break;
          case BTN_STOP:
          {
            CAction action;
            action.id = ACTION_BROWSER_STOP_LOADING;

            if (g_application.m_pPlayer)
            {
              g_application.m_pPlayer->OnAction(action);
              Close();
            }
          }
          break;
          case BTN_CLEAR_HISTORY:
          {
            CGUIDialogYesNo* dlgYesNo = (CGUIDialogYesNo*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

            if (dlgYesNo)
            {
              dlgYesNo->SetHeading(55403);
              dlgYesNo->SetLine(0, 53415);
              dlgYesNo->DoModal();

              if (dlgYesNo->IsConfirmed())
              {
                m_itemsHistory->Clear();
                g_application.GetBoxeeBrowseHistoryList().ClearAllHistory();

                CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), HISTORY_CONTAINER, 1, 0, m_itemsHistory);
                g_windowManager.SendMessage(msg);
                SetEmptyForHistory();
              }
            }
            LoadHistory();
          }
        }
      }
    }
    break;
    case GUI_MSG_WINDOW_DEINIT:
    {
      if (m_bBrowserClosed)
      {
        g_Mouse.SetEnabled(g_Mouse.IsEnabledInSettings());
      }
      else
      {
        g_Mouse.SetEnabled(m_bMouseEnabled);
      }
    }
    break;
  }
  return CGUIDialogBoxeeCtx::OnMessage(message);
}

void CGUIDialogBoxeeBrowserCtx::Render()
{
  CGUIDialogBoxeeCtx::Render();
}

void CGUIDialogBoxeeBrowserCtx::OnInitWindow()
{  
  m_bBrowserClosed = false;
  CFileItemPtr pItem (new CFileItem(m_item));
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_ITEM_ID, 0);
  OnMessage(msg);
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_ITEM_ID, 0, 0, pItem);
  OnMessage(winmsg);

  m_bMouseEnabled = g_Mouse.IsEnabled();

  CGUIDialogBoxeeCtx::OnInitWindow();
  
  g_Mouse.SetEnabled(false);
  g_Mouse.SetActive(false);
  SetProperty("PassthroughKeys", true);
  m_autoClosing = false;
  
  CGUIMessage msgreset(GUI_MSG_LABEL_RESET, GetID(), CONTROLS_LIST, 0);
  OnMessage(msgreset);

  if (g_application.m_pPlayer && g_application.CurrentFileItem().GetPropertyBOOL("browsercansetfullscreen"))
  {
    SET_CONTROL_VISIBLE(CONTROLS_LIST);    
    SET_CONTROL_FOCUS(CONTROLS_LIST, 0);    
  }
  else
  {
    SET_CONTROL_VISIBLE(ADDRESS_BAR_EDIT);    
    SET_CONTROL_FOCUS(ADDRESS_BAR_EDIT, 0);    
  }

  LoadFavorites();

  SET_CONTROL_HIDDEN(HOME_SCREEN);
  SET_CONTROL_VISIBLE(TOP_OSD);

  SET_CONTROL_VISIBLE(FAVORITES_CONTAINER);
  SET_CONTROL_HIDDEN(HISTORY_CONTAINER);

  SET_CONTROL_HIDDEN(HISTORY_LABEL);
  SET_CONTROL_VISIBLE(FAVORITES_LABEL);

  SetFavoriteProperty();

  if (pItem->GetPropertyBOOL("launch-as-app"))
  {
    // launch as app -> open in favorites screen
    OnGoToFavorites();
  }
}

bool CGUIDialogBoxeeBrowserCtx::BrowseToAddressBarUrl()
{
  int focusedControlId = GetFocusedControlID();
  CStdString url;

  CGUIEditControl* c = NULL;

  switch (focusedControlId)
  {
  case ADDRESS_BAR_EDIT:
  {
    c = (CGUIEditControl*)GetControl(ADDRESS_BAR_EDIT);
  }
  break;
  case ADDRESS_BAR_EDIT_FAVORITES:
  {
    c = (CGUIEditControl*)GetControl(ADDRESS_BAR_EDIT_FAVORITES);
  }
  break;
  case ADDRESS_BAR_EDIT_HISTORY:
  {
    c = (CGUIEditControl*)GetControl(ADDRESS_BAR_EDIT_HISTORY);
  }
  break;
  default:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeBrowserCtx::BrowseToAddressBarUrl - focused control ISN'T editbox -> return FALSE. [%d]\n",focusedControlId);
    return false;
  }
  }

  if (!c)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeBrowserCtx::BrowseToAddressBarUrl - FAILED to get EditControl -> return FALSE.");
    return false;
  }

  url = c->GetLabel2();

  if (url.IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeBrowserCtx::BrowseToAddressBarUrl - url is EMPTY [%s] -> return FALSE.",url.c_str());
    return false;
  }

  CGUIListItemPtr pItem(new CFileItem(url,false));
  OnGoToURL(pItem);

  return true;
}

void CGUIDialogBoxeeBrowserCtx::LoadFavorites()
{
  m_itemsFavorites->Clear();

  std::vector<BOXEE::BXObject> webFavoritesVec;

  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetWebFavorites(webFavoritesVec);

  std::vector<BOXEE::BXObject>::iterator it;

  for( it = webFavoritesVec.begin(); it != webFavoritesVec.end(); it++ )
  {
    CStdString url = it->GetValue(MSG_KEY_URL);
    CStdString urlTitle = it->GetValue(MSG_KEY_TITLE);
    CUtil::UrlDecode(url);
    CUtil::UrlDecode(urlTitle);

    CFileItemPtr pItem(new CFileItem(urlTitle));
    pItem->SetLabel2(url);
    pItem->m_strPath = url;
    pItem->SetProperty(MSG_KEY_BOOKMARK_INDEX, it->GetValue(MSG_KEY_BOOKMARK_INDEX));
    pItem->SetThumbnailImage(pItem->GetCachedVideoThumb().c_str());

    m_itemsFavorites->Add(pItem);
  }

  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), FAVORITES_CONTAINER, 1, 0, m_itemsFavorites);
  g_windowManager.SendMessage(msg2);

}

void CGUIDialogBoxeeBrowserCtx::LoadHistory()
{
  m_itemsHistory->Clear();
  g_application.GetBoxeeBrowseHistoryList().GetFilesHistory(*m_itemsHistory);
  CGUIMessage msg3(GUI_MSG_LABEL_BIND, GetID(), HISTORY_CONTAINER, 1, 0, m_itemsHistory);
  g_windowManager.SendMessage(msg3);
}

void CGUIDialogBoxeeBrowserCtx::OnAddWebFavorite()
{
  std::vector<BOXEE::BXObject> webFavoritesVec;

  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetWebFavorites(webFavoritesVec);

  if (m_itemsFavorites->Size() < MAX_FAVORITES)
  {
    CStdString urlTitle = m_item.GetLabel();
    CStdString url = m_item.m_strPath;

    CAction action;
    action.id = ACTION_BROWSER_THUMBNAIL_GENERATE;
    action.amount1 = 320;
    action.amount2 = 180;
    action.strAction = _P(m_item.GetCachedVideoThumb().c_str()).c_str();

    if (g_application.m_pPlayer)
      g_application.m_pPlayer->OnAction(action);

    CUtil::URLEncode(urlTitle);
    CUtil::URLEncode(url);

    CFileItemPtr pItem(new CFileItem(urlTitle));
    pItem->m_strPath = url;
    pItem->SetThumbnailImage(m_item.GetCachedVideoThumb().c_str());

    WebFavoriteJob* job = new WebFavoriteJob(url, urlTitle, true);

    CUtil::RunInBG(job);
    m_itemsFavorites->Add(pItem);
    CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), FAVORITES_CONTAINER, 1, 0, m_itemsFavorites);
    g_windowManager.SendMessage(msg2);
  }
  else
  {
    CStdString titleStr = g_localizeStrings.Get(55408);
    CStdString textStr = g_localizeStrings.Get(55409) + "[CR]" + g_localizeStrings.Get(55410);
    CGUIDialogOK2::ShowAndGetInput(titleStr, textStr);
  }

  LoadFavorites();

  SetFavoriteProperty();

  SET_CONTROL_VISIBLE(BTN_FAVORITE_ON);
  SET_CONTROL_FOCUS(BTN_FAVORITE_ON,0);
}

void CGUIDialogBoxeeBrowserCtx::SetFavoriteProperty()
{
  if (m_itemsFavorites->Contains(m_item.m_strPath))
  {
    SetProperty(IS_FAVORITE_PROPERTY, true);
  }
  else
  {
    SetProperty(IS_FAVORITE_PROPERTY, false);
  }
}

void CGUIDialogBoxeeBrowserCtx::OnRemoveWebFavorite(CGUIListItemPtr ptrListItem)
{
  CGUIDialogYesNo2* dlgYesNo = (CGUIDialogYesNo2*)g_windowManager.GetWindow(WINDOW_DIALOG_YES_NO);

 if (dlgYesNo)
 {
   unsigned long bookmarkIndex;
   CFileItem* selectedFileItem;

   if (ptrListItem->IsFileItem())
   {
     selectedFileItem = (CFileItem*) ptrListItem.get();
     bookmarkIndex = selectedFileItem->GetPropertyULong(MSG_KEY_BOOKMARK_INDEX);
   }
   else
   {
     CGUIDialogOK::ShowAndGetInput(257,55411,20022,20022);
     return;
   }

   CStdString urlTitle = selectedFileItem->GetLabel();
   CStdString url = selectedFileItem->m_strPath;

   CStdString strHeading = g_localizeStrings.Get(55403);
   CStdString strText = g_localizeStrings.Get(55407) + "[CR]" + urlTitle + "?";

   if (dlgYesNo->ShowAndGetInput(strHeading, strText))
   {
     WebFavoriteJob* job = new WebFavoriteJob(url, urlTitle, false, bookmarkIndex);
     CUtil::RunInBG(job);

     BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateWebFavoritesListNow();

     m_itemsFavorites->Remove(selectedFileItem);
     CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), FAVORITES_CONTAINER, 1, 0, m_itemsFavorites);
     g_windowManager.SendMessage(msg);
   }
 }
 else
   return;
}

void CGUIDialogBoxeeBrowserCtx::OnMoreInfo()
{
  g_windowManager.CloseDialogs(true);
  CGUIWindowBoxeeMediaInfo::Show(&m_item);
}

void CGUIDialogBoxeeBrowserCtx::OnGoToURL(CGUIListItemPtr ptrListItem)
{
  CAction action;
  action.id = ACTION_BROWSER_NAVIGATE;
  CStdString url;
  if (ptrListItem->IsFileItem())
  {
    CFileItem* selectedFileItem = (CFileItem*) ptrListItem.get();
    url = selectedFileItem->m_strPath;
  }
  action.strAction = url;
  if (g_application.m_pPlayer)
  {
    g_application.m_pPlayer->OnAction(action);
    Close();
  }
}

void CGUIDialogBoxeeBrowserCtx::SetItem(const CFileItem &item)
{
  m_item = item;  
  
  CFileItemPtr pItem (new CFileItem(m_item));
  
  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_ITEM_ID, 0);
  OnMessage(msg);
  
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_ITEM_ID, 0, 0, pItem);
  OnMessage(winmsg);
  
  ManipulateAddressBar(m_item.m_strPath, true);

  if (m_item.GetPropertyBOOL("load-failed"))
  {
    SetProperty("browser-page-load-failed", true);
    CONTROL_DISABLE(BTN_ADD_FAVORITE);
  }
  else
  {
    SetProperty("browser-page-load-failed", false);
    CONTROL_ENABLE(BTN_ADD_FAVORITE);
  }

  SetState();
}

void CGUIDialogBoxeeBrowserCtx::OnGoToHistory()
{
  LoadHistory();

  SET_CONTROL_VISIBLE(HOME_SCREEN);
  SET_CONTROL_HIDDEN(TOP_OSD);
  SET_CONTROL_HIDDEN(FAVORITES_CONTAINER);
  SET_CONTROL_VISIBLE(HISTORY_CONTAINER);
  SET_CONTROL_HIDDEN(REMOVE_FAVORITES_GROUP);
  SET_CONTROL_HIDDEN(FAVORITES_INFO);
  SET_CONTROL_VISIBLE(HISTORY_LABEL);
  SET_CONTROL_HIDDEN(FAVORITES_LABEL);
  SET_CONTROL_VISIBLE(BTN_CLEAR_HISTORY);
  SET_CONTROL_HIDDEN(FAVORITES_BG_CONTAINER);

  SetEmptyForHistory();
}

void CGUIDialogBoxeeBrowserCtx::OnGoToFavorites()
{
  LoadFavorites();

  g_windowManager.GetWindow(g_windowManager.GetActiveWindow())->SetProperty(REMOVE_FAVORITES_MODE, false);

  SET_CONTROL_HIDDEN(BTN_DONE);
  SET_CONTROL_VISIBLE(HOME_SCREEN);
  SET_CONTROL_HIDDEN(TOP_OSD);
  SET_CONTROL_HIDDEN(HISTORY_CONTAINER);
  SET_CONTROL_VISIBLE(FAVORITES_CONTAINER);
  SET_CONTROL_VISIBLE(REMOVE_FAVORITES_GROUP);
  SET_CONTROL_VISIBLE(FAVORITES_INFO);
  SET_CONTROL_HIDDEN(HISTORY_LABEL);
  SET_CONTROL_VISIBLE(FAVORITES_LABEL);
  SET_CONTROL_HIDDEN(BTN_CLEAR_HISTORY);
  SET_CONTROL_VISIBLE(BTN_REMOVE_FAVORITES);
  SET_CONTROL_VISIBLE(FAVORITES_BG_CONTAINER);

  SetEmptyForFavorites();
}

void CGUIDialogBoxeeBrowserCtx::SetEmptyForFavorites()
{
  CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (m_itemsFavorites->Size()>0)
  {
    activeWindow->SetProperty("empty", false);
    SET_CONTROL_FOCUS(FAVORITES_CONTAINER, 0);
  }
  else
  {
    activeWindow->SetProperty("empty", true);
    SET_CONTROL_FOCUS(ADDRESS_BAR_EDIT_FAVORITES, 0);
  }
}

void CGUIDialogBoxeeBrowserCtx::SetEmptyForHistory()
{
  CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (m_itemsHistory->Size()>0)
  {
    activeWindow->SetProperty("empty", false);
    SET_CONTROL_FOCUS(HISTORY_CONTAINER, 0);
  }
  else
  {
    activeWindow->SetProperty("empty", true);
    SET_CONTROL_FOCUS(ADDRESS_BAR_EDIT_HISTORY, 0);
  }
}

bool CGUIDialogBoxeeBrowserCtx::ManipulateAddressBar(const CStdString &label, bool SetHighlighted)
{
  SET_CONTROL_HIDDEN(LOCK_ICON);
  CGUIEditControl *c = (CGUIEditControl *)GetControl(ADDRESS_BAR_EDIT);
  if(c)
  {
    CStdString strPath = label;
    c->SetHighlighted(SetHighlighted);
    //remove "http://"
    if(strPath.Left(7) == "http://")
    {
      strPath = strPath.substr(7);
    }
    //remove "https://"
    if(strPath.Left(8) == "https://")
    {
      SET_CONTROL_VISIBLE(LOCK_ICON);
      strPath = strPath.substr(8);
      //shorten path
      int end = strPath.Find("/");
      strPath = strPath.substr(0,end);
    }
    c->SetLabel2(strPath);
    return true;
  }
  return false;
}

void CGUIDialogBoxeeBrowserCtx::SetState()
{
}
