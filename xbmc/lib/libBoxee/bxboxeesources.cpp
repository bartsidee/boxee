//
// C++ Implementation: bxboxeeproducts
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxboxeesources.h"
#include "logger.h"

namespace BOXEE
{

BXBoxeeSources::BXBoxeeSources() : BXXMLDocument()
{
  m_timestamp=0;
  m_bLoaded = false;
}

BXBoxeeSources::~BXBoxeeSources()
{
 
}

void BXBoxeeSources::Clear()
{
  m_sources.clear();
  m_bLoaded = false;
}

bool BXBoxeeSources::Parse()
{
  TiXmlHandle docHandle(&m_doc);

  /*
  TiXmlNode* pTSChild = docHandle.FirstChild("sources").FirstChild("timestamp").FirstChild().Node();
  
  if (pTSChild)
  {
    m_timestamp = atoi(pTSChild->Value());
  }
  */
  
  TiXmlElement *pChild = docHandle.FirstChild("sources").Element();
  if (!pChild)
  {
    LOG(LOG_LEVEL_ERROR,"BXBoxeeSources::Parse - FAILED to find <sources> node");
    return false;
  }
  
  m_sources.clear();

  TiXmlNode *pMsgNode = 0;
  while ((pMsgNode = pChild->IterateChildren(pMsgNode)) != NULL)
  {
    if (pMsgNode->ValueStr().compare("source") == 0)
    {
      BXObject obj;

      TiXmlElement* sourceElement = (TiXmlElement*)pMsgNode;

      std::string strSourceId = sourceElement->Attribute("id") ? sourceElement->Attribute("id") : "";
      std::string strSourceType = sourceElement->Attribute("type") ? sourceElement->Attribute("type") : "";
      std::string strSourceGeo = sourceElement->Attribute("geo") ? sourceElement->Attribute("geo") : "";
      std::string strSourceThumb = sourceElement->Attribute("thumb") ? sourceElement->Attribute("thumb") : "";
      std::string strSourceName = sourceElement->GetText() ? sourceElement->GetText() : "";
      std::string strSourcePremium = sourceElement->Attribute("premium") ? sourceElement->Attribute("premium") : "false";
      std::string strSourceOffer = sourceElement->Attribute("offer") ? sourceElement->Attribute("offer") : "free";

      //LOG(LOG_LEVEL_DEBUG,"BXBoxeeSources::Parse - [sourceId=%s][name=%s][type=%s][geo=%s][thumb=%s][premium=%s][offer=%s] (source)",strSourceId.c_str(),strSourceName.c_str(),strSourceType.c_str(),strSourceGeo.c_str(),strSourceThumb.c_str(),strSourcePremium.c_str(),strSourceOffer.c_str());

      obj.SetValue("source_id", strSourceId);
      obj.SetValue("source_name", strSourceName);
      obj.SetValue("source_type", strSourceType);
      obj.SetValue("source_geo", strSourceGeo);
      obj.SetValue("source_thumb", strSourceThumb);
      obj.SetValue("source_premium", strSourcePremium);
      obj.SetValue("source_offer", strSourceOffer);

      m_sources.push_back(obj);
    }
  }
  
  return true;
}

unsigned long BXBoxeeSources::GetTimeStamp() const
{
  return m_timestamp;
}

int BXBoxeeSources::GetNumOfSources() const
{
  return m_sources.size();
}

void BXBoxeeSources::AddSource(BXObject source)
{
  m_sources.push_back(source);
}

BXObject BXBoxeeSources::GetSource(int index) const
{
  if (index >= GetNumOfSources())
  {
    return BXObject();
  }
  
  return m_sources[index];
}

bool BXBoxeeSources::IsLoaded()
{
  return m_bLoaded;
}

void BXBoxeeSources::SetLoaded(bool bLoaded)
{
  m_bLoaded = bLoaded;
}

}

