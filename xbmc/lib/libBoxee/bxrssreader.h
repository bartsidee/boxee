// Copyright © 2008 BOXEE. All rights reserved.
/*
* libBoxee
*
*
*
* --- property of boxee.tv
* 
*/
#ifndef BOXEEBXRSSREADER_H
#define BOXEEBXRSSREADER_H

#include "bxxmldocument.h"
#include "bxfeedreader.h"

namespace BOXEE {

class BXRSSReader : public IBXFeedReader {
public:
  BXRSSReader(BXXMLDocument& document);
  virtual ~BXRSSReader();

	// parse will query the values which are stored in members of this class.
	virtual bool Parse();	

	// operations on RSS feed
	virtual std::string GetChannelDisplay();
	virtual std::string GetChannelDesc();
	virtual std::string GetChannelTitle();
	virtual std::string GetChannelLink();
	virtual std::string GetChannelPubDate();
	virtual std::string GetChannelImageLink();
	virtual std::string QueryChannel(const std::string &strXPath);
	virtual std::string QueryChannelAttribute(const std::string &strElement, const std::string &strAttribute);

	virtual int GetNumOfItems();

	// NOTE: items are not 0 based. first item is 1 not 0!
	virtual std::string GetItemDesc(int nItem);
	virtual std::string GetItemTitle(int nItem);
	virtual std::string GetItemPubDate(int nItem);
	virtual std::string GetItemLink(int nItem);

	// QueryItem is a generic way to query item information.
	// the string (second) parameter represents the xpath of the queried data - under the xml tree of the specific item. 
	// no need for the "text()" function - it will be concatenated.
	// example: QueryItem(2, "title") would teturn the second item's title (same as GetItemTitle(2))
	virtual std::string QueryItem(int nItem, const std::string &strItemData);
	virtual std::string QueryItemAttribute(int nItem, const std::string &strElement, const std::string &strAttribute); 
  
  virtual std::string GetItemTag();  

  virtual BXFeedType GetFeedType();

  virtual time_t ParseDateTime(const std::string& datetimestr);
  
protected:
	// the following members will be initialized in the "Parse" stage.
	// only reason to make them members is not to parse xpath expressions each time they are
	// queried.
	// so basically every value which is frequently queried should become a member.
	int 		m_nItems;
	std::string	m_strChannelPubDate;
	std::string	m_strChannelTitle;
	std::string	m_strChannelLink;
	
	BXXMLDocument& m_document;
};

}

#endif
