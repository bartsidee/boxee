#include "XAPP_Http.h"
#include "FileCurl.h"
#include "bxcurl.h"
#include "AppManager.h"

#include <algorithm>

#define APP_ID_HTTP_HEADER "x-boxee-app-id"

namespace XAPP
{
  Http::Http()
  {
    Reset();
  }
  
  Http::~Http()
  {
  }
  
  void Http::SetHttpHeader(const std::string &strKey, const std::string &strValue)
  {
    std::string strKeyCopy = strKey;
    std::transform(strKeyCopy.begin(), strKeyCopy.end(), strKeyCopy.begin(), ::tolower);

    if (strKeyCopy == APP_ID_HTTP_HEADER)
      return;
    
    m_curl.SetRequestHeader(strKey.c_str(), strValue.c_str());
  }
  
  std::string Http::GetHttpHeader(const std::string &strKey)
  {
    const CHttpHeader &h = m_curl.GetHttpHeader();
    return h.GetValue(strKey.c_str());
  }
  
  int Http::GetHttpResponseCode()
  {
    int nResp = m_curl.GetLastRetCode();
    if (nResp > 0)
      return nResp;
    
    const CHttpHeader &h = m_curl.GetHttpHeader();
    CStdString proto = h.GetProtoLine();
    size_t pos = proto.Find(" ");
    if (pos != CStdString::npos)
      return atoi(proto.Mid(pos+1));
    
    return 500;
  }
  
  void Http::Reset()
  {
    m_curl.Reset();

    const char *strAppId = CAppManager::GetInstance().GetCurrentContextAppId();
    if (strAppId)
    {
      m_curl.SetRequestHeader(APP_ID_HTTP_HEADER, strAppId);
    }
  }

  void Http::SetUserAgent(const std::string &strUserAgent)
  {
    m_curl.SetUserAgent(strUserAgent.c_str());
  }

  std::string Http::Get(const std::string &strUrl)
  {
    CStdString strResp;
    m_curl.Get(strUrl.c_str(), strResp);
    return strResp.c_str();
  }
  
  std::string Http::Delete(const std::string &strUrl)
  {
    BOXEE::BXCurl curl;
    CStdString strResp = curl.HttpDelete(strUrl.c_str());
    return strResp.c_str();
  }
  
  std::string Http::Post(const std::string &strUrl, const std::string &strPostData)
  {
    CStdString strResp;
    m_curl.Post(strUrl.c_str(), strPostData.c_str(), strResp);
    return strResp.c_str();
  }
  
  bool Http::Download(const std::string &strUrl, const std::string &strLocalPath)
  {
    return m_curl.Download(strUrl.c_str(), strLocalPath.c_str());
  }
  
}

