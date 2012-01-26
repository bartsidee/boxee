#ifndef BOXEESHORTCUTSDIRECTORY_H_
#define BOXEESHORTCUTSDIRECTORY_H_

#include "IDirectory.h"

namespace DIRECTORY
{
class CBoxeeShortcutsDirectory: public IDirectory
{
public:
  CBoxeeShortcutsDirectory();
  virtual ~CBoxeeShortcutsDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
};
}

#endif /*BOXEESHORTCUTSDIRECTORY_H_*/
