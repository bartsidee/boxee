/*!
 \file FileItem.h
 \brief
 */
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

#include "GUIListItem.h"
#include "utils/Archive.h"
#include "utils/ISerializable.h"
#include "DateTime.h"
#include "SortFileItem.h"
#include "utils/LabelFormatter.h"
#include "GUIPassword.h"
#include "utils/CriticalSection.h"

#include <vector>
#include "boost/shared_ptr.hpp"

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}
class CVideoInfoTag;
class CPictureInfoTag;

class CAlbum;
class CArtist;
class CSong;
class CGenre;

class CFileItemList;

// Boxee
class CLinkBoxeeType
{
public:
  enum LinkBoxeeTypeEnums
  {
    NONE=0,
    FEATURE=1,
    CLIP=2,
    TRAILER=3,
    LOCAL=4,
    NUM_OF_LINK_BOXEE_TYPE=5,
    UNKNOWN=6
  };
};

class CLinkBoxeeOffer
{
public:
  enum LinkBoxeeOfferEnums
  {
    UNAVAILABLE=0,
    FREE=1,
    RENT=2,
    BUY=3,
    SUBSCRIPTION=4,
    EXT_SUBSCRIPTION=5,
    NUM_OF_LINK_BOXEE_OFFER=6,
    UNKNOWN=7
  };
};
//end Boxee

/* special startoffset used to indicate that we wish to resume */
#define STARTOFFSET_RESUME (-1)

class CMediaSource;

class CFileItem;

/*!
  \brief A shared pointer to CFileItem
  \sa CFileItem
  */
typedef boost::shared_ptr<CFileItem> CFileItemPtr;

/*!
  \brief Represents a file on a share
  \sa CFileItemList
  */
class CFileItem :
      public CGUIListItem, public IArchivable, public ISerializable
{
public:
  CFileItem(void);
  CFileItem(const CFileItem& item);
  CFileItem(const CGUIListItem& item);
  CFileItem(const CStdString& strLabel);
  CFileItem(const CStdString& strPath, bool bIsFolder);
  CFileItem(const CSong& song);
  CFileItem(const CStdString &path, const CAlbum& album);
  CFileItem(const CArtist& artist);
  CFileItem(const CGenre& genre);
  CFileItem(const CVideoInfoTag& movie);
  CFileItem(const CMediaSource& share);
  virtual ~CFileItem(void);

  void Reset();
  const CFileItem& operator=(const CFileItem& item);
  virtual void Archive(CArchive& ar);
  virtual void Serialize(CVariant& value);
  virtual bool IsFileItem() const { return true; };

  bool Exists() const;
  bool IsVideo() const;
  bool IsPicture() const;
  bool IsLyrics() const;
  bool IsAudio() const;
  bool IsCUESheet() const;
  bool IsShoutCast() const;
  bool IsLastFM() const;
  bool IsInternetPlayList() const;
  bool IsInternetStream() const;
  bool IsPlayList(bool bAllowQuery = false) const;
  bool IsSmartPlayList() const;
  bool IsPythonScript() const;
  bool IsXBE() const;
  bool IsPlugin() const;
  bool IsPluginRoot() const;
  bool IsDefaultXBE() const;
  bool IsShortCut() const;
  bool IsNFO() const;
  bool IsDVDImage() const;
  bool IsDVDFile(bool bVobs = true, bool bIfos = true) const;
  bool IsRAR() const;
  bool IsZIP() const;
  bool IsCBZ() const;
  bool IsCBR() const;
  bool IsISO9660() const;
  bool IsCDDA() const;
  bool IsDVD() const;
  bool IsOnDVD() const;
  bool IsOnLAN() const;
  bool IsHD() const;
  bool IsRemote() const;
  bool IsSmb() const;
  bool IsDAAP() const;
  bool IsStack() const;
  bool IsMultiPath() const;
  bool IsMusicDb() const;
  bool IsVideoDb() const;
  bool IsType(const char *ext) const;
  bool IsVirtualDirectoryRoot() const;
  bool IsReadOnly() const;
  bool CanQueue() const;
  void SetCanQueue(bool bYesNo);
  bool IsParentFolder() const;
  bool IsFileFolder() const;
  bool IsMemoryUnit() const;
  bool IsRemovable() const;
  bool IsTuxBox() const;
  bool IsUPnP() const;
  bool IsNFS() const;
  bool IsAFP() const;
  bool IsMythTV() const;
  bool IsHDHomeRun() const;
  bool IsVTP() const;
  bool IsLiveTV() const;

//Boxee
  bool IsMMS() const;
  bool IsRSS() const;
  bool IsFlash() const;
  bool IsScript() const;
  bool IsBoxeeDb() const;
  bool IsHidden() const;
  bool IsApp() const;
  bool IsPlayableFolder() const;
  void SetPlayableFolder(bool isDVD);

  bool IsAdult() const;
  void SetAdult(bool adult);

  bool IsCountryAllowed() const;
  void GetCountryRestriction(CStdString& countries, bool& allow) const;
  void SetCountryRestriction(const CStdString& countries, bool allow);

  const CStdString& GetContentLang() const;
  void  SetContentLang(const CStdString& contentLang);
  
  bool IsAllowed() const;
  
  void Dump() const;

  static CLinkBoxeeType::LinkBoxeeTypeEnums GetLinkBoxeeTypeAsEnum(const CStdString& boxeeType);
  static CStdString GetLinkBoxeeTypeAsString(CLinkBoxeeType::LinkBoxeeTypeEnums boxeeType);
  static CLinkBoxeeOffer::LinkBoxeeOfferEnums GetLinkBoxeeOfferAsEnum(const CStdString& boxeeOffer);
  static CStdString GetLinkBoxeeOfferAsString(CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOffer);
//end Boxee

  void RemoveExtension();
  void CleanString();
  void FillInDefaultIcon();
  void SetMusicThumb(bool alwaysCheckRemote = false);
  void SetFileSizeLabel();
  virtual void SetLabel(const CStdString &strLabel);
  bool IsLabelPreformated() const { return m_bLabelPreformated; }
  void SetLabelPreformated(bool bYesNo) { m_bLabelPreformated=bYesNo; }

  inline bool HasMusicInfoTag() const
  {
    return m_musicInfoTag != NULL;
  }

  MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag();

  inline const MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag() const
  {
    return m_musicInfoTag;
  }

  inline bool HasVideoInfoTag() const
  {
    return m_videoInfoTag != NULL;
  }

  CVideoInfoTag* GetVideoInfoTag();

  inline const CVideoInfoTag* GetVideoInfoTag() const
  {
    return m_videoInfoTag;
  }

  inline bool HasPictureInfoTag() const
  {
    return m_pictureInfoTag != NULL;
  }

  inline const CPictureInfoTag* GetPictureInfoTag() const
  {
    return m_pictureInfoTag;
  }

  CPictureInfoTag* GetPictureInfoTag();

  // Gets the cached thumb filename (no existence checks)
  CStdString GetCachedVideoThumb() const;
  CStdString GetCachedEpisodeThumb() const;
  CStdString GetCachedPictureThumb() const;
  CStdString GetCachedArtistThumb() const;
  CStdString GetCachedProgramThumb() const;
  CStdString GetCachedGameSaveThumb() const;
  CStdString GetCachedProfileThumb() const;
  CStdString GetCachedSeasonThumb() const;
  CStdString GetCachedActorThumb() const;
  CStdString GetCachedFanart() const;
  static CStdString GetCachedThumb(const CStdString &path, const CStdString& strPath2, bool split=false);

  // Sets the video thumb (cached first, else caches user thumb)
  void SetVideoThumb();
  CStdString CacheFanart(bool probe=false) const;

  // Sets the cached thumb for the item if it exists
  void SetCachedVideoThumb();
  void SetCachedPictureThumb();
  void SetCachedArtistThumb();
  void SetCachedProgramThumb();
  void SetCachedGameSavesThumb();
  void SetCachedMusicThumb();
  void SetCachedSeasonThumb();

  // Gets the .tbn file associated with this item
  CStdString GetTBNFile() const;
  // Gets the folder image associated with this item (defaults to folder.jpg)
  CStdString GetFolderThumb(const CStdString &folderJPG = "folder.jpg") const;
  // Gets the correct movie title
  CStdString GetMovieName(bool bUseFolderNames = false) const;

  // Gets the user thumb, if it exists
  CStdString GetUserVideoThumb() const;
  CStdString GetUserMusicThumb(bool alwaysCheckRemote = false) const;

  int64_t    GetSize() { return m_dwSize; }

  // Caches the user thumb and assigns it to the item
  void SetUserVideoThumb();
  void SetUserProgramThumb();
  void SetUserMusicThumb(bool alwaysCheckRemote = false);

  // finds a matching local trailer file
  CStdString FindTrailer() const;

  virtual bool LoadMusicTag();
  virtual bool LoadVideoTag();

  /* returns the content type of this item if known. will lookup for http streams */
  const CStdString& GetContentType(bool bAllowQuery=true) const;

  /* sets the contenttype if known beforehand */
  void              SetContentType(const CStdString& content) { m_contenttype = content; } ;

  /* general extra info about the contents of the item, not for display */
  void SetExtraInfo(const CStdString& info) { m_extrainfo = info; };
  const CStdString& GetExtraInfo() const { return m_extrainfo; };

  bool IsSamePath(const CFileItem *item) const;

  bool IsAlbum() const;

  void GetIPhoneXml(CStdString& xmlRes);
  
  void SetExternalFileItem(CFileItemPtr externalFileItem);
  CFileItemPtr GetExternalFileItem() const;
  bool HasExternlFileItem() const;

  static bool CreateLink(CFileItemPtr& linkItem ,const CStdString& title, const CStdString& url, const CStdString& contentType, CLinkBoxeeType::LinkBoxeeTypeEnums boxeeTypeEnum, const CStdString& provider, const CStdString& providerName, const CStdString& providerThumb, const CStdString& countries, bool countriesRelAllow, const CStdString &qualityLabel, int quality, CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOfferEnum, const CStdString& productsList);
  bool AddLink(const CFileItemPtr& linkItem , bool bPushBack=true);
  bool AddLink(const CStdString& title, const CStdString& url, const CStdString& contentType, CLinkBoxeeType::LinkBoxeeTypeEnums boxeeTypeEnum, const CStdString& provider, const CStdString& providerName, const CStdString& providerThumb, const CStdString& countries, bool countriesRelAllow, const CStdString& qualityLabel, int quality, CLinkBoxeeOffer::LinkBoxeeOfferEnums boxeeOfferEnum, const CStdString& productsList , bool bPushBack=true);
  void SetLinksList(const CFileItemList* linksFileItemList);
  void ClearLinksList();
  const CFileItemList* GetLinksList() const;
  bool HasLinksList() const;
  void SortLinkList(CBoxeeSort& boxeeSort) const;

  bool HasPath();

  CFileItemPtr GetPrevItem() const;
  CFileItemPtr GetNextItem() const;
  void         SetPrevItem(CFileItemPtr prev);
  void         ResetPrevItem();
  void         SetNextItem(CFileItemPtr next);
  void         ResetNextItem();
  void         ResetPrevAndNextItems();
  
private:
  // Gets the previously cached thumb file (with existence checks)
  CStdString GetPreviouslyCachedMusicThumb() const;

  void SetPropertyForLinkType(CStdString boxeeTypeStr, CFileItemPtr linkItem);
  void SetPropertyForLinkOffer(CStdString boxeeOfferStr, CFileItemPtr linkItem);

public:
  CStdString m_strPath;            ///< complete path to item
  bool m_bIsShareOrDrive;    ///< is this a root share/drive
  int m_iDriveType;     ///< If \e m_bIsShareOrDrive is \e true, use to get the share type. Types see: CMediaSource::m_iDriveType
  CDateTime m_dateTime;             ///< file creation date & time
  int64_t m_dwSize;             ///< file size (0 for folders)
  CStdString m_strDVDLabel;
  CStdString m_strTitle;
  int m_iprogramCount;
  int m_idepth;
  int m_lStartOffset;
  int m_lEndOffset;
  LockType m_iLockMode;
  CStdString m_strLockCode;
  int m_iHasLock; // 0 - no lock 1 - lock, but unlocked 2 - locked
  int m_iBadPwdCount;
  
  std::vector<CStdString> m_fallbackPaths;  
private:

  bool m_bPlayableFolder;
  bool m_bIsParentFolder;
  bool m_bCanQueue;
  bool m_bLabelPreformated;
  CStdString m_contenttype;
  CStdString m_extrainfo;
  MUSIC_INFO::CMusicInfoTag* m_musicInfoTag;
  CVideoInfoTag* m_videoInfoTag;
  CPictureInfoTag* m_pictureInfoTag;
  CFileItemPtr m_externalFileItem;
  bool m_bIsAlbum;

  CFileItemPtr m_prevItem;
  CFileItemPtr m_nextItem;

  CFileItemPtr m_linksFileItemList;
};

/*!
  \brief A vector of pointer to CFileItem
  \sa CFileItem
  */
typedef std::vector< CFileItemPtr > VECFILEITEMS;

/*!
  \brief Iterator for VECFILEITEMS
  \sa CFileItemList
  */
typedef std::vector< CFileItemPtr >::iterator IVECFILEITEMS;

/*!
  \brief A map of pointers to CFileItem
  \sa CFileItem
  */
typedef std::map<CStdString, CFileItemPtr > MAPFILEITEMS;

/*!
  \brief Iterator for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::map<CStdString, CFileItemPtr >::iterator IMAPFILEITEMS;

/*!
  \brief Pair for MAPFILEITEMS
  \sa MAPFILEITEMS
  */
typedef std::pair<CStdString, CFileItemPtr > MAPFILEITEMSPAIR;

typedef bool (*FILEITEMLISTCOMPARISONFUNC) (const CFileItemPtr &pItem1, const CFileItemPtr &pItem2);
typedef void (*FILEITEMFILLFUNC) (CFileItemPtr &item);

// This class holds page related information for items list
class CPageContext 
{
public:
  CPageContext() 
  {
    Clear();
  }
  
  void Clear()
  {
    m_startIndex = -1;
    m_itemsPerPage = -1;
    m_totalResults = -1;
  }
  
  bool IsSet() 
  {
    return (m_startIndex != -1 && m_itemsPerPage != -1 && m_totalResults != -1);
  }

public:
  int m_startIndex;
  int m_itemsPerPage;
  int m_totalResults;
};

/*!
  \brief Represents a list of files
  \sa CFileItemList, CFileItem
  */
class CFileItemList : public CFileItem
{
public:
  enum CACHE_TYPE { CACHE_NEVER = 0, CACHE_IF_SLOW, CACHE_ALWAYS };

  CFileItemList();
  CFileItemList(const CStdString& strPath);
  CFileItemList(const CFileItemList &itemList);

  virtual ~CFileItemList();


  virtual void Archive(CArchive& ar);
  CFileItemPtr operator[] (int iItem);
  const CFileItemPtr operator[] (int iItem) const;
  CFileItemPtr operator[] (const CStdString& strPath);
  const CFileItemPtr operator[] (const CStdString& strPath) const;
  void Clear();
  void ClearItems();
  void Add(const CFileItemPtr &pItem);
  void AddFront(const CFileItemPtr &pItem, int itemPosition);
  void Remove(CFileItem* pItem);
  void Remove(int iItem);
  CFileItemPtr Get(int iItem);
  const CFileItemPtr Get(int iItem) const;

  CFileItemPtr Get(const CStdString& strPath);
  CFileItemPtr Get(const CStdString& strPropertyName, const CStdString& strPropertyValue);
  const CFileItemPtr Get(const CStdString& strPath) const;
  const CFileItemPtr Get(const CStdString& strPropertyName, const CStdString& strPropertyValue) const;

  void AssignTo(CFileItemList& outList, unsigned int start, unsigned int end);
  int Size() const;
  bool IsEmpty() const;
  void Append(const CFileItemList& itemlist);
  void Assign(const CFileItemList& itemlist, bool append = false);
  bool Copy  (const CFileItemList& item);
  const CFileItemList &operator=(const CFileItemList& item);
  void Reserve(int iCount);
  void Sort(SORT_METHOD sortMethod, SORT_ORDER sortOrder);
  void Sort(CBoxeeSort& boxeeSort);
  void Randomize();
  void SetMusicThumbs();
  void FillInDefaultIcons();
  int GetFolderCount() const;
  int GetFileCount() const;
  int GetSelectedCount() const;
  int GetObjectCount() const;
  void FilterCueItems();
  void RemoveExtensions();
  void SetFastLookup(bool fastLookup);
  void SetFastLookup(bool fastLookup, const CStdString& strProperty);
  bool Contains(const CStdString& fileName) const;
  bool Contains(const CStdString& propertyName, const CStdString& propertyValue) const;
  bool GetFastLookup() const { return m_fastLookup; };
  void Stack();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; }
  SORT_METHOD GetSortMethod() const { return m_sortMethod; }
  void SetAtIndex(int iItem, const CFileItemPtr &pItem);

  bool Save();
  bool Save(const CStdString& filePath);
  bool Load();
  bool Load(const CStdString& filePath);

  void SetCacheToDisc(CACHE_TYPE cacheToDisc) { m_cacheToDisc = cacheToDisc; }
  bool CacheToDiscAlways() const { return m_cacheToDisc == CACHE_ALWAYS; }
  bool CacheToDiscIfSlow() const { return m_cacheToDisc == CACHE_IF_SLOW; }
  void RemoveDiscCache() const;
  bool AlwaysCache() const;

  void SetCachedVideoThumbs();
  void SetCachedProgramThumbs();
  void SetCachedGameSavesThumbs();
  void SetCachedMusicThumbs();
  void SetProgramThumbs();
  void SetGameSavesThumbs();

  void Swap(unsigned int item1, unsigned int item2);

  void UpdateItem(const CFileItem *item);

  void AddSortMethod(SORT_METHOD method, int buttonLabel, const LABEL_MASKS &labelMasks);
  bool HasSortDetails() const { return m_sortDetails.size() != 0; };
  const std::vector<SORT_METHOD_DETAILS> &GetSortDetails() const { return m_sortDetails; };
  bool GetReplaceListing() const { return m_replaceListing; };
  void SetReplaceListing(bool replace);
  void SetContent(const CStdString &content) { m_content = content; };
  const CStdString &GetContent() const { return m_content; };
  
  void SetPageContext(const CPageContext& pageContext) {m_pageContext = pageContext;}
  CPageContext GetPageContext() { return m_pageContext; }
  bool HasPageContext() { return m_pageContext.IsSet(); }
  
  void ClearSortState();
  void CleanStrings();
  
private:
  void Sort(FILEITEMLISTCOMPARISONFUNC func);
  void FillSortFields(FILEITEMFILLFUNC func);
  CStdString GetDiscCacheFile() const;

  VECFILEITEMS m_items;
  MAPFILEITEMS m_map;
  std::map<CStdString, MAPFILEITEMS> m_mapPropertyLookup;
  bool m_fastLookup;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
  CACHE_TYPE m_cacheToDisc;
  bool m_replaceListing;
  CStdString m_content;
  
  CPageContext m_pageContext;

  std::vector<SORT_METHOD_DETAILS> m_sortDetails;

  CCriticalSection m_lock;
};
