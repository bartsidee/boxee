
#include "GUIWindowBoxeeBrowseMovies.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIDialogBoxeeSearch.h"
#include "boxee.h"
#include "FileSystem/File.h"
#include "Picture.h"
#include "bxcscmanager.h"
#include "BoxeeBrowseMenuManager.h"

using namespace std;
using namespace BOXEE;

#define STORE_FLAG "movies-store"
#define STORE_LABEL_FLAG "movies-store-label"

#define ITEM_COUNT_LABEL "item-summary-count"

#define ITEMS_FILTERED_OUT_PROPERTY  "FilteredOut"

#define HIDE_SORT_DROPDOWN_FLAG "hide-sort-dropdown"
#define HIDE_FILTER_DROPDOWN_FLAG "hide-filter-dropdown"

CRemoteMoviesSource::CRemoteMoviesSource(int iWindowID) : CBrowseWindowSource("remotemoviesource", "boxee://movies/movies/",iWindowID)
{
  SetPageSize(CURRENT_PAGE_SIZE);
}

CRemoteMoviesSource::~CRemoteMoviesSource()
{

}

void CRemoteMoviesSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
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

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

CMovieStoreSource::CMovieStoreSource(int iWindowID) : CBrowseWindowSource("moviestoresource", "boxee://movies/movies/",iWindowID)
{
  SetPageSize(CURRENT_PAGE_SIZE);
}

CMovieStoreSource::~CMovieStoreSource()
{

}

void CMovieStoreSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
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


CLocalMoviesSource::CLocalMoviesSource(int iWindowID) : CBrowseWindowSource("localmoviesource", "boxeedb://movies/",iWindowID)
{
}

CLocalMoviesSource::~CLocalMoviesSource()
{

}

void CLocalMoviesSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_mapFilters["genre"].empty())
  {
    mapOptions["genre"] = m_mapFilters["genre"];
  }

  if (m_mapFilters.find("unwatched") != m_mapFilters.end() && m_mapFilters["unwatched"].Compare("true") == 0)
    mapOptions["unwatched"] = "true";

  if (m_mapFilters.find("source") != m_mapFilters.end())
    mapOptions["source"] = m_mapFilters["source"];

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

CTrailersSource::CTrailersSource(int iWindowID) : CBrowseWindowSource("trailerssource", "boxee://trailers/",iWindowID)
{
  SetPageSize(CURRENT_PAGE_SIZE);
}

CTrailersSource::~CTrailersSource()
{

}

void CTrailersSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_sourceSort.m_id.IsEmpty())
  {
    CStdString strSortMethod = m_sourceSort.m_id;
    CUtil::URLEncode(strSortMethod);
    CUtil::URLEncode(strSortMethod); //the CURL GetOptionsAsMap is doing url decode, it may leave us without encoding
    mapOptions["sort"] = strSortMethod;
  }

  if (!m_mapFilters["section"].empty())
  {
    CStdString strSection = m_mapFilters["section"];
    CUtil::URLEncode(strSection);
    mapOptions["section"] = strSection;
  }

  if (!m_mapFilters["genre"].empty())
  {
    CStdString strGenre = m_mapFilters["genre"];
    CUtil::URLEncode(strGenre);
    mapOptions["genre"] = strGenre;
  }

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

/////////////////////////////////////////////////////////////

CMoviesWindowState::CMoviesWindowState(CGUIWindowBoxeeBrowse* pWindow) : CTvShowsWindowState(pWindow)
{
  m_sourceController.RemoveAllSources();
  m_sourceController.AddSource(new CLocalMoviesSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CRemoteMoviesSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CMovieStoreSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CTrailersSource(m_pWindow->GetID()));
  
  m_strSearchType = "movies";
}

void CMoviesWindowState::SetTrailerSection(const TrailerSectionItem& section)
{
  m_selectedSection = section;

  // If Library is selected, genre filter should be disabled
  if (m_selectedSection.m_Id != DEFAULT_SECTION)
  {
    m_sourceController.SetFilter("section", m_selectedSection.m_Id);
  }
  else
  {
    m_sourceController.ClearFilter("section");
  }

  if (m_selectedSection.m_Id == DEFAULT_SECTION || m_selectedSection.m_Id == GENRE_SECTION)
  {
    //set the paging
    SetPageSize(CURRENT_PAGE_SIZE);
  }
  else
  {
    //hide the sorts if we're not in genre or library
    m_pWindow->SetProperty(HIDE_SORT_DROPDOWN_FLAG,true);
  }
}

void CMoviesWindowState::SetCategory(const CStdString& strCategory)
{
  m_sourceController.ActivateAllSources(false,true);
  m_pWindow->SetProperty("is-category-local", false);
  m_pWindow->m_strItemDescription = g_localizeStrings.Get(90041);
  m_vecSortMethods.clear();
  SetPageSize(DISABLE_PAGING);
  m_pWindow->SetProperty(HIDE_SORT_DROPDOWN_FLAG,false);
  m_pWindow->SetProperty(HIDE_FILTER_DROPDOWN_FLAG,false);

  if (strCategory.CompareNoCase("local")==0) // local
  {
    bool IgnorePrefix = g_guiSettings.GetBool("sort.showstarter");
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, IgnorePrefix?SORT_METHOD_LABEL_IGNORE_THE:SORT_METHOD_LABEL, SORT_ORDER_ASC , g_localizeStrings.Get(53535), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ZTOA, IgnorePrefix?SORT_METHOD_LABEL_IGNORE_THE:SORT_METHOD_LABEL, SORT_ORDER_DESC , g_localizeStrings.Get(53536), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_DATE, SORT_METHOD_DATE_ADDED, SORT_ORDER_DESC, g_localizeStrings.Get(51402), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_RELEASE_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53537), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE_REVERSE, SORT_METHOD_RELEASE_DATE, SORT_ORDER_ASC, g_localizeStrings.Get(53538), ""));

    m_sourceController.ActivateSource("localmoviesource",true,true);
    m_pWindow->SetProperty("is-category-local", true);
  }
  else if (strCategory.CompareNoCase("trailers") == 0)
  {
    //don't sort the content we get from the server
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_POPULARITY, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53504), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53537), ""));

    m_sourceController.ActivateSource("trailerssource",true,true);
    m_pWindow->m_strItemDescription = g_localizeStrings.Get(90045);
    m_pWindow->SetProperty(HIDE_FILTER_DROPDOWN_FLAG,true);
  }
  else
  {
    //don't sort the content we get from the server
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_POPULARITY, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53504), ""));
    m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_RELEASE, SORT_METHOD_NONE, SORT_ORDER_NONE, g_localizeStrings.Get(53537), ""));

    // Activate relevant sources according to selected category
    if (strCategory.CompareNoCase("all")==0) //all
    {
      m_sourceController.ActivateSource("remotemoviesource",true,true);
    }
    else if (strCategory.CompareNoCase("store")==0) // store
    {
      m_sourceController.ActivateSource("moviestoresource",true,true);
    }
    SetPageSize(CURRENT_PAGE_SIZE);
  }

  CBrowseWindowState::SetCategory(strCategory);

  m_pWindow->SetProperty("is-category-default", !m_pWindow->GetPropertyBOOL("is-category-local"));
}

void CMoviesWindowState::OnBind(CFileItemList& items)
{
  CTvShowsWindowState::OnBind(items);
  m_pWindow->SetProperty(ITEMS_FILTERED_OUT_PROPERTY,items.GetPropertyBOOL(ITEMS_FILTERED_OUT_PROPERTY));
}

void CMoviesWindowState::SetStore(const CStdString& strStoreId)
{
  m_strStoreId = strStoreId;

  if (strStoreId.CompareNoCase("all") != 0)
  {
    std::vector<BXSourcesItem> vecSources;

    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieSources(vecSources);

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

void CMoviesWindowState::SetSource(const CStdString& strSourcePath)
{
  if (!strSourcePath.IsEmpty())
  {
    m_sourceController.SetFilter("source",strSourcePath);
  }
  else
  {
    m_sourceController.ClearFilter("source");
  }
}

CStdString CMoviesWindowState::GetItemSummary()
{
  std::map<CStdString , CStdString> mapTitleItemValue;
  CStdString strSummary = "";

  if (m_bUnwatched)
  {
    mapTitleItemValue["filter"] = g_localizeStrings.Get(53714);
  }

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

  if (GetCategory().CompareNoCase("local")==0)
  {
    mapTitleItemValue["source"] = g_localizeStrings.Get(53753);
  }

  bool bSetSort = true;
  if (GetCategory().CompareNoCase("trailers")==0)
  {
    mapTitleItemValue["media"] = g_localizeStrings.Get(1053); //should be "Trailers"
    if (m_selectedSection.m_Id != DEFAULT_SECTION && m_selectedSection.m_Id != GENRE_SECTION)
    {
      bSetSort = false;
      mapTitleItemValue["source"] = m_selectedSection.m_Text;

    }
  }
  else
  {
    mapTitleItemValue["media"] = g_localizeStrings.Get(20342); //should have "Movies"
  }

  if (!m_sort.m_sortName.empty())
  {
    if (bSetSort)
    {
      if (m_sort.m_id == VIEW_SORT_METHOD_RELEASE)
      {
        mapTitleItemValue["sort"] = g_localizeStrings.Get(53534); //prefix + sort name
      }
      else if (m_sort.m_id == VIEW_SORT_METHOD_RELEASE_REVERSE)
      {
        mapTitleItemValue["sort"] = g_localizeStrings.Get(53533); //prefix + sort name
      }
      else if ( m_sort.m_id != VIEW_SORT_METHOD_ATOZ && m_sort.m_id != VIEW_SORT_METHOD_ZTOA )
      {//show the sort only if its not A TO Z
        mapTitleItemValue["sort"] = m_sort.m_sortName; //prefix + sort name
      }
    }
    //if there is a prefix in our language and we have a sort in the title, append the relevant prefix
    if (!g_localizeStrings.Get(90006).IsEmpty() && mapTitleItemValue.find("sort") != mapTitleItemValue.end())
      mapTitleItemValue["sortprefix"] = g_localizeStrings.Get(90006);

  }

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90001), mapTitleItemValue,strSummary))
  {
    strSummary = g_localizeStrings.Get(20342);
    CLog::Log(LOGERROR,"CMoviesWindowState::GetItemSummary, Error in Strings.xml for the current language [id=90001], the template is bad. (browse)");
  }

  return strSummary;
}

// /////////////////////////////////////////////////////////

CGUIWindowBoxeeBrowseMovies::CGUIWindowBoxeeBrowseMovies() : CGUIWindowBoxeeBrowseTvShows(WINDOW_BOXEE_BROWSE_MOVIES,"boxee_browse_movies.xml")
{
  m_strItemDescription = g_localizeStrings.Get(90041);

  SetWindowState(new CMoviesWindowState(this));
}

CGUIWindowBoxeeBrowseMovies::~CGUIWindowBoxeeBrowseMovies()
{

}

/*
void CGUIWindowBoxeeBrowseMovies::FillStoreList(CFileItemList& store)
{
  std::vector<BXSourcesItem> sources;
  std::vector<BXSourcesItem>::iterator it;

  Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieSources(sources);

  CFileItemPtr allItem (new CFileItem(g_localizeStrings.Get(54911)));
  allItem->SetProperty("type", "movies_store");
  allItem->SetProperty("value", "all");
  store.Add(allItem);

  for (it = sources.begin() ; it != sources.end() ; it++)
  {
    if (!it->GetSourceGeo().empty() && !CUtil::IsCountryAllowed(it->GetSourceGeo(), true))
    {
      continue;
    }

    CFileItemPtr storeItem (new CFileItem( it->GetSourceName() ) );
    storeItem->SetProperty("type", it->GetSourceType());
    storeItem->SetProperty("value", it->GetSourceId());
    storeItem->SetThumbnailImage(it->GetSourceThumb());

    store.Add(storeItem);
  }

}
*/

void CGUIWindowBoxeeBrowseMovies::OnInitWindow()
{
  CGUIWindowBoxeeBrowseTvShows::OnInitWindow();

  SetProperty(ITEMS_FILTERED_OUT_PROPERTY,false);
}

void CGUIWindowBoxeeBrowseMovies::ConfigureState(const CStdString& param)
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
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieGenres(vecGenres);

      for (std::vector<GenreItem>::iterator it = vecGenres.begin() ; it != vecGenres.end() ; ++it)
      {
        if (strGenreId == it->m_genreId)
        {
          foundGenre = (*it);
          break;
        }
      }
    }

    ((CMoviesWindowState*)m_windowState)->SetGenre(foundGenre);
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
      ((CMoviesWindowState*)m_windowState)->SetCategory("store");
      ((CMoviesWindowState*)m_windowState)->SetStore(optionsMap["provider"]);
    }
    else
    {
      ((CMoviesWindowState*)m_windowState)->SetStore("all");
    }

    CStdString sourcePath;

    if (optionsMap.find("source") != optionsMap.end())
    {
      sourcePath = optionsMap["source"];
    }

    ((CMoviesWindowState*)m_windowState)->SetSource(sourcePath);

    TrailerSectionItem foundTrailerSection;

    if (optionsMap.find("section") != optionsMap.end())
    {
      CStdString strTrailerSectioneId = optionsMap["section"];

      std::vector<TrailerSectionItem> vecTrailerSections;
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieTrailerSections(vecTrailerSections);

      for (std::vector<TrailerSectionItem>::iterator it = vecTrailerSections.begin() ; it != vecTrailerSections.end() ; ++it)
      {
        if (strTrailerSectioneId.CompareNoCase(it->m_Id.c_str()) == 0)
        {
          foundTrailerSection = (*it);

          ((CMoviesWindowState*)m_windowState)->SetTrailerSection(foundTrailerSection);
          break;
        }
      }
    }
  }
}

bool CGUIWindowBoxeeBrowseMovies::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_UPDATE:
  {
    if (message.GetSenderId() == WINDOW_INVALID && message.GetControlId() == GUI_MSG_MEDIA_CHANNELS_UPDATE)
    {
      //CBoxeeBrowseMenuManager::GetInstance().ClearMenu("mn_library_movies_providers");
      return true;
    }
  }
  break;
  case GUI_MSG_UPDATE_ITEM:
  {
    CStdString param = message.GetStringParam();

    if (param == "MarkAsWatched")
    {
      if (message.GetSenderId() == WINDOW_BOXEE_BROWSE_MOVIES)
      {
        CStdString itemId = message.GetStringParam(1);

        if (itemId.IsEmpty())
        {
          CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseMovies::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since itemId is EMPTY. [itemId=%s] (browse)",itemId.c_str());
          return false;
        }

        CStdString watchedType = message.GetStringParam(2);

        if (watchedType.IsEmpty())
        {
          CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseMovies::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since watchedType is EMPTY. [watchedType=%s] (browse)",watchedType.c_str());
          return false;
        }

        CFileItemPtr pItem = m_viewItemsIndex[itemId];
        if (!pItem.get())
        {
          CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseMovies::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since there is NO item for [itemId=%s] (browse)",itemId.c_str());
          return false;
        }

        pItem->SetProperty("watched", (watchedType == "1") ? true : false);

        return true;
      }
    }
  }
  break;
  case GUI_MSG_CLICKED:
  {
    int control = message.GetControlId();
    int sender = message.GetSenderId();
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::OnMessage - GUI_MSG_CLICKED - [control=%d][sender=%d][BrowseMenuLevel1=%d][BrowseMenuLevel2=%d][BrowseMenuLevel3=%d] (bm)",control,sender,g_settings.GetSkinBool(g_settings.TranslateSkinBool("browsemenulevel1")),g_settings.GetSkinBool(g_settings.TranslateSkinBool("browsemenulevel2")),g_settings.GetSkinBool(g_settings.TranslateSkinBool("browsemenulevel3")));
  }
  break;
  case GUI_MSG_LABEL_BIND:
  {
    int saveFocusedControl = GetFocusedControlID();
    bool retVal = CGUIWindowBoxeeBrowseTvShows::OnMessage(message);

    if (GetPropertyBOOL(ITEMS_FILTERED_OUT_PROPERTY))
    {
      SET_CONTROL_FOCUS(saveFocusedControl,0);
    }

    return retVal;
  }
  break;
  }

  return CGUIWindowBoxeeBrowseTvShows::OnMessage(message);
}

void CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  CStdString startMenuId = "";

  if (category == "all")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - handle [category=%s=all] -> set library_movie menu (bm)",category.c_str());
    startMenuId = "mn_library_movies";
  }
  else if (category == "local")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - handle [category=%s=local] -> set local_movie menu (bm)",category.c_str());
    startMenuId = "mn_local_movies";
  }
  else if (category == "store")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - handle [category=%s=local] -> set local_movie menu (bm)",category.c_str());
    startMenuId = "mn_library_movies_providers";
  }
  else if (category == "trailers")
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - handle [category=%s=trailers] -> set library_movies_trailers menu (bm)",category.c_str());
    startMenuId = "mn_library_movies_trailers";
  }
  else
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - FAILED to set menu since [category=%s] is unknown (bm)",category.c_str());
  }

  if (!startMenuId.IsEmpty())
  {
    CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure(startMenuId,browseMenuLevelList);
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu]. [category=%s] (bm)",browseMenuLevelList.size(),category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseMovies::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());
}
