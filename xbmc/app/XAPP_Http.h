#ifndef XAPP_HTTP_H_
#define XAPP_HTTP_H_

#include <string>
#include <map>

#include "FileCurl.h"

namespace XAPP
{

/**
 * This class represents the Http object 
 */
class Http
{
public:
  
  /**
   * Creates a new http object
   */
  Http();
  virtual ~Http();
  
  /**
   * set a request http header 
   * @param strKey header name
   * @param strValue header value
   */  
  void SetHttpHeader(const std::string &strKey, const std::string &strValue);

  /**
   * retrieves an http header from the reqponse 
   * @param strKey header name
   */  
  std::string GetHttpHeader(const std::string &strKey);

  /**
   * get the http response code from the operation
   */  
  int GetHttpResponseCode();
  
  /**
   * perform an HTTP GET request
   * @param strUrl the url to GET
   */  
  std::string Get(const std::string &strUrl);

  /**
   * perform an HTTP DELETE request (verb - DELETE)
   * @param strUrl the url to DELETE
   */  
  std::string Delete(const std::string &strUrl);
  
  /**
   * perform an HTTP POST request
   * @param strUrl the url to POST to
   * @param strPostData the data (must be textual) to post in the request
   */  
  std::string Post(const std::string &strUrl, const std::string &strPostData);

  /**
   * download a file
   * @param strUrl the url of the file to download
   * @param strLocalPath the path to save the file to
   */  
  bool        Download(const std::string &strUrl, const std::string &strLocalPath);
  
  /**
   * set the user-agent field of the http request
   * @param strUserAgent the user-agent value to use
   */  
  void SetUserAgent(const std::string &strUserAgent);
  
  /**
   * reset the object - all data will re-initialize
   */
  void Reset();
  
private:
  XFILE::CFileCurl m_curl;  
};

}

#endif
