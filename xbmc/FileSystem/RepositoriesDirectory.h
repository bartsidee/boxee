#ifndef CREPOSITORIESDIRECTORY_H_
#define CREPOSITORIESDIRECTORY_H_

#include "IDirectory.h"

namespace DIRECTORY
{
  class CRepositoriesDirectory : public IDirectory
  {
  public:
    CRepositoriesDirectory();
    virtual ~CRepositoriesDirectory();
    virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);

  private:

    void GetRepositoriesList(CFileItemList &items, const CStdString& type);
  };
}

#endif /*CREPOSITORIESDIRECTORY_H_*/
