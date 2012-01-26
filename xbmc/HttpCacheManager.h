#ifndef __HTTP_CACHE_MANAGER__H__
#define __HTTP_CACHE_MANAGER__H__

#include "utils/CriticalSection.h"
#include "lib/libBoxee/bxscheduletaskmanager.h"
#include "lib/libBoxee/bxbgprocess.h"

#include <map>
#include <string>

#ifdef _WIN32
  #ifndef strcasecmp
  #define strcasecmp _stricmp
  #endif
#endif

namespace XBMC
{
  struct ignorecasecomperand
  {
    bool operator() (const std::string& a, const std::string& b) const
    {
      return (strcasecmp (a.c_str ( ), b.c_str ( )) < 0);
    }
  };
  
  
  typedef void* HttpCacheHandle;
  typedef std::map<std::string, std::string, ignorecasecomperand> HttpCacheHeaders;
  
  typedef enum { HTTP_CACHE_OK = 0,
                 HTTP_CACHE_NO_SUCH_ENTRY,
                 HTTP_CACHE_FILE_IN_USE,
                 HTTP_CACHE_ALREADY_EXISTS,
                 HTTP_CACHE_ALREADY_IN_PROGRESS,
                 HTTP_CACHE_TIMEOUT,
                 HTTP_CACHE_FAILED
               } HttpCacheReturnCode; 

  //
  // helper class to make sure handle is closed when a function returns 
  //
  class CHttpCacheHandleGuard
  {
  public:
    CHttpCacheHandleGuard(HttpCacheHandle h); 
    ~CHttpCacheHandleGuard();
    void SetHandle(HttpCacheHandle h);
  private:
    HttpCacheHandle m_handle;
  };
  
  //
  // The reference count and progress information for the cache objects
  //
  struct CacheRefObj
  {
  public:
  	unsigned long  m_accessTime;
  	unsigned long  m_refCount;
  	bool           m_progress;
  	CacheRefObj(unsigned long  now) :m_accessTime(now), m_refCount(0), m_progress(0) {}
  };
  typedef std::map<std::string, CacheRefObj*> CacheRefMap;

  class CHttpCacheManager
  {
  public:
    CHttpCacheManager();
    virtual ~CHttpCacheManager();

    virtual bool Initialize(const std::string &strLocalCachePath);
    virtual void Deinitialize();
    virtual void Delete();

    // to use the cache - one must retrieve a handle using Open, and release (Close) it when the operation is over.
    virtual HttpCacheHandle Open();
    virtual void            Close(HttpCacheHandle h);
    
    // Exists:
    // returns true if the file is already cached.
    // returns the local file name in strLocalName, its time in cache in timeInCache, its etag in strETag and its time string as received from server in "httpTime"
    // the local file is guaranteed to exist at least until the handle is closed
    virtual bool  Exists(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, bool &bExpired, unsigned int &timeInCache, std::string &strETag, unsigned long &httpTime);     
    virtual bool  Exists(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName); // simplified     

    // RemoveFromCache:
    // removes the entry and local file if those exist
    virtual HttpCacheReturnCode RemoveFromCache(HttpCacheHandle h, const std::string &strUrl);     
    
    // StartCachingURL:
    // mark a URL as being retrieved. 
    // if the method succeeds, the caller must save the content of the url to the local path returned and call either DoneCachingURL or FailedCachingURL.
    //
    // it is important to check the return value of this function. 
    // the method may return HTTP_CACHE_ALREADY_IN_PROGRESS if this url is currently being retrieved. in this case - the caller should wait for the operation to end (WaitForURL)
    virtual HttpCacheReturnCode StartCachingURL(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, std::string &strETag, unsigned long &httpTime);

    // DoneCachingURL:
    // url retrieve operation successfully completed and the local file (returned in  StartCachingURL) is ready for use
    // cacheHeaders may contain relevant cache and expiration headers (e.g. "Expiry", "Etag", "Date" and "Last-Modified")
    // for convenience use "GetUsedReponseHeaders" to get a list of headers used. the last entry in the list is NULL.
    static  const char **GetUsedReponseHeaders();
    virtual void DoneCachingURL(HttpCacheHandle h, const std::string &strUrl, long nHttpRespCode, const HttpCacheHeaders & cacheHeaders);

    // FailedCachingURL:
    // url retrieve operation failed. the local file is not to be used.
    virtual void FailedCachingURL(HttpCacheHandle h, const std::string &strUrl);

    // CancelCachingURL:
    // url retrieve operation cancelled. the local file is untouched.
    virtual void CancelCachingURL(HttpCacheHandle h, const std::string &strUrl);
    
    // WaitForURL:
    // wait nMillis milliseconds for the url to be available in cache.
    // if successful - the local path is returned in strLocalName
    // the local file name is guaranteed to exist at least until the handle is closed
    virtual HttpCacheReturnCode WaitForURL(HttpCacheHandle h, const std::string &strUrl, std::string &strLocalName, int nMillis);
    
    virtual std::string GetCachedName(const std::string &strUrl);
    
    void SetMaxAgeInCache(int nSeconds);
    int  GetMaxAgeInCache();

    void SetDefaultExpiry(int nSeconds);
    int  GetDefaultExpiry();
    
    int  GetCacheDirSize(HttpCacheHandle h);
    void SetMaxCacheSize(int nMaxSize);
    
protected:
    
    void Cleanup();

    std::string      m_localCachePath;
    std::string      m_localCacheDBPath;
    int              m_maxAgeInCache; // maximum time in seconds that an entry can live in cache (after that it is considered expired). overrides any http header values.
    int              m_defaultExpiry; // in seconds, will be set as the "max-age" value if no other cache directive exists
    int              m_nMaxDirSize;
    std::map<std::string, HANDLE> m_mapDoneEvents;
    CCriticalSection m_lock;

    CacheRefMap      m_refMap;
    void UpdateRefMap(const std::string  &strUrl, unsigned long  accessTime, int  refDiff, bool  progressStat = false, bool shouldUpdateProgress = false);
    bool IsUrlInProgress(const std::string  &strUrl);
    bool IsUrlInMap(const std::string  &strUrl);
    int  GetUrlRefCount(const std::string  &strUrl);
    void RemoveFromRefMap(const std::string  &strUrl);
    void ClearRefMap();

private:
    void EnsureCacheSize(HttpCacheHandle h, int64_t newFileSize);
    class HttpCacheManagerCleanup : public BOXEE::BXBGJob
    {
    public:

      HttpCacheManagerCleanup(CHttpCacheManager* cacheHandler);
      virtual ~HttpCacheManagerCleanup();
      virtual void DoWork();

      private:

      CHttpCacheManager* m_cacheHandler;
  };

    BOXEE::BXBGProcess m_scanProcessor;
    unsigned long      m_lastCleanup;


  };
}



#endif

