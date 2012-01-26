

#include "FileItem.h"
#include "NetworkDirectory.h"
#include "Directory.h"

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

  dir.GetDirectory("smb://", l);
  items.Append(l);
  l.Clear();

  //dir.GetDirectory("upnp://", l);
  //items.Append(l);
  
  CFileItemPtr item (new CFileItem("UPnP"));
  item->m_strPath = "upnp://all";
  item->m_bIsFolder = true;

  items.Add(item);

  return true;
}

bool CNetworkDirectory::Exists(const char* strPath)
{
  return true;
}

}
