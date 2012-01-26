#include "HttpServer.h"
#include "log.h"
#include "StdString.h"
#include "system.h"
#include "Application.h"
#include "GUIInfoManager.h"
#include "SystemInfo.h"
#include <stdlib.h>

static int HeadersIterator(void* cls,enum MHD_ValueKind kind,const char* key, const char* value)
{
  std::map<const char*, const char*>* headerParams = (std::map<const char*, const char*>*)cls;
  (*headerParams)[key] = value;

  return MHD_YES;
}

class CReaderCallback
{
public:
  CReaderCallback(const CStdString& deviceId, const CStdString& url) { done = false; m_deviceId = deviceId; m_url = url;}

  IHttpChunkedResponse* chunkedCB;
  bool done;
  std::vector<unsigned char> remaining;
  CStdString m_deviceId;
  CStdString m_url;
};

static ssize_t readerCallback (void *a_cls, uint64_t pos, char *buf, size_t max)
{
  CReaderCallback* cls = (CReaderCallback*) a_cls;
  if (!cls)
  {
    return -2; // End of stream with error
  }

  if (cls->done)
  {
    delete cls;
    return -1; // Done!
  }

  if (!cls->chunkedCB)
  {
    delete cls;
    return -2; // End of stream with error
  }

  // handle long responses, which needs to be split into shorter buffers
  if (cls->remaining.size() > 0)
  {
    snprintf(buf, max, "%s", &cls->remaining[0]);
    size_t toRemove = cls->remaining.size() < max ? cls->remaining.size() : max;
    cls->remaining.erase(cls->remaining.begin(), cls->remaining.begin() + toRemove);
    return strlen(buf);
  }

  std::vector<unsigned char> data;
  cls->chunkedCB->GetChunkedResponse(cls->m_deviceId,cls->m_url,data);

  // Make sure we got a response, if not, it's the end..
  if (data.size() > 0)
  {
    // If the response is longer than max (+ length + \r\n), loop and send it in chunks
    if (data.size() + 10 > max)
    {
      cls->remaining = data;
      cls->remaining.push_back('\r');
      cls->remaining.push_back('\n');
      sprintf(buf, "%X\r\n", (unsigned int) data.size());
    }
    else
    {
      snprintf(buf, max, "%X\r\n%s\r\n", (unsigned int) data.size(), &data[0]);
    }
  }
  else
  {
    sprintf(buf, "0\r\n\r\n");
    cls->done = true;
  }

  return strlen(buf);
}

static int OnHttpRequest(void* cls,
                         struct MHD_Connection* connection,
                         const char* url,
                         const char* method,
                         const char* version,
                         const char* upload_data,
                         size_t* upload_data_size,
                         void** ptr)
{
  if (!*ptr)
  {
    *ptr = new std::string;
    return MHD_YES;
  }

  std::string* str = (std::string*) *ptr;

  if (strcmp(method, "POST") == 0 && *upload_data_size)
  {
    *upload_data_size = 0;
    (*str) += upload_data;
    return MHD_YES;
  }

  CLog::Log(LOGDEBUG,"OnHttpRequest - enter function [url=%s][method=%s][version=%s][upload_data=%s][upload_data_size=%lu] (hsrv)",url,method,version,upload_data,*upload_data_size);

  CHttpServer* httpServer = (CHttpServer*)cls;
  if (!httpServer)
  {
    CLog::Log(LOGERROR,"OnHttpRequest - FAILED to get server context (hsrv)");
    return MHD_NO;
  }

  MAPHTTPHEADER header;
  MHD_get_connection_values(connection, MHD_HEADER_KIND, HeadersIterator, &header);

#if 0
  std::map<char*, char*>::const_iterator end = headerParams.end();
  for (std::map<char*, char*>::const_iterator it = headerParams.begin(); it != end; ++it)
  {
      std::cout << "Key: " << it->first;
      std::cout << "Value: " << it->second << '\n';
  }
#endif

  CHttpRequest httpRequest((char*) method, (char*) version, (char*) url, header, str->size(), (char*) str->c_str());
  CHttpResponse httpResponse;

  CLog::Log(LOGDEBUG,"OnHttpRequest - BEFORE HandleRequest with [method=%s][version=%s][url=%s][str=%s][strSize=%lu] (hsrv)",method,version,url,str->c_str(),str->size());
  httpServer->HandleRequest(httpRequest, httpResponse);
  CLog::Log(LOGDEBUG,"OnHttpRequest - AFTER HandleRequest with [method=%s][version=%s][url=%s][str=%s][strSize=%lu]. [IsChunked=%d] (hsrv)",method,version,url,str->c_str(),str->size(),httpResponse.IsChunked());

  struct MHD_Response* response;

  if (httpResponse.IsChunked())
  {
    CStdString deviceId = "";
    IMAPHTTPHEADER it = header.find("X-Boxee-Device-ID");
    if (it == header.end())
    {
      CLog::Log(LOGWARNING,"OnHttpRequest - FAILED to get X-Boxee-Device-ID from header for chunked request (hsrv)");
    }
    else
    {
      deviceId = it->second;
    }

    CReaderCallback* readerCls = new CReaderCallback(deviceId, CStdString(url));
    readerCls->chunkedCB = httpResponse.GetChunkedCallback();

    CLog::Log(LOGDEBUG,"OnHttpRequest - going to response_from_callback. [method=%s][version=%s][url=%s][str=%s][strSize=%lu]. [IsChunked=%d] (hsrv)",method,version,url,str->c_str(),str->size(),httpResponse.IsChunked());

    response = MHD_create_response_from_callback(MHD_SIZE_UNKNOWN, 65535, readerCallback, readerCls, NULL);

    if (MHD_add_response_header(response, "Transfer-Encoding", "chunked") == MHD_NO)
    {
      CLog::Log(LOGERROR,"OnHttpRequest - FAILED to add server to header");
      return MHD_NO;
    }
  }
  else
  {
    unsigned char* bodyPtr = &(httpResponse.GetBody()[0]);
    response = MHD_create_response_from_buffer(httpResponse.GetBody().size(), bodyPtr, MHD_RESPMEM_MUST_COPY);
  }

  if (!response)
  {
    CLog::Log(LOGERROR,"OnHttpRequest - FAILED to create response. [%s] (hsrv)",method);
    return MHD_NO;
  }

  if (MHD_add_response_header(response, "Server", httpServer->GetServerAgent().c_str()) == MHD_NO)
  {
    CLog::Log(LOGERROR,"OnHttpRequest - FAILED to add server to header");
    return MHD_NO;
  }

  std::map<std::string, std::string>::iterator it = httpResponse.GetHeader().begin();
  while (it != httpResponse.GetHeader().end())
  {
    if (MHD_add_response_header(response, it->first.c_str(), it->second.c_str()) == MHD_NO)
    {
      CLog::Log(LOGERROR,"OnHttpRequest - FAILED to add response header [%s=%s] (hsrv)", it->first.c_str(), it->second.c_str());
    }

    it++;
  }

  CLog::Log(LOGDEBUG,"OnHttpRequest - going to queue response. [code=%d][bodySize=%lu] (hsrv)",httpResponse.GetCode(), httpResponse.GetBody().size());

  int retVal = MHD_YES;
  if (MHD_queue_response(connection, httpResponse.GetCode(), response) != MHD_YES)
  {
    CLog::Log(LOGERROR,"OnHttpRequest - FAILED to queue response. [%s] (hsrv)",method);
    retVal = MHD_NO;
  }

  MHD_destroy_response(response);

  delete str;

  return retVal;
}

CHttpServer::CHttpServer()
{
  m_serverAgent = "BOXEE/";
  m_serverAgent += g_infoManager.GetVersion().c_str();
  m_serverAgent += " (";
  m_serverAgent += g_sysinfo.GetKernelVersion();
  m_serverAgent += ")";
  m_httpServer = NULL;
}

bool CHttpServer::Start(int nPort)
{
  CLog::Log(LOGDEBUG,"CHttpServer::Start - enter function. [port=%d] (hsrv)",nPort);

  m_nPort = nPort;

  CLog::Log(LOGDEBUG,"CHttpServer::Start - after set [port=%d] (hsrv)",m_nPort);

  static char rnd[8];
  for (int i=0; i<8;i++)
  {
    rnd[i]=((uint)rand() % 256);
  }

  m_httpServer = MHD_start_daemon(MHD_USE_THREAD_PER_CONNECTION, m_nPort, NULL, NULL,
                                OnHttpRequest, this,
                                MHD_OPTION_DIGEST_AUTH_RANDOM, sizeof(rnd), rnd,
                                MHD_OPTION_NONCE_NC_SIZE, 300,
                                MHD_OPTION_END);
  if (!m_httpServer)
  {
    CLog::Log(LOGERROR,"CHttpServer::Start - FAILED to start server. [port=%d] (hsrv)",m_nPort);
    return false;
  }

  CLog::Log(LOGDEBUG,"CHttpServer::Start - successfully started the server. [port=%d] (hsrv)",m_nPort);

  return true;
}

void CHttpServer::Stop()
{
  CLog::Log(LOGDEBUG,"CHttpServer::Stop - enter function (hsrv)");

  MHD_stop_daemon(m_httpServer);
}

