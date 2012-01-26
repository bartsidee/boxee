#ifndef BOXEEITEMSHISTORY_H_
#define BOXEEITEMSHISTORY_H_

#include "FileItem.h"
#include "utils/Event.h"
#include <set>

#define BOXEE_FILES_HISTORY_SIZE 500
#define BOXEE_BROWSE_HISTORY_SIZE 50

class CHistorySeperator
{
public:
  enum HistorySeperatorEnums
  {
    TODAY=0,
    YESTERDAY=1,
    OLDER=2,
    NUM_OF_HISTORY_SEPERATORS=3
  };
};


class CBoxeeItemsHistory
{
public:

  CBoxeeItemsHistory(CStdString filepath, int iHistorySize = BOXEE_FILES_HISTORY_SIZE);
  virtual ~CBoxeeItemsHistory();

  virtual bool GetFilesHistory(CFileItemList& itemsHistoryList);
  bool AddItemToHistory(const CFileItem& item,CStdString historyId="");

  bool LoadItemsHistory();
  
  bool WaitForHistoryReady(DWORD dwTimeout);
  bool RemoveItemFromHistory(const CFileItem& historyItem);
  bool ClearAllHistory();
  
  virtual int GetHistorySize();

protected:

  bool SaveItemsHistory();

  CFileItem* CreateItemForHistoryList(const CFileItem& item);

  CStdString getSeperatorLabel(time_t labelTime, CHistorySeperator::HistorySeperatorEnums seperatorType);

  std::set<CStdString> m_historyIdsOfItemsInHistoryList;
  CFileItemList m_itemsHistoryList;
  CCriticalSection m_itemsHistorylock;
  int m_iHistoryMaxSize;
  CStdString    m_strHistoryFilePath;

  HANDLE m_historyReadyEvent;
};

class CBoxeeBrowserHistory : public CBoxeeItemsHistory
{
public:

  CBoxeeBrowserHistory(CStdString filepath, int iHistorySize = BOXEE_BROWSE_HISTORY_SIZE);
  virtual ~CBoxeeBrowserHistory();

  bool AddLinkToHistory(const CStdString& link, const CStdString& label);
  CStdString GetDomain(CStdString link);
};

#endif /*BOXEEITEMSHISTORY_H_*/
