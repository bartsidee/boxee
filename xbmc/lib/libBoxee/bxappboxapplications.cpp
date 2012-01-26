//
// C++ Implementation: bxappboxapplications
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxappboxapplications.h"
#include "logger.h"

namespace BOXEE
{

BXAppBoxApplications::BXAppBoxApplications() : BXXMLDocument()
{
  m_bLoaded = false;
}

BXAppBoxApplications::~BXAppBoxApplications()
{

}

void BXAppBoxApplications::Clear()
{
  m_bLoaded = false;
  m_appBoxApplicationIdsMap.clear();
}

bool BXAppBoxApplications::Parse()
{
  TiXmlElement* rootElement = m_doc.GetDocument()->RootElement();

  if (!rootElement)
  {
    LOG(LOG_LEVEL_ERROR,"BXAppBoxApplications::Parse - FAILED to get root element");
    return false;
  }

  int counter = 0;
  TiXmlElement* appNode = rootElement->FirstChildElement("app");
  while (appNode)
  {
    counter++;
    TiXmlElement* element = appNode->FirstChildElement("id");
    if (!element || (element && !element->FirstChild()) || (element && element->FirstChild() && !element->FirstChild()->Value()))
    {
      LOG(LOG_LEVEL_ERROR,"BXAppBoxApplications::Parse - [%d] - FAILED to read <id> element",counter);
      continue;
    }

    m_appBoxApplicationIdsMap.insert(element->FirstChild()->Value());

    appNode = appNode->NextSiblingElement("app");
  }

  return true;
}

int BXAppBoxApplications::GetNumOfApplication() const
{
  return m_appBoxApplicationIdsMap.size();
}

bool BXAppBoxApplications::IsLoaded()
{
  return m_bLoaded;
}

void BXAppBoxApplications::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

bool BXAppBoxApplications::IsApplicationIdExist(std::string appId)
{
  if (m_appBoxApplicationIdsMap.find(appId) != m_appBoxApplicationIdsMap.end())
  {
    return true;
}
  else
  {
    return false;
  }
}

}

