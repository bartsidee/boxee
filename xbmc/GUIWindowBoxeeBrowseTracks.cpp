
#include "GUIWindowBoxeeBrowseTracks.h"
#include "PlayListPlayer.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "MusicInfoTag.h"
#include "BoxeeShortcut.h"
#include "Settings.h"
#include "GUIWindowManager.h"
#include "GUIDialogBoxeeShare.h"
#include "Application.h"
#include "BoxeeUtils.h"
#include "Directory.h"
#include "ThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include <iostream>
#include <sstream>
#include <string>
#include "GUIImage.h"
#include "FileSystem/File.h"
#include "GUIDialogBoxeeManualResolveAudio.h"
#include "GUIDialogFileBrowser.h"
#include "bxmetadataengine.h"
#include "Picture.h"
#include "Util.h"

using namespace std;

#define BUTTON_SHARE          150

#define BUTTON_RESCAN_ALBUM   170
#define BUTTON_MANUAL_RESOLVE 180
#define BUTTON_TRACK_MISSING  190
#define BUTTON_CHANGE_THUMB   140

#define ALBUM_COVER           8005  



CTracksSource::CTracksSource(int iWindowID) : CBrowseWindowSource("remoteepisodessource", "boxeedb://tracks/", iWindowID)
{

}

CTracksSource::~CTracksSource(){}

void CTracksSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  mapOptions["albumId"] =  m_strAlbumId;

  CBrowseWindowSource::AddStateParameters(mapOptions);
}


// STATE IMPLEMENTATION

CTracksWindowState::CTracksWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.SetNewSource(new CTracksSource(m_pWindow->GetID()));
}

void CTracksWindowState::InitState()
{
  // Set the album id to the source
  CBrowseWindowSource* source = m_sourceController.GetSourceById("remoteepisodessource");
  if (source)
  {
    if (((CTracksSource*)source)->m_strAlbumId != m_strAlbumId)
    {
      ((CTracksSource*)source)->m_strAlbumId = m_strAlbumId;
      m_iSelectedItem = 0;
    }
  }

  CBrowseWindowState::InitState();
}

void CTracksWindowState::SortItems(CFileItemList &items)
{
  items.Sort(SORT_METHOD_TRACKNUM, SORT_ORDER_ASC);
}

void CTracksWindowState::SetAlbumData(const CStdString& strAlbumId, const CStdString& strAlbumName, const CStdString& strAlbumThumb)
{
  m_strAlbumId    = strAlbumId;
  m_strAlbumName  = strAlbumName;
  m_strAlbumThumb = strAlbumThumb;
}

void CTracksWindowState::SetAlbumId(const CStdString& strAlbumId)
{
  m_strAlbumId = strAlbumId;
}

////////////////////////////////////////////////////////////////////
CGUIWindowBoxeeBrowseTracks::CGUIWindowBoxeeBrowseTracks()
: CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_TRACKS, "boxee_browse_music_tracks.xml")
{
  SetWindowState(new CTracksWindowState(this));
}

CGUIWindowBoxeeBrowseTracks::~CGUIWindowBoxeeBrowseTracks()
{

}

void CGUIWindowBoxeeBrowseTracks::OnInitWindow()
{
  CGUIWindowBoxeeBrowse::OnInitWindow();
}

bool CGUIWindowBoxeeBrowseTracks::OnBind(CGUIMessage& message)
{
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    CFileItemList *items = (CFileItemList *)message.GetPointer();

    if (m_albumItem.get() == NULL || m_albumItem->GetProperty("BoxeeDBAlbumId") != items->GetProperty("BoxeeDBAlbumId") || m_albumItem->m_strPath != items->m_strPath)
    {
      m_bResetPlaylist = true;
      m_albumItem = CFileItemPtr(new CFileItem(*items));

      CStdString strAlbumId = items->GetProperty("BoxeeDBAlbumId");
      CStdString strAlbumName = items->GetLabel();
      CStdString strAlbumThumb = items->GetThumbnailImage();

      ((CTracksWindowState*)m_windowState)->SetAlbumData(strAlbumId, strAlbumName, strAlbumThumb);

      CMusicThumbLoader loader;
      loader.LoadItem(m_albumItem.get(), false);

      SetProperty("title", m_albumItem->GetLabel());
      SetProperty("albumthumb", m_albumItem->GetThumbnailImage());
      SetProperty("albumyear", m_albumItem->GetMusicInfoTag()->GetYear());
      SetProperty("albumartist", m_albumItem->GetMusicInfoTag()->GetArtist());
      SetProperty("albumgenre", m_albumItem->GetMusicInfoTag()->GetGenre());
    }

  }

  return CGUIWindowBoxeeBrowse::OnBind(message);
}

bool CGUIWindowBoxeeBrowseTracks::OnClick(int iItem)
{
  if (m_bResetPlaylist || g_playlistPlayer.GetCurrentPlaylist() != PLAYLIST_MUSIC)
  {
    m_bResetPlaylist = false;

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, m_vecViewItems);

    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
  }

  g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(m_albumItem.get()));
  g_playlistPlayer.Play(iItem);

  // show visualization
  g_application.getApplicationMessenger().SwitchToFullscreen();

  return true;
}

void CGUIWindowBoxeeBrowseTracks::OnBack()
{
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_ALBUMS);
}

bool CGUIWindowBoxeeBrowseTracks::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_WINDOW_INIT)
  {
    // Get show id from the message and set it to the state
    CStdString strAlbumId = message.GetStringParam();
    if (!strAlbumId.IsEmpty())
      ((CTracksWindowState*)m_windowState)->SetAlbumId(strAlbumId);
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseTracks::SetThumbFile(const CStdString& filename)
{
  CGUIImage* pImage = (CGUIImage*)GetControl(ALBUM_COVER);
  if (pImage)
  {
    pImage->SetFileName(filename);
    return true;
  }

  return false;
}

bool CGUIWindowBoxeeBrowseTracks::ReloadThumb()
{
  CGUIImage* pImage = (CGUIImage*)GetControl(ALBUM_COVER);
  if (pImage)
  {
    pImage->AllocResources();
    pImage->SetVisible(true);
    return true;
  }
  return false;
}

bool CGUIWindowBoxeeBrowseTracks::UnloadThumb()
{
  CGUIImage* pImage = (CGUIImage*)GetControl(ALBUM_COVER);
  if (pImage)
  {
    pImage->SetVisible(false);
    pImage->FreeResources();
    pImage->SetFileName("");
    return true;
  }
  return false;
}

bool CGUIWindowBoxeeBrowseTracks::OnRescan()
{
  CStdString pathToRescan = m_albumItem->GetProperty("AlbumFolderPath");

  if (m_albumItem->GetThumbnailImage().size() > 0)
  {
    XFILE::CFile::Delete(m_albumItem->GetThumbnailImage()); //delete the file old cached thumbnail
  }

  UnloadThumb();

  BOXEE::Boxee::GetInstance().GetMetadataEngine().Rescan(pathToRescan);

  return true;
}

bool CGUIWindowBoxeeBrowseTracks::OnManualResolve()
{
  return CGUIDialogBoxeeManualResolveAudio::Show(m_albumItem);
}

bool CGUIWindowBoxeeBrowseTracks::OnShare()
{
  CGUIDialogBoxeeShare* pShare = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);

  if (pShare)
  {
    m_albumItem->Dump();

    pShare->SetItem(m_albumItem.get());
    pShare->DoModal();
    return true;
  }

  return false;
}

bool CGUIWindowBoxeeBrowseTracks::OnThumbChange()
{
  VECSOURCES shares(g_settings.m_musicSources);
  CStdString path = m_albumItem->GetProperty("AlbumFolderPath");
  
  if (CGUIDialogFileBrowser::ShowAndGetFile(shares,g_stSettings.m_pictureExtensions,g_localizeStrings.Get(300),path,true))
  {
    CStdString albumThumbPath = CUtil::GetCachedAlbumThumb(m_albumItem->GetMusicInfoTag()->GetAlbum(), m_albumItem->GetMusicInfoTag()->GetAlbumArtist()); //get the cached thumb
    XFILE::CFile::Delete(albumThumbPath); //delete the old thumb
    CPicture::CreateThumbnail(path, albumThumbPath , false); //create the new thumb

    CUtil::ThumbCacheAdd(albumThumbPath,true); //add it to the cache

    m_albumItem->SetThumbnailImage(albumThumbPath); //set the album item to contain the new thumb

    //update the db
    BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();    
    BXAlbum Album;

    MDE.GetAlbumByPath(m_albumItem->GetProperty("AlbumFolderPath"),&Album);

    //the path string should have the full path to the selected thumbnail after the ShowAndGetFile
    Album.m_strArtwork = path;

    MDE.UpdateAlbum(&Album);

    return true;
  }
  return false;
}

bool CGUIWindowBoxeeBrowseTracks::OnAddTrack()
{
  VECSOURCES shares(g_settings.m_musicSources);
  CStdString path = m_albumItem->GetProperty("AlbumFolderPath");
  
  if (CGUIDialogFileBrowser::ShowAndGetFile(shares,g_stSettings.m_musicExtensions,g_localizeStrings.Get(299),path))
  {
    BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
    BOXEE::BXMetadata metadata(MEDIA_ITEM_TYPE_AUDIO); //used for the selected audio file

    //get the selected audio file info
    MDE.GetAudioByPath(path,&metadata);

    BXAudio* pAudio = (BXAudio*) metadata.GetDetail(MEDIA_DETAIL_AUDIO);
    
    BXMetadata metadataAlbum(MEDIA_ITEM_TYPE_ALBUM); //used for the album we're currently appending to
    BXAlbum* pAlbum;

    //read the current album from the database
    MDE.GetAlbum(m_albumItem->GetMusicInfoTag()->GetAlbum(),m_albumItem->GetMusicInfoTag()->GetArtist(),&metadataAlbum);

    pAlbum = (BXAlbum*) metadataAlbum.GetDetail(MEDIA_DETAIL_ALBUM);

    //append the id of the album and artist to the audio file
    pAudio->m_iAlbumId =  pAlbum->m_iId;
    pAudio->m_iArtistId = pAlbum->m_iArtistId;

    //remove the old file
    MDE.RemoveAudioByPath(pAudio->m_strPath);

    //add the same file with the new ids, now related to the album we're in.
    MDE.AddAudio(pAudio);

    //no need to resolve this audio file again
    MDE.UpdateAudioFileStatus(pAudio->m_strPath,STATUS_RESOLVED);

    return true;
  }
  
  return false;
}

void CGUIWindowBoxeeBrowseTracks::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTracks::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_local_music_categories",browseMenuLevelList);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTracks::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu]. [category=%s] (bm)",browseMenuLevelList.size(),category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTracks::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());
}

