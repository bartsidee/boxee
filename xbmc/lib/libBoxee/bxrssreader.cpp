
#include "bxrssreader.h"
#include "tinyxpath/xpath_static.h"
#include "logger.h"
#include "bxxmldocument.h"

namespace BOXEE {

BXRSSReader::BXRSSReader(BXXMLDocument& document) : m_nItems(0), m_document(document)
{
  //m_doc.SetConvertToUtf8(true);
}

BXRSSReader::~BXRSSReader()
{
}

std::string BXRSSReader::QueryChannel(const std::string &strElement) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  snprintf(szQuery,1023,"/rss/channel/%s/text()", strElement.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXRSSReader::QueryChannelAttribute(const std::string &strElement, const std::string &strAttribute) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  snprintf(szQuery,1023,"/rss/channel/%s/@%s", strElement.c_str(), strAttribute.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXRSSReader::GetChannelDesc() {
  return QueryChannel("description");
}

std::string BXRSSReader::GetChannelDisplay()
{
  return QueryChannel("boxee:display");
}

std::string BXRSSReader::GetChannelTitle() {
  return m_strChannelTitle;
}

std::string BXRSSReader::GetChannelLink() {
  return m_strChannelLink;
}

std::string BXRSSReader::GetChannelPubDate() {
  return m_strChannelPubDate;
}

std::string BXRSSReader::GetChannelImageLink() {
  return QueryChannel("image/url");
}

int BXRSSReader::GetNumOfItems() {
  return m_nItems;
}

std::string BXRSSReader::QueryItem(int nItem, const std::string &strItemData) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  // The XPath element count is 1 based, therefore idex is increased to maintain 0 based standard
  nItem = nItem + 1;

  snprintf(szQuery,1023,"/rss/channel/item[%d]/%s/text()", nItem, strItemData.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXRSSReader::QueryItemAttribute(int nItem, const std::string &strElement, const std::string &strAttribute) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  // The XPath element count is 1 based, therefore idex is increased to maintain 0 based standard
  nItem = nItem + 1;

  snprintf(szQuery,1023,"/rss/channel/item[%d]/%s/@%s", nItem, strElement.c_str(), strAttribute.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXRSSReader::GetItemDesc(int nItem) {
  return QueryItem(nItem, "description");
}

std::string BXRSSReader::GetItemTitle(int nItem) {
  return QueryItem(nItem, "title");
}

std::string BXRSSReader::GetItemPubDate(int nItem) {
  return QueryItem(nItem, "pubDate");
}

std::string BXRSSReader::GetItemLink(int nItem) {
  return QueryItem(nItem, "link");
}

bool BXRSSReader::Parse() {
  TiXmlNode *pRoot=m_document.GetDocument().FirstChild("rss");
  if (!pRoot)
    return false;

  m_nItems = TinyXPath::i_xpath_int(pRoot, "count(/rss/channel/item)");
  m_strChannelPubDate = TinyXPath::S_xpath_string (pRoot, "/rss/channel/pubDate/text()");
  m_strChannelTitle = TinyXPath::S_xpath_string (pRoot, "/rss/channel/title/text()");
  m_strChannelLink = TinyXPath::S_xpath_string (pRoot, "/rss/channel/link/text()");

  return true;
}

std::string BXRSSReader::GetItemTag()
{
  return "item";
}

BXFeedType BXRSSReader::GetFeedType()
{
  return FEED_TYPE_RSS;
}

time_t BXRSSReader::ParseDateTime(const std::string& strDate)
{
  struct tm pubDate = {0};
#ifdef _LINUX
  strptime(strDate.c_str(), "%a, %d %b %Y %H:%M:%S", &pubDate);
#else
  // WIN32, Use sscanf to manuall parse the string in a specified format
    
  // Fri, 21 Nov 2008 23:00:00 GMT
    
  char abDayOfWeek[3] = "";
  int iDayOfMonth = 0;
  char abMonth[3] = "";
  int iYear = 0;
  int iHour = 0;
  int iMinute = 0;
  int iSecond = 0;

  sscanf(strDate.c_str(), "%[a-z-A-Z], %d %s %d %d:%d:%d", abDayOfWeek, &iDayOfMonth, abMonth, &iYear, &iHour, &iMinute, &iSecond);

  pubDate.tm_mday = iDayOfMonth;

       if (_strnicmp(abDayOfWeek, "Sun", 3) == 0) { pubDate.tm_wday = 0;}
  else if (_strnicmp(abDayOfWeek, "Mon", 3) == 0) { pubDate.tm_wday = 1; }
  else if (_strnicmp(abDayOfWeek, "Tue", 3) == 0) { pubDate.tm_wday = 2; }
  else if (_strnicmp(abDayOfWeek, "Wed", 3) == 0) { pubDate.tm_wday = 3; }
  else if (_strnicmp(abDayOfWeek, "Thu", 3) == 0) { pubDate.tm_wday = 4; }
  else if (_strnicmp(abDayOfWeek, "Fri", 3) == 0) { pubDate.tm_wday = 5; }
  else if (_strnicmp(abDayOfWeek, "Sat", 3) == 0) { pubDate.tm_wday = 6; }

       if (_strnicmp(abMonth, "Jan", 3) == 0) { pubDate.tm_mon = 0;}
  else if (_strnicmp(abMonth, "Feb", 3) == 0) { pubDate.tm_mon = 1; }
  else if (_strnicmp(abMonth, "Mar", 3) == 0) { pubDate.tm_mon = 2; }
  else if (_strnicmp(abMonth, "Apr", 3) == 0) { pubDate.tm_mon = 3; }
  else if (_strnicmp(abMonth, "May", 3) == 0) { pubDate.tm_mon = 4; }
  else if (_strnicmp(abMonth, "Jun", 3) == 0) { pubDate.tm_mon = 5; }
  else if (_strnicmp(abMonth, "Jul", 3) == 0) { pubDate.tm_mon = 6; }
  else if (_strnicmp(abMonth, "Aug", 3) == 0) { pubDate.tm_mon = 7; }
  else if (_strnicmp(abMonth, "Sep", 3) == 0) { pubDate.tm_mon = 8; }
  else if (_strnicmp(abMonth, "Oct", 3) == 0) { pubDate.tm_mon = 9; }
  else if (_strnicmp(abMonth, "Nov", 3) == 0) { pubDate.tm_mon = 10; }
  else if (_strnicmp(abMonth, "Dec", 3) == 0) { pubDate.tm_mon = 11; }


  pubDate.tm_year = iYear - 1900;

  pubDate.tm_hour = iHour;
  pubDate.tm_min = iMinute;
  pubDate.tm_sec = iSecond;

#endif
  return mktime(&pubDate);  
}

}
