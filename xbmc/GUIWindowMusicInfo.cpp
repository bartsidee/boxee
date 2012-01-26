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

#include "GUIWindowMusicInfo.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "GUIImage.h"
#include "Picture.h"
#include "GUIDialogFileBrowser.h"
#include "GUIPassword.h"
#include "MusicDatabase.h"
#include "LastFmManager.h"
#include "MusicInfoTag.h"
#include "URL.h"
#include "FileSystem/File.h"
#include "FileItem.h"
#include "Application.h"
#include "GUIWindowManager.h"
#include "cores/dvdplayer/DVDPlayer.h"
#include "cores/dvdplayer/DVDFileInfo.h"
#include "PlayListPlayer.h"
//#include "GUIDialogBoxeeManualResolve.h"
#include "VideoInfoTag.h"
#include "GUIUserMessages.h"

#include "MediaManager.h"
#include "FileSystem/Directory.h"
#include "utils/AsyncFileCopy.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "GUISettings.h"
#include "LocalizeStrings.h"
#include "utils/log.h"

using namespace XFILE;
using namespace PLAYLIST;

#define CONTROL_ALBUM         20
#define CONTROL_ARTIST        21
#define CONTROL_DATE          22
#define CONTROL_RATING        23
#define CONTROL_GENRE         24
#define CONTROL_MOODS         25
#define CONTROL_STYLES        26

#define CONTROL_IMAGE          3
//#define CONTROL_TEXTAREA       4

#define CONTROL_BTN_TRACKS     5
#define CONTROL_BTN_REFRESH    6

#define CONTROL_BTN_GET_THUMB 10
#define CONTROL_BTN_LASTFM    11

#define CONTROL_LIST          50
#define CONTROL_TEXTAREA      100

//Boxee
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
#include "LastFmManager.h"
//end Boxee

CGUIWindowMusicInfo::CGUIWindowMusicInfo(void)
    : CGUIDialog(WINDOW_MUSIC_INFO, "DialogAlbumInfo.xml")
    , m_albumItem(new CFileItem)
{
  m_bRefresh = false;
  m_albumSongs = new CFileItemList;
  m_iFirstTrack = 1;
  m_bCanResolve = false;
}

CGUIWindowMusicInfo::~CGUIWindowMusicInfo(void)
{
  delete m_albumSongs;
}

bool CGUIWindowMusicInfo::OnAction(const CAction &action)
{
  if(action.id == ACTION_PREVIOUS_MENU)
  {
    // We want to send GUI_MSG_UPDATE to the previous window in order to update the HistoryList
    int activeWindowId = g_windowManager.GetActiveWindow();
    CGUIMessage message(GUI_MSG_UPDATE, activeWindowId, 0);
    g_windowManager.SendMessage(message);
    
    Close();
    return true;    
  }
  
  return CGUIDialog::OnAction(action);
}

bool CGUIWindowMusicInfo::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_PLAYLISTPLAYER_CHANGED:
  {
    CGUIMessage msg(GUI_MSG_NOTIFY_ALL, GetID(), 0, GUI_MSG_REFRESH_LIST);
    OnMessage(msg);
    
    int iCurrentItem = g_playlistPlayer.GetCurrentSong();
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, iCurrentItem);
    OnMessage(msg2);
    
    break;
  }
  case GUI_MSG_WINDOW_DEINIT:
    {
      CGUIMessage message(GUI_MSG_LABEL_RESET, GetID(), CONTROL_LIST);
      OnMessage(message);
      m_albumSongs->Clear();
      m_iFirstTrack = 1;
    }
    break;

  case GUI_MSG_WINDOW_INIT:
    {
      CGUIDialog::OnMessage(message);
      m_bRefresh = false;
      if (g_guiSettings.GetBool("network.enableinternet"))
        RefreshThumb();
      
      // Other that this initialization
      // the review mode switching is handled exclusively through the skin
      
      if (m_bViewReview) {
        SET_CONTROL_VISIBLE(100);
        SET_CONTROL_HIDDEN(50);
      }
      else 
      {
        // Start playing the first track
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Add(PLAYLIST_MUSIC, *m_albumSongs);
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        CLog::Log(LOGDEBUG,"CGUIWindowMusicInfo::OnMessage, GUI_MSG_WINDOW_INIT, play first track = %d", m_iFirstTrack); 
        g_playlistPlayer.Play(m_iFirstTrack - 1);
  
        CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, m_iFirstTrack -1);
        OnMessage(msg2);
      }
      
      Update();
      
      return true;
    }
    break;


  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (iControl == CONTROL_BTN_REFRESH)
      {
        CUtil::ClearCache();

        m_bRefresh = true;
        Close();
        return true;
      }
      else if (iControl == CONTROL_BTN_GET_THUMB)
      {
        OnGetThumb();
      }
      else if (iControl == CONTROL_BTN_TRACKS)
      {
        m_bViewReview = !m_bViewReview;
        Update();
      }
      //Boxee
      else if (iControl == BTN_RATE)
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
        CGUIDialogBoxeeShare *pFriends = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);
        pFriends->DoModal();
      }
      else if (iControl == BTN_PLAY)
      {
        g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Reset();
        g_playlistPlayer.Add(PLAYLIST_MUSIC, *m_albumSongs);
        g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
        g_playlistPlayer.Play();

        CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_LIST, 0);
        OnMessage(msg2);

        SET_CONTROL_FOCUS(CONTROL_LIST, 1);
      }
#ifdef HAS_LASTFM
      else if (iControl == BTN_LASTFM)
      {
        if (m_albumItem->HasMusicInfoTag()) {
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
        CLog::Log(LOGDEBUG,"CGUIWindowMusicInfo::OnMessage, MUSICINFO, selected item = %d", iItem);
      
        int iAction = message.GetParam1();
        if (ACTION_SELECT_ITEM == iAction && m_bArtistInfo)
        {
          CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), iControl);
          g_windowManager.SendMessage(msg);
          int iItem = msg.GetParam1();
          if (iItem < 0 || iItem >= (int)m_albumSongs->Size())
            break;
          CFileItemPtr item = m_albumSongs->Get(iItem);
          OnSearch(item.get());
          return true;
        }
//Boxee
        else if (ACTION_SELECT_ITEM == iAction)
        {
          CFileItemList queuedItems;
          queuedItems.Append(*m_albumSongs);
          g_playlistPlayer.ClearPlaylist(PLAYLIST_MUSIC);
          g_playlistPlayer.Reset();
          g_playlistPlayer.Add(PLAYLIST_MUSIC, queuedItems);
          g_playlistPlayer.SetCurrentPlaylist(PLAYLIST_MUSIC);
          g_playlistPlayer.Play(iItem);
        }
//end Boxee
      }
#ifdef HAS_LASTFM
      else if (iControl == CONTROL_BTN_LASTFM)
      {
        CStdString strArtist = m_album.strArtist;
        CUtil::URLEncode(strArtist);
        CStdString strLink;
        strLink.Format("lastfm://artist/%s/similarartists", strArtist.c_str());
        CURI url(strLink);
        CLastFmManager::GetInstance()->ChangeStation(url);
      }
#endif
      else if (iControl == BTN_MANUAL)
      {
//        CLog::Log(LOGDEBUG,"CGUIWindowMusicInfo::OnMessage, MANUAL, resolve item manually, path = %s", m_albumItem->m_strPath.c_str());
//        m_albumItem->SetProperty("BoxeeDBalbumId", m_iAlbumId);
//        CGUIDialogBoxeeManualResolve::Show(m_albumItem.get());
      }
    }
    break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIWindowMusicInfo::SetAlbum(const CAlbum& album, const CStdString &path)
{
  m_album = album;
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbum(m_album.strAlbum);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_album.strArtist);
  m_albumItem->GetMusicInfoTag()->SetYear(m_album.iYear);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetRating('0' + (m_album.iRating + 1) / 2);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_album.strGenre);
  m_albumItem->GetMusicInfoTag()->SetComment(m_album.strReview);
  m_albumItem->SetProperty("albumstyles", m_album.strStyles);
  m_albumItem->SetProperty("albummoods", m_album.strMoods);
  m_albumItem->SetProperty("albumthemes", m_album.strThemes);
  m_albumItem->SetProperty("albumreview", m_album.strReview);
  m_albumItem->SetProperty("albumlabel", m_album.strLabel);
  m_albumItem->SetProperty("albumtype", m_album.strType);
  
  if (m_album.thumbURL.m_url.size() > 0)
  {
    m_albumItem->SetProperty("OriginalThumb", m_album.thumbURL.m_url[0].m_url);
  }
  m_albumItem->SetMusicThumb();
  SetSongs(m_album.songs);
  // set the artist thumb
  CFileItem artist(m_album.strArtist);
  artist.SetCachedArtistThumb();
  if (CFile::Exists(artist.GetThumbnailImage()))
    m_albumItem->SetProperty("artistthumb", artist.GetThumbnailImage());
  m_hasUpdatedThumb = false;
  m_bArtistInfo = false;
  m_albumSongs->SetContent("albums");
}

void CGUIWindowMusicInfo::SetArtist(const CArtist& artist, const CStdString &path)
{
  m_artist = artist;
  SetDiscography();
  *m_albumItem = CFileItem(path, true);
  m_albumItem->SetLabel(artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetAlbumArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetArtist(m_artist.strArtist);
  m_albumItem->GetMusicInfoTag()->SetLoaded(true);
  m_albumItem->GetMusicInfoTag()->SetGenre(m_artist.strGenre);
  m_albumItem->SetProperty("styles", m_artist.strStyles);
  m_albumItem->SetProperty("moods", m_artist.strMoods);
  m_albumItem->SetProperty("biography", m_artist.strBiography);
  m_albumItem->SetProperty("instruments", m_artist.strInstruments);
  m_albumItem->SetProperty("born", m_artist.strBorn);
  m_albumItem->SetProperty("formed", m_artist.strFormed);
  m_albumItem->SetProperty("died", m_artist.strDied);
  m_albumItem->SetProperty("disbanded", m_artist.strDisbanded);
  m_albumItem->SetProperty("yearsactive", m_artist.strYearsActive);
  m_albumItem->SetCachedArtistThumb();
  m_hasUpdatedThumb = false;
  m_bArtistInfo = true;
  m_albumSongs->SetContent("artists");
}

void CGUIWindowMusicInfo::SetSongs(const VECSONGS &songs)
{
  m_albumSongs->Clear();
  for (unsigned int i = 0; i < songs.size(); i++)
  {
    const CSong& song = songs[i];
    CFileItemPtr item(new CFileItem(song));
    
    // Set all the relevant properties in order to allow the album to be presented correctly in the Info dialog
    item->SetProperty("albumPath", m_albumItem->m_strPath);
    item->SetProperty("OriginalThumb", m_albumItem->GetProperty("OriginalThumb"));
    item->SetProperty("isloaded", 1);
    
    CLog::Log(LOGDEBUG, "CGUIWindowMusicInfo::SetSongs, SONG, title = %s, album = %s, artist = %s", 
        item->GetMusicInfoTag()->GetTitle().c_str(), item->GetMusicInfoTag()->GetAlbum().c_str(), item->GetMusicInfoTag()->GetArtist().c_str());

    m_albumSongs->Add(item);
  }
}

void CGUIWindowMusicInfo::SetDiscography()
{
  m_albumSongs->Clear();
  CMusicDatabase database;
  database.Open();

  for (unsigned int i=0;i<m_artist.discography.size();++i)
  {
    CFileItemPtr item(new CFileItem(m_artist.discography[i].first));
    item->SetLabel2(m_artist.discography[i].second);
    long idAlbum = database.GetAlbumByName(item->GetLabel(),m_artist.strArtist);
    CStdString strThumb;
    if (idAlbum != -1) // we need this slight stupidity to get correct case for the album name
      database.GetAlbumThumb(idAlbum,strThumb);

    if (!strThumb.IsEmpty() && CFile::Exists(strThumb))
      item->SetThumbnailImage(strThumb);
    else
      item->SetThumbnailImage("DefaultAlbumCover.png");

    m_albumSongs->Add(item);
  }
}

void CGUIWindowMusicInfo::Update()
{
  if (m_bArtistInfo)
  {
    SetLabel(CONTROL_ARTIST, m_artist.strArtist );
    SetLabel(CONTROL_GENRE, m_artist.strGenre);
    SetLabel(CONTROL_MOODS, m_artist.strMoods);
    SetLabel(CONTROL_STYLES, m_artist.strStyles );
    if (m_bViewReview)
    {
      SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
      SET_CONTROL_HIDDEN(CONTROL_LIST);
      SetLabel(CONTROL_TEXTAREA, m_artist.strBiography);
      //SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21888);
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_LIST);
      if (GetControl(CONTROL_LIST))
      {
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
        CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
        OnMessage(message);
      }
      else
        CLog::Log(LOGERROR, "Out of date skin - needs list with id %i", CONTROL_LIST);
      //SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 21887);
    }
  }
  else
  {
    SetLabel(CONTROL_ALBUM, m_album.strAlbum );
    SetLabel(CONTROL_ARTIST, m_album.strArtist );
    CStdString date; date.Format("%d", m_album.iYear);
    SetLabel(CONTROL_DATE, date );

    CStdString strRating;
    if (m_album.iRating > 0)
      strRating.Format("%i/9", m_album.iRating);
    SetLabel(CONTROL_RATING, strRating );

    SetLabel(CONTROL_GENRE, m_album.strGenre);
    SetLabel(CONTROL_MOODS, m_album.strMoods);
    SetLabel(CONTROL_STYLES, m_album.strStyles );
    
    CGUIMessage message(GUI_MSG_LABEL_BIND, GetID(), CONTROL_LIST, 0, 0, m_albumSongs);
    OnMessage(message);

    if (m_bViewReview)
    {
      SET_CONTROL_VISIBLE(CONTROL_TEXTAREA);
      SET_CONTROL_HIDDEN(CONTROL_LIST);
      SetLabel(CONTROL_TEXTAREA, m_album.strReview);
      //SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 182);
    }
    else
    {
      SET_CONTROL_VISIBLE(CONTROL_LIST);
      if (GetControl(CONTROL_LIST))
      {
        SET_CONTROL_HIDDEN(CONTROL_TEXTAREA);
      }
      else
        CLog::Log(LOGERROR, "Out of date skin - needs list with id %i", CONTROL_LIST);
      //SET_CONTROL_LABEL(CONTROL_BTN_TRACKS, 183);
    }
  }
  // update the thumbnail
  const CGUIControl* pControl = GetControl(CONTROL_IMAGE);
  if (pControl)
  {
    CGUIImage* pImageControl = (CGUIImage*)pControl;
    pImageControl->FreeResources();
    if (m_strExternalThumbnail != "") {
      pImageControl->SetFileName(m_strExternalThumbnail);
    }
    else {
      pImageControl->SetFileName(m_albumItem->GetThumbnailImage());
    }
    
  }

  // disable the GetThumb button if the user isn't allowed it
  CONTROL_ENABLE_ON_CONDITION(CONTROL_BTN_GET_THUMB, g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].canWriteDatabases() || g_passwordManager.bMasterUser);

#ifdef HAS_LASTFM
  if (!m_album.strArtist.IsEmpty() && CLastFmManager::GetInstance()->IsLastFmEnabled())
  {
    SET_CONTROL_VISIBLE(CONTROL_BTN_LASTFM);
  }
  else
  {
    SET_CONTROL_HIDDEN(CONTROL_BTN_LASTFM);
  }
#endif
}

void CGUIWindowMusicInfo::SetLabel(int iControl, const CStdString& strLabel)
{
  if (strLabel.IsEmpty())
  {
    SET_CONTROL_LABEL(iControl, (iControl == CONTROL_TEXTAREA) ? (m_bArtistInfo?547:414) : 416);
  }
  else
  {
    SET_CONTROL_LABEL(iControl, strLabel);
  }
}

void CGUIWindowMusicInfo::Render()
{
  CGUIDialog::Render();
}


void CGUIWindowMusicInfo::RefreshThumb()
{
  CStdString thumbImage = m_albumItem->GetThumbnailImage();
  if (!m_albumItem->HasThumbnail())
  {
    if (m_bArtistInfo)
      thumbImage = m_albumItem->GetCachedArtistThumb();
    else
      thumbImage = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);
  }

  if (!CFile::Exists(thumbImage))
  {
    DownloadThumbnail(thumbImage);
    m_hasUpdatedThumb = true;
  }

  if (!CFile::Exists(thumbImage) )
    thumbImage.Empty();

  m_albumItem->SetThumbnailImage(thumbImage);
}

bool CGUIWindowMusicInfo::NeedRefresh() const
{
  return m_bRefresh;
}

int CGUIWindowMusicInfo::DownloadThumbnail(const CStdString &thumbFile, bool bMultiple)
{
  // Download image and save as thumbFile
  if (m_bArtistInfo)
  {
    if (m_artist.thumbURL.m_url.size() == 0)
      return 0;

    int iResult=0;
    int iMax = 1;
    if (bMultiple)
      iMax = INT_MAX;
    for (unsigned int i=0;i<m_artist.thumbURL.m_url.size()&&iResult<iMax;++i)
    {
      CStdString strThumb;

      if (bMultiple)
        strThumb.Format("%s%i.tbn",thumbFile.c_str(),i);
      else
        strThumb = thumbFile;
      if (CScraperUrl::DownloadThumbnail(strThumb,m_artist.thumbURL.m_url[i]))
        iResult++;
    }
    return iResult;
  }
  else
  {
    if (m_album.thumbURL.m_url.size() == 0)
      return 0;

    int iResult=0;
    int iMax = 1;
    if (bMultiple)
      iMax = INT_MAX;
    for (unsigned int i=0;i<m_album.thumbURL.m_url.size() && iResult<iMax;++i)
    {
      CStdString strThumb;
      if (bMultiple)
        strThumb.Format("%s%i.tbn",thumbFile.c_str(),i);
      else
        strThumb = thumbFile;
      if (CScraperUrl::DownloadThumbnail(strThumb,m_album.thumbURL.m_url[i]))
        iResult++;
    }
    return iResult;
  }
  return 0;
}

void CGUIWindowMusicInfo::OnInitWindow()
{

  CGUIDialog::OnInitWindow();

  if (! m_albumItem)
    return;

  if ((m_albumItem->IsHD() || m_albumItem->IsSmb()) && !m_albumItem->m_bIsFolder && !m_albumItem->m_bIsShareOrDrive && 
                    !m_albumItem->m_strPath.IsEmpty() && !m_albumItem->GetPropertyBOOL("MetaDataExtracted") && 
                    g_application.IsPathAvailable(m_albumItem->m_strPath))
        CDVDFileInfo::GetFileMetaData(m_albumItem->m_strPath, m_albumItem.get());

  CLog::Log(LOGDEBUG,"CGUIWindowMusicInfo::OnInitWindow, dumping album item");
  m_albumItem->Dump();

  CGUIMessage msg(GUI_MSG_LABEL_RESET, GetID(), INFO_HIDDEN_LIST);
  OnMessage(msg);

  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), INFO_HIDDEN_LIST, 0, 0, m_albumItem);
  OnMessage(winmsg);
  
  // If the item can not be manually resolved hide the button
  if (!m_bCanResolve)
  {
    SET_CONTROL_HIDDEN(BTN_MANUAL);
  }
  
  if (m_albumSongs->Size() == 0)
  {
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
void CGUIWindowMusicInfo::OnGetThumb()
{
  CFileItemList items;

  // Current thumb
  if (CFile::Exists(m_albumItem->GetThumbnailImage()))
  {
    CFileItemPtr item(new CFileItem("thumb://Current", false));
    item->SetThumbnailImage(m_albumItem->GetThumbnailImage());
    item->SetLabel(g_localizeStrings.Get(20016));
    items.Add(item);
  }

  // Grab the thumbnail(s) from the web
  CScraperUrl url;
  if (m_bArtistInfo)
    url = m_artist.thumbURL;
  else
    url = m_album.thumbURL;

  for (unsigned int i = 0; i < url.m_url.size(); i++)
  {
    CStdString strThumb;
    strThumb.Format("thumb://Remote%i",i);
    CFileItemPtr item(new CFileItem(strThumb, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetVideoInfoTag()->m_strPictureURL.m_url.push_back(url.m_url[i]);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20055));
    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  // local thumb
  CStdString cachedLocalThumb;
  CStdString localThumb;
  if (m_bArtistInfo)
  {
    CMusicDatabase database;
    database.Open();
    CStdString strArtistPath;
    database.GetArtistPath(m_artist.idArtist,strArtistPath);
    CUtil::AddFileToFolder(strArtistPath,"folder.jpg",localThumb);
  }
  else
    CStdString localThumb = m_albumItem->GetUserMusicThumb();
  if (CFile::Exists(localThumb))
  {
    CUtil::AddFileToFolder(g_advancedSettings.m_cachePath, "localthumb.jpg", cachedLocalThumb);
    if (CPicture::CreateThumbnail(localThumb, cachedLocalThumb))
    {
      CFileItemPtr item(new CFileItem("thumb://Local", false));
      item->SetThumbnailImage(cachedLocalThumb);
      item->SetLabel(g_localizeStrings.Get(20017));
      items.Add(item);
    }
  }
  
  CFileItemPtr item(new CFileItem("thumb://None", false));
  if (m_bArtistInfo)
    item->SetIconImage("DefaultArtist.png");
  else
    item->SetIconImage("DefaultAlbum.png");
  item->SetLabel(g_localizeStrings.Get(20018));
  items.Add(item);
  
  CStdString result;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, g_settings.m_musicSources, g_localizeStrings.Get(1030), result))
    return;   // user cancelled

  if (result == "thumb://Current")
    return;   // user chose the one they have

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  CStdString cachedThumb;
  if (m_bArtistInfo)
    cachedThumb = m_albumItem->GetCachedArtistThumb();
  else
    cachedThumb = CUtil::GetCachedAlbumThumb(m_album.strAlbum, m_album.strArtist);

  if (result.Left(14).Equals("thumb://Remote"))
  {
    CStdString strFile;
    CFileItem chosen(result, false);
    CStdString thumb = chosen.GetCachedPictureThumb();
    if (CFile::Exists(thumb))
    {
      // NOTE: This could fail if the thumbloader was too slow and the user too impatient
      CFile::Cache(thumb, cachedThumb);
    }
    else
      result = "thumb://None";
  }
  if (result == "thumb://None")
  { // cache the default thumb
    CFile::Delete(cachedThumb);
    cachedThumb = "";
  }
  else if (result == "thumb://Local")
    CFile::Cache(cachedLocalThumb, cachedThumb);
  else if (CFile::Exists(result))
    CPicture::CreateThumbnail(result, cachedThumb);

  m_albumItem->SetThumbnailImage(cachedThumb);
  m_hasUpdatedThumb = true;

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}

// Allow user to select a Fanart
void CGUIWindowMusicInfo::OnGetFanart()
{
  CFileItemList items;

  CFileItemPtr itemNone(new CFileItem("fanart://None", false));
  itemNone->SetIconImage("DefaultArtist.png");
  itemNone->SetLabel(g_localizeStrings.Get(20018));
  items.Add(itemNone);

  CStdString cachedThumb(itemNone->GetCachedThumb(m_artist.strArtist,g_settings.GetMusicFanartFolder()));
  if (CFile::Exists(cachedThumb))
  {
    CFileItemPtr itemCurrent(new CFileItem("fanart://Current",false));
    itemCurrent->SetThumbnailImage(cachedThumb);
    itemCurrent->SetLabel(g_localizeStrings.Get(20016));
    items.Add(itemCurrent);
  }

  CMusicDatabase database;
  database.Open();
  CStdString strArtistPath;
  database.GetArtistPath(m_artist.idArtist,strArtistPath);
  CFileItem item(strArtistPath,true);
  CStdString strLocal = item.CacheFanart(true);
  if (!strLocal.IsEmpty())
  {
    CFileItemPtr itemLocal(new CFileItem("fanart://Local",false));
    itemLocal->SetThumbnailImage(strLocal);
    itemLocal->SetLabel(g_localizeStrings.Get(20017));
    items.Add(itemLocal);
  }
  
  // Grab the thumbnails from the web
  CStdString strPath;
  CUtil::AddFileToFolder(g_advancedSettings.m_cachePath,"fanartthumbs",strPath);
  CUtil::WipeDir(strPath);
  DIRECTORY::CDirectory::Create(strPath);
  for (unsigned int i = 0; i < m_artist.fanart.GetNumFanarts(); i++)
  {
    CStdString strItemPath;
    strItemPath.Format("fanart://Remote%i",i);
    CFileItemPtr item(new CFileItem(strItemPath, false));
    item->SetThumbnailImage("http://this.is/a/thumb/from/the/web");
    item->SetIconImage("DefaultPicture.png");
    item->GetVideoInfoTag()->m_fanart = m_artist.fanart;
    item->SetProperty("fanart_number", (int)i);
    item->SetLabel(g_localizeStrings.Get(415));
    item->SetProperty("labelonthumbload", g_localizeStrings.Get(20015));

    // make sure any previously cached thumb is removed
    if (CFile::Exists(item->GetCachedPictureThumb()))
      CFile::Delete(item->GetCachedPictureThumb());
    items.Add(item);
  }

  CStdString result;
  VECSOURCES sources(g_settings.m_musicSources);
  g_mediaManager.GetLocalDrives(sources);
  bool flip=false;
  if (!CGUIDialogFileBrowser::ShowAndGetImage(items, sources, g_localizeStrings.Get(20019), result, &flip))
    return;   // user cancelled

  // delete the thumbnail if that's what the user wants, else overwrite with the
  // new thumbnail
  if (result.Equals("fanart://Current"))
   return;

  if (result.Equals("fanart://Local"))
    result = strLocal;

  if (CFile::Exists(cachedThumb))
    CFile::Delete(cachedThumb);

  if (!result.Equals("fanart://None"))
  { // local file
    if (result.Left(15)  == "fanart://Remote")
    {
      int iFanart = atoi(result.Mid(15).c_str());
      m_artist.fanart.SetPrimaryFanart(iFanart);
      // download the fullres fanart image
      CStdString tempFile = "special://temp/fanart_download.jpg";
      CAsyncFileCopy downloader;
      if (!downloader.Copy(m_artist.fanart.GetImageURL(), tempFile, g_localizeStrings.Get(13413)))
        return;
      result = tempFile;
    }

    if (flip)
      CPicture::ConvertFile(result, cachedThumb,0,1920,-1,100,true);
    else
      CPicture::CacheImage(result, cachedThumb);

    m_albumItem->SetProperty("fanart_image",cachedThumb);
    m_hasUpdatedThumb = true;
  }

  // tell our GUI to completely reload all controls (as some of them
  // are likely to have had this image in use so will need refreshing)
  CGUIMessage msg(GUI_MSG_NOTIFY_ALL, 0, 0, GUI_MSG_REFRESH_THUMBS);
  g_windowManager.SendMessage(msg);
  // Update our screen
  Update();
}

void CGUIWindowMusicInfo::OnSearch(const CFileItem* pItem)
{
  CMusicDatabase database;
  database.Open();
  long idAlbum = database.GetAlbumByName(pItem->GetLabel(),m_artist.strArtist);
  if (idAlbum != -1)
  {
    CAlbum album;
    CStdString strPath;

    if (database.GetAlbumInfo(idAlbum,album,&album.songs))
    {
      database.GetAlbumPath(idAlbum,strPath);
      SetAlbum(album,strPath);
      Update();
    }
  }
}

CFileItemPtr CGUIWindowMusicInfo::GetCurrentListItem(int offset)
{ 
  return m_albumItem; 
}

void CGUIWindowMusicInfo::AddItems(const CFileItemList& items)
{
        for ( int i = 0; i < items.Size(); i++)
        {
                CFileItemPtr newItem ( new CFileItem(*items[i]) );
                m_albumSongs->Add(newItem);
        }
}

void CGUIWindowMusicInfo::SetAlbumId(int iAlbumId)
{
  m_iAlbumId = iAlbumId;
}
