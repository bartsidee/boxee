/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIViewState.h"
#include "GUIViewStateMusic.h"
#include "GUIViewStateVideo.h"
#include "GUIViewStatePictures.h"
#include "GUIViewStatePrograms.h"
#include "GUIViewStateScripts.h"
#include "PlayListPlayer.h"
#include "Util.h"
#include "URL.h"
#include "GUIPassword.h"
#include "GUIBaseContainer.h" // for VIEW_TYPE_*
#include "ViewDatabase.h"
#include "AutoSwitch.h"
#include "GUIWindowManager.h"
#include "ViewState.h"
#include "GUISettings.h"
#include "Settings.h"
#include "FileItem.h"
#include "VideoInfoTag.h"
#include "utils/log.h"
#include "Key.h"

using namespace std;

CStdString CGUIViewState::m_strPlaylistDirectory;
VECSOURCES CGUIViewState::m_sources;

CGUIViewState* CGUIViewState::GetViewState(int windowId, const CFileItemList& items)
{

  if (windowId == 0)
    return GetViewState(g_windowManager.GetActiveWindow(),items);

  const CURI& url = CURI(items.m_strPath);
  CStdString strUrl;
  strUrl = url.Get();
  CLog::Log(LOGDEBUG, "CGUIViewState::GetViewState, VIEWSTATE: window id = %d, url %s, protocol = %s, host = %s, share = %s, domain = %s, filename = %s, mode = %d", 
      windowId, strUrl.c_str(), url.GetProtocol().c_str(), url.GetHostName().c_str(), url.GetShareName().c_str(), url.GetDomain().c_str(), url.GetFileName().c_str(), items.GetPropertyInt("browsemode"));

  if (items.HasSortDetails())
    return new CGUIViewStateFromItems(items);

  if (url.GetProtocol()=="musicdb")
    return new CGUIViewStateMusicDatabase(items);

  if (url.GetProtocol()=="musicsearch")
    return new CGUIViewStateMusicSearch(items);

  if (items.IsSmartPlayList())
  {
    if (items.GetContent() == "songs")
      return new CGUIViewStateMusicSmartPlaylist(items);
    else if (items.GetContent() == "musicvideos") // TODO: Update this
      return new CGUIViewStateMusicSmartPlaylist(items);
    else if (items.GetContent() == "tvshows")
      return new CGUIViewStateVideoTVShows(items);
    else if (items.GetContent() == "episodes")
      return new CGUIViewStateVideoEpisodes(items);
    else if (items.GetContent() == "movies")
      return new CGUIViewStateVideoMovies(items);
  }

  if (items.IsPlayList())
    return new CGUIViewStateMusicPlaylist(items);

  if (url.GetProtocol() == "shout" && CUtil::HasSlashAtEnd(url.GetFileName()))
    return new CGUIViewStateMusicShoutcast(items);

  if (url.GetProtocol() == "shout" && !CUtil::HasSlashAtEnd(url.GetFileName()))
      return new CGUIViewStateMusicShoutcastGenre(items);

//  if (url.GetProtocol() == "lastfm" && url.GetFileName() == "")
//    return new CGUIViewStateMusicLastFM(items);
//
//  if (url.GetProtocol() == "lastfm" && url.GetFileName() != "")
//      return new CGUIViewStateMusicLastFMTags(items);
  
  if (url.GetProtocol() == "lastfm")
    return new CGUIViewStateBoxee5(items);

  // ======================== WINDOW_BOXEE_BROWSE views ================================================
  
  // hulu categories
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && (url.GetShareName() == "hulu" || url.GetShareName() == "cbs"))
  {
    // Try to determine whether we are in a category or in the list of episodes
    bool bAllEpisodes = true;
    bool bAllFolders = true;
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items.Get(i); 
      if (!item->m_bIsFolder)
      {
        bAllFolders = false;
      }
      
      if (!item->HasVideoInfoTag() || item->GetVideoInfoTag()->m_iEpisode == 0)
      {
        bAllEpisodes = false;
      }
    }
    
    if (bAllFolders)
    {
      CLog::Log(LOGDEBUG, "CGUIViewState::GetViewState, VIEWSTATE, hulu, all folders");
      return new CGUIViewStateBoxee8(items);
    }
    else 
    {
      if (bAllEpisodes)
      {
        CLog::Log(LOGDEBUG, "CGUIViewState::GetViewState, VIEWSTATE, hulu, all episodes");
        return new CGUIViewStateBoxee7(items);
      }
      else
      {
        CLog::Log(LOGDEBUG, "CGUIViewState::GetViewState, VIEWSTATE, hulu, mixed videos");
        return new CGUIViewStateBoxee6(items);
      }
    }
    
    // default, we should not get here
    return new CGUIViewStateBoxee1(items);
  }
  
  
  // video applications main
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "apps" && url.GetHostName() == "video" && CUtil::HasSlashAtEnd(strUrl))
        return new CGUIViewStateBoxee1(items);
  
  // pictures applications main
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "apps" && url.GetHostName() == "pictures" && CUtil::HasSlashAtEnd(strUrl))
          return new CGUIViewStateBoxee1(items);
  
  // Revision3
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "rss.boxee.tv" && url.GetShareName() == "revision3.xml")
    return new CGUIViewStateBoxee10(items);
  
  // NPR
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "rss.boxee.tv" && url.GetShareName() == "npr.xml")
      return new CGUIViewStateBoxee5(items);
  
  // jamendo
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "rss.boxee.tv" && url.GetShareName() == "jamendo")
      return new CGUIViewStateBoxee5(items);
  
  // BBC
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "rss.boxee.tv" && url.GetShareName() == "bbc.xml")
        return new CGUIViewStateBoxee5(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "rss.boxee.tv" && url.GetShareName() == "video_downloads.xml")
         return new CGUIViewStateBoxee5(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "www.npr.org" && url.GetShareName() == "rss")
        return new CGUIViewStateBoxee5(items);
  
  // My channels
    if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && url.GetHostName() == "app.boxee.tv" && url.GetShareName() == "rss")
      return new CGUIViewStateBoxee5(items);
  
  // BlipTV, categories
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetHostName() == "video" && url.GetShareName() == "BlipTV")
      return new CGUIViewStateBoxee5(items);
  
  // Movie trailers
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetHostName() == "video" && url.GetShareName() == "Movie Trailers")
        return new CGUIViewStateBoxee6(items);
    
  
  // rss category
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss")
  {
    // Try to determine whether we are in a category or in the list of episodes
    for (int i = 0; i < items.Size(); i++)
    {
      CFileItemPtr item = items.Get(i); 
     if (!item->m_bIsFolder)
     {
       // If at least one item is not a folder, return list view sorted by date
       return new CGUIViewStateBoxee3(items);
     }
    }
    return new CGUIViewStateBoxee10(items);
  }
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "history")
  {
    return new CGUIViewStateBoxee9(items);
  }
  
  // Boxee DB special cases
  
  // TV seasons
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "boxeedb" && url.GetHostName() == "series" && CUtil::HasSlashAtEnd(strUrl))
        return new CGUIViewStateBoxee5(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "boxeedb" && url.GetHostName() == "season" && CUtil::HasSlashAtEnd(strUrl))
         return new CGUIViewStateBoxee7(items);
  
  
  // YouTube plugin, main categories
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetShareName() == "Youtube")
      return new CGUIViewStateBoxee4(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetShareName() == "flickr" && CUtil::HasSlashAtEnd(strUrl))
        return new CGUIViewStateBoxee4(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetShareName() == "Seeqpod")
         return new CGUIViewStateBoxee5(items);

  // General settings for RSS channels
//  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && CUtil::HasSlashAtEnd(url.GetFileName()))
//    return new CGUIViewStateWindowVideoRssDirectory(items);
//
//  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && !CUtil::HasSlashAtEnd(url.GetFileName()))
//    return new CGUIViewStateWindowVideoRssItems(items);
//
//  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && CUtil::HasSlashAtEnd(url.GetFileName()))
//    return new CGUIViewStateWindowMusicRssDirectory(items);
//
//  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "rss" && !CUtil::HasSlashAtEnd(url.GetFileName()))
//    return new CGUIViewStateWindowMusicRssItems(items);

  // CNN plugin, categories
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetFileName() == "CNN Video/" && CUtil::HasSlashAtEnd(strUrl))
      return new CGUIViewStateWindowVideoRssDirectory(items);
  
  if (windowId==WINDOW_BOXEE_BROWSE && url.GetProtocol() == "plugin" && url.GetFileName() == "CNN Video/" && !CUtil::HasSlashAtEnd(strUrl))
        return new CGUIViewStateWindowMusicRssItems(items);

//  if (windowId== WINDOW_BOXEE_BROWSE && items.GetProperty("browsemode") == BROWSE_MODE_VIDEO)
//    return new CGUIViewStateBoxeeBrowseVideo(items);
//
//  if (windowId== WINDOW_BOXEE_BROWSE && items.GetProperty("browsemode") == BROWSE_MODE_MUSIC)
//    return new CGUIViewStateBoxee2(items);
//
//  if (windowId== WINDOW_BOXEE_BROWSE && items.GetProperty("browsemode") == BROWSE_MODE_PICTURES)
//    return new CGUIViewStateBoxeeBrowsePictures(items);
//
//  if (windowId== WINDOW_BOXEE_BROWSE && items.GetProperty("browsemode") == BROWSE_MODE_OTHER)
//    return new CGUIViewStateBoxeeBrowseOther(items);

  // ======================== WINDOW_BOXEE_BROWSE views ================================================
  
  if (items.IsSmartPlayList())
    return new CGUIViewStateMusicSmartPlaylist(items);

  if (windowId==WINDOW_MUSIC_NAV)
    return new CGUIViewStateWindowMusicNav(items);

  if (windowId==WINDOW_MUSIC_FILES && url.GetProtocol() == "boxeedb")
      return new CGUIViewStateWindowMusicSongsBoxeeDb(items);

  if (windowId==WINDOW_MUSIC_FILES)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST)
    return new CGUIViewStateWindowMusicPlaylist(items);

  if (windowId==WINDOW_MUSIC_PLAYLIST_EDITOR)
    return new CGUIViewStateWindowMusicSongs(items);

  if (windowId==WINDOW_VIDEO_FILES && url.GetProtocol() == "boxeedb")
       return new CGUIViewStateWindowVideoBoxeeDb(items);

  if (windowId==WINDOW_VIDEO_FILES)
    return new CGUIViewStateWindowVideoFiles(items);

  if (windowId==WINDOW_VIDEO_NAV)
    return new CGUIViewStateWindowVideoNav(items);

  if (windowId==WINDOW_VIDEO_PLAYLIST)
    return new CGUIViewStateWindowVideoPlaylist(items);

  if (windowId==WINDOW_SCRIPTS)
    return new CGUIViewStateWindowScripts(items);

  if (windowId==WINDOW_PICTURES)
    return new CGUIViewStateWindowPictures(items);

  if (windowId==WINDOW_PROGRAMS)
    return new CGUIViewStateWindowPrograms(items);

  //  Use as fallback/default
  return new CGUIViewStateGeneral(items);
}

CGUIViewState::CGUIViewState(const CFileItemList& items) : m_items(items)
{
  m_currentViewAsControl=0;
  m_currentSortMethod=0;
  m_sortOrder=SORT_ORDER_ASC;
}

CGUIViewState::~CGUIViewState()
{
}

SORT_ORDER CGUIViewState::GetDisplaySortOrder() const
{
  // we actually treat some sort orders in reverse, so that we can have
  // the one sort order variable to save but it can be ascending usually,
  // and descending for the views which should be usually descending.
  // default sort order for date, size, program count + rating is reversed
  SORT_METHOD sortMethod = GetSortMethod();
  if (sortMethod == SORT_METHOD_DATE || sortMethod == SORT_METHOD_SIZE ||
      sortMethod == SORT_METHOD_VIDEO_RATING || sortMethod == SORT_METHOD_PROGRAM_COUNT ||
      sortMethod == SORT_METHOD_SONG_RATING)
  {
    if (m_sortOrder == SORT_ORDER_ASC) return SORT_ORDER_DESC;
    if (m_sortOrder == SORT_ORDER_DESC) return SORT_ORDER_ASC;
  }
  return m_sortOrder;
}

SORT_ORDER CGUIViewState::SetNextSortOrder()
{
  if (m_sortOrder==SORT_ORDER_ASC)
    SetSortOrder(SORT_ORDER_DESC);
  else
    SetSortOrder(SORT_ORDER_ASC);

  SaveViewState();

  return m_sortOrder;
}

int CGUIViewState::GetViewAsControl() const
{
  return m_currentViewAsControl;
}

void CGUIViewState::SetViewAsControl(int viewAsControl)
{
  if (viewAsControl == DEFAULT_VIEW_AUTO)
    m_currentViewAsControl = CAutoSwitch::GetView(m_items);
  else
    m_currentViewAsControl = viewAsControl;
}

void CGUIViewState::SaveViewAsControl(int viewAsControl)
{
  SetViewAsControl(viewAsControl);
  SaveViewState();
}

SORT_METHOD CGUIViewState::GetSortMethod() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size()) {
    return m_sortMethods[m_currentSortMethod].m_sortMethod;
  }

  return SORT_METHOD_NONE;
}

int CGUIViewState::GetSortMethodLabel() const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
    return m_sortMethods[m_currentSortMethod].m_buttonLabel;

  return 103; // Sort By: Name
}

void CGUIViewState::GetSortMethods(vector< pair<int,int> > &sortMethods) const
{
  for (unsigned int i = 0; i < m_sortMethods.size(); i++)
    sortMethods.push_back(make_pair(m_sortMethods[i].m_sortMethod, m_sortMethods[i].m_buttonLabel));
}

void CGUIViewState::GetSortMethodLabelMasks(LABEL_MASKS& masks) const
{
  if (m_currentSortMethod>=0 && m_currentSortMethod<(int)m_sortMethods.size())
  {
    masks=m_sortMethods[m_currentSortMethod].m_labelMasks;
    return;
  }

  masks.m_strLabelFile.Empty();
  masks.m_strLabel2File.Empty();
  masks.m_strLabelFolder.Empty();
  masks.m_strLabel2Folder.Empty();
  return;
}

void CGUIViewState::AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks)
{
  SORT_METHOD_DETAILS sort;
  sort.m_sortMethod=sortMethod;
  sort.m_buttonLabel=buttonLabel;
  sort.m_labelMasks=labelmasks;

  m_sortMethods.push_back(sort);
}

void CGUIViewState::SetCurrentSortMethod(int method)
{
  bool ignoreThe = g_guiSettings.GetBool("filelists.ignorethewhensorting");

  if (method < SORT_METHOD_NONE || method >= SORT_METHOD_MAX)
    return; // invalid

  // compensate for "Ignore The" options to make it easier on the skin
  if (ignoreThe && (method == SORT_METHOD_LABEL || method == SORT_METHOD_TITLE || method == SORT_METHOD_ARTIST || method == SORT_METHOD_ALBUM || method == SORT_METHOD_STUDIO || method == SORT_METHOD_VIDEO_SORT_TITLE))
    method++;
  else if (!ignoreThe && (method == SORT_METHOD_LABEL_IGNORE_THE || method == SORT_METHOD_TITLE_IGNORE_THE || method == SORT_METHOD_ARTIST_IGNORE_THE || method==SORT_METHOD_ALBUM_IGNORE_THE || method == SORT_METHOD_STUDIO_IGNORE_THE || method == SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE))
    method--;

  SetSortMethod((SORT_METHOD)method);
  SaveViewState();
}

void CGUIViewState::SetSortMethod(SORT_METHOD sortMethod)
{
  for (int i=0; i<(int)m_sortMethods.size(); ++i)
  {
    if (m_sortMethods[i].m_sortMethod==sortMethod)
    {
      m_currentSortMethod=i;
      break;
    }
  }
}

SORT_METHOD CGUIViewState::SetNextSortMethod(int direction /* = 1 */)
{
  m_currentSortMethod += direction;

  if (m_currentSortMethod >= (int)m_sortMethods.size())
    m_currentSortMethod=0;
  if (m_currentSortMethod < 0)
    m_currentSortMethod = m_sortMethods.size() ? (int)m_sortMethods.size() - 1 : 0;

  SaveViewState();

  return GetSortMethod();
}

bool CGUIViewState::HideExtensions()
{
  return g_guiSettings.GetBool("filelists.hideextensions");
}

bool CGUIViewState::HideParentDirItems()
{
  return g_guiSettings.GetBool("filelists.hideparentdiritems");
}

bool CGUIViewState::DisableAddSourceButtons()
{
  if (g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteSources() || g_passwordManager.bMasterUser)
    return g_guiSettings.GetBool("filelists.disableaddsourcebuttons");

  return true;
}

int CGUIViewState::GetPlaylist()
{
  return PLAYLIST_NONE;
}

const CStdString& CGUIViewState::GetPlaylistDirectory()
{
  return m_strPlaylistDirectory;
}

void CGUIViewState::SetPlaylistDirectory(const CStdString& strDirectory)
{
  m_strPlaylistDirectory=strDirectory;
    CUtil::RemoveSlashAtEnd(m_strPlaylistDirectory);
}

bool CGUIViewState::IsCurrentPlaylistDirectory(const CStdString& strDirectory)
{
  if (g_playlistPlayer.GetCurrentPlaylist()!=GetPlaylist())
    return false;

  CStdString strDir=strDirectory;
    CUtil::RemoveSlashAtEnd(strDir);

  return (m_strPlaylistDirectory==strDir);
}


bool CGUIViewState::UnrollArchives()
{
  return false;
}

bool CGUIViewState::AutoPlayNextItem()
{
  return false;
}

CStdString CGUIViewState::GetLockType()
{
  return "";
}

CStdString CGUIViewState::GetExtensions()
{
  return "";
}

VECSOURCES& CGUIViewState::GetSources()
{
  return m_sources;
}

CGUIViewStateGeneral::CGUIViewStateGeneral(const CFileItemList& items) : CGUIViewState(items)
{
  AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%F", "%I", "%L", ""));  // Filename, size | Foldername, empty
  SetSortMethod(SORT_METHOD_LABEL);

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
}

void CGUIViewState::SetSortOrder(SORT_ORDER sortOrder)
{
  if (GetSortMethod() == SORT_METHOD_NONE)
    m_sortOrder = SORT_ORDER_NONE;
  else
    m_sortOrder = sortOrder;
}

void CGUIViewState::LoadViewState(const CStdString &path, int windowID)
{ // get our view state from the db
  CViewDatabase db;
  if (db.Open())
  {
    CViewState state;
    if (db.GetViewState(path, windowID, state))
    {
      SetViewAsControl(state.m_viewMode);
      SetSortMethod(state.m_sortMethod);
      SetSortOrder(state.m_sortOrder);
    }
    db.Close();
  }
}

void CGUIViewState::SaveViewToDb(const CStdString &path, int windowID, CViewState *viewState)
{
  CViewDatabase db;
  if (db.Open())
  {
    time_t now = time(NULL);
    CViewState state(m_currentViewAsControl, GetSortMethod(), m_sortOrder, now);
    if (viewState)
      *viewState = state;
    db.SetViewState(path, windowID, state);
    db.Close();
    if (viewState)
      g_settings.Save();
  }
}

CGUIViewStateFromItems::CGUIViewStateFromItems(const CFileItemList &items) : CGUIViewState(items)
{
  const vector<SORT_METHOD_DETAILS> &details = items.GetSortDetails();
  for (unsigned int i = 0; i < details.size(); i++)
  {
    const SORT_METHOD_DETAILS sort = details[i];
    AddSortMethod(sort.m_sortMethod, sort.m_buttonLabel, sort.m_labelMasks);
  }
  // TODO: Should default sort/view mode be specified?
  m_currentSortMethod = 0;

  SetViewAsControl(DEFAULT_VIEW_LIST);

  SetSortOrder(SORT_ORDER_ASC);
  LoadViewState(items.m_strPath, g_windowManager.GetActiveWindow());
}

void CGUIViewStateFromItems::SaveViewState()
{
  SaveViewToDb(m_items.m_strPath, g_windowManager.GetActiveWindow());
}

CGUIViewStateBoxeeBrowse::CGUIViewStateBoxeeBrowse(const CFileItemList &items) : CGUIViewState(items)
{

}

void CGUIViewStateBoxeeBrowse::SaveViewState()
{
  // The view setttings are saved based on their browsing mode, saved as item list property
  //SaveViewToDb(m_items.GetProperty("browsemode"), WINDOW_BOXEE_BROWSE);
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowse::LoadViewState, VIEWSTATE, path = %s", m_items.m_strPath.c_str());
  SaveViewToDb(m_items.m_strPath, WINDOW_BOXEE_BROWSE);
}

void CGUIViewStateBoxeeBrowse::LoadViewState(const CFileItemList& items, int windowID)
{
  // Check whether the parent path
  CStdString strPath = items.m_strPath;
  CStdString strParentPath = items.GetProperty("parentPath");
  
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowse::LoadViewState, VIEWSTATE, path = %s, parent path = %s", strPath.c_str(), strParentPath.c_str());
  
  CViewDatabase db;
  if (db.Open())
  {
    CViewState parentState;
    CViewState ownState;
    if (db.GetViewState(strParentPath, windowID, parentState))
    {
      // Get own state
      if (db.GetViewState(strPath, windowID, ownState))
      {
        // has both states assume latest
        if (parentState.m_changeTime > ownState.m_changeTime)
        {
          CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowse::LoadViewState, VIEWSTATE, set parent state, path = %s, parent path = %s", strPath.c_str(), strParentPath.c_str());
          // Parent state wins
          SetViewAsControl(parentState.m_viewMode);
          SetSortMethod(parentState.m_sortMethod);
          SetSortOrder(parentState.m_sortOrder);
          SaveViewToDb(strPath, windowID);
        }
        else
        {
          // Own state wins
          SetViewAsControl(ownState.m_viewMode);
          SetSortMethod(ownState.m_sortMethod);
          SetSortOrder(ownState.m_sortOrder);
        }
        
      }
      else 
      {
        // No own state, and parent state exists, assume parent state
        SetViewAsControl(parentState.m_viewMode);
        SetSortMethod(parentState.m_sortMethod);
        SetSortOrder(parentState.m_sortOrder);
        SaveViewToDb(strPath, windowID);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowse::LoadViewState, VIEWSTATE, loading by current path, path = %s, parent path = %s", strPath.c_str(), strParentPath.c_str());
      CGUIViewState::LoadViewState(items.m_strPath, windowID);
    }
    
    db.Close();
  }
  else
  {
    CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowse::LoadViewState, VIEWSTATE, could not open database, try to load by items path");
    CGUIViewState::LoadViewState(items.m_strPath, windowID);
  }
}

CGUIViewStateBoxeeBrowseVideo::CGUIViewStateBoxeeBrowseVideo(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowseVideo, VIEWSTATE");
  const CURI& url = CURI(items.m_strPath);

  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date

  if (url.GetHostName() == "season") {
    AddSortMethod(SORT_METHOD_EPISODE, 20359, LABEL_MASKS()); // Preformated
    SetSortMethod(SORT_METHOD_EPISODE);
    SetSortOrder(SORT_ORDER_ASC);
  }
  else if (url.GetProtocol() == "rss") {
    SetSortMethod(SORT_METHOD_DATE_WITH_SHARES);
    SetSortOrder(SORT_ORDER_DESC);
  }
  else {
    SetSortMethod(SORT_METHOD_LABEL_WITH_SHARES);
    SetSortOrder(SORT_ORDER_ASC);
  }

  //SetViewAsControl(g_guiSettings.GetInt("myvideos.viewmode"));
  SetViewAsControl(DEFAULT_VIEW_ICONS | 52);

  LoadViewState(items.GetProperty("browsemode"), WINDOW_BOXEE_BROWSE);
}

//CGUIViewStateBoxeeBrowseMusic::CGUIViewStateBoxeeBrowseMusic(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
//{
//  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowseMusic, VIEWSTATE");
//  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
//  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
//  
//  SetSortMethod(SORT_METHOD_LABEL_WITH_SHARES);
//  SetSortOrder(SORT_ORDER_ASC);
//
//  SetViewAsControl(DEFAULT_VIEW_ICONS | 52);
//
//  LoadViewState(items.GetProperty("browsemode"), WINDOW_BOXEE_BROWSE);
//
//}

CGUIViewStateBoxeeBrowsePictures::CGUIViewStateBoxeeBrowsePictures(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowsePictures, VIEWSTATE");
  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
  
  SetSortMethod(SORT_METHOD_DATE_WITH_SHARES);
  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(DEFAULT_VIEW_ICONS | 52);

  LoadViewState(items.GetProperty("browsemode"), WINDOW_BOXEE_BROWSE);

}

CGUIViewStateBoxeeBrowseOther::CGUIViewStateBoxeeBrowseOther(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxeeBrowseOther, VIEWSTATE");
  const CURI& url = CURI(items.m_strPath);

  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    
  if (url.GetHostName() == "activity" || url.GetHostName() == "recommend" || url.GetHostName() == "all") {
    SetSortMethod(SORT_METHOD_DATE_WITH_SHARES);
    SetSortOrder(SORT_ORDER_DESC);
  }
  else if (url.GetHostName() == "recent") {
    AddSortMethod(SORT_METHOD_DATE_ADDED, 570, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    SetSortMethod(SORT_METHOD_DATE_ADDED);
    SetSortOrder(SORT_ORDER_DESC);
  }
  else {
    
    SetSortMethod(SORT_METHOD_LABEL_WITH_SHARES);
    SetSortOrder(SORT_ORDER_ASC);
  }
  

  //SetViewAsControl(DEFAULT_VIEW_ICONS | 50);
  SetViewAsControl(50);

  LoadViewState(items.GetProperty("browsemode"), WINDOW_BOXEE_BROWSE);

}

CGUIViewStateBoxee1::CGUIViewStateBoxee1(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee1, VIEWSTATE");
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
  }
  else {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);
  }
    
  SetSortOrder(SORT_ORDER_ASC);
   
  SetViewAsControl(DEFAULT_VIEW_ICONS | 52);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee2::CGUIViewStateBoxee2(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee2, date with shares and label with shares, by label asc, 3 rows default view, VIEWSTATE");
  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    
  SetSortMethod(SORT_METHOD_LABEL_WITH_SHARES);
  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(DEFAULT_VIEW_ICONS | 52);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee3::CGUIViewStateBoxee3(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee3, date with shares and label with shares, by label asc, list w/Preview default view, VIEWSTATE");
  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    
  SetSortMethod(SORT_METHOD_DATE_WITH_SHARES);
  SetSortOrder(SORT_ORDER_DESC);

  SetViewAsControl(50);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee4::CGUIViewStateBoxee4(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee4, label only, by label asc, details default view, VIEWSTATE");
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
  }
  else {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);
  }

  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(50);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee5::CGUIViewStateBoxee5(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee5, label only, by label asc, list w/Preview  default view, VIEWSTATE");
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
  }
  else {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);
  }

  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(50);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee6::CGUIViewStateBoxee6(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee6, date with shares and label with shares, by label asc, 3 rows with preview default view, VIEWSTATE");
  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
    
  SetSortMethod(SORT_METHOD_LABEL_WITH_SHARES);
  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(DEFAULT_VIEW_ICONS | 51);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee7::CGUIViewStateBoxee7(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee7, date with shares and label with shares, by label asc, list w/Preview default view, VIEWSTATE");
  AddSortMethod(SORT_METHOD_LABEL_WITH_SHARES, 551, LABEL_MASKS("%L", "%I", "%L", ""));  // FileName, Size | Foldername, empty
  AddSortMethod(SORT_METHOD_DATE_WITH_SHARES, 552, LABEL_MASKS("%L", "%J", "%L", "%J"));  // FileName, Date | Foldername, Date
  AddSortMethod(SORT_METHOD_EPISODE, 20359, LABEL_MASKS()); // Preformated
  SetSortMethod(SORT_METHOD_EPISODE);
  SetSortOrder(SORT_ORDER_ASC);
    
  SetViewAsControl(50);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee8::CGUIViewStateBoxee8(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee8, label only, by label asc, details default view, VIEWSTATE");
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
  }
  else {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);
  }

  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(53);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee9::CGUIViewStateBoxee9(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee9, label only, by label asc, details default view, VIEWSTATE");
//  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
//    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
//    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
//  }
//  else {
//    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
//    SetSortMethod(SORT_METHOD_LABEL);
//  }
//
//  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(50);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

CGUIViewStateBoxee10::CGUIViewStateBoxee10(const CFileItemList &items) : CGUIViewStateBoxeeBrowse(items)
{
  CLog::Log(LOGDEBUG, "CGUIViewStateBoxee10, label only, by label asc, 3row w/preview default view, VIEWSTATE");
  if (g_guiSettings.GetBool("filelists.ignorethewhensorting")) {
    AddSortMethod(SORT_METHOD_LABEL_IGNORE_THE, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL_IGNORE_THE);
  }
  else {
    AddSortMethod(SORT_METHOD_LABEL, 551, LABEL_MASKS("%K", "%I", "%L", ""));  // Titel, Size | Foldername, empty
    SetSortMethod(SORT_METHOD_LABEL);
  }

  SetSortOrder(SORT_ORDER_ASC);

  SetViewAsControl(51);

  LoadViewState(items, WINDOW_BOXEE_BROWSE);
  
}

