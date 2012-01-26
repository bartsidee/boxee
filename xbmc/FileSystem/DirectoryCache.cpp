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

#include "DirectoryCache.h"
#include "Util.h"
#include "Settings.h"
#include "FileItem.h"
#include "URL.h"
#include "MultiPathDirectory.h"
#include "utils/SingleLock.h"
#include "utils/log.h"

using namespace std;
using namespace DIRECTORY;

CDirectoryCache g_directoryCache;

CDirectoryCache::CDir::CDir(DIR_CACHE_TYPE cacheType)
{
  m_cacheType = cacheType;
  m_lastAccess = 0;
  time(&m_createTime);
  m_Items = new CFileItemList;
  m_Items->SetFastLookup(true);
}

CDirectoryCache::CDir::~CDir()
{
  delete m_Items;
}

void CDirectoryCache::CDir::SetLastAccess(unsigned int &accessCounter)
{
  m_lastAccess = accessCounter++;
}

time_t CDirectoryCache::CDir::GetCreateTime()
{
  return m_createTime;
}

CDirectoryCache::CDirectoryCache(void)
{
  m_iThumbCacheRefCount = 0;
  m_iMusicThumbCacheRefCount = 0;
  m_accessCounter = 0;
  m_cacheBlacklistSet.clear();
#ifdef _DEBUG
  m_cacheHits = 0;
  m_cacheMisses = 0;
#endif
}

CDirectoryCache::~CDirectoryCache(void)
{
}

bool CDirectoryCache::GetDirectory(const CStdString& strPath, CFileItemList &items, bool retrieveAll)
{
  CSingleLock lock (m_cs);
  
  CStdString storedPath = strPath;
  if (CUtil::HasSlashAtEnd(storedPath))
    storedPath.Delete(storedPath.size() - 1);
  
  // this does not work, should be removed
  //CUtil::RemoveSlashAtEnd(storedPath);
  if (g_directoryCache.m_cacheBlacklistSet.find(strPath) != g_directoryCache.m_cacheBlacklistSet.end())
  {
    CLog::Log(LOGDEBUG,"CDirectoryCache::GetDirectory, CACHE, black list (false): %s", strPath.c_str());
    return false;
  }

  iCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
  {
    CDir* dir = i->second;
    bool bExpired = IsPathExpired(storedPath, i->second->GetCreateTime());    
    if (!bExpired &&
       (dir->m_cacheType == DIRECTORY::DIR_CACHE_ALWAYS ||
       (dir->m_cacheType == DIRECTORY::DIR_CACHE_ONCE && retrieveAll)) )
    {
      items.Copy(*dir->m_Items);
      CLog::Log(LOGDEBUG,"CDirectoryCache::GetDirectory, returning %d CACHED items for path %s",items.Size(),storedPath.c_str());
      dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
      m_cacheHits+=items.Size();
#endif
      return true;
    }
    else
      m_cache.erase(i);
  }
  return false;
}

bool CDirectoryCache::UpdateItem(const CFileItem& fileItem)
{
  CSingleLock lock (m_cs);

  CStdString strDirPath = fileItem.GetProperty("directoryPath");
  CStdString strCacheId = fileItem.GetProperty("cacheId");
  CStdString strItemPath = fileItem.m_strPath;

  CStdString strPathNoPassword = strItemPath;
  CUtil::RemovePasswordFromPath(strPathNoPassword);
  CLog::Log(LOGDEBUG,"CDirectoryCache::UpdateItem, CACHE updating dirPath = %s , cacheId = %s , item = %s, path = %s (dircache)", strDirPath.c_str() ,strCacheId.c_str(), fileItem.GetLabel().c_str(), strPathNoPassword.c_str());

  if  (strDirPath == "" || strItemPath == "") {
    return false;
  }

  if (CUtil::HasSlashAtEnd(strDirPath)) //remove the slash at the end
    strDirPath.Delete(strDirPath.size() - 1);

  ciCache i = m_cache.find(strDirPath);
  if (i != m_cache.end())
  {
    CLog::Log(LOGDEBUG,"CDirectoryCache::UpdateItem, found the relevant dir - strDirPath = %s (dircache)", strDirPath.c_str());
    CDir* dir = i->second;
    
    for (int j = 0; j < (int) dir->m_Items->Size(); ++j)
    {
      CFileItemPtr currentItem = dir->m_Items->Get(j);
      if (strCacheId != "" && strCacheId == currentItem->GetProperty("cacheId"))
      {
        *(currentItem) = fileItem;
        CLog::Log(LOGDEBUG,"CDirectoryCache::UpdateItem, found the item - strCacheId = %s (dircache)", strCacheId.c_str());
        return true;
      }
      else
      {
        if (currentItem->m_strPath == strItemPath) 
        {
          *(currentItem) = fileItem;
          CLog::Log(LOGDEBUG,"CDirectoryCache::UpdateItem, found the item - strItemPath = %s (dircache)", strItemPath.c_str());
          return true;
        }
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CDirectoryCache::UpdateItem, could not find the relevant dir - strDirPath = %s (dircache)", strDirPath.c_str());
  }

  return false;
}

void CDirectoryCache::SetDirectory(const CStdString& strPath, const CFileItemList &items, DIR_CACHE_TYPE cacheType)
{
  if (cacheType == DIR_CACHE_NEVER)
    return; // nothing to do
  
  // caches the given directory using a copy of the items, rather than the items
  // themselves.  The reason we do this is because there is often some further
  // processing on the items (stacking, transparent rars/zips for instance) that
  // alters the URL of the items.  If we shared the pointers, we'd have problems
  // as the URLs in the cache would have changed, so things such as
  // CDirectoryCache::FileExists() would fail for files that really do exist (just their
  // URL's have been altered).  This is called from CFile::Exists() which causes
  // all sorts of hassles.
  // IDEALLY, any further processing on the item would actually create a new item 
  // instead of altering it, but we can't really enforce that in an easy way, so
  // this is the best solution for now.
  CSingleLock lock (m_cs);
  
  CStdString storedPath = strPath;
  
  if (CUtil::HasSlashAtEnd(storedPath))
    storedPath.Delete(storedPath.size() - 1);

  ClearDirectory(storedPath);

  CheckIfFull();
  
  CDir* dir = new CDir(cacheType);
  dir->m_Items->Copy(items);
  dir->SetLastAccess(m_accessCounter);
  m_cache.insert(pair<CStdString, CDir*>(storedPath, dir));  
}

void CDirectoryCache::ClearFile(const CStdString& strFile)
{
  CStdString strPath;
  CUtil::GetDirectory(strFile, strPath);
  ClearDirectory(strPath);
}

void CDirectoryCache::ClearDirectory(const CStdString& strPath)
{
  CSingleLock lock (m_cs);
  
  CStdString storedPath = strPath;
  CUtil::RemoveSlashAtEnd(storedPath);
  
  iCache i = m_cache.find(storedPath);
  if (i != m_cache.end())
    Delete(i);
}

void CDirectoryCache::ClearDirectoriesThatIncludeUrl(const CStdString& url)
{
  CSingleLock lock (m_cs);

  CStdString trimUrl = url;

  /* trim the last slash */
  if (CUtil::HasSlashAtEnd(trimUrl))
    trimUrl.Delete(trimUrl.size() - 1);

  if(!trimUrl.Equals(""))
  {
    iCache i = m_cache.begin();
    while (i != m_cache.end())
    {
      //CLog::Log(LOGDEBUG,"CDirectoryCache, comparing cached dir: %s with base URL: %s",i->first.c_str(),trimUrl.c_str());
      if (i->first.Find(trimUrl) >= 0)
      {
        //CLog::Log(LOGDEBUG,"CDirectoryCache, removing cached dir: %s",i->first.c_str());
        Delete(i++);
      }
      else
      {
        i++;
      }
    }
  } 
}

void CDirectoryCache::ClearSubPaths(const CStdString& strPath)
{
  CSingleLock lock (m_cs);

  CStdString storedPath = strPath;

  if (CUtil::HasSlashAtEnd(storedPath))
  {
    storedPath.Delete(storedPath.size() - 1);
  }

  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    CStdString path = i->first;
    if (strncmp(path.c_str(), storedPath.c_str(), storedPath.GetLength()) == 0)
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::AddFile(const CStdString& strFile)
    {
  CSingleLock lock (m_cs);

  CStdString strPath;
  CUtil::GetDirectory(strFile, strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  ciCache i = m_cache.find(strPath);
  if (i != m_cache.end())
      {
    CDir *dir = i->second;
    CFileItemPtr item(new CFileItem(strFile, false));
    dir->m_Items->Add(item);
    dir->SetLastAccess(m_accessCounter);
      }
    }

bool CDirectoryCache::FileExists(const CStdString& strFile, bool& bInCache)
{
  CSingleLock lock (m_cs);
  bInCache = false;

  CStdString strPath;
  CUtil::GetDirectory(strFile, strPath);
  CUtil::RemoveSlashAtEnd(strPath);

  ciCache i = m_cache.find(strPath);
  if (i != m_cache.end())
  {
    bInCache = true;
    CDir *dir = i->second;
    dir->SetLastAccess(m_accessCounter);
#ifdef _DEBUG
    m_cacheHits++;
#endif
    return dir->m_Items->Contains(strFile);
  }
#ifdef _DEBUG
  m_cacheMisses++;
#endif
  return false;
}

void CDirectoryCache::Clear()
{
  // this routine clears everything except things we always cache
  CSingleLock lock (m_cs);

  iCache i = m_cache.begin();
  while (i != m_cache.end() )
  {
    if (!IsCacheDir(i->first))
      Delete(i++);
    else
      i++;
  }
}

void CDirectoryCache::ClearAll()
{
  CSingleLock lock (m_cs);

  iCache i = m_cache.begin();
  while (i != m_cache.end() )
  {
    Delete(i++);
  }
}

void CDirectoryCache::InitCache(set<CStdString>& dirs)
{
  set<CStdString>::iterator it;
  for (it = dirs.begin(); it != dirs.end(); ++it)
  {
    const CStdString& strDir = *it;
    CFileItemList items;
    CDirectory::GetDirectory(strDir, items, "", false);
    items.Clear();
  }
}

void CDirectoryCache::ClearCache(set<CStdString>& dirs)
{ 
  iCache i = m_cache.begin();
  while (i != m_cache.end())
  {
    if (dirs.find(i->first) != dirs.end())
      Delete(i++);
    else
      i++;
  }
}

bool CDirectoryCache::IsCacheDir(const CStdString &strPath) const
{
  if (m_thumbDirs.find(strPath) == m_thumbDirs.end())
    return false;
  if (m_musicThumbDirs.find(strPath) == m_musicThumbDirs.end())
    return false;

  return true;
}

void CDirectoryCache::InitThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iThumbCacheRefCount > 0)
  {
    m_iThumbCacheRefCount++;
    return ;
  }
  m_iThumbCacheRefCount++;

  // Init video, pictures cache directories
  if (m_thumbDirs.size() == 0)
  {
    // thumbnails directories
/*    m_thumbDirs.insert(g_settings.GetThumbnailsFolder());
    for (unsigned int hex=0; hex < 16; hex++)
    {
      CStdString strHex;
      strHex.Format("\\%x",hex);
      m_thumbDirs.insert(g_settings.GetThumbnailsFolder() + strHex);
    }*/
  }

  InitCache(m_thumbDirs);
}

void CDirectoryCache::ClearThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iThumbCacheRefCount > 1)
  {
    m_iThumbCacheRefCount--;
    return ;
  }

  m_iThumbCacheRefCount--;
  ClearCache(m_thumbDirs);
}

void CDirectoryCache::InitMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iMusicThumbCacheRefCount > 0)
  {
    m_iMusicThumbCacheRefCount++;
    return ;
  }
  m_iMusicThumbCacheRefCount++;

  // Init music cache directories
  if (m_musicThumbDirs.size() == 0)
  {
    // music thumbnails directories
    for (int i = 0; i < 16; i++)
    {
      CStdString hex, folder;
      hex.Format("%x", i);
      CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), hex, folder);
      m_musicThumbDirs.insert(folder);
    }
  }

  InitCache(m_musicThumbDirs);
}

void CDirectoryCache::ClearMusicThumbCache()
{
  CSingleLock lock (m_cs);

  if (m_iMusicThumbCacheRefCount > 1)
  {
    m_iMusicThumbCacheRefCount--;
    return ;
  }

  m_iMusicThumbCacheRefCount--;
  ClearCache(m_musicThumbDirs);
}

void CDirectoryCache::CheckIfFull()
{
  CSingleLock lock (m_cs);
  static const unsigned int max_cached_dirs = 10;

  // find the last accessed folder, and remove if the number of cached folders is too many
  iCache lastAccessed = m_cache.end();
  unsigned int numCached = 0;
  for (iCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    // ensure dirs that are always cached aren't cleared
    if (!IsCacheDir(i->first) && i->second->m_cacheType != DIR_CACHE_ALWAYS)
    {
      if (lastAccessed == m_cache.end() || i->second->GetLastAccess() < lastAccessed->second->GetLastAccess())
        lastAccessed = i;
      numCached++;
    }
  }
  if (lastAccessed != m_cache.end() && numCached >= max_cached_dirs)
    Delete(lastAccessed);
}

void CDirectoryCache::Delete(iCache it)
{
  CDir* dir = it->second;
  delete dir;
  m_cache.erase(it);
}

#ifdef _DEBUG
void CDirectoryCache::PrintStats() const
{
  CSingleLock lock (m_cs);
  CLog::Log(LOGDEBUG, "%s - total of %u cache hits, and %u cache misses", __FUNCTION__, m_cacheHits, m_cacheMisses);
  // run through and find the oldest and the number of items cached
  unsigned int oldest = UINT_MAX;
  unsigned int numItems = 0;
  unsigned int numDirs = 0;
  for (ciCache i = m_cache.begin(); i != m_cache.end(); i++)
  {
    if (!IsCacheDir(i->first))
    {
      CDir *dir = i->second;
      oldest = min(oldest, dir->GetLastAccess());
      numItems += dir->m_Items->Size();
      numDirs++;
    }
  }
  CLog::Log(LOGDEBUG, "%s - %u folders cached, with %u items total.  Oldest is %u, current is %u", __FUNCTION__, numDirs, numItems, oldest, m_accessCounter);
}
#endif



bool CDirectoryCache::IsPathExpired(const CStdString& strPath, time_t timestamp) {
  
  // TODO: Change this, current default 30 seconds
  time_t now;
  time(&now);
  
  if (strPath.Left(7).Equals("lastfm:")) {
    if (now - timestamp > 1800)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("shout:")) {
    if (now - timestamp > 1800)  //seconds
      return true;
  }
  else if (strPath.Left(10).Equals("boxeedb://")) {
    if (now - timestamp > 30)  //seconds
      return true;
  }
  else if (strPath.Left(8).Equals("boxee://")) {
    if (now - timestamp > 30)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("rss://")) {
    if (now - timestamp > 120)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("bms://")) {
    if (now - timestamp > 120)  //seconds
      return true;
  }
  else if (strPath == "smb:/" || strPath == "upnp:/" || strPath == "network:/")
      return true;
  else if (strPath.Left(6).Equals("smb://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("nfs://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("afp://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("upnp://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(7).Equals("http://")) { // this is also upnp (upnp urls are http:// at the end)
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(9).Equals("plugin://")) {
    if (now - timestamp > 600)  //seconds
      return true;
  }
  else if (strPath.Left(7).Equals("feed://")) {
    CURI url(strPath);
    if (url.GetHostName() == "queue")
    {
      return true; // don't use cache for queue
    }
    else if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("zip://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else if (strPath.Left(6).Equals("rar://")) {
    if (now - timestamp > 60)  //seconds
      return true;
  }
  else {
    return false;
  }
  
  return false;
}

bool CDirectoryCache::HasDirectory(const CStdString& strPath)
{
  CSingleLock lock (m_cs);
  
  CStdString strPath1 = strPath;
  if (CUtil::HasSlashAtEnd(strPath1))
    strPath1.Delete(strPath1.size() - 1);
  
  ciCache i = m_cache.find(strPath1);
  if (i != m_cache.end())
  {  
    if (IsPathExpired(strPath1, i->second->GetCreateTime())) 
    {
      return false;
    }
    
    return true;
  }
  
  return false;
}

bool CDirectoryCache::AddDirectoryToBlacklist(const CStdString& strPath)
{
  CSingleLock lock (m_cs);
  g_directoryCache.m_cacheBlacklistSet.insert(strPath);
  return true;
}

