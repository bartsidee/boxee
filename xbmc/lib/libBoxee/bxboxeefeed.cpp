//
// C++ Implementation: bxboxeefeed
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeefeed.h"
#include "logger.h"

namespace BOXEE {

BXBoxeeFeed::BXBoxeeFeed() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}


BXBoxeeFeed::~BXBoxeeFeed()
{
  
}

void BXBoxeeFeed::Clear()
{
  m_actions.clear();
  m_bLoaded = false;
}

bool BXBoxeeFeed::Parse()
{
  TiXmlHandle docHandle( &m_doc );
  TiXmlNode* pTSChild = docHandle.FirstChild( "boxeefeed" ).FirstChild( "timestamp" ).FirstChild().Node();
  
  if ( pTSChild )
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  else
  {
    pTSChild = docHandle.FirstChild( "actions" ).FirstChild( "timestamp" ).FirstChild().Node();
    
    if ( pTSChild )
    {
      m_timestamp = atoi(pTSChild->Value());
    }
  }
  
  // this class is used to parse both incoming feed and actions (single user's out feed)
  // so first - try parse as feed and if failed - as actions
  TiXmlElement *pChild = docHandle.FirstChild( "boxeefeed" ).Element();
  if (!pChild)
  {
    pChild = docHandle.FirstChild( "actions" ).Element();
  }
  
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"cant find feed/actions node");
    return false;
  }
  
  m_actions.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ( (pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("message") == 0)
    {
      BXGeneralMessage msg;
      if (msg.ParseFromActionNode(pMsgNode))
      {
				m_actions.push_back(msg);
      }
    }
  }
  
  return true;
}

unsigned long BXBoxeeFeed::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeFeed::GetNumOfActions() const
{
  return m_actions.size();
}

BXGeneralMessage BXBoxeeFeed::GetAction(int nIndex) const
{
  if (nIndex >= GetNumOfActions())
  {
    return BXGeneralMessage();
  }
  
  return m_actions[nIndex];
}

bool BXBoxeeFeed::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeFeed::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

