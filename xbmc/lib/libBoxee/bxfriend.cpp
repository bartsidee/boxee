//
// C++ Implementation: bxfriend
//
// Description: 
//
//
// Author: roee vulkan,,, <vulkan@vulkan-laptop>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "bxfriend.h"
#include "bxobject.h"
#include "logger.h"

namespace BOXEE {

std::map<std::string,std::string> BXFriend::m_xmlLookupTable;

BXFriend::BXFriend()
{
}

BXFriend::~BXFriend()
{
}

void BXFriend::Initialize() {
	// initialize the lookup table m_xmlLookupTable
	// ...
}

bool BXFriend::HasAttribute(const std::string &strAttrib) 
{
	return m_friendObj.HasValue(strAttrib);	
}

std::string	BXFriend::GetAttribute(const std::string &strAttrib) 
{
	return m_friendObj.GetValue(strAttrib);
}

void BXFriend::SetAttribute(const std::string &strAttrib, const std::string &strValue) 
{
	m_friendObj.SetValue(strAttrib,strValue);
}

std::string	BXFriend::GetFriendId() const 
{
	return m_friendObj.GetID();
}

void BXFriend::SetFriendId(const std::string &strFriendId) 
{
	m_friendObj.SetID(strFriendId);
}

int	BXFriend::GetActionCount() 
{
	return m_actions.size();
}

BXObject BXFriend::GetAsObject() 
{
	return m_friendObj;
}

BXGeneralMessage BXFriend::GetAction(int nIndex) const {
	if (nIndex > (int)m_actions.size() || nIndex < 0) {
		LOG(LOG_LEVEL_ERROR,"index %d out of bound for friend actions", nIndex);
		return BXGeneralMessage();
	}

	return m_actions[nIndex];
}

bool BXFriend::ParseFromXml(const TiXmlNode *pNode) {
	if (!pNode || !pNode->Type() == TiXmlNode::ELEMENT)
		return false;

	const TiXmlNode *pChild = 0;
	while ( (pChild = pNode->IterateChildren(pChild)) )
	{
		if ( pChild && pChild->Type() == TiXmlNode::ELEMENT && pChild->ValueStr().compare("object") == 0) 
		{
			m_friendObj.FromXML(pChild);
		}
		else if (pChild && pChild->Type() == TiXmlNode::ELEMENT && pChild->ValueStr().compare("actions") == 0)
		{
			const TiXmlNode *pMsg = pChild->FirstChild();
			while (pMsg) {
				if (pMsg->Type() == TiXmlNode::ELEMENT && pMsg->ValueStr() == "message") {
					BXGeneralMessage msg;
					if (msg.ParseFromActionNode(pMsg)) {
						m_actions.push_back(msg);
					}
				}

				pMsg = pMsg->NextSibling();
			}
		}
	}		
	
	return m_friendObj.IsValid();
}

}
