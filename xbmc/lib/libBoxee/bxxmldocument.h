// Copyright © 2008 BOXEE. All rights reserved.
/*
* libBoxee
*
*
*
* --- property of boxee.tv
* 
*/
#ifndef BOXEEBXXMLDOCUMENT_H
#define BOXEEBXXMLDOCUMENT_H

#include <string>
#include <vector>
#include <map>
#include "tinyXML/tinyxml.h"

#include "bxcurl.h"
#include "bxcredentials.h"

namespace BOXEE {

typedef std::vector<std::string> ListHttpHeaders;

/**
	BXXMLDocument -
	handle XML documents.
	currently - wrapper class for tinyxml and tinyxpath.
*/
class BXXMLDocument{
public:
    BXXMLDocument();
    virtual ~BXXMLDocument();

	// parse an XML string
	virtual bool LoadFromString(const std::string &strDoc);

	// parse an XML file
	virtual bool LoadFromFile(const std::string &strPath);

	// load XML document from a url (HTTP).
	// listHeaders is a vector of http headers to add to the request
	// if postData is not empty- the request will use POST and send this data in the request's body
	virtual bool LoadFromURL(const std::string &strURL, const std::string &postData="", bool bUseCache = true);
	virtual bool LoadFromURL(const std::string &strURL, const ListHttpHeaders &listHeaders, const std::string &postData="", bool bUseCache = true);

	void SetCredentials(const BXCredentials &credentials);

	std::string GetRespHeader(const std::string &strHeader);
	virtual TiXmlElement * GetRoot();
	virtual TiXmlDocument& GetDocument();

	// this method should parse the loaded document into the derived class members
	virtual bool Parse() { return true; }	

	void SetVerbose(bool bVerbose);
	
	virtual int GetErrorCol();
	virtual int GetErrorRow();
	virtual const char* GetErrorDesc();

	std::string GetCredentialsUserName();
	std::string GetCredentialsPassword();
	
  int GetLastNetworkError() { return m_curl.GetLastRetCode(); }
  
  long GetLastRetCode();

protected:

	friend class Boxee;

	TiXmlDocument m_doc;
	BXCurl		  m_curl;
	BXCredentials m_credentials;
	bool		  m_bVerbose;

	std::map<std::string,std::string> m_headers;
};

}

#endif
