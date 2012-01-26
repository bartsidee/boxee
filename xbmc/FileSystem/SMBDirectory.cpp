
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

/*
* know bugs:
* - when opening a server for the first time with ip adres and the second time
*   with server name, access to the server is denied.
* - when browsing entire network, user can't go back one step
*   share = smb://, user selects a workgroup, user selects a server.
*   doing ".." will go back to smb:// (entire network) and not to workgroup list.
*
* debugging is set to a max of 10 for release builds (see local.h)
*/

#include "system.h"

#if defined(HAS_FILESYSTEM_SMB)
#include "SMBDirectory.h"
#include "Util.h"
#include "LocalizeStrings.h"
#include "GUIPassword.h"
#include "GUIWindowManager.h"
#include "GUIDialogOK.h"
#include "Application.h"
#include "FileItem.h"
#include "AdvancedSettings.h"
#include "StringUtils.h"
#include "utils/log.h"
#include "BrowserService.h"
#include "utils/SingleLock.h"
#include "SMBUtils.h"
#include "BoxeeUtils.h"
#include <set>

#include "DirectoryCache.h"

#ifndef _WIN32PC
#include <libsmbclient.h>
#endif

#ifdef __APPLE__
#define XBMC_SMB_MOUNT_PATH "Library/Application Support/BOXEE/Mounts/"
#else
#define XBMC_SMB_MOUNT_PATH "/media/xbmc/smb/"
#endif

struct CachedDirEntry
{
  unsigned int type;
  CStdString name;
};

using namespace DIRECTORY;
using namespace std;

CSMBDirectory::CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddActiveConnection();
#endif
} 

CSMBDirectory::~CSMBDirectory(void)
{
#ifdef _LINUX
  smb.AddIdleConnection();
#endif
}


bool CSMBDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  if(SMBUtils::ProcessPath(strPath, items))
  {
    return true;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

#ifndef HAS_EMBEDDED
  // We accept smb://[[[domain;]user[:password@]]server[/share[/path[/file]]]]
  if( strPath == "smb://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::SMB_SHARE, items );
    return true;
  }
#endif
  
	// If we have the items in the cache, return them
	if (g_directoryCache.GetDirectory(strPath, items)) {
		return true;
	}
  /* samba isn't thread safe with old interface, always lock */
  CSingleLock lock(smb);

  smb.Init();

  /* we need an url to do proper escaping */
  CURI url(strPath);

  //Separate roots for the authentication and the containing items to allow browsing to work correctly
  CStdString strRoot = strPath;
  CStdString strAuth;

  lock.Leave(); // OpenDir is locked
  int fd = OpenDir(url, strAuth);

  if (fd == -2)
  {
    // OpenDir failed because credentials were required but could not be obtained
    items.SetProperty("getcredentials", true);
  }

  if (fd < 0)
    return false;
  
  CUtil::AddSlashAtEnd(strRoot);
  CUtil::AddSlashAtEnd(strAuth);

  CStdString strFile;

  // need to keep the samba lock for as short as possible.
  // so we first cache all directory entries and then go over them again asking for stat
  // "stat" is locked each time. that way the lock is freed between stat requests
  vector<CachedDirEntry> vecEntries;
  struct smbc_dirent* dirEnt;
  
  lock.Enter();
  while ((dirEnt = smbc_readdir(fd)))
  {
    CachedDirEntry aDir;
    aDir.type = dirEnt->smbc_type;
    aDir.name = dirEnt->name;
    vecEntries.push_back(aDir);
  }
  smbc_closedir(fd);
  lock.Leave();

  for (size_t i=0; !g_application.m_bStop && i<vecEntries.size(); i++)
  {
    CachedDirEntry aDir = vecEntries[i];

    // We use UTF-8 internally, as does SMB
    strFile = aDir.name;

    if (!strFile.Equals(".") && !strFile.Equals("..")
      && aDir.type != SMBC_PRINTER_SHARE && aDir.type != SMBC_IPC_SHARE)
    {
      int64_t iSize = 0;
      bool bIsDir = true;
      int64_t lTimeDate = 0;
      bool hidden = false;

      if(strFile.Right(1).Equals("$") && aDir.type == SMBC_FILE_SHARE )
        continue;

      struct timespec tm;

      // only stat files that can give proper responses
      if ( aDir.type == SMBC_FILE ||
           aDir.type == SMBC_DIR )
      {
        // set this here to if the stat should fail
        bIsDir = (aDir.type == SMBC_DIR);
       
#if !defined(_LINUX)
        struct __stat64 info = {0};
#elif defined(__APPLE__)
        struct stat64 info;
#else
        struct stat info;
#endif
        if (m_extFileInfo && g_advancedSettings.m_sambastatfiles)
        {
          // make sure we use the authenticated path wich contains any default username
          CStdString strFullName = strAuth + smb.URLEncode(strFile);
 
          lock.Enter();

          if( !g_application.m_bStop && smbc_stat(strFullName.c_str(), (struct stat *)&info) == 0 )
          {
            tm.tv_sec = info.st_mtime;

#ifndef _LINUX
            if ((info.st_mode & S_IXOTH))
            hidden = true;
#else             
            char value[20];
            // We poll for extended attributes which symbolizes bits but split up into a string. Where 0x02 is hidden and 0x12 is hidden directory.
            // According to the libsmbclient.h it's supposed to return 0 if ok, or the length of the string. It seems always to return the length wich is 4
            if ( !g_application.m_bStop && smbc_getxattr(strFullName, "system.dos_attr.mode", value, sizeof(value)) > 0)
            {
              long longvalue = strtol(value, NULL, 16);
              if (longvalue & SMBC_DOS_MODE_HIDDEN)
              hidden = true;
            }
            else
              CLog::Log(LOGERROR, "Getting extended attributes for the share: '%s'\nunix_err:'%x' error: '%s'", strFullName.c_str(), errno, strerror(errno));
#endif

            bIsDir = (info.st_mode & S_IFDIR) ? true : false;
            lTimeDate = info.st_mtime;
            if(lTimeDate == 0) // if modification date is missing, use create date
              lTimeDate = info.st_ctime;
            iSize = info.st_size;
          }
          else
            CLog::Log(LOGERROR, "%s - Failed to stat file %s", __FUNCTION__, strFullName.c_str());

          lock.Leave();
        }
      }

      if (bIsDir)
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        pItem->m_strPath = strRoot;

        // needed for network / workgroup browsing
        // skip if root if we are given a server
        if (aDir.type == SMBC_SERVER)
        {
          /* create url with same options, user, pass.. but no filename or host*/
          CURI rooturl(strRoot);
          rooturl.SetFileName("");
          rooturl.SetHostName("");
          pItem->m_strPath = smb.URLEncode(rooturl);
        }
        pItem->m_strPath += aDir.name;
        CUtil::AddSlashAtEnd(pItem->m_strPath);
        pItem->m_bIsFolder = true;
        pItem->m_dateTime = tm.tv_sec;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
      else
      {
        CFileItemPtr pItem(new CFileItem(strFile));
        pItem->m_strPath = strRoot + aDir.name;
        pItem->m_bIsFolder = false;
        pItem->m_dwSize = iSize;
        pItem->m_dateTime = tm.tv_sec;
        if (hidden)
          pItem->SetProperty("file:hidden", true);
        items.Add(pItem);
      }
    }
  }

  return true;
}

bool CSMBDirectory::ProcessPath(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);

  if(url.GetProtocol() != "smb")
  {
    CLog::Log(LOGERROR, "CSMBDirectory::ProcessPath - invalid protocol [%s]", url.GetProtocol().c_str());
    return false;
  }

  CStdString strComp;
  CStdString strDevices;
  std::map<std::string, std::string> mapParams;

  // Parse boxeedb url
  if (!BoxeeUtils::ParseBoxeeDbUrl(strPath, strComp, strDevices, mapParams))
  {
    return false;
  }

  if(strComp.Equals("computers") && mapParams["name"].empty())
  {
    GetComputers(items);
    return true;
  }

  if(strComp.Equals("computers") && !mapParams["name"].empty())
  {
    CStdString selectedComp = mapParams["name"];
    if(selectedComp.IsEmpty())
    {
      return false;
    }
    GetComputerDevices(strPath, selectedComp, items);
    return true;
  }

  return false;
}

bool CSMBDirectory::GetComputers(CFileItemList &items)
{
  const CStdString strPath = "smb://";

  CDirectory::GetDirectory(strPath, items);

  bool bInsert;
  std::set<CStdString> Computers;

  int i = 0;
  while(i < items.Size())
  {
    CStdString compName = GetComputerName(items[i]->GetLabel());
    bInsert = Computers.insert(compName).second;

    if(bInsert)
    {
      items[i]->SetLabel(compName);
      items[i]->m_strPath =  "smb://computers/?name=" + compName;
      i++;
    }
    else
    {
      items.Remove(i);
    }
  }

  return true;
}

bool CSMBDirectory::GetComputerDevices(const CStdString& strSmbPath,const CStdString& selectedComp, CFileItemList &items)
{
  const CStdString strPathTmp = "smb://";
  CDirectory::GetDirectory(strPathTmp, items);

  size_t index;
  int length, i = 0;
  int indexToBeDeleted = -1;
  CStdString label;

  while(i < items.Size())
  {
    CStdString compName = GetComputerName(items[i]->GetLabel());

    if(!selectedComp.Equals(compName))
    {
      items.Remove(i);
    }
    else
    {
      //remove the computer from list displayed
      if(selectedComp.Equals(items[i]->GetLabel()))
      {
        indexToBeDeleted = i;
      }

      //remove computer name
      label = items[i]->GetLabel();
      index = label.find_first_of("//");
      if (index != std::string::npos)
      {
        length = label.length() - (index + 1);
        label = label.substr(index + 2,length);
        items[i]->SetLabel(label);
      }

      i++;
    }
  }

  if(items.Size() > 1 && indexToBeDeleted != -1)
  {
    items.Remove(indexToBeDeleted);
  }

  return true;
}

CStdString CSMBDirectory::GetComputerName(const CStdString& computer)
{
  CStdString compName;
  int index = computer.Find('/');

  if(index > 0)
  {
    compName = computer.substr(0,index - 1);
  }
  else
  {
    compName = computer;
  }
  return compName;
}

int CSMBDirectory::Open(const CURI &url)
{
  smb.Init();
  CStdString strAuth;  
  return OpenDir(url, strAuth);
}

/// \brief Checks authentication against SAMBA share and prompts for username and password if needed
/// \param strAuth The SMB style path
/// \return SMB file descriptor
int CSMBDirectory::OpenDir(const CURI& url, CStdString& strAuth)
{
  int fd = -1;
#ifndef _LINUX
  int nt_error;
#endif
  bool userCreds = false;
  bool cachedCreds = false;
  
  /* make a writeable copy */
  CURI urlIn(url);

  /* set original url */
  strAuth = smb.URLEncode(urlIn);
  CURI origUrl(url);

  CStdString strPath;
  CStdString strShare;
  /* must url encode this as, auth code will look for the encoded value */
  strShare  = smb.URLEncode(urlIn.GetHostName());
  strShare += "/";
  strShare += smb.URLEncode(urlIn.GetShareName());

  CStdString strOrigPassword = origUrl.GetPassWord();
  CStdString strOrigUserName = origUrl.GetUserName();

  if( strOrigUserName.length() != 0 &&
      (!strOrigUserName.Equals("guest") || strOrigPassword.length() != 0) )
  {
    userCreds = true;
  }
  strOrigPassword = "";
  strOrigUserName = "";

  IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.find(strShare);
  if(it != g_passwordManager.m_mapSMBPasswordCache.end())
  {
    // if share found in cache use it to supply username and password
    CURI url(it->second);		// map value contains the full url of the originally authenticated share. map key is just the share
    CStdString strPassword = url.GetPassWord();
    CStdString strUserName = url.GetUserName();

    CStdString strOrigPassword = origUrl.GetPassWord();
    CStdString strOrigUserName = origUrl.GetUserName();
    if( userCreds &&
        ( strOrigUserName != strUserName ||
          strOrigPassword != strPassword ) )
    {
      CLog::Log(LOGDEBUG, "%s - credential change for url %s\n", __FUNCTION__, strPath.c_str());
    }
    else
    {
      urlIn.SetPassword(strPassword);
      urlIn.SetUserName(strUserName);
      cachedCreds = true;
    }
  }
  
  // for a finite number of attempts use the following instead of the while loop:
  // for(int i = 0; i < 3, fd < 0; i++)
  bool prompet = false;
  while (fd < 0 && !prompet)
  {
#ifdef HAS_EMBEDDED
    strPath = urlIn.Get();
#else
    /* samba has a stricter url encoding, than our own.. CURI can decode it properly */
    /* however doesn't always encode it correctly (spaces for example) */
    strPath = smb.URLEncode(urlIn);
#endif
    // remove the / or \ at the end. the samba library does not strip them off
    // don't do this for smb:// !!
    CStdString s = strPath;
    int len = s.length();
    if (len > 1 && s.at(len - 2) != '/' &&
        (s.at(len - 1) == '/' || s.at(len - 1) == '\\'))
    {
      s.erase(len - 1, 1);
    }

    CLog::Log(LOGDEBUG, "%s - Using authentication url %s", __FUNCTION__, s.c_str());
    { 
      CSingleLock lock(smb);
      fd = smbc_opendir(s.c_str());
    }

    if (fd < 0)
    {
#ifndef _LINUX
      nt_error = smb.ConvertUnixToNT(errno);

      // if we have an 'invalid handle' error we don't display the error
      // because most of the time this means there is no cdrom in the server's
      // cdrom drive.
      if (nt_error == NT_STATUS_INVALID_HANDLE)
        break;
#endif

      // NOTE: be sure to warn in XML file about Windows account lock outs when too many attempts
      // if the error is access denied, prompt for a valid user name and password
#ifndef _LINUX
      if (nt_error == NT_STATUS_ACCESS_DENIED)
#else
      if (errno == EACCES || errno == EPERM)
#endif
      {
        // take a guess at a username/password that may be viable here, by combing through
        // the shares and seeing if we can find something relevant
        if( !userCreds && !cachedCreds )
        {
          CStdString hostname = smb.URLEncode(urlIn.GetHostName());

          CStdString strPassword;
          CStdString strUserName;
          bool bHaveGuess = false;
          IMAPPASSWORDS bestGuess;
          IMAPPASSWORDS it = g_passwordManager.m_mapSMBPasswordCache.begin();

          while( it != g_passwordManager.m_mapSMBPasswordCache.end() )
          {
            CURI url(it->second);
            strPassword = url.GetPassWord();
            strUserName = url.GetUserName();

            bool hasCreds = strUserName.length() > 0;

            // if this url has credentials for the host, and they aren't 'guest', then use those.
            if( it->first.Find(hostname) >= 0 && hasCreds )
            {
              cachedCreds = true;
              break;
            }
            else if( hasCreds && !bHaveGuess)
            {
              // it has creds for a different host; may work in a local network
              bHaveGuess = true;
              bestGuess = it;
            }
            it++;
          }

          // if we're here, then we looped with only a guess as to what to set
          // reload that username/pass
          if( !cachedCreds && bHaveGuess  )
          {
            CURI url(bestGuess->second);
            strPassword = url.GetPassWord();
            strUserName = url.GetUserName();

            cachedCreds = true;
          }

          // ok, we found cached credentials we should use
          // set those and continue, forcing one more pass on the loop.
          if( cachedCreds )
          {
            CLog::Log(LOGDEBUG, "%s - trying related credentials for %s on %s\n", __FUNCTION__, hostname.c_str(), strPath.c_str());

            urlIn.SetPassword(strPassword);
            urlIn.SetUserName(strUserName);
            continue;
          }

          // otherwise we couldn't find useful creds; just fall through
        }

        if (m_allowPrompting)
        {
          g_passwordManager.SetSMBShare(strPath);
          if (!g_passwordManager.GetSMBShareUserPassword())  // Do this bit via a threadmessage?
          	break;

          /* must do this as our urlencoding for spaces is invalid for samba */
          /* and doing double url encoding will fail */
          /* curl doesn't decode / encode filename yet */
          CURI urlnew( g_passwordManager.GetSMBShare() );
          CStdString u = urlnew.GetUserName(), p = urlnew.GetPassWord();
#ifndef HAS_CIFS
          CUtil::UrlDecode(u);
          CUtil::UrlDecode(p);
          urlIn.SetUserName(u);
          urlIn.SetPassword(p);
          prompet = false;
#else
          urlIn.SetUserName(u);
          urlIn.SetPassword(p);
          prompet = true;
#endif
        }
        else
        {
          // We are not allowed to prompt user for credentials here, pass indication to the caller that
          // call failed because of credentials were required but could not be received
          fd = -2;
          break;
        }
      }
      else
      {
        CStdString cError;
#ifndef _LINUX
        if (nt_error == NT_STATUS_OBJECT_NAME_NOT_FOUND)
          cError = g_localizeStrings.Get(770);
        else
          cError = get_friendly_nt_error_msg(nt_error);
#else
        if (errno == ENODEV || errno == ENOENT)
          cError = g_localizeStrings.Get(770);
        else
          cError = strerror(errno);
#endif
        if (m_allowPrompting)
        {
          CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
          pDialog->SetHeading(257);
          pDialog->SetLine(0, cError);
          pDialog->SetLine(1, "");
          pDialog->SetLine(2, "");

          ThreadMessage tMsg(TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow());
          g_application.getApplicationMessenger().SendMessage(tMsg, false);
        }
        break;
      }
    }
  }

  if (fd < 0)
  {
    // write error to logfile
#ifndef _LINUX
    CLog::Log(LOGDEBUG, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' nt_err : '%x' error : '%s'", strPath.c_str(), errno, nt_error, get_friendly_nt_error_msg(nt_error));
#else
    CLog::Log(LOGDEBUG, "SMBDirectory->GetDirectory: Unable to open directory : '%s'\nunix_err:'%x' error : '%s'", strPath.c_str(), errno, strerror(errno));
#endif
  }
  else
  {
    if (!strShare.IsEmpty()) // we succeeded so cache it
    {
      g_passwordManager.m_mapSMBPasswordCache[strShare] = strPath;
      strAuth = strPath;
    }
  }

  return fd;
}

bool CSMBDirectory::Create(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURI url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_mkdir(strFileName.c_str(), 0);

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0 || EEXIST == result);
}

bool CSMBDirectory::Remove(const char* strPath)
{
  CSingleLock lock(smb);
  smb.Init();

  CURI url(strPath);
  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);

  int result = smbc_rmdir(strFileName.c_str());

  if(result != 0)
#ifndef _LINUX
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, get_friendly_nt_error_msg(smb.ConvertUnixToNT(errno)));
#else
    CLog::Log(LOGERROR, "%s - Error( %s )", __FUNCTION__, strerror(errno));
#endif

  return (result == 0);
}

bool CSMBDirectory::Exists(const char* strPath)
{
  bool ret;
  int statval;
  CURI url(strPath);
  
  CBrowserService* pBrowser = g_application.GetBrowserService();
  if (pBrowser)
  {
    if (!pBrowser->IsHostAvailable(url.GetHostName()))
    {
      CLog::Log(LOGDEBUG,"Host <%s> not available according to browser-service",url.GetHostName().c_str());
      return false;
    }
  }
  
  //// locked
  {
  CSingleLock lock(smb);
  smb.Init();

  CStdString strFileName = smb.URLEncode(url);
  strFileName = g_passwordManager.GetSMBAuthFilename(strFileName);
  
#if !defined(_LINUX)
    struct __stat64 info = {0};
#elif defined(__APPLE__)
    struct stat64 info;
#else
    struct stat info;
#endif
    statval = smbc_stat( strFileName.c_str(), (struct stat *)&info );
    if( 0 == statval )
    {
      ret = (info.st_mode & S_IFDIR) ? true : false;
    }
    else
    {
      ret = false;
    }
  }
  //// drop out of lock

  if( 0 != statval )
  {
    // Note, we spam warnings for subtitle paths here. Not much we can do without skipping potentially
    // important logs
    CStdString printUrl = url.GetWithoutUserDetails();
    CLog::Log(LOGWARNING, "Path <%s> not available according to smbc_stat (%d, %d)\n", printUrl.c_str(), statval, errno);
  }

  return ret;
}

CStdString CSMBDirectory::MountShare(const CStdString &smbPath, const CStdString &strType, const CStdString &strName, 
    const CStdString &strUser, const CStdString &strPass)
{
#ifdef _LINUX
  UnMountShare(strType, strName);

  CStdString strMountPoint = GetMountPoint(strType, strName);
  
#ifdef __APPLE__
  // Create the directory.
  CUtil::UrlDecode(strMountPoint);
  CreateDirectory(strMountPoint, NULL);
  
  // Massage the path.
  CStdString smbFullPath = "//";
  if (smbFullPath.length() > 0)
  {
    smbFullPath += strUser;
    if (strPass.length() > 0)
      smbFullPath += ":" + strPass;
    
    smbFullPath += "@";
  }
  
  CStdString newPath = smbPath;
  newPath.TrimLeft("/");
  smbFullPath += newPath;
  
  // Make the mount command.
  CStdStringArray args;
  args.push_back("/sbin/mount_smbfs");
  args.push_back("-o");
  args.push_back("nobrowse");
  args.push_back(smbFullPath);
  args.push_back(strMountPoint);
  
  // Execute it.
  if (CUtil::Command(args))
    return strMountPoint;
#else
  CUtil::SudoCommand("mkdir -p " + strMountPoint);

  CStdString strCmd = "mount -t cifs " + smbPath + " " + strMountPoint + 
    " -o rw,nobrl,directio";
  if (!strUser.IsEmpty())
    strCmd += ",user=" + strUser + ",password=" + strPass; 
  else 
    strCmd += ",guest";

  if (CUtil::SudoCommand(strCmd))
    return strMountPoint;
#endif
#endif
  return StringUtils::EmptyString;
}

void CSMBDirectory::UnMountShare(const CStdString &strType, const CStdString &strName)
{
#ifdef __APPLE__
  // Decode the path.
  CStdString strMountPoint = GetMountPoint(strType, strName);
  CUtil::UrlDecode(strMountPoint);
  
  // Make the unmount command.
  CStdStringArray args;
  args.push_back("/sbin/umount");
  args.push_back(strMountPoint);
  
  // Execute command.
  CUtil::Command(args);
#elif defined(_LINUX)
  CStdString strCmd = "umount " + GetMountPoint(strType, strName);
  CUtil::SudoCommand(strCmd);
#endif
}

CStdString CSMBDirectory::GetMountPoint(const CStdString &strType, const CStdString &strName)
{
  CStdString strPath = strType + strName;
  CUtil::URLEncode(strPath);
  
#ifdef __APPLE__
  CStdString str = getenv("HOME");
  return str + "/" + XBMC_SMB_MOUNT_PATH + strPath;
#else
  return XBMC_SMB_MOUNT_PATH + strPath;
#endif
}

#endif
