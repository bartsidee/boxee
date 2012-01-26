
#include "GUIWindowBoxeeBrowseAlbums.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "FileSystem/BoxeeDatabaseDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "GUIWindowBoxeeBrowseTracks.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "MusicInfoTag.h"
#include "FileSystem/Directory.h"
#include "Application.h"
#include "GUIDialogSelect.h"
#include "lib/libBoxee/bxutils.h"
#include "bxaudiodatabase.h"
#include "lib/libBoxee/boxee.h"
#include "SpecialProtocol.h"
#include "BoxeeDatabaseDirectory.h"


using namespace std;
using namespace BOXEE;

#define BUTTON_ARTISTS 120
#define BUTTON_ALBUMS  130
#define BUTTON_SHOW_APPS  131
#define BUTTON_GENRES  150
#define BUTTON_SEARCH  160

#define STATE_SHOW_ALL_ALBUMS    1
#define STATE_SHOW_ALL_ARTISTS   2
#define STATE_SHOW_ARTIST_ALBUMS 3

#define CUSTOM_GENRE_FILTER 600

#define RESOLVED_AUDIO_LABEL    501
#define SCANNING_LABEL          502

// STATE IMPLEMENTATION

CAlbumsWindowState::CAlbumsWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
  m_strGenre = g_localizeStrings.Get(53511);
  m_iState = STATE_SHOW_ALL_ARTISTS;

  SetSearchType("music");

  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort("title", SORT_METHOD_ALBUM, SORT_ORDER_ASC, g_localizeStrings.Get(53505), ""));
  m_vecSortMethods.push_back(CBoxeeSort("release", SORT_METHOD_DATE_ADDED, SORT_ORDER_DESC, g_localizeStrings.Get(53506), ""));

  SetSort(m_vecSortMethods[0]);

  m_bInTracks = false;
  m_iSelectedArtist = -1;
}

void CAlbumsWindowState::Reset()
{
  CBrowseWindowState::Reset();

  // Reset view labels
  switch (m_iState)
  {
  case STATE_SHOW_ALL_ALBUMS:
    m_pWindow->SetProperty("albums-set", true);
    m_pWindow->SetProperty("artists-set", false);
    m_pWindow->SetProperty("type-label", "ALBUMS");
    break;
  case STATE_SHOW_ALL_ARTISTS:
    m_pWindow->SetProperty("albums-set", false);
    m_pWindow->SetProperty("artists-set", true);
    m_pWindow->SetProperty("type-label", "ARTISTS");
    break;
  case STATE_SHOW_ARTIST_ALBUMS:
    m_pWindow->SetProperty("albums-set", true);
    m_pWindow->SetProperty("artists-set", true);
    m_pWindow->SetProperty("type-label", m_strArtist);
    m_pWindow->SetProperty("sort-label", "");
    break;
  }

  m_strArtist = "";

  // Reset genre
  SetGenre(g_localizeStrings.Get(53511));

  CBrowseWindowState::Reset();

}

CStdString CAlbumsWindowState::CreatePath()
{
  CStdString strPath;

  if (InSearchMode())
  {
    if (!m_strSearchString.IsEmpty())
    {
      strPath = "boxeedb://music/?search=";
      strPath += GetSearchString();
    }
  }
  else
  {
    switch (m_iState)
    {
    case STATE_SHOW_ALL_ALBUMS:
      strPath = "boxeedb://albums/";
      break;
    case STATE_SHOW_ALL_ARTISTS:
      strPath = "boxeedb://artists/";
      break;
    case STATE_SHOW_ARTIST_ALBUMS:
      return GetCurrentPath(); //m_configuration.m_strPath;
    }

    strPath = AddGuiStateParameters(strPath);
  }

  CLog::Log(LOGDEBUG,"CAlbumsWindowState::CreatePath, created path = %s (browse)", strPath.c_str());
  return strPath;
}

CStdString CAlbumsWindowState::AddGuiStateParameters(const CStdString& _strPath)
{
  std::map<CStdString, CStdString> mapOptions;

  mapOptions["genre"] = m_strGenre;

  CStdString strPath = _strPath;
  strPath += BoxeeUtils::BuildParameterString(mapOptions);

  return strPath;
}

void CAlbumsWindowState::SortItems(CFileItemList &items)
{
  if (m_iState == STATE_SHOW_ALL_ARTISTS)
  {
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  }
  else
  {
    CBrowseWindowState::SortItems(items);
  }
}

bool CAlbumsWindowState::OnBack()
{
  if (OnSearchEnd())
  {
    if (m_iState == STATE_SHOW_ALL_ALBUMS)
    {
      OnAlbums();
    }
    else
    {
      OnArtists();
    }
    return true;
  }
  else if (m_iState == STATE_SHOW_ARTIST_ALBUMS)
  {
    OnArtists();
    return true;
  }
  return false;
}

bool CAlbumsWindowState::OnAlbums()
{
  if (m_iState == STATE_SHOW_ALL_ALBUMS)
    return false;

  m_iState = STATE_SHOW_ALL_ALBUMS;

  Reset();

  return true;
}

bool CAlbumsWindowState::OnArtists()
{
  if (m_iState == STATE_SHOW_ALL_ARTISTS)
    return false;

  m_iState = STATE_SHOW_ALL_ARTISTS;
  m_configuration.m_iSelectedItem = m_iSelectedArtist;

  Reset();

  return true;
}

bool CAlbumsWindowState::OnArtist(const CStdString& strArtist)
{
  m_strArtist = strArtist;

  if (m_iState == STATE_SHOW_ARTIST_ALBUMS)
    return false;

  m_iState = STATE_SHOW_ARTIST_ALBUMS;

  Reset();

  return true;
}

void CAlbumsWindowState::SetArtist(const CStdString& strArtist)
{
  m_strArtist = strArtist;
}

void CAlbumsWindowState::SetGenre(const CStdString& strGenre)
{
  m_strGenre = strGenre;
  //m_configuration.ClearActiveFilters();

  m_configuration.RemoveCustomFilter(CUSTOM_GENRE_FILTER);

  if (m_strGenre.CompareNoCase(g_localizeStrings.Get(53511)) != 0)
  {
    m_pWindow->SetProperty("genre-label", m_strGenre);
    m_configuration.AddCustomFilter(new CBrowseWindowAlbumGenreFilter(CUSTOM_GENRE_FILTER, "Album Genre Filter", m_strGenre));
  }
  else
  {
    m_pWindow->SetProperty("genre-label", ""); // reset genre
  }
}

// WINDOW IMPLEMENTATION

CGUIWindowBoxeeBrowseAlbums::CGUIWindowBoxeeBrowseAlbums()
: CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_ALBUMS, "boxee_browse_music.xml"), m_renderCount(0)
{
  SetWindowState(new CAlbumsWindowState(this));
}

CGUIWindowBoxeeBrowseAlbums::~CGUIWindowBoxeeBrowseAlbums()
{

}

void CGUIWindowBoxeeBrowseAlbums::Render()
{
  CGUIWindow::Render();

  m_renderCount ++;
  if (m_renderCount == 120) {
    SetAudioCounters(true);
    m_renderCount = 0;
  }
}

void CGUIWindowBoxeeBrowseAlbums::OnInitWindow()
{
  // clear vector before initializing
  m_vecGenres.clear();

  DIRECTORY::CBoxeeDatabaseDirectory dir;
  dir.GetMusicGenres(m_vecGenres);

  if (((CAlbumsWindowState*)m_windowState)->m_bInTracks)
  {
    ((CAlbumsWindowState*)m_windowState)->m_bInTracks = false;
  }
  else
  {
    m_windowState->Reset();
  }
  // Reset Audio Counters
  SetAudioCounters(true);

  CGUIWindowBoxeeBrowseWithPanel::OnInitWindow();
}

bool CGUIWindowBoxeeBrowseAlbums::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    if (iControl == BUTTON_ALBUMS)
    {
      ResetHistory();
      ((CAlbumsWindowState*)m_windowState)->OnAlbums();
      Refresh();
      return true;
    }
    else if (iControl == BUTTON_ARTISTS)
    {
      ResetHistory();
      ((CAlbumsWindowState*)m_windowState)->OnArtists();
      Refresh();
      return true;
    }
    else if (iControl == BUTTON_SHOW_APPS)
    {
      ((CGUIWindowBoxeeBrowseWithPanel*)g_windowManager.GetWindow(WINDOW_BOXEE_BROWSE_APPS))->ShowPanel();
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS,"apps://all");
      return true;
    }
    else if (iControl == BUTTON_GENRES)
    {
      CFileItemList genres;
      FillGenresList(genres);

      CStdString value;
      if (CGUIDialogBoxeeDropdown::Show(genres, g_localizeStrings.Get(53561), value))
      {
        ((CAlbumsWindowState*)m_windowState)->SetGenre(value);
        UpdateFileList();
      }

      return true;
    }
    else if (iControl == BUTTON_SEARCH)
    {
      if (m_windowState->OnSearchStart())
      {
        ClearView();
        SET_CONTROL_FOCUS(9000,0);

        Refresh(true);
      }

      return true;
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED
  }// switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}

bool CGUIWindowBoxeeBrowseAlbums::OnAction(const CAction &action)
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
      CGUIDialogBoxeeMainMenu* pMenu = (CGUIDialogBoxeeMainMenu*)g_windowManager.GetWindow(WINDOW_BOXEE_DIALOG_MAIN_MENU);
      pMenu->DoModal();
      return true;
    }
  }
  };

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseAlbums::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::OnClick, item = %d (browse)", iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
    return true;

  item.Dump();

  if (item.GetPropertyBOOL("isalbum"))
  {
    CStdString strAlbum = item.GetMusicInfoTag()->GetAlbum();
    strAlbum = BXUtils::URLEncode(strAlbum);

    CStdString strArtist = item.GetMusicInfoTag()->GetArtist().IsEmpty() ? item.GetMusicInfoTag()->GetAlbumArtist() :item.GetMusicInfoTag()->GetArtist();
    strArtist = BXUtils::URLEncode(strArtist);

    CStdString strAlbumPath;
    strAlbumPath.Format("boxeedb://album/?title=%s&artist=%s", strAlbum.c_str(), strArtist.c_str());


    CFileItemList albumItems;
    CFileItemList availableAlbums;
    DIRECTORY::CDirectory::GetDirectory(strAlbumPath, albumItems);

    // Fill available options into a context menu dialog

    for (int i = 0; i < albumItems.Size(); i++)
    {
      CStdString strPath = albumItems.Get(i)->GetProperty("AlbumFolderPath");
      if (g_application.IsPathAvailable(strPath, true))
      {
        availableAlbums.Add(albumItems.Get(i));
      }
    }

    CStdString strAlbumId;
    if (availableAlbums.Size() > 1)
    {
      // with SELECT DIALOG
      CGUIDialogSelect *pDlgSelect = (CGUIDialogSelect*)g_windowManager.GetWindow(WINDOW_DIALOG_SELECT);

      pDlgSelect->SetHeading(396); //"Select Location"
      pDlgSelect->Reset();

      for (int i = 0; i < availableAlbums.Size(); i++)
      {
        pDlgSelect->Add(availableAlbums.Get(i)->GetProperty("AlbumFolderPath"));
      }

      pDlgSelect->EnableButton(TRUE);
      pDlgSelect->SetButtonLabel(222); //'Cancel' button returns to weather settings
      pDlgSelect->DoModal();

      int selectedIndex = pDlgSelect->GetSelectedLabel();
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::OnClick - CGUIDialogSelect returned [selectedIndex=%d] (browse)",selectedIndex);

      if (selectedIndex >= 0)
      {
        strAlbumId = availableAlbums.Get(pDlgSelect->GetSelectedLabel())->GetProperty("BoxeeDBAlbumId");
      }

    }
    else if (availableAlbums.Size() == 1)
    {
      strAlbumId = availableAlbums.Get(0)->GetProperty("BoxeeDBAlbumId");
    }
    else
    {
      // this should not happen
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseAlbums::OnClick, no available albums to play (browse)");
      return false;
    }

    if (!strAlbumId.IsEmpty())
    {
      CStdString strPath = "boxeedb://tracks/";
      strPath += strAlbumId;

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseAlbums::OnClick - Going to activate WINDOW_BOXEE_BROWSE_TRACKS with [strPath=%s] (browse)",strPath.c_str());

      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TRACKS, strPath);
      ((CAlbumsWindowState*)m_windowState)->m_bInTracks = true;
    }

    return true;
  }
  else if (item.GetPropertyBOOL("isartist"))
  {
    // Update state in order to be able to return to correct result
    ((CAlbumsWindowState*)m_windowState)->m_iSelectedArtist = iItem;
    ((CAlbumsWindowState*)m_windowState)->OnArtist(item.GetLabel());
  }

  return CGUIWindowBoxeeBrowse::OnClick(iItem);
}

void CGUIWindowBoxeeBrowseAlbums::FillGenresList(CFileItemList& genres)
{
  CFileItemPtr allItem (new CFileItem(g_localizeStrings.Get(53511)));
  allItem->SetProperty("type", "genre");
  allItem->SetProperty("value", g_localizeStrings.Get(53511));
  genres.Add(allItem);

  for (size_t i = 0; i < m_vecGenres.size(); i++)
  {
    CFileItemPtr genreItem (new CFileItem(m_vecGenres[i]));
    genreItem->SetProperty("type", "genre");
    genreItem->SetProperty("value", m_vecGenres[i]);
    genres.Add(genreItem);
  }
}

void CGUIWindowBoxeeBrowseAlbums::SetAudioCounters(bool bOn) {

  int resolved_count = 0;
  int unresolved_count = 0;
  bool is_scanning = false;

  if (bOn)
  {
    BOXEE::BXAudioDatabase audio_db;

    DIRECTORY::CBoxeeDatabaseDirectory dummyDir;
    std::vector<std::string> vecAudioShares;
    if (!dummyDir.CreateShareFilter("music",vecAudioShares,false))
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvShows::SetVideoCounters - Couldnt create video share list");
      return;
    }

    //build video share list.
    CStdString audio_share_list = "";

    for (size_t i = 0; i < vecAudioShares.size(); i++)
    {
      audio_share_list += "\'";
      audio_share_list += _P(vecAudioShares[i].c_str());
      audio_share_list += "\'";

      if (i < vecAudioShares.size() -1 )
        audio_share_list += ',';
    }


    resolved_count = audio_db.GetUserUnresolvedAudioFilesCount(audio_share_list, STATUS_RESOLVED);
    unresolved_count = audio_db.GetUserUnresolvedAudioFilesCount(audio_share_list, STATUS_UNRESOLVED);
    is_scanning = audio_db.AreAudioFilesBeingScanned(audio_share_list);

  }

  char     tmp[100];

  sprintf(tmp, "%d files found, %d unresolved" , resolved_count + unresolved_count, unresolved_count);
  SET_CONTROL_LABEL(RESOLVED_AUDIO_LABEL, tmp);

  if (is_scanning)
    SET_CONTROL_VISIBLE(SCANNING_LABEL);
  else
    SET_CONTROL_HIDDEN(SCANNING_LABEL);

}

