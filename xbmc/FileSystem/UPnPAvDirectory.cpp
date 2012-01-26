#include "system.h"

#ifdef HAS_UPNP_AV
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "Directory.h"
#include "BrowserService.h"
#include "DirectoryCache.h"
#include "Application.h"
#include "utils/log.h"
#include "bxutils.h"
#include "Util.h"
#include "File.h"
#include "RSSDirectory.h"
#include "UPnPAvDirectory.h"
#include "GUIWindowManager.h"
#include "GUIDialogProgress.h"

#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/param.h>

#ifdef HAS_EMBEDDED
#include "HalServices.h"
#endif

#define UPNP_MNT "/tmp/mnt/upnp"

using namespace DIRECTORY;
using namespace XFILE;


static bool
ScanUPnPShares(CFileItemList &items)
{
  bool bMounted = false;
  CStdString strDevicesFile = CStdString(UPNP_MNT) + "/devices";

  bMounted = CUtil::IsMountpoint(UPNP_MNT) && CFile::Exists(strDevicesFile);

  if(!bMounted)
  {
    CLog::Log(LOGERROR, "CUPnPAvDirectory:%s - UPnP mountpoint doesnt exist", __func__);
    return false;
  }

  CFile fDevices;
  char szDevice[1024] = {0};


  if (!fDevices.Open(strDevicesFile))
  {
    CLog::Log(LOGERROR, "CUPnPAvDirectory:%s - Failed to open [%s]", __func__, strDevicesFile.c_str());
    return false;
  }

  items.Clear();

  while(fDevices.ReadString(szDevice, sizeof(szDevice)))
  {
    CFileItemPtr pItem(new CFileItem(szDevice));
    CStdString strDevice = szDevice;

    CUtil::URLEncode(strDevice);

    pItem->m_bIsFolder = true;
    pItem->m_strPath.Format("upnp://%s", strDevice);

    items.Add(pItem);
  }

  fDevices.Close();

  return items.Size() != 0;
}

bool
CUPnPAvDirectory::ReadDir(const CURI& url, CFileItemList &items)
{
  CStdString strFileName = url.GetFileName();
  CStdString strHostName = url.GetHostName();
  CStdString strHddPath;

  CUtil::UrlDecode(strHostName);

  strHddPath.Format("%s/%s/%s", UPNP_MNT, strHostName, strFileName);

  // on UPnP GetHDDDirectory works much faster than CDirectory::GetHDDDirectory
  bool bResult = CUtil::GetHDDDirectory(strHddPath, items);

  for(int i=0; i<items.Size(); i++)
  {
    CUtil::RemoveSlashAtEnd(items[i]->m_strPath);

    CStdString strName = CUtil::GetFileName(items[i]->m_strPath);

    if(!CUtil::HasSlashAtEnd(url.Get()))
      items[i]->m_strPath.Format("%s/%s", url.Get(),strName);
    else
      items[i]->m_strPath.Format("%s%s", url.Get(),strName);

    if(items[i]->m_bIsFolder)
      CUtil::AddSlashAtEnd(items[i]->m_strPath);
  }

  return bResult;
}

void
CUPnPAvDirectory::GetFriendlyPath(CStdString& path)
{
  CURI url(path);
  CStdString strHostName = url.GetHostName();
  CStdString strHostNameDecoded = strHostName;

  CUtil::UrlDecode(strHostNameDecoded);

  path.Replace(strHostName, strHostNameDecoded);
}

bool
CUPnPAvDirectory::GetResource(const CURI& path, CFileItem &item)
{
  CStdString strFileName = path.GetFileName();
  CStdString strHostName = path.GetHostName();
  CStdString strHddPath;

  CUtil::UrlDecode(strHostName);

  strHddPath.Format("%s/%s/%s", UPNP_MNT, strHostName, strFileName);

  if(!CFile::Exists(strHddPath))
  {
    CLog::Log(LOGERROR, "CUPnPAvDirectory::%s - upnp node [%s] for [%s] doesnt exists", __func__, strHddPath.c_str(), path.Get().c_str());
    return false;
  }

  item.m_strPath = strHddPath;
  item.SetLabel(path.Get());

  struct stat st;
  if (stat(strHddPath.c_str(),&st) == 0)
  {
    item.m_bIsFolder = S_ISDIR(st.st_mode);
  }

  return true;
}


CUPnPAvDirectory::CUPnPAvDirectory(void)
{}

CUPnPAvDirectory::~CUPnPAvDirectory(void)
{}



bool CUPnPAvDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  if(url.GetProtocol() != "upnp")
  {
     CLog::Log(LOGERROR, "CUPnPAvDirectory::%s - invalid protocol [%s]", __func__, url.GetProtocol().c_str());
     return false;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

  if( strPath == "upnp://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::UPNP_SHARE, items );
    return true;
  }

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strPath, items))
  {
    return true;
  }

  if(strPath == "upnp://all")
  {
     bool bSuccess = ScanUPnPShares(items);
     if(!bSuccess)
     {
       IHalServices& client = CHalServicesFactory::GetInstance();
       client.DjmountRestart();

       CLog::Log(LOGWARNING, "CUPnPAvDirectory::%s - failed to scan UPnP shares, retrying...", __func__);

       CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
       if (progress)
       {
         CStdString strId;
         strId.Format("%d-upnp-scan-retry", CThread::GetCurrentThreadId());
         progress->StartModal(strId);
         progress->Progress();
       }

       // hopefully djmount will be able to find all shares
       Sleep(5000);

       bSuccess = ScanUPnPShares(items);

       progress->Close();
     }

     return bSuccess;
  }

  return ReadDir(url, items);
}

bool CUPnPAvDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
#endif
