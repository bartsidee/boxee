// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxfriendslist
//
// Description: 
// implementation of a friends list
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXFRIENDSLIST_H
#define BOXEEBXFRIENDSLIST_H

#include "bxxmldocument.h"
#include "bxfriend.h"

namespace BOXEE {

typedef std::map<std::string, BXFriend> FriendsList;

//
// iterator to go over the friends list
//
class FriendsListIterator {
public:
	FriendsListIterator();
	~FriendsListIterator();

	BXFriend 	GetFriend();
	std::string GetFriendName();
	
	bool		IsEndOfList();
	bool		Next();

protected:
	friend class BXFriendsList;
	FriendsList::iterator	m_iterator;
	FriendsList::iterator	m_end;
};

/**
*/
class BXFriendsList : public BXXMLDocument
{
public:
    BXFriendsList();
    virtual ~BXFriendsList();

	virtual bool Parse() ;	

	BXFriend	GetFriend(const std::string &szName);
	unsigned long GetListTimeStamp();

	// get number of friends in list
	int 		GetCount();

	void Clear();

	bool IsLoaded();
	void SetLoaded(bool bLoaded=true);

	// get friends list iterator.
	// iterator will point to the begining of the list.
	// move throught the list using iterator's Next().
	FriendsListIterator	Iterate();

	bool AddFriend(const BXFriend &aFriend);

protected:
	friend class BXFriendsListDocument;
	std::string QueryFriendAttrib(int nFriend, const std::string &strAttrib);

	FriendsList		m_friendsList;
	time_t			m_timeStamp;
	bool			m_bLoaded;
};

}

#endif
