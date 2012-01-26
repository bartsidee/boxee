#ifndef UDF_DIRECTORY_H
#define UDF_DIRECTORY_H

#include "IDirectory.h"

namespace DIRECTORY
{
class CUDFDirectory :
      public IDirectory
{
public:
  CUDFDirectory(void);
  virtual ~CUDFDirectory(void);
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
};
}

#endif

