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
#include "system.h"
#ifdef __APPLE__
#include <sys/param.h>
#include <mach-o/dyld.h>
#include <Carbon/Carbon.h>
#include "osx/XBMCHelper.h"
#endif

#ifdef _LINUX
#include <sys/types.h>
#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/param.h>
#ifndef __APPLE__
#include <sys/statfs.h>
#else
#include <sys/param.h>
#include <sys/mount.h>
#endif
#endif
#include "Application.h"
#include "AutoPtrHandle.h"
#include "Util.h"
#include "xbox/IoSupport.h"
#include "FileSystem/StackDirectory.h"
#include "FileSystem/VirtualPathDirectory.h"
#include "FileSystem/MultiPathDirectory.h"
#include "FileSystem/DirectoryCache.h"
#include "FileSystem/SpecialProtocol.h"
#include "ThumbnailCache.h"
#include "FileSystem/RarManager.h"
#include "FileSystem/CMythDirectory.h"
#ifdef HAS_UPNP
#include "FileSystem/UPnPDirectory.h"
#endif
#ifdef HAS_CREDITS
#include "Credits.h"
#endif
#ifdef HAS_VIDEO_PLAYBACK
#include "cores/VideoRenderers/RenderManager.h"
#endif
#include "utils/RegExp.h"
#include "GUISettings.h"
#include "TextureManager.h"
#include "GUIDialogProgress.h"
#include "utils/fstrcmp.h"
#include "MediaManager.h"
#ifdef HAS_FTP_SERVER
#include "lib/libfilezilla/xbfilezilla.h"
#endif
#include "DirectXGraphics.h"
#include "DNSNameCache.h"
#include "GUIWindowManager.h"
#ifdef _WIN32
#include <shlobj.h>
#include "WIN32Util.h"
#include "win32/dirent.h"
#endif
#include "GUIDialogYesNo.h"
#include "GUIUserMessages.h"
#include "FileSystem/File.h"
#include "Crc32.h"
#include "AppManager.h"
#include "AppRegistry.h"
#include "lib/libBoxee/boxee.h"
#include "GUIDialogBoxeeMediaAction.h"
#include "utils/Thread.h"
#include <vector>
#include "GUIWindowApp.h"
#include "GUIToggleButtonControl.h"
#include "GUIDialogBoxeeVideoCtx.h"
#include "GUIDialogBoxeeMusicCtx.h"
#include "GUIDialogBoxeePictureCtx.h"
#include "GUIInfoManager.h"
#include "SystemInfo.h"
#include "utils/RssReader.h"
#include "Settings.h"
#include "StringUtils.h"
#include "AdvancedSettings.h"
#ifdef HAS_LIRC
#include "common/LIRC.h"
#endif
#ifdef HAS_IRSERVERSUITE
  #include "common/IRServerSuite/IRServerSuite.h"
#endif
#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
#include <HalServices.h>
#include <ucontext.h>
#include <dlfcn.h>
#include <cxxabi.h>
#include <execinfo.h>
using __cxxabiv1::__cxa_demangle;
#endif
#include "WindowingFactory.h"
#include "LocalizeStrings.h"
#include "utils/TimeUtils.h"
#include "bxoemconfiguration.h"
#include "utils/md5.h"
#include "BoxeeUtils.h"

#include "Picture.h"

#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include "utils/Base64.h"

#ifdef _WIN32
#define MAXPATHLEN MAX_PATH
#define lstat stat
#endif

using namespace std;
using namespace DIRECTORY;

#define clamp(x) (x) > 255.f ? 255 : ((x) < 0 ? 0 : (BYTE)(x+0.5f)) // Valid ranges: brightness[-1 -> 1 (0 is default)] contrast[0 -> 2 (1 is default)]  gamma[0.5 -> 3.5 (1 is default)] default[ramp is linear]
static const __int64 SECS_BETWEEN_EPOCHS = 11644473600LL;
static const __int64 SECS_TO_100NS = 10000000;

using namespace AUTOPTR;
using namespace XFILE;
using namespace PLAYLIST;

#ifdef HAS_DX
static D3DGAMMARAMP oldramp, flashramp;
#endif

XBOXDETECTION v_xboxclients;

CUtil::CUtil(void)
{
}

CUtil::~CUtil(void)
{}

/* returns filename extension including period of filename */
const CStdString CUtil::GetExtension(const CStdString& strFileName)
{
  if(strFileName.Find("://") >= 0)
  {
    CURI url(strFileName);
    return CUtil::GetExtension(url.GetFileName());
  }

  int period = strFileName.find_last_of('.');
  if(period >= 0)
  {
    if( strFileName.find_first_of('/', period+1) != string::npos ) return "";
    if( strFileName.find_first_of('\\', period+1) != string::npos ) return "";
    /* url options could be at the end of a url */
    const int options = strFileName.find_first_of('?');

    if(options >= 0 && options > period)
      return strFileName.substr(period, options-period);
    else
      return strFileName.substr(period);
  }
  else
    return "";
}

void CUtil::GetExtension(const CStdString& strFile, CStdString& strExtension)
{
  strExtension = GetExtension(strFile);
}

/* returns a filename given an url */
/* handles both / and \, and options in urls*/
const CStdString CUtil::GetFileName(const CStdString& strFileNameAndPath)
{
  if(strFileNameAndPath.Find("://") >= 0)
  {
    CURI url(strFileNameAndPath);
    return CUtil::GetFileName(url.GetFileName());
  }
  
  /* find any slashes */
  const int slash1 = strFileNameAndPath.find_last_of('/');
  const int slash2 = strFileNameAndPath.find_last_of('\\');

  /* select the last one */
  int slash;
  if(slash2>slash1)
    slash = slash2;
  else
    slash = slash1;

  /* check if there is any options in the url */
  const int options = strFileNameAndPath.find_first_of('?', slash+1);
  if(options < 0)
    return _P(strFileNameAndPath.substr(slash+1));
  else
    return _P(strFileNameAndPath.substr(slash+1, options-(slash+1)));
}
CStdString CUtil::GetTitleFromBoxeePath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  CURI url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();

  CStdString path(strFileNameAndPath);
  RemoveSlashAtEnd(path);
  CStdString strFilename = GetFileName(path);

  if (url.GetProtocol() == "boxeedb")
  {
    return strHostname.ToUpper();
  }
  else if (url.GetProtocol() == "sources")
  {
    return strHostname.ToUpper();
  }
  else if (url.GetProtocol() == "history")
  {
    return strHostname.ToUpper();
  }

  return "";

}
CStdString CUtil::GetTitleFromPath(const CStdString& strFileNameAndPath, bool bIsFolder /* = false */)
{
  CStdString title = GetTitleFromBoxeePath(strFileNameAndPath, bIsFolder);
  if (!title.IsEmpty())
    return title;

  // use above to get the filename
  CStdString path(strFileNameAndPath);
  
  if (CUtil::IsHD(path))
     CUtil::HideExternalHDPath(path, path);

  RemoveSlashAtEnd(path);
  CStdString strFilename = GetFileName(path);

  CURI url(strFileNameAndPath);
  CStdString strHostname = url.GetHostName();
#ifdef HAS_UPNP
  // UPNP
  if (url.GetProtocol() == "upnp")
    strFilename = CUPnPDirectory::GetFriendlyName(strFileNameAndPath.c_str());
#endif

  // LastFM
  if (url.GetProtocol() == "lastfm")
  {
    if (strFilename.IsEmpty())
      strFilename = g_localizeStrings.Get(15200);
    else
      strFilename = g_localizeStrings.Get(15200) + " - " + strFilename;
  }
  else if (strFilename.IsEmpty() && (url.GetProtocol() == "afp" || url.GetProtocol() == "nfs"))
  {
    strFilename = strHostname;
  }
  // Shoutcast
  else if (url.GetProtocol() == "shout")
  {
    const int genre = strFileNameAndPath.find_first_of('=');
    if(genre <0)
      strFilename = g_localizeStrings.Get(260);
    else
      strFilename = g_localizeStrings.Get(260) + " - " + strFileNameAndPath.substr(genre+1).c_str();
  }

  // Windows SMB Network (SMB)
  else if (url.GetProtocol() == "smb" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20171);

  // XBMSP Network
  else if (url.GetProtocol() == "xbms" && strFilename.IsEmpty())
    strFilename = "XBMSP Network";

  // iTunes music share (DAAP)
  else if (url.GetProtocol() == "daap" && strFilename.IsEmpty())
    strFilename = g_localizeStrings.Get(20174);

  // HDHomerun Devices
  else if (url.GetProtocol() == "hdhomerun" && strFilename.IsEmpty())
    strFilename = "HDHomerun Devices";

  // ReplayTV Devices
  else if (url.GetProtocol() == "rtv")
    strFilename = "ReplayTV Devices";

  // SAP Streams
  else if (url.GetProtocol() == "sap" && strFilename.IsEmpty())
    strFilename = "SAP Streams";

  // Music Playlists
  else if (path.Left(24).Equals("special://musicplaylists"))
    strFilename = g_localizeStrings.Get(20011);

  // Video Playlists
  else if (path.Left(24).Equals("special://videoplaylists"))
    strFilename = g_localizeStrings.Get(20012);

  // now remove the extension if needed
  if (g_guiSettings.GetBool("filelists.hideextensions") && !bIsFolder)
  {
    RemoveExtension(strFilename);
    return strFilename;
  }
  return strFilename;
}

bool CUtil::GetVolumeFromFileName(const CStdString& strFileName, CStdString& strFileTitle, CStdString& strVolumeNumber)
{
  const CStdStringArray &regexps = g_advancedSettings.m_videoStackRegExps;

  CStdString strFileNameTemp = strFileName;
  CStdString strFileNameLower = strFileName;
  strFileNameLower.MakeLower();

  CStdString strVolume;
  CStdString strTestString;
  CRegExp reg;

//  CLog::Log(LOGDEBUG, "GetVolumeFromFileName:[%s]", strFileNameLower.c_str());
  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    CStdString strRegExp = regexps[i];
    if (!reg.RegComp(strRegExp.c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "Invalid RegExp:[%s]", regexps[i].c_str());
      continue;
    }
//    CLog::Log(LOGDEBUG, "Regexp:[%s]", regexps[i].c_str());

    int iFoundToken = reg.RegFind(strFileNameLower.c_str());
    if (iFoundToken >= 0)
    {
      int iRegLength = reg.GetFindLen();
      int iCount = reg.GetSubCount();

      /*
      reg.DumpOvector(LOGDEBUG);
      CLog::Log(LOGDEBUG, "Subcount=%i", iCount);
      for (int j = 0; j <= iCount; j++)
      {
        CStdString str = reg.GetMatch(j);
        CLog::Log(LOGDEBUG, "Sub(%i):[%s]", j, str.c_str());
      }
      */

      // simple regexp, only the volume is captured
      if (iCount == 1)
      {
        strVolumeNumber = reg.GetMatch(1);
        if (strVolumeNumber.IsEmpty()) return false;

        // Remove the extension (if any).  We do this on the base filename, as the regexp
        // match may include some of the extension (eg the "." in particular).
        // The extension will then be added back on at the end - there is no reason
        // to clean it off here. It will be cleaned off during the display routine, if
        // the settings to hide extensions are turned on.
        CStdString strFileNoExt = strFileNameTemp;
        RemoveExtension(strFileNoExt);
        CStdString strFileExt = strFileNameTemp.Right(strFileNameTemp.length() - strFileNoExt.length());
        CStdString strFileRight = strFileNoExt.Mid(iFoundToken + iRegLength);
        strFileTitle = strFileName.Left(iFoundToken) + strFileRight + strFileExt;

        return true;
      }

      // advanced regexp with prefix (1), volume (2), and suffix (3)
      else if (iCount == 3)
      {
        // second subpatten contains the stacking volume
        strVolumeNumber = reg.GetMatch(2);
        if (strVolumeNumber.IsEmpty()) return false;

        // everything before the regexp match
        strFileTitle = strFileName.Left(iFoundToken);

        // first subpattern contains prefix
        strFileTitle += reg.GetMatch(1);

        // third subpattern contains suffix
        strFileTitle += reg.GetMatch(3);

        // everything after the regexp match
        strFileTitle += strFileNameTemp.Mid(iFoundToken + iRegLength);

        return true;
      }

      // unknown regexp format
      else
      {
        CLog::Log(LOGERROR, "Incorrect movie stacking regexp format:[%s]", regexps[i].c_str());
      }
    }
  }
  return false;
}

void CUtil::RemoveExtension(CStdString& strFileName)
{
  if(strFileName.Find("://") >= 0)
  {
    CURI url(strFileName);
    strFileName = url.GetFileName();
    RemoveExtension(strFileName);
    url.SetFileName(strFileName);
    strFileName = url.Get();
    return;
  }

  int iPos = strFileName.ReverseFind(".");
  // Extension found
  if (iPos > 0)
  {
    CStdString strExtension;
    CUtil::GetExtension(strFileName, strExtension);
    strExtension.ToLower();

    CStdString strFileMask;
    strFileMask = g_stSettings.m_pictureExtensions;
    strFileMask += g_stSettings.m_musicExtensions;
    strFileMask += g_stSettings.m_videoExtensions;
    strFileMask += ".py|.xml|.milk|.xpr|.cdg";

    // Only remove if its a valid media extension
    if (strFileMask.Find(strExtension.c_str()) >= 0)
      strFileName = strFileName.Left(iPos);
  }
}

void CUtil::CleanString(CStdString& strFileName, CStdString& strTitle, CStdString& strTitleAndYear, CStdString& strYear, bool bIsFolder /* = false */)
{

  strTitleAndYear = strFileName;

  if (strFileName.Equals(".."))
   return;

  const CStdStringArray &regexps = g_advancedSettings.m_videoCleanStringRegExps;

  CRegExp reTags, reYear;
  CStdString strExtension;
  GetExtension(strFileName, strExtension);

  if (!reYear.RegComp(g_advancedSettings.m_videoCleanDateTimeRegExp))
  {
    CLog::Log(LOGERROR, "%s: Invalid datetime clean RegExp:'%s'", __FUNCTION__, g_advancedSettings.m_videoCleanDateTimeRegExp.c_str());
  }
  else
    {
    if (reYear.RegFind(strTitleAndYear.c_str()) >= 0)
    {
      strTitleAndYear = reYear.GetReplaceString("\\1");
    strYear = reYear.GetReplaceString("\\2");
            }
  }

  RemoveExtension(strTitleAndYear);

  for (unsigned int i = 0; i < regexps.size(); i++)
          {
    if (!reTags.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid string clean RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
          }
    int j=0;
    if ((j=reTags.RegFind(strFileName.ToLower().c_str())) > 0)
      strTitleAndYear = strTitleAndYear.Mid(0, j);
          }

  // final cleanup - special characters used instead of spaces:
  // all '_' tokens should be replaced by spaces
  // if the file contains no spaces, all '.' tokens should be replaced by
  // spaces - one possibility of a mistake here could be something like:
  // "Dr..StrangeLove" - hopefully no one would have anything like this.
  {
    bool initialDots = true;
    bool alreadyContainsSpace = (strTitleAndYear.Find(' ') >= 0);

    for (int i = 0; i < (int)strTitleAndYear.size(); i++)
  {
      char c = strTitleAndYear.GetAt(i);

      if (c != '.')
        initialDots = false;

      if ((c == '_') || ((!alreadyContainsSpace) && !initialDots && (c == '.')))
      {
        strTitleAndYear.SetAt(i, ' ');
      }
    }
  }

  strTitle = strTitleAndYear.Trim();

  // append year
  if (!strYear.IsEmpty())
    strTitleAndYear = strTitle + " (" + strYear + ")";

  // restore extension if needed
  if (!g_guiSettings.GetBool("filelists.hideextensions") && !bIsFolder)
    strTitleAndYear += strExtension;

}

void CUtil::GetCommonPath(CStdString& strParent, const CStdString& strPath)
{
  // find the common path of parent and path
  unsigned int j = 1;
  while (j <= min(strParent.size(), strPath.size()) && strnicmp(strParent.c_str(), strPath.c_str(), j) == 0)
    j++;
  strParent = strParent.Left(j - 1);
  // they should at least share a / at the end, though for things such as path/cd1 and path/cd2 there won't be
  if (!CUtil::HasSlashAtEnd(strParent))
  {
    // currently GetDirectory() removes trailing slashes
    CUtil::GetDirectory(strParent.Mid(0), strParent);
    CUtil::AddSlashAtEnd(strParent);
  }
}

bool CUtil::GetRecursiveParentPath(const CStdString& strChildPath, std::vector<CStdString>& output, unsigned int countHowManyLevels /* = 0 - means all levels */)
{
  CStdString strCurrentParent = "";
  CStdString strCurrentPath = strChildPath;
  unsigned int i = 0;

  if (strChildPath.IsEmpty())
    return false;

  while (GetParentPath(strCurrentPath, strCurrentParent))
  {
    if (strCurrentParent.IsEmpty())
    {
      return false;
    }

    output.push_back(strCurrentParent);
    strCurrentPath = strCurrentParent;
    strCurrentParent.clear();

    i++;

    if (i == countHowManyLevels)
      return true;
  }

  return true;
}

bool CUtil::GetParentPath(const CStdString& strPath, CStdString& strParent)
{
  strParent = "";

  CURI url(strPath);
  CStdString strFile = url.GetFileName();
  if ( ((url.GetProtocol() == "rar") || (url.GetProtocol() == "zip")) && strFile.IsEmpty())
  {
    strFile = url.GetHostName();
    return GetParentPath(strFile, strParent);
  }
  else if (url.GetProtocol() == "stack")
  {
    CStackDirectory dir;
    CFileItemList items;
    dir.GetDirectory(strPath,items);
    CUtil::GetDirectory(items[0]->m_strPath,items[0]->m_strDVDLabel);
    if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
      GetParentPath(items[0]->m_strDVDLabel, strParent);
    else
      strParent = items[0]->m_strDVDLabel;
    for( int i=1;i<items.Size();++i)
    {
      CUtil::GetDirectory(items[i]->m_strPath,items[i]->m_strDVDLabel);
      if (items[0]->m_strDVDLabel.Mid(0,6).Equals("rar://") || items[0]->m_strDVDLabel.Mid(0,6).Equals("zip://"))
        GetParentPath(items[i]->m_strDVDLabel, items[i]->m_strPath);
      else
        items[i]->m_strPath = items[i]->m_strDVDLabel;

      GetCommonPath(strParent,items[i]->m_strPath);
    }
    return true;
  }
  else if (url.GetProtocol() == "multipath")
  {
    // get the parent path of the first item
    return GetParentPath(CMultiPathDirectory::GetFirstPath(strPath), strParent);
  }
  else if (url.GetProtocol() == "plugin")
  {
    if (!url.GetOptions().IsEmpty())
    {
      url.SetOptions("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetFileName().IsEmpty())
    {
      url.SetFileName("");
      strParent = url.Get();
      return true;
    }
    if (!url.GetHostName().IsEmpty())
    {
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return true;  // already at root
  }
  else if (url.GetProtocol() == "special")
  {
    if (HasSlashAtEnd(strFile) )
      strFile = strFile.Left(strFile.size() - 1);
    if(strFile.ReverseFind('/') < 0)
      return false;
  }
  else if (strFile.size() == 0)
  {
    if (url.GetHostName().size() > 0)
    {
      // we have an share with only server or workgroup name
      // set hostname to "" and return true to get back to root
      url.SetHostName("");
      strParent = url.Get();
      return true;
    }
    return false;
  }

  if (HasSlashAtEnd(strFile) )
  {
    strFile = strFile.Left(strFile.size() - 1);
  }

  int iPos = strFile.ReverseFind('/');
#ifndef _LINUX
  if (iPos < 0)
  {
    iPos = strFile.ReverseFind('\\');
  }
#endif
  if (iPos < 0)
  {
    url.SetFileName("");
    strParent = url.Get();
    return true;
  }

  strFile = strFile.Left(iPos);

  CUtil::AddSlashAtEnd(strFile);

  url.SetFileName(strFile);
  strParent = url.Get();
  return true;
}

void CUtil::GetQualifiedFilename(const CStdString &strBasePath, CStdString &strFilename)
{
  //Make sure you have a full path in the filename, otherwise adds the base path before.
  CURI plItemUrl(strFilename);
  CURI plBaseUrl(strBasePath);
  int iDotDotLoc, iBeginCut, iEndCut;

  if (plBaseUrl.IsLocal()) //Base in local directory
  {
    if (plItemUrl.IsLocal() ) //Filename is local or not qualified
    {
#ifdef _LINUX
      if (!( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') ) ) //Filename not fully qualified
#else
      if (!( strFilename.c_str()[1] == ':')) //Filename not fully qualified
#endif
      {
        if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath))
        {
          strFilename = strBasePath + strFilename;
          strFilename.Replace('/', '\\');
        }
        else
        {
          strFilename = strBasePath + '\\' + strFilename;
          strFilename.Replace('/', '\\');
        }
      }
    }
    strFilename.Replace("\\.\\", "\\");
    while ((iDotDotLoc = strFilename.Find("\\..\\")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('\\') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
  else //Base is remote
  {
    if (plItemUrl.IsLocal()) //Filename is local
    {
#ifdef _LINUX
      if ( (strFilename.c_str()[1] == ':') || (strFilename.c_str()[0] == '/') )  //Filename not fully qualified
#else
      if (strFilename[1] == ':') // already fully qualified
#endif
        return;
      if (strFilename.c_str()[0] == '/' || strFilename.c_str()[0] == '\\' || HasSlashAtEnd(strBasePath)) //Begins with a slash.. not good.. but we try to make the best of it..

      {
        strFilename = strBasePath + strFilename;
        strFilename.Replace('\\', '/');
      }
      else
      {
        strFilename = strBasePath + '/' + strFilename;
        strFilename.Replace('\\', '/');
      }
    }
    strFilename.Replace("/./", "/");
    while ((iDotDotLoc = strFilename.Find("/../")) > 0)
    {
      iEndCut = iDotDotLoc + 4;
      iBeginCut = strFilename.Left(iDotDotLoc).ReverseFind('/') + 1;
      strFilename.Delete(iBeginCut, iEndCut - iBeginCut);
    }
  }
}

void CUtil::RunShortcut(const char* szShortcutPath)
{
}

void CUtil::GetHomePath(CStdString& strPath)
{
  char szXBEFileName[1024];
  CIoSupport::GetXbePath(szXBEFileName);
  strPath = getenv("XBMC_HOME");
  if (strPath != NULL && !strPath.IsEmpty())
  {
#ifdef _WIN32
    //expand potential relative path to full path
    if(GetFullPathName(strPath, 1024, szXBEFileName, 0) != 0)
    {
      strPath = szXBEFileName;
    }
#endif
  }
  else
  {
#ifdef __APPLE__
    int      result = -1;
    char     given_path[2*MAXPATHLEN];
    uint32_t path_size = 2*MAXPATHLEN;

    result = _NSGetExecutablePath(given_path, &path_size);
    if (result == 0)
    {
      // Move backwards to last /.
      for (int n=strlen(given_path)-1; given_path[n] != '/'; n--)
        given_path[n] = '\0';

      // Assume local path inside application bundle.
      strcat(given_path, "../Resources/Boxee/");

      // Convert to real path.
      char real_path[2*MAXPATHLEN];
      if (realpath(given_path, real_path) != NULL)
      {
        strPath = real_path;
        return;
      }
    }
#endif
    char *szFileName = strrchr(szXBEFileName, PATH_SEPARATOR_CHAR);
    *szFileName = 0;
    strPath = szXBEFileName;
  }
}

void CUtil::ReplaceExtension(const CStdString& strFile, const CStdString& strNewExtension, CStdString& strChangedFile)
{
  if(strFile.Find("://") >= 0)
  {
    CURI url(strFile);
    ReplaceExtension(url.GetFileName(), strNewExtension, strChangedFile);
    url.SetFileName(strChangedFile);
    strChangedFile = url.Get();
    return;
  }

  CStdString strExtension;
  GetExtension(strFile, strExtension);
  if ( strExtension.size() )
  {
    strChangedFile = strFile.substr(0, strFile.size() - strExtension.size()) ;
    strChangedFile += strNewExtension;
  }
  else
  {
    strChangedFile = strFile;
    strChangedFile += strNewExtension;
  }
}

bool CUtil::HasSlashAtEnd(const CStdString& strFile)
{
  if (strFile.size() == 0) return false;
  char kar = strFile.c_str()[strFile.size() - 1];

  if (kar == '/' || kar == '\\')
    return true;

  return false;
}

bool CUtil::IsRemote(const CStdString& strFile)
{
  if (IsMemCard(strFile) || IsCDDA(strFile) || IsISO9660(strFile) || IsPlugin(strFile))
    return false;

  if (IsSpecial(strFile))
    return false;

  if(IsStack(strFile))
    return IsRemote(CStackDirectory::GetFirstStackedFile(strFile));

  if (IsVirtualPath(strFile))
  { // virtual paths need to be checked separately
    CVirtualPathDirectory dir;
    vector<CStdString> paths;
    if (dir.GetPathes(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  if(IsMultiPath(strFile))
  { // virtual paths need to be checked separately
    vector<CStdString> paths;
    if (CMultiPathDirectory::GetPaths(strFile, paths))
    {
      for (unsigned int i = 0; i < paths.size(); i++)
        if (IsRemote(paths[i])) return true;
    }
    return false;
  }

  CURI url(strFile);
  if(IsInArchive(strFile))
    return IsRemote(url.GetHostName());

  if (!url.IsLocal())
    return true;

  return false;
}

bool CUtil::IsOnDVD(const CStdString& strFile)
{
#ifdef _WIN32
  if (strFile.Mid(1,1) == ":")
    return (GetDriveType(strFile.Left(2)) == DRIVE_CDROM);
#else
  if (strFile.Left(2).CompareNoCase("d:") == 0)
    return true;
#endif

  if (strFile.Left(4).CompareNoCase("dvd:") == 0)
    return true;

  if (strFile.Left(4).CompareNoCase("udf:") == 0)
    return true;

  if (strFile.Left(8).CompareNoCase("iso9660:") == 0)
    return true;

  if (strFile.Left(5).CompareNoCase("cdda:") == 0)
    return true;

  return false;
}

bool CUtil::IsOnLAN(const CStdString& strPath)
{
  if(IsMultiPath(strPath))
    return CUtil::IsOnLAN(CMultiPathDirectory::GetFirstPath(strPath));
  if(IsStack(strPath))
    return CUtil::IsOnLAN(CStackDirectory::GetFirstStackedFile(strPath));
  if(IsSpecial(strPath))
    return CUtil::IsOnLAN(CSpecialProtocol::TranslatePath(strPath));
  if(IsDAAP(strPath))
    return true;
  if(IsTuxBox(strPath))
    return true;
  if(IsUPnP(strPath))
    return true;

  CURI url(strPath);
  if(IsInArchive(strPath))
    return CUtil::IsOnLAN(url.GetHostName());

  if(!IsRemote(strPath))
    return false;

  CStdString host = url.GetHostName();
  if(host.length() == 0)
    return false;


  unsigned long address = ntohl(inet_addr(host.c_str()));
  if(address == INADDR_NONE)
  {
    CStdString ip;
    if(CDNSNameCache::Lookup(host, ip))
      address = ntohl(inet_addr(ip.c_str()));
  }

  if(address == INADDR_NONE)
  {
    // assume a hostname without dot's
    // is local (smb netbios hostnames)
    if(host.find('.') == string::npos)
      return true;
  }
  else
  {
    // check if we are on the local subnet
    if (!g_application.getNetwork().GetFirstConnectedInterface())
      return false;
    unsigned long subnet = ntohl(inet_addr(g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentNetmask()));
    unsigned long local  = ntohl(inet_addr(g_application.getNetwork().GetFirstConnectedInterface()->GetCurrentIPAddress()));
    if( (address & subnet) == (local & subnet) )
      return true;
  }
  return false;
}

bool CUtil::IsMultiPath(const CStdString& strPath)
{
  if (strPath.Left(12).Equals("multipath://")) return true;
  return false;
}

bool CUtil::IsDVD(const CStdString& strFile)
{
  CStdString strFileLow = strFile;
  strFileLow.MakeLower();
#if defined(_WIN32)
  if((GetDriveType(strFile.c_str()) == DRIVE_CDROM) || strFile.Left(6).Equals("dvd://"))
    return true;
#else
  if (strFileLow == "d:/"  || strFileLow == "d:\\"  || strFileLow == "d:" || strFileLow == "iso9660://" || strFileLow == "udf://" || strFileLow == "dvd://1" )
    return true;
#endif

  return false;
}

bool CUtil::IsVirtualPath(const CStdString& strFile)
{
  if (strFile.Left(14).Equals("virtualpath://")) return true;
  return false;
}

bool CUtil::IsStack(const CStdString& strFile)
{
  if (strFile.Left(8).Equals("stack://")) return true;
  return false;
}

bool CUtil::IsRAR(const CStdString& strFile)
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);

  if (strExtension.Equals(".001") && strFile.Mid(strFile.length()-7,7).CompareNoCase(".ts.001"))
    return true;
  if (strExtension.CompareNoCase(".cbr") == 0)
    return true;
  if (strExtension.CompareNoCase(".rar") == 0)
    return true;

  return false;
}

bool CUtil::IsInArchive(const CStdString &strFile)
{
  return IsInZIP(strFile) || IsInRAR(strFile);
}

bool CUtil::IsInZIP(const CStdString& strFile)
{
    CURI url(strFile);
  return url.GetProtocol() == "zip" && url.GetFileName() != "";
  }

bool CUtil::IsInRAR(const CStdString& strFile)
{
    CURI url(strFile);
  return url.GetProtocol() == "rar" && url.GetFileName() != "";
  }

bool CUtil::IsZIP(const CStdString& strFile) // also checks for comic books!
{
  CStdString strExtension;
  CUtil::GetExtension(strFile,strExtension);
  if (strExtension.CompareNoCase(".zip") == 0) return true;
  if (strExtension.CompareNoCase(".cbz") == 0) return true;
  return false;
}

bool CUtil::IsSpecial(const CStdString& strFile)
{
  return strFile.Left(8).Equals("special:");
}

bool CUtil::IsPlugin(const CStdString& strFile)
{
  return strFile.Left(7).Equals("plugin:");
}

bool CUtil::IsCDDA(const CStdString& strFile)
{
  return strFile.Left(5).Equals("cdda:");
}

bool CUtil::IsISO9660(const CStdString& strFile)
{
  return strFile.Left(8).Equals("iso9660:");
}

bool CUtil::IsSmb(const CStdString& strFile)
{
  return strFile.Left(4).Equals("smb:");
}

bool CUtil::IsDAAP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("daap:");
}

bool CUtil::IsUPnP(const CStdString& strFile)
{
    return strFile.Left(5).Equals("upnp:");
}

bool CUtil::IsNfs(const CStdString& strFile)
{
    return strFile.Left(4).Equals("nfs:");
}

bool CUtil::IsAfp(const CStdString& strFile)
{
    return strFile.Left(4).Equals("afp:");
}

bool CUtil::IsBms(const CStdString& strFile)
{
    return strFile.Left(4).Equals("bms:");
}

bool CUtil::IsRSS(const CStdString& strFile)
{
    return strFile.Left(4).ToLower().Equals("rss:");
}

bool CUtil::IsFlash(const CStdString& strFile)
{
  return strFile.Left(6).ToLower().Equals("flash:");
}

bool CUtil::IsMMS(const CStdString& strFile)
{
  return strFile.Left(4).ToLower().Equals("mms:");
}

bool CUtil::IsHTTP(const CStdString& strFile)
{
  return strFile.Left(5).ToLower().Equals("http:") || strFile.Left(6).ToLower().Equals("https:");
}

bool CUtil::IsScript(const CStdString& strFile)
{
    return strFile.Left(7).ToLower().Equals("script:");
}

bool CUtil::IsApp(const CStdString& strFile)
{
    return strFile.Left(4).ToLower().Equals("app:");
}

bool CUtil::IsMemCard(const CStdString& strFile)
{
  return strFile.Left(3).Equals("mem");
}

bool CUtil::IsTuxBox(const CStdString& strFile)
{
  return strFile.Left(7).Equals("tuxbox:");
}

bool CUtil::IsMythTV(const CStdString& strFile)
{
  return strFile.Left(5).Equals("myth:");
}

bool CUtil::IsDVB(const CStdString& strFile)
{
  return strFile.Left(4).Equals("dvb:");
}

bool CUtil::IsHDHomeRun(const CStdString& strFile)
{
  return strFile.Left(10).Equals("hdhomerun:");
}

bool CUtil::IsShoutCast(const CStdString& strFile)
{
  if (strstr(strFile.c_str(), "shout:") ) return true;
  return false;
}
bool CUtil::IsLastFM(const CStdString& strFile)
{
#ifdef HAS_LASTFM
  if (strstr(strFile.c_str(), "lastfm:") ) return true;
#endif
  return false;
}

bool CUtil::IsVTP(const CStdString& strFile)
{
  return strFile.Left(4).Equals("vtp:");
}

bool CUtil::IsHTSP(const CStdString& strFile)
{
  return strFile.Left(5).Equals("htsp:");
}

bool CUtil::IsLiveTV(const CStdString& strFile)
{
  CURI url(strFile);

  if (IsTuxBox(strFile) || IsVTP(strFile) || IsHDHomeRun(strFile) || IsHTSP(strFile))
    return true;

  if (IsMythTV(strFile) && url.GetFileName().Left(9) == "channels/")
    return true;

  if(IsDVB(strFile))
    return true;

  return false;
}

bool CUtil::IsBoxeeDb(const CStdString& strFile)
{
  if (strstr(strFile.c_str(), "boxeedb:") ) return true;
  return false;
}

bool CUtil::ExcludeFileOrFolder(const CStdString& strFileOrFolder, const CStdStringArray& regexps)
{
  if (strFileOrFolder.IsEmpty())
    return false;

  CStdString strExclude = strFileOrFolder;
  RemoveSlashAtEnd(strExclude);
  strExclude = GetFileName(strExclude);
  strExclude.MakeLower();

  CRegExp regExExcludes;

  for (unsigned int i = 0; i < regexps.size(); i++)
  {
    if (!regExExcludes.RegComp(regexps[i].c_str()))
    { // invalid regexp - complain in logs
      CLog::Log(LOGERROR, "%s: Invalid exclude RegExp:'%s'", __FUNCTION__, regexps[i].c_str());
      continue;
    }
    if (regExExcludes.RegFind(strExclude) > -1)
    {
      CLog::Log(LOGDEBUG, "%s: File '%s' excluded. (Matches exclude rule RegExp:'%s')", __FUNCTION__, strFileOrFolder.c_str(), regexps[i].c_str());
      return true;
    }
  }
  return false;
}

void CUtil::GetFileAndProtocol(const CStdString& strURL, CStdString& strDir)
{
  strDir = strURL;
  if (!IsRemote(strURL)) return ;
  if (IsDVD(strURL)) return ;

  CURI url(strURL);
  strDir.Format("%s://%s", url.GetProtocol().c_str(), url.GetFileName().c_str());
}

int CUtil::GetDVDIfoTitle(const CStdString& strFile)
{
  CStdString strFilename = GetFileName(strFile);
  if (strFilename.Equals("video_ts.ifo")) return 0;
  //VTS_[TITLE]_0.IFO
  return atoi(strFilename.Mid(4, 2).c_str());
}

void CUtil::UrlDecode(CStdString& strURLData)
//modified to be more accomodating - if a non hex value follows a % take the characters directly and don't raise an error.
// However % characters should really be escaped like any other non safe character (www.rfc-editor.org/rfc/rfc1738.txt)
{
  CStdString strResult;

  /* result will always be less than source */
  strResult.reserve( strURLData.length() );

  for (unsigned int i = 0; i < strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    if (kar == '+') strResult += ' ';
    else if (kar == '%')
    {
      if (i < strURLData.size() - 2)
      {
        CStdString strTmp;
        strTmp.assign(strURLData.substr(i + 1, 2));
        int dec_num=-1;
        sscanf(strTmp,"%x",(unsigned int *)&dec_num);
        if (dec_num<0 || dec_num>255)
          strResult += kar;
        else
        {
          strResult += (char)dec_num;
          i += 2;
        }
      }
      else
        strResult += kar;
    }
    else strResult += kar;
  }
  strURLData = strResult;
}

void CUtil::URLEncode(CStdString& strURLData)
{
  CStdString strResult;

  /* wonder what a good value is here is, depends on how often it occurs */
  strResult.reserve( strURLData.length() * 2 );

  for (int i = 0; i < (int)strURLData.size(); ++i)
  {
    int kar = (unsigned char)strURLData[i];
    //if (kar == ' ') strResult += '+';
    if (isalnum(kar)) strResult += kar;
    else
    {
      CStdString strTmp;
      strTmp.Format("%%%02.2x", kar);
      strResult += strTmp;
    }
  }
  strURLData = strResult;
}

bool CUtil::GetDirectoryName(const CStdString& strFileName, CStdString& strDescription)
{
  CStdString strFName = CUtil::GetFileName(strFileName);
  strDescription = strFileName.Left(strFileName.size() - strFName.size());
  CUtil::RemoveSlashAtEnd(strDescription);

  int iPos = strDescription.ReverseFind("\\");
  if (iPos < 0)
    iPos = strDescription.ReverseFind("/");
  if (iPos >= 0)
  {
    CStdString strTmp = strDescription.Right(strDescription.size()-iPos-1);
    strDescription = strTmp;//strDescription.Right(strDescription.size() - iPos - 1);
  }
  else if (strDescription.size() <= 0)
    strDescription = strFName;
  return true;
}

bool CUtil::ConstructStringFromTemplate(const CStdString& strTemplate, const std::map<CStdString , CStdString>& mapTemplateKeyToValue, CStdString& output, const CStdString& delimiter)
{
  if (strTemplate.IsEmpty() || mapTemplateKeyToValue.empty())
  {
    return false;
  }

  output = strTemplate;

  size_t firstTag=0;

  while ((firstTag = output.Find(delimiter,firstTag)) != CStdString::npos)
  {
    if (firstTag != CStdString::npos && firstTag+1 < output.size())
    {
      //find the postfix delimiter
      size_t secondTag = output.Find(delimiter , firstTag+1);

      //make sure the postfix is after the prefix
      if (secondTag != CStdString::npos && secondTag > firstTag)
      {
        int iTagNameLength = secondTag-firstTag-1;

        //get the key name from the string
        CStdString tagName = output.substr(firstTag+1, iTagNameLength);

        //delete the variable from the string
        output.Delete(firstTag, iTagNameLength+2);

        //try finding it in the
        std::map<CStdString , CStdString>::const_iterator it = mapTemplateKeyToValue.find(tagName);
        if (it != mapTemplateKeyToValue.end())
        {
          //if we found a value for that key, we insert it into the string instead of the variable
          CStdString strInjected = " " + it->second;
          output.Insert(firstTag , strInjected);
          firstTag += strInjected.size();
        }
      }
      else
      {
        return false;
      }
    }
    else
    {
      return false;
    }
  }

  output.Trim();

  return true;
}

void CUtil::GetDVDDriveIcon( const CStdString& strPath, CStdString& strIcon )
{
  if ( !g_mediaManager.IsDiscInDrive() )
  {
    strIcon = "DefaultDVDEmpty.png";
    return ;
  }

  if ( IsDVD(strPath) )
  {
#ifdef HAS_DVD_DRIVE
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    //  xbox DVD
    if ( pInfo != NULL && pInfo->IsUDFX( 1 ) )
    {
      strIcon = "DefaultXboxDVD.png";
      return ;
    }
#endif    
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( IsISO9660(strPath) )
  {
#ifdef HAS_DVD_DRIVE    
    CCdInfo* pInfo = g_mediaManager.GetCdInfo();
    if ( pInfo != NULL && pInfo->IsVideoCd( 1 ) )
    {
      strIcon = "DefaultVCD.png";
      return ;
    }
#endif    
    strIcon = "DefaultDVDRom.png";
    return ;
  }

  if ( IsCDDA(strPath) )
  {
    strIcon = "DefaultCDDA.png";
    return ;
  }
}

void CUtil::RemoveTempFiles()
{
  WIN32_FIND_DATA wfd;

  CStdString strAlbumDir = CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), "*.tmp");
  memset(&wfd, 0, sizeof(wfd));

  CAutoPtrFind hFind( FindFirstFile(_P(strAlbumDir).c_str(), &wfd));
  if (!hFind.isValid())
    return ;
  do
  {
    if ( !(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) )
      CFile::Delete(CUtil::AddFileToFolder(g_settings.GetDatabaseFolder(), wfd.cFileName));
    }
  while (FindNextFile(hFind, &wfd));
}

bool CUtil::IsHD(const CStdString& strFileName)
{
  CURI url(_P(strFileName));
  return url.IsLocal();
  }

void CUtil::ClearSubtitles()
{
  //delete cached subs
  WIN32_FIND_DATA wfd;
#ifndef _LINUX
  CAutoPtrFind hFind ( FindFirstFile(_P("special://temp/*.*"), &wfd));
#else
  CAutoPtrFind hFind ( FindFirstFile(_P("special://temp/*"), &wfd));
#endif
  if (hFind.isValid())
  {
    do
    {
      if (wfd.cFileName[0] != 0)
      {
        if ( (wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0 )
        {
          CStdString strFile;
          strFile.Format("special://temp/%s", wfd.cFileName);
          if (strFile.Find("subtitle") >= 0 )
              CFile::Delete(strFile);
          else if (strFile.Find("vobsub_queue") >= 0 )
            CFile::Delete(strFile);
          }
        }
      }
    while (FindNextFile((HANDLE)hFind, &wfd));
  }
}

static const char * sub_exts[] = { ".utf", ".utf8", ".utf-8", ".sub", ".srt", ".smi", ".rt", ".txt", ".ssa", ".aqt", ".jss", ".ass", ".idx", NULL};

void CUtil::CacheSubtitles(const CStdString& strMovie, const CStdString& strContent, CStdString& strExtensionCached, XFILE::IFileCallback *pCallback )
{
  unsigned int startTimer = CTimeUtils::GetTimeMS();
  CLog::Log(LOGDEBUG,"%s: START", __FUNCTION__);

  // new array for commons sub dirs
  const char * common_sub_dirs[] = {"subs",
                              "Subs",
                              "subtitles",
                              "Subtitles",
                              "vobsubs",
                              "Vobsubs",
                              "sub",
                              "Sub",
                              "vobsub",
                              "Vobsub",
                              "subtitle",
                              "Subtitle",
                              NULL};

  vector<CStdString> vecExtensionsCached;
  strExtensionCached = "";

  ClearSubtitles();

  vector<CStdString> strLookInPaths;

  CStdString strFileName;
  CStdString strFileNameNoExt;
  CStdString strPath;

  if (strContent == "bluray/folder" || strContent == "dvd/folder")
  {
    CUtil::Split(strMovie, strPath, strFileName);
   
    strFileNameNoExt = strFileName = "index";
  }
  else if(strContent == "bluray/iso")
  {
    CStdString strParent;

    GetParentPath(strMovie, strPath);
    GetParentPath(strPath, strParent);

    RemoveSlashAtEnd(strParent);
    CUtil::Split(strParent, strPath, strFileName);
    ReplaceExtension(strFileName, "", strFileNameNoExt);   
  }
  else
  {
    CFileItem item(strMovie, false);
    if (item.IsInternetStream()) return ;
    if (item.IsHDHomeRun()) return ;
    if (item.IsPlayList()) return ;
    if (!item.IsVideo()) return ;


    CUtil::Split(strMovie, strPath, strFileName);
    ReplaceExtension(strFileName, "", strFileNameNoExt);
  }

  strLookInPaths.push_back(strPath);

  if (!g_stSettings.iAdditionalSubtitleDirectoryChecked && !g_guiSettings.GetString("subtitles.custompath").IsEmpty()) // to avoid checking non-existent directories (network) every time..
  {
    if (!g_application.getNetwork().IsAvailable() && !IsHD(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonaccessible");
      g_stSettings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }
    else if (!CDirectory::Exists(g_guiSettings.GetString("subtitles.custompath")))
    {
      CLog::Log(LOGINFO,"CUtil::CacheSubtitles: disabling alternate subtitle directory for this session, it's nonexistant");
      g_stSettings.iAdditionalSubtitleDirectoryChecked = -1; // disabled
    }

    g_stSettings.iAdditionalSubtitleDirectoryChecked = 1;
  }

  if (strMovie.substr(0,6) == "rar://") // <--- if this is found in main path then ignore it!
  {
    CURI url(strMovie);
    CStdString strArchive = url.GetHostName();
    CUtil::Split(strArchive, strPath, strFileName);
    strLookInPaths.push_back(strPath);
  }

  // checking if any of the common subdirs exist ..
  CLog::Log(LOGDEBUG,"%s: Checking for common subirs...", __FUNCTION__);

  vector<CStdString> token;
  Tokenize(strPath,token,"/\\");
  if (token.size() > 0 && token[token.size()-1].size() == 3 && token[token.size()-1].Mid(0,2).Equals("cd"))
  {
    CStdString strPath2;
    GetParentPath(strPath,strPath2);
    strLookInPaths.push_back(strPath2);
  } 
  int iSize = strLookInPaths.size();
  for (int i=0;i<iSize;++i)
  {
    for (int j=0; common_sub_dirs[j]; j++)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],common_sub_dirs[j],strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
        }
      }
  // .. done checking for common subdirs

  // check if there any cd-directories in the paths we have added so far
  char temp[6];
  iSize = strLookInPaths.size();
  for (int i=0;i<9;++i) // 9 cd's
  {
    sprintf(temp,"cd%i",i+1);
    for (int i=0;i<iSize;++i)
    {
      CStdString strPath2;
      CUtil::AddFileToFolder(strLookInPaths[i],temp,strPath2);
      if (CDirectory::Exists(strPath2))
        strLookInPaths.push_back(strPath2);
    }
  }
  // .. done checking for cd-dirs

  // this is last because we dont want to check any common subdirs or cd-dirs in the alternate <subtitles> dir.
  if (g_stSettings.iAdditionalSubtitleDirectoryChecked == 1)
  {
    strPath = g_guiSettings.GetString("subtitles.custompath");
    if (!HasSlashAtEnd(strPath))
      strPath += "/"; //Should work for both remote and local files
    strLookInPaths.push_back(strPath);
  }

  unsigned int nextTimer = CTimeUtils::GetTimeMS();
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(nextTimer - startTimer));

  CStdString strLExt;
  CStdString strDest;
  CStdString strItem;

  // 2 steps for movie directory and alternate subtitles directory
  CLog::Log(LOGDEBUG,"%s: Searching for subtitles...", __FUNCTION__);
  for (unsigned int step = 0; step < strLookInPaths.size(); step++)
  {
    if (strLookInPaths[step].length() != 0)
    {
      CFileItemList items;

      CDirectory::GetDirectory(strLookInPaths[step], items,".utf|.utf8|.utf-8|.sub|.srt|.smi|.rt|.txt|.ssa|.text|.ssa|.aqt|.jss|.ass|.idx|.ifo|.rar|.zip",false);
      int fnl = strFileNameNoExt.size();

      CStdString strFileNameNoExtNoCase(strFileNameNoExt);
      strFileNameNoExtNoCase.MakeLower();
      for (int j = 0; j < (int)items.Size(); j++)
      {
        Split(items[j]->m_strPath, strPath, strItem);

        // is this a rar-file ..
        if ((CUtil::IsRAR(strItem) || CUtil::IsZIP(strItem)) && g_guiSettings.GetBool("subtitles.searchrars"))
        {
          CStdString strRar, strItemWithPath;
          CUtil::AddFileToFolder(strLookInPaths[step],strFileNameNoExt+CUtil::GetExtension(strItem),strRar);
          CUtil::AddFileToFolder(strLookInPaths[step],strItem,strItemWithPath);

          unsigned int iPos = strMovie.substr(0,6)=="rar://"?1:0;
          iPos = strMovie.substr(0,6)=="zip://"?1:0;
          if ((step != iPos) || (strFileNameNoExtNoCase+".rar").Equals(strItem) || (strFileNameNoExtNoCase+".zip").Equals(strItem))
            CacheRarSubtitles( vecExtensionsCached, items[j]->m_strPath, strFileNameNoExtNoCase);
        }
        else
        {
          for (int i = 0; sub_exts[i]; i++)
          {
            int l = strlen(sub_exts[i]);

            //Cache any alternate subtitles.
            if (strItem.Left(9).ToLower() == "subtitle." && strItem.Right(l).ToLower() == sub_exts[i])
            {
              strLExt = strItem.Right(strItem.GetLength() - 9);
              strDest.Format("special://temp/subtitle.alt-%s", strLExt);
              if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
              {
                CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                strExtensionCached = strLExt;
              }
            }

            //Cache subtitle with same name as movie
            if (strItem.Right(l).ToLower() == sub_exts[i] && strItem.Left(fnl).ToLower() == strFileNameNoExt.ToLower())
            {
              strLExt = strItem.Right(strItem.size() - fnl);
              strDest.Format("special://temp/subtitle%s", strLExt);
              if (find(vecExtensionsCached.begin(),vecExtensionsCached.end(),strLExt) == vecExtensionsCached.end())
              {
                if (CFile::Cache(items[j]->m_strPath, strDest, pCallback, NULL))
                {
                  vecExtensionsCached.push_back(strLExt);
                  CLog::Log(LOGINFO, " cached subtitle %s->%s\n", strItem.c_str(), strDest.c_str());
                }
              }
            }
          }
        }
      }

      g_directoryCache.ClearDirectory(strLookInPaths[step]);
    }
  }
  CLog::Log(LOGDEBUG,"%s: Done (time: %i ms)", __FUNCTION__, (int)(CTimeUtils::GetTimeMS() - nextTimer));

  // construct string of added exts?
  for (vector<CStdString>::iterator it=vecExtensionsCached.begin(); it != vecExtensionsCached.end(); ++it)
    strExtensionCached += *it+" ";

  CLog::Log(LOGDEBUG,"%s: END (total time: %i ms)", __FUNCTION__, (int)(CTimeUtils::GetTimeMS() - startTimer));
}

bool CUtil::CacheRarSubtitles(vector<CStdString>& vecExtensionsCached, const CStdString& strRarPath, const CStdString& strCompare, const CStdString& strExtExt)
{
  bool bFoundSubs = false;
  CFileItemList ItemList;

  // zip only gets the root dir
  if (CUtil::GetExtension(strRarPath).Equals(".zip"))
  {
    CStdString strZipPath;
    CUtil::CreateArchivePath(strZipPath,"zip",strRarPath,"");
    if (!CDirectory::GetDirectory(strZipPath,ItemList,"",false))
      return false;
  }
  else
  {
    // get _ALL_files in the rar, even those located in subdirectories because we set the bMask to false.
    // so now we dont have to find any subdirs anymore, all files in the rar is checked.
    if( !g_RarManager.GetFilesInRar(ItemList, strRarPath, false, "") )
      return false;
  }
  for (int it= 0 ; it <ItemList.Size();++it)
  {
    CStdString strPathInRar = ItemList[it]->m_strPath;
    CStdString strExt = CUtil::GetExtension(strPathInRar);

    if (find(vecExtensionsCached.begin(),vecExtensionsCached.end(),strExt) != vecExtensionsCached.end())
      continue;

    CLog::Log(LOGDEBUG, "CacheRarSubs:: Found file %s", strPathInRar.c_str());
    // always check any embedded rar archives
    // checking for embedded rars, I moved this outside the sub_ext[] loop. We only need to check this once for each file.
    if (CUtil::IsRAR(strPathInRar) || CUtil::IsZIP(strPathInRar))
    {
      CStdString strExtAdded;
      CStdString strRarInRar;
      if (CUtil::GetExtension(strPathInRar).Equals(".rar"))
        CUtil::CreateArchivePath(strRarInRar, "rar", strRarPath, strPathInRar);
      else
        CUtil::CreateArchivePath(strRarInRar, "zip", strRarPath, strPathInRar);
      CacheRarSubtitles(vecExtensionsCached,strRarInRar,strCompare, strExtExt);
    }
    // done checking if this is a rar-in-rar

    int iPos=0;
    CStdString strFileName = CUtil::GetFileName(strPathInRar);
    CStdString strFileNameNoCase(strFileName);
    strFileNameNoCase.MakeLower();
    if (strFileNameNoCase.Find(strCompare) >= 0)
      while (sub_exts[iPos])
      {
        if (strExt.CompareNoCase(sub_exts[iPos]) == 0)
        {
          CStdString strSourceUrl, strDestUrl;
          if (CUtil::GetExtension(strRarPath).Equals(".rar"))
            CUtil::CreateArchivePath(strSourceUrl, "rar", strRarPath, strPathInRar);
          else
            strSourceUrl = strPathInRar;

          CStdString strDestFile;
          strDestFile.Format("special://temp/subtitle%s%s", sub_exts[iPos],strExtExt.c_str());

          if (CFile::Cache(strSourceUrl,strDestFile))
          {
            vecExtensionsCached.push_back(CStdString(sub_exts[iPos]));
            CLog::Log(LOGINFO, " cached subtitle %s->%s", strPathInRar.c_str(), strDestFile.c_str());
            bFoundSubs = true;
            break;
          }
        }

        iPos++;
      }
  }
  return bFoundSubs;
}

int64_t CUtil::ToInt64(uint32_t high, uint32_t low)
{
  int64_t n;
  n = high;
  n <<= 32;
  n += low;
  return n;
}

bool CUtil::IsDOSPath(const CStdString &path)
{
  if (path.size() > 1 && path[1] == ':' && isalpha(path[0]))
    return true;

  return false;
}

void CUtil::AddFileToFolder(const CStdString& strFolder, const CStdString& strFile, CStdString& strResult)
  {
  if(strFolder.Find("://") >= 0)
  {
    CURI url(strFolder);
    AddFileToFolder(url.GetFileName(), strFile, strResult);
    url.SetFileName(strResult);
    strResult = url.Get();
    return;
  }

  strResult = strFolder;
  if(!strResult.IsEmpty())
    AddSlashAtEnd(strResult);

  // Remove any slash at the start of the file
  if (!strResult.IsEmpty() && strFile.size() && (strFile[0] == '/' || strFile[0] == '\\'))
    strResult += strFile.Mid(1);
  else
    strResult += strFile;

  // re-add the stack:// protocol
  if (IsStack(strFolder))
    strResult = "stack://" + strResult;

  // correct any slash directions
  if (!IsDOSPath(strFolder))
    strResult.Replace('\\', '/');
  else
    strResult.Replace('/', '\\');
}

void CUtil::AddSlashAtEnd(CStdString& strFolder)
{
  if(strFolder.Find("://") >= 0)
  {
    CURI url(strFolder);
    strFolder = url.GetFileName();
    if(!strFolder.IsEmpty())
    {
      AddSlashAtEnd(strFolder);
      url.SetFileName(strFolder);
    }
    strFolder = url.Get();
    return;
  }

  if (!CUtil::HasSlashAtEnd(strFolder))
  {
    if (IsDOSPath(strFolder))
      strFolder += '\\';
    else
      strFolder += '/';
  }
}

void CUtil::RemoveSlashAtEnd(CStdString& strFolder)
{
  if(strFolder.Find("://") >= 0)
  {
    CURI url(strFolder);
    strFolder = url.GetFileName();
    if (!strFolder.IsEmpty())
    {
      RemoveSlashAtEnd(strFolder);
      url.SetFileName(strFolder);
    }
    strFolder = url.Get();
    return;
  }

  while (CUtil::HasSlashAtEnd(strFolder))
    strFolder.Delete(strFolder.size() - 1);
}

void CUtil::GetDirectory(const CStdString& strFilePath, CStdString& strDirectoryPath)
{
  // Will from a full filename return the directory the file resides in.
  // Keeps the final slash at end

  int iPos1 = strFilePath.ReverseFind('/');
  int iPos2 = strFilePath.ReverseFind('\\');

  if (iPos2 > iPos1)
  {
    iPos1 = iPos2;
  }

  if (iPos1 > 0)
  {
    strDirectoryPath = strFilePath.Left(iPos1 + 1); // include the slash
  }
}

void CUtil::Split(const CStdString& strFileNameAndPath, CStdString& strPath, CStdString& strFileName)
{
  //Splits a full filename in path and file.
  //ex. smb://computer/share/directory/filename.ext -> strPath:smb://computer/share/directory/ and strFileName:filename.ext
  //Trailing slash will be preserved
  strFileName = "";
  strPath = "";
  int i = strFileNameAndPath.size() - 1;
  while (i > 0)
  {
    char ch = strFileNameAndPath[i];
    // Only break on ':' if it's a drive separator for DOS (ie d:foo)
    if (ch == '/' || ch == '\\' || (ch == ':' && i == 1)) break;
    else i--;
  }
  if (i == 0)
    i--;

  strPath = strFileNameAndPath.Left(i + 1);
  strFileName = strFileNameAndPath.Right(strFileNameAndPath.size() - i - 1);
}

void CUtil::CreateArchivePath(CStdString& strUrlPath, const CStdString& strType,
                              const CStdString& strArchivePath,
                              const CStdString& strFilePathInArchive,
                              const CStdString& strPwd)
{
  CStdString strBuffer;

  strUrlPath = strType+"://";

  if( !strPwd.IsEmpty() )
  {
    strBuffer = strPwd;
    CUtil::URLEncode(strBuffer);
    strUrlPath += strBuffer;
    strUrlPath += "@";
  }

  strBuffer = strArchivePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += strBuffer;

  strBuffer = strFilePathInArchive;
  strBuffer.Replace('\\', '/');
  strBuffer.TrimLeft('/');

  strUrlPath += "/";
  strUrlPath += strBuffer;

#if 0 // options are not used
  strBuffer = strCachePath;
  CUtil::URLEncode(strBuffer);

  strUrlPath += "?cache=";
  strUrlPath += strBuffer;

  strBuffer.Format("%i", wOptions);
  strUrlPath += "&flags=";
  strUrlPath += strBuffer;
#endif
}

bool CUtil::ThumbExists(const CStdString& strFileName, bool bAddCache)
{
  return CThumbnailCache::GetThumbnailCache()->ThumbExists(strFileName, bAddCache);
}

void CUtil::ThumbCacheAdd(const CStdString& strFileName, bool bFileExists)
{
  CThumbnailCache::GetThumbnailCache()->Add(strFileName, bFileExists);
}

void CUtil::ThumbCacheClear()
{
  CThumbnailCache::GetThumbnailCache()->Clear();
}

bool CUtil::ThumbCached(const CStdString& strFileName)
{
  return CThumbnailCache::GetThumbnailCache()->IsCached(strFileName);
}

void CUtil::PlayDVD()
{
#ifdef HAS_DVD_DRIVE
  CIoSupport::Dismount("Cdrom0");
  CIoSupport::RemapDriveLetter('D', "Cdrom0");
  CFileItem item("dvd://1", false);
  item.SetLabel(g_mediaManager.GetDiskLabel());
  g_application.PlayFile(item);
#endif
}

CStdString CUtil::GetNextFilename(const CStdString &fn_template, int max)
{
  if (!fn_template.Find("%03d"))
    return "";

  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(fn_template.c_str(), i);

  WIN32_FIND_DATA wfd;
  HANDLE hFind;
      memset(&wfd, 0, sizeof(wfd));
    if ((hFind = FindFirstFile(_P(name).c_str(), &wfd)) != INVALID_HANDLE_VALUE)
        FindClose(hFind);
      else
      {
        // FindFirstFile didn't find the file 'szName', return it
      return name;
      }
    }
  return "";
  }

CStdString CUtil::GetNextPathname(const CStdString &path_template, int max)
{
  if (!path_template.Find("%04d"))
    return "";

  for (int i = 0; i <= max; i++)
  {
    CStdString name;
    name.Format(path_template.c_str(), i);
    if (!CFile::Exists(name))
      return name;
  }
  return "";
}

void CUtil::InitGamma()
{
#ifdef HAS_DX
  g_Windowing.Get3DDevice()->GetGammaRamp(0, &oldramp);
#endif
}
void CUtil::RestoreBrightnessContrastGamma()
{
  g_graphicsContext.Lock();
#ifdef HAS_DX
  g_Windowing.Get3DDevice()->SetGammaRamp(0, GAMMA_RAMP_FLAG, &oldramp);
#endif
  g_graphicsContext.Unlock();
}

void CUtil::SetBrightnessContrastGammaPercent(float brightness, float contrast, float gamma, bool immediate)
{
  if (brightness < 0.0f) brightness = 0.0f;
  if (brightness > 100.0f) brightness = 100.0f;
  if (contrast < 0.0f) contrast = 0.0f;
  if (contrast > 100.0f) contrast = 100.0f;
  if (gamma < 0.0f) gamma = 0.0f;
  if (gamma > 100.0f) gamma = 100.0f;

  float fBrightNess = brightness / 50.0f - 1.0f; // -1..1    Default: 0
  float fContrast = contrast / 50.0f;            // 0..2     Default: 1
  float fGamma = gamma / 40.0f + 0.5f;           // 0.5..3.0 Default: 1
  CUtil::SetBrightnessContrastGamma(fBrightNess, fContrast, fGamma, immediate);
}

void CUtil::SetBrightnessContrastGamma(float Brightness, float Contrast, float Gamma, bool bImmediate)
{
  // calculate ramp
#ifdef HAS_DX
  D3DGAMMARAMP ramp;
#endif

  Gamma = 1.0f / Gamma;
#ifdef HAS_DX
  for (int i = 0; i < 256; ++i)
  {
    float f = (powf((float)i / 255.f, Gamma) * Contrast + Brightness) * 255.f;
    ramp.blue[i] = ramp.green[i] = ramp.red[i] = clamp(f);
  }
#endif

  // set ramp next v sync
  g_graphicsContext.Lock();
#ifdef HAS_DX
  g_Windowing.Get3DDevice()->SetGammaRamp(0, bImmediate ? GAMMA_RAMP_FLAG : 0, &ramp);
#endif
  g_graphicsContext.Unlock();
}


void CUtil::Tokenize(const CStdString& path, vector<CStdString>& tokens, const string& delimiters)
{
  // Tokenize ripped from http://www.linuxselfhelp.com/HOWTO/C++Programming-HOWTO-7.html
  // Skip delimiters at beginning.
  string::size_type lastPos = path.find_first_not_of(delimiters, 0);
  // Find first "non-delimiter".
  string::size_type pos = path.find_first_of(delimiters, lastPos);

  while (string::npos != pos || string::npos != lastPos)
  {
    // Found a token, add it to the vector.
    tokens.push_back(path.substr(lastPos, pos - lastPos));
    // Skip delimiters.  Note the "not_of"
    lastPos = path.find_first_not_of(delimiters, pos);
    // Find next "non-delimiter"
    pos = path.find_first_of(delimiters, lastPos);
  }
}


void CUtil::FlashScreen(bool bImmediate, bool bOn)
{
  static bool bInFlash = false;

  if (bInFlash == bOn)
    return ;
  bInFlash = bOn;
  g_graphicsContext.Lock();
  if (bOn)
  {
#ifdef HAS_DX
    g_Windowing.Get3DDevice()->GetGammaRamp(0, &flashramp);
#endif
    SetBrightnessContrastGamma(0.5f, 1.2f, 2.0f, bImmediate);
  }
  else
#ifdef HAS_DX
    g_Windowing.Get3DDevice()->SetGammaRamp(0, bImmediate ? GAMMA_RAMP_FLAG : 0, &flashramp);
#endif
  g_graphicsContext.Unlock();
}

void CUtil::TakeScreenshot(const char* fn, bool flashScreen)
{
#ifdef HAS_DX
    LPDIRECT3DSURFACE9 lpSurface = NULL;
	LPDIRECT3DDEVICE9 lpD3dDevice = NULL;
	HRESULT hr;

    g_graphicsContext.Lock();
    if (g_application.IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    if (0)
    { // reset calibration to defaults
      OVERSCAN oscan;
      memcpy(&oscan, &g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, sizeof(OVERSCAN));
      g_graphicsContext.ResetOverscan(g_graphicsContext.GetVideoResolution(), g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan);
      g_application.Render();
      memcpy(&g_settings.m_ResInfo[g_graphicsContext.GetVideoResolution()].Overscan, &oscan, sizeof(OVERSCAN));
    }
    // now take screenshot
    g_application.RenderNoPresent();
	
	lpD3dDevice = g_Windowing.Get3DDevice();

  if(lpD3dDevice)
    {
    D3DVIEWPORT9 d3dvViewport;

    hr = lpD3dDevice->GetViewport(&d3dvViewport);

		if (SUCCEEDED(hr))
      {
      D3DDISPLAYMODE displayMode;

      hr = lpD3dDevice->GetDisplayMode(0, &displayMode);    

			if (SUCCEEDED(hr))
			{
        hr = lpD3dDevice->CreateOffscreenPlainSurface(displayMode.Width, displayMode.Height,
                              D3DFMT_A8R8G8B8, D3DPOOL_SYSTEMMEM, &lpSurface, NULL);

        if(SUCCEEDED(hr))
        {
				hr = lpD3dDevice->GetFrontBufferData(0, lpSurface);

				if (SUCCEEDED(hr))
				{
            POINT WindowPoint = {0,0};
            RECT WindowRect;
            HWND hWnd = g_Windowing.GetHwnd();

            ClientToScreen(hWnd, &WindowPoint );
            GetClientRect(hWnd, &WindowRect );

            WindowRect.top = WindowPoint.y; 
            WindowRect.left = WindowPoint.x;
					  WindowRect.bottom += WindowPoint.y; 
            WindowRect.right += WindowPoint.x;

            hr = D3DXSaveSurfaceToFile(_P(fn), D3DXIFF_BMP, lpSurface, NULL, &WindowRect);

            if(FAILED(hr))
            {
              CLog::Log(LOGERROR, "D3DXSaveSurfaceToFile failed, error code 0x%x\n", hr);
			}
		}
	}
      }
    }
  }

	if (FAILED(hr))
	{
		CLog::Log(LOGERROR, "Failed to Generate Screenshot, error = 0x%x", hr);
      }
      else
      {
        CLog::Log(LOGINFO, "Screen shot saved as %s", fn);
      }



	if (lpSurface)
	{
      lpSurface->Release();
    }

    g_graphicsContext.Unlock();
    if (flashScreen)
    {
      FlashScreen(true, true);
      Sleep(10);
      FlashScreen(true, false);
    }
#else

#endif

#if defined(HAS_GL)

    g_graphicsContext.BeginPaint();
    if (g_application.IsPlayingVideo())
    {
#ifdef HAS_VIDEO_PLAYBACK
      g_renderManager.SetupScreenshot();
#endif
    }
    g_application.RenderNoPresent();

    GLint viewport[4];
    void *pixels = NULL;
    glReadBuffer(GL_BACK);
    glGetIntegerv(GL_VIEWPORT, viewport);
    pixels = malloc(viewport[2] * viewport[3] * 4);
    if (pixels)
    {
      glReadPixels(viewport[0], viewport[1], viewport[2], viewport[3], GL_BGRA, GL_UNSIGNED_BYTE, pixels);
      XGWriteSurfaceToFile(pixels, viewport[2], viewport[3], fn);
      free(pixels);
    }
    g_graphicsContext.EndPaint();

#endif

}

void CUtil::TakeScreenshot()
{
  // check to see if we have a screenshot folder yet
  CStdString strDir = g_guiSettings.GetString("pictures.screenshotpath", false);
  if (strDir.IsEmpty())
    strDir = "special://home/screenshots";
  CUtil::RemoveSlashAtEnd(strDir);

  if (!strDir.IsEmpty())
  {
    CStdString file = CUtil::GetNextFilename(CUtil::AddFileToFolder(strDir, "screenshot%03d.bmp"), 999);

    if (!file.IsEmpty())
    {
      TakeScreenshot(file.c_str(), true);
          }
    else
    {
      CLog::Log(LOGWARNING, "Too many screen shots or invalid folder");
    }
  }
}

void CUtil::ClearCache()
{
  for (int i = 0; i < 16; i++)
  {
    CStdString strHex, folder;
    strHex.Format("%x", i);
    CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), strHex, folder);
    g_directoryCache.ClearDirectory(folder);
  }
}

void CUtil::StatToStatI64(struct _stati64 *result, struct stat *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = (int64_t)stat->st_size;

#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::Stat64ToStatI64(struct _stati64 *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#else
  result->_st_atime = (long)(stat->st_atime & 0xFFFFFFFF);
  result->_st_mtime = (long)(stat->st_mtime & 0xFFFFFFFF);
  result->_st_ctime = (long)(stat->st_ctime & 0xFFFFFFFF);
#endif
}

void CUtil::StatI64ToStat64(struct __stat64 *result, struct _stati64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
  result->st_size = stat->st_size;
#ifndef _LINUX
  result->st_atime = stat->st_atime;
  result->st_mtime = stat->st_mtime;
  result->st_ctime = stat->st_ctime;
#else
  result->st_atime = stat->_st_atime;
  result->st_mtime = stat->_st_mtime;
  result->st_ctime = stat->_st_ctime;
#endif
}

void CUtil::Stat64ToStat(struct stat *result, struct __stat64 *stat)
{
  result->st_dev = stat->st_dev;
  result->st_ino = stat->st_ino;
  result->st_mode = stat->st_mode;
  result->st_nlink = stat->st_nlink;
  result->st_uid = stat->st_uid;
  result->st_gid = stat->st_gid;
  result->st_rdev = stat->st_rdev;
#ifndef _LINUX
  if (stat->st_size <= LONG_MAX)
    result->st_size = (_off_t)stat->st_size;
#else
  if (sizeof(stat->st_size) <= sizeof(result->st_size) )
    result->st_size = (off_t)stat->st_size;
#endif
  else
  {
    result->st_size = 0;
    CLog::Log(LOGWARNING, "WARNING: File is larger than 32bit stat can handle, file size will be reported as 0 bytes");
  }
  result->st_atime = (time_t)stat->st_atime;
  result->st_mtime = (time_t)stat->st_mtime;
  result->st_ctime = (time_t)stat->st_ctime;
}

bool CUtil::CreateDirectoryEx(const CStdString& _strPath)
{
  CStdString strPath = _strPath;

  if(CUtil::IsSpecial(strPath))
  {
    strPath = _P(strPath);
  }
  
  // Function to create all directories at once instead
  // of calling CreateDirectory for every subdir.
  // Creates the directory and subdirectories if needed.

  // return true if directory already exist
  if (CDirectory::Exists(strPath)) return true;

  // we currently only allow HD and smb paths
  if (!CUtil::IsHD(strPath) && !CUtil::IsSmb(strPath))
  {
    CLog::Log(LOGERROR,"%s called with an unsupported path: %s", __FUNCTION__, strPath.c_str());
    return false;
  }

  CURI url(strPath);
  // silly CStdString can't take a char in the constructor
  CStdString sep(1, url.GetDirectorySeparator());
  
  // split the filename portion of the URL up into separate dirs
  CStdStringArray dirs;
  StringUtils::SplitString(url.GetFileName(), sep, dirs);
  
  // we start with the root path
  CStdString dir;
  dir = url.GetWithoutFilename();
  unsigned int i = 0;
  if (dir.IsEmpty())
  { // local directory - start with the first dirs member so that
    // we ensure CUtil::AddFileToFolder() below has something to work with
    dir = dirs[i++] + sep;
  }
  // and append the rest of the directories successively, creating each dir
  // as we go
  for (; i < dirs.size(); i++)
  {
    dir = CUtil::AddFileToFolder(dir, dirs[i]);
    CDirectory::Create(dir);
  }

  // was the final destination directory successfully created ?
  if (!CDirectory::Exists(strPath))
  { 
    return false;
  }
  else
  {
  return true;
}
}

CStdString CUtil::MakeLegalFileName(const CStdString &strFile, int LegalType)
{
  CStdString result = strFile;

  result.Replace('/', '_');
  result.Replace('\\', '_');
  result.Replace('?', '_');

  if (LegalType == LEGAL_WIN32_COMPAT) 
  {
    // just filter out some illegal characters on windows
    result.Replace(':', '_');
    result.Replace('*', '_');
    result.Replace('?', '_');
    result.Replace('\"', '_');
    result.Replace('<', '_');
    result.Replace('>', '_');
    result.Replace('|', '_');
    result.TrimRight(".");
    result.TrimRight(" ");
  }
  return result;
}

// same as MakeLegalFileName, but we assume that we're passed a complete path,
// and just legalize the filename
CStdString CUtil::MakeLegalPath(const CStdString &strPathAndFile, int LegalType)
{
  CStdString strPath;
  GetDirectory(strPathAndFile,strPath);
  CStdString strFileName = GetFileName(strPathAndFile);
  return strPath + MakeLegalFileName(strFileName, LegalType);
}

bool CUtil::IsUsingTTFSubtitles()
{
#ifdef __APPLE__
  // That's all we use for now, baby.
  return true;
#else
  return CUtil::GetExtension(g_guiSettings.GetString("subtitles.font")).Equals(".ttf");
#endif
}

void CUtil::SplitExecFunction(const CStdString &execString, CStdString &function, vector<CStdString> &parameters)
{
  CStdString paramString;

  int iPos = execString.Find("(");
  int iPos2 = execString.ReverseFind(")");
  if (iPos > 0 && iPos2 > 0)
  {
    paramString = execString.Mid(iPos + 1, iPos2 - iPos - 1);
    function = execString.Left(iPos);
  }
  else
    function = execString;

  // remove any whitespace, and the standard prefix (if it exists)
  function.Trim();
  if( function.Left(5).Equals("xbmc.", false) )
    function.Delete(0, 5);
  if( function.Left(6).Equals("boxee.", false) )
    function.Delete(0, 6);

  // now split up our parameters - we may have quotes to deal with as well as brackets and whitespace
  bool inQuotes = false;
  int inFunction = 0;
  size_t whiteSpacePos = 0;
  CStdString parameter;
  parameters.clear();
  for (size_t pos = 0; pos < paramString.size(); pos++)
  {
    char ch = paramString[pos];
    bool escaped = (pos > 0 && paramString[pos - 1] == '\\');
    if (inQuotes)
    { // if we're in a quote, we accept everything until the closing quote
      if (ch == '\"' && !escaped)
      { // finished a quote - no need to add the end quote to our string
        inQuotes = false;
        continue;
}
    }
    else
    { // not in a quote, so check if we should be starting one
      if (ch == '\"' && !escaped)
      { // start of quote - no need to add the quote to our string
        inQuotes = true;
        continue;
      }
      if (inFunction && ch == ')')
      { // end of a function
        inFunction--;
      }
      if (ch == '(')
      { // start of function
        inFunction++;
      }
      if (!inFunction && ch == ',')
      { // not in a function, so a comma signfies the end of this parameter
        if (whiteSpacePos)
          parameter = parameter.Left(whiteSpacePos);
        parameters.push_back(parameter);
        parameter.Empty();
        whiteSpacePos = 0;
        continue;
      }
    }
    // whitespace handling - we skip any whitespace at the left or right of an unquoted parameter
    if (ch == ' ' && !inQuotes)
    {
      if (parameter.IsEmpty()) // skip whitespace on left
        continue;
      if (!whiteSpacePos) // make a note of where whitespace starts on the right
        whiteSpacePos = parameter.size();
    }
    else
      whiteSpacePos = 0;
    parameter += ch;
  }
  if (inFunction || inQuotes)
    CLog::Log(LOGWARNING, "%s(%s) - end of string while searching for ) or \"", __FUNCTION__, execString.c_str());
  if (whiteSpacePos)
    parameter = parameter.Left(whiteSpacePos);
  if (!parameter.IsEmpty())
    parameters.push_back(parameter);
}

int CUtil::GetMatchingSource(const CStdString& strPath1, VECSOURCES& VECSOURCES, bool& bIsSourceName)
{
  if (strPath1.IsEmpty())
    return -1;

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing original path/name [%s]", strPath1.c_str());

  // copy as we may change strPath
  CStdString strPath = strPath1;

  // Check for special protocols
  CURI checkURL(strPath);

  // stack://
  if (checkURL.GetProtocol() == "stack")
    strPath.Delete(0, 8); // remove the stack protocol

  if (checkURL.GetProtocol() == "shout")
    strPath = checkURL.GetHostName();
  if (checkURL.GetProtocol() == "lastfm")
    return 1;
  if (checkURL.GetProtocol() == "tuxbox")
    return 1;
  if (checkURL.GetProtocol() == "plugin")
    return 1;
  if (checkURL.GetProtocol() == "multipath")
    strPath = CMultiPathDirectory::GetFirstPath(strPath);

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing for matching name [%s]", strPath.c_str());
  bIsSourceName = false;
  int iIndex = -1;
  int iLength = -1;
  // we first test the NAME of a source
  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);
    CStdString strName = share.strName;
    CStdString strSharePath = _P(share.strPath);

    // special cases for dvds
    if (IsOnDVD(strSharePath))
    {
      if (IsOnDVD(strPath))
        return i;

      // not a path, so we need to modify the source name
      // since we add the drive status and disc name to the source
      // "Name (Drive Status/Disc Name)"
      int iPos = strName.ReverseFind('(');
      if (iPos > 1)
        strName = strName.Mid(0, iPos - 1);
    }
    //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing name [%s]", strName.c_str());
    if (strPath.Equals(strName))
    {
      bIsSourceName = true;
      return i;
    }
  }

  // now test the paths

  // remove user details, and ensure path only uses forward slashes
  // and ends with a trailing slash so as not to match a substring
  CURI urlDest(strPath);
  CStdString strDest;
  urlDest.SetOptions("");
  strDest = urlDest.GetWithoutUserDetails();
  ForceForwardSlashes(strDest);
  if (!HasSlashAtEnd(strDest))
    strDest += "/";
  int iLenPath = strDest.size();

  //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, testing url [%s]", strDest.c_str());

  for (int i = 0; i < (int)VECSOURCES.size(); ++i)
  {
    CMediaSource share = VECSOURCES.at(i);

    // does it match a source name?
    if (share.strPath.substr(0,8) == "shout://")
    {
      CURI url(share.strPath);
      if (strPath.Equals(url.GetHostName()))
        return i;
    }

    // doesnt match a name, so try the source path
    vector<CStdString> vecPaths;

    // add any concatenated paths if they exist
    if (share.vecPaths.size() > 0)
      vecPaths = share.vecPaths;

    // add the actual share path at the front of the vector
    vecPaths.insert(vecPaths.begin(), share.strPath);

    // test each path
    for (int j = 0; j < (int)vecPaths.size(); ++j)
    {
      // remove user details, and ensure path only uses forward slashes
      // and ends with a trailing slash so as not to match a substring
      CURI urlShare(_P(vecPaths[j]));
      CStdString strShare;
      urlShare.SetOptions("");
      strShare = urlShare.GetWithoutUserDetails();
      ForceForwardSlashes(strShare);

      if (strShare.IsEmpty())
        continue;

      if (!HasSlashAtEnd(strShare))
      {
        strShare += "/";
      }
      else if (strShare.Left(7) == "upnp://")
      {
        RemoveSlashAtEnd(strShare);
      }

      int iLenShare = strShare.size();
      //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, comparing url [%s] with share [%s]",strDest.c_str(), strShare.c_str());

      if ((iLenPath >= iLenShare) && (strDest.Left(iLenShare).Equals(strShare)) && (iLenShare > iLength))
      {
        //CLog::Log(LOGDEBUG,"CUtil::GetMatchingSource, Found matching source at index %i: [%s], Len = [%i]", i, strShare.c_str(), iLenShare);

        // if exact match, return it immediately
        if (iLenPath == iLenShare)
        {
          // if the path EXACTLY matches an item in a concatentated path
          // set source name to true to load the full virtualpath
          bIsSourceName = false;
          if (vecPaths.size() > 1)
            bIsSourceName = true;
          return i;
        }
        iIndex = i;
        iLength = iLenShare;
      }
    }
  }

  // return the index of the share with the longest match
  if (iIndex == -1)
  {

    // rar:// and zip://
    // if archive wasn't mounted, look for a matching share for the archive instead
    if( strPath.Left(6).Equals("rar://") || strPath.Left(6).Equals("zip://") )
    {
      // get the hostname portion of the url since it contains the archive file
      strPath = checkURL.GetHostName();

      bIsSourceName = false;
      bool bDummy;
      return GetMatchingSource(strPath, VECSOURCES, bDummy);
    }

    CLog::Log(LOGWARNING,"CUtil::GetMatchingSource... no matching source found for [%s]", strPath1.c_str());
  }
  return iIndex;
}

CStdString CUtil::TranslateSpecialSource(const CStdString &strSpecial)
{
  CStdString strReturn=strSpecial;
  if (!strSpecial.IsEmpty() && strSpecial[0] == '$')
  {
    if (strSpecial.Left(5).Equals("$HOME"))
      CUtil::AddFileToFolder("special://home/", strSpecial.Mid(5), strReturn);
    else if (strSpecial.Left(10).Equals("$SUBTITLES"))
      CUtil::AddFileToFolder("special://subtitles/", strSpecial.Mid(10), strReturn);
    else if (strSpecial.Left(9).Equals("$USERDATA"))
      CUtil::AddFileToFolder("special://userdata/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(9).Equals("$DATABASE"))
      CUtil::AddFileToFolder("special://database/", strSpecial.Mid(9), strReturn);
    else if (strSpecial.Left(11).Equals("$THUMBNAILS"))
      CUtil::AddFileToFolder("special://thumbnails/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(11).Equals("$RECORDINGS"))
      CUtil::AddFileToFolder("special://recordings/", strSpecial.Mid(11), strReturn);
    else if (strSpecial.Left(12).Equals("$SCREENSHOTS"))
      CUtil::AddFileToFolder("special://screenshots/", strSpecial.Mid(12), strReturn);
    else if (strSpecial.Left(15).Equals("$MUSICPLAYLISTS"))
      CUtil::AddFileToFolder("special://musicplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(15).Equals("$VIDEOPLAYLISTS"))
      CUtil::AddFileToFolder("special://videoplaylists/", strSpecial.Mid(15), strReturn);
    else if (strSpecial.Left(7).Equals("$CDRIPS"))
      CUtil::AddFileToFolder("special://cdrips/", strSpecial.Mid(7), strReturn);
    // this one will be removed post 2.0
    else if (strSpecial.Left(10).Equals("$PLAYLISTS"))
      CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath",false), strSpecial.Mid(10), strReturn);
  }
  return strReturn;
}

CStdString CUtil::MusicPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "music", strReturn);
  vec.push_back(strReturn);
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return DIRECTORY::CMultiPathDirectory::ConstructMultiPath(vec);;
}

CStdString CUtil::VideoPlaylistsLocation()
{
  vector<CStdString> vec;
  CStdString strReturn;
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "video", strReturn);
  vec.push_back(strReturn);
  CUtil::AddFileToFolder(g_guiSettings.GetString("system.playlistspath"), "mixed", strReturn);
  vec.push_back(strReturn);
  return DIRECTORY::CMultiPathDirectory::ConstructMultiPath(vec);;
}

void CUtil::DeleteMusicDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("mdb");
}

void CUtil::DeleteVideoDatabaseDirectoryCache()
{
  CUtil::DeleteDirectoryCache("vdb");
}

void CUtil::DeleteDirectoryCache(const CStdString strType /* = ""*/)
{
  WIN32_FIND_DATA wfd;
  memset(&wfd, 0, sizeof(wfd));

  CStdString strFile = "special://temp/";
  if (!strType.IsEmpty())
  {
    strFile += strType;
    if (!strFile.Right(1).Equals("-"))
      strFile += "-";
  }
  strFile += "*.fi";
  CAutoPtrFind hFind(FindFirstFile(_P(strFile).c_str(), &wfd));
  if (!hFind.isValid())
    return;
  do
  {
    if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
      CFile::Delete(CUtil::AddFileToFolder("special://temp/", wfd.cFileName));
    }
  while (FindNextFile(hFind, &wfd));
}

bool CUtil::SetSysDateTimeYear(int iYear, int iMonth, int iDay, int iHour, int iMinute)
{
  TIME_ZONE_INFORMATION tziNew;
  SYSTEMTIME CurTime;
  SYSTEMTIME NewTime;
  GetLocalTime(&CurTime);
  GetLocalTime(&NewTime);
  int iRescBiases, iHourUTC;
  int iMinuteNew;

  DWORD dwRet = GetTimeZoneInformation(&tziNew);  // Get TimeZone Informations
  float iGMTZone = (float(tziNew.Bias)/(60));     // Calc's the GMT Time

  CLog::Log(LOGDEBUG, "------------ TimeZone -------------");
  CLog::Log(LOGDEBUG, "-      GMT Zone: GMT %.1f",iGMTZone);
  CLog::Log(LOGDEBUG, "-          Bias: %lu minutes",tziNew.Bias);
  CLog::Log(LOGDEBUG, "-  DaylightBias: %lu",tziNew.DaylightBias);
  CLog::Log(LOGDEBUG, "-  StandardBias: %lu",tziNew.StandardBias);

  switch (dwRet)
  {
    case TIME_ZONE_ID_STANDARD:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 1, Standart");
      }
      break;
    case TIME_ZONE_ID_DAYLIGHT:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias + tziNew.DaylightBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 2, Daylight");
      }
      break;
    case TIME_ZONE_ID_UNKNOWN:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: 0, Unknown");
      }
      break;
    case TIME_ZONE_ID_INVALID:
      {
        iRescBiases   = tziNew.Bias + tziNew.StandardBias;
        CLog::Log(LOGDEBUG, "-   Timezone ID: Invalid");
      }
      break;
    default:
      iRescBiases   = tziNew.Bias + tziNew.StandardBias;
  }
    CLog::Log(LOGDEBUG, "--------------- END ---------------");

  // Calculation
  iHourUTC = GMTZoneCalc(iRescBiases, iHour, iMinute, iMinuteNew);
  iMinute = iMinuteNew;
  if(iHourUTC <0)
  {
    iDay = iDay - 1;
    iHourUTC =iHourUTC + 24;
  }
  if(iHourUTC >23)
  {
    iDay = iDay + 1;
    iHourUTC =iHourUTC - 24;
  }

  // Set the New-,Detected Time Values to System Time!
  NewTime.wYear     = (WORD)iYear;
  NewTime.wMonth    = (WORD)iMonth;
  NewTime.wDay      = (WORD)iDay;
  NewTime.wHour     = (WORD)iHourUTC;
  NewTime.wMinute   = (WORD)iMinute;

  FILETIME stNewTime, stCurTime;
  SystemTimeToFileTime(&NewTime, &stNewTime);
  SystemTimeToFileTime(&CurTime, &stCurTime);
  return false;
}
int CUtil::GMTZoneCalc(int iRescBiases, int iHour, int iMinute, int &iMinuteNew)
{
  int iHourUTC, iTemp;
  iMinuteNew = iMinute;
  iTemp = iRescBiases/60;

  if (iRescBiases == 0 )return iHour;   // GMT Zone 0, no need calculate
  if (iRescBiases > 0)
    iHourUTC = iHour + abs(iTemp);
  else
    iHourUTC = iHour - abs(iTemp);

  if ((iTemp*60) != iRescBiases)
  {
    if (iRescBiases > 0)
      iMinuteNew = iMinute + abs(iTemp*60 - iRescBiases);
    else
      iMinuteNew = iMinute - abs(iTemp*60 - iRescBiases);

    if (iMinuteNew >= 60)
    {
      iMinuteNew = iMinuteNew -60;
      iHourUTC = iHourUTC + 1;
    }
    else if (iMinuteNew < 0)
    {
      iMinuteNew = iMinuteNew +60;
      iHourUTC = iHourUTC - 1;
    }
  }
  return iHourUTC;
}

bool CUtil::AutoDetection()
{
bool bReturn=false;
if (g_guiSettings.GetBool("autodetect.onoff"))
{
    static unsigned int pingTimer = 0;
    if( CTimeUtils::GetTimeMS() - pingTimer < (unsigned int)g_advancedSettings.m_autoDetectPingTime * 1000)
    return false;
    pingTimer = CTimeUtils::GetTimeMS();

  // send ping and request new client info
  if ( CUtil::AutoDetectionPing(
    g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("servers.ftpserveruser"):"anonymous",
    g_guiSettings.GetBool("Autodetect.senduserpw") ? g_guiSettings.GetString("servers.ftpserverpassword"):"anonymous",
    g_guiSettings.GetString("autodetect.nickname"),21 /*Our FTP Port! TODO: Extract FTP from FTP Server settings!*/) )
  {
    CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
    CStdStringArray arSplit;
    // do we have clients in our list ?
    for(unsigned int i=0; i < v_xboxclients.client_ip.size(); i++)
    {
      // extract client informations
      StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
      if ((int)arSplit.size() > 1 && !v_xboxclients.client_informed[i])
      {
        //extract client info and build the ftp link!
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i],strFtpPort.c_str());

        //Do Notification for this Client
        CStdString strtemplbl;
        strtemplbl.Format("%s %s",strNickName, v_xboxclients.client_ip[i]);
        g_application.m_guiDialogKaiToast.QueueNotification(g_localizeStrings.Get(1251), strtemplbl);

        //Debug Log
        CLog::Log(LOGDEBUG,"%s: %s FTP-Link: %s", g_localizeStrings.Get(1251).c_str(), strNickName.c_str(), strFTPPath.c_str());

        //set the client_informed to TRUE, to prevent loop Notification
        v_xboxclients.client_informed[i]=true;

        //YES NO PopUP: ask for connecting to the detected client via Filemanger!
        if (g_guiSettings.GetBool("autodetect.popupinfo") && CGUIDialogYesNo::ShowAndGetInput(1251, 0, 1257, 0))
        {
            g_windowManager.ActivateWindow(WINDOW_FILES, strFTPPath); //Open in MyFiles
        }
        bReturn = true;
      }
    }
  }
}
return bReturn;
}

bool CUtil::AutoDetectionPing(CStdString strFTPUserName, CStdString strFTPPass, CStdString strNickName, int iFTPPort)
{
  bool bFoundNewClient= false;
  CStdString strLocalIP;
  CStdString strSendMessage = "ping\0";
  CStdString strReceiveMessage = "ping";
  int iUDPPort = 4905;
  char sztmp[512];

  static int udp_server_socket, inited=0;
#ifndef _LINUX
  int cliLen;
#else
  socklen_t cliLen;
#endif
  int t1,t2,t3,t4, life=0;

  struct sockaddr_in server;
  struct sockaddr_in cliAddr;
  struct timeval timeout={0,500};
  fd_set readfds;
    char hostname[255];
#ifndef _LINUX
    WORD wVer;
    WSADATA wData;
    PHOSTENT hostinfo;
    wVer = MAKEWORD( 2, 2 );
    if (WSAStartup(wVer,&wData) == 0)
    {
#else
    struct hostent * hostinfo;
#endif
      if(gethostname(hostname,sizeof(hostname)) == 0)
      {
        if((hostinfo = gethostbyname(hostname)) != NULL)
        {
          strLocalIP = inet_ntoa (*(struct in_addr *)*hostinfo->h_addr_list);
          strNickName.Format("%s",hostname);
        }
      }
#ifndef _LINUX
      WSACleanup();
    }
#endif
  // get IP address
  sscanf( (char *)strLocalIP.c_str(), "%d.%d.%d.%d", &t1, &t2, &t3, &t4 );
  if( !t1 ) return false;
  cliLen = sizeof( cliAddr);
  // setup UDP socket
  if( !inited )
  {
      
    int tUDPsocket  = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP);
    char value      = 1;
    setsockopt( tUDPsocket, SOL_SOCKET, SO_BROADCAST, &value, value );
    struct sockaddr_in addr;
    
    memset(&(addr),0,sizeof(addr));
    addr.sin_family       = AF_INET;
    addr.sin_addr.s_addr  = INADDR_ANY;
    addr.sin_port         = htons(iUDPPort);
    
    ::bind(tUDPsocket,(struct sockaddr *)(&addr),sizeof(addr));
    udp_server_socket = tUDPsocket;
    inited = 1;
   
  }
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );
  if (life == SOCKET_ERROR )
    return false;
  memset(&(server),0,sizeof(server));
  server.sin_family = AF_INET;
#ifndef _LINUX
  server.sin_addr.S_un.S_addr = INADDR_BROADCAST;
#else
  server.sin_addr.s_addr = INADDR_BROADCAST;
#endif
  server.sin_port = htons(iUDPPort);
  sendto(udp_server_socket,(char *)strSendMessage.c_str(),5,0,(struct sockaddr *)(&server),sizeof(server));
  FD_ZERO(&readfds);
  FD_SET(udp_server_socket, &readfds);
  life = select( 0,&readfds, NULL, NULL, &timeout );

  unsigned int iLookUpCountMax = 2;
  unsigned int i=0;
  bool bUpdateShares=false;

  // Ping able clients? 0:false
  if (life == 0 )
  {
    if(v_xboxclients.client_ip.size() > 0)
    {
      // clients in list without life signal!
      // calculate iLookUpCountMax value counter dependence on clients size!
      if(v_xboxclients.client_ip.size() > iLookUpCountMax)
        iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

      for (i=0; i<v_xboxclients.client_ip.size(); i++)
      {
        bUpdateShares=false;
        //only 1 client, clear our list
        if(v_xboxclients.client_lookup_count[i] >= iLookUpCountMax && v_xboxclients.client_ip.size() == 1 )
        {
          v_xboxclients.client_ip.clear();
          v_xboxclients.client_info.clear();
          v_xboxclients.client_lookup_count.clear();
          v_xboxclients.client_informed.clear();

          // debug log, clients removed from our list
          CLog::Log(LOGDEBUG,"Autodetection: all Clients Removed! (mode LIFE 0)");
          bUpdateShares = true;
        }
        else
        {
          // check client lookup counter! Not reached the CountMax, Add +1!
          if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
            v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
          else
          {
            // client lookup counter REACHED CountMax, remove this client
            v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
            v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
            v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
            v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

            // debug log, clients removed from our list
            CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 0)",i );
            bUpdateShares = true;
          }
        }
        if(bUpdateShares)
        {
          // a client is removed from our list, update our shares
          CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
          g_windowManager.SendThreadMessage(msg);
        }
      }
    }
  }
  // life !=0 we are online and ready to receive and send
  while( life )
  {
    bFoundNewClient = false;
    bUpdateShares = false;
    // Receive ping request or Info
    int iSockRet = recvfrom(udp_server_socket, sztmp, 512, 0,(struct sockaddr *) &cliAddr, &cliLen);
    if (iSockRet != SOCKET_ERROR)
    {
      CStdString strTmp;
      // do we received a new Client info or just a "ping" request
      if(strReceiveMessage.Equals(sztmp))
      {
        // we received a "ping" request, sending our informations
        strTmp.Format("%s;%s;%s;%d;%d\r\n\0",
          strNickName.c_str(),  // Our Nick-, Device Name!
          strFTPUserName.c_str(), // User Name for our FTP Server
          strFTPPass.c_str(), // Password for our FTP Server
          iFTPPort, // FTP PORT Adress for our FTP Server
          0 ); // BOOSMODE, for our FTP Server!
        sendto(udp_server_socket,(char *)strTmp.c_str(),strlen((char *)strTmp.c_str())+1,0,(struct sockaddr *)(&cliAddr),sizeof(cliAddr));
      }
      else
      {
        //We received new client information, extracting information
        CStdString strInfo, strIP;
        strInfo.Format("%s",sztmp); //this is the client info
        strIP.Format("%d.%d.%d.%d",
#ifndef _LINUX
          cliAddr.sin_addr.S_un.S_un_b.s_b1,
          cliAddr.sin_addr.S_un.S_un_b.s_b2,
          cliAddr.sin_addr.S_un.S_un_b.s_b3,
          cliAddr.sin_addr.S_un.S_un_b.s_b4
#else
          (int)((char *)(cliAddr.sin_addr.s_addr))[0],
          (int)((char *)(cliAddr.sin_addr.s_addr))[1],
          (int)((char *)(cliAddr.sin_addr.s_addr))[2],
          (int)((char *)(cliAddr.sin_addr.s_addr))[3]
#endif
        ); //this is the client IP

        //Is this our Local IP ?
        if ( !strIP.Equals(strLocalIP) )
        {
          //is our list empty?
          if(v_xboxclients.client_ip.size() <= 0 )
          {
            // the list is empty, add. this client to the list!
            v_xboxclients.client_ip.push_back(strIP);
            v_xboxclients.client_info.push_back(strInfo);
            v_xboxclients.client_lookup_count.push_back(0);
            v_xboxclients.client_informed.push_back(false);
            bFoundNewClient = true;
            bUpdateShares = true;
          }
          // our list is not empty, check if we allready have this client in our list!
          else
          {
            // this should be a new client or?
            // check list
            bFoundNewClient = true;
            for (i=0; i<v_xboxclients.client_ip.size(); i++)
            {
              if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                bFoundNewClient=false;
            }
            if(bFoundNewClient)
            {
              // bFoundNewClient is still true, the client is not in our list!
              // add. this client to our list!
              v_xboxclients.client_ip.push_back(strIP);
              v_xboxclients.client_info.push_back(strInfo);
              v_xboxclients.client_lookup_count.push_back(0);
              v_xboxclients.client_informed.push_back(false);
              bUpdateShares = true;
            }
            else // this is a existing client! check for LIFE & lookup counter
            {
              // calculate iLookUpCountMax value counter dependence on clients size!
              if(v_xboxclients.client_ip.size() > iLookUpCountMax)
                iLookUpCountMax += (v_xboxclients.client_ip.size()-iLookUpCountMax);

              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                if(strIP.Equals(v_xboxclients.client_ip[i].c_str()))
                {
                  // found client in list, reset looup_Count and the client_info
                  v_xboxclients.client_info[i]=strInfo;
                  v_xboxclients.client_lookup_count[i] = 0;
                }
                else
                {
                  // check client lookup counter! Not reached the CountMax, Add +1!
                  if(v_xboxclients.client_lookup_count[i] < iLookUpCountMax )
                    v_xboxclients.client_lookup_count[i] = v_xboxclients.client_lookup_count[i]+1;
                  else
                  {
                    // client lookup counter REACHED CountMax, remove this client
                    v_xboxclients.client_ip.erase(v_xboxclients.client_ip.begin()+i);
                    v_xboxclients.client_info.erase(v_xboxclients.client_info.begin()+i);
                    v_xboxclients.client_lookup_count.erase(v_xboxclients.client_lookup_count.begin()+i);
                    v_xboxclients.client_informed.erase(v_xboxclients.client_informed.begin()+i);

                    // debug log, clients removed from our list
                    CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] Removed! (mode LIFE 1)",i );

                    // client is removed from our list, update our shares
                    CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
                    g_windowManager.SendThreadMessage(msg);
                  }
                }
              }
              // here comes our list for debug log
              for (i=0; i<v_xboxclients.client_ip.size(); i++)
              {
                CLog::Log(LOGDEBUG,"Autodetection: Client ID:[%i] (mode LIFE=1)",i );
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
                CLog::Log(LOGDEBUG,"IP:%s Info:%s LookUpCount:%i Informed:%s",
                  v_xboxclients.client_ip[i].c_str(),
                  v_xboxclients.client_info[i].c_str(),
                  v_xboxclients.client_lookup_count[i],
                  v_xboxclients.client_informed[i] ? "true":"false");
                CLog::Log(LOGDEBUG,"----------------------------------------------------------------" );
              }
            }
          }
          if(bUpdateShares)
          {
            // a client is add or removed from our list, update our shares
            CGUIMessage msg(GUI_MSG_NOTIFY_ALL,0,0,GUI_MSG_UPDATE_SOURCES);
            g_windowManager.SendThreadMessage(msg);
          }
        }
      }
    }
    else
    {
       CLog::Log(LOGDEBUG, "Autodetection: Socket error %u", WSAGetLastError());
    }
    timeout.tv_sec=0;
    timeout.tv_usec = 5000;
    FD_ZERO(&readfds);
    FD_SET(udp_server_socket, &readfds);
    life = select( 0,&readfds, NULL, NULL, &timeout );
  }
  return bFoundNewClient;
}

void CUtil::AutoDetectionGetSource(VECSOURCES &shares)
{
  if(v_xboxclients.client_ip.size() > 0)
  {
    // client list is not empty, add to shares
    CMediaSource share;
    for (unsigned int i=0; i< v_xboxclients.client_ip.size(); i++)
    {
      //extract client info string: NickName;FTP_USER;FTP_Password;FTP_PORT;BOOST_MODE
      CStdString strFTPPath, strNickName, strFtpUserName, strFtpPassword, strFtpPort, strBoosMode;
      CStdStringArray arSplit;
      StringUtils::SplitString(v_xboxclients.client_info[i],";", arSplit);
      if ((int)arSplit.size() > 1)
      {
        strNickName     = arSplit[0].c_str();
        strFtpUserName  = arSplit[1].c_str();
        strFtpPassword  = arSplit[2].c_str();
        strFtpPort      = arSplit[3].c_str();
        strBoosMode     = arSplit[4].c_str();
        strFTPPath.Format("ftp://%s:%s@%s:%s/",strFtpUserName.c_str(),strFtpPassword.c_str(),v_xboxclients.client_ip[i].c_str(),strFtpPort.c_str());

        strNickName.TrimRight(' ');
        share.strName.Format("FTP XBMC_PC (%s)", strNickName.c_str());
        share.strPath.Format("%s",strFTPPath.c_str());
        shares.push_back(share);
      }
    }
  }
}
bool CUtil::IsFTP(const CStdString& strFile)
{
  if( strFile.Left(6).Equals("ftp://") ) return true;
  else if( strFile.Left(7).Equals("ftpx://") ) return true;
  else if( strFile.Left(7).Equals("ftps://") ) return true;
  else return false;
}

bool CUtil::GetFTPServerUserName(int iFTPUserID, CStdString &strFtpUser1, int &iUserMax )
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  class CXFUser* m_pUser;
  vector<CXFUser*> users;
  g_application.m_pFileZilla->GetAllUsers(users);
  iUserMax = users.size();
  if (iUserMax > 0)
  {
    //for (int i = 1 ; i < iUserSize; i++){ delete users[i]; }
    m_pUser = users[iFTPUserID];
    strFtpUser1 = m_pUser->GetName();
    if (strFtpUser1.size() != 0) return true;
    else return false;
  }
  else
#endif
    return false;
}
bool CUtil::SetFTPServerUserPassword(CStdString strFtpUserName, CStdString strFtpUserPassword)
{
#ifdef HAS_FTP_SERVER
  if( !g_application.m_pFileZilla )
    return false;

  CStdString strTempUserName;
  class CXFUser* p_ftpUser;
  vector<CXFUser*> v_ftpusers;
  bool bFoundUser = false;
  g_application.m_pFileZilla->GetAllUsers(v_ftpusers);
  int iUserSize = v_ftpusers.size();
  if (iUserSize > 0)
  {
    int i = 1 ;
    while( i <= iUserSize)
    {
      p_ftpUser = v_ftpusers[i-1];
      strTempUserName = p_ftpUser->GetName();
      if (strTempUserName.Equals(strFtpUserName.c_str()) )
      {
        if (p_ftpUser->SetPassword(strFtpUserPassword.c_str()) != XFS_INVALID_PARAMETERS)
        {
          p_ftpUser->CommitChanges();
          g_guiSettings.SetString("servers.ftpserverpassword",strFtpUserPassword.c_str());
          return true;
        }
        break;
      }
      i++;
    }
  }
#endif
  return false;
}

void CUtil::GetRecursiveListing(const CStdString& strPath, CFileItemList& items, const CStdString& strMask, bool bUseFileDirectories)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,strMask,bUseFileDirectories);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder)
      CUtil::GetRecursiveListing(myItems[i]->m_strPath,items,strMask,bUseFileDirectories);
    else if (!myItems[i]->IsRAR() && !myItems[i]->IsZIP())
      items.Add(myItems[i]);
  }
}

void CUtil::GetRecursiveDirsListing(const CStdString& strPath, CFileItemList& item)
{
  CFileItemList myItems;
  CDirectory::GetDirectory(strPath,myItems,"",false);
  for (int i=0;i<myItems.Size();++i)
  {
    if (myItems[i]->m_bIsFolder && !myItems[i]->m_strPath.Equals(".."))
    {
      item.Add(myItems[i]);
      CUtil::GetRecursiveDirsListing(myItems[i]->m_strPath,item);
    }
  }
}

void CUtil::ForceForwardSlashes(CStdString& strPath)
{
  int iPos = strPath.ReverseFind('\\');
  while (iPos > 0)
  {
    strPath.at(iPos) = '/';
    iPos = strPath.ReverseFind('\\');
  }
}

double CUtil::AlbumRelevance(const CStdString& strAlbumTemp1, const CStdString& strAlbum1, const CStdString& strArtistTemp1, const CStdString& strArtist1)
{
  // case-insensitive fuzzy string comparison on the album and artist for relevance
  // weighting is identical, both album and artist are 50% of the total relevance
  // a missing artist means the maximum relevance can only be 0.50
  CStdString strAlbumTemp = strAlbumTemp1;
  strAlbumTemp.MakeLower();
  CStdString strAlbum = strAlbum1;
  strAlbum.MakeLower();
  double fAlbumPercentage = fstrcmp(strAlbumTemp, strAlbum, 0.0f);
  double fArtistPercentage = 0.0f;
  if (!strArtist1.IsEmpty())
  {
    CStdString strArtistTemp = strArtistTemp1;
    strArtistTemp.MakeLower();
    CStdString strArtist = strArtist1;
    strArtist.MakeLower();
    fArtistPercentage = fstrcmp(strArtistTemp, strArtist, 0.0f);
  }
  double fRelevance = fAlbumPercentage * 0.5f + fArtistPercentage * 0.5f;
  return fRelevance;
}

CStdString CUtil::SubstitutePath(const CStdString& strFileName)
{
  //CLog::Log(LOGDEBUG,"%s checking source filename:[%s]", __FUNCTION__, strFileName.c_str());
  // substitutes paths to correct issues with remote playlists containing full paths
  for (unsigned int i = 0; i < g_advancedSettings.m_pathSubstitutions.size(); i++)
  {
    vector<CStdString> vecSplit;
    StringUtils::SplitString(g_advancedSettings.m_pathSubstitutions[i], " , ", vecSplit);

    // something is wrong, go to next substitution
    if (vecSplit.size() != 2)
      continue;

    CStdString strSearch = vecSplit[0];
    CStdString strReplace = vecSplit[1];
    strSearch.Replace(",,",",");
    strReplace.Replace(",,",",");

      CUtil::AddSlashAtEnd(strSearch);
      CUtil::AddSlashAtEnd(strReplace);

    // if left most characters match the search, replace them
    //CLog::Log(LOGDEBUG,"%s testing for path:[%s]", __FUNCTION__, strSearch.c_str());
    int iLen = strSearch.size();
    if (strFileName.Left(iLen).Equals(strSearch))
    {
      // fix slashes
      CStdString strTemp = strFileName.Mid(iLen);
      strTemp.Replace("\\", strReplace.Right(1));
      CStdString strFileNameNew = strReplace + strTemp;
      //CLog::Log(LOGDEBUG,"%s new filename:[%s]", __FUNCTION__, strFileNameNew.c_str());
      return strFileNameNew;
    }
  }
  // nothing matches, return original string
  //CLog::Log(LOGDEBUG,"%s no matches", __FUNCTION__);
  return strFileName;
}

bool CUtil::MakeShortenPath(CStdString StrInput, CStdString& StrOutput, int iTextMaxLength)
{
  int iStrInputSize = StrInput.size();
  if((iStrInputSize <= 0) || (iTextMaxLength >= iStrInputSize))
    return false;

  char cDelim = '\0';
  size_t nFirstDelim, nGreaterDelim, nPos;

  nPos = StrInput.find_last_of( '\\' );
  if ( nPos != CStdString::npos )
    cDelim = '\\';
  else
  {
    nPos = StrInput.find_last_of( '/' );
    if ( nPos != CStdString::npos )
      cDelim = '/';
  }
  if ( cDelim == '\0' )
    return false;

  if (nPos == StrInput.size() - 1)
  {
    StrInput.erase(StrInput.size() - 1);
    nPos = StrInput.find_last_of( cDelim );
  }

  // Get the FIRST pos of cDelim
  nFirstDelim = StrInput.find_first_of(cDelim);
  
  while( iTextMaxLength < iStrInputSize )
  {
    // Get the LAST pos of cDelim
    nPos = StrInput.find_last_of( cDelim, nPos );
    nGreaterDelim = nPos;
    
    if ( nPos != CStdString::npos )
    {
      // Get the BEFORE LAST pos of cDelim
      nPos = StrInput.find_last_of( cDelim, nPos - 1 );
    }

    if ( nPos == nFirstDelim )
    {
      // In case BEFORE LAST pos is FIRST pos -> Break
      break;
    }
    else if ( nPos == CStdString::npos )
    {
      // In case BEFORE LAST pos is npos (there are no cDelim before LAST pos) -> Break
      break;
    }
    
    if ( nGreaterDelim > nPos )
    {
      StrInput.replace( nPos + 1, nGreaterDelim - nPos - 1, ".." );
    }
    
    iStrInputSize = StrInput.size();
  }
  // replace any additional /../../ with just /../ if necessary
  CStdString replaceDots;
  replaceDots.Format("..%c..", cDelim);
  while (StrInput.size() > (unsigned int)iTextMaxLength)
    if (!StrInput.Replace(replaceDots, ".."))
      break;
  // finally, truncate our string to force inside our max text length,
  // replacing the last 2 characters with ".."

  // eg end up with:
  // "smb://../Playboy Swimsuit Cal.."
  if (iTextMaxLength > 2 && StrInput.size() > (unsigned int)iTextMaxLength)
  {
    StrInput = StrInput.Left(iTextMaxLength - 2);
    StrInput += "..";
  }
  StrOutput = StrInput;
  return true;
}

bool CUtil::SupportsFileOperations(const CStdString& strPath)
{
  // currently only hd and smb support delete and rename
  if (IsHD(strPath))
    return true;
  if (IsSmb(strPath))
    return true;
#ifdef HAS_FILESYSTEM_MYTH
  if (IsMythTV(strPath))
  {
    /*
     * Can't use CFile::Exists() to check whether the myth:// path supports file operations because
     * it hits the directory cache on the way through, which has the Live Channels and Guide
     * items cached.
     */
    return CCMythDirectory::SupportsFileOperations(strPath);
  }
#endif
  if (IsStack(strPath))
  {
    CStackDirectory dir;
    return SupportsFileOperations(dir.GetFirstStackedFile(strPath));
  }
  if (IsMultiPath(strPath))
    return CMultiPathDirectory::SupportsFileOperations(strPath);

  return false;
}

CStdString CUtil::GetCachedAlbumThumb(const CStdString& album, const CStdString& artist)
{
  // do not create thumb for empty or unknown album
  if (album.IsEmpty() || (album.CompareNoCase("Unknown Album") == 0) || artist.IsEmpty() || (artist.CompareNoCase("Unknown Artist") == 0))
    return "";
  
  return GetCachedMusicThumb(album+artist);
}

CStdString CUtil::GetCachedMusicThumb(const CStdString& path)
{
  Crc32 crc;
  CStdString noSlashPath(path);
  RemoveSlashAtEnd(noSlashPath);
  crc.ComputeFromLowerCase(noSlashPath);
  CStdString hex;
  hex.Format("%08x", (unsigned __int32) crc);
  CStdString thumb;
  thumb.Format("%c/%s.tbn", hex[0], hex.c_str());
  return CUtil::AddFileToFolder(g_settings.GetMusicThumbFolder(), thumb);
}

CStdString CUtil::GetDefaultFolderThumb(const CStdString &folderThumb)
{
  if (g_TextureManager.HasTexture(folderThumb))
    return folderThumb;
  return "";
}

void CUtil::GetSkinThemes(vector<CStdString>& vecTheme)
{
  CStdString strPath;
  CUtil::AddFileToFolder(g_graphicsContext.GetMediaDir(),"media",strPath);
  CFileItemList items;
  CDirectory::GetDirectory(strPath, items);
  // Search for Themes in the Current skin!
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr pItem = items[i];
    if (!pItem->m_bIsFolder)
    {
      CStdString strExtension;
      CUtil::GetExtension(pItem->m_strPath, strExtension);
      if (strExtension == ".xpr" && pItem->GetLabel().CompareNoCase("Textures.xpr"))
      {
        CStdString strLabel = pItem->GetLabel();
        vecTheme.push_back(strLabel.Mid(0, strLabel.size() - 4));
      }
    }
  }
  sort(vecTheme.begin(), vecTheme.end(), sortstringbyname());
}

void CUtil::WipeDir(const CStdString& strPath) // DANGEROUS!!!!
{
  if (!CDirectory::Exists(strPath)) return;

  CFileItemList items;
  CUtil::GetRecursiveListing(strPath,items,"");
  for (int i=0;i<items.Size();++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->m_strPath);
  }
  items.Clear();
  CUtil::GetRecursiveDirsListing(strPath,items);
  for (int i=items.Size()-1;i>-1;--i) // need to wipe them backwards
  {
    CUtil::AddSlashAtEnd(items[i]->m_strPath);
    CDirectory::Remove(items[i]->m_strPath);
  }

  CStdString tmpPath = strPath;
  AddSlashAtEnd(tmpPath);
  CDirectory::Remove(tmpPath);
}

void CUtil::CopyDirRecursive(const CStdString& strSrcPath, const CStdString& strDstPath)
{
  if (!CDirectory::Exists(strSrcPath)) return;

  // create root first
  CStdString destPath;

  destPath = strDstPath;
  AddSlashAtEnd(destPath);
  CDirectory::Create(destPath);

  CFileItemList items;
  CUtil::GetRecursiveDirsListing(strSrcPath,items);
  for (int i=0;i<items.Size();++i)
  {
    destPath = items[i]->m_strPath;
    destPath.Replace(strSrcPath,"");
    destPath = CUtil::AddFileToFolder(strDstPath, destPath);
    CDirectory::Create(destPath);
  }
  items.Clear();
  CUtil::GetRecursiveListing(strSrcPath,items,"");
  for (int i=0;i<items.Size();i++)
  {
    destPath = items[i]->m_strPath;
    destPath.Replace(strSrcPath,"");
    destPath = CUtil::AddFileToFolder(strDstPath, destPath);
    CFile::Cache(items[i]->m_strPath, destPath);
  }
}

void CUtil::ClearFileItemCache()
{
  CFileItemList items;
  CDirectory::GetDirectory("special://temp/", items, ".fi", false);
  for (int i = 0; i < items.Size(); ++i)
  {
    if (!items[i]->m_bIsFolder)
      CFile::Delete(items[i]->m_strPath);
  }
}

void CUtil::InitRandomSeed()
{
  // Init random seed 
  int64_t now; 
  now = CurrentHostCounter(); 
  unsigned int seed = now;
//  CLog::Log(LOGDEBUG, "%s - Initializing random seed with %u", __FUNCTION__, seed);
  srand(seed);
}

#ifdef _LINUX
bool CUtil::RunCommandLine(const CStdString& cmdLine, bool waitExit)
{
  CStdStringArray args;

  StringUtils::SplitString(cmdLine, ",", args);

  // Strip quotes and whitespace around the arguments, or exec will fail.
  // This allows the python invocation to be written more naturally with any amount of whitespace around the args.
  // But it's still limited, for example quotes inside the strings are not expanded, etc.
  // TODO: Maybe some python library routine can parse this more properly ?
  for (size_t i=0; i<args.size(); i++)
  {
    CStdString &s = args[i];
    CStdString stripd = s.Trim();
    if (stripd[0] == '"' || stripd[0] == '\'')
    {
      s = s.TrimLeft();
      s = s.Right(s.size() - 1);
    }
    if (stripd[stripd.size() - 1] == '"' || stripd[stripd.size() - 1] == '\'')
    {
      s = s.TrimRight();
      s = s.Left(s.size() - 1);
    }
  }

  return Command(args, waitExit);
}

//
// FIXME, this should be merged with the function below.
//
bool CUtil::Command(const CStdStringArray& arrArgs, bool waitExit)
{
#ifdef _DEBUG
  printf("Executing: ");
  for (size_t i=0; i<arrArgs.size(); i++)
    printf("%s ", arrArgs[i].c_str());
  printf("\n");
#endif

  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0);
    close(1);
    close(2);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      for (size_t i=0; i<arrArgs.size(); i++)
        args[i] = (char *)arrArgs[i].c_str();
      execvp(args[0], args);
    }
  }
  else
  {
    if (waitExit) waitpid(child, &n, 0);
  }

  return (waitExit) ? (WEXITSTATUS(n) == 0) : true;
}

bool CUtil::SudoCommand(const CStdString &strCommand)
{
  CLog::Log(LOGDEBUG, "Executing sudo command: <%s>", strCommand.c_str());
  pid_t child = fork();
  int n = 0;
  if (child == 0)
  {
    close(0); // close stdin to avoid sudo request password
    close(1);
    close(2);
    CStdStringArray arrArgs;
    StringUtils::SplitString(strCommand, " ", arrArgs);
    if (arrArgs.size() > 0)
    {
      char **args = (char **)alloca(sizeof(char *) * (arrArgs.size() + 3));
      memset(args, 0, (sizeof(char *) * (arrArgs.size() + 3)));
      args[0] = (char *)"/usr/bin/sudo";
      args[1] = (char *)"-S";
      for (size_t i=0; i<arrArgs.size(); i++)
      {
        args[i+2] = (char *)arrArgs[i].c_str();
      }
      execvp("/usr/bin/sudo", args);
    }
  }
  else
    waitpid(child, &n, 0);

  return WEXITSTATUS(n) == 0;
}
#endif

bool CUtil::CreatePictureFolderThumb(CFileItem* pictureFolderItem)
{
  if(pictureFolderItem->m_bIsFolder == false)
	{
	  CLog::Log(LOGERROR,"Failed to CreatePictureFolderThumb because the item received is not a folder [%s]", (pictureFolderItem->m_strPath).c_str());

		return false;
	}

  CFileItemList items;
  CDirectory::GetDirectory(pictureFolderItem->m_strPath, items, g_stSettings.m_pictureExtensions, false, false);

  // count the number of images in the directory
  for (int i=0; i < items.Size();)
  {
    if (!items[i]->IsPicture() || items[i]->IsZIP() || items[i]->IsRAR() || items[i]->IsPlayList())
    {
      items.Remove(i);
    }
    else
		{
      i++;
		}
  }

  if(items.Size() == 0)
	{
	  CLog::Log(LOGERROR,"Cannot CreatePictureFolderThumb because the item contain [%d] pictures", items.Size());

		return false;
	}

  // randomize them
  items.Randomize();

  CStdString folderThumb;

  if (items.Size() < 4)
  {
	  // less than 4 images, so just grab a single random thumb
    folderThumb = pictureFolderItem->GetCachedPictureThumb();

		CLog::Log(LOGDEBUG,"Folder contain [%d] pictures and the cache returned [%s] for thumb. Going to create FolderThumb (th)",items.Size(),folderThumb.c_str());

    CPicture::CreateThumbnail(items[0]->m_strPath, folderThumb, true);
  }
  else
  {
    // ok, now we've got the files to get the thumbs from, lets create it...
    // we basically load the 4 thumbs, resample to 62x62 pixels, and add them
    CStdString strFiles[4];
    for (int thumb = 0; thumb < 4; thumb++)
		{
      strFiles[thumb] = items[thumb]->m_strPath;
		}

		folderThumb = pictureFolderItem->GetCachedPictureThumb();

		CLog::Log(LOGDEBUG,"Folder contain [%d] pictures and the cache returned [%s] for thumb. Going to create FolderThumb (th)",items.Size(),folderThumb.c_str());
    CPicture::CreateFolderThumb(strFiles, folderThumb);
  }

  return true;
}

int CUtil::VersionCompare(const CStdString version1, const CStdString version2)
{
  vector<CStdString> tokens1;
  CUtil::Tokenize(version1, tokens1, ".");

  vector<CStdString> tokens2;
  CUtil::Tokenize(version2, tokens2, ".");
  
  for (size_t i = 0; i < min(tokens1.size(), tokens2.size()); i++)
  {
    int v1 = atoi(tokens1[i].c_str());
    int v2 = atoi(tokens2[i].c_str());
    
    if (v1 < v2) return -1;
    else if (v1 > v2) return 1;
  }
  
  if (tokens1.size() != tokens2.size())
  {
    // Match point
    if (tokens1.size() < tokens2.size()) return -1;
    else if (tokens1.size() > tokens2.size()) return 1;
  }
  
  return 0;
}

/**
 * Runs the job in the background, always waits in for the job to finish
 */
Job_Result CUtil::RunInBG(IRunnable* pJob, bool deleteJob)
{
  Job_Result jobResult = JOB_FAILED;
  DWORD locks = ExitCriticalSection(g_graphicsContext);

  CStdString strId;
  strId.Format("RunInBG-%p",pJob);

  CGUIDialogProgress* progress = NULL;
  if (!pJob->m_IsSilent)
  {
    progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    progress->SetCanCancel(pJob->m_allowAbort);
    progress->StartModal(strId);
    progress->Progress();
  }

  CThread *thread = new CThread(pJob);
  thread->Create();
  
  while (!thread->WaitForThreadExit(50))
  {
    if (progress)
    {
      if (progress->IsCanceled() || !progress->IsDialogRunning())
      {
        pJob->m_IsCanceled = true;
        jobResult = JOB_ABORTED;

        //CLog::Log(LOGDEBUG,"CUtil::RunInBG, Got Cancelled (utils)");
        thread->Detach();
        progress->Close();
        RestoreCriticalSection(g_graphicsContext, locks);

        return jobResult;
      }

      progress->Progress();
    }
  }

  pJob->m_IsCanceled = false;
  RestoreCriticalSection(g_graphicsContext, locks);

  if (progress)
    progress->EndProgressOperation(strId);

  delete thread;
  
  bool bResult = pJob->m_bJobResult;

  if (bResult)
  {
    jobResult = JOB_SUCCEEDED;
  }

  if (!bResult || deleteJob)
  {
    // delete job if going to return FALSE OR specifically asked to
    delete pJob;
    pJob = NULL;
  }

  return jobResult;
}

bool CUtil::IsFrontMost()
{
#ifdef __APPLE__
  ProcessSerialNumber curr, front;
  Boolean same = true;
  ::GetCurrentProcess(&curr);
  ::GetFrontProcess(&front);
  ::SameProcess(&curr, &front, &same);
  return same;
#endif
  
  return true;
}

CStdString CUtil::MD5File(const CStdString& _strFilePath)
{
  CStdString strFilePath = _strFilePath;
  
  if(CUtil::IsSpecial(strFilePath))
  {
    strFilePath = _P(strFilePath);
  }

  XBMC::MD5 md5;
  CStdString res;
  
  unsigned char digest[16];
  CFile f;
  if (f.Open(strFilePath, true))
  {
    char chunk[4096];
    int nBytes = 0;
    while ( (nBytes = f.Read(chunk, 4096)) > 0 )
      md5.append((unsigned char*)chunk, nBytes);
    f.Close();
    md5.getDigest(digest);
    for (int i=0;i<16;i++)
    {
      CStdString strHex;
      strHex.Format("%02x", digest[i]);
      res += strHex;
    }
  }
  
  return res;
}

bool CUtil::IsEmbedded()
{
#ifdef HAS_EMBEDDED
  return true;
#else
  return false;
#endif
}

CStdString CUtil::GetPlatform()
{
  CStdString platform;

#if defined(DEVICE_PLATFORM)
  platform = DEVICE_PLATFORM;
#elif defined(_LINUX) && !defined(__APPLE__)
  platform = "linux";
#elif defined(__APPLE__)
  if(CSysInfo::IsAppleTV())
  {
    platform = "atv";
  }
  else
  {
    platform = "mac";
  }
#elif defined(_WIN32)
  platform = "win";
#endif
  
  return platform;
}

CStdString CUtil::GetSpecialPathPrefix(const CStdString& specialPath)
{
  CStdString specialPathPrefix = "";
  
  if(CUtil::IsSpecial(specialPath))
  {
    if(specialPath.length()>10)
    {
      ///////////////////////////
      // Case of special://xxx //
      ///////////////////////////
      
      int pos = specialPath.Find('/',10);
      
      if(pos != (-1))
      {
        specialPathPrefix = specialPath.substr(0,pos);
      }
      else
      {
        specialPathPrefix = specialPath;
      }
    }
    else
    {
      specialPathPrefix = specialPath;      
    }
  }
  
  return specialPathPrefix;
}

bool CUtil::IsCountryAllowed(const CStdString& _countries, bool allow)
{  
  if (!g_guiSettings.GetBool("filelists.filtergeoip2"))
  {
    return true;
  }
  
  const CStdString& country = g_application.GetCountryCode();
  if (country.IsEmpty())
  {
    return true;
  }
   
  CStdString countries = _countries;
  countries.ToLower();

  // From spec:  If the "allow" element is empty and the type is relationship is "allow", 
  // it is assumed that the empty list means "allow nobody" and the media should not be syndicated.
  if (countries.size() == 0 || countries == "none") return !allow;  
  if (countries == "all") return allow;

  // If the country is the first in the list of countries..  
  if (countries[0] == country[0] && countries[1] == country[1]) return allow;
  
  // Prepare a search term
  CStdString searchTerm = " ";
  searchTerm += country;
  
  bool result = (countries.Find(searchTerm) > 0);
  if (!allow)
    result = !result;
  
  return result;
}

bool CUtil::IsAdultAllowed()
{
  CProfile& currentUserProfile = g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex];
  
  return !currentUserProfile.adultLocked();
}

void CUtil::FilterUnallowedItems(std::vector<CGUIListItemPtr>& items)
{
  try
  {
    for (std::vector< CGUIListItemPtr >::iterator it = items.begin(); it != items.end(); /* BLANK */)
    {
      CFileItem* fileItem = dynamic_cast<CFileItem*> ((*it).get());
      if (fileItem && !fileItem->IsAllowed())
      {
         it = items.erase(it);
      } 
      else 
      {
         ++it;
      }
    }
  }
  catch (const std::bad_cast& e)
  {
    // Failed cast...nothing to do much here
  }
}

#define HIDDEN_PASSWORD "//******@"

bool CUtil::IsPasswordHidden(CStdString& path)
{
  return (path.Find(HIDDEN_PASSWORD) > 0);
}

bool CUtil::RemovePasswordFromPath(CStdString &path,bool completelyRemove)
{
  bool passwordWasRemoved = false;

  CRegExp regPass;
  CRegExp regPath;

  if (regPath.RegComp("(//.*?/.*?@)"))
  {
    if (regPath.RegFind(path.c_str()) >= 0)
      return passwordWasRemoved;
  }

  if (regPass.RegComp("(//.*?@)"))
  {
    if (regPass.RegFind(path.c_str()) >= 0)
    {
      char *rep = regPass.GetReplaceString("\\1");
      if (rep)
      {
        if(completelyRemove)
          path.Replace(rep,"//");
        else
          path.Replace(rep, HIDDEN_PASSWORD);
        passwordWasRemoved = true;
        free(rep);
      }
    }
  }

  return passwordWasRemoved;
}

void CUtil::CreateTempDirectory(const CStdString &origPath)
{
#ifdef _WIN32
  CDirectory::Create(origPath);
#else
  CStdString p = origPath;
  
  p.Replace("special://masterprofile/profiles/","");
  p.Replace("special://masterprofile/","");
  p.Replace("special://home/profiles/","");
  p.Replace("special://home/","");
  if (g_settings.m_iLastLoadedProfileIndex < (int) g_settings.m_vecProfiles.size() && g_settings.m_iLastLoadedProfileIndex > 0) 
    p.Replace("special://profile", g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].getID());
  else
    p.Replace("special://profile", "");
  
  CStdString folder = "/tmp/profile/" + p;
  CDirectory::Create(folder);
  
  symlink(folder.c_str(), CSpecialProtocol::TranslatePath(origPath).c_str());
#endif
}

bool CUtil::MatchesPlatform(const CStdString& platform)
{
  if (platform == "all")
  {
    return true;
  }
  
  static CStdString myPlatform = "";
  
  if (myPlatform.length() == 0)
  {
#if defined(DEVICE_PLATFORM)
      myPlatform = DEVICE_PLATFORM;
#elif defined(__APPLE__)
    if (CSysInfo::IsAppleTV())
      myPlatform = "atv";
    else
      myPlatform = "mac";
#elif defined(_LINUX)
    myPlatform = "linux";
#else
    myPlatform = "win";
#endif
  }
  
  CStdStringArray platforms;
  bool found = false;
  StringUtils::SplitString(platform, ",", platforms);

  for (size_t i = 0; i < platforms.size(); i++)
  {
    if (platforms[i] == myPlatform)
    {
      found = true;
      break;
    }

   if (BOXEE::BXOEMConfiguration::GetInstance().HasParam("Boxee.Device.Name") &&
      platforms[i] == BOXEE::BXOEMConfiguration::GetInstance().GetStringParam("Boxee.Device.Name"))
    {
      found = true;
      break;
    }
  }
  
  return found;
}

CStdString CUtil::LongestCommonPrefix(const CStdString& str1, const CStdString& str2)
{
  /*
   *  $minLen = min(strlen($str1), strlen($str2));
  for ($i = 0; $i < $minLen; $i++) {
      if ($str1[$i] != $str2[$i]) {
          return substr($str1, 0, $i);
      }
  }
  return substr($str1, 0, $minLen);
   *
   */

  int minLen = min(str1.size(), str2.size());
  for (int i = 0; i < minLen; i++)
  {
    if (str1[i] != str2[i])
    {
      return str1.substr(0, i);
    }
  }
  return str1.substr(0, minLen);
}


// get path size - in linux only!!
Uint32 CUtil::GetDirSize(const CStdString& path) {

  Uint32 value = 0;

// expected to function only with linux
#ifdef __linux__
  FILE   *p = NULL;
  CStdString duCmdPath = "/usr/bin/du";
  CStdString duCmdArgs = "-sk";
  CStdString popenCmd = duCmdPath + " " + duCmdArgs + " " + path;

  p = popen (popenCmd.c_str(), "r");
  if (p)
  {
    fscanf(p, "%u", &value);
    pclose(p);
  }
#endif
  return value;
}

bool CUtil::IsDVDFolder(const CStdString& strPath, const CFileItemList* pPathItems)
{
  bool bDVDFolder = false;
  CFileItemList pathItems;

  if(!BoxeeUtils::IsPathLocal(strPath))
    return false;

  // optimization for network pathes
  // for protocol://hostname no need to search for dvd folder
  if(!CUtil::IsHD(strPath))
  {
    CURI u(strPath);
    if(u.GetFileName().IsEmpty())
      return false;
  }

  pathItems.SetProperty("notOpenPassword",true);
  if (!pPathItems)
  {
    CDirectory::GetDirectory(strPath, pathItems);
    pPathItems = &pathItems;
  }

#ifdef __APPLE__
  return CFile::Exists(strPath + "/VIDEO_TS/VIDEO_TS.IFO");
#endif

  for(int i=0; pPathItems && i<pPathItems->Size() && !bDVDFolder; i++)
  {
    CStdString strPath;
    CUtil::GetDirectory(pPathItems->Get(i)->m_strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);

    if(CUtil::GetFileName(strPath) == "VIDEO_TS")
    {
      CFileItemList videoTsItems;
      CDirectory::GetDirectory(pPathItems->Get(i)->m_strPath, videoTsItems);

      for(int j=0; j<videoTsItems.Size() && !bDVDFolder; j++)
      {
        if(videoTsItems[j]->m_strPath.Find("VIDEO_TS.IFO") != -1)
        {
          bDVDFolder = true;
          break;
        }
      }
    }
  }

  return bDVDFolder;
}

bool CUtil::IsBlurayFolder(const CStdString& strPath, const CFileItemList* pPathItems)
{
  bool bBlurayFolder = false;
  CFileItemList pathItems;

  if(!BoxeeUtils::IsPathLocal(strPath))
    return false;

  // optimization for network pathes
  // for protocol://hostname no need to search for bluray folder
  if(!CUtil::IsHD(strPath))
  {
    CURI u(strPath);
    if(u.GetFileName().IsEmpty())
      return false;
  }

  pathItems.SetProperty("notOpenPassword",true);
  if (!pPathItems)
  {
    CDirectory::GetDirectory(strPath, pathItems);
    pPathItems = &pathItems;
  }

#ifdef __APPLE__
  return CFile::Exists(strPath + "/BDMV/index.bdmv");
#endif

  for(int i=0; pPathItems && i<pPathItems->Size() && !bBlurayFolder; i++)
  {
    CStdString strPath;
    CUtil::GetDirectory(pPathItems->Get(i)->m_strPath,strPath);
    CUtil::RemoveSlashAtEnd(strPath);

    if(CUtil::GetFileName(strPath) == "BDMV")
    {
      CFileItemList bdmvItems;
      CDirectory::GetDirectory(pPathItems->Get(i)->m_strPath, bdmvItems);

      for(int j=0; j<bdmvItems.Size() && !bBlurayFolder; j++)
      {
        if(bdmvItems[j]->m_strPath.Find("index.bdmv") != -1)
        {
          bBlurayFolder = true;
          break;
        }
      }
    }
  }

  return bBlurayFolder;
}

bool CUtil::GetIsoDiskType(const CStdString& strIsoPath, CStdString& strDiskType)
{
  CStdString strFilename = strIsoPath;

  CUtil::RemoveSlashAtEnd(strFilename);

  if(!CUtil::GetExtension(strFilename).Equals(".iso"))
  {
    return false;
  } 

  CStdString strDVDDir = strFilename + "/VIDEO_TS";
  CUtil::URLEncode(strDVDDir);
  strDVDDir.Format("udf://%s", strDVDDir);

  CFileItemList items;
  CDirectory::GetDirectory(strDVDDir, items);

  strDiskType = "";

  // DVD
  if(items.Size() > 0)
  {
    for(int i=0; i<items.Size(); i++)
    {
      if(items[i]->m_strPath.Find("VIDEO_TS.IFO"))
      {
        strDiskType = "DVD";
        break;
      }
    }
  }
  
  // not DVD, probe for Bluray
  if(strDiskType.empty())
  {
    CStdString strBDDir = strFilename + "/BDMV";
    CUtil::URLEncode(strBDDir);
    strBDDir.Format("udf://%s", strBDDir);

    CFileItemList items;
    CDirectory::GetDirectory(strBDDir, items); 
    
    for(int i=0; i<items.Size(); i++)
    {
      if(items[i]->m_strPath.Find("index.bdmv"))
      {
        strDiskType = "BD";
        break;
      }
    }
  }

  return !strDiskType.empty();
}

bool CUtil::IsValidIp(const CStdString& ip)
{
#define IP_REGEXP "^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$"

  CRegExp reg;
  reg.RegComp(IP_REGEXP);
  if (reg.RegFind(ip.c_str()) == -1)
    return false;

  return true;
}

#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
CStdString CUtil::DumpStack(bool bPrintStack)
{
  CStdString line, stack;
  unsigned int max_frames = 63;
  void* addresses[max_frames+1];
  size_t funcnamesize;
  int f = 0, status;

  int addrlen = backtrace(addresses, sizeof(addresses) / sizeof(addresses[0]));
  if(addrlen == 0)
  {
    CLog::Log(LOGERROR, "backtrace is empty, possibly currupt");
    return "";
  }

  char** symbollist = backtrace_symbols(addresses, addrlen);
  char* funcname = (char*)malloc(funcnamesize);

  for (int i = 1; i < addrlen; i++)
  {
    char *begin_name = 0, *begin_offset = 0, *end_offset = 0;

    for (char *p = symbollist[i]; *p; ++p)
    {
      if (*p == '(')
        begin_name = p;
      else if (*p == '+')
        begin_offset = p;
      else if (*p == ')' && begin_offset)
      {
        end_offset = p;
        break;
      }
    }

    if (begin_name && begin_offset && end_offset && begin_name < begin_offset)
    {
      *begin_name++ = '\0';
      *begin_offset++ = '\0';
      *end_offset = '\0';
    }

    char * tmp = __cxa_demangle(begin_name, funcname, &funcnamesize, &status);

    if (status == 0 && tmp)
    {
      funcname = tmp;
      line.Format("% 2d: %p <%s+%s> (%s)\n", ++f, addresses[i], funcname, begin_offset, symbollist[i]);
    }
    else
    {
      line.Format("% 2d: %p <%s+%s> (%s)\n", ++f, addresses[i], symbollist[i], begin_name, symbollist[i]);
    }

    stack += line;
  }

  free(funcname);
  free(symbollist);

  if(bPrintStack)
    printf("%s\n", stack.c_str());

  return stack;
}
#endif

/*
 * Hide the mount point and replace it with something nicer
 */

bool CUtil::HideExternalHDPath(CStdString inputPath, CStdString& outputPath)
{
#ifdef HAS_EMBEDDED
  if (inputPath.Find(HAL_MOUNT_POINT, 0) == 0)
  {
    CStdString label("");

    VECSOURCES devices;
    CMediaManager::GetInstance().GetLocalDrives(devices);

    bool found = false;
    for (size_t i = 0; i < devices.size(); i++)
    {
      if (inputPath.find(devices[i].strPath) == 0)
      {
        label = devices[i].strName;
        found = true;
        break;
      }
    }

    if (!found)
      return false;

    if (label == "")
    {
      label = g_localizeStrings.Get(51363);
    }

    int rc = inputPath.find_first_of('/', strlen(HAL_MOUNT_POINT));
    if (rc == -1)
    {
      inputPath = label;
    }
    else
    {
      inputPath.replace(0, rc, label);
    }
  }
#endif
  outputPath = inputPath;
  return true;
}

bool CUtil::CheckFileSignature(const CStdString &file, const CStdString &validSig)
{
  SHA_CTX context;
  unsigned char hash[SHA_DIGEST_LENGTH];
  unsigned char buff[4096];
  
  SHA1_Init(&context);
  
  XFILE::CFile f;
  f.Open(file);
  int nRead=0;
  while ( (nRead = f.Read(buff, 4096)) > 0 )
  {
    SHA1_Update(&context, buff, nRead);
  }
  
  SHA1_Final(hash, &context);
  f.Close();
  
  std::string sig = CBase64::Decode(validSig.c_str());
  if (sig.empty())
    return false;
  
  RSA *rsa = NULL;
  FILE *fkey = fopen(_P("special://xbmc/system/apps-public.pem"), "r");
  if (fkey)
  {
    PEM_read_RSA_PUBKEY(fkey, &rsa, NULL, NULL);
    fclose(fkey);
    
    if (rsa)
    {
      bool bOk = RSA_verify(NID_sha1, hash, SHA_DIGEST_LENGTH, (unsigned char*)sig.data(), sig.size(), rsa);
      RSA_free(rsa);
      return bOk;
    }
    else
      return false;
  }
  else
    return false;
}

bool CUtil::GetHDDDirectory(const CStdString& path, CFileItemList& items)
{
  struct stat stDirInfo;
  struct dirent * stFiles;
  DIR * stDirIn;
  char szFullName[MAXPATHLEN];
  struct stat stFileInfo;
  //short int mode;

  if (lstat( path.c_str(), &stDirInfo) < 0)
    return false;

  if (!S_ISDIR(stDirInfo.st_mode))
      return false;

  if ((stDirIn = opendir( path.c_str())) == NULL)
      return false;

  while (( stFiles = readdir(stDirIn)) != NULL)
  {
    if(CUtil::HasSlashAtEnd(path))
      sprintf(szFullName, "%s%s", path.c_str(), stFiles->d_name);
    else
      sprintf(szFullName, "%s/%s", path.c_str(), stFiles->d_name);

    if (lstat(szFullName, &stFileInfo) < 0 || strlen(stFiles->d_name) == 0)
      continue;

    if (stFiles->d_name[0] == '.' || !strcmp(stFiles->d_name, "..") || !strcmp(stFiles->d_name, "."))
      continue;

    CFileItemPtr pItem(new CFileItem(stFiles->d_name));

    pItem->m_strPath = szFullName;
    pItem->m_bIsFolder = S_ISDIR(stFileInfo.st_mode);

    if (pItem->m_bIsFolder)
    {
      CUtil::AddSlashAtEnd(pItem->m_strPath);
    }
    else
    {
      pItem->m_dwSize = stFileInfo.st_size;
    }
    pItem->m_dateTime = stFileInfo.st_mtime;

    items.Add(pItem);
  }

  closedir(stDirIn);

  return true;
}

#if defined(HAS_EMBEDDED) && !defined(__APPLE__)
void CUtil::MemLeakDetectorStart()
{
  typedef void (*MemLeakDetectorStartFunc)(void);
  MemLeakDetectorStartFunc MemLeakDetectorStart =NULL;

  void* handle = dlopen("./leakdetector.so", RTLD_LAZY);
   
  if(handle)
  {
    MemLeakDetectorStart = (MemLeakDetectorStartFunc)dlsym(handle, "BxLeakDetectorStart");
  }

  if(MemLeakDetectorStart != NULL)
  {
    CLog::Log(LOGDEBUG, "Starting memory leak detector\n");

    MemLeakDetectorStart();
  }

  if(handle)
  {
    dlclose(handle);
  }
}


void CUtil::MemLeakDetectorStop()
{
  typedef void (*MemLeakDetectorStopFunc)(void);

  MemLeakDetectorStopFunc MemLeakDetectorStop = NULL;

  void* handle = dlopen("./leakdetector.so", RTLD_LAZY);

  if(handle)
  {
    MemLeakDetectorStop = (MemLeakDetectorStopFunc)dlsym(handle, "BxLeakDetectorStop");
  }

  if(MemLeakDetectorStop != NULL)
  {
    CLog::Log(LOGDEBUG,"Stopping memory leak detector\n");
    MemLeakDetectorStop();
  }

  if(handle)
  {
    dlclose(handle);
  }
}


void CUtil::MemLeakDetectorReport(const CStdString& reportFile)
{
  typedef void (*MemLeakDetectorReportFunc)(const char* file);

  MemLeakDetectorReportFunc MemLeakDetectorReport = NULL;

  CUtil::MemLeakDetectorStop();

  void* handle = dlopen("./leakdetector.so", RTLD_LAZY);

  if(handle)
  {
    MemLeakDetectorReport = (MemLeakDetectorReportFunc)dlsym(handle, "BxLeakDetectorPrintReport");
  }

  if(MemLeakDetectorReport != NULL)
  {
    CLog::Log(LOGDEBUG,"Reporting leaks to file %s\n", reportFile.c_str());
    MemLeakDetectorReport(reportFile.c_str());
  }

  if(handle)
  {
    dlclose(handle);
  }
}
#endif

IsPlayableFolderJob::IsPlayableFolderJob(CStdString path)
{
  m_strPath = path;
  m_result = CPlayableFolderType::PF_NO;
}

IsPlayableFolderJob::~IsPlayableFolderJob()
{

}

void IsPlayableFolderJob::Run()
{
  m_bJobResult = CUtil::IsBlurayFolder(m_strPath);

  if (m_bJobResult)
  {
    m_result = CPlayableFolderType::PF_BLURAY;
  }
  else
  {
    m_bJobResult = CUtil::IsDVDFolder(m_strPath);

    if (m_bJobResult)
    {
      m_result = CPlayableFolderType::PF_DVD;
    }
  }
}

bool CUtil::FindFile(const CStdString& filename, const CStdString& directory, std::vector<CStdString>& vecResult)
{
  bool bFound = false;
  CFileItemList items;

  CDirectory::GetDirectory(directory, items);

  for(int i=0; i<items.Size(); i++)
  {
    if(items[i]->m_bIsFolder)
    {
      bFound = FindFile(filename, items[i]->m_strPath, vecResult);
    }
    else
    {
      CStdString strFilename = CUtil::GetFileName(items[i]->m_strPath);

      if(strFilename == filename)
      {
        vecResult.push_back(items[i]->m_strPath);
      }
    }
  }

  return vecResult.size() != 0;
}

#ifdef _LINUX
bool CUtil::IsMountpoint(const CStdString& strPath)
{
  struct stat path_stat;
  bool result = false;

  if(stat(strPath.c_str(), &path_stat) == 0 && S_ISDIR(path_stat.st_mode))
  {
    CStdString parent = strPath + "/..";
    struct stat parent_stat;

    if(stat(parent.c_str(), &parent_stat) == 0)
    {
      result = (path_stat.st_dev != parent_stat.st_dev);
    }
  }

  return result;
}

int CUtil::GetFsMagic(const CStdString& strPath)
{
  struct statfs st;

  int res = statfs(strPath.c_str(), &st);
  if(res)
    return -1;

  return st.f_type;
}

bool CUtil::ValidateIpAddress(const CStdString& ipAddress)
{
  struct sockaddr_in sa;
  int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
  return result != 0;
}

#endif

bool CUtil::GetHostByName(const CStdString& strHostName, CStdString& strHostIp)
{
  struct hostent *hp = NULL;
  bool bSuccess = false;

  hp = gethostbyname(strHostName.c_str());
  if(hp)
  {
    strHostIp = inet_ntoa( *( struct in_addr*)( hp->h_addr_list[0]));

    if(!strHostIp.IsEmpty())
      bSuccess = true;
  }

  return bSuccess;
}

#ifdef _WIN32

/*
 * ascii-to-longlong conversion
 *
 * no error checking; assumes decimal digits
 *
 * efficient conversion: 
 *   start with value = 0
 *   then, starting at first character, repeat the following
 *   until the end of the string:
 *
 *     new value = (10 * (old value)) + decimal value of next character
 *
 */

long long atoll(const char *instr)
{
  long long retval;
  int i;

  retval = 0;
  for (; *instr; instr++) {
    retval = 10*retval + (*instr - '0');
  }
  return retval;
}

//struct tm * 
//localtime_r (const time_t *timer, struct tm *result) 
//{ 
//   struct tm *local_result; 
//   local_result = localtime (timer); 
//
//   if (local_result == NULL || result == NULL) 
//     return NULL; 
//
//   memcpy (result, local_result, sizeof (result)); 
//   return result; 
//} 

struct tm * 
gmtime_r (const time_t *timer, struct tm *result) 
{ 
   struct tm *local_result; 
   local_result = gmtime (timer); 

   if (local_result == NULL || result == NULL) 
     return NULL; 

   memcpy (result, local_result, sizeof (result)); 
   return result; 
} 

#endif
