
#include <stdio.h>
#include <stdlib.h>

#include "logger.h"
#include "bxcurl.h"
#include "bxxmldocument.h"
#include "bxexceptions.h"

namespace BOXEE {

BXXMLDocument::BXXMLDocument() : m_bVerbose(false)
{
}


BXXMLDocument::~BXXMLDocument()
{
  m_doc.Clear();
}

void BXXMLDocument::SetVerbose(bool bVerbose) {
  m_bVerbose = bVerbose;
}

TiXmlElement* BXXMLDocument::GetRoot()
{
  return m_doc.RootElement();
}

TiXmlDocument& BXXMLDocument::GetDocument()
{
  return m_doc;
}

bool BXXMLDocument::LoadFromString(const std::string &strDoc) {
  m_doc.Parse(strDoc.c_str(),0,TIXML_ENCODING_UNKNOWN);
  if (m_doc.Error())
    m_doc.Parse(strDoc.c_str(),0,TIXML_ENCODING_UTF8); 
  return !m_doc.Error() && Parse();
}

bool BXXMLDocument::LoadFromFile(const std::string &strPath) {
  std::string strPathCopy = strPath;
  int nPos = strPathCopy.find("://");
  if (nPos > 0) {
    strPathCopy.erase(0,nPos+2); // leave 1 "/" for the full path...
    if (strPathCopy[1] == '.') // unless its "." e.g. file://./test.xml
      strPathCopy.erase(0,1);
  }

  if (!m_doc.LoadFile(strPathCopy.c_str(),TIXML_ENCODING_UNKNOWN))
    m_doc.LoadFile(strPathCopy.c_str(),TIXML_ENCODING_UTF8);
  return  !m_doc.Error() && Parse();
}

std::string BXXMLDocument::GetRespHeader(const std::string &strHeader)
{
  return m_curl.GetHttpHeader(strHeader);
}

bool BXXMLDocument::LoadFromURL(const std::string &strURL, const std::string &postData, bool bUseCache)
{
  ListHttpHeaders listHeaders;
  return LoadFromURL(strURL, listHeaders, postData, bUseCache);
}

bool BXXMLDocument::LoadFromURL(const std::string &strURL, const ListHttpHeaders &listHeaders, const std::string &postData, bool bUseCache)
{
  if (strURL.substr(0,5) == "file:")
    return LoadFromFile(strURL);

  m_curl.SetVerbose(m_bVerbose);
  m_curl.SetCredentials(m_credentials);

  ListHttpHeaders headers;
  headers.push_back("Connection: keep-alive");
  m_curl.HttpSetHeaders(headers);

  if (listHeaders.size() > 0)
    m_curl.HttpSetHeaders(listHeaders);
  std::string strDoc;

  if (postData.empty())
    strDoc = m_curl.HttpGetString(strURL.c_str(), bUseCache);
  else
    strDoc = m_curl.HttpPostString(strURL.c_str(), postData);

  if (strDoc == "" && m_curl.GetLastRetCode() == 200)
    return true;

  // auth and network errors
  if (m_curl.GetLastRetCode() >= 400)
    return false;
  
  m_doc.Parse(strDoc.c_str(),0,TIXML_ENCODING_UNKNOWN);
  if (m_doc.Error())
    m_doc.Parse(strDoc.c_str(),0,TIXML_ENCODING_UTF8);

  return !m_doc.Error() && Parse();
}

void BXXMLDocument::SetCredentials(const BXCredentials &credentials) {
  m_credentials = credentials;
}

int BXXMLDocument::GetErrorCol()
{
  if (m_doc.Error())
  {
    return m_doc.ErrorCol();
  }

  return -1;
}

int BXXMLDocument::GetErrorRow()
{
  if (m_doc.Error())
  {
    return m_doc.ErrorRow();
  }

  return -1;
}

const char* BXXMLDocument::GetErrorDesc()
{
  if (m_doc.Error())
  {
    return m_doc.ErrorDesc();
  }

  return NULL;
}

std::string BXXMLDocument::GetCredentialsUserName()
{
  return m_credentials.GetUserName();
}

std::string BXXMLDocument::GetCredentialsPassword()
{
  return m_credentials.GetPassword();
}

long BXXMLDocument::GetLastRetCode()
{
  return m_curl.GetLastRetCode();
}

}
