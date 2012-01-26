#include "RSSDirectory.h"

#include "SDL/SDL.h"
#include "Application.h"
#include "RSSDirectory.h"
#include "Util.h"
#include "utils/log.h"
#include "../lib/libBoxee/bxrssreader.h"
#include "RssSourceManager.h"
#include "GUIDialogProgress.h"
#include "GUIDialogOK.h"
#include "DirectoryCache.h"
#include "Crc32.h"
#include "GUIWindowManager.h"
#include "SpecialProtocol.h"
#include "TimeUtils.h"
#include "SingleLock.h"
#include "guilib/LocalizeStrings.h"

#ifdef _LINUX
#include "linux/PlatformDefs.h"
#endif

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

#define REQUEST_WAIT_PERIOD 120000 //2 minutes

CRSSDirectory::CRSSDirectory()
{
  m_pOpFinishedMutex = SDL_CreateMutex();
  m_pOpFinishedCond = SDL_CreateCond();

  BOXEE::Boxee::GetInstance().AddListener(this);
}

CRSSDirectory::~CRSSDirectory()
{
  // first remove us from the listeners list, then delete the cond and mutex
  BOXEE::Boxee::GetInstance().RemoveListener(this);

  if (m_pOpFinishedCond)
  {
    SDL_DestroyCond(m_pOpFinishedCond);
  }

  if (m_pOpFinishedMutex)
  {
    SDL_DestroyMutex(m_pOpFinishedMutex);
  }
}

bool CRSSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory - path [%s]", strPath.c_str());

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  CStdString strURL = strPath;
  CStdString newURL;

  CStdString strRoot = strPath;
  if (CUtil::HasSlashAtEnd(strRoot))
    strRoot.Delete(strRoot.size() - 1);

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strRoot, items))
  {
    return true;
  }

  // Remove the rss:// prefix and replace it with http://
  if (strURL.Left(7) == "http://")
  {
    newURL = strURL;
  }
  else
  {
    strURL.Delete(0,6);
    
    // if first symbol is '/', we have local file
    if (strURL.Left(1) == "/")
    {
      newURL = "file://";
    }
    else 
    {
      newURL = "http://";
    }
    newURL = newURL + strURL;
  }
  
  // Remove the last slash
  if (CUtil::HasSlashAtEnd(newURL))
  {
    CUtil::RemoveSlashAtEnd(newURL);
  }

  // Create new thread and run the feed retreival from it
  // In order to allow progress dialog and cancel operation

  m_strUrl = newURL;

  Crc32 crc;
  crc.ComputeFromLowerCase(newURL);

  CStdString strLocalFile;
  strLocalFile.Format("special://temp/rss-%08x-%lu.xml", (unsigned __int32)crc, CTimeUtils::GetTimeMS());

  CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory - going to load url [%s] to file [%s]", newURL.c_str(), strLocalFile.c_str());

  if (!BOXEE::Boxee::GetInstance().AsyncLoadUrl(newURL, _P(strLocalFile), "rss-load", NULL))
  {
    CGUIDialogOK::ShowAndGetInput(51014,0,0,0);
    return false;
  }

  SDL_LockMutex(m_pOpFinishedMutex);
  int result = SDL_CondWaitTimeout(m_pOpFinishedCond,m_pOpFinishedMutex,REQUEST_WAIT_PERIOD);
  SDL_UnlockMutex(m_pOpFinishedMutex);

  m_feed.GetItemList(items);

  if (result != 0)
  {
    m_cacheDirectory = DIR_CACHE_NEVER;
    // set this property so that the UI will handle the timeout
    CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory, loading timed out, path [%s] loaded:%d out of %d", strPath.c_str(), items.Size(),items.GetPageContext().m_itemsPerPage);
    items.SetProperty("isRequestTimedOut",true);
  }

  CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory - done loading url, got [%d] items (result=[%d])",items.Size(),result);

  if (items.Size() == 0)
  {
    m_cacheDirectory = DIR_CACHE_NEVER;
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory - Going to add [DefaultSortLabel] property to each item. [path=%s][NumOfItems=%d] (vns)",strPath.c_str(),items.Size());

    for(int i=0; i<items.Size(); i++)
    {
      CFileItemPtr item = items[i];

      char pos[5];
      sprintf(pos,"%d",i+1);

      item->SetProperty("DefaultSortLabel",pos);
 
      //CLog::Log(LOGDEBUG,"CRSSDirectory::GetDirectory - For item [path=%s] set property [DefaultSortLabel=%s] (vns)", (item->m_strPath).c_str(),(item->GetProperty("DefaultSortLabel")).c_str());
    }

    items.SetProperty("preferredsortmethod", SORT_METHOD_DEFAULT);
    items.SetProperty("preferredsortorder", SORT_ORDER_ASC);
  }

  return true;
}

void CRSSDirectory::OnFileDownloaded(const std::string &strLocalFile, 
		const std::string &strUrl, 
		const std::string &strTransactionId,
		void *pArg)
{
  if (strUrl == m_strUrl && CFile::Exists(strLocalFile))
  {
    CLog::Log(LOGDEBUG,"CRSSDirectory::OnFileDownloaded file download finished successfully (%s)", strUrl.c_str());
    m_feed.Init(strLocalFile, strUrl);
    m_feed.ReadFeed();
    ::DeleteFile(strLocalFile.c_str());
    SignalOp();
  }
}

void CRSSDirectory::OnFileDownloadFailed(const std::string &strLocalFile, 
                                     const std::string &strUrl, 
                                     const std::string &strTransactionId,
                                     void *pArg)
{
  CLog::Log(LOGDEBUG,"CRSSDirectory::OnFileDownloadFailed file download failed (%s)", strUrl.c_str());
  SignalOp();
}

void CRSSDirectory::OnForcedStop()
{
  SignalOp();
}

void CRSSDirectory::SignalOp()
{
  SDL_LockMutex(m_pOpFinishedMutex);
  SDL_CondSignal(m_pOpFinishedCond);
  SDL_UnlockMutex(m_pOpFinishedMutex);
}

DIR_CACHE_TYPE CRSSDirectory::GetCacheType(const CStdString& strPath) const
{
  return m_cacheDirectory;
}


