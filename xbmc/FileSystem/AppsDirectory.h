#ifndef CAPPSDIRECTORY_H_
#define CAPPSDIRECTORY_H_

#include "IDirectory.h"
#include "AppDescriptor.h"

namespace DIRECTORY
{
  class CAppsDirectory : public IDirectory
  {
  public:
    CAppsDirectory();
    virtual ~CAppsDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

  private:

    bool HandleAppRequest(const CURL& url, CFileItemList &items);
    bool GetAppDirectoryByType(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, const CURL& url, CFileItemList &items);

    bool AddTestApps(std::set<CStdString> userInstalledAppsSet, CFileItemList &items);
  };
}

#endif /*CAPPSDIRECTORY_H_*/
