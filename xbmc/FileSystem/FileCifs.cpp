
#include "system.h"

#ifdef HAS_CIFS

#include "FileItem.h"
#include "FileFactory.h"
#include "FileCifs.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "CIFSDirectory.h"
#include "File.h"


using namespace XFILE;
using namespace DIRECTORY;

CFileCifs::CFileCifs()
{
  m_file = NULL;
  m_bOpened = false;
}

CFileCifs::~CFileCifs()
{
  Close();

  if( m_file )
    delete m_file;

  m_file = NULL;
}


int64_t CFileCifs::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_file->GetPosition();
}

int64_t CFileCifs::GetLength()
{
  if (!m_bOpened) return -1;
  return m_file->GetLength();
}

bool CFileCifs::Open(const CURI& url)
{
  if(url.GetProtocol() != "smb")
  {
    CLog::Log(LOGERROR, "CFileCifs::Open - invalid protocol");
    return false;
  }

  CFileItem item;
  CCifsDirectory dir;
  int status;

  status = dir.GetResource(url, item);

  if(status == 0)
  {
    m_file = CFileFactory::CreateLoader(item.m_strPath);
    if( m_file )
    {
      m_bOpened = true;

      CLog::Log(LOGDEBUG, "CFileCifs::Open on stream (%s)", item.m_strPath.c_str());
      return m_file->Open( item.m_strPath );
    }
  }

  CLog::Log(LOGERROR, "CFileCifs::Open - failed to get resource for file %s", url.Get().c_str());

  return false;
}

bool CFileCifs::Exists(const CURI& url)
{
  CFileItem item;
  CCifsDirectory dir;
  int status;

  status = dir.GetResource(url, item);

  if(status == 0)
  {
     return CFile::Exists(item.m_strPath);
  }

  return false;
}

int CFileCifs::Stat(const CURI& url, struct __stat64* buffer)
{
  int res = -1;
  CFileItem item;
  CCifsDirectory dir;

  if(url.GetProtocol() != "smb")
  {
    CLog::Log(LOGERROR, "CFileCifs::Stat - invalid protocol");
    return -1;
  }

  if(dir.GetResource(url, item) == 0)
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

unsigned int CFileCifs::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return -1;
  return m_file->Read(lpBuf, uiBufSize);
}

int64_t CFileCifs::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  return m_file->Seek(iFilePosition, iWhence);
}

void CFileCifs::Close()
{
  if( m_file )
    m_file->Close();

  m_bOpened = false;
}

#endif // HAS_CIFS

