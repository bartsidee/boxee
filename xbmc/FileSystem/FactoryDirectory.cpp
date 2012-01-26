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
#include "FactoryDirectory.h"
#include "HDDirectory.h"
#include "SpecialProtocolDirectory.h"
#include "VirtualPathDirectory.h"
#include "MultiPathDirectory.h"
#include "StackDirectory.h"
#include "FactoryFileDirectory.h"
#include "PlaylistDirectory.h"
#include "MusicDatabaseDirectory.h"
#include "MusicSearchDirectory.h"
#include "VideoDatabaseDirectory.h"
#include "ShoutcastDirectory.h"
#ifdef HAS_LASTFM
#include "LastFMDirectory.h"
#endif
#ifdef HAS_FTP
#include "FTPDirectory.h"
#endif
#include "RSSDirectory.h"
#include "HTTPDirectory.h"
#include "Application.h"
#include "NetworkDirectory.h"

#include "BoxeeDatabaseDirectory.h"
#include "BoxeeFeedDirectory.h"
#include "BoxeeFriendsDirectory.h"
#include "BoxeeItemsHistoryDirectory.h"
#include "BoxeeUserActionsDirectory.h"
#include "BoxeeShortcutsDirectory.h"
#include "SourcesDirectory.h"
#include "BoxeeServerDirectory.h"
#include "BoxeeServerOTADirectory.h"
#include "BoxeeBrowseMenuDirectory.h"

#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
#include "WINSMBDirectory.h"
#else
#include "SMBDirectory.h"
#endif
#endif
#ifdef HAS_CCXSTREAM
#include "XBMSDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "CDDADirectory.h"
#endif
#include "PluginDirectory.h"
#ifdef HAS_FILESYSTEM
#include "ISO9660Directory.h"
#include "XBMSDirectory.h"
#ifdef HAS_FILESYSTEM_RTV
#include "RTVDirectory.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "SndtrkDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "DAAPDirectory.h"
#endif
#endif
#ifdef HAS_XBOX_HARDWARE
#include "MemUnitDirectory.h"
#endif
#ifdef HAS_UPNP
#include "UPnPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_SAP
#include "SAPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_VTP
#include "VTPDirectory.h"
#endif
#ifdef HAS_FILESYSTEM_HTSP
#include "HTSPDirectory.h"
#endif
#include "../utils/Network.h"
#include "ZipDirectory.h"
#include "RarDirectory.h"
#ifdef HAS_FILESYSTEM_TUXBOX
#include "DirectoryTuxBox.h"
#endif
#ifdef HAS_FILESYSTEM_HDHOMERUN
#include "HDHomeRun.h"
#endif
#include "AppsDirectory.h"
#include "AppBoxDirectory.h"
#include "RepositoriesDirectory.h"
#ifdef HAS_FILESYSTEM_MYTH
#include "CMythDirectory.h"
#endif
#include "ScriptDirectory.h"
#include "FileItem.h"
#include "URL.h"
#include "RSSDirectory.h"
#ifdef HAS_ZEROCONF
#include "ZeroconfDirectory.h"
#endif
#include "UDFDirectory.h"
#ifdef HAS_NFS
#include "NfsDirectory.h"
#endif
#ifdef HAS_AFP
#include "AfpDirectory.h"
#endif
#ifdef HAS_CIFS
#include "CIFSDirectory.h"
#endif
#ifdef HAS_BMS
#include "BMSDirectory.h"
#endif
#ifdef HAS_UPNP_AV
#include "UPnPAvDirectory.h"
#endif


using namespace DIRECTORY;

/*!
 \brief Create a IDirectory object of the share type specified in \e strPath .
 \param strPath Specifies the share type to access, can be a share or share with path.
 \return IDirectory object to access the directories on the share.
 \sa IDirectory
 */
IDirectory* CFactoryDirectory::Create(const CStdString& strPath)
{
  CURI url(strPath);

  CFileItem item;
  IFileDirectory* pDir=CFactoryFileDirectory::Create(strPath, &item);
  if (pDir)
    return pDir;

  CStdString strProtocol = url.GetProtocol();

  if (strProtocol.size() == 0 || strProtocol == "file") return new CHDDirectory();
  if (strProtocol == "special") return new CSpecialProtocolDirectory();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  if (strProtocol == "cdda") return new CCDDADirectory();
#endif
#ifdef HAS_FILESYSTEM
  if (strProtocol == "iso9660") return new CISO9660Directory();
#endif
  if (strProtocol == "udf") return new CUDFDirectory();
  if (strProtocol == "plugin") return new CPluginDirectory();
  if (strProtocol == "script") return new CScriptDirectory();
  if (strProtocol == "zip") return new CZipDirectory();
  if (strProtocol == "rar") return new CRarDirectory();
  if (strProtocol == "virtualpath") return new CVirtualPathDirectory();
  if (strProtocol == "multipath") return new CMultiPathDirectory();
  if (strProtocol == "stack") return new CStackDirectory();
  if (strProtocol == "playlistmusic") return new CPlaylistDirectory();
  if (strProtocol == "playlistvideo") return new CPlaylistDirectory();
  if (strProtocol == "musicdb") return new CMusicDatabaseDirectory();
  if (strProtocol == "boxeedb") return new CBoxeeDatabaseDirectory();
  if (strProtocol == "boxee") return new CBoxeeServerDirectory();
  if (strProtocol == "sources") return new CSourcesDirectory();
  if (strProtocol == "feed") return new CBoxeeFeedDirectory();
  if (strProtocol == "history") return new CBoxeeItemsHistoryDirectory();
  if (strProtocol == "actions") return new CBoxeeUserActionsDirectory();
  if (strProtocol == "friends") return new CBoxeeFriendsDirectory();
  if (strProtocol == "shortcuts") return new CBoxeeShortcutsDirectory();
  if (strProtocol == "network") return new CNetworkDirectory();
  if (strProtocol == "musicsearch") return new CMusicSearchDirectory();
  if (strProtocol == "videodb") return new CVideoDatabaseDirectory();
  if (strProtocol == "ota") return new CBoxeeServerOTADirectory();
  if (strProtocol == "browsemenu") return new CBoxeeBrowseMenuDirectory();
  if (strProtocol == "filereader")
    return CFactoryDirectory::Create(url.GetFileName());


  if (strProtocol == "shout") return new CShoutcastDirectory();
#ifdef HAS_LASTFM
  if (strProtocol == "lastfm") return new CLastFMDirectory();
#endif
#ifdef HAS_FILESYSTEM_TUXBOX
  if (strProtocol == "tuxbox") return new CDirectoryTuxBox();
#endif
#ifdef HAS_FTP
  if (strProtocol == "ftp" ||  strProtocol == "ftpx" ||  strProtocol == "ftps") return new CFTPDirectory();
#endif
  if (strProtocol == "http" || strProtocol == "https") return new CRSSDirectory();
#ifdef HAS_FILESYSTEM_SMB
#if defined(_WIN32)
  if (strProtocol == "smb") return new CWINSMBDirectory();
#elif defined(HAS_CIFS)
  if (strProtocol == "smb") return new CCifsDirectory();
#else
  if (strProtocol == "smb") return new CSMBDirectory();
#endif
#endif
#ifdef HAS_CCXSTREAM
  if (strProtocol == "xbms") return new CXBMSDirectory();
#endif
#ifdef HAS_FILESYSTEM
#ifdef HAS_FILESYSTEM_DAAP
  if (strProtocol == "daap") return new CDAAPDirectory();
#endif
#ifdef HAS_FILESYSTEM_RTV
  if (strProtocol == "rtv") return new CRTVDirectory();
#endif
#endif
#if defined(HAS_UPNP_AV)
  if (strProtocol == "upnp") return new CUPnPAvDirectory();
#elif defined(HAS_UPNP)
  if (strProtocol == "upnp") return new CUPnPDirectory();
#endif
#ifdef HAS_FILESYSTEM_HDHOMERUN
  if (strProtocol == "hdhomerun") return new CDirectoryHomeRun();
#endif
  if (strProtocol == "rss")  return new CRSSDirectory();
  if (strProtocol == "apps") return new CAppsDirectory();
  if (strProtocol == "appbox") return new CAppBoxDirectory();
  if (strProtocol == "repositories") return new CRepositoriesDirectory();
#ifdef HAS_FILESYSTEM_MYTH
  if (strProtocol == "myth") return new CCMythDirectory();
  if (strProtocol == "cmyth") return new CCMythDirectory();
#endif
  if (strProtocol == "rss") return new CRSSDirectory();
#ifdef HAS_FILESYSTEM_SAP
  if (strProtocol == "sap") return new CSAPDirectory();
#endif
#ifdef HAS_FILESYSTEM_VTP
  if (strProtocol == "vtp") return new CVTPDirectory();
#endif
#ifdef HAS_FILESYSTEM_HTSP
  if (strProtocol == "htsp") return new CHTSPDirectory();
#endif
#ifdef HAS_ZEROCONF
    if (strProtocol == "zeroconf") return new CZeroconfDirectory();
#endif
#ifdef HAS_NFS
    if (strProtocol == "nfs") return new CNfsDirectory();
#endif
#ifdef HAS_AFP
    if (strProtocol == "afp") return new CAfpDirectory();
#endif
#ifdef HAS_BMS
    if (strProtocol == "bms") return new CBmsDirectory();
#endif

  return NULL;
}

