// Copyright © 2008 BOXEE. All rights reserved.
#ifndef BXMETADATA_H_
#define BXMETADATA_H_

#include <map>
#include <string>
#include <vector>

#include "logger.h"

namespace BOXEE {

#define AUDIO_TITLE "title"
#define AUDIO_ARTIST "artist"
#define AUDIO_GENRE "genre"
#define AUDIO_ALBUM "album"
#define AUDIO_YEAR "year"

#define MEDIA_DETAIL_AUDIO 1
#define MEDIA_DETAIL_VIDEO 2
#define MEDIA_DETAIL_ALBUM 3
#define MEDIA_DETAIL_ARTIST 4
#define MEDIA_DETAIL_ACTOR 5
#define MEDIA_DETAIL_DIRECTOR 6
#define MEDIA_DETAIL_PICTURE 7
#define MEDIA_DETAIL_SERIES 7
#define MEDIA_DETAIL_SEASON 8

#define MEDIA_ITEM_TYPE_ALBUM "album"
#define MEDIA_ITEM_TYPE_ARTIST "artist"
#define MEDIA_ITEM_TYPE_AUDIO "audio"
#define MEDIA_ITEM_TYPE_VIDEO "video"
#define MEDIA_ITEM_TYPE_SERIES "series"
#define MEDIA_ITEM_TYPE_SEASON "season"
#define MEDIA_ITEM_TYPE_DIR   "dir"
#define MEDIA_ITEM_TYPE_FILE  "file"
#define MEDIA_ITEM_TYPE_PICTURE  "picture"
#define MEDIA_ITEM_TYPE_MUSIC_FOLDER  "musicFolder"
#define MEDIA_ITEM_TYPE_VIDEO_FOLDER  "videoFolder"
#define MEDIA_ITEM_TYPE_PICTURE_FOLDER "pictureFolder"

#define FOLDER_TYPE_SINGLE_MOVIE 1
#define FOLDER_TYPE_MIX 2 
#define FOLDER_TYPE_SERIES 3

#define MEDIA_ORDER_DATE 1 // date when the media item was added
#define MEDIA_ORDER_NAME 2 

#define STATUS_NEW 0
#define STATUS_RESOLVING 1
#define STATUS_RESOLVED 2
#define STATUS_RETRY 3
#define STATUS_UNRESOLVED 4
#define STATUS_PROCESSING 5
#define STATUS_ALL 10

class BXMetadataDetail
{
public:
  BXMetadataDetail();
  virtual ~BXMetadataDetail();
  int m_iDetailType;
  int m_iId; // id of the detail in the media database if exists
  virtual BXMetadataDetail* Clone() = 0;
  virtual void Dump() const {}
};

class BXArtist : public BXMetadataDetail
{
public:
  BXArtist() { m_iDetailType = MEDIA_DETAIL_ARTIST; m_iId = -1;}
  BXArtist(int iId) { m_iDetailType = MEDIA_DETAIL_ARTIST; m_iId = iId; }
  virtual ~BXArtist() {}
  int m_iId;
  std::string m_strName;
  std::string m_strPortrait;
  std::string m_strBio;
  virtual BXMetadataDetail* Clone();
};

class BXAudio : public BXMetadataDetail
{
public:
  BXAudio(); 
  virtual ~BXAudio();
  virtual BXMetadataDetail* Clone();
  int GetArtist(unsigned int index  = 0) const;
  void AddArtist(int iArtist);
  std::string m_strTitle;
  std::string m_strAlbum;
  std::string m_strDescription;
  std::string m_strLanguageId;
  //std::string m_strArtwork;
  int m_iDuration;
  int m_iYear;
  std::string m_strGenre;
  std::vector<int> m_vecArtists;
  std::string m_strPath;
  int m_iAlbumId;
  int m_iArtistId;
  int m_iTrackNumber;
  unsigned long m_iDateAdded;
  unsigned long m_iDateModified;
  int m_iId;

};

class BXAlbum : public BXMetadataDetail
{
public:
  BXAlbum(); 
  virtual BXMetadataDetail* Clone();
  virtual ~BXAlbum();
  BXAlbum(const BXAlbum& that);
  BXAlbum& operator=(const BXAlbum& that);
  
  void SetPath(const std::string& strPath) {
    m_strPath = strPath;
  }
  
  std::string GetPath() const {
    return m_strPath;
  }
  
  std::vector<BXAudio*> m_vecSongs;
  std::string m_strTitle;
  std::string m_strArtist;
  std::string m_strGenre;
  std::string m_strArtwork;
  std::string m_strDescription;
  std::string m_strLanguage;
  int m_iArtistId;
  int m_iNumTracs;
  int m_iDuration;
  int m_iYear;
  int m_iRating;
  unsigned long m_iDateAdded;
  unsigned long m_iDateModified; // matches the modification date of the album directory
  int m_iId;
  
  // indicates the resolving status of the album, can hold the following integer values (up from 0)
  // NEW - album was not resolved yet
  // RESOLVING = album is currently being processed
  // RESOLVED - all available metadata have been retrieved and updated in the database
  // UNRESOLVED_ONCE - first attempt 
  // UNRESOLVED_TWICE - second attempt
  // UNRESOLVED - album metadata could not be found
  int m_iStatus;
    
  // indicates whether the album corresponds to a real album on the share
  // otherwise the album is just there for the metadata
  // if the Virtual is 0, an entry should also exist in album_instances table
  int m_iVirtual;
private:
  std::string m_strPath;
};

class BXPath
{
public:
  std::string m_strPath;
  std::vector<std::string> m_vecParts;
};

class BXVideoLink
{
public:
  std::string m_strURL;
  std::string m_strProvider;
  std::string m_strProviderName;
  std::string m_strProviderThumb;
  std::string m_strTitle;
  std::string m_strQuality;
  std::string m_strQualityLabel;
  bool        m_bIsHD;
  std::string m_strCountryCodes;
  std::string m_strCountryRel;
  std::string m_strOffer;
  std::string m_strBoxeeType;
  std::string m_strType;
};

class BXVideo : public BXMetadataDetail
{
public:
  BXVideo();
  virtual ~BXVideo();
  virtual BXMetadataDetail* Clone();
  void Dump() const;
  std::string m_strBoxeeId;
  std::string m_strTitle;
  std::string m_strShowTitle;
  std::string m_strShowId;
  std::string m_strPath;
  std::string m_strType; // used to indicate movie parts
  std::string m_strDirector;
  std::string m_strDescription;
  std::string m_strExtDescription;
  std::string m_strIMDBKey;
  std::string m_strMPAARating;
  std::string m_strCredits;
  std::string m_strStudio;
  std::string m_strTagLine;
  std::string m_strLanguageId;
  std::string m_strCover;
  std::string m_strOriginalCover;
  std::string m_strTrailerUrl;
  std::string m_strGenre;
  int m_iSeason;
  int m_iEpisode;
  int m_iFolderId;
  int m_iRating;
  int m_iPopularity;
  int m_iDuration;
  int m_iYear;
  unsigned long m_iDateAdded;
  unsigned long m_iDateModified;
  unsigned long m_iReleaseDate;
  std::vector<std::string> m_vecGenres;
  std::vector<std::string> m_vecActors;
  std::vector<std::string> m_vecParts;
  std::vector<BXPath> m_vecLinks;
  std::vector<BXVideoLink> m_vecVideoLinks;
  bool m_bMovie;
  std::string m_episodeDate;
  
  std::string m_strFirstAired;
  std::string m_strNfoPath;
  int m_iNfoAccessedTime;
  int m_iNfoModifiedTime;
  int m_iCriticsScore;
  int m_iAudienceScore;
  std::string m_strCriticsRating;
  std::string m_strAudienceRating;
};

class BXSeries : public BXMetadataDetail
{
public:
  BXSeries(); 
  virtual BXMetadataDetail* Clone();
  virtual ~BXSeries();
  BXSeries(const BXSeries& that);
  BXSeries& operator=(const BXSeries& that);
  
  void SetPath(const std::string& strPath) {
    m_strPath = strPath;
  }
  
  std::string GetPath() const {
    return m_strPath;
  }
  
  
  std::vector<BXVideo*> m_vecEpisodes;
  std::vector<std::string> m_vecGenres;
  std::string m_strTitle;
  std::string m_strCover;
  std::string m_strDescription;
  std::string m_strLanguage;
  std::string m_strBoxeeId;
  std::string m_strGenre;
  std::string m_strNfoPath;
  int m_iNfoAccessedTime;
  int m_iNfoModifiedTime;
  int m_iYear;
  
  // Holds the release date of the most recent episode of the series
  unsigned long m_iRecentlyAired;

  // indicates whether the album corresponds to a real album on the share
  // otherwise the album is just there for the metadata
  // if the Virtual is 0, an entry should also exist in album_instances table
  int m_iVirtual;
private:  
  std::string m_strPath;
  
};

class BXSeason : public BXMetadataDetail
{
public:
  BXSeason(); 
  virtual BXMetadataDetail* Clone();
  virtual ~BXSeason();
  BXSeason(const BXSeason& that);
  BXSeason& operator=(const BXSeason& that);
  
  void SetPath(const std::string& strPath) {
    m_strPath = strPath;
  }
  
  std::string GetPath() const {
    return m_strPath;
  }
  
  std::vector<BXVideo*> m_vecEpisodes;
  std::vector<std::string> m_vecGenres;
  std::string m_strTitle;
  std::string m_strCover;
  std::string m_strDescription;
  std::string m_strLanguage;
  int m_iSeriesId;
  int m_iSeasonNum;
  int m_iYear;
  int m_iVirtual;
private:  
  std::string m_strPath;
  
};

class BXPicture : public BXMetadataDetail
{
public:
  BXPicture() { m_iDetailType = MEDIA_DETAIL_PICTURE; }
  virtual ~BXPicture() {}
  virtual BXMetadataDetail* Clone();
  unsigned long m_iDate;
  unsigned long m_iDateModified;
};

/**
 * Holds metadata for the media item
 */
class BXMetadata
{
public:
  //BXMetadata();
  BXMetadata(const std::string& strType, const std::string& strPath = "");
  BXMetadata(const BXMetadata& that);
  BXMetadata& operator=(const BXMetadata& that);
  virtual ~BXMetadata();
  
  virtual void Dump() const;

  // Getters and Setters for basic media file fields
  BXMetadataDetail* GetDetail(int iMetadataDetailType) const;
  
  void SetDetail(int iMetadataDetailType, BXMetadataDetail* pMetadataDetail);

  const std::string& GetPath() const;
  void SetPath(const std::string& strPath);

  const std::string& GetType() const;
  void SetType(const std::string& strType);

  const std::string& GetFolder() const;
  void SetFolder(const std::string& strFolder);
  
  const std::string& GetHash() const;
  void SetHash(const std::string& strHash);

  void SetMediaFileId(int iId);
  int GetMediaFileId() const;

  void SetConfidence(int iConfidence);
  int GetConfidence() const;

  void SetResolverId(const std::string& strResolverId);
  const std::string& GetResolverId() const;

  void SetFileSize(int64_t fileSize);
  int64_t GetFileSize() const;

  void Reset();
  std::string ToString();
  static void PrintMetadataVector(std::vector<BXMetadata*> vecMediaFiles);
  bool m_bResolved;
  int m_iNeedsRescan;
  std::string m_strTitle;
  std::string m_strThumbnail;
  bool m_bGroup;
  bool m_bShare;
  int m_iPart;
  
  unsigned long m_iDateAdded;
  unsigned long m_iDateModified;
  int64_t m_dwSize;
  
private:

  std::map<int, BXMetadataDetail*> m_mapMetadataDetails;

  std::string m_strPath;
  std::string m_strType;
  // Holds the name of the meaningful folder
  // i.e. folder which name is the closest to reflect the title
  // used in case where no metadata is available
  std::string m_strFolder;
  std::string m_strHash;
  int m_iFileId; // the id of the file in the database

  /**
   * The confidence is a grade between 1 and 100 that indicates the chance (in %)
   * of this information being correct. This is a subjective grade given by the 
   * resolver and represents raw estimate
   */
  int m_iConfidence; // TODO: Not in use

  /**
   * Id of the resolver that created the metadata
   */
  std::string m_strResolverId;
  

};

class BXFolder : public BXMetadata
{
public:
  BXFolder(const std::string& strType, const std::string& strPath = "");
  BXFolder(const BXFolder& that);
  BXFolder& operator=(const BXFolder& that);
  virtual ~BXFolder() ;
  
  int m_iFolderType;
  std::vector<BXMetadata*> m_vecFiles;
  
};

}

#endif /*BXMETADATA_H_*/
