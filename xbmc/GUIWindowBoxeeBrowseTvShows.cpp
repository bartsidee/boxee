#include "GUIWindowBoxeeBrowseTvShows.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIWindowBoxeeBrowseTvEpisodes.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIDialogBoxeeBrowseMenu.h"
#include "boxee.h"
#include "bxvideodatabase.h"
#include "lib/libBoxee/boxee.h"
#include "SpecialProtocol.h"
#include "BoxeeDatabaseDirectory.h"
#include "GUIDialogBoxeeNetworkNotification.h"
#include "Application.h"
#include "Picture.h"
#include "FileSystem/File.h"
#include "GUIUserMessages.h"
#include "MediaSource.h"
#include "GUISettings.h"
#include "GUIWindowStateDatabase.h"
#include "GUIDialogYesNo.h"
#include "GUIDialogBoxeeChannelFilter.h"
#include "BoxeeBrowseMenuManager.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_SORT    110
#define BUTTON_MY      120
#define BUTTON_ALL     130
#define BUTTON_SHOW_LIBRARY 131
#define BUTTON_GENRES  150
#define BUTTON_FREE 151
#define BUTTON_SOURCES  153

//#define BUTTON_UNWATCHED 152
//#define BUTTON_SEARCH  160
//#define BUTTON_SEARCH_SHOW  161
//#define BUTTON_BROWSE_SOURCES 7002

#define BROWSE_MENU 9001

#define DROP_DOWN_READY	9010
#define DROP_DOWN_STORE 9011

#define LABEL_ITEMS_COUNT   9019

#define THUMB_VIEW_LIST       50
#define LINE_VIEW_LIST        51

#define HIDDEN_CONTAINER      5000

#define SORT_LABEL_FLAG "sort-label"
#define GENRE_LABEL_FLAG "genre-label"
#define GENRE_SET_FLAG "genre-set"
#define SOURCE_LABEL_FLAG "source-label"
#define SOURCE_SET_FLAG "source-set"
#define FREE_LABEL_FLAG "free-label"
#define FREE_SET_FLAG "free-set"
#define SEARCH_LABEL_FLAG "search-label"
#define UNWATCHED_SET_FLAG "unwatched-set"
#define UNWATCHED_LABEL_FLAG "unwatched-label"
#define SCANNING_FLAG "scanning-label"
#define READY_LABEL_FLAG "ready-to-watch-label"
#define READY_SET_FLAG "ready-to-watch-set"
#define STORE_FLAG "movies-store"
#define STORE_LABEL_FLAG "movies-store-label"
#define SEARCH_BUTTON_FLAG "search-button"

#define GENRE_FILTER_ID   600
#define GENRE_FILTER_NAME "genre"

#define UNWATCHED_FILTER_ID   601
#define UNWATCHED_FILTER_NAME "unwatched"

#define READY_TO_WATCH_FILTER_ID 602
#define READY_TO_WATCH_FILTER_NAME "ready"

#define SWITCH_VIEW_THUMBS   8001
#define SWITCH_VIEW_LIST   8002
#define SWITCH_VIEW_FLAG "show-thumbnails"
//#define SORT_DROPDOWN_BUTTON 8014

#define SCANNING_LABEL          501
#define RESOLVED_VIDEO_LABEL    502

#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define ITEM_COUNT_LABEL "item-summary-count"

#define LETTER_SCROLLBAR 7000

#define USERNAME_DROPDOWN_POS_X                          0.0
#define USERNAME_DROPDOWN_POS_Y                          15.0

#define EMPTY_FAVORITES_CANCEL_BTN   7191
#define EMPTY_STATE_LOCAL_BTN        7193

// Sources for the TV shows window ///////////////////////////

CRemoteTVShowsSource::CRemoteTVShowsSource(int iWindowID) : CBrowseWindowSource("remotetvsource", "boxee://tvshows/shows/", iWindowID)
{
  SetPageSize(CURRENT_PAGE_SIZE);
}

CRemoteTVShowsSource::~CRemoteTVShowsSource(){}

void CRemoteTVShowsSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_sourceSort.m_id.IsEmpty())
  {
    CStdString strSortMethod = m_sourceSort.m_id;
    CUtil::URLEncode(strSortMethod);
    CUtil::URLEncode(strSortMethod); //the CURL GetOptionsAsMap is doing url decode, it may leave us without encoding
    mapOptions["sort"] = strSortMethod;
  }

  if (!m_mapFilters["genre"].empty())
  {
    CStdString strGenre = m_mapFilters["genre"];
    CUtil::URLEncode(strGenre);
    mapOptions["genre"] = strGenre;
  }

  CStdString excludedChannels = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetExcludedSources();
  if (!excludedChannels.IsEmpty())
  {
    CUtil::URLEncode(excludedChannels);
    mapOptions["provider_exclude"] = excludedChannels;
  }

  //mapOptions["free"] = "true";

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

CTVShowsStoreSource::CTVShowsStoreSource(int iWindowID) : CBrowseWindowSource("store", "boxee://tvshows/shows/", iWindowID)
{
  SetPageSize(CURRENT_PAGE_SIZE);
}

CTVShowsStoreSource::~CTVShowsStoreSource(){}

void CTVShowsStoreSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_sourceSort.m_id.IsEmpty())
  {
    CStdString strSortMethod = m_sourceSort.m_id;
    CUtil::URLEncode(strSortMethod);
    CUtil::URLEncode(strSortMethod); //the CURL GetOptionsAsMap is doing url decode, it may leave us without encoding
    mapOptions["sort"] = strSortMethod;
  }

  if (!m_mapFilters["genre"].empty())
  {
    CStdString strGenre = m_mapFilters["genre"];
    CUtil::URLEncode(strGenre);
    mapOptions["genre"] = strGenre;
  }

  if (!m_mapFilters["provider"].empty())
  {
    CStdString strProvider = m_mapFilters["provider"];
    CUtil::URLEncode(strProvider);
    mapOptions["provider"] = strProvider;
  }

  CBrowseWindowSource::AddStateParameters(mapOptions);
}


CLocalTVShowsSource::CLocalTVShowsSource(int iWindowID) : CBrowseWindowSource("localtvsource", "boxeedb://tvshows/", iWindowID)
{

}

CLocalTVShowsSource::~CLocalTVShowsSource(){}

void CLocalTVShowsSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_mapFilters["genre"].empty())
  {
    mapOptions["genre"] = m_mapFilters["genre"];
  }

  if (m_mapFilters.find("unwatched") != m_mapFilters.end() && m_mapFilters["unwatched"].Compare("true") == 0)
    mapOptions["unwatched"] = "true";

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

void CLocalTVShowsSource::BindItems(CFileItemList& items)
{
  // Perform post processing here
  CBrowseWindowTvShowUnwatchedFilter* filter = NULL;

  if (m_mapFilters.find("unwatched") != m_mapFilters.end())
  {
    CStdString strShowUnwatched = m_mapFilters["unwatched"];

    if (strShowUnwatched == "true")
    {
      filter = new CBrowseWindowTvShowUnwatchedFilter(UNWATCHED_FILTER_ID,UNWATCHED_FILTER_NAME, true);

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

CTVShowsSubscriptionsSource::CTVShowsSubscriptionsSource(int iWindowID) : CBrowseWindowSource("subscriptionstvsource", "boxee://subscriptions/", iWindowID)
{
}

CTVShowsSubscriptionsSource::~CTVShowsSubscriptionsSource(){}

void CTVShowsSubscriptionsSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_sourceSort.m_id.IsEmpty())
  {
    CStdString strSortMethod = m_sourceSort.m_id;
    CUtil::URLEncode(strSortMethod);
    CUtil::URLEncode(strSortMethod); //the CURL GetOptionsAsMap is doing url decode, it may leave us without encoding
    mapOptions["sort"] = strSortMethod;
  }
}

////////////////////////////////////////////////////
// TV Shows window state ///////////////////////////

CTvShowsWindowState::CTvShowsWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.RemoveAllSources();
  m_sourceController.AddSource(new CRemoteTVShowsSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CLocalTVShowsSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CTVShowsStoreSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CTVShowsSubscriptionsSource(m_pWindow->GetID()));

  m_iLastSelectedItem = 0;
  m_bUnwatched = false;
}

void CTvShowsWindowState::ResetFilters()
{
  GenreItem defaultGenre;

  SetGenre(defaultGenre); // all
  //SetSource(g_localizeStrings.Get(53512), g_localizeStrings.Get(53512));
}

// /////////////////////////////////////////////////////////
// Filter functions
void CTvShowsWindowState::SetGenre(const GenreItem& genre)
{
  m_currentGenre = genre;

  // If All Genres is selected, genre filter should be disabled
  if (m_currentGenre.m_genreId != "all")
  {
    m_sourceController.SetFilter("genre", m_currentGenre.m_genreId);
  }
  else
  {
    m_sourceController.ClearFilter("genre");
  }
}

void CTvShowsWindowState::SetUnwatched(bool unwatched)
{
  m_bUnwatched = unwatched;

  if (m_bUnwatched)
  {
    m_sourceController.SetFilter("unwatched", "true");
  }
  else
  {
    m_sourceController.ClearFilter("unwatched");
  }
}

void CTvShowsWindowState::SetStore(const CStdString& strStoreId)
{
  m_strStoreId = strStoreId;

  if (strStoreId.CompareNoCase(g_localizeStrings.Get(54911)) != 0)
  {
    std::vector<BXSourcesItem> vecSources;

    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvSources(vecSources);

    for (std::vector<BXSourcesItem>::iterator it = vecSources.begin() ; it != vecSources.end() ; ++it)
    {
      if (strStoreId == it->GetSourceId())
      {
        m_strStoreName = it->GetSourceName();
        break;
      }
    }

    m_pWindow->SetProperty(STORE_LABEL_FLAG, strStoreId);
    m_pWindow->SetProperty(STORE_FLAG, true);
    m_sourceController.SetFilter("provider", strStoreId);
  }
  else
  {
    m_pWindow->SetProperty(STORE_LABEL_FLAG, "");
    m_pWindow->SetProperty(STORE_FLAG, false);
    m_sourceController.ClearFilter("provider");
    m_strStoreName = "";
  }
}



CStdString CTvShowsWindowState::GetItemSummary()
{
  std::map<CStdString , CStdString> mapTitleItemValue;
  CStdString strSummary = "";

  if (!m_sort.m_sortName.empty())
  {
    if (m_sort.m_id == VIEW_SORT_METHOD_RELEASE)
    {
      mapTitleItemValue["sort"] = g_localizeStrings.Get(53534);
    }
    else if (m_sort.m_id == VIEW_SORT_METHOD_RELEASE_REVERSE)
    {
      mapTitleItemValue["sort"] = g_localizeStrings.Get(53533);
    }
    else if ( m_sort.m_id != VIEW_SORT_METHOD_ATOZ && m_sort.m_id != VIEW_SORT_METHOD_ZTOA)
    {//show the sort only if its not A TO Z
      mapTitleItemValue["sort"] = m_sort.m_sortName;
    }

    //if there is a prefix in our language and we have a sort in the title, append the relevant prefix
    if (!g_localizeStrings.Get(90006).IsEmpty() && mapTitleItemValue.find("sort") != mapTitleItemValue.end())
      mapTitleItemValue["sortprefix"] = g_localizeStrings.Get(90006);
  }

  /*
  if (m_bUnwatched)
  {
    mapTitleItemValue["filter"] = g_localizeStrings.Get(53714);
  }
  */

  if (m_currentGenre.m_genreId != "all")
  {
    mapTitleItemValue["filter"] = m_currentGenre.m_genreText;
  }

  if (!m_strStoreName.empty())
  { //append the string
    mapTitleItemValue["channel"] = m_strStoreName;

    if (!g_localizeStrings.Get(90007).IsEmpty())
      mapTitleItemValue["channelprefix"] = g_localizeStrings.Get(90007);
  }

  if (GetCategory().CompareNoCase("favorite")==0)
  {
    mapTitleItemValue["source"] = g_localizeStrings.Get(53729);
  }

  if (GetCategory().CompareNoCase("local")==0)
  {
    mapTitleItemValue["source"] = g_localizeStrings.Get(53753);
  }

  mapTitleItemValue["media"] = g_localizeStrings.Get(20343); //should have "Shows"

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90002), mapTitleItemValue,strSummary))
  {
    strSummary = g_localizeStrings.Get(20343);
    CLog::Log(LOGERROR,"CTvShowsWindowState::GetItemSummary, Error in Strings.xml for the current language [id=90002], the template is bad. (browse)");
  }

  return strSummary;
}

void CTvShowsWindowState::SetCategory(const CStdString& strCategory)
{
  // Activate relevant sources according to selected category
  m_sourceController.ActivateAllSources(false, true);
  m_pWindow->SetProperty("is-category-favorites", false);
  m_pWindow->SetProperty("is-category-local", false);


  if (strCategory.CompareNoCase("local")==0)
  {
    bool IgnorePrefix = g_guiSettings.GetBool("sort.showstarter");
    m_vecSortMethods.clear();
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, IgnorePrefix?SORT_METHOD_LABEL_IGNORE_THE:SORT_METHOD_LABEL, SORT_ORDER_ASC , g_localizeStrings.Get(53535), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ZTOA, IgnorePrefix?SORT_METHOD_LABEL_IGNORE_THE:SORT_METHOD_LABEL, SORT_ORDER_DESC , g_localizeStrings.Get(53536), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_DATE, SORT_METHOD_DATE_ADDED, SORT_ORDER_DESC, g_localizeStrings.Get(51402), ""));

    m_sourceController.ActivateSource("localtvsource", true, true);
    m_pWindow->SetProperty("is-category-local", true);

    SetPageSize(DISABLE_PAGING);
  }
  else
  {
    m_vecSortMethods.clear();
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_POPULARITY, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53504), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53537), ""));

    if (strCategory.CompareNoCase("all")==0)
    {
      m_sourceController.ActivateSource("remotetvsource", true, true);
      SetPageSize(CURRENT_PAGE_SIZE);
    }
    else if (strCategory.CompareNoCase("store")==0)
    {
      m_sourceController.ActivateSource("store", true, true);
      SetPageSize(CURRENT_PAGE_SIZE);
    }
    else if (strCategory.CompareNoCase("favorite")==0)
    {
      m_vecSortMethods.clear();
      m_sourceController.ActivateSource("subscriptionstvsource", true, true);
      m_pWindow->SetProperty("is-category-favorites", true);
      SetPageSize(DISABLE_PAGING);
    }
  }

  CBrowseWindowState::SetCategory(strCategory);

  m_pWindow->SetProperty("is-category-default", !m_pWindow->GetPropertyBOOL("is-category-favorites") && !m_pWindow->GetPropertyBOOL("is-category-local"));
}

void CTvShowsWindowState::SetDefaultView()
{
  m_iCurrentView = THUMB_VIEW_LIST;
}

void CTvShowsWindowState::SetDefaultCategory()
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

void CTvShowsWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  if (bResetSelected)
    m_iSelectedItem = 0;

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());
}

// ///////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF THE CGUIWindowBoxeeBrowseTvShows ////////////////////

CGUIWindowBoxeeBrowseTvShows::CGUIWindowBoxeeBrowseTvShows(): CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_TVSHOWS, "boxee_browse_tvshows.xml"), m_renderCount(0)
{
  m_strItemDescription = g_localizeStrings.Get(90042);

  SetWindowState(new CTvShowsWindowState(this));
}

CGUIWindowBoxeeBrowseTvShows::CGUIWindowBoxeeBrowseTvShows(DWORD dwID, const CStdString &xmlFile) : CGUIWindowBoxeeBrowse(dwID, xmlFile)
{
  SetWindowState(new CTvShowsWindowState(this));
}

CGUIWindowBoxeeBrowseTvShows::~CGUIWindowBoxeeBrowseTvShows()
{
}

void CGUIWindowBoxeeBrowseTvShows::OnInitWindow()
{

  UpdateVideoCounters(true);

  SetProperty(SEARCH_BUTTON_FLAG, false);

  CGUIWindowBoxeeBrowse::OnInitWindow();

  //if we were in local sorted by a-z, we're not refreshing the window and we should use the currently items which are already in the view
  if (((m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ATOZ) ||
       (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ZTOA)) &&
       (m_windowState->GetCategory() == "local" || m_windowState->GetCategory() == "favorite"))
  { //if the sort is A to Z and we're in local content, we should update the A to Z scroll
    GenerateAlphabetScrollbar(m_vecViewItems);
  }
}

bool CGUIWindowBoxeeBrowseTvShows::HandleEmptyState()
{
  bool isEmpty = false;

  if (m_vecViewItems.Size() == 0)
  {
    isEmpty = true;

    SetProperty("empty", isEmpty);

    if (!GetPropertyBOOL("is-category-local"))
    {
      SET_CONTROL_FOCUS(BROWSE_SETTINGS,0);

      if (GetPropertyBOOL("is-category-favorites"))
      {
        SET_CONTROL_FOCUS(EMPTY_FAVORITES_CANCEL_BTN,0);
      }

      if (!g_application.IsConnectedToInternet())
      {
        SET_CONTROL_FOCUS(EMPTY_STATE_LOCAL_BTN,0);
      }
    }
    else
    {
      isEmpty &= CGUIWindowBoxeeBrowse::HandleEmptyState();
    }
  }
  else
  {
    SetProperty("empty",isEmpty);
  }

  return isEmpty;
}

void CGUIWindowBoxeeBrowseTvShows::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    GenreItem foundGenre;

    if (optionsMap.find("genre") != optionsMap.end())
    {
      CStdString strGenreId = optionsMap["genre"];

      std::vector<GenreItem> vecGenres;

      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvGenres(vecGenres);

      for (std::vector<GenreItem>::iterator it = vecGenres.begin() ; it != vecGenres.end() ; ++it)
      {
        if (strGenreId == it->m_genreId)
        {
          foundGenre = (*it);
          break;
        }
      }
    }

    ((CTvShowsWindowState*)m_windowState)->SetGenre(foundGenre);
    UpdateUIGenre(foundGenre.m_genreText);

    CStdString strUnwatched = "false";
    if (optionsMap.find("unwatched") != optionsMap.end())
    {
      strUnwatched = optionsMap["unwatched"];
    }

    if (strUnwatched == "true")
      ((CTvShowsWindowState*)m_windowState)->SetUnwatched(true);
    else
      ((CTvShowsWindowState*)m_windowState)->SetUnwatched(false);

    if (optionsMap.find("provider") != optionsMap.end())
    {
      ((CTvShowsWindowState*)m_windowState)->SetCategory("store");
      ((CTvShowsWindowState*)m_windowState)->SetStore(optionsMap["provider"]);
    }
    else
    {
      ((CTvShowsWindowState*)m_windowState)->SetStore(g_localizeStrings.Get(54911));
    }
  }
}

void CGUIWindowBoxeeBrowseTvShows::UpdateUIGenre(const CStdString& strValue)
{
  SetProperty(GENRE_LABEL_FLAG, strValue);
  SetProperty(GENRE_SET_FLAG, strValue != "all" );
}

void CGUIWindowBoxeeBrowseTvShows::ShowItems(CFileItemList& list, bool append)
{
  CGUIWindowBoxeeBrowse::ShowItems(list,append);

  SetProperty(SWITCH_VIEW_FLAG, (m_windowState->GetCurrentView() != THUMB_VIEW_LIST));

  if (((m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ATOZ) ||
       (m_windowState->GetSort().m_id == VIEW_SORT_METHOD_ZTOA)) &&
       (m_windowState->GetCategory() == "local" || m_windowState->GetCategory() == "favorite"))
  {
    //if the sort is A to Z and we're in local content, we should update the A to Z scroll
    GenerateAlphabetScrollbar(list);
  }
}

bool CGUIWindowBoxeeBrowseTvShows::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_UPDATE:
  {
    if (message.GetSenderId() == WINDOW_INVALID && message.GetControlId() == GUI_MSG_MEDIA_CHANNELS_UPDATE)
    {
      //CBoxeeBrowseMenuManager::GetInstance().ClearMenu("mn_library_shows_providers");
      return true;
    }

    CStdString param = message.GetStringParam();

    //we want to avoid refreshing on paged windows unless we're on local content
    if (param.Equals("sort.showstarter") && m_windowState->GetCategory() != "local")
      return true;
    //geo lock is not relevant on local content, so we do not refresh
    else if (param.Equals("filelists.filtergeoip2") && m_windowState->GetCategory() == "local")
      return true;

    m_windowState->ClearCachedSources();
  }
  break;
  case GUI_MSG_WINDOW_INIT:
  {
    //don't refresh the window if we got the message from the window manager (previous window) and we're not in the favorite category
    if (message.GetParam1() == WINDOW_INVALID && m_windowState->GetCategory() != "favorite")
    {
      CGUIWindow::OnMessage(message);

      // Update current view type
      m_viewControl.SetCurrentView(m_windowState->GetCurrentView());
      m_viewControl.SetSelectedItem(m_windowState->GetSelectedItem());

      ApplyBrowseMenuFromStack();

      //call made from previous window, we should not reset our items because we are paged
      //continue as we were before
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
    else if (iControl == BROWSE_SETTINGS)
    {
      return CGUIDialogBoxeeChannelFilter::Show();
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}


bool CGUIWindowBoxeeBrowseTvShows::OnAction(const CAction &action)
{
  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseTvShows::OnClick(int iItem)
{
  ((CTvShowsWindowState*)m_windowState)->m_iLastSelectedItem = iItem;

  CFileItem item;
  if (!GetClickedItem(iItem, item))
      return true;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::OnClick, item clicked, path = %s (browse)", item.m_strPath.c_str());
  item.Dump();

  if (item.GetPropertyBOOL("istvshowfolder") && !item.GetProperty("boxeeid").IsEmpty())
  {
    CStdString strSeriesUrl;
    CStdString strTitle = item.GetLabel();
    CStdString strBoxeeId = item.GetProperty("boxeeid").c_str();

    CUtil::URLEncode(strBoxeeId);
    CUtil::URLEncode(strTitle);

    CStdString episodesCategory = "remote";

    if (m_windowState->GetCategory().CompareNoCase("local") == 0)
      episodesCategory = "local";

    strSeriesUrl.Format("boxeeui://tvshows/?category=%s&seriesId=%s&title=%s",episodesCategory.c_str(),strBoxeeId.c_str(),strTitle.c_str());

    if (!item.GetProperty("parentPath").IsEmpty())
    {
      std::map<CStdString, CStdString> optionsMap;
      CURI parentPath(item.GetProperty("parentPath"));
      optionsMap = parentPath.GetOptionsAsMap();

      if (optionsMap.find("provider") != optionsMap.end())
      {
        CStdString allowProviders = optionsMap["provider"];
        CUtil::URLEncode(allowProviders);
        strSeriesUrl += "&allowProviders=";
        strSeriesUrl += allowProviders;
      }
    }

    CGUIWindowBoxeeBrowseTvEpisodes* pEpisodeWindow = (CGUIWindowBoxeeBrowseTvEpisodes*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_TVEPISODES);
    if (pEpisodeWindow)
    {
      pEpisodeWindow->SetTvShowItem(item);
    }
    else
    {
      CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseTvShows::OnClick - FAILED to get BROWSE_TVEPISODE window in order to set TvShow item. [label=%s][path=%s] (browse)",item.GetLabel().c_str(),item.m_strPath.c_str());
    }

    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVEPISODES, strSeriesUrl);

    return true;
  }

  return CGUIWindowBoxeeBrowse::OnClick(iItem);
}
/*
void CGUIWindowBoxeeBrowseTvShows::FillGenresList(CFileItemList& genres)
{
  CFileItemPtr allItem (new CFileItem(g_localizeStrings.Get(53511)));
  allItem->SetProperty("type", "genre");
  allItem->SetProperty("value", g_localizeStrings.Get(54901));
  genres.Add(allItem);

  for (size_t i = 0; i < m_vecGenres.size(); i++) 
  {
    CFileItemPtr genreItem (new CFileItem(m_vecGenres[i].m_genreText));
    genreItem->SetProperty("type", "genre");
    genreItem->SetProperty("value", m_vecGenres[i].m_genreId);
    genres.Add(genreItem);
  }
}

void CGUIWindowBoxeeBrowseTvShows::FillReadyToWatchList(CFileItemList& ready)
{
  for (size_t i = 0; i < 4; i++)
  {
    CFileItemPtr item (new CFileItem(g_localizeStrings.Get(54901+i)));
    item->SetProperty("type", "ready_to_watch");
    item->SetProperty("value", (g_localizeStrings.Get(54901+i)));
    ready.Add(item);
  }
}
*/
/**
 * The purpose of this function is to update the local media resolver status in the TVshows and Movies screens
 */
void CGUIWindowBoxeeBrowseTvShows::UpdateVideoCounters(bool bOn)
{
  int total_count = 0;
  int resolved_count = 0;
  int unresolved_count = 0;

  bool is_scanning = false;

  if (bOn)
  {
    BOXEE::BXVideoDatabase video_db;

    //build video share list.

    DIRECTORY::CBoxeeDatabaseDirectory dummyDir;
    std::vector<std::string> vecVideoShares;
    if (!dummyDir.CreateShareFilter("video",vecVideoShares,false))
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvShows::SetVideoCounters - Couldnt create video share list");
      return;
    }

    CStdString video_share_list = "";
    for (size_t i = 0; i < vecVideoShares.size(); i++)
    {
      video_share_list += "\'";
      video_share_list += _P(vecVideoShares[i].c_str());
      video_share_list += "\'";

      if (i < vecVideoShares.size() -1 )
      {
        video_share_list += ',';
      }
    }


    total_count = video_db.GetUserUnresolvedVideoFilesCount(video_share_list, STATUS_ALL);
    resolved_count = video_db.GetUserUnresolvedVideoFilesCount(video_share_list, STATUS_RESOLVED);
    unresolved_count = video_db.GetUserUnresolvedVideoFilesCount(video_share_list, STATUS_UNRESOLVED);
    is_scanning = video_db.AreVideoFilesBeingScanned(video_share_list);

  }
  char     tmp[100];

  if (is_scanning)
  {
    SET_CONTROL_LABEL(SCANNING_LABEL, g_localizeStrings.Get(54065));
    SET_CONTROL_HIDDEN(RESOLVED_VIDEO_LABEL);
  }
  else
  {
    if (total_count > resolved_count + unresolved_count)
    {
      // We still have files with status NEW that were not reached by the resolver
      SET_CONTROL_LABEL(RESOLVED_VIDEO_LABEL, g_localizeStrings.Get(54068));

      sprintf(tmp, g_localizeStrings.Get(54067) , total_count);
      SET_CONTROL_LABEL(SCANNING_LABEL, tmp);
    }
    else if (total_count == resolved_count)
    {
      sprintf(tmp, g_localizeStrings.Get(54069) , total_count);
      SET_CONTROL_LABEL(SCANNING_LABEL, tmp);
      SET_CONTROL_HIDDEN(RESOLVED_VIDEO_LABEL);
    }
    else
    {
      sprintf(tmp, g_localizeStrings.Get(54067) , total_count);
      SET_CONTROL_LABEL(SCANNING_LABEL, tmp);

      sprintf(tmp, "%d files can not be identified", unresolved_count);
      SET_CONTROL_LABEL(RESOLVED_VIDEO_LABEL, tmp);
    }
  }
}

void CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  CStdString startMenuId = "";

  if (category == "all")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s=all] -> set library_shows menu (bm)",category.c_str());
    startMenuId = "mn_library_shows";
  }
  else if (category == "local")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s=local] -> set local_shows menu (bm)",category.c_str());
    startMenuId = "mn_local_shows";
  }
  else if (category == "store")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s] -> set library_shows_provider menu (bm)",category.c_str());
    startMenuId = "mn_library_shows_providers";
  }
  else if (category == "favorite")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - handle [category=%s] -> set mn_favorites_shows menu (bm)",category.c_str());
    startMenuId = "mn_favorites_shows";
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - FAILED to set menu since [category=%s] is unknown (bm)",category.c_str());
  }

  if (!startMenuId.IsEmpty())
  {
    CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure(startMenuId,browseMenuLevelList);
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu]. [category=%s] (bm)",browseMenuLevelList.size(),category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());
}

