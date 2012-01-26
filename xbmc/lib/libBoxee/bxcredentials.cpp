//
// C++ Implementation: bxcredentials
//
// Description: 
//
//
// Author: roee vulkan,,, <vulkan@vulkan-laptop>, (C) 2007
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include "bxcredentials.h"
#include "bxconfiguration.h"

namespace BOXEE {

BXCredentials::BXCredentials()
{
	m_strUserName = BXConfiguration::GetInstance().GetStringParam("User");
	m_strPassword = BXConfiguration::GetInstance().GetStringParam("Password");
}


BXCredentials::~BXCredentials()
{
}

void BXCredentials::SetUserName(const std::string &strName) {
	m_strUserName = strName;
}

void BXCredentials::SetPassword(const std::string &strPassword) {
	m_strPassword = strPassword;
}

void BXCredentials::SetProxyUserName(const std::string &strName) {
  m_strProxyUsername = strName;
}

void BXCredentials::SetProxyPassword(const std::string &strPassword) {
  m_strProxyPassword = strPassword;

}
void BXCredentials::SetAuthKey(const std::string &strKey) {
	m_strAuthKey = strKey;
}

void BXCredentials::SetProxy(const std::string &strProxy) {
  m_strProxyAddress = strProxy;
}
  
void BXCredentials::Clear()
{
	m_strUserName = "";
	m_strPassword = "";
	m_strAuthKey = "";
}

std::string BXCredentials::GetUserName() {
	return m_strUserName;
}

std::string BXCredentials::GetPassword() {
	return m_strPassword;
}

std::string BXCredentials::GetAuthKey() {
	return m_strAuthKey;
}

std::string BXCredentials::GetProxyAddress() {
  return m_strProxyAddress;
}
  
std::string BXCredentials::GetProxyUsername()
{
  return m_strProxyUsername;
}

std::string BXCredentials::GetProxyPassword()
{
  return m_strProxyPassword;
}

}
