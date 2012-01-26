#ifndef BOXEEITEMSHISTORYDIRECTORY_H_
#define BOXEEITEMSHISTORYDIRECTORY_H_

#include "IDirectory.h"
#include "lib/libBoxee/bxmetadata.h"

#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxmessages.h"
#include "lib/libBoxee/bxboxeefeed.h"

#include "BoxeeFeedItemsLoader.h"
#include "BoxeeDatabaseDirectory.h"

#include "utils/Event.h"

namespace DIRECTORY
{
class CBoxeeMediaTypes
{
public:
	enum BoxeeMediaTypesEnums
	{
		ALL=0,
		VIDEO=1,
		AUDIO=2,
		PICTURE=3,
		NONE=4
	};
};

class CBoxeeItemsHistoryDirectory : public IDirectory
{
public:
  CBoxeeItemsHistoryDirectory();
  virtual ~CBoxeeItemsHistoryDirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);

private:
  static const char* GetBoxeeMediaTypeEnumAsString(CBoxeeMediaTypes::BoxeeMediaTypesEnums boxeeMediaTypeEnum);

  void UpdateHistoryItemProperties(CFileItemPtr item);

  CBoxeeMediaTypes::BoxeeMediaTypesEnums GetTypeToRetrieve(const CStdString& strPath);
  bool DoesFileMatchForRetrieve(CFileItem* item,CBoxeeMediaTypes::BoxeeMediaTypesEnums typeToRetrieve);
  
};
}
#endif /*BOXEEITEMSHISTORYDIRECTORY_H_*/
