//
// C++ Implementation: bxboxeeservices
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeeservices.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeServices::BXBoxeeServices() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeServices::~BXBoxeeServices()
{
 
}

void BXBoxeeServices::Clear()
{
  m_services.clear();
  m_bLoaded = false;
}

bool BXBoxeeServices::Parse()
{
  TiXmlHandle docHandle(&m_doc);
  TiXmlNode* pTSChild = docHandle.FirstChild("services").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  
  TiXmlElement *pChild = docHandle.FirstChild("services").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeServices::Parse - FAILED to find <applications> node");
    return false;
  }
  
  m_services.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("object") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_services.push_back(obj);
      }
    }
  }
  
  return true;
}

unsigned long BXBoxeeServices::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeServices::GetNumOfServices() const
{
  return m_services.size();
}

BXObject BXBoxeeServices::GetService(int index) const
{
  if (index >= GetNumOfServices())
  {
    return BXObject();
  }
  
  return m_services[index];
}

bool BXBoxeeServices::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeServices::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

