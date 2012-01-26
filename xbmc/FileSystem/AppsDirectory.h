#ifndef CAPPSDIRECTORY_H_
#define CAPPSDIRECTORY_H_

#include "IDirectory.h"
#include "AppDescriptor.h"
#include "URL.h"

namespace DIRECTORY
{
  class CAppsDirectory : public IDirectory
  {
  public:
    CAppsDirectory();
    virtual ~CAppsDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

  private:

    bool HandleAppRequest(const CURI& url, CFileItemList &items);
    bool GetAppDirectoryByType(const CAppDescriptor::AppDescriptorsMap& availableAppsDesc, const CURI& url, CFileItemList &items, const CStdString& strExtLink="");

    bool AddTestApps(std::set<CStdString> userInstalledAppsSet, CFileItemList &items,const CStdString& path="special://home/apps", bool bOnRemovableStorage=false);
    bool VerifyDeviceSignature(const CFileItemPtr& appItem, const CStdString& path);
  };
}

#endif /*CAPPSDIRECTORY_H_*/
