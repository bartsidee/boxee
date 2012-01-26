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
#include "FileFactory.h"
#include "FileHD.h"
#include "FileCurl.h"
#include "FilePlaylist.h"
#include "FileShoutcast.h"
#ifdef HAS_LASTFM
#include "FileLastFM.h"
#endif
#include "FileFileReader.h"
#include "FileCache.h"
#ifdef HAS_FILESYSTEM_SMB
#ifdef _WIN32
#include "WINFileSmb.h"
#else
#include "FileSmb.h"
#endif
#endif
#ifdef HAS_CCXSTREAM
#include "FileXBMSP.h"
#endif
#ifdef HAS_FILESYSTEM_CDDA
#include "FileCDDA.h"
#endif
#ifdef HAS_FILESYSTEM
#include "FileISO.h"
#ifdef HAS_FILESYSTEM_RTV
#include "FileRTV.h"
#endif
#ifdef HAS_XBOX_HARDWARE
#include "FileSndtrk.h"
#endif
#ifdef HAS_FILESYSTEM_DAAP
#include "FileDAAP.h"
#endif
#endif
#ifdef HAS_FILESYSTEM_SAP
#include "SAPFile.h"
#endif
#ifdef HAS_FILESYSTEM_VTP
#include "VTPFile.h"
#endif
#include "FileZip.h"
#include "FileRar.h"
#include "FileMusicDatabase.h"
#include "FileSpecialProtocol.h"
#include "MultiPathFile.h"
#include "../utils/Network.h"
#ifdef HAS_FILESYSTEM_TUXBOX
#include "FileTuxBox.h"
#endif
#ifdef HAS_FILESYSTEM_HDHOMERUN
#include "HDHomeRun.h"
#endif
#ifdef HAS_FILESYSTEM_MYTH
#include "CMythFile.h"
#endif
#include "Application.h"
#include "URL.h"
#include "PipesManager.h"
#include "FilePipe.h"
#include "FileUPnP.h"
#include "FileUDF.h"
#ifdef HAS_NFS
#include "FileNfs.h"
#endif
#ifdef HAS_AFP
#include "FileAfp.h"
#endif
#ifdef HAS_CIFS
#include "FileCifs.h"
#endif
#ifdef HAS_BMS
#include "FileBms.h"
#endif
#ifdef HAS_UPNP_AV
#include "FileUPnPAv.h"
#endif


using namespace XFILE;

CFileFactory::CFileFactory()
{
}

CFileFactory::~CFileFactory()
{
}

IFile* CFileFactory::CreateLoader(const CStdString& strFileName)
{
  CURI url(strFileName);
  return CreateLoader(url);
}

IFile* CFileFactory::CreateLoader(const CURI& url)
{
  CStdString strProtocol = url.GetProtocol();
  strProtocol.MakeLower();

  if (strProtocol == "zip") return new CFileZip();
  else if (strProtocol == "rar") return new CFileRar();
  else if (strProtocol == "musicdb") return new CFileMusicDatabase();
  else if (strProtocol == "special") return new CFileSpecialProtocol();
  else if (strProtocol == "playlist") return new CFilePlaylist();
  else if (strProtocol == "multipath") return new CMultiPathFile();
  else if (strProtocol == "file" || strProtocol.IsEmpty()) return new CFileHD();
  else if (strProtocol == "filereader") return new CFileFileReader();
#if defined(HAS_FILESYSTEM_CDDA) && defined(HAS_DVD_DRIVE)
  else if (strProtocol == "cdda") return new CFileCDDA();
#endif
#ifdef HAS_FILESYSTEM
  else if (strProtocol == "iso9660") return new CFileISO();
#ifdef HAS_XBOX_HARDWARE
  else if (strProtocol == "soundtrack") return new CFileSndtrk();
#endif
#endif
#ifdef HAS_XBOX_HARDWARE
  else if (strProtocol.Left(3) == "mem") return new CFileMemUnit();
#endif
  else if(strProtocol == "udf") return new CFileUDF();

  if (strProtocol == "http" 
    ||  strProtocol == "https")
      return new CFileCurl();
   else if (strProtocol == "ftp" 
       ||  strProtocol == "ftpx"
       ||  strProtocol == "ftps") return new CFileCurl();
#ifdef HAS_UPNP_AV
  else if (strProtocol == "upnp") return new CFileUPnPAv();
#else
  else if (strProtocol == "upnp") return new CFileUPnP();
#endif
  else if (strProtocol == "shout") return new CFileShoutcast();
#ifdef HAS_LASTFM
  else if (strProtocol == "lastfm") return new CFileLastFM();
#endif
#ifdef HAS_FILESYSTEM_TUXBOX
  else if (strProtocol == "tuxbox") return new CFileTuxBox();
#endif
#ifdef HAS_FILESYSTEM_HDHOMERUN
  else if (strProtocol == "hdhomerun") return new CFileHomeRun();
#endif
#ifdef HAS_FILESYSTEM_MYTH
  else if (strProtocol == "myth") return new CCMythFile();
  else if (strProtocol == "cmyth") return new CCMythFile();
#endif
#ifdef HAS_FILESYSTEM_SMB
#if defined(_WIN32)
  else if (strProtocol == "smb") return new CWINFileSMB();
#elif defined(HAS_CIFS)
  else if (strProtocol == "smb") return new CFileCifs();
#else
  else if (strProtocol == "smb") return new CFileSMB();
#endif
#endif
#ifdef HAS_CCXSTREAM
  else if (strProtocol == "xbms") return new CFileXBMSP();
#endif
#ifdef HAS_FILESYSTEM
#ifdef HAS_FILESYSTEM_RTV
  else if (strProtocol == "rtv") return new CFileRTV();
#endif
#ifdef HAS_FILESYSTEM_DAAP
  else if (strProtocol == "daap") return new CFileDAAP();
#endif
#endif
#ifdef HAS_FILESYSTEM_SAP
  else if (strProtocol == "sap") return new CSAPFile();
#endif
#ifdef HAS_FILESYSTEM_VTP
  else if (strProtocol == "vtp") return new CVTPFile();
#endif
  else if (strProtocol == "pipe") return new XFILE::CFilePipe();
#ifdef HAS_NFS
  else if (strProtocol == "nfs") return new CFileNfs();
#endif
#ifdef HAS_AFP
  else if (strProtocol == "afp") return new CFileAfp();
#endif
#ifdef HAS_BMS
  else if (strProtocol == "bms") return new CFileBms();
#endif
  return NULL;
}
