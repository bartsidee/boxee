#ifndef BOXEEITEMLAUNCHER_H_
#define BOXEEITEMLAUNCHER_H_

#include "FileItem.h"

class CBoxeeItemLauncher
{
public:
  static bool Launch(const CFileItem& item);
  static void PlayDVD();
  static bool LaunchPictureItem(const CFileItem& item);
  static bool PlayFolder(const CFileItem& item, bool isDVD);
  static bool ExecutePlayableFolder (const CFileItem& item);

private:
  static void RunScript(const CStdString& strPath);
};

#endif /* BOXEEITEMLAUNCHER_H_ */
