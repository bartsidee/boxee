/*
 * BXFeedReader.h
 *
 *  Created on: Jul 16, 2009
 *      Author: yuvalt
 */

#ifndef BXFEEDREADER_H_
#define BXFEEDREADER_H_

#include <time.h>

namespace BOXEE
{

typedef enum { FEED_TYPE_RSS, FEED_TYPE_ATOM } BXFeedType;

class IBXFeedReader
{
public:
  virtual ~IBXFeedReader() { }
  virtual bool Parse() = 0 ;  

  // operations on RSS feed
  virtual std::string GetChannelDisplay() = 0;
  virtual std::string GetChannelDesc() = 0;
  virtual std::string GetChannelTitle() = 0;
  virtual std::string GetChannelLink() = 0;
  virtual std::string GetChannelPubDate() = 0;
  virtual std::string GetChannelImageLink() = 0;
  virtual std::string QueryChannel(const std::string &strXPath) = 0;
  virtual std::string QueryChannelAttribute(const std::string &strElement, const std::string &strAttribute) = 0;

  virtual int GetNumOfItems() = 0;

  // NOTE: items are not 0 based. first item is 1 not 0!
  virtual std::string GetItemDesc(int nItem) = 0;
  virtual std::string GetItemTitle(int nItem) = 0;
  virtual std::string GetItemPubDate(int nItem) = 0;
  virtual std::string GetItemLink(int nItem) = 0;

  // QueryItem is a generic way to query item information.
  // the string (second) parameter represents the xpath of the queried data - under the xml tree of the specific item. 
  // no need for the "text()" function - it will be concatenated.
  // example: QueryItem(2, "title") would teturn the second item's title (same as GetItemTitle(2))
  virtual std::string QueryItem(int nItem, const std::string &strItemData) = 0;
  virtual std::string QueryItemAttribute(int nItem, const std::string &strElement, const std::string &strAttribute) = 0;
  
  virtual std::string GetItemTag() = 0;
  
  virtual BXFeedType GetFeedType() = 0;
  
  virtual time_t ParseDateTime(const std::string& datetimestr) = 0;  
};

}

#endif /* BXFEEDREADER_H_ */
