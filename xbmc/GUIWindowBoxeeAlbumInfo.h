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

#include "GUIWindow.h"
#include "Song.h"
#include "Artist.h"
#include "Album.h"

class CFileItem;
class CFileItemList;

class CGUIWindowBoxeeAlbumInfo: public CGUIWindow
{
public:
  CGUIWindowBoxeeAlbumInfo(void);
  virtual ~CGUIWindowBoxeeAlbumInfo(void);
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction &action);

  void SetAlbumItem(CFileItemPtr albumItem);

  void SetSongs(const CFileItemList& songs);

  //void SetItems(const CFileItemList& items);
  void AddItems(const CFileItemList& items);

  //void RefreshThumb();

  void SetReviewMode(bool bReviewMode) {
    m_bViewReview = bReviewMode;
  }
  void SetFirstTrack(int iTrackId) {
    m_iFirstTrack = iTrackId;
  }
  void SetManualResolve(bool bCanResolve) {
    m_bCanResolve = bCanResolve;
  }

  static void Show(CFileItemPtr pAlbumItem, bool bReviewMode, int iStartFromTrack = 1);

  //void SetExternalThumbnail(const CStdString& strThumbnail) {m_strExternalThumbnail = strThumbnail;}
protected:
  virtual void OnInitWindow();
  void Update();
  void SetLabel(int iControl, const CStdString& strLabel);

  //int DownloadThumbnail(const CStdString &thumbFile, bool bMultiple=false);

  void OnGetThumb();

  void HandlePlayForAlbumItem();

  bool m_bViewReview;
  int m_iFirstTrack;
  bool m_bCanResolve;

  CFileItemPtr m_albumItem;
  CFileItemList* m_albumSongs;

  int m_iAlbumId;
};
