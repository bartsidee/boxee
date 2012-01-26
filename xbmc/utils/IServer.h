#ifndef ISERVER_H
#define ISERVER_H

class IServer
{
public:
  IServer();
  virtual ~IServer();

  virtual bool Start(int nPort) = 0;
  virtual void Stop() = 0;
  virtual bool IsRunning();
  virtual int GetPort();

protected:
  bool m_bRunning;
  int m_nPort;
};

#endif // ISERVER_H
