#ifndef HTTPSERVER_H
#define HTTPSERVER_H

#include "IServer.h"
#include "StdString.h"
#include "lib/libmicrohttpd/microhttpd.h"
#include "string.h"

#include <map>

struct cmp_str
{
   bool operator()(char const *a, char const *b)
   {
      return strcmp(a, b) < 0;
   }
};

typedef std::map<char*, char*, cmp_str> MAPHTTPHEADER;
typedef std::map<char*, char*, cmp_str>::iterator IMAPHTTPHEADER;

class CHttpRequest
{
public:
  CHttpRequest(char* method, char* version, char* uri, MAPHTTPHEADER& header, size_t bodySize, char* body)
  {
    m_method = method;
    m_version = version;
    m_uri = uri;
    m_header = header;
    m_bodySize = bodySize;
    m_body = body;
  }

  char* GetMethod()   { return m_method; }
  char* GetVersion()  { return m_version; }
  char* GetUri()      { return m_uri; }
  MAPHTTPHEADER& GetHeader()   { return m_header; }
  size_t GetBodySize() { return m_bodySize; }
  char* GetBody()     { return m_body; }

private:
  char* m_method;
  char* m_version;
  char* m_uri;
  MAPHTTPHEADER m_header;
  size_t m_bodySize;
  char* m_body;
};

class IHttpChunkedResponse
{
public:
  virtual ~IHttpChunkedResponse() {}

  // This needs to be implemented. Will be called for every chunk query.
  virtual void GetChunkedResponse(const CStdString& deviceId, const CStdString& url, std::vector<unsigned char>& data) = 0;
};

class CHttpResponse
{
public:
  CHttpResponse()
  {
    m_code = 500;
    m_isChunked = false;
    m_chunkedCb = NULL;
  }

  void SetCode(int code) { m_code = code; }
  void SetHeader(std::map<std::string, std::string> header) { m_header = header; }
  void SetBody(std::vector<unsigned char> body) { m_body = body; }
  void SetChunked(bool isChunked) { m_isChunked = isChunked; }
  void SetChunkedCallback(IHttpChunkedResponse* cb) { m_chunkedCb = cb; }

  int GetCode() { return m_code; }
  std::map<std::string, std::string>& GetHeader() { return m_header; }
  std::vector<unsigned char>& GetBody() { return m_body; }
  bool IsChunked() { return m_isChunked; }
  IHttpChunkedResponse* GetChunkedCallback() { return m_chunkedCb; }

private:
  int m_code;
  std::map<std::string, std::string> m_header;
  std::vector<unsigned char> m_body;
  bool m_isChunked;
  IHttpChunkedResponse* m_chunkedCb;
};

class CHttpServer : public IServer
{
public:
  CHttpServer();
  bool Start(int port);
  void Stop();
  std::string GetServerAgent() { return m_serverAgent; }

  virtual void HandleRequest(CHttpRequest& request, CHttpResponse& response) = 0;

private:
  std::string m_serverAgent;
  MHD_Daemon* m_httpServer;
};

#endif // HTTPSERVER_H
