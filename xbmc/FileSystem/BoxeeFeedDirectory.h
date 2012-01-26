#ifndef BOXEEFEEDDIRECTORY_H_
#define BOXEEFEEDDIRECTORY_H_

#include "IDirectory.h"
#include "lib/libBoxee/bxmetadata.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxboxeefeed.h"

#include "BoxeeFeedItemsLoader.h"

namespace DIRECTORY
{
class CBoxeeFeedDirectory : public IDirectory
{
public:
	CBoxeeFeedDirectory();
	virtual ~CBoxeeFeedDirectory();
	virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
	virtual bool Exists(const char* strPath);
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const { return DIR_CACHE_ALWAYS; };

	BOXEE::BXBoxeeFeed      m_boxeeFeed;
	CBoxeeFeedItemsLoader   *m_feedItemLoader;
	
private:
  
  bool HandleRequestWithLimit(const CStdString& typeToRetrieveForLog,CFileItemList& items,std::map<std::string, std::string>& mapParams, bool sortByDate = true);
  bool ConvertServerResponse(const CStdString& typeToRetrieveForLog,CFileItemList& listItems);
  bool GetItemsFromServer(const CStdString& typeToRetrieveForLog, std::map<std::string, std::string>& mapParams);
  
  bool HandleQueueRequest(CFileItemList& items,std::map<std::string, std::string>& mapParams);
  int GetTheLastItemIndexById(const CStdString& lastItemId);

  void UpdateItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item);
  void UpdateDiscoverItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item);
  void UpdateQueueItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item);

  static void SetResponseType(const CStdString& typeToRetrieveForLog,bool& bRecommend, bool& bQueue, bool& bFeatured);
};
}
#endif /*BOXEEFEEDDIRECTORY_H_*/
