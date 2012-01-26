
#include "bxobject.h"
#include "bxmessages.h"
#include "logger.h"

namespace BOXEE {

BXObject::BXObject(bool bValid)
{
	m_bValid = bValid;
	m_extensions.clear();
}

BXObject::BXObject(const BXObject &obj)
{
  *this = obj;
}

BXObject::~BXObject()
{
  for (size_t i=0; i<m_extensions.size(); i++)
  {
    Extension *e = m_extensions[i]; 
    delete e;
  }
  m_extensions.clear();
}

std::string BXObject::GetName() const {
	return GetValue(MSG_KEY_NAME);
}

std::string BXObject::GetType() const {
	return m_type;
}

std::string BXObject::GetID() const{
	return m_id;
}

std::string BXObject::GetDescription() const {
	return GetValue(MSG_KEY_DESCRIPTION);
}

unsigned long BXObject::GetVersion() const {
	return atoi(GetValue(MSG_KEY_VERSION).c_str());
}

void BXObject::SetType(const std::string &strType) {
	m_type = strType;
}

void BXObject::SetID(const std::string &strID) {
	m_id = strID;
}

void BXObject::SetName(const std::string &strName) {
	SetValue(MSG_KEY_NAME,strName);
}

void BXObject::SetDescription(const std::string &strDesc) {
	SetValue(MSG_KEY_DESCRIPTION,strDesc);
}

std::string BXObject::ToXML() const {
	std::string strResult = "<object";
	if (!m_type.empty())
		strResult += " type=\""+m_type+"\"";

	if (!m_id.empty())
		strResult += " id=\""+m_id+"\"";

	strResult += ">";
	strResult += BXStringMap::ToXML();

	strResult += "</object>";
	return strResult;
}

bool BXObject::FromXML(const TiXmlNode *pNode) {
  if (!pNode || pNode->Type() != TiXmlNode::ELEMENT)
    return false;
  
  TiXmlElement *pElement = (TiXmlElement *)pNode;
  const char *szType = pElement->Attribute("type");
  const char *szId = pElement->Attribute("id");

	if (szType)
		SetType(szType);

	if (szId)
		SetID(szId);

	GetExtensions(pNode);
	return BXStringMap::FromXML(pNode);
}

int BXObject::GetExtensions(const TiXmlNode *pNode)
{
  const TiXmlNode *pExtensionsTag = 0;
  while (pExtensionsTag = pNode->IterateChildren(pExtensionsTag))
  {    
    if(pExtensionsTag->ValueStr() == "extensions")
    {
      const TiXmlNode *pExtensionTag = 0;
      while (pExtensionTag = pExtensionsTag->IterateChildren(pExtensionTag))
      {
        if(pExtensionTag->ValueStr() == "extension")
        {
          Extension* extension = new Extension();
        
          const char* provider = ((TiXmlElement*)pExtensionTag)->Attribute("provider");
          if(provider)
          {
            extension->AddToMap("provider",provider);
          }
          
          const TiXmlNode *pChild = 0;
          while (pChild = pExtensionTag->IterateChildren(pChild)) 
          {
            const TiXmlNode *pValue = pChild->FirstChild();
            
            if ( pValue && pValue->Type() == TiXmlNode::TEXT)
            {      
              extension->AddToMap(pChild->ValueStr(),pValue->ValueStr());
            }
          }
          
          m_extensions.push_back(extension);
        }
      }
      
      break; // There won't be another <extensions> tags
    }
  }
  
  return m_extensions.size();
}

int BXObject::HasExtensions() const
{
  return m_extensions.size();
}

std::string Extension::GetFromMap(const std::string& key) const
{
  std::string value = "";
  
  std::map<std::string, std::string>::const_iterator it = m_Map.find(key);
  
  if(it != m_Map.end())
  {
    value = (*it).second;
  }
  
	return value;
}

bool BXObject::IsValid() const {
	return m_bValid;
}

void BXObject::SetValid(bool bValid) {
	m_bValid = bValid;
}

bool BXObject::Parse() 
{
	return FromXML(m_doc.FirstChild("object"));
}

bool BXObject::FromXML(const std::string &strXML) 
{
  return LoadFromString(strXML) ;
}

bool BXObject::LoadFromString(const std::string &strDoc) 
{
	SetValid(BXXMLDocument::LoadFromString(strDoc));
	return IsValid();
}

bool BXObject::LoadFromFile(const std::string &strPath)
{
	SetValid(BXXMLDocument::LoadFromFile(strPath));
	return IsValid();
}

bool BXObject::LoadFromURL(const std::string &strURL, const std::string &postData)
{
  SetValid(BXXMLDocument::LoadFromURL(strURL,postData));
	return IsValid();
}

bool BXObject::LoadFromURL(const std::string &strURL, const ListHttpHeaders &listHeaders, const std::string &postData)
{
  SetValid(BXXMLDocument::LoadFromURL(strURL, listHeaders, postData));
	return IsValid();
}

bool BXObject::DumpToFile(const std::string &strPath)
{
	std::string strXML = ToXML();
	FILE *fOut = fopen(strPath.c_str(), "w");
	if (!fOut)
		return false;
    
	int nSize = strXML.size();
	int nOut = fwrite(strXML.c_str(), nSize, 1, fOut);

	if (nOut != 1)
	{
		fclose(fOut);
		return false;
	}

	fclose(fOut);
	return true;
}

BXObject &BXObject::operator=(const BXObject &other)
{
  m_type = other.m_type;
  m_id = other.m_id;
  m_bValid = other.m_bValid;
  m_values = other.m_values;
  m_curl = other.m_curl;
  m_doc = other.m_doc;
  m_credentials = other.m_credentials;
  m_bVerbose = other.m_bVerbose;
  m_headers = other.m_headers;
  
  for (size_t i=0; i< m_extensions.size(); i++)
  {
    delete m_extensions[i];
  }
  m_extensions.clear();

  for (size_t i=0; i< other.m_extensions.size(); i++)
  {
    Extension *e = new Extension;
    *e = *(other.m_extensions[i]);
    m_extensions.push_back(e);
  }
  return *this;
}
  
const Extension* BXObject::GetExtension(int pos) const
{
  if(pos > (m_extensions.size()-1))
  {
    return NULL;  
  }
  else
  {
    return m_extensions[pos];
  }
}

Extension::~Extension()
{
  m_Map.clear();
}
  
  void Extension::AddToMap(std::string key, std::string value)
{
  m_Map[key] = value; 
}

}
