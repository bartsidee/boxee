#pragma once

#include "system.h"

#ifdef HAS_AIRPLAY

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string>
#include <vector>
#include "Thread.h"
#include "CriticalSection.h"
#include "HttpParser.h"
#include "StdString.h"

class CAirPlayServer : public CThread
{
public:
  static bool StartServer(int port, bool nonlocal);
  static void StopServer(bool bWait);

protected:
  void Process();

private:
  CAirPlayServer(int port, bool nonlocal);
  bool Initialize();
  void Deinitialize();

  class CTCPClient
  {
  public:
    CTCPClient();
    //Copying a CCriticalSection is not allowed, so copy everything but that
    //when adding a member variable, make sure to copy it in CTCPClient::Copy
    CTCPClient(const CTCPClient& client);
    CTCPClient& operator=(const CTCPClient& client);
    void PushBuffer(CAirPlayServer *host, const char *buffer, int length);
    void Disconnect();

    int m_socket;
    struct sockaddr m_cliaddr;
    socklen_t m_addrlen;
    CCriticalSection m_critSection;

  private:
    int ProcessRequest(CStdString& responseHeader, CStdString& response);
    void Copy(const CTCPClient& client);
    HttpParser* m_httpParser;
  };

  std::vector<CTCPClient> m_connections;
  int m_ServerSocket;
  int m_port;
  bool m_nonlocal;

  static CAirPlayServer *ServerInstance;
};

#endif
