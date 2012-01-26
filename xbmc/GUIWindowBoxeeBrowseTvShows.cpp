#include "GUIWindowBoxeeBrowseTvShows.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIWindowBoxeeBrowseTvEpisodes.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "boxee.h"
#include "bxvideodatabase.h"
#include "lib/libBoxee/boxee.h"
#include "SpecialProtocol.h"
#include "BoxeeDatabaseDirectory.h"
#include "GUIDialogBoxeeNetworkNotification.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_SORT    110
#define BUTTON_MY      120
#define BUTTON_ALL     130
#define BUTTON_SHOW_LIBRARY 131
#define BUTTON_GENRES  150
#define BUTTON_FREE    151
#define BUTTON_UNWATCHED 152
#define BUTTON_SOURCES  153
#define BUTTON_SEARCH  160

#define SORT_LABEL_FLAG "sort-label"
#define GENRE_LABEL_FLAG "genre-label"
#define GENRE_SET_FLAG "genre-set"
#define SOURCE_LABEL_FLAG "source-label"
#define SOURCE_SET_FLAG "source-set"
#define SHOW_SOURCE_FLAG "show_source"
#define FREE_LABEL_FLAG "free-label"
#define FREE_SET_FLAG "free-set"
#define SEARCH_LABEL_FLAG "search-label"
#define UNWATCHED_SET_FLAG "unwatched-set"
#define UNWATCHED_LABEL_FLAG "unwatched-label"
#define SCANNING_FLAG "scanning-label"

#define CUSTOM_GENRE_FILTER 600
#define CUSTOM_UNWATCHED_FILTER 601
#define CUSTOM_SOURCE_FILTER 602

#define SCANNING_LABEL          501
#define RESOLVED_VIDEO_LABEL    502

// IMPLEMENTATION OF THE CBrowseWindowState ///////////////////////////

CTvShowsWindowState::CTvShowsWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
  SetSearchType("tvshows");

  m_bUnwatched = false;
  m_bFree = false;
  m_bInShow = false;
}

void CTvShowsWindowState::RestoreWindowState()
{
  CBrowseWindowState::RestoreWindowState();

  // we clear the genre when going back from search
  if (InSearchMode())
    ResetFilters();
}

void CTvShowsWindowState::ResetFilters()
{
  SetGenre(g_localizeStrings.Get(53511));
  SetSource(g_localizeStrings.Get(53512), g_localizeStrings.Get(53512));
  SetFree(false);
  SetUnwatchedFilter(false);
}

void CTvShowsWindowState::Reset()
{
  CBrowseWindowState::Reset();

  ResetFilters();
}

CStdString CTvShowsWindowState::CreatePath()
{
  CStdString strPath = "boxee://tvshows/shows";
  strPath = AddGuiStateParameters(strPath);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::CreatePath, created path = %s (browse)", strPath.c_str());
  return strPath;

}

CStdString CTvShowsWindowState::AddGuiStateParameters(const CStdString& _strPath)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, path = %s (browse)", _strPath.c_str());
  CStdString strPath = _strPath;

  std::map<CStdString, CStdString> mapOptions;

  if (InSearchMode())
  {
    if (!m_strSearchString.IsEmpty())
    {
      mapOptions["remote"] = "true";
      mapOptions["local"] = "true";
      mapOptions["search"] = GetSearchString();
    }
  }
  else
  {

    // Add sort options
    mapOptions["sort"] = m_sort.m_id;

    // Add genre parameter only if specific genre is selected
    if (m_strGenre.CompareNoCase(g_localizeStrings.Get(53511)) != 0)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, add genre: [%s] (browse)", m_strGenre.c_str());
      mapOptions["genre"] = m_strGenre;
    }

    if (m_bFree)
    {
      mapOptions["free"] = "true";

      std::vector<std::string> servicesIdsVec;
      bool succeeded = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetServicesIds(servicesIdsVec);

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, Got [%d] serviceId's. [succeeded=%d] (browse)",(int)servicesIdsVec.size(),succeeded);

      if (succeeded)
      {
        CStdString servicesIds = "";

        for (size_t i=0; i<servicesIdsVec.size(); i++)
        {
          if (i>0)
          {
            servicesIds += ",";
          }

          servicesIds += servicesIdsVec[i];
        }

        if (!servicesIds.IsEmpty())
        {
          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, Going to add [services=%s] (browse)",servicesIds.c_str());
          mapOptions["services"] = servicesIds;
        }
      }
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, FAILED to get services id's (browse)");
      }
    }
  }

  strPath += BoxeeUtils::BuildParameterString(mapOptions);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::AddGuiStateParameters, return path = %s (browse)", strPath.c_str());
  return strPath;
}

void CTvShowsWindowState::SetGenre(const CStdString& strGenre)
{
  m_strGenre = strGenre;
  //m_configuration.ClearActiveFilters();
  m_configuration.RemoveCustomFilter(CUSTOM_GENRE_FILTER);
  
  if (strGenre.CompareNoCase(g_localizeStrings.Get(53511)) != 0)
  {
    m_pWindow->SetProperty(GENRE_LABEL_FLAG, strGenre);
    m_pWindow->SetProperty(GENRE_SET_FLAG, true);
    m_configuration.AddCustomFilter(new CBrowseWindowTvShowGenreFilter(CUSTOM_GENRE_FILTER, "Tv Show Genre Filter", strGenre));
  }
  else
  {
    m_pWindow->SetProperty(GENRE_LABEL_FLAG, "");
    m_pWindow->SetProperty(GENRE_SET_FLAG, false);
  }
}

void CTvShowsWindowState::SetSource(const CStdString& strSourceId, const CStdString& strSourceName)
{
  m_strSource = strSourceId;

  //m_configuration.RemoveCustomFilter(CUSTOM_SOURCE_FILTER);

  if (strSourceId.CompareNoCase(g_localizeStrings.Get(53512)) != 0)
  {
    m_pWindow->SetProperty(SOURCE_LABEL_FLAG, strSourceName);
    m_pWindow->SetProperty(SOURCE_SET_FLAG, true);
    //m_configuration.AddCustomFilter(new CBrowseWindowTvShowSourceFilter(CUSTOM_SOURCE_FILTER, "Tv Show Source Filter", strSourceId));
  }
  else
  {
    m_pWindow->SetProperty(SOURCE_LABEL_FLAG, "");
    m_pWindow->SetProperty(SOURCE_SET_FLAG, false);
  }
}

void CTvShowsWindowState::SortItems(CFileItemList &items)
{
  if (InSearchMode())
  {
    items.Sort(SORT_METHOD_LABEL_EXACT, SORT_ORDER_ASC);
  }
  else
  {
    CBrowseWindowState::SortItems(items);
  }
}

bool CTvShowsWindowState::OnBack()
{
  return OnSearchEnd();
}

void CTvShowsWindowState::OnFree()
{
  SetFree(!m_bFree);
}

void CTvShowsWindowState::SetFree(bool bFree)
{
  m_bFree = bFree;
  m_pWindow->SetProperty(FREE_SET_FLAG, m_bFree);
  if (m_bFree)
  {
    m_pWindow->SetProperty(FREE_LABEL_FLAG, g_localizeStrings.Get(53525));
  }
  else
  {
    m_pWindow->SetProperty(FREE_LABEL_FLAG, "");
  }
}

void CTvShowsWindowState::OnUnwatched()
{
  m_bUnwatched = !m_bUnwatched;
  SetUnwatchedFilter(m_bUnwatched);
}

void CTvShowsWindowState::SetUnwatchedFilter(bool bOn)
{
  //m_configuration.ClearActiveFilters();
  m_configuration.RemoveCustomFilter(CUSTOM_UNWATCHED_FILTER);

  if (bOn)
  {
    m_pWindow->SetProperty(UNWATCHED_SET_FLAG, true);
    m_pWindow->SetProperty(UNWATCHED_LABEL_FLAG, g_localizeStrings.Get(53526));
    m_configuration.AddCustomFilter(new CBrowseWindowTvShowUnwatchedFilter(CUSTOM_UNWATCHED_FILTER, "Tv Show Genre Filter", true));
  }
  else
  {
    m_pWindow->SetProperty(UNWATCHED_SET_FLAG, false);
    m_pWindow->SetProperty(UNWATCHED_LABEL_FLAG, "");
  }
}

CMyShowsWindowState::CMyShowsWindowState(CGUIWindow* pWindow) : CTvShowsWindowState(pWindow)
{
  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort("title", SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), ""));
  m_vecSortMethods.push_back(CBoxeeSort("release", SORT_METHOD_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53506), ""));

  SetSort(m_vecSortMethods[0]);
}

void CMyShowsWindowState::Reset()
{
  CTvShowsWindowState::Reset();

  m_pWindow->SetProperty(SHOW_SOURCE_FLAG, false);
  m_pWindow->SetProperty("all-set", false);
  m_pWindow->SetProperty("my-set", true);
}

CStdString CMyShowsWindowState::AddGuiStateParameters(const CStdString& _strPath)
{
  CLog::Log(LOGDEBUG,"CMyShowsWindowState::AddGuiStateParameters, input path = %s (browse)", _strPath.c_str());
  CStdString strPath = CTvShowsWindowState::AddGuiStateParameters(_strPath);

  if (InSearchMode())
  {
    return strPath;
  }

  std::map<CStdString, CStdString> mapOptions;

  mapOptions["local"] = "true";
  mapOptions["subscribed"] = "true";

  strPath = BoxeeUtils::AppendParameters(strPath, mapOptions);

  CLog::Log(LOGDEBUG,"CMyShowsWindowState::AddGuiStateParameters, return path = %s (browse)", strPath.c_str());
  return strPath;
}

CAllShowsWindowState::CAllShowsWindowState(CGUIWindow* pWindow) : CTvShowsWindowState(pWindow)
{
  m_vecSortMethods.push_back(CBoxeeSort("popular", SORT_METHOD_DEFAULT, SORT_ORDER_ASC, g_localizeStrings.Get(53504), ""));
  m_vecSortMethods.push_back(CBoxeeSort("release", SORT_METHOD_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53506), ""));

  SetSort(m_vecSortMethods[0]);
}

void CAllShowsWindowState::Reset()
{
  CTvShowsWindowState::Reset();

  m_pWindow->SetProperty(SHOW_SOURCE_FLAG, true);
  m_pWindow->SetProperty("all-set", true);
  m_pWindow->SetProperty("my-set", false);
}

CStdString CAllShowsWindowState::AddGuiStateParameters(const CStdString& _strPath)
{
  CLog::Log(LOGDEBUG,"CAllShowsWindowState::AddGuiStateParameters, return path = %s (browse)", _strPath.c_str());
  CStdString strPath = CTvShowsWindowState::AddGuiStateParameters(_strPath);

  if (InSearchMode())
  {
    return strPath;
  }

  std::map<CStdString, CStdString> mapOptions;

  mapOptions["remote"] = "true";

  if (!m_strSource.empty() && m_strSource != g_localizeStrings.Get(53512))
  {
	mapOptions["provider"] = m_strSource;
  }

  strPath = BoxeeUtils::AppendParameters(strPath, mapOptions);

  CLog::Log(LOGDEBUG,"CAllShowsWindowState::AddGuiStateParameters, return path = %s (browse)", strPath.c_str());
  return strPath;
}

// ///////////////////////////////////////////////////////////////////////

// IMPLEMENTATION OF THE CGUIWindowBoxeeBrowseTvShows ///////////////////////////

CGUIWindowBoxeeBrowseTvShows::CGUIWindowBoxeeBrowseTvShows()
: CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_TVSHOWS, "boxee_browse_tvshows.xml"),m_renderCount(0)
{
  myShowsState = new CMyShowsWindowState(this);
  allShowsState = new CAllShowsWindowState(this);

  SetWindowState(myShowsState);
}

CGUIWindowBoxeeBrowseTvShows::CGUIWindowBoxeeBrowseTvShows(DWORD dwID, const CStdString &xmlFile)
: CGUIWindowBoxeeBrowseWithPanel(dwID, xmlFile)
{
  myShowsState = new CMyShowsWindowState(this);
  allShowsState = new CAllShowsWindowState(this);

  SetWindowState(myShowsState);
}

CGUIWindowBoxeeBrowseTvShows::~CGUIWindowBoxeeBrowseTvShows()
{
  delete myShowsState;
  delete allShowsState;
}

void CGUIWindowBoxeeBrowseTvShows::OnInitWindow()
{

  // Initialize genres list
  SetGenres();

  SetSources();

  // Filter sources by geo location and hide sources button if nothing is left
  std::vector<BOXEE::BXSourcesItem>::iterator it = m_vecSources.begin();
  while(it != m_vecSources.end())
  {
	  if (!it->GetSourceGeo().empty() && !CUtil::IsCountryAllowed(it->GetSourceGeo(), true))
    {
		it = m_vecSources.erase(it);
	}
	else
	{
		it++;
	}
  }

  if (m_vecSources.size() == 0)
  {
	  SetProperty(SHOW_SOURCE_FLAG, false);
  }

  // We came back from the episodes screen, should return to the previous state
  if (((CTvShowsWindowState*)m_windowState)->m_bInShow)
  {
    ((CTvShowsWindowState*)m_windowState)->m_bInShow = false;
  }
  else
  {
    m_windowState->Reset();
  }

  // Reset Audio Counters
  SetVideoCounters(true);

  CGUIWindowBoxeeBrowseWithPanel::OnInitWindow();
}

bool CGUIWindowBoxeeBrowseTvShows::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (message.GetControlId() == 0 || message.GetControlId() != GetID())
      break;

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::ProcessPanelMessages, GUI_MSG_CLICKED, control = %d (browse)", iControl);

    if (iControl == BUTTON_MY)
    {
      m_windowState = myShowsState;
      m_windowState->Reset();
      Refresh(true);
      return true;
    }
    else if (iControl == BUTTON_ALL || iControl == BUTTON_SHOW_LIBRARY)
    {
      if (!g_application.IsConnectedToInternet())
      {
        CGUIDialogBoxeeNetworkNotification::ShowAndGetInput(53743,53744);
        return true;
      }

      m_windowState = allShowsState;
      m_windowState->Reset();
      Refresh(true);
      return true;
    }
    else if (iControl == BUTTON_FREE)
    {
      ((CTvShowsWindowState*)m_windowState)->OnFree();
      Refresh(true);
      return true;
    }
    else if (iControl == BUTTON_UNWATCHED)
    {
      ((CTvShowsWindowState*)m_windowState)->OnUnwatched();
      UpdateFileList();
    }
    else if (iControl == BUTTON_GENRES)
    {
      CFileItemList genres;
      FillGenresList(genres);

      CStdString value = ((CTvShowsWindowState*)m_windowState)->GetGenre();
      if (CGUIDialogBoxeeDropdown::Show(genres, g_localizeStrings.Get(53561), value))
      {
        ((CTvShowsWindowState*)m_windowState)->SetGenre(value);
        Refresh(true);
      }

      return true;
    }
    else if (iControl == BUTTON_SOURCES)
    {
      CFileItemList sources;
      FillSourcesList(sources);

      CStdString value = ((CTvShowsWindowState*)m_windowState)->GetSource();
      if (CGUIDialogBoxeeDropdown::Show(sources, g_localizeStrings.Get(53565), value))
      {
        CStdString sourceName;
        for (int i = 0; i < sources.Size(); i++)
        {
          if (sources.Get(i)->GetProperty("value") == value)
          {
            ((CTvShowsWindowState*)m_windowState)->SetSource(value, sources.Get(i)->GetLabel());
            Refresh(true);
          }
        }
      }

      return true;
    }
    else if (iControl == BUTTON_SEARCH)
    {
      if (m_windowState->OnSearchStart())
      {
        ClearView();
        SET_CONTROL_FOCUS(50,0);

        Refresh(true);
      }

      return true;
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED

  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}


bool CGUIWindowBoxeeBrowseTvShows::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (m_windowState->OnBack())
    {
      Refresh();
      return true;
    }
    else
    {
      // Open a main menu
      CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);
      pMenu->DoModal();
      return true;
    }
  }
  };

  return CGUIWindowBoxeeBrowse::OnAction(action);
}
void CGUIWindowBoxeeBrowseTvShows::Render()
{
  CGUIWindow::Render();

  m_renderCount ++;
  if (m_renderCount == 120) {
    SetVideoCounters(true);
    m_renderCount = 0;
  }
}

bool CGUIWindowBoxeeBrowseTvShows::OnClick(int iItem)
{
  ((CTvShowsWindowState*)m_windowState)->m_bInShow = true;
  return CGUIWindowBoxeeBrowse::OnClick(iItem);
}

void CGUIWindowBoxeeBrowseTvShows::SetGenres()
{
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvGenres(m_vecGenres);
}

void CGUIWindowBoxeeBrowseTvShows::SetSources()
{
  m_vecSources.clear();
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvSources(m_vecSources);
}

void CGUIWindowBoxeeBrowseTvShows::FillGenresList(CFileItemList& genres)
{
  CFileItemPtr allItem (new CFileItem(g_localizeStrings.Get(53511)));
  allItem->SetProperty("type", "genre");
  allItem->SetProperty("value", g_localizeStrings.Get(53511));
  genres.Add(allItem);

  for (size_t i = 0; i < m_vecGenres.size(); i++) 
  {
    CFileItemPtr genreItem (new CFileItem(m_vecGenres[i].m_genreText));
    genreItem->SetProperty("type", "genre");
    genreItem->SetProperty("value", m_vecGenres[i].m_genreText);
    genres.Add(genreItem);
  }
}

void CGUIWindowBoxeeBrowseTvShows::FillSourcesList(CFileItemList& sources)
{
  CFileItemPtr allItem (new CFileItem(g_localizeStrings.Get(53512)));
  allItem->SetProperty("type", "source");
  allItem->SetProperty("value", g_localizeStrings.Get(53512));
  sources.Add(allItem);

  for (size_t i = 0; i < m_vecSources.size(); i++)
  {
    CFileItemPtr sourceItem (new CFileItem(m_vecSources[i].GetSourceName()));
    sourceItem->SetProperty("type", "source");
    sourceItem->SetProperty("value", m_vecSources[i].GetSourceId());
    sources.Add(sourceItem); 
  }
}

void CGUIWindowBoxeeBrowseTvShows::SetVideoCounters(bool bOn)
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
