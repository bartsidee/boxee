
#include "GUIWindowBoxeeBrowseApps.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/boxee.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIWindowStateDatabase.h"
#include "BoxeeBrowseMenuManager.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define MY_APPS_WINDOW_LABEL       9001
#define APPS_LIBRARY_WINDOW_LABEL  9002
#define MY_APPS_WINDOW_LABEL       9001

#define REPOSITORIES_FLAG         "repositories-flag"

#define MENU_LIST             100
#define BUTTON_SORT           110
#define BTN_MY_APPS           120
#define BTN_MY_APPS2          121
#define BTN_APP_STORE         130
#define BTN_APP_STORE2        132

#define BUTTON_GENRES         150
#define BTN_MY_REPOSITORIES   200
//#define BTN_SEARCH            160

#define BTN_INSTALLED_APPS  172

#define STATE_SHOW_MY_APPS          "favorite"
#define STATE_SHOW_ALL_APPS         "all"
#define STATE_SHOW_MY_REPOSITORIES  "repos"
#define STATE_SHOW_FEATURED         "featured"

#define APP_THUMB_VIEW        55
#define APP_LINE_VIEW         56

// Filters

#define FILTER_APPS_VIDEO_ID      600
#define FILTER_APPS_VIDEO_NAME    "appsvideo"

#define FILTER_APPS_MUSIC_ID      601
#define FILTER_APPS_MUSIC_NAME    "appsmusic"

#define FILTER_APPS_PICTURES_ID   602
#define FILTER_APPS_PICTURES_NAME "appspictures"

#define SHOW_FILTERS_AND_SORT     9014
#define SHOW_FILTERS_AND_SORT_FLAG "filters-and-sort"

#define SWITCH_VIEW_THUMBS   8001
#define SWITCH_VIEW_LIST   8002
#define SWITCH_VIEW_FLAG "show-thumbnails"

#define BTN_INSTALLED_APPS_FLAG   "installed-apps"

#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define ITEM_COUNT_LABEL "item-summary-count"

CMyAppsSource::CMyAppsSource(int iWindowID) : CBrowseWindowSource("installedappssource", "apps://all/", iWindowID)
{
}

CMyAppsSource::~CMyAppsSource()
{

}

void CMyAppsSource::BindItems(CFileItemList& items)
{
  // Perform post processing here
  CBrowseWindowPropertyValueFilter* filter = NULL;

  if (m_mapFilters.find("apptype") != m_mapFilters.end())
  {
    CStdString filtername = m_mapFilters["apptype"];

    if (filtername != "")
    {

      if (filtername == "video")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_VIDEO_ID, FILTER_APPS_VIDEO_NAME, "app-media", "video");
      }
      else if (filtername == "music")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_MUSIC_ID, FILTER_APPS_MUSIC_NAME, "app-media", "music");
      }
      else if (filtername == "pictures")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_PICTURES_ID, FILTER_APPS_PICTURES_NAME, "app-media", "pictures");
      }

      if (filter != NULL)
      {
        int i = 0;

        while (i < items.Size())
        {
          if (filter->Apply(&*items[i]) != true)
          {//need to remove this item
            items.Remove(i);
          }
          else
          {
            i++;
          }
        }

        delete filter;
      }
    }
  }

  CBrowseWindowSource::BindItems(items);
}



CAppStoreSource::CAppStoreSource(int iWindowID) : CBrowseWindowSource("appstoresource", "appbox://all/", iWindowID)
{
}

CAppStoreSource::~CAppStoreSource()
{

}

void CAppStoreSource::BindItems(CFileItemList& items)
{
  // Perform post processing here
  CBrowseWindowPropertyValueFilter* filter = NULL;

  if (m_mapFilters.find("apptype") != m_mapFilters.end())
  {
    CStdString filtername = m_mapFilters["apptype"];

    if (filtername != "")
    {
      if (filtername == "video")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_VIDEO_ID, FILTER_APPS_VIDEO_NAME, "app-media", "video");
      }
      else if (filtername == "music")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_MUSIC_ID, FILTER_APPS_MUSIC_NAME, "app-media", "music");
      }
      else if (filtername == "pictures")
      {
        filter = new CBrowseWindowPropertyValueFilter(FILTER_APPS_PICTURES_ID, FILTER_APPS_PICTURES_NAME, "app-media", "pictures");
      }
    }

    if (filter != NULL)
    {
      int i = 0;

      while (i < items.Size())
      {
        if (filter->Apply(&*items[i]) != true)
        {//need to remove this item
          items.Remove(i);
        }
        else
        {
          i++;
        }
      }

      delete filter;
    }
  }

  CBrowseWindowSource::BindItems(items);
}

void CAppStoreSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_strFilterCategory.empty())
  {
    mapOptions["category"] = m_strFilterCategory;
  }
}

CRepositorySource::CRepositorySource(int iWindowID) : CBrowseWindowSource("reposource", "appbox://all/repository", iWindowID)
{
}

CRepositorySource::~CRepositorySource()
{

}

void CRepositorySource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_strRepositoryId.empty())
    mapOptions["id"] = m_strRepositoryId;

  if (!m_strLabel.empty())
    mapOptions["label"] = m_strLabel;
}

// STATE IMPLEMENTATION

CAppsWindowState::CAppsWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  // Initialize default path to show installed applications
  m_sourceController.AddSource(new CMyAppsSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CAppStoreSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CRepositorySource(m_pWindow->GetID()));

  m_strApplicationType = g_localizeStrings.Get(53800);

  // Initialize sort vector
  SetApplicationType(m_strApplicationType);
}

void CAppsWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());
}

CStdString CAppsWindowState::GetFilterCategory()
{
  return m_strFilterCategory;
}

CStdString CAppsWindowState::GetItemSummary()
{
  CStdString itemSummary = "";
  std::map<CStdString , CStdString> mapTitleItemValue;

  if (GetCategory() == STATE_SHOW_MY_REPOSITORIES)
  {
    return m_strCurrentRepository;
  }

  if (!m_sort.m_sortName.empty() && m_sort.m_id != VIEW_SORT_METHOD_ATOZ && m_sort.m_id != VIEW_SORT_METHOD_ZTOA)
  {
    mapTitleItemValue["sort"] = m_sort.m_sortName;
  }

  if (!m_strFilterCategory.IsEmpty())
  {
    CStdString filterCategoryLabel = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetAppCategoryLabel(m_strFilterCategory);
    if (!filterCategoryLabel.IsEmpty())
    {
      mapTitleItemValue["filter"] = filterCategoryLabel;
    }
  }

  if (GetCategory() == STATE_SHOW_MY_APPS)
  {
    mapTitleItemValue["source"] = g_localizeStrings.Get(53825);
  }

  if (GetCategory() == STATE_SHOW_FEATURED)
  {
    mapTitleItemValue["source"] = g_localizeStrings.Get(54063);
  }

  mapTitleItemValue["media"] = g_localizeStrings.Get(53914);

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90000), mapTitleItemValue,itemSummary))
  {
    itemSummary = g_localizeStrings.Get(53914);
    CLog::Log(LOGERROR,"CAppsWindowState::GetItemSummary, Error in Strings.xml for the current language [id=90000], the template is bad. (browse)");
  }

  return itemSummary;
}


void CAppsWindowState::SetDefaultView()
{
  m_iCurrentView = APP_THUMB_VIEW;
}

/*
void CAppsWindowState::SortItems(CFileItemList &items)
{
  if (InSearchMode())
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::SortItems - Enter function [InSearchMode=TRUE]. [itemsListSize=%d] (bapps)(browse)",items.Size());

    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  }
  else
  {
    CLog::Log(LOGDEBUG,"CAppsWindowState::SortItems - Enter function [InSearchMode=FALSE]. Going to call CBrowseWindowState::SortItems(). [itemsListSize=%d] (bapps)(browse)",items.Size());

    CBrowseWindowState::SortItems(items);
  }
}
*/


bool CAppsWindowState::OnBack()
{
  CLog::Log(LOGDEBUG,"CAppsWindowState::OnBack - Enter function Going to call OnSearchEnd()  (bapps)(browse)");

  if (GetCategory() == STATE_SHOW_MY_REPOSITORIES)
  {// if we're in repositories

    //set the previous values of the screen
    SetCategory(m_iPreviousCategory);
    m_sort = m_savedSort;

    //activate the repositories browse screen
    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_REPOSITORIES);

    return true;
  }
  else
  {
    CBrowseWindowState::OnBack();
  }

  return false;
}

void CAppsWindowState::SetCategory(const CStdString& strCategory)
{
  m_sourceController.ActivateAllSources(false,true);

  if (strCategory == STATE_SHOW_MY_APPS)
  {
    OnMyApps();
    m_pWindow->SetProperty(BTN_INSTALLED_APPS_FLAG, true);
    m_pWindow->SetProperty(REPOSITORIES_FLAG, false);
  }
  else if (strCategory == STATE_SHOW_ALL_APPS || strCategory == STATE_SHOW_FEATURED)
  {
    OnAllApps(); 
    m_pWindow->SetProperty(BTN_INSTALLED_APPS_FLAG, false);
    m_pWindow->SetProperty(REPOSITORIES_FLAG, false);
  }
  else if (strCategory == STATE_SHOW_MY_REPOSITORIES)
  {
    OnRepositories();
    m_pWindow->SetProperty(BTN_INSTALLED_APPS_FLAG, false);
    m_pWindow->SetProperty(REPOSITORIES_FLAG, true);
  }

  CBrowseWindowState::SetCategory(strCategory);
}

bool CAppsWindowState::OnMyApps()
{
  m_iPreviousCategory = GetCategory();

  // Update the controller with new source
  m_sourceController.ActivateSource("installedappssource",true,true);

  // Fill relevant sort methods
  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53535), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_LAST_USED, SORT_METHOD_APP_LAST_USED_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(54250), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_USAGE, SORT_METHOD_APP_USAGE, SORT_ORDER_DESC, g_localizeStrings.Get(53507), ""));

  return true;
}

bool CAppsWindowState::OnAllApps()
{
  m_iPreviousCategory = GetCategory();

  m_sourceController.ActivateSource("appstoresource",true,true);

  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_POPULARITY, SORT_METHOD_APP_POPULARITY, SORT_ORDER_DESC, g_localizeStrings.Get(53504), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53535), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_APP_RELEASE_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53506), ""));

  CLog::Log(LOGDEBUG,"CAppsWindowState::OnAllApps - After set [Category=%s] (bapps)(browse)",GetCategory().c_str());

  return true;
}

bool CAppsWindowState::OnRepositories()
{
  CLog::Log(LOGDEBUG,"CAppsWindowState::OnRepositories - Enter function. Going to open WINDOW_BOXEE_BROWSE_REPOSITORIES (bapps)");

  if (GetCategory() != STATE_SHOW_MY_REPOSITORIES)
  {
    m_iPreviousCategory = GetCategory();
    m_savedSort = m_sort;

    m_sourceController.ActivateSource("reposource",true,true);
  }

  return true;
}

void CAppsWindowState::SetDefaultCategory()
{
  CStdString strCategory;

  CGUIWindowStateDatabase sdb;
  sdb.GetDefaultCategory(m_pWindow->GetID(), strCategory);

  if (!strCategory.IsEmpty())
  {
    SetCategory(strCategory.ToLower());
  }
  else
  {
    SetCategory("all");
  }
}


void CAppsWindowState::SetRepository(const CStdString& strRepositoryId, const CStdString& strLabel)
{
  CRepositorySource* source = (CRepositorySource*)m_sourceController.GetSourceById("reposource");

  if (source)
  {
    source->m_strRepositoryId = strRepositoryId;
    source->m_strLabel = strLabel;
    m_strCurrentRepository = strLabel;
  }
}

void CAppsWindowState::OnBind(CFileItemList& items)
{
  CBrowseWindowState::OnBind(items);
}

void CAppsWindowState::SetCategoryFilter(const CStdString& filter)
{
  CAppStoreSource* source = (CAppStoreSource*)m_sourceController.GetSourceById("appstoresource");

  m_strFilterCategory = filter;
  if (source)
  {
    source->m_strFilterCategory = filter;
  }
}

void CAppsWindowState::SetApplicationType(const CStdString& strType)
{
  // Remove all previous application type filters
//  RemoveLocalFilter(FILTER_APPS_VIDEO_NAME);
//  RemoveLocalFilter(FILTER_APPS_MUSIC_NAME);
//  RemoveLocalFilter(FILTER_APPS_PICTURES_NAME);

  m_pWindow->SetProperty("type-label", strType);
  m_strApplicationType = strType;

  if (strType == g_localizeStrings.Get(53800)) // all applications
  {
    // Show all applications
    CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=ALL] (bapps)",strType.c_str());
    m_pWindow->SetProperty("type-set", false);

    m_sourceController.ClearFilter("apptype");

  }
  else
  {
    // Specific type was selected
    m_pWindow->SetProperty("type-set", true);
    if (strType == "video")
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=VIDEO] (bapps)",strType.c_str());

      m_sourceController.SetFilter("apptype", "video");
    }
    else if (strType == "music")
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=MUSIC] (bapps)",strType.c_str());

      m_sourceController.SetFilter("apptype", "music");
    }
    else if (strType == "photos")
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s=PICTURES] (bapps)",strType.c_str());

      m_sourceController.SetFilter("apptype", "pictures");
    }
    else
    {
      CLog::Log(LOGDEBUG,"CAppsWindowState::SetApplicationType - Handling click on [%s] which is UNKNOWN (bapps)",strType.c_str());
    }
  }
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseApps::CGUIWindowBoxeeBrowseApps() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_APPS,"boxee_browse_apps.xml")
{
  m_strItemDescription = g_localizeStrings.Get(90040);

  SetWindowState(new CAppsWindowState(this));
}

CGUIWindowBoxeeBrowseApps::~CGUIWindowBoxeeBrowseApps()
{

}

bool CGUIWindowBoxeeBrowseApps::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_UPDATE:
  {
    if (message.GetSenderId() == WINDOW_INVALID && message.GetControlId() == GUI_MSG_APPS_CATEGORIES_UPDATE)
    {
      //CBoxeeBrowseMenuManager::GetInstance().ClearMenu("mn_library_apps_categories");
      return true;
    }
  }
  break;
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == SWITCH_VIEW_LIST || iControl == SWITCH_VIEW_THUMBS)
    {
      SetProperty(SWITCH_VIEW_FLAG, !GetPropertyBOOL(SWITCH_VIEW_FLAG));
      return true;
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

/*
bool CGUIWindowBoxeeBrowseApps::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnMessage - GUI_MSG_CLICKED - [control=%d] (bapps)(browse)", iControl);

    if (iControl == BTN_MY_APPS || iControl == BTN_MY_APPS2)
    {
      if (!GetPropertyBOOL(BTN_INSTALLED_APPS_FLAG))
      {
        m_windowState->SetCategory(STATE_SHOW_MY_APPS);
        Refresh();
      }

      return true;
    }
    if (iControl == BTN_APP_STORE || iControl == BTN_APP_STORE2)
    {
      if (GetPropertyBOOL(BTN_INSTALLED_APPS_FLAG))
      {
        m_windowState->SetCategory(STATE_SHOW_ALL_APPS);
        Refresh();
      }

      return true;
    }


    else if (iControl == BTN_MY_REPOSITORIES)
    {
      //ResetHistory();

      ((CAppsWindowState*)m_windowState)->OnRepositories();
      //g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_REPOSITORIES);

      return true;
    }

    else if (iControl == SWITCH_VIEW_THUMBS || iControl == SWITCH_VIEW_LIST)
    {
      SetProperty(SWITCH_VIEW_FLAG, !GetPropertyBOOL(SWITCH_VIEW_FLAG));
      ((CAppsWindowState*)m_windowState)->SetLineView(!((CAppsWindowState*)m_windowState)->IsLineView());

      return true;
    }
    else if (iControl == BTN_INSTALLED_APPS)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnMessage - GUI_MSG_CLICKED - [control=%d=BTN_INSTALLED_APPS] - Going to call HandleBtnApplicationType (bapps)(browse)", iControl);

      return HandleBtnApplicationType();
    }
    else if (iControl == SHOW_FILTERS_AND_SORT)
    {
      SetProperty(SHOW_FILTERS_AND_SORT_FLAG, !GetPropertyBOOL(SHOW_FILTERS_AND_SORT_FLAG));    
      //Refresh(true);
      return true;
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED

  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}
*/

bool CGUIWindowBoxeeBrowseApps::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnClick - Enter function with [item=%d] (bapps)(browse)", iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
  {
    return true;
  }

  item.Dump();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::OnClick - Going to open MediaAction with item [label=%s][path=%s] (bapps)(browse)", item.GetLabel().c_str(),item.m_strPath.c_str());

  return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
}

bool CGUIWindowBoxeeBrowseApps::HandleBtnApplicationType()
{
  CFileItemList applicationTypes;
  FillDropdownWithApplicationTypes(applicationTypes);

  CStdString value;

  CGUIDialogBoxeeDropdown *dialog = (CGUIDialogBoxeeDropdown *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_DROPDOWN);

  if (dialog && dialog->Show(applicationTypes, g_localizeStrings.Get(53563), value, 900, 1))
  {
    ((CAppsWindowState*)m_windowState)->SetApplicationType(value);

    Refresh();
    return true;
  }

  return false;
}

bool CGUIWindowBoxeeBrowseApps::FillDropdownWithApplicationTypes(CFileItemList& applicationTypes)
{
  CFileItemPtr appTypeAll (new CFileItem(g_localizeStrings.Get(53800)));
  appTypeAll->SetProperty("type", "apps-type");
  appTypeAll->SetProperty("value", g_localizeStrings.Get(53800));
  applicationTypes.Add(appTypeAll);

  CFileItemPtr appTypeVideo (new CFileItem(g_localizeStrings.Get(53801)));
  appTypeVideo->SetProperty("type", "apps-type");
  appTypeVideo->SetProperty("value", g_localizeStrings.Get(53801));
  applicationTypes.Add(appTypeVideo);

  CFileItemPtr appTypeMusic (new CFileItem(g_localizeStrings.Get(53802)));
  appTypeMusic->SetProperty("type", "apps-type");
  appTypeMusic->SetProperty("value", g_localizeStrings.Get(53802));
  applicationTypes.Add(appTypeMusic);

  CFileItemPtr appTypePictures (new CFileItem(g_localizeStrings.Get(53803)));
  appTypePictures->SetProperty("type", "apps-type");
  appTypePictures->SetProperty("value", g_localizeStrings.Get(53803));
  applicationTypes.Add(appTypePictures);

  return true;
}

void CGUIWindowBoxeeBrowseApps::ShowItems(CFileItemList& list, bool append)
{
  CGUIWindowBoxeeBrowse::ShowItems(list,append);

  SetProperty(SWITCH_VIEW_FLAG, (m_windowState->GetCurrentView() != APP_THUMB_VIEW));

  if ((m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ATOZ) ||
      (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ZTOA))
  { //if the sort is A to Z and we're in local content, we should update the A to Z scroll
    GenerateAlphabetScrollbar(list);
  }
}

void CGUIWindowBoxeeBrowseApps::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.find("repository") != optionsMap.end() && optionsMap.find("label") != optionsMap.end())
    {
      CStdString strRepositories = optionsMap["repository"];
      CStdString strLabel = optionsMap["label"];

      ((CAppsWindowState*)m_windowState)->SetCategory(STATE_SHOW_MY_REPOSITORIES);
      ((CAppsWindowState*)m_windowState)->SetRepository(strRepositories, strLabel);
    }

    CStdString strFilterCategory;

    if (optionsMap.find("categoryfilter") != optionsMap.end())
    {
      strFilterCategory = optionsMap["categoryfilter"];
    }

    ((CAppsWindowState*)m_windowState)->SetCategoryFilter(strFilterCategory);
  }
}

void CGUIWindowBoxeeBrowseApps::SetWindowLabel(int controlId, const CStdString windowLabel)
{
  SET_CONTROL_LABEL(controlId,windowLabel);
}

bool CGUIWindowBoxeeBrowseApps::IsInMyAppsState()
{
  return (((CAppsWindowState*)m_windowState)->GetCategory() == STATE_SHOW_MY_APPS);
}

void CGUIWindowBoxeeBrowseApps::FromRepositories()
{
  (((CAppsWindowState*)m_windowState)->SetCategory(STATE_SHOW_MY_REPOSITORIES));
}

bool CGUIWindowBoxeeBrowseApps::HandleEmptyState()
{
  int saveFocusedControl = GetFocusedControlID();
  bool isEmpty = CGUIWindowBoxeeBrowse::HandleEmptyState();

  if (isEmpty && g_application.IsConnectedToInternet())
  {
    SET_CONTROL_FOCUS(saveFocusedControl,0);
  }

  return isEmpty;
}

void CGUIWindowBoxeeBrowseApps::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();
  CStdString filterCategory = ((CAppsWindowState*)m_windowState)->GetFilterCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::GetStartMenusStructure - enter function [category=%s][filterCategory=%s] (bm)",category.c_str(),filterCategory.c_str());

  CStdString startMenuId = "";

  if (filterCategory.IsEmpty() || filterCategory == "99")
  {
    startMenuId = "mn_library_apps";

    if (filterCategory == "99")
    {
      m_initSelectPosInBrowseMenu = 2;
    }

    if (category == STATE_SHOW_MY_APPS)
    {
      m_initSelectPosInBrowseMenu = 1;
    }
    else if (category == STATE_SHOW_MY_REPOSITORIES)
    {
      m_initSelectPosInBrowseMenu = 5;
    }

  }
  else
  {
    startMenuId = "mn_library_apps_categories";
    ((CAppsWindowState*)m_windowState)->SetCategoryFilter(filterCategory);

    if (filterCategory == "9")
    {
      m_initSelectPosInBrowseMenu = 7;
    }
    else if (filterCategory == "11")
    {
      m_initSelectPosInBrowseMenu = 9;
    }
  }

  if (!startMenuId.IsEmpty())
  {
    CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure(startMenuId,browseMenuLevelList);
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu]. [category=%s] (bm)",browseMenuLevelList.size(),category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseApps::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());
}

