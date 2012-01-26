#ifndef NFS_DIRECTORY_H
#define NFS_DIRECTORY_H

#include "system.h"

#ifdef HAS_NFS
#include "IDirectory.h"

namespace DIRECTORY
{
class CNfsDirectory :
      public IDirectory
{
public:
  CNfsDirectory(void);
  virtual ~CNfsDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

  static bool GetResource(const CURI& path, CFileItem &item);
};
}
#endif

#endif
