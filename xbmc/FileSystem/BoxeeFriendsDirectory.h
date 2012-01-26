#ifndef BOXEEFRIENDSDIRECTORY_H_
#define BOXEEFRIENDSDIRECTORY_H_

#include "IDirectory.h"
#include "lib/libBoxee/bxmetadata.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxboxeefeed.h"

#include "BoxeeFeedItemsLoader.h"

namespace DIRECTORY
{
class CBoxeeFriendsDirectory : public IDirectory
{
public:
	CBoxeeFriendsDirectory();
	virtual ~CBoxeeFriendsDirectory();
	virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
	virtual bool Exists(const char* strPath);

	BOXEE::BXBoxeeFeed      m_boxeeFeed;
	CBoxeeFeedItemsLoader   *m_feedItemLoader;

private:

  int GetNumOfFriendsToRetrieve(const CStdString& strPath, int numOfFriends, int limit);

};
}



#endif /*BOXEEFRIENDSDIRECTORY_H_*/
