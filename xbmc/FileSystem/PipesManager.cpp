#include "PipesManager.h"
#include "utils/SingleLock.h"

using namespace XFILE;

Pipe::Pipe(const CStdString &name, int nMaxSize)
{
  m_buffer.Create(nMaxSize);
  m_nRefCount = 1;
  m_readEvent.Reset();
  m_writeEvent.Set();
  m_strPipeName = name;
  m_bOpen = true;
  m_bEof = false;
}

Pipe::~Pipe()
{
}

const CStdString &Pipe::GetName() 
{
  return m_strPipeName;
}

void Pipe::AddRef()
{
  CSingleLock lock(m_lock);
  m_nRefCount++;
}

void Pipe::DecRef()
{
  CSingleLock lock(m_lock);
  m_nRefCount--;
}

int  Pipe::RefCount()
{
  CSingleLock lock(m_lock);
  return m_nRefCount;
}

void Pipe::SetEof()
{
  m_bEof = true;
}

bool Pipe::IsEof()
{
  return m_bEof;
}

bool Pipe::IsEmpty()
{
  return (m_buffer.GetMaxReadSize() == 0);
}

int  Pipe::Read(char *buf, int nMaxSize, int nWaitMillis)
{
  CSingleLock lock(m_lock);
  
  if (!m_bOpen)
  {
    return -1;
  }
  
  int nResult = 0;
  if (!IsEmpty())
  {
    int nToRead = std::min(m_buffer.GetMaxReadSize(), nMaxSize);
    m_buffer.ReadBinary(buf, nToRead);
    nResult = nToRead;
  }
  else if (m_bEof)
  {
    nResult = 0;
  }
  else
  {
    // we're leaving the guard - add ref to make sure we are not getting erased.
    // at the moment we leave m_listeners unprotected which might be a problem in future
    // but as long as we only have 1 listener attaching at startup and detaching on close we're fine
    AddRef();
    lock.Leave();    
    
    for (size_t l=0; l<m_listeners.size(); l++)
      m_listeners[l]->OnPipeUnderFlow();
    
    bool bHasData = m_readEvent.WaitMSec(nWaitMillis);
    lock.Enter();
    DecRef();
    
    if (!m_bOpen)
      return -1;
    
    if (bHasData)
    {
      int nToRead = std::min(m_buffer.GetMaxReadSize(), nMaxSize);
      m_buffer.ReadBinary(buf, nToRead);
      nResult = nToRead;
    }
  }
  
  CheckStatus();
  
  return nResult;
}

bool Pipe::Write(const char *buf, int nSize, int nWaitMillis)
{
  CSingleLock lock(m_lock);
  if (!m_bOpen)
    return false;
  bool bOk = false;
  int writeSize = m_buffer.GetMaxWriteSize();
  if (writeSize > nSize)
  {
    m_buffer.WriteBinary(buf, nSize);
    bOk = true;
  }
  else
  {
    while ( m_buffer.GetMaxWriteSize() < nSize && m_bOpen )
    {
      lock.Leave();
      for (size_t l=0; l<m_listeners.size(); l++)
        m_listeners[l]->OnPipeOverFlow();

      bool bClear = m_writeEvent.WaitMSec(nWaitMillis);
      lock.Enter();
      if (bClear && m_buffer.GetMaxWriteSize() >= nSize)
      {
        m_buffer.WriteBinary(buf, nSize);
        bOk = true;
        break;
      }
      
      if (nWaitMillis != INFINITE)
        break;
    }
  }

  CheckStatus();
  
  return bOk && m_bOpen;
}

void Pipe::CheckStatus()
{
  if (m_bEof)
  {
    m_writeEvent.Set();
    m_readEvent.Set();  
    return;
  }
  
  if (m_buffer.GetMaxWriteSize() == 0)
    m_writeEvent.Reset();
  else
    m_writeEvent.Set();
  
  if (m_buffer.GetMaxReadSize() == 0)
    m_readEvent.Reset();
  else
    m_readEvent.Set();  
}

void Pipe::Close()
{
  CSingleLock lock(m_lock);
  m_bOpen = false;
  m_readEvent.Set();
  m_writeEvent.Set();
}

void Pipe::AddListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void Pipe::RemoveListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  std::vector<XFILE::IPipeListener *>::iterator i = m_listeners.begin();
  while(i != m_listeners.end())
  {
    if ( (*i) == l)
      i = m_listeners.erase(i);
    else
      i++;
  }
}

PipesManager::PipesManager() : m_nGenIdHelper(1)
{
}

PipesManager::~PipesManager()
{
}

PipesManager &PipesManager::GetInstance()
{
  static PipesManager instance;
  return instance;
}

CStdString   PipesManager::GetUniquePipeName()
{
  CSingleLock lock(m_lock);
  CStdString id;
  id.Format("pipe://%d/",m_nGenIdHelper++);
  return id;
}

XFILE::Pipe *PipesManager::CreatePipe(const CStdString &name, int nMaxPipeSize)
{
  CStdString pName = name;
  if (pName.IsEmpty())
    pName = GetUniquePipeName();
  
  CSingleLock lock(m_lock);
  if (m_pipes.find(pName) != m_pipes.end())
    return NULL;
  
  XFILE::Pipe *p = new XFILE::Pipe(pName, nMaxPipeSize);
  m_pipes[pName] = p;
  return p;
}

XFILE::Pipe *PipesManager::OpenPipe(const CStdString &name)
{
  CSingleLock lock(m_lock);
  if (m_pipes.find(name) == m_pipes.end())
    return NULL;
  m_pipes[name]->AddRef();
  return m_pipes[name];
}

void         PipesManager::ClosePipe(XFILE::Pipe *pipe)
{
  CSingleLock lock(m_lock);
  if (!pipe)
    return ;
  
  pipe->DecRef();
  pipe->Close();
  if (pipe->RefCount() == 0)
  {
    m_pipes.erase(pipe->GetName());
    delete pipe;
  }
}

bool         PipesManager::Exists(const CStdString &name)
{
  CSingleLock lock(m_lock);
  return (m_pipes.find(name) != m_pipes.end());
}

