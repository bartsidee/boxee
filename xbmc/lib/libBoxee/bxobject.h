// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxobject
//
// Description: 
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXOBJECT_H
#define BOXEEBXOBJECT_H

#include <string>
#include "bxstringmap.h"
#include "bxxmldocument.h"

namespace BOXEE {

class Extension
{
public:
  Extension() {}
  virtual ~Extension();
  void AddToMap(std::string key, std::string value);
  std::string GetFromMap(const std::string& key) const;
private:
  std::map<std::string, std::string> m_Map;
};

/**
*/
class BXObject : public BXStringMap, public BXXMLDocument 
{
public:
  BXObject(bool bValid=true);
  BXObject(const BXObject &obj);
  virtual ~BXObject();
	
	std::string GetType() const ;
	std::string GetID()   const ;
	std::string GetDescription() const;
	std::string GetName() const ;
	unsigned long GetVersion() const;

	void SetType(const std::string &strType);
	void SetID(const std::string &strID);
	void SetDescription(const std::string &strDesc);
	void SetName(const std::string &strName);

	virtual std::string ToXML() const;
	virtual bool FromXML(const TiXmlNode *pNode);
	virtual bool FromXML(const std::string &strXML);

	void SetValid(bool bValid);
	bool IsValid() const;

	virtual bool LoadFromString(const std::string &strDoc);
	virtual bool LoadFromFile(const std::string &strPath);
	virtual bool LoadFromURL(const std::string &strURL, const std::string &postData="");
	virtual bool LoadFromURL(const std::string &strURL, const ListHttpHeaders &listHeaders, const std::string &postData="");
	virtual bool Parse() ;
	
	bool DumpToFile(const std::string &strPath);
	
	int HasExtensions() const;
	const Extension* GetExtension(int pos) const;
	
  BXObject &operator=(const BXObject &other);
  
protected:
  
  int GetExtensions(const TiXmlNode *pNode);

	std::string  m_type;
	std::string  m_id;
	bool  m_bValid;
	
	std::vector<Extension*> m_extensions;
};

}

#endif
