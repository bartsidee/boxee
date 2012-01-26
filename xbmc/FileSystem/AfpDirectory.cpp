#include "system.h"

#ifdef HAS_NFS
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
#include "../Util.h"
#include "Util.h"
#include "ZeroconfAvahi.h"
#include "ZeroconfBrowser.h"
#include "GUISettings.h"
#include "GUIPassword.h"
#include "StringUtils.h"

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mount.h>
#include <netdb.h>

extern "C" {
#include <afp.h>
}

using namespace DIRECTORY;
using namespace std;

#define MOUNT_ROOT "/tmp/mnt/"

#ifndef AFP_SUPER_MAGIC
#define AFP_SUPER_MAGIC 0x65735546
#endif


static bool ScanAfpShares(CFileItemList &items)
{
  CFileItemList shares;

  CDirectory::GetDirectory("zeroconf://", shares);

  for(int i=0; i<shares.Size(); i++)
  {
    CStdString protocol = shares[i]->GetProperty("protocol");

    if(protocol.ToLower() == "afp")
    {
      CFileItemPtr pItem(new CFileItem());
      CStdString name = shares[i]->GetProperty("name");

      pItem->SetLabel(name);
      pItem->m_bIsFolder = true;
      pItem->SetProperty("isNetwork",true);

      pItem->m_strPath.Format("afp://%s", shares[i]->GetProperty("hostName"));
      CUtil::AddSlashAtEnd(pItem->m_strPath);

      items.Add(pItem);
    }
  }

  return items.Size() != 0;
}


/* Converts an integer value to its hex character*/
static char to_hex(char code)
{
  static char hex[] = "0123456789abcdef";
  return hex[code & 15];
}

static CStdString url_encode(char *str)
{
  char *pstr = str, *buf = (char*)malloc(strlen(str) * 3 + 1), *pbuf = buf;
  CStdString strResult;

  while (*pstr) {
    if (isalnum(*pstr) || *pstr == '-' || *pstr == '_' || *pstr == '.' || *pstr == '~')
      *pbuf++ = *pstr;
    else if (*pstr == ' ')
      *pbuf++ = '+';
    else
      *pbuf++ = '%', *pbuf++ = to_hex(*pstr >> 4), *pbuf++ = to_hex(*pstr & 15);
    pstr++;
  }
  *pbuf = '\0';

  strResult = buf;
  free(buf);

  return strResult;
}

static void RunCommand(const CStdString& command, int& status, CStdString& strOutput)
{
  char fullPathCommand[2048];

  strcpy(fullPathCommand, command.c_str());


  status = -1;
  strOutput = "";

  FILE* f = popen(fullPathCommand, "r");
  if (!f)
  {
    return;
  }

  char szBuffer[1024];
  ssize_t nBytes = 0;

  while (!feof(f))
  {
    memset(&szBuffer[0], 0, 1024);
    nBytes = fread(&szBuffer[0], 1, 1024, f);
    if (nBytes > 0)
    {
      strOutput.append(&szBuffer[0]);
    }
  }

  status = pclose(f);
}

static int UnMountShare(const CStdString &strMountPoint)
{
  return umount(strMountPoint.c_str());
}

static bool MountShare(const CStdString &afpPath, const CStdString &strMountPoint)
{
  CStdString cmd;

  if(CUtil::IsMountpoint(strMountPoint))
  {
    UnMountShare(strMountPoint);
  }

  ::CreateDirectory(strMountPoint, NULL);

  cmd = "afp_client mount " + afpPath + CStdString(" ") + strMountPoint;

  system(cmd.c_str());

  // test for mountpoint
  if(!CUtil::IsMountpoint(strMountPoint))
  {
    CLog::Log(LOGERROR, "%s - Failed to mount [%s]", __func__, strMountPoint.c_str());
    return false;
  }

  return true;
}

static CStdString GetMountPoint(const CStdString &strType, const CStdString &strHost, const CStdString &strVolume)
{
  CStdString v;
  for (int i = 0; i < (int)strVolume.size(); ++i)
  {
    int kar = (unsigned char)strVolume[i];
    if (isalnum(kar)) v += kar;
    else
    {
      CStdString strTmp;
      strTmp.Format("%d", kar);
      v += strTmp;
    }
  }

  CStdString strPath = strType + "_" + strHost + "_" +  v;

  return MOUNT_ROOT + strPath;
}


static bool BuildAfpUrl(const CURI& url, struct afp_url* afp_url)
{
  struct passwd * passwd;
  CStdString strUserName = url.GetUserName();
  CStdString strPassword =  url.GetPassWord();

  afp_default_url(afp_url);

  if (afp_parse_url(afp_url, url.Get().c_str(),0) != 0)
  {
    CLog::Log(LOGERROR, "CAfpDirectory::%s - failed to parse url %s", __func__, url.Get().c_str());
    return false;
  }

  if (strUserName.IsEmpty() && strlen(afp_url->uamname) == 0)
  {
    CStdString strUamName = "No User Authent";

    strncpy(afp_url->uamname, strUamName.c_str(),strUamName.length());
    passwd = getpwuid(getuid());
    strncpy(afp_url->username, passwd->pw_name,AFP_MAX_USERNAME_LEN);
  }

  return true;
}

static bool HasReadPermissions(const CStdString& path, bool bGuest)
{
  struct __stat64 stat;
  bool bResult = true;

  if (_stat64(path.c_str(),&stat) == 0)
  {
    bResult = bGuest ? stat.st_mode & S_IROTH : stat.st_mode & S_IRUSR;
  }

  return bResult;
}

bool CAfpDirectory::GetResource(const CURI& path, CFileItem &item)
{
  if(path.GetProtocol() != "afp")
  {
     CLog::Log(LOGERROR, "CAfpDirectory::%s - invalid protocol [%s]", __func__, path.GetProtocol().c_str());
     return false;
  }

  struct afp_url afp_url;

  if (!BuildAfpUrl(path, &afp_url))
  {
    return false;
  }

  CStdString strMountpoint = GetMountPoint("afp",  afp_url.servername, afp_url.volumename);
  CStdString strFileName = afp_url.path;

  // path is not mounted - need to mount it
  if(!CUtil::IsMountpoint(strMountpoint) && CUtil::GetFsMagic(strMountpoint+strFileName) != AFP_SUPER_MAGIC)
  {
    CStdString afpPath;

    if(strlen(afp_url.username) != 0)
      afpPath += CStdString(" -u ") +  CStdString(afp_url.username) + CStdString(" ");

    if(strlen(afp_url.uamname) != 0)
      afpPath += CStdString("-a ") +  CStdString("\"") + CStdString(afp_url.uamname) + CStdString("\"") + CStdString(" ");

    if(strlen(afp_url.password) != 0)
      afpPath += "-p " + CStdString(afp_url.password) + CStdString(" ");

    afpPath += CStdString(afp_url.servername) + CStdString(":") + url_encode(afp_url.volumename);

    CLog::Log(LOGDEBUG, "CAfpDirectory::%s - mounting AFP share [%s:%s] ==> [%s]", __func__, afp_url.servername, afp_url.volumename, strMountpoint.c_str());

    if(MountShare(afpPath, strMountpoint) == false)
    {
      UnMountShare(strMountpoint);
      CLog::Log(LOGERROR, "CAfpDirectory::%s - failed to mount AFP share [%s:%s]", __func__, afp_url.servername, afp_url.volumename);
      return false;
    }
  }

  CStdString strHddPath = strMountpoint + strFileName;

  item.m_strPath = strHddPath;
  item.SetLabel(path.Get());

  item.SetProperty("filename", strFileName);

  struct stat st;
  if (stat(strHddPath.c_str(),&st) == 0)
  {
    item.m_bIsFolder = S_ISDIR(st.st_mode);
    item.m_dateTime = st.st_mtime;
  }

  if(item.m_bIsFolder)
  {
    item.SetProperty("isNetwork",true);
  }

  return true;
}

bool CAfpDirectory::GetVolumes(const CURI& url, CFileItemList &items)
{
  CStdString strVolumesCommand, strOutput;
  int status;
  struct afp_url afp_url;

  if (!BuildAfpUrl(url, &afp_url))
  {
    return false;
  }

  strVolumesCommand = "afp_client volumes";

  if (strlen(afp_url.username) != 0)
    strVolumesCommand += " -u " + CStdString(afp_url.username);

  if (strlen(afp_url.password) != 0)
    strVolumesCommand += " -p " + CStdString(afp_url.password);

  if (strlen(afp_url.uamname) != 0)
    strVolumesCommand += " -a " + CStdString("\"") + CStdString(afp_url.uamname) + CStdString("\"");

  strVolumesCommand += " " + CStdString(afp_url.servername);

  RunCommand(strVolumesCommand, status, strOutput);

  if(strOutput.Find("Login error:") != -1 || strOutput.Find("Could not pick a matching UAM") != -1 || strOutput.Find("Connection timed out") != -1 || strOutput.IsEmpty())
  {
    EjectUser(url.Get());
    return false;
  }

  if (strOutput.Find(';') == -1)
  {
    EjectUser(url.Get());
    CLog::Log(LOGERROR, "CAfpDirectory::%s - failed to read volumes, server [%s], error [%s]", __func__, afp_url.servername, strOutput.c_str());
    return false;
  }

  CStdStringArray arr;
  StringUtils::SplitString(strOutput, ";", arr);

  for(size_t i=0; i<arr.size(); i++)
  {
    if(arr[i].IsEmpty()) continue;

    CFileItemPtr pItem(new CFileItem(arr[i]));

    pItem->m_bIsFolder = true;
    pItem->m_strPath.Format("%s%s", url.Get(), arr[i]);
    CUtil::AddSlashAtEnd(pItem->m_strPath);
    pItem->SetProperty("isNetwork",true);

    items.Add(pItem);
  }

  return true;
}

CAfpDirectory::CAfpDirectory(void)
{}

CAfpDirectory::~CAfpDirectory(void)
{}

void CAfpDirectory::EjectUser(const CStdString& strPath)
{
  CURI url(strPath);
  CStdString hostName = url.GetHostName();

  CStdString cmd = "afp_client logout " + hostName;

  system(cmd.c_str());

  hostName = url.GetHostName();

  IMAPUSERNAMEPASSWORDS it = g_passwordManager.m_mapAFPPasswordCache.find(hostName);
  if(it != g_passwordManager.m_mapAFPPasswordCache.end())
  {
    g_passwordManager.m_mapAFPPasswordCache.erase(it);
  }
}

CStdString CAfpDirectory::HandlePath(const CStdString& strPath , bool dontUseGuestCredentials)
{
  if(strPath.Equals("afp://") || strPath.Equals("afp://all"))
  {
    return strPath;
  }

  CStdString resultPath;
  CURI urlIn(strPath);

  IMAPUSERNAMEPASSWORDS it = g_passwordManager.m_mapAFPPasswordCache.find(urlIn.GetHostName());
  if(!dontUseGuestCredentials && it == g_passwordManager.m_mapAFPPasswordCache.end())
  {
    CStdString compName = urlIn.GetHostName();
    CStdString userName = urlIn.GetUserName();
    CStdString userPassword = urlIn.GetPassWord();

    if(userName.IsEmpty())
    {
      userName = "guest";
      userPassword  = "";
      urlIn.SetPassword("");
    }

    //insert to map
    g_passwordManager.m_mapAFPPasswordCache[compName] = make_pair(userName, userPassword);

    resultPath = urlIn.Get();

    return resultPath;
  }

  //test if share has password in map
  it = g_passwordManager.m_mapAFPPasswordCache.find(urlIn.GetHostName());
  if(it != g_passwordManager.m_mapAFPPasswordCache.end())
  {
    CStdString strPassword = g_passwordManager.m_mapAFPPasswordCache[urlIn.GetHostName()].second;
    CStdString strUserName = g_passwordManager.m_mapAFPPasswordCache[urlIn.GetHostName()].first;

    //if enter as guest previously
    if(strUserName.Equals("guest"))
    {
      resultPath = strPath;
    }
    else
    {
      urlIn.SetPassword(strPassword);
      urlIn.SetUserName(strUserName);
      resultPath = urlIn.Get();
    }
  }
  //pop enter userName and password dialog
  else
  {
    EjectUser(strPath);
    g_passwordManager.SetAFPShare(strPath);
    if(g_passwordManager.GetAFPShareUserPassword())
    {
      CURI urlnew( g_passwordManager.GetAFPShare());
      CStdString compName = urlnew.GetHostName();
      CStdString userName = urlnew.GetUserName();
      CStdString userPassword = urlnew.GetPassWord();

      if(userName.Equals("guest") || userName.Equals("GUEST") || userName.Equals("Guest") )
      {
        userName = "guest";
        userPassword = "";
        resultPath = strPath;
      }
      else
      {
        resultPath = urlnew.Get();
      }
      //insert to map
      g_passwordManager.m_mapAFPPasswordCache[compName] = make_pair(userName, userPassword);
    }
  }

  return resultPath;
}

bool CAfpDirectory::GetDirectory(const CStdString& _strPath, CFileItemList &items)
{
  CStdString strPath;
  CURI tempUrl(_strPath);

  if(!items.GetPropertyBOOL("notOpenPassword") && tempUrl.GetUserName().IsEmpty())
  {

    if(items.GetPropertyBOOL("dontUseGuestCredentials"))
    {
      strPath = HandlePath(_strPath, true);
    }
    else
    {
      strPath = HandlePath(_strPath);
    }
    if(strPath.IsEmpty())
    {
      items.SetProperty("NotErrorDontShowErrorDialog",true);
      return false;
    }
  }
  else
  {
    strPath = _strPath;
  }

  CURI url(strPath);
  bool bResult = false;

  if(url.GetProtocol() != "afp")
  {
    CLog::Log(LOGERROR, "CAfpDirectory::%s - invalid protocol [%s]", __func__, url.GetProtocol().c_str());
    return false;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

  if( strPath == "afp://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::AFP_SHARE, items );
    return true;
  }

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strPath, items))
  {
    return true;
  }

  if(strPath == "afp://all")
  {
    return ScanAfpShares(items);
  }

  CStdString fileName = url.GetFileName();

  if(fileName.IsEmpty())
  {
    bResult = GetVolumes(url, items);
  }
  else
  {
    CUtil::RemoveSlashAtEnd(fileName);
    url.SetFileName(fileName);
    CFileItem item;

    if(false == GetResource(url, item))
    {
      CLog::Log(LOGERROR, "CAfpDirectory::%s - failed to get resource for path [%s]", __func__, url.Get().c_str());
      return false;
    }

    if(false == HasReadPermissions(item.m_strPath, url.GetUserName().IsEmpty()))
    {
      CLog::Log(LOGERROR, "CAfpDirectory::%s - no read permissions for path [%s]", __func__, item.m_strPath.c_str());
      return false;
    }

    bResult = CDirectory::GetDirectory(item.m_strPath, items);

    for(int i=0; i<items.Size(); i++)
    {
      if(items[i]->m_bIsFolder)
      {
        items[i]->SetProperty("isNetwork",true);
      }
      // build afp path
      CStdString afpPath = items[i]->m_strPath;
      afpPath.Replace(item.m_strPath, strPath);

      if (afpPath.length() >= strPath.length())
      {
        CStdString strNewFilename = afpPath.substr(strPath.length());

        CStdString strAfpFileName;
        CStdString strFileName = item.GetProperty("filename");

        strAfpFileName = strNewFilename;

        if(strAfpFileName.Left(1).Equals("/") && CUtil::HasSlashAtEnd(strPath))
        {
          CUtil::RemoveSlashAtEnd(strPath);
        }

        items[i]->m_strPath.Format("%s%s", strPath, strAfpFileName);
      }
    }
  }

  return bResult;
}

bool CAfpDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
#endif
