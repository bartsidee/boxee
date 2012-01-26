//
// C++ Implementation: bxfriendslist
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
#include "bxfriendslist.h"
#include "tinyxpath/xpath_processor.h"
#include "tinyxpath/xpath_static.h"
#include "logger.h"

namespace BOXEE {
FriendsListIterator::FriendsListIterator() {
}

FriendsListIterator::~FriendsListIterator() {
}

BXFriend FriendsListIterator::GetFriend() {
	return m_iterator->second;
}
std::string FriendsListIterator::GetFriendName() {
	return m_iterator->first;
}

bool FriendsListIterator::IsEndOfList() {
	return m_iterator == m_end;
}

bool FriendsListIterator::Next() {
	m_iterator++;
	if (m_iterator == m_end)
		return false;

	return true;
}

BXFriendsList::BXFriendsList() : m_bLoaded(false)
{
}


BXFriendsList::~BXFriendsList()
{
}

BXFriend BXFriendsList::GetFriend(const std::string &szName) {	
	return m_friendsList[szName];
}

unsigned long BXFriendsList::GetListTimeStamp() {
	return m_timeStamp;
}

FriendsListIterator	BXFriendsList::Iterate() {
	FriendsListIterator iter;
	iter.m_iterator = m_friendsList.begin();
	iter.m_end = m_friendsList.end();
	return iter;
}

bool BXFriendsList::AddFriend(const BXFriend &aFriend) {
	std::string strId = aFriend.GetFriendId();
	if (strId.empty())
		return false;

	m_friendsList[strId] = aFriend;

	return true;
}

void BXFriendsList::Clear()
{
	m_friendsList.clear();
	m_bLoaded = false;
}

int BXFriendsList::GetCount() {
	return m_friendsList.size();
}

bool BXFriendsList::IsLoaded()
{
	return m_bLoaded;
}

void BXFriendsList::SetLoaded(bool bLoaded)
{
	m_bLoaded = bLoaded;
}

std::string BXFriendsList::QueryFriendAttrib(int nFriend, const std::string &strAttrib) {
	char szQuery[1024];
	memset(szQuery,0,1024);
	snprintf(szQuery,1023,"/friends/friend[%d]/%s/text()", nFriend, strAttrib.c_str());
	return TinyXPath::S_xpath_string (m_doc.RootElement(), szQuery);
}

bool BXFriendsList::Parse() {
	TiXmlNode *pRoot=m_doc.FirstChild("friends");
	if (!pRoot)
		return false;
		
	m_timeStamp = TinyXPath::i_xpath_int(pRoot, "/friends/timestamp/text()");

	TiXmlNode *pFriendNode = NULL;
	for( pFriendNode = pRoot->FirstChild(); pFriendNode; pFriendNode = pFriendNode->NextSibling() ) {
		if ( pFriendNode->ValueStr() == "friend") {
			BXFriend aFriend;
			if (aFriend.ParseFromXml(pFriendNode)) {
				AddFriend(aFriend);
			}
		}
 	}
	return true;	
}

}
