
#include "BrowseWindowConfiguration.h"
#include "GUIViewState.h"
#include "Util.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"

#define MUSIC_TITLE 1211
#define VIDEO_TITLE 1212
#define PICTURES_TITLE 1213

#define IS_ALLOW_FILTER 900

std::map<int, CBrowseWindowFilter*> CBrowseWindowConfiguration::m_filters;

CBrowseWindowConfiguration::CBrowseWindowConfiguration() {
  m_loaded = false;
  m_iSelectedItem = -1;

  m_browseWindowAllowFilter = new CBrowseWindowAllowFilter(IS_ALLOW_FILTER,"Item Allow Filter");

	// Create master list of filters
	CreateFilters();
}

CBrowseWindowConfiguration::~CBrowseWindowConfiguration() 
{
  m_activeFilters.clear();
  m_currentFilters.clear();
  m_sortMethods.clear();
  m_viewTypes.clear();
}

void CBrowseWindowConfiguration::CreateFilters()
{
  if (m_filters.size() > 0)
  {
    return;
  }

	m_filters[FILTER_ALL] = new CBrowseWindowFilter(FILTER_ALL, "All");
	m_filters[FILTER_MOVIE] = new CBrowseWindowPropertyFilter(FILTER_MOVIE, "Movies", "ismovie");
	m_filters[FILTER_MOVIE_HD] = new CBrowseWindowPropertyFilter(FILTER_MOVIE_HD, "HD", "ishd");
	m_filters[FILTER_TV_SHOW] = new CBrowseWindowPropertyFilter(FILTER_TV_SHOW, "TV Shows", "istvshow");
	m_filters[FILTER_FOLDER_MEDIA_ITEM] = new CBrowseWindowMediaItemFilter(FILTER_FOLDER_MEDIA_ITEM, "Media Items");
	m_filters[FILTER_VIDEO] = new CBrowseWindowVideoFilter(FILTER_VIDEO, "Video Files");
	m_filters[FILTER_AUDIO] = new CBrowseWindowAudioFilter(FILTER_AUDIO, "Audio Files");
	m_filters[FILTER_PICTURE] = new CBrowseWindowPictureFilter(FILTER_PICTURE, "Picture Files");

	m_filters[FILTER_APPS_VIDEO] = new CBrowseWindowPropertyValueFilter(FILTER_APPS_VIDEO, "Apps Video", "app-media", "video");
	m_filters[FILTER_APPS_MUSIC] = new CBrowseWindowPropertyValueFilter(FILTER_APPS_MUSIC, "Apps Music", "app-media", "music");
	m_filters[FILTER_APPS_PICTURES] = new CBrowseWindowPropertyValueFilter(FILTER_APPS_PICTURES, "Apps Photos", "app-media", "pictures");
	m_filters[FILTER_APPS_GENERAL] = new CBrowseWindowPropertyValueFilter(FILTER_APPS_GENERAL, "Apps General", "app-media", "general");

	// Go over all Genres specified in the strings.xml file (range 51100 to 51124)
	for (int i = 51100; i < 51125; i++)
	{
  	m_filters[i - 51100 + 50] = new CBrowseWindowVideoGenreFilter(i - 51100 + 50, g_localizeStrings.Get(i), g_localizeStrings.Get(i));
	}
}

CBrowseWindowFilter* CBrowseWindowConfiguration::GetFilter(int iFilterId) const
{
	return m_filters[iFilterId];
}

void CBrowseWindowConfiguration::AddCustomFilter(CBrowseWindowFilter* pCustomFilter)
{
  if (pCustomFilter)
    m_customFilters.push_back(pCustomFilter);
}

void CBrowseWindowConfiguration::AddActiveFilter(int iFilterId)
{
	m_activeFilters.push_back(iFilterId);
}

void CBrowseWindowConfiguration::ClearActiveFilters()
{
	m_activeFilters.clear();
	for (size_t i = 0; i < m_customFilters.size(); i++)
	{
	  delete m_customFilters[i];
	}
	m_customFilters.clear();
}

void CBrowseWindowConfiguration::RemoveCustomFilter(int filterId)
{
  std::vector<CBrowseWindowFilter*>::iterator it = m_customFilters.begin();
  while (it != m_customFilters.end())
  {
    if ((*it)->m_iId == filterId)
    {
      delete *it;
      it = m_customFilters.erase(it);
    }
    else
    {
      ++it;
    }
  }
}

void CBrowseWindowConfiguration::ClearSortMethods()
{
  m_sortMethods.clear();
  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::ClearSortMethods - After clear of m_sortMethods. [m_sortMethodsSize=%lu] (vns)",m_sortMethods.size());
}

void CBrowseWindowConfiguration::ClearViewTypes()
{
  m_viewTypes.clear();
  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::ClearViewTypes - After clear of m_viewsTypesVec. [m_viewTypesMapSize=%lu] (vns)",m_viewTypes.size());
}

bool CBrowseWindowConfiguration::HasFilters()
{
  return ((m_activeFilters.size() > 0) || (m_customFilters.size() > 0));
}

bool CBrowseWindowConfiguration::ApplyFilter(const CFileItem* pItem) const
{
  if (!pItem)
  {
		CLog::Log(LOGERROR,"CBrowseWindowConfiguration::ApplyFilter, unable to apply filter, item is NULL (filter)");
		return false; // sanity
	}

  // check if item is allow
  if(m_browseWindowAllowFilter && !m_browseWindowAllowFilter->Apply(pItem))
	{
	  return false;
	}
	
	bool bResult = true;

	// Go over all active filters
	for (size_t i = 0; i < m_activeFilters.size() && bResult; i++)
	{
		CBrowseWindowFilter* pFilter = GetFilter(m_activeFilters[i]);
		//CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::ApplyFilter, apply active filter %s (filter)", pFilter->m_strName.c_str());

		bResult &= ApplySpecificFilter(pItem, pFilter);
	}

	for (size_t i = 0; i < m_customFilters.size() && bResult; i++)
	{
	  CBrowseWindowFilter* pFilter = m_customFilters[i];
	  //CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::ApplyFilter, apply custom filter %s (filter)", pFilter->m_strName.c_str());

	  bResult &= ApplySpecificFilter(pItem, pFilter);
	}

  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::ApplyFilter - For item [label=%s] going to return [%d] (iaf)(filter)",pItem->GetLabel().c_str(),bResult);

	return bResult;
}

bool CBrowseWindowConfiguration::ApplySpecificFilter(const CFileItem* pItem, CBrowseWindowFilter* pFilter) const
{
  if (pFilter)
  {
    return pFilter->Apply(pItem);
  }
  else
  {
    CLog::Log(LOGERROR,"CBrowseWindowConfiguration::ApplySpecificFilter, could not get filter, id = %s (filter)", pFilter->m_strName.c_str());
    return false;
  }
}

void CBrowseWindowConfiguration::UpdateFiltersByPath(const CStdString& strPath, CBrowseWindowConfiguration& configuration)
{
  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::UpdateFiltersByPath, path = %s (browse)", strPath.c_str());

  configuration.ClearActiveFilters();
  configuration.m_currentFilters.clear();

  if (strPath.Left(10) == "boxeedb://")
  {

    if (configuration.m_strType == BROWSE_MODE_VIDEO)
    {
      InitializeFilters("video", configuration);
    }
    else if (configuration.m_strType == BROWSE_MODE_PICTURES)
    {
      InitializeFilters("pictures", configuration);
    }
  }
  else if (strPath.Left(7)  == "feed://"    ||
           strPath.Left(10) == "friends://" ||
           strPath.Left(10) == "actions://" ||
           strPath.Left(9)  == "plugin://"  ||
           strPath.Left(10) == "sources://" ||
           strPath.Left(8)  == "boxee://" ||
           strPath.Left(6)  == "rss://" ||
           strPath.Left(7)  == "apps://" ||
           strPath.Left(9)  == "appbox://" ||
           strPath.Left(15) == "repositories://" ||
           strPath.Left(12) == "shortcuts://")
  {
    // no filters here
  }
  else 
  {
    if (configuration.m_strType == BROWSE_MODE_PICTURES)
    {
      InitializeFilters("pictures", configuration);
    }
    else 
    {
      InitializeFilters("other", configuration);
    }
  }

  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::UpdateFiltersByPath, added %d filters (browse)", (int)configuration.m_currentFilters.size());
}

void CBrowseWindowConfiguration::InitializeFilters(const CStdString& strType, CBrowseWindowConfiguration& configuration)
{
  if (strType == "video") 
  {
    // Add filters
    InitializeVideoFilters(configuration);
  }
  else if (strType == "pictures") 
  {
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_ALL));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_FOLDER_MEDIA_ITEM));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_VIDEO));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_AUDIO));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_PICTURE));
    configuration.AddActiveFilter(FILTER_PICTURE);
  }
  else if (strType == "other") 
  {
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_ALL));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_FOLDER_MEDIA_ITEM));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_VIDEO));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_AUDIO));
    configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_PICTURE));

    configuration.AddActiveFilter(FILTER_FOLDER_MEDIA_ITEM);
  }
  else if (strType == "none") 
  {
    configuration.ClearActiveFilters();
    configuration.m_currentFilters.clear();
  }
}

void CBrowseWindowConfiguration::InitializeVideoFilters(CBrowseWindowConfiguration& configuration)
{
  configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_ALL));
  configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_MOVIE));
  configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_MOVIE_HD));
  configuration.m_currentFilters.push_back(CBrowseWindowFilterContainer(FILTER_TV_SHOW));

  //Genre -> Action, Adventure, Animation, Biography, Comedy, Crime,
    //Documentary, Drama, Family, Fantasy, Film-Noir, Game Show, History,
    //Horror, Music, Musical, Mystery, News, Reality-TV, Romance, Sci-Fi,
    //Short, Sport, Talk-Show, Thriller, War, Western

  CBrowseWindowFilterContainer genreFilter(FILTER_MOVIE_GENRE);
    genreFilter.m_strName = "Genre";
    
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_ACTION);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_ADVENTURE);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_ANIMATION);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_BIOGRAPHY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_COMEDY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_CRIME);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_DOCUMENTARY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_DRAMA);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_FAMILY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_FANTASY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_FILM_NOIR);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_GAME_SHOW);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_HISTORY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_HORROR);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_MUSIC);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_MUSICAL);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_MYSTERY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_NEWS);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_REALITY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_ROMANCE);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_SCI_FI);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_REALITY);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_SHORT);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_SPORT);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_TALK_SHOW);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_THRILLER);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_WAR);
    genreFilter.m_filters.push_back(FILTER_MOVIE_GENRE_WESTERN);

    configuration.m_currentFilters.push_back(genreFilter);
}

CBrowseWindowConfiguration CBrowseWindowConfiguration::GetConfigurationByPath(const CStdString& _strPath)
{
  // Very straightforward implementation at this point
  CStdString strPath = _strPath;
  strPath.ToLower();
  
  if (CUtil::HasSlashAtEnd(strPath))
    CUtil::RemoveSlashAtEnd(strPath);

  CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::GetConfigurationByPath, path = %s (browse)", strPath.c_str());

  CBrowseWindowConfiguration configuration;

  if (strPath == "boxeedb://movies")
  {
    configuration.m_strTitle = g_localizeStrings.Get(20342);
    configuration.m_strLabel = g_localizeStrings.Get(20342);
    configuration.m_strTitleIcon = "icon_video.png";
    configuration.m_strType = BROWSE_MODE_VIDEO;
  }
  else if (strPath == "boxeedb://tvshows")
  {
    configuration.m_strTitle = g_localizeStrings.Get(20343);
    configuration.m_strLabel = g_localizeStrings.Get(20343);
    configuration.m_strTitleIcon = "icon_video.png";
    configuration.m_strType = BROWSE_MODE_VIDEO;
  }
  else if (strPath == "boxeedb://music")
  {
    configuration.m_strTitle = g_localizeStrings.Get(133);
    configuration.m_strLabel = g_localizeStrings.Get(133);
    configuration.m_strTitleIcon = "icon_music.png";
    configuration.m_strType = BROWSE_MODE_MUSIC;
  }
  else if (strPath == "boxeedb://albums")
  {
    configuration.m_strTitle = g_localizeStrings.Get(132);
    configuration.m_strLabel = g_localizeStrings.Get(132);
    configuration.m_strTitleIcon = "icon_music.png";
    configuration.m_strType = BROWSE_MODE_MUSIC;
  }
  else if (strPath == "boxeedb://pictures")
  {
    configuration.m_strTitle = g_localizeStrings.Get(53519);
    configuration.m_strLabel = g_localizeStrings.Get(53519);
    configuration.m_strTitleIcon = "icon_pictures.png";
    configuration.m_strType = BROWSE_MODE_PICTURES;
  }
  else if (strPath == "sources://all")
  {
    configuration.m_strTitle = g_localizeStrings.Get(52044);
    configuration.m_strLabel = g_localizeStrings.Get(52044);
    configuration.m_strTitleIcon = "icon_sources.png";
    configuration.m_strType = BROWSE_MODE_FOLDER;
  }
  else if (strPath == "sources://music")
  {
    configuration.m_strTitle = g_localizeStrings.Get(52040);
    configuration.m_strLabel = g_localizeStrings.Get(52040);
    configuration.m_strTitleIcon = "icon_sources.png";
    configuration.m_strType = BROWSE_MODE_FOLDER;
  }
  else if (strPath == "sources://video")
  {
    configuration.m_strTitle = g_localizeStrings.Get(52041);
    configuration.m_strLabel = g_localizeStrings.Get(52041);
    configuration.m_strTitleIcon = "icon_sources.png";
    configuration.m_strType = BROWSE_MODE_FOLDER;
  }
  else if (strPath == "sources://pictures")
  {
    configuration.m_strTitle = g_localizeStrings.Get(52042);
    configuration.m_strLabel = g_localizeStrings.Get(52042);
    configuration.m_strTitleIcon = "icon_sources.png";
    configuration.m_strType = BROWSE_MODE_FOLDER;
  }
  else if (strPath == "apps://all")
  {
    configuration.m_strTitle = g_localizeStrings.Get(51415);
    configuration.m_strLabel = g_localizeStrings.Get(51415);
    configuration.m_strTitleIcon = "icon_applications.png";
    configuration.m_strType = BROWSE_MODE_VIDEO;
  }
  else if (strPath == "apps://video")
  {
    configuration.m_strTitle = g_localizeStrings.Get(51411);
    configuration.m_strLabel = g_localizeStrings.Get(51411);
    configuration.m_strTitleIcon = "icon_video.png";
    configuration.m_strType = BROWSE_MODE_VIDEO;
  }
  else if (strPath == "apps://music")
  {
    configuration.m_strTitle = g_localizeStrings.Get(51409);
    configuration.m_strLabel = g_localizeStrings.Get(51409);
    configuration.m_strTitleIcon = "icon_music.png";
    configuration.m_strType = BROWSE_MODE_MUSIC;
  }
  else if (strPath == "apps://pictures")
  {
    configuration.m_strTitle = g_localizeStrings.Get(51413);
    configuration.m_strLabel = g_localizeStrings.Get(51413);
    configuration.m_strTitleIcon = "icon_pictures.png";
    configuration.m_strType = BROWSE_MODE_PICTURES;
  }
  else if (strPath == "feed://share")
  {
    configuration.m_strTitle = g_localizeStrings.Get(53518);
    configuration.m_strLabel = g_localizeStrings.Get(53518);
    configuration.m_strTitleIcon = "icon_pictures.png";
    configuration.m_strType = BROWSE_MODE_OTHER;
  }
  else if (strPath == "feed://queue")
  {
    configuration.m_strTitle = g_localizeStrings.Get(53517);
    configuration.m_strLabel = g_localizeStrings.Get(53517);
    configuration.m_strType = BROWSE_MODE_OTHER;
  }
  else if (strPath == "history://all")
  {
    configuration.m_strTitle = g_localizeStrings.Get(53520);
    configuration.m_strLabel = g_localizeStrings.Get(53520);
    configuration.m_strType = BROWSE_MODE_OTHER;
  }
  else
  {
    return GetConfigurationByType("other");
  }

  return configuration;
}

CBrowseWindowConfiguration CBrowseWindowConfiguration::GetConfigurationByType(const CStdString& strType)
{
	CBrowseWindowConfiguration configuration;

  configuration.m_strType = strType;

	CLog::Log(LOGDEBUG,"CBrowseWindowConfiguration::GetConfigurationByType, type = %s (browse)", strType.c_str());

	if (strType == "video") {
		configuration.m_strTitle = g_localizeStrings.Get(VIDEO_TITLE);// 1212
		configuration.m_strLabel = g_localizeStrings.Get(VIDEO_TITLE);// 1212
		configuration.m_strTitleIcon = "icon_video.png";
	}
	else if ((strType == "audio") || (strType == "music")) {
		configuration.m_strTitle = g_localizeStrings.Get(MUSIC_TITLE);// 1211
		configuration.m_strLabel = g_localizeStrings.Get(MUSIC_TITLE);// 1212
		configuration.m_strTitleIcon = "icon_music.png";
	}
	else if (strType == "pictures") {
		configuration.m_strTitle = g_localizeStrings.Get(53519);
		configuration.m_strLabel = g_localizeStrings.Get(53519);
		configuration.m_strTitleIcon = "icon_pictures.png";
	}
	else if (strType == "recommendations") {
	    configuration.m_strTitle = g_localizeStrings.Get(51400);
		configuration.m_strLabel = g_localizeStrings.Get(51400);
		configuration.m_strTitleIcon = "icon_whatsnew.png";
	}
	else if (strType == "activity") {
	    configuration.m_strTitle = g_localizeStrings.Get(51401);
		configuration.m_strLabel = g_localizeStrings.Get(51401);
		configuration.m_strTitleIcon = "icon_whatsnew.png";
	}
	else if (strType == "added") {
	    configuration.m_strTitle = g_localizeStrings.Get(51402);
	    configuration.m_strLabel = g_localizeStrings.Get(51402);
	    configuration.m_strTitleIcon = "icon_whatsnew.png";
	}
	else if (strType == "used") {
	    configuration.m_strTitle = g_localizeStrings.Get(51403);
	    configuration.m_strLabel = g_localizeStrings.Get(51403);
		configuration.m_strTitleIcon = "icon_whatsnew.png";

    }
	else if (strType == "friends") {
	    configuration.m_strTitleIcon = "icon_profile.png";
		configuration.m_strLabel = g_localizeStrings.Get(51404);
    }
	else if (strType == "unresolved") {
	    configuration.m_strTitleIcon = "icon_whatsnew.png";
	    configuration.m_strLabel = g_localizeStrings.Get(51414);
	    configuration.m_strTitle = g_localizeStrings.Get(51414);
	  }
	else if (strType == "other") {
		configuration.m_strTitleIcon = "icon_whatsnew.png";
	}
	else if (strType == "dvd") {
	    configuration.m_strTitleIcon = "main_nav_disc_button_roll_over.png";
    }
	else {
      // default configuration
	  CLog::Log(LOGERROR,"CBrowseWindowConfiguration::GetConfigurationByType, NEWUI, unrecognized configuration type = %s", strType.c_str());
	}
	
	InitializeFilters(strType, configuration);

	return configuration;
}

