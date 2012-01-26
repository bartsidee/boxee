#ifndef RSSSOURCEMANAGER_H_
#define RSSSOURCEMANAGER_H_

#include <string>
#include <vector>
#include <queue>
#include <time.h>

#include "FileItem.h"
#include "utils/Thread.h"
#include "lib/libBoxee/bxfeedreader.h"

class TiXmlElement;
class TiXmlDocument;
class CFileItem;

struct CRssFeedPlaybackItem
{
  CRssFeedPlaybackItem()
  {
    priority = LINK;
    path = "";
    contentType = "";
    isFolder = false;
    isVideo = false;
    isFlash = false;
    isInternetStream = false;
    duration = "";
  }
  
  typedef enum { LINK, MEDIA_PLAYER_WITH_TYPE, MEDIA_CONTENT_WITH_TYPE, ENCLOSURE_URL_WITH_TYPE, BOXEE_KNOWN} Priority;
  
  Priority priority;
  CStdString path;
  CStdString contentType;
  CStdString lang;
  bool isFolder;
  bool isVideo;
  bool isFlash;
  bool isInternetStream;
  CStdString duration;
  
  inline bool operator<(const CRssFeedPlaybackItem aValue) const 
  {
    return ((int) priority) < ((int) aValue.priority);
  }
};

class CRssFeedParseContext
{
public:
  CRssFeedParseContext();
  void Clear();
  void Push(const CRssFeedPlaybackItem& item);
  const CRssFeedPlaybackItem& Top();
  size_t Size();
  
private:
  std::priority_queue<CRssFeedPlaybackItem> m_queue;
};

/**
 * The purpose of this class is to describe an RSS feed
 */
class CRssFeed 
{
public:
  CRssFeed();
  virtual ~CRssFeed();

  bool Init(const CStdString& strURL, const CStdString& originalURL);

  void GetItemList(CFileItemList &feedItems) {
    feedItems.Assign(items,true);
  }

  int GetIntervalSeconds() const {
    return m_iIntervalSeconds;
  }

  void SetIntervalSeconds(int intervalSeconds) {
    m_iIntervalSeconds = intervalSeconds;
  }

  time_t GetTimestamp() const {
    return m_Timestamp;
  }

  void SetTimestamp(time_t timestamp) {
    m_Timestamp = timestamp;
  }
  
  const CStdString& GetUrl() {
    return m_strURL;
  }
  
  void CheckFeed();
  bool ReadFeed();
  bool ParseItem(BOXEE::IBXFeedReader* reader, TiXmlElement* item_child, CFileItemPtr item, CRssFeedParseContext& parseContext);

  void SerializeToXml(TiXmlElement *pElement);
  void LoadFromXml(TiXmlElement *pElement);
  
  bool IsKnownContentType(CStdString contentType);

  static CStdString CleanDescription(const CStdString& strDescription);

private:
  
  bool ParseItemElements(BOXEE::IBXFeedReader* reader, TiXmlElement* child, CFileItemPtr item, CRssFeedParseContext& parseContext);
  
  bool IsPathToMedia(const CStdString& strPath );
  bool IsPathToThumbnail(const CStdString& strPath );

  bool HasBoxeeTypeAttribute(const TiXmlElement* item_child);
  void AddLinkItem(const TiXmlElement* item_child, CFileItemPtr item);

  CFileItemList items;
  
  // Lock that protects the feed vector
  CCriticalSection m_ItemVectorLock;

  CStdString m_strURL;
  CStdString m_originalURL;
  
  // Channel information
  CStdString m_strTitle;
  CStdString m_strAuthor;
  CStdString m_strLink;
  CStdString m_strGuid; // globally unique identifier of the item in the entire feed
  CStdString m_strDescription;
  CStdString m_strThumbnail;
  CStdStringArray m_arrCategories;
  bool m_bExplicit;

  int m_iNewItems;

  // The time in seconds at which the feed should be checked next time
  time_t m_Timestamp;
  // Holds the date on which the feed was last read
  time_t m_LastRead;
  // How often the feed should be checked
  int m_iIntervalSeconds;
};

// Sorting functor for STL::sort algorithm
bool RssFeedSortByTimestamp(const CRssFeed* lhs, const CRssFeed* rhs);



class CRssSourceManager : public CThread
{
public:
  CRssSourceManager();
  virtual ~CRssSourceManager();

  /**
   * Initializes the RSSManager by reading the list of feeds
   * from the rssfeeds.xml file located in the user space
   */
  virtual bool Init();
  virtual bool Stop();
  virtual bool Save();
  void AddFeed(CRssFeed* pFeed);
  CRssFeed*  AddFeed(const CStdString& strFeedURL);
  
  // TODO: Add RemoveFeed

private:
  std::vector<CRssFeed*> m_vecFeeds;
  
  // Lock that protects the feed vector
  CCriticalSection m_FeedVectorLock;

  // Event that causes the manager to wake up and check the feeds
  HANDLE m_WakeEvent;

  void Process();
  

};

#endif /*RSSSOURCEMANAGER_H_*/
