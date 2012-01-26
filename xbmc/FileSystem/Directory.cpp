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

#include "Directory.h"
#include "FactoryDirectory.h"
#include "FactoryFileDirectory.h"
#ifndef _LINUX
#include "utils/Win32Exception.h"
#endif
#include "FileItem.h"
#include "DirectoryCache.h"
#include "GUISettings.h"
#include "utils/log.h"

#include "Application.h"
#include "BoxeeDatabaseDirectory.h"
#include "DirectoryCache.h"
#include "BoxeeUtils.h"
#include "File.h"

#include "Util.h"
#include "AdvancedSettings.h"

using namespace std;
using namespace DIRECTORY;
using namespace XFILE;


/**
 * Job for retreiving remote metadata for audio albums
 * 
 */
class ThumbCreatorJob : public BOXEE::BXBGJob
{
public:
  ThumbCreatorJob(CFileItem& dir, CFileItemList &items);
  virtual ~ThumbCreatorJob(){};
  virtual void DoWork();
  
private:
  CFileItem m_dir;
  CFileItemList m_items;
};

ThumbCreatorJob::ThumbCreatorJob(CFileItem& dir, CFileItemList &items) : BXBGJob("BXDirectoryThumbCreatorJob")
{
  m_dir = dir;
  m_items = items;
}

void ThumbCreatorJob::DoWork()
{
  CLog::Log(LOGDEBUG,"ThumbCreatorJob::DoWork - Going to call BoxeeUtils::CreateDirectoryThumbnail with [path=%s][ItemsSize=%d] (foldert)",(m_dir.m_strPath).c_str(),m_items.Size());
  
  BoxeeUtils::CreateDirectoryThumbnail(m_dir, m_items);
}

#define MAX_WORK_CAPACITY 10000

BOXEE::BXBGProcess CDirectory::s_thumbCreatorProcess(MAX_WORK_CAPACITY);

CDirectory::CDirectory()
{

}

CDirectory::~CDirectory()
{

}

bool CDirectory::GetDirectory(CFileItem* pItem, CFileItemList &items)
{
  bool bResult = CDirectory::GetDirectory(pItem->m_strPath, items);

  items.SetProperty("parentPath", pItem->GetProperty("parentPath"));
  
  if (bResult && pItem->GetThumbnailImage() != "") {
    for(int i = 0; i < items.Size(); i++) {
      if (items.Get(i)->GetThumbnailImage() != "")
      {
        items.Get(i)->SetProperty("OriginalThumb", items.Get(i)->GetThumbnailImage());
      }
      if (pItem->HasProperty("OriginalThumb") && !pItem->GetProperty("OriginalThumb").IsEmpty()) {
        items.Get(i)->SetProperty("parentThumb", pItem->GetProperty("OriginalThumb"));
      }
      else {
        items.Get(i)->SetProperty("parentThumb", pItem->GetThumbnailImage());
      }
    }
  }
  
  ////////////////////////////////////
  // Create the directory thumbnail //
  ////////////////////////////////////

  if(bResult)
  {
    // In case of success in call to GetDirectory(), check if item thumbnail is local and already exist
    
    CLog::Log(LOGDEBUG,"CDirectory::GetDirectory - Handling item [path=%s][label=%s][thumbnail=%s] (foldert)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetThumbnailImage()).c_str());

    CStdString dirThumbPath = pItem->GetThumbnailImage();

    if (!CUtil::IsHD(pItem->m_strPath) && !CUtil::IsSmb(pItem->m_strPath) && !CUtil::IsUPnP(pItem->m_strPath))
    {
      CLog::Log(LOGDEBUG,"CDirectory::GetDirectory - For item [path=%s] path is not local or lan -> dont create dir thumb (foldert)", (pItem->m_strPath).c_str());
    }
    else if((!dirThumbPath.IsEmpty()) && (CUtil::IsHD(dirThumbPath)) && (CFile::Exists(dirThumbPath)))
    {
      CLog::Log(LOGDEBUG,"CDirectory::GetDirectory - For item [path=%s][label=%s][thumbnail=%s] the thumb is HD [%d] and exist [%d] -> NO need to call CreateDirectoryThumbnail() (foldert)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),(pItem->GetThumbnailImage()).c_str(),CUtil::IsHD(dirThumbPath),CFile::Exists(dirThumbPath));
    }
    else
    {
      // Item thumb isn't HD or doesn't exist -> Need to create the directory thumbnail
    
      CStdString dirCacheThumbPath = pItem->GetCachedPictureThumb();

      if(!CFile::Exists(dirCacheThumbPath))
      {
        CLog::Log(LOGDEBUG,"CDirectory::GetDirectory - Going to queue job for CreateDirectoryThumbnail() for item [path=%s][label=%s] because [dirCacheThumbPath=%s] doesn't exist (foldert)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),dirCacheThumbPath.c_str());

        ThumbCreatorJob* job = new ThumbCreatorJob(*pItem,items);

        // This call will block if the processor is at maximum capacity
        // This ensures that the thread will not overload
        if (!s_thumbCreatorProcess.QueueJob(job)) 
        {
          LOG(LOG_LEVEL_ERROR, "CDirectory::GetDirectory - FAILED to queue job for folder [path=%s][label=%s] (foldert)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str());
          delete job;
      }
      }
      else
      {
        CLog::Log(LOGDEBUG,"CDirectory::GetDirectory - The cache thumb [%s] for item [path=%s][label=%s] exist -> NO need to call CreateDirectoryThumbnail() (foldert)",dirCacheThumbPath.c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str());      
      }
    }
  }
  
  return bResult;
}

bool CDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items, CStdString strMask /*=""*/, bool bUseFileDirectories /* = true */, bool allowPrompting /* = false */, DIR_CACHE_TYPE cacheDirectory /* = DIR_CACHE_ONCE */, bool extFileInfo /* = true */)
{
  if (strPath.IsEmpty())
  {
    CLog::Log(LOGERROR, "%s - Error getting items from empty path", __FUNCTION__);
    return false;
  }
  try 
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (!pDirectory.get())
    {
      return false;
    }
    
    // check our cache for this path
    CLog::Log(LOGDEBUG , "CDirectory::GetDirectory , calling g_directoryCache.GetDirectory with path: %s , items: %d , retreiveAll: %d (cache check)",strPath.c_str(), items.Size(), (cacheDirectory == DIR_CACHE_ALWAYS));
    if (g_directoryCache.GetDirectory(strPath, items, cacheDirectory == DIR_CACHE_ALWAYS))
    {
      CLog::Log(LOGDEBUG , "CDirectory::GetDirectory , g_directoryCache.GetDirectory returned true (cache check)");
      items.m_strPath = strPath;
    }
    else
    {
      // need to clear the cache (in case the directory fetch fails)
      // and (re)fetch the folder
      CLog::Log(LOGDEBUG , "CDirectory::GetDirectory , g_directoryCache.GetDirectory returned false (cache check)");

      if (cacheDirectory != DIR_CACHE_NEVER)
        g_directoryCache.ClearDirectory(strPath);
      
      pDirectory->SetAllowPrompting(allowPrompting);
      pDirectory->SetCacheDirectory(cacheDirectory);
      pDirectory->SetUseFileDirectories(bUseFileDirectories);
      pDirectory->SetExtFileInfo(extFileInfo);
      
      items.m_strPath = strPath;
      
      if (!pDirectory->GetDirectory(strPath, items))
      {
        CLog::Log( LOGDEBUG, "%s - Error getting %s", __FUNCTION__, strPath.c_str() );
        return false;
      }
      //CLog::Log(LOGDEBUG , "CDirectory::GetDirectory after trying to get the directory , path: %s , items: %d (cache check)",strPath.c_str(), items.Size());
      // cache the directory, if necessary
      if ((cacheDirectory != DIR_CACHE_NEVER) && (!items.IsEmpty()))
        g_directoryCache.SetDirectory(strPath, items, pDirectory->GetCacheType(strPath));
    }
    
    // now filter for allowed files
    pDirectory->SetMask(strMask);
    for (int i = 0; i < items.Size(); ++i)
    {
      CFileItemPtr item = items[i];
      if ((!item->m_bIsFolder && !pDirectory->IsAllowed(item->m_strPath))/* ||
          (item->GetPropertyBOOL("file:hidden") && !g_guiSettings.GetBool("filelists.showhidden"))*/)
      {
        items.Remove(i);
        i--; // don't confuse loop
      }
    }
    
    //  Should any of the files we read be treated as a directory?
    //  Disable for database folders, as they already contain the extracted items
    if (bUseFileDirectories && !items.IsMusicDb() && !items.IsVideoDb() && !items.IsSmartPlayList() && !items.IsBoxeeDb())
    {
      for (int i=0; i< items.Size(); ++i)
      {
        CFileItemPtr pItem=items[i];
        if (pItem.get() != NULL)
        {
          if (g_advancedSettings.m_playlistAsFolders && pItem->IsPlayList())
          {
            pItem->m_bIsFolder = true;
          }
          else if ((!pItem->m_bIsFolder) && (!pItem->IsInternetStream()) && !pItem->IsPicture() )
          {
            auto_ptr<IFileDirectory> pDirectory(CFactoryFileDirectory::Create(pItem->m_strPath,pItem.get(),strMask));
            if (pDirectory.get())
              pItem->m_bIsFolder = true;
          }

          pItem->SetProperty("parentPath", strPath);
          CBoxeeDatabaseDirectory::FillItemDetails(pItem.get());
        }
      }      
    }
    return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...) 
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log( LOGDEBUG, "%s - Error getting %s", __FUNCTION__, strPath.c_str() );
  return false;
}

bool CDirectory::Create(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      if(pDirectory->Create(strPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error creating %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::Exists(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      return pDirectory->Exists(strPath.c_str());
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error checking for %s", __FUNCTION__, strPath.c_str());    
  return false;
}

bool CDirectory::Remove(const CStdString& strPath)
{
  try
  {
    auto_ptr<IDirectory> pDirectory(CFactoryDirectory::Create(strPath));
    if (pDirectory.get())
      if(pDirectory->Remove(strPath.c_str()))
        return true;
  }
#ifndef _LINUX
  catch (const win32_exception &e) 
  {
    e.writelog(__FUNCTION__);
  }
#endif
  catch (...)
  {
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
  CLog::Log(LOGERROR, "%s - Error removing %s", __FUNCTION__, strPath.c_str());
  return false;
}

bool CDirectory::IsCached(const CStdString& strPath)
{
  return g_directoryCache.HasDirectory(strPath);
}

bool CDirectory::CreateRecursive(const CStdString& strDirectory)
{	// Recursively create all the non existing parent folders up to (and including) strDirectory.
      CStdString curr_dir = strDirectory;
      CUtil::RemoveSlashAtEnd(curr_dir);  // for the test below
	  if(Exists(curr_dir))
		  return true;
	  vector<CStdString> tokens;
      if (!(curr_dir.size() == 2 && curr_dir[1] == ':'))
      {
        CURI url(curr_dir);
        CStdString pathsep;
#ifndef _LINUX        
        pathsep = "\\";
#else
        pathsep = "/";
#endif
        CUtil::Tokenize(url.GetFileName(),tokens,pathsep.c_str());
        CStdString strCurrPath;
        // Handle special
        if (!url.GetProtocol().IsEmpty()) {
          pathsep = "/";
          strCurrPath += url.GetProtocol() + "://";
        } // If the directory has a / at the beginning, don't forget it
        else if (curr_dir[0] == pathsep[0])
          strCurrPath += pathsep;
        for (vector<CStdString>::iterator iter=tokens.begin();iter!=tokens.end();++iter)
        {
          strCurrPath += *iter+pathsep;
          if(!Create(strCurrPath))
			  return false;
        }
      }
	  return true;
}
