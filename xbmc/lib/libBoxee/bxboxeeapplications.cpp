//
// C++ Implementation: bxboxeeapplications
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeeapplications.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeApplications::BXBoxeeApplications() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeApplications::~BXBoxeeApplications()
{

}

void BXBoxeeApplications::Clear()
{
  m_applications.clear();
  m_applicationIdsMap.clear();
  m_bLoaded = false;
}

bool BXBoxeeApplications::Parse()
{
  TiXmlHandle docHandle(&m_doc);
  TiXmlNode* pTSChild = docHandle.FirstChild("applications").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  
  TiXmlElement *pChild = docHandle.FirstChild("applications").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeApps::Parse - FAILED to find <applications> node");
    return false;
  }
  
  m_applications.clear();
  m_applicationIdsMap.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("object") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_applications.push_back(obj);
        m_applicationIdsMap.insert(obj.GetID());
      }
    }
  }
  
  return true;
}

unsigned long BXBoxeeApplications::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeApplications::GetNumOfApplications() const
{
  return m_applications.size();
}

BXObject BXBoxeeApplications::GetApplication(int index) const
{
  if (index >= GetNumOfApplications())
  {
    return BXObject();
  }
  
  return m_applications[index];
}

bool BXBoxeeApplications::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeApplications::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

bool BXBoxeeApplications::IsApplicationIdExist(std::string appId)
{
  std::set<std::string>::iterator it;
  it=m_applicationIdsMap.find(appId);

  if(it == m_applicationIdsMap.end())
  {
    return false;
}
  else
  {
    return true;
  }
}

}

