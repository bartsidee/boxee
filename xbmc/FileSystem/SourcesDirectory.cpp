

#include "SourcesDirectory.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "Application.h"
#include "FileItem.h"
#include "DetectDVDType.h"
#include "MediaManager.h"
#include "URL.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "SpecialProtocol.h"
#include "MultiPathDirectory.h"
#include "FactoryDirectory.h"
#include "Directory.h"

namespace DIRECTORY
{

CSourcesDirectory::CSourcesDirectory() {
  // TODO Auto-generated constructor stub

}

CSourcesDirectory::~CSourcesDirectory() {
  // TODO Auto-generated destructor stub
}

bool CSourcesDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  
  CStdString strParams;
  CStdString strDir;
  CStdString strFile;
  std::map<std::string, std::string> mapParams;

  CURI url(strPath);
  CLog::Log(LOGDEBUG, "CSourcesDirectory::GetDirectory, GetHostName = %s, GetShareName = %s, GetDomain = %s, GetFileName = %s, GetOptions = %s (bsd) (browse)",
        url.GetHostName().c_str(), url.GetShareName().c_str(), url.GetDomain().c_str(), url.GetFileName().c_str(), url.GetOptions().c_str());

  // Parse url using boxee db utility
  if (!BoxeeUtils::ParseBoxeeDbUrl(strPath, strDir, strFile, mapParams)) 
  {
    return false;
  }
  
  // Retreive 
  VECSOURCES * pVecShares = g_settings.GetSourcesFromType( strDir );
  
  if (pVecShares) 
  {
    for (IVECSOURCES it = pVecShares->begin(); it != pVecShares->end(); it++)
    {
      // Add shares themselves to the list of results
      if (!CUtil::IsLastFM(it->strPath) && 
          !CUtil::IsShoutCast(it->strPath) && 
          !CUtil::IsRSS(it->strPath) &&
          !CUtil::IsApp(it->strPath) &&
          !CUtil::IsScript(it->strPath) && 
          !(it->strPath.Left(9).Equals("plugin://")) &&
          !(it->strPath.Left(6).Equals("mms://")) &&
          !(it->strPath.Left(7).Equals("http://"))) 
      {
        CFileItemPtr source (new CFileItem());

        if(it->strPath.Find("afp://") != -1)
        {
          CURI urlPath(it->strPath);
          urlPath.SetUserName("");
          urlPath.SetPassword("");
          source->m_strPath = urlPath.Get();
        }
        else
        {
          source->m_strPath = it->strPath;
        }
        source->SetLabel(it->strName);
        source->SetIconImage(it->m_strThumbnailImage);
        source->SetProperty("isshare", 1);
        source->m_bIsFolder = true;
        
        if(((source->m_strPath).Left(6)).Equals("smb://"))
        {
          source->SetProperty("isSMB", true);
          source->SetProperty("IsNetwork", true);
        }
        else if(((source->m_strPath).Left(7)).Equals("upnp://"))
        {
          source->SetProperty("isUPNP", true);
          source->SetProperty("IsNetwork", true);
        }
        else if(((source->m_strPath).Left(6)).Equals("ftp://"))
        {
          source->SetProperty("isFTP", true);
          source->SetProperty("IsNetwork", true);
        }
        else
        {
          // Do nothing
        }
        
        items.Add(source);        
      }
    }
  }
 
#ifdef HAS_DVD_DRIVE 
  if (MEDIA_DETECT::CDetectDVDMedia::IsDiscInDrive())
  {
    CCdInfo* pInfo = CDetectDVDMedia::GetCdInfo();
    if (pInfo != NULL && (pInfo->IsIso9660(1) || pInfo->IsIso9660Interactive(1)))
    {
      CFileItemPtr pItem (new CFileItem(g_localizeStrings.Get(51011)));
      pItem->m_strPath = "/media/cdrom";
      pItem->SetProperty("IsDVDDrive","1");
      pItem->m_bIsFolder = true;
      items.Add(pItem);
    }
  }
#endif

  /*
  CFileItemPtr networkShare (new CFileItem(g_localizeStrings.Get(51302)));
  networkShare->m_strPath = "network://protocols";
  networkShare->SetProperty("IsShare","1");
  networkShare->SetProperty("isnetwork", 1);
  networkShare->m_bIsFolder = true;
  items.Add(networkShare);
  */

  // Add one more item, for the unresolved video folders

  CFileItemList resultList;

  if (DIRECTORY::CDirectory::GetDirectory("boxeedb://unresolvedVideoFiles/?limit=1",resultList) && !resultList.IsEmpty())
  {
    CFileItemPtr pItem ( new CFileItem(g_localizeStrings.Get(51361)));
    pItem->m_strPath = "boxeedb://unresolvedVideoFiles";
    pItem->m_bIsFolder = true;
    pItem->SetProperty("isgroup", true);
    items.Add(pItem);
  }

  return true;
}

bool CSourcesDirectory::Exists(const char* strPath)
{
  return true;
}

}
