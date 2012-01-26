// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxconfiguration
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXCONFIGURATION_H
#define BOXEEBXCONFIGURATION_H

#include <string>
#include <map>
#include "system.h"
namespace BOXEE {

/**
*/
class BXConfiguration{
public:
    virtual ~BXConfiguration();
	
	static BXConfiguration &GetInstance();

	bool 	Load();

	bool	HasParam(const std::string &strKey);
	int		GetIntParam(const std::string &strKey, int nDefault=0);
	double	GetDoubleParam(const std::string &strKey, double dDefault=0.0);
	std::string GetStringParam(const std::string &strKey, const std::string strDefault="");
	std::string GetURLParam(const std::string &strKey, const std::string strDefault="");
	
	bool Getline(std::string& fLine,FILE *fFile);
	
protected:
    BXConfiguration();

	std::map<std::string, std::string>	m_config;
};

}

#endif
