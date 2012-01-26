//
// C++ Implementation: bxboxeesubscriptions
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeesubscriptions.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeSubscriptions::BXBoxeeSubscriptions() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeSubscriptions::~BXBoxeeSubscriptions()
{

}

void BXBoxeeSubscriptions::Clear()
{
  m_subscriptions.clear();
  m_bLoaded = false;
}

bool BXBoxeeSubscriptions::Parse()
{
  TiXmlHandle docHandle(&m_doc);
  TiXmlNode* pTSChild = docHandle.FirstChild("subscriptions").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  
  TiXmlElement *pChild = docHandle.FirstChild("subscriptions").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeSubscriptions::Parse - FAILED to find <applications> node");
    return false;
  }
  
  m_subscriptions.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("object") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_subscriptions.push_back(obj);
      }
    }
  }
  
  return true;
}

unsigned long BXBoxeeSubscriptions::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeSubscriptions::GetNumOfSubscriptions() const
{
  return m_subscriptions.size();
}

BXObject BXBoxeeSubscriptions::GetSubscription(int index) const
{
  if (index >= GetNumOfSubscriptions())
  {
    return BXObject();
  }
  
  return m_subscriptions[index];
}

bool BXBoxeeSubscriptions::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeSubscriptions::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

