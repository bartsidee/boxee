
#include "system.h"

#ifdef HAS_NFS

#include "FileItem.h"
#include "FileFactory.h"
#include "FileNfs.h"
#include "URL.h"
#include "utils/log.h"
#include "NfsDirectory.h"
#include "File.h"
using namespace XFILE;
using namespace DIRECTORY;

CFileNfs::CFileNfs()
{
  m_file = NULL;
  m_bOpened = false;
}

CFileNfs::~CFileNfs()
{
  Close();

  if( m_file )
    delete m_file;

  m_file = NULL;
}

int64_t CFileNfs::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_file->GetPosition();
}

int64_t CFileNfs::GetLength()
{
  if (!m_bOpened) return -1;
  return m_file->GetLength();
}

bool CFileNfs::Open(const CURI& url)
{
  if(url.GetProtocol() != "nfs")
  {
    CLog::Log(LOGERROR, "CFileNfs::Open - invalid protocol");
    return false;
  }
  
  CFileItem item;
  CNfsDirectory dir;

  if(dir.GetResource(url, item))
  {
    m_bOpened = true;
    m_file = CFileFactory::CreateLoader(item.m_strPath);
    if( m_file )
    {
      CLog::Log(LOGDEBUG, "CFileNfs::Open on stream (%s)", item.m_strPath.c_str());
      return m_file->Open( item.m_strPath );
    } 
  }

  CLog::Log(LOGERROR, "CFileNfs::Open - failed to get resource for file %s", url.Get().c_str());

  return false;
}

bool CFileNfs::Exists(const CURI& url)
{
  CFileItem item;
  CNfsDirectory dir;

  if(dir.GetResource(url, item))
  {
    return CFile::Exists(item.m_strPath);
  }

  return false;
}

int CFileNfs::Stat(const CURI& url, struct __stat64* buffer)
{
  int res = -1;
  CFileItem item;
  CNfsDirectory dir;

  if(url.GetProtocol() != "nfs")
  {
    CLog::Log(LOGERROR, "CFileNfs::Stat - invalid protocol");
    return -1;
  }

  if(dir.GetResource(url, item))
  {
    IFile* file;

    file = CFileFactory::CreateLoader(item.m_strPath);
    if(file)
    {
      CURI u(item.m_strPath);
      res = file->Stat(u,buffer);
      delete file;
    }
  }

  return res;
}

unsigned int CFileNfs::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return -1;
  return m_file->Read(lpBuf, uiBufSize);
}

int64_t CFileNfs::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  return m_file->Seek(iFilePosition, iWhence);
}

void CFileNfs::Close()
{
  if( m_file )
    m_file->Close();

  m_bOpened = false;
}

#endif // HAS_NFS

