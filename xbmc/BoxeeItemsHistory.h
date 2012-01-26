#ifndef BOXEEITEMSHISTORY_H_
#define BOXEEITEMSHISTORY_H_

#include "FileItem.h"
#include "utils/Event.h"
#include <set>

class CBoxeeItemsHistory
{
public:

  CBoxeeItemsHistory();
  virtual ~CBoxeeItemsHistory();

  bool GetFilesHistory(CFileItemList& itemsHistoryList);
  bool AddItemToHistory(const CFileItem& item,CStdString historyId="");

  bool LoadItemsHistory();
  
  bool WaitForHistoryReady(DWORD dwTimeout);
  bool RemoveItemFromHistory(const CFileItem& historyItem);
  
protected:

  bool SaveItemsHistory();

  CFileItem* CreateItemForHistoryList(const CFileItem& item);

  std::set<CStdString> m_historyIdsOfItemsInHistoryList;
  CFileItemList m_itemsHistoryList;
  CCriticalSection m_itemsHistorylock;

  HANDLE m_historyReadyEvent;
};

#endif /*BOXEEITEMSHISTORY_H_*/
