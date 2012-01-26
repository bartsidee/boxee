//
// C++ Implementation: bxmessages
//
// Description: 
//
//
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "bxmessages.h"
#include "logger.h"

namespace BOXEE {

std::map<std::string,std::string> BXGeneralMessage::m_xmlLookupTable;

BXGeneralMessage::BXGeneralMessage() 
{
}

BXGeneralMessage::~BXGeneralMessage() 
{
}

void BXGeneralMessage::Initialize() {
	// initialize lookup table
}

std::string BXGeneralMessage::GetMessageType() const {
	return m_type;
}

void BXGeneralMessage::SetMessageType(const std::string &strType) {
	m_type = strType;
}

std::string BXGeneralMessage::GetReferral() const{
  return m_referral;
}

void BXGeneralMessage::SetReferral(const std::string &strReferral) {
  m_referral = strReferral;
}

std::string BXGeneralMessage::GetSource() const
{
  return m_source;
}

void BXGeneralMessage::SetSource(const std::string &strSource)
{
  m_source = strSource;
}

int BXGeneralMessage::GetTimestamp() const {
	return atoi(GetValue(MSG_KEY_TIMESTAMP).c_str());
}

void BXGeneralMessage::SetTimestamp(int nTimestamp) {
	char szConv[64] = { 0 };
	sprintf(szConv, "%lu", nTimestamp);

	SetValue(MSG_KEY_TIMESTAMP, szConv);
}

void BXGeneralMessage::SetTimestamp(const std::string  &strTimestamp) {
	SetValue(MSG_KEY_TIMESTAMP, strTimestamp);
}

std::string BXGeneralMessage::ToXML() const {

	std::string strResult = "<message";
	if (!m_type.empty())
		strResult += " type=\""+m_type+"\"";

  if (!m_referral.empty())
      strResult += " referral=\""+m_referral+"\"";

  strResult += ">";
	strResult += BXStringMap::ToXML();

	for (unsigned int n=0; n<m_objects.size(); n++)
		strResult += m_objects[n].ToXML();

	strResult += "</message>";
	return strResult;
}

bool BXGeneralMessage::ParseFromActionNode(const TiXmlNode *pActionNode) {
	if (!pActionNode || pActionNode->Type() != TiXmlNode::ELEMENT)
		return false;

	TiXmlElement *pElement = (TiXmlElement *)pActionNode;
	const char *szType = pElement->Attribute("type");
  const char *szReferral = pElement->Attribute("referral");
  const char *szSource = pElement->Attribute("source");

	if (szType)
		SetMessageType(szType);
	
  if (szReferral)
    SetReferral(szReferral);

  if (szSource)
    SetSource(szSource);

	m_objects.clear();

	const TiXmlNode *pNode = NULL;
	for( pNode = pActionNode->FirstChild(); pNode; pNode = pNode->NextSibling() ) {
		if ( pNode->ValueStr() == "object") {
			BXObject aObj;
			if (aObj.FromXML(pNode)) {
				m_objects.push_back(aObj);
			}
		}
 	}

	return BXStringMap::FromXML(pActionNode);
}

int BXGeneralMessage::GetObjectCount() const {
	return m_objects.size();
}

BXObject BXGeneralMessage::GetObject(int nIndex) const  {
	if (nIndex < 0 || nIndex >= (int)m_objects.size())
		return BXObject(false);

	return m_objects[nIndex];
}

BXObject BXGeneralMessage::GetObjectByType(const std::string &strType) const {
	for (unsigned int n=0; n<m_objects.size(); n++) {
		BXObject obj=m_objects[n];
		if (obj.IsValid() && obj.GetType() == strType)
			return obj;
	}

	return BXObject(false);
}

BXObject BXGeneralMessage::GetObjectByID(const std::string &strID) const {
	for (unsigned int n=0; n<m_objects.size(); n++) {
		BXObject obj=m_objects[n];
		if (obj.IsValid() && obj.GetID() == strID)
			return obj;
	}

	return BXObject(false);
}

void BXGeneralMessage::AddObject(const BXObject &obj) {
	m_objects.push_back(obj);
}


}
