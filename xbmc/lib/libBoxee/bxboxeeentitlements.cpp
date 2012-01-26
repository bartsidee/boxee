//
// C++ Implementation: bxboxeeproducts
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeeentitlements.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeEntitlements::BXBoxeeEntitlements() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeEntitlements::~BXBoxeeEntitlements()
{
 
}

void BXBoxeeEntitlements::Clear()
{
  m_entitlements.clear();
  m_bLoaded = false;
}

bool BXBoxeeEntitlements::Parse()
{
  TiXmlHandle docHandle(&m_doc);

  /*
  TiXmlNode* pTSChild = docHandle.FirstChild("entitlements").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  */
  
  TiXmlElement *pChild = docHandle.FirstChild("entitlements").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeEntitlements::Parse - FAILED to find <entitlements> node");
    return false;
  }
  
  m_entitlements.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("entitlement") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_entitlements.push_back(obj);
      }
    }
  }
  
  return true;
}

unsigned long BXBoxeeEntitlements::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeEntitlements::GetNumOfEntitlements() const
{
  return m_entitlements.size();
}

BXObject BXBoxeeEntitlements::GetEntitlement(int index) const
{
  if (index >= GetNumOfEntitlements())
  {
    return BXObject();
  }
  
  return m_entitlements[index];
}

bool BXBoxeeEntitlements::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeEntitlements::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

