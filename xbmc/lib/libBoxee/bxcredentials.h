// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxcredentials
//
// Description: 
//
//
// Author: roee vulkan,,, <vulkan@vulkan-laptop>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXCREDENTIALS_H
#define BOXEEBXCREDENTIALS_H

#include <string>

//#undef GetUserName

namespace BOXEE {

/**
	BXCredentials -
	this class describe user credentials for authentication with server side components.
*/
class BXCredentials{
public:
    BXCredentials();
    virtual ~BXCredentials();

	void SetUserName(const std::string &strName);
	void SetPassword(const std::string &strPassword);
    void SetProxy(const std::string &strProxy);
	void SetAuthKey(const std::string &strKey);
	void Clear();

	std::string GetUserName();
	std::string GetPassword();
	std::string GetAuthKey();

    std::string GetProxyAddress();
  
protected:
    std::string     m_strProxyAddress;    
  
	std::string		m_strUserName;
	std::string		m_strPassword;
	std::string		m_strAuthKey; // e.g. session id. server identification of the session.
};

}

#endif
