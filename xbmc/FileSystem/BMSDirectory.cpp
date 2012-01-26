#include "system.h"

#ifdef HAS_BMS
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "Directory.h"
#include "AfpDirectory.h"
#include "BrowserService.h"
#include "DirectoryCache.h"
#include "Application.h"
#include "utils/log.h"
#include "bxutils.h"
#include "Util.h"
#include "ZeroconfAvahi.h"
#include "ZeroconfBrowser.h"
#include "RSSDirectory.h"
#include "BMSDirectory.h"



using namespace DIRECTORY;

#define DEFAULT_PORT "8088"



static bool ScanBmsShares(CFileItemList &items)
{
  CFileItemList shares;

  CDirectory::GetDirectory("zeroconf://", shares);

  for(int i=0; i<shares.Size(); i++)
  {
    CStdString protocol = shares[i]->GetProperty("protocol");

    if(protocol.ToLower() == "bms")
    {
      CFileItemPtr pItem(new CFileItem());
      CStdString name = shares[i]->GetProperty("name");
      CStdString domain = shares[i]->GetProperty("domain");

      if(pItem->m_bIsFolder)
      {
        pItem->SetProperty("isNetwork",true);
      }
      pItem->SetLabel(name);
      pItem->m_bIsFolder = true;

      pItem->m_strPath.Format("bms://%s.%s", name, domain);
      CUtil::AddSlashAtEnd(pItem->m_strPath);

      items.Add(pItem);
    }
  }

  return items.Size() != 0;
}

static bool
GetHostAddress(const CStdString& strHostName, CStdString& strHostIp, CStdString& strHostPort)
{
  CStdString domain = "local";
  CStdString name;
  int iPos = -1;

  iPos = strHostName.ReverseFind('.');
  if(iPos != -1)
  {
    name = strHostName.substr(0, iPos);
    domain = strHostName.substr(iPos + 1, strHostName.length());
  }

  CZeroconfBrowser::ZeroconfService service(name, BOXEE_SERVICE_NAME, domain);

  if(!CZeroconfBrowser::GetInstance()->ResolveService(service))
    return false;

  strHostIp = service.GetIP();
  strHostPort.Format("%d", service.GetPort());

  return true;
}

bool CBmsDirectory::GetResource(const CURI& path, CFileItem &item)
{
  CStdString strFilename, strHostname, strHostIp;
  CStdString strPath = path.Get();
  CStdString strPort;

  if(CUtil::HasSlashAtEnd(strPath))
    CUtil::RemoveSlashAtEnd(strPath);

  strHostname = path.GetHostName();
  strFilename = path.GetFileName();

  if (CUtil::ValidateIpAddress(strHostname))
  {
    strHostIp = strHostname;
  }
  else if (false == GetHostAddress(strHostname, strHostIp, strPort))
  {
    CLog::Log(LOGERROR, "CBmsDirectory::%s - failed to found host address [%s]", __func__, strHostname.c_str());
    return false;

  }
  if(item.m_bIsFolder)
  {
    item.SetProperty("isNetwork",true);
  }
  item.m_strPath.Format("http://%s:%s/%s", strHostIp, strPort, strFilename);

  return true;
}


CBmsDirectory::CBmsDirectory(void)
{}

CBmsDirectory::~CBmsDirectory(void)
{}


bool CBmsDirectory::ReadDir(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);
  CStdString strRssUrl, strHostIp;
  CStdString strHostName = url.GetHostName();
  CStdString strFileName = url.GetFileName();
  CStdString strPort;

  if (!strFileName.IsEmpty())
  {
    if (strFileName.c_str()[0] != '/')
      strFileName = "/" + strFileName;

    CUtil::URLEncode(strFileName);
  }

  if (CUtil::ValidateIpAddress(strHostName))
  {
    strHostIp = strHostName;
  }
  else if (false == GetHostAddress(strHostName, strHostIp, strPort))
  {
    CLog::Log(LOGERROR, "CBmsDirectory::%s - failed to found host address [%s]", __func__, strHostName.c_str());
    return false;

  }

  if(strPort == "")
  {
    strPort = DEFAULT_PORT;
  }
  strRssUrl.Format("rss://%s:%s/rss?path=%s", strHostIp, strPort, strFileName);

  CDirectory::GetDirectory(strRssUrl, items);

  for(int i=0; i<items.Size(); i++)
  {
    CURI u(items[i]->m_strPath);
    CStdString strPath;

    // hack for playlist files
    if (items[i]->IsPlayList() && items[i]->m_bIsFolder)
    {
      items[i]->m_bIsFolder = false;
    }

    items[i]->ClearProperty("isinternetstream");
    items[i]->ClearProperty("isrss");

    if(items[i]->m_bIsFolder)
    {
      std::map<CStdString, CStdString> optionMap = u.GetOptionsAsMap();

      strPath = optionMap["path"];
      CUtil::UrlDecode(strPath);
    }
    else
    {
      strPath = u.GetFileName();
      if (strPath.c_str()[0] != '/')
        strPath = "/" + strPath;
    }

    CStdString strSlash;

    if(items[i]->m_bIsFolder && !CUtil::HasSlashAtEnd(strPath))
      CUtil::AddSlashAtEnd(strPath);

    items[i]->m_strPath.Format("bms://%s%s", strHostName, strPath);
  }

  return true;
}

bool CBmsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  if(url.GetProtocol() != "bms")
  {
     CLog::Log(LOGERROR, "CBmsDirectory::%s - invalid protocol [%s]", __func__, url.GetProtocol().c_str());
     return false;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

  if( strPath == "bms://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::BMS_SHARE, items );
    return true;
  }

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strPath, items))
  {
    return true;
  }

  if(strPath == "bms://all")
  {
     return ScanBmsShares(items);
  }

  return ReadDir(strPath, items);
}

bool CBmsDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
#endif
