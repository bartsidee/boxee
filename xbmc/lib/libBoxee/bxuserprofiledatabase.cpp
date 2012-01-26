#include "bxuserprofiledatabase.h"
bool BOXEE::BXUserProfileDatabase::m_bTableCheck = false;

using namespace dbiplus;

namespace BOXEE
{
  BXUserProfileDatabase::BXUserProfileDatabase(void)
  {
    mDatabaseFilePath = _P("special://profile/Database/") + "boxee_user_catalog.db";
    Open();

    if (!m_bTableCheck && !TablesExist() && CreateTables()) //ensure it is done only once
          CopyDataFromMediaDatabase();

    m_bTableCheck = true;
	}

  BXUserProfileDatabase::~BXUserProfileDatabase(void)
  {
  }

  bool BXUserProfileDatabase::TablesExist()
  {
    bool bExists;
    LOG(LOG_LEVEL_DEBUG, "File exists, check if tables are created");

    // Check if the version table can be accessed
    Dataset* pDS = Query("select distinct 1 from sqlite_master where tbl_name='watched'");

    if (pDS && pDS->num_rows() > 0) 
      bExists = true;
    else 
      bExists = false;

    if (pDS)
      delete pDS;

    return bExists;
  }

	
  void BXUserProfileDatabase::MarkAsWatched(const std::string& strPath, const std::string& strBoxeeId, double iLastPosition)
  {
    LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::MarkAsWatched, path = %s, boxeeid = %d (watched)", strPath.c_str(), strBoxeeId.c_str());
    Dataset* pDS = Query("select 1 from watched where strPath='%s' and strBoxeeId='%s'", strPath.c_str(), strBoxeeId.c_str());
    if (pDS)
    {
      time_t now = time(NULL);

      if (pDS->num_rows() != 0)
      {
        LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::MarkAsWatched, video already exists, path = %s, boxeeid = %s (watched)", strPath.c_str(), strBoxeeId.c_str());
        Dataset* pDS2 = Exec("update watched set iPlayCount=iPlayCount+1, iLastPlayed=%i, fPositionInSeconds=%f where  strPath='%s' and strBoxeeId='%s'",
        now, iLastPosition, strPath.c_str(), strBoxeeId.c_str());
        if (pDS2)
        {
          delete pDS2;
        }
      }
      else
      {
        LOG(LOG_LEVEL_DEBUG, "BXMediaDatabase::MarkAsWatched, add new watched, path = %s, boxeeid = %s (watched)", strPath.c_str(), strBoxeeId.c_str());
        Insert("insert into watched (idWatched, strPath, strBoxeeId, iPlayCount, iLastPlayed, fPositionInSeconds) "
        "values (NULL, '%s', '%s', %i, %i, %f)", strPath.c_str(), strBoxeeId.c_str(), 1, now, iLastPosition);
      }
      delete pDS;
    }
  }

  void BXUserProfileDatabase::MarkAsUnWatched(const std::string& strPath, const std::string& strBoxeeId)
  {
    if (!strBoxeeId.empty())
    {
      // if boxee id is not empty remove all entries with this id
      Dataset* pDS = Exec("delete from watched where strBoxeeId='%s'", strBoxeeId.c_str());
      if (pDS) {
        delete pDS;
      }
    }
    else
    {
      // otherwise delete entry by provided path
      Dataset* pDS = Exec("delete from watched where strPath='%s'", strPath.c_str());
      if (pDS)
      {
        delete pDS;
      }
    }
  }



  bool BXUserProfileDatabase::GetTimeWatchedByPath(const std::string& strPath, double& out_Time )
  {
    if (strPath.empty())
      return false;
    bool bWatched = false;

    Dataset* pDS = Query("select fPositionInSeconds from watched where strPath='%s'", strPath.c_str());
    if (pDS)
    {
      if (pDS->num_rows() != 0)
      {
        out_Time = pDS->fv("fPositionInSeconds").get_asDouble();
        bWatched = true;
      }
      delete pDS;
    }
    return bWatched;
  }

  bool BXUserProfileDatabase::IsWatchedById(const std::string& strBoxeeId , double& out_Time )
  {
    if (strBoxeeId.empty())
    return false;
    bool bWatched = false;
    Dataset* pDS = Query("select fPositionInSeconds from watched where strBoxeeId='%s'", strBoxeeId.c_str());
    if (pDS)
    {
      if (pDS->num_rows() != 0)
      {
        out_Time = pDS->fv("fPositionInSeconds").get_asDouble();
        bWatched = true;
      }
      delete pDS;
    }

    return bWatched;
  }

  bool BXUserProfileDatabase::IsWatchedByPath(const std::string& strPath)
  {
    if (strPath.empty())
    return false;
    bool bWatched = false;
    Dataset* pDS = Query("select 1 from watched where strPath='%s'", strPath.c_str());

      if (pDS)
      {
        if (pDS->num_rows() != 0)
        {
          bWatched = true;
        }
        delete pDS;
      }

      return bWatched;
  }

  bool BXUserProfileDatabase::IsWatchedById(const std::string& strBoxeeId)
  {
    if (strBoxeeId.empty())
    return false;
    bool bWatched = false;
    Dataset* pDS = Query("select 1 from watched where strBoxeeId='%s'", strBoxeeId.c_str());
      if (pDS)
      {
        if (pDS->num_rows() != 0)
        {
          bWatched = true;
        }
      delete pDS;
      }

    return bWatched;
  }

  bool BXUserProfileDatabase::CopyDataFromMediaDatabase()
  {
    bool retVal = true;
    BXDatabase bxd;

    LOG(LOG_LEVEL_INFO, "LIBRARY: Copying watched entries from boxee_catalog.db");		

    //take the info from the old database
    bxd.mDatabaseFilePath = BOXEE::Boxee::GetInstance().getDatabaseFolderPath() + "boxee_catalog.db";
    //connect to the old db

    if (bxd.Open())
    {
      //take all the data from watched
      Dataset* pDS = bxd.Query("select * from watched");

      LOG(LOG_LEVEL_DEBUG, "Migrating to new profile db..");
      if (pDS)
      {
        LOG(LOG_LEVEL_DEBUG, "Watch data..");

        if (pDS->num_rows() > 0)
        { //copy to the new db
          for (int i = 0 ; i < pDS->num_rows() ; i++)
          {

            LOG(LOG_LEVEL_DEBUG, "path = %s, boxeeid = %s (watched)", pDS->fv("strPath").get_asString().c_str() , pDS->fv("strBoxeeId").get_asString().c_str());

            try
            {
              Insert(	"insert into watched (idWatched, strPath, strBoxeeId, iPlayCount, iLastPlayed, fPositionInSeconds) "
	            "values (NULL, '%s', '%s', %i, %i, %f)", pDS->fv("strPath").get_asString().c_str(), pDS->fv("strBoxeeId").get_asString().c_str(), 
	            pDS->fv("iPlayCount").get_asInt(), pDS->fv("iLastPlayed").get_asInt(), pDS->fv("fPositionInSeconds").get_asDouble());
            }
            catch(dbiplus::DbErrors& e) 
            {
              LOG(LOG_LEVEL_ERROR, "BOXEE DATABASE, Could not migrate user data to new location.");
              retVal = false;
            }
          }
        }
        else
        {
          retVal = false;
        }
          delete pDS;
      }
      else
      {
        retVal = false;
      }
    }
    //if nothing was copied / table was not found - this will return false.
    return retVal;
  }


  bool BXUserProfileDatabase::CreateTables()
  {
    bool result = true;
    try
    {
      // Creating tables for watched feature
      LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating watched table (watched)");
      result &= Create("CREATE TABLE watched ( idWatched integer primary key autoincrement, strPath text, strBoxeeId text, iPlayCount integer, iLastPlayed integer, fPositionInSeconds double);");
      result &= Create("CREATE INDEX watched_srtPath_idx ON watched(strPath);\n");
      result &= Create("CREATE INDEX watched_strBoxeeId_idx ON watched(strBoxeeId);\n");
    }
    catch(dbiplus::DbErrors& e) 
    {
      LOG(LOG_LEVEL_ERROR, e.getMsg());
      result = false;
    }
    catch(...) 
    {
      LOG(LOG_LEVEL_ERROR, "Could not create tables, exception was caught during operation");
      result = false;
    }

    return result;
  }

}