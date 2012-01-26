#include "system.h"

#ifdef HAS_NFS
#include "Util.h"
#include "URL.h"
#include "FileItem.h"
#include "Directory.h"
#include "NfsDirectory.h"
#include "BrowserService.h"
#include "DirectoryCache.h"
#include "Application.h"
#include "utils/log.h"
#include "bxutils.h"

#include <sys/socket.h>
#include <sys/wait.h>
#include <sys/vfs.h>
#include <sys/statfs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/mount.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#include <vector>

using namespace DIRECTORY;

#define MOUNT_PROGNUM 100005
#define MOUNT_VERSNUM 1
#define	MOUNTPROC_EXPORT 5
#define	RPCMNT_NAMELEN	255
#define	RPCMNT_PATHLEN	1024

#define MOUNT_ROOT "/tmp/mnt/"

#ifndef NFS_SUPER_MAGIC
#define NFS_SUPER_MAGIC 0x6969
#endif

typedef struct NfsShare
{
  CStdString ipAddr;
  CStdString hostName;
} NfsShare;  


typedef struct groupnode {
  char gr_name[RPCMNT_NAMELEN+1];
  groupnode *gr_next;
} groupnode;


typedef struct exportnode {
  char	ex_dir[RPCMNT_PATHLEN+1];
  groupnode* ex_groups;
  exportnode *ex_next;
} exportnode;

std::vector<CStdString> g_vecMountpoints;

#ifdef _LINUX
static int 
xdr_broadcast(void* out, struct sockaddr_in *who)
{
  register struct hostent *hp;
  std::vector<NfsShare>* pNfsShares = (std::vector<NfsShare>*) out;
  NfsShare share;

  hp = gethostbyaddr ((char *) &who->sin_addr, sizeof who->sin_addr,
                      AF_INET);

  share.ipAddr = inet_ntoa (who->sin_addr);
  share.hostName = (hp == NULL) ? ("") : hp->h_name;

  std::vector<NfsShare>::iterator it = pNfsShares->begin();
  bool bDuplicate = false;
  while (it != pNfsShares->end()) 
  {
    const NfsShare& sh = *it;
    if(sh.ipAddr == share.ipAddr)
    {
      bDuplicate = true;
      break;
    }
    it++;
  }

  if(!bDuplicate)
    pNfsShares->push_back(share);

  return bDuplicate;  
}

static int
xdr_exports(XDR *xdrsp, struct exportnode **exp)
{
  struct exportnode *ep = NULL;
  struct groupnode *gp = NULL;
  int b, grpbool;
  char *strp;

  *exp = (struct exportnode *)0;
   if (!xdr_bool(xdrsp, &b))
     return (0);

   while (b) 
   {
     ep = (struct exportnode *)malloc(sizeof(struct exportnode));
     if (ep == NULL)
       return (0);

     ep->ex_groups = (struct groupnode *)0;
     strp = ep->ex_dir;
     if (!xdr_string(xdrsp, &strp, RPCMNT_PATHLEN))
       return (0);

     if (!xdr_bool(xdrsp, &grpbool))
       return (0);

     while (grpbool) {
       gp = (struct groupnode *)malloc(sizeof(struct groupnode));
       if (gp == NULL)
         return (0);

       strp = gp->gr_name;
       if (!xdr_string(xdrsp, &strp, RPCMNT_NAMELEN))
         return (0);

       gp->gr_next = ep->ex_groups;
       ep->ex_groups = gp;

       if (!xdr_bool(xdrsp, &grpbool))
         return (0);
    }
    ep->ex_next = *exp;
    *exp = ep;

    if (!xdr_bool(xdrsp, &b))
     return (0);
  }

  return (1);
}

static CStdString 
GetMountPoint(const CStdString &strType, const CStdString &strHost, const CStdString &exportDirName)
{
  CStdString strPath = strType + "_" + strHost + "_" +  exportDirName;
  CUtil::URLEncode(strPath);

  return MOUNT_ROOT + strPath;
}

static int
UnMountShare(const CStdString &strMountPoint)
{
  return umount(strMountPoint.c_str());
}

static bool 
MountShare(const CStdString &nfsPath, const CStdString &strMountPoint)
{
  CStdString cmd;
  
  if(CUtil::IsMountpoint(strMountPoint))
  {
    UnMountShare(strMountPoint);
  }
  
  ::CreateDirectory(strMountPoint, NULL);

  cmd = "mount -o nolock -t nfs " + CStdString("\"") + nfsPath + CStdString("\"") + " " + strMountPoint;
  
  system(cmd.c_str());

  // test for mountpoint
  if(!CUtil::IsMountpoint(strMountPoint))
  {
    CLog::Log(LOGERROR, "%s - Failed to mount [%s]", __func__, strMountPoint.c_str());
    return false;
  }
  
  return true;
}

static AUTH* 
nfs_authsys_create(void)
{
  char machname[MAXHOSTNAMELEN + 1];
  uid_t   uid = geteuid();
  gid_t   gid = getegid();

  if (gethostname(machname, sizeof(machname)) == -1)
    return NULL;

  return authunix_create(machname, uid, gid, 1, &gid);
}
 
static bool 
GetExports(const char* hostname, std::vector<CStdString>& exports)
{
  CLIENT *client = NULL;
  struct timeval total_timeout;
  exportnode* exportlist;
  enum clnt_stat clnt_stat;
  bool result = false;

  do
  {
    client = clnt_create(hostname, MOUNT_PROGNUM, MOUNT_VERSNUM, "tcp");
    if (!client)
    {
      client = clnt_create(hostname, MOUNT_PROGNUM, MOUNT_VERSNUM, "udp");
    }
  
    if(!client)
    {
       CLog::Log(LOGERROR, "%s - clnt_create failed, errno[%d][%s]", __func__, errno, strerror(errno));
       break;       
    }

     client->cl_auth = nfs_authsys_create();
     if (client->cl_auth == NULL) 
     {
       CLog::Log(LOGERROR, "%s - unable to create RPC auth handle", __func__);
       break;
     }

     total_timeout.tv_sec = 20;
     total_timeout.tv_usec = 0;

     memset(&exportlist, '\0', sizeof(exportlist));

     clnt_stat = clnt_call(client, MOUNTPROC_EXPORT,
                       (xdrproc_t) xdr_void, NULL,
                       (xdrproc_t) xdr_exports, (caddr_t) &exportlist,
                        total_timeout);   

     if (clnt_stat == RPC_PROGVERSMISMATCH) 
     {
       CLog::Log(LOGERROR, "%s - invalid RPC version", __func__);
       break;
     } 

     if (clnt_stat != RPC_SUCCESS) 
     {
       CLog::Log(LOGERROR, "%s - failed with errro [%d]", __func__, clnt_stat);
       break;
     }

     while (exportlist) 
     {
       exportnode* node = exportlist;

       exports.push_back(node->ex_dir);
       exportlist = exportlist->ex_next;

       free(node->ex_groups);
       free(node);
  
     }

     result = true;

  } while(false);


  if(client)
  {
    if(client->cl_auth)
      auth_destroy(client->cl_auth);

    clnt_destroy(client);
  }
  return result;
}
#endif

static bool 
ScanNfsShares(std::vector<NfsShare>& shares)
{
  bool bResult = true;
#ifdef _LINUX
  enum clnt_stat rpc_stat;
  rpc_stat = clnt_broadcast (MOUNT_PROGNUM, MOUNT_VERSNUM, NULLPROC, (xdrproc_t) xdr_void,
                             NULL, (xdrproc_t) xdr_void, (char*)&shares,
                             (resultproc_t) xdr_broadcast);

  if ((rpc_stat != RPC_SUCCESS) && (rpc_stat != RPC_TIMEDOUT))
  {
    CLog::Log(LOGERROR, "%s - failed, rpc error [%s]", __func__, clnt_sperrno (rpc_stat));
    bResult = false;
  }
#endif
  
  return bResult;    
}

CNfsDirectory::CNfsDirectory(void)
{}

CNfsDirectory::~CNfsDirectory(void)
{}

bool CNfsDirectory::GetResource(const CURI& path, CFileItem &item)
{
  if(path.GetProtocol() != "nfs")
  {
     CLog::Log(LOGERROR, "CNfsDirectory::%s - invalid protocol [%s]", __func__, path.GetProtocol().c_str());
     return false;
  }

  CStdString strHostName = path.GetHostName();
  CStdString strFullPath = path.GetFileName();
  CStdString strFileName, strExportDir;

  int iPos = strFullPath.Find(":", 0);
  if (iPos != -1)
  {
    strExportDir = strFullPath.substr(0, iPos);
    strFileName  = strFullPath.substr(iPos + 1, strFullPath.length());
  }
  else
  {
    strExportDir = strFullPath;
  }

  CStdString strMountPoint = GetMountPoint("nfs",  strHostName, strExportDir);

  // path is not mounted - need to mount it
  if(!CUtil::IsMountpoint(strMountPoint) && CUtil::GetFsMagic(strMountPoint+strFileName) != NFS_SUPER_MAGIC)
  {
    CStdString nfsPath = strHostName + ":" + "/" + strExportDir;

    CLog::Log(LOGDEBUG, "CNfsDirectory::%s - mounting NFS share [%s] ==> [%s]", __func__, nfsPath.c_str(), strMountPoint.c_str());

    if(MountShare(nfsPath, strMountPoint) == false)
    {
      CLog::Log(LOGERROR, "CNfsDirectory::%s - failed to mount NFS share [%s]", __func__, nfsPath.c_str());
      return false;
    }
  }

  CStdString strHddPath = strMountPoint + strFileName;

  item.m_strPath = strHddPath;
  item.SetLabel(path.Get());

  item.SetProperty("filename", strFileName);
  item.SetProperty("mountpoint", strMountPoint);

  struct stat st;
  if (stat(strHddPath.c_str(),&st) == 0)
  {
    item.m_bIsFolder = S_ISDIR(st.st_mode);
  }

  return true;
}

bool CNfsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CURI url(strPath);
  bool bResult = false;

  if(url.GetProtocol() != "nfs")
  {
     CLog::Log(LOGERROR, "CNfsDirectory::%s - invalid protocol [%s]", __func__, url.GetProtocol().c_str());
     return false;
  }

  CBrowserService* pBrowser = g_application.GetBrowserService();

  if( strPath == "nfs://" && pBrowser )
  {
    pBrowser->GetShare( CBrowserService::NFS_SHARE, items );
    return true;
  }

  // If we have the items in the cache, return them
  if (g_directoryCache.GetDirectory(strPath, items)) 
  {
    return true;
  }

  if(strPath == "nfs://all")
  {
     std::vector<NfsShare> shares;

     bool succedded = ScanNfsShares(shares);
     if(!succedded)
     {
       CLog::Log(LOGERROR, "CNfsDirectory::%s - failed to scan NFS shares", __func__);
       return false;
     }

     for(size_t i=0; i<shares.size(); i++)
     {
       CStdString hostName = shares[i].hostName.IsEmpty() ? shares[i].ipAddr : shares[i].hostName;
       CFileItemPtr pItem(new CFileItem(hostName));
       
       pItem->m_strTitle = hostName;
       pItem->m_bIsFolder = true;
       pItem->m_strPath.Format("nfs://%s", hostName);
       pItem->SetProperty("isNetwork",true);

       items.Add(pItem);
     }

     CLog::Log(LOGDEBUG, "CNfsDirectory::%s - found %d NFS devices", __func__,  shares.size());
     return true;
  } 

  CStdString hostName = url.GetHostName();
  CStdString fileName = url.GetFileName();
   
  if(fileName.IsEmpty())
  {
    std::vector<CStdString> exports;

    if(false == GetExports(hostName.c_str(), exports)) 
    {
      CLog::Log(LOGERROR, "CNfsDirectory::%s - failed to get export for host [%s]", __func__, hostName.c_str());
      return false;
    }
 
    CLog::Log(LOGDEBUG, "CNfsDirectory::%s - found [%d] exports for [%s] ", __func__,  exports.size(), hostName.c_str());

    for(size_t i=0; i<exports.size(); i++)
    {
      CStdString title = exports[i];
      CFileItemPtr pItem(new CFileItem(title));
      pItem->m_strTitle = title;
      pItem->m_bIsFolder = true;
      pItem->SetProperty("isNetwork",true);

      pItem->m_strPath.Format("nfs://%s%s", hostName, exports[i]);
      
      items.Add(pItem);
    }
   
    bResult = true;
  }
  else
  {
    CFileItem item;
    
    if(false == GetResource(url, item))
    {
      CLog::Log(LOGERROR, "CNfsDirectory::%s - failed to get resource for path [%s]", __func__, url.Get().c_str());
      return false;
    }

    bResult = CUtil::GetHDDDirectory(item.m_strPath,items);

    for(int i=0; i<items.Size(); i++)
    {
      if(items[i]->m_bIsFolder)
      {
        items[i]->SetProperty("isNetwork",true);
      }
       // build nfs path 
       CStdString nfsPath = items[i]->m_strPath;
       nfsPath.Replace(item.m_strPath, strPath);
       
       if (nfsPath.length() >= strPath.length())
       {
         CStdString strNewFilename = nfsPath.substr(strPath.length());

         CStdString strNfsFileName;
         CStdString strFileName = item.GetProperty("filename");
         if (strFileName.IsEmpty())
           strNfsFileName = ":" + strNewFilename;
         else
           strNfsFileName = strNewFilename;

         items[i]->m_strPath.Format("%s%s", strPath.c_str(), strNfsFileName.c_str());

       }
    } 
  }

  return bResult;
}

bool CNfsDirectory::Exists(const char* strPath)
{
  CFileItemList items;
  if (GetDirectory(strPath,items))
    return true;

  return false;
}
#endif
