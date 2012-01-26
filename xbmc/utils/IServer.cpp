#include "IServer.h"

IServer::IServer()
{
  m_bRunning = false;
  m_nPort = -1;
}

IServer::~IServer()
{
  
}

bool IServer::IsRunning()
{
  return m_bRunning;
}

int IServer::GetPort()
{
  return m_nPort;
}
