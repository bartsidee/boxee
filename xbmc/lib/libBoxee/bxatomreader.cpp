#include "bxatomreader.h"
#include "tinyxpath/xpath_static.h"
#include "logger.h"

namespace BOXEE {

BXAtomReader::BXAtomReader(BXXMLDocument& document) : m_nItems(0), m_document(document)
{
  //m_doc.SetConvertToUtf8(true);
}

BXAtomReader::~BXAtomReader()
{
}

std::string BXAtomReader::QueryChannel(const std::string &strElement) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  snprintf(szQuery,1023,"/feed/%s/text()", strElement.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXAtomReader::QueryChannelAttribute(const std::string &strElement, const std::string &strAttribute) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  snprintf(szQuery,1023,"/feed/%s/@%s", strElement.c_str(), strAttribute.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXAtomReader::GetChannelDesc() {
  return QueryChannel("subtitle");
}

std::string BXAtomReader::GetChannelDisplay()
{
  return QueryChannel("boxee:display");
}

std::string BXAtomReader::GetChannelTitle() {
  return m_strChannelTitle;
}

std::string BXAtomReader::GetChannelLink() {
  return m_strChannelLink;
}

std::string BXAtomReader::GetChannelPubDate() {
  return m_strChannelPubDate;
}

std::string BXAtomReader::GetChannelImageLink() {
  return QueryChannel("logo");
}

int BXAtomReader::GetNumOfItems() {
  return m_nItems;
}

std::string BXAtomReader::QueryItem(int nItem, const std::string &strItemData) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  // The XPath element count is 1 based, therefore idex is increased to maintain 0 based standard
  nItem = nItem + 1;

  snprintf(szQuery,1023,"/feed/entry[%d]/%s/text()", nItem, strItemData.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXAtomReader::QueryItemAttribute(int nItem, const std::string &strElement, const std::string &strAttribute) {
  char szQuery[1024];
  memset(szQuery,0,1024);
  // The XPath element count is 1 based, therefore idex is increased to maintain 0 based standard
  nItem = nItem + 1;

  snprintf(szQuery,1023,"/feed/entry[%d]/%s/@%s", nItem, strElement.c_str(), strAttribute.c_str());
  return TinyXPath::S_xpath_string (m_document.GetRoot(), szQuery);
}

std::string BXAtomReader::GetItemDesc(int nItem) {
  return QueryItem(nItem, "summary");
}

std::string BXAtomReader::GetItemTitle(int nItem) {
  return QueryItem(nItem, "title");
}

std::string BXAtomReader::GetItemPubDate(int nItem) {
  return QueryItem(nItem, "updated");
}

std::string BXAtomReader::GetItemLink(int nItem) {
  return QueryItemAttribute(nItem, "link", "href");
}

bool BXAtomReader::Parse() {
  TiXmlNode *pRoot=m_document.GetDocument().FirstChild("feed");
  if (!pRoot)
    return false;

  m_nItems = TinyXPath::i_xpath_int(pRoot, "count(/feed/entry)");
  m_strChannelPubDate = TinyXPath::S_xpath_string (pRoot, "/feed/updated/text()");
  m_strChannelTitle = TinyXPath::S_xpath_string (pRoot, "/feed/title/text()");
  m_strChannelLink = TinyXPath::S_xpath_string (pRoot, "/feed/link@href");

  return true;
}

std::string BXAtomReader::GetItemTag()
{
  return "entry";
}

BXFeedType BXAtomReader::GetFeedType()
{
  return FEED_TYPE_ATOM;
}

time_t BXAtomReader::ParseDateTime(const std::string& strDate)
{
  struct tm result = {0};
  sscanf(strDate.c_str(), "%d-%d-%dT%d:%d:%dZ", &result.tm_year, &result.tm_mon, &result.tm_mday, 
      &result.tm_hour, &result.tm_min, &result.tm_sec);
  result.tm_year -= 1900;
  result.tm_mon -= 1;
  return mktime(&result);    
}

}
