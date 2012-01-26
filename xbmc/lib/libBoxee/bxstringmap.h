// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxstringmap
//
// Description: 
//
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXSTRINGMAP_H
#define BOXEEBXSTRINGMAP_H

#include <string>
#include <map>
#include "tinyXML/tinyxml.h"

namespace BOXEE {

typedef std::map<std::string, std::string> StringMapValues;

/**
*/
class BXStringMap{
public:
    BXStringMap();
    virtual ~BXStringMap();

	bool 		HasValue(const std::string &strKey) const;
	virtual std::string	GetValue(const std::string &strKey) const;
	virtual void 		SetValue(const std::string &strKey, const std::string &strValue);

	virtual std::string ToXML() const;
	virtual bool FromXML(const TiXmlNode *pNode);

protected:
	StringMapValues		m_values;
};

}

#endif
