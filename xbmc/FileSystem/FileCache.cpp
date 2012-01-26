/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "AutoPtrHandle.h"
#include "FileCache.h"
#include "utils/Thread.h"
#include "File.h"
#include "URL.h"

#include "CacheMemBuffer.h"
#include "utils/SingleLock.h"

#include "FileItem.h"

using namespace AUTOPTR;
using namespace XFILE;
 
#define READ_CACHE_CHUNK_SIZE (64*1024)

CFileCache::CFileCache()
{
   m_bDeleteCache = true;
   m_nSeekResult = 0;
   m_seekPos = 0;
   m_readPos = 0;
   m_seekPossible = 0;
   m_pCache = NULL;
   m_bCanSeekBack = true;
}

CFileCache::CFileCache(CCacheStrategy *pCache, bool bDeleteCache)
{
  m_pCache = pCache;
  m_bDeleteCache = bDeleteCache;
  m_seekPos = 0;
  m_readPos = 0; 
  m_nSeekResult = 0;
  m_bCanSeekBack = true;
}

CFileCache::~CFileCache()
{
  Close();

  if (m_bDeleteCache && m_pCache)
    delete m_pCache;

  m_pCache = NULL;
}

void CFileCache::SetCacheStrategy(CCacheStrategy *pCache, bool bDeleteCache) 
{
  if (m_bDeleteCache && m_pCache)
    delete m_pCache;

  m_pCache = pCache;
  m_bDeleteCache = bDeleteCache;
}

IFile *CFileCache::GetFileImp() {
  return m_source.GetImplemenation();
}

bool CFileCache::Open(const CURI& url)
{
  Close();

  CSingleLock lock(m_sync);

  CLog::Log(LOGDEBUG,"CFileCache::Open - opening <%s> using cache", url.GetFileName().c_str());

  m_sourcePath = url.Get();

  // opening the source file.
  if(!m_source.Open(m_sourcePath, READ_NO_CACHE | READ_TRUNCATED | READ_CHUNKED)) {
    CLog::Log(LOGERROR,"%s - failed to open source <%s>", __FUNCTION__, m_sourcePath.c_str());
    Close();
    return false;
  }

  // check if source can seek
  m_seekPossible = m_source.Seek(0, SEEK_POSSIBLE);
  m_bCanSeekBack = m_seekPossible ? true : false;

  __int64 nLength = m_source.GetLength();
  CFileItem item;
  item.m_strPath = m_sourcePath;
  item.SetContentType(GetContent());
  
  if (m_pCache && m_bDeleteCache)
  {
    m_pCache->Close();
    delete m_pCache;
  }

  m_bDeleteCache = true;
  
  // we use file cache for files which are not seekable, pictures and in general small files (under 10 M)
  if ( item.IsPicture() || 
       ( nLength > 0 && nLength < 15 * 1024 * 1024 ) || 
       ( 1 != m_seekPossible && nLength > 0 && nLength < (512*1024*1024) )
     )
  {
    CLog::Log(LOGDEBUG, "%s - using file cache", __FUNCTION__);
    m_pCache = new CSimpleFileCache;
    m_bCanSeekBack = true;
  }
  else 
  {
    CLog::Log(LOGDEBUG, "%s - using mem buffer cache", __FUNCTION__);
    m_pCache = new CacheMemBuffer;
  }
  
  // open cache strategy
  if (m_pCache->Open() != CACHE_RC_OK) {
    CLog::Log(LOGERROR,"CFileCache::Open - failed to open cache");
    Close();
    return false;
  }
  
  m_readPos = 0;
  m_seekEvent.Reset();
  m_seekEnded.Reset();

  CThread::Create(false);

  Sleep(100);

  return true;
}

bool CFileCache::Attach(IFile *pFile) {
    CSingleLock lock(m_sync);

  if (!m_pCache)
  {
    m_pCache = new CacheMemBuffer;
    m_bDeleteCache = true;
  }
  
  if (!pFile || !m_pCache)
    return false;

  m_source.Attach(pFile);

  if (m_pCache->Open() != CACHE_RC_OK) {
    CLog::Log(LOGERROR,"CFileCache::Attach - failed to open cache");
    Close();
    return false;
  }

  CThread::Create(false);

  return true;
}

void CFileCache::Process() 
{
  if (!m_pCache) {
    CLog::Log(LOGERROR,"CFileCache::Process - sanity failed. no cache strategy");
    return;
  }

  // setup read chunks size
  int chunksize = CFile::GetChunkSize(m_source.GetChunkSize(), READ_CACHE_CHUNK_SIZE);

  // create our read buffer
  auto_aptr<char> buffer(new char[chunksize]);
  if (buffer.get() == NULL)
  {
    CLog::Log(LOGERROR, "%s - failed to allocate read buffer", __FUNCTION__);
    return;
  }
  
  while(!m_bStop)
  {
    // check for seek events
    if (m_seekEvent.WaitMSec(0)) 
    {
      m_seekEvent.Reset();
      CLog::Log(LOGDEBUG,"%s, request seek on source to %"PRId64, __FUNCTION__, m_seekPos);  
      if ((m_nSeekResult = m_source.Seek(m_seekPos, SEEK_SET)) != m_seekPos)
        CLog::Log(LOGERROR,"%s, error %d seeking. seek returned %"PRId64, __FUNCTION__, (int)GetLastError(), m_nSeekResult);
      else
        m_pCache->Reset(m_seekPos);

      m_seekEnded.Set();
    }

    int iRead = m_source.Read(buffer.get(), chunksize);
    if(iRead == 0)
    {
      CLog::Log(LOGDEBUG, "CFileCache::Process - Hit eof.");
      m_pCache->EndOfInput();
      
      // since there is no more to read - wait either for seek or close 
      // WaitForSingleObject is CThread::WaitForSingleObject that will also listen to the
      // end thread event.
      int nRet = CThread::WaitForSingleObject(m_seekEvent.GetHandle(), INFINITE);
      if (nRet == WAIT_OBJECT_0) 
      {
        m_pCache->ClearEndOfInput();
        m_seekEvent.Set(); // hack so that later we realize seek is needed
      }
      else 
        break;
    }
    else if (iRead < 0)
      m_bStop = true;

    int iTotalWrite=0;
    while (!m_bStop && (iTotalWrite < iRead))
    {
      int iWrite = 0;
      iWrite = m_pCache->WriteToCache(buffer.get()+iTotalWrite, iRead - iTotalWrite);

      // write should always work. all handling of buffering and errors should be
      // done inside the cache strategy. only if unrecoverable error happened, WriteToCache would return error and we break.
      if (iWrite < 0) 
      {
      	CLog::Log(LOGERROR,"CFileCache::Process - error writing to cache");
        m_bStop = true;
        break;
      }
      else if (iWrite == 0)
        Sleep(20);

      iTotalWrite += iWrite;

      // check if seek was asked. otherwise if cache is full we'll freeze.
      if (m_seekEvent.WaitMSec(0))
      {
        m_seekEvent.Set(); // make sure we get the seek event later. 
        break;
      }
    }
  }
}

void CFileCache::OnExit()
{
  m_bStop = true;

  // make sure cache is set to mark end of file (read may be waiting).
  if(m_pCache)
    m_pCache->EndOfInput();

  // just in case someone's waiting...
  m_seekEnded.Set();
}

bool CFileCache::Exists(const CURI& url)
{
  CStdString strPath;
  strPath = url.Get();
  return CFile::Exists(strPath);
}

int CFileCache::Stat(const CURI& url, struct __stat64* buffer)
{
  CStdString strPath;
  strPath = url.Get();
  return CFile::Stat(strPath, buffer);
}

unsigned int CFileCache::Read(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_sync);
  if (!m_pCache) {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return 0;
  }
  int64_t iRc;

retry:
  // attempt to read
  iRc = m_pCache->ReadFromCache((char *)lpBuf, (size_t)uiBufSize);
  if (iRc > 0)
  {
    m_readPos += iRc;
    return (int)iRc;
  }

  if (iRc == CACHE_RC_WOULD_BLOCK)
  {
    // just wait for some data to show up
    iRc = m_pCache->WaitForData(5000, 10000);
    if (iRc > 0)
      goto retry;
  }
  
  if (iRc == CACHE_RC_TIMEOUT)
  {
    CLog::Log(LOGWARNING, "%s - timeout waiting for data", __FUNCTION__);
    return 0;
  }

  if (iRc == CACHE_RC_EOF || iRc == 0)
    return 0;

  // unknown error code
  CLog::Log(LOGERROR, "%s - cache strategy returned unknown error code %d", __FUNCTION__, (int)iRc);  
  return 0;
}

int64_t CFileCache::Seek(int64_t iFilePosition, int iWhence)
{  
  CSingleLock lock(m_sync);

  if (!m_pCache) {
    CLog::Log(LOGERROR,"%s - sanity failed. no cache strategy!", __FUNCTION__);
    return -1;
  }

  int64_t iCurPos = m_readPos;
  int64_t iTarget = iFilePosition;
  if (iWhence == SEEK_END)
    iTarget = GetLength() + iTarget;
  else if (iWhence == SEEK_CUR)
    iTarget = iCurPos + iTarget;
  else if (iWhence == SEEK_POSSIBLE)
  {
    if (iFilePosition < iCurPos)
      return m_bCanSeekBack ? 1 : 0;
    
    if (iFilePosition > iCurPos && iFilePosition < iCurPos + m_pCache->GetAvailableRead())
      return 1;
    
    return m_seekPossible;
  }
  else if (iWhence != SEEK_SET)
    return -1;

  if (iTarget == m_readPos)
    return m_readPos;
  
  //
  // hack - many times a seek to the end of file is issued in order to get its size. if we actually do the seek - 
  // we break on some (most) web servers. so we continue to read as usual and hope that the next seek would not be relative (SEEK_SET is the only one used)
  // 
  if (iTarget == GetLength() && iTarget != 0)
    return iTarget;
  
  if ((m_nSeekResult = m_pCache->Seek(iTarget, SEEK_SET)) != iTarget)
  {
    if(m_seekPossible == 0)
      return m_nSeekResult;

    m_seekPos = iTarget;
    m_seekEvent.Set();
    if (!m_seekEnded.WaitMSec(INFINITE)) 
    {
      CLog::Log(LOGWARNING,"%s - seek to %"PRId64" failed.", __FUNCTION__, m_seekPos);
      return -1;
    }
  }

  if (m_nSeekResult >= 0)
    m_readPos = m_nSeekResult;

  return m_nSeekResult;
}

void CFileCache::Close()
{
  StopThread();

  CSingleLock lock(m_sync);
  if (m_pCache)
    m_pCache->Close();
  
  m_source.Close();
}

int64_t CFileCache::GetPosition()
{
  return m_readPos;
}

int64_t CFileCache::GetLength()
{
  return m_source.GetLength();
}

ICacheInterface* CFileCache::GetCache()
{
  if(m_pCache)
    return m_pCache->GetInterface();
  return NULL;
}

void CFileCache::StopThread(bool bWait /*= true*/)
{
  m_bStop = true;
  //Process could be waiting for seekEvent
  m_seekEvent.Set();
  CThread::StopThread(bWait);
}

CStdString CFileCache::GetContent()
{
  if (!m_source.GetImplemenation())
    return IFile::GetContent();
  
  return m_source.GetImplemenation()->GetContent();
}

__int64	CFileCache::GetAvailableRead()
{
  if (!m_pCache)
    return 0;
  
  return m_pCache->GetAvailableRead();
}
