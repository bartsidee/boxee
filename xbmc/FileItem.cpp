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

#include "FileItem.h"
#include "LocalizeStrings.h"
#include "StringUtils.h"
#include "Util.h"
#include "Picture.h"
#include "PlayListFactory.h"
#include "Shortcut.h"
#include "Crc32.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/FileCurl.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/MusicDatabaseDirectory.h"
#include "FileSystem/VideoDatabaseDirectory.h"
#include "FileSystem/IDirectory.h"
#include "FileSystem/FactoryDirectory.h"
#include "MusicInfoTagLoaderFactory.h"
#include "CueDocument.h"
#include "utils/fstrcmp.h"
#include "VideoDatabase.h"
#include "MusicDatabase.h"
#include "SortFileItem.h"
#include "utils/TuxBoxUtil.h"
#include "VideoInfoTag.h"
#include "utils/SingleLock.h"
#include "MusicInfoTag.h"
#include "PictureInfoTag.h"
#include "Artist.h"
#include "Album.h"
#include "Song.h"
#include "URL.h"
#include "GUISettings.h"
#include "AdvancedSettings.h"
#include "Settings.h"
#include "utils/TimeUtils.h"
#include "Application.h"
#include "UPnPDirectory.h"
#include "utils/Variant.h"

//Boxee
#include "BoxeeUtils.h"
#include "GUIBoxeeViewState.h"
#include "boxee.h"
//end Boxee

using namespace std;
using namespace XFILE;
using namespace DIRECTORY;
using namespace PLAYLIST;
using namespace MUSIC_INFO;

CFileItem::CFileItem(const CSong& song)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(song.strTitle);
  m_strPath = song.strFileName;
  GetMusicInfoTag()->SetSong(song);
  m_lStartOffset = song.iStartOffset;
  m_lEndOffset = song.iEndOffset;
  m_strThumbnailImage = song.strThumb;
}

CFileItem::CFileItem(const CStdString &path, const CAlbum& album)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(album.strAlbum);
  m_strPath = path;
  m_bIsFolder = true;
  m_strLabel2 = album.strArtist;
  CUtil::AddSlashAtEnd(m_strPath);
  GetMusicInfoTag()->SetAlbum(album);
  if (album.thumbURL.m_url.size() > 0)
    m_strThumbnailImage = album.thumbURL.m_url[0].m_url;
  else
    m_strThumbnailImage.clear();
  m_bIsAlbum = true;

  SetProperty("description", album.strReview);
  SetProperty("theme", album.strThemes);
  SetProperty("mood", album.strMoods);
  SetProperty("style", album.strStyles);
  SetProperty("type", album.strType);
  SetProperty("label", album.strLabel);
  if (album.iRating > 0)
    SetProperty("rating", album.iRating);
}

CFileItem::CFileItem(const CVideoInfoTag& movie)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(movie.m_strTitle);
  if (movie.m_strFileNameAndPath.IsEmpty())
  {
    m_strPath = movie.m_strPath;
    CUtil::AddSlashAtEnd(m_strPath);
    m_bIsFolder = true;
  }
  else
  {
    m_strPath = movie.m_strFileNameAndPath;
    m_bIsFolder = false;
  }
  *GetVideoInfoTag() = movie;
  FillInDefaultIcon();
  SetCachedVideoThumb();
}

CFileItem::CFileItem(const CArtist& artist)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(artist.strArtist);
  m_strPath = artist.strArtist;
  m_bIsFolder = true;
  CUtil::AddSlashAtEnd(m_strPath);
  GetMusicInfoTag()->SetArtist(artist.strArtist);
}

CFileItem::CFileItem(const CGenre& genre)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(genre.strGenre);
  m_strPath = genre.strGenre;
  m_bIsFolder = true;
  CUtil::AddSlashAtEnd(m_strPath);
  GetMusicInfoTag()->SetGenre(genre.strGenre);
}

CFileItem::CFileItem(const CFileItem& item): CGUIListItem(), IArchivable(item), ISerializable(item)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  *this = item;
}

CFileItem::CFileItem(const CGUIListItem& item)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  // not particularly pretty, but it gets around the issue of Reset() defaulting
  // parameters in the CGUIListItem base class.
  *((CGUIListItem *)this) = item;
}

CFileItem::CFileItem(void)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
}

CFileItem::CFileItem(const CStdString& strLabel)
    : CGUIListItem()
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  SetLabel(strLabel);
}

CFileItem::CFileItem(const CStdString& strPath, bool bIsFolder)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  m_strPath = strPath;
  m_bIsFolder = bIsFolder;

  if (m_bIsFolder)
    CUtil::AddSlashAtEnd(m_strPath);
}

CFileItem::CFileItem(const CMediaSource& share)
{
  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
  Reset();
  m_bIsFolder = true;
  m_bIsShareOrDrive = true;
  m_strPath = share.strPath;
  CUtil::AddSlashAtEnd(m_strPath);
  CStdString label = share.strName;
  if (!share.strStatus.IsEmpty())
    label.Format("%s (%s)", share.strName.c_str(), share.strStatus.c_str());
  SetLabel(label);
  m_iLockMode = share.m_iLockMode;
  m_strLockCode = share.m_strLockCode;
  m_iHasLock = share.m_iHasLock;
  m_iBadPwdCount = share.m_iBadPwdCount;
  m_iDriveType = share.m_iDriveType;
  m_strThumbnailImage = share.m_strThumbnailImage;
  SetAdult(share.m_adult);
  SetCountryRestriction(share.m_country, share.m_countryAllow);
  SetLabelPreformated(true);
}

CFileItem::~CFileItem(void)
{
  if (m_musicInfoTag)
    delete m_musicInfoTag;
  if (m_videoInfoTag)
    delete m_videoInfoTag;
  if (m_pictureInfoTag)
    delete m_pictureInfoTag;

  m_musicInfoTag = NULL;
  m_videoInfoTag = NULL;
  m_pictureInfoTag = NULL;
}

const CFileItem& CFileItem::operator=(const CFileItem& item)
{
  if (this == &item) return * this;
  CGUIListItem::operator=(item);
  m_bLabelPreformated=item.m_bLabelPreformated;
  FreeMemory();
  m_strPath = item.m_strPath;
  m_bPlayableFolder = item.m_bPlayableFolder;
  m_bIsParentFolder = item.m_bIsParentFolder;
  m_iDriveType = item.m_iDriveType;
  m_bIsShareOrDrive = item.m_bIsShareOrDrive;
  m_dateTime = item.m_dateTime;
  m_dwSize = item.m_dwSize;
  if (item.HasMusicInfoTag())
  {
    m_musicInfoTag = GetMusicInfoTag();
    if (m_musicInfoTag)
      *m_musicInfoTag = *item.m_musicInfoTag;
  }
  else
  {
      delete m_musicInfoTag;
    m_musicInfoTag = NULL;
  }

  if (item.HasVideoInfoTag())
  {
    m_videoInfoTag = GetVideoInfoTag();
    if (m_videoInfoTag)
      *m_videoInfoTag = *item.m_videoInfoTag;
  }
  else
  {
      delete m_videoInfoTag;
    m_videoInfoTag = NULL;
  }

  if (item.HasPictureInfoTag())
  {
    m_pictureInfoTag = GetPictureInfoTag();
    if (m_pictureInfoTag)
      *m_pictureInfoTag = *item.m_pictureInfoTag;
  }
  else
  {
      delete m_pictureInfoTag;
    m_pictureInfoTag = NULL;
  }

  m_lStartOffset = item.m_lStartOffset;
  m_lEndOffset = item.m_lEndOffset;
  m_strDVDLabel = item.m_strDVDLabel;
  m_strTitle = item.m_strTitle;
  m_iprogramCount = item.m_iprogramCount;
  m_idepth = item.m_idepth;
  m_iLockMode = item.m_iLockMode;
  m_strLockCode = item.m_strLockCode;
  m_iHasLock = item.m_iHasLock;
  m_iBadPwdCount = item.m_iBadPwdCount;
  m_bCanQueue=item.m_bCanQueue;
  m_contenttype = item.m_contenttype;
  m_extrainfo = item.m_extrainfo;
  m_externalFileItem = item.GetExternalFileItem();
  
  if (item.HasLinksList())
  {
    if (!HasLinksList())
    {
      m_linksFileItemList = CFileItemPtr(new CFileItemList());
    }

    ((CFileItemList*)m_linksFileItemList.get())->Clear();
    ((CFileItemList*)m_linksFileItemList.get())->Copy(*(item.GetLinksList()));
  }
  else
  {
    if (HasLinksList())
    {
      m_linksFileItemList.reset();
    }
  }
  
  m_prevItem = item.m_prevItem;
  m_nextItem = item.m_nextItem;
  
  return *this;
}

CFileItemPtr CFileItem::GetPrevItem() const
{
  return m_prevItem; 
}

CFileItemPtr CFileItem::GetNextItem() const
{
  return m_nextItem; 
}

void         CFileItem::SetPrevItem(CFileItemPtr prev)
{
  m_prevItem = prev;
}

void CFileItem::ResetPrevItem()
{
  m_prevItem.reset();
}

void         CFileItem::SetNextItem(CFileItemPtr next)
{
  m_nextItem = next;
}

void CFileItem::ResetNextItem()
{
  m_nextItem.reset();
}

void CFileItem::ResetPrevAndNextItems()
{
  ResetPrevItem();
  ResetNextItem();
}

bool CFileItem::HasPath()
{
  if (!m_strPath.IsEmpty())
  {
    return true;
  }

  if (HasLinksList())
  {
    CFileItemList* list = (CFileItemList*)m_linksFileItemList.get();
    if (list->Size() > 0)
    {
      return true;
    }
  }

  return false;
}

void CFileItem::Reset()
{
  m_strLabel2.Empty();
  SetLabel("");
  m_bLabelPreformated=false;
  FreeIcons();
  m_overlayIcon = ICON_OVERLAY_NONE;
  m_bSelected = false;
  m_bIsAlbum = false;
  m_strDVDLabel.Empty();
  m_strTitle.Empty();
  m_strPath.Empty();
  m_dwSize = 0;
  m_bIsFolder = false;
  m_bPlayableFolder = false;
  m_bIsParentFolder=false;
  m_bIsShareOrDrive = false;
  m_dateTime.Reset();
  m_iDriveType = CMediaSource::SOURCE_TYPE_UNKNOWN;
  m_lStartOffset = 0;
  m_lEndOffset = 0;
  m_iprogramCount = 0;
  m_idepth = 1;
  m_iLockMode = LOCK_MODE_EVERYONE;
  m_strLockCode = "";
  m_iBadPwdCount = 0;
  m_iHasLock = 0;
  m_bCanQueue=true;
  m_contenttype = "";
  if (m_musicInfoTag)
    delete m_musicInfoTag;
  m_musicInfoTag=NULL;
  if (m_videoInfoTag)
    delete m_videoInfoTag;
  m_videoInfoTag=NULL;
  if (m_pictureInfoTag)
    delete m_pictureInfoTag;
  m_pictureInfoTag=NULL;
  m_extrainfo.Empty();

  m_linksFileItemList.reset();

  m_nextItem.reset();
  m_prevItem.reset();

  // m_externalFileItem should not be reset, since it will happen automatically either during assignment or at destruction
  ClearProperties();
  SetInvalid();
}

void CFileItem::Archive(CArchive& ar)
{
  CGUIListItem::Archive(ar);

  if (ar.IsStoring())
  {
    // mark this as new-archive
    unsigned char cMark = 0xff;
    ar << cMark;
    
    int nDumpVersion = 3;
    ar << nDumpVersion;
    
    ar << m_bPlayableFolder;
    ar << m_bIsParentFolder;
    ar << m_bLabelPreformated;
    ar << m_strPath;
    ar << m_bIsShareOrDrive;
    ar << m_iDriveType;
    ar << m_dateTime;
    ar << m_dwSize;
    ar << m_strDVDLabel;
    ar << m_strTitle;
    ar << m_iprogramCount;
    ar << m_idepth;
    ar << m_lStartOffset;
    ar << m_lEndOffset;
    ar << m_iLockMode;
    ar << m_strLockCode;
    ar << m_iBadPwdCount;

    ar << m_bCanQueue;
    ar << m_contenttype;
    ar << m_extrainfo;

    if (m_musicInfoTag)
    {
      ar << 1;
      ar << *m_musicInfoTag;
    }
    else
      ar << 0;
    if (m_videoInfoTag)
    {
      ar << 1;
      ar << *m_videoInfoTag;
    }
    else
      ar << 0;
    if (m_pictureInfoTag)
    {
      ar << 1;
      ar << *m_pictureInfoTag;
    }
    else
      ar << 0;

    if (HasLinksList())
    {
      ar << 1;
      ar << *((CFileItemList*)m_linksFileItemList.get());
    }
    else
    {
      ar << 0;
    }
  }
  else
  {
    bool bIsParentFolder = false;
    unsigned char cMark = 0x00;
    int nVersion = 0;
    
    ar >> cMark;
    if (cMark == 0xff)
    {
      ar >> nVersion;
      ar >> bIsParentFolder;
    }
    else
    {
      bIsParentFolder = (cMark != 0x00);
    }
    
    m_bIsParentFolder = bIsParentFolder;
    
    ar >> m_bPlayableFolder;
    ar >> m_bLabelPreformated;
    ar >> m_strPath;
    ar >> m_bIsShareOrDrive;
    ar >> m_iDriveType;
    ar >> m_dateTime;
    ar >> m_dwSize;
    ar >> m_strDVDLabel;
    ar >> m_strTitle;
    ar >> m_iprogramCount;
    ar >> m_idepth;
    ar >> m_lStartOffset;
    ar >> m_lEndOffset;
    int lockmode;
    ar >> lockmode;
    m_iLockMode = (LockType)lockmode;
    ar >> m_strLockCode;
    ar >> m_iBadPwdCount;

    ar >> m_bCanQueue;
    ar >> m_contenttype;
    ar >> m_extrainfo;

    int iType;
    ar >> iType;
    if (iType == 1)
      ar >> *GetMusicInfoTag();
    ar >> iType;
    if (iType == 1)
      ar >> *GetVideoInfoTag();
    ar >> iType;
    if (iType == 1)
      ar >> *GetPictureInfoTag();


    if (nVersion > 2)
    {
      ar >> iType;
      if (iType == 1)
      {
        if (!HasLinksList())
        {
          m_linksFileItemList = CFileItemPtr(new CFileItemList());
        }

        ((CFileItemList*)m_linksFileItemList.get())->Clear();
        ar >> *((CFileItemList*)m_linksFileItemList.get());
      }
    }

    SetInvalid();
  }
}

void CFileItem::Serialize(CVariant& value)
{
  //CGUIListItem::Serialize(value["CGUIListItem"]);

  value["strPath"] = m_strPath;
  value["dateTime"] = (m_dateTime.IsValid()) ? m_dateTime.GetAsRFC1123DateTime() : "";
  value["size"] = (int) m_dwSize / 1000;
  value["DVDLabel"] = m_strDVDLabel;
  value["title"] = m_strTitle;
  value["extrainfo"] = m_extrainfo;

  if (m_musicInfoTag)
    (*m_musicInfoTag).Serialize(value["musicInfoTag"]);

  if (m_videoInfoTag)
    (*m_videoInfoTag).Serialize(value["videoInfoTag"]);

  if (m_pictureInfoTag)
    (*m_pictureInfoTag).Serialize(value["pictureInfoTag"]);
}

bool CFileItem::Exists() const
{
  if (m_strPath.IsEmpty() || m_strPath.Equals("add") || IsInternetStream() || IsParentFolder() || IsVirtualDirectoryRoot() || IsPlugin())
    return true;

  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.Exists();
  }

  CStdString strPath = m_strPath;
 
  if (CUtil::IsMultiPath(strPath))
    strPath = CMultiPathDirectory::GetFirstPath(strPath);
 
  if (CUtil::IsStack(strPath))
    strPath = CStackDirectory::GetFirstStackedFile(strPath);

  if (m_bIsFolder)
    return CDirectory::Exists(strPath);
  else
    return CFile::Exists(strPath);

  return false;
}

bool CFileItem::IsVideo() const
{
  if (HasVideoInfoTag()) return true;

  if(IsPlayableFolder()) return true;

  /* check preset content type */
  if( m_contenttype.Left(6).Equals("video/") )
    return true;

  if (m_strPath.Left(7).Equals("tuxbox:"))
    return true;

  if (m_strPath.Left(6).Equals("flash:"))
    return true;

  if (m_strPath.Left(10).Equals("hdhomerun:"))
    return true;
  
  if (m_contenttype.Equals("application/x-shockwave-flash") ||
      m_contenttype.Equals("application/x-vnd.movenetworks.qm") ||
      m_contenttype.Equals("text/html") ||
      m_contenttype.Equals("application/x-silverlight-2") ||
      m_contenttype.Equals("application/x-silverlight"))
    return true;

  if (m_strPath.Left(4).Equals("dvd:"))
    return true;

   if(m_bPlayableFolder)
     return true;

  CStdString extension;
  if( m_contenttype.Left(12).Equals("application/") )
  { /* check for some standard types */
    extension = m_contenttype.Mid(12);
    if( extension.Equals("ogg")
     || extension.Equals("mp4")
     || extension.Equals("mxf") )
     return true;
  }

  CUtil::GetExtension(m_strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();

  // if the file is rar file - then make sure that it is the part0 file
  // the function "is rar" check it.
  if (extension.Equals(".rar"))
  {
    return IsRAR();
  }

  if (g_stSettings.m_videoExtensions.Find(extension) != -1)
    return true;

  return false;
}

bool CFileItem::IsAudio() const
{
  if (HasVideoInfoTag()) return false;
  if (HasMusicInfoTag()) return true;
  if (HasPictureInfoTag()) return false;
  if (IsCDDA()) return true;
  if (!m_bIsFolder && IsShoutCast()) return true;
  if (!m_bIsFolder && IsLastFM()) return true;

  /* check preset content type */
  if( m_contenttype.Left(6).Equals("audio/") )
    return true;

  CStdString extension;
  if( m_contenttype.Left(12).Equals("application/") )
  { /* check for some standard types */
    extension = m_contenttype.Mid(12);
    if( extension.Equals("ogg")
     || extension.Equals("mp4")
     || extension.Equals("mxf") )
     return true;
  }

  CUtil::GetExtension(m_strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_stSettings.m_musicExtensions.Find(extension) != -1)
    return true;

  return false;
}

bool CFileItem::IsPicture() const
{
  if (HasPictureInfoTag()) return true;
  if (HasMusicInfoTag()) return false;
  if (HasVideoInfoTag()) return false;

  if( m_contenttype.Left(6).Equals("image/") )
    return true;

  CStdString extension;
  CUtil::GetExtension(m_strPath, extension);

  if (extension.IsEmpty())
    return false;

  extension.ToLower();
  if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
    return true;

  if (extension == ".tbn" || extension == ".dds")
    return true;

  return false;
}

bool CFileItem::IsLyrics() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  return (strExtension.CompareNoCase(".cdg") == 0 || strExtension.CompareNoCase(".lrc") == 0);
}

bool CFileItem::IsCUESheet() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  return (strExtension.CompareNoCase(".cue") == 0);
}

bool CFileItem::IsShoutCast() const
{
  return CUtil::IsShoutCast(m_strPath);
}
bool CFileItem::IsLastFM() const
{
  return CUtil::IsLastFM(m_strPath);
}

bool CFileItem::IsInternetPlayList() const
{
  CURI url(m_strPath);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();

  if (strProtocol == "playlist")
  {
    CStdString path = url.GetUrlWithoutOptions();
    if (path.IsEmpty())
      path = m_strPath;

    // remove playlist://
    path = path.substr(11);
    CUtil::UrlDecode(path);
    CURI u(path);

    strProtocol = u.GetProtocol();
    strProtocol.ToLower();

    if(strProtocol == "http")
      return true;
  }

  return false;
}

bool CFileItem::IsInternetStream() const
{
  CURI url(m_strPath);
  CStdString strProtocol = url.GetProtocol();
  strProtocol.ToLower();

  if (strProtocol.IsEmpty() || HasProperty("IsHTTPDirectory"))
    return false;

  // there's nothing to stop internet streams from being stacked
  if (strProtocol == "stack")
  {
    CFileItem fileItem(CStackDirectory::GetFirstStackedFile(m_strPath), false);
    return fileItem.IsInternetStream();
  }

  if (strProtocol == "shout" || strProtocol == "mms" ||
      strProtocol == "http" || /*strProtocol == "ftp" ||*/
      strProtocol == "rtsp" || strProtocol == "rtp" ||
      strProtocol == "udp"  || strProtocol == "lastfm" ||
      strProtocol == "https" || strProtocol == "rss" ||
      strProtocol == "rtmp" || strProtocol == "hulu" ||
      strProtocol == "flash" )
    return true;

  if(GetPropertyBOOL("isinternetstream") == true)
  {
    return true;  
  }
  
  return false;
}

bool CFileItem::IsFileFolder() const
{
  return (
    m_bIsFolder && (
    IsPlugin() ||
    IsSmartPlayList() ||
    IsPlayList() ||
    IsZIP() ||
    IsRAR() ||
    IsType(".ogg") ||
    IsType(".nsf") ||
    IsType(".sid") ||
    IsShoutCast() ||
    IsRSS() ||
    IsLastFM() ||
    IsType(".sap") ||
    IsShoutCast()
    )
    );
}


bool CFileItem::IsSmartPlayList() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  strExtension.ToLower();
  return (strExtension == ".xsp");
}

bool CFileItem::IsPlayList(bool bAllowQuery) const
{
  return CPlayListFactory::IsPlaylist(*this, bAllowQuery);
}

bool CFileItem::IsPythonScript() const
{
  return CUtil::GetExtension(m_strPath).Equals(".py", false) || 
         CUtil::GetExtension(m_strPath).Equals(".pyo", false) ||
         CUtil::GetExtension(m_strPath).Equals(".pyc", false);
}

bool CFileItem::IsXBE() const
{
  return CUtil::GetExtension(m_strPath).Equals(".xbe", false);
}

bool CFileItem::IsType(const char *ext) const
{
  return CUtil::GetExtension(m_strPath).Equals(ext, false);
}

bool CFileItem::IsDefaultXBE() const
{
  CStdString filename = CUtil::GetFileName(m_strPath);
  if (filename.Equals("default.xbe")) return true;
  return false;
}

bool CFileItem::IsShortCut() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cut", false);
}

bool CFileItem::IsNFO() const
{
  return CUtil::GetExtension(m_strPath).Equals(".nfo", false);
}

bool CFileItem::IsDVDImage() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);
  strExtension.ToLower();
  if (strExtension.Equals(".img") || strExtension.Equals(".iso") || strExtension.Equals(".nrg")) return true;
  return false;
}

bool CFileItem::IsDVDFile(bool bVobs /*= true*/, bool bIfos /*= true*/) const
{
  CStdString strFileName = CUtil::GetFileName(m_strPath);
  if (bIfos)
  {
    if (strFileName.Equals("video_ts.ifo")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(6).Equals("_0.ifo") && strFileName.length() == 12) return true;
  }
  if (bVobs)
  {
    if (strFileName.Equals("video_ts.vob")) return true;
    if (strFileName.Left(4).Equals("vts_") && strFileName.Right(4).Equals(".vob")) return true;
  }

  return false;
}

bool CFileItem::IsRAR() const
{
  CStdString strExtension;
  CUtil::GetExtension(m_strPath, strExtension);

  // in case that the ext is rar -  we only refer to the first part01.rar as rar files, ignore all other parts
  if (strExtension.CompareNoCase(".rar") == 0)
  {
    vector<CStdString> tokens;
    CUtil::Tokenize(m_strPath,tokens,".");
    if (tokens.size() <= 2)
    {
      return true;
    }
    CStdString token = tokens[tokens.size()-2];
    if (token.Left(4).CompareNoCase("part") != 0) // only list '.part01.rar'
    {
      return true;
    }
    // need this crap to avoid making mistakes - yeyh for the new rar naming scheme :/
    struct __stat64 stat;
    int digits = token.size()-4;
    CStdString strNumber, strFormat;
    strFormat.Format("part%%0%ii",digits);
    strNumber.Format(strFormat.c_str(),1);
    CStdString strPath2=m_strPath;
    strPath2.Replace(token,strNumber);
    if (atoi(token.substr(4).c_str()) > 1 && CFile::Stat(strPath2,&stat) == 0)
    {
  return false;
}
    return true;

  }
  if (((strExtension.Equals(".001") && m_strPath.Mid(m_strPath.length()-7,7).CompareNoCase(".ts.001"))) ) return true; // sometimes the first rar is named .001

  return false;
}

bool CFileItem::IsZIP() const
{
  return CUtil::GetExtension(m_strPath).Equals(".zip", false);
}

bool CFileItem::IsCBZ() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cbz", false);
}

bool CFileItem::IsCBR() const
{
  return CUtil::GetExtension(m_strPath).Equals(".cbr", false);
}

bool CFileItem::IsStack() const
{
  return CUtil::IsStack(m_strPath);
}

bool CFileItem::IsPlugin() const
{
  CURI url(m_strPath);
  return url.GetProtocol().Equals("plugin") && !url.GetFileName().IsEmpty();
}

bool CFileItem::IsPluginRoot() const
{
  CURI url(m_strPath);
  return url.GetProtocol().Equals("plugin") && url.GetFileName().IsEmpty();
}

bool CFileItem::IsMultiPath() const
{
  return CUtil::IsMultiPath(m_strPath);
}

bool CFileItem::IsCDDA() const
{
  return CUtil::IsCDDA(m_strPath);
}

bool CFileItem::IsDVD() const
{
  return CUtil::IsDVD(m_strPath) || m_iDriveType == CMediaSource::SOURCE_TYPE_DVD;
}

bool CFileItem::IsOnDVD() const
{
  return CUtil::IsOnDVD(m_strPath) || m_iDriveType == CMediaSource::SOURCE_TYPE_DVD;
}

bool CFileItem::IsOnLAN() const
{
  return CUtil::IsOnLAN(m_strPath);
}

bool CFileItem::IsISO9660() const
{
  return CUtil::IsISO9660(m_strPath);
}

bool CFileItem::IsRemote() const
{
  return CUtil::IsRemote(m_strPath);
}

bool CFileItem::IsSmb() const
{
  return CUtil::IsSmb(m_strPath);
}

bool CFileItem::IsDAAP() const
{
  return CUtil::IsDAAP(m_strPath);
}

bool CFileItem::IsTuxBox() const
{
  return CUtil::IsTuxBox(m_strPath);
}

bool CFileItem::IsUPnP() const
{
  return CUtil::IsUPnP(m_strPath);
}

bool CFileItem::IsNFS() const
{
  return CUtil::IsNfs(m_strPath);
}

bool CFileItem::IsAFP() const
{
  return CUtil::IsAfp(m_strPath);
}

bool CFileItem::IsMythTV() const
{
  return CUtil::IsMythTV(m_strPath);
}

bool CFileItem::IsHDHomeRun() const
{
  return CUtil::IsHDHomeRun(m_strPath);
}

bool CFileItem::IsVTP() const
{
  return CUtil::IsVTP(m_strPath);
}

bool CFileItem::IsLiveTV() const
{
  return CUtil::IsLiveTV(m_strPath);
}

bool CFileItem::IsHD() const
{
  return CUtil::IsHD(m_strPath);
}

bool CFileItem::IsMusicDb() const
{
  if (strstr(m_strPath.c_str(), "musicdb:") ) return true;
  return false;
}

bool CFileItem::IsVideoDb() const
{
  if (strstr(m_strPath.c_str(), "videodb:") ) return true;
  return false;
}

bool CFileItem::IsVirtualDirectoryRoot() const
{
  return (m_bIsFolder && m_strPath.IsEmpty());
}

bool CFileItem::IsMemoryUnit() const
{
  CURI url(m_strPath);
  return url.GetProtocol().Left(3).Equals("mem");
}

bool CFileItem::IsRemovable() const
{
  return IsOnDVD() || IsCDDA() || IsMemoryUnit() || m_iDriveType == CMediaSource::SOURCE_TYPE_REMOVABLE;
}

bool CFileItem::IsReadOnly() const
{
  if (IsParentFolder()) return true;
  if (m_bIsShareOrDrive) return true;
  return !CUtil::SupportsFileOperations(m_strPath);
}

bool CFileItem::IsMMS() const
{
  return CUtil::IsMMS(m_strPath) || GetPropertyBOOL("IsMMS");
}

bool CFileItem::IsRSS() const
{
  return CUtil::IsRSS(m_strPath) || GetPropertyBOOL("IsRss");
}

bool CFileItem::IsFlash() const
{
  return CUtil::IsFlash(m_strPath) || GetPropertyBOOL("IsFlash");
}

bool CFileItem::IsScript() const
{
  return CUtil::IsScript(m_strPath) || GetPropertyBOOL("IsScript");
}

bool CFileItem::IsApp() const
{
  return CUtil::IsApp(m_strPath) || GetPropertyBOOL("IsApp");
}

bool CFileItem::IsBoxeeDb() const
{
  if (strstr(m_strPath.c_str(), "boxeedb:") ) return true;
  return false;
}

bool CFileItem::IsHidden() const
{
	CURI url(m_strPath);
	return url.GetFileName().Left(1).Equals(".");
}

void CFileItem::SetPlayableFolder(bool isDVD)
{
  if(isDVD)
  {
    this->SetProperty("isDVDFolder", true);
    this->SetProperty("contenttype", "dvd/folder");
  }
  else
  {
    this->SetProperty("isBlurayFolder", true);
    this->SetProperty("contenttype", "bluray/folder");
  }

  m_bPlayableFolder = 1;
}

bool CFileItem::IsPlayableFolder() const
{
  return m_bPlayableFolder;
}

void CFileItem::Dump() const
{
  CLog::Log(LOGDEBUG,"============================FileItem Dump Start===================================");
  CLog::Log(LOGDEBUG,"Is Folder: %s", m_bIsFolder?"true":"false");
  CLog::Log(LOGDEBUG,"Is Playable Folder: %s", m_bPlayableFolder?"true":"false");
  CLog::Log(LOGDEBUG,"Is Parent Folder: %s", m_bIsParentFolder?"true":"false");
  CLog::Log(LOGDEBUG,"Label: %s", GetLabel().c_str());
  CLog::Log(LOGDEBUG,"Label2: %s", m_strLabel2.c_str());
  CLog::Log(LOGDEBUG,"path: %s", m_strPath.c_str());
  CLog::Log(LOGDEBUG,"Label preformatted: %s", m_bLabelPreformated?"true":"false");
  CLog::Log(LOGDEBUG,"Thumb: %s", m_strThumbnailImage.c_str());
  CLog::Log(LOGDEBUG,"Icon: %s", m_strIcon.c_str());
  CLog::Log(LOGDEBUG,"Title: %s", m_strTitle.c_str());
  CLog::Log(LOGDEBUG,"m_contenttype: %s", m_contenttype.c_str());
  CLog::Log(LOGDEBUG,"m_extrainfo: %s", m_extrainfo.c_str());
  CLog::Log(LOGDEBUG,"Selected: %s", m_bSelected?"true":"false");
  CLog::Log(LOGDEBUG,"Size: %lld", m_dwSize);
  CLog::Log(LOGDEBUG,"m_iprogramCount: %d", m_iprogramCount);
  CLog::Log(LOGDEBUG,"m_idepth: %d", m_idepth);
  CLog::Log(LOGDEBUG,"m_lStartOffset: %d", m_lStartOffset);
  CLog::Log(LOGDEBUG,"m_lEndOffset: %d", m_lEndOffset);
  CLog::Log(LOGDEBUG,"DVD Label: %s", m_strDVDLabel.c_str());
  CLog::Log(LOGDEBUG,"OverlayIcon: %d", m_overlayIcon);
  CLog::Log(LOGDEBUG,"m_iDriveType: %d", m_iDriveType);
  CLog::Log(LOGDEBUG,"m_bCanQueue: %s", m_bCanQueue?"true":"false");
  CLog::Log(LOGDEBUG,"m_bIsShareOrDrive: %s", m_bIsShareOrDrive?"true":"false");
  CLog::Log(LOGDEBUG,"m_dateTime: %s", m_dateTime.GetAsDBDate().c_str());
  CLog::Log(LOGDEBUG,"m_iLockMode: %d", m_iLockMode);
  CLog::Log(LOGDEBUG,"m_strLockCode: %s", m_strLockCode.c_str());
  CLog::Log(LOGDEBUG,"m_iBadPwdCount: %d", m_iBadPwdCount);

  if (m_videoInfoTag)
    m_videoInfoTag->Dump();
  else
    CLog::Log(LOGDEBUG,"** No video info tag");

  if (m_musicInfoTag)
    m_musicInfoTag->Dump();
  else
    CLog::Log(LOGDEBUG,"** No music info tag");

  CLog::Log(LOGDEBUG,"Properties (%zu):", m_mapProperties.size());
  std::map<CStdString, CStdString,icompare>::const_iterator iter = m_mapProperties.begin();
  while (iter != m_mapProperties.end())
  {
    CLog::Log(LOGDEBUG,"[%s] -> [%s]", iter->first.c_str(), iter->second.c_str());
    iter++;
  }
  
  if (HasExternlFileItem())
  {
    CLog::Log(LOGDEBUG, "** External file item:");
    m_externalFileItem->Dump();
  }
  else
  {
    CLog::Log(LOGDEBUG, "** No external file item");
  }
  
  if (HasLinksList())
  {
    CFileItemList* list = (CFileItemList*)m_linksFileItemList.get();
    int numLinks = list->Size();
    CLog::Log(LOGDEBUG,"Num of links: %d", numLinks);
    for (int i = 0; i < numLinks; i++)
    {
      CLog::Log(LOGDEBUG,"Links [%d] : %s", i, list->Get(i)->m_strPath.c_str());
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "** No additional links");
  }

  CLog::Log(LOGDEBUG,"============================FileItem Dump End===================================");
}

void CFileItem::FillInDefaultIcon()
{
  //CLog::Log(LOGINFO, "FillInDefaultIcon(%s)", pItem->GetLabel().c_str());
  // find the default icon for a file or folder item
  // for files this can be the (depending on the file type)
  //   default picture for photo's
  //   default picture for songs
  //   default picture for videos
  //   default picture for shortcuts
  //   default picture for playlists
  //   or the icon embedded in an .xbe
  //
  // for folders
  //   for .. folders the default picture for parent folder
  //   for other folders the defaultFolder.png

  if (GetIconImage().IsEmpty())
  {
    if (!m_bIsFolder)
    {
      /* To reduce the average runtime of this code, this list should
       * be ordered with most frequently seen types first.  Also bear
       * in mind the complexity of the code behind the check in the
       * case of IsWhatater() returns false.
       */
      if ( IsAudio() )
      {
        // audio
        SetIconImage("DefaultAudio.png");
      }
      else if ( IsVideo() )
      {
        // video
        SetIconImage("DefaultVideo.png");
      }
      else if ( IsPicture() )
      {
        // picture
        SetIconImage("DefaultPicture.png");
      }
      else if ( IsPlayList(false) )
      {
        SetIconImage("DefaultPlaylist.png");
      }
      else if ( IsXBE() )
      {
        // xbe
        SetIconImage("DefaultProgram.png");
      }
      else if ( IsShortCut() && !IsLabelPreformated() )
      {
        // shortcut
        CStdString strFName = CUtil::GetFileName(m_strPath);
        int iPos = strFName.ReverseFind(".");
        CStdString strDescription = strFName.Left(iPos);
        SetLabel(strDescription);
        SetIconImage("DefaultShortcut.png");
      }
      else if ( IsPythonScript() )
      {
        SetIconImage("DefaultScript.png");
      }
      else
      {
        // default icon for unknown file type
        SetIconImage("DefaultFile.png");
      }
    }
    else
    {
      if ( IsPlayList() )
      {
        SetIconImage("DefaultPlaylist.png");
      }
      else if (IsParentFolder())
      {
        SetIconImage("DefaultFolderBack.png");
      }
      else
      {
        SetIconImage("DefaultFolder.png");
      }
    }
  }

  //Boxee
  BoxeeUtils::SetDefaultIcon(this);
  //end Boxee

  // Set the icon overlays (if applicable)
  if (!HasOverlay())
  {
    if (CUtil::IsInRAR(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_RAR);
    else if (CUtil::IsInZIP(m_strPath))
      SetOverlayImage(CGUIListItem::ICON_OVERLAY_ZIP);
  }
}

CStdString CFileItem::GetCachedArtistThumb() const
{
  return GetCachedThumb("artist"+GetLabel(),g_settings.GetMusicArtistThumbFolder());
}

CStdString CFileItem::GetCachedProfileThumb() const
{
  return GetCachedThumb("profile"+m_strPath,CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"Thumbnails\\Profiles"));
}

CStdString CFileItem::GetCachedSeasonThumb() const
{
  Crc32 crc;
  CStdString seasonPath;
  if (HasVideoInfoTag())
    seasonPath = GetVideoInfoTag()->m_strPath;

  return GetCachedThumb("season"+seasonPath+GetLabel(),g_settings.GetVideoThumbFolder(),true);
}

CStdString CFileItem::GetCachedActorThumb() const
{
  return GetCachedThumb("actor"+GetLabel(),g_settings.GetVideoThumbFolder(),true);
}

void CFileItem::SetCachedArtistThumb()
{
  CStdString thumb(GetCachedArtistThumb());
  if (CFile::Exists(thumb))
  {
    // found it, we are finished.
    SetThumbnailImage(thumb);
  }
}

// set the album thumb for a file or folder
void CFileItem::SetMusicThumb(bool alwaysCheckRemote /* = true */)
{
  if (HasThumbnail()) return;
  
  SetCachedMusicThumb();
  if (!HasThumbnail())
    SetUserMusicThumb(alwaysCheckRemote);
}

void CFileItem::SetCachedSeasonThumb()
{
  CStdString thumb(GetCachedSeasonThumb());
  if (CFile::Exists(thumb))
  {
    // found it, we are finished.
    SetThumbnailImage(thumb);
  }
}

void CFileItem::RemoveExtension()
{
  if (m_bIsFolder)
    return ;
  CStdString strLabel = GetLabel();
  CUtil::RemoveExtension(strLabel);
  SetLabel(strLabel);
}

void CFileItem::CleanString()
{
  if (IsLiveTV())
    return ;

  bool bIsFolder = m_bIsFolder;

  // make sure we don't append the extension to stacked dvd folders
  if (HasProperty("isstacked") && IsDVDFile(false, true))
    bIsFolder = true;

  CStdString strLabel = GetLabel();
  CStdString strTitle, strTitleAndYear, strYear;
  CUtil::CleanString(strLabel, strTitle, strTitleAndYear, strYear, bIsFolder);
  SetLabel(strTitleAndYear);
}

void CFileItem::SetLabel(const CStdString &strLabel)
{
  if (strLabel=="..")
  {
    m_bIsParentFolder=true;
    m_bIsFolder=true;
    SetLabelPreformated(true);
  }
  CGUIListItem::SetLabel(strLabel);
}

void CFileItem::SetFileSizeLabel()
{
  if( m_bIsFolder && m_dwSize == 0 )
    SetLabel2("");
  else
    SetLabel2(StringUtils::SizeToString(m_dwSize));
}

bool CFileItem::CanQueue() const
{
  return m_bCanQueue;
}

void CFileItem::SetCanQueue(bool bYesNo)
{
  m_bCanQueue=bYesNo;
}

bool CFileItem::IsParentFolder() const
{
  return m_bIsParentFolder;
}

const CStdString& CFileItem::GetContentType(bool bAllowQuery) const
{
  if( m_contenttype.IsEmpty() || GetPropertyBOOL("NeedVerify"))
  {
    // discard const qualifyier
    CStdString& m_ref = (CStdString&)m_contenttype;

    CStdString contentType = GetProperty("contenttype");

    if(contentType != "")
    {
      // discard const qualifyier
      CStdString& m_ref = (CStdString&)m_contenttype;
      m_ref = contentType;
    }
    else
    {
      if( m_bIsFolder ) 
      {
        m_ref = "x-directory/normal";
      }
      else if (bAllowQuery)
      {
        // dconti- this block is terrible; we are doing a network round trip
        // just to get the content type
        /* bondar - takes ridiculous amount of time
        if (m_strPath.Left(7).Equals("upnp://"))
        {
          CUPnPDirectory dir;
          CFileItemList list;

          CStdString strPath = m_strPath;
          CUtil::UrlDecode(strPath);
          CUtil::AddSlashAtEnd(strPath);
          if (dir.GetDirectory(strPath, list))
          {
            if (!list.IsEmpty())
            {
              m_ref = list[0]->GetContentType();
              CLog::Log(LOGDEBUG,"CFileItem::GetContentType, FOUND CONTENT TYPE path:[%s].",strPath.c_str());
            }
            else
            {
              CLog::Log(LOGDEBUG,"CFileItem::GetContentType, empty list for directory path:[%s].",strPath.c_str());
            }
          }
          else
          {
            CLog::Log(LOGDEBUG,"CFileItem::GetContentType, couldn't get content type for path:[%s].",strPath.c_str());
          }
        }
        else*/
        if (m_strPath.Left(8).Equals("shout://") || m_strPath.Left(7).Equals("http://") || m_strPath.Left(8).Equals("https://"))
        {
          CFileCurl::GetContent(CURI(m_strPath), m_ref);

          // try to get content type again but with an NSPlayer User-Agent
          // in order for server to provide correct content-type.  Allows us
          // to properly detect an MMS stream
          if (m_ref.Left(11).Equals("video/x-ms-"))
            CFileCurl::GetContent(CURI(m_strPath), m_ref, "NSPlayer/11.00.6001.7000");

          // make sure there are no options set in content type
          // content type can look like "video/x-ms-asf ; charset=utf8"
          int i = m_ref.Find(';');
          if (i >= 0)
            m_ref.Delete(i, m_ref.length() - i);
          m_ref.Trim();
        }

        // if it's still empty set to an unknown type
        if (m_ref.IsEmpty() && bAllowQuery)
          m_ref = "application/octet-stream";
      }
    }
    
    if (bAllowQuery)
    {
      ((CFileItem*)this) -> SetProperty("ContentTypeVerified",true);
      ((CFileItem*)this) -> SetProperty("NeedVerify",false);
  }
  }

  // change protocol to mms for the following content-type.  Allows us to create proper FileMMS.
  if( m_contenttype.Left(32).Equals("application/vnd.ms.wms-hdr.asfv1") || m_contenttype.Left(24).Equals("application/x-mms-framed") )
  {
    CStdString& m_path = (CStdString&)m_strPath;
    m_path.Replace("http:", "mms:");
  }

  return m_contenttype;
}

bool CFileItem::IsSamePath(const CFileItem *item) const
{
  if (!item)
    return false;

  if (item->m_strPath == m_strPath && item->m_lStartOffset == m_lStartOffset) return true;
  if (IsMusicDb() && HasMusicInfoTag())
  {
    CFileItem dbItem(m_musicInfoTag->GetURL(), false);
    dbItem.m_lStartOffset = m_lStartOffset;
    return dbItem.IsSamePath(item);
  }
  if (IsVideoDb() && HasVideoInfoTag())
  {
    CFileItem dbItem(m_videoInfoTag->m_strFileNameAndPath, false);
    dbItem.m_lStartOffset = m_lStartOffset;
    return dbItem.IsSamePath(item);
  }
  if (item->IsMusicDb() && item->HasMusicInfoTag())
  {
    CFileItem dbItem(item->m_musicInfoTag->GetURL(), false);
    dbItem.m_lStartOffset = item->m_lStartOffset;
    return IsSamePath(&dbItem);
  }
  if (item->IsVideoDb() && item->HasVideoInfoTag())
  {
    CFileItem dbItem(item->m_videoInfoTag->m_strFileNameAndPath, false);
    dbItem.m_lStartOffset = item->m_lStartOffset;
    return IsSamePath(&dbItem);
  }
  return false;
}

void CFileItem::GetIPhoneXml(CStdString& itemXml)
{
  itemXml.clear();
  
  // Add attribute for item path
  itemXml += " path=\"";
  itemXml += m_strPath;
  itemXml += "\"";

  // Add attribute for item thumb
  if(m_strThumbnailImage.IsEmpty() == false)
  {
    itemXml += " thumb=\"";
    itemXml += m_strThumbnailImage;
    itemXml += "\"";

    CLog::Log(LOGDEBUG,"CFileItem::GetIPhoneXml - After adding [m_strThumbnailImage=%s] to ItemXml (bgmi)",m_strThumbnailImage.c_str());
  }

  // Add attribute for MediaType
  CStdString mediaType = "";
  if(IsVideo())
  {
    mediaType += "video";      
  }
  else if(IsAudio())
  {
    mediaType += "audio";            
  }
  else if(IsPicture())
  {
    mediaType += "picture";      
  }
  else
  {
    // No MediaType      
  }

  if(mediaType.IsEmpty() == false)
  {
    itemXml += " mediatype=\"";
    itemXml += mediaType;
    itemXml += "\"";

    CLog::Log(LOGDEBUG,"CFileItem::GetIPhoneXml - After adding [mediaType=%s] to ItemXml (bgmi)",mediaType.c_str());
  }
  else
  {
    if(m_bIsFolder == false)
    {
      CLog::Log(LOGWARNING,"CFileItem::GetIPhoneXml - There is no MediaType [%s] although the item [path=%s] is NOT a folder (bgmi)",mediaType.c_str(),m_strPath.c_str());
    }
  }

  // Add atribute for IsFolder
  itemXml += " isfolder=\"";

  char tmp[5];
  itoa(m_bIsFolder,tmp,10);
  itemXml += tmp;
  itemXml += "\">";

  CLog::Log(LOGDEBUG,"CFileItem::GetIPhoneXml - After adding [isfolder=%s] to ItemXml (bgmi)",tmp);

  // Add parameter for item label
  itemXml += "<label>";
  itemXml += GetLabel();
  itemXml += "</label>";
  
  CLog::Log(LOGDEBUG,"CFileItem::GetIPhoneXml - After adding [label=%s] to ItemXml (bgmi)",(GetLabel()).c_str());
}

void CFileItem::SetExternalFileItem(CFileItemPtr externalFileItem)
{
  m_externalFileItem = externalFileItem;
}

CFileItemPtr CFileItem::GetExternalFileItem() const
{
  return m_externalFileItem;
}

bool CFileItem::HasExternlFileItem() const
{
  return m_externalFileItem.get() != NULL;
}

bool CFileItem::AddLink(const CFileItemPtr& linkItem , bool bPushBack)
{
  if (!linkItem->HasProperty("link-url") || !linkItem->HasProperty("link-boxeetype") || !linkItem->HasProperty("link-type") || !linkItem->HasProperty("contenttype"))
  {
    //like in the regular AddLink function, we must have certain properties in the link
    return false;
  }

  if (!HasLinksList())
  {
    m_linksFileItemList = CFileItemPtr(new CFileItemList());
  }

  SetPropertyForLinkType(linkItem->GetProperty("link-boxeetype"), linkItem);
  SetPropertyForLinkOffer(linkItem->GetProperty("link-boxeeoffer"), linkItem);

  //CLog::Log(LOGDEBUG,"CFileItem::AddLink - [ItemLabel=%s][NumOfLinks=%d] - Adding link item at %s position [label=%s][path=%s][thumb=%s][contenttype=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma1)",GetLabel().c_str(),((CFileItemList*)m_linksFileItemList.get())->Size(),bPushBack?"Last":"First",linkItem->GetLabel().c_str(),linkItem->m_strPath.c_str(),linkItem->GetThumbnailImage().c_str(),linkItem->GetProperty("contenttype").c_str(),linkItem->GetProperty("link-title").c_str(),linkItem->GetProperty("link-url").c_str(),linkItem->GetProperty("link-boxeetype").c_str(),linkItem->GetProperty("link-boxeeoffer").c_str(),linkItem->GetProperty("link-type").c_str(),linkItem->GetProperty("link-provider").c_str(),linkItem->GetProperty("link-providername").c_str(),linkItem->GetProperty("link-providerthumb").c_str(),linkItem->GetProperty("link-countrycodes").c_str(),linkItem->GetPropertyBOOL("link-countryrel"),linkItem->GetProperty("quality-lbl").c_str(),linkItem->GetPropertyInt("quality"),linkItem->GetProperty("is-hd").c_str(),linkItem->GetProperty("link-productslist").c_str());

  if (bPushBack)
  {
    ((CFileItemList*)m_linksFileItemList.get())->Add(linkItem);
  }
  else
  {
    ((CFileItemList*)m_linksFileItemList.get())->AddFront(linkItem,0);
  }

  return true;
}

bool CFileItem::CreateLink(CFileItemPtr& linkItem ,const CStdString& title, const CStdString& url, const CStdString& contentType, CLinkBoxeeType::LinkBoxeeTypeEnums boxeeTypeEnum, const CStdString& provider, const CStdString& providerName, const CStdString& providerThumb, const CStdString& countries, bool countriesRelAllow, const CStdString &qualityLabel, int quality, CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOfferEnum, const CStdString& productsList)
{
  if (title.IsEmpty())
  {
    //CLog::Log(LOGDEBUG,"CFileItem::AddLink - [title] is empty so item label will be set to [Play]. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerName=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerName.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum).c_str(),productsList.c_str());

    linkItem->SetProperty("link-title","Play");
    linkItem->SetLabel("Play");
  }
  else
  {
    linkItem->SetProperty("link-title",title);
    linkItem->SetLabel(title);
  }

  if (url.IsEmpty())
  {
    CLog::Log(LOGERROR,"CFileItem::CreateLink - Link will not be created because [url] is empty. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum).c_str(),productsList.c_str());
    return false;
  }

  linkItem->SetProperty("link-url",url);
  CStdString path(url);
  linkItem->m_strPath = path;
  linkItem->SetProperty("isinternetstream", linkItem->IsInternetStream());

  CStdString boxeeTypeStr = CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum);
  if (boxeeTypeStr.IsEmpty())
  {
    CLog::Log(LOGERROR,"CFileItem::CreateLink - Link will not be created because [boxeeTypeEnum=%d] is unknown. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",boxeeTypeEnum,title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum).c_str(),productsList.c_str());
    return false;
  }

  linkItem->SetProperty("link-boxeetype",boxeeTypeStr);

  // 130910 - Not handling BoxeeOffer at the moment
  CStdString boxeeOfferStr = CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum);
  if (boxeeOfferStr.IsEmpty())
  {
    // 140910 - don't fail link on BoxeeOffer since production server doesn't support it at the moment.
    boxeeOfferStr = "unknown";
    //CLog::Log(LOGERROR,"CFileItem::AddLink - Link will not be add because [boxeeOfferEnum=%d] is unknown. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",boxeeOfferEnum,title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,boxeeOfferStr.c_str(),productsList.c_str());
    //return false;
  }

  linkItem->SetProperty("link-boxeeoffer",boxeeOfferStr);

  if (contentType.IsEmpty())
  {
    CLog::Log(LOGERROR,"CFileItem::CreateLink - Link will not be created because [contentType] is empty. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum).c_str(),productsList.c_str());
    return false;
  }

  linkItem->SetProperty("link-type",contentType);
  linkItem->SetProperty("contenttype",contentType);

  linkItem->SetProperty("link-provider",provider);

  linkItem->SetProperty("link-providername",providerName);
  linkItem->SetLabel2(providerName);

  linkItem->SetProperty("link-providerthumb",providerThumb);
  linkItem->SetThumbnailImage(providerThumb);

  linkItem->SetProperty("link-countrycodes",countries);
  linkItem->SetProperty("link-countryrel",countriesRelAllow);
  linkItem->SetCountryRestriction(countries, countriesRelAllow);

  linkItem->SetProperty("quality-lbl",qualityLabel);
  linkItem->SetProperty("quality",quality);

  if (quality >= 720)
  {
    linkItem->SetProperty("is-hd","true");
  }

  linkItem->SetProperty("link-productslist",productsList);

  return true;
}

bool CFileItem::AddLink(const CStdString& title, const CStdString& url, const CStdString& contentType, CLinkBoxeeType::LinkBoxeeTypeEnums boxeeTypeEnum, const CStdString& provider, const CStdString& providerName, const CStdString& providerThumb, const CStdString& countries, bool countriesRelAllow, const CStdString &qualityLabel, int quality, CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOfferEnum, const CStdString& productsList , bool bPushBack)
{
  CFileItemPtr linkItem(new CFileItemList());

  if (CFileItem::CreateLink(linkItem,title,url,contentType,boxeeTypeEnum, provider, providerName, providerThumb , countries, countriesRelAllow , qualityLabel, quality , boxeeOfferEnum, productsList))
  {
    if (!HasLinksList())
    {
      m_linksFileItemList = CFileItemPtr(new CFileItemList());
    }

    //CLog::Log(LOGDEBUG,"CFileItem::AddLink - [ItemLabel=%s][NumOfLinks=%d] - Adding link item at %s position [label=%s][path=%s][thumb=%s][contenttype=%s]. [link-title=%s][link-url=%s][link-boxeetype=%s][link-boxeeoffer=%s][link-type=%s][link-provider=%s][link-providername=%s][link-providerthumb=%s][link-countrycodes=%s][link-countryrel=%d][quality-lbl=%s][quality=%d][is-hd=%s][productsList=%s] (bbma)",GetLabel().c_str(),((CFileItemList*)m_linksFileItemList.get())->Size(), bPushBack?"last":"first",linkItem->GetLabel().c_str(),linkItem->m_strPath.c_str(),linkItem->GetThumbnailImage().c_str(),linkItem->GetProperty("contenttype").c_str(),linkItem->GetProperty("link-title").c_str(),linkItem->GetProperty("link-url").c_str(),linkItem->GetProperty("link-boxeetype").c_str(),linkItem->GetProperty("link-boxeeoffer").c_str(),linkItem->GetProperty("link-type").c_str(),linkItem->GetProperty("link-provider").c_str(),linkItem->GetProperty("link-providername").c_str(),linkItem->GetProperty("link-providerthumb").c_str(),linkItem->GetProperty("link-countrycodes").c_str(),linkItem->GetPropertyBOOL("link-countryrel"),linkItem->GetProperty("quality-lbl").c_str(),linkItem->GetPropertyInt("quality"),linkItem->GetProperty("is-hd").c_str(),linkItem->GetProperty("link-productslist").c_str());

    SetPropertyForLinkType(linkItem->GetProperty("link-boxeetype"), linkItem);
    SetPropertyForLinkOffer(linkItem->GetProperty("link-boxeeoffer"), linkItem);

    if (bPushBack)
    {
      ((CFileItemList*)m_linksFileItemList.get())->Add(linkItem);
    }
    else
    {
      ((CFileItemList*)m_linksFileItemList.get())->AddFront(linkItem,0);
    }

    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"CFileItem::AddLink - [ItemLabel=%s] - FAILED to add link. [title=%s][url=%s][contentType=%s][boxeeType=%s][provider=%s][providerThumb=%s][countries=%s][countriesRelAllow=%d][qualityLabel=%s][quality=%d][boxeeOffer=%s][productsList=%s] (bma)",GetLabel().c_str(),title.c_str(),url.c_str(),contentType.c_str(),CFileItem::GetLinkBoxeeTypeAsString(boxeeTypeEnum).c_str(),provider.c_str(),providerThumb.c_str(),countries.c_str(),countriesRelAllow,qualityLabel.c_str(),quality,CFileItem::GetLinkBoxeeOfferAsString(boxeeOfferEnum).c_str(),productsList.c_str());
    return false;
  }
}

void CFileItem::SetPropertyForLinkType(CStdString boxeeTypeStr, CFileItemPtr linkItem)
{
  CLinkBoxeeType::LinkBoxeeTypeEnums linkBoxeeTypeEnum = CFileItem::GetLinkBoxeeTypeAsEnum(boxeeTypeStr);

  switch (linkBoxeeTypeEnum)
  {
  case CLinkBoxeeType::FEATURE:
  {
    SetProperty("haslink-feature",true);
  }
  break;
  case CLinkBoxeeType::CLIP:
  {
    SetProperty("haslink-clip",true);
    linkItem->SetLabel(g_localizeStrings.Get(53771));
  }
  break;
  case CLinkBoxeeType::TRAILER:
  {
    SetProperty("haslink-trailer",true);
    linkItem->SetLabel(g_localizeStrings.Get(53772));
  }
  break;
  /*
  case CLinkBoxeeType::RENT:
  {
    SetProperty("rent-play",true);
  }
  break;
  case CLinkBoxeeType::BUY:
  {
    SetProperty("buy-play",true);
  }
  break;
  case CLinkBoxeeType::SUBSCRIPTION:
  {
    SetProperty("sub-play",true);
    linkItem->SetLabel(g_localizeStrings.Get(53773));

    bool isRegisterToServices = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsRegisterToServices(linkItem->GetProperty("link-provider"),BOXEE::CServiceIdentifierType::NAME);
    if (isRegisterToServices)
    {
      SetProperty("free-play",true);
  }
  }
  break;
  */
  case CLinkBoxeeType::LOCAL:
  {
    SetProperty("haslink-free-local",true);
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }
}

void CFileItem::SetPropertyForLinkOffer(CStdString boxeeOfferStr, CFileItemPtr linkItem)
{
  CLinkBoxeeOffer::LinkBoxeeOfferEnums linkBoxeeOfferEnum = CFileItem::GetLinkBoxeeOfferAsEnum(boxeeOfferStr);

  switch (linkBoxeeOfferEnum)
  {
  case CLinkBoxeeOffer::UNAVAILABLE:
  {
    SetProperty("haslink-unavailable",true);
  }
  break;
  case CLinkBoxeeOffer::FREE:
  {
    SetProperty("haslink-free",true);
    linkItem->SetLabel(g_localizeStrings.Get(53770));
  }
  break;
  case CLinkBoxeeOffer::RENT:
  {
    SetProperty("haslink-rent",true);
  }
  break;
  case CLinkBoxeeOffer::BUY:
  {
    SetProperty("haslink-buy",true);
  }
  break;
  case CLinkBoxeeOffer::SUBSCRIPTION:
  {
    SetProperty("haslink-sub",true);

    bool isEntitle = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsInEntitlements(linkItem->GetProperty("link-productslist"));

    if (isEntitle)
    {
      SetProperty("haslink-free",true);
    }
  }
  break;
  case CLinkBoxeeOffer::EXT_SUBSCRIPTION:
  {
    SetProperty("haslink-exsub",true);
    linkItem->SetLabel(g_localizeStrings.Get(53773));

    bool isRegisterToServices = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsRegisterToServices(linkItem->GetProperty("link-provider"),BOXEE::CServiceIdentifierType::NAME);
    if (isRegisterToServices)
    {
      SetProperty("haslink-free",true);
      linkItem->SetLabel("");
    }
    else
    {
      linkItem->SetLabel(g_localizeStrings.Get(53773));
    }
  }
  break;
  default:
  {
    // do nothing
  }
  break;
  }
}

void CFileItem::SetLinksList(const CFileItemList* linksFileItemList)
{
  if (!HasLinksList())
  {
    m_linksFileItemList = CFileItemPtr(new CFileItemList());
  }

  ((CFileItemList*)m_linksFileItemList.get())->Clear();
  ((CFileItemList*)m_linksFileItemList.get())->Copy(*linksFileItemList);
}

void CFileItem::ClearLinksList()
{
  if (HasLinksList())
  {
    ((CFileItemList*)m_linksFileItemList.get())->Clear();
  }
}

const CFileItemList* CFileItem::GetLinksList() const
{
  CFileItemList* linksList = NULL;

  if (HasLinksList())
  {
    linksList = (CFileItemList*)m_linksFileItemList.get();
  }

  return linksList;
}

bool CFileItem::HasLinksList() const
{
  return m_linksFileItemList.get() != NULL;
}

void CFileItem::SortLinkList(CBoxeeSort& boxeeSort) const
{
  if (!HasLinksList())
  {
    return;
  }
  ((CFileItemList*)m_linksFileItemList.get())->Sort(boxeeSort);
}

bool CFileItem::IsAlbum() const
{
  return m_bIsAlbum;
}

/////////////////////////////////////////////////////////////////////////////////
/////
///// CFileItemList
/////
//////////////////////////////////////////////////////////////////////////////////

CFileItemList::CFileItemList()
{
  m_fastLookup = false;
  m_bIsFolder=true;
  m_cacheToDisc=CACHE_IF_SLOW;
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_replaceListing = false;
}

CFileItemList::CFileItemList(const CStdString& strPath)
{
  m_strPath=strPath;
  m_fastLookup = false;
  m_bIsFolder=true;
  m_cacheToDisc=CACHE_IF_SLOW;
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_replaceListing = false;
}

CFileItemList::CFileItemList(const CFileItemList &itemList) :  CFileItem(itemList)
{
  m_fastLookup = false;
  m_bIsFolder=true;
  m_cacheToDisc=CACHE_IF_SLOW;
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_replaceListing = false;

  *this = itemList;
}

CFileItemList::~CFileItemList()
{
  Clear();
}

CFileItemPtr CFileItemList::operator[] (int iItem)
{
  return Get(iItem);
}

const CFileItemPtr CFileItemList::operator[] (int iItem) const
{
  return Get(iItem);
}

void CFileItemList::SetAtIndex(int iItem, const CFileItemPtr &pItem)
{
  CSingleLock lock(m_lock);

  if ((int)m_items.size() <= iItem)
    m_items.resize(iItem + 1);

  m_items[iItem] = pItem;
  if (m_fastLookup)
  {
    CStdString path(pItem->m_strPath); path.ToLower();
    m_map.insert(MAPFILEITEMSPAIR(path, pItem));
  }
}

CFileItemPtr CFileItemList::operator[] (const CStdString& strPath)
{
  return Get(strPath);
}

const CFileItemPtr CFileItemList::operator[] (const CStdString& strPath) const
{
  return Get(strPath);
}

void CFileItemList::SetFastLookup(bool fastLookup)
{
  CSingleLock lock(m_lock);

  if (fastLookup && !m_fastLookup)
  { // generate the map
    m_map.clear();
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItemPtr pItem = m_items[i];
      CStdString path(pItem->m_strPath); path.ToLower();
      m_map.insert(MAPFILEITEMSPAIR(path, pItem));
    }
  }
  if (!fastLookup && m_fastLookup)
    m_map.clear();
  m_fastLookup = fastLookup;
}

void CFileItemList::SetFastLookup(bool fastLookup, const CStdString& strPropertyName)
{
  CSingleLock lock(m_lock);

  if (fastLookup && m_mapPropertyLookup.find(strPropertyName) == m_mapPropertyLookup.end())
  { // generate the map
    MAPFILEITEMS map;
    for (unsigned int i=0; i < m_items.size(); i++)
    {
      CFileItemPtr pItem = m_items[i];
      CStdString strPropertyValue(pItem->GetProperty(strPropertyName)); strPropertyValue.ToLower();
      map.insert(MAPFILEITEMSPAIR(strPropertyValue, pItem));
    }
    m_mapPropertyLookup.insert(std::pair<CStdString, MAPFILEITEMS >(strPropertyName, map));
  }

  std::map<CStdString, MAPFILEITEMS >::iterator it;
  if (!fastLookup && ((it = m_mapPropertyLookup.find(strPropertyName)) != m_mapPropertyLookup.end()))
    m_mapPropertyLookup.erase(it);

}

bool CFileItemList::Contains(const CStdString& fileName) const
{
  CSingleLock lock(m_lock);

  // checks case insensitive
  CStdString checkPath(fileName); checkPath.ToLower();
  if (m_fastLookup)
    return m_map.find(checkPath) != m_map.end();
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    const CFileItemPtr pItem = m_items[i];
    if (pItem->m_strPath.Equals(checkPath))
      return true;
  }
  return false;
}

bool CFileItemList::Contains(const CStdString& propertyName, const CStdString& propertyValue) const
{
  // find a map of lookups for the property
  std::map<CStdString, MAPFILEITEMS >::const_iterator it = m_mapPropertyLookup.find(propertyName);
  if (it != m_mapPropertyLookup.end())
  {
    const MAPFILEITEMS& propertyMap = it->second;

    if (propertyMap.find(propertyValue) != propertyMap.end())
      return true;
    else
      return false;
  }
  else
  {
    // slow method...
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
      const CFileItemPtr pItem = m_items[i];
      if (pItem->GetProperty(propertyName).Equals(propertyValue))
        return true;
    }
    return false;
  }
}

void CFileItemList::Clear()
{
  CSingleLock lock(m_lock);

  ClearItems();
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
  m_cacheToDisc=CACHE_IF_SLOW;
  m_sortDetails.clear();
  m_replaceListing = false;
  m_content.Empty();
  m_pageContext.Clear();
}

void CFileItemList::ClearItems()
{
  CSingleLock lock(m_lock);
  // make sure we free the memory of the items (these are GUIControls which may have allocated resources)
  FreeMemory();
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr item = m_items[i];
    item->FreeMemory();
  }
  m_items.clear();
  m_map.clear();
}

void CFileItemList::Add(const CFileItemPtr &pItem)
{
  CSingleLock lock(m_lock);

  m_items.push_back(pItem);
  if (m_fastLookup)
  {
    CStdString path(pItem->m_strPath); path.ToLower();
    m_map.insert(MAPFILEITEMSPAIR(path, pItem));
  }
}

void CFileItemList::AddFront(const CFileItemPtr &pItem, int itemPosition)
{
  CSingleLock lock(m_lock);

  if (itemPosition >= 0)
  {
    m_items.insert(m_items.begin()+itemPosition, pItem);
  }
  else
  {
    m_items.insert(m_items.begin()+(m_items.size()+itemPosition), pItem);
  }
  if (m_fastLookup)
  {
    CStdString path(pItem->m_strPath); path.ToLower();
    m_map.insert(MAPFILEITEMSPAIR(path, pItem));
  }
}

void CFileItemList::Remove(CFileItem* pItem)
{
  CSingleLock lock(m_lock);

  for (IVECFILEITEMS it = m_items.begin(); it != m_items.end(); ++it)
  {
    if (pItem == it->get())
    {
      m_items.erase(it);
      if (m_fastLookup)
      {
        CStdString path(pItem->m_strPath); path.ToLower();
        m_map.erase(path);
      }
      break;
    }
  }
}

void CFileItemList::Remove(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem >= 0 && iItem < (int)Size())
  {
    CFileItemPtr pItem = *(m_items.begin() + iItem);
    if (m_fastLookup)
    {
      CStdString path(pItem->m_strPath); path.ToLower();
      m_map.erase(path);
    }
    m_items.erase(m_items.begin() + iItem);
  }
}

void CFileItemList::Append(const CFileItemList& itemlist)
{
  CSingleLock lock(m_lock);

  for (int i = 0; i < itemlist.Size(); ++i)
  {
    // Clone.
    CFileItem* pItem = new CFileItem();
    (*pItem) = *(itemlist[i].get());

    Add(CFileItemPtr(pItem));
  }
}

void CFileItemList::Assign(const CFileItemList& itemlist, bool append)
{
  CSingleLock lock(m_lock);
  if (!append)
    Clear();
  Append(itemlist);
  m_strPath = itemlist.m_strPath;
  m_sortDetails = itemlist.m_sortDetails;
  m_replaceListing = itemlist.m_replaceListing;
  m_content = itemlist.m_content;
  m_mapProperties = itemlist.m_mapProperties;
  m_cacheToDisc = itemlist.m_cacheToDisc;
  m_pageContext = itemlist.m_pageContext;
}

const CFileItemList &CFileItemList::operator=(const CFileItemList& item)
{
  Clear();
  Copy(item);
  return *this;
}

bool CFileItemList::Copy(const CFileItemList& items)
{
  // assign all CFileItem parts
  *(CFileItem*)this = *(CFileItem*)&items;

  // assign the rest of the CFileItemList properties
  m_replaceListing = items.m_replaceListing;
  m_content        = items.m_content;
  m_mapProperties  = items.m_mapProperties;
  m_cacheToDisc    = items.m_cacheToDisc;
  m_sortDetails    = items.m_sortDetails;
  m_sortMethod     = items.m_sortMethod;
  m_sortOrder      = items.m_sortOrder;
  m_pageContext    = items.m_pageContext;

  // make a copy of each item
  for (int i = 0; i < items.Size(); i++)
  {
    //CFileItemPtr newItem(items[i]);
    CFileItemPtr newItem(new CFileItem(*items[i]));
    Add(newItem);
  }

  return true;
}

CFileItemPtr CFileItemList::Get(int iItem)
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int) m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(int iItem) const
{
  CSingleLock lock(m_lock);

  if (iItem > -1 && iItem < (int)m_items.size())
    return m_items[iItem];

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const CStdString& strPath)
{
  CSingleLock lock(m_lock);

  CStdString pathToCheck(strPath); pathToCheck.ToLower();
  if (m_fastLookup)
  {
    IMAPFILEITEMS it=m_map.find(pathToCheck);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_strPath.Equals(pathToCheck))
      return pItem;
  }

  return CFileItemPtr();
}

CFileItemPtr CFileItemList::Get(const CStdString& strPropertyName, const CStdString& strPropertyValue)
{
  CSingleLock lock(m_lock);

  std::map<CStdString, MAPFILEITEMS>::iterator it = m_mapPropertyLookup.find(strPropertyName);
  if (it != m_mapPropertyLookup.end())
  {
    MAPFILEITEMS& propertyMap = it->second;

    IMAPFILEITEMS it2 = propertyMap.find(strPropertyValue);
    if (it2 != propertyMap.end())
      return it2->second;
    else
      return CFileItemPtr();
  }
  else
  {
    // slow method...
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
      const CFileItemPtr pItem = m_items[i];
      if (pItem->GetProperty(strPropertyName).Equals(strPropertyValue))
        return pItem;
    }
    return CFileItemPtr();
  }

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(const CStdString& strPath) const
{
  CSingleLock lock(m_lock);

  CStdString pathToCheck(strPath); pathToCheck.ToLower();
  if (m_fastLookup)
  {
    map<CStdString, CFileItemPtr>::const_iterator it=m_map.find(pathToCheck);
    if (it != m_map.end())
      return it->second;

    return CFileItemPtr();
  }
  // slow method...
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_strPath.Equals(pathToCheck))
      return pItem;
  }

  return CFileItemPtr();
}

const CFileItemPtr CFileItemList::Get(const CStdString& strPropertyName, const CStdString& strPropertyValue) const
{
  CSingleLock lock(m_lock);

  std::map<CStdString, MAPFILEITEMS>::const_iterator it = m_mapPropertyLookup.find(strPropertyName);
  if (it != m_mapPropertyLookup.end())
  {
    const MAPFILEITEMS& propertyMap = it->second;

    map<CStdString, CFileItemPtr>::const_iterator it2 = propertyMap.find(strPropertyValue);
    if (it2 != propertyMap.end())
      return it2->second;
    else
      return CFileItemPtr();
  }
  else
  {
    // slow method...
    for (unsigned int i = 0; i < m_items.size(); i++)
    {
      const CFileItemPtr pItem = m_items[i];
      if (pItem->GetProperty(strPropertyName).Equals(strPropertyValue))
        return pItem;
    }
    return CFileItemPtr();
  }

  return CFileItemPtr();
}

void CFileItemList::AssignTo(CFileItemList& outList, unsigned int start, unsigned int end)
{
  unsigned int itemsSize = m_items.size();
  IVECFILEITEMS itListBegin = m_items.begin() + start;
  IVECFILEITEMS itListEnd = m_items.begin() + end;

  if (start < end && start < itemsSize && end <= itemsSize)
  {
    outList.m_items.insert(outList.m_items.begin(), itListBegin , itListEnd);
  }
  else
  {

  }
}

int CFileItemList::Size() const
{
  CSingleLock lock(m_lock);
  return (int)m_items.size();
}

bool CFileItemList::IsEmpty() const
{
  CSingleLock lock(m_lock);
  return (m_items.size() <= 0);
}

void CFileItemList::Reserve(int iCount)
{
  CSingleLock lock(m_lock);
  m_items.reserve(iCount);
}

void CFileItemList::Sort(FILEITEMLISTCOMPARISONFUNC func)
{
  CSingleLock lock(m_lock);
  std::sort(m_items.begin(), m_items.end(), func);
}

void CFileItemList::FillSortFields(FILEITEMFILLFUNC func)
{
  CSingleLock lock(m_lock);
  std::for_each(m_items.begin(), m_items.end(), func);
}

void CFileItemList::Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder)
{
  //  Already sorted?
  if (sortMethod==m_sortMethod && m_sortOrder==sortOrder) {
    return;
  }

  bool bSorted = false;

  switch (sortMethod)
  {
  case SORT_METHOD_LABEL:
    FillSortFields(SSortFileItem::ByLabel);
    break;
  case SORT_METHOD_LABEL_IGNORE_THE:
    FillSortFields(SSortFileItem::ByLabelNoThe);
    break;
  case SORT_METHOD_DATE:
    FillSortFields(SSortFileItem::ByDate);
    break;
  case SORT_METHOD_SIZE:
    FillSortFields(SSortFileItem::BySize);
    break;
  case SORT_METHOD_DRIVE_TYPE:
    FillSortFields(SSortFileItem::ByDriveType);
    break;
  case SORT_METHOD_TRACKNUM:
    FillSortFields(SSortFileItem::BySongTrackNum);
    break;
  case SORT_METHOD_EPISODE:
    FillSortFields(SSortFileItem::ByEpisodeNum);
    break;
  case SORT_METHOD_DURATION:
    FillSortFields(SSortFileItem::BySongDuration);
    break;
  case SORT_METHOD_TITLE_IGNORE_THE:
    FillSortFields(SSortFileItem::BySongTitleNoThe);
    break;
  case SORT_METHOD_TITLE:
    FillSortFields(SSortFileItem::BySongTitle);
    break;
  case SORT_METHOD_ARTIST:
    FillSortFields(SSortFileItem::BySongArtist);
    break;
  case SORT_METHOD_ARTIST_IGNORE_THE:
    FillSortFields(SSortFileItem::BySongArtistNoThe);
    break;
  case SORT_METHOD_ALBUM:
    FillSortFields(SSortFileItem::BySongAlbum);
    break;
  case SORT_METHOD_ALBUM_IGNORE_THE:
    FillSortFields(SSortFileItem::BySongAlbumNoThe);
    break;
  case SORT_METHOD_GENRE:
    FillSortFields(SSortFileItem::ByGenre);
    break;
  case SORT_METHOD_FILE:
    FillSortFields(SSortFileItem::ByFile);
    break;
  case SORT_METHOD_VIDEO_RATING:
    FillSortFields(SSortFileItem::ByMovieRating);
    break;
  case SORT_METHOD_VIDEO_TITLE:
    FillSortFields(SSortFileItem::ByMovieTitle);
    break;
  case SORT_METHOD_VIDEO_SORT_TITLE:
    FillSortFields(SSortFileItem::ByMovieSortTitle);
    break;
  case SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE:
    FillSortFields(SSortFileItem::ByMovieSortTitleNoThe);
    break;
  case SORT_METHOD_YEAR:
    FillSortFields(SSortFileItem::ByYear);
    break;
  case SORT_METHOD_PRODUCTIONCODE:
    FillSortFields(SSortFileItem::ByProductionCode);
    break;
  case SORT_METHOD_PROGRAM_COUNT:
  case SORT_METHOD_PLAYLIST_ORDER:
    // TODO: Playlist order is hacked into program count variable (not nice, but ok until 2.0)
    FillSortFields(SSortFileItem::ByProgramCount);
    break;
  case SORT_METHOD_SONG_RATING:
    FillSortFields(SSortFileItem::BySongRating);
    break;
  case SORT_METHOD_MPAA_RATING:
    FillSortFields(SSortFileItem::ByMPAARating);
    break;
  case SORT_METHOD_VIDEO_RUNTIME:
    FillSortFields(SSortFileItem::ByMovieRuntime);
    break;
  case SORT_METHOD_STUDIO:
    FillSortFields(SSortFileItem::ByStudio);
    break;
  case SORT_METHOD_STUDIO_IGNORE_THE:
    FillSortFields(SSortFileItem::ByStudioNoThe);
    break;
  case SORT_METHOD_FULLPATH:
    FillSortFields(SSortFileItem::ByFullPath);
    break;
  case SORT_METHOD_DEFAULT:
      FillSortFields(SSortFileItem::ByDefault);
      // In case of SORT_METHOD_DEFAULT and SortOrder was not set -> We want to sort in SORT_ORDER_ASC order
      // that will match the "DefaultSortLabel" that was set to each item
      if(sortOrder == SORT_ORDER_NONE)
      {
        sortOrder = SORT_ORDER_ASC;
      }
      break;
  case SORT_METHOD_LABEL_FILES_FIRST:
      Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelFilesFirstAscending : SSortFileItem::LabelFilesFirstDescending);
      bSorted = true;
      break;
  case SORT_METHOD_LABEL_IGNORE_THE_EXACT:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelAscendingNoTheExact : SSortFileItem::LabelDescendingNoTheExact);
       bSorted = true;
       break;
  case SORT_METHOD_LABEL_EXACT:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelAscendingExact : SSortFileItem::LabelDescendingExact);
       bSorted = true;
       break;
  case SORT_METHOD_LABEL_WITH_SHARES:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::LabelAscendingWithShares : SSortFileItem::LabelDescendingWithShares);
       bSorted = true;
       break;
  case SORT_METHOD_DATE_WITH_SHARES:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DateAscendingWithShares : SSortFileItem::DateDescendingWithShares);
       bSorted = true;
       break;
  case SORT_METHOD_DATE_ADDED:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DateAddedAscending : SSortFileItem::DateAddedDescending);
       bSorted = true;
       break;
  case SORT_METHOD_DATE_MODIFIED:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DateModifiedAscending : SSortFileItem::DateModifiedDescending);
       bSorted = true;
       break;
  case SORT_METHOD_RSS_ITEMS:
       Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::RssItems : SSortFileItem::RssItems);
       bSorted = true;
       break;
  case SORT_METHOD_APP_POPULARITY:
       FillSortFields(SSortFileItem::ByAppPopularity);
       // In case of SORT_METHOD_APP_POPULARITY and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
       // that will match the "popularity" that was set to each item
       if(sortOrder == SORT_ORDER_NONE)
       {
         sortOrder = SORT_ORDER_DESC;
       }
  break;
  case SORT_METHOD_APP_RELEASE_DATE:
       FillSortFields(SSortFileItem::ByAppReleaseDate);
       // In case of SORT_METHOD_APP_RELEASE_DATE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
       // that will match the "popularity" that was set to each item
       if(sortOrder == SORT_ORDER_NONE)
       {
         sortOrder = SORT_ORDER_DESC;
       }
  break;
  case SORT_METHOD_APP_USAGE:
       FillSortFields(SSortFileItem::ByAppUsage);
       // In case of SORT_METHOD_APP_USAGE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
       // that will match the "popularity" that was set to each item
       if(sortOrder == SORT_ORDER_NONE)
       {
         sortOrder = SORT_ORDER_DESC;
       }
  break;
  case SORT_METHOD_APP_LAST_USED_DATE:
       FillSortFields(SSortFileItem::ByAppLastUsedDate);
       // In case of SORT_METHOD_APP_LAST_USED_DATE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
       // that will match the "popularity" that was set to each item
       if(sortOrder == SORT_ORDER_NONE)
       {
         sortOrder = SORT_ORDER_DESC;
       }
  break;
  case SORT_METHOD_RELEASE_DATE:
  {
/*     FillSortFields(SSortFileItem::ByReleaseDate);
     if(sortOrder == SORT_ORDER_NONE)
     {
       sortOrder = SORT_ORDER_DESC;
     }*/
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::ReleaseDateAscending : SSortFileItem::ReleaseDateDescending);
    bSorted = true;
  }
  break;
  case SORT_METHOD_SEARCH_COUNT:
  {
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::SearchResultPopularity : SSortFileItem::SearchResultPopularity);
    bSorted = true;
    break;
  }
  break;
  default:
	  break;
  }
  if (sortMethod == SORT_METHOD_FILE)
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::IgnoreFoldersAscending : SSortFileItem::IgnoreFoldersDescending);
  else if (sortMethod != SORT_METHOD_NONE && !bSorted)
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::Ascending : SSortFileItem::Descending);

  m_sortMethod=sortMethod;
  m_sortOrder=sortOrder;
}

void CFileItemList::Sort(CBoxeeSort& boxeeSort)
{   
  CLog::Log(LOGDEBUG,"CFileItemList::Sort - Enter function with BoxeeSort [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
  
  SORT_ORDER sortOrder = boxeeSort.m_sortOrder; 

  bool needToSort = true;
  
  switch (boxeeSort.m_sortMethod)
  {
  case SORT_METHOD_LABEL:
    FillSortFields(SSortFileItem::ByLabel);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByLabel) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    break;
  case SORT_METHOD_LABEL_IGNORE_THE:
    FillSortFields(SSortFileItem::ByLabelNoThe);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByLabelNoThe) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());    
    break;
  case SORT_METHOD_DATE:
    FillSortFields(SSortFileItem::ByDate);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByDate) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    break;
  case SORT_METHOD_EPISODE:
    FillSortFields(SSortFileItem::ByEpisodeNum);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByEpisodeNum) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    break;
  case SORT_METHOD_YEAR:
    FillSortFields(SSortFileItem::ByYear);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByYear) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    break;
  case SORT_METHOD_ALBUM:
    FillSortFields(SSortFileItem::BySongAlbum);
    break;
  case SORT_METHOD_LINK_TITLE:
    FillSortFields(SSortFileItem::ByLinkTitle);
    break;
  case SORT_METHOD_DEFAULT:
    FillSortFields(SSortFileItem::ByDefault);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByDefault) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_DEFAULT and SortOrder was not set -> We want to sort in SORT_ORDER_ASC order 
    // that will match the "DefaultSortLabel" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_ASC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  case SORT_METHOD_APP_POPULARITY:
    FillSortFields(SSortFileItem::ByAppPopularity);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByPopularity) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_APP_POPULARITY and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
    // that will match the "popularity" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_DESC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  case SORT_METHOD_APP_RELEASE_DATE:
    FillSortFields(SSortFileItem::ByAppReleaseDate);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByAppReleaseDate) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_APP_RELEASE_DATE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
    // that will match the "popularity" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_DESC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  case SORT_METHOD_DATE_ADDED:
    Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::DateAddedAscending : SSortFileItem::DateAddedDescending);
    needToSort = false;
    break;
  case SORT_METHOD_APP_USAGE:
    FillSortFields(SSortFileItem::ByAppUsage);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByAppUsage) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_APP_USAGE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
    // that will match the "popularity" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_DESC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  case SORT_METHOD_APP_LAST_USED_DATE:
    FillSortFields(SSortFileItem::ByAppLastUsedDate);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByAppLastUsedDate) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_APP_LAST_USED_DATE and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
    // that will match the "popularity" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_DESC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  case SORT_METHOD_VIDEO_QUALITY:
    FillSortFields(SSortFileItem::ByVideoQuality);
    CLog::Log(LOGDEBUG,"CFileItemList::Sort - After call to FillSortFields(SSortFileItem::ByViedoQuality) - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    // In case of SORT_METHOD_VIDEO_QUALITY and SortOrder was not set -> We want to sort in SORT_ORDER_DESC order
    // that will match the "popularity" that was set to each item
    if(sortOrder == SORT_ORDER_NONE)
    {
      sortOrder = SORT_ORDER_DESC;
      CLog::Log(LOGDEBUG,"CFileItemList::Sort - Since SortOrder was [SORT_ORDER_NONE] in was changed to [%s=%d] for SortMethod [%s=%d] - [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s]. [path=%s] (vns)(srt)",(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(sortOrder)).c_str(),sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str(),m_strPath.c_str());
    }
    break;
  default:
    CLog::Log(LOGERROR,"CFileItemList::Sort - Failed to find a match to [sortMethod=%s=%d]. Sort on FileList [path=%s] WON'T be execute (vns)",(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,m_strPath.c_str());
    needToSort = false;
    break;
  }

  if(needToSort)
  {
    if((boxeeSort.m_folderPosition).Equals("start"))
    {
      Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::Ascending : SSortFileItem::Descending);      
    }
    else if((boxeeSort.m_folderPosition).Equals("end"))
    {
      Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::AscendingFilesFirst : SSortFileItem::DescendingFilesFirst);      
    }
    else
    {
      // Case of boxeeSort.m_folderPosition == "integral"
    
      Sort(sortOrder==SORT_ORDER_ASC ? SSortFileItem::IgnoreFoldersAscending : SSortFileItem::IgnoreFoldersDescending);
    }
  }  
}

void CFileItemList::Randomize()
{
  CSingleLock lock(m_lock);
  random_shuffle(m_items.begin(), m_items.end());
}

void CFileItemList::Archive(CArchive& ar)
{
  CSingleLock lock(m_lock);
  if (ar.IsStoring())
  {
    CFileItem::Archive(ar);

    // mark this as new-archive
    int nMark = -1;
    ar << nMark;

    int nDumpVersion = 2;
    ar << nDumpVersion;

    int i = 0;
    if (m_items.size() > 0 && m_items[0]->IsParentFolder())
      i = 1;

    ar << (int)(m_items.size() - i);

    ar << m_fastLookup;

    ar << (int)m_sortMethod;
    ar << (int)m_sortOrder;
    ar << (int)m_cacheToDisc;

    ar << (int)m_sortDetails.size();
    for (unsigned int j = 0; j < m_sortDetails.size(); ++j)
    {
      const SORT_METHOD_DETAILS &details = m_sortDetails[j];
      ar << (int)details.m_sortMethod;
      ar << details.m_buttonLabel;
      ar << details.m_labelMasks.m_strLabelFile;
      ar << details.m_labelMasks.m_strLabelFolder;
      ar << details.m_labelMasks.m_strLabel2File;
      ar << details.m_labelMasks.m_strLabel2Folder;
    }

    ar << m_content;

    for (; i < (int)m_items.size(); ++i)
    {
      CFileItemPtr pItem = m_items[i];
      ar << *pItem;
    }
  }
  else
  {
    CFileItemPtr pParent;
    if (!IsEmpty())
    {
      CFileItemPtr pItem=m_items[0];
      if (pItem->IsParentFolder())
        pParent.reset(new CFileItem(*pItem));
    }

    SetFastLookup(false);
    Clear();

    CFileItem::Archive(ar);

    int iSize = 0;
    int nMarker = 0;
    int nVersion = 0;

    ar >> nMarker;
    if (nMarker == -1)
    {
      ar >> nVersion;
      ar >> iSize;
    }
    else
    {
      iSize = nMarker;
      if (iSize > 500) // sanity
        return;
    }

    if (iSize <= 0)
      return ;

    if (pParent)
    {
      m_items.reserve(iSize + 1);
      m_items.push_back(pParent);
    }
    else
      m_items.reserve(iSize);

    bool fastLookup=false;
    ar >> fastLookup;

    int tmp;
    ar >> tmp;
    m_sortMethod = SORT_METHOD(tmp);
    ar >> tmp;
    m_sortOrder = SORT_ORDER(tmp);

    if (nVersion > 0)
    {
      ar >> tmp;
      m_cacheToDisc = CACHE_TYPE(tmp);
    }
    else
    {
      bool bCache = true;
      ar >> bCache;
      if (bCache)
        m_cacheToDisc = CACHE_ALWAYS;
      else
        m_cacheToDisc = CACHE_NEVER;
    }

    unsigned int detailSize = 0;
    ar >> detailSize;
    for (unsigned int j = 0; j < detailSize; ++j)
    {
      SORT_METHOD_DETAILS details;
      ar >> tmp;
      details.m_sortMethod = SORT_METHOD(tmp);
      ar >> details.m_buttonLabel;
      ar >> details.m_labelMasks.m_strLabelFile;
      ar >> details.m_labelMasks.m_strLabelFolder;
      ar >> details.m_labelMasks.m_strLabel2File;
      ar >> details.m_labelMasks.m_strLabel2Folder;
      m_sortDetails.push_back(details);
    }

    ar >> m_content;

    for (int i = 0; i < iSize; ++i)
    {
      CFileItemPtr pItem(new CFileItem);
      ar >> *pItem;
      Add(pItem);
    }

    SetFastLookup(fastLookup);
  }
}

void CFileItemList::FillInDefaultIcons()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->FillInDefaultIcon();
  }
}

void CFileItemList::SetMusicThumbs()
{
  CSingleLock lock(m_lock);
  //cache thumbnails directory
  g_directoryCache.InitMusicThumbCache();

  for (int i = 0; i < (int)m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->SetMusicThumb();
  }

  g_directoryCache.ClearMusicThumbCache();
}

int CFileItemList::GetFolderCount() const
{
  CSingleLock lock(m_lock);
  int nFolderCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->m_bIsFolder)
      nFolderCount++;
  }

  return nFolderCount;
}

int CFileItemList::GetObjectCount() const
{
  CSingleLock lock(m_lock);

  int numObjects = (int)m_items.size();
  if (numObjects && m_items[0]->IsParentFolder())
    numObjects--;

  return numObjects;
}

int CFileItemList::GetFileCount() const
{
  CSingleLock lock(m_lock);
  int nFileCount = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
      nFileCount++;
  }

  return nFileCount;
}

int CFileItemList::GetSelectedCount() const
{
  CSingleLock lock(m_lock);
  int count = 0;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsSelected())
      count++;
  }

  return count;
}

void CFileItemList::FilterCueItems()
{
  CSingleLock lock(m_lock);
  // Handle .CUE sheet files...
  VECSONGS itemstoadd;
  CStdStringArray itemstodelete;
  for (int i = 0; i < (int)m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (!pItem->m_bIsFolder)
    { // see if it's a .CUE sheet
      if (pItem->IsCUESheet())
      {
        CCueDocument cuesheet;
        if (cuesheet.Parse(pItem->m_strPath))
        {
          VECSONGS newitems;
          cuesheet.GetSongs(newitems);

          std::vector<CStdString> MediaFileVec;
          cuesheet.GetMediaFiles(MediaFileVec);

          // queue the cue sheet and the underlying media file for deletion
          for(std::vector<CStdString>::iterator itMedia = MediaFileVec.begin(); itMedia != MediaFileVec.end(); itMedia++)
          {
            CStdString strMediaFile = *itMedia;
            CStdString fileFromCue = strMediaFile; // save the file from the cue we're matching against,
                                                   // as we're going to search for others here...
            bool bFoundMediaFile = CFile::Exists(strMediaFile);
            // queue the cue sheet and the underlying media file for deletion
            if (!bFoundMediaFile)
            {
              // try file in same dir, not matching case...
              if (Contains(strMediaFile))
              {
                bFoundMediaFile = true;
              }
              else
              {
                // try removing the .cue extension...
                strMediaFile = pItem->m_strPath;
                CUtil::RemoveExtension(strMediaFile);
                CFileItem item(strMediaFile, false);
                if (item.IsAudio() && Contains(strMediaFile))
                {
                  bFoundMediaFile = true;
                }
                else
                { // try replacing the extension with one of our allowed ones.
                  CStdStringArray extensions;
                  StringUtils::SplitString(g_stSettings.m_musicExtensions, "|", extensions);
                  for (unsigned int i = 0; i < extensions.size(); i++)
                  {
                    CUtil::ReplaceExtension(pItem->m_strPath, extensions[i], strMediaFile);
                    CFileItem item(strMediaFile, false);
                    if (!item.IsCUESheet() && !item.IsPlayList() && Contains(strMediaFile))
                    {
                      bFoundMediaFile = true;
                      break;
                    }
                  }
                }
              }
            }
            if (bFoundMediaFile)
            {
              itemstodelete.push_back(pItem->m_strPath);
              itemstodelete.push_back(strMediaFile);
              // get the additional stuff (year, genre etc.) from the underlying media files tag.
              CMusicInfoTag tag;
              auto_ptr<IMusicInfoTagLoader> pLoader (CMusicInfoTagLoaderFactory::CreateLoader(strMediaFile));
              if (NULL != pLoader.get())
              {
                // get id3tag
                pLoader->Load(strMediaFile, tag);
              }
              // fill in any missing entries from underlying media file
              for (int j = 0; j < (int)newitems.size(); j++)
              {
                CSong song = newitems[j];
                // only for songs that actually match the current media file
                if (song.strFileName == fileFromCue)
                {
                  // we might have a new media file from the above matching code
                  song.strFileName = strMediaFile;
                  if (tag.Loaded())
                  {
                    if (song.strAlbum.empty() && !tag.GetAlbum().empty()) song.strAlbum = tag.GetAlbum();
                    if (song.strAlbumArtist.empty() && !tag.GetAlbumArtist().empty()) song.strAlbumArtist = tag.GetAlbumArtist();
                    if (song.strGenre.empty() && !tag.GetGenre().empty()) song.strGenre = tag.GetGenre();
                    if (song.strArtist.empty() && !tag.GetArtist().empty()) song.strArtist = tag.GetArtist();
                    if (tag.GetDiscNumber()) song.iTrack |= (tag.GetDiscNumber() << 16); // see CMusicInfoTag::GetDiscNumber()
                    SYSTEMTIME dateTime;
                    tag.GetReleaseDate(dateTime);
                    if (dateTime.wYear) song.iYear = dateTime.wYear;
                  }
                  if (!song.iDuration && tag.GetDuration() > 0)
                  { // must be the last song
                    song.iDuration = (tag.GetDuration() * 75 - song.iStartOffset + 37) / 75;
                  }
                  // add this item to the list
                  itemstoadd.push_back(song);
                }
              }
            }
            else
            { // remove the .cue sheet from the directory
              itemstodelete.push_back(pItem->m_strPath);
            }
          }
        }
        else
        { // remove the .cue sheet from the directory (can't parse it - no point listing it)
          itemstodelete.push_back(pItem->m_strPath);
        }
      }
    }
  }
  // now delete the .CUE files and underlying media files.
  for (int i = 0; i < (int)itemstodelete.size(); i++)
  {
    for (int j = 0; j < (int)m_items.size(); j++)
    {
      CFileItemPtr pItem = m_items[j];
      if (stricmp(pItem->m_strPath.c_str(), itemstodelete[i].c_str()) == 0)
      { // delete this item
        m_items.erase(m_items.begin() + j);
        break;
      }
    }
  }
  // and add the files from the .CUE sheet
  for (int i = 0; i < (int)itemstoadd.size(); i++)
  {
    // now create the file item, and add to the item list.
    CFileItemPtr pItem(new CFileItem(itemstoadd[i]));
    m_items.push_back(pItem);
  }
}

// Remove the extensions from the filenames
void CFileItemList::RemoveExtensions()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->RemoveExtension();
}

void CFileItemList::CleanStrings()
{
  CSingleLock lock(m_lock);
  for (int i = 0; i < Size(); ++i)
    m_items[i]->CleanString();
}

void CFileItemList::Stack()
{
  CSingleLock lock(m_lock);

  // not allowed here
  if (IsVirtualDirectoryRoot() || IsLiveTV())
    return;

  // items needs to be sorted for stuff below to work properly
  Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

  // stack folders
  bool isDVDFolder(false);
  int i = 0;
  for (i = 0; i < Size(); ++i)
  {
    CFileItemPtr item = Get(i);
    if (item->GetLabel().Equals("VIDEO_TS.IFO"))
    {
      isDVDFolder = true;
      break;
    }
    // combined the folder checks
    if (item->m_bIsFolder)
    {
      // only check known fast sources?
      // xbms included because it supports file existance
      // NOTES:
      // 1. xbms would not have worked previously: item->m_strPath.Left(5).Equals("xbms", false)
      // 2. rars and zips may be on slow sources? is this supposed to be allowed?
      if( !item->IsRemote()
        || item->IsSmb()
        || item->m_strPath.Left(7).Equals("xbms://")
        || CUtil::IsInRAR(item->m_strPath)
        || CUtil::IsInZIP(item->m_strPath)
        )
      {
        // stack cd# folders if contains only a single video file
        // NOTE: if we're doing this anyway, why not collapse *all* folders with just a single video file?
        CStdString folderName = item->GetLabel();
        if (folderName.Left(2).Equals("CD") && StringUtils::IsNaturalNumber(folderName.Mid(2)))
        {
          CFileItemList items;
          CDirectory::GetDirectory(item->m_strPath,items,g_stSettings.m_videoExtensions,true);
          // optimized to only traverse listing once by checking for filecount
          // and recording last file item for later use
          int nFiles = 0;
          int index = -1;
            for (int j = 0; j < items.Size(); j++)
            {
              if (!items[j]->m_bIsFolder)
              {
              nFiles++;
              index = j;
            }
            if (nFiles > 1)
                break;
              }
          if (nFiles == 1)
          {
            *item = *items[index];
          }
        }

        // check for dvd folders
        else
        {
          CStdString path;
          CStdString dvdPath;
          CUtil::AddFileToFolder(item->m_strPath, "VIDEO_TS.IFO", path);
          if (CFile::Exists(path))
            dvdPath = path;
          else
          {
            CUtil::AddFileToFolder(item->m_strPath, "VIDEO_TS", dvdPath);
            CUtil::AddFileToFolder(dvdPath, "VIDEO_TS.IFO", path);
            dvdPath.Empty();
            if (CFile::Exists(path))
              dvdPath = path;
          }
          if (!dvdPath.IsEmpty())
          {
            // NOTE: should this be done for the CD# folders too?
            /* set the thumbnail based on folder */
            item->SetCachedVideoThumb();
            if (!item->HasThumbnail())
              item->SetUserVideoThumb();

            item->m_bIsFolder = false;
            item->m_strPath = dvdPath;
            item->SetLabel2("");
            item->SetLabelPreformated(true);
            m_sortMethod = SORT_METHOD_NONE; /* sorting is now broken */

            /* override the previously set thumb if video_ts.ifo has any */
            /* otherwise user can't set icon on the stacked file as that */
            /* will allways be set on the video_ts.ifo file */
            CStdString thumb(item->GetCachedVideoThumb());
            if(CFile::Exists(thumb))
              item->SetThumbnailImage(thumb);
            else
              item->SetUserVideoThumb();
          }
        }
      }
    }
  }


  // now stack the files, some of which may be from the previous stack iteration
  i = 0;
  while (i < Size())
  {
    CFileItemPtr item = Get(i);

    // set property
    item->SetProperty("isstacked", "1");

    // skip folders, nfo files, playlists, dvd images
    if (item->m_bIsFolder
      || item->IsParentFolder()
      || item->IsNFO()
      || item->IsPlayList()
      || item->IsDVDImage()
      )
    {
      // increment index
      i++;
      continue;
    }

    if (isDVDFolder)
    {
      // remove any other ifo files in this folder
      if (item->IsDVDFile(false, true) && !item->GetLabel().Equals("VIDEO_TS.IFO"))
      {
        Remove(i);
        continue;
      }
    }

    CStdString fileName, filePath;
    CUtil::Split(item->m_strPath, filePath, fileName);
    CStdString fileTitle, volumeNumber;
    // hmmm... should this use GetLabel() or fileName?
    if (CUtil::GetVolumeFromFileName(item->GetLabel(), fileTitle, volumeNumber))
    {
      vector<int> stack;
      stack.push_back(i);
      int64_t size = item->m_dwSize;

      int j = i + 1;
      while (j < Size())
      {
        CFileItemPtr item2 = Get(j);
        CStdString fileName2, filePath2;
        CUtil::Split(item2->m_strPath, filePath2, fileName2);
        // only do a stacking comparison if the first letter of the filename is the same
        if (fileName2.size() && fileName2.at(0) != fileName.at(0))
          break;

        CStdString fileTitle2, volumeNumber2;
        // hmmm... should this use GetLabel() or fileName2?
        if (CUtil::GetVolumeFromFileName(item2->GetLabel(), fileTitle2, volumeNumber2))
        {
          if (fileTitle2.Equals(fileTitle))
          {
              stack.push_back(j);
              size += item2->m_dwSize;
          }
        }

        // increment index
        j++;
      }

      if (stack.size() > 1)
      {
          // have a stack, remove the items and add the stacked item
          CStackDirectory dir;
          // dont actually stack a multipart rar set, just remove all items but the first
          CStdString stackPath;
          if (Get(stack[0])->IsRAR())
            stackPath = Get(stack[0])->m_strPath;
          else
            stackPath = dir.ConstructStackPath(*this, stack);
          item->m_strPath = stackPath;
        // clean up list
        for (unsigned int k = stack.size() - 1; k > 0; --k)
        {
          Remove(stack[k]);
        }
        // item->m_bIsFolder = true;  // don't treat stacked files as folders
        // the label may be in a different char set from the filename (eg over smb
        // the label is converted from utf8, but the filename is not)
        CUtil::GetVolumeFromFileName(item->GetLabel(), fileTitle, volumeNumber);
        if (g_guiSettings.GetBool("filelists.hideextensions"))
          CUtil::RemoveExtension(fileTitle);
        item->SetLabel(fileTitle);
        item->m_dwSize = size;
      }
    }

    // increment index
    i++;
  }
}

bool CFileItemList::Load()
{
  if (m_strPath.IsEmpty())
    return false;

  CFile file;
  if (file.Open(GetDiscCacheFile()))
  {
    CLog::Log(LOGDEBUG,"Loading fileitems [%s]",m_strPath.c_str());
    CArchive ar(&file, CArchive::load);
    ar >> *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, directory: %s sort method: %i, ascending: %s",Size(),m_strPath.c_str(), m_sortMethod, m_sortOrder ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

bool CFileItemList::Load(const CStdString& filePath)
{
  if(filePath.IsEmpty())
  {
    CLog::Log(LOGERROR,"Failed to load FileItemLIst from file because empty FilePath was received (hs)");

    return false;
  }

  CFile file;

  if (file.Open(filePath))
  {
    CArchive ar(&file, CArchive::load);
    ar >> *this;
    ar.Close();
    file.Close();

    CLog::Log(LOGDEBUG,"FileItemLIst was loaded from file [%s] (hs)",filePath.c_str());

    return true;
  }
  else
  {
    CLog::Log(LOGERROR,"Failed to open file [%s] for loading FileItemList (hs)",filePath.c_str());

    return false;
  }
}

bool CFileItemList::Save()
{
  int iSize = Size();
  if (iSize <= 0)
    return false;

  CLog::Log(LOGDEBUG,"Saving fileitems [%s]",m_strPath.c_str());

  CFile file;
  if (file.OpenForWrite(GetDiscCacheFile(), true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << *this;
    CLog::Log(LOGDEBUG,"  -- items: %i, sort method: %i, ascending: %s",iSize,m_sortMethod, m_sortOrder ? "true" : "false");
    ar.Close();
    file.Close();
    return true;
  }

  return false;
}

bool CFileItemList::Save(const CStdString& filePath)
{
  if(filePath.IsEmpty())
  {
    CLog::Log(LOGERROR,"Failed to save FileItemLIst because empty file path was received (hs)");

    return false;
  }

  CFile file;

  if (file.OpenForWrite(filePath, true)) // overwrite always
  {
    CArchive ar(&file, CArchive::store);
    ar << *this;
    ar.Close();
    file.Close();

    CLog::Log(LOGDEBUG,"FileItemLIst was saved to file [%s] (hs)",filePath.c_str());

    return true;
  }

    CLog::Log(LOGERROR,"Failed to open file [%s] for saving FileItemList (hs)",filePath.c_str());
    return false;
  }

void CFileItemList::RemoveDiscCache() const
{
  CLog::Log(LOGDEBUG,"Clearing cached fileitems [%s]",m_strPath.c_str());
  if (CFile::Exists(GetDiscCacheFile()))
    CFile::Delete(GetDiscCacheFile());
}

CStdString CFileItemList::GetDiscCacheFile() const
{
  CStdString strPath=m_strPath;
  CUtil::RemoveSlashAtEnd(strPath);

  Crc32 crc;
  crc.ComputeFromLowerCase(strPath);

  CStdString cacheFile;
  if (IsCDDA() || IsOnDVD())
    cacheFile.Format("special://temp/r-%08x.fi", (unsigned __int32)crc);
  else if (IsMusicDb())
    cacheFile.Format("special://temp/mdb-%08x.fi", (unsigned __int32)crc);
  else if (IsVideoDb())
    cacheFile.Format("special://temp/vdb-%08x.fi", (unsigned __int32)crc);
  else
    cacheFile.Format("special://temp/%08x.fi", (unsigned __int32)crc);
  return cacheFile;
}

bool CFileItemList::AlwaysCache() const
{
  // some database folders are always cached
  if (IsMusicDb())
    return CMusicDatabaseDirectory::CanCache(m_strPath);
  if (IsVideoDb())
    return CVideoDatabaseDirectory::CanCache(m_strPath);
  return false;
}

void CFileItemList::SetCachedVideoThumbs()
{
  CSingleLock lock(m_lock);
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->SetCachedVideoThumb();
  }
}

void CFileItemList::SetCachedProgramThumbs()
{
  CSingleLock lock(m_lock);
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->SetCachedProgramThumb();
  }
}

void CFileItemList::SetCachedMusicThumbs()
{
  CSingleLock lock(m_lock);
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->SetCachedMusicThumb();
  }
}

CStdString CFileItem::GetCachedPictureThumb() const
{
  CStdString cachedPictureThumb = StringUtils::EmptyString;

  CStdString thumbPath = GetThumbnailImage();

  if (!thumbPath.IsEmpty())
  {
    cachedPictureThumb = GetCachedThumb(thumbPath,g_settings.GetPicturesThumbFolder(),true);
}
  else if (!m_strPath.IsEmpty() && (m_bIsFolder || IsPicture()))
  {
    cachedPictureThumb = GetCachedThumb(m_strPath,g_settings.GetPicturesThumbFolder(),true);
  }

  return cachedPictureThumb;
}

void CFileItem::SetCachedMusicThumb()
{
  // if it already has a thumbnail, then return
  if (HasThumbnail() || m_bIsShareOrDrive) return ;

  // streams do not have thumbnails
  if (IsInternetStream()) return ;

  //  music db items already have thumbs or there is no thumb available
  if (IsMusicDb()) return;

  // ignore the parent dir items
  if (IsParentFolder()) return;

  CStdString cachedThumb(GetPreviouslyCachedMusicThumb());
  if (!cachedThumb.IsEmpty())
    SetThumbnailImage(cachedThumb);
    // SetIconImage(cachedThumb);
}

CStdString CFileItem::GetPreviouslyCachedMusicThumb() const
{
  // look if an album thumb is available,
  // could be any file with tags loaded or
  // a directory in album window
  CStdString strAlbum, strArtist;
  if (HasMusicInfoTag() && m_musicInfoTag->Loaded())
  {
    strAlbum = m_musicInfoTag->GetAlbum();
    if (!m_musicInfoTag->GetAlbumArtist().IsEmpty())
      strArtist = m_musicInfoTag->GetAlbumArtist();
    else
      strArtist = m_musicInfoTag->GetArtist();
  }
  if (!strAlbum.IsEmpty() && !strArtist.IsEmpty())
  {
    // try permanent album thumb using "album name + artist name"
    CStdString thumb(CUtil::GetCachedAlbumThumb(strAlbum, strArtist));
    if (CFile::Exists(thumb))
      return thumb;
  }

  // if a file, try to find a cached filename.tbn
  if (!m_bIsFolder)
  {
    // look for locally cached tbn
    CStdString thumb(CUtil::GetCachedMusicThumb(m_strPath));
    if (CFile::Exists(thumb))
      return thumb;
  }

  // try and find a cached folder thumb (folder.jpg or folder.tbn)
  CStdString strPath;
  if (!m_bIsFolder)
    CUtil::GetDirectory(m_strPath, strPath);
  else
    strPath = m_strPath;
  // music thumbs are cached without slash at end
  CUtil::RemoveSlashAtEnd(strPath);

  CStdString thumb(CUtil::GetCachedMusicThumb(strPath));
  if (CFile::Exists(thumb))
    return thumb;

  return "";
}

CStdString CFileItem::GetUserMusicThumb(bool alwaysCheckRemote /* = false */) const
{
  if (m_bIsShareOrDrive) return "";
  if (IsInternetStream()) return "";
  if (IsParentFolder()) return "";
  if (IsMusicDb()) return "";
	
	// In case this is a BoxeeDB album, use album folder path
  CStdString strPath;
	if (IsBoxeeDb())
	{
	  strPath = GetProperty("AlbumFolderPath");
	}
	else
	{
	  strPath = m_strPath;
	}
	
	if (strPath == "") return "";
	
  CURI url(strPath);
  if (url.GetProtocol() == "rar" || url.GetProtocol() == "zip") return "";
  if (url.GetProtocol() == "upnp" || url.GetProtocol() == "ftp" || url.GetProtocol() == "ftps") return "";

  // we first check for <filename>.tbn or <foldername>.tbn
  CStdString fileThumb(GetTBNFile());
  if (CFile::Exists(fileThumb))
    return fileThumb;
  // if a folder, check for folder.jpg
  if (m_bIsFolder && (!IsRemote() || alwaysCheckRemote || g_guiSettings.GetBool("musicfiles.findremotethumbs")))
  {
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_musicThumbs, "|", thumbs);
    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      CStdString folderThumb;
      if (IsBoxeeDb()) 
      {
        CUtil::AddFileToFolder(strPath, thumbs[i], folderThumb);
      }
      else
      {
        folderThumb = GetFolderThumb(thumbs[i]);
      }
      
      if (CFile::Exists(folderThumb))
      {
        return folderThumb;
      }
    }
  }
  // this adds support for files which inherit a folder.jpg icon which has not been cached yet.
  // this occurs when queueing a top-level folder which has not been traversed yet.
  else if (!IsRemote() || alwaysCheckRemote || g_guiSettings.GetBool("musicfiles.findremotethumbs"))
  {
    CStdString strFolder, strFile;
    CUtil::Split(strPath, strFolder, strFile);
    CFileItem folderItem(strFolder, true);
    folderItem.SetMusicThumb(alwaysCheckRemote);
    if (folderItem.HasThumbnail())
      return folderItem.GetThumbnailImage();
  }
  // No thumb found
  return "";
}

void CFileItem::SetUserMusicThumb(bool alwaysCheckRemote /* = false */)
{
  // caches as the local thumb
  CStdString thumb(GetUserMusicThumb(alwaysCheckRemote));
  if (!thumb.IsEmpty())
  {
    CStdString cachedThumb(CUtil::GetCachedMusicThumb(m_strPath));
    CPicture::CreateThumbnail(thumb, cachedThumb);
  }

  SetCachedMusicThumb();
}

void CFileItem::SetCachedPictureThumb()
{
  if (IsParentFolder()) return;
  CStdString cachedThumb(GetCachedPictureThumb());
  if (CFile::Exists(cachedThumb))
  {
    SetThumbnailImage(cachedThumb);
}
}

CStdString CFileItem::GetCachedVideoThumb() const
{
  if (IsStack())
    return GetCachedThumb(CStackDirectory::GetFirstStackedFile(m_strPath),g_settings.GetVideoThumbFolder(),true);
  else
    return GetCachedThumb(m_strPath,g_settings.GetVideoThumbFolder(),true);
  }

CStdString CFileItem::GetCachedEpisodeThumb() const
{
  // get the locally cached thumb
  CStdString strCRC;
  strCRC.Format("%sepisode%i",GetVideoInfoTag()->m_strFileNameAndPath.c_str(),GetVideoInfoTag()->m_iEpisode);
  return GetCachedThumb(strCRC,g_settings.GetVideoThumbFolder(),true);
}

void CFileItem::SetCachedVideoThumb()
{
  if (IsParentFolder()) return;
  CStdString cachedThumb(GetCachedVideoThumb());
  if (CFile::Exists(cachedThumb))
    SetThumbnailImage(cachedThumb);
}

// Gets the .tbn filename from a file or folder name.
// <filename>.ext -> <filename>.tbn
// <foldername>/ -> <foldername>.tbn
CStdString CFileItem::GetTBNFile() const
{
  CStdString thumbFile;
  CStdString strFile = m_strPath;

  if (IsStack())
  {
    CStdString strPath, strReturn;
    CUtil::GetParentPath(m_strPath,strPath);
    CFileItem item(CStackDirectory::GetFirstStackedFile(strFile),false);
    CStdString strTBNFile = item.GetTBNFile();
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(strTBNFile),strReturn);
    if (CFile::Exists(strReturn))
      return strReturn;

    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(CStackDirectory::GetStackedTitlePath(strFile)),strFile);
  }

  if (CUtil::IsInRAR(strFile) || CUtil::IsInZIP(strFile))
  {
    CStdString strPath, strParent;
    CUtil::GetDirectory(strFile,strPath);
    CUtil::GetParentPath(strPath,strParent);
    CUtil::AddFileToFolder(strParent,CUtil::GetFileName(m_strPath),strFile);
  }

  CURI url(strFile);
  strFile = url.GetFileName();

  if (m_bIsFolder && !IsFileFolder())
    CUtil::RemoveSlashAtEnd(strFile);

  if (!strFile.IsEmpty())
  {
    if (m_bIsFolder && !IsFileFolder())
      thumbFile = strFile + ".tbn"; // folder, so just add ".tbn"
  else
    CUtil::ReplaceExtension(strFile, ".tbn", thumbFile);
    url.SetFileName(thumbFile);
    thumbFile = url.Get();
  }
  return thumbFile;
}

CStdString CFileItem::GetUserVideoThumb() const
{
#ifdef HAS_FILESYSTEM_TUXBOX
  if (IsTuxBox())
  {
    if (!m_bIsFolder)
      return g_tuxbox.GetPicon(GetLabel());
    else return "";
  }
#endif

  if (m_strPath.IsEmpty()
  || m_bIsShareOrDrive
  || IsInternetStream()
  || CUtil::IsUPnP(m_strPath)
  || IsParentFolder()
  || IsLiveTV())
    return "";


  // 1. check <filename>.tbn or <foldername>.tbn
  CStdString fileThumb(GetTBNFile());
  if (CFile::Exists(fileThumb))
    return fileThumb;

  // 2. - check movie.tbn, as long as it's not a folder
  if (!m_bIsFolder)
  {
    CStdString strPath, movietbnFile;
    CUtil::GetParentPath(m_strPath, strPath);
    //CUtil::AddFileToFolder(strPath, "movie.tbn", movietbnFile);
    CUtil::AddFileToFolder(strPath, "folder.jpg", movietbnFile);
    if (CFile::Exists(movietbnFile))
      return movietbnFile;
  }

  // 3. check folder image in_m_dvdThumbs (folder.jpg)
  if (m_bIsFolder)
  {
    CStdStringArray thumbs;
    StringUtils::SplitString(g_advancedSettings.m_dvdThumbs, "|", thumbs);
    for (unsigned int i = 0; i < thumbs.size(); ++i)
    {
      CStdString folderThumb(GetFolderThumb(thumbs[i]));
    if (CFile::Exists(folderThumb))
      {
      return folderThumb;
  }
    }
  }
  // No thumb found
  return "";
}

CStdString CFileItem::GetFolderThumb(const CStdString &folderJPG /* = "folder.jpg" */) const
{
  CStdString folderThumb;
  CStdString strFolder = m_strPath;

  if (IsStack())
  {
    CStdString strPath;
    CUtil::GetParentPath(m_strPath,strPath);
    CStdString strFolder = CStackDirectory::GetStackedTitlePath(m_strPath);
  }

  if (CUtil::IsInRAR(strFolder) || CUtil::IsInZIP(strFolder))
  {
    CStdString strPath, strParent;
    CUtil::GetDirectory(strFolder,strPath);
    CUtil::GetParentPath(strPath,strParent);
  }

  if (IsMultiPath())
    strFolder = CMultiPathDirectory::GetFirstPath(m_strPath);

  CUtil::AddFileToFolder(strFolder, folderJPG, folderThumb);
  return folderThumb;
}

CStdString CFileItem::GetMovieName(bool bUseFolderNames /* = false */) const
{
  if (IsLabelPreformated())
    return GetLabel();
  
  CStdString strMovieName = m_strPath;

  if (IsMultiPath())
    strMovieName = CMultiPathDirectory::GetFirstPath(m_strPath);

  if (CUtil::IsStack(strMovieName))
    strMovieName = CStackDirectory::GetStackedTitlePath(strMovieName);

  if ((!m_bIsFolder || IsDVDFile(false, true) || CUtil::IsInArchive(m_strPath)) && bUseFolderNames)
  {
    CUtil::GetParentPath(m_strPath, strMovieName);
    if (CUtil::IsInArchive(m_strPath) || strMovieName.Find( "VIDEO_TS" ) != -1)
    {
      CStdString strArchivePath;
      CUtil::GetParentPath(strMovieName, strArchivePath);
      strMovieName = strArchivePath;
    }
  }

  CUtil::RemoveSlashAtEnd(strMovieName);
  strMovieName = CUtil::GetFileName(strMovieName);

  return strMovieName;
}

void CFileItem::SetVideoThumb()
{
  if (HasThumbnail()) return;
  SetCachedVideoThumb();
  if (!HasThumbnail())
    SetUserVideoThumb();
}

void CFileItem::SetUserVideoThumb()
{
  if (m_bIsShareOrDrive) return;
  if (IsParentFolder()) return;

  // caches as the local thumb
  CStdString thumb(GetUserVideoThumb());
  SetProperty("OriginalThumb", thumb);
  if (!thumb.IsEmpty())
  {
    CStdString cachedThumb(GetCachedVideoThumb());
    CPicture::CreateThumbnail(thumb, cachedThumb);
  }
  SetCachedVideoThumb();
}

///
/// If a cached fanart image already exists, then we're fine.  Otherwise, we look for a local fanart.jpg
/// and cache that image as our fanart.
CStdString CFileItem::CacheFanart(bool probe) const
{

  // For Boxee use only - don't do nothing at this function.
  // since currently we don't support fanArt images.
  // a problem was occur when we retrieve SMB directory content without file extended information, and cached it.
  // other threads that need the extended information (such as file size and date) wont get it.
  // So since we don't support it we neutralize it
  return StringUtils::EmptyString;

  if (IsVideoDb())
  {
    if (!HasVideoInfoTag())
      return ""; // nothing can be done
    CFileItem dbItem(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath, m_bIsFolder);
    return dbItem.CacheFanart();
  }

  CStdString cachedFanart(GetCachedFanart());
  if (!probe)
  {
    // first check for an already cached fanart image
  if (CFile::Exists(cachedFanart))
      return "";
  }

  CStdString strFile2;
  CStdString strFile = m_strPath;
  if (IsStack())
  {
    CStdString strPath;
    CUtil::GetParentPath(m_strPath,strPath);
    CStackDirectory dir;
    CStdString strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(strPath2),strFile);
    CFileItem item(dir.GetFirstStackedFile(m_strPath),false);
    CStdString strTBNFile = item.GetTBNFile();
    CUtil::ReplaceExtension(strTBNFile, "-fanart",strTBNFile);
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(strTBNFile),strFile2);
  }
  if (CUtil::IsInRAR(strFile) || CUtil::IsInZIP(strFile))
  {
    CStdString strPath, strParent;
    CUtil::GetDirectory(strFile,strPath);
    CUtil::GetParentPath(strPath,strParent);
    CUtil::AddFileToFolder(strParent,CUtil::GetFileName(m_strPath),strFile);
  }

  // no local fanart available for these
  if (IsInternetStream()
  || CUtil::IsUPnP(strFile)
  || IsLiveTV() 
  || IsPlugin() 
  || CUtil::IsFTP(strFile)
  || m_strPath.IsEmpty())
    return "";

  CStdString localFanart;

  // we don't have a cached image, so let's see if the user has a local image ..
  CStdString strDir;
  CUtil::GetDirectory(strFile, strDir);

  if (strDir.IsEmpty())
    return "";

  CFileItemList items;

  if (CUtil::IsHD(strDir) || ( CUtil::IsSmb(strDir) && g_application.IsPathAvailable(strDir, false)))
  CDirectory::GetDirectory(strDir, items, g_stSettings.m_pictureExtensions, false, false, DIR_CACHE_ALWAYS, false);

  CStdStringArray fanarts;
  StringUtils::SplitString(g_advancedSettings.m_fanartImages, "|", fanarts);

  CUtil::RemoveExtension(strFile);
  strFile += "-fanart";
  fanarts.push_back(CUtil::GetFileName(strFile));

  if (!strFile2.IsEmpty())
    fanarts.push_back(CUtil::GetFileName(strFile2));

  for (unsigned int i = 0; i < fanarts.size(); ++i)
  {
    for (int j = 0; j < items.Size(); j++)
    {
      CStdString strCandidate = CUtil::GetFileName(items[j]->m_strPath);
    CUtil::RemoveExtension(strCandidate);
      CStdString strFanart = fanarts[i];
      CUtil::RemoveExtension(strFanart);
      if (strCandidate.CompareNoCase(strFanart) == 0)
    {
        localFanart = items[j]->m_strPath;
      break;
    }
  }
    // exit main loop if fanart was found
    if (!localFanart.IsEmpty())
      break;
  }

  // no local fanart found
  if(localFanart.IsEmpty())
    return "";

  if (!probe)
    CPicture::CacheImage(localFanart, cachedFanart);

  return localFanart;
}

CStdString CFileItem::GetCachedFanart() const
{
  // get the locally cached thumb
  if (IsVideoDb())
  {
    if (!HasVideoInfoTag())
      return "";
    if (!GetVideoInfoTag()->m_strArtist.IsEmpty())
      return GetCachedThumb(GetVideoInfoTag()->m_strArtist,g_settings.GetMusicFanartFolder());
    if (!m_bIsFolder && !GetVideoInfoTag()->m_strShowTitle.IsEmpty())
    {
      CVideoDatabase database;
      database.Open();
      int iShowId = database.GetTvShowId(GetVideoInfoTag()->m_strPath);
      CStdString showPath;
      database.GetFilePathById(iShowId,showPath,VIDEODB_CONTENT_TVSHOWS);
      return GetCachedThumb(showPath,g_settings.GetVideoFanartFolder()); 
  }
    
    return GetCachedThumb(m_bIsFolder ? GetVideoInfoTag()->m_strPath : GetVideoInfoTag()->m_strFileNameAndPath,g_settings.GetVideoFanartFolder());
}

  if (HasMusicInfoTag())
{
    return GetCachedThumb(GetMusicInfoTag()->GetArtist(),g_settings.GetMusicFanartFolder());
  }

  return GetCachedThumb(m_strPath,g_settings.GetVideoFanartFolder());
}

CStdString CFileItem::GetCachedThumb(const CStdString &path, const CStdString &path2, bool split)
{
  if (path.length() == 0)
  {
    return path;
  }

  // get the locally cached thumb
  Crc32 crc;
  crc.ComputeFromLowerCase(path);

  CStdString thumb;
  if (split)
  {
    CStdString hex;
    hex.Format("%08x", (__int32)crc);
    thumb.Format("%c\\%08x.tbn", hex[0], (unsigned __int32)crc);
  }
  else
    thumb.Format("%08x.tbn", (unsigned __int32)crc);

  return CUtil::AddFileToFolder(path2, thumb);
}

CStdString CFileItem::GetCachedProgramThumb() const
{
  return GetCachedThumb(m_strPath,g_settings.GetProgramsThumbFolder());
    }

CStdString CFileItem::GetCachedGameSaveThumb() const
    {
  return "";
}

void CFileItem::SetCachedProgramThumb()
{
  // don't set any thumb for programs on DVD, as they're bound to be named the
  // same (D:\default.xbe).
  if (IsParentFolder()) return;
  CStdString thumb(GetCachedProgramThumb());
  if (CFile::Exists(thumb))
    SetThumbnailImage(thumb);
}

void CFileItem::SetUserProgramThumb()
{
  if (m_bIsShareOrDrive) return;
  if (IsParentFolder()) return;

  if (IsShortCut())
  {
    CShortcut shortcut;
    if ( shortcut.Create( m_strPath ) )
    {
      // use the shortcut's thumb
      if (!shortcut.m_strThumb.IsEmpty())
        m_strThumbnailImage = shortcut.m_strThumb;
      else
      {
        CFileItem item(shortcut.m_strPath,false);
        item.SetUserProgramThumb();
        m_strThumbnailImage = item.m_strThumbnailImage;
      }
      return;
    }
  }
  // 1.  Try <filename>.tbn
  CStdString fileThumb(GetTBNFile());
  CStdString thumb(GetCachedProgramThumb());
  if (CFile::Exists(fileThumb))
  { // cache
    if (CPicture::CreateThumbnail(fileThumb, thumb))
      SetThumbnailImage(thumb);
  }
  else if (m_bIsFolder)
  {
    // 3. cache the folder image
    CStdString folderThumb(GetFolderThumb());
    if (CFile::Exists(folderThumb))
    {
      if (CPicture::CreateThumbnail(folderThumb, thumb))
        SetThumbnailImage(thumb);
    }
  }
}

/*void CFileItem::SetThumb()
{
  // we need to know the type of file at this point
  // as differing views have differing inheritance rules for thumbs:

  // Videos:
  // Folders only use <foldername>/folder.jpg or <foldername>.tbn
  // Files use <filename>.tbn
  //  * Thumbs are cached from here using file or folder path

  // Music:
  // Folders only use <foldername>/folder.jpg or <foldername>.tbn
  // Files use <filename>.tbn or the album/path cached thumb or inherit from the folder
  //  * Thumbs are cached from here using file or folder path

  // Programs:
  // Folders only use <foldername>/folder.jpg or <foldername>.tbn
  // Files use <filename>.tbn or the embedded xbe.  Shortcuts have the additional step of the <thumbnail> tag to check
  //  * Thumbs are cached from here using file or folder path

  // Pictures:
  // Folders use <foldername>/folder.jpg or <foldername>.tbn, or auto-generated from 4 images in the folder
  // Files use <filename>.tbn or a resized version of the picture
  //  * Thumbs are cached from here using file or folder path

}*/

void CFileItemList::SetProgramThumbs()
{
  // TODO: Is there a speed up if we cache the program thumbs first?
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsParentFolder())
      continue;
    pItem->SetCachedProgramThumb();
    if (!pItem->HasThumbnail())
      pItem->SetUserProgramThumb();
  }
}

bool CFileItem::LoadMusicTag()
{
  // not audio
  if (!IsAudio())
    return false;
  // already loaded?
  if (HasMusicInfoTag() && m_musicInfoTag->Loaded())
    return true;
  // check db
  CMusicDatabase musicDatabase;
  if (musicDatabase.Open())
  {
    CSong song;
    if (musicDatabase.GetSongByFileName(m_strPath, song))
    {
      GetMusicInfoTag()->SetSong(song);
      SetThumbnailImage(song.strThumb);
      return true;
    }
    musicDatabase.Close();
  }
  // load tag from file
  CLog::Log(LOGDEBUG, "%s: loading tag information for file: %s", __FUNCTION__, m_strPath.c_str());
  CMusicInfoTagLoaderFactory factory;
  auto_ptr<IMusicInfoTagLoader> pLoader (factory.CreateLoader(m_strPath));
  if (NULL != pLoader.get())
  {
    if (pLoader->Load(m_strPath, *GetMusicInfoTag()))
      return true;
  }
  // no tag - try some other things
  if (IsCDDA())
  {
    // we have the tracknumber...
    int iTrack = GetMusicInfoTag()->GetTrackNumber();
    if (iTrack >= 1)
    {
      CStdString strText = g_localizeStrings.Get(554); // "Track"
      if (strText.GetAt(strText.size() - 1) != ' ')
        strText += " ";
      CStdString strTrack;
      strTrack.Format(strText + "%i", iTrack);
      GetMusicInfoTag()->SetTitle(strTrack);
      GetMusicInfoTag()->SetLoaded(true);
      return true;
    }
  }
  else
  {
    CStdString fileName = CUtil::GetFileName(m_strPath);
    CUtil::RemoveExtension(fileName);
    for (unsigned int i = 0; i < g_advancedSettings.m_musicTagsFromFileFilters.size(); i++)
    {
      CLabelFormatter formatter(g_advancedSettings.m_musicTagsFromFileFilters[i], "");
      if (formatter.FillMusicTag(fileName, GetMusicInfoTag()))
      {
        GetMusicInfoTag()->SetLoaded(true);
        return true;
      }
    }
  }
  return false;
}

bool CFileItem::LoadVideoTag()
{
  // not audio
  if (!IsVideo())
    return false;

  // check db
  CVideoDatabase database;
  database.Open();
  bool bHasInfo = database.HasMovieInfo(m_strPath);
  if (bHasInfo)
    database.GetMovieInfo(m_strPath, *GetVideoInfoTag());
  database.Close();
  return bHasInfo;
}

void CFileItem::SetCachedGameSavesThumb()
{
  if (IsParentFolder()) return;
  CStdString thumb(GetCachedGameSaveThumb());
  if (CFile::Exists(thumb))
    SetThumbnailImage(thumb);
}

void CFileItemList::SetCachedGameSavesThumbs()
{
  // TODO: Investigate caching time to see if it speeds things up
  for (unsigned int i = 0; i < m_items.size(); ++i)
  {
    CFileItemPtr pItem = m_items[i];
    pItem->SetCachedGameSavesThumb();
  }
}

void CFileItemList::SetGameSavesThumbs()
{
  // No User thumbs
  // TODO: Is there a speed up if we cache the program thumbs first?
  for (unsigned int i = 0; i < m_items.size(); i++)
  {
    CFileItemPtr pItem = m_items[i];
    if (pItem->IsParentFolder())
      continue;
    pItem->SetCachedGameSavesThumb();  // was  pItem->SetCachedProgramThumb(); oringally
  }
}

void CFileItemList::Swap(unsigned int item1, unsigned int item2)
{
  if (item1 != item2 && item1 < m_items.size() && item2 < m_items.size())
    std::swap(m_items[item1], m_items[item2]);
}

void CFileItemList::UpdateItem(const CFileItem *item)
{
  if (!item) return;
  CFileItemPtr oldItem = Get(item->m_strPath);
  if (oldItem)
    *oldItem = *item;
}

void CFileItemList::AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, const LABEL_MASKS &labelMasks)
{
  SORT_METHOD_DETAILS sort;
  sort.m_sortMethod=sortMethod;
  sort.m_buttonLabel=buttonLabel;
  sort.m_labelMasks=labelMasks;

  m_sortDetails.push_back(sort);
}

void CFileItemList::SetReplaceListing(bool replace)
{
  m_replaceListing = replace;
}

void CFileItemList::ClearSortState()
{
  m_sortMethod=SORT_METHOD_NONE;
  m_sortOrder=SORT_ORDER_NONE;
}

CVideoInfoTag* CFileItem::GetVideoInfoTag()
{
  if (!m_videoInfoTag)
    m_videoInfoTag = new CVideoInfoTag;

  return m_videoInfoTag;
}

CPictureInfoTag* CFileItem::GetPictureInfoTag()
{
  if (!m_pictureInfoTag)
    m_pictureInfoTag = new CPictureInfoTag;

  return m_pictureInfoTag;
}

MUSIC_INFO::CMusicInfoTag* CFileItem::GetMusicInfoTag()
{
  if (!m_musicInfoTag)
    m_musicInfoTag = new MUSIC_INFO::CMusicInfoTag;

  return m_musicInfoTag;
}

CStdString CFileItem::FindTrailer() const
{
  CStdString strFile2, strTrailer;
  CStdString strFile = m_strPath;
  if (IsStack())
  {
    CStdString strPath;
    CUtil::GetParentPath(m_strPath,strPath);
    CStackDirectory dir;
    CStdString strPath2;
    strPath2 = dir.GetStackedTitlePath(strFile);
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(strPath2),strFile);
    CFileItem item(dir.GetFirstStackedFile(m_strPath),false);
    CStdString strTBNFile = item.GetTBNFile();
    CUtil::ReplaceExtension(strTBNFile, "-trailer",strTBNFile);
    CUtil::AddFileToFolder(strPath,CUtil::GetFileName(strTBNFile),strFile2);
  }
  if (CUtil::IsInRAR(strFile) || CUtil::IsInZIP(strFile))
  {
    CStdString strPath, strParent;
    CUtil::GetDirectory(strFile,strPath);
    CUtil::GetParentPath(strPath,strParent);
    CUtil::AddFileToFolder(strParent,CUtil::GetFileName(m_strPath),strFile);
  }

  // no local trailer available for these
  if (IsInternetStream() || CUtil::IsUPnP(strFile) || IsLiveTV() || IsPlugin())
    return strTrailer;

  CStdString strDir;
  CUtil::GetDirectory(strFile, strDir);
  CFileItemList items;
  CDirectory::GetDirectory(strDir, items, g_stSettings.m_videoExtensions, true, false, DIR_CACHE_ALWAYS, false);
  CUtil::RemoveExtension(strFile);
  strFile += "-trailer";
  CStdString strFile3 = CUtil::AddFileToFolder(strDir, "movie-trailer");

  for (int i = 0; i < items.Size(); i++)
  {
    CStdString strCandidate = items[i]->m_strPath;
    CUtil::RemoveExtension(strCandidate);
    if (strCandidate.CompareNoCase(strFile) == 0 ||
        strCandidate.CompareNoCase(strFile2) == 0 ||
        strCandidate.CompareNoCase(strFile3) == 0)
    {
      strTrailer = items[i]->m_strPath;
      break;
    }
  }

  return strTrailer;
}

bool CFileItem::IsAdult() const
{
  return GetPropertyBOOL("adult") || GetPropertyBOOL("custom:adult");
}

void CFileItem::SetAdult(bool adult)
{
  SetProperty("adult", adult);
}
  
bool CFileItem::IsCountryAllowed() const
{
  bool isCountryAllowed = false;
  
  if (!HasLinksList())
  {
    isCountryAllowed = CUtil::IsCountryAllowed(GetProperty("country-codes"), GetPropertyBOOL("country-rel"));
  }
  else
  {
    const CFileItemList* linkList = GetLinksList();

    if (!linkList)
    {
      CLog::Log(LOGERROR,"CFileItem::IsCountryAllowed() - FAILED to get LinkList. [label=%s] (ica)",GetLabel().c_str());
      isCountryAllowed = true;
    }
    else
    {
      for (int i=0; (i<linkList->Size() && !isCountryAllowed); i++)
      {
        CFileItemPtr linkFileItem = linkList->Get(i);
        isCountryAllowed |= linkFileItem->IsCountryAllowed();
      }
    }
  }

  //CLog::Log(LOGDEBUG,"CFileItem::IsCountryAllowed() - For item [label=%s] going to return [isCountryAllowed=%d]. [country-codes=%s][country-rel=%d]. [cc=%s] (ica)",GetLabel().c_str(),isCountryAllowed,GetProperty("country-codes").c_str(),GetPropertyBOOL("country-rel"),g_application.GetCountryCode().c_str());
  return isCountryAllowed;
}

void CFileItem::SetCountryRestriction(const CStdString& countries, bool allow)
{
  CStdString countriesLower = countries;
  countriesLower.ToLower();

  SetProperty("country-codes", countriesLower);
  SetProperty("country-rel", allow);
}

void CFileItem::GetCountryRestriction(CStdString& countries, bool& allow) const
{
  if (HasProperty("country-codes"))
  {
    countries = GetProperty("country-codes");
  }
  else
  {
    countries = "all";
  }
  
  if (HasProperty("country-rel"))
  {
    allow = GetPropertyBOOL("country-rel");
  }
  else
  {
    allow = true;
  }
}

const CStdString& CFileItem::GetContentLang() const
{
  return GetProperty("content-lang");
}

void CFileItem::SetContentLang(const CStdString& contentLang)
{
  SetProperty("content-lang", contentLang);
}

bool CFileItem::IsAllowed() const
{
  if (IsAdult() && !CUtil::IsAdultAllowed())
  {
    return false;
  }
  
  bool isCountryAllowed = IsCountryAllowed();
  if (!isCountryAllowed)
  {
    return false;
  }

#ifndef HAS_LASTFM
  CURI url(m_strPath);
  if (url.GetProtocol() == "lastfm")
  {
    return false;
  }
#endif

  return true;
}

CLinkBoxeeType::LinkBoxeeTypeEnums CFileItem::GetLinkBoxeeTypeAsEnum(const CStdString& boxeeType)
{
  CLinkBoxeeType::LinkBoxeeTypeEnums boxeeTypeEnum = CLinkBoxeeType::UNKNOWN;

  CStdString boxeeTypeStr = boxeeType;
  boxeeTypeStr = boxeeTypeStr.ToLower();

  if (boxeeTypeStr == "play" || boxeeTypeStr == "feature")
  {
    boxeeTypeEnum = CLinkBoxeeType::FEATURE;
  }
  else if (boxeeTypeStr == "clip")
  {
    boxeeTypeEnum = CLinkBoxeeType::CLIP;
  }
  else if (boxeeTypeStr == "trailer")
  {
    boxeeTypeEnum = CLinkBoxeeType::TRAILER;
  }
  else if (boxeeTypeStr == "local")
  {
    boxeeTypeEnum = CLinkBoxeeType::LOCAL;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileItem::GetLinkBoxeeTypeAsEnum - Unknown boxeeType string [boxeeType=%s]",boxeeType.c_str());
  }

  return boxeeTypeEnum;
}

CLinkBoxeeOffer::LinkBoxeeOfferEnums CFileItem::GetLinkBoxeeOfferAsEnum(const CStdString& boxeeOffer)
  {
  CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOfferEnum = CLinkBoxeeOffer::UNKNOWN;

  CStdString boxeeOfferStr = boxeeOffer;
  boxeeOfferStr = boxeeOfferStr.ToLower();

  if (boxeeOfferStr == "unavailable")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::UNAVAILABLE;
  }
  else if (boxeeOfferStr == "free")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::FREE;
  }
  else if (boxeeOfferStr == "rent")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::RENT;
  }
  else if (boxeeOfferStr == "buy")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::BUY;
  }
  else if (boxeeOfferStr == "subscription")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::SUBSCRIPTION;
  }
  else if (boxeeOfferStr == "external subscription" || boxeeOfferStr == "external_subscription")
  {
    boxeeOfferEnum = CLinkBoxeeOffer::EXT_SUBSCRIPTION;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFileItem::GetLinkBoxeeTypeAsEnum - Unknown boxeeType string [boxeeOffer=%s]",boxeeOffer.c_str());
  }

  return boxeeOfferEnum;

}

CStdString CFileItem::GetLinkBoxeeTypeAsString(CLinkBoxeeType::LinkBoxeeTypeEnums boxeeType)
{
  CStdString boxeeTypeStr = "";

  switch(boxeeType)
  {
  case CLinkBoxeeType::FEATURE:
    boxeeTypeStr = "feature";
    break;
  case CLinkBoxeeType::CLIP:
    boxeeTypeStr = "clip";
    break;
  case CLinkBoxeeType::TRAILER:
    boxeeTypeStr = "trailer";
    break;
  case CLinkBoxeeType::LOCAL:
    boxeeTypeStr = "local";
    break;
  default:
    CLog::Log(LOGDEBUG,"CFileItem::GetLinkBoxeeTypeAsString - Unknown boxeeType enum [boxeeType=%d]",boxeeType);
    break;
  }

  return boxeeTypeStr;
}

CStdString CFileItem::GetLinkBoxeeOfferAsString(CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOffer)
{
  CStdString boxeeOfferStr = "";

  switch(boxeeOffer)
  {
  case CLinkBoxeeOffer::UNAVAILABLE:
    boxeeOfferStr = "unavailable";
    break;
  case CLinkBoxeeOffer::FREE:
    boxeeOfferStr = "free";
    break;
  case CLinkBoxeeOffer::RENT:
    boxeeOfferStr = "rent";
    break;
  case CLinkBoxeeOffer::BUY:
    boxeeOfferStr = "buy";
    break;
  case CLinkBoxeeOffer::SUBSCRIPTION:
    boxeeOfferStr = "subscription";
    break;
  case CLinkBoxeeOffer::EXT_SUBSCRIPTION:
    boxeeOfferStr = "external_subscription";
    break;
  default:
    CLog::Log(LOGDEBUG,"CFileItem::GetLinkBoxeeOfferAsString - Unknown boxeeOffer enum [boxeeOffer=%d]",boxeeOffer);
    break;
  }

  return boxeeOfferStr;
}

