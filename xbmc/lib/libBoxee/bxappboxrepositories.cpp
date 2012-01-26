//
// C++ Implementation: bxappboxrepositories
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxappboxrepositories.h"
#include "logger.h"

namespace BOXEE
{

BXAppBoxRepositories::BXAppBoxRepositories() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXAppBoxRepositories::~BXAppBoxRepositories()
{

}

void BXAppBoxRepositories::Clear()
{
  m_repositories.clear();
  m_bLoaded = false;
}

bool BXAppBoxRepositories::Parse()
{
  TiXmlHandle docHandle(&m_doc);
  TiXmlNode* pTSChild = docHandle.FirstChild("repositories").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  
  TiXmlElement *pChild = docHandle.FirstChild("repositories").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXAppBoxRepositories::Parse - FAILED to find <repositories> node (repos)");
    return false;
  }
  
  m_repositories.clear();
  
  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("object") == 0)
    {
      BXObject obj;
      if (obj.FromXML(pMsgNode))
      {
        m_repositories.push_back(obj);
      }
    }
  }
  
  return true;
}

unsigned long BXAppBoxRepositories::GetTimeStamp() const
{
  return m_timestamp;
}

int BXAppBoxRepositories::GetNumOfRepositories() const
{
  return m_repositories.size();
}

BXObject BXAppBoxRepositories::GetRepository(int index) const
{
  if (index >= GetNumOfRepositories())
  {
    return BXObject();
  }
  
  return m_repositories[index];
}

bool BXAppBoxRepositories::IsLoaded()
{
  return m_bLoaded;
}

void BXAppBoxRepositories::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

