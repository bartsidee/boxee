#include "GUIWindowStateDatabase.h"
#include "SpecialProtocol.h"
#include "logger.h"
#include "utils/log.h"

using namespace dbiplus;

CGUIWindowStateDatabase::CGUIWindowStateDatabase()
{
  mDatabaseFilePath = _P("special://profile/Database/") + "boxee_window_state.db";
  Open();
}

CGUIWindowStateDatabase::~CGUIWindowStateDatabase()
{
  Close();
}


bool CGUIWindowStateDatabase::Init()
{
  if (!TablesExist())
    return CreateTables();
  else
    return UpdateTables();
}

bool CGUIWindowStateDatabase::DropTables()
{
  LOG(LOG_LEVEL_DEBUG, "LIBRARY: deleting windowstate table (windowstate)");

  bool retVal = Create("DROP table windowstate;");

  return retVal;
}

bool CGUIWindowStateDatabase::TablesExist()
{
  bool bExists = BXDatabase::TablesExist();

  if (bExists)
  {
    LOG(LOG_LEVEL_DEBUG, "File exists, check if tables are created");

    // Check if the version table can be accessed
    Dataset* pDS = Query("select distinct 1 from sqlite_master where tbl_name='windowstate'");

    if (pDS && pDS->num_rows() > 0)
      bExists = true;
    else
      bExists = false;

    if (pDS)
      delete pDS;
  }
  return bExists;
}

bool CGUIWindowStateDatabase::CreateTables()
{
  bool result = BXDatabase::CreateTables();

  if (result)
  {
    try
    {
      result = DropTables();
      if (result)
      {
        LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating windowstate table (windowstate)");
        result = Create("CREATE TABLE windowstate ( idWindow integer unsigned NOT NULL, strCategory varchar(20) NOT NULL, idView smallint unsigned , strSort varchar(20), iSelectedItem integer unsigned, PRIMARY KEY(idWindow, strCategory) )");

        result = Create("CREATE TABLE windowdefaults (idWindow integer unsigned primary key, strDefaultCategory varchar(20) NOT NULL)");

        LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating userdefaults table (userdefaults)");
        result = Create("CREATE TABLE userdefaults (id integer primary key AUTOINCREMENT, key varchar(20) NOT NULL, value varchar(20) NOT NULL)");

        LOG(LOG_LEVEL_DEBUG, "LIBRARY: Creating userServices table (userServices)");
        result = Create("CREATE TABLE userServices(id integer primary key AUTOINCREMENT, serviceId varchar(20) NOT NULL, serviceName varchar(20) NOT NULL, sendShare integer NOT NULL, unique (serviceId)) ");

        Dataset* pDS = Exec("update version set iVersion=11");
        if (pDS)
        {
          delete pDS;
        }
      }
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
  }

  return result;
}

bool CGUIWindowStateDatabase::SaveState(int iWindow, const std::string& strSelectedCategory, int iView, const std::string& strSortMethod, int iSelectedItem)
{
  LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SaveState, iWindow=%d , iView=%d , strSortMethod=%s , iSelectedItem=%d, iSelectedCategory=%s",iWindow,iView,strSortMethod.c_str(),iSelectedItem,strSelectedCategory.c_str());

  //lets check if there is such record
  Dataset* pDS = Query("select 1 from windowstate where (idWindow='%d' AND strCategory='%s')", iWindow , strSelectedCategory.c_str());
  if (pDS)
  {//query succeeded 
    if (pDS->num_rows() != 0)
    {
      //a record found
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SaveState, state found, idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());
      //update that record
      Dataset* pDS2 = Exec("update windowstate set idView='%d', strSort='%s', iSelectedItem='%d' where (idWindow='%d' AND strCategory='%s')",
                                                   iView, strSortMethod.c_str(), iSelectedItem, iWindow, strSelectedCategory.c_str());\
      if (pDS2)
      {
        delete pDS2;
      }
      else
      {
        LOG(LOG_LEVEL_DEBUG,"GUIWindowStateDatabase error: %s",GetLastErrorMessage());
      }
    }
    else
    { 
      //no such record, create new
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SaveState, add new state, idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());

      Insert("insert into windowstate (idWindow, strCategory, idView, strSort, iSelectedItem) "
      "values ('%d', '%s', %d, '%s', '%d')", iWindow, strSelectedCategory.c_str(), iView, strSortMethod.c_str(), iSelectedItem);
    }
    delete pDS;
    return true;
  }

  return false;

}

bool CGUIWindowStateDatabase::LoadState(int iWindow , const std::string& strSelectedCategory, int& iView, std::string& strSortMethod, int& iSelectedItem)
{
  bool bRetVal = false;

  //lets check if there is such record
  Dataset* pDS = Query("select * from windowstate where (idWindow='%d' AND strCategory='%s')", iWindow , strSelectedCategory.c_str());
 
  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::LoadState, loading state for idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());

      iView = pDS->fv("idView").get_asInt();
      strSortMethod = pDS->fv("strSort").get_asString();
      iSelectedItem = pDS->fv("iSelectedItem").get_asInt();

      bRetVal = true;  
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::LoadState, no state found for idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());
    }

    delete pDS;
  }

  if (bRetVal)
  {
    LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::LoadState, iWindow=%d , iView=%d , strSortMethod=%s, iSelectedItem=%d , strSelectedCategory=%s",iWindow,iView,strSortMethod.c_str(),iSelectedItem,strSelectedCategory.c_str());
  }

  return bRetVal;
}


bool CGUIWindowStateDatabase::SetDefaultCategory(int iWindow, const std::string& strSelectedCategory)
{
  LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetDefaultCategory, iWindow=%d , strSelectedCategory=%s",iWindow,strSelectedCategory.c_str());

  //lets check if there is such record
  Dataset* pDS = Query("select 1 from windowdefaults where idWindow='%d'", iWindow);
  if (pDS)
  {//query succeeded
    if (pDS->num_rows() != 0)
    {
      //a record found
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetDefaultCategory, state found, idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());
      //update that record
      Dataset* pDS2 = Exec("update windowdefaults set strDefaultCategory='%s' where (idWindow='%d')", strSelectedCategory.c_str(), iWindow);
      if (pDS2)
      {
        delete pDS2;
      }
    }
    else
    {
      //no such record, create new
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetDefaultCategory, add new state, idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());

      Insert("insert into windowdefaults (idWindow, strDefaultCategory) values ('%d', '%s')", iWindow, strSelectedCategory.c_str());
    }
    delete pDS;
    return true;
  }

  return false;

}

bool CGUIWindowStateDatabase::GetDefaultCategory(int iWindow, std::string& strSelectedCategory)
{
  bool bRetVal = false;
  LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetDefaultCategory, iWindow=%d",iWindow);

  //lets check if there is such record
  Dataset* pDS = Query("select * from windowdefaults where (idWindow=%d)", iWindow);

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetDefaultCategory, state found, idWindow='%d' , strCategory='%s'", iWindow, strSelectedCategory.c_str());

      strSelectedCategory = pDS->fv("strDefaultCategory").get_asString();

      bRetVal = true;
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetDefaultCategory, state not found, idWindow='%d', strCategory='%s'", iWindow, strSelectedCategory.c_str());
    }

    delete pDS;
  }

  return bRetVal;
}

bool CGUIWindowStateDatabase::GetUserEnabledServices(std::vector<std::string>& vecEnabledServices)
{
  Dataset* pDS = Query("select serviceId from userServices where sendShare='1'");

  if (pDS)
  {
    while(!pDS->eof())
    {
      std::string serviceId = pDS->fv(0).get_asString();

      vecEnabledServices.push_back(serviceId);

      pDS->next();
    }

    LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::GetUserEnabledServices, returned %d records.",pDS->num_rows());

    delete pDS;
    return true;
  }

  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::GetUserEnabledServices, failed.",pDS->num_rows());

  return false;
}

bool CGUIWindowStateDatabase::GetUserService(const std::string& serviceId, std::string& serviceName, bool& sendShare)
{
  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::GetUserService, [serviceId=%s]",serviceId.c_str());

  Dataset* pDS = Query("select * from userServices where serviceId='%s'", serviceId.c_str());
  if (pDS)
  {//query succeeded
    if (pDS->num_rows() != 0)
    {
      serviceName = pDS->fv("serviceName").get_asString();
      sendShare = pDS->fv("sendShare").get_asBool();
      LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::GetUserService, [serviceId=%s][serviceName=%s][sendShare=%d]",serviceId.c_str(),serviceName.c_str(),(int)sendShare);
      return true;
    }
    delete pDS;
    return false;
  }

  return false;
}

bool CGUIWindowStateDatabase::SetUserServiceState(const std::string& serviceId, bool sendShare)
{
  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::SetUserServiceState, [serviceId=%s][sendShare=%d]",serviceId.c_str(),(int)sendShare);
  Dataset* pDS2 = Exec("update userServices set sendShare='%d' where serviceId='%s'", (int)sendShare, serviceId.c_str() );

  if (pDS2)
  {
    delete pDS2;
    LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::SetUserServiceState, succeeded[serviceId=%s]",serviceId.c_str());
    return true;
  }

  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::SetUserServiceState, failed[serviceId=%s]",serviceId.c_str());
  return false;
}

bool CGUIWindowStateDatabase::RemoveUserService(const std::string& serviceId)
{
  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::RemoveUserService, [serviceId=%s]",serviceId.c_str());

  Dataset* pDS2 = Exec("delete from userServices where serviceId='%s'", serviceId.c_str() );

  if (pDS2)
  {
    delete pDS2;
    LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::RemoveUserService, succeeded[serviceId=%s]",serviceId.c_str());
    return true;
  }

  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::RemoveUserService, failed[serviceId=%s]",serviceId.c_str());
  return false;
}

bool CGUIWindowStateDatabase::AddUserService(const std::string& serviceId, const std::string& serviceName, bool sendShare)
{
  LOG(LOG_LEVEL_DEBUG, "CGUIWindowStateDatabase::AddUserService, [serviceId=%s][serviceName=%s][sendShare=%d]",serviceId.c_str(),serviceName.c_str(),(int)sendShare);

  int iId = Insert("insert into userServices(id, serviceId, serviceName, sendShare) values (NULL, '%s','%s','%d')",serviceId.c_str(),serviceName.c_str(),(int)sendShare);

  if (iId != MEDIA_DATABASE_ERROR)
  {//query succeeded
    return true;
  }

  return false;
}

bool CGUIWindowStateDatabase::SetUserSetting(const std::string& strKey, const std::string& strValue)
{
  LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetUserSetting, strKey=%s , strValue=%s",strKey.c_str(),strValue.c_str());

  //lets check if there is such record
  Dataset* pDS = Query("select * from userdefaults where key='%s'", strKey.c_str());
  if (pDS)
  {//query succeeded
    if (pDS->num_rows() != 0)
    {
      //a record found
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetUserSetting, change setting strKey='%s' to strValue='%s'", strKey.c_str(), strValue.c_str());
      //update that record
      Dataset* pDS2 = Exec("update userdefaults set value='%s' where key='%s'", strValue.c_str(), strKey.c_str());
      if (pDS2)
      {
        delete pDS2;
      }
    }
    else
    {
      //no such record, create new
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::SetUserSetting, add new setting strKey='%s' == strValue='%s'", strKey.c_str(), strValue.c_str());

      Insert("insert into userdefaults ('key', 'value') values ('%s', '%s')", strKey.c_str(), strValue.c_str());
    }
    delete pDS;
    return true;
  }

  return false;

}

bool CGUIWindowStateDatabase::GetUserSetting(const std::string& strKey, std::string& strValue)
{
  bool bRetVal = false;
  LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetUserSetting, strKey =",strKey.c_str());

  //lets check if there is such record
  Dataset* pDS = Query("select * from userdefaults where key='%s'", strKey.c_str());

  if (pDS)
  {
    if (pDS->num_rows() != 0)
    {
      strValue = pDS->fv("value").get_asString();

      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetUserSetting, setting found, strKey='%s', strValue='%s'", strKey.c_str(), strValue.c_str());

      bRetVal = true;
    }
    else
    {
      LOG(LOG_LEVEL_DEBUG, "GUIWindowStateDatabase::GetUserSetting, setting not found, strKey='%s'", strKey.c_str());
    }

    delete pDS;
  }

  return bRetVal;
}


bool CGUIWindowStateDatabase::UpdateTables()
{
  bool bRetVal = false;

  Dataset* pDS = Query ("select * from version");

  if (!pDS)
  {//that means its the first pre mature version of the db - we should delete all tables (because they are still unused) and recreate them properly
    CreateTables();
  }
  else
  {
    if (pDS->num_rows() > 0)
    {
      int iVersion = pDS->fv("iVersion").get_asInt();

      if (iVersion <= 7)
      {
        bRetVal = Create("CREATE TABLE windowdefaults (idWindow integer unsigned primary key, strDefaultCategory varchar(20) NOT NULL)");

        if (bRetVal)
        {
          Dataset* pDS2 = Exec("update version set iVersion=8");
          if (pDS2)
          {
            delete pDS2;
          }
        }
      }
      if (iVersion <= 8)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowStateDatabase, %s is in version %d.", mDatabaseFilePath.c_str(), iVersion);

        bRetVal = Create("CREATE TABLE userdefaults (id integer primary key AUTOINCREMENT, key varchar(20) NOT NULL, value varchar(20) NOT NULL)");

        if (bRetVal)
        {
          CLog::Log(LOGDEBUG,"CGUIWindowStateDatabase, Advancing to 10 because of previous error with version 9");
          Dataset* pDS2 = Exec("update version set iVersion=10");
          if (pDS2)
          {
            delete pDS2;
          }
        }
      }
      if (iVersion <= 9)
      {

        CLog::Log(LOGDEBUG,"CGUIWindowStateDatabase, %s is in version %d", mDatabaseFilePath.c_str(), iVersion);

        Dataset* pDS3 = Query ("select * from userdefaults");

        if(!pDS3)
        {
          bRetVal = Create("CREATE TABLE userdefaults (id integer primary key AUTOINCREMENT, key varchar(20) NOT NULL, value varchar(20) NOT NULL)");

          if (bRetVal)
          {
            Dataset* pDS2 = Exec("update version set iVersion=10");
            if (pDS2)
            {
              delete pDS2;
            }
          }
        }
        else
        {
          Dataset* pDS2 = Exec("update version set iVersion=10");
          if (pDS2)
          {
            delete pDS2;
          }
          delete pDS3;
        }
      }
      if (iVersion <= 10)
      {
        bRetVal = Create("CREATE TABLE userServices(id integer primary key AUTOINCREMENT, serviceId varchar(20) NOT NULL, serviceName varchar(20) NOT NULL, sendShare integer NOT NULL, unique (serviceId)) ");

        if (bRetVal)
        {
          Dataset* pDS2 = Exec("update version set iVersion=11");
          if (pDS2)
          {
            delete pDS2;
          }
        }
      }
      if (iVersion == 11)
      {
        CLog::Log(LOGDEBUG,"CGUIWindowStateDatabase, %s is in version %d", mDatabaseFilePath.c_str(), iVersion);
      }
    }
    delete pDS;
  }

  return bRetVal;
}
