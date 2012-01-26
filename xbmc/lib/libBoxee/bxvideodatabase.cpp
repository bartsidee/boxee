
#include "bxvideodatabase.h"
#include "bxmediafile.h"
#include "bxmetadata.h"
#include "logger.h"
#include "bxutils.h"
#include "../../BoxeeUtils.h"
#include "bxconfiguration.h"
#include "bxobject.h"
#include "bxmessages.h"
#include "boxee.h"
#include "bxexceptions.h"
#include <time.h>
#include "../../MetadataResolverVideo.h"

#define GET_VIDEOS_QUERY "select v1.*, s1.strTitle \"sTitle\", s1.strBoxeeId \"sBoxeeId\", s1.strDescription \"sDescription\", s1.strCover \"sCover\", "\
    "s1.idSeries \"sIdSeries\", s1.strPath \"sPath\", s1.strLanguage \"sLanguage\", "\
    "s1.strGenre \"sGenre\", "\
    "s1.iYear \"sYear\", s1.iVirtual \"sVirtual\" , s1.strNfoPath \"sNfoPath\" , s1.iNfoAccessTime \"sNfoAccessTime\", s1.iNfoModifiedTime \"sNfoModifiedTime\" "\
    "from video_files v1 left outer join series s1 on v1.strSeriesId=s1.strBoxeeId where v1.idFile = 1 and v1.iDropped=0 "

#define GET_SERIES_QUERY "select s1.strTitle \"sTitle\", s1.strBoxeeId \"sBoxeeId\", s1.strDescription \"sDescription\", s1.strCover \"sCover\", "\
    "s1.idSeries \"sIdSeries\", s1.strPath \"sPath\", s1.strLanguage \"sLanguage\", s1.strGenre \"sGenre\", "\
    "s1.iYear \"sYear\", s1.iVirtual \"sVirtual\" , s1.strNfoPath \"sNfoPath\" , s1.iNfoAccessTime \"sNfoAccessTime\", s1.iNfoModifiedTime \"sNfoModifiedTime\", s1.iLatestLocalEpisodeDate \"sLatestEpisodeDate\" from series s1 "


using namespace dbiplus;

namespace BOXEE
{

CCriticalSection BOXEE::BXVideoDatabase::m_lock;

BXVideoDatabase::BXVideoDatabase()
{
  Open();
}

BXVideoDatabase::~BXVideoDatabase()
{

}

/*
 * Returns real videos located right under the specific folder
 */
int BXVideoDatabase::GetVideosByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, const std::string& _strFolderPath, bool bGetFiles)
{
  std::map<int, std::string> mapDirectors;
  MapDirectorsByVideoId(mapDirectors);

  std::map<int, std::vector<std::string> > mapActors;
  MapActorsByVideoId(mapActors);

  std::string strFolderPath = _strFolderPath;
  BXUtils::RemoveSlashAtEnd(strFolderPath);

  int iResult = MEDIA_DATABASE_ERROR;
  Dataset* pDSa = Query(GET_VIDEOS_QUERY " and v1.strPath LIKE '%s%%'", strFolderPath.c_str());

  if (pDSa)
  {
    if (pDSa->num_rows() != 0)
    {
      while (!pDSa->eof())
      {
        std::string strPath = pDSa->fv("v1.strPath").get_asString();
        std::string strType = pDSa->fv("v1.strType").get_asString();
        std::string strParentPath = BXUtils::GetParentPath(strPath);
        BXUtils::RemoveSlashAtEnd(strPath);
        BXUtils::RemoveSlashAtEnd(strParentPath);

        // In case of multipart video, the path of the item will be the path of the folder
        if (strPath != strFolderPath || strType != "part")
        {
          // Take only the files that are immediately under the specified folder
          if (strParentPath != strFolderPath)
          {
            pDSa->next();
            continue;
          }
          else if ((strParentPath == strFolderPath) && strType == "part")
          {
            pDSa->next();
            continue;
          }
        }

        BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);
        if (CreateVideoFromDataset(pDSa, pMetadata, mapDirectors, mapActors))
        {
          if (strType == "part" && bGetFiles)
          {
            BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

            std::vector<std::string> vecVideoParts;
            if (GetVideoParts(pVideo->m_iId, vecVideoParts))
            {
              for (unsigned int i = 0; i < vecVideoParts.size(); i++)
              {
                BXMetadata* pPart = new BXMetadata(*pMetadata);
                pPart->SetPath(vecVideoParts[i]);
                mapMediaFiles[pPart->GetPath()] = pPart;
              }
              delete pMetadata;
            }
          }
          else
          {
            mapMediaFiles[pMetadata->GetPath()] = pMetadata;
          }
        }
        else
        {
          delete pMetadata;
        }

        pDSa->next();

      } //while
    }
    else
    {
      iResult = MEDIA_DATABASE_NOT_FOUND;
    }
    delete pDSa;
  }
  else
  {
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

int BXVideoDatabase::GetUnresolvedVideoFilesByFolder(std::map<std::string, BXMetadata*> &mapMediaFiles, int folderId)
{

  int iResult = MEDIA_DATABASE_ERROR;

  //LOG(LOG_LEVEL_DEBUG, "BXAudioDatabase::GetAudiosByFolder, QUERY, select audio_files.*, artists.*, albums.* from audio_files, artists, albums");
  Dataset* pDS = Query("select * from unresolved_video_files where idFolder=%d", folderId);

  if (pDS)
  {
    try {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          std::string strPath = pDS->fv("strPath").get_asString();

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);
          BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

          pVideo->m_strPath = strPath;
          pVideo->m_iId = pDS->fv("idVideo").get_asInt();
          pVideo->m_iDateAdded = pDS->fv("iDateAdded").get_asUInt();

          // The the path of the audio as path of the whole item
          pMetadata->SetPath(pVideo->m_strPath);

          mapMediaFiles[pVideo->m_strPath] = pMetadata;

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


/*
 * Returns real videos from specific path
 */
int BXVideoDatabase::GetVideoById(int iId, BXMetadata* pMetadata)
{
  LOG(LOG_LEVEL_DEBUG, "LIBRARY, GetVideoById,  path = %d", iId);

  if (!pMetadata || !(pMetadata->GetType() == MEDIA_ITEM_TYPE_VIDEO)) return MEDIA_DATABASE_ERROR;

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*) pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDSa = Query(GET_VIDEOS_QUERY " and v1.idVideo = %d", iId);

  if (pDSa) {
    if (pDSa->num_rows() != 0)
    {
      if (LoadFullVideoFromDataset(pDSa, pVideo) == MEDIA_DATABASE_OK)
      {
        BXPath path;
        if (pVideo->m_strType == "part")
        {
          GetVideoParts(pVideo->m_iId, path.m_vecParts);
        }
        else
        {
          path.m_strPath = pVideo->m_strPath;
        }
        pVideo->m_vecLinks.push_back(path);

        if (!pVideo->m_strShowId.empty())
        {
          // Get the series details
          if (GetSeriesByBoxeeId(pVideo->m_strShowId, pSeries))
          {
            pMetadata->SetPath(pDSa->fv("strPath").get_asString());
            pMetadata->SetMediaFileId(pDSa->fv("idFile").get_asInt());
            iResult = MEDIA_DATABASE_OK;
          }
          else
          {
            iResult = MEDIA_DATABASE_ERROR;
          }
        }
        else
        {
          iResult = MEDIA_DATABASE_OK;
        }
      }
      else
      {
        iResult = MEDIA_DATABASE_ERROR;
      }
    }
    else
    {
      iResult = MEDIA_DATABASE_NOT_FOUND;
    }
    delete pDSa;
  }
  else
  {
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

/*
 * Returns real videos from specific path
 */
int BXVideoDatabase::GetVideoByPath(const std::string& strPath, BXMetadata* pMetadata)
{
  if (!pMetadata || !(pMetadata->GetType() == MEDIA_ITEM_TYPE_VIDEO)) return MEDIA_DATABASE_ERROR;

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*) pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDSa = Query(GET_VIDEOS_QUERY " and v1.strPath = '%s'", strPath.c_str());

  if (pDSa)
  {
    if (pDSa->num_rows() != 0)
    {
      if (LoadFullVideoFromDataset(pDSa, pVideo) == MEDIA_DATABASE_OK)
      {
        BXPath path;
        if (pVideo->m_strType == "part")
        {
          GetVideoParts(pVideo->m_iId, path.m_vecParts);
        }
        else
        {
          path.m_strPath = pVideo->m_strPath;
        }
        pVideo->m_vecLinks.push_back(path);

        if (!pVideo->m_strShowId.empty())
        {
          // Get the series details
          if (GetSeriesByBoxeeId(pVideo->m_strShowId, pSeries))
          {
            pMetadata->SetPath(strPath);
            pMetadata->SetMediaFileId(pDSa->fv("idFile").get_asInt());
            iResult = MEDIA_DATABASE_OK;
          }
          else
          {
            iResult = MEDIA_DATABASE_ERROR;
          }
        }
        else
        {
          iResult = MEDIA_DATABASE_OK;
        }
      }
      else
      {
        iResult = MEDIA_DATABASE_ERROR;
      }
    }
    else 
    {
      LOG(LOG_LEVEL_DEBUG, "LIBRARY, get part by path,  path = %s", strPath.c_str());
      // Check whether the path is found in the video parts
      Dataset* pDSb = Query("select * from video_parts where strPath = '%s'", strPath.c_str());
      if (pDSb)
      {
        if (pDSb->num_rows() != 0)
        {
          // Get the id of the video
          int idVideo = pDSb->fv("idVideo").get_asInt();
          iResult = GetVideoById(idVideo, pMetadata);
        }
        else
        {
          iResult = MEDIA_DATABASE_NOT_FOUND;
        }
      }
      else
      {
        iResult = MEDIA_DATABASE_ERROR;
      }
      delete pDSb; 
    }
    delete pDSa;
  }
  else
  {
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

/*
int BXVideoDatabase::GetVideoByIMDBId(const std::string& strId, BXMetadata* pMetadata)
{
  return GetVideoByStringField("strIMDBKey", strId, pMetadata);
}
*/
int BXVideoDatabase::GetVideoByBoxeeIdAndPath(const std::string& strId, const std::string& strPath, BXMetadata* pMetadata)
{
  CStdString _strPath = strPath;
  //look for path in db with userName and apssword if exists
  if(CUtil::IsSmb(_strPath))
  {
    _strPath = g_passwordManager.GetCifsPathCredentials(strPath);
  }

  if (!pMetadata || !(pMetadata->GetType() == MEDIA_ITEM_TYPE_VIDEO)) return MEDIA_DATABASE_ERROR;

   BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

   int iResult = MEDIA_DATABASE_ERROR;
   Dataset* pDSa = Query("select * from video_files where strBoxeeId='%s' and strPath = '%s' and iDropped=0", strId.c_str(), _strPath.c_str());
   if (pDSa) {
     if (pDSa->num_rows() != 0)
     {
       if (LoadFullVideoFromDataset(pDSa, pVideo) == MEDIA_DATABASE_OK)
       {
         if (pVideo->m_strType == "part")
         {
           GetVideoParts(pVideo->m_iId, pVideo->m_vecParts);
         }

         pMetadata->SetPath(pDSa->fv("strPath").get_asString());
         pMetadata->SetMediaFileId(pDSa->fv("idFile").get_asInt());
         iResult = MEDIA_DATABASE_OK;
       }
       else {
         iResult = MEDIA_DATABASE_ERROR;
       }
     }
     else {
       iResult = MEDIA_DATABASE_NOT_FOUND;
     }
     delete pDSa;
   }
   else {
     iResult = MEDIA_DATABASE_ERROR;
   }
   return iResult;
}



int BXVideoDatabase::GetVideoByStringField(const std::string& strFieldName, const std::string& strFieldValue, BXMetadata* pMetadata)
{
  if (!pMetadata || !(pMetadata->GetType() == MEDIA_ITEM_TYPE_VIDEO)) return MEDIA_DATABASE_ERROR;

  BXVideo* pVideo = (BXVideo*) pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

  int iResult = MEDIA_DATABASE_ERROR;
  Dataset* pDSa = Query("select * from video_files where %s like '%s' and iDropped=0", strFieldName.c_str(), strFieldValue.c_str());
  if (pDSa) {
    if (pDSa->num_rows() != 0)
    {
      if (LoadFullVideoFromDataset(pDSa, pVideo) == MEDIA_DATABASE_OK)
      {
        if (pVideo->m_strType == "part")
        {
          GetVideoParts(pVideo->m_iId, pVideo->m_vecParts);
        }

        pMetadata->SetPath(pDSa->fv("strPath").get_asString());
        pMetadata->SetMediaFileId(pDSa->fv("idFile").get_asInt());
        iResult = MEDIA_DATABASE_OK;
      }
      else {
        iResult = MEDIA_DATABASE_ERROR;
      }
    }
    else {
      iResult = MEDIA_DATABASE_NOT_FOUND;
    }
    delete pDSa;
  }
  else {
    iResult = MEDIA_DATABASE_ERROR;
  }
  return iResult;
}


int BXVideoDatabase::GetActorIdByName(const std::string& strName)
{
  int iID = MEDIA_DATABASE_NOT_FOUND;
  Dataset* pDS = Query("select * from actors where strName like '%s'", strName.c_str());
  if (pDS) {
    try {
      if (pDS->num_rows() != 0)
      {
        iID = (int)pDS->fv("idActor").get_asInt();
      }
    }
    catch(dbiplus::DbErrors& e) {
      iID = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iID = MEDIA_DATABASE_ERROR;
  }
  return iID;
}

int BXVideoDatabase::AddActor(std::string strName)
{
  BXUtils::StringTrim(strName);
  if (strName.empty())
    return MEDIA_DATABASE_ERROR;

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  int iID = GetActorIdByName(strName);

  if (iID == MEDIA_DATABASE_NOT_FOUND)
  {
    iID = Insert("insert into actors (idActor, strName) values( NULL, '%s' )", strName.c_str());
    if (iID == -1) {
      LOG(LOG_LEVEL_ERROR, "Could not add artist: %s", strName.c_str());
    }
  }
  return iID;
}

int BXVideoDatabase::GetDirectorIdByName(const std::string& strName)
{
  int iID = MEDIA_DATABASE_NOT_FOUND;
  Dataset* pDS = Query("select * from directors where strName like '%s'", strName.c_str());
  if (pDS) {
    try {
      if (pDS->num_rows() != 0)
      {
        iID = (int)pDS->fv("idDirector").get_asInt();
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iID = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iID = MEDIA_DATABASE_ERROR;
  }

  return iID;
}

bool BXVideoDatabase::LoadSeriesFromDataset(Dataset* pDS, BXSeries* pSeries)
{
  if (!pDS || !pSeries) return false;

  pSeries->m_iId = pDS->fv("sIdSeries").get_asInt();
  pSeries->m_strTitle = pDS->fv("sTitle").get_asString();
  pSeries->m_strBoxeeId = pDS->fv("sBoxeeId").get_asString();
  pSeries->m_strCover = pDS->fv("sCover").get_asString();
  pSeries->m_iYear = pDS->fv("sYear").get_asInt();
  pSeries->m_strDescription = pDS->fv("sDescription").get_asString();
  pSeries->m_strLanguage = pDS->fv("sLanguage").get_asString();
  pSeries->m_strGenre = pDS->fv("sGenre").get_asString();
  pSeries->m_iVirtual = pDS->fv("sVirtual").get_asInt();
  pSeries->m_strNfoPath = pDS->fv("sNfoPath").get_asString();
  pSeries->m_iNfoAccessedTime = pDS->fv("sNfoAccessTime").get_asInt();
  pSeries->m_iNfoModifiedTime = pDS->fv("sNfoModifiedTime").get_asInt();
  pSeries->m_iRecentlyAired = pDS->fv("sLatestEpisodeDate").get_asUInt();

  return true;
}

int BXVideoDatabase::GetSeriesById(int iSeriesId, BXSeries* pSeries)
{
  int iResult = MEDIA_DATABASE_NOT_FOUND;
  Dataset* pDS = Query(GET_SERIES_QUERY "where idSeries=%i", iSeriesId);
  if (pDS) {
    try {
      if (pDS->num_rows() != 0)
      {
        LoadSeriesFromDataset(pDS, pSeries);
        iResult = MEDIA_DATABASE_OK;
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

bool BXVideoDatabase::GetSeriesByBoxeeId(const std::string& strBoxeeId, BXSeries* pSeries)
{
  bool bResult = true;
  Dataset* pDS = Query(GET_SERIES_QUERY "where strBoxeeId='%s'", strBoxeeId.c_str());
  if (pDS) {
    try {
      if (pDS->num_rows() != 0)
      {
        LoadSeriesFromDataset(pDS, pSeries);
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else {
    bResult = false;
  }

  return bResult;
}

int BXVideoDatabase::GetSeriesByName(const std::string& strSeriesName, BXSeries* pSeries)
{
  int iResult = MEDIA_DATABASE_NOT_FOUND;
  Dataset* pDS = Query(GET_SERIES_QUERY "where sTitle ='%s'", strSeriesName.c_str());

  if (pDS) {
    try {
      if (pDS->num_rows() != 0)
      {

        LoadSeriesFromDataset(pDS, pSeries);
        iResult = MEDIA_DATABASE_OK;
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      iResult = MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }
  else {
    iResult = MEDIA_DATABASE_ERROR;
  }

  return iResult;
}

unsigned long BXVideoDatabase::GetLatestEpisodeDate(const std::string& strBoxeeId)
{
  unsigned long iResult = 0;
  Dataset* pDS = Query("select * from video_files where strSeriesId='%s' and iDropped=0 order by iDateAdded desc", strBoxeeId.c_str());
  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        iResult = pDS->fv("video_files.iDateAdded").get_asUInt();
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
    delete pDS;
  }

  return iResult;
}

int BXVideoDatabase::AddDirector(std::string strName)
{
  BXUtils::StringTrim(strName);

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Adding director: %s", strName.c_str());

  if (strName.empty()) {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not add director, name is empty");
    return MEDIA_DATABASE_ERROR;
  }

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  int iID = GetDirectorIdByName(strName);

  if (iID == MEDIA_DATABASE_ERROR) {
    return iID;
  }

  if (iID == MEDIA_DATABASE_NOT_FOUND) {
    //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddDirector, QUERY WRITE, insert into directors");
    iID = Insert("insert into directors (idDirector, strName) values( NULL, '%s' )", strName.c_str());
    if (iID == -1)
    {
      LOG(LOG_LEVEL_ERROR, "Could not add director: %s", strName.c_str());
    }
  }
  return iID;
}

int BXVideoDatabase::AddActorToVideo(int iActorId, int iVideoId)
{
  int iID = MEDIA_DATABASE_ERROR;

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  Dataset* pDS = Query("select * from actor_to_video where idActor=%i and idVideo=%i", iActorId, iVideoId);
  if (pDS) {
    if (pDS->num_rows() == 0) {
      //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddActorToVideo, QUERY WRITE, insert into actor_to_video");
      iID = Insert("insert into actor_to_video (idActor, idVideo) values (%i, %i)", iActorId, iVideoId);
    }
    delete pDS;
  }
  return iID;
}

int BXVideoDatabase::AddDirectorToVideo(int iDirectorId, int iVideoId)
{
  int iID = MEDIA_DATABASE_ERROR;

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  Dataset* pDS = Query("select * from director_to_video where idDirector=%i and idVideo=%i", iDirectorId, iVideoId);
  if (pDS) {
    if (pDS->num_rows() == 0) {
      //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddDirectorToVideo, QUERY WRITE, insert into director_to_video");
      iID = Insert("insert into director_to_video (idDirector, idVideo) values (%i, %i)", iDirectorId, iVideoId);
    }
    delete pDS;
  }
  return iID;
}

int BXVideoDatabase::AddSeries(const BXSeries* pSeries)
{
  if ((!pSeries) || (pSeries->m_strBoxeeId == ""))
  {
    return MEDIA_DATABASE_ERROR;
  }

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeries - Adding series [title=%s][boxeeId=%s] (videodb)",pSeries->m_strTitle.c_str(),pSeries->m_strBoxeeId.c_str());

  int iID = MEDIA_DATABASE_ERROR;
  int iVirtual = -1;

  // Check if the series with this name exists, lock to avoid race condition between the select (checking if there's existing episode) and the insert query
  CSingleLock lock (m_lock);

  Dataset* pDS = Query("select * from series where strBoxeeId='%s'", pSeries->m_strBoxeeId.c_str());
  time_t now = time(NULL);

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      // series already exists, check if existing series is virtual
      iVirtual = pDS->fv("iVirtual").get_asInt();

      if (iVirtual == 1 && pSeries->m_iVirtual == 0)
      {
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeries - Update virtual series [title=%s][boxeeId=%s] (videodb)", pSeries->m_strTitle.c_str(),pSeries->m_strBoxeeId.c_str());
        Dataset* pDS2 = Exec("update series set iVirtual=0 , iLatestLocalEpisodeDate=%i where strBoxeeId='%s'", now, pSeries->m_strBoxeeId.c_str());
        if (pDS2)
        {
          delete pDS2;
          delete pDS; //there was a leak here
          return MEDIA_DATABASE_OK;
        }
        else
        {
          delete pDS; //there was a leak here
          return MEDIA_DATABASE_ERROR;
        }
      }

      Dataset* pDS3 = Exec("update series set iLatestLocalEpisodeDate=%i where strBoxeeId='%s'", now, pSeries->m_strBoxeeId.c_str());

      if (pDS3)
      {
        delete pDS3;
      }

      //TODO: check if there's difference between the modified time of the metadata, update the db accordingly

      LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeries - Series already exists [title=%s][boxeeId=%s] (videodb)", pSeries->m_strTitle.c_str(),pSeries->m_strBoxeeId.c_str());
      delete pDS;
      return MEDIA_DATABASE_OK;
    }

    delete pDS;
  }

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeries - Going to add new series [title=%s][boxeeId=%s] (videodb)",pSeries->m_strTitle.c_str(),pSeries->m_strBoxeeId.c_str());

  iID = Insert("insert into series (idSeries, strTitle, strPath, strBoxeeId, strCover, strDescription, strLanguage, "
      "strGenre, iYear, iVirtual, strNfoPath, iNfoAccessTime, iNfoModifiedTime, iLatestLocalEpisodeDate) "
      "values( NULL, '%s', '%s', '%s', '%s', '%s', '%s', '%s', %i, %i , '%s', %i, %i, %i)", pSeries->m_strTitle.c_str(),
      pSeries->GetPath().c_str(), pSeries->m_strBoxeeId.c_str(), pSeries->m_strCover.c_str(), pSeries->m_strDescription.c_str(), pSeries->m_strLanguage.c_str(),
      pSeries->m_strGenre.c_str(), pSeries->m_iYear, pSeries->m_iVirtual , pSeries->m_strNfoPath.c_str(), pSeries->m_iNfoAccessedTime , pSeries->m_iNfoModifiedTime, now);

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeries - Could not add new series [title=%s][boxeeId=%s] (videodb)",pSeries->m_strTitle.c_str(),pSeries->m_strBoxeeId.c_str());
  }

  return iID;
}

int BXVideoDatabase::AddSeason(const BXSeason* pSeason)
{

  // If season is null or no series id, return error
  if ((!pSeason) || pSeason->m_iSeriesId == MEDIA_DATABASE_ERROR) {
    return MEDIA_DATABASE_ERROR;
  }
  //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Adding season %d for series id %d, ", pSeason->m_iSeasonNum, pSeason->m_iSeriesId);

  int iID = MEDIA_DATABASE_ERROR;

  CSingleLock lock (m_lock);
  // Check if the series with this name exists
  Dataset* pDS = Query("select * from seasons where idSeries=%i and iSeasonNum=%i", pSeason->m_iSeriesId, pSeason->m_iSeasonNum);

  if (pDS) {
    if (pDS->num_rows() != 0) {

      iID = pDS->fv("idSeason").get_asInt();
    }
    delete pDS;
  }

  if (iID != MEDIA_DATABASE_ERROR) return iID;

  //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddSeason, QUERY WRITE, insert into seasons");
  iID = Insert("insert into seasons (idSeason, idSeries, iSeasonNum, strPath, strCover, strDescription, strLanguage, iYear, iVirtual) "
      "values( NULL, %i, %i, '%s', '%s', '%s', '%s', %i, %i)", pSeason->m_iSeriesId, pSeason->m_iSeasonNum,
      pSeason->GetPath().c_str(), pSeason->m_strCover.c_str(), pSeason->m_strDescription.c_str(), pSeason->m_strLanguage.c_str(),
      pSeason->m_iYear, pSeason->m_iVirtual);

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not insert series, %s", pSeason->m_strTitle.c_str());

  }

  return iID;
}

int BXVideoDatabase::AddPart(int iVideoId, int iPartNumber, const std::string& strPath)
{
  int iID = MEDIA_DATABASE_ERROR;

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY, VIDEO DATABASE, LIB1: Adding video part, path =  %s, video = %d, part = %d", strPath.c_str(), iVideoId, iPartNumber);
  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  // Check if the series with this name exists
  Dataset* pDS = Query("select * from video_parts where strPath='%s'", strPath.c_str());

  if (pDS) {
    if (pDS->num_rows() != 0) {

      iID = pDS->fv("idVideo").get_asInt();
    }
    delete pDS;
  }

  if (iID != MEDIA_DATABASE_ERROR) return iID;

  //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddPart, QUERY WRITE, insert into video_parts");
  iID = Insert("insert into video_parts (idPart, idVideo, iPartNumber, strPath) values (NULL, %i, %i, '%s')", iVideoId, iPartNumber, strPath.c_str());

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not insert video part , %s", strPath.c_str());
  }

  return iID;
}

// This function finds whether we already have this video in our feed
// and replaces it with new details
int BXVideoDatabase::RemoveFeedVideo(const BXMetadata* pMetadata)
{
  if (!pMetadata)
  {
    return MEDIA_DATABASE_ERROR;
  }

  //lock to avoid race condition between the select (checking if there's existing record) and the delete query
  CSingleLock lock (m_lock);

  // Get video details
  BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  Dataset* pDS = Query("select * from video_files where idFile = -1 and strTitle='%s'", pVideo->m_strTitle.c_str());
  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          // Compare other details to make sure its the same video
          if ((pVideo->m_iEpisode != -1 && (pVideo->m_iEpisode == pDS->fv("iEpisode").get_asInt())) ||
              (pVideo->m_iEpisode == -1 && (pVideo->m_strIMDBKey == pDS->fv("strIMDBKey").get_asString())))
          {
            int iVideoId = pDS->fv("idVideo").get_asInt();
            // We found a virtual video with the same name, delete it
            LOG(LOG_LEVEL_DEBUG, "removing video from database id: [%d]", iVideoId);
            Dataset* pDS_t = Exec("delete from video_files where idVideo=%i", iVideoId);
            if (pDS_t)
            {
              delete pDS_t;
              delete pDS;
              return MEDIA_DATABASE_OK;
            }
            else
            {
              LOG(LOG_LEVEL_ERROR, "Could not ERASE video id = %d, database error", iVideoId);
              delete pDS;
              return MEDIA_DATABASE_ERROR;
            }
          }
          pDS->next();
        }
      }
      else
      {
        delete pDS;
        return MEDIA_DATABASE_OK;
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      delete pDS;
      return MEDIA_DATABASE_ERROR;
    }
    delete pDS;
  }

  return MEDIA_DATABASE_ERROR;
}

int BXVideoDatabase::AddVideoFile(const std::string& strPath, const std::string& strSharePath, int FolderId, int64_t size)
{
  int iID = MEDIA_DATABASE_ERROR;

  if ((FolderId  == -1) || (FolderId  == 0))
  {
    LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::AddVideoFiles, ERROR, Attempts to add unresolved video, without folder id, path = %s (videodb)", strPath.c_str());
  }

  Dataset* pDS = Query("select * from unresolved_video_files where strPath='%s'", strPath.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      iID = pDS->fv("idVideoFile").get_asInt();
    }
    delete pDS;
  }

  if (iID != MEDIA_DATABASE_ERROR)
    return iID;

  time_t now = time(NULL);

  iID = Insert("insert into unresolved_video_files (idVideoFile, strPath, strSharePath, idFolder, iStatus, idVideo, iSize, iDateAdded) values (NULL, '%s', '%s', %i, %i, %i ,%lld , %i)",
      strPath.c_str(), strSharePath.c_str(), FolderId, STATUS_NEW, -1, size , now);

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not insert video file , %s", strPath.c_str());
  }

  return iID;
}

int BXVideoDatabase::UpdateVideoFileStatus(const std::string& strPath, int iStatus, int idVideo)
{
  Dataset* pDS = Exec("update unresolved_video_files set iStatus=%i, idVideo=%i where strPath='%s'",iStatus, idVideo, strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXVideoDatabase::MarkNewFolderFilesAsProcessing(int FolderId)
{
  Dataset* pDS = Exec("update unresolved_video_files set iStatus=%i where idFolder=%i and iStatus=%i", STATUS_PROCESSING, FolderId, STATUS_NEW);
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

bool BXVideoDatabase::GetUnresolvedVideoFiles(std::vector<BXMetadata*> &vecUnresolvedVideos, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  Dataset* pDS = Query("select * from unresolved_video_files where iStatus=%d limit %d", STATUS_UNRESOLVED, iItemLimit);
  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          std::string strPath = pDS->fv("strPath").get_asString(); // path
          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath))
          {
            pDS->next();
            continue;
          }

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_FILE);

          // idVideo integer primary key autoincrement, strPath text, strSharePath text, idFolder integer, iStatus integer, idVideo integer
          pMetadata->SetPath(pDS->fv(1).get_asString()); // strPath
          pMetadata->SetFolder(pDS->fv(2).get_asString()); // strSharePath
          pMetadata->SetFileSize((uint64_t) pDS->fv("iSize").get_asDouble());
          pMetadata->m_iDateAdded = pDS->fv("iDateAdded").get_asUInt();

          vecUnresolvedVideos.push_back(pMetadata);

          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      delete pDS;
      return false;
    }

    delete pDS;
    return true;
  }
  return false;

}

int  BXVideoDatabase::GetShareUnresolvedVideoFilesCountByFolder(int folderId, int iStatus)
{
  int count = 0;
  Dataset* pDS = NULL;

  if (iStatus == STATUS_ALL)
  {
    pDS = Query("select  count(*) from unresolved_video_files where idFolder=%d ",folderId);
  }
  else
  {
    pDS = Query("select  count(*) from unresolved_video_files where idFolder=%d and iStatus=%d  ", folderId ,iStatus);
  }

  if (pDS)
  {
    count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return count;
}

int  BXVideoDatabase::GetShareUnresolvedVideoFilesCount(const std::string& iStrSharePath, int iStatus)
{
  int count = 0;
  Dataset* pDS = NULL;

  if (iStatus == STATUS_ALL)
  {
    pDS = Query("select  count(*) from unresolved_video_files where strSharePath='%s' ",iStrSharePath.c_str());
  }
  else
  {
    pDS = Query("select  count(*) from unresolved_video_files where strSharePath='%s' and iStatus=%d  ",iStrSharePath.c_str() ,iStatus);
  }

  if (pDS)
  {
	count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return count;
}

int BXVideoDatabase::GetUserUnresolvedVideoFilesCount(const std::string& iSharesList, int iStatus)
{
  int count = 0;
  Dataset* pDS = NULL;

  if (iStatus == STATUS_ALL)
  {
    pDS = UnQuotedQuery( "select  count(*) from unresolved_video_files where strSharePath in (%s)",iSharesList.c_str());
  }
  else
  {
    pDS = UnQuotedQuery( "select  count(*) from unresolved_video_files where strSharePath in (%s) and iStatus=%d  ",iSharesList.c_str() ,iStatus);
  }

  if (pDS)
  {
        count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }

  return count;

}

int BXVideoDatabase::RemoveUnresolvedVideoByPath(const std::string& strPath)
{
  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveUnresolvedVideoByPath, : path = %s", strPath.c_str());
  Dataset* pDS_t = Exec("delete from unresolved_video_files where strPath='%s'", strPath.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}
int BXVideoDatabase::RemoveUnresolvedVideoByFolder(const std::string& strFolder)
{
  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveUnresolvedVideoByFolder, : path like %s", strFolder.c_str());
  Dataset* pDS_t = Exec("delete from unresolved_video_files where strPath like '%s%%'", strFolder.c_str());
  if (pDS_t)
  {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

bool BXVideoDatabase::AreVideoFilesBeingScanned(const std::string& iSharesList)
{
  int count = 0;

  Dataset* pDS = UnQuotedQuery( "select  count(*) from media_shares where strType='video' and strPath in (%s) and iLastScanned=1  ",iSharesList.c_str());
  if (pDS)
  {
        count = pDS->fv("count(*)").get_asInt();
    delete pDS;
  }
  return (count > 0) ;

}


int BXVideoDatabase::AddVideo(const BXMetadata* pMetadata)
{
  // We only handle video files here that have at least title
  if (!pMetadata || pMetadata->GetType() != "video" || (pMetadata->GetDetail(MEDIA_DETAIL_VIDEO) == NULL))
  {
    LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::AddVideo - FAILED not add video. invalid type (videodb)");
    return MEDIA_DATABASE_ERROR;
  }

  int iID = MEDIA_DATABASE_ERROR;

  BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);
  BXSeason* pSeason = (BXSeason*)pMetadata->GetDetail(MEDIA_DETAIL_SEASON);

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideo, Adding video, path = %s (videodb)", pMetadata->GetPath().c_str());

  std::string strBoxeeId = pVideo->m_strBoxeeId;

  if (strBoxeeId.empty())
  {
    LOG(LOG_LEVEL_INFO, "BXVideoDatabase::AddVideo, added video without boxee id, path = %s (videodb)", pMetadata->GetPath().c_str());
    //LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::AddVideo, DESIGN ERROR, Could not add video, no boxee id, path = %s (videodb)", pMetadata->GetPath().c_str());
    //pMetadata->Dump();

    // for now we do not really enforce this
    //return MEDIA_DATABASE_ERROR;
  }

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideo, Adding video, title %s, path %s", pVideo->m_strTitle.c_str(), pMetadata->GetPath().c_str());

  if (pMetadata->GetMediaFileId() == -1)
  {
    LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::AddVideo, DESIGN ERROR, Attempts to add  not add video, no boxee id, path = %s (videodb)", pMetadata->GetPath().c_str());
  }

  if ((pVideo->m_iFolderId  == -1) || (pVideo->m_iFolderId  == 0))
  {
    LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::AddVideo, ERROR, Attempts to add video, without folder id, path = %s (videodb)", pMetadata->GetPath().c_str());
  }

  BXMetadata existingVideoMetadata(MEDIA_ITEM_TYPE_VIDEO);

  int iResult = MEDIA_DATABASE_ERROR;

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  if (strBoxeeId.empty())
  {
    iResult = GetVideoByPath(pMetadata->GetPath(), &existingVideoMetadata);
  }
  else
  {
    iResult = GetVideoByBoxeeIdAndPath(strBoxeeId, pMetadata->GetPath(), &existingVideoMetadata);
  }

  if (iResult == MEDIA_DATABASE_OK)
  {
    BXVideo* pExistingVideo = (BXVideo*)existingVideoMetadata.GetDetail(MEDIA_DETAIL_VIDEO);

    //Todo: check if there's difference between the modified time of the metadata, update the db accordingly
    // Check if the existing video has the same path
    LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideo, video exists, path %s (videodb)", pMetadata->GetPath().c_str());
    return (pExistingVideo)->m_iId;
  }

  // Add series
  if (!pVideo->m_bMovie)
  {
    LOG(LOG_LEVEL_DEBUG, "Add series, title = %s (videodb)", pSeries->m_strTitle.c_str());

    if (!pSeries->m_strBoxeeId.empty())
    {
      int iSeriesId = AddSeries(pSeries);
      if (iSeriesId == MEDIA_DATABASE_ERROR)
      {
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideo, Could not add series, title = %s (videodb)", pSeries->m_strTitle.c_str());
        return MEDIA_DATABASE_ERROR;
      }
      else
      {
        // Set the series id to the specific season and add it to the database
        pSeason->m_iSeriesId = iSeriesId;
        pSeason->m_iId = AddSeason(pSeason);
        if (pSeason->m_iId == MEDIA_DATABASE_ERROR)
        {
          LOG(LOG_LEVEL_DEBUG, "Could not add season, title = %d", pSeason->m_iSeasonNum);
          return MEDIA_DATABASE_ERROR;
        }
      }
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "Could not add series, boxee id is empty, title = %s", pSeries->m_strTitle.c_str());
      return MEDIA_DATABASE_ERROR;
    }
  }

  time_t now = time(NULL);
  //make sure path saves in db with user name and password if exists
  CStdString strPath = pMetadata->GetPath();
  if(CUtil::IsSmb(strPath))
  {
    strPath = g_passwordManager.GetCifsPathCredentials(strPath);
  }

  iID = Insert("insert into video_files (idVideo, idFile, strPath, idFolder, strTitle, strBoxeeId, "
      "iDuration, strType, strSeriesId, iSeason, iEpisode, strDescription, strExtDescription, strIMDBKey, "
      "strMPAARating, strCredits, strStudio, strTagLine, strCover, strLanguage, "
      "strGenre, iReleaseDate, iYear, strTrailerUrl, strShowTitle, "
      "iDateAdded, iHasMetadata, iRating, iPopularity, iDateModified, iDropped, strNfoPath, iNfoAccessTime, iNfoModifiedTime, strOriginalCover, iRTCriticsScore, strRTCriticsRating, iRTAudienceScore, strRTAudienceRating) "
      "values( NULL, %i, '%s', %i, '%s', '%s', %i, '%s', '%s', %i, %i, '%s', '%s', '%s', '%s', '%s', '%s', '%s', "
      "'%s', '%s', '%s', %i, %i, '%s', '%s', %i, %i, %i, %i, %i, 0, '%s', %i, %i, '%s', %i, '%s', %i, '%s')",
      pMetadata->GetMediaFileId(), 
      strPath.c_str(),
      pVideo->m_iFolderId,
      pVideo->m_strTitle.c_str(), 
      pVideo->m_strBoxeeId.c_str(),
      pVideo->m_iDuration, 
      pVideo->m_strType.c_str(), 
      pVideo->m_strShowId.c_str(),
      pVideo->m_iSeason, 
      pVideo->m_iEpisode,
      pVideo->m_strDescription.c_str(),
      pVideo->m_strExtDescription.c_str(),
      pVideo->m_strIMDBKey.c_str(),
      pVideo->m_strMPAARating.c_str(),
      pVideo->m_strCredits.c_str(),
      pVideo->m_strStudio.c_str(),
      pVideo->m_strTagLine.c_str(),
      pVideo->m_strCover.c_str(),
      pVideo->m_strLanguageId.c_str(),
      pVideo->m_strGenre.c_str(),
      pVideo->m_iReleaseDate,
      pVideo->m_iYear, 
      pVideo->m_strTrailerUrl.c_str(), 
      pVideo->m_strShowTitle.c_str(), 
      now, 
      pMetadata->m_bResolved ? 1 : 0, 
          pVideo->m_iRating,
          pVideo->m_iPopularity,
          pVideo->m_iDateModified, pVideo->m_strNfoPath.c_str() , pVideo->m_iNfoAccessedTime, pVideo->m_iNfoModifiedTime, pVideo->m_strOriginalCover.c_str() ,
          pVideo->m_iCriticsScore, pVideo->m_strCriticsRating.c_str(), pVideo->m_iAudienceScore, pVideo->m_strAudienceRating.c_str());

  if (iID == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Could not insert video, %s", pVideo->m_strTitle.c_str());
    return iID;
  }

  // Add director and actors
  int iDirectorId = AddDirector(pVideo->m_strDirector);
  AddDirectorToVideo(iDirectorId, iID);

  for (unsigned int i=0; i < pVideo->m_vecActors.size(); i++)
  {
    int iActorId = AddActor(pVideo->m_vecActors[i]);
    AddActorToVideo(iActorId, iID);
  }

  //update the metadataresolver
  if (!pVideo->m_bMovie)
  {
    CMetadataResolverVideo::AddLocalShowsGenres(pSeries->m_strGenre);
  }
  else
  {
    CMetadataResolverVideo::AddLocalMoviesGenres(pVideo->m_strGenre);
  }

  for (std::vector<BXVideoLink>::iterator it = pVideo->m_vecVideoLinks.begin() ; it != pVideo->m_vecVideoLinks.end() ; it++)
  {
    AddVideoLink(pVideo->m_strBoxeeId, *it);
  }

  return iID;

}

bool BXVideoDatabase::LoadVideoFromDataset(Dataset* pDSa, BXVideo* pVideo)
{
  if (!pDSa || !pVideo) return false;

  try
  {
    // Load all video details from database
    pVideo->m_iId = pDSa->fv(0).get_asInt(); // idVideo
    // 1, file id //idFile
    pVideo->m_strPath = pDSa->fv(2).get_asString(); // strPath text,
    pVideo->m_iFolderId = pDSa->fv(3).get_asInt(); //idFolder integer,
    pVideo->m_strTitle = pDSa->fv(4).get_asString(); //strTitle text,
    pVideo->m_strBoxeeId = pDSa->fv(5).get_asString(); // strBoxeeId text
    pVideo->m_iDuration = pDSa->fv(6).get_asInt(); // "iDuration integer,
    pVideo->m_strType = pDSa->fv(7).get_asString(); //   strType text,
    pVideo->m_strShowId = pDSa->fv(8).get_asString(); // m_strShowId text,
    pVideo->m_iSeason = pDSa->fv(9).get_asInt();  // iSeason integer,
    pVideo->m_iEpisode = pDSa->fv(10).get_asInt();  // iEpisode integer,

    //////////////////////////////////////////////////////////////////////
    // Description and ExtDescription are entered in reverse to the DB, //
    // so switch read for backwards compatibility                       //
    //////////////////////////////////////////////////////////////////////
    pVideo->m_strDescription = pDSa->fv(12).get_asString();  // strDescription text,
    pVideo->m_strExtDescription = pDSa->fv(11).get_asString();  // strExtDescription text,

    pVideo->m_strIMDBKey = pDSa->fv(13).get_asString();  // strIMDBKey text,
    pVideo->m_strMPAARating = pDSa->fv(14).get_asString(); // strMPAARating text, "
    pVideo->m_strCredits = pDSa->fv(15).get_asString();   // strCredits text,
    pVideo->m_strStudio = pDSa->fv(16).get_asString(); // strStudio text,
    pVideo->m_strTagLine = pDSa->fv(17).get_asString(); // strTagLine text,
    pVideo->m_strCover = pDSa->fv(18).get_asString(); // strCover text,
    pVideo->m_strLanguageId = pDSa->fv(19).get_asString(); // strLanguage text, "
    pVideo->m_strGenre = pDSa->fv(20).get_asString(); // strGenre
    pVideo->m_iReleaseDate = pDSa->fv(21).get_asUInt();
    pVideo->m_iYear = pDSa->fv(22).get_asInt(); // iYear integer,
    pVideo->m_strTrailerUrl = pDSa->fv(23).get_asString(); // strTrailerUrl text,
    pVideo->m_strShowTitle = pDSa->fv(24).get_asString();  // strShowTitle text, "
    pVideo->m_iDateAdded = pDSa->fv(25).get_asUInt(); // "iDateAdded integer,
    // 26, iHasMetadta iHasMetadata integer,
    pVideo->m_iRating = pDSa->fv(27).get_asInt();  //iRating integer,
    pVideo->m_iPopularity = pDSa->fv(28).get_asInt();  //iPopularity integer,
    pVideo->m_iDateModified = pDSa->fv(29).get_asUInt();  //iDateModified integer,
    // 30.  iDropped integer
    pVideo->m_strNfoPath = pDSa->fv(31).get_asString();
    pVideo->m_iNfoAccessedTime = pDSa->fv(32).get_asInt();
    pVideo->m_iNfoModifiedTime = pDSa->fv(33).get_asInt();
    pVideo->m_strOriginalCover = pDSa->fv(34).get_asString();

    //rotten tomato columns
    pVideo->m_iCriticsScore = pDSa->fv(35).get_asInt();
    pVideo->m_strCriticsRating = pDSa->fv(36).get_asString();
    pVideo->m_iAudienceScore = pDSa->fv(37).get_asInt();
    pVideo->m_strAudienceRating = pDSa->fv(38).get_asString();

    return true;
  }
  catch(dbiplus::DbErrors& e)
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught, could not load video details from dataset. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    return false;
  }
}

bool BXVideoDatabase::LoadFullVideoFromDataset(Dataset* pDSa, BXVideo* pVideo)
{
  // This function should receive a dataset from the video_files table
  if (!pDSa || !pVideo) return false;

  try 
  {
    if (LoadVideoFromDataset(pDSa, pVideo))
    {
      pVideo->m_strDirector = GetDirectorByVideoId(pVideo->m_iId);
      GetActorsByVideoId(pVideo->m_iId, pVideo->m_vecActors);
      return true;
    }
    else
    {
      return false;
    }
  }
  catch(dbiplus::DbErrors& e) 
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught, could not load video details from dataset. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    return false;
  }

}

void BXVideoDatabase::MapDirectorsByVideoId(std::map<int, std::string>& mapDirectors)
{
  std::string strDirector;

  int iVideoId;

  // Load all directors into a map, by video id
  Dataset* pDSb = Query("select dtv.idVideo, a.strName from director_to_video dtv left outer join directors a on dtv.idDirector=a.idDirector");
  if (pDSb)
  {
    try
    {
      if (pDSb->num_rows() != 0 )
      {
        while (!pDSb->eof())
        {
          iVideoId = pDSb->fv(0).get_asInt();
          strDirector = pDSb->fv(1).get_asString();

          mapDirectors[iVideoId] = strDirector;
          pDSb->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::MapDirectorsByVideoId, Exception caught. Error = %s, msg= %s (videodb)", GetLastErrorMessage(), e.getMsg());
    }
    delete pDSb;
  }
}

void BXVideoDatabase::MapActorsByVideoId(std::map<int, std::vector<std::string> >& mapActors)
{
  std::vector<std::string> vecActors;
  std::string strActor;
  int iVideoId;

  Dataset* pDSb = Query("select atv.idVideo, a.strName from actor_to_video atv left outer join actors a on atv.idActor=a.idActor");
  if (pDSb) 
  {
    try 
    {
      if (pDSb->num_rows() != 0 ) 
      {
        while (!pDSb->eof()) 
        {
          // Get actor details
          iVideoId = pDSb->fv(0).get_asInt();
          strActor = pDSb->fv(1).get_asString();

          std::map<int, std::vector<std::string> >::iterator it = mapActors.find(iVideoId);

          if (it == mapActors.end())
          {
            vecActors.push_back(strActor);
            mapActors[iVideoId] = vecActors;
            vecActors.clear();
          }
          else
          {
            it->second.push_back(strActor);
          }

          pDSb->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, "BXVideoDatabase::MapActorsByVideoId, Exception caught. Error = %s, msg= %s (videodb)", GetLastErrorMessage(), e.getMsg());
    }
    delete pDSb;
  }  
}

bool BXVideoDatabase::GetMovies(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit , bool bLoadOnlyFirstTrailerLink /* = false (by default) */)
{
  // Get all movies files appropriately ordered
  // We only want real ones, those that are located on one of the shares, therefore we check for idFile = 1
  Dataset* pDS = NULL;

  if ( (strGenre != "") && (strPrefix != ""))
  {
    if (strPrefix == "#")
    {
      pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and ORD(s1.sTitle) < 65 and (strGenre LIKE '%%%s%%')",
          strPrefix.c_str(), strGenre.c_str());
    }
    else
    {
      pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and strTitle LIKE '%s%%' and (strGenre LIKE '%%%s%%')",
          strPrefix.c_str(), strGenre.c_str());
    }
  }
  else if ( (strGenre != "") && (strPrefix == ""))
  {
    pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and (strGenre LIKE '%%%s%%')", strGenre.c_str());
  }
  else if ( (strGenre == "") && (strPrefix != ""))
  {
    if (strPrefix == "#")
    {
      pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and ORD(s1.sTitle) < 65", strPrefix.c_str());
    }
    else
    {
      pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and strTitle LIKE '%s%%' ", strPrefix.c_str());
    }

    pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1 and strTitle LIKE '%s%%'", strPrefix.c_str());
  }
  else if ( (strGenre == "") && (strPrefix == "") && bLoadOnlyFirstTrailerLink)
  {
    pDS = Query("select * from video_files as files left outer join (select * from video_links as links where idLink = (select min(idLink) from video_links as l where l.strBoxeeId = links.strBoxeeId and strBoxeeType='trailer') and strBoxeeType='trailer') as firstLink on firstLink.strBoxeeId = files.strBoxeeId where (idFile = 1 AND iDropped=0 AND iSeason=-1 AND iEpisode = -1)");
  }
  else if ( (strGenre == "") && (strPrefix == ""))
  {
    pDS = Query("select * from video_files where idFile = 1 and iDropped=0 and iSeason=-1");
  }

  CreateVideosFromDataset(pDS, vecMediaFiles, vecPathFilter, iItemLimit , bLoadOnlyFirstTrailerLink);

  return true;
}

bool BXVideoDatabase::SearchMoviesByTitle( const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  if (strTitle.empty())
    return true;

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::SearchMoviesByTitle, title = %s (search)", strTitle.c_str());
  Dataset* pDS = Query( "select * from video_files where (strTitle LIKE '%%%s%%') and idFile=1 and iDropped=0 and iSeason=-1",
      strTitle.c_str() );

  return CreateVideosFromDataset(pDS, vecMediaFiles, vecPathFilter, iItemLimit);
}

// Get all episodes from a TV show with specific boxee id
bool BXVideoDatabase::GetEpisodes(const std::string& strTvShowBoxeeId, int iSeason, std::vector<BXMetadata*> &vecEpisodes, const std::vector<std::string>& vecPathFilter)
{
  BXSeries series;
  if (!GetSeriesByBoxeeId(strTvShowBoxeeId, &series))
    return false;

  Dataset* pDS = NULL;
  if (iSeason == -1)
  {
    pDS = Query("select * from video_files where strSeriesId='%s' and idFile=1 and iDropped=0", strTvShowBoxeeId.c_str());
  }
  else
  {
    pDS = Query("select * from video_files where strSeriesId='%s' and iSeason=%i and idFile=1 and iDropped=0", strTvShowBoxeeId.c_str(), iSeason);
  }

  bool result = CreateVideosFromDataset(pDS, vecEpisodes, vecPathFilter, 10000);

  // go over the items and set the series title
  for (int i = 0; i < (int)vecEpisodes.size(); i++)
  {
    BXMetadata* pMetadata = vecEpisodes[i];
    BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

    *pSeries = series;
}

  return result;
}

bool BXVideoDatabase::CreateVideosFromDataset(Dataset* pDS, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit , bool bLoadOnlyFirstTrailerLink)
{
  bool bResult = true;

  std::map<int, std::string> mapDirectors;
  MapDirectorsByVideoId(mapDirectors);

  std::map<int, std::vector<std::string> > mapActors;
  MapActorsByVideoId(mapActors);

  std::map<std::string, int> mapMovieIndexByBoxeeId;

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase: GetMovies, processing %d rows", pDS->num_rows());
        while (!pDS->eof() && iItemLimit > 0)
        {
          std::string strPath = pDS->fv(2).get_asString(); // path

          //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase: CreateVideosFromDataset, Handling video file. Path = %s (videodb)", strPath.c_str());

          if (!BXUtils::CheckPathFilter(vecPathFilter, strPath))
          {
            //LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::CreateVideosFromDataset, path not in filter,  path %s (videodb)", strPath.c_str());
            pDS->next();
            continue;
          }

          std::string strBoxeeId = pDS->fv(5).get_asString(); // boxee id

          bool bFound = false;

          // if the boxee id is empty like it can happen in case of the episodes
          // we need to look by show id
          if (strBoxeeId.empty())
          {
            // check if we have a tvshow id
            std::string strShowId = pDS->fv(8).get_asString(); // m_strShowId text,
            int iSeason = pDS->fv(9).get_asInt();  // iSeason integer,
            int iEpisode = pDS->fv(10).get_asInt();  // iEpisode integer,

            if (!strShowId.empty())
            {
              for (int i = 0; i < (int)vecMediaFiles.size(); i++)
              {
                BXVideo* existingVideo = (BXVideo*)vecMediaFiles[i]->GetDetail(MEDIA_DETAIL_VIDEO);
                if (existingVideo->m_strShowId == strShowId && existingVideo->m_iSeason == iSeason && existingVideo->m_iEpisode == iEpisode)
                {
                  // Get parameters of the new video to check whether it has parts
                  int videoId = pDS->fv(0).get_asInt(); // video id
                  std::string strType = pDS->fv(7).get_asString(); // type
                  std::vector<std::string> vecParts;

                  BXPath path;
                  if (strType == "part")
                  {
                    GetVideoParts(videoId, path.m_vecParts);
                  }
                  else
                  {
                    path.m_strPath = pDS->fv(2).get_asString(); // path
                  }

                  existingVideo->m_vecLinks.push_back(path);

                  bFound = true;
                }
              }
            }
          }
          else
          {
            // look up the item by boxee id
            std::map<std::string, int>::iterator it = mapMovieIndexByBoxeeId.find(strBoxeeId);
            if (it != mapMovieIndexByBoxeeId.end())
            {
              LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::CreateVideosFromDataset, add path to existing video,  path %s (videodb)", strPath.c_str());

              // Get pointer to existing video
              int i = it->second;
              BXVideo* existingVideo = (BXVideo*)vecMediaFiles[i]->GetDetail(MEDIA_DETAIL_VIDEO);

              // Get parameters of the new video to check whether it has parts
              int videoId = pDS->fv(0).get_asInt(); // video id
              std::string strType = pDS->fv(7).get_asString(); // type
              std::vector<std::string> vecParts;

              BXPath path;
              if (strType == "part")
              {
                GetVideoParts(videoId, path.m_vecParts);
              }
              else
              {
                path.m_strPath = pDS->fv(2).get_asString(); // path
              }

              existingVideo->m_vecLinks.push_back(path);

              // update ModifyDate to the latest one in order to sort by RecentlyAdded to work properly
              unsigned long addedPathModifiedDate = pDS->fv(29).get_asUInt(); // Date Modified

              if (addedPathModifiedDate > existingVideo->m_iDateModified)
              {
                existingVideo->m_iDateModified = addedPathModifiedDate;
              }

              bFound = true;
            }
          }

          if (!bFound)
          {
            LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::CreateVideosFromDataset, create new video,  path %s (videodb)", strPath.c_str());
            BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_VIDEO);

            if (CreateVideoFromDataset(pDS, pMetadata, mapDirectors, mapActors) )
            {
              // Add created movie to the map by boxee id
              mapMovieIndexByBoxeeId[strBoxeeId] = vecMediaFiles.size();

              if (bLoadOnlyFirstTrailerLink)
              {
                BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);

                if (pVideo)
                {
                  BXVideoLink link;

                  link.m_strURL = pDS->fv(37).get_asString();
                  link.m_strProvider = pDS->fv(38).get_asString();
                  link.m_strProviderName = pDS->fv(39).get_asString();
                  link.m_strProviderThumb = pDS->fv(40).get_asString();
                  link.m_strTitle = pDS->fv(41).get_asString();
                  link.m_strQuality = pDS->fv(42).get_asString();
                  link.m_strQualityLabel = pDS->fv(43).get_asString();
                  link.m_bIsHD = pDS->fv(44).get_asBool();
                  link.m_strType = pDS->fv(45).get_asString();
                  link.m_strBoxeeType = pDS->fv(46).get_asString();
                  link.m_strCountryCodes = pDS->fv(47).get_asString();
                  link.m_strCountryRel = pDS->fv(48).get_asString();
                  link.m_strOffer = pDS->fv(49).get_asString();

                  pVideo->m_vecVideoLinks.push_back(link);
                }
              }

              // Add movie to output vector
              vecMediaFiles.push_back(pMetadata);
              iItemLimit--;
            }
            else
            {
              delete pMetadata;
            }
          }
          else
          {

          }
          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught, could not get video files. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else {
    bResult = false;
  }

  return bResult;
}

bool BXVideoDatabase::CreateVideoFromDataset(Dataset* pDS, BXMetadata* pMetadata, std::map<int, std::string>& mapDirectors, std::map<int, std::vector<std::string> >& mapActors)
{
  BXVideo* pVideo = (BXVideo*)pMetadata->GetDetail(MEDIA_DETAIL_VIDEO);
  BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

  if (LoadVideoFromDataset(pDS, pVideo) == MEDIA_DATABASE_OK)
  {
    // If this is a multipart videos, get all its parts
    BXPath path;
    if (pVideo->m_strType == "part")
    {
      GetVideoParts(pVideo->m_iId, path.m_vecParts);
    }
    else
    {
      path.m_strPath = pVideo->m_strPath;
    }
    pVideo->m_vecLinks.push_back(path);

    // If this is an episodes, load the series details
    if (!pVideo->m_strShowId.empty())
    {
      GetSeriesByBoxeeId(pVideo->m_strShowId, pSeries);
    }

    pVideo->m_strDirector = mapDirectors[pVideo->m_iId];
    pVideo->m_vecActors = mapActors[pVideo->m_iId];

    pMetadata->SetPath(pVideo->m_strPath);
    pMetadata->m_iDateAdded = pVideo->m_iDateAdded;
    pMetadata->m_iDateModified = pVideo->m_iDateModified;
    return true;
  }
  return false;
}

// Checks whether a series has episodes under the provided path list
bool BXVideoDatabase::HasEpisodes(const std::string& strBoxeeId, int iSeason, const std::vector<std::string>& vecPathFilter)
{
  Dataset* pDS = NULL;
  if (iSeason == -1)
    pDS = Query("select strPath from video_files where idFile = 1 and iDropped=0 and strSeriesId='%s'", strBoxeeId.c_str());
  else
    pDS = Query("select strPath from video_files where idFile = 1 and iDropped=0 and strSeriesId='%s' and iSeason=%i", strBoxeeId.c_str(), iSeason);

  if (pDS) 
  {
    try 
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof())
        {
          std::string strPath = pDS->fv(0).get_asString();
          if (BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
          {
            delete pDS;
            return true;
          }
          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      return false;
    }
    delete pDS;
  }

  return false;
}

unsigned int BXVideoDatabase::GetLatestEpisodeReleaseDate(const std::string& strTvShowId, int iSeason)
{
  Dataset* pDS = NULL;
  if (iSeason == -1)
  {
    pDS = Query("SELECT * from video_files WHERE strSeriesId='%s' and idFile=1 and iDropped=0 ORDER BY iReleaseDate DESC", strTvShowId.c_str(), iSeason);
  }
  else
  {
    pDS = Query("SELECT * from video_files WHERE strSeriesId='%s' and iSeason=%i and idFile=1 and iDropped=0 ORDER BY iReleaseDate DESC", strTvShowId.c_str(), iSeason);
  }

  unsigned int iReleaseDate = 0;
  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        iReleaseDate = pDS->fv("video_files.iReleaseDate").get_asUInt();
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
    delete pDS;
  }

  LOG(LOG_LEVEL_DEBUG, "Latest episode for show %s, is %d (release)", strTvShowId.c_str(), iReleaseDate);
  return iReleaseDate;
}

bool BXVideoDatabase::GetTvShows(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strGenre, const std::string& strPrefix, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::GetTvShows, genre = %s, strPrefix = %s (videodb)", strGenre.c_str(), strPrefix.c_str());

  int bResult = false;

  //find all the series available for this user by the episodes according to the shares
  std::set<std::string> setAvailableSeries;
  Dataset* pDS2 = Query("select strSeriesId, strPath from video_files where strSeriesId!=''");
  if (pDS2)
  {
    try
    {
      if (pDS2->num_rows() != 0)
      {
        while (!pDS2->eof())
        {
          if (BXUtils::CheckPathFilter(vecPathFilter, pDS2->fv("strPath").get_asString()))
          {
            setAvailableSeries.insert(pDS2->fv("strSeriesId").get_asString());
            bResult = true;
          }
          pDS2->next();
        }
      }
    }
    catch (dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS2;
  }

  //there are no episodes available for this user
  if (!bResult)
    return false;

  Dataset* pDS = NULL;
  if ( (!strGenre.empty()) && (!strPrefix.empty()))
  {
    if (strPrefix == "#")
    {
      pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0 and ORD(sTitle) < 65 and sGenre LIKE '%%%s%%'",
          strPrefix.c_str(), strGenre.c_str());
    }
    else
    {
      pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0 and sTitle LIKE '%s%%' and sGenre LIKE '%%%s%%'",
          strPrefix.c_str(), strGenre.c_str());
    }
  }
  else if ( (!strGenre.empty()) && (strPrefix.empty()))
  {
    pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0 and sGenre LIKE '%%%s%%'", strGenre.c_str());
  }
  else if ( (strGenre.empty()) && (!strPrefix.empty()))
  {
    if (strPrefix == "#")
    {
      pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0 and ORD(sTitle) < 65 ", strPrefix.c_str());
    }
    else
    {
      pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0 and sTitle LIKE '%s%%' ", strPrefix.c_str());
    }

  }
  else if ( (strGenre.empty()) && (strPrefix.empty()))
  {
    pDS = Query(GET_SERIES_QUERY "where s1.iVirtual=0");
  }

  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0)
        {
          //determine if this series is allowed according to if it has any episodes
          if (setAvailableSeries.find(pDS->fv("sBoxeeId").get_asString()) != setAvailableSeries.end())
          {
            BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_SERIES);
            BXSeries* pSeries = (BXSeries*) pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

            if (LoadSeriesFromDataset(pDS, pSeries) == MEDIA_DATABASE_OK)
            {
              std::string strSeriesPath = "boxeedb://series/";
              strSeriesPath += pSeries->m_strBoxeeId;
              strSeriesPath += "/";

              pMetadata->SetPath(strSeriesPath);
              pMetadata->m_iDateAdded = pSeries->m_iRecentlyAired;

              vecMediaFiles.push_back(pMetadata);
              iItemLimit--;
              bResult = true;
            }
            else
            {
              delete pMetadata;
            }
          }
          pDS->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else {
    bResult = false;
  }

  return bResult;
}

bool BXVideoDatabase::ConvertDatasetToUniqueSet(std::set<std::string>& output, dbiplus::Dataset* pDS , const std::vector<std::string>& vecPathFilter)
{
  if (pDS && pDS->num_rows() != 0)
  {
    while (!pDS->eof())
    {
      std::string rowGenreValue = pDS->fv(0).get_asString();
      std::string rowPathValue = pDS->fv(1).get_asString();

      if (BXUtils::CheckPathFilter(vecPathFilter, rowPathValue))
      {
        std::vector<std::string> genres = BXUtils::StringTokenize(rowGenreValue,",");

        for (std::vector<std::string>::iterator it = genres.begin(); it != genres.end() ; it++)
        {
          BXUtils::StringTrim((*it));
          BXUtils::StringToLower((*it));
          output.insert((*it));
        }
      }

      pDS->next();
    }
    return true;
  }
  return false;
}

bool BXVideoDatabase::GetTvShowsGenres (std::set<std::string> &setGenres , const std::vector<std::string>& vecPathFilter)
{
  bool bResult = false;
  Dataset* pDS = Query(" select sr.strGenre, vf.strPath from video_files vf, series sr where (sr.strBoxeeId == vf.strSeriesId AND iDropped == 0 AND idFile == 1) ");

  try
  {
    if (pDS && pDS->num_rows() != 0)
      bResult = ConvertDatasetToUniqueSet(setGenres , pDS, vecPathFilter);
  }
  catch(dbiplus::DbErrors& e)
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
  }

  delete pDS;

  return bResult;
}

bool BXVideoDatabase::GetMoviesGenres (std::set<std::string> &setGenres , const std::vector<std::string>& vecPathFilter)
{
  bool bResult = false;
  Dataset* pDS = Query(" select distinct strGenre, strPath from video_files where (strGenre <> '' AND iSeason == -1 AND iEpisode == -1 AND iDropped == 0 AND idFile == 1) ");

  try
  {
    if (pDS && pDS->num_rows() != 0)
      bResult = ConvertDatasetToUniqueSet(setGenres , pDS, vecPathFilter);
  }
  catch(dbiplus::DbErrors& e)
  {
    LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
  }

  delete pDS;

  return bResult;
}

bool BXVideoDatabase::SearchTvShowsByTitle(const std::string& strTitle, std::vector<BXMetadata*> &vecMediaFiles, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  if (strTitle.empty())
    return true;

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::SearchTvShowsByTitle, title = %s (search)", strTitle.c_str());
  bool bResult = true;

  Dataset* pDS = Query( GET_SERIES_QUERY "where (sTitle LIKE '%%%s%%' or sTitle LIKE '%%%s%%') and iVirtual=0", strTitle.c_str(), strTitle.c_str());
  if (pDS)
  {
    try
    {
      if (pDS->num_rows() != 0)
      {
        while (!pDS->eof() && iItemLimit > 0)
        {
          // Since we have retrieved only non virtual series, there should be at least one episode in each
          if (!HasEpisodes(pDS->fv("sBoxeeId").get_asString(), -1, vecPathFilter))
          {
            pDS->next();
            continue;
          }

          BXMetadata* pMetadata = new BXMetadata(MEDIA_ITEM_TYPE_SERIES);
          BXSeries* pSeries = (BXSeries*)pMetadata->GetDetail(MEDIA_DETAIL_SERIES);

          if (LoadSeriesFromDataset(pDS, pSeries) == MEDIA_DATABASE_OK)
          {
            std::string strSeriesPath = "boxeedb://series/";
            strSeriesPath += pSeries->m_strBoxeeId;
            strSeriesPath += "/";

            pMetadata->SetPath(strSeriesPath);

            pMetadata->m_iDateAdded = pSeries->m_iRecentlyAired;// GetLatestEpisodeDate(pSeries->m_strBoxeeId);

            vecMediaFiles.push_back(pMetadata);
            iItemLimit--;
          }
          else
          {
            delete pMetadata;
          }

          pDS->next();
        }
      }
      else
      {
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::SearchTvShowsByTitle, found no tv shows with name = %s (browse)", strTitle.c_str());
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDS;
  }
  else
  {
    bResult = false;
  }

  return bResult;
}

bool BXVideoDatabase::GetVideosByTitle(std::vector<BXMetadata*> &vecMediaFiles, const std::string& strTitle, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{
  Dataset* pDS = Query("select * from video_files where idFile = 1 and strTitle='%s'", strTitle.c_str());
  return CreateVideosFromDataset(pDS, vecMediaFiles, vecPathFilter, iItemLimit);
}

bool BXVideoDatabase::GetVideoParts(int iVideoId, std::vector<std::string> &vecVideoParts)
{
  bool bResult = true;
  Dataset* pDSb = Query("select strPath from video_parts where idVideo=%d ORDER BY iPartNumber", iVideoId);
  if (pDSb)
  {
    try
    {
      if (pDSb->num_rows() != 0 )
      {
        while (!pDSb->eof())
        {
          vecVideoParts.push_back(pDSb->fv(0).get_asString());
          pDSb->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDSb;
  }
  else {
    bResult = false;
  }

  return bResult;
}

bool BXVideoDatabase::GetLinks(const std::string& strBoxeeId, std::vector<BXPath> &vecLinks, const std::vector<std::string>& vecPathFilter)
{
  bool bResult = true;
  Dataset* pDSa = Query("select idVideo, strType, strPath from video_files where idFile=1 and iDropped=0 and strBoxeeId = '%s'", strBoxeeId.c_str());

  if (pDSa)
  {
    try
    {
      if (pDSa->num_rows() != 0)
      {
        while (!pDSa->eof())
        {
          int id = pDSa->fv(0).get_asInt(); // idVideo
          std::string strType =pDSa->fv(1).get_asString(); //   strType
          std::string strPath = pDSa->fv(2).get_asString(); // strPath

          if (BXUtils::CheckPathFilter(vecPathFilter,strPath))
          {
            BXPath path;
            if (strType == "part")
            {
              GetVideoParts(id, path.m_vecParts);
            }
            else
            {
              path.m_strPath = strPath;
            }

            vecLinks.push_back(path);

            LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::GetLinks, added %d paths (ri)", vecLinks.size());
          }
          pDSa->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDSa;
  }
  else
  {
    bResult = false;
  }

  return bResult;
}

bool BXVideoDatabase::GetActorsByVideoId(int iVideoId, std::vector<std::string>& vecActors)
{
  // Retreive all actors from specific video
  std::string strActor;
  bool bResult = true;

  Dataset* pDSb = Query("select a.strName from actor_to_video atv left outer join actors a on atv.idActor=a.idActor where atv.idVideo=%d", iVideoId);
  if (pDSb)
  {
    try
    {
      if (pDSb->num_rows() != 0 )
      {
        while (!pDSb->eof())
        {
          // Get actor details
          strActor = pDSb->fv(0).get_asString();
          vecActors.push_back(strActor);

          pDSb->next();
        }
      }
    }
    catch(dbiplus::DbErrors& e)
    {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
      bResult = false;
    }
    delete pDSb;
  }
  else
  {
    bResult = false;
  }

  return bResult;

}

std::string BXVideoDatabase::GetDirectorById(int iDirectorId)
{
  std::string strDirector = "";
  Dataset* pDSb = Query("select strName from directors where idDirector=%d", iDirectorId);
  if (pDSb) {
    try {
      if (pDSb->num_rows() != 0 ) {
        strDirector = pDSb->fv(0).get_asString();
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
    delete pDSb;
  }
  return strDirector;
}

std::string BXVideoDatabase::GetDirectorByVideoId(int iVideoId)
{
  std::string strDirector = "";

  Dataset* pDSb = Query("select a.strName from director_to_video dtv left outer join directors a on dtv.idDirector=a.idDirector where dtv.idVideo=%d", iVideoId);
  if (pDSb) {
    try {
      if (pDSb->num_rows() != 0 ) {
        strDirector = pDSb->fv(0).get_asString();
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
    delete pDSb;
  }
  return strDirector;
}

int BXVideoDatabase::RemoveVideoByPath(const std::string& strPath)
{
  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveVideoByPath, removing video from database: path = %s", strPath.c_str());
  Dataset* pDS_t = Exec("delete from video_files where strPath='%s'", strPath.c_str());
  if (pDS_t)
    delete pDS_t;

  //lock to avoid race condition between the select (checking if there's existing record) and the delete query
  CSingleLock lock (m_lock);

  // Check if this is a part
  Dataset* pDS = Query("select * from video_parts where strPath='%s'", strPath.c_str());

  if (pDS) {
    try {
      if (pDS->num_rows() != 0 ) {
        int iVideoId = pDS->fv("idVideo").get_asInt();

        // Delete all parts related to this video
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveVideoByPath, removing video from database parts: id = %d", iVideoId);
        pDS_t = Exec("delete from video_parts where idVideo=%d", iVideoId);
        if (pDS_t)
          delete pDS_t;

        // Delete all parts related to the video itself
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveVideoByPath, removing video from database video_files: id = %d", iVideoId);
        pDS_t = Exec("delete from video_files where idVideo=%d", iVideoId);
        if (pDS_t)
          delete pDS_t;
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
  }

  return MEDIA_DATABASE_OK;
}

int BXVideoDatabase::RemoveVideoByFolder(const std::string& strFolderPath)
{
  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveVideoByFolder, erasing all video by folder path = %s", strFolderPath.c_str());
  Dataset* pDS_t = Exec("delete from video_files where strPath LIKE '%s%%'", strFolderPath.c_str());
  if (pDS_t) {
    delete pDS_t;
  }

  LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::RemoveVideoByFolder, erasing all video parts by folder path = %s", strFolderPath.c_str());
  pDS_t = Exec("delete from video_parts where strPath LIKE '%s%%'", strFolderPath.c_str());
  if (pDS_t) {
    delete pDS_t;
  }

  // TODO: What about series removal

  return MEDIA_DATABASE_OK;
}

int BXVideoDatabase::RemoveSeries(int iSeriesId, int iSeason)
{
  bool bRemoveSeason = true;
  bool bRemoveSeries = true;

  //lock to avoid race condition between the select (checking if there's existing record) and the delete query
  CSingleLock lock (m_lock);

  // Get all vido files that belong to the series
  Dataset* pDS = Query("select * from video_files where idSeries=%d", iSeriesId);
  if (pDS) {
    if (pDS->num_rows() != 0 ) {
      bRemoveSeries = false;
      if (iSeason != -1) {
        // Look for episodes from specific season
        while (!pDS->eof()) {
          if (pDS->fv("iSeason").get_asInt() == iSeason) {
            // We found an episode from the requested season, do not delete
            bRemoveSeason = false;
          }

          pDS->next();
        }
      }
    }

    delete pDS;
  }
  else {
    return MEDIA_DATABASE_ERROR;
  }

  if (bRemoveSeason) {
    LOG(LOG_LEVEL_DEBUG, "removing season, id= %d, series = %d", iSeason, iSeriesId);
    Dataset* pDS_t = Exec("delete from seasons where idSeries=%d and iSeasonNum=%d", iSeriesId, iSeason);
    if (pDS_t) {
      delete pDS_t;
    }
  }

  if (bRemoveSeries) {
    LOG(LOG_LEVEL_DEBUG, "removing series,  series = %d", iSeriesId);
    Dataset* pDS_t = Exec("delete from series where idSeries=%d", iSeriesId);
    if (pDS_t) {
      delete pDS_t;
    }
  }

  return MEDIA_DATABASE_OK;

}

std::string BXVideoDatabase::GetActorById(int iActorId)
{
  std::string strActor = "";
  Dataset* pDSb = Query("select strName from actors where idActor=%d", iActorId);
  if (pDSb) {
    try {
      if (pDSb->num_rows() != 0 ) {
        strActor = pDSb->fv(0).get_asString();
      }
    }
    catch(dbiplus::DbErrors& e) {
      LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
    }
    delete pDSb;
  }
  return strActor;
}

int BXVideoDatabase::AddVideoLink(const std::string& strBoxeeId , const BXVideoLink& videoLink)
{
  int iID = -1;

  //lock to avoid race condition between the select (checking if there's existing record) and the insert query
  CSingleLock lock (m_lock);

  Dataset* pDS = Query("select 1 from video_files where strBoxeeId='%s'", strBoxeeId.c_str());
  if (pDS)
  {
    if (pDS->num_rows() > 0)
    {
      //lets query the db instead of getting an sql error while trying to create a duplicate entry
      Dataset* pDS2 = Query("select 1 from video_links where (strBoxeeId='%s' and strURL='%s')",strBoxeeId.c_str(),videoLink.m_strURL.c_str());

      if (pDS2)
      {
        if (pDS2->num_rows() < 1)
        {
          LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideoLink, creating video link : [videoLink=%s] for [strBoxeeId=%s]",videoLink.m_strURL.c_str(),strBoxeeId.c_str());

          iID = Insert("insert into video_links (idLink, strBoxeeId, strURL, strProvider, strProviderName, strProviderThumb, strTitle,"
                                          "strQuality, strQualityLabel, bIsHD, strType, strBoxeeType, strCountryCodes, strCountryRel, strOffer )"
                                          "values( NULL, '%s', '%s', '%s', '%s', '%s' , '%s' , '%s' ,'%s' , '%d' , '%s', '%s', '%s', '%s' ,'%s' )",
                                          strBoxeeId.c_str(), videoLink.m_strURL.c_str(), videoLink.m_strProvider.c_str(), videoLink.m_strProviderName.c_str(),
                                          videoLink.m_strProviderThumb.c_str(), videoLink.m_strTitle.c_str(), videoLink.m_strQuality.c_str(),
                                          videoLink.m_strQualityLabel.c_str(), videoLink.m_bIsHD?1:0, videoLink.m_strType.c_str(), videoLink.m_strBoxeeType.c_str(),
                                          videoLink.m_strCountryCodes.c_str(), videoLink.m_strCountryRel.c_str(), videoLink.m_strOffer.c_str());
        }
        else
        {
          LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideoLink, found existing video link for: [strBoxeeId=%s][videoLink=%s]",strBoxeeId.c_str(),videoLink.m_strURL.c_str());
        }

        delete pDS2;
      }
      else
      {
        LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideoLink, Error while trying to find specific video link for: [strBoxeeId=%s][videoLink=%s]",strBoxeeId.c_str(),videoLink.m_strURL.c_str());
      }
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideoLink, Didn't find relevant BoxeeID in video files: [strBoxeeId=%s].",strBoxeeId.c_str());
    }
    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_DEBUG, "BXVideoDatabase::AddVideoLink, Error while trying to find relevant BoxeeID in video files: [strBoxeeId=%s].",strBoxeeId.c_str());
  }

  return iID;
}

int BXVideoDatabase::RemoveVideoLinkByURL(const std::string& strURL)
{
  LOG(LOG_LEVEL_DEBUG, "removing video link with path = %s", strURL.c_str());
  Dataset* pDS = Exec("delete from video_links where strURL='%s'" , strURL.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXVideoDatabase::RemoveVideoLinksByBoxeeId(const std::string& strBoxeeId)
{
  LOG(LOG_LEVEL_DEBUG, "removing video link with BoxeeID = %s", strBoxeeId.c_str());
  Dataset* pDS = Exec("delete from video_links where strBoxeeId='%s'" , strBoxeeId.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXVideoDatabase::GetVideoLinks(std::vector<BXVideoLink>& videoLinks, const std::string& strBoxeeId, const std::string& strBoxeeType , int iCount)
{
  if (strBoxeeId.empty() && strBoxeeType.empty())
  {
    LOG(LOG_LEVEL_ERROR,"BXVideoDatabase::GetVideoLinks - enter function with EMPTY parameters. [boxeeId=%s][strBoxeeType=%s]",strBoxeeId.c_str(),strBoxeeType.c_str());
    return MEDIA_DATABASE_ERROR;
  }

  LOG(LOG_LEVEL_DEBUG, "reading video links BoxeeId: %s , strBoxeeType: %s", strBoxeeId.c_str(), strBoxeeType.c_str());
  std::string strSQL = "select * from video_links";

  if (!strBoxeeId.empty() || !strBoxeeType.empty())
  {
    strSQL += " where ";

    if (!strBoxeeId.empty())
    {
      std::string strBoxeeIdQuery = "strBoxeeId='" + strBoxeeId + "'";
      strSQL += strBoxeeIdQuery;
    }

    if (!strBoxeeType.empty())
    {
      if (!strBoxeeId.empty())
      {
        strSQL += " AND ";
      }

      std::string strTypeQuery = "strBoxeeType='" + strBoxeeType + "'";
      strSQL += strTypeQuery;
    }

    if (iCount > 0)
    {
      strSQL += " order by idLink asc limit " + BXUtils::IntToString(iCount);
    }
  }

  Dataset* pDS = Query(strSQL.c_str());

  if (pDS)
  {
    if (pDS->num_rows() > 0)
    {
      while (!pDS->eof())
      {
        BXVideoLink link;

        link.m_strURL = pDS->fv(2).get_asString();
        link.m_strProvider = pDS->fv(3).get_asString();
        link.m_strProviderName = pDS->fv(4).get_asString();
        link.m_strProviderThumb = pDS->fv(5).get_asString();
        link.m_strTitle = pDS->fv(6).get_asString();
        link.m_strQuality = pDS->fv(7).get_asString();
        link.m_strQualityLabel = pDS->fv(8).get_asString();
        link.m_bIsHD = pDS->fv(9).get_asBool();
        link.m_strType = pDS->fv(10).get_asString();
        link.m_strBoxeeType = pDS->fv(11).get_asString();
        link.m_strCountryCodes = pDS->fv(12).get_asString();
        link.m_strCountryRel = pDS->fv(13).get_asString();
        link.m_strOffer = pDS->fv(14).get_asString();

        videoLinks.push_back(link);

        pDS->next();
      }
    }
  }
  else
  {
    return MEDIA_DATABASE_ERROR;
  }
  return MEDIA_DATABASE_OK;
}

int BXVideoDatabase::DropVideoByPath(const std::string& strPath)
{
  LOG(LOG_LEVEL_DEBUG, "LIBRARY, DROP: drop video,  path %s", strPath.c_str());
  Dataset* pDS = Exec("update video_files set iDropped=1 where strPath = '%s'", strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXVideoDatabase::DropVideoById(int iId)
{
  LOG(LOG_LEVEL_DEBUG, "LIBRARY, DROP: drop video,  id =  %d", iId);
  Dataset* pDS = Exec("update video_files set iDropped=1 where idVideo = %i", iId);
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

}  // namespace BOXEE
