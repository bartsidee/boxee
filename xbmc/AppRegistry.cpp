
#include "AppRegistry.h"
#include "Util.h"
#include "tinyXML/tinyxml.h"
#include "StringUtils.h"
#include "Settings.h"
#include "File.h"
#include "SpecialProtocol.h"
#include "utils/log.h"

#include "utils/SingleLock.h"

/*
  <registry>
    <value id="momo">lomo</key>
    <value id="momo">lomo</key>
  </registry>

 */
CAppRegistry::CAppRegistry()
{
}

CAppRegistry::~CAppRegistry()
{
  m_data.clear();
}

void CAppRegistry::Load(const CAppDescriptor& desc, bool persistent)
{
  CSingleLock lock(m_lock);
  
  m_data.clear();

  if (!persistent)
  {
    m_dir = _P(g_settings.GetProfileUserDataFolder());
    CUtil::AddFileToFolder(m_dir, "apps", m_dir);
    ::CreateDirectory(m_dir, NULL);
    CUtil::AddFileToFolder(m_dir, desc.GetId(), m_dir);
    ::CreateDirectory(m_dir, NULL);
  }
  else
  {
    m_dir = "/.persistent";
    CUtil::AddFileToFolder(m_dir, "apps", m_dir);
    ::CreateDirectory(m_dir, NULL);
    CUtil::AddFileToFolder(m_dir, desc.GetId(), m_dir);
    ::CreateDirectory(m_dir, NULL);
  }

  m_fileName = m_dir;
  CUtil::AddFileToFolder(m_fileName, "registry.xml", m_fileName);

  // Registry was not found. Silently return.
  if (!XFILE::CFile::Exists(m_fileName))
  {
    return;
  }
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(m_fileName))
  {
    CLog::Log(LOGERROR, "Cannot load descriptor file for app: %s at row=%d/col=%d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow(), xmlDoc.ErrorCol());
    return;
  }

  TiXmlElement* rootElement = xmlDoc.RootElement();
  if (strcmpi(rootElement->Value(), "registry") != 0)
  {
    CLog::Log(LOGERROR, "Invalid registry file, root element should be registry: %s", m_fileName.c_str());
    return;
  }

  const TiXmlElement *node = rootElement->FirstChildElement("value");
  while (node)
  {
    const char* keyStr = node->Attribute("id");
    if (!keyStr)
    {
      CLog::Log(LOGWARNING, "Attribute id missing for element: %s", m_fileName.c_str());
      continue;
    }

    const char* valueStr = node->GetText();

    CStdString key = keyStr;
    CStdString value = "";
    if (valueStr)
    {
      value = valueStr;
    }

    if (m_data.find(key) == m_data.end())
    {
      RegistryValue values;
      m_data[key] = values;
    }

    RegistryValue& values = m_data[key];
    values.push_back(value);

    node = node->NextSiblingElement("value");
  }
}

CStdString CAppRegistry::Implode(const CStdString glue, const CStdString& key)
{
  CSingleLock lock(m_lock);

  if (m_data.find(key) == m_data.end())
    return StringUtils::EmptyString;

  CStdString result = "";
  RegistryValue& values = m_data[key];
  for (size_t i = 0; i < values.size(); i++)
  {
    if (result != "")
    {
      result += glue;
    }
    
    result += values[i];
  }
  
  return result;
}

bool CAppRegistry::Has(const CStdString& key)
{
  CSingleLock lock(m_lock);
  
  CStdString tempKey = key;
  size_t i = 0;
  
  // If we are asked for a variable with {}, take it from the vector, otherwise return the first
  int pos1 = key.Find("{");
  int pos2 = key.Find("}");
  if (pos1 >= 0 && pos2 > 0)
  {
    i = atoi(key.Mid(pos1+1, pos2 - pos1-1).c_str());
    tempKey = key.Mid(0, pos1);
  }
  
  if (m_data.find(tempKey) == m_data.end())
    return false;
  return true;
}

const CStdString& CAppRegistry::Get(const CStdString& key)
{
  CSingleLock lock(m_lock);

  CStdString tempKey = key;
  size_t i = 0;

  // If we are asked for a variable with {}, take it from the vector, otherwise return the first
  int pos1 = key.Find("{");
  int pos2 = key.Find("}");
  if (pos1 >= 0 && pos2 > 0)
  {
    i = atoi(key.Mid(pos1+1, pos2 - pos1-1).c_str());
    tempKey = key.Mid(0, pos1);
  }

  if (m_data.find(tempKey) == m_data.end())
    return StringUtils::EmptyString;

  RegistryValue& values = m_data[tempKey];
  if (i > values.size()-1)
    return StringUtils::EmptyString;

  return values[i];
}

void CAppRegistry::Set(const CStdString& key, const CStdString& value)
{
  CSingleLock lock(m_lock);

  if (m_data.find(key) == m_data.end())
  {
    RegistryValue values;
    m_data[key] = values;
  }

  RegistryValue& values = m_data[key];
  values.resize(1);
  values[0] = value;
  
  Save();
}

void CAppRegistry::Set(const CStdString& key, const char* value)
{
  Set(key, CStdString(value));
}

/**
 * Push a item to the end of the list with limit on the number of elements
 */
void CAppRegistry::PushBack(const CStdString& key, const CStdString& value, size_t limit)
{
  CSingleLock lock(m_lock);

  if (m_data.find(key) == m_data.end())
  {
    RegistryValue values;
    m_data[key] = values;
  }

  RegistryValue& values = m_data[key];
  values.push_back(value);

  if (limit > 0)
  {
    while (values.size() > limit)
    {
      values.pop_front();
    }
  }

  Save();
}

/**
 * Push a item to the beginning of the list with limit on the number of elements
 */
void CAppRegistry::PushFront(const CStdString& key, const CStdString& value, size_t limit)
{
  CSingleLock lock(m_lock);

  if (m_data.find(key) == m_data.end())
  {
    RegistryValue values;
    m_data[key] = values;
  }

  RegistryValue& values = m_data[key];
  values.push_front(value);

  if (limit > 0)
  {
    while (values.size() > limit)
    {
      values.pop_back();
    }
  }

  Save();
}

void CAppRegistry::Unset(const CStdString& key)
{
  CSingleLock lock(m_lock);

  CStdString tempKey = key;
  size_t i = 0;

  // If we are asked for a variable with {}, take it from the vector, otherwise return the first
  int pos1 = key.Find("{");
  int pos2 = key.Find("}");
  if (pos1 >= 0 && pos2 > 0)
  {
    i = atoi(key.Mid(pos1+1, pos2 - pos1-1).c_str());
    tempKey = key.Mid(0, pos1);
  }

  // Check if the key exists
  if (m_data.find(tempKey) == m_data.end())
    return;

  // Check that the index is not out of bounds
  RegistryValue& values = m_data[tempKey];
  if (i > values.size()-1)
    return;

  // Remove the item from the deque
  values.erase(values.begin() + i);
  
  // If the the map item has an empty deque, delete it 
  if (values.size() == 0)
  {
    m_data.erase(tempKey);
  }
  
  Save();
}

void CAppRegistry::Clear()
{
  CSingleLock lock(m_lock);

  m_data.clear();
  Save();
}

void CAppRegistry::Save()
{
  CSingleLock lock(m_lock);

  TiXmlElement rootElement("registry");
  CStdString key;
  for (RegistryDataIterator keyIter = m_data.begin(); keyIter != m_data.end(); keyIter++)
  {
    key = keyIter->first;
    RegistryValue& values = keyIter->second;
    
    for (size_t i = 0; i < values.size(); i++)
    {
      TiXmlElement valueElement("value");
      valueElement.SetAttribute("id", key.c_str());
      
      TiXmlText valueText(values[i]);
      valueElement.InsertEndChild(valueText);
      
      rootElement.InsertEndChild(valueElement);
    }
  }

  TiXmlDocument xmlDoc;
  xmlDoc.InsertEndChild(rootElement);

  ::CreateDirectory(m_dir, NULL);
  
  xmlDoc.SaveFile(m_fileName);
}
