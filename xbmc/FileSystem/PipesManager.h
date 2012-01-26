#ifndef __PIPES_MANAGER__H__
#define __PIPES_MANAGER__H__

#include "utils/CriticalSection.h"
#include "utils/Event.h"
#include "StdString.h"
#include "RingBuffer.h"

#include <map>

#define PIPE_DEFAULT_MAX_SIZE (6 * 1024 * 1024)

namespace XFILE
{
 
class IPipeListener
{
public:
  virtual ~IPipeListener() {}
  virtual void OnPipeOverFlow() = 0;
  virtual void OnPipeUnderFlow() = 0;
};
  
class Pipe
  {
  public:
    Pipe(const CStdString &name, int nMaxSize = PIPE_DEFAULT_MAX_SIZE ); 
    virtual ~Pipe();
    const CStdString &GetName();
    
    void AddRef();
    void DecRef();   // a pipe does NOT delete itself with ref-count 0. 
    int  RefCount(); 
    
    bool IsEmpty();
    int  Read(char *buf, int nMaxSize, int nWaitMillis);
    bool Write(const char *buf, int nSize, int nWaitMillis);

    void Flush();
    
    void CheckStatus();
    void Close();
    
    void AddListener(IPipeListener *l);
    void RemoveListener(IPipeListener *l);
    
    void SetEof();
    bool IsEof();
    
    int	GetAvailableRead();
    void SetOpenThreashold(int threashold);

  protected:
    
    bool        m_bOpen;
    bool        m_bReadyForRead;

    bool        m_bEof;
    CRingBuffer m_buffer;
    CStdString  m_strPipeName;  
    int         m_nRefCount;
    int         m_nOpenThreashold;

    CEvent     m_readEvent;
    CEvent     m_writeEvent;
    
    std::vector<XFILE::IPipeListener *> m_listeners;
    
    CCriticalSection m_lock;
  };

  
class PipesManager
{
public:
  virtual ~PipesManager();
  static PipesManager &GetInstance();

  CStdString   GetUniquePipeName();
  XFILE::Pipe *CreatePipe(const CStdString &name="", int nMaxPipeSize = PIPE_DEFAULT_MAX_SIZE);
  XFILE::Pipe *OpenPipe(const CStdString &name);
  void         ClosePipe(XFILE::Pipe *pipe);
  bool         Exists(const CStdString &name);
  
protected:
  PipesManager();
  int    m_nGenIdHelper;
  std::map<CStdString, XFILE::Pipe *> m_pipes;  
  
  CCriticalSection m_lock;
};

}

#endif

