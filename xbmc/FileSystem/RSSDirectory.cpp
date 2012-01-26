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

#ifdef _LINUX
#include "linux/PlatformDefs.h"
#endif

using namespace XFILE;
using namespace DIRECTORY;
using namespace BOXEE;

CRSSDirectory::CRSSDirectory()
{
  m_Finished = false;
  BOXEE::Boxee::GetInstance().AddListener(this);
}

CRSSDirectory::~CRSSDirectory()
{
  BOXEE::Boxee::GetInstance().RemoveListener(this);
}

bool CRSSDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
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

  m_Finished = false;
  m_strUrl = newURL;

  Crc32 crc;
  crc.ComputeFromLowerCase(newURL);

  CStdString strLocalFile;
  strLocalFile.Format("special://temp/rss-%08x-%lu.xml", (unsigned __int32)crc, time(NULL));

  if (!BOXEE::Boxee::GetInstance().AsyncLoadUrl(newURL, _P(strLocalFile), "rss-load", NULL))
  {
    CGUIDialogOK::ShowAndGetInput(51014,0,0,0);
    return false;
  }

  int nRounds =0;
  while (!g_application.m_bStop && nRounds < 150 && !m_Finished) // wait max 15 sec for answer
  {
    Sleep(100);
    nRounds++;
  }

  m_feed.GetItemList(items);

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
    m_feed.Init(strLocalFile, strUrl);
    m_feed.ReadFeed();
    ::DeleteFile(strLocalFile.c_str());

    m_Finished = true;	  
  }
}

void CRSSDirectory::OnFileDownloadFailed(const std::string &strLocalFile, 
                                     const std::string &strUrl, 
                                     const std::string &strTransactionId,
                                     void *pArg)
{
  CLog::Log(LOGDEBUG,"file download failed (%s)", strUrl.c_str());
  m_Finished = true;	  
}

DIR_CACHE_TYPE CRSSDirectory::GetCacheType(const CStdString& strPath) const
{
  return m_cacheDirectory;
}


