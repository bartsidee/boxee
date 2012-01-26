
#include "Application.h"
#include "GUIWindowBoxeeBrowse.h"
#include "PlayListPlayer.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "URL.h"
#include "GUIListContainer.h"
#include "utils/LabelFormatter.h"
#include "FileSystem/File.h"
#include "GUIDialogOK.h"
#include "utils/GUIInfoManager.h"
#include "ThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/Directory.h"
#include "GUIBoxeeViewStateFactory.h"
#include "GUIImage.h"
#include "BoxeeViewDatabase.h"
#include "SpecialProtocol.h"
#include "BoxeeItemLauncher.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIPassword.h"
#include "FileSmb.h"
#include "VideoInfoTag.h"

#include "utils/log.h"

#include <vector>

// Mark the range of view controls
#define CONTROL_VIEW_START        50
#define CONTROL_VIEW_END          59
// Mark the view selection control
#define CONTROL_BTNVIEWASICONS     2


#define LABEL_WINDOW_TITLE 820
#define IMAGE_WINDOW_TITLE 810


using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

CGUIWindowBoxeeBrowse::CGUIWindowBoxeeBrowse()
: CGUIWindow(WINDOW_BOXEE_BROWSE, "BoxeeBrowseWindow.xml")
{
  InitializeWindowState();
}

CGUIWindowBoxeeBrowse::CGUIWindowBoxeeBrowse(DWORD dwID, const CStdString &xmlFile)
: CGUIWindow(dwID, xmlFile)
{
  InitializeWindowState();
}

void CGUIWindowBoxeeBrowse::InitializeWindowState()
{
  m_windowState = NULL;
  SetWindowState(new CBrowseWindowState(this));
  m_iLastControl = -1;

  // Initialize configuration
  CBrowseWindowConfiguration::CreateFilters();

  m_boxeeViewState.reset(CGUIBoxeeViewStateFactory::GetBoxeeViewState(GetID(), m_vecViewItems));
}

void CGUIWindowBoxeeBrowse::SetWindowState(CBrowseWindowState* pWindowState)
{
  if (m_windowState)
    delete m_windowState;

  m_windowState = pWindowState;
}

CGUIWindowBoxeeBrowse::~CGUIWindowBoxeeBrowse()
{
  delete m_windowState;
}

void CGUIWindowBoxeeBrowse::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  if (m_iLastControl == -1)
    m_iLastControl = m_defaultControl;
  
  // Clear existing background image
  ClearProperty("BrowseBackgroundImage");

  m_windowState->OnLoading();

  CFileItem baseItem;
  baseItem.m_strPath = m_windowState->CreatePath();
  baseItem.SetProperty("BrowseBackgroundImage", m_windowState->GetBackgroundImage());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnInitWindow, initial path = %s (browse)", baseItem.m_strPath.c_str());
  UpdatePath(&baseItem, false);

  m_vecModelItems.m_strPath = m_windowState->GetCurrentPath();

  m_windowState->UpdateWindowProperties();

  // TODO: Fix window icon
  // Set window icon
  //  const CGUIControl* pControl = GetControl(IMAGE_WINDOW_TITLE);
  //  if (pControl)
  //  {
  //    CGUIImage* pImageControl = (CGUIImage*)pControl;
  //    pImageControl->FreeResources();
  //    pImageControl->SetFileName(m_configuration.m_strTitleIcon);
  //  }

  // Restore control focus
  SET_CONTROL_FOCUS(m_iLastControl, 0);

  SetProperty("empty", false);
  SetWindowTitle(m_windowState->GetLabel());
}

bool CGUIWindowBoxeeBrowse::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_MOUSE:
  case ACTION_MOVE_LEFT:
  case ACTION_MOVE_RIGHT:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  {
    // pass the action onward for handling
    bool bResult = CGUIWindow::OnAction(action);

    // check current position, after the action was performed, to determine whether to load the next page
    int iSelectedItem = m_viewControl.GetSelectedItem();

    m_windowState->SetSelectedItem(iSelectedItem);

    return bResult;
  }
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    OnBack();
    return true;
  }

  } // switch
  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeBrowse::OnBind(CGUIMessage& message)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBind, sender = %d, control = %d (browse)", message.GetSenderId(), message.GetControlId());

  SetProperty("loading", false);

  // Check if control id is 0, meaning that this message is from the item loader
  // Otherwise this message is destined for the specific view control
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    CFileItemList *items = (CFileItemList *)message.GetPointer();

    if (items->m_strPath != m_windowState->GetCurrentPath())
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBind, dump items, path = %s, items path = %s (browse)",
          items->m_strPath.c_str(), m_windowState->GetCurrentPath().c_str());
      // The items that arrived do not match the current path, disregard them
      delete items;
      return true;
    }

    // Check whether additional credentials are required from the user
    if (items->GetPropertyBOOL("getcredentials"))
    {
      g_passwordManager.SetSMBShare(items->m_strPath);
      if (g_passwordManager.GetSMBShareUserPassword())
      {
        /* must do this as our urlencoding for spaces is invalid for samba */
        /* and doing double url encoding will fail */
        /* curl doesn't decode / encode filename yet */
        CURL urlnew( g_passwordManager.GetSMBShare() );
        CStdString newPath = BoxeeUtils::URLEncode(urlnew);
        CUtil::UrlDecode(newPath); // we have to decode the url before it is sent to the loader

        SetProperty("loading", true);
        m_windowState->SetPath(newPath);
        g_application.GetItemLoader().AddControl(GetID(), 0, newPath);
        delete items;
        return true;
      }
    }

    if (HasAppData())
    {
      for (int i=0; i<items->Size(); i++)
      {
        SetItemWithAppData(*((*items)[i]));
      }
    }
    
    // Set model items as received from the loader
    m_vecModelItems.Assign(*items);

    if (items->HasPageContext())
    {
      CPageContext pageContext = items->GetPageContext();
      m_vecModelItems.SetPageContext(pageContext);
    }

    // Set new item path
    m_vecModelItems.m_strPath = items->m_strPath;
    m_vecModelItems.SetProperty("parentPath", items->GetProperty("parentPath"));

    //Update background image, if any
    if (items->HasProperty("BrowseBackgroundImage"))
    {
      m_windowState->SetBackgroundImage(items->GetProperty("BrowseBackgroundImage"));
    }

    if(items->HasProperty("ServerViewAndSort"))
    {
      m_vecModelItems.SetProperty("ServerViewAndSort", items->GetProperty("ServerViewAndSort"));

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBind, After setting ServerViewAndSort property [%s] from items  (browse)",(m_vecModelItems.GetProperty("ServerViewAndSort")).c_str());
    }
    else
    {
      m_vecModelItems.SetProperty("ServerViewAndSort","");
    }

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBind, items before processing, path = %s, (%d items) (browse) (paging)",
        m_vecModelItems.m_strPath.c_str(), m_vecModelItems.Size());

    // Send model items to the state for processing
    m_windowState->ProcessItems(m_vecModelItems, m_vecViewItems);

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBind, items after processing, path = %s, (%d items) (browse) (paging)",
        m_vecModelItems.m_strPath.c_str(), m_vecViewItems.Size());

    if (items->Size() == 0)
    {
      SetProperty("empty", true);
    }
    else
    {
      SetProperty("empty", false);
    }

    // We are responsible to delete the items we have received in this message
    delete items;

    // Update view and sort configuration
    // IMPORTANT NOTE: The "browsemode" property, a hidden parameter to the GetBoxeeViewState function, that is used
    // by the CGUIBoxeeViewStateFactory to get appropriate view state
    // is set in the CGUIWindowBoxeeBrowse specific implementation of the ProcessItems function
    m_boxeeViewState.reset(CGUIBoxeeViewStateFactory::GetBoxeeViewState(GetID(), m_vecViewItems));

    // Update previous / next item labels and set items to the controller
    SetViewItems(m_vecViewItems);

    // Update current view type
    if (m_boxeeViewState.get())
    {
      m_viewControl.SetCurrentView(m_boxeeViewState->GetViewType());
    }

    // Update selected item
    m_viewControl.SetSelectedItem(m_windowState->GetSelectedItem());

    UpdateIndexMaps();

    OnLoaded();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, got items for path = %s (browse)", m_vecModelItems.m_strPath.c_str());

    return true;
  }

  if (m_vecViewItems.Size() == 0)
  {
    SetProperty("empty", true);
  }
  else
  {
    SetProperty("empty", false);
  }

  return false;
}

bool CGUIWindowBoxeeBrowse::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_UPDATE:
  {
    Refresh();
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    // Check if path was set in the initialization message
    CStdString strPath = message.GetStringParam();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage,GUI_MSG_WINDOW_INIT, initial path = %s, (browse)", strPath.c_str());

    if (strPath != "")
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_WINDOW_INIT, creating new configuration for path = %s  (browse)", strPath.c_str());

      ClearFileItems();
      ResetHistory();

      m_windowState->InitState(strPath);
    }

    CGUIWindow::OnMessage(message);
    return true;
  }
  case GUI_MSG_WINDOW_DEINIT:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_WINDOW_DEINIT (browse)");

    m_windowState->SetSelectedItem(m_viewControl.GetSelectedItem());

    m_iLastControl = GetFocusedControlID();
    CGUIWindow::OnMessage(message);
    ClearFileItems();
    ResetAppData();
    return true;
  }
  case GUI_MSG_CLICKED:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_CLICKED, action = %d (browse)", message.GetParam1());
    int iControl = message.GetSenderId();
    if (m_viewControl.HasControl(iControl))  // list/thumb control
    {
      // Get the selected item from the view
      int iItem = m_viewControl.GetSelectedItem();
      int iAction = message.GetParam1();
      if (iItem < 0) break;
      if (iAction == ACTION_SELECT_ITEM || iAction == ACTION_MOUSE_LEFT_CLICK)
      {
        return OnClick(iItem);
      }
    }
    break;
  }
  case GUI_MSG_CHANGE_SORT_METHOD:
  {    
    CFileItem* fileItem = NULL;
    fileItem = (CFileItem*)message.GetPointer();

    if(fileItem)
    {
      CStdString sortId = fileItem->GetProperty("sortid");
      m_boxeeViewState->SetSortById(sortId);

      UpdateFileList();

      m_boxeeViewState->SaveBoxeeViewState();
    }

    return true;
  }
  break;
  case GUI_MSG_CHANGE_VIEW_MODE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_CHANGE_VIEW_MODE, set view to %d (browse)", message.GetParam1());

    int viewMode = 0;

    CFileItem* fileItem = NULL;
    fileItem = (CFileItem*)message.GetPointer();

    if(fileItem)
    {
      if(fileItem->HasProperty("viewType"))
      {
        // Set specific view type
        viewMode = fileItem->GetPropertyInt("viewType");
      }
    }
    else if (message.GetParam2() == 1 || message.GetParam2() == -1)
    {
      // Set next or previous view depending on message param
      viewMode = m_viewControl.GetNextViewID((int)message.GetParam2());
    }
    else
    {
      viewMode = message.GetParam1();
    }

    if (m_boxeeViewState.get())
    {
      m_boxeeViewState->SetViewType(viewMode);
      m_boxeeViewState->SaveBoxeeViewState();
    }

    m_viewControl.SetCurrentView(viewMode);

    return true;
  }
  break;
  case GUI_MSG_SET_CONTAINER_PATH:
  {
    // Path was set by external source
    CFileItem *pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);
    if (!pItem)
      return true;

    m_windowState->OnLoading();

    if (!pItem->GetProperty("appid").IsEmpty())
    {
      SetAppData(pItem->GetProperty("appid"), pItem->GetProperty("provider"));
    }
    else
    {
      ResetAppData();
    }

    UpdateHistory();
    UpdatePath(pItem, true);

    UpdateLabel(pItem);

    delete pItem;

    return true;
  }
  break;
  case GUI_MSG_SETFOCUS:
  {
    if (m_viewControl.HasControl(message.GetControlId()) && m_viewControl.GetCurrentControl() != message.GetControlId())
    {
      m_viewControl.SetFocused();
      return true;
    }
  }
  break;
  case GUI_MSG_ITEM_LOADED:
  {    
    // New data received from the item loader, update existing item
    CFileItem *pItem = (CFileItem *)message.GetPointer();
    message.SetPointer(NULL);

    // We don't handle a broadcast message that was sent from this window in order to avoid loop.
    if((message.GetSenderId() == 0) && (message.GetControlId() == 0) && (message.GetParam1() == GetID()))
    {
      return true;      
    }

    if (pItem)
    {
      OnItemLoaded(pItem);
    }

    return true;
  }
  case GUI_MSG_LOAD_FAILED:
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_LOAD_FAILED (browse)");

    int iErrorOccured = message.GetParam1();

    if (iErrorOccured == 1)
    {
      // In case the ErrorOccured param was set to 1, an error occur and
      // we want to show an error dialog.

      CFileItem *pItem = (CFileItem *)message.GetPointer();
      OnLoadFailed(pItem);
    }
    else
    {
      // In case the ErrorOccured param was set to 0, we used this message
      // to signal an ESC hit. As a result we want to return to the previous screen.

      OnBack();
    }

    return true;
  }
  case GUI_MSG_LOADING:
  {
    // Check that control id is zero to prevent recursive calls
    if (message.GetControlId() == 0)
    {
      SetProperty("loading", true);
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, 4 GUI_MSG_LOADING, window = %d, control = %d (basecontainer)", GetID(), m_viewControl.GetCurrentControl());
//      CGUIMessage msg(GUI_MSG_LOADING, GetID(), m_viewControl.GetCurrentControl());
//      g_windowManager.SendMessage(msg);
      return true;
    }
    break;
  }
  case GUI_MSG_BG_PICTURE:
  {
    CStdString strBGImage = message.GetStringParam();
    SetProperty("BrowseBackgroundImage", strBGImage.c_str());
    m_windowState->SetBackgroundImage(strBGImage);

    return true;
  }
  case GUI_MSG_LABEL_BIND:
  {
    if (OnBind(message))
      return true;

    // otherwise, propagate the mesage to parent

    break;
  }

  } // switch

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeBrowse::UpdateIndexMaps()
{
  CFileItemPtr pItem;
  // Initialize maps to allow for fast access during update
  for (int i = 0; i < m_vecModelItems.Size(); i++)
  {
    pItem = m_vecModelItems.Get(i);
    m_itemsIndex[pItem->GetProperty("itemid")] = pItem;
  }

  for (int i = 0; i < m_vecViewItems.Size(); i++)
  {
    pItem = m_vecViewItems.Get(i);
    m_filteredItemsIndex[pItem->GetProperty("itemid")] = pItem;
  }
}

static bool IsShowSortedDescending(const CFileItemList &items, int iCurrentItemIdx)
{
	CVideoInfoTag *pItemToCheckVideoInfoTag = NULL; 
	CVideoInfoTag *pVideoInfoTag = NULL;;
	CFileItemPtr pItemToCheck;
	CFileItemPtr pItem;
	int iSize = items.Size();
	bool bResult = false;
	
	if(iSize < 2)
		return false;

	if(iCurrentItemIdx < iSize - 1 )
	{
		pItem = items[iCurrentItemIdx];
		pItemToCheck = items[iCurrentItemIdx + 1]; 
	}
	else 
	{
		pItem = items[iCurrentItemIdx - 1];
		pItemToCheck = items[iCurrentItemIdx];
	}

	if(pItem && pItem->HasVideoInfoTag())
		pVideoInfoTag = pItem->GetVideoInfoTag();


	if(pItemToCheck && pItemToCheck->HasVideoInfoTag()) 
		pItemToCheckVideoInfoTag = pItemToCheck->GetVideoInfoTag();
	
	if(pVideoInfoTag && pItemToCheckVideoInfoTag)
	{
		if (pVideoInfoTag->m_iSeason			!= -1	&& pVideoInfoTag->m_iEpisode			!= -1 && 
			pItemToCheckVideoInfoTag->m_iSeason != -1   && pItemToCheckVideoInfoTag->m_iEpisode != -1)
		{
			//
			// same show and different episodes
			//
			if(pVideoInfoTag->m_strShowTitle == pItemToCheckVideoInfoTag->m_strShowTitle)
			{
				if( (pItemToCheckVideoInfoTag->m_iSeason + 1000) * pItemToCheckVideoInfoTag->m_iEpisode < 
					(pVideoInfoTag->m_iSeason + 1000) * pVideoInfoTag->m_iEpisode)	
				{
					bResult = true;
				}
			}
		}
	}

	return bResult;
}

void CGUIWindowBoxeeBrowse::SetViewItems(CFileItemList &items)
{
  int iSize = items.Size();

  for (int i = 0; i < iSize; i++)
  {
    CFileItemPtr pItem=items[i];

    // reset prev and next items before assign
    pItem->ResetPrevAndNextItems();

    if (i>0)
    {
      CFileItemPtr prevItem;
      int prevPos = i;
      bool foundValidPrevItem = false;

      while (prevPos > 0 && !foundValidPrevItem)
      {

		if(pItem->HasVideoInfoTag() && IsShowSortedDescending(items, i)) 
		{
			const CVideoInfoTag *pVideoInfoTag = pItem->GetVideoInfoTag();
			
			if(pVideoInfoTag)
			{
				int iCurrentPos = (prevPos + 1) % iSize;

				//
				// first item ==> no prev
				//
				if (iCurrentPos == 0)
					break;

				prevItem = items[iCurrentPos];			
			}

		} 
		
		if(!prevItem) 
		{
			prevItem = items[prevPos - 1];
		}

        if (prevItem && !prevItem->m_bIsFolder && (!prevItem->m_strPath.IsEmpty() || prevItem->HasLinksList()))
        {
          // found a valid prev item
          foundValidPrevItem = true;
        }

        prevPos--;
      }


      if (foundValidPrevItem)
      {
        pItem->SetProperty("PrevItemLabel",prevItem->GetLabel());
        pItem->SetProperty("PrevItemLabel2",prevItem->GetLabel2());
        pItem->SetProperty("PrevItemPath",prevItem->m_strPath);
        pItem->SetProperty("HasPrevItem",true);

        pItem->SetPrevItem(prevItem);
      }
    }

    if (i<iSize)
    {
      CFileItemPtr nextItem;
      int nextPos = i;
      bool foundValidNextItem = false;

      while ((nextPos < items.Size()) && !foundValidNextItem)
      {
		if(pItem->HasVideoInfoTag() && IsShowSortedDescending(items, i)) 
		{
			const CVideoInfoTag *pVideoInfoTag = pItem->GetVideoInfoTag();
			
			if(pVideoInfoTag)
			{	
				int iCurrentPos = nextPos-1;

				//
				// last item ==> no next
				//
				if (iCurrentPos == 0) 
					break;

				nextItem = items[iCurrentPos];			
			}

		}

		if(!nextItem) 
		{
			nextItem = items[nextPos + 1];
		}

        if (nextItem && !nextItem->m_bIsFolder && (!nextItem->m_strPath.IsEmpty() || nextItem->HasLinksList()))
        {
          // found a valid next item
          foundValidNextItem = true;
        }

        nextPos++;
      }

      if (foundValidNextItem)
      {
        pItem->SetProperty("NextItemLabel",nextItem->GetLabel());
        pItem->SetProperty("NextItemLabel2",nextItem->GetLabel2());
        pItem->SetProperty("NextItemPath",nextItem->m_strPath);
        pItem->SetProperty("HasNextItem",true);

        pItem->SetNextItem(nextItem);
      }
    }
  }

  m_viewControl.SetItems(items);
}

void CGUIWindowBoxeeBrowse::ClearView()
{
  m_vecViewItems.Clear();
  SetViewItems(m_vecViewItems);
}

void CGUIWindowBoxeeBrowse::OnItemLoaded(const CFileItem* pItem)
{
  // Check whether the item belongs to the same directory that is currently being loaded
  if (!pItem->HasProperty("directoryPath") || pItem->GetProperty("directoryPath") != m_vecModelItems.m_strPath)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnItemLoaded, NEWUI, discard item, current path = %s, item path = %s (browse)",
        m_vecModelItems.m_strPath.c_str(), pItem->m_strPath.c_str());
    delete pItem;
    return;
  }

  CStdString itemId = pItem->GetProperty("itemid");
  if (itemId == m_strInActionItemId)
  {    
    // Inform media action dialog about the updated item. We add the GetID() as param in order to avoid loop
    // WARNING: media action dialog is responsible for deleting the new item allocated here
    CGUIMessage winmsg(GUI_MSG_ITEM_LOADED, 0, 0, GetID(), 0);
    winmsg.SetPointer(new CFileItem(*pItem));
    g_windowManager.SendThreadMessage(winmsg, 0);
  }

  CFileItemPtr pCurrentItem = m_itemsIndex[itemId];
  if (pCurrentItem)
  {
    // If the new item does not have a thumbnail, preserve the one we have now
    CStdString strPreviousIcon;
    strPreviousIcon = pCurrentItem->GetIconImage();

    // Copy the entire item
    *pCurrentItem = *pItem;

    // If the new item does not have an icon, restore the old one, unless it is a loading item
    if (!pCurrentItem->HasIcon() && !pCurrentItem->GetPropertyBOOL("IsLoading")) {
      pCurrentItem->SetIconImage(strPreviousIcon);
    }
  }

  pCurrentItem = m_filteredItemsIndex[itemId];
  if (pCurrentItem)
  {
    // If the new item does not have a thumbnail, preserve the one we have now
    CStdString strPreviousIcon;
    strPreviousIcon = pCurrentItem->GetIconImage();

    // Copy the entire item
    *pCurrentItem = *pItem;

    // If the new item does not have an icon, restore the old one, unless it is a loading item
    if (!pCurrentItem->HasIcon() && !pCurrentItem->GetPropertyBOOL("IsLoading")) {
      pCurrentItem->SetIconImage(strPreviousIcon);
    }
  }

  // Inform all observers that items have been loaded, including windows that extend browse window
  std::set<DWORD> observers = g_windowManager.GetWindowObservers(WINDOW_BOXEE_BROWSE);
  std::set<DWORD>::iterator it = observers.begin();
  while (it != observers.end())
  {
    DWORD dwObserverId = (*it);

    CGUIMessage winmsg(GUI_MSG_ITEM_LOADED, 0, 0, GetID(), 0);
    winmsg.SetPointer(new CFileItem(*pItem));
    g_windowManager.SendThreadMessage(winmsg, dwObserverId);

    it++;
  }

  delete pItem;
}

void CGUIWindowBoxeeBrowse::InitializeViewController()
{
  // Initialize view controller
  m_viewControl.Reset();
  m_viewControl.SetParentWindow(GetID());

  // Register views (containers with ids between CONTROL_VIEW_START (50) and CONTROL_VIEW_END (59))
  std::vector<CGUIControl *> controls;
  GetContainers(controls);
  for (ciControls it = controls.begin(); it != controls.end(); it++)
  {
    CGUIControl *control = *it;
    if (control->GetID() >= CONTROL_VIEW_START && control->GetID() <= CONTROL_VIEW_END)
    {
      m_viewControl.AddView(control);
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeViewController, adding controller id = [%d] (browse)", control->GetID());
    }
  }

//  if (m_boxeeViewState.get())
//  {
//    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeViewController, set current view id = [%d] (browse)", m_boxeeViewState->GetViewType());
//    m_viewControl.SetCurrentView(m_boxeeViewState->GetViewType());
//  }

}

void CGUIWindowBoxeeBrowse::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  InitializeViewController();
}

void CGUIWindowBoxeeBrowse::UpdateHistory()
{  
  m_windowState->SetSelectedItem(m_viewControl.GetSelectedItem());
  m_history.push_back(m_windowState->ToHistory());
}

void CGUIWindowBoxeeBrowse::UpdateLabel(const CFileItem* pItem)
{
  // Check the type of the item and add the appropriate label to the label history
  //  if (pItem->GetPropertyBOOL("istvshowfolder") || pItem->GetPropertyBOOL("isalbum")
  //      || pItem->GetPropertyBOOL("isartist")
  //      || pItem->GetPropertyBOOL("isrss")
  //      || pItem->GetPropertyBOOL("isplugin")
  //      || pItem->GetPropertyBOOL("isshare")
  //      || pItem->GetPropertyBOOL("isdvddrive")
  //      || pItem->IsLastFM()
  //      || pItem->IsShoutCast()
  //      || pItem->IsPlugin()
  //      || pItem->GetPropertyBOOL("isgroup")
  //      || pItem->GetPropertyBOOL("isfriend"))
  //  {
  //    m_configuration.m_strLabel = pItem->GetLabel();
  //  }
  //  else
  //  {
  //    CStdString strDirectory;
  //    CUtil::GetDirectoryName(_P(pItem->m_strPath), strDirectory);
  //    if (strDirectory == "" && CUtil::IsSmb(pItem->m_strPath))
  //    {
  //      CURL smbUrl(pItem->m_strPath);
  //      strDirectory = smbUrl.GetHostName();
  //    }
  //
  //    m_configuration.m_strLabel = strDirectory;
  //  }
  //
  //  SetWindowTitle(GetLocationPath());
}

void CGUIWindowBoxeeBrowse::SetWindowTitle(const CStdString& strTitle)
{
  SET_CONTROL_LABEL(LABEL_WINDOW_TITLE, strTitle);
  SetProperty("title", strTitle);
}

// IMPORTANT: This function receives the path by value because, the original item is deleted
void CGUIWindowBoxeeBrowse::UpdatePath(CStdString strPath, bool bResetSelected)
{
  // Create new file item with specified path
  CFileItem item;
  item.m_strPath = strPath;
  item.SetProperty("BrowseBackgroundImage", m_windowState->GetBackgroundImage());

  UpdatePath(&item, bResetSelected); // this function resets the selected item
}

void CGUIWindowBoxeeBrowse::UpdatePath(const CFileItem* pItem, bool bResetSelected)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdatePath, item path = %s, reset selected = %d (browse)", pItem->m_strPath.c_str(), bResetSelected ? 1 : 0);

  ClearFileItems();

  // update browse screen filters by current path
  m_windowState->OnPathChanged(pItem->m_strPath, bResetSelected);

  CFileItem* newItem = new CFileItem(*pItem);

  g_application.GetItemLoader().AddControl(GetID(), 0, newItem);
}


void CGUIWindowBoxeeBrowse::OnLoaded()
{
  m_windowState->OnLoaded();


  // Inform all observers that items have been loaded, including windows that extend browse window
  std::set<DWORD> observers = g_windowManager.GetWindowObservers(WINDOW_BOXEE_BROWSE);
  std::set<DWORD>::iterator it = observers.begin();
  while (it != observers.end())
  {
    DWORD dwObserverId = (*it);

    // Send BIND message with control id set to zero
    CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), 0);
    g_windowManager.SendThreadMessage(msg, dwObserverId);

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnLoaded, send GUI_MSG_LABEL_BIND to observer %u (browse)", dwObserverId);

    it++;
  }
}

void CGUIWindowBoxeeBrowse::OnLoadFailed(const CFileItem* pItem)
{
  CStdString errorMessageHeader = g_localizeStrings.Get(51600);
  CStdString errorMessageLine1 = g_localizeStrings.Get(51601);
  CStdString errorMessageLine2 = "";
  CStdString errorMessageLine3 = "";

  if(pItem == NULL)
  {
    CLog::Log(LOGDEBUG,"Enter to function CGUIWindowBoxeeBrowse::OnLoadFailed with NULL FileItem (browse)");
  }
  else
  {
    errorMessageLine2 = pItem->m_strPath;

    bool hasItemAvailible = pItem->HasProperty("IsAvailable");

    if(hasItemAvailible == true)
    {
      bool isItemAvailible = pItem->GetPropertyBOOL("IsAvailable");

      if(isItemAvailible == false)
      {
        errorMessageLine3 = g_localizeStrings.Get(51602);
        CLog::Log(LOGDEBUG,"Enter to function CGUIWindowBoxeeBrowse::OnLoadFailed for FileItem [%s]. FileItem HAS IsAvailable property and it value is FALSE (browse)", errorMessageLine2.c_str());
      }
      else
      {
        CLog::Log(LOGDEBUG,"Enter to function CGUIWindowBoxeeBrowse::OnLoadFailed for FileItem [%s]. FileItem HAS IsAvailable property and it value is TRUE (browse)",errorMessageLine2.c_str());
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"Enter to function CGUIWindowBoxeeBrowse::OnLoadFailed for FileItem [%s]. FileItem HAS NO IsAvailable property (browse)",errorMessageLine2.c_str());
    }
  }

  m_windowState->OnLoadFailed();

  // Show some information dialog
  CGUIDialogOK *pDlgOK = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);

  if (pDlgOK)
  {
    pDlgOK->SetHeading(errorMessageHeader);
    pDlgOK->SetLine(0, "");
    pDlgOK->SetLine(1, errorMessageLine1);
    pDlgOK->SetLine(2, errorMessageLine2);
    pDlgOK->SetLine(3, errorMessageLine3);
    pDlgOK->DoModal();
  }

  OnBack();
}

bool CGUIWindowBoxeeBrowse::GetClickedItem(int iItem, CFileItem& item)
{
  if ( iItem < 0 || iItem >= (int)m_vecViewItems.Size() )
    return false;
  item = (*m_vecViewItems.Get(iItem));

  return true;
}

bool CGUIWindowBoxeeBrowse::OnClick(int iItem)
{
  CFileItem item;

  if (!GetClickedItem(iItem, item))
    return true;

  m_strInActionItemId = item.GetProperty("itemid");

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnClick, item clicked, path = %s (browse)", item.m_strPath.c_str());

  item.Dump();

  if (CBoxeeItemLauncher::Launch(item))
  {
    // TODO: Redundant item copy, think about it
    // OnItemLoaded(new CFileItem(item));
    return true;
  }

  if ((item.m_bIsFolder) && (!item.IsPlayList()))
  {
    if (HasProperty("BrowseBackgroundImage"))
    {
      item.SetProperty("BrowseBackgroundImage",GetProperty("BrowseBackgroundImage").c_str());
    }

    m_windowState->OnLoading();
    UpdateHistory();
    UpdatePath(&item, true);
    UpdateLabel(&item);

    return true;
  }

  return false;
}

void CGUIWindowBoxeeBrowse::OnBack()
{
  if (m_history.empty())
  {
    if (m_strAppId == "")
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnBack, history is empty (browse)");
      //g_windowManager.PreviousWindow();

      // Open a main menu
      CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);
      pMenu->DoModal();
    }
    else
    {
      g_windowManager.PreviousWindow();
    }
  }
  else 
  {
    CBrowseWindowState* oldState = m_history.back();
    m_windowState->FromHistory(oldState);
    m_history.pop_back();
    delete oldState;

    UpdatePath(m_windowState->GetCurrentPath(), false);
    m_viewControl.SetSelectedItem(m_windowState->GetSelectedItem());

    SetWindowTitle(m_windowState->GetLabel());
  }
}

void CGUIWindowBoxeeBrowse::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecModelItems.Clear(); // will clean up everything
  m_vecViewItems.Clear();
}

CGUIControl *CGUIWindowBoxeeBrowse::GetFirstFocusableControl(int id)
{
  // Detect the attempt to access one of the views, and substitute it with the current view
  if (m_viewControl.HasControl(id))
    id = m_viewControl.GetCurrentControl();
  return CGUIWindow::GetFirstFocusableControl(id);
}

void CGUIWindowBoxeeBrowse::OnWindowUnload()
{
  CGUIWindow::OnWindowUnload();
  m_viewControl.Reset();

}

bool CGUIWindowBoxeeBrowse::OnPlayMedia(CFileItem item)
{
  // Reset Playlistplayer, playback started now does
  // not use the playlistplayer.
  g_playlistPlayer.Reset();
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_NONE);

  bool bResult = false;
  if (item.IsInternetStream() || item.IsPlayList())
    bResult = g_application.PlayMedia(item, PLAYLIST_NONE);
  else
    bResult = g_application.PlayFile(item);

  return bResult;
}

CFileItemPtr CGUIWindowBoxeeBrowse::GetCurrentListItem(int offset)
{
  int item = m_viewControl.GetSelectedItem();
  if (!m_vecViewItems.Size() || item < 0)
    return CFileItemPtr();
  item = (item + offset) % m_vecViewItems.Size();
  if (item < 0) item += m_vecViewItems.Size();
  return m_vecViewItems.Get(item);
}

CStdString CGUIWindowBoxeeBrowse::GetWindowStateType()
{
  return m_windowState->GetWindowStateType();
}

void CGUIWindowBoxeeBrowse::ResetWindowState(const CStdString &strPath, const CStdString &strType, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory)
{
  ClearFileItems();

  if (bResetHistory)
    ResetHistory();
  else
    UpdateHistory();

  // Configuration of the browse window is initialized using specific type string such as "video", "music", or "pictures"
  m_windowState->UpdateConfigurationByType(strType);
  m_windowState->SetPath(strPath);
  m_windowState->SetBackgroundImage(strBackgroundImage);
  m_windowState->SetLabel(strLabel);
  m_windowState->UpdateWindowProperties();
}

void CGUIWindowBoxeeBrowse::Show(const CStdString &strPath, const CStdString &strType, const CStdString &strLabel, const CStdString &strBackgroundImage, bool bResetHistory, const CStdString &strAppId)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::Show, show path = %s", strPath.c_str());

  CGUIWindowBoxeeBrowse* pWindow = (CGUIWindowBoxeeBrowse*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE);
  if (pWindow)
  {
    pWindow->ResetWindowState(strPath, strType, strLabel, strBackgroundImage, bResetHistory);
    pWindow->SetAppData(strAppId, strLabel);
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE);
  }
}

void CGUIWindowBoxeeBrowse::Prefetch(const CStdString& strPath)
{
  CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::Prefetch, NOT IMPLEMENTED");
}

void CGUIWindowBoxeeBrowse::Refresh(bool bResetSelected)
{
  UpdatePath(m_windowState->CreatePath(), bResetSelected);
}

void CGUIWindowBoxeeBrowse::ResetHistory()
{
  for (size_t i = 0; i < m_history.size(); i++)
  {
    delete m_history[i];
  }
  m_history.clear();
}

CStdString CGUIWindowBoxeeBrowse::GetLocationPath()
{
  return "";
  //  CStdString path = "";
  //  for (size_t i = 0; i < m_history.size(); i++)
  //  {
  //    CStdString label = m_history[i].m_strLabel;
  //    path += label;
  //    path += " / ";
  //  }
  //
  //  path += m_configuration.m_strLabel;
  //
  //  if (m_configuration.m_activeFilters.size() > 0) {
  //
  //    CStdString filterName = m_configuration.GetFilter(m_configuration.m_activeFilters[0])->m_strName;
  //
  //    bool bShowFilter = true;
  //
  //    if (m_configuration.m_activeFilters[0] == FILTER_ALL)
  //      bShowFilter = false;
  //
  //    if (m_configuration.m_activeFilters[0] == FILTER_FOLDER_MEDIA_ITEM)
  //      bShowFilter = false;
  //
  //    // Do not show filter all and matching filters in browse screens. For example when browsing pictures, picture
  //    // filter is automatically applied but not shown
  //    if (m_configuration.m_iBrowseMode == BROWSE_MODE_PICTURES && m_configuration.m_activeFilters[0] == FILTER_PICTURE)
  //      bShowFilter = false;
  //
  //    if (bShowFilter) {
  //      path += " / ";
  //      path += filterName;
  //    }
  //  }
  //
  //  return path;
}

void CGUIWindowBoxeeBrowse::UpdateFileList(bool bPreserveSelected)
{
  // Save the path of the currently selected item, to restore it after the update (if possible)
  int nItem = m_viewControl.GetSelectedItem();
  CStdString strSelected = "";

  if (nItem >= 0 && nItem < m_vecViewItems.Size())
  {
    CFileItemPtr pItem = m_vecViewItems.Get(nItem);
    if (pItem)
      strSelected = pItem->m_strPath;
  }

  m_windowState->ProcessItems(m_vecModelItems, m_vecViewItems);
  SetViewItems(m_vecViewItems);

  // Restore selected item
  if (bPreserveSelected && strSelected != "")
  {
    m_viewControl.SetSelectedItem(strSelected);
  }
  else
  {
    m_viewControl.SetSelectedItem(0);
  }

  SetWindowTitle(m_windowState->GetLabel());
}

const CFileItemList& CGUIWindowBoxeeBrowse::GetItems()
{
  return m_vecViewItems;
}

int CGUIWindowBoxeeBrowse::GetCurrentView()
{
  if (m_boxeeViewState.get())
  {
    return m_boxeeViewState->GetViewType();
  }
  return -1;
}

void CGUIWindowBoxeeBrowse::SetAppData(const CStdString& strAppId, const CStdString& strAppName)
{
  m_strAppId = strAppId;
  m_strAppName = strAppName;
}

bool CGUIWindowBoxeeBrowse::HasAppData()
{
  return (!m_strAppId.IsEmpty());
}

void CGUIWindowBoxeeBrowse::SetItemWithAppData(CFileItem& item)
{
  item.SetProperty("appid",m_strAppId);
  item.SetProperty("provider",m_strAppName);
}

void CGUIWindowBoxeeBrowse::ResetAppData()
{
  m_strAppId.clear();
  m_strAppName.clear();
}

