// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxfriend
//
// Description: 
// an item in the friends list (friend).
// it can have any string attribute. its in general a string to string map (like BXGeneralMessage).
//
// a friend also has a BXGeneralMessage member that specifies the last action this friend did.
// it also has a vector of BXGeneralMessage as history of actions. 
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXFRIEND_H
#define BOXEEBXFRIEND_H

#include <vector>
#include <string>
#include <map>
#include "tinyXML/tinyxml.h"

#include "bxmessages.h"
#include "bxobject.h"

namespace BOXEE {

#define FRIEND_ATTRIB_NAME "name"
#define FRIEND_ATTRIB_PRESENCE "presence"
#define FRIEND_ATTRIB_THUMB "thumb"

/**
*/
class BXFriend{
public:
    BXFriend();
    virtual ~BXFriend();

	bool 		HasAttribute(const std::string &strAttrib);
	std::string	GetAttribute(const std::string &strAttrib);
	void 		SetAttribute(const std::string &strAttrib, const std::string &strValue);

	// a friend has to have a unique ID so that it can be indexed in a friends list.
	// if a unique id does not exist - the friend is invalid
	std::string	GetFriendId() const;
	void SetFriendId(const std::string &strFriendId);

	int					GetActionCount();
	BXGeneralMessage 	GetAction(int nIndex) const;

	bool ParseFromXml(const TiXmlNode *pNode);

    BXObject GetAsObject();

	// need to be called on startup
	static	void Initialize();

protected:
	BXObject						m_friendObj;
	std::vector<BXGeneralMessage> 	m_actions;

	// the lookup table will translate XML tag values to friends attribute values.
	// values that will not be found will be copies as is.
	static std::map<std::string,std::string> m_xmlLookupTable;
};

}

#endif
