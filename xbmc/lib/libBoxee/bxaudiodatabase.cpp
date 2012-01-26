#include "bxaudiodatabase.h"
#include "bxmediafile.h"
#include "bxmediadatabase.h"
#include "bxmetadata.h"
#include "logger.h"
#include "bxutils.h"
#include "bxconfiguration.h"
#include "boxee.h"
#include <time.h>

using namespace dbiplus;
using namespace std;

namespace BOXEE
{

BXAudioDatabase::BXAudioDatabase()
{
  Open();
}

BXAudioDatabase::~BXAudioDatabase()
{

}

int BXAudioDatabase::AddArtist(const BXArtist* pArtist)
{
  if (!pArtist) {
    LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddArtist, Could not add artist, artist is NULL (audiodb)");
    return MEDIA_DATABASE_ERROR;
  }

  std::string strName = pArtist->m_strName;
  BXUtils::StringTrim(strName);

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY, AUDIO DATABASE, THUMB, : Adding artist name = %s, portrait = %s", strName.c_str(), strPortrait.c_str());

  if (strName.empty()) {
    LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddArtist, Could not add artist: %s, name is empty (audiodb)", strName.c_str());
    return MEDIA_DATABASE_ERROR;
  }

  BXArtist artist;
  int result = GetArtistByName(strName, &artist);
  if (result == MEDIA_DATABASE_OK)
  {
    LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddArtist, Artist already exists, name = %s (audiodb)", strName.c_str());
    return artist.m_iId;
  }

  // In any case it will be impossible to insert two identical artists due to database restriction
  result = Insert("insert into artists (idArtist, strName, strPortrait, strBio) values( NULL, '%s', '%s', '%s' )",
      strName.c_str(), pArtist->m_strPortrait.c_str(), pArtist->m_strBio.c_str());

  if (result == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_ERROR, "BXAudioDatabase::AddArtist, Could not add artist: %s (audiodb)", strName.c_str());
  }

  LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddArtist, Artist added,  name = %s (audiodb)", strName.c_str());

  return result;
}

int BXAudioDatabase::AddAlbum(const BXAlbum* pAlbum)
{  
  if (!pAlbum) return MEDIA_DATABASE_ERROR;

  // Check that artist id is initialized, otherwise duplicate albums may be added to the database
  if (pAlbum->m_iArtistId == -1 || pAlbum->m_strTitle == "")
  {
    LOG(LOG_LEVEL_ERROR, "BXAudioDatabase::AddAlbum, unable to add album, no title or artist id (musicresolver)");
    return MEDIA_DATABASE_ERROR;
  }

  int iID = MEDIA_DATABASE_ERROR;
  bool bRemovePrevious = false;
  int iPreviousID = MEDIA_DATABASE_ERROR;
  Dataset* pDS;

  // If this is a virtual album, look for another album with this title and artist in the database
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAlbum, QUERY, select * from albums");
  pDS = Query("select * from albums where idArtist=%i and strTitle like '%s'", pAlbum->m_iArtistId, pAlbum->m_strTitle.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {

      int iVirtual = pDS->fv("iVirtual").get_asInt();
      iID = pDS->fv("idAlbum").get_asInt();

      if (iVirtual == 0)
      {
        if (pAlbum->m_iVirtual == 0)
        {
          // Check if path is the same
          if (pAlbum->GetPath() == pDS->fv("strPath").get_asString())
          {
            // Database holds library album, and we are adding the same library album, return
            delete pDS;
            return iID;
          }
          else
          {
            // The album will be added to the library as another instance
          }
        }
        else
        {
          // Database holds library album, and we are adding the same feed album, return
          delete pDS;
          return iID;
        }
      }
      else 
      {
        if (pAlbum->m_iVirtual == 0)
        {
          // Database holds feed album, and we are adding library album
          bRemovePrevious = true;
          iPreviousID = iID;
        }
        else
        {
          // Database holds feed album, and we are adding feed album
          delete pDS;
          return iID;
        }
      }
    }

    LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAlbum, Adding new album %s, virtual = %d, path = %s (musicresolver) (audiodb)",
        pAlbum->m_strTitle.c_str(), pAlbum->m_iVirtual, pAlbum->GetPath().c_str());

    time_t now = time(NULL);

    iID = Insert("insert into albums (idAlbum, strTitle, strPath, iNumTracks, idArtist, iDuration, strArtwork, strDescription, strLanguage, "
        "strGenre, iYear, iVirtual, iRating, iDateAdded, iDateModified, iDropped, iStatus) "
        "values( NULL, '%s', '%s', %i, %i, %i, '%s', '%s', '%s', '%s', %i, %i, %i, %i, %i, 0, 0)",
        pAlbum->m_strTitle.c_str(), pAlbum->GetPath().c_str(), pAlbum->m_iNumTracs, pAlbum->m_iArtistId, pAlbum->m_iDuration, pAlbum->m_strArtwork.c_str(), pAlbum->m_strDescription.c_str(),
        pAlbum->m_strLanguage.c_str(), pAlbum->m_strGenre.c_str(), pAlbum->m_iYear, pAlbum->m_iVirtual, pAlbum->m_iRating, now, pAlbum->m_iDateModified);

    if (iID == MEDIA_DATABASE_ERROR)
    {
      LOG(LOG_LEVEL_ERROR, "BXAudioDatabase::AddAlbum, Could not add album: %s for artist %d (musicresolver) (audiodb)", pAlbum->m_strTitle.c_str(), pAlbum->m_iArtistId);
    }

    delete pDS;
  }
  else {
    iID = MEDIA_DATABASE_ERROR;
  }

  if (bRemovePrevious)
  {
    // We have to delete the previous album here, because otherwise this might activate the trigger to remove the artist
    // in case of an only album for this artist
    LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAlbum, DELETE, Replacing virtual album %s with real one (musicresolver) (audiodb) ", pAlbum->m_strTitle.c_str());
    Dataset* pDS_t = Exec("delete from albums where idAlbum=%i", iPreviousID);

    if (pDS_t)
    {
      delete pDS_t;
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAlbum, Could not delete album %d, database error (audiodb)", iPreviousID);
    }
  }

  return iID;

}

// This function is used to update information regarding the existing album, after it has been resolved from external metadata source
int BXAudioDatabase::UpdateAlbum(const BXAlbum* pAlbum)
{
  if (pAlbum == NULL) {
    return MEDIA_DATABASE_ERROR;
  }

  int iId = pAlbum->m_iId;

  LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::UpdateAlbum, album id = %d, status = %d (musicresolver) (audiodb)", iId, pAlbum->m_iStatus);

  Dataset* pDS = Exec("update albums set strDescription='%s', iNumTracks=%i, strArtwork='%s', iYear=%i, iRating=%i, iStatus=%i where idAlbum=%i", 
      pAlbum->m_strDescription.c_str(), pAlbum->m_iNumTracs, pAlbum->m_strArtwork.c_str(), pAlbum->m_iYear, pAlbum->m_iRating, 
      pAlbum->m_iStatus, iId);

  if (pDS) 
  {
    delete pDS;
  }

  return MEDIA_DATABASE_OK;
}

int BXAudioDatabase::UpdateAlbumStatus(int iId, int iStatus)
{
  Dataset* pDS = Exec("update albums set iStatus=%d where idAlbum=%i", iStatus, iId);
  if (pDS) {
    delete pDS;
  }

  return MEDIA_DATABASE_OK;
}

void BXAudioDatabase::ResetAlbumStatus()
{
  Dataset* pDS = Exec("update albums set iStatus=%d where iStatus=%d", STATUS_NEW, STATUS_RESOLVING);
  if (pDS) {
    delete pDS;
  }
}

int BXAudioDatabase::GetUnresolvedAlbums(std::vector<BXAlbum*> &vecUnresolvedAlbums)
{
  int iResult = MEDIA_DATABASE_NOT_FOUND;

  // Get all albums where number of tracks is zero
  Dataset* pDS = Query("select * from albums a1 left outer join artists ar1 on a1.idArtist = ar1.idArtist where a1.iStatus=%d OR a1.iStatus=%d ", STATUS_NEW, STATUS_RETRY);

  Dataset* pDS2 = Exec("update albums set iStatus=%d where iStatus=%d OR iStatus=%d", STATUS_RESOLVING, STATUS_NEW, STATUS_RETRY);

  if (pDS2)
  {
    delete pDS2;
  }

  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof()) 
      {
        BXAlbum* pAlbum = new BXAlbum();
        LoadAlbumFromDataset(pDS, pAlbum);

        pAlbum->m_strArtist = pDS->fv("ar1.strName").get_asString();

        vecUnresolvedAlbums.push_back(pAlbum);
        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    else
    {
      // no unresolved albums
      iResult = MEDIA_DATABASE_NOT_FOUND;
    }
    delete pDS;
  }
  else 
  {
    LOG(LOG_LEVEL_ERROR, "Could not retreive the list of unresolved albums");
    return MEDIA_DATABASE_ERROR;
  }
  return iResult;
}

int BXAudioDatabase::AddAudio(const BXAudio* pAudio)
{

  LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAudio, path = %s, title = %s, idAlbum = %d, idArtist = %d (musicresolver)",
      pAudio->m_strPath.c_str(), pAudio->m_strTitle.c_str(), pAudio->m_iAlbumId, pAudio->m_iArtistId);

  int iID = MEDIA_DATABASE_ERROR;
  Dataset* pDS = Query("select idAudio from audio_files where strTitle like '%s' and idArtist=%i and idAlbum=%i",
      pAudio->m_strTitle.c_str(), pAudio->m_iAlbumId, pAudio->m_iArtistId);
  if (pDS)
  {
    if (pDS->num_rows() == 0) 
    {
      time_t now;
      time(&now);

      iID = Insert("insert into audio_files (idAudio, idFile, strPath, strTitle, idAlbum, idArtist,  "
          "idArtist1,  idArtist2, idArtist3, idArtist4, idArtist5, "
          "strGenre, iYear, iDuration, iDateAdded, iTrackNumber, iDateModified) "
          "values( NULL, %i, '%s', '%s', %i, %i, %i, %i, %i, %i, %i, '%s', %i, %i, %i, %i, %i)", 0, pAudio->m_strPath.c_str(),
          pAudio->m_strTitle.c_str(), pAudio->m_iAlbumId, pAudio->m_iArtistId,
          pAudio->GetArtist(0), pAudio->GetArtist(1), pAudio->GetArtist(2), pAudio->GetArtist(3), pAudio->GetArtist(4),
          pAudio->m_strGenre.c_str(), pAudio->m_iYear, pAudio->m_iDuration, now, pAudio->m_iTrackNumber, pAudio->m_iDateModified);
    }
    else
    {
      iID = pDS->fv("idAudio").get_asInt();
      LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::AddAudio, ALREADY EXISTS, id = %d, path = %s, title = %s, idAlbum = %d, idArtist = %d (musicresolver)",
            iID, pAudio->m_strPath.c_str(), pAudio->m_strTitle.c_str(), pAudio->m_iAlbumId, pAudio->m_iArtistId);

    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to find audio file %s", pAudio->m_strTitle.c_str());
  }
  return iID;
}

bool BXAudioDatabase::GetArtistByName(const std::string& strName, BXArtist* pArtist)
{
  bool bResult = false;

  Dataset* pDS = Query("select * from artists where strName like '%s'", strName.c_str());
  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      bResult = LoadArtistFromDataset(pDS, pArtist);
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not find artist %s", strName.c_str());
      bResult = false;
    }
    delete pDS;
  }

  return bResult;
  }

bool BXAudioDatabase::GetArtistById(int iArtistId, BXArtist* pArtist)
{
  bool bResult = false;

  Dataset* pDS = Query("select * from artists where idArtist=%d", iArtistId);

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        bResult = LoadArtistFromDataset(pDS, pArtist);
      }
      else
      {
        bResult = false;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get artist. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive artist");
    bResult = false;
  }
  return bResult;
}

int BXAudioDatabase::GetAlbumByPath(const std::string& strPath, BXAlbum* pAlbum, bool bGetDropped)
{
  if (!pAlbum) return MEDIA_DATABASE_ERROR;

  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDS = NULL;
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAlbumByPath, QUERY, select * from albums");
  if (bGetDropped) 
  {
    pDS = Query("select * from albums where strPath='%s'", strPath.c_str());
  }
  else 
  {
    pDS = Query("select * from albums where strPath='%s' and iDropped=0", strPath.c_str());
  }

  if (pDS) {
    try {

      if (pDS->num_rows() != 0)
      {
        // Get all album details
        iResult = LoadAlbumFromDataset(pDS, pAlbum);

        // Load artist
        BXArtist artist;
        GetArtistById(pAlbum->m_iArtistId, &artist);
        pAlbum->m_strArtist = artist.m_strName;
      }
      else {
        iResult = MEDIA_DATABASE_NOT_FOUND;
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get album. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult =  MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }
  else {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive album");
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;

}

// Retrieves the album using internal database id of the album
bool BXAudioDatabase::GetAlbumById(int iAlbumId, BXAlbum* pAlbum, bool bGetDropped)
{
  if (!pAlbum) return false;

  int bResult = false;

  Dataset* pDS = NULL;

  if (bGetDropped)
  {
    pDS = Query("select * from albums where idAlbum=%d", iAlbumId);
  }
  else 
  {
    pDS = Query("select * from albums where idAlbum=%d and iDropped=0", iAlbumId);
  }

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        // Get all album details
        bResult = LoadAlbumFromDataset(pDS, pAlbum);

        // Load artist
        BXArtist artist;
        GetArtistById(pAlbum->m_iArtistId, &artist);
        pAlbum->m_strArtist = artist.m_strName;
      }
      else
      {
        bResult = false;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get album. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult =  false;
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive album");
    bResult = false;
  }

  return bResult;

}

bool BXAudioDatabase::LoadAlbumFromDataset(Dataset* pDS, BXAlbum* pAlbum)
{
  if (!pDS || !pAlbum) return MEDIA_DATABASE_ERROR;

  try {
    pAlbum->m_iId = pDS->fv("albums.idAlbum").get_asInt();
    pAlbum->m_strTitle = pDS->fv("albums.strTitle").get_asString();
    pAlbum->SetPath(pDS->fv("albums.strPath").get_asString());
    pAlbum->m_iNumTracs = pDS->fv("albums.iNumTracks").get_asInt();
    pAlbum->m_iArtistId = pDS->fv("albums.idArtist").get_asInt();
    pAlbum->m_iDuration = pDS->fv("albums.iDuration").get_asInt();
    pAlbum->m_strArtwork = pDS->fv("albums.strArtwork").get_asString();
    pAlbum->m_strDescription = pDS->fv("albums.strDescription").get_asString();
    pAlbum->m_strLanguage = pDS->fv("albums.strLanguage").get_asString();
    pAlbum->m_strGenre = pDS->fv("albums.strGenre").get_asString();
    pAlbum->m_iYear = pDS->fv("albums.iYear").get_asInt();
    pAlbum->m_iVirtual = pDS->fv("albums.iVirtual").get_asInt();
    pAlbum->m_iRating = pDS->fv("albums.iRating").get_asInt();
    pAlbum->m_iDateAdded = pDS->fv("albums.iDateAdded").get_asUInt();
    pAlbum->m_iDateModified = pDS->fv("albums.iDateModified").get_asUInt();
    pAlbum->m_iStatus = pDS->fv("albums.iStatus").get_asInt();
  }
  catch(dbiplus::DbErrors& e) {
    LOG(LOG_LEVEL_ERROR, "Could not load album from dataset. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    return false;
  }
  return true;
}

bool BXAudioDatabase::LoadAudioFromDataset(Dataset* pDS, BXAudio* pAudio)
{
  if (!pDS || !pAudio) return false;

  try
  {
    pAudio->m_strTitle = pDS->fv("audio_files.strTitle").get_asString();
    pAudio->m_iAlbumId = pDS->fv("audio_files.idAlbum").get_asInt();
    pAudio->m_iArtistId = pDS->fv("audio_files.idArtist").get_asInt();
    pAudio->m_iYear = pDS->fv("audio_files.iYear").get_asInt();
    pAudio->m_iDuration = pDS->fv("audio_files.iDuration").get_asInt();
    pAudio->m_iTrackNumber = pDS->fv("audio_files.iTrackNumber").get_asInt();
    pAudio->m_iDateAdded = pDS->fv("audio_files.iDateAdded").get_asUInt();
    pAudio->m_iDateModified = pDS->fv("audio_files.iDateModified").get_asUInt();
    pAudio->m_vecArtists.push_back(pDS->fv("audio_files.idArtist1").get_asInt());
    pAudio->m_vecArtists.push_back(pDS->fv("audio_files.idArtist2").get_asInt());
    pAudio->m_vecArtists.push_back(pDS->fv("audio_files.idArtist3").get_asInt());
    pAudio->m_vecArtists.push_back(pDS->fv("audio_files.idArtist4").get_asInt());
    pAudio->m_vecArtists.push_back(pDS->fv("audio_files.idArtist5").get_asInt());
    pAudio->m_strGenre = pDS->fv("audio_files.strGenre").get_asString();

  }
  catch(dbiplus::DbErrors& e)
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught, could not load all audio details from database. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    return false;
  }

  return true;

}

bool BXAudioDatabase::LoadArtistFromDataset(Dataset* pDS, BXArtist* pArtist)
{
  if (!pDS || !pArtist) return false;

  try
  {
    pArtist->m_strName = pDS->fv("artists.strName").get_asString();
    pArtist->m_strPortrait = pDS->fv("artists.strPortrait").get_asString();
    pArtist->m_iId  = pDS->fv("artists.idArtist").get_asInt();
  }
  catch(dbiplus::DbErrors& e)
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught, could not load all audio details from database. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    return false;
  }
  return true;

}


int BXAudioDatabase::GetAudioByPath(const std::string& strPath, BXMetadata* pMetadata)
{
  if (!pMetadata || pMetadata->GetType() != MEDIA_ITEM_TYPE_AUDIO) return MEDIA_DATABASE_ERROR;

  LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudioByPath, Getting audio by path = %s (audiodb)", pMetadata->GetPath().c_str());

  // Get all audio files
  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDS = Query("select * from audio_files where strPath='%s'", strPath.c_str());
  if (pDS)
  {
    try {

      if (pDS->num_rows() != 0)
      {
        BXAudio* pAudio = (BXAudio*) pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);
        BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
        BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

        pAudio->m_strPath = pDS->fv("audio_files.strPath").get_asString();
        pAudio->m_iId = pDS->fv("audio_files.idAudio").get_asInt();

        LoadAudioFromDataset(pDS, pAudio);

        int idAlbum = pDS->fv("audio_files.idAlbum").get_asInt();
        int idArtist = pDS->fv("audio_files.idArtist").get_asInt();

        GetArtistById(idArtist, pArtist);
        iResult = GetAlbumById(idAlbum, pAlbum);

        if (pAudio->m_strPath == "")
        {
          std::string strPath = "boxeedb://song/";
          strPath += BXUtils::IntToString(pAudio->m_iId);
          pAudio->m_strPath = strPath;
        }

        // The the path of the audio as path of the whole item
        pMetadata->SetPath(pAudio->m_strPath);

        LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudioByPath, album title = %s, artist = %s (audiodb)", pAlbum->m_strTitle.c_str(), pArtist->m_strName.c_str());

      } // if
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "BXAudioDatabase::GetAudioByPath, Exception caught, could not get music files. Error = %s, msg= %s (audiodb)", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when looking for audio %s", strPath.c_str());
  }

  return iResult;

}

bool BXAudioDatabase::GetAlbum(const std::string& strAlbum, const std::string& strArtist, BXMetadata* pMetadata,  bool bGetDropped)
{
  if (strAlbum.empty() || strArtist.empty()) return false;

  bool bResult = false;

  BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
  BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

  bResult = GetArtistByName(strArtist, pArtist);
  if (!bResult)
  {
    return false;
  }

  Dataset* pDS = NULL;
  if (bGetDropped)
  {
    pDS = Query("select * from albums where strTitle like '%s' and idArtist='%i'", strAlbum.c_str(), pArtist->m_iId);
  }
  else 
  {
    pDS = Query("select * from albums where strTitle like '%s' and idArtist='%i' and iDropped=0", strAlbum.c_str(), pArtist->m_iId);
  }

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        bResult = LoadAlbumFromDataset(pDS, pAlbum);
        pAlbum->m_strArtist = strArtist;
      }
      else
      {
        bResult = false;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get album. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive album %s for artist %s", strAlbum.c_str(), strArtist.c_str());
    bResult = false;
  }

  return bResult;
}

int BXAudioDatabase::GetAudiosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& strFolderPath)
{
  int iResult = MEDIA_DATABASE_ERROR;

  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudiosByFolder, QUERY, select audio_files.*, artists.*, albums.* from audio_files, artists, albums");
  Dataset* pDS = Query("select audio_files.*, artists.*, albums.* from audio_files, artists, albums "
      "where audio_files.idArtist=artists.idArtist AND audio_files.idAlbum=albums.idAlbum AND audio_files.strPath LIKE '%s%%'", strFolderPath.c_str());

  if (pDS)
  {
    try {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          std::string strPath = pDS->fv("audio_files.strPath").get_asString();
          // Take only the files that are immediately under the specified folder
          if (BXUtils::GetParentPath(strPath) != strFolderPath)
          {
            pDS->next();
            continue;
          }

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_AUDIO);
          BXAudio* pAudio = (BXAudio*) pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);
          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
          BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

          pAudio->m_strPath = strPath;
          pAudio->m_iId = pDS->fv("audio_files.idAudio").get_asInt();

          // The the path of the audio as path of the whole item
          pMetadata->SetPath(pAudio->m_strPath);

          LoadAudioFromDataset(pDS, pAudio);
          LoadArtistFromDataset(pDS, pArtist);
          LoadAlbumFromDataset(pDS, pAlbum);

          mapMediaFiles[pAudio->m_strPath] = pMetadata;

          pDS->next();
        }

        iResult = MEDIA_DATABASE_OK;

      } // if
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }

  return iResult;

}
int BXAudioDatabase::GetUnresolvedAudioFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId)
{

  int iResult = MEDIA_DATABASE_ERROR;

  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudiosByFolder, QUERY, select audio_files.*, artists.*, albums.* from audio_files, artists, albums");
  Dataset* pDS = Query("select * from  unresolved_audio_files where idFolder=%d", folderId);

  if (pDS)
  {
    try {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          std::string strPath = pDS->fv("strPath").get_asString();

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_AUDIO);
          BXAudio* pAudio = (BXAudio*) pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);

          pAudio->m_strPath = strPath;
          pAudio->m_iId = pDS->fv("idAudio").get_asInt();

          // The the path of the audio as path of the whole item
          pMetadata->SetPath(pAudio->m_strPath);

          mapMediaFiles[pAudio->m_strPath] = pMetadata;

          pDS->next();
        }

        iResult = MEDIA_DATABASE_OK;

      } // if
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }

  return iResult;

}

int BXAudioDatabase::GetAlbumsByTitleAndArtist(const std::string& strTitle, int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, bool bGetDropped)
{
  // Load the artist
  BXArtist artist;
  GetArtistById(iArtistId, &artist);

  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDS = NULL;
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAlbumsByTitleAndArtist, QUERY, select * from albums");
  if (bGetDropped)
  {
    pDS = Query("select * from albums where iVirtual=0 and idArtist=%d and strTitle='%s'", iArtistId, strTitle.c_str());
  }
  else 
  {
    pDS = Query("select * from albums where iVirtual=0 and idArtist=%d and strTitle='%s' and iDropped=0", iArtistId, strTitle.c_str());
  }

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ALBUM);
          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
          BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

          *pArtist = artist;

          if (LoadAlbumFromDataset(pDS, pAlbum) == MEDIA_DATABASE_ERROR)
          {
            iResult = MEDIA_DATABASE_ERROR;
            delete pMetadata;
            break;
          }

          std::string strAlbumPath = "boxeedb://album/?id=";
          strAlbumPath += BXUtils::IntToString(pAlbum->m_iId);
          strAlbumPath += "/";
          LOG(LOG_LEVEL_DEBUG, "LIBRARY: Album name %s, album path = %s", pAlbum->m_strTitle.c_str(), strAlbumPath.c_str());
          pMetadata->SetPath(strAlbumPath);

          vecMediaFiles.push_back(pMetadata);
          pDS->next();

        } // while
        iResult = MEDIA_DATABASE_OK;
      }
      else
      {
        iResult = MEDIA_DATABASE_NOT_FOUND;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive audio_files");
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;

}

int BXAudioDatabase::GetAlbumsByTitleAndArtist(const std::string& strTitle, const std::string& strArtist, std::vector<BXMetadata*> &vecMediaFiles, bool bGetDropped)
{
  // Load the artist
  BXArtist artist;
  if (GetArtistByName(strArtist, &artist))
    return GetAlbumsByTitleAndArtist(strTitle, artist.m_iId, vecMediaFiles, bGetDropped);
  else
    return MEDIA_DATABASE_ERROR;

}


int BXAudioDatabase::GetAlbumsByArtist(int iArtistId, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter)
{
  // Load the artist
  BXArtist artist;
  GetArtistById(iArtistId, &artist);

  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDS = Query("select * from albums where iVirtual=0 and idArtist=%d and iDropped=0", iArtistId);


  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {

          std::string strPath = pDS->fv("strPath").get_asString();

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
          {
            pDS->next();
            continue; // skip this file and move on to the next one
          }

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ALBUM);
          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
          BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

          *pArtist = artist;

          if (LoadAlbumFromDataset(pDS, pAlbum) == MEDIA_DATABASE_ERROR)
          {
            iResult = MEDIA_DATABASE_ERROR;
            delete pMetadata;
            break;
          }

          // Check if this album does not already exist in the list
          bool bFound  = false;
          BXAlbum* tempAlbum = NULL;
          BXMetadata* pTempMetadata = NULL;
          unsigned int i=0;
          for (; i < vecMediaFiles.size() && !bFound; i++)
          {
            pTempMetadata = vecMediaFiles[i];
            if (pTempMetadata->GetType() == MEDIA_ITEM_TYPE_ALBUM)
            {
              tempAlbum = (BXAlbum*)pTempMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
              if (tempAlbum && tempAlbum->m_strTitle == pAlbum->m_strTitle && tempAlbum->m_iArtistId== pAlbum->m_iArtistId)
              {
                bFound = true;
              }
            }
          }

          if (bFound)
          {
            // Create group with the specified album title and artist id
            std::string strAlbumPath = "boxeedb://album/?title=";
            strAlbumPath += BXUtils::URLEncode(pAlbum->m_strTitle);
            strAlbumPath += "&artistId=";
            strAlbumPath += BXUtils::IntToString(pAlbum->m_iArtistId);
            strAlbumPath += "/";

            tempAlbum->SetPath(strAlbumPath);
            pTempMetadata->m_strThumbnail = pDS->fv("albums.strArtwork").get_asString();
            pTempMetadata->m_strTitle = pArtist->m_strName;
            pTempMetadata->m_strTitle += " - ";
            pTempMetadata->m_strTitle += pAlbum->m_strTitle;

            pTempMetadata->SetPath(strAlbumPath);
            pTempMetadata->m_bGroup = true;

            //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Album exists, replaced with group: name %s, album path = %s, index = %d", tempAlbum->m_strTitle.c_str(), strAlbumPath.c_str(),i);
            delete pMetadata;

          }
          else
          {
            std::string strAlbumPath = "boxeedb://album/?id=";
            strAlbumPath += BXUtils::IntToString(pAlbum->m_iId);
            strAlbumPath += "/";
            //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Album added, name %s, album path = %s", pAlbum->m_strTitle.c_str(), strAlbumPath.c_str());
            pMetadata->SetPath(strAlbumPath);
            vecMediaFiles.push_back(pMetadata);
          }

          pDS->next();

        } // while
        iResult = MEDIA_DATABASE_OK;
      }
      else {
        iResult = MEDIA_DATABASE_NOT_FOUND;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive audio_files");
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

int BXAudioDatabase::GetSongsFromAlbum(int iAlbumId, std::vector<BXMetadata*> &vecMediaFiles)
{
  int iResult = MEDIA_DATABASE_ERROR;

  BXAlbum album;
  BXArtist artist;

  GetAlbumById(iAlbumId, &album);
  GetArtistById(album.m_iArtistId, &artist);

  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetSongsFromAlbum, QUERY, select audio_files.* from audio_files");
  Dataset* pDS = Query("select audio_files.* from audio_files where audio_files.idAlbum=%d order by iTrackNumber", iAlbumId);

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while(!pDS->eof())
        {
          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_AUDIO);
          BXAudio* pAudio = (BXAudio*) pMetadata->GetDetail(MEDIA_DETAIL_AUDIO);
          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
          BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

          *pAlbum = album;
          *pArtist = artist;


          pAudio->m_strPath = pDS->fv("audio_files.strPath").get_asString();
          pAudio->m_strAlbum = pAlbum->m_strTitle;
          pAudio->m_iId = pDS->fv("audio_files.idAudio").get_asInt();

          // In case this is a virtual song that does not exist on the drive, provide a database path
          if (pAudio->m_strPath == "")
          {
            std::string strPath = "boxeedb://song/";
            strPath += BXUtils::IntToString(pAudio->m_iId);
            pAudio->m_strPath = strPath;
          }

          // The the path of the audio as path of the whole item
          pMetadata->SetPath(pAudio->m_strPath);

          LoadAudioFromDataset(pDS, pAudio);

          vecMediaFiles.push_back(pMetadata);
          pDS->next();
        } // while
        iResult = MEDIA_DATABASE_OK;
      } // if
      else {
        iResult = MEDIA_DATABASE_NOT_FOUND;
      }

    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive audio file");
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

int BXAudioDatabase::GetAudios(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{

  // The logic of item limit works as follows, we need to return the total of X items,
  // however, several items extracted from the database can land in the same BXMetadata item.
  // For example several files that do not have metadata may be added to the same directory
  // or several files that have metadata may land in the same album.
  // We therefore may need to extract more from the database.

  int iResult = MEDIA_DATABASE_ERROR;
  Dataset* pDS;

  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudios, QUERY, select m1.*, a1.*, ar1.* from media_folders m1 left outer join albums");
  pDS = Query("select m1.*, a1.*, ar1.* from media_folders m1 left outer join albums a1 on m1.iMetadataId = a1.idAlbum "
      "left outer join artists ar1 on a1.idArtist = ar1.idArtist where strType='musicFolder' and iHasMetadata=1 and a1.iDropped=0");

  if (pDS) 
  {
    try 
    {

      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0) // go over all media folders
        {

          // Print file path, for debug purpose
          std::string strPath = pDS->fv("strPath").get_asString();
          //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Handling audio folder path = %s", strPath.c_str());

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
          {
            pDS->next();
            continue; // skip this file and move on to the next one
          }

          int iArtistId = pDS->fv("ar1.idArtist").get_asInt();

          // Check if the file has metadata
          int iHasMetadata = pDS->fv("iHasMetadata").get_asInt();


          if (iHasMetadata == 1) 
          {
            // We have metadata for this folder

            // Check if the album was already added to the list
            bool bFound  = false;
            for (unsigned int i=0; i < vecMediaFiles.size() && !bFound; i++) 
            {
              if (vecMediaFiles[i]->GetType() == MEDIA_ITEM_TYPE_ARTIST) 
              {
                BXArtist* tempArtist = (BXArtist*)vecMediaFiles[i]->GetDetail(MEDIA_DETAIL_ARTIST);
                if (tempArtist && tempArtist->m_iId == iArtistId) 
                {
                  bFound = true;
                  break;
                }
              }
            }

            if (!bFound) 
            {

              BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ARTIST);
              pMetadata->SetFolder(strPath);

              // Load artist

              BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
              if (LoadArtistFromDataset(pDS, pArtist) == MEDIA_DATABASE_OK)
              {
                std::string strArtistPath = "boxeedb://artist/";
                strArtistPath += BXUtils::IntToString(iArtistId);
                strArtistPath += "/";
                LOG(LOG_LEVEL_DEBUG, "LIBRARY, AUDIO DATABASE, LIB3: Artist name %s, artist path = %s", pArtist->m_strName.c_str(), strArtistPath.c_str());
                pMetadata->SetPath(strArtistPath);
              }

              BXAlbum* pAlbum = (BXAlbum*)pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
              LoadAlbumFromDataset(pDS, pAlbum);

              vecMediaFiles.push_back(pMetadata);
              iItemLimit--;
            }
            else
            {
              //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Album id = %d already added to the list", iAlbumId);
            }
          }
          else 
          {

            //LOG(LOG_LEVEL_DEBUG, "LIBRARY, AUDIO DATABASE, LIB2, Creating directory item for id = %d path = %s", iId, strPath.c_str());
            strPath = BXUtils::GetSubShareFolderName(vecPathFilter, strPath);

            bool bFound  = false;
            for (unsigned int i=0; i < vecMediaFiles.size() && !bFound; i++) 
            {
              if (vecMediaFiles[i]->GetType() == MEDIA_ITEM_TYPE_DIR) 
              {
                if (vecMediaFiles[i]->GetPath() == strPath) 
                {
                  bFound = true;
                }
              }
            }

            if (!bFound) 
            {
              BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_DIR, strPath);
              pMetadata->m_iDateAdded = pDS->fv("iDateAdded").get_asInt();
              pMetadata->m_iDateModified = pDS->fv("iDateModified").get_asInt();
              vecMediaFiles.push_back(pMetadata);
              iItemLimit--;
            }
          }
          pDS->next();
        } // while

      }
      iResult = MEDIA_DATABASE_OK;
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else 
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive media files of type 'audio'");
    iResult = MEDIA_DATABASE_ERROR;
  }

  LOG(LOG_LEVEL_DEBUG, "LIBRARY, DATABASE, LIB2, Audio database retreived %d music items", vecMediaFiles.size());

  return iResult;
}

int BXAudioDatabase::GetAllArtists(std::map<std::string, BXArtist> &mapArtists)
{
  int iResult = MEDIA_DATABASE_OK;

  Dataset* pDS = Query("select idArtist,strName,strPortrait from artists");

  if (pDS) // query successful
  {
    try 
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          BXArtist artist;
          artist.m_iId  = pDS->fv(0).get_asInt();
          artist.m_strName = pDS->fv(1).get_asString();
          artist.m_strPortrait = pDS->fv(2).get_asString();
          mapArtists[artist.m_strName] = artist;
          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get all artists. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else 
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive all artists");
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

int BXAudioDatabase::GetArtists(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit, const std::string& strName)
{
  int iResult = MEDIA_DATABASE_OK;

  Dataset* pDS = NULL;
  if (strName.empty())
  {
    pDS = Query("select ar.strName,ar.strPortrait,ar.idArtist,al.strPath from artists ar,albums al where ar.idArtist = al.idArtist and al.iVirtual=0");
  }
  else
  {
    pDS = Query("select ar.strName,ar.strPortrait,ar.idArtist,al.strPath from artists ar,albums al where ar.idArtist = al.idArtist and al.iVirtual=0 and (ar.strName LIKE '%s%%' or ar.strName LIKE ' %s%%') ",
        strName.c_str(), strName.c_str());
  }

  std::set<int> setArtists;

  if (pDS) // query successful
  {
    try 
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0)
        {
          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ARTIST);
          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);

          pArtist->m_strName = pDS->fv(0).get_asString();
          pArtist->m_strPortrait = pDS->fv(1).get_asString();
          pArtist->m_iId  = pDS->fv(2).get_asInt();

          std::string strPath = pDS->fv(3).get_asString();

          // Check that this artist has available albums, otherwise do not add it
          if (setArtists.find(pArtist->m_iId) != setArtists.end() || !BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
          {
            pDS->next();
            delete pMetadata;
            continue; // skip this file and move on to the next one
          }

          setArtists.insert(pArtist->m_iId);

          // Build artist path
          std::string strArtistPath = "boxeedb://artist/";
          strArtistPath += BXUtils::IntToString(pArtist->m_iId);
          strArtistPath += "/";
          pMetadata->SetPath(strArtistPath);

          vecMediaFiles.push_back(pMetadata);
          iItemLimit--;

          pDS->next();

        } // while

      }
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get artists. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else 
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive artists");
    iResult = MEDIA_DATABASE_ERROR;
  }

  LOG(LOG_LEVEL_DEBUG, "LIBRARY, DATABASE, LIB2, Audio database retreived %d music items", vecMediaFiles.size());

  return iResult;
}

bool BXAudioDatabase::SearchMusic(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  std::vector<BXMetadata*> vecAlbums;
  Dataset* pDS = Query("select a1.*, ar1.* from albums a1 "
      "left outer join artists ar1 on a1.idArtist = ar1.idArtist where (a1.strTitle LIKE '%s%%' or a1.strTitle LIKE ' %s%%') and a1.iVirtual=0",
      strTitle.c_str(), strTitle.c_str() );

  bool bGotAlbums = CreateAlbumsFromDataset(pDS, vecAlbums, vecPathFilter, iItemLimit);

  std::vector<BXMetadata*> vecArtists;
  bool bGotArtists = GetArtists(vecArtists, vecPathFilter, iItemLimit, strTitle);

  vecMediaFiles.insert( vecMediaFiles.end(), vecAlbums.begin(), vecAlbums.end() );
  vecMediaFiles.insert( vecMediaFiles.end(), vecArtists.begin(), vecArtists.end() );

  return bGotAlbums && bGotArtists;

}

bool BXAudioDatabase::GetAlbums(std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  Dataset* pDS = Query("select a1.*, ar1.* from albums a1 "
      "left outer join artists ar1 on a1.idArtist = ar1.idArtist where a1.iVirtual=0");

  return CreateAlbumsFromDataset(pDS, vecMediaFiles, vecPathFilter, iItemLimit);
}

bool BXAudioDatabase::CreateAlbumsFromDataset(Dataset* pDS, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  bool bResult = true;
  if (pDS) // query successful
  {
    try 
    {
      if (pDS->num_rows() != 0)
      {
        std::map<std::pair<std::string, int>,  BXMetadata*> mapDuplicates;

        while (!pDS->eof() && iItemLimit > 0) // go over all media folders
        {
          std::string strPath = pDS->fv("a1.strPath").get_asString();

          // Check that the album path matches provided filter

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
          {
            pDS->next();
            continue; // skip this file and move on to the next one
          }


          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_ALBUM);
          pMetadata->SetFolder(strPath);

          BXArtist* pArtist = (BXArtist*) pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
          BXAlbum* pAlbum = (BXAlbum*) pMetadata->GetDetail(MEDIA_DETAIL_ALBUM);

          if (LoadAlbumFromDataset(pDS, pAlbum) == MEDIA_DATABASE_ERROR) 
          {
            bResult = false;
            delete pMetadata;
            break;
          }

          // Check if this album does not already exist in the list
          bool bFound  = false;
          BXAlbum* tempAlbum = NULL;
          BXMetadata* pTempMetadata = NULL;

          std::pair<std::string, int> albumPair = std::make_pair(pAlbum->m_strTitle, pAlbum->m_iArtistId);
          std::map<std::pair<std::string, int>, BXMetadata*>::iterator it = mapDuplicates.find(albumPair);
          if (it != mapDuplicates.end()) 
          {
            pTempMetadata = it->second;
            if (pTempMetadata->GetType() == MEDIA_ITEM_TYPE_ALBUM) 
            {
              tempAlbum = (BXAlbum*)pTempMetadata->GetDetail(MEDIA_DETAIL_ALBUM);
              bFound = true;
            }
          }

          if (bFound) 
          {
            // TODO: Handle groups differently

            // Create group with the specified album title and artist id
            std::string strAlbumPath = "boxeedb://album/?title=";
            strAlbumPath += BXUtils::URLEncode(pAlbum->m_strTitle);
            strAlbumPath += "&artistId=";
            strAlbumPath += BXUtils::IntToString(pAlbum->m_iArtistId);
            strAlbumPath += "/";

            tempAlbum->SetPath(strAlbumPath);
            pTempMetadata->m_strThumbnail = pDS->fv("albums.strArtwork").get_asString();
            pTempMetadata->m_strTitle = pArtist->m_strName;
            pTempMetadata->m_strTitle += " - ";
            pTempMetadata->m_strTitle += pAlbum->m_strTitle;

            pTempMetadata->SetPath(strAlbumPath);
            pTempMetadata->m_bGroup = true;

            // Get the smallest date added of the two, to get consistent results

            tempAlbum->m_iDateAdded = (tempAlbum->m_iDateAdded < pAlbum->m_iDateAdded ? tempAlbum->m_iDateAdded : pAlbum->m_iDateAdded);

            pTempMetadata->m_iDateAdded = tempAlbum->m_iDateAdded;
            pTempMetadata->m_iDateModified = tempAlbum->m_iDateModified;

            //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Album exists, replaced with group: name %s, album path = %s, index = %d", tempAlbum->m_strTitle.c_str(), strAlbumPath.c_str(),i);
            delete pMetadata;

          }
          else 
          {
            std::string strAlbumPath = "boxeedb://album/?id=";
            strAlbumPath += BXUtils::IntToString(pAlbum->m_iId);
            strAlbumPath += "/";
            pMetadata->SetPath(strAlbumPath);
            BXArtist* pArtist = (BXArtist*)pMetadata->GetDetail(MEDIA_DETAIL_ARTIST);
            LoadArtistFromDataset(pDS, pArtist);

            pMetadata->m_iDateAdded = pAlbum->m_iDateAdded;
            pMetadata->m_iDateModified = pAlbum->m_iDateModified;

            std::pair<std::string, int> albumPair = std::make_pair(pAlbum->m_strTitle, pAlbum->m_iArtistId);
            mapDuplicates.insert(std::pair<std::pair<std::string, int >, BXMetadata* >(albumPair, pMetadata));

            vecMediaFiles.push_back(pMetadata);
            iItemLimit--;
          }

          pDS->next();

        } // while
      }
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get music files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else 
  {
    LOG(LOG_LEVEL_ERROR, "Error when trying to retreive media files of type 'audio'");
    bResult = false;
  }

  return bResult;
}

int BXAudioDatabase::RemoveAudioByPath(const std::string& strPath)
{
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::RemoveAudioByPath, QUERY, delete from audio_files");
  Dataset* pDS_t = Exec("delete from audio_files where strPath='%s'", strPath.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXAudioDatabase::RemoveAudioByFolder(const std::string& strFolderPath)
{
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::RemoveAudioByFolder, QUERY, delete from audio_files");
  Dataset* pDS_t = Exec("delete from audio_files where strPath LIKE '%s%%'", strFolderPath.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }

  return MEDIA_DATABASE_ERROR;
}

int BXAudioDatabase::RemoveAlbumById(int iId)
{
  Dataset* pDS_t = Exec("delete from audio_files where idAlbum = %d", iId);
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }

  return MEDIA_DATABASE_ERROR;
}

bool BXAudioDatabase::DropAlbumById(int iId)
{
  LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::DropAlbumById, DROP: drop album,  id =  %d", iId);
  Dataset* pDS = Exec("update albums set iDropped=1 where idAlbum = %i", iId);
  if (pDS)
  {
    delete pDS;
    return true;
  }
  return false;
}

int BXAudioDatabase::GetMusicGenres(std::vector<std::string>& vecGenres)
{
  int iResult = MEDIA_DATABASE_ERROR;
  Dataset*pDS = Query("select distinct strGenre from albums");

  if (pDS)
  {
    try
    {

      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof()) // go over all media folders
        {
          std::string strGenre = pDS->fv("strGenre").get_asString();
          if (strGenre != "")
            vecGenres.push_back(strGenre);
          pDS->next();
        }
      }
      else
      {
        LOG(LOG_LEVEL_ERROR, "Could not get list of genres, query is empty. (genre)");
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get list of genres. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Dataset is null. (genre)");
  }
  return MEDIA_DATABASE_ERROR;
}

int BXAudioDatabase::AddAudioFile(const std::string& strPath, const std::string& strSharePath, int iFolderId, int64_t size)
{
  int iID = MEDIA_DATABASE_ERROR;

  Dataset* pDS = Query("select * from unresolved_audio_files where strPath='%s'", strPath.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      iID = pDS->fv("idAudioFile").get_asInt();
}
    delete pDS;
  }

  if (iID != MEDIA_DATABASE_ERROR)
    return iID;

  iID = Insert("insert into unresolved_audio_files (idAudioFile, strPath, strSharePath, idFolder, iStatus, idAudio, iSize) values (NULL, '%s', '%s', %i, %i, %i, %lld)",
      strPath.c_str(), strSharePath.c_str(), iFolderId, STATUS_UNRESOLVED, -1, size);

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not insert video file , %s", strPath.c_str());
  }

  return iID;
}

int BXAudioDatabase::UpdateAudioFileStatus(const std::string& strPath, int iStatus, int idAudio)
{
  Dataset* pDS = Exec("update unresolved_audio_files set iStatus=%i, idAudio=%i where strPath='%s'",iStatus, idAudio, strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXAudioDatabase::RemoveUnresolvedAudioByPath(const std::string& strPath)
{
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::RemoveAudioByPath, QUERY, delete from audio_files");
  Dataset* pDS_t = Exec("delete from unresolved_audio_files where strPath='%s'", strPath.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}
int BXAudioDatabase::RemoveUnresolvedAudioByFolder(const std::string& strFolder)
{
  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::RemoveAudioByFolder, QUERY, delete from audio_files");
  Dataset* pDS_t = Exec("delete from unresolved_audio_files where strPath LIKE '%s%%'", strFolder.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int  BXAudioDatabase::GetShareUnresolvedAudioFilesCount(const std::string& iStrSharePath, int iStatus)
{
  int count = 0;
  Dataset* pDS =NULL;

  if (iStatus == STATUS_ALL)
  {
    pDS = Query("select  count(*) from unresolved_audio_files where strSharePath='%s' ",iStrSharePath.c_str());
  }
  else
  {
    pDS = Query("select  count(*) from unresolved_audio_files where strSharePath='%s' and iStatus=%d  ",iStrSharePath.c_str() ,iStatus);
  }
  if (pDS)
  {
        count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return count;
}

int BXAudioDatabase::GetUserUnresolvedAudioFilesCount(const std::string& iSharesList, int iStatus)
{
  int count = 0;

  Dataset* pDS = UnQuotedQuery( "select  count(*) from unresolved_audio_files where strSharePath in (%s) and iStatus=%d  ",iSharesList.c_str() ,iStatus);
  if (pDS)
  {
        count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return count;

}
bool BXAudioDatabase::AreAudioFilesBeingScanned(const std::string& iSharesList)
{
  int count = 0;

  Dataset* pDS = UnQuotedQuery( "select  count(*) from media_shares where strType='music' and strPath in (%s) and iLastScanned=1  ",iSharesList.c_str());
  if (pDS)
  {
        count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return (count > 0) ;

}

}

