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

#include "BackgroundInfoLoader.h"
#include "Application.h"
#include "FileItem.h"
#include "PictureThumbLoader.h"
#include "File.h"
#include "Crc32.h"
#include "lib/libBoxee/bxcurl.h"
#include "SpecialProtocol.h"
#include "Settings.h"
#include "AdvancedSettings.h"
#include "utils/SingleLock.h"
#include "utils/log.h"

using namespace XFILE;
using namespace std;

#define ITEMS_PER_THREAD 5

#define MAX_SMB_THREAD_COUNT 5

CBackgroundInfoLoader::CBackgroundInfoLoader(int nThreads)
{
  m_bStop = true;
  m_pObserver=NULL;
  m_pProgressCallback=NULL;
  m_pVecItems = NULL;
  m_nRequestedThreads = nThreads;
  m_bStartCalled = false;
  m_nActiveThreads = 0;
  m_bDetached = false;
}

CBackgroundInfoLoader::~CBackgroundInfoLoader()
{
  StopThread(false);
}

void CBackgroundInfoLoader::SetNumOfWorkers(int nThreads)
{
  m_nRequestedThreads = nThreads;
}

void CBackgroundInfoLoader::Run()
{
  try
  {
    if (m_vecItems.size() > 0)
    {
      {
        CSingleLock lock(m_lock);
        if (!m_bStartCalled)
        {
          OnLoaderStart();
          m_bStartCalled = true;
        }
      }

      while (!m_bStop)
      {
        CSingleLock lock(m_lock);
        CFileItemPtr pItem;
        vector<CFileItemPtr>::iterator iter = m_vecItems.begin();
        if (iter != m_vecItems.end())
        {
          pItem = *iter;
          m_vecItems.erase(iter);
        }

        if (pItem == NULL)
          break;

        // Ask the callback if we should abort
        if ((m_pProgressCallback && m_pProgressCallback->Abort()) || m_bStop)
          m_bStop=true;

        lock.Leave();
        try
        {
          if (!m_bStop && LoadItem(pItem.get(), true) && m_pObserver)
            m_pObserver->OnItemLoaded(pItem.get());
        }
        catch (...)
        {
          CLog::Log(LOGERROR, "%s::LoadItem - Unhandled exception for item %s", __FUNCTION__, pItem->m_strPath.c_str());
        }
      }
    }

    CSingleLock lock(m_lock);
    if (m_nActiveThreads == 1)
    {
      OnLoaderFinish();
     
      // make main thread delete this loader if this is a detached loader
      if (m_bDetached)
      {
        ThreadMessage msg;
        msg.lpVoid = this;
        msg.dwMessage = TMSG_DELETE_BG_LOADER;
        g_application.getApplicationMessenger().SendMessage(msg);
      }
    }
    m_nActiveThreads--;

  }
  catch (...)
  {
    m_nActiveThreads--;
    CLog::Log(LOGERROR, "%s - Unhandled exception", __FUNCTION__);
  }
}

void CBackgroundInfoLoader::Load(CFileItemList& items)
{
  StopThread();

  if (items.Size() == 0)
    return;
  
  EnterCriticalSection(m_lock);

  for (int nItem=0; nItem < items.Size(); nItem++)
    m_vecItems.push_back(items[nItem]);

  m_pVecItems = &items;
  m_bStop = false;
  m_bStartCalled = false;

  int nThreads = m_nRequestedThreads;
  if (nThreads == -1)
    nThreads = (m_vecItems.size() / (ITEMS_PER_THREAD+1)) + 1;

  if (nThreads > g_advancedSettings.m_bgInfoLoaderMaxThreads)
    nThreads = g_advancedSettings.m_bgInfoLoaderMaxThreads;

  if (items.IsSmb() && nThreads > MAX_SMB_THREAD_COUNT)
    nThreads = MAX_SMB_THREAD_COUNT;

  m_nActiveThreads = nThreads;
  CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::Load - creating %d threads",nThreads);
  for (int i=0; i < nThreads; i++)
  {
    CThread *pThread = new CThread(this); 
    pThread->SetName("Background Loader");
    m_workers.push_back(pThread);
    pThread->Create();
#ifndef _LINUX
    pThread->SetPriority(THREAD_PRIORITY_BELOW_NORMAL);
#endif
  }
      
  LeaveCriticalSection(m_lock);
}

void CBackgroundInfoLoader::StopAsync()
{
  EnterCriticalSection(m_lock);
  m_bStop = true;
  m_vecItems.clear();
  LeaveCriticalSection(m_lock);
}

void CBackgroundInfoLoader::StopThread(bool bDetach)
{
  StopAsync();

  CSingleLock lock(m_lock);

  if (bDetach)
    m_bDetached = true;

  if (!bDetach || m_workers.size() == 0 || m_nActiveThreads == 0)
  {
    if (m_workers.size() > 0)
    {
      CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::StopThread - stopping %zu threads",m_workers.size());

      lock.Leave();

      for (size_t i=0; i<m_workers.size(); i++)
      {
        m_workers[i]->StopThread();
        delete m_workers[i];
      }

      m_workers.clear();
    }

    if (m_bDetached && m_pVecItems)  // when detached - the destructor will get here
    {
      CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::StopThread - deleting %d items", m_pVecItems->Size());
      delete m_pVecItems;
    }

    m_pVecItems = NULL;
    m_nActiveThreads = 0;
  }
}

bool CBackgroundInfoLoader::IsLoading()
{
  return m_nActiveThreads > 0;
}

void CBackgroundInfoLoader::SetObserver(IBackgroundLoaderObserver* pObserver)
{
  m_pObserver = pObserver;
}

void CBackgroundInfoLoader::SetProgressCallback(IProgressCallback* pCallback)
{
  m_pProgressCallback = pCallback;
}

void CBackgroundInfoLoader::LoadItemLinks(CFileItem* pItem, bool bCanBlock)
{
  if (pItem->HasLinksList())
  {
    const CFileItemList* linksList = pItem->GetLinksList();

    if (linksList)
    {
      int numOfLinks = linksList->Size();

      for (int i=0;i<numOfLinks;i++)
      {
        CFileItemPtr linkFileItem = linksList->Get(i);

        CPictureThumbLoader loader;
        loader.LoadItem(linkFileItem.get(), bCanBlock);
      }
    }
  }
}

void CBackgroundInfoLoader::LoadCustomButtons(CFileItem* pItem, bool bCanBlock)
{
  // In case the item has CustomButtons we want to load the remote thumb
  // and replace the path to the loaded one (local)

  if(pItem->HasProperty("NumOfCustomButtons"))
  {
    int NumOfCustomButtons = pItem->GetPropertyInt("NumOfCustomButtons");
    for (int i = 0; i < NumOfCustomButtons; i++)
    {
      CStdString buttonThumb;
      buttonThumb.Format("contextmenuthumb-%i", i);
      
      LoadImageProperty(pItem,buttonThumb,bCanBlock);      
    }
  }
  
  /*
  // Load PLAY provider thumbs data
  if(pItem->HasProperty("play_provider_thumb"))
  {
    LoadImageProperty(pItem,"play_provider_thumb",bCanBlock);
  }
  
  if(pItem->HasProperty("play_provider_thumb_on"))
  {
    LoadImageProperty(pItem,"play_provider_thumb_on",bCanBlock);
  }
  */
}

void CBackgroundInfoLoader::LoadImageProperty(CFileItem* pItem, const CStdString& imagePropertyName, bool bCanBlock)
{
  CStdString thumbPath = pItem->GetProperty(imagePropertyName);
  
  CFileItem tmpItem(thumbPath,false);
  
  CPictureThumbLoader picThumbLoader;
  if(picThumbLoader.LoadItem(&tmpItem,bCanBlock))
  {
    if((tmpItem.GetThumbnailImage()).IsEmpty())
    {
      CLog::Log(LOGERROR,"CBackgroundInfoLoader::LoadImageProperty - Call to CPictureThumbLoader::LoadItem() with item [path=%s][label=%s] FAILED (an empty thumbnail path was returned), therefore preperty [%s=%s] WASN'T updated. [%s=%s] (custb)",(tmpItem.m_strPath).c_str(),(tmpItem.GetLabel()).c_str(),imagePropertyName.c_str(),thumbPath.c_str(),imagePropertyName.c_str(),(pItem->GetProperty(imagePropertyName)).c_str());
    }
    else
    {
      CStdString thumbLocalPath = tmpItem.GetThumbnailImage();
      pItem->SetProperty(imagePropertyName, thumbLocalPath);          
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CBackgroundInfoLoader::LoadImageProperty - Call to CPictureThumbLoader::LoadItem() with item [path=%s][label=%s] FAILED, therefore preperty [%s=%s] WASN'T updated. [%s=%s] (custb)",(tmpItem.m_strPath).c_str(),(tmpItem.GetLabel()).c_str(),imagePropertyName.c_str(),thumbPath.c_str(),imagePropertyName.c_str(),(pItem->GetProperty(imagePropertyName)).c_str());
  }  
}

void CBackgroundInfoLoader::LoadCustomImages(CFileItem* pItem, bool bCanBlock)
{
  //CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::LoadCustomImages - Enter function with item [path=%s][label=%s]. [bCanBlock=%d] (custi)",(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);
  
  for (int nImage=0; nImage < 10; nImage++)
  {
    CStdString strPropName;
    strPropName.Format("Image%d", nImage);
    if (!pItem->HasProperty(strPropName))
    {
      break;
    }
    
    CStdString strImagePath = pItem->GetProperty(strPropName);
    
    //CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::LoadCustomImages - [%d] Going to try and download property [%s] value [ImagePath=%s] for item [path=%s][label=%s]. [bCanBlock=%d] (custi)",nImage,strPropName.c_str(),strImagePath.c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);

    // Get path for cache picture
    Crc32 crc;
    crc.ComputeFromLowerCase(strImagePath);
    CStdString hex;
    hex.Format("%08x", (unsigned __int32) crc);
    CStdString cacheImagePath;
    cacheImagePath.Format("%s\\%c\\%s_image.tbn", g_settings.GetPicturesThumbFolder().c_str(), hex[0], hex.c_str());
    
    cacheImagePath = _P(cacheImagePath);
    
    //CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::LoadCustomImages - [%d] For image [ImagePath=%s] the cacheImagePath is [%s]. Item [path=%s][label=%s]. [bCanBlock=%d] (custi)",nImage,strImagePath.c_str(),cacheImagePath.c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);

    if(CFile::Exists(cacheImagePath))
    {
      // Picture exist in cache
      pItem->SetProperty(strPropName, cacheImagePath);
      
      //CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::LoadCustomImages - [%d] cacheImagePath [%s] already exist and was set to property [%s=%s]. item [path=%s][label=%s]. [bCanBlock=%d] (custi)",nImage,cacheImagePath.c_str(),strPropName.c_str(),(pItem->GetProperty(strPropName)).c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);
    }
    else
    {
      // Picture doesn't exist in cache -> Need to download it
      
      BOXEE::BXCurl http;
      bool success = http.HttpDownloadFile(strImagePath,cacheImagePath,"");
      
      if(success)
      {
        pItem->SetProperty(strPropName, cacheImagePath);
        
        //CLog::Log(LOGDEBUG,"CBackgroundInfoLoader::LoadCustomImages - [%d] successed downloading image. cacheImagePath [%s]  was set to property [%s=%s]. item [path=%s][label=%s]. [bCanBlock=%d] (custi)",nImage,cacheImagePath.c_str(),strPropName.c_str(),(pItem->GetProperty(strPropName)).c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);
      }
      else
      {
        CLog::Log(LOGERROR,"CBackgroundInfoLoader::LoadCustomImages - [%d] Failed to download [ImagePath=%s] for item [path=%s][label=%s]. [bCanBlock=%d] (pthumb)(custi)",nImage,strImagePath.c_str(),(pItem->m_strPath).c_str(),(pItem->GetLabel()).c_str(),bCanBlock);
      }
    }
  }  
}



