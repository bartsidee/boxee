
#include "bxmediadatabase.h"
#include "bxmediafile.h"
#include "bxmetadata.h"
#include "logger.h"
#include "bxutils.h"
#include "bxconfiguration.h"
#include "boxee.h"
#include "time.h"

using namespace dbiplus;



namespace BOXEE {

bool BOXEE::BXMediaDatabase::m_bHasNewFiles = false;



#define BOXEE_DATABASE_VERSION 4

BXMediaDatabase::BXMediaDatabase()
{
  Open();
}

BXMediaDatabase::~BXMediaDatabase()
{

}

bool BXMediaDatabase::TablesExist()
{
  bool bExists;
  LOG(LOG_LEVEL_DEBUG, "File exists, check if tables are created");

  // Check if the version table can be accessed
  Dataset* pDS = Query("select distinct 1 from sqlite_master where tbl_name='media_folders'");

  if (pDS && pDS->num_rows() > 0) 
    bExists = true;
  else 
    bExists = false;

  if (pDS)
    delete pDS;

  return bExists;
}

bool BXMediaDatabase::CleanTables()
{
  std::vector<std::string> vecQueries;

  Dataset* pDS = Query("select 'DELETE FROM ' || name || ';' from sqlite_master where type = 'table';");

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof())
      {
        std::string strQuery = pDS->fv(0).get_asString();
        vecQueries.push_back(strQuery);
        pDS->next();
      }
    }
    delete pDS;
  }

  Create("BEGIN TRANSACTION;");

  for (unsigned int i = 0; i < vecQueries.size(); i++)
  {
    Create(vecQueries[i].c_str());
  }

  Create("COMMIT;");
  Create("REINDEX;");

  return true;
}

bool BXMediaDatabase::CreateTables()
{
  bool result = true;
  try {
    // We assume that the version tables, etc, were already created

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating media_folders table");
    result &= Create("CREATE TABLE media_folders ( idFolder integer primary key autoincrement, strPath text, strType text, iMetadataId integer, "
        "strStatus text, iDateStatus integer, iHasMetadata integer, iDateAdded integer, iDateModified integer, iNeedsRescan integer);");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating media_folders indexes");
    result &= Create("CREATE INDEX media_folders_srtPath_idx ON media_folders(strPath);\n");
    result &= Create("CREATE INDEX media_folders_strStatus_idx ON media_folders(strStatus);\n");

    // Creating tables for watched feature
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating watched table (wchd)");
    result &= Create("CREATE TABLE watched ( idWatched integer primary key autoincrement, strPath text, strBoxeeId text, iPlayCount integer, iLastPlayed integer, fPositionInSeconds double);");
    result &= Create("CREATE INDEX watched_srtPath_idx ON watched(strPath);\n");
    result &= Create("CREATE INDEX watched_strBoxeeId_idx ON watched(strBoxeeId);\n");

    result &= Create("CREATE TABLE queue ( idQueue integer primary key autoincrement, strLabel text, strPath text, strThumb text, strClientId text);");
    result &= Create("CREATE INDEX queue_strPath_idx ON queue(strPath);\n");
    result &= Create("CREATE INDEX queue_strClientId_idx ON queue(strClientId);\n");

    result &= Create("CREATE TABLE media_shares ( idShare integer primary key autoincrement, strLabel text, strPath text, strType text, iScanType integer, iLastScanned integer);");
    //result &= Create("CREATE INDEX shares_strPath_idx ON queue(strPath);\n");
    //result &= Create("CREATE INDEX queue_strClientId_idx ON queue(strClientId);\n");

    // Table with custom properties, with name and value
    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating properties table");
    result &= Create("CREATE TABLE properties (strName text, strValue text)");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating audio_files table");
    result &= Create("CREATE TABLE audio_files ( idAudio integer primary key autoincrement, idFile integer, strPath text, strTitle text, idAlbum integer, "
        "idArtist integer, idArtist1 integer,  idArtist2 integer, idArtist3 integer, idArtist4 integer, idArtist5 integer, "
        "strGenre text, iYear integer, iDuration integer, iDateAdded integer, iTrackNumber integer, iDateModified integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating audio files index");
    result &= Create("CREATE INDEX audio_idAlbum_idx ON audio_files(idAlbum);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating artists table");
    result &= Create("CREATE TABLE artists ( idArtist integer primary key autoincrement, strName text, strPortrait text, strBio text);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating artists index");
    result &= Create("CREATE UNIQUE INDEX artists_strName_idx ON artists(strName);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating albums table");
    result &= Create("CREATE TABLE albums ( idAlbum integer primary key autoincrement, strTitle text, strPath text, "
        "iNumTracks integer, idArtist integer, iDuration integer, strArtwork text, strDescription text, strLanguage text, "
        "strGenre text, iYear integer, iVirtual integer, iRating integer, "
        "iDateAdded integer, iDateModified integer, iDropped integer, iStatus integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating albums index");
    result &= Create("CREATE INDEX albums_strTitle_idx ON albums(strTitle);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating albums path index");
    result &= Create("CREATE INDEX albums_strPath_idx ON albums(strPath);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating albums artist index");
    result &= Create("CREATE INDEX albums_idArtist_idx ON albums(idArtist);\n");

    // Trigger that will automatically remove the album when the last of its tracks was deleted
    result &= Create("CREATE TRIGGER delete_album_trigger AFTER DELETE ON audio_files "
        "BEGIN "
        "DELETE FROM albums WHERE idAlbum=OLD.idAlbum AND (SELECT COUNT(*) from audio_files where idAlbum=OLD.idAlbum)=0;"
        "END;");


    // Trigger that will automatically remove the artist when the last of his albums was deleted
    result &= Create("CREATE TRIGGER delete_artist_trigger AFTER DELETE ON albums "
        "BEGIN "
        "DELETE FROM artists WHERE idArtist=OLD.idArtist AND (SELECT COUNT(*) from albums where idArtist=OLD.idArtist)=0;"
        "END;");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating album_to_song table");
    result &= Create("CREATE TABLE album_to_song ( idFile integer, idAudio integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating video_files table");
    result &= Create("CREATE TABLE video_files ( idVideo integer primary key autoincrement, idFile integer, strPath text, idFolder integer, strTitle text, strBoxeeId text, "
        "iDuration integer, strType text, strSeriesId text, iSeason integer, iEpisode integer, strDescription text, strExtDescription text, strIMDBKey text, strMPAARating text, "
        "strCredits text, strStudio text, strTagLine text, strCover text, strLanguage text, "
        "strGenre text, iReleaseDate integer, iYear integer, strTrailerUrl text, strShowTitle text, "
        "iDateAdded integer, iHasMetadata integer, iRating integer, iPopularity integer, iDateModified integer, iDropped integer, strNfoPath text, iNfoAccessTime integer, iNfoModifiedTime integer, strOriginalCover text, "
        "iRTCriticsScore integer, strRTCriticsRating text, iRTAudienceScore integer, strRTAudienceRating text);\n");

    result &= Create("CREATE INDEX video_files_strPath_idx ON video_files(strPath);\n");
    result &= Create("CREATE INDEX video_files_strTitle_idx ON video_files(strTitle);\n");
    result &= Create("CREATE INDEX video_files_strBoxeeId_idx ON video_files(strBoxeeId);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating video_parts table");
    result &= Create("CREATE TABLE video_parts (idPart integer primary key autoincrement, idVideo integer, iPartNumber integer, strPath text);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating video_parts index");
    result &= Create("CREATE unique INDEX video_parts_strPath_idx ON video_parts(strPath);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating unresolved_video_files table");
    result &= Create("CREATE TABLE unresolved_video_files ( idVideoFile integer primary key autoincrement, strPath text, strSharePath text, idFolder integer, iStatus integer, idVideo integer, iSize integer, iDateAdded integer);");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating unresolved_video_files index");
    result &= Create("CREATE unique INDEX unresolved_videos_strPath_idx ON unresolved_video_files(strPath);\n");

    // Trigger to automatically remove  video files when its folder is deleted
    result &= Create("CREATE TRIGGER delete_video_files_by_folder_trigger AFTER DELETE ON media_folders "
        "BEGIN "
        "DELETE FROM video_files WHERE video_files.idFolder = OLD.idFolder; "
        "END;");
    // Trigger to automatically remove  video parts  when a video is deleted
    result &= Create("CREATE TRIGGER delete_video_parts_by_id_trigger AFTER DELETE ON video_files "
        "BEGIN "
        "DELETE FROM video_parts WHERE video_parts.idVideo = OLD.idVideo; "
        "END;");

    // Trigger to automatically remove unresolved video files when its folder is deleted
    result &= Create("CREATE TRIGGER delete_unresolved_video_files_by_folder_trigger AFTER DELETE ON media_folders "
        "BEGIN "
        "DELETE FROM unresolved_video_files WHERE unresolved_video_files.idFolder = OLD.idFolder; "
        "END;");

    // Trigger to automatically remove unresolved audio files when its folder is deleted
    result &= Create("CREATE TRIGGER delete_unresolved_audio_files_by_folder_trigger AFTER DELETE ON media_folders "
        "BEGIN "
        "DELETE FROM unresolved_audio_files WHERE unresolved_audio_files.idFolder = OLD.idFolder; "
        "END;");


    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating unresolved_audio_files table");
    result &= Create("CREATE TABLE unresolved_audio_files ( idAudioFile integer primary key autoincrement, strPath text, strSharePath text, idFolder integer, iStatus integer, idAudio integer, iSize integer);");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating unresolved_video_files index");
    result &= Create("CREATE unique INDEX unresolved_audio_strPath_idx ON unresolved_audio_files(strPath);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating provider_preferences");
    result &= Create("CREATE TABLE provider_preferences (iPrefId integer primary key autoincrement, iProvider text, iSelectedQuality integer);");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating provider_preferences table index");
    result &= Create("CREATE unique INDEX provider_preferences_idx ON provider_preferences(iProvider);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating playable_folders");
    result &= Create("CREATE TABLE playable_folders (idFolder integer primary key autoincrement, iFolder text);");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating playable_folders table index");
    result &= Create("CREATE unique INDEX playable_folders_strPath_idx ON playable_folders(iFolder);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating series table");
    result &= Create("CREATE TABLE series ( idSeries integer primary key autoincrement, strTitle text, strPath text, strBoxeeId text, "
        "strCover text, strDescription text, strLanguage text, "
        "strGenre text, iYear integer, iVirtual integer, strNfoPath text, iNfoAccessTime integer, iNfoModifiedTime integer, iLatestLocalEpisodeDate integer);\n");

    //delete the series when all its episodes are deleted
    result &= Create("CREATE TRIGGER delete_series_video_trigger AFTER DELETE ON video_files "
        "BEGIN "
        "DELETE FROM series WHERE length(OLD.strSeriesId)>0 AND series.strBoxeeId = OLD.strSeriesId AND (SELECT COUNT(*) from video_files where strSeriesId=OLD.strSeriesId)=0; "
        "END;");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating series index");
    result &= Create("CREATE unique INDEX series_strBoxeeId_idx ON series(strBoxeeId);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating seasons table");
    result &= Create("CREATE TABLE seasons ( idSeason integer primary key autoincrement, idSeries integer, iSeasonNum integer, strPath text, "
        "strCover text, strDescription text, strLanguage text, iYear integer, iVirtual integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating actors table");
    result &= Create("CREATE TABLE actors ( idActor integer primary key autoincrement, strName text);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating actors index");
    result &= Create("CREATE INDEX actors_strName_idx ON actors(strName);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating directors table");
    result &= Create("CREATE TABLE directors ( idDirector integer primary key autoincrement, strName text);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating directors index");
    result &= Create("CREATE INDEX directors_strName_idx ON directors(strName);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating actor_to_video table");
    result &= Create("CREATE TABLE actor_to_video ( idActor integer, idVideo integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating actor_to_video index");
    result &= Create("CREATE INDEX actor_to_video_idx ON actor_to_video(idActor);\n");
    result &= Create("CREATE INDEX video_to_actor_idx ON actor_to_video(idVideo);\n");

    // Trigger to automatically remove actor when all his movies were deleted
    result &= Create("CREATE TRIGGER delete_actor_video_trigger AFTER DELETE ON video_files "
        "BEGIN "
        "DELETE FROM actor_to_video WHERE actor_to_video.idVideo = OLD.idVideo; "
        "END;");


    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating director_to_video table");
    result &= Create("CREATE TABLE director_to_video ( idDirector integer, idVideo integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating director_to_video index");
    result &= Create("CREATE INDEX director_to_video_idx ON director_to_video(idDirector);\n");
    result &= Create("CREATE INDEX video_to_director_idx ON director_to_video(idVideo);\n");

    // Trigger to automatically remove director when all his movies were deleted
    result &= Create("CREATE TRIGGER delete_director_video_trigger AFTER DELETE ON video_files "
        "BEGIN "
        "DELETE FROM director_to_video WHERE director_to_video.idVideo = OLD.idVideo; "
        "END;");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating pictures table");
    result &= Create("CREATE TABLE pictures ( idPicture integer primary key autoincrement, idFile integer, strPath text, iDate integer, iDateAdded integer, iHasMetadata integer, iDateModified integer);\n");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating video_links table");
    result &= Create("CREATE TABLE video_links ( idLink integer primary key autoincrement, strBoxeeId text, strURL text, strProvider text , strProviderName text, strProviderThumb text, strTitle text, strQuality text, strQualityLabel text, bIsHD integer, strType text, strBoxeeType text, strCountryCodes text, strCountryRel text, strOffer text, UNIQUE(strBoxeeId,strURL));\n");

    // Trigger to automatically remove video_links if there are no more related video_files
    result &= Create("CREATE TRIGGER delete_video_links_trigger AFTER DELETE ON video_files "
        "BEGIN "
        "DELETE FROM video_links WHERE strBoxeeId=OLD.strBoxeeId AND (SELECT COUNT(*) from video_files where strBoxeeId=OLD.strBoxeeId)=0; "
        "END;");

    LOG(LOG_LEVEL_DEBUG, "LIBRARY: Finished creating media database");

    //update the version of db
    Dataset* pDS = Exec("update version set iVersion=17");
    if (pDS)
    {
      delete pDS;
    }
  }
  catch(dbiplus::DbErrors& e) {
    LOG(LOG_LEVEL_ERROR, e.getMsg());
    result = false;
  }
  catch(...) {
    LOG(LOG_LEVEL_ERROR, "Could not create tables, exception was caught during operation");
    result = false;
  }
  return result;
}

bool BXMediaDatabase::UpdateTables()
{
  Dataset* pDS = Query("select * from version");

  bool bResult = true;

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      int iVersion = pDS->fv("iVersion").get_asInt();

      if (iVersion <= 1) 
      {
        LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Older database version (2) detected, updating tables");

        Create("alter table video_files add iDropped integer");
        Create("alter table albums add iDropped integer");

        Dataset* pDS = Exec("update version set iVersion=3");
        if (pDS)
        {
          delete pDS;
        }

        bResult &= true;
      }

      if (iVersion <= 2)
      {
        // Update the series table, using video files, to distinguish between tvshows from feed and from disks
        Dataset* pDS2 = Query("select * from series");
        if (pDS2) 
        {
          try 
          {
            if (pDS2->num_rows() != 0)
            {
              while (!pDS2->eof())
              {
                // Get id of the series and check whether there is at least 1 non virtual episode for it
                int idSeries = pDS2->fv("idSeries").get_asInt();

                Dataset* pDS3 = Query("select * from video_files where idSeries=%i and idFile=1", idSeries);
                if (pDS3 && pDS3->num_rows() != 0 )
                {
                  delete pDS3;
                  // Update the series to non virtual status
                  Dataset* pDS4 = Exec("update series set iVirtual=0 where strTitle='%s'", pDS2->fv("strTitle").get_asString().c_str());
                  if (pDS4)
                  {
                    delete pDS4;
                  }
                  else
                  {
                    bResult = false;
                  }
                }

                pDS2->next();
              }
              bResult &= true;
            }
          }
          catch(dbiplus::DbErrors& e) {
            LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Could not update database");
            bResult = false;
          }
          delete pDS2;
        }

        Dataset* pDS = Exec("update version set iVersion=3");
        if (pDS)
        {
          delete pDS;
        }
      }

      if (iVersion <= 3) 
      {
        LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Older database version (3) detected, updating tables");

        // Add status field to albums table
        Create("alter table albums add iStatus integer");

        // Set resolved status to all existing albums 
        Dataset* pDS = Exec("update albums set iStatus=%i", STATUS_RESOLVED);
        if (pDS)
        {
          delete pDS;
        }

        pDS = Exec("update version set iVersion=5");
        if (pDS)
        {
          delete pDS;
        }

        bResult &= true;
      }

      if (iVersion <= 4) 
      {
        LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Older database version (4) detected, updating indexes");

        Create("DROP   INDEX media_folders_srtPath_strType_idx;\n");
        Create("CREATE INDEX media_folders_srtPath_idx ON media_folders(strPath);\n");
        Create("CREATE INDEX media_folders_strStatus_idx ON media_folders(strStatus);\n");
        Create("CREATE INDEX albums_idArtist_idx ON albums(idArtist);\n");

        Dataset* pDS = Exec("update version set iVersion=5");
        if (pDS)
          delete pDS;

        bResult &= true;
      }

      if (iVersion <= 5)
      {
        LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Older database version (5) detected, updating indexes");

        Create("CREATE TABLE unresolved_video_files ( idVideoFile integer primary key autoincrement, strPath text, strSharePath text, idFolder integer, iStatus integer, idVideo integer, iSize integer);");
        Create("CREATE unique INDEX unresolved_videos_strPath_idx ON unresolved_video_files(strPath);\n");
        Create("CREATE TABLE unresolved_audio_files ( idAudioFile integer primary key autoincrement, strPath text, strSharePath text, idFolder integer, iStatus integer, idAudio integer, iSize integer);");
        Create("CREATE unique INDEX unresolved_audio_strPath_idx ON unresolved_audio_files(strPath);\n");

        Create("CREATE TABLE provider_preferences (iPrefId integer primary key autoincrement, iProvider text, iSelectedQuality integer);");
        Create("CREATE unique INDEX provider_preferences_idx ON provider_preferences(iProvider);\n");

        Dataset* pDS = Exec("update version set iVersion=6");
        if (pDS)
          delete pDS;
        bResult &= true;
      }

      if (iVersion <= 6)
      {
    	    // Trigger to automatically remove  video files when its folder is deleted
    	    Create("CREATE TRIGGER delete_video_files_by_folder_trigger AFTER DELETE ON media_folders "
    	        "BEGIN "
    	        "DELETE FROM video_files WHERE video_files.idFolder = OLD.idFolder; "
    	        "END;");

    	    // Trigger to automatically remove  video parts  when a video is deleted
    	    Create("CREATE TRIGGER delete_video_parts_by_id_trigger AFTER DELETE ON video_files "
    	        "BEGIN "
    	        "DELETE FROM video_parts WHERE video_parts.idVideo = OLD.idVideo; "
    	        "END;");

    	    // Trigger to automatically remove unresolved video files when its folder is deleted
    	    Create("CREATE TRIGGER delete_unresolved_video_files_by_folder_trigger AFTER DELETE ON media_folders "
    	        "BEGIN "
    	        "DELETE FROM unresolved_video_files WHERE unresolved_video_files.idFolder = OLD.idFolder; "
    	        "END;");

    	    // Trigger to automatically remove unresolved audio files when its folder is deleted
    	    Create("CREATE TRIGGER delete_unresolved_audio_files_by_folder_trigger AFTER DELETE ON media_folders "
    	        "BEGIN "
    	        "DELETE FROM unresolved_audio_files WHERE unresolved_audio_files.idFolder = OLD.idFolder; "
    	        "END;");

            Dataset* pDS = Exec("update version set iVersion=7");
            if (pDS)
              delete pDS;
        bResult &= true;

      }

      if (iVersion <= 7)
      {
        Create("CREATE TABLE playable_folder (idFolder integer primary key autoincrement, iFolder text);");
        Create("CREATE unique INDEX playable_folders_strPath_idx ON playable_folders(iFolder);\n");

        Dataset* pDS = Exec("update version set iVersion=8");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 8 )
      {
        Create("DROP TABLE playable_folder;");
        Create("DROP INDEX playable_folders_strPath_idx;");

        Create("CREATE TABLE playable_folders (idFolder integer primary key autoincrement, iFolder text);");
        Create("CREATE unique INDEX playable_folders_strPath_idx ON playable_folders(iFolder);\n");

        Dataset* pDS = Exec("update version set iVersion=9");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 9)
      {
        Create("alter table series add column strNfoPath text;");
        Create("alter table series add column iNfoAccessTime integer;");
        Create("alter table series add column iNfoModifiedTime integer;");

        Create("alter table video_files add column strNfoPath text;");
        Create("alter table video_files add column iNfoAccessTime integer;");
        Create("alter table video_files add column iNfoModifiedTime integer;");

        Dataset* pDS = Exec("update version set iVersion=10");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 10)
      {
        Create("CREATE TABLE video_links ( idLink integer primary key autoincrement, strBoxeeId text, strURL text, strProvider text , strProviderName text, strProviderThumb text, strTitle text, strQuality text, strQualityLabel text, bIsHD integer, strType text, strBoxeeType text, strCountryCodes text, strCountryRel text, strOffer text, UNIQUE(strBoxeeId,strURL));");

        // Trigger to automatically remove video_links if there are no more related video_files
        Create("CREATE TRIGGER delete_video_links_trigger AFTER DELETE ON video_files "
            "BEGIN "
            "DELETE FROM video_links WHERE strBoxeeId=OLD.strBoxeeId AND (SELECT COUNT(*) from video_files where strBoxeeId=OLD.strBoxeeId)=0; "
            "END;");

        Dataset* pDS = Exec("update version set iVersion=11");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 11)
      {
        Create("alter table unresolved_video_files add column iDateAdded integer;");

        Dataset* pDS = Exec("update version set iVersion=12");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 12)
      {
        Create("DROP INDEX series_strTitle_idx;\n");

        Dataset* pDS = Exec("update version set iVersion=13");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 13)
      {
        Create("alter table video_files add column strOriginalCover text;\n");

        Dataset* pDS = Exec("update version set iVersion=14");
            if (pDS)
              delete pDS;

        bResult &= true;
      }

      if (iVersion <= 14)
      {
        Create("alter table series add column iLatestLocalEpisodeDate integer;\n");

        //delete the series when all its episodes are deleted
        Create("CREATE TRIGGER delete_series_video_trigger AFTER DELETE ON video_files "
               "BEGIN "
               "DELETE FROM series WHERE length(OLD.strSeriesId)>0 AND series.strBoxeeId = OLD.strSeriesId AND (SELECT COUNT(*) from video_files where strSeriesId=OLD.strSeriesId)=0; "
               "END;");

        Dataset* pDS = Exec("update version set iVersion=15");
            if (pDS)
              delete pDS;

        bResult &= true;
      }
      if (iVersion <= 15)
      {
        //migrate the latest episode date to the tv show
        Dataset* pDS = Query("select strBoxeeId,iLatestLocalEpisodeDate from series where iLatestLocalEpisodeDate is null");

        if (pDS)
        {
          try
          {
            if (pDS->num_rows() != 0)
            {
              LOG(LOG_LEVEL_DEBUG,"Migrating db to version 16, updating column iLatestLocalEpisodeDate in series for %d rows. (dbupdate)",pDS->num_rows());
              while (!pDS->eof())
              {
                Dataset* pDSb = Query("select iDateAdded from video_files where strSeriesId='%s' and iDropped=0 order by iDateAdded desc limit 1",pDS->fv("strBoxeeId").get_asString().c_str());
                if (pDSb)
                {
                  try
                  {
                    if (pDSb->num_rows() != 0)
                    {
                      unsigned int latestEpisode = pDSb->fv("iDateAdded").get_asUInt();

                      LOG(LOG_LEVEL_DEBUG, "Latest episode for Series id=%s is %d (dbupdate)",pDS->fv("strBoxeeId").get_asString().c_str(),latestEpisode);

                      Dataset* pDSc = Exec("update series set iVirtual=0 , iLatestLocalEpisodeDate=%i where strBoxeeId='%s'", latestEpisode, pDS->fv("strBoxeeId").get_asString().c_str());
                      if (pDSc)
                      {
                        delete pDSc;
                      }
                    }
                  }
                  catch(dbiplus::DbErrors& e)
                  {
                    LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
                  }
                  delete pDSb;
                }
                else
                {
                  LOG(LOG_LEVEL_DEBUG, "Couldn't find episodes for the Series (id=%s), this show will soon be deleted from the db automatically.. (dbupdate)",pDS->fv("strBoxeeId").get_asString().c_str());
                }
                pDS->next();
              }
            }
          }
          catch(dbiplus::DbErrors& e)
          {
            LOG(LOG_LEVEL_ERROR, "Exception caught. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
            bResult = false;
          }
          delete pDS;
        }

        Dataset* pDSa = Exec("update version set iVersion=16");
             if (pDSa)
               delete pDSa;

        bResult &= true;
      }
      if (iVersion <= 16)
      {
        Create("alter table video_files add column iRTCriticsScore integer;\n");
        Create("alter table video_files add column strRTCriticsRating text;\n");
        Create("alter table video_files add column iRTAudienceScore integer;\n");
        Create("alter table video_files add column strRTAudienceRating text;\n");

        Dataset* pDS = Exec("update version set iVersion=17");
                   if (pDS)
                     delete pDS;

        bResult &= true;
      }
      if (iVersion == 17)
      {
        LOG(LOG_LEVEL_DEBUG, "BOXEE DATABASE, Database version is up to date (16)");
        bResult &= true;
      }
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Could not check database version");
  }

  return bResult;
}

int BXMediaDatabase::AddMediaFolder(const std::string& strPath, const std::string& strType, int iDateModified)
{
  time_t now = time(NULL);

  Dataset* pDS = Query("select iDateModified, idFolder from media_folders where strPath='%s' and strType='%s'", strPath.c_str(), strType.c_str());
  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      // Folder already exists in the database
      int iOldModifiedDate  = pDS->fv("iDateModified").get_asInt();
      int iFolderId = pDS->fv("idFolder").get_asInt();
      if (iDateModified > iOldModifiedDate)
      {
        // The directory changed and should be rescanned

        // TODO: Check if this is necessary
        Dataset* pDS2 = Exec("update media_folders set strStatus='scanned', iDateStatus=%i, iMetadataId=0, iHasMetadata=0, "
            "iDateModified=%i where idFolder=%i", now, iDateModified, iFolderId);
        if (pDS2)
        {
          delete pDS2;
        }
      }

      delete pDS;
      return iFolderId;
    }

    delete pDS;
  }

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::AddMediaFolder, QUERY WRITE, insert into media_folders");
  int iResult = Insert("insert into media_folders (idFolder, strPath, strType, iMetadataId, strStatus, iDateStatus, iHasMetadata, iDateAdded, iDateModified, iNeedsRescan) "
      "values (NULL, '%s', '%s', %i, '%s', %i, %i, %i, %i, %i)", strPath.c_str(), strType.c_str(), -1, "scanned", now, 0, now, iDateModified, FOLDER_RETRY);

  if (iResult == MEDIA_DATABASE_ERROR)
  {
    LOG(LOG_LEVEL_ERROR,"BXMediaDatabase::AddMediaFolder - FAILED to insert folder [path=%s][type=%s][dateModified=%d] into media_folders",strPath.c_str(),strType.c_str(),iDateModified);
  }

  return GetMediaFolderId(strPath,strType);
}

int BXMediaDatabase::GetMediaFolderId(const std::string& strPath, const std::string& strType)
{

  Dataset* pDS = Query("select idFolder from media_folders where strPath='%s' and strType='%s'", strPath.c_str(), strType.c_str());
  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      // Folder already exists in the database
      int iFolderId = pDS->fv("idFolder").get_asInt();

      delete pDS;
      return iFolderId;
    }

    delete pDS;
  }

  return 0;
}

bool BXMediaDatabase::UpdateIfModified(const std::string& strPath, int iModDate, bool & bModified)
{
  Dataset* pDS = Query("select iDateModified from media_folders where strPath='%s'", strPath.c_str());

  time_t now = time(NULL);

  bool bResult = false;

  if (pDS) 
  {
    if (pDS->num_rows() != 0)
    {
      int iOldModDate = pDS->fv("iDateModified").get_asInt();

      if (iModDate > iOldModDate) 
      {
        bModified = true;
        //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::AddMediaFolder, QUERY WRITE, update media_folders");
        Dataset* pDS2 = Exec("update media_folders set iDateModified=%i, strStatus='new', iDateStatus=%i where strPath='%s'",
            iModDate, now, strPath.c_str());
        if (pDS2)
        {
          bResult = true;
          delete pDS2;
        }
        else {
          LOG(LOG_LEVEL_ERROR, "Could not update folder modification date, database error");
        }
      }
      else
      {
        bResult = true;
        bModified = false;
      }
    }
    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "Could not check whether the folder was modified, database error");
  }
  return bResult;

}

int BXMediaDatabase::GetUnresolvedFoldersByPath(const std::string& strPath, std::vector<BXMetadata*>& vecFolders)
{
  BXMetadata* pFolder;
  int iResult = MEDIA_DATABASE_OK;

  Dataset* pDS = Query("select strPath,idFolder from media_folders where strStatus='%s' and strPath LIKE '%s%%'", MEDIA_FILE_STATUS_UNRESOLVED, strPath.c_str());
  if (pDS) 
  {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof())
      {

        pFolder = new BXMetadata(MEDIA_ITEM_TYPE_DIR, pDS->fv("strPath").get_asString());
        pFolder->SetMediaFileId(pDS->fv("idFolder").get_asInt());

        vecFolders.push_back(pFolder);

        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    delete pDS;
  }
  else {
    iResult =  MEDIA_DATABASE_ERROR;
  }

  return iResult;
}


int BXMediaDatabase::GetUnresolvedFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType)
{

  BXFolder* pFolder;

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::GetUnresolvedFolders, QUERY, select * from media_folders");
  Dataset* pDS = Query("select strStatus,iDateStatus,strPath,strType,idFolder from media_folders where"
      " (strStatus=='%s' OR strStatus=='%s' OR (strStatus=='%s' AND iNeedsRescan > 0)) AND strType=='%s'",
      MEDIA_FILE_STATUS_NEW, MEDIA_FILE_STATUS_ABORTED, MEDIA_FILE_STATUS_UNRESOLVED, strType.c_str());

  time_t now;
  time(&now);
  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::GetUnresolvedFolders, QUERY WRITE, update media_folders set strStatus");
  Dataset* pDS2 = Exec("update media_folders set strStatus='pending', iDateStatus=%i where (strStatus=='%s' OR strStatus=='%s' OR (strStatus=='%s' AND iNeedsRescan > 0)) AND strType=='%s'",
      now, MEDIA_FILE_STATUS_NEW, MEDIA_FILE_STATUS_ABORTED, MEDIA_FILE_STATUS_UNRESOLVED, strType.c_str());

  if (pDS2)
  {
    delete pDS2;
  }

  int iResult = MEDIA_DATABASE_OK;
  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof())
      {
        // In case of
        std::string strStatus = pDS->fv(0).get_asString();

        if (strStatus == MEDIA_FILE_STATUS_ABORTED) {
          // Check if the last status date is old enough
          time_t tDateStatus = pDS->fv(1).get_asInt();
          time_t now = time(NULL);

          if ((now - tDateStatus) < 1 * 60) {
            pDS->next();
            continue;
          }
        }

        pFolder = new BXFolder(strType);

        pFolder->SetPath(pDS->fv(2).get_asString());
        pFolder->SetType(pDS->fv(3).get_asString());
        pFolder->SetMediaFileId(pDS->fv(4).get_asInt());

        vecFolders.push_back(pFolder);

        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    delete pDS;
  }
  else {
    iResult =  MEDIA_DATABASE_ERROR;
  }

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Retrieved %d unresolved folders...", vecFolders.size());

  return iResult;
}

int BXMediaDatabase::GetUnresolvedVideoFolders(std::vector<BXMetadata*>& vecFolders, const std::vector<std::string>& vecPathFilter, int iItemLimit)
{

  BXFolder* pFolder;

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::GetUnresolvedFolders, QUERY, select * from media_folders");
  Dataset* pDS = Query("select strPath from media_folders where (strStatus<>'resolved' AND strType=='videoFolder')");

  int iResult = MEDIA_DATABASE_OK;
  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof() && iItemLimit > 0)
      {
        // In case of
        std::string strPath = pDS->fv(0).get_asString();

        if (!BXUtils::CheckPathFilter(vecPathFilter, strPath)) 
        {
          pDS->next();
          continue;
        }

        pFolder = new BXFolder(MEDIA_ITEM_TYPE_DIR);
        pFolder->SetPath(strPath);
        pFolder->SetType(MEDIA_ITEM_TYPE_DIR);

        vecFolders.push_back(pFolder);
        iItemLimit--;

        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    delete pDS;
  }
  else {
    iResult =  MEDIA_DATABASE_ERROR;
  }

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Retrieved %d unresolved folders...", vecFolders.size());

  return iResult;
}

int BXMediaDatabase::GetMediaFolders(std::vector<BXFolder*>& vecFolders, const std::string& strType)
{
  BXFolder* pFolder;

  Dataset* pDS;
  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::GetMediaFolders, QUERY, select * from media_folders");
  if (strType == "") {
    pDS = Query("select strPath,strType,idFolder from media_folders");
  }
  else {
    pDS = Query("select strPath,strType,idFolder from media_folders where strType='%s'", strType.c_str());
  }

  int iResult = MEDIA_DATABASE_OK;
  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      while (!pDS->eof())
      {
        // The type of folder should not matter for the callers of this function
        // TODO: Add generic folder type
        pFolder = new BXFolder(MEDIA_ITEM_TYPE_MUSIC_FOLDER);

        pFolder->SetPath(pDS->fv(0).get_asString());
        pFolder->SetType(pDS->fv(1).get_asString());
        pFolder->SetMediaFileId(pDS->fv(2).get_asInt());

        vecFolders.push_back(pFolder);

        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    delete pDS;
  }
  else {
    iResult =  MEDIA_DATABASE_ERROR;
  }

  //LOG(LOG_LEVEL_DEBUG, "LIBRARY: Retrieved %d unresolved folders...", vecFolders.size());

  return iResult;
}

bool BXMediaDatabase::ResetUnresolvedFiles()
{
  Dataset* pDS = Exec("update media_folders set strStatus='new', iDateStatus=%i, iNeedsRescan=%i where "
      "strStatus<>'resolved' AND strStatus<>'unresolved'", time(NULL), FOLDER_RETRY);

  if (pDS)
  {
    delete pDS;
    return true;
  }

  return false;
}

int BXMediaDatabase::ResetMetadata()
{

  LOG(LOG_LEVEL_ERROR, "!!! ERASE, All metadata will be ERASED !!!");

  Create("DELETE FROM audio_files");
  Create("DELETE FROM artists");
  Create("DELETE FROM albums");
  Create("DELETE FROM video_files");
  Create("DELETE FROM video_parts");
  Create("DELETE FROM actors");
  Create("DELETE FROM directors");

  time_t now = time(NULL);

  Create("update media_folders set strStatus='new', iDateStatus=%i, iHasMetadata=0", now);

  return MEDIA_DATABASE_OK;
}

int BXMediaDatabase::UpdateFolderNew(const std::string& strPath)
{
  time_t now = time(NULL);

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderNew, QUERY, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='new', iDateStatus=%i where strPath='%s'", now, strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

bool BXMediaDatabase::IsFolderBeingResolved(const std::string& strPath)
{
  Dataset* pDS = Query("select 1 from media_folders where strPath='%s' and (strStatus='resolving' or strStatus='pending' or strStatus='new' or strStatus='scanned')",strPath.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      delete pDS;
      return true;
    }
  }

  return false;
}

int BXMediaDatabase::UpdateFolderTreeNew(const std::string& strPath)
{
  time_t now = time(NULL);

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderNew, QUERY, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='new', iDateStatus=%i where strPath LIKE '%s%%'", now, strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::UpdateFolderResolving(const BXFolder* pFolder)
{
  time_t now = time(NULL);

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderResolving, QUERY WRITE, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='resolving', iDateStatus=%i where idFolder=%i", now, pFolder->GetMediaFileId());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::UpdateFolderUnresolved(const std::string& strPath , bool bDecreaseNeedsRescan /* = true */)
{
  std::string strDecreaseRescan = ", iNeedsRescan=iNeedsRescan-1";
  time_t now = time(NULL);

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderUnresolved, QUERY WRITE, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='unresolved', iDateStatus=%i %s where strPath='%s'", now, (bDecreaseNeedsRescan?strDecreaseRescan.c_str():""), strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::UpdateFolderAborted(const std::string& strPath)
{
  time_t now =time(NULL);


  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderAborted, QUERY WRITE, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='aborted', iDateStatus=%i where strPath='%s'", now, strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::UpdateFolderResolved(const std::string& strPath, int iMetadataId)
{
  time_t now = time(NULL);

  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateFolderResolved, QUERY WRITE, update media_folders");
  Dataset* pDS = Exec("update media_folders set strStatus='resolved', iDateStatus=%i, iMetadataId=%i, iHasMetadata=1 where strPath='%s'",
      now, iMetadataId,  strPath.c_str());
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::SetFolderMetadata(int iFolderId, int iMetadataId)
{
  //LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::SetFolderMetadata, QUERY WRITE, update media_folders");
  Dataset* pDS = Exec("update media_folders set iMetadataId=%d where idFolder=%d", iMetadataId, iFolderId);
  if (pDS)
  {
    delete pDS;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::GetChildFolders(std::set<std::string> &setFolders, const std::string &strPath , bool getImmediate)
{
  int iResult = MEDIA_DATABASE_ERROR;
  Dataset* pDS = Query("select strPath from media_folders where strPath LIKE '%s%%'", strPath.c_str());

  std::string strFolderPath = strPath;
  BXUtils::RemoveSlashAtEnd(strFolderPath);

  if (pDS) {
    if (pDS->num_rows() != 0) {
      while (!pDS->eof()) {

        std::string strPath = pDS->fv(0).get_asString();
        std::string strParentPath = BXUtils::GetParentPath(strPath);
        BXUtils::RemoveSlashAtEnd(strParentPath);

        // Take only the files that are immediately under the specified folder
        if (getImmediate && (strParentPath != strFolderPath)) {
          pDS->next();
          continue;
        }

        setFolders.insert(strPath);
        pDS->next();
      }
      iResult = MEDIA_DATABASE_OK;
    }
    delete pDS;
  }
  return iResult;
}

int BXMediaDatabase::GetFolderCount(const std::string &strSharePath, const std::string &strStatus)
{
  int iResult = MEDIA_DATABASE_ERROR;

  Dataset* pDS = NULL;
  if (strSharePath == "")
  {
    if (strStatus == "total") 
    {
      pDS = Query("select count(*) as \"folderCount\" from media_folders");
    }
    else
    {
      pDS = Query("select count(*) as \"folderCount\" from media_folders  where strStatus='%s' ", strStatus.c_str());
    }
  }
  else
  {
    if (strStatus == "total") 
    {
      pDS = Query("select count(*) as \"folderCount\" from media_folders where strPath LIKE '%s%%'", strSharePath.c_str());
    }
    else
    {
      pDS = Query("select count(*) as \"folderCount\" from media_folders  where strStatus='%s' and strPath LIKE '%s%%'", strStatus.c_str(), strSharePath.c_str());
    }
  }


  if (pDS) {
    if (pDS->num_rows() != 0) {

      iResult = pDS->fv("folderCount").get_asInt();
    }
    delete pDS;
  }
  return iResult;
}

int BXMediaDatabase::UpdateFolderAvailability(const std::string& strPath, bool bAvailable)
{
  int iResult = MEDIA_DATABASE_ERROR;

  time_t now;
  time(&now);

  Dataset* pDS = NULL;
  if (bAvailable)
  {
    pDS = Exec("update media_folders set strStatus='new', iDateStatus=%i where strStatus<>'resolved' and strPath LIKE '%s%%'", now, strPath.c_str());
  }
  else
  {
    pDS = Exec("update media_folders set strStatus='unavailable', iDateStatus=%i where strStatus<>'resolved' and strPath LIKE '%s%%'", now, strPath.c_str());
  }

  if (pDS) {

    iResult = MEDIA_DATABASE_OK;

    delete pDS;
  }
  return iResult;
}

int BXMediaDatabase::RemoveFolderByPath(const std::string& strPath)
{
  LOG(LOG_LEVEL_DEBUG, "LIBRARY, !!! ERASE folder, path = %s", strPath.c_str());
  Dataset* pDS_t = Exec("delete from media_folders where strPath like '%s%%'", strPath.c_str());
  if (pDS_t) {
    delete pDS_t;
    return MEDIA_DATABASE_OK;
  }
  return MEDIA_DATABASE_ERROR;
}

bool BXMediaDatabase::AddQueueItem(const std::string& strLabel, const std::string& strPath, const std::string& strThumbPath, const std::string& strClientId)
{
  LOG(LOG_LEVEL_DEBUG,"BXMediaDatabase::AddQueueItem - Enter function with [strLabel=%s][strpath=%s][strThumbPath=%s][clientId=%s] (queue)", strLabel.c_str(), strPath.c_str(), strThumbPath.c_str(), strClientId.c_str());

  Dataset* pDS = Query("select strClientId from queue where strPath='%s'", strPath.c_str());

  LOG(LOG_LEVEL_DEBUG,"BXMediaDatabase::AddQueueItem - Query for [path=%s] returned [pDS=%p] (queue)", strPath.c_str(), pDS);

  if (pDS)
  {
    if ((pDS->num_rows() > 0))
    {
    LOG(LOG_LEVEL_ERROR, "BXMediaDatabase::AddQueueItem - Item already exists [rows=%d]. [path=%s][clientId=%s] (queue)", pDS->num_rows(), strPath.c_str(), strClientId.c_str());
    delete pDS;
    return false;
  }
    delete pDS;
  }

  int iResult = Insert("insert into queue (idQueue, strLabel, strPath, strThumb, strClientId) "
        "values (NULL, '%s', '%s', '%s', '%s')", strLabel.c_str(), strPath.c_str(), strThumbPath.c_str(), strClientId.c_str());

  bool retVal = false;

  if (iResult != MEDIA_DATABASE_ERROR)
  {
    retVal = true;
  }
  else
  {
    retVal = false;
  }

  LOG(LOG_LEVEL_DEBUG,"BXMediaDatabase::AddQueueItem - Add item [strLabel=%s][strpath=%s][strThumbPath=%s][clientId=%s] to queue DB returned [%d]. Exit function and return [retVal=%d] (queue)", strLabel.c_str(), strPath.c_str(), strThumbPath.c_str(), strClientId.c_str(), iResult, retVal);

  return retVal;
}

bool BXMediaDatabase::GetQueueItem(const std::string& strClientId, std::string& strLabel, std::string& strPath, std::string& strThumbPath)
{
  if (strClientId.empty())
  {
    return false;
  }

  bool succeeded = false;

  Dataset* pDS = Query("select strLabel, strPath, strThumb from queue where strClientId='%s'", strClientId.c_str());

  if (pDS)
  {
    if (pDS->num_rows() == 1)
    {
      strLabel = pDS->fv(0).get_asString();
      strPath = pDS->fv(1).get_asString();
      strThumbPath = pDS->fv(2).get_asString();

      succeeded = true;
    }
    else
    {
      LOG(LOG_LEVEL_ERROR, "BXMediaDatabase::GetQueueItem - For [clientId=%s] there are [rows=%d] in DB (queue)", strClientId.c_str(), pDS->num_rows());
    }

    delete pDS;
  }
  else
  {
    LOG(LOG_LEVEL_ERROR, "BXMediaDatabase::GetQueueItem - FAILED to find entry for [clientId=%s] in DB (queue)", strClientId.c_str());
  }

  return succeeded;
}

bool BXMediaDatabase::RemoveQueueItem(const std::string& strClientId)
{
  if (strClientId.empty())
  {
    return false;
  }

  Dataset* pDS = Exec("delete from queue where strClientId='%s'", strClientId.c_str());

  if (pDS)
  {
    delete pDS;
    return true;
  }
  else
  {
    return false;
  }
}

bool BXMediaDatabase::AddMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType)
{
  Dataset* pDS = Query("select strLabel from media_shares where strLabel='%s' and strPath='%s' and strType='%s'",
      strLabel.c_str(), strPath.c_str(), strType.c_str());

  if (pDS)
  {
    if (pDS->num_rows() > 0)
    {
      delete pDS;
      return false;
    }

    delete pDS;
  }

  int iResult = Insert("insert into media_shares (idShare, strLabel, strPath, strType, iScanType, iLastScanned) "
      "values (NULL, '%s', '%s', '%s', %i, %i)", strLabel.c_str(), strPath.c_str(), strType.c_str(), iScanType, 0);

  if (iResult == MEDIA_DATABASE_ERROR)
  {
    return false;
  }

  return true;
}

bool BXMediaDatabase::UpdateMediaShare(const std::string& strOrgLabel, const std::string& strOrgType, const std::string& strLabel, const std::string& strPath, const std::string& strType, int iScanType)
{
  Dataset* pDS = Exec("update media_shares set strLabel='%s', strPath='%s', strType='%s',iScanType=%i where strLabel='%s' and strType='%s'",
      strLabel.c_str(), strPath.c_str(), strType.c_str(), iScanType, strOrgLabel.c_str(), strOrgType.c_str());

  if (pDS)
  {
    delete pDS;
    return true;
  }

  return false;
}

bool BXMediaDatabase::DeleteMediaShare(const std::string& strLabel, const std::string& strPath, const std::string& strType)
{
  Dataset* pDS = Exec("delete from media_shares where strLabel='%s' and strPath='%s' and strType='%s'",
      strLabel.c_str(), strPath.c_str(), strType.c_str());

  if (pDS)
  {
    delete pDS;
    return true;
  }

  return false;
}

bool BXMediaDatabase::GetScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t& iLastScanned)
{
  bool bResult = false;

  Dataset* pDS = Query("select iLastScanned from media_shares where strLabel='%s' and strPath='%s' and strType='%s'",
      strLabel.c_str(), strPath.c_str(), strType.c_str());

  if (pDS)
  {
    if (pDS->num_rows() == 1)
    {
      iLastScanned = pDS->fv(0).get_asInt();
      bResult = true;
    }

    delete pDS;
  }
  return bResult;
}

bool BXMediaDatabase::UpdateScanTime(const std::string& strLabel, const std::string& strPath, const std::string& strType, time_t iLastScanned)
{
  Dataset* pDS = Exec("update media_shares set iLastScanned=%i where strLabel='%s' and strPath='%s' and strType='%s'",
      iLastScanned, strLabel.c_str(), strPath.c_str(), strType.c_str());

  if (pDS)
  {
    delete pDS;
    return true;
  }

  return false;
}


unsigned int BXMediaDatabase::GetFolderDate(const std::string& strPath)
{
  unsigned int date = 0;

  Dataset* pDS = Query("select iDateModified from media_folders where strPath='%s'", strPath.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      date = pDS->fv(0).get_asUInt();
    }

    delete pDS;
  }
  return date;
}


int BXMediaDatabase::AddProviderPerf(const std::string& strProvider, int quality)
{
  LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::AddProviderPerf save quality [%d] for provider [%s]", quality, strProvider.c_str());

  int iID = Insert("insert into provider_preferences values( NULL,'%s', %d )", strProvider.c_str(), quality);
  if (iID == MEDIA_DATABASE_ERROR) {
    LOG(LOG_LEVEL_ERROR, "Could not add quality : [%d]  for provider [%s]", quality, strProvider.c_str());
  }
  return iID;

}
int BXMediaDatabase::UpdateProviderPerf(const std::string& strProvider, int quality)
{
  LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::UpdateProviderPerf save quality [%d] for provider [%s]", quality, strProvider.c_str());

  Dataset* pDS = Query("select * from provider_preferences where iProvider = '%s'",strProvider.c_str());

  int iID = MEDIA_DATABASE_OK;

  bool bShouldUpdate = false;

  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      bShouldUpdate = true;
  }
    delete pDS;
  }

  if (bShouldUpdate)
  {
    Dataset* pDS1 = Exec("update provider_preferences set iSelectedQuality = %d where iProvider = '%s'",quality, strProvider.c_str());
	if (pDS1)
	{
	 delete pDS1;
}
	 iID = MEDIA_DATABASE_ERROR;
  }
  else
  {
    iID  = AddProviderPerf(strProvider,quality);
  }

  return iID;
}

int BXMediaDatabase::GetProviderPerfQuality(const std::string& strProvider)
{
  Dataset* pDS = Query("select iSelectedQuality from provider_preferences where iProvider = '%s'", strProvider.c_str());
  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      // Get the id of the video
      int iQuality = pDS->fv("iSelectedQuality").get_asInt();
      delete pDS;

      return iQuality;
    }
  }
  return MEDIA_DATABASE_ERROR;
}

int BXMediaDatabase::AddPlayableFolder(const std::string& strPath)
{
  LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::AddPlayableFolder path [%s]", strPath.c_str());

  int iID = Insert("insert into playable_folders values( NULL,'%s' )", strPath.c_str());
  if (iID == MEDIA_DATABASE_ERROR) {
    LOG(LOG_LEVEL_ERROR, "Could not add playable folder : [%s]", strPath.c_str());
  }
  return iID;
}

bool BXMediaDatabase::IsPlayableFolder(const std::string& strPath)
{
  Dataset* pDS = Query("select iFolder from playable_folders where iFolder = '%s'", strPath.c_str());

  bool bSuccedded = false;

  if (pDS)
  {
    if (pDS->num_rows() == 1 )
    {
      CStdString strFolder = pDS->fv(0).get_asString();
  
      if(strPath == strFolder)
       bSuccedded = true;

      delete pDS;
    }
  }
  return bSuccedded;
  }
} // namespace BOXEE

