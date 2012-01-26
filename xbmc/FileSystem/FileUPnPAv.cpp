
#include "system.h"

#ifdef HAS_UPNP_AV

#include "FileItem.h"
#include "FileFactory.h"
#include "FileUPnPAv.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "UPnPAvDirectory.h"
#include "File.h"


using namespace XFILE;
using namespace DIRECTORY;

CFileUPnPAv::CFileUPnPAv()
{
  m_file = NULL;
  m_bOpened = false;
}

CFileUPnPAv::~CFileUPnPAv()
{
  Close();

  if( m_file )
    delete m_file;

  m_file = NULL;
}


int64_t CFileUPnPAv::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_file->GetPosition();
}

int64_t CFileUPnPAv::GetLength()
{
  if (!m_bOpened) return -1;
  return m_file->GetLength();
}

bool CFileUPnPAv::Open(const CURI& url)
{
  if(url.GetProtocol() != "upnp")
  {
    CLog::Log(LOGERROR, "CFileUPnPAv::Open - invalid protocol");
    return false;
  }

  CFileItem item;
  CUPnPAvDirectory dir;

  if(dir.GetResource(url, item))
  {
    m_bOpened = true;
    m_file = CFileFactory::CreateLoader(item.m_strPath);
    if( m_file )
    {
      CLog::Log(LOGDEBUG, "CFileUPnPAv::Open on stream (%s)", item.m_strPath.c_str());
      return m_file->Open( item.m_strPath );
    }
  }

  CLog::Log(LOGERROR, "CFileUPnPAv::Open - failed to get resource for file %s", url.Get().c_str());

  return false;
}

bool CFileUPnPAv::Exists(const CURI& url)
{
  CFileItem item;
  CUPnPAvDirectory dir;

  if(dir.GetResource(url, item))
  {
    return CFile::Exists(item.m_strPath);
  }

  return false;
}

int CFileUPnPAv::Stat(const CURI& url, struct __stat64* buffer)
{
  int res = -1;
  CFileItem item;
  CUPnPAvDirectory dir;

  if(url.GetProtocol() != "upnp")
  {
    CLog::Log(LOGERROR, "CFileUPnPAv::Stat - invalid protocol");
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

unsigned int CFileUPnPAv::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return -1;
  return m_file->Read(lpBuf, uiBufSize);
}

int64_t CFileUPnPAv::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  return m_file->Seek(iFilePosition, iWhence);
}

void CFileUPnPAv::Close()
{
  if( m_file )
    m_file->Close();

  m_bOpened = false;
}

#endif // HAS_UPNP_AV

