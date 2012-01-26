

#include "HttpCacheManager.h"
#include "lib/sqLite/sqlitedataset.h"
#include "StringUtils.h"
#include "utils/log.h"
#include "Util.h"
#include "utils/SingleLock.h"
#include "SpecialProtocol.h"
#include "File.h"
#include "StdString.h"
#include "FileItem.h"
#include "FileSystem/Directory.h"
#include "lib/libBoxee/boxee.h"
#include "AdvancedSettings.h"
#include "Application.h"
#include <inttypes.h>

#include <vector>

//extern "C" time_t curl_getdate(const char * datestring , time_t *now );
//extern "C" char * curl_escape( const char * url , int length ); 
//extern "C" void   curl_free( char * ptr );
//#include "FileSystem/curl/curl.h"
#include "FileSystem/DllLibCurl.h"

#define HTTP_CACHE_DB_VERSION 4
#define FOLDER_CLEANUP_PERIOD  3600

using namespace XBMC;
using namespace XFILE;



#define    _CACHE_REF_DONT_UPDATE_TIME         0


void CHttpCacheManager::UpdateRefMap(const std::string  &strUrl, unsigned long  accessTime, int  refDiff, bool  progressStat, bool shouldUpdateProgress)
{
  if (IsUrlInMap(strUrl))
  {
    CacheRefObj *objRef = m_refMap[strUrl];
    if ((int (objRef->m_refCount) + refDiff)  >= 0 )
    {
      objRef->m_refCount =  objRef->m_refCount + refDiff;
    }
    else
    {
      CLog::Log(
          LOGWARNING,
          "CHttpCacheManager::UpdateRefMap negative reference count for url %s (hcm)",
          strUrl.c_str());
      return;
    }

    if (shouldUpdateProgress)
    {
      objRef->m_progress =  progressStat;
    }

    if (accessTime != _CACHE_REF_DONT_UPDATE_TIME )
    {
      objRef->m_accessTime =  accessTime;
    }
  }
  else
  {
    CacheRefObj *newObjRef =  new (CacheRefObj)(accessTime);

    // make sure that we dont want to decrease the ref count or hit count to a non existing obj
    if ((refDiff < 0 ))
    {
      CLog::Log(
          LOGDEBUG,
          "CHttpCacheManager::UpdateRefMap Can not decrease ref count to new object %s (hcm)",
          strUrl.c_str());
      return;
    }
    else
    {
      newObjRef->m_refCount += refDiff;
      if (shouldUpdateProgress)
      {
        newObjRef->m_progress = progressStat;
      }
    }

    m_refMap[strUrl] = newObjRef;
  }

  CLog::Log(LOGDEBUG,
      "update ref map. url %s new ref_count %ld  progress %d (hcm)",
      strUrl.c_str(), m_refMap[strUrl]->m_refCount,
      m_refMap[strUrl]->m_progress);

}
bool CHttpCacheManager::IsUrlInMap(const std::string  &strUrl)
{
  return (m_refMap.find(strUrl) != m_refMap.end());
}

bool CHttpCacheManager::IsUrlInProgress(const std::string  &strUrl)
{
  if (IsUrlInMap(strUrl))
  {
    return m_refMap[strUrl]->m_progress;
  }

  return false;
}

int CHttpCacheManager::GetUrlRefCount(const std::string  &strUrl)
{
  if (IsUrlInMap(strUrl))
  {
    return m_refMap[strUrl]->m_refCount;
  }

  return -1;
}

void CHttpCacheManager::RemoveFromRefMap(const std::string  &strUrl)
{
  if (IsUrlInMap(strUrl))
  {
    CLog::Log(LOGDEBUG,
        "CHttpCacheManager::RemoveFromRefMap remove url %s  (hcm)",
        strUrl.c_str());

    CacheRefObj *objRef = m_refMap[strUrl];
    m_refMap.erase(strUrl);

    delete objRef;
  }
}
void CHttpCacheManager::ClearRefMap()
{
  CLog::Log(LOGDEBUG,"CHttpCacheManager::ClearRefMap");
  CacheRefMap::iterator db_it = m_refMap.begin();
  while (db_it != m_refMap.end())
  {
    CacheRefObj *objRef = db_it->second;
    m_refMap.erase(db_it++);

    delete objRef;
  }
}

//
// internal structure of the cache handle
//
struct CacheContext
{
public:
  dbiplus::SqliteDatabase  *m_pDatabase;
  CHttpCacheManager        *m_pCacheManager;
  std::vector<std::string>  m_references;
  std::vector<std::string>  m_openTransactions;
  
  CacheContext() : m_pDatabase(NULL), m_pCacheManager(NULL) { }
};

CHttpCacheHandleGuard::CHttpCacheHandleGuard(HttpCacheHandle h) : m_handle(h) {}  

CHttpCacheHandleGuard::~CHttpCacheHandleGuard() 
{
  CacheContext *ctx = (CacheContext *)m_handle;
  if (ctx && ctx->m_pCacheManager)
    ctx->m_pCacheManager->Close(m_handle);
}

void CHttpCacheHandleGuard::SetHandle(HttpCacheHandle h)
{
  m_handle = h;
}

// helper functions
static bool DBActionInternal(dbiplus::SqliteDatabase* pDB, bool bQuery, dbiplus::Dataset** ds, const char *szSql)
{
  if (ds)
    *ds = NULL;

  if (!szSql)
    return false;
  
  dbiplus::Dataset* pDataset = pDB->CreateDataset();
  if (pDataset == NULL)
  {
    CLog::Log(LOGERROR,"failed to create dataset!");
    return false;
  }
  
  int iRetries = 5;
  while (iRetries > 0)
  {
    try 
    {
      bQuery?pDataset->query(szSql):pDataset->exec(szSql);
      
      if (!bQuery)
      {
        delete pDataset;
        pDataset = NULL;
      }
      else
      {
        if (ds)
          *ds = pDataset;
      }
      
      return true;
    }
    catch(dbiplus::DbErrors& e) 
    {
      int nErr = sqlite3_errcode(pDB->getHandle());
      const char *msg = sqlite3_errmsg(pDB->getHandle());
      if (nErr == SQLITE_LOCKED || nErr == SQLITE_BUSY || nErr == SQLITE_CANTOPEN)
      {
        Sleep(10);
        iRetries--;
        
        // log the last attempt as error
        if (iRetries == 0)
          CLog::Log(LOGERROR, "Exception caught, could not execute query. Error = %d, msg= %s", nErr, msg);

        continue;
      }
      else if (nErr == SQLITE_IOERR || nErr == SQLITE_CORRUPT)
      {
        CLog::Log(LOGERROR, "Fatal exception caught on query, re-creating db. Error = %d, msg= %s", nErr, msg);
        iRetries = 0;
      }
      else
      {
        CLog::Log(LOGERROR, "Exception caught on query. Error = %d, msg= %s", nErr, msg);
        iRetries = 0;
      }
    }
  }
  
  delete pDataset;
  return false;
}

static dbiplus::Dataset* DBAction(dbiplus::SqliteDatabase* pDB, bool bQuery, const char *strQuery, ...)
{
  va_list args;
  va_start(args, strQuery);

  CStdString strStmt = strQuery;
  strStmt.Replace("%s", "%q");
  strStmt.Replace("%I64", "%ll");

  char *szSql = sqlite3_vmprintf(strStmt.c_str(), args);
  va_end(args);

  dbiplus::Dataset* ds;
  DBActionInternal(pDB, bQuery, &ds, szSql);

  sqlite3_free(szSql);

  return ds;
}

static bool DBAction(dbiplus::SqliteDatabase* pDB, bool bQuery, dbiplus::Dataset** ds, const char *strQuery, ...)
{
  va_list args;
  va_start(args, strQuery);

  CStdString strStmt = strQuery;
  strStmt.Replace("%s", "%q");
  strStmt.Replace("%I64", "%ll");

  char *szSql = sqlite3_vmprintf(strStmt.c_str(), args);
  va_end(args);

  bool success = DBActionInternal(pDB, bQuery, ds, szSql);
  sqlite3_free(szSql);
  return success;
}
CHttpCacheManager::CHttpCacheManager() : m_maxAgeInCache(-1), m_defaultExpiry(0), m_nMaxDirSize(100 * 1024 * 1024)
{
  m_scanProcessor.SetName("Http Cache CleanUp");
  m_scanProcessor.Start(1);
  unsigned long now = (unsigned long)time(NULL);

  // its a trick for the first cleanup process launch.
  // we define that the last time that the system was cleaned up was an hour ago(PERIOD_TIME) + 2 min,
  // so the next time we will scan should be at least 2 second from the application initialization.
  m_lastCleanup = now - FOLDER_CLEANUP_PERIOD + 120;
}

CHttpCacheManager::~CHttpCacheManager() 
{
	  m_scanProcessor.Stop();
}

bool CHttpCacheManager::Initialize(const std::string &strLocalCachePath)
{
  std::string strTranslatedPath = PTH_IC(strLocalCachePath);
  m_localCachePath = strTranslatedPath;
  m_localCacheDBPath = CUtil::AddFileToFolder(strTranslatedPath, "http_cache.db");  

  dbiplus::SqliteDatabase* pDatabase = new dbiplus::SqliteDatabase();
  pDatabase->setDatabase(m_localCacheDBPath.c_str());
  
  try 
  {
    if ( pDatabase->connect() != DB_CONNECTION_OK)
    {
      CLog::Log(LOGERROR, "could not connect to database file %s\n", m_localCacheDBPath.c_str());
      delete pDatabase;
      return false;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception caught during db connect to database file %s\n", m_localCacheDBPath.c_str());
    delete pDatabase;
    return false;
  }

  bool success = true;

  dbiplus::Dataset* pDS = DBAction(pDatabase, true, "select distinct 1 from sqlite_master where tbl_name='tb_version'");
  if (pDS && pDS->num_rows() > 0)
  {
    if (pDS)
      delete pDS;

    pDS = DBAction(pDatabase, true, "select vernum from tb_version");
    if (pDS)
    {
      int vernum = pDS->fv(0).get_asInt();
      if (vernum < HTTP_CACHE_DB_VERSION)
      {
        DBAction(pDatabase, false, "alter table tb_cache_map add column file_size integer\n");
        DBAction(pDatabase, false, "alter table tb_cache_map add column hit_count integer\n");

        // We cant remove column from a table so we initiate the unnecessary fields with -1
        success &= DBAction(pDatabase, false, NULL, "update tb_cache_map set access_time = -1,  ref_count  = -1, in_progress = -1");

        success &= DBAction(pDatabase, false, NULL, "delete from tb_version\n");
        success &= DBAction(pDatabase, false, NULL, "insert into tb_version (vernum) values (%d)", HTTP_CACHE_DB_VERSION);
      }
    }
    else if (!pDS)
    {
      success = false;
    }
  }
  else
  {
    CLog::Log(LOGINFO,"need to create new cache db");
    DBAction(pDatabase, false, "PRAGMA cache_size=50000\n");
    DBAction(pDatabase, false, "PRAGMA default_cache_size=50000\n");
    DBAction(pDatabase, false, "PRAGMA encoding=\"UTF-8\"\n");
    DBAction(pDatabase, false, "PRAGMA case_sensitive_like=0\n");
    DBAction(pDatabase, false, "PRAGMA synchronous=NORMAL\n");
    DBAction(pDatabase, false, "PRAGMA temp_store=MEMORY\n");

    DBAction(pDatabase, false, "create table tb_version (vernum integer)\n");
    DBAction(pDatabase, false, "delete from tb_version"); // in case it already exists
    DBAction(pDatabase, false, "insert into tb_version (vernum) values (%d)", HTTP_CACHE_DB_VERSION);

    DBAction(pDatabase, false, "create table tb_cache_map (url text primary key, create_time integer, access_time integer, expire_time integer, ref_count integer, in_progress integer, local_path text, server_time text, etag text, file_size integer, hit_count integer)\n");
  }
  
  if (pDS)
    delete pDS;
  
  // We cant remove column from a table so we initiate the unnecessary fields with -1
  success &= DBAction(pDatabase, false, NULL, "update tb_cache_map set access_time = -1,  ref_count  = -1, in_progress = -1");
  
  pDatabase->disconnect();
  delete pDatabase;

  return success;
}

void CHttpCacheManager::Deinitialize()
{

  CSingleLock lock(m_lock);

  std::map<std::string, HANDLE>::iterator iter = m_mapDoneEvents.begin();
  while (iter != m_mapDoneEvents.end())
  {
    CLog::Log(LOGWARNING,"CHttpCacheManager::Deinitialize - url <%s> has open event handle", iter->first.c_str());
    if (iter->second)
      CloseHandle(iter->second);
    iter++;
  }
  m_mapDoneEvents.clear();
  
  dbiplus::SqliteDatabase* pDatabase = new dbiplus::SqliteDatabase();
  pDatabase->setDatabase(m_localCacheDBPath.c_str());
  
  try 
  {
    if ( pDatabase->connect() != DB_CONNECTION_OK)
    {
      CLog::Log(LOGERROR, "could not connect to database file %s\n", m_localCacheDBPath.c_str());
      delete pDatabase;
      return;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception caught during db connect to database file %s\n", m_localCacheDBPath.c_str());
    delete pDatabase;
    return;
  }
  
  for(CacheRefMap::const_iterator db_it = m_refMap.begin(); db_it != m_refMap.end(); ++db_it)
  {
    if (db_it->second && db_it->second->m_refCount > 0)
    {
	  CLog::Log(LOGWARNING,"CHttpCacheManager::Deinitialize - url <%s> has %ld open references", db_it->first.c_str(), db_it->second->m_refCount );
    }
  }
  
  ClearRefMap();

  pDatabase->disconnect();
  delete pDatabase;
}

void CHttpCacheManager::Delete()
{
  XFILE::CFile::Delete(m_localCacheDBPath);
}

HttpCacheHandle CHttpCacheManager::Open()
{
  CacheContext *ctx = new CacheContext;
  ctx->m_pCacheManager = this;
  ctx->m_pDatabase = new dbiplus::SqliteDatabase();
  ctx->m_pDatabase->setDatabase(m_localCacheDBPath.c_str());

  try 
  {
    if ( ctx->m_pDatabase->connect() != DB_CONNECTION_OK)
    {
      CLog::Log(LOGERROR, "could not connect to database file %s\n", m_localCacheDBPath.c_str());
      delete ctx->m_pDatabase;
      delete ctx;
      return NULL;
    }
  }
  catch (...)
  {
    CLog::Log(LOGERROR, "exception caught during db connect to database file %s\n", m_localCacheDBPath.c_str());
    delete ctx->m_pDatabase;
    delete ctx;
    return NULL;
  }
  
  return ctx; 
}

void CHttpCacheManager::Close(HttpCacheHandle h)
{
  CSingleLock lock(m_lock);

  CacheContext *ctx = (CacheContext *)h;
  
  if (!ctx || !ctx->m_pDatabase)
    return;

  // fail all open transactions
  size_t i;
  std::vector<std::string> vecInProgress = ctx->m_openTransactions; // copy a side since FailedCachingURL will remove it from the vector
  for (i=0; i<vecInProgress.size(); i++)
  {
    FailedCachingURL(h, vecInProgress[i]);
  }
  ctx->m_openTransactions.clear();

  for (i=0; i<ctx->m_references.size(); i++)
  {
    UpdateRefMap(ctx->m_references[i], _CACHE_REF_DONT_UPDATE_TIME, -1);

    int refCount = GetUrlRefCount(ctx->m_references[i]);
    if (refCount  == 0)
    {
      HANDLE h = m_mapDoneEvents[ctx->m_references[i]];
      if (h)
      {
          CloseHandle(h);
      }
        m_mapDoneEvents.erase(ctx->m_references[i]);
      }
    }
  
  ctx->m_pDatabase->disconnect();
  delete ctx->m_pDatabase;
  delete ctx;
}

bool CHttpCacheManager::Exists(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName)
{
  bool bExpired=false;
  unsigned int nTimeInCache;
  std::string etag;
  unsigned long httpTime=0;
  bool bExists = Exists(h, strUrl, strLocalName, bExpired, nTimeInCache, etag, httpTime);
  return bExists && !bExpired;
}

bool CHttpCacheManager::Exists(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, bool &bExpired, unsigned int &timeInCache, std::string &strETag, unsigned long &httpTime)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return false;
  
  if (ctx->m_pCacheManager != this) // handle from another manager
  {
    CLog::Log(LOGERROR,"got handler from a different manager! url: <%s>", strUrl.c_str());
    return false;
  }
  
  CSingleLock lock(m_lock);
  
  dbiplus::Dataset* pDS = DBAction(ctx->m_pDatabase, true, "select local_path, expire_time, create_time, server_time, etag from tb_cache_map where url='%s'", strUrl.c_str());
  
  bool bExists = false;

 // there is no point to check if the file's status since we add it to the database only after we done te process
  if (pDS && pDS->num_rows() > 0)
  {
    if (!pDS->fv(0).get_isNull())
    {
      strLocalName = pDS->fv(0).get_asString();
      bExists = !strLocalName.empty() && CFile::Exists(strLocalName);
    }
    
    httpTime = 0;
    strETag.clear();
    
    if (!pDS->fv(3).get_isNull())
      httpTime = g_curlInterface.getdate(pDS->fv(3).get_asString().c_str(), 0);

    if (!pDS->fv(4).get_isNull())
      strETag  = pDS->fv(4).get_asString();

    unsigned long now = (unsigned long)time(NULL);
    bExpired = true;
    if (!pDS->fv(1).get_isNull() && pDS->fv(1).get_asUInt() > 0)
      bExpired = (pDS->fv(1).get_asUInt() < now); // should hold expiry time already transformed to local time (upon insert) so it can be compared with time(NULL)
    
    if (bExists)
    {
      ctx->m_references.push_back(strUrl);

      // TODO - update hit count - when ever we will support LRU
      // DBAction(ctx->m_pDatabase, false, "update tb_cache_map set ref_count = ref_count + 1, hit_count = hit_count + 1, access_time=%u where url='%s'", now, strUrl.c_str());
      UpdateRefMap(strUrl, now , 0);
    }
    else
    {
      // The url is exist in the database, but the file is not exist - delete it from the database
      DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
      CLog::Log(LOGDEBUG, "CHttpCacheManager::Exist -  about to delete %s from database(hcm)",strUrl.c_str());
      RemoveFromRefMap(strUrl);
    }
  }

  if (pDS)
    delete pDS;
  
  return bExists;
}

HttpCacheReturnCode CHttpCacheManager::RemoveFromCache(HttpCacheHandle h, const std::string &strUrl)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return HTTP_CACHE_FAILED;
  
  if (ctx->m_pCacheManager != this) // handle from another manager
    return HTTP_CACHE_FAILED;

  CSingleLock lock(m_lock);
  
  dbiplus::Dataset* pDS = DBAction(ctx->m_pDatabase, true, "select local_path from tb_cache_map where url='%s'", strUrl.c_str());

  HttpCacheReturnCode ret = HTTP_CACHE_NO_SUCH_ENTRY;
  {
    std::string strLocalName = pDS->fv(0).get_asString();
    int  nRefCount = GetUrlRefCount(strUrl) ;
    bool bInProgress = IsUrlInProgress(strUrl);

    if (bInProgress || nRefCount > 0)
    {
      ret = HTTP_CACHE_FILE_IN_USE;
    }
    else
    {
      if (!strLocalName.empty())
        ::DeleteFile(strLocalName.c_str());
      DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
      CLog::Log(LOGDEBUG, "CHttpCacheManager::RemoveFromCache -  about to delete %s (hcm) from database",strUrl.c_str());
      RemoveFromRefMap(strUrl);
      ret = HTTP_CACHE_OK;
    }
  }
  
  if (pDS)
    delete pDS;

  return ret;
}

HttpCacheReturnCode CHttpCacheManager::StartCachingURL(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, std::string &strETag, unsigned long &httpTime)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return HTTP_CACHE_FAILED;

  if (ctx->m_pCacheManager != this) // handle from another manager
    return HTTP_CACHE_FAILED;

  CSingleLock lock(m_lock);
  
  bool bExpired=false;
  unsigned int tmInCache=0;
  unsigned int now = (unsigned int)time(NULL);

  strETag.clear();
  httpTime = 0;

  if (Exists(h, strUrl, strLocalName, bExpired, tmInCache, strETag, httpTime))
  {
    if (!bExpired)
    {
      UpdateRefMap(strUrl, now, 1);
      return HTTP_CACHE_ALREADY_EXISTS;
    }
  }
  
  HttpCacheReturnCode ret = HTTP_CACHE_OK;
  strLocalName = GetCachedName(strUrl);

  if (IsUrlInMap(strUrl))
  {
    UpdateRefMap(strUrl, now, 1);

      if (std::find(ctx->m_openTransactions.begin(), ctx->m_openTransactions.end(), strUrl) == ctx->m_openTransactions.end())
        ctx->m_openTransactions.push_back(strUrl);

  }
  else
  {
    UpdateRefMap(strUrl, now, 1,true);
  }

  if (std::find(ctx->m_references.begin(), ctx->m_references.end(), strUrl) == ctx->m_references.end())
  {
    CLog::Log(LOGDEBUG,"CHttpCacheManager::StartCachingURL   push %s to  m_references (hcm)", strUrl.c_str());
      ctx->m_references.push_back(strUrl);
  }
  
  return ret;
}

const char **CHttpCacheManager::GetUsedReponseHeaders() 
{
  static const char *reqHeaders[] = {"Date", "Expires", "Etag", "Last-Modified", "Cache-Control", "Pragma", "Range", "Content-Range", "Content-Length", "Content-Type", 0};
  return reqHeaders;
}

void CHttpCacheManager::DoneCachingURL(HttpCacheHandle h, const std::string &strUrl, long nHttpRespCode, const HttpCacheHeaders & cacheHeaders)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
  {
    CLog::Log(LOGERROR, "%s - sanity fail. empty handle.", __FUNCTION__);
    return;
  }

  if (ctx->m_pCacheManager != this) // handle from another manager
    return ;

  CSingleLock lock(m_lock);
  HttpCacheHeaders::const_iterator expiry = cacheHeaders.find("Expires");
  HttpCacheHeaders::const_iterator etag   = cacheHeaders.find("Etag");
  HttpCacheHeaders::const_iterator date   = cacheHeaders.find("Date");
  HttpCacheHeaders::const_iterator modified = cacheHeaders.find("Last-Modified");
  HttpCacheHeaders::const_iterator cacheControl = cacheHeaders.find("Cache-Control");
  HttpCacheHeaders::const_iterator pragma = cacheHeaders.find("Pragma");
  HttpCacheHeaders::const_iterator content = cacheHeaders.find("Content-Type");
  
  // check if we should cache this page
  bool bShouldCache = true;
  bool bShouldRevalidate = false;
  if (pragma != cacheHeaders.end() && pragma->second.find("no-cache") != std::string::npos)
  {
    bShouldCache = false;
  }

  if (cacheControl != cacheHeaders.end() && 
      ( 
         cacheControl->second.find("no-cache") != std::string::npos || 
         cacheControl->second.find("no-store") != std::string::npos  
      ) )
    bShouldCache = false;

  if (nHttpRespCode == 206 || nHttpRespCode > 304) // partial and error response
  {
    bShouldCache = false;
  }
  
  if (cacheHeaders.find("Range") != cacheHeaders.end() || cacheHeaders.find("Content-Range") != cacheHeaders.end()) // no support for ranges
  {
    bShouldCache = false;
  }
    
  if (cacheControl != cacheHeaders.end() && cacheControl->second.find("must-revalidate") != std::string::npos)
    bShouldRevalidate = true;
    
  int nForceExpiry = 0;
  if (bShouldCache && content != cacheHeaders.end())
  {
    if (content->second.find("image/") == 0)
    {
      nForceExpiry = 3600; // one hour
    }
  }
    
  if (bShouldCache)
  {
    bool         row_exist = false;
    unsigned int now = (unsigned int)time(NULL);
    unsigned int tmExpire = now+m_defaultExpiry;
    unsigned int inDbExpire = now+m_defaultExpiry;
    
    // we need to load previous values if those exist to avoid 304 empty responses overriding cache values 
    std::string modTime;
    std::string strETag;
    dbiplus::Dataset* pDS = DBAction(ctx->m_pDatabase, true, "select server_time, expire_time, etag from tb_cache_map where url='%s'", strUrl.c_str());

    if (pDS && pDS->num_rows()>0)
    {
      row_exist = true;

      if (!pDS->fv(0).get_isNull())
        modTime = pDS->fv(0).get_asString();

      if (!pDS->fv(1).get_isNull())
      {
        tmExpire = pDS->fv(1).get_asUInt();
        inDbExpire = tmExpire;
        nForceExpiry = 0;
      }

      if (!pDS->fv(2).get_isNull())
        strETag = pDS->fv(2).get_asString();
    }

    if (pDS)
      delete pDS;
    
    // set etag
    if (etag != cacheHeaders.end())
      strETag = etag->second;
    
    // we want the "if-modified-since" request header to contain the "Last-Modified" value if it exists. if not - fallback to "Date" reported by the server.
    if (modified != cacheHeaders.end())
      modTime = modified->second;
    else if (modTime.empty() && date != cacheHeaders.end())
      modTime = date->second;
        
    // check expiry time
    unsigned int tmServer = now;
    if (date != cacheHeaders.end())
    {
      tmServer = (unsigned int)g_curlInterface.getdate(date->second.c_str(),NULL);
    }
    
    if (bShouldRevalidate)
      tmExpire = 0;
    else if ( expiry != cacheHeaders.end() )
    {
      unsigned int exp = (unsigned int)g_curlInterface.getdate(expiry->second.c_str(), NULL);
      if (exp < tmServer)
        tmExpire = 0;
      else
        tmExpire = now + (exp - tmServer);
    }
    
    // check max-age cache-control directive. it should override "Expires" if it exists
    if (cacheControl != cacheHeaders.end())
    {
      size_t pos = cacheControl->second.find("max-age");
      if (pos != std::string::npos)
      {
        const char *ptr = cacheControl->second.c_str() + pos + 7;
        while (*ptr && ( (*ptr) == ' ' || (*ptr) == '=' ))
          ptr++;
        
        tmExpire = now + atoi(ptr);
      }
    }

    if (m_maxAgeInCache > 0 && tmExpire > now + m_maxAgeInCache)
      tmExpire = now + m_maxAgeInCache;
    
    if (nForceExpiry > 0 && tmExpire < now + nForceExpiry)
      tmExpire = now + nForceExpiry;

    struct __stat64 buffer;
    CFile::Stat(GetCachedName(strUrl), &buffer);
    
    UpdateRefMap(strUrl, _CACHE_REF_DONT_UPDATE_TIME, 0);

    if (row_exist)
    {
      if (inDbExpire != tmExpire)
      {
        CLog::Log(LOGDEBUG,"update expire time of url %s to the database (hcm)", strUrl.c_str());
        DBAction(ctx->m_pDatabase, false, "update tb_cache_map set expire_time=%u, server_time='%s', etag='%s', file_size=%d where url='%s'", tmExpire,
           modTime.c_str(), strETag.c_str(), (int)buffer.st_size, strUrl.c_str());
      }
    }
    else
    {
      std::string strLocalName = GetCachedName(strUrl);

      // check the cache quota, and check if we want to delete old files
      EnsureCacheSize(h, buffer.st_size);
      CLog::Log(LOGDEBUG,"insert %s to the database (hcm)", strUrl.c_str());
      DBAction(ctx->m_pDatabase, false, "insert into tb_cache_map values('%s', %u, -1, %u, -1, -1, '%s', '%s', '%s', %d, 0)",
       		                                                           strUrl.c_str(), now,tmExpire, strLocalName.c_str(), modTime.c_str(),strETag.c_str(), (int)buffer.st_size);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG, "CHttpCacheManager::DoneCachingURL -  about to delete %s (hcm) from database", strUrl.c_str());
    DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
    ::DeleteFile(GetCachedName(strUrl).c_str());
    RemoveFromRefMap(strUrl);
  }
  
  std::vector<std::string>::iterator iter = ctx->m_openTransactions.begin();
  while (iter != ctx->m_openTransactions.end())
  {
    if (*iter == strUrl)
      iter = ctx->m_openTransactions.erase(iter);
    else
      iter++;
  }
  
  HANDLE e = m_mapDoneEvents[strUrl];
  if (e)
    SetEvent(e);
  
  // create a clean process
  unsigned long now = (unsigned long)time(NULL);

  if (m_lastCleanup + FOLDER_CLEANUP_PERIOD < now )
  {
    // update last cleanup time
    m_lastCleanup = now;

    HttpCacheManagerCleanup* pJob = new HttpCacheManagerCleanup(this);
    m_scanProcessor.QueueJob( pJob );
  }
}

void CHttpCacheManager::FailedCachingURL(HttpCacheHandle h, const std::string &strUrl)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return;

  if (ctx->m_pCacheManager != this) // handle from another manager
    return ;

  CSingleLock lock(m_lock);

  HANDLE e = m_mapDoneEvents[strUrl];
  if (e)
    SetEvent(e);
  
  if (RemoveFromCache(h, strUrl) != HTTP_CACHE_OK)
  {
    CLog::Log(LOGDEBUG, "CHttpCacheManager::FailedCachingURL -  about to delete %s from database (hcm)", strUrl.c_str());
    // we have no idea what happened and if the local file is valid or not - just remove the entry and hope for the best
    DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
    RemoveFromRefMap(strUrl);
  }
  
  std::vector<std::string>::iterator iter = ctx->m_openTransactions.begin();
  while (iter != ctx->m_openTransactions.end())
  {
    if (*iter == strUrl)
      iter = ctx->m_openTransactions.erase(iter);
    else
      iter++;
  }
  
}

void CHttpCacheManager::CancelCachingURL(HttpCacheHandle h, const std::string &strUrl)
{
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return;

  if (ctx->m_pCacheManager != this) // handle from another manager
    return ;

  CSingleLock lock(m_lock);
  
  HANDLE e = m_mapDoneEvents[strUrl];
  if (e)
    SetEvent(e);
  
  UpdateRefMap(strUrl, _CACHE_REF_DONT_UPDATE_TIME, 0);
  
  std::vector<std::string>::iterator iter = ctx->m_openTransactions.begin();
  while (iter != ctx->m_openTransactions.end())
  {
    if (*iter == strUrl)
      iter = ctx->m_openTransactions.erase(iter);
    else
      iter++;
  }
}

HttpCacheReturnCode CHttpCacheManager::WaitForURL(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, int nMillis)
{  
  CacheContext *ctx = (CacheContext *)h;
  if (!ctx || !ctx->m_pDatabase)
    return HTTP_CACHE_FAILED;

  if (ctx->m_pCacheManager != this) // handle from another manager
    return HTTP_CACHE_FAILED;

  CSingleLock lock(m_lock);

  HANDLE e = m_mapDoneEvents[strUrl];
  if (!e)
  {
    e = CreateEvent(NULL, TRUE, FALSE, NULL);
    m_mapDoneEvents[strUrl] = e;
  }
  
  HANDLE copy;
  if (DuplicateHandle(GetCurrentProcess(), e, GetCurrentProcess(), &copy, 0, FALSE, DUPLICATE_SAME_ACCESS))
  {
    lock.Leave();
    ::WaitForSingleObject(copy, nMillis);
    CloseHandle(copy);
  }
  
  return Exists(h, strUrl, strLocalName)?HTTP_CACHE_OK:HTTP_CACHE_TIMEOUT;
}

std::string CHttpCacheManager::GetCachedName(const std::string &strUrl)
{
  char *e = g_curlInterface.escape(strUrl.c_str(), strUrl.size());
  if (e)
  {
    std::string strRes = CUtil::AddFileToFolder(m_localCachePath, e);  
    g_curlInterface.free_curl(e);
    return strRes;
  }
  return StringUtils::EmptyString;
}

void CHttpCacheManager::SetMaxAgeInCache(int nSeconds)
{
  m_maxAgeInCache = nSeconds;
}

int  CHttpCacheManager::GetMaxAgeInCache()
{
  return m_maxAgeInCache;
}

void CHttpCacheManager::SetDefaultExpiry(int nSeconds)
{
  m_defaultExpiry = nSeconds;
}

int  CHttpCacheManager::GetDefaultExpiry()
{
  return m_defaultExpiry;
}

int  CHttpCacheManager::GetCacheDirSize(HttpCacheHandle h)
{
  CacheContext *ctx = (CacheContext *)h;
  dbiplus::Dataset* pDS = DBAction(ctx->m_pDatabase, true, "select sum(file_size) from tb_cache_map");
  if (pDS && pDS->num_rows()>0)
  {
    int s = pDS->fv(0).get_asInt();  
    delete pDS;
    return s;
  }
  
  if (pDS)
    delete pDS;  
  
  return -1;
}

void CHttpCacheManager::Cleanup()
{
  CSingleLock lock(m_lock);

  HttpCacheHandle cacheHandle = Open();

  CacheContext *ctx = (CacheContext *)cacheHandle;

  if (!ctx)
    return;
  
  // first - delete all expired files
  unsigned long now = (unsigned long)time(NULL);
  dbiplus::Dataset* pDS = DBAction(ctx->m_pDatabase, true, "select url,local_path from tb_cache_map where (expire_time is null or expire_time < %u) ", now);
  if (pDS)
  {
    while (!pDS->eof())
    {
      bool delete_file  = false;
      CStdString strUrl  = pDS->fv(0).get_asString();
      CStdString strFile = pDS->fv(1).get_asString();
      if (!strFile.IsEmpty())
      {
      	int refCount = GetUrlRefCount(strFile);
      	bool inProgress = IsUrlInProgress(strFile);

        if (!inProgress && refCount <= 0)
        {
          delete_file = true;
        }

        if (delete_file)
        {
          CLog::Log(LOGDEBUG,"deleting expired cache entry <%s> from database (hcm)", strUrl.c_str());
          DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
          ::DeleteFile(strFile);
          RemoveFromRefMap(strFile);
        }
      }
      pDS->next();
    }

    delete pDS;
  }
  
  lock.Leave();

  CFileItemList files;
  DIRECTORY::CDirectory::GetDirectory(m_localCachePath, files);
  for (int i=0; i< files.Size(); i++)
  {
    CStdString path = files[i]->m_strPath;
    dbiplus::Dataset* pDS2 = DBAction(ctx->m_pDatabase, true, "select url from tb_cache_map where local_path='%s' and (expire_time is null or expire_time < %u)", path.c_str(), now);
    if (pDS2)
    {
      if (pDS2->num_rows() > 0)
      {
        bool delete_file = false;
        CStdString strUrl = pDS2->fv(0).get_asString();
        if (!strUrl.IsEmpty())
        {
          int refCount = GetUrlRefCount(path);
          bool inProgress = IsUrlInProgress(path);

          if (!inProgress && refCount <= 0)
          {
            delete_file = true;
          }

          if (delete_file)
          {
            CLog::Log(LOGDEBUG,"deleting expired cache entry <%s>", strUrl.c_str());
            DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
            ::DeleteFile(path);
              RemoveFromRefMap(path);
          }
        }
      }
      delete pDS2;
    }
  }
  
  int nSize = GetCacheDirSize(cacheHandle);
  
  if (nSize > m_nMaxDirSize)
  {
    // TODO: implement cleanup according to LRU
  }

  Close(cacheHandle);
}

void CHttpCacheManager::SetMaxCacheSize(int nMaxSize)
{
  m_nMaxDirSize = nMaxSize;
}

void CHttpCacheManager::EnsureCacheSize(HttpCacheHandle h,int64_t newFileSize)
{

  CacheContext *ctx = (CacheContext *)h;

  uint64_t cacheSize = GetCacheDirSize(h) + newFileSize;

  if (cacheSize  < g_advancedSettings.m_httpCacheMaxSize)
  {
    return;
  }

  uint64_t requiredCacheSize = (uint64_t) (g_advancedSettings.m_httpCacheMaxSize * 0.6f);

  CLog::Log(LOGDEBUG,"%s Execced cache limitation %"PRIu64"  %"PRIu64" (hcm)",__FUNCTION__, g_advancedSettings.m_httpCacheMaxSize, cacheSize);

  dbiplus::Dataset* pDS = NULL;
  pDS = DBAction(ctx->m_pDatabase, true, "select url, local_path, file_size from tb_cache_map order by expire_time");

  if (pDS && pDS->num_rows() > 0)
  {
    while (cacheSize  > requiredCacheSize  && !pDS->eof())
    {
      CStdString strUrl = pDS->fv(0).get_asString();
      CStdString strLocalName = pDS->fv(1).get_asString();
      uint64_t   fileSize = pDS->fv(2).get_asInt64();

      CLog::Log(LOGDEBUG,"%s delete url %s local %s size %lld  (hcm)",__FUNCTION__, strUrl.c_str(), strLocalName.c_str(), fileSize);

      if (!strLocalName.empty())
      {
        ::DeleteFile(strLocalName.c_str());
      }
      DBAction(ctx->m_pDatabase, false, "delete from tb_cache_map where url='%s'", strUrl.c_str());
      CLog::Log(LOGDEBUG, "CHttpCacheManager::RemoveFromCache -  about to delete %s (hcm) from database",strUrl.c_str());
      RemoveFromRefMap(strUrl);

      cacheSize -= fileSize;
      pDS->next();
    }

    delete pDS;
  }
}

//
// internal class - delete the cache in background process
//

CHttpCacheManager::HttpCacheManagerCleanup::HttpCacheManagerCleanup(CHttpCacheManager* cacheHandler):BXBGJob("HttpCacheManagerCleanup")
{
  m_cacheHandler = cacheHandler;
}

CHttpCacheManager::HttpCacheManagerCleanup::~HttpCacheManagerCleanup()
{
}
void CHttpCacheManager::HttpCacheManagerCleanup::DoWork()
{
  if (!g_application.m_pPlayer || !g_application.m_pPlayer->IsPlaying())
  {
    CLog::Log(LOGDEBUG, "CHttpCacheManager::HttpCacheManagerCleanup::DoWork");
    m_cacheHandler->Cleanup();
  }
}

