#ifndef NETWORKDIRECTORY_H_
#define NETWORKDIRECTORY_H_

#include "IDirectory.h"

namespace DIRECTORY
{
class CNetworkDirectory : public IDirectory
{
public:
  CNetworkDirectory();
  virtual ~CNetworkDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
};
}

#endif /* NETWORKDIRECTORY_H_ */

