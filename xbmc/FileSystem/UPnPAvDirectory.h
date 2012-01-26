#ifndef UPNP_AV_DIRECTORY_H
#define UPNP_AV_DIRECTORY_H

#include "system.h"

#ifdef HAS_UPNP_AV
#include "IDirectory.h"

namespace DIRECTORY
{
class CUPnPAvDirectory :
      public IDirectory
{
   bool  ReadDir(const CURI& url, CFileItemList &items);
public:
  CUPnPAvDirectory(void);
  virtual ~CUPnPAvDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

  static bool GetResource(const CURI& path, CFileItem &item);
  static void GetFriendlyPath(CStdString& path);
};
}
#endif

#endif
