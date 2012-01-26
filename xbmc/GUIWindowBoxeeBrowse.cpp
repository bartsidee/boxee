
#include "Application.h"
#include "GUIWindowBoxeeBrowse.h"
#include "BoxeeUtils.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "URL.h"
#include "GUIListContainer.h"
#include "FileSystem/File.h"
#include "GUIDialogOK2.h"
#include "utils/GUIInfoManager.h"
#include "ThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/Directory.h"
#include "GUIBoxeeViewStateFactory.h"
#include "GUIDialogBoxeeBrowseMenu.h"
#include "GUIImage.h"
#include "BoxeeViewDatabase.h"
#include "SpecialProtocol.h"
#include "BoxeeItemLauncher.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"
#include "GUIPassword.h"
#include "FileSmb.h"
#include "VideoInfoTag.h"
#include "GUIDialogYesNo2.h"
#include "GUISettings.h"
#include "utils/log.h"
#include "bxutils.h"

#include <vector>

// Mark the range of view controls
#define CONTROL_VIEW_START        50
#define CONTROL_VIEW_END          59
#define CONTROL_LABEL_INFO        61

#define LABEL_WINDOW_TITLE 820
#define IMAGE_WINDOW_TITLE 810

#define SEARCH_BUTTON_FLAG "search-button"

#define SHOW_FILTERS_AND_SORT     9014
#define SHOW_FILTERS_AND_SORT_FLAG "filters-and-sort"

#define SORT_DROPDOWN_BUTTON 8014
#define RESCAN_BUTTON   8004

#define LETTER_SCROLLBAR_THUMB  7000
#define LETTER_SCROLLBAR_LIST   7100
#define MAIN_ITEM_CONTAINER     9050
#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define ITEM_COUNT_LABEL "item-summary-count"

std::stack<CStdString> CGUIWindowBoxeeBrowse::m_browseMenuStack;
CSelectBrowseMenuItemInfo CGUIWindowBoxeeBrowse::m_selectBrowseMenuItemInfo;

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

CGUIWindowBoxeeBrowse::CGUIWindowBoxeeBrowse() : CGUIWindow(WINDOW_BOXEE_BROWSE, "BoxeeBrowseWindow.xml") , m_strItemDescription(g_localizeStrings.Get(90050))
{
  InitializeWindowState();
}

CGUIWindowBoxeeBrowse::CGUIWindowBoxeeBrowse(DWORD dwID, const CStdString &xmlFile) : CGUIWindow(dwID, xmlFile) , m_strItemDescription(g_localizeStrings.Get(90050))
{
  InitializeWindowState();
}

void CGUIWindowBoxeeBrowse::InitializeWindowState()
{
  m_iPageThreshold = 0.9;
  m_vecViewItems.SetFastLookup("itemid");

  m_windowState = NULL;

  // Set initial state to NULL
  SetWindowState(NULL);

  m_bAllItemsLoaded = false;
  m_bPageRequested = false;

  m_iMiscItems = 0;

  m_hasBrowseMenu = true;
  m_initSelectPosInBrowseMenu = 0;
  m_shouldFocusToPanel = true;
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
  
  // Clear existing background image
  ClearProperty("BrowseBackgroundImage");

  ClearProperty("empty");

  // Initialize state, will call ItemLoader to bring all required items
  m_windowState->InitState();

  SetWindowTitle(m_windowState->GetLabel());

  m_bAllItemsLoaded = false;

  m_shouldFocusToPanel = true;
}

void CGUIWindowBoxeeBrowse::OnDeinitWindow(int nextWindowID)
{
  CGUIWindow::OnDeinitWindow(nextWindowID);
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
    int focusedControlId = GetFocusedControlID();

    if ((action.id == ACTION_MOVE_LEFT) && focusedControlId >= BROWSE_MENU_LIST_LEVEL_START_ID && focusedControlId <= BROWSE_MENU_LIST_LEVEL_END_ID)
    {
      return UpdateWindowBrowseMenu("GO_BACK");
    }

    bool bResult = CGUIWindow::OnAction(action);
    int itemCount = m_vecViewItems.Size();
    SourceItemState itemState = GOT_ITEMS;
        
    // check current position, after the action was performed, to determine whether to load the next page
    int iSelectedItem = m_viewControl.GetSelectedItem();
    
    // Update current configuration 
    m_windowState->SetSelectedItem(iSelectedItem);
    
    //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnAction, selected item = %d, itemsSize = %d (browse) (paging)", iSelectedItem, m_vecViewItems.Size());

    // If another page was not already requested and not all items have been loaded yet, check whether it is time to request another page
    if (!m_bAllItemsLoaded && !m_bPageRequested && itemCount > 6)
    {
      // Calculate paging threshold
      int threshold = CURRENT_PAGE_SIZE - ((int) (m_iPageThreshold * CURRENT_PAGE_SIZE)); //12
      int thresholdItem = threshold > 8 ? threshold : 8;
   
      if ((iSelectedItem) >= itemCount - thresholdItem )
      {
        m_bPageRequested = true;

        itemState = m_windowState->GetNextPage();

        switch (itemState)
        {
          case GOT_ITEMS:
          {
            CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnAction, items updated (browse) (paging)");
          }
          break;

          case WAITING_FOR_ITEMS:
          {
            CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnAction, one of the sources requested next page (browse) (paging)");
          }
          break;

          case NO_MORE_ITEMS:
          {
            m_bAllItemsLoaded = true;
          }
          break;
        }
      }
    }

    return bResult;
  }
  break;
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    int focusedControlId = GetFocusedControlID();
    if (focusedControlId >= BROWSE_MENU_LIST_LEVEL_START_ID && focusedControlId <= BROWSE_MENU_LIST_LEVEL_END_ID)
    {
      if (m_browseMenuStack.size() > 1)
      {
        return UpdateWindowBrowseMenu("GO_BACK");
      }
      else
      {
        OnBack();
        return true;
      }
    }
    else
    {
      OnBack();
      return true;
    }
  }
  break;

  } // switch

  if (action.id >= KEY_ASCII)
  {
    CGUIDialogBoxeeBrowseMenu::OpenSearchWithAction(action);
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeBrowse::IsEqualPaths(const CStdString& left, const CStdString& right)
{
  CURI leftPath(left);
  CURI rightPath(right);

  if (leftPath.GetUrlWithoutOptions() != rightPath.GetUrlWithoutOptions())
    return false; //everything before the options does not match

  std::map<CStdString, CStdString> leftPathMap  = leftPath.GetOptionsAsMap();
  std::map<CStdString, CStdString> rightPathMap = rightPath.GetOptionsAsMap();

  if (leftPathMap.size() <= 0 && (leftPathMap.size() != rightPathMap.size()))
    return false; //should have the same amount of options

  for ( std::map<CStdString, CStdString>::iterator it = leftPathMap.begin();
        it != leftPathMap.end() ; it++)
  {
    if (! (it->second.Equals(rightPathMap[it->first])) ) //do the options values match?
      return false;
  }

  return true;
}

CStdString CGUIWindowBoxeeBrowse::GetItemDescription()
{
  CStdString strItemCount="";

  strItemCount.Format("%d %s", m_windowState->GetTotalItemCount() , m_strItemDescription.c_str());

  return strItemCount;
}

void CGUIWindowBoxeeBrowse::ShowItems(CFileItemList& list, bool append)
{
  m_bPageRequested = false;

  if (append)
  {
    //use the list instead
    CFileItemList vecAppendedList;
    int viewItemsSize = m_vecViewItems.Size(); //we extract the FileItemPtr(s) from m_vecViewItems using viewItemsSize
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ShowItems, before appending viewItemsSize=%d", viewItemsSize);
    m_vecViewItems.Assign(list, append);
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ShowItems, after appending m_vecViewItems.Size()=%d" , m_vecViewItems.Size());

    if (viewItemsSize+list.Size() <= m_vecViewItems.Size()) //should always happen
      m_vecViewItems.AssignTo(vecAppendedList,viewItemsSize,viewItemsSize+list.Size());

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ShowItems, vecAppendedList size=%d", vecAppendedList.Size());
    //append it to the view
    PrepareViewItems(m_vecViewItems);//, vecAppendedList);

    m_viewControl.AppendItems(m_vecViewItems , vecAppendedList);
  }
  else
  {
    m_vecViewItems.Assign(list, append);
    PrepareViewItems(m_vecViewItems);

    m_viewControl.SetItems(m_vecViewItems);
  }

  HandleEmptyState();

  SetProperty("loading", false);

  // Update current view type
  m_viewControl.SetCurrentView(m_windowState->GetCurrentView(),(m_shouldFocusToPanel && !GetPropertyBOOL("empty")));
  m_viewControl.SetSelectedItem(m_windowState->GetSelectedItem());

  BoxeeUtils::IndexItems(m_viewItemsIndex,m_vecViewItems);

  SetProperty(ITEM_COUNT_LABEL , GetItemDescription());
}

bool CGUIWindowBoxeeBrowse::HandleEmptyState()
{
  bool isEmpty = false;

  if (m_vecViewItems.Size() == 0)
  {
    isEmpty = true;
    SetProperty("empty", isEmpty);
    SET_CONTROL_FOCUS(7092, 0);
  }
  else
  {
    SetProperty("empty",isEmpty);
  }

  return isEmpty;
}

void CGUIWindowBoxeeBrowse::HandleLoadingTimeout(const CStdString& customMessage)
{
  CStdString displayMessage = customMessage.IsEmpty()?g_localizeStrings.Get(51611):displayMessage;

  CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(51600),displayMessage);
  //g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_EXCLAMATION,displayMessage,"",TOAST_DISPLAY_TIME,KAI_RED_COLOR,KAI_RED_COLOR);
}

bool CGUIWindowBoxeeBrowse::OnBind(CGUIMessage& message)
{
  return false;
}

void CGUIWindowBoxeeBrowse::GenerateAlphabetScrollbar(CFileItemList& list)
{
  std::set<CStdString> setLetters;
  std::vector<CStdString> vecAlphabet;
  std::list<CStdString> listAlphabet;
  bool bReverse = (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ZTOA);
  CFileItemList outputList1;
  CFileItemList outputList2;
  m_iMiscItems = 0 ;

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), LETTER_SCROLLBAR_LIST);
  OnMessage(msgReset);

  CGUIMessage msgReset2(GUI_MSG_LABEL_RESET, GetID(), LETTER_SCROLLBAR_THUMB);
  OnMessage(msgReset2);

  CStdStringW strAlphabet;// = "#,A,B,C,D,E,F,G,H,I,J,K,L,M,N,O,P,Q,R,S,T,U,V,W,X,Y,Z,*";

  g_charsetConverter.utf8ToW(g_localizeStrings.Get(60000),strAlphabet,true,true);

  CStdString strSeparator = g_localizeStrings.Get(60003);

  CUtil::Tokenize(strAlphabet,vecAlphabet,strSeparator); //vecAlphabet has A to Z

  listAlphabet.assign(vecAlphabet.begin(),vecAlphabet.end());

  if (bReverse)
  {//reverse the list to be z-a
    listAlphabet.reverse();
    //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GenerateAlphabetScrollbar, sorting is z-a. (alphabet scrollbar)");
  }
  //build a alphabet set of each items first letter
  for (int i = 0 ; i < list.Size() ; i++)
  {
    CStdString strItemLabel = list[i]->GetSortLabel();

    if (strItemLabel.IsEmpty())
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GenerateAlphabetScrollbar, Item's sort label is empty, using default label instead (alphabet scrollbar)");
      strItemLabel = list[i]->GetLabel();
    }
    CStdString firstLetter = strItemLabel.Left(1); //it is important to use the GetSortLabel and not anything else because the scrollbar represents the items by their sort
    setLetters.insert(firstLetter); //generate the first letters of the items we have in the window

    //we need to set the m_iMiscItems according to the sort

    if (m_iMiscItems == 0)
    {
      if (bReverse)
      { // A is the last one, find a letter that is smaller than A
        char minimal = 'A'-1;
        if (StringUtils::AlphaNumericCompare(firstLetter.c_str(),&minimal) < 0 )
        {
          m_iMiscItems = i;
        }
      }
      else
      {
        char maximal = 'z'+1;
        if (StringUtils::AlphaNumericCompare(firstLetter.c_str(),&maximal) > 0 )
        {
          m_iMiscItems = i;
        }
      }
    }
  }

  if (m_iMiscItems == 0)
    m_iMiscItems = list.Size()-1;

  //build a vector of a-z
  for (std::list<CStdString>::iterator it = listAlphabet.begin(); it != listAlphabet.end() ; it++)
  {
    CFileItemPtr letterItem(new CFileItem((*it)));
    CFileItemPtr letterItem2(new CFileItem((*it)));

    if ((*it) != g_localizeStrings.Get(60001) && (*it) != g_localizeStrings.Get(60002) && setLetters.find((*it)) == setLetters.end())
    {
      //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GenerateAlphabetScrollbar, disabled %s (alphabet scrollbar)",it->c_str());
      letterItem->SetProperty("isseparator",true);
      letterItem2->SetProperty("isseparator",true);
    }

    outputList1.Add(letterItem);
    outputList2.Add(letterItem2);
  }

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), LETTER_SCROLLBAR_LIST, 0, 0, &outputList1);
  OnMessage(msg);

  CGUIMessage msg2(GUI_MSG_LABEL_BIND, GetID(), LETTER_SCROLLBAR_THUMB, 0, 0, &outputList2);
  OnMessage(msg2);
}

void CGUIWindowBoxeeBrowse::ConfigureState(const CStdString& param)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ConfigureState - enter function with [param=%s]. [windowId=%d] (browsemenu)",param.c_str(),GetID());

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.find("category") != optionsMap.end())
    {
      m_windowState->SetCategory(optionsMap["category"]);
    }
    else
    {
      m_windowState->SetDefaultCategory();
    }

    if (optionsMap.find("sort") != optionsMap.end())
    {
      CBoxeeSort* sort = NULL;

      CStdString strSortId = optionsMap["sort"];

      sort = m_windowState->GetSortById(strSortId);

      if (sort != NULL)
      {
        m_windowState->SetSort(*sort);
      }
      else
      {
        m_windowState->SetDefaultSort();
      }
    }
    else
    {
      m_windowState->SetDefaultSort();
    }

    m_windowState->SetSelectedItem(0);
    SetSelectedItem(0);
  }
}

bool CGUIWindowBoxeeBrowse::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_LOADING_TIMEDOUT:
  {
    CStdString strPath = message.GetStringParam();

    if (g_windowManager.IsWindowVisible(GetID()) && m_windowState->IsPathRelevant(strPath,message.GetParam1()))
    {
      HandleLoadingTimeout(message.GetLabel());
    }

    return true;
  }
  case GUI_MSG_FOCUSED:
  {
    int controlId = message.GetControlId();

    if ((controlId >= BROWSE_MENU_LIST_LEVEL_START_ID) && (controlId <= BROWSE_MENU_LIST_LEVEL_END_ID))
    {
      if (m_currentMenuLevelStr.IsEmpty())
      {
        m_currentMenuLevelStr = g_settings.GetSkinString(g_settings.TranslateSkinString("activemenulevel"));
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage - GUI_MSG_FOCUSED - menu [id=%d] got focused. save [m_currentMenuLevelStr=%s] (bm)",controlId,m_currentMenuLevelStr.c_str());
      }
    }
  }
  break;
  case GUI_MSG_UPDATE:
  {
    Refresh();
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    CStdString param = message.GetStringParam();

    ClearFileItems();

    CGUIWindow::OnMessage(message);

    if (!param.empty())
    {
      //init by params if needed
      ConfigureState(param);
    }

    if (m_hasBrowseMenu)
    {
      ClearBrowseMenuStack();
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage - GUI_MSG_WINDOW_INIT - going to call InitWindowBrowseMenu (browse)(bm)(bmst)");
      InitWindowBrowseMenu();
    }

    //request the items anyway
    Refresh(false);

    return true;
  }
  break;
  case GUI_MSG_WINDOW_DEINIT:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_WINDOW_DEINIT (browse)");
    m_windowState->SaveState();
    m_windowState->SetSelectedItem(m_viewControl.GetSelectedItem());

    CGUIWindow::OnMessage(message);
    //ClearFileItems();

    return true;
  }
  break;
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
    else if (iControl == SORT_DROPDOWN_BUTTON)
    {
      if (m_windowState->OnSort())
      {
        Refresh(true);
        SET_CONTROL_FOCUS(MAIN_ITEM_CONTAINER,0);
        return true;
      }
    }
    else if (iControl == RESCAN_BUTTON)
    {
      if (CGUIDialogYesNo2::ShowAndGetInput(51542,51547,51548,51542))
      {
        for (int i=0; i<(int)g_settings.m_videoSources.size(); i++)
        {
          if (g_settings.m_videoSources[i].m_iScanType != CMediaSource::SCAN_TYPE_PRIVATE)
          {
            CStdString strName = g_settings.m_videoSources[i].strName;
            CStdString strPath = g_settings.m_videoSources[i].strPath;
            CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage - BUTTON_SCAN - adding video source [name=%s][path=%s] to scan (rescan)",strName.c_str(),strPath.c_str());
            g_application.GetBoxeeFileScanner().AddUserPath(_P(strPath));
          }
        }
      }
    }
    else if (iControl == LETTER_SCROLLBAR_THUMB || iControl == LETTER_SCROLLBAR_LIST)
    {
      CGUIListContainer* container = (CGUIListContainer*)GetControl(iControl);

      if (container != NULL)
      {
        CGUIListItemPtr selectedListItem = container->GetSelectedItemPtr();
        CFileItem* selectedFileItem = (CFileItem*) selectedListItem.get();

        //selectedFileItem->Dump();

        CStdString letter = selectedFileItem->GetLabel();
        if (letter == g_localizeStrings.Get(60001)) // #
        {
          if (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ZTOA)
          {
            m_viewControl.SetSelectedItem(m_iMiscItems);
          }
          else
          {
            m_viewControl.SetSelectedItem(0);
          }
        }
        else if (letter == g_localizeStrings.Get(60002)) // *
        {
          if (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ATOZ)
          {
            m_viewControl.SetSelectedItem(m_iMiscItems);
          }
          else
          {
            m_viewControl.SetSelectedItem(0);
          }
        }
        else
        {
          CGUIMessage messageJumpLetter(GUI_MSG_JUMPTOLETTER, GetID(), m_windowState->GetCurrentView());
          messageJumpLetter.SetLabel(letter);
          OnMessage(messageJumpLetter);
        }
        SET_CONTROL_FOCUS(MAIN_ITEM_CONTAINER,-1);
      }
    }
    else if ((iControl >= BROWSE_MENU_LIST_LEVEL_START_ID) && (iControl <= BROWSE_MENU_LIST_LEVEL_END_ID))
    {
      CGUIListContainer* pListcontainer = (CGUIListContainer*)GetControl(iControl);

      if (!pListcontainer)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage - GUI_MSG_CLICKED - FAILED to get menu control [id=%d] (bm)",iControl);
        return false;
      }

      CGUIListItemPtr selectedListItem = pListcontainer->GetSelectedItemPtr();

      if (!selectedListItem.get())
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage - GUI_MSG_CLICKED - FAILED to get clicked item from menu control [id=%d] (bm)",iControl);
        return false;
      }

      CFileItemPtr selectedFileItem(new CFileItem(*((CFileItem*)selectedListItem.get())));
      selectedFileItem->SetProperty("pos-in_menu",pListcontainer->GetSelectedItem());

      return HandleClickOnBrowseMenu(selectedFileItem);
    }
  }
  break;
  case GUI_MSG_CHANGE_VIEW_MODE:
  {
    int viewMode = message.GetParam1();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, GUI_MSG_CHANGE_VIEW_MODE, set view to %d (browse)", viewMode);

    m_windowState->SetCurrentView(viewMode);
    m_viewControl.SetCurrentView(viewMode);

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
  break;
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
  break;
  case GUI_MSG_LOADING:
  {
    // Check that control id is zero to prevent recursive calls
    if (message.GetControlId() == 0)
    {
      SetProperty("loading", true);
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnMessage, 4 GUI_MSG_LOADING, window = %d, control = %d (basecontainer) (browse)", GetID(), m_viewControl.GetCurrentControl());
      return true;
    }
  }
  break;
  case GUI_MSG_BG_PICTURE:
  {
    CStdString strBGImage = message.GetStringParam();
    SetProperty("BrowseBackgroundImage", strBGImage.c_str());

    return true;
  }
  break;
  case GUI_MSG_LABEL_BIND:
  {
    // Should be changed to some other function that is implemented in children
    OnBind(message); 

    if (message.GetPointer() && message.GetControlId() == 0)
    {
      CFileItemList *items = (CFileItemList *)message.GetPointer();

      if (g_windowManager.GetActiveWindow() == GetID())
      {
        //if the window is not active, disregard the items
        m_windowState->OnBind(*items);
      }
      else if (items != NULL)
      {
        CLog::Log(LOGDEBUG,"Got (%d) items from url request:[%s] targeted to window [%d], but the window is not visible. disregarding the items.",items->Size(), items->m_strPath.c_str(),GetID());
      }

      if (items != NULL)
      {
        delete items;
      }

      if(m_bAllItemsLoaded)
      {
        SetProperty("loading", false);
      }

      return true;
    }
  }
  break;

  } // switch

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeBrowse::UpdateIndexMaps()
{
  CFileItemPtr pItem;

  for (int i = 0; i < m_vecViewItems.Size(); i++)
  {
    pItem = m_vecViewItems.Get(i);
    m_viewItemsIndex[pItem->GetProperty("itemid")] = pItem;
  }
}

/**
 * The purpose of this function is to check whether the list of items belongs to a TV show which is sorted in descending order
 */
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
		if (pVideoInfoTag->m_iSeason != -1 && pVideoInfoTag->m_iEpisode != -1 &&
			pItemToCheckVideoInfoTag->m_iSeason != -1 && pItemToCheckVideoInfoTag->m_iEpisode != -1)
		{
			//
			// same show and different episodes
			//
			if(pVideoInfoTag->m_strShowTitle == pItemToCheckVideoInfoTag->m_strShowTitle)
			{
				if( (pItemToCheckVideoInfoTag->m_iSeason + 1000) * pItemToCheckVideoInfoTag->m_iEpisode < 
					  (pVideoInfoTag->m_iSeason + 1000) * pVideoInfoTag->m_iEpisode )
				{
					bResult = true;
				}
			}
		}
	}

	return bResult;
}

void CGUIWindowBoxeeBrowse::PrepareViewItems(CFileItemList& items)
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

}

void CGUIWindowBoxeeBrowse::ClearView()
{
  m_vecViewItems.Clear();
  PrepareViewItems(m_vecViewItems);
}

void CGUIWindowBoxeeBrowse::OnItemLoaded(const CFileItem* pItem)
{
  // Check whether the item belongs to the same directory that is currently being loaded
  /*if (!pItem->HasProperty("directoryPath") || m_windowState->IsPathRelevant(pItem->GetProperty("directoryPath")))// m_vecModelItems.m_strPath)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnItemLoaded, NEWUI, discard item, current path = %s, item path = %s (browse)",
        m_windowState->GetCurrentPath().c_str(), pItem->m_strPath.c_str());
    delete pItem;
    return;
  }*/

  CStdString itemId = pItem->GetProperty("itemid");

  // Update items in the view
  CFileItemPtr pCurrentItem = m_viewItemsIndex[itemId];
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

  // Update state
  m_windowState->OnItemLoaded(pItem);

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
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeViewController - adding controller [id=%d] (browse)",control->GetID());
    }
  }
}

void CGUIWindowBoxeeBrowse::OnWindowLoaded()
{
  CGUIWindow::OnWindowLoaded();
  InitializeViewController();
}

void CGUIWindowBoxeeBrowse::SetWindowTitle(const CStdString& strTitle)
{
  SET_CONTROL_LABEL(LABEL_WINDOW_TITLE, strTitle);
  SetProperty("title", strTitle);
}

void CGUIWindowBoxeeBrowse::UpdateUIFlags()
{

}

void CGUIWindowBoxeeBrowse::OnLoadFailed(const CFileItem* pItem)
{
  CGUIDialogOK2::ShowAndGetInput(51600, 51601);
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

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::OnClick, item clicked, path = %s (browse)", item.m_strPath.c_str());
  item.Dump();

  // Pass the clicked item to the launcher
  if (CBoxeeItemLauncher::Launch(item))
  {
    return true;
  }

  // In case the item was not handled by the launcher pass it to the state
  return m_windowState->OnClick(item);
}

void CGUIWindowBoxeeBrowse::OnBack()
{
  m_windowState->OnBack();
}

//void CGUIWindowBoxeeBrowse::OnSearch()
//{
//  // TODO: Rewrite this function
//  m_windowState->OnSearchStart();
//}

void CGUIWindowBoxeeBrowse::ClearFileItems()
{
  m_viewControl.Clear();
  m_vecViewItems.Clear();

  m_bAllItemsLoaded = false;
  m_bPageRequested =false;
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
  m_viewControl.Reset();
  CGUIWindow::OnWindowUnload();
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

void CGUIWindowBoxeeBrowse::Refresh(bool bResetSelected)
{
  SetProperty(ITEM_COUNT_LABEL , "");
  m_windowState->Refresh(bResetSelected);
  m_bAllItemsLoaded = false;
}

bool CGUIWindowBoxeeBrowse::InitWindowBrowseMenu()
{
  if (!m_hasBrowseMenu)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitWindowBrowseMenu - [hasBrowseMenu=%d] for [windowId=%d] (bm)",m_hasBrowseMenu,GetID());
    return true;
  }

  CStdString category = m_windowState->GetCategory();

  if (!category.IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitWindowBrowseMenu - since [category=%s] going to call UpdateWindowBrowseMenu (browse)(bm)",category.c_str());

    m_currentMenuLevelStr = "";
    g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),"0");

    if (!UpdateWindowBrowseMenu())
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::InitWindowBrowseMenu - GUI_MSG_WINDOW_INIT - FAILED to generate BrowseMenu (browse)(bm)");
      return false;
    }
  }

  return true;
}

bool CGUIWindowBoxeeBrowse::UpdateWindowBrowseMenu(const CStdString& menuId)
{
  if (!m_hasBrowseMenu)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateWindowBrowseMenu - [hasBrowseMenu=%d] for [windowId=%d] (bm)",m_hasBrowseMenu,GetID());
    return true;
  }

  CStdString category = m_windowState->GetCategory();

  bool retVal = false;

  if (menuId.IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateWindowBrowseMenu - [menuId=%s] is EMPTY -> call InitializeStartMenuStructure. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
    retVal = InitializeStartMenuStructure();
  }
  else if (menuId == "GO_BACK")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateWindowBrowseMenu - [menuId=%s=GO_BACK] -> call GoBackwardInWindowBrowseMenu. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
    retVal = GoBackwardInWindowBrowseMenu();
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateWindowBrowseMenu - handle [menuId=%s] -> call UpdateCurrentWindowBrowseMenu. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
    retVal = GoForwardInWindowBrowseMenu(menuId);
  }

  return retVal;
}

bool CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu(const CStdString& menuId)
{
  CStdString category = m_windowState->GetCategory();

  CFileItemList* browseMenu = NULL;
  int menuLevelToReset = -1;
  CStdString currentMenuLevelStr = g_settings.GetSkinString(g_settings.TranslateSkinString("activemenulevel"));
  int iCurrentMenuLevel = atoi(currentMenuLevelStr.c_str());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu - enter with [menuId=%s][currentActivemenulevel=%s=%d][category=%s][windowId=%d] (bm)",menuId.c_str(),currentMenuLevelStr.c_str(),iCurrentMenuLevel,category.c_str(),GetID());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu - handle [menuId=%s] -> call GetMenu. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
  browseMenu = CBoxeeBrowseMenuManager::GetInstance().GetMenu(menuId);
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu - handle [menuId=%s] -> after call GetMenu [browseMenuSize=%d]. [category=%s][windowId=%d] (bm)",menuId.c_str(),(browseMenu ? browseMenu->Size() : -1),category.c_str(),GetID());
  menuLevelToReset = BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel;

  if (!browseMenu)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu - FAILED to get browse menu for [menuId=%s]. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
    return false;
  }

  if (!AddButtonItemsToBrowseMenu(*browseMenu,(BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel)))
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::GoForwardInWindowBrowseMenu - FAILED to add items to browse menu for [menuId=%s]. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
    return false;
  }

  // increase the menu level
  iCurrentMenuLevel++;
  char tmp[5];
  itoa(iCurrentMenuLevel,tmp,10);

  g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),tmp);

  SET_CONTROL_FOCUS((BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),0);

  return true;
}

bool CGUIWindowBoxeeBrowse::InitializeStartMenuStructure()
{
  CStdString category = m_windowState->GetCategory();

  std::list<CFileItemList> browseMenuLevelList;

  m_initSelectPosInBrowseMenu = 0;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - going to call GetStartMenusStructure. [category=%s][windowId=%d][m_initSelectPosInBrowseMenu=%d] (bm)",category.c_str(), GetID(), m_initSelectPosInBrowseMenu);

  GetStartMenusStructure(browseMenuLevelList);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - call to GetStartMenusStructure returned [listSize=%zu]. [category=%s][windowId=%d] (bm)",browseMenuLevelList.size(),category.c_str(), GetID());

  if (browseMenuLevelList.empty())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - FAILED to get start menu structure [listSize=%zu]. [category=%s][windowId=%d] (bm)",browseMenuLevelList.size(),category.c_str(), GetID());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - going to reset ALL browse menu levels. [category=%s][windowId=%d] (bm)",category.c_str(), GetID());

  ResetBrowseMenuLevel(-1);

  int iCurrentMenuLevel = -1;

  CFileItemList browseMenu;

  while (!browseMenuLevelList.empty())
  {
    browseMenu = browseMenuLevelList.front();
    browseMenuLevelList.pop_front();

    CStdString menuId = browseMenu.GetProperty("id");

    //int menuLevelToReset = BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel;

    iCurrentMenuLevel++;

    if (browseMenuLevelList.empty())
    {
      int initSelectPos = browseMenu.GetProperty("init-select-pos").IsEmpty() ? 0 : browseMenu.GetPropertyInt("init-select-pos");
      CStdString initSelectLabel = browseMenu.GetProperty("init-select-label");

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - going to save selected menu button. [StackSize=%zu] (bm)",m_browseMenuStack.size());
      m_selectBrowseMenuItemInfo.Set(menuId,initSelectPos,initSelectLabel);
    }

    if (!AddButtonItemsToBrowseMenu(browseMenu,(BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel)))
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - FAILED to add items to browse menu for [menuId=%s]. [category=%s][windowId=%d] (bm)",menuId.c_str(),category.c_str(),GetID());
      return false;
    }
  }

  iCurrentMenuLevel++;
  char tmp[5];
  itoa(iCurrentMenuLevel,tmp,10);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::InitializeStartMenuStructure - going to set [activemenulevel=%s] and focus to [pos=%d] in [MenuListId=%d]. [category=%s][windowId=%d] (bm)",tmp,m_initSelectPosInBrowseMenu,(BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),category.c_str(), GetID());

  g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),tmp);

  SET_CONTROL_FOCUS((BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),0);
  CONTROL_SELECT_ITEM((BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),m_initSelectPosInBrowseMenu);

  return true;
}

bool CGUIWindowBoxeeBrowse::AddButtonItemsToBrowseMenu(const CFileItemList& browseMenu,int browseMenuListControlId,bool fromStack)
{
  if (browseMenu.IsEmpty())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::AddButtonItemsToBrowseMenu - enter function with an EMPTY browse menu list (bm)");
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::AddButtonItemsToBrowseMenu - going to call ResetBrowseMenuLevel with [browseMenuListControlId=%d] (bm)",browseMenuListControlId);
  ResetBrowseMenuLevel(browseMenuListControlId);

  for (int i=0; i<browseMenu.Size(); i++)
  {
    CFileItemPtr menuItem(new CFileItem(*(browseMenu.Get(i).get())));
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::AddButtonItemsToBrowseMenu - [%d/%d] - adding menu item [label=%s][path=%s][isseparator=%d]. [menuListControlIs=%d] (bm)",i+1,browseMenu.Size(),menuItem->GetLabel().c_str(),menuItem->m_strPath.c_str(),menuItem->GetPropertyBOOL("isseparator"),browseMenuListControlId);
    CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), browseMenuListControlId, 0, 0, menuItem);
    OnMessage(winmsg);
  }

  if (!fromStack)
  {
    if (browseMenu.GetProperty("level") == "1")
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::AddButtonItemsToBrowseMenu - since [BrowseMenuId=%s][level=%s=1] first clear BrowseMenuStack. [browseMenuListControlId=%d] (bm)",browseMenu.GetProperty("id").c_str(),browseMenu.GetProperty("level").c_str(),browseMenuListControlId);
      ClearBrowseMenuStack();

      m_shouldFocusToPanel = true;
    }

    AddBrowseMenuToStack(browseMenu.GetProperty("id"));
    UpdateBrowseMenuData(browseMenu, browseMenuListControlId);
  }

  return true;
}

void CGUIWindowBoxeeBrowse::UpdateBrowseMenuData(const CFileItemList& browseMenu, int browseMenuListControlId)
{
  CStdString menuTitleStr = browseMenu.GetProperty("title");

  if (menuTitleStr.IsEmpty())
  {
    // keep the title from the previous level

    CStdString previousMenuTitleId;
    previousMenuTitleId.Format("BrowseMenuLevel%dLabel",(browseMenuListControlId - BROWSE_MENU_LIST_LEVEL_START_ID));
    menuTitleStr = g_settings.GetSkinString(g_settings.TranslateSkinString(previousMenuTitleId));

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateBrowseMenuData - going to keep previous menu title [%s] (bm)",menuTitleStr.c_str());
  }

  CStdString menuTitleId;
  menuTitleId.Format("BrowseMenuLevel%dLabel",(browseMenuListControlId - BROWSE_MENU_LIST_LEVEL_START_ID + 1));

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateBrowseMenuData - going to set menu title [%s] (bm)",menuTitleStr.c_str());
  g_settings.SetSkinString(g_settings.TranslateSkinString(menuTitleId),"[UPPERCASE][B]" + menuTitleStr + "[/B][/UPPERCASE]");

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateBrowseMenuData - [NewMenuId=%s][%s=saveSelectMenuId]. [index=%d][label=%s] (bm)",browseMenu.GetProperty("id").c_str(),m_selectBrowseMenuItemInfo.m_menuId.c_str(),m_selectBrowseMenuItemInfo.m_selectedIndex,m_selectBrowseMenuItemInfo.m_selectedLabel.c_str());

  UpdateTopBrowseMenuSelection();
}

void CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection()
{
  if (m_browseMenuStack.empty())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection - FAILED to set selection in top menu since BrowseMenuStack is EMPTY [size=%zu]. [browseMenuId=%s][index=%d][label=%s]  (bm)",m_browseMenuStack.size(),m_selectBrowseMenuItemInfo.m_menuId.c_str(),m_selectBrowseMenuItemInfo.m_selectedIndex,m_selectBrowseMenuItemInfo.m_selectedLabel.c_str());
    return;
  }

  CStdString menuId = m_browseMenuStack.top();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection - top menu is [id=%s]. try to set selection to [browseMenuId=%s][index=%d][label=%s]  (bm)",menuId.c_str(),m_selectBrowseMenuItemInfo.m_menuId.c_str(),m_selectBrowseMenuItemInfo.m_selectedIndex,m_selectBrowseMenuItemInfo.m_selectedLabel.c_str());

  if (menuId == m_selectBrowseMenuItemInfo.m_menuId)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection - top menu [id=%s] is the selected on. going set selection to [browseMenuId=%s][index=%d][label=%s]  (bm)",menuId.c_str(),m_selectBrowseMenuItemInfo.m_menuId.c_str(),m_selectBrowseMenuItemInfo.m_selectedIndex,m_selectBrowseMenuItemInfo.m_selectedLabel.c_str());

    for (int browseMenuListLevelId=BROWSE_MENU_LIST_LEVEL_START_ID; browseMenuListLevelId<=BROWSE_MENU_LIST_LEVEL_END_ID; browseMenuListLevelId++)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection - [%d/%d] - going to RESET selection in [menuListId=%d] (bm)",(browseMenuListLevelId - BROWSE_MENU_LIST_LEVEL_START_ID + 1),(BROWSE_MENU_LIST_LEVEL_END_ID - BROWSE_MENU_LIST_LEVEL_START_ID + 1),browseMenuListLevelId);

      CGUIMessage winmsg(GUI_MSG_UNMARK_ALL_ITEMS, GetID(), browseMenuListLevelId);
      OnMessage(winmsg);

      if ((browseMenuListLevelId - BROWSE_MENU_LIST_LEVEL_START_ID) == ((int)m_browseMenuStack.size() - 1))
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::UpdateTopBrowseMenuSelection - [%d/%d] - going to SET selection in [menuListId=%d] to [pos=%d] (bm)",(browseMenuListLevelId - BROWSE_MENU_LIST_LEVEL_START_ID + 1),(BROWSE_MENU_LIST_LEVEL_END_ID - BROWSE_MENU_LIST_LEVEL_START_ID + 1),browseMenuListLevelId,m_selectBrowseMenuItemInfo.m_selectedIndex);

        CGUIMessage winmsg(GUI_MSG_MARK_ITEM, GetID(),browseMenuListLevelId,m_selectBrowseMenuItemInfo.m_selectedIndex);
        OnMessage(winmsg);
      }
    }
  }
}

bool CGUIWindowBoxeeBrowse::GoBackwardInWindowBrowseMenu()
{
  // move in browse menu
  CStdString currentMenuLevelStr = g_settings.GetSkinString(g_settings.TranslateSkinString("activemenulevel"));
  int iCurrentMenuLevel = atoi(currentMenuLevelStr.c_str());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GoBackwardInWindowBrowseMenu - [CurrentMenuLevel=%d] (bm)",iCurrentMenuLevel);

  if (iCurrentMenuLevel > 1)
  {
    // not in level 1 -> move back
    iCurrentMenuLevel--;
    char tmp[5];
    itoa(iCurrentMenuLevel,tmp,10);
    g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),tmp);

    SET_CONTROL_FOCUS((BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),0);

    PopBrowseMenuFromStack();

    UpdateTopBrowseMenuSelection();
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GoBackwardInWindowBrowseMenu - [NewMenuLevel=%s] -> return TRUE (bm)",g_settings.GetSkinString(g_settings.TranslateSkinString("activemenulevel")).c_str());

  return true;
}

bool CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu(const CFileItemPtr selectedFileItem)
{
  CStdString category = m_windowState->GetCategory();

  if (!selectedFileItem.get())
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - enter function with NULL item. [category=%s] (bm)",category.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - enter function with item [label=%s][path=%s]. [category=%s] (bm)",selectedFileItem->GetLabel().c_str(),selectedFileItem->m_strPath.c_str(),category.c_str());

  if (selectedFileItem->GetPropertyBOOL("isClickable"))
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - handle click on [isClickable=%d=TRUE] item [label=%s] (bm)",selectedFileItem->GetPropertyBOOL("isClickable"),selectedFileItem->GetLabel().c_str());

    if (selectedFileItem->m_strPath.IsEmpty())
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - FAILED to handle click on [label=%s][isClickable=%d=TRUE] since [path=%s] is EMPTY. [child=%s] (bm)",selectedFileItem->GetLabel().c_str(),selectedFileItem->GetPropertyBOOL("isClickable"),selectedFileItem->m_strPath.c_str(),selectedFileItem->GetProperty("child").c_str());
      return false;
    }

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - item [label=%s] has [path=%s] -> execute. [category=%s][openInWindow=%s] (bm)",selectedFileItem->GetLabel().c_str(),selectedFileItem->m_strPath.c_str(),category.c_str(),selectedFileItem->GetProperty("openInWindow").c_str());

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - [isClickable=TRUE] - going to save selected menu button. [StackSize=%zu][posInMenu=%d][buttonLabel=%s] (bm)",m_browseMenuStack.size(),selectedFileItem->GetPropertyInt("pos-in_menu"),selectedFileItem->GetLabel().c_str());
    m_selectBrowseMenuItemInfo.Set(m_browseMenuStack.top(),selectedFileItem->GetPropertyInt("pos-in_menu"),selectedFileItem->GetLabel());

    UpdateTopBrowseMenuSelection();

    int openInWindowId = selectedFileItem->GetPropertyInt("openInWindow");
    if ((openInWindowId == 0) || (openInWindowId == GetID()))
    {
      // execute path in the same window

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - Since [openInWindow=%s] is EMPTY -> call ConfigureState with [path=%s]. [category=%s] (bm)",selectedFileItem->GetProperty("openInWindow").c_str(),selectedFileItem->m_strPath.c_str(),category.c_str());

      //save the previous state before applying the new one
      m_windowState->SaveState();
      m_windowState->SetSelectedItem(m_viewControl.GetSelectedItem());

      ConfigureState(selectedFileItem->m_strPath);

      m_shouldFocusToPanel = false;

      m_currentMenuLevelStr = "";
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - after reset [currentMenuLevelStr=%s]. [category=%s][openInWindow=%s] (bm)",m_currentMenuLevelStr.c_str(),category.c_str(),selectedFileItem->GetProperty("openInWindow").c_str());

      bool needToUpdateMenu = selectedFileItem->GetPropertyBOOL("updateMenu");

      if (needToUpdateMenu)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - Since [updateMenu=%d] call UpdateWindowBrowseMenu() with no menuId. [category=%s] (bm)",needToUpdateMenu,category.c_str());

        if (!UpdateWindowBrowseMenu())
        {
          CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - FAILED to generate BrowseMenu (bm)");
        }
      }

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - call Refresh() for the screen [id=%d]. [category=%s][openInWindow=%s][shouldFocusToPanel=%d] (bm)",GetID(),category.c_str(),selectedFileItem->GetProperty("openInWindow").c_str(),m_shouldFocusToPanel);

      Refresh();
    }
    else
    {
      // execute path in a new window

      int windowId = atoi(selectedFileItem->GetProperty("openInWindow").c_str());

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - Since [openInWindow=%s] -> call ActivateWindow with [windowId=%d][path=%s]. [category=%s] (bm)",selectedFileItem->GetProperty("openInWindow").c_str(),windowId,selectedFileItem->m_strPath.c_str(),category.c_str());

      g_windowManager.ActivateWindow(windowId,selectedFileItem->m_strPath);
      return true;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - handle click on [isClickable=%d=FALSE] item [label=%s] (bm)",selectedFileItem->GetPropertyBOOL("isClickable"),selectedFileItem->GetLabel().c_str());

    if (!selectedFileItem->GetProperty("child").IsEmpty())
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - item [label=%s] HAS have [child=%s] -> update. [path=%s][category=%s] (bm)",selectedFileItem->GetLabel().c_str(),selectedFileItem->GetProperty("child").c_str(),selectedFileItem->m_strPath.c_str(),category.c_str());

      if (!UpdateWindowBrowseMenu(selectedFileItem->GetProperty("child")))
      {
        CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::ConfigureState - FAILED to generate BrowseMenu (bm)");
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::HandleClickOnBrowseMenu - FAILED to handle click on [label=%s][path=%s][isClickable=%d][child=%s] (bm)",selectedFileItem->GetLabel().c_str(),selectedFileItem->m_strPath.c_str(),selectedFileItem->GetPropertyBOOL("isClickable"),selectedFileItem->GetProperty("child").c_str());
    }
  }

  return true;
}

void CGUIWindowBoxeeBrowse::ResetBrowseMenuLevel(int menuListId)
{
  if (menuListId != -1)
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ResetBrowseMenuLevel - going to reset specific [menuListId=%d] (bm)",menuListId);

    CGUIMessage winmsg(GUI_MSG_LABEL_RESET, GetID(), menuListId);
    OnMessage(winmsg);
  }
  else
  {
    for (int browseMenuListLevelId=BROWSE_MENU_LIST_LEVEL_START_ID; browseMenuListLevelId<=BROWSE_MENU_LIST_LEVEL_END_ID; browseMenuListLevelId++)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ResetBrowseMenuLevel - [%d/%d] - going to reset [menuListId=%d] (bm)",(browseMenuListLevelId - BROWSE_MENU_LIST_LEVEL_START_ID + 1),(BROWSE_MENU_LIST_LEVEL_END_ID - BROWSE_MENU_LIST_LEVEL_START_ID + 1),browseMenuListLevelId);

      CGUIMessage winmsg(GUI_MSG_LABEL_RESET, GetID(), browseMenuListLevelId);
      OnMessage(winmsg);
    }
  }

  g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),"0");
}

void CGUIWindowBoxeeBrowse::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GetStartMenusStructure - NO IMPLEMENTED for [windowId=%d]. [category=%s] (bm)",GetID(),m_windowState->GetCategory().c_str());

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GetStartMenusStructure - enter function. [browseMenuLevelListSize=%zu][m_initSelectPosInBrowseMenu=%d] (bm)",browseMenuLevelList.size(),m_initSelectPosInBrowseMenu);

  if (!browseMenuLevelList.empty() && (browseMenuLevelList.back().Size() > m_initSelectPosInBrowseMenu))
  {
    CFileItemList& menu = browseMenuLevelList.back();
    menu.SetProperty("init-select-pos",m_initSelectPosInBrowseMenu);
    menu.SetProperty("init-select-label",menu.Get(m_initSelectPosInBrowseMenu)->GetLabel());

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::GetStartMenusStructure - after setting top menu with properties [init-select-pos=%d][init-select-label=%s]. [browseMenuLevelStackSize=%zu][m_initSelectPosInBrowseMenu=%d] (bm)",menu.GetPropertyInt("init-select-pos"),menu.GetProperty("init-select-label").c_str(),browseMenuLevelList.size(),m_initSelectPosInBrowseMenu);
  }
}

void CGUIWindowBoxeeBrowse::ClearBrowseMenuStack()
{
  size_t stackSize = m_browseMenuStack.size();
  int counter = 0;
  CStdString topMenuId;

  while (!m_browseMenuStack.empty())
  {
    topMenuId = m_browseMenuStack.top();
    m_browseMenuStack.pop();
    counter++;
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::ClearBrowseMenuStack - [%d/%zu] - after POP [topMenuId=%s] from stack. [BrowseMenuStackSize=%zu] (bm)(bmst)",counter,stackSize,topMenuId.c_str(),m_browseMenuStack.size());
  }
}

void CGUIWindowBoxeeBrowse::AddBrowseMenuToStack(const CStdString& menuId)
{
  m_browseMenuStack.push(menuId);
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::AddBrowseMenuToStack - after ADD [menuId=%s] to stack. [BrowseMenuStackSize=%zu] (bm)(bmst)",menuId.c_str(),m_browseMenuStack.size());
}

void CGUIWindowBoxeeBrowse::PopBrowseMenuFromStack()
{
  CStdString topMenuId;

  if (m_browseMenuStack.empty())
  {
    CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowse::PopBrowseMenuFromStack - try to POP from EMPTY stack. [BrowseMenuStackSize=%zu] (bm)(bmst)",m_browseMenuStack.size());
    return;
  }

  topMenuId = m_browseMenuStack.top();
  m_browseMenuStack.pop();
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::PopBrowseMenuFromStack - after POP [topMenuId=%s] from stack. [BrowseMenuStackSize=%zu] (bm)(bmst)",topMenuId.c_str(),m_browseMenuStack.size());
}

void CGUIWindowBoxeeBrowse::ApplyBrowseMenuFromStack()
{
  if (m_browseMenuStack.empty())
  {
    CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowse::ApplyBrowseMenuFromStack - BrowseMenu stack is EMPTY. [BrowseMenuStackSize=%zu] (bm)(bmst)",m_browseMenuStack.size());
    return;
  }

  std::stack<CStdString> browseMenuStack = m_browseMenuStack;
  int menuLevelToSet = (int)browseMenuStack.size() - 1;

  while (!browseMenuStack.empty())
  {
    CStdString menuId = browseMenuStack.top();
    browseMenuStack.pop();

    CFileItemList* browseMenu = NULL;
    browseMenu = CBoxeeBrowseMenuManager::GetInstance().GetMenu(menuId);

    if (!browseMenu)
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowse::ApplyBrowseMenuFromStack - FAILED to get browse menu for [menuId=%s]. [windowId=%d] (bm)",menuId.c_str(),GetID());
      return;
    }

    int browseMenuListControlId = BROWSE_MENU_LIST_LEVEL_START_ID + menuLevelToSet;

    AddButtonItemsToBrowseMenu(*browseMenu,browseMenuListControlId,true);

    menuLevelToSet--;
  }

  g_settings.SetSkinString(g_settings.TranslateSkinString("activemenulevel"),BOXEE::BXUtils::IntToString((int)m_browseMenuStack.size()));
  UpdateTopBrowseMenuSelection();
}

void CSelectBrowseMenuItemInfo::Reset()
{
  m_menuId = "";
  m_selectedIndex = -1;
  m_selectedLabel = "";
}

void CSelectBrowseMenuItemInfo::Set(const CStdString& menuId, int selectedIndex, const CStdString& selectedLabel)
{
  m_menuId = menuId;
  m_selectedIndex = selectedIndex;
  m_selectedLabel = selectedLabel;

  CLog::Log(LOGDEBUG,"CSelectBrowseMenuItemInfo::Set - after set [menuId=%s][selectedIndex=%d][selectedLabel=%s] (bm)",m_menuId.c_str(),m_selectedIndex,m_selectedLabel.c_str());
}

