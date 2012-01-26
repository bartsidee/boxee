#include "KeyboardManager.h"
#include "StringUtils.h"
#include "tinyXML/tinyxml.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"
#include "GUIDialogKeyboard.h"
#include "GUIWindowManager.h"

using namespace XBMC;

#define USER_KEYBOARDS_FILE "special://profile/keyboards.xml"

Keyboard KeyboardManager::m_dummy;

Keyboard::Keyboard() : m_hasCaps(false), m_rtl(false), m_nMaxKey(0)
{
}

Keyboard::~Keyboard()
{
}

CStdString Keyboard::GetQwertyCharacter(const CStdString &engEquiv) const
{
  std::map<CStdString, CStdString>::const_iterator iter = m_qwertyChars.find(engEquiv);
  if (iter != m_qwertyChars.end())
    return iter->second;
  return engEquiv;
}

CStdString Keyboard::GetCharacter(int nOrder, bool bCaps) const
{
  if (!m_hasCaps)
    bCaps = false;
  
  std::map<std::pair<int,bool>, CStdString>::const_iterator iter = m_chars.find(std::make_pair(nOrder,bCaps));
  if ( iter != m_chars.end())
    return iter->second;
  return StringUtils::EmptyString;
}

bool Keyboard::Load(const CStdString &strLang)
{
  CStdString path;
  path.Format("special://xbmc/language/%s/keyboard.xml",strLang.c_str());
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(PTH_IC(path)))
  {
    CLog::Log(LOGDEBUG, "unable to load keyboard for %s: %s at line %d", strLang.c_str(), xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueStr()!=CStdString("keyboard"))
  {
    CLog::Log(LOGERROR, "%s Doesn't contain <keyboard>", path.c_str());
    return false;
  }

  const char* langCode=pRootElement->Attribute("lang");
  if (langCode)
    m_langCode = langCode;

  const char* langDesc=pRootElement->Attribute("desc");
  if (langDesc)
    m_langDesc = langDesc;

  const char* nativeDesc=pRootElement->Attribute("native_desc");
  if (nativeDesc)
    m_nativeDesc = nativeDesc;
  else if (langDesc)
    m_nativeDesc = langDesc;
  
  const char* hasCapsAttr=pRootElement->Attribute("has_caps");
  if (hasCapsAttr && strcmp(hasCapsAttr,"true") == 0)
    m_hasCaps = true;

  const char* rtl=pRootElement->Attribute("rtl");
  if (rtl && strcmp(rtl,"true") == 0)
    m_rtl = true;
  
  m_langPath = strLang;
  
  m_nMaxKey = 0;
  const TiXmlElement *pChild = pRootElement->FirstChildElement("char");
  while (pChild)
  {
    // Load new style language file with id as attribute
    const char* attrId=pChild->Attribute("order");
    if (attrId && !pChild->NoChildren())
    {
      int id = atoi(attrId);
      if (id > m_nMaxKey)
        m_nMaxKey = id;
      
      const char* attrCaps=pChild->Attribute("caps");
      bool bCaps = (attrCaps && strcmp(attrCaps,"true") == 0);

      const char* attrQwerty=pChild->Attribute("qwerty_equiv");
      if (attrQwerty)
        m_qwertyChars[attrQwerty] = pChild->FirstChild()->Value();
        
      m_chars[std::make_pair(id,bCaps)] = pChild->FirstChild()->Value();
    }
    pChild = pChild->NextSiblingElement("char");
  }
  
  return true;
}

void Keyboard::Reset()
{
  m_langCode.clear();
  m_langDesc.clear();
  m_langPath.clear();
  m_nativeDesc.clear();
  m_hasCaps = false;
  m_rtl = false;
  m_nMaxKey = 0;
  m_chars.clear();
  m_qwertyChars.clear();
}

int Keyboard::GetCharCount() const
{
  return m_chars.size();
}

const CStdString &Keyboard::GetLangCode() const
{
  return m_langCode;
}

const CStdString &Keyboard::GetLangDesc() const
{
  return m_langDesc;
}

const CStdString &Keyboard::GetLangPath() const
{
  return m_langPath;
}

const CStdString &Keyboard::GetLangNativeDesc() const
{
  return m_nativeDesc;
}

bool  Keyboard::HasCaps() const
{
  return m_hasCaps;
}

bool  Keyboard::IsRTL() const
{
  return m_rtl;
}

int  Keyboard::GetMaxID() const
{
  return m_nMaxKey;
}

KeyboardManager::KeyboardManager()
{
  m_nActiveKeyboard = 0;
}

KeyboardManager::~KeyboardManager()
{
}

void KeyboardManager::Load()
{
  m_keyboards.clear();
  InstallKeyboard("English");
  
  TiXmlDocument xmlDoc;
  if (!xmlDoc.LoadFile(_P(USER_KEYBOARDS_FILE)))
    return;
  
  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueStr()!=CStdString("keyboards"))
    return;
  
  const TiXmlElement *pChild = pRootElement->FirstChildElement("keyboard");
  while (pChild)
  {
    // Load new style language file with id as attribute
    const char* attrLang=pChild->Attribute("lang");
    if (attrLang)
      InstallKeyboard(attrLang);
    pChild = pChild->NextSiblingElement("keyboard");
  } 
}
                           
void KeyboardManager::Save()
{
  TiXmlDocument doc;  
  TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );  
  doc.LinkEndChild( decl );  
  
  TiXmlElement * element = new TiXmlElement( "keyboards" );  
  doc.LinkEndChild( element );  
  
  for (size_t k=0; k<m_keyboards.size(); k++)
  {
    TiXmlElement * keyboard = new TiXmlElement( "keyboard" );  
    keyboard->SetAttribute("lang", m_keyboards[k].GetLangPath().c_str());
    
    element->LinkEndChild( keyboard );  
  }
  
  doc.SaveFile(_P(USER_KEYBOARDS_FILE));
}

void KeyboardManager::GetUserKeyboards(std::vector<Keyboard> &keyboards) const
{
  keyboards = m_keyboards;
}

const Keyboard &KeyboardManager::GetActiveKeyboard() const
{
  if (m_nActiveKeyboard < m_keyboards.size())
    return m_keyboards[m_nActiveKeyboard];
  return m_dummy;
}

void KeyboardManager::ToggleKeyboards()
{
  m_nActiveKeyboard++;
  if (m_nActiveKeyboard >= m_keyboards.size())
    m_nActiveKeyboard = 0;
  
  UpdateDialogs();
}

void KeyboardManager::UpdateDialogs()
{
  // refresh all keyboards
  static int keyboards[] = { WINDOW_DIALOG_KEYBOARD, WINDOW_DIALOG_BOXEE_SEARCH, WINDOW_DIALOG_BOXEE_GLOBAL_SEARCH, 0 };
  for (int i=0; keyboards[i]; i++)
  {
    CGUIDialogKeyboard *p = (CGUIDialogKeyboard *)g_windowManager.GetWindow(keyboards[i]);
    if (p)
      p->UpdateButtons();
  }  
}

void KeyboardManager::SetActiveKeyboard(const CStdString &lang)
{
  for (size_t k=0; k<m_keyboards.size(); k++)
  {
    const Keyboard &kb = m_keyboards[k];
    if (kb.GetLangCode() == lang || kb.GetLangDesc() == lang || kb.GetLangPath() == lang)
    {
      m_nActiveKeyboard = k;
      break;
    }
  }
  UpdateDialogs();
}

void KeyboardManager::InstallKeyboard(const CStdString &lang)
{
  for (size_t k=0; k<m_keyboards.size(); k++)
  {
    const Keyboard &kb = m_keyboards[k];
    if (kb.GetLangCode() == lang || kb.GetLangDesc() == lang || kb.GetLangPath() == lang)
      return;
  }
  
  Keyboard kb;
  if (kb.Load(lang))
    m_keyboards.push_back(kb);  
}

void KeyboardManager::UnInstallKeyboard(const CStdString &lang)
{
  if (lang.CompareNoCase("English") == 0)
    return; // english keyboard can not be removed (we always have at least this keyboard)
  
  std::vector<Keyboard>::iterator iter = m_keyboards.begin();
  while (iter != m_keyboards.end())
  {
    if ( iter->GetLangCode() == lang || iter->GetLangDesc() == lang || iter->GetLangPath() == lang)
    {
      m_keyboards.erase(iter);
      return;
    }
    
    iter++;
  }
}
