#ifndef BOXEEUSERACTIONSDIRECTORY_H_
#define BOXEEUSERACTIONSDIRECTORY_H_

#include "IDirectory.h"
#include "lib/libBoxee/bxmetadata.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxboxeefeed.h"

#include "BoxeeFeedItemsLoader.h"
#include "BoxeeDatabaseDirectory.h"

namespace DIRECTORY
{
class CBoxeeUserActionsDirectory : public IDirectory
{
public:
	CBoxeeUserActionsDirectory();
	virtual ~CBoxeeUserActionsDirectory();
	virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
	virtual bool Exists(const char* strPath);

private:

  int GetNumOfActionsToRetrieve(const CStdString& strPath, int numOfActions, int limit);

};
}
#endif /*BOXEEUSERACTIONSDIRECTORY_H_*/
