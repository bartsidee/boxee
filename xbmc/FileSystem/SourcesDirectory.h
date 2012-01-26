#ifndef SOURCESDIRECTORY_H_
#define SOURCESDIRECTORY_H_

#include "IDirectory.h"

namespace DIRECTORY
{
class CSourcesDirectory : public IDirectory
{
public:
  CSourcesDirectory();
  virtual ~CSourcesDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
};
}

#endif /* SOURCESDIRECTORY_H_ */
