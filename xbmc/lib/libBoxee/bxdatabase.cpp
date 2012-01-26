#include "bxdatabase.h"
#include "bxutils.h"
#include "logger.h"
#include "boxee.h"

#include <sys/types.h>
#include <errno.h>
#include <time.h>

#include "SqliteConnectionPoolObject.h"
#include "Application.h"

using namespace dbiplus;

namespace BOXEE {

BXDatabase::BXDatabase() : mDatabaseFilePath("")
{
  m_bOpen = false;
  m_pDatabase = NULL;
  m_pDBPoolObject = NULL;
  mDatabaseFilePath = BOXEE::Boxee::GetInstance().getDatabaseFolderPath() + "boxee_catalog.db";
}

BXDatabase::~BXDatabase() {
  Close();
}

int BXDatabase::GetLastErrorCode()
{
  //return m_pDatabase->last_err;
  return sqlite3_errcode(m_pDatabase->getHandle());
}

const char* BXDatabase::GetLastErrorMessage()
{
  return sqlite3_errmsg(m_pDatabase->getHandle());
}

bool BXDatabase::Open()
{
  std::string databaseFilePath = "";

  if (m_pDatabase)
    Close();
  
  try
  {
    databaseFilePath = getDatabaseFilePath();
    m_pDBPoolObject = g_application.GetDBConnectionPool()->GetSqliteConnectionPoolObject( databaseFilePath );  
    m_pDatabase = m_pDBPoolObject->GetDatabase();

    if ( !m_pDatabase )
    {
      LOG(LOG_LEVEL_ERROR, "ERROR: Could not get valid database from pool. file [%s]\n", databaseFilePath.c_str());
      Close();
      return false;
    }
  }
  catch(...)
  {
    LOG(LOG_LEVEL_ERROR, "Error when opening database file [%s], error %s\n", databaseFilePath.c_str(), GetLastErrorMessage());
    return false;
  }

  m_bOpen = true;

  return true;
}

bool BXDatabase::CreateTables()
{
  LOG(LOG_LEVEL_DEBUG, "BXDatabase::Creating tables");
  bool bResult = true;
  bResult &= Create("PRAGMA cache_size=50000\n");
  bResult &= Create("PRAGMA default_cache_size=50000\n");
  bResult &= Create("PRAGMA encoding=\"UTF-8\"\n");
  bResult &= Create("PRAGMA case_sensitive_like=0\n");
  bResult &= Create("PRAGMA synchronous=NORMAL\n");
  bResult &= Create("PRAGMA temp_store=MEMORY\n");

  LOG(LOG_LEVEL_DEBUG, "Creating version table");
  const char* temp = "CREATE TABLE version (id integer primary key autoincrement, iVersion integer)\n";
  bResult &= Create(temp);

  Insert("insert into version (id, iVersion) values (NULL, 0)");

  return bResult;
}

dbiplus::Dataset* BXDatabase::Query(const char* query, ...)
{
  va_list args;
  va_start (args, query);
  Dataset* pDS = QuotedQuery(true, query, args);
  va_end (args);
  return pDS;
}
dbiplus::Dataset* BXDatabase::UnQuotedQuery(const char* query, ...)
{
  va_list args;
  va_start (args, query);
  Dataset* pDS = QuotedQuery(false, query, args);
  va_end (args);
  return pDS;
}
dbiplus::Dataset* BXDatabase::QuotedQuery(bool shouldQuote, const char* query,  va_list argList)
{
  if (!IsOpen()) {
    return NULL;
  }
  
  std::string strQuery = query;
  if (shouldQuote)
  BXUtils::StringReplace(strQuery, "%s", "%q");
  BXUtils::StringReplace(strQuery, "%I64", "%ll");

  va_list args;
  va_copy(args, argList);
  char *szSql = sqlite3_vmprintf(strQuery.c_str(), args);
  va_end(args);

  std::string strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  Dataset* pDataset = m_pDatabase->CreateDataset();
  if (pDataset == NULL)
  {
    LOG(LOG_LEVEL_ERROR,"BXDatabase::QuotedQuery - could not create dataset. query [%s] will not be executed",strResult.c_str());
    return NULL;
  }

  int iRetries = MEDIA_DATABSE_LOCK_RETRIES;  
  while (iRetries > 0)
  {
    try
    {
      pDataset->query(strResult.c_str());
      return pDataset;
    }
    catch(dbiplus::DbErrors& e) 
    {
      if (GetLastErrorCode() == SQLITE_LOCKED || GetLastErrorCode() == SQLITE_BUSY || GetLastErrorCode() == SQLITE_CANTOPEN)
      {
        LOG(LOG_LEVEL_DEBUG,"BXDatabase::QuotedQuery - Database was locked, retry. [error=%s][msg=%s]",GetLastErrorMessage(),e.getMsg());

        SDL_Delay(BXUtils::GetRandInt(10) + 10);
        iRetries--;

        // log the last attempt as error
        if (iRetries == 0)
        {
          LOG(LOG_LEVEL_ERROR,"BXDatabase::QuotedQuery - Exception caught, could not execute query [%s]. [error=%s][msg=%s]",strResult.c_str(),GetLastErrorMessage(),e.getMsg());
        }

        continue;
      }
      else
      {
        // Some other error
        LOG(LOG_LEVEL_ERROR,"BXDatabase::QuotedQuery - Exception caught, could not execute query [%s]. [error=%s][msg=%s]",strResult.c_str(),GetLastErrorMessage(),e.getMsg());
        iRetries = 0;
      }
    }
  }

  delete pDataset;
  return NULL;
}

int BXDatabase::Insert(const char* query, ...)
{
  if (!IsOpen())
  {
    return MEDIA_DATABASE_ERROR;
  }
  
  //LOG(LOG_LEVEL_DEBUG, "Inserting query for database %d in thread %d", m_pDatabase, pthread_self());

  std::string strQuery = query;
  BXUtils::StringReplace(strQuery, "%s", "%q");
  BXUtils::StringReplace(strQuery, "%I64", "%ll");

  va_list args;
  va_start(args, query);
  char *szSql = sqlite3_vmprintf(strQuery.c_str(), args);
  va_end(args);

  std::string strResult;
  if (szSql)
  {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  //LOG(LOG_LEVEL_DEBUG, "Inserting query: %s for database %d in thread %d", strResult.c_str(), m_pDatabase, pthread_self());

  Dataset* pDataset = m_pDatabase->CreateDataset();
  if (pDataset == NULL)
  {
    LOG(LOG_LEVEL_ERROR, "Could not create dataset, query %s not executed", strResult.c_str());
    return MEDIA_DATABASE_ERROR;
  }

  int iID = MEDIA_DATABASE_ERROR;

  int iRetries = MEDIA_DATABSE_LOCK_RETRIES;

  while (iRetries > 0)
  {
    try
    {
      // In case of the error, exception will be thrown
      pDataset->exec(strResult.c_str());

      iID = (int)sqlite3_last_insert_rowid(m_pDatabase->getHandle());
      iRetries = 0;
    }
    catch(dbiplus::DbErrors& e)
    {
      if (GetLastErrorCode() == SQLITE_LOCKED || GetLastErrorCode() == SQLITE_BUSY || GetLastErrorCode() == SQLITE_CANTOPEN)
      {
        LOG(LOG_LEVEL_DEBUG, "Database was locked, retry. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        // Sleep random amount of time 
        SDL_Delay(BXUtils::GetRandInt(10) + 10);
        iRetries--;

        // log the last attempt as error
        if (iRetries == 0)
          LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        continue;
      }
      else
      {
        // Some other error
        LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
        iRetries = 0;
      }
    }
  }

  delete pDataset;

  return iID;
}

bool BXDatabase::Create(const char *query, ...)
{
  if (!IsOpen()) {
    return false;
  }
  
  std::string strQuery = query;
  BXUtils::StringReplace(strQuery, "%s", "%q");
  BXUtils::StringReplace(strQuery, "%I64", "%ll");

  va_list args;
  va_start(args, query);
  char *szSql = sqlite3_vmprintf(strQuery.c_str(), args);
  va_end(args);

  std::string strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  Dataset* pDataset = m_pDatabase->CreateDataset();
  if (pDataset == NULL)
  {
    LOG(LOG_LEVEL_ERROR, "Could not create dataset, query %s not executed", strResult.c_str());
    return false;
  }

  int iRetries = MEDIA_DATABSE_LOCK_RETRIES;

  while (iRetries > 0)
  {
    try {

      // In case of the error, exception will be thrown
      pDataset->exec(strResult.c_str());
      iRetries = 0;

    }
    catch(dbiplus::DbErrors& e) {
      if (GetLastErrorCode() == SQLITE_LOCKED || GetLastErrorCode() == SQLITE_BUSY || GetLastErrorCode() == SQLITE_CANTOPEN)
      {
        LOG(LOG_LEVEL_DEBUG, "Database was locked, retry. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        SDL_Delay(BXUtils::GetRandInt(500) + 300);
        iRetries--;

        // log the last attempt as error
        if (iRetries == 0)
          LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        continue;
      }
      else
      {
        // Some other error
        LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
        iRetries = 0;
      }
    }
  }

  delete pDataset;
  return true;
}

dbiplus::Dataset* BXDatabase::Exec(const char *query, ...)
{
  if (!IsOpen()) {
    return NULL;
  }
  
  std::string strQuery = query;
  BXUtils::StringReplace(strQuery, "%s", "%q");
  BXUtils::StringReplace(strQuery, "%I64", "%ll");

  va_list args;
  va_start(args, query);
  char *szSql = sqlite3_vmprintf(strQuery.c_str(), args);
  va_end(args);

  std::string strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  Dataset* pDataset = m_pDatabase->CreateDataset();
  if (pDataset == NULL)
  {
    LOG(LOG_LEVEL_ERROR, "Could not create dataset, query %s not executed", strResult.c_str());
    return NULL;
  }

  int iRetries = MEDIA_DATABSE_LOCK_RETRIES;

  while (iRetries > 0)
  {
    try 
    {
      pDataset->exec(strResult.c_str());
      return pDataset;
    }
    catch(dbiplus::DbErrors& e) {
      if (GetLastErrorCode() == SQLITE_LOCKED || GetLastErrorCode() == SQLITE_BUSY || GetLastErrorCode() == SQLITE_CANTOPEN)
      {
        LOG(LOG_LEVEL_DEBUG, "Database was locked, retry. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        SDL_Delay(BXUtils::GetRandInt(10) + 10);
        iRetries--;
        
        // log the last attempt as error
        if (iRetries == 0)
          LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());

        continue;
      }
      else
      {
        // Some other error
        LOG(LOG_LEVEL_ERROR, "Exception caught, could not execute query. Error = %s, msg= %s", GetLastErrorMessage(), e.getMsg());
        iRetries = 0;
      }
    }
  }

  delete pDataset;
  return NULL;
}

bool BXDatabase::IsOpen()
{
  return m_bOpen;
}

bool BXDatabase::FileExists()
{
  std::string databaseFilePath = getDatabaseFilePath();
  bool bExists = BXUtils::FileExists(databaseFilePath);
  if (!bExists)
  {
    LOG(LOG_LEVEL_ERROR, "Database file does not exist");
  }

  return bExists;
}

bool BXDatabase::TablesExist()
{
  bool bExists;
  LOG(LOG_LEVEL_DEBUG, "File exists, check if tables are created");

  // Check if the version table can be accessed
  Dataset* pDS = Query("select * from version");

  if (pDS) {
    if (pDS->num_rows() != 0)
    {
      try {
        int iVersion = pDS->fv("iVersion").get_asInt();
        LOG(LOG_LEVEL_DEBUG, "Tables are ok. version: %d", iVersion);
        bExists = true;
      }
      catch(dbiplus::DbErrors& e) {
        LOG(LOG_LEVEL_ERROR, "Exception caught, Should create tables");
        bExists = false;
      }
    }
    else {
      LOG(LOG_LEVEL_ERROR, "No rows were extracted, Should create tables");
      bExists = false;
    }

    delete pDS;
  }
  else {
    bExists = false;
  }

  return bExists;
}

void BXDatabase::Close()
{
  if (!m_bOpen)
  {
    return ;
  }

  m_bOpen = false;

  if (m_pDatabase && m_pDBPoolObject)
    g_application.GetDBConnectionPool()->ReturnToPool( m_pDBPoolObject, getDatabaseFilePath() );  

  m_pDatabase = NULL;
  m_pDBPoolObject = NULL;
}

void BXDatabase::StartTransaction()
{

  if (m_pDatabase)
  {
    m_pDatabase->start_transaction();
  }

}

bool BXDatabase::CommitTransaction()
{
  try
  {
    if (m_pDatabase)
    {
      m_pDatabase->commit_transaction();
    }
  }
  catch (...)
  {
    printf("ERROR: Could not commit transaction");
    return false;
  }
  return true;
}

void BXDatabase::RollbackTransaction()
{
  if (m_pDatabase)
  {
    m_pDatabase->rollback_transaction();
  }
}

bool BXDatabase::InTransaction()
{
  if (!m_pDatabase) return false;
  return m_pDatabase->in_transaction();
}

std::string BXDatabase::PrepareSQLStatement(std::string strStmt, ...)
{
  BXUtils::StringReplace(strStmt, "%s", "%q");
  BXUtils::StringReplace(strStmt, "%I64", "%ll");

  va_list args;
  va_start(args, strStmt);
  char *szSql = sqlite3_vmprintf(strStmt.c_str(), args);
  va_end(args);

  std::string strResult;
  if (szSql) {
    strResult = szSql;
    sqlite3_free(szSql);
  }

  return strResult;
}

std::string BXDatabase::getDatabaseFilePath()
{
  return mDatabaseFilePath;
}

} // namespace BOXEE
