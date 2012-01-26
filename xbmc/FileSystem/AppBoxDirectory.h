#ifndef CAPPBOXDIRECTORY_H_
#define CAPPBOXDIRECTORY_H_

#include "IDirectory.h"
#include "AppDescriptor.h"

namespace DIRECTORY
{
  class CAppBoxDirectory : public IDirectory
  {
  public:
    CAppBoxDirectory();
    virtual ~CAppBoxDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

  private:

    bool HandleAppBoxRequest(const CURL& url, CFileItemList &items);
    bool GetAppBoxDirectoryByType(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, const CURL& url, CFileItemList &items);
  };
}

#endif /*CAPPBOXDIRECTORY_H_*/
