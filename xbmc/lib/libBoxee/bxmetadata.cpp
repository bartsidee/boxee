
#include "bxmetadata.h"
#include "logger.h"

namespace BOXEE {

BXMetadataDetail::BXMetadataDetail()
{
  m_iId = -1;
  m_iDetailType = -1;
}

BXMetadataDetail::~BXMetadataDetail()
{
}

void BXMetadata::PrintMetadataVector(std::vector<BXMetadata*> vecMediaFiles)
{
  if (vecMediaFiles.empty()) return;
  LOG(LOG_LEVEL_DEBUG, " LIBRARY: !!!!!!!!!! Printing list of %d media items", vecMediaFiles.size());
  BXMetadata* pTemp;
  for (unsigned int i=0; i < vecMediaFiles.size(); i++)
  {
    pTemp = vecMediaFiles[i];
    if (pTemp) {
      LOG(LOG_LEVEL_DEBUG, "LIBRARY: Type:  %s", pTemp->GetType().c_str());
      if (pTemp->GetType() == MEDIA_ITEM_TYPE_DIR) {
        LOG(LOG_LEVEL_DEBUG, "LIBRARY, METADATA, LIB1, Directory: path= %s", pTemp->GetPath().c_str());
      }
      else if (pTemp->GetType() == MEDIA_ITEM_TYPE_ALBUM) {
        BXAlbum* pAlbum = (BXAlbum*) pTemp->GetDetail(MEDIA_DETAIL_ALBUM);
        if (pAlbum) {
          LOG(LOG_LEVEL_DEBUG, "LIBRARY, METADATA, LIB1, Album: name= %s, path=%s, thumb=%s", pAlbum->m_strTitle.c_str(), pTemp->GetPath().c_str(), pAlbum->m_strArtwork.c_str());
        }
      }
      else if (pTemp->GetType() == MEDIA_ITEM_TYPE_ARTIST) {
    	  BXArtist* pArtist = (BXArtist*) pTemp->GetDetail(MEDIA_DETAIL_ARTIST);
    	  if (pArtist) {
    		  LOG(LOG_LEVEL_DEBUG, "LIBRARY, METADATA, LIB1, Artist: name = %s, path = %s, portrait = %s", pArtist->m_strName.c_str(), pTemp->GetPath().c_str(), pArtist->m_strPortrait.c_str());
    	  }
      }
      else if (pTemp->GetType() == MEDIA_ITEM_TYPE_AUDIO) {
        BXAudio* pAudio = (BXAudio*) pTemp->GetDetail(MEDIA_DETAIL_AUDIO);
        if (pAudio) {
          LOG(LOG_LEVEL_DEBUG, "LIBRARY: Audio: name= %s", pAudio->m_strTitle.c_str());
        }
      }
      else if(pTemp->GetType() == MEDIA_ITEM_TYPE_VIDEO) {
        BXVideo* pVideo = (BXVideo*) pTemp->GetDetail(MEDIA_DETAIL_VIDEO);
        if (pVideo) {
          LOG(LOG_LEVEL_DEBUG, "LIBRARY: Video: name= %s", pVideo->m_strTitle.c_str());
        }
      }
      else if(pTemp->GetType() == MEDIA_ITEM_TYPE_SERIES) {
        BXSeries* pSeries = (BXSeries*) pTemp->GetDetail(MEDIA_DETAIL_SERIES);
        if (pSeries) {
          LOG(LOG_LEVEL_DEBUG, "LIBRARY: Series: name= %s", pSeries->m_strTitle.c_str());
        }
      }
      else if(pTemp->GetType() == "season") {
        BXSeries* pSeries = (BXSeries*) pTemp->GetDetail(MEDIA_DETAIL_SERIES);
        if (pSeries) {
          LOG(LOG_LEVEL_DEBUG, "LIBRARY: Series: name= %s", pSeries->m_strTitle.c_str());
        }
      }
      else
      {
        LOG(LOG_LEVEL_DEBUG, "LIBRARY: Unrecognized media item type, path=%s", pTemp->GetPath().c_str());
      }
    }
    else {
      LOG(LOG_LEVEL_DEBUG, "LIBRARY: Media item is NULL");
    }
  }
}

// ==============================================================
// SEASON

BXSeason::BXSeason() 
{ 
  m_iVirtual = 0;
  m_iDetailType = MEDIA_DETAIL_SEASON;
  m_iSeasonNum = -1;
  m_iSeriesId = -1;
  m_iId = -1;
  m_iYear=0; 
}

BXSeason::~BXSeason() 
{
  for (int i = 0; i < (int)m_vecEpisodes.size(); i++)
  {
    delete m_vecEpisodes[i];
  }
  m_vecEpisodes.clear();
}

BXMetadataDetail* BXSeason::Clone()
{
  BXSeason* pSeason = new BXSeason();
  *pSeason = *this;
  return pSeason;
}

BXSeason::BXSeason(const BXSeason& that) : BXMetadataDetail(that)
{
  *this = that;
}

BXSeason& BXSeason::operator=(const BXSeason& that) 
{
  if (this != &that) {

    BXMetadataDetail::operator=(that);


    m_strTitle = that.m_strTitle;

    m_strPath = that.m_strPath;
    m_strCover = that.m_strCover;
    m_strDescription = that.m_strDescription;
    m_strLanguage = that.m_strLanguage;
    m_iYear = that.m_iYear;
    m_iSeasonNum = that.m_iSeasonNum;
    m_iVirtual = that.m_iVirtual;

    // Clean the memory from the previous data
    for (int i = 0; i < (int)m_vecEpisodes.size(); i++) {
      delete m_vecEpisodes[i];
    }
    m_vecEpisodes.clear();

    for (int i = 0; i < (int)that.m_vecEpisodes.size(); i++)
    {
      BXVideo* pVideo = new BXVideo();
      *pVideo = *(that.m_vecEpisodes[i]);
      m_vecEpisodes.push_back(pVideo);
    }
  }

  return *this;

}

// ==============================================================
// SERIES

BXSeries::BXSeries() 
{ 
  m_iVirtual = 0;
  m_iDetailType = MEDIA_DETAIL_SERIES; 
  m_iYear=0; 
}

BXSeries::~BXSeries() 
{
  //$$LOG(LOG_LEVEL_DEBUG, "$$$ Delete BXSeries %x", this);
  for (int i = 0; i < (int)m_vecEpisodes.size(); i++) {
    delete m_vecEpisodes[i];
  }
  m_vecEpisodes.clear();
}

BXMetadataDetail* BXSeries::Clone()
{
  BXSeries* pSeries = new BXSeries();
  *pSeries = *this;
  return pSeries;
}

BXSeries::BXSeries(const BXSeries& that) : BXMetadataDetail(that)
{
  *this = that;
}

BXSeries& BXSeries::operator=(const BXSeries& that) 
{
  if (this != &that) {

    BXMetadataDetail::operator=(that);


    m_strTitle = that.m_strTitle;

    m_strPath = that.m_strPath;
    m_strCover = that.m_strCover;
    m_strDescription = that.m_strDescription;
    m_strLanguage = that.m_strLanguage;
    m_iYear = that.m_iYear;
    m_iVirtual = that.m_iVirtual;

    // Clean the memory from the previous data
    for (int i = 0; i < (int)m_vecEpisodes.size(); i++) {
      delete m_vecEpisodes[i];
    }
    m_vecEpisodes.clear();

    for (int i = 0; i < (int)that.m_vecEpisodes.size(); i++)
    {
      BXVideo* pVideo = new BXVideo();
      *pVideo = *(that.m_vecEpisodes[i]);
      m_vecEpisodes.push_back(pVideo);
    }
  }

  return *this;

}

BXAlbum::BXAlbum() 
{ 
  m_iVirtual = 0;
  m_iDetailType = MEDIA_DETAIL_ALBUM; 
  m_iNumTracs = 0; 
  m_iDuration = 0; 
  m_iYear=0; 
  m_iArtistId = -1;
  m_iRating = 0;
  m_iDateAdded = 0;
  m_iDateModified = 0;
  m_iStatus = 0;
}

BXAlbum::~BXAlbum() 
{
  //$$LOG(LOG_LEVEL_DEBUG, "$$$ Delete BXAlbum %x", this);
  // Clean the memory from the previous data
  for (int i = 0; i < (int)m_vecSongs.size(); i++) {
    delete m_vecSongs[i];
  }
  m_vecSongs.clear();
}

BXAlbum::BXAlbum(const BXAlbum& that) : BXMetadataDetail(that)
{
  *this = that;
}

BXAlbum& BXAlbum::operator=(const BXAlbum& that) 
{
  if (this != &that) {

    BXMetadataDetail::operator=(that);

    m_strGenre = that.m_strGenre;
    m_strTitle = that.m_strTitle;
    m_strArtist = that.m_strArtist;
    m_strPath = that.m_strPath;
    m_strArtwork = that.m_strArtwork;
    m_strDescription = that.m_strDescription;
    m_strLanguage = that.m_strLanguage;

    m_iArtistId = that.m_iArtistId;
    m_iNumTracs = that.m_iNumTracs;
    m_iDuration = that.m_iDuration;
    m_iYear = that.m_iYear;
    m_iRating = that.m_iRating;
    m_iDateAdded = that.m_iDateAdded;
    m_iDateModified = that.m_iDateModified;
    m_iId = that.m_iId;
    
    m_iVirtual = that.m_iVirtual;

    // Clean the memory from the previous data
    for (int i = 0; i < (int)m_vecSongs.size(); i++) {
      delete m_vecSongs[i];
    }
    m_vecSongs.clear();

    for (int i = 0; i < (int)that.m_vecSongs.size(); i++)
    {
      BXAudio* pAudio = new BXAudio();
      *pAudio = *(that.m_vecSongs[i]);
      m_vecSongs.push_back(pAudio);
    }
  }

  return *this;

}

BXMetadataDetail* BXAlbum::Clone()
{
  BXAlbum* pAlbum = new BXAlbum();
  *pAlbum = *this;
  return pAlbum;
}

BXMetadataDetail* BXArtist::Clone()
{
  BXArtist* pArtist = new BXArtist();
  *pArtist = *this;
  return pArtist;
}

BXAudio::BXAudio() 
{ 
  //$$LOG(LOG_LEVEL_DEBUG, "$$$ Construct BXAudio %x", this);
  m_iDetailType = MEDIA_DETAIL_AUDIO; 
  m_iDuration = 0; 
  m_iYear=0; 
  m_iAlbumId = -1;
  m_iArtistId = -1;
  m_iTrackNumber = 0;
  m_iDateModified = 0;
}

BXMetadataDetail* BXAudio::Clone()
{
  BXAudio* pAudio = new BXAudio();
  *pAudio = *this;
  return pAudio;
}

void BXAudio::AddArtist(int iArtist)
{
  m_vecArtists.push_back(iArtist);
}

BXAudio::~BXAudio()
{
}

int BXAudio::GetArtist(unsigned int index) const
{
  if(index >= m_vecArtists.size()) return -1;
  return m_vecArtists[index];
}

BXVideo::BXVideo() 
{
  m_iDetailType = MEDIA_DETAIL_VIDEO; 
  m_iDuration = 0; 
  m_iYear=0; 
  m_iSeason=-1; 
  m_iEpisode=-1;
  m_iRating = 0;
  m_bMovie = false;
  m_iDateAdded = 0;
  m_iDateModified = 0;
  m_iFolderId = 0;
  m_iPopularity = 0;
  m_iReleaseDate = 0;
  m_iCriticsScore = -1;
  m_iAudienceScore = -1;
}

BXVideo::~BXVideo() 
{
}

void BXVideo::Dump() const
{
	LOG(LOG_LEVEL_DEBUG, "= START VIDEO DETAIL =");
	LOG(LOG_LEVEL_DEBUG, "Path: %s", m_strPath.c_str());
	LOG(LOG_LEVEL_DEBUG, "Title: %s", m_strTitle.c_str());
	LOG(LOG_LEVEL_DEBUG, "Thumb: %s", m_strCover.c_str());
	LOG(LOG_LEVEL_DEBUG, "RottenTomato: Critic Score/Rating:(%d/'%s') | Audience Score/Rating:(%d/'%s')", m_iCriticsScore, m_strCriticsRating.c_str(), m_iAudienceScore, m_strAudienceRating.c_str());

	LOG(LOG_LEVEL_DEBUG, "= END VIDEO DETAIL =");
}

BXMetadataDetail* BXVideo::Clone()
{
  BXVideo* pVideo = new BXVideo();
  *pVideo = *this;
  return pVideo;
}

BXMetadataDetail* BXPicture::Clone()
{
  BXPicture* pPicture = new BXPicture();
  *pPicture = *this;
  return pPicture;
}

BXMetadata::BXMetadata(const std::string& strType, const std::string& strPath) 
{
  //$$LOG(LOG_LEVEL_DEBUG, "$$$ Construct BXMetadata %x", this);
  m_iFileId = -1;
  m_iNeedsRescan = 0;
  m_strType = strType;
  m_strPath = strPath;
  m_bResolved = false;
  m_bGroup = false;
  m_iPart = -1;
  m_iDateAdded = 0;
  m_iDateModified = 0;
  m_dwSize = 0;

  Reset();
}

void BXMetadata::Reset() 
{
  // Go over the map and delete all details
  std::map<int, BXMetadataDetail*>::const_iterator it = m_mapMetadataDetails.begin();
  while ( it != m_mapMetadataDetails.end()) {
    delete (*it).second;
    it++;
  }
  m_mapMetadataDetails.clear();

  std::string strType = m_strType;

  if (strType == MEDIA_ITEM_TYPE_AUDIO) {
    SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum());
    SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist());
    SetDetail(MEDIA_DETAIL_AUDIO, new BXAudio());
  }
  else if (strType == MEDIA_ITEM_TYPE_VIDEO) {
    SetDetail(MEDIA_DETAIL_VIDEO, new BXVideo());
    SetDetail(MEDIA_DETAIL_SERIES, new BXSeries());
    SetDetail(MEDIA_DETAIL_SEASON, new BXSeason());
  }
  else if (strType == MEDIA_ITEM_TYPE_PICTURE) {
    SetDetail(MEDIA_DETAIL_PICTURE, new BXPicture()); 
  }
  else if (strType == MEDIA_ITEM_TYPE_ALBUM) {
    SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum());
    SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist());
  }
  else if (strType == MEDIA_ITEM_TYPE_ARTIST) {
      SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum());
      SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist());
    }
  else if (strType == MEDIA_ITEM_TYPE_SERIES) {
    SetDetail(MEDIA_DETAIL_SERIES, new BXSeries());
    SetDetail(MEDIA_DETAIL_SEASON, new BXSeason());
  }
  else if (strType == MEDIA_ITEM_TYPE_SEASON) {
    SetDetail(MEDIA_DETAIL_SERIES, new BXSeries());
    SetDetail(MEDIA_DETAIL_SEASON, new BXSeason());
  }
  else if (strType == MEDIA_ITEM_TYPE_MUSIC_FOLDER) {
	  SetDetail(MEDIA_DETAIL_ALBUM, new BXAlbum());
	  SetDetail(MEDIA_DETAIL_ARTIST, new BXArtist());
  }
  else if (strType == MEDIA_ITEM_TYPE_VIDEO_FOLDER) {
    // do nothing meantime 
  }
  else if (strType == MEDIA_ITEM_TYPE_PICTURE_FOLDER) {
    // do nothing meantime
  }
  else if (strType == MEDIA_ITEM_TYPE_DIR) {
    // do nothing meantime
  }
  else if (strType == MEDIA_ITEM_TYPE_FILE) {
    // do nothing meantime
  }
  else {
    LOG(LOG_LEVEL_ERROR, "Design bug. Invalid metadata type %s", strType.c_str());
    throw -1;
  }
}

BXMetadata::~BXMetadata()
{
  //$$LOG(LOG_LEVEL_DEBUG, "$$$ Delete BXMetadata %x", this);
  std::map<int, BXMetadataDetail*>::const_iterator it = m_mapMetadataDetails.begin();
  while ( it != m_mapMetadataDetails.end()) {
    delete (*it).second;
    it++;
  }
  m_mapMetadataDetails.clear();

}

void BXMetadata::Dump() const
{
	LOG(LOG_LEVEL_DEBUG, "========================================================");
	LOG(LOG_LEVEL_DEBUG, "Path: %s", m_strPath.c_str());
	LOG(LOG_LEVEL_DEBUG, "Type: %s", m_strType.c_str());
	LOG(LOG_LEVEL_DEBUG, "Thumb: %s", m_strThumbnail.c_str());
	// Print metadata fields
	std::map<int, BXMetadataDetail*>::const_iterator it = m_mapMetadataDetails.begin();
	while ( it != m_mapMetadataDetails.end()) {
		(*it).second->Dump();
		it++;
	}

	LOG(LOG_LEVEL_DEBUG, "========================================================");
}

BXMetadata::BXMetadata(const BXMetadata& that)
{
  *this = that;
}

BXMetadata& BXMetadata::operator=(const BXMetadata& that)
{
  if (this != &that) {
    // Copy all fields
    m_strFolder = that.m_strFolder;
    m_strHash = that.m_strHash;
    m_iFileId = that.m_iFileId;
    m_iConfidence = that.m_iConfidence;
    m_strPath = that.m_strPath;
    m_strResolverId = that.m_strResolverId;
    m_strType = that.m_strType;
    m_bResolved = that.m_bResolved;
    m_strTitle = that.m_strTitle;
    m_bGroup = that.m_bGroup;
    m_strThumbnail = that.m_strThumbnail;
    m_iPart = that.m_iPart;
    m_iDateAdded = that.m_iDateAdded;
    m_iDateModified = that.m_iDateModified;

    // Clean the previous data
    std::map<int, BXMetadataDetail*>::const_iterator it = m_mapMetadataDetails.begin();
    while ( it != m_mapMetadataDetails.end()) {
      delete (*it).second;
      it++;
    }
    m_mapMetadataDetails.clear();

    // Copy all details
    for (std::map<int, BXMetadataDetail*>::const_iterator i = that.m_mapMetadataDetails.begin(); i != that.m_mapMetadataDetails.end(); i++)
    {
      BXMetadataDetail* pDetail = i->second->Clone();
      SetDetail(pDetail->m_iDetailType, pDetail);
    }
  }
  return *this;
}

BXMetadataDetail* BXMetadata::GetDetail(int iMetadataDetailType) const
{
  std::map<int, BXMetadataDetail*>::const_iterator it = m_mapMetadataDetails.find(iMetadataDetailType);
  if(it != m_mapMetadataDetails.end())
  {
    return (*it).second;
  }
  return NULL;
}

void BXMetadata::SetDetail(int iMetadataDetailType, BXMetadataDetail* pMetadataDetail)
{
  std::map<int, BXMetadataDetail*>::iterator it = m_mapMetadataDetails.find(iMetadataDetailType);
  if(it != m_mapMetadataDetails.end())
  {
    BXMetadataDetail* pPrevDetail = (*it).second;
    m_mapMetadataDetails.erase(it);
    delete pPrevDetail;
  }

  m_mapMetadataDetails[iMetadataDetailType] = pMetadataDetail;
}

const std::string& BXMetadata::GetPath() const
{
  return m_strPath;
}

void BXMetadata::SetPath(const std::string& strPath)
{
  m_strPath = strPath;
}

const std::string& BXMetadata::GetType() const
{
  return m_strType;
}

void BXMetadata::SetType(const std::string& strType)
{
  m_strType = strType;
}

const std::string& BXMetadata::GetHash() const
{
  return m_strHash;
}

void BXMetadata::SetHash(const std::string& strHash)
{
  m_strHash = strHash;
}

const std::string& BXMetadata::GetFolder() const
{
  return m_strFolder;
}

void BXMetadata::SetFolder(const std::string& strFolder)
{
  m_strFolder = strFolder;
}

void BXMetadata::SetMediaFileId(int iFileId)
{
  m_iFileId = iFileId;
}

int BXMetadata::GetMediaFileId() const
{
  return m_iFileId;
}


void BXMetadata::SetConfidence(int iConfidence)
{
  m_iConfidence = iConfidence;
}

int BXMetadata::GetConfidence() const
{
  return m_iConfidence;
}

void BXMetadata::SetResolverId(const std::string& strResolverId)
{
  m_strResolverId = strResolverId;
}

const std::string& BXMetadata::GetResolverId() const
{
  return m_strResolverId;
}


void BXMetadata::SetFileSize(int64_t fileSize)
{
  m_dwSize = fileSize;
}
int64_t BXMetadata::GetFileSize() const
{
  return m_dwSize;
}

std::string BXMetadata::ToString() 
{
  std::string strMetadata = "\nMetadata:";
  strMetadata += " Path: " + GetPath();
  strMetadata += " Type: " + GetType();
  //  strMetadata += " Title: " + m_strTitle;
  //  strMetadata += " Year: " + m_iYear;
  //  strMetadata += " Duration: " + m_iDuration;
  strMetadata += "\n";



  return strMetadata;
}

BXFolder::BXFolder(const std::string& strType, const std::string& strPath) : BXMetadata(strType, strPath)
{
  m_iFolderType = -1;
}

BXFolder::BXFolder(const BXFolder& that) : BXMetadata(that)
{
  *this = that;
}

BXFolder::~BXFolder()
{
  for (int i = 0; i < (int)m_vecFiles.size(); i++) {
    delete m_vecFiles[i];
  }
  m_vecFiles.clear();
}

BXFolder& BXFolder::operator=(const BXFolder& that)
{
  if (this != &that) {

    BXMetadata::operator=(that);

    // Clean the memory from the previous data
    for (int i = 0; i < (int)m_vecFiles.size(); i++) {
      delete m_vecFiles[i];
    }
    m_vecFiles.clear();

    for (int i = 0; i < (int)that.m_vecFiles.size(); i++)
    {
      BXMetadata* pMetadata = new BXMetadata(*(that.m_vecFiles[i]));
      m_vecFiles.push_back(pMetadata);
    }
  }

  return *this;
}



} // namespace BOXEE
