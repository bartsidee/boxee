#ifndef _CIFS_DIRECTORY_H
#define _CIFS_DIRECTORY_H

#include "system.h"

#ifdef HAS_CIFS
#include "IDirectory.h"

namespace DIRECTORY
{
class CCifsDirectory :
      public IDirectory
{
public:
  CCifsDirectory(void);
  virtual ~CCifsDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

  static int GetResource(const CURI& path, CFileItem &item);

private:
  int HandlePath(const CStdString& strPath, CFileItem &item, bool dontOpenPasswordDialog = false);
  bool ScanShares(CFileItemList &items);

};
}
#endif

#endif
