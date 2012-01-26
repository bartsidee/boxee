#ifndef __KEYBOARD_MANAGER_H__
#define __KEYBOARD_MANAGER_H__

#include <map>
#include <vector>
#include "StdString.h"

namespace XBMC
{
  
  class Keyboard
  {
  public:
    Keyboard();
    virtual ~Keyboard();

    bool Load(const CStdString &strLang);
    void Reset();
    
    CStdString GetCharacter(int nOrder, bool bCaps=false) const;
    CStdString GetQwertyCharacter(const CStdString &engEquiv) const;
    int        GetCharCount() const ;

    const CStdString &GetLangCode() const;
    const CStdString &GetLangDesc() const;
    const CStdString &GetLangPath() const;
    const CStdString &GetLangNativeDesc() const;
    bool  HasCaps()  const;
    bool  IsRTL()    const;
    int   GetMaxID() const;
    
  protected:
    CStdString m_langCode;
    CStdString m_langDesc;
    CStdString m_langPath;
    CStdString m_nativeDesc;
    bool       m_hasCaps;
    bool       m_rtl;
    int        m_nMaxKey;
    std::map<std::pair<int,bool>, CStdString> m_chars;
    std::map<CStdString, CStdString>                m_qwertyChars;
  };

  class KeyboardManager
  {
  public:
    KeyboardManager();
    virtual ~KeyboardManager();

    void Load();
    void Save();
    
    void GetUserKeyboards(std::vector<Keyboard> &keyboards) const;
    const Keyboard &GetActiveKeyboard() const;
    
    void ToggleKeyboards();
    void SetActiveKeyboard(const CStdString &lang);
    void InstallKeyboard(const CStdString &lang);
    void UnInstallKeyboard(const CStdString &lang);
    
    void UpdateDialogs();
    
  protected:
    size_t  m_nActiveKeyboard;
    std::vector<Keyboard> m_keyboards;

    static Keyboard m_dummy;
  };

}
#endif

