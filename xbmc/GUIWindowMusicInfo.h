#pragma once

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

#include "GUIDialog.h"
#include "Song.h"
#include "Artist.h"
#include "Album.h"

class CFileItem;
class CFileItemList;

class CGUIWindowMusicInfo :
      public CGUIDialog
{
public:
  CGUIWindowMusicInfo(void);
  virtual ~CGUIWindowMusicInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);
  virtual void Render();
  void SetAlbum(const CAlbum& album, const CStdString &path);
  void SetArtist(const CArtist& artist, const CStdString &path);
  void SetSongs(const VECSONGS &songs);
  void SetItems(const CFileItemList& items);
  void AddItems(const CFileItemList& items);
  void SetAlbumId(int iAlbumId);

  bool NeedRefresh() const;
  bool HasUpdatedThumb() const { return m_hasUpdatedThumb; };
  void RefreshThumb();
  
  void SetReviewMode(bool bReviewMode) { m_bViewReview = bReviewMode; }
  void SetFirstTrack(int iTrackId) { m_iFirstTrack = iTrackId; }
  void SetManualResolve(bool bCanResolve) { m_bCanResolve = bCanResolve; }

  virtual bool HasListItems() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0);
  const CFileItemList& CurrentDirectory() const { return *m_albumSongs; };
  
  void SetExternalThumbnail(const CStdString& strThumbnail) {m_strExternalThumbnail = strThumbnail;}
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);
  int DownloadThumbnail(const CStdString &thumbFile, bool bMultiple=false);
  void OnGetThumb();
  void OnGetFanart();
  void SetDiscography();
  void OnSearch(const CFileItem* pItem);

  CAlbum m_album;
  CArtist m_artist;
  bool m_bViewReview;
  int m_iFirstTrack;
  bool m_bCanResolve;
  bool m_bRefresh;
  bool m_hasUpdatedThumb;
  bool m_bArtistInfo;
  CFileItemPtr   m_albumItem;
  CFileItemList* m_albumSongs;
  CStdString m_strExternalThumbnail;
  int m_iAlbumId;
};
