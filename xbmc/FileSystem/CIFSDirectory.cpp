#include "system.h"

#ifdef HAS_CIFS
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "Directory.h"
#include "CIFSDirectory.h"
#include "SMBUtils.h"
#include "BrowserService.h"
#include "SMBDirectory.h"
#include "DirectoryCache.h"
#include "Application.h"
#include "utils/log.h"
#include "bxutils.h"
#include "GUISettings.h"
#include "TimeUtils.h"

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mount.h>
#include <netdb.h>
#include <vector>

#include <stdio.h>
#include <BoxeeUtils.h>

using namespace DIRECTORY;


#define MOUNT_ROOT "/tmp/mnt/"

#ifndef CIFS_SUPER_MAGIC
#define CIFS_SUPER_MAGIC 0xFF534D42
#endif


static CStdString GetMountPoint(const CStdString &strType, const CStdString &strHost, const CStdString &volumesName)
{
  CStdString strPath = strType + "_" + strHost + "_" +  volumesName;
  CUtil::URLEncode(strPath);

  return MOUNT_ROOT + strPath + "/";
}

static int UnMountShare(const CStdString &strMountPoint)
{
  return umount(strMountPoint.c_str());
}

static int MountShare(const CStdString &strShareName, const CStdString &strMountPoint, const CStdString& strOptions)
{
  int status;

  if(CUtil::IsMountpoint(strMountPoint))
  {
    UnMountShare(strMountPoint);
  }

  ::CreateDirectory(strMountPoint, NULL);

  status = mount(strShareName.c_str(), strMountPoint.c_str(), "cifs", MS_RDONLY, strOptions.c_str());


  if(status)
  {
    if (errno)
      status = errno;
    return status;
  }

  // test for mountpoint
  if(!CUtil::IsMountpoint(strMountPoint))
  {
    CLog::Log(LOGERROR, "%s - Failed to mount [%s]", __func__, strMountPoint.c_str());
    return -1;
  }

  return status;
}

static bool GetHostAddress(const CStdString& strHostName, CStdString& strHostIp, CStdString& strWorkgroup)
{
  CBrowserService* pBrowser = g_application.GetBrowserService();
  bool bSuccess = false;

  if( pBrowser )
  {
    bSuccess = pBrowser->GetHostAddress(strHostName, strHostIp, strWorkgroup);
  }

  if(!bSuccess)
  {
    bSuccess = CUtil::GetHostByName(strHostName, strHostIp);
    if(!bSuccess)
    {
      CStdString strHostNameDomain = strHostName + ".local";

      bSuccess = CUtil::GetHostByName(strHostNameDomain, strHostIp);
    }

  }

  return bSuccess;
}

CCifsDirectory::CCifsDirectory(void)
{}

CCifsDirectory::~CCifsDirectory(void)
{}

int CCifsDirectory::GetResource(const CURI& path, CFileItem &item)
{
  if(path.GetProtocol() != "smb")
  {
     CLog::Log(LOGERROR, "CCifsDirectory::%s - invalid protocol [%s]", __func__, path.GetProtocol().c_str());
     return false;
  }

  CStdString strHostName = path.GetHostName();
  CStdString strFullPath = path.GetFileName();
  CStdString strVolumeName, strFileName;
  int status;
  const char * cifsAuthMethods[] = {"ntlmssp","ntlmsspi","ntlm","ntlmi"}; // ntlmsspi, ntlmssp, ntlmv2i, ntlmv2, ntlmi, ntlm, nontlm

  if (strFullPath.IsEmpty())
  {
    CLog::Log(LOGERROR, "CCifsDirectory::%s - invalid volume name in path [%s]", __func__, path.Get().c_str());
    return -1;
  }

  int iPos = strFullPath.Find("/", 1);
  if (iPos != -1)
  {
    strVolumeName = strFullPath.substr(0, iPos);
    strFileName   = strFullPath.substr(iPos + 1, strFullPath.length());
  }
  else
  {
    strVolumeName = strFullPath;
  }

  CStdString strMountpoint = GetMountPoint("smb",  strHostName, strVolumeName);

  // path is not mounted - need to mount it
  if(!CUtil::IsMountpoint(strMountpoint) && CUtil::GetFsMagic(strMountpoint+strFileName) != CIFS_SUPER_MAGIC)
  {
    CStdString strOptions, strShareName, strHostIp, strWorkgroup;

    if (CUtil::ValidateIpAddress(strHostName))
    {
      strHostIp = strHostName;
    }

    GetHostAddress(strHostName, strHostIp, strWorkgroup);

    if (!path.GetUserName().IsEmpty())
      strOptions += ",user=" + path.GetUserName();
    else
      strOptions += ",user=guest";

    if(!strWorkgroup.IsEmpty())
      strOptions += ",domain=" + strWorkgroup;

    if (!path.GetPassWord().IsEmpty())
    {
      CStdString pass = path.GetPassWord();
      if(pass.Find(",") != -1)
      {
        pass.Replace(",",",,");
      }
      CUtil::UrlDecode(pass);
      strOptions += ",pass=" + pass;
    }
    if(!strHostIp.IsEmpty())
      strOptions += ",ip=" + strHostIp;

    strOptions += ",iocharset=utf8";

    strShareName.Format("//%s/%s", strHostIp.IsEmpty() ? strHostName : strHostIp, strVolumeName);

    CLog::Log(LOGDEBUG, "CCifsDirectory::%s - mounting CIFS share [%s] ==> [%s]", __func__, strShareName.c_str(), strMountpoint.c_str());

    bool eacces = false;

    status = MountShare(strShareName, strMountpoint, strOptions);

    for(int i=0; ((status == EINVAL) || (status == EOPNOTSUPP) || (status == EACCES)) && i<sizeof(cifsAuthMethods)/sizeof(cifsAuthMethods[0]); i++)
    {
      if(status == EACCES)
      {
        eacces = true;
      }
      CStdString strSecOptions;
      strSecOptions.Format(",nounix,sec=%s", cifsAuthMethods[i]);

      status = MountShare(strShareName, strMountpoint, strOptions + strSecOptions);
    } // for

    if (status)
    {
      if(eacces)
      {
        status = EACCES;
      }
      CLog::Log(LOGINFO, "CCifsDirectory::%s - failed to mount CIFS share, status[%d][%s], share[%s]", __func__, status, strerror(status), strShareName.c_str());
      return status;
    }
  }

  CStdString strHddPath =  strMountpoint + strFileName;

  item.m_strPath = strHddPath;
  item.SetLabel(path.Get());

  item.SetProperty("filename", strFileName);
  item.SetProperty("mountpoint", strMountpoint);

  struct stat st;
  if (stat(strHddPath.c_str(),&st) == 0)
  {
    item.m_dateTime = st.st_mtime;
    item.m_bIsFolder = S_ISDIR(st.st_mode);
    if(item.m_bIsFolder)
      CUtil::AddSlashAtEnd(item.m_strPath);
  }

  return 0;
}

int CCifsDirectory::HandlePath(const CStdString& strPath, CFileItem &item, bool dontOpenPasswordDialog)
{
  int status = EACCES;
  CURI url(strPath);
  CURI origUrl(strPath);
  CStdString strHostName = url.GetHostName();
  CStdString strFileName = url.GetFileName();
  CStdString strUserName = url.GetUserName();
  CStdString strPassword = url.GetPassWord();
  CStdString strKeyPath;
  bool fileNameEmpty = strFileName == "";
  IMAPUSERNAMEPASSWORDS it;

  //test map share
  if(!fileNameEmpty)
  {
    strKeyPath = strHostName + "/" + strFileName;

    it = g_passwordManager.m_mapCIFSPasswordCache.find(strKeyPath);
    if(it != g_passwordManager.m_mapCIFSPasswordCache.end())
    {
      strUserName = g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first;
      strPassword = g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second;

      url.SetUserName(strUserName);
      url.SetPassword(strPassword);

      status = GetResource(url, item);
      if(status != EACCES && status != EKEYEXPIRED)
        return status;
    }
  }

  //test map host
  strKeyPath = strHostName;

  it = g_passwordManager.m_mapCIFSPasswordCache.find(strKeyPath);
  if(it != g_passwordManager.m_mapCIFSPasswordCache.end())
  {
    strUserName = g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first;
    strPassword = g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second;

    url.SetUserName(strUserName);
    url.SetPassword(strPassword);

    status = GetResource(url, item);

    if(!fileNameEmpty && !status)
    {
      strKeyPath = strHostName + "/" + strFileName;
      g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first = strUserName;
      g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second = strPassword;
    }
    if(status != EACCES && status != EKEYEXPIRED)
      return status;
  }

  //test current
  status = GetResource(origUrl, item);

  if(!status && strUserName != "")
  {
    if(!fileNameEmpty)
    {
      strKeyPath = strHostName + "/" + strFileName;
    }
    else
    {
      strKeyPath = strHostName;
    }

    g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first = origUrl.GetUserName();
    g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second = origUrl.GetPassWord();
  }

  if( status != EACCES && status != EKEYEXPIRED)
    return status;

  //test default
  if(!g_guiSettings.GetString("smb.username").IsEmpty())
  {
    strUserName = g_guiSettings.GetString("smb.username");
    strPassword = g_guiSettings.GetString("smb.password");

    url.SetUserName(strUserName);
    url.SetPassword(strPassword);

    status = GetResource(url, item);

    if(!status)
    {
      if(!fileNameEmpty)
      {
        strKeyPath = strHostName + "/" + strFileName;
      }
      else
      {
        strKeyPath = strHostName;
      }
      g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first = strUserName;
      g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second = strPassword;
    }
    if(status != EACCES && status != EKEYEXPIRED)
      return status;
  }

  //pop dialog
  if(!dontOpenPasswordDialog)
  {
    g_passwordManager.SetCIFSShare(strPath);
    if(g_passwordManager.GetCIFSShareUserPassword())
    {
      CURI urlnew(g_passwordManager.GetCIFSShare());

      strUserName = urlnew.GetUserName();
      strPassword = urlnew.GetPassWord();

      status = GetResource(urlnew, item);

      if(!status)
      {
        if(!fileNameEmpty)
        {
          strKeyPath = strHostName + "/" + strFileName;
        }
        else
        {
          strKeyPath = strHostName;
        }
        g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].first = strUserName;
        g_passwordManager.m_mapCIFSPasswordCache[strKeyPath].second = strPassword;
      }
    }
    else
    {
      item.SetProperty("NotErrorDontShowErrorDialog",true);
    }
  }
  return status;
}

bool CCifsDirectory::ScanShares(CFileItemList &items)
{
  CSMBDirectory smbDir;
  CFileItemList list;
  unsigned int nStartTime = CTimeUtils::GetTimeMS();
  int nTries = 3;
  
  while(nTries--)
  {
    smbDir.GetDirectory("smb://", list);

    if(!list.IsEmpty())
      break;

    Sleep(1000);
  }

  if(list.IsEmpty())
  {
    CLog::Log(LOGERROR, "CCifsDirectory::%s - failed to enum samba workgroups, error %s", __func__, strerror(errno));
    return false;
  }

  // loop over all workgroups
  for(int i=0; i<list.Size(); i++)
  {
    CURI wg(list[i]->m_strPath);
    CFileItemList l;
    
    nTries = 3;
    while(nTries--)
    {
      smbDir.GetDirectory(list[i]->m_strPath, l);
      if(!l.IsEmpty())
        break;

      Sleep(1000);
    }

    // loop over all computers in the workgroup
    for(int j=0; j<l.Size(); j++)
    {
      CURI u(l[j]->m_strPath);

      if(l.IsEmpty())
      {
        CLog::Log(LOGERROR, "CCifsDirectory::%s - workgroup [%s] seems to be empty", __func__, wg.GetHostName().c_str());
        continue;
      }
      
      l[j]->SetProperty("Workgroup", wg.GetHostName());
      l[j]->SetProperty("HostName", u.GetHostName());      
    }
    
    items.Append(l);
  }

  CLog::Log(LOGDEBUG, "CifsDirectory::ScanShares - took %d ms, found %d computers", CTimeUtils::GetTimeMS() - nStartTime, items.Size());

  return true;
}

bool CCifsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);
  bool bResult = false;

  if(url.GetProtocol() != "smb")
  {
     CLog::Log(LOGERROR, "CCifsDirectory::%s - invalid protocol [%s]", __func__, url.GetProtocol().c_str());
     return false;
  }

  if(SMBUtils::ProcessPath(strPath, items))
  {
    return true;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

  if( strPath == "smb://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::SMB_SHARE, items );
    return true;
  }

  if (strPath == "smb://all")
  {
    return ScanShares(items);
  }

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strPath, items))
  {
    return true;
  }

  CStdString fileName = url.GetFileName();

  if (fileName.IsEmpty())
  {
    CSMBDirectory smbDir;

    bResult = smbDir.GetDirectory(strPath, items);

    CURI url(strPath);
    if(url.GetUserName() != "" && bResult == 1)
    {
      g_passwordManager.m_mapCIFSPasswordCache[url.GetHostName()].first = url.GetUserName();
      g_passwordManager.m_mapCIFSPasswordCache[url.GetHostName()].second = url.GetPassWord();
    }
  }
  else
  {
    CFileItem item;
    int status = EACCES;

    status = HandlePath(strPath, item, items.GetPropertyBOOL("notOpenPassword"));


    if(item.GetPropertyBOOL("NotErrorDontShowErrorDialog"))
    {
      items.SetProperty("NotErrorDontShowErrorDialog",true);
      return false;
    }

    if (status && status != EACCES)
    {
      CLog::Log(LOGERROR, "CCifsDirectory::%s - failed to get resource for path [%s]", __func__, url.Get().c_str());
      return false;
    }

    // due a bug in scandir(), CHDDDirectory::GetDirectory sometimes fails to read cifs mountpoints
    bResult = CUtil::GetHDDDirectory(item.m_strPath, items);

    for(int i=0; i<items.Size(); i++)
    {
      // build smb path
      if(items[i]->m_bIsFolder)
      {
        items[i]->SetProperty("isNetwork",true);
      }
      CStdString smbPath = items[i]->m_strPath;
      smbPath.Replace(item.m_strPath, strPath);

      if (smbPath.length() >= strPath.length())
      {
        CStdString strNewFilename = smbPath.substr(strPath.length());

        CStdString strSmbFileName;
        CStdString strFileName = item.GetProperty("filename");

        strSmbFileName = strNewFilename;

        if (CUtil::HasSlashAtEnd(strPath))
          items[i]->m_strPath.Format("%s%s", strPath, strSmbFileName);
        else
          items[i]->m_strPath.Format("%s/%s", strPath, strSmbFileName);
      }
    }
  }

  return bResult;
}

bool CCifsDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
#endif
