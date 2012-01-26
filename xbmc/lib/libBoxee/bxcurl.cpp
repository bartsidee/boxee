
#include <stdio.h>
#include <stdlib.h>

#include "FileSystem/curl/curl.h"
#include "bxcurl.h"
#include "logger.h"
#include "bxexceptions.h"
#include "boxee.h"

#include "StdString.h"
#include "CharsetConverter.h"

#ifdef _LINUX
#include "linux/XFileUtils.h"
#endif

#include "PoolMngr.h"

using namespace std;

#ifdef _WIN32
#define STRCASECMP(X,Y)  stricmp(X,Y)
#else
#define STRCASECMP(X,Y)  strcasecmp(X,Y)
#endif

#ifdef _WIN32
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

namespace BOXEE {

class CurlHandleWrapper
{
public:
  CurlHandleWrapper()
  {
    handle = curl_easy_init();
  }
  
  ~CurlHandleWrapper() 
  {
    curl_easy_cleanup(handle);
  }
  void *handle;
};
  
static CPoolMngr<CurlHandleWrapper> sHttpConnectionPool(3);

bool BXCurl::m_bInitialized = false;
bool BXCurl::m_bDefaultVerbose = false;
std::string BXCurl::m_globalUserAgent = "boxee (alpha/dev)";

BXCurl::BXCurl() : m_bVerbose(m_bDefaultVerbose), m_headers(0)
{
  Initialize();
}

BXCurl::BXCurl(const BXCurl &src)
{
  Initialize();
  *this = src;
}

BXCurl::~BXCurl()
{
  HttpResetHeaders();
}

const BXCurl &BXCurl::operator=(const BXCurl &src)
{
  m_strUserAgent = src.m_strUserAgent;
  m_credentials = src.m_credentials;
  m_strCredString = src.m_strCredString;
  m_bVerbose = src.m_bVerbose;
  m_headers = 0;
  m_nLastRetCode = 0;

  return *this;
}

void BXCurl::SetDefaultVerbose(bool bVerbose)
{
}

bool BXCurl::Initialize()
{
  if (m_bInitialized)
    return true;
  
  bool bOk = (curl_global_init(CURL_GLOBAL_ALL) == 0);
  if (bOk) {
    curl_version_info_data *pData = curl_version_info(CURLVERSION_NOW);
    if (pData) {
      LOG(LOG_LEVEL_DEBUG,"curl initialized. version <%s>\n",pData->version);
    }
  }
  else {
    LOG(LOG_LEVEL_ERROR,"failed to initialize curl!");
  }

  m_bInitialized = bOk;
  return bOk;
}

void BXCurl::SetUserAgent(const std::string &strUserAgent) {
  m_strUserAgent = strUserAgent;
}

void BXCurl::SetVerbose(bool bOn) {
  m_bVerbose = bOn;
}

void BXCurl::DeInitialize()
{
  if (!m_bInitialized)
    return;

  curl_global_cleanup();
  
  m_bInitialized = false;  
}

std::string BXCurl::GetServerIP()
{
  return m_strServerIP;
}

void BXCurl::FinalizeTransfer(void* curlHandle, bool bSuccess)
{
  if (curlHandle)
  {
    char *ip = NULL;
    curl_easy_getinfo(curlHandle, CURLINFO_PRIMARY_IP, &ip);
    if (ip && *ip)
      m_strServerIP = ip;
  }
  
  XBMC::CHttpCacheManager *cache = Boxee::GetInstance().GetHttpCacheManager();
  if (cache && m_cacheHandle && !m_strCachedUrl.empty())
  {
    if (bSuccess)
    {
      XBMC::HttpCacheHeaders headers;
      const char **ptr = cache->GetUsedReponseHeaders();
      while (ptr && *ptr)
      {
        std::string strHeader = GetHttpHeader(*ptr);
        if (!strHeader.empty())
          headers[*ptr] = strHeader;
        ptr++;
      }
      cache->DoneCachingURL(m_cacheHandle, m_strCachedUrl, m_nLastRetCode, headers);
    }
    else if (!m_strLocalCacheName.empty())
      cache->CancelCachingURL(m_cacheHandle, m_strCachedUrl);
  }
  
  if (cache && m_cacheHandle)
    cache->Close(m_cacheHandle);

  m_cacheHandle = NULL;
  m_strCachedUrl.clear();
  m_strLocalCacheName.clear();
  m_usingCacheUrl = false;
  
  if (  m_headers )
    curl_slist_free_all(m_headers);
  
  m_headers = NULL;
  
  if (curlHandle)
  {
    curl_easy_cleanup(curlHandle);
  }

  curlHandle = NULL;
}

void* BXCurl::InitHttpTransfer(const char *szUrl, bool bUseCache) {
  std::string strUrl = szUrl;  
  m_cacheHandle = NULL;
  unsigned long httpTime=0;
  std::string strEtag;
  m_nLastRetCode = 0;
  m_usingCacheUrl = false;

  XBMC::CHttpCacheManager *cache = Boxee::GetInstance().GetHttpCacheManager();
  if (bUseCache && cache)
  {
    m_cacheHandle = cache->Open();
    if (m_cacheHandle)
    {
      XBMC::HttpCacheReturnCode ret = cache->StartCachingURL(m_cacheHandle, szUrl, m_strLocalCacheName, strEtag, httpTime);
      if (ret == XBMC::HTTP_CACHE_ALREADY_IN_PROGRESS)
      {
        if ( cache->WaitForURL(m_cacheHandle, szUrl, m_strLocalCacheName, 5000) == XBMC::HTTP_CACHE_OK )
          ret = XBMC::HTTP_CACHE_ALREADY_EXISTS;
        else
          ret = cache->StartCachingURL(m_cacheHandle, szUrl, m_strLocalCacheName, strEtag, httpTime);
      }
      
      if (ret == XBMC::HTTP_CACHE_ALREADY_EXISTS)
      {
        char *local = curl_escape(m_strLocalCacheName.c_str(), m_strLocalCacheName.size());
        if (local)
        {
          strUrl = "file://";
          strUrl += local;
          curl_free(local);
        }
        else
        {
          strUrl = "file://"+m_strLocalCacheName;
        }

        m_strLocalCacheName.clear();
        m_strCachedUrl.clear();

        m_usingCacheUrl = true;
      }
      else if (ret != XBMC::HTTP_CACHE_OK)
      {
        m_strLocalCacheName.clear();
        cache->Close(m_cacheHandle);
        m_cacheHandle = NULL;
        m_strCachedUrl.clear();
        strEtag.clear();
        httpTime=0;
      }
      else
        m_strCachedUrl = szUrl;
    }
  }
    
  void* curlHandle = curl_easy_init();

  if ( curl_easy_setopt(curlHandle, CURLOPT_URL, strUrl.c_str()) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for url <%s> failed!", strUrl.c_str());
    FinalizeTransfer(curlHandle);
    return NULL;
  }

  if (m_bVerbose)
    curl_easy_setopt(curlHandle, CURLOPT_VERBOSE, 1);

  if ( curl_easy_setopt(curlHandle, CURLOPT_HEADERFUNCTION, ProcessResponseHeaders) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for header function failed!");
    FinalizeTransfer(curlHandle);
    return NULL;
  }

  m_parseContext.m_parseState = PRE_HEADER;

  // clear the headers (may contain previous response's headers)
  m_parseContext.m_respHeaders.clear();

  // private data pointer for the handle headers function would be the respHeaders array
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEHEADER, &m_parseContext) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for header function (data) failed!");
    FinalizeTransfer(curlHandle);
    return NULL;
  }

  // setup credentials
  if (!m_credentials.GetUserName().empty()) {
    // set user/pass
    m_strCredString = m_credentials.GetUserName() + ":" + m_credentials.GetPassword();
    LOG(LOG_LEVEL_DEBUG,"setting creds to: <%s>", m_strCredString.c_str());
    if ( curl_easy_setopt(curlHandle, CURLOPT_USERPWD, m_strCredString.c_str()) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_USERPWD failed!");
    }

    if ( curl_easy_setopt(curlHandle, CURLOPT_HTTPAUTH, CURLAUTH_ANY) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_HTTPAUTH failed!");
    }

  }
  
  //m_strUserAgent = "Mozilla/5.0 (Macintosh; U; Intel Mac OS X 10.5; en-US; rv:1.9.0.4) Gecko/2008102920 Firefox/3.0.4";
  if (m_strUserAgent.empty())    
  {
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, m_globalUserAgent.c_str());
  }
  else
    curl_easy_setopt(curlHandle, CURLOPT_USERAGENT, m_strUserAgent.c_str());
  
  //m_credentials.SetProxy("127.0.0.1:8080");
  if (!m_credentials.GetProxyAddress().empty())
  {
    curl_easy_setopt(curlHandle, CURLOPT_PROXY, m_credentials.GetProxyAddress().c_str());
  }
  
  curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYPEER, 0);
  curl_easy_setopt(curlHandle, CURLOPT_SSL_VERIFYHOST, 0);
  
  curl_easy_setopt(curlHandle, CURLOPT_NOSIGNAL , 1);
  curl_easy_setopt(curlHandle, CURLOPT_DNS_CACHE_TIMEOUT , 0);
  curl_easy_setopt(curlHandle, CURLOPT_FAILONERROR, 1);

  curl_easy_setopt(curlHandle, CURLOPT_CONNECTTIMEOUT, 10);
  
  /*  
  curl_easy_setopt(curlHandle, CURLOPT_LOW_SPEED_LIMIT, 1);
  curl_easy_setopt(curlHandle, CURLOPT_LOW_SPEED_TIME, 10);
  */
  
  std::string strCookieJar = GetCookieJar();
  curl_easy_setopt(curlHandle, CURLOPT_AUTOREFERER, 1);
  curl_easy_setopt(curlHandle, CURLOPT_FOLLOWLOCATION, 1);
  curl_easy_setopt(curlHandle, CURLOPT_MAXREDIRS, 8);
  curl_easy_setopt(curlHandle, CURLOPT_COOKIEFILE, strCookieJar.c_str()); // enable cookie usage
  curl_easy_setopt(curlHandle, CURLOPT_COOKIEJAR, strCookieJar.c_str()); // enable cookie usage

  // allow zipped responses
  curl_easy_setopt(curlHandle, CURLOPT_ENCODING, "");

  m_headers = curl_slist_append(m_headers, "Accept-Charset: ISO-8859-1,utf-8;q=0.7,*;q=0.7");
  m_headers = curl_slist_append(m_headers, "Accept-Language: en-us,en;q=0.5");
  m_headers = curl_slist_append(m_headers, "Keep-Alive: 300");
  m_headers = curl_slist_append(m_headers, "Connection: keep-alive");
  
  if (!strEtag.empty())
  {
    m_headers = curl_slist_append(m_headers, std::string("If-None-Match: "+strEtag).c_str());
  }
  else if (httpTime > 0)
  {
    curl_easy_setopt(curlHandle, CURLOPT_TIMEVALUE, httpTime);
    curl_easy_setopt(curlHandle, CURLOPT_TIMECONDITION, CURL_TIMECOND_IFMODSINCE);
  }
    
  curl_easy_setopt (curlHandle, CURLOPT_HTTPHEADER, m_headers);
  
  return curlHandle;
}

void BXCurl::HttpResetHeaders() {
  if (m_headers)
    curl_slist_free_all(m_headers);

  m_headers = 0;
}

void BXCurl::HttpSetHeaders(const ListHttpHeaders &listHeaders) {
  for (size_t i=0; i< listHeaders.size(); i++) {
    m_headers = curl_slist_append(m_headers, listHeaders[i].c_str());
  }
}

void BXCurl::SetCredentials(const BXCredentials &cred) 
{
  m_credentials = cred;
}

std::string BXCurl::HttpDelete(const char *szUrl)
{
  void* curlHandle = NULL;
  curlHandle = InitHttpTransfer(szUrl, false);
  
  if(curlHandle == NULL)
  {
    return "";
  }
  
  if ( curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, "DELETE") != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for DELETE failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }
  
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, ProcessStringData) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }
  
  string strResult;
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &strResult) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function (data) failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }
  
  CURLcode retCode = curl_easy_perform(curlHandle);
  
  curl_easy_setopt(curlHandle, CURLOPT_CUSTOMREQUEST, NULL);
  
  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &m_nLastRetCode);
  char strCode[1024];
  memset(strCode, 0, 1024);
  
  curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, strCode);
  if (strCode[0] != '\0')
    m_parseContext.m_strLastRetCode = strCode;
  
  if ( retCode != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_perform failed (%s)!", strCode);
    FinalizeTransfer(curlHandle);
    return "";
  }
  
  FinalizeTransfer(curlHandle);
  return strResult;  
}

std::string BXCurl::HttpGetString(const char *szUrl, bool bUseCache) 
{
  std::string strUrl = szUrl;

  void* curlHandle = NULL;
  curlHandle = InitHttpTransfer(szUrl, bUseCache);

  if(curlHandle == NULL)
  {
    return "";
  }

  bool usingCacheUrl = m_usingCacheUrl;

  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, ProcessStringData) != CURLE_OK ) 
  {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  string strResult;
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &strResult) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function (data) failed!");
        FinalizeTransfer(curlHandle);
    return "";
  }

  // make sure we have no BODY (GET), and that the request type is GET.
  // this is important in the case of handle re-use (first POST and then GET with the
  // same object).
  if ( curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_NOBODY failed!");
        FinalizeTransfer(curlHandle);
    return "";
  }

  long useGet=1;
  if ( curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, &useGet) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_HTTPGET failed!");
        FinalizeTransfer(curlHandle);
    return "";
  }

  char strCode[1024];
  memset(strCode, 0, 1024);
  
  curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, strCode);
  
  CURLcode retCode = curl_easy_perform(curlHandle);
  if (retCode == CURLE_COULDNT_RESOLVE_HOST || retCode == CURLE_COULDNT_CONNECT)
  {
    FinalizeTransfer(curlHandle);
    m_nLastRetCode = 503;
    return "";
  }
  if (retCode == CURLE_OPERATION_TIMEDOUT )
  {
    LOG(LOG_LEVEL_DEBUG,"curl_easy_preform failed - got TIMEOUT!");
    FinalizeTransfer(curlHandle);
    m_nLastRetCode = 408;
    return "";
  }

  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &m_nLastRetCode);

  if(m_nLastRetCode == 401)
  {
    FinalizeTransfer(curlHandle);
    return "";
  }

  if (strCode[0] != '\0')
    m_parseContext.m_strLastRetCode = strCode;

  if ( retCode != CURLE_OK ) {
    LOG(m_nLastRetCode == 404?LOG_LEVEL_DEBUG:LOG_LEVEL_ERROR,"curl_easy_perform failed (%s)!", strCode);
    FinalizeTransfer(curlHandle);
    return "";
  }

  if(m_nLastRetCode == 304)
  {
    // use cached copy
    if (!m_strLocalCacheName.empty())
    {
      FILE *fin = fopen(m_strLocalCacheName.c_str(), "rb");
      if (fin)
      {
        strResult.clear();
        char buf[16*1024];
        size_t ret=0;
        while ( (ret = fread(buf, 1, sizeof(buf)-1, fin)) > 0 )
          strResult.append(buf, ret);
        fclose(fin);
      }
      else
      {
        LOG(LOG_LEVEL_ERROR, "cant open local cache file %s",  m_strLocalCacheName.c_str());
        XBMC::CHttpCacheManager *cache = Boxee::GetInstance().GetHttpCacheManager();
        if (cache)
          cache->FailedCachingURL(m_cacheHandle, m_strCachedUrl);
        return "";
      }
      m_strLocalCacheName.clear(); // clearly no need to save...
    }
    else
    {
      LOG(LOG_LEVEL_ERROR, "missing cache item for %s", szUrl);
      XBMC::CHttpCacheManager *cache = Boxee::GetInstance().GetHttpCacheManager();
      if (cache)
        cache->FailedCachingURL(m_cacheHandle, m_strCachedUrl);
      return "";
    }
  }
  
  if (!m_strLocalCacheName.empty())
  {
    FILE *fout = fopen(m_strLocalCacheName.c_str(), "w");
    if (!fout)
    {
      LOG(LOG_LEVEL_DEBUG,"cant open local cache file (%s)!", m_strLocalCacheName.c_str());
      FinalizeTransfer(curlHandle, false);
      return strResult;
    }
    
    if (fwrite(strResult.c_str(), 1, strResult.size(), fout) != strResult.size())
    {
      LOG(LOG_LEVEL_DEBUG,"failed to write string result to cache file (%s)!", m_strLocalCacheName.c_str());
      FinalizeTransfer(curlHandle, false);
      fclose(fout);
      return strResult;
    }

    fclose(fout);
  }

  FinalizeTransfer(curlHandle, true);

  if ((m_nLastRetCode == 0) && usingCacheUrl)
  {
    LOG(LOG_LEVEL_DEBUG,"Since [LastRetCode=%ld][usingCacheUrl=%d] going to update LastRetCode to 304. [url=%s] (bxcurl)",m_nLastRetCode,usingCacheUrl,strUrl.c_str());
    m_nLastRetCode = 304;
  }

  return strResult;

}

bool BXCurl::HttpHEAD(const char *szUrl) {

  void* curlHandle = NULL;
  curlHandle = InitHttpTransfer(szUrl, false);

  if(curlHandle == NULL)
  {
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, ProcessStringData) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function failed!");
    FinalizeTransfer(curlHandle);
    return false;
  }

  string strResult;
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &strResult) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function (data) failed!");
    FinalizeTransfer(curlHandle);
    return false;
  }

  // send a HEAD request
  if ( curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_NOBODY failed!");
    FinalizeTransfer(curlHandle);
    return false;
  }

  CURLcode retCode = curl_easy_perform(curlHandle);

  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &m_nLastRetCode);
  char strCode[1024];
  memset(strCode, 0, 1024);

  curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, strCode);
  if (strCode[0] != '\0')
    m_parseContext.m_strLastRetCode = strCode;

  if ( retCode != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_perform failed (%s)!", strCode);
    FinalizeTransfer(curlHandle);
    return false;
  }

  FinalizeTransfer(curlHandle);
  return true;
}

bool BXCurl::HttpDownloadFile(const char *szUrl, const char *szTargetFile, const std::string &strPostData, bool bUseCache ) {

  if (!szUrl || !szTargetFile)
    return false;

  std::string strSource = szUrl;
  std::string strDest = szTargetFile;

  if (strSource.find("file://") == 0)
  {
    strSource = strSource.substr(7);
    if (strSource[0] == '/')
      strSource = strSource.substr(1);

    FILE *fIn = fopen_utf8(strSource.c_str(), "rb");
    FILE *fOut = fopen_utf8(szTargetFile, "wb");

    bool bOk = (fIn && fOut);
    if (bOk)
    {
      char chunk[4096];
      size_t b;
      while ( (b = fread(chunk,1,4096,fIn)) > 0 )
        fwrite(chunk, 1, b, fOut);
    }

    if (fIn)
      fclose(fIn);

    if (fOut)
      fclose(fOut);

    if (bOk)
      return true;
  }

  void* curlHandle = NULL;
  curlHandle = InitHttpTransfer(szUrl, bUseCache);

  if(curlHandle == NULL)
  {
    return "";
  }

  if (strPostData.empty())
  {
    // make sure we have no BODY (GET), and that the request type is GET.
    // this is important in the case of handle re-use (first POST and then GET with the
    // same object).
    if ( curl_easy_setopt(curlHandle, CURLOPT_NOBODY, 1) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_NOBODY failed!");
      FinalizeTransfer(curlHandle);
      return false;
    }

    long useGet=1;
    if ( curl_easy_setopt(curlHandle, CURLOPT_HTTPGET, &useGet) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for CURLOPT_HTTPGET failed!");
      FinalizeTransfer(curlHandle);
      return false;
    }
  }
  else
  {
    if ( curl_easy_setopt(curlHandle, CURLOPT_POST, 1) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POST failed!");
      FinalizeTransfer(curlHandle);
      return false;
    }

    if ( curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, strPostData.c_str()) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POSTFIELDS failed!");
      FinalizeTransfer(curlHandle);
      return false;
    }

    if ( curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, strPostData.length()) != CURLE_OK ) {
      LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POSTFIELDSIZE failed!");
      FinalizeTransfer(curlHandle);
      return false;
    }
  }

  FILE *fOut = fopen_utf8(szTargetFile, "wb");
  if (!fOut)
    {
      FinalizeTransfer(curlHandle);
      return false;
    }

  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, ProcessFileDesc) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function failed!");
        FinalizeTransfer(curlHandle);
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, fOut) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function (data) failed!");
    fclose(fOut);
    FinalizeTransfer(curlHandle);
    return false;
  }

  char strCode[1024];
  memset(strCode, 0, 1024);
  curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, strCode);

  CURLcode retCode = curl_easy_perform(curlHandle);

  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &m_nLastRetCode);
  if (strCode[0] != '\0')
    m_parseContext.m_strLastRetCode = strCode;

  if ( retCode != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_perform failed (%s)!", strCode);
    fclose(fOut);
    FinalizeTransfer(curlHandle);
    return false;
  }

  fclose(fOut);

  if (m_nLastRetCode == 304)
  {
    if (!::CopyFile(m_strLocalCacheName.c_str(), szTargetFile, false))
    {
      FinalizeTransfer(curlHandle, false);
      return true;
    }    
  }
  else if (!m_strLocalCacheName.empty())
  {
    if (!::CopyFile(szTargetFile, m_strLocalCacheName.c_str(), false))
    {
      FinalizeTransfer(curlHandle, false);
      return true;
    }
  }
  
  FinalizeTransfer(curlHandle, true);
  return true;
}

std::string BXCurl::HttpPostString(const char *szUrl, const std::string &strPostData ) {

  void* curlHandle = NULL;
  curlHandle = InitHttpTransfer(szUrl, false);

  if(curlHandle == NULL)
  {
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_POST, 1) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POST failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDS, strPostData.c_str()) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POSTFIELDS failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_POSTFIELDSIZE, strPostData.length()) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for POSTFIELDSIZE failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEFUNCTION, ProcessStringData) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  string strResult;
  if ( curl_easy_setopt(curlHandle, CURLOPT_WRITEDATA, &strResult) != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_setopt for write function (data) failed!");
    FinalizeTransfer(curlHandle);
    return "";
  }

  CURLcode retCode = curl_easy_perform(curlHandle);

  curl_easy_getinfo(curlHandle, CURLINFO_RESPONSE_CODE, &m_nLastRetCode);
  char strCode[1024];
  memset(strCode, 0, 1024);

  curl_easy_setopt(curlHandle, CURLOPT_ERRORBUFFER, strCode);
  if (strCode[0] != '\0')
    m_parseContext.m_strLastRetCode = strCode;

  if ( retCode != CURLE_OK ) {
    LOG(LOG_LEVEL_ERROR,"curl_easy_perform failed (%s)!", strCode);
    FinalizeTransfer(curlHandle);
    return "";
  }

  FinalizeTransfer(curlHandle);
  return strResult;
}

size_t BXCurl::ProcessStringData(void *pBuffer, size_t nSize, size_t nmemb, void *userp) {
  if (!userp) {
    LOG(LOG_LEVEL_ERROR,"invalid user pointer in callback, terminating transaction");
    return 0;
  }
  
  if (!BOXEE::Boxee::GetInstance().IsRunning())
    return 0;

  string *str = (string *)userp;
  str->append((char *)pBuffer,nSize * nmemb);

  return nSize * nmemb;
}

size_t BXCurl::ProcessFileDesc(void *pBuffer, size_t nSize, size_t nmemb, void *userp) {
  if (!userp) {
    LOG(LOG_LEVEL_ERROR,"invalid user pointer in callback, terminating transaction");
    return 0;
  }

  if (!BOXEE::Boxee::GetInstance().IsRunning())
    return 0;

  FILE *fd = (FILE *)userp;
  int nCount = fwrite(pBuffer, nSize, nmemb, fd);
  if (nCount == nmemb)
    return nSize * nmemb;

  LOG(LOG_LEVEL_ERROR,"failed to write %d bytes from file desc callback (curl)", nSize * nmemb);
  return 0;
}

std::string BXCurl::GetLastRetCodeString() {
  return m_parseContext.m_strLastRetCode;
}

long    BXCurl::GetLastRetCode() {
  return (m_nLastRetCode==0)?200:m_nLastRetCode;
}

size_t BXCurl::ProcessResponseHeaders( void *pHeader, size_t nLineSize, size_t nmemb, void *userp) {

  HttpParseContext *pContext = (HttpParseContext *)userp;

  if (!pContext) {
    LOG(LOG_LEVEL_ERROR,"no context in parse");
    return 0;
  }

  string strLine((char *)pHeader, nLineSize*nmemb);
  size_t nPos = strLine.find('\r');
  if (nPos != string::npos || (nPos = strLine.find('\n')) > 0)
    strLine.erase(nPos);

  if (pContext->m_parseState == IN_BODY){
    // reset - we should only get here in the case of redirect.
    pContext->m_parseState = PRE_HEADER;
    pContext->m_respHeaders.clear();
  }

  // HTTP header state machine.
  // the first line is the status line.
  // after that - the header (key: value pairs)
  // and an empty line to mark the http body start.
  if (pContext->m_parseState == PRE_HEADER) {

    // the only field in our interest is the last one (space separated) which is the text
    // explaining the code.
    nPos = strLine.find(' ');
    if (nPos > 0)
      nPos = strLine.find(' ', nPos+1);

    if (nPos > 0)
      pContext->m_strLastRetCode = strLine.substr(nPos+1);

    pContext->m_parseState = IN_HEADER;
  }
  else if (pContext->m_parseState == IN_HEADER) {
    nPos = strLine.find(':');
    if (nPos != string::npos && nPos < strLine.size() - 1) {
      pContext->m_respHeaders.push_back(pair<string,string>(strLine.substr(0,nPos),strLine.substr(nPos+2)));
    }

    if (strLine.empty()) {
      pContext->m_parseState = IN_BODY;
    }
  }
  else {
    LOG(LOG_LEVEL_WARNING, "got header <%s> when in invalid parse state", strLine.c_str());
  }

  return nmemb * nLineSize;
}

int BXCurl::GetHttpHeader(const std::string &strHeader, std::vector<std::string> &vecValues) {

  vecValues.clear();

  for (size_t i=0; i < m_parseContext.m_respHeaders.size(); i++) {
    pair<string,string> header = m_parseContext.m_respHeaders[i];
    if (STRCASECMP(header.first.c_str(), strHeader.c_str()) == 0) {
      vecValues.push_back(header.second);
    }
  }

  return vecValues.size();
}

std::string BXCurl::GetHttpHeader(const std::string &strHeader)
{
  std::vector<std::string> vecValues;
  GetHttpHeader(strHeader,vecValues);
  if (vecValues.size() == 0)
    return "";

  return vecValues[0];
}

std::string BXCurl::GetCookieJar()
{
  std::string cookieJarPath = Boxee::GetInstance().GetTempFolderPath();

  std::string userName = Boxee::GetInstance().GetCredentials().GetUserName();
  if (!userName.empty())
  {
    cookieJarPath += userName;
    cookieJarPath += "-";
  }

  cookieJarPath += "boxee-cookies.dat";

  return cookieJarPath;
}
  
const void BXCurl::SetGlobalUserAgent(const std::string &strAgent)
{
  m_globalUserAgent = strAgent;  
}

const std::string& BXCurl::GetGlobalUserAgent()
{
  return m_globalUserAgent;
}

void BXCurl::DeleteCookieJarFile()
{
  std::string strCookieJar = GetCookieJar();

  if(!strCookieJar.empty())
  {
    remove(strCookieJar.c_str());
  }  
}

} // namespace BOXEE

