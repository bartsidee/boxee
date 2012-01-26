/*
 * BoxeeShortcut.cpp
 *
 *  Created on: Jul 13, 2009
 *      Author: yuvalt
 */


#include "BoxeeShortcut.h"
#include "Util.h"
#include "BoxeeItemLauncher.h"
#include "SpecialProtocol.h"
#include "XMLUtils.h"
#include "utils/log.h"
#include "utils/Builtins.h"
#include "File.h"
#include "GUIDialogOK2.h"

#define SHORTCUTS_FILENAME "special://profile/shortcuts.xml"

CBoxeeShortcut::CBoxeeShortcut()
{
  Init();
}

void CBoxeeShortcut::Init()
{
  m_name = "";
  m_isNameLocalize = false;
  m_path = "";
  m_command = "";
  m_thumbPath = "";
  m_isAdult = false;
  m_country = "all";
  m_countryAllow = true;
  m_isFolder = false;
  m_isReadOnly = false;

  m_type = "";
  m_boxeeId = "";
  m_linksList = NULL;
}

CBoxeeShortcut::CBoxeeShortcut(const CFileItem& fileItem)
{
  Init();
  m_name = fileItem.GetLabel();
  m_path = fileItem.m_strPath;
  m_thumbPath = fileItem.GetThumbnailImage();
  m_isAdult = fileItem.IsAdult();
  fileItem.GetCountryRestriction(m_country, m_countryAllow);
  m_isFolder = fileItem.m_bIsFolder;
  m_isReadOnly = fileItem.HasProperty("shortcut-readonly");
  
  if (fileItem.HasProperty("shortcut-command"))
  {
    m_command = fileItem.GetProperty("shortcut-command");
  }

  if (fileItem.HasProperty("boxee-mediatype"))
  {
    m_type = fileItem.GetProperty("boxee-mediatype");
  }

  if (fileItem.HasProperty("boxeeid"))
  {
    m_boxeeId = fileItem.GetProperty("boxeeid");
  }

  m_linksList = NULL;
  if (fileItem.HasLinksList())
  {
    m_linksList = new CFileItemList();

    if (m_linksList)
    {
      m_linksList->Copy(*(fileItem.GetLinksList()));
    }
  }
}

CBoxeeShortcut::CBoxeeShortcut(const CBoxeeShortcut& shortcut)
{
  Init();

  m_name = shortcut.GetName();
  m_isNameLocalize = shortcut.IsNameLocalize();
  m_path = shortcut.GetPath();
  m_command = shortcut.GetCommand();
  m_thumbPath = shortcut.GetThumbPath();
  m_isFolder = shortcut.GetIsFolder();
  m_isAdult = shortcut.GetIsAdult();
  m_country = shortcut.GetCountry();
  m_countryAllow = shortcut.GetCountryAllow();
  m_isReadOnly = shortcut.IsReadOnly();

  m_type = shortcut.GetType();
  m_boxeeId = shortcut.GetBoxeeId();

  if (shortcut.GetLinksList())
  {
    m_linksList = new CFileItemList();

    if (m_linksList)
    {
      m_linksList->Copy(*(shortcut.GetLinksList()));
    }
  }
}

CBoxeeShortcut::~CBoxeeShortcut()
{
  if (m_linksList)
  {
    delete m_linksList;
    m_linksList = NULL;
  }
}

bool CBoxeeShortcut::ToXML(TiXmlElement* rootElement)
{
  if (m_name.IsEmpty() || (m_path.IsEmpty() && m_command.IsEmpty() && m_boxeeId.IsEmpty()))
  {
    return false;
  }
  
  XMLUtils::SetString(rootElement, "name", m_name);
  TiXmlElement* nameElement = rootElement->FirstChildElement("name");
  if (nameElement && m_isNameLocalize)
  {
    nameElement->SetAttribute("localize","1");
  }

  XMLUtils::SetString(rootElement, "thumbnail", m_thumbPath);
  XMLUtils::SetBoolean(rootElement, "adult", m_isAdult);
  
  if (!m_path.IsEmpty())
  {
    XMLUtils::SetString(rootElement, "path", m_path);
    XMLUtils::SetBoolean(rootElement, "is-folder", m_isFolder);
  }
  
  if (!m_command.IsEmpty())
  {
    XMLUtils::SetString(rootElement, "command", m_command);
  }
  
  if (!m_type.IsEmpty())
  {
    XMLUtils::SetString(rootElement, "type", m_type);
  }

  if (!m_boxeeId.IsEmpty())
  {
    XMLUtils::SetString(rootElement, "boxeeid", m_boxeeId);
  }

  if (!m_country.IsEmpty())
  {
    XMLUtils::SetString(rootElement, "country", m_country);
    XMLUtils::SetBoolean(rootElement, "country-allow", m_countryAllow);
  }
  
  XMLUtils::SetBoolean(rootElement, "readonly", m_isReadOnly);
  
  return true;
}

bool CBoxeeShortcut::FromXML(TiXmlElement* rootElement)
{
  Init();
    
  if (!XMLUtils::GetString(rootElement, "name", m_name))
  {
    CLog::Log(LOGERROR, "Invalid shortcut item, name element not found");
    return false;
  }

  m_isNameLocalize = false;
  const TiXmlElement* nameElement = rootElement->FirstChildElement("name");
  if (nameElement)
  {
    if (nameElement->FirstChild())
    {
      CStdString localize = nameElement->Attribute("localize");
      localize.ToLower();

      if (localize == "1" || localize == "true")
      {
        m_isNameLocalize = true;
      }
    }
  }

  XMLUtils::GetString(rootElement, "path", m_path);
  XMLUtils::GetString(rootElement, "command", m_command);
  XMLUtils::GetString(rootElement, "thumbnail", m_thumbPath);
  XMLUtils::GetString(rootElement, "type", m_type);
  XMLUtils::GetString(rootElement, "boxeeid", m_boxeeId);
  XMLUtils::GetBoolean(rootElement, "adult", m_isAdult);
  XMLUtils::GetString(rootElement, "country", m_country);
  XMLUtils::GetBoolean(rootElement, "country-allow", m_countryAllow);
  XMLUtils::GetBoolean(rootElement, "is-folder", m_isFolder);
  XMLUtils::GetBoolean(rootElement, "readonly", m_isReadOnly);

  if (m_path.IsEmpty() && m_command.IsEmpty() && m_boxeeId.IsEmpty())
  {
    CLog::Log(LOGERROR,"Shortcut [name=%s] is invalid. path, command and boxeeId are empty. [path=%s][command=%s][boxeeId=%s] (shortcut)", m_name.c_str(), m_path.c_str(), m_command.c_str(), m_boxeeId.c_str());
    return false;    
  }
 
  return true;
}

CFileItem CBoxeeShortcut::AsFileItem() const
{
  CFileItem result(m_name);

  if (m_isNameLocalize)
  {
    result.SetProperty("isLabelLocalize", m_isNameLocalize);
  }

  result.m_strPath = m_path;
  result.m_bIsFolder = m_isFolder;
  result.SetThumbnailImage(m_thumbPath);
  result.SetProperty("shortcut-command", m_command);
  result.SetAdult(m_isAdult);
  result.SetCountryRestriction(m_country, m_countryAllow);

  if (m_isReadOnly)
  {
    result.SetProperty("shortcut-readonly", m_isReadOnly);
  }

  if (!m_boxeeId.IsEmpty())
  {
    result.SetProperty("type", m_type);
    result.SetProperty("boxeeid", m_boxeeId);
    result.SetProperty("IsBoxeeServerItem",true);
  }

  result.SetProperty("isshortcut", true);

  return result;
}

CStdString CBoxeeShortcut::GetName() const
{
  return m_name;
}

void CBoxeeShortcut::SetName(CStdString name)
{
  m_name = name;
}

bool CBoxeeShortcut::IsNameLocalize() const
{
  return m_isNameLocalize;
}

void CBoxeeShortcut::SetIsNameLocalize(bool isNameLocalize)
{
  m_isNameLocalize = isNameLocalize;
}

CStdString CBoxeeShortcut::GetPath() const
{
  return m_path;
}

void CBoxeeShortcut::SetPath(CStdString path)
{
  m_path = path;
}

CStdString CBoxeeShortcut::GetCommand() const
{
  return m_command;
}

void CBoxeeShortcut::SetCommand(CStdString command)
{
  m_command = command;
}

CStdString CBoxeeShortcut::GetThumbPath() const
{
  return m_thumbPath;
}

void CBoxeeShortcut::SetThumbPath(CStdString thumbPath)
{
  m_thumbPath = thumbPath;
}

CStdString CBoxeeShortcut::GetType() const
{
  return m_type;
}

void CBoxeeShortcut::SetType(CStdString type)
{
  m_type = type;
}

CStdString CBoxeeShortcut::GetBoxeeId() const
{
  return m_boxeeId;
}

void CBoxeeShortcut::SetBoxeeId(CStdString boxeeId)
{
  m_boxeeId = boxeeId;
}

const CFileItemList* CBoxeeShortcut::GetLinksList() const
{
  return m_linksList;
}

bool CBoxeeShortcut::GetIsAdult() const
{
  return m_isAdult;
}

void CBoxeeShortcut::SetIsAdult(bool isAdult)
{
  m_isAdult = isAdult;
}

bool CBoxeeShortcut::GetIsFolder() const
{
  return m_isFolder;
}

void CBoxeeShortcut::SetIsFolder(bool isFolder)
{
  m_isFolder = isFolder;
}

CStdString CBoxeeShortcut::GetCountry() const
{
  return m_country;
}

void CBoxeeShortcut::SetCountry(CStdString country)
{
  m_country = country;
}

bool CBoxeeShortcut::GetCountryAllow() const
{
  return m_countryAllow;
}

void CBoxeeShortcut::SetCountryAllow(bool countryAllow)
{
  m_countryAllow = countryAllow;
}

void CBoxeeShortcut::Launch()
{
  if (!m_command.IsEmpty())
  {
    CBuiltins::Execute(m_command);
  }
  else
  {
    CFileItem launchItem(m_path, m_isFolder);

    launchItem.SetLabel(m_name);
    launchItem.SetThumbnailImage(m_thumbPath);

    if (m_linksList)
    {
      launchItem.SetLinksList(m_linksList);
    }

    if (!m_boxeeId.IsEmpty())
    {
      launchItem.SetProperty("boxeeid", m_boxeeId);
    }

    CBoxeeItemLauncher::Launch(launchItem);
  }
}

bool CBoxeeShortcut::IsReadOnly() const
{
  return m_isReadOnly;
}

void CBoxeeShortcut::SetReadOnly(bool readOnly)
{
  m_isReadOnly = readOnly;
}

CBoxeeShortcutList::CBoxeeShortcutList()
{
}

bool CBoxeeShortcutList::Load()
{
  CStdString inputFile = SHORTCUTS_FILENAME;
  inputFile = _P(inputFile);
  
  if (!XFILE::CFile::Exists(inputFile))
  {
    CLog::Log(LOGWARNING,"CBoxeeShortcutList::Load - Shortcut file [%s] doesn't exist. Going to create a default one (shortcut)",inputFile.c_str());
    XFILE::CFile::Cache("special://xbmc/userdata/shortcuts.xml", inputFile);
  }

  TiXmlDocument doc;
  if (!doc.LoadFile(inputFile))
  {
    CLog::Log(LOGERROR, "Unable to load %s (row %i column %i)", inputFile.c_str(), doc.Row(), doc.Column());
    return false;
  }
  
  TiXmlElement *root = doc.RootElement();
  if (!root || strcmp(root->Value(), "shortcuts"))
  {
    CLog::Log(LOGERROR, "shortcuts.xml doesn't contain the <shortcuts> root element");
    return false;
  }

  m_items.clear();
  
  TiXmlElement *shortcut = root->FirstChildElement("shortcut");
  while (shortcut)
  {
    CBoxeeShortcut loadedShortcut;
    if (loadedShortcut.FromXML(shortcut))
    {
      m_items.push_back(loadedShortcut);
    }
    
    shortcut = shortcut->NextSiblingElement("shortcut");
  }
  
  return true;  
}

bool CBoxeeShortcutList::Save()
{
  TiXmlDocument doc;
  TiXmlElement xmlRootElement("shortcuts");
  TiXmlNode *rootNode = doc.InsertEndChild(xmlRootElement);
  if (!rootNode) return false;

  for (size_t i = 0; i < m_items.size(); i++)
  {
    TiXmlElement shortcutNode("shortcut");
    if (m_items[i].ToXML(&shortcutNode))
    {
      char index[5];
      sprintf(index,"%d",(int)i+1);

      shortcutNode.SetAttribute("index",index);
      rootNode->InsertEndChild(shortcutNode);
    }
  }

  CStdString outputFile = SHORTCUTS_FILENAME;
  outputFile = _P(outputFile);
  return doc.SaveFile(outputFile);  
}

std::vector<CBoxeeShortcut>& CBoxeeShortcutList::GetItems()
{
  return m_items;
}

bool CBoxeeShortcutList::AddShortcut(const CBoxeeShortcut& cut)
{
  CStdString shortcutPath = cut.GetPath();
  CStdString shortcutCommand = cut.GetCommand();
  CStdString shortcutBoxeeId = cut.GetBoxeeId();

  if (shortcutPath.IsEmpty() && shortcutCommand.IsEmpty() && shortcutBoxeeId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeShortcutList::AddShortcutt - CAN'T add shortcut with empty path, command and boxeeId. [path=%s][command=%s][boxeeId=%s] (shortcut)",shortcutPath.c_str(),shortcutCommand.c_str(),shortcutBoxeeId.c_str());
    return false;
  }

  // check if another shortcut with the same path or command already exists
  bool bFound = IsInShortcut(cut);

  if (!bFound)
  {
    m_items.insert(m_items.begin(),cut);
    CLog::Log(LOGDEBUG,"CBoxeeShortcutList::AddShortcutt - After adding CBoxeeShortcut with [name=%s][path=%s][command=%s][thumb=%s][boxeeId=%s]. [NumOfShortcuts=%d] (shortcut)",cut.GetName().c_str(),cut.GetPath().c_str(),cut.GetCommand().c_str(),cut.GetThumbPath().c_str(),cut.GetBoxeeId().c_str(),(int)m_items.size());

    Save();
    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeShortcutList::AddShortcutt - Shortcut with [path=%s][command=%s][boxeeId=%s] already exist (shortcut)",cut.GetPath().c_str(),cut.GetCommand().c_str(),cut.GetBoxeeId().c_str());
    CGUIDialogOK2::ShowAndGetInput(53700, 53311);
  return false;
}
}

bool CBoxeeShortcutList::RemoveShortcut(const CBoxeeShortcut& cut)
{
  bool wasRemoved = false;

  bool inShortcut = IsInShortcut(cut);;

  if (inShortcut)
  {
    CStdString shortcutPath = cut.GetPath();
    CStdString shortcutCommand = cut.GetCommand();
    CStdString shortcutBoxeeId = cut.GetBoxeeId();

    for (size_t i = 0; i < m_items.size(); i++)
    {
      if ((!shortcutPath.IsEmpty() && shortcutPath == m_items[i].GetPath()) || (!shortcutCommand.IsEmpty() && shortcutCommand == m_items[i].GetCommand()) || (!shortcutBoxeeId.IsEmpty() && shortcutBoxeeId == m_items[i].GetBoxeeId()))
      {
        m_items.erase(m_items.begin() + i);
        wasRemoved = true;
        break;
      }
    }
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeShortcutList::RemoveShortcut - Shortcut [path=%s][command=%s][boxeeId=%s] doesn't exist in the list (shortcut)",cut.GetPath().c_str(),cut.GetCommand().c_str(),cut.GetBoxeeId().c_str());
  }

  if (wasRemoved)
  {
    CLog::Log(LOGERROR,"CBoxeeShortcutList::RemoveShortcut - Shortcut [path=%s][command=%s][boxeeId=%s] was removed. Going to save shortcut.xml (shortcut)",cut.GetPath().c_str(),cut.GetCommand().c_str(),cut.GetBoxeeId().c_str());
    Save();
  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeShortcutList::RemoveShortcut - FAILED to remove shortcut [path=%s][command=%s][boxeeId=%s] (shortcut)",cut.GetPath().c_str(),cut.GetCommand().c_str(),cut.GetBoxeeId().c_str());
  }

  return wasRemoved;
}

bool CBoxeeShortcutList::IsInShortcut(const CBoxeeShortcut& cut)
{
  CStdString shortcutPath = cut.GetPath();
  CStdString shortcutCommand = cut.GetCommand();
  CStdString shortcutBoxeeId = cut.GetBoxeeId();

  if (shortcutPath.IsEmpty() && shortcutCommand.IsEmpty() && shortcutBoxeeId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeShortcutList::IsInShortcut - Both shortcut path, command  and boxeeId are empty. Going to return FALSE [path=%s][command=%s][boxeeId=%s] (shortcut)",shortcutPath.c_str(),shortcutCommand.c_str(),shortcutBoxeeId.c_str());
    return false;
  }

  bool inShortcut = (HasShortcutByPath(shortcutPath) || HasShortcutByCommand(shortcutCommand) || HasShortcutByBoxeeId(shortcutBoxeeId));

  return inShortcut;
}

bool CBoxeeShortcutList::HasShortcutByPath(const CStdString& strPath)
{
  if (strPath.IsEmpty())
  {
    return false;
  }

  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (strPath == m_items[i].GetPath())
    {
      return true;
  }
  }

  return false;
}

bool CBoxeeShortcutList::HasShortcutByCommand(const CStdString& strCommand)
{
  if (strCommand.IsEmpty())
  {
    return false;
  }

  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (strCommand == m_items[i].GetCommand())
    {
      return true;
  }
  }

  return false;
}

bool CBoxeeShortcutList::HasShortcutByBoxeeId(const CStdString& strBoxeeId)
{
  if (strBoxeeId.IsEmpty())
  {
    return false;
  }

  for (size_t i = 0; i < m_items.size(); i++)
  {
    if (strBoxeeId == m_items[i].GetBoxeeId())
    {
      return true;
    }
  }

  return false;
}

