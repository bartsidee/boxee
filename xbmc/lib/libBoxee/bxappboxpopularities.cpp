//
// C++ Implementation: bxappboxpopularities
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxappboxpopularities.h"
#include "logger.h"

namespace BOXEE
{

BXAppBoxPopularities::BXAppBoxPopularities() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXAppBoxPopularities::~BXAppBoxPopularities()
{

}

void BXAppBoxPopularities::Clear()
{
  m_popularities.clear();
  m_bLoaded = false;
}

bool BXAppBoxPopularities::Parse()
{
  TiXmlHandle docHandle(&m_doc);
  TiXmlNode* pTSChild = docHandle.FirstChild("popularity").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  
  TiXmlElement *pChild = docHandle.FirstChild("popularity").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXAppBoxPopularities::Parse - FAILED to find <popularity> node (appbox)");
    return false;
  }
  
  m_popularities.clear();

  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("object") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_popularities[obj.GetID()] = obj;
      }
    }
  }
  
  return true;
}

int BXAppBoxPopularities::GetNumOfPopularities() const
{
  return m_popularities.size();
}

unsigned long BXAppBoxPopularities::GetTimeStamp() const
{
  return m_timestamp;
}

std::string BXAppBoxPopularities::GetPopularityById(const std::string& id)
{
  std::string popularity = "0";

  std::map<std::string,BXObject>::iterator it = m_popularities.find(id);
  if (it != m_popularities.end())
  {
    popularity = (*it).second.GetValue("popularity");
  }
  else
  {
    LOG(LOG_LEVEL_WARNING,"BXAppBoxPopularities::GetPopularityById - FAILED to find popularity for [id=%s] (appbox)",id.c_str());
  }
  
  return popularity;
}

bool BXAppBoxPopularities::IsLoaded()
{
  return m_bLoaded;
}

void BXAppBoxPopularities::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

