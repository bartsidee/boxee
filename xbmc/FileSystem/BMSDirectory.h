#ifndef BMS_DIRECTORY_H
#define BMS_DIRECTORY_H

#include "system.h"

#ifdef HAS_BMS
#include "IDirectory.h"

namespace DIRECTORY
{
class CBmsDirectory :
      public IDirectory
{
  bool ReadDir(const CStdString& strPath, CFileItemList &items);
public:
  CBmsDirectory(void);
  virtual ~CBmsDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

  static bool GetResource(const CURI& path, CFileItem &item);
};
}
#endif

#endif
