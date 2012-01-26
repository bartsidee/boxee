// Copyright © 2008 BOXEE. All rights reserved.
/*
* libBoxee
*
*
*
* --- property of boxee.tv
*
*/
#ifndef BOXEEBXCURL_H
#define BOXEEBXCURL_H

#include <string>
#include <map>
#include <vector>

#include "HttpCacheManager.h"
#include "bxcredentials.h"
#include "FileSystem/DllLibCurl.h"

//struct curl_slist;

namespace BOXEE {

typedef std::vector<std::string> ListHttpHeaders;

/**
wrapper for libcurl.
this class will be used for http/ftp operations
*/

class BXCurl{
public:
	BXCurl();
	BXCurl(const BXCurl &src);
	virtual ~BXCurl();

	const BXCurl &operator=(const BXCurl &src);

	// simple Http GET operation for string document.
	// if return string is empty - something went wrong. check last return code (GetLastRetCode)
	std::string HttpGetString(const char *szUrl, bool bUseCache = true);

  // Http DELETE operation (DELETE verb).
	std::string HttpDelete(const char *szUrl);
  
    // attempt to read url's headers (can be used to test comm)
	bool HttpHEAD(const char *szUrl);

	// simple Http POST operation for string document.
	// if return string is empty - something went wrong. check last return code (GetLastRetCode)
	std::string HttpPostString(const char *szUrl, const std::string &strPostData );

	// Http download a file
	bool HttpDownloadFile(const char *szUrl, const char *szTargetFile, const std::string &strPostData, bool bUseCache = true);

	// set additional headers to the HTTP request.
	// the list is a vector of strings in the format of: "Header: Value"
	// any item on the headers list will be added to the already existing list of headers (therefore, this method can be called several times before the request).
	// call HttpResetHeaders to remove all previously headed headers.
	void HttpSetHeaders(const ListHttpHeaders &listHeaders);

	// clear the headers list
	void HttpResetHeaders();

	// http credentials for auth
	void SetCredentials(const BXCredentials &cred);

	void SetUserAgent(const std::string &strUserAgent);

	// error code for last operation
	std::string GetLastRetCodeString();
	long		GetLastRetCode();

	// query header value (from last response's http headers)
	// the output is a string vector since the same header may have appeared more than once
	// example: GetHttpHeader("Cookie") will return a list of values used for this header in the http response.
	// the "int" return value is the number of items in the output vector.
	int GetHttpHeader(const std::string &strHeader, std::vector<std::string> &vecValues);

	// return the first matching header
	std::string GetHttpHeader(const std::string &strHeader);

  // get the last connection's server ip
  std::string GetServerIP();
  
	// SetVerbose will toggle excessive logging (on/off)
	void SetVerbose(bool bOn);

	static bool Initialize();
	static void DeInitialize();
  static void SetDefaultVerbose(bool bVerbose);

	// in order to keep cookies across requests we need a file - cookie jar.
	// this method will return the file used by boxee.
	static std::string GetCookieJar();
	
	// set the default user agent for requests
	static void SetGlobalUserAgent(const std::string &strAgent);
	static const std::string& GetGlobalUserAgent();
	
	// Delete CookieJar file
	static void DeleteCookieJarFile(const std::string& userName = "");
	
protected:

	// setup for HTTP request
    void* InitHttpTransfer(const char *szUrl, bool bUseCache=true);
    void FinalizeTransfer(void* curlHandle, bool bSuccess=false);

	// ProcessStringData is a callback function, called by curl whenever a chunk of data is read from the
	// response.
	// we will use this callback for string data, simply concatenating the chunks.
	// the "user pointer" userp should hold a pointer to std::string.
	static size_t ProcessStringData(void *pBuffer, size_t nSize, size_t nmemb, void *userp);

	// ProcessFileDesc is a callback function, called by curl whenever a chunk of data is read from the
	// response.
	// we will use this callback for data written into a file (file desc)
	static size_t ProcessFileDesc(void *pBuffer, size_t nSize, size_t nmemb, void *userp);

	// ProcessResponseHeaders will be called by curl for every http header in the response
	static size_t ProcessResponseHeaders( void *pHeader, size_t nLineSize, size_t nmemb, void *userp);

	std::string 		m_strUserAgent;
	BXCredentials		m_credentials;
	std::string			m_strCredString;
	std::string     m_strProxyCred;
	bool				    m_bVerbose;
	struct curl_slist 	*m_headers;
	long				    m_nLastRetCode;

	// when parsing an HTTP response, the headers will be saved in a vector of key/value pairs
	typedef std::pair<std::string, std::string> HttpHeaderPair;
	typedef std::vector<HttpHeaderPair> 		    HttpHeaders;

	typedef enum {PRE_HEADER, IN_HEADER, IN_BODY} ParseState;

	// the parse will keep calling a static callback. a user defined param will be passed to the callback
	// containing the context of the parse action.
	// the context is defined by HttpParseContext and contains the header parse state and the headers array
	typedef struct __HttpParseContext {
		ParseState	m_parseState;
		HttpHeaders m_respHeaders;
		std::string	m_strLastRetCode;

		__HttpParseContext() : m_parseState(PRE_HEADER) {}
	} HttpParseContext;

	HttpParseContext	m_parseContext;

  XBMC::HttpCacheHandle m_cacheHandle;
  std::string           m_strLocalCacheName;
  std::string           m_strCachedUrl;
  bool                  m_usingCacheUrl;
  std::string           m_strServerIP;
  
	static bool m_bInitialized;
	static bool m_bDefaultVerbose;
  static std::string m_globalUserAgent;
};

}

#endif
