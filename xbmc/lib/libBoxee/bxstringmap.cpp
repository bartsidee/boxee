//
// C++ Implementation: bxstringmap
//
// Description: 
//
//
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "bxstringmap.h"

namespace BOXEE {

BXStringMap::BXStringMap()
{
}


BXStringMap::~BXStringMap()
{
}

std::string BXStringMap::ToXML() const {

	std::string strResult ;

	StringMapValues::const_iterator iter;
	for (iter=m_values.begin(); iter!= m_values.end(); iter++) {
		TIXML_STRING strOut;
		std::string str1 = iter->first;
		std::string str2 = iter->second;
		TiXmlBase::EncodeString(iter->second, &strOut);
		strResult += "<"+iter->first+">"+strOut+"</"+iter->first+">";
	}

	return strResult;
}

bool BXStringMap::FromXML(const TiXmlNode *pNode) {
	if (!pNode || pNode->Type() != TiXmlNode::ELEMENT)
		return false;

	const TiXmlNode *pChild = 0;
	while (pChild = pNode->IterateChildren(pChild)) {
		const TiXmlNode *pValue = pChild->FirstChild();
		if ( pValue && pValue->Type() == TiXmlNode::TEXT) {
			SetValue(pChild->ValueStr(), pValue->ValueStr());
		}		
	}

	return true;
}

bool BXStringMap::HasValue(const std::string &strKey) const {
	return m_values.find(strKey) != m_values.end();
}

std::string BXStringMap::GetValue(const std::string &strKey) const {
	StringMapValues::const_iterator iter = m_values.find(strKey);
    if (iter == m_values.end())
		return "";

	return iter->second;
}

void BXStringMap::SetValue(const std::string &strKey, const std::string &strValue) {
	m_values[strKey] = strValue;
}


}
