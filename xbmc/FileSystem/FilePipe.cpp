#include "FilePipe.h"
#include "utils/SingleLock.h"
#include "PipesManager.h"
#include "StringUtils.h"

using namespace XFILE;

CFilePipe::CFilePipe() : m_pos(0), m_length(-1), m_pipe(NULL)
{
}

CFilePipe::~CFilePipe()
{
  Close();
}

int64_t CFilePipe::GetPosition()
{
  return m_pos;
}

int64_t CFilePipe::GetLength()
{
  return m_length;
}

void CFilePipe::SetLength(int64_t len)
{
  m_length = len;
}

bool CFilePipe::Open(const CURL& url)
{
  CStdString name;
  url.GetURL(name);
  m_pipe = PipesManager::GetInstance().OpenPipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CFilePipe::Exists(const CURL& url)
{
  CStdString name;
  url.GetURL(name);
  return PipesManager::GetInstance().Exists(name);
}

int CFilePipe::Stat(const CURL& url, struct __stat64* buffer)
{
  return -1;
}

int CFilePipe::Stat(struct __stat64* buffer)
{
  memset(buffer,0,sizeof(struct __stat64));
  buffer->st_size = m_length;
  return 0;
}

unsigned int CFilePipe::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  return m_pipe->Read((char *)lpBuf,uiBufSize,INFINITE);
}

int CFilePipe::Write(const void* lpBuf, int64_t uiBufSize)
{
  if (!m_pipe)
    return -1;
  
  return (int)(m_pipe->Write((const char *)lpBuf,uiBufSize,INFINITE)); // its not the size. its bool. either all was written or not.
}

void CFilePipe::SetEof()
{
  if (!m_pipe)
    return ;
  m_pipe->SetEof();
}

bool CFilePipe::IsEof()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEof();
}

bool CFilePipe::IsEmpty()
{
  if (!m_pipe)
    return true;
  return m_pipe->IsEmpty();
}

int64_t CFilePipe::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

void CFilePipe::Close()
{
  if (m_pipe)
  {
    PipesManager::GetInstance().ClosePipe(m_pipe);
    m_pipe->RemoveListener(this);
  }
  m_pipe = NULL;
}

bool CFilePipe::IsClosed()
{
  return (m_pipe == NULL);
}

void CFilePipe::Flush()
{
}

bool CFilePipe::OpenForWrite(const CURL& url, bool bOverWrite)
{
  CStdString name;
  url.GetURL(name);

  m_pipe = PipesManager::GetInstance().CreatePipe(name);
  if (m_pipe)
    m_pipe->AddListener(this);
  return (m_pipe != NULL);
}

bool CFilePipe::Delete(const CURL& url)
{
  return false;
}

bool CFilePipe::Rename(const CURL& url, const CURL& urlnew)
{
  return false;
}

int CFilePipe::IoControl(int request, void* param)
{
  return -1;
}

CStdString CFilePipe::GetName() const
{
  if (!m_pipe)
    return StringUtils::EmptyString;
  return m_pipe->GetName();
}

void CFilePipe::OnPipeOverFlow()
{
  CSingleLock lock(m_lock);
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeOverFlow();
}

void CFilePipe::OnPipeUnderFlow()
{
  for (size_t l=0; l<m_listeners.size(); l++)
    m_listeners[l]->OnPipeUnderFlow();
}

void CFilePipe::AddListener(IPipeListener *l)
{
  CSingleLock lock(m_lock);
  for (size_t i=0; i<m_listeners.size(); i++)
  {
    if (m_listeners[i] == l)
      return;
  }
  m_listeners.push_back(l);
}

void CFilePipe::RemoveListener(IPipeListener *l)
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
