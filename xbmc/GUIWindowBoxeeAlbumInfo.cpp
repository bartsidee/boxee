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

#include "GUIWindowBoxeeAlbumInfo.h"
#include "Util.h"
#include "GUIImage.h"
#include "Picture.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "MusicDatabase.h"
#ifdef HAS_LASTFM
#include "LastFmManager.h"
#endif
#include "MusicInfoTag.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "FileSystem/Directory.h"
#include "FileItem.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "PlayListPlayer.h"
//#include "GUIDialogBoxeeManualResolve.h"
#include "VideoInfoTag.h"
#include "GUIDialogYesNo2.h"
#include "PlayList.h"
#include "lib/libBoxee/bxmetadata.h"
#include "lib/libBoxee/boxee.h"
#include "BoxeeUtils.h"
#include "LocalizeStrings.h"
#include "AdvancedSettings.h"
#include "GUIUserMessages.h"
#include "log.h"

using namespace XFILE;
using namespace PLAYLIST;

#define CONTROL_BTN_GET_THUMB 10
#define CONTROL_BTN_LASTFM    11

#define CONTROL_LIST          50
#define CONTROL_TEXTAREA      100

#define CONTROL_BTN_IMAGE 120
#define CONTROL_IMAGE 121

#define INFO_HIDDEN_LIST 5000

#define BTN_PLAY      9001
#define BTN_DOWNLOAD  9002
#define BTN_MOREINFO  9003
//#define BTN_ALBUM     9004
#define BTN_LASTFM    9005
#define BTN_RATE      9006
#define BTN_RECOMMEND 9007

#define BTN_MANUAL 9008 

#include "GUIDialogBoxeeRate.h"
#include "GUIDialogBoxeeShare.h"
#include "GUIDialogBoxeeCtx.h"
#include "BoxeeUtils.h"
#ifdef HAS_LASTFM
#include "LastFmManager.h"
#endif

CGUIWindowBoxeeAlbumInfo::CGUIWindowBoxeeAlbumInfo(void) :
  CGUIWindow(WINDOW_BOXEE_ALBUM_INFO, "boxee_album_info.xml"), m_albumItem(new CFileItem) 
{
  m_albumSongs = new CFileItemList;
  m_iFirstTrack = 1;
  m_bCanResolve = false;
}

CGUIWindowBoxeeAlbumInfo::~CGUIWindowBoxeeAlbumInfo(void) {
  delete m_albumSongs;
}

bool CGUIWindowBoxeeAlbumInfo::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowBoxeeAlbumInfo::OnMessage(CGUIMessage& message) {
  switch (message.GetMessage()) {
  case GUI_MSG_PLAYLISTPLAYER_CHANGED: {
    // Highlight a currently playing song in the list
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    OnMessage(msg);

    int iCurrentItem = g_playlistPlayer.GetCurrentSong();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, iCurrentItem);
    OnMessage(msg2);

    break;
  }
  case GUI_MSG_WINDOW_DEINIT: 
  {
    // nothing to do here
  }
  break;
  case GUI_MSG_WINDOW_INIT: 
  {
    CGUIWindow::OnMessage(message);

    // Other that this initialization
    // the review mode switching is handled exclusively through the skin

    int iCurrentItem = g_playlistPlayer.GetCurrentSong();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, iCurrentItem);
    OnMessage(msg2);

    if (m_bViewReview) 
    {
      SET_CONTROL_VISIBLE(100);
      SET_CONTROL_HIDDEN(50);
    } 
  
    Update();

    return true;
  }
  break;

  case GUI_MSG_CLICKED: {
    int iControl = message.GetSenderId();

    if (iControl == BTN_RATE) 
    {
      bool bLike;
      if (CGUIDialogBoxeeRate::ShowAndGetInput(bLike)) 
      {
        BoxeeUtils::Rate(m_albumItem.get(), bLike);
        g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_STAR, "", g_localizeStrings.Get(51034), 5000 , KAI_YELLOW_COLOR, KAI_GREY_COLOR);
      }
    } 
    else if (iControl == BTN_RECOMMEND) 
    {
      CGUIDialogBoxeeShare *pFriends = (CGUIDialogBoxeeShare *) g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);
      pFriends->DoModal();
    } 
    else if (iControl == BTN_PLAY) 
    {
      if(m_albumItem && m_albumItem.get() && m_albumSongs && m_albumSongs->Size())
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::OnMessage - In BTN_PLAY -> Going to call HandlePlayForAlbumItem (aip)");
        HandlePlayForAlbumItem();
      }
      else
      {
        if(m_albumItem && m_albumSongs)
        {
          CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::OnMessage - In BTN_PLAY. [m_albumItem=%p][m_albumSongsSize=%d] -> Not handling item (aip)",m_albumItem.get(),m_albumSongs->Size());
        }
        
        g_windowManager.PreviousWindow();    
      }
    } 
#ifdef HAS_LASTFM
    else if (iControl == BTN_LASTFM) 
    {
      if (m_albumItem->HasMusicInfoTag()) 
      {
        CMusicInfoTag* pInfoTag = m_albumItem->GetMusicInfoTag();
        CStdString strArtist = pInfoTag->GetArtist();
        CUtil::URLEncode(strArtist);
        CStdString strLink;
        strLink.Format("lastfm://artist/%s/similarartists", strArtist.c_str());
        CURI url(strLink);
        CLastFmManager::GetInstance()->ChangeStation(url);
      }
    } 
#endif
    else if (iControl == CONTROL_LIST) 
    {
      CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_LIST);
      g_windowManager.SendMessage(msg);
      int iItem = msg.GetParam1();
      CLog::Log(LOGDEBUG,
          "CGUIWindowBoxeeAlbumInfo::OnMessage, MUSICINFO, selected item = %d",
          iItem);

      int iAction = message.GetParam1();

      if (ACTION_SELECT_ITEM == iAction) 
      {
        CFileItemList queuedItems;
        queuedItems.Append(*m_albumSongs);
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Play(iItem);
      }
    } 
    else if (iControl == BTN_MANUAL) 
    {
      
      //CGUIDialogBoxeeManualResolve::Show(m_albumItem.get());
    } 
    else if (iControl == CONTROL_BTN_IMAGE) 
    {
      // Change album art
      OnGetThumb();
    }
  }
  break;
  }

  return CGUIWindow::OnMessage(message);
}

void CGUIWindowBoxeeAlbumInfo::SetAlbumItem(CFileItemPtr albumItem) {
  m_albumItem = albumItem;
}

void CGUIWindowBoxeeAlbumInfo::SetSongs(const CFileItemList& songs) {
  m_albumSongs->Clear();
  m_albumSongs->Append(songs);
}

void CGUIWindowBoxeeAlbumInfo::Update() {

  CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0,
      m_albumSongs);
  OnMessage(message);

  if (m_bViewReview) {
    SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
    SET_CONTROL_HIDDEN(CONTROL_LIST);

    // Set the album review  (TODO: Move to skin?)   
    SetLabel(CONTROL_TEXTAREA, m_albumItem->GetMusicInfoTag()->GetComment());
  } else {
    SET_CONTROL_VISIBLE(CONTROL_LIST);
    SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
  }

#ifdef HAS_LASTFM
  // Disable the last.fm button in case of a missing artist 
  // TODO: Probably should be disabled for compilations as well
  if (!m_albumItem->GetMusicInfoTag()->GetArtist().IsEmpty()
      && CLastFmManager::GetInstance()->IsLastFmEnabled()) {
    SET_CONTROL_VISIBLE(CONTROL_BTN_LASTFM);
  } else {
    SET_CONTROL_HIDDEN(CONTROL_BTN_LASTFM);
  }
#endif
  
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    pImageControl->SetFileName(m_albumItem->GetThumbnailImage());

  }

}

void CGUIWindowBoxeeAlbumInfo::SetLabel(int iControl,  const CStdString& strLabel) 
{
  if (strLabel.IsEmpty()) 
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? 414 : 416);
  } 
  else 
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

/*
void CGUIWindowBoxeeAlbumInfo::RefreshThumb() {
  CStdString thumbImage = m_albumItem->GetThumbnailImage();
  if (!m_albumItem->HasThumbnail()) {

    thumbImage = CUtil::GetCachedAlbumThumb(m_albumItem->GetMusicInfoTag()->GetAlbum(), m_albumItem->GetMusicInfoTag()->GetArtist());
  }

  if (!CFile::Exists(thumbImage)) {
    DownloadThumbnail(thumbImage);
    m_hasUpdatedThumb = true;
  }

  if (!CFile::Exists(thumbImage))
    thumbImage.Empty();

  m_albumItem->SetThumbnailImage(thumbImage);
}
*/

/*
int CGUIWindowBoxeeAlbumInfo::DownloadThumbnail(const CStdString &thumbFile,  bool bMultiple) {
  // Download image and save as thumbFile

  if (m_albumItem->GetThumbnailImage().size() == 0)
    return 0;

  int iResult = 0;
  
  CStdString strThumb = thumbFile;

  CScraperUrl::SUrlEntry entry;
  entry.m_url = m_albumItem->GetThumbnailImage();
  
  if (CScraperUrl::DownloadThumbnail(strThumb, entry))
  {
    iResult = 1;
  }
  
  return iResult;

}
*/

void CGUIWindowBoxeeAlbumInfo::OnInitWindow() {

  CGUIWindow::OnInitWindow();

  if (!m_albumItem)
    return;

  if ((m_albumItem->IsHD() || m_albumItem->IsSmb())
      && !m_albumItem->m_bIsFolder && !m_albumItem->m_bIsShareOrDrive
      && !m_albumItem->m_strPath.IsEmpty() && !m_albumItem->GetPropertyBOOL(
          "MetaDataExtracted") && g_application.IsPathAvailable(
              m_albumItem->m_strPath))
    CDVDFileInfo::GetFileMetaData(m_albumItem->m_strPath, m_albumItem.get());

  CLog::Log(LOGDEBUG,
      "CGUIWindowBoxeeAlbumInfo::OnInitWindow, dumping album item");
  m_albumItem->Dump();

  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), INFO_HIDDEN_LIST);
  OnMessage(msg);

  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), INFO_HIDDEN_LIST, 0, 0,
      m_albumItem);
  OnMessage(winmsg);

  // If the item can not be manually resolved hide the button
  if (!m_bCanResolve) {
    SET_CONTROL_HIDDEN(BTN_MANUAL);
  }

  if (m_albumSongs->Size() == 0) {
    SET_CONTROL_HIDDEN(BTN_PLAY);
    SET_CONTROL_HIDDEN(BTN_MOREINFO);
  }
}

// Get Thumb from user choice.
// Options are:
// 1.  Current thumb
// 2.  AllMusic.com thumb
// 3.  Local thumb
// 4.  No thumb (if no Local thumb is available)

// TODO: Currently no support for "embedded thumb" as there is no easy way to grab it
//       without sending a file that has this as it's album to this class
void CGUIWindowBoxeeAlbumInfo::OnGetThumb() {
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_albumItem->GetThumbnailImage())) {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_albumItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  CScraperUrl::SUrlEntry entry;
  entry.m_url = m_albumItem->GetProperty("OriginalThumb");
	
  CStdString strThumb = "thumb://Remote";
  CFileItemPtr remoteItem(new CFileItem(strThumb, false));
  remoteItem->GetVideoInfoTag()->m_strPictureURL.m_url.push_back(entry);
  remoteItem->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
  remoteItem->SetIconImage("defaultPicture.png");
  remoteItem->SetLabel(g_localizeStrings.Get(415));
  remoteItem->SetProperty("labelonthumbload", g_localizeStrings.Get(20055));
  items.Add(remoteItem);

  // local thumb
  CStdString cachedLocalThumb;

  CStdString localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb)) {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    if (CPicture::CreateThumbnail(localThumb, cachedLocalThumb)) {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetThumbnailImage(cachedLocalThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }

  CFileItemPtr item(new CFileItem("thumb://None", false));
  item->SetThumbnailImage("defaultmusicalbumBig.png");
  item->SetLabel(g_localizeStrings.Get(20018));
  items.Add(item);

  CStdString result;
  // Open dialog and present user with thumb options
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_musicSources, g_localizeStrings.Get(1030), result))
    return; // user cancelled

  if (result == "thumb://Current")
    return; // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb;

  //cachedThumb = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);
  
  CStdString albumPath = m_albumItem->m_strPath;
  cachedThumb = CUtil::GetCachedMusicThumb(albumPath);
  
  if (CFile::Exists(cachedThumb))
     CFile::Delete(cachedThumb);

  if (result.Left(14).Equals("thumb://Remote")) 
  {
    CStdString strFile;
    CFileItem chosen(result, false);
    CStdString thumb = chosen.GetCachedPictureThumb();
    if (CFile::Exists(thumb)) {
      // NOTE: This could fail if the thumbloader was too slow and the user too impatient
      CFile::Cache(thumb, cachedThumb);
    } else
      result = "thumb://None";
  }
  if (result == "thumb://None") 
  { // cache the default thumb
    CFile::Delete(cachedThumb);
    cachedThumb = "";
  } 
  else if (result == "thumb://Local")
  {
    CFile::Cache(cachedLocalThumb, cachedThumb);
  }
  else if (CFile::Exists(result)) 
  {
    CPicture::CreateThumbnail(result, cachedThumb);
  }

  m_albumItem->SetThumbnailImage(cachedThumb);

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}


void CGUIWindowBoxeeAlbumInfo::AddItems(const CFileItemList& items) {
  for (int i = 0; i < items.Size(); i++) {
    CFileItemPtr newItem(new CFileItem(*items[i]));
    m_albumSongs->Add(newItem);
  }
}

void CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem()
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem - Enter function. [MusicPlayListSize=%d] (aip)",g_playlistPlayer.GetPlaylist(PLAYLIST_MUSIC).size());
  
  if(g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying())
  {
    if(g_application.m_pPlayer)
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem - [IsPlaying=%d][IsPaused=%d] (aip)",g_application.m_pPlayer->IsPlaying(),g_application.m_pPlayer->IsPaused());
    }
    
    bool dlgWasCanceled;
    if (CGUIDialogYesNo2::ShowAndGetInput(0,51964,-1,-1,dlgWasCanceled,0,0))
    {
      if(g_application.m_pPlayer)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem - Return from CGUIDialogYesNo2::ShowAndGetInput [dlgWasCanceled=%d]. [IsPaused=%d] and click RESTART -> Going to RESTART (aip)",dlgWasCanceled,g_application.m_pPlayer->IsPaused());
      }

      // Restart
      
      g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(m_albumItem.get()));

      g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
      g_playlistPlayer.Reset();
      g_playlistPlayer.Add(PLAYLIST_MUSIC, *m_albumSongs);
      g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
      g_playlistPlayer.Play();

      CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, 0);
      OnMessage(msg2);

      SET_CONTROL_FOCUS(CONTROL_LIST, 1);
    }
    else
    {
      if(g_application.m_pPlayer && g_application.m_pPlayer->IsPaused())
      {
        // In case of PAUSE -> We want to RESUME the video

        CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem - Return from CGUIDialogYesNo2::ShowAndGetInput [dlgWasCanceled=%d]. [IsPaused=%d] and click RESUME -> Going to RESUME (aip)",dlgWasCanceled,g_application.m_pPlayer->IsPaused());
        
        g_application.m_pPlayer->Pause();
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeAlbumInfo::HandlePlayForAlbumItem - [g_application.m_pPlayer=NULL] or [g_application.m_pPlayer.IsPlaying=FALSE] -> Going to RESTART (aip)");
    
    // Restart
    
    g_application.GetBoxeeItemsHistoryList().AddItemToHistory(*(m_albumItem.get()));

    g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Reset();
    g_playlistPlayer.Add(PLAYLIST_MUSIC, *m_albumSongs);
    g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
    g_playlistPlayer.Play();

    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, 0);
    OnMessage(msg2);

    SET_CONTROL_FOCUS(CONTROL_LIST, 1);    
  }
}

void CGUIWindowBoxeeAlbumInfo::Show(CFileItemPtr pAlbumItem, bool bReviewMode, int iStartFromTrack)
{  
  CGUIWindowBoxeeAlbumInfo* pWindow = (CGUIWindowBoxeeAlbumInfo*)g_windowManager.GetWindow(WINDOW_BOXEE_ALBUM_INFO);
  if(!pWindow)
  {
    return;
  }

  pWindow->SetAlbumItem(pAlbumItem);
  
  std::vector<BOXEE::BXMetadata*> vecSongs;
  CFileItemList songs;

  // Get all songs from the album
  if (BOXEE::Boxee::GetInstance().GetMetadataEngine().GetSongsFromAlbum(pAlbumItem->GetPropertyInt("BoxeeDBAlbumId"), vecSongs) == MEDIA_DATABASE_OK) 
  {
    for (size_t i = 0; i < vecSongs.size(); i++) 
    {
      BOXEE::BXAudio* pAudio = (BOXEE::BXAudio*)vecSongs[i]->GetDetail(MEDIA_DETAIL_AUDIO);
      
      CSong song;
      BoxeeUtils::ConvertBXAudioToCSong(pAudio, song);
      song.strArtist = pAlbumItem->GetMusicInfoTag()->GetArtist();
      song.strComment = pAlbumItem->GetMusicInfoTag()->GetComment();
      
      CFileItemPtr item(new CFileItem(song));

      // Set all the relevant properties in order to allow the album to be presented correctly in the Info dialog
      item->SetProperty("albumTrack", true);
      item->SetProperty("albumPath", pAlbumItem->m_strPath);
      item->SetProperty("OriginalThumb", pAlbumItem->GetProperty("OriginalThumb"));
      item->SetProperty("isloaded", 1);
      item->SetProperty("BoxeeDBAlbumId", pAlbumItem->GetPropertyInt("BoxeeDBAlbumId"));
      
      songs.Add(item);
      
      
      delete vecSongs[i];
    }
    
    pWindow->SetSongs(songs);
  }

  vecSongs.clear();

  CFileItemList tracks;
  // If album item has path to the directory with tracks, add all unrecognized tracks
  CStdString strAlbumDirectory = pAlbumItem->GetProperty("AlbumFolderPath");
  if (strAlbumDirectory != "" && strAlbumDirectory.Left(7) != "feed://") 
  {
    CFileItemList items;
    if (DIRECTORY::CDirectory::GetDirectory(strAlbumDirectory, items)) 
    {
      // Sort the retreived files by filename
      items.Sort(SORT_METHOD_FILE, SORT_ORDER_ASC);

      // Go over all files in the directory and add those that do not appear in the list of songs
      for (int i = 0; i < items.Size(); i++) 
      {
        CFileItemPtr pItem = items[i];
        if (pItem->IsAudio()) 
        {
          bool bShouldAdd = true;
          for ( int j = 0; j < songs.Size(); j++) 
          {
            if (songs[j]->m_strPath == pItem->m_strPath) 
            {
              // song already exists in the list of recognized tracks
              bShouldAdd = false;
              break;
            }
          }

          if (bShouldAdd) {
            pItem->SetProperty("albumTrack", true);
            tracks.Add(pItem);
          }
        }
      }
    }
  }
  pWindow->AddItems(tracks);
  
  // Set whether the dialog should be opened in review mode or tracks mode
  pWindow->SetReviewMode(bReviewMode);
  // Set the first track that should be played
  pWindow->SetFirstTrack(iStartFromTrack);
  
  g_windowManager.ActivateWindow(WINDOW_BOXEE_ALBUM_INFO);
  
  g_windowManager.CloseDialogs(true);

   // Start playing the first track
  g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
  g_playlistPlayer.Reset();
  
  g_playlistPlayer.Add(PLAYLIST_MUSIC, *pWindow->m_albumSongs);
  g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);

  if (!bReviewMode)
    g_playlistPlayer.Play(iStartFromTrack);
}

