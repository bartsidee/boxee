
#include "system.h"

#ifdef HAS_BMS

#include "FileItem.h"
#include "FileFactory.h"
#include "FileBms.h"
#include "URL.h"
#include "Util.h"
#include "utils/log.h"
#include "BMSDirectory.h"
#include "File.h"


using namespace XFILE;
using namespace DIRECTORY;

CFileBms::CFileBms()
{
  m_file = NULL;
  m_bOpened = false;
}

CFileBms::~CFileBms()
{
  Close();

  if( m_file )
    delete m_file;

  m_file = NULL;
}


int64_t CFileBms::GetPosition()
{
  if (!m_bOpened) return -1;
  return m_file->GetPosition();
}

int64_t CFileBms::GetLength()
{
  if (!m_bOpened) return -1;
  return m_file->GetLength();
}

bool CFileBms::Open(const CURI& url)
{
  if(url.GetProtocol() != "bms")
  {
    CLog::Log(LOGERROR, "CFileBms::Open - invalid protocol");
    return false;
  }

  CFileItem item;
  CBmsDirectory dir;

  if(dir.GetResource(url, item))
  {
    m_bOpened = true;
    m_file = CFileFactory::CreateLoader(item.m_strPath);
    if( m_file )
    {
      CLog::Log(LOGDEBUG, "CFileBms::Open on stream (%s)", item.m_strPath.c_str());
      return m_file->Open( item.m_strPath );;
    }
  }

  CLog::Log(LOGERROR, "CFileBms::Open - failed to get resource for file %s", url.Get().c_str());

  return false;
}

bool CFileBms::Exists(const CURI& url)
{
  CFileItem item;
  CBmsDirectory dir;

  if(dir.GetResource(url, item))
  {
    return CFile::Exists(item.m_strPath);
  }

  return false;
}

int CFileBms::Stat(const CURI& url, struct __stat64* buffer)
{
  int res = -1;
  CFileItem item;
  CBmsDirectory dir;
  CFileItemList items;

  if(url.GetProtocol() != "bms")
  {
    CLog::Log(LOGERROR, "CFileBms::Stat - invalid protocol");
    return -1;
  }

  dir.GetDirectory(url.Get(), items);

  // test for folder
  if(items.Size() != 0 )
  {
    buffer->st_mode = 0;
    buffer->st_mode |= S_IFDIR;
    return 0;
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

unsigned int CFileBms::Read(void* lpBuf, int64_t uiBufSize)
{
  if (!m_bOpened) return -1;
  return m_file->Read(lpBuf, uiBufSize);
}

int64_t CFileBms::Seek(int64_t iFilePosition, int iWhence)
{
  if (!m_bOpened) return -1;
  return m_file->Seek(iFilePosition, iWhence);
}

void CFileBms::Close()
{
  if( m_file )
    m_file->Close();

  m_bOpened = false;
}

#endif // HAS_BMS

