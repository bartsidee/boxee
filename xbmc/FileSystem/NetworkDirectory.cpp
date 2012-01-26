

#include "FileItem.h"
#include "NetworkDirectory.h"
#include "Directory.h"
#include "LocalizeStrings.h"
#include "MediaManager.h"
#include "SpecialProtocol.h"


namespace DIRECTORY
{

CNetworkDirectory::CNetworkDirectory() 
{
}

CNetworkDirectory::~CNetworkDirectory() 
{
}

bool CNetworkDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CDirectory dir;
  CFileItemList l;

  if(strPath.Find("network://protocols") != -1)
  {
    // mounted devices (Removable)
#ifdef _LINUX
#ifndef HAS_EMBEDDED
    CFileItemPtr share (new CFileItem(g_localizeStrings.Get(51360)));
    share->SetProperty("IsShare","1");
    share->m_strPath = _P("special://userhome");
    share->m_bIsFolder = true;
    items.Add(share);
#endif
    VECSOURCES removableDrives;
    g_mediaManager.GetRemovableDrives(removableDrives);

    for (size_t i = 0; i < removableDrives.size(); i++)
    {
      CMediaSource removableDrive = removableDrives[i];
      CFileItemPtr share (new CFileItem(removableDrive.strName));
      share->SetProperty("IsShare","1");
      share->m_strPath = removableDrive.strPath;
      share->m_bIsFolder = true;
      share->SetProperty("IsRemovable",true);

      if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_USB)
        share->SetProperty("IsUSB",true);
      else if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_SD)
        share->SetProperty("IsSD",true);
      else if (removableDrive.m_localSourceType == CMediaSource::LOCAL_SOURCE_TYPE_INTERNAL_HD)
        share->SetProperty("IsInternalHD",true);
      else
        share->SetProperty("IsUSB",true);

      l.Add(share);

    } //for
#else
    // Add windows drives as sources
    CStdString devicePath;

    VECSOURCES drives;
    g_mediaManager.GetLocalDrives(drives);
    for (size_t i = 0; i < drives.size(); i++)
    {
      CMediaSource source = drives[i];
      devicePath = source.strPath;

      CFileItemPtr windowsDrive(new CFileItem("Local disk (" + source.strName + ")"));
      windowsDrive->m_strPath = (devicePath);
      windowsDrive->m_bIsFolder = true;
      windowsDrive->SetProperty("IsFolder",true);
      l.Add(windowsDrive);
    }
    drives.clear();
    g_mediaManager.GetRemovableDrives(drives);
    for (size_t i = 0; i < drives.size(); i++)
    {
      CMediaSource source = drives[i];
      devicePath = source.strPath;

      CFileItemPtr windowsDrive(new CFileItem(source.strName));
      windowsDrive->m_strPath = (devicePath);
      windowsDrive->m_bIsFolder = true;
      windowsDrive->SetProperty("IsFolder",true);
      l.Add(windowsDrive);
    }
#endif

#ifdef HAS_AFP
    CFileItemPtr afpprotocol(new CFileItem ( g_localizeStrings.Get(51253)));
    afpprotocol->m_strPath = "afp://";
    afpprotocol->m_bIsFolder = true;
    afpprotocol->SetProperty("IsNetwork",true);
    l.Add(afpprotocol);
#endif

#ifdef HAS_BMS
    CFileItemPtr bmsprotocol(new CFileItem ( g_localizeStrings.Get(51255)));
    bmsprotocol->m_strPath = "bms://";
    bmsprotocol->m_bIsFolder = true;
    bmsprotocol->SetProperty("IsNetwork",true);
    l.Add(bmsprotocol);
#endif

#ifdef HAS_NFS
    CFileItemPtr nfsprotocol(new CFileItem ( g_localizeStrings.Get(51251)));
    nfsprotocol->m_strPath = "nfs://";
    nfsprotocol->m_bIsFolder = true;
    nfsprotocol->SetProperty("IsNetwork",true);
    l.Add(nfsprotocol);
#endif

    CFileItemPtr upnpprotocol(new CFileItem ( g_localizeStrings.Get(51252)));
    upnpprotocol->m_strPath = "upnp://all";
    upnpprotocol->m_bIsFolder = true;
    upnpprotocol->SetProperty("IsNetwork",true);
    l.Add(upnpprotocol);

    CFileItemPtr smbprotocol(new CFileItem ( g_localizeStrings.Get(51250)));
    smbprotocol->m_strPath = "smb://computers";
    smbprotocol->m_bIsFolder = true;
    smbprotocol->SetProperty("IsNetwork",true);
    l.Add(smbprotocol);

    items.Append(l);
    return true;
  }

  dir.GetDirectory("smb://", l);
  items.Append(l);
  l.Clear();

  //dir.GetDirectory("upnp://", l);
  //items.Append(l);
  
  if(strPath.Find("all") != -1)
  {
    dir.GetDirectory("upnp://all", l);

    items.Append(l);
    l.Clear();

    dir.GetDirectory("nfs://", l);
    items.Append(l);
    l.Clear();
  }
  else
  {
    CFileItemPtr item (new CFileItem("UPnP"));
    item->m_strPath = "upnp://all";
    item->m_bIsFolder = true;

    items.Add(item);


    CFileItemPtr nfs (new CFileItem("NFS"));
    nfs->m_strPath = "nfs://";
    nfs->m_bIsFolder = true;

    items.Add(nfs);
  }

  CFileItemPtr afp (new CFileItem("AFP (Apple Filing Protocol)"));
  afp->m_strPath = "afp://";
  afp->m_bIsFolder = true;
  items.Add(afp);

  return true;
}

bool CNetworkDirectory::Exists(const char* strPath)
{
  return true;
}

}
