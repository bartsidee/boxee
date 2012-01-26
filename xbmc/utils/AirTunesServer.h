#pragma once

#include "system.h"

#ifdef HAS_AIRTUNES

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

class CAirTunesServer : public CThread
{
public:
  static bool StartServer(int port, bool nonlocal);
  static void StopServer(bool bWait);

protected:
  void Process();

private:
  CAirTunesServer(int port, bool nonlocal);
  bool Initialize();
  void Deinitialize();

  int m_port;
  static CAirTunesServer *ServerInstance;
  static CStdString m_macAddress;
};

#endif
