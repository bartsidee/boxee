#ifndef BOXEESHORTCUT_H_
#define BOXEESHORTCUT_H_

#include <vector>
#include <tinyXML/tinyxml.h>
#include "FileItem.h"

class CBoxeeShortcut
{
public:
  CBoxeeShortcut();
  CBoxeeShortcut(const CFileItem& fileItem);
  CBoxeeShortcut(const CBoxeeShortcut& shortcut);
  virtual ~CBoxeeShortcut();
  
  bool FromXML(TiXmlElement* xmlElement);  
  bool ToXML(TiXmlElement* xmlElement);
  
  CFileItem AsFileItem() const;
  
  CStdString GetName() const;
  void SetName(CStdString name);
  CStdString GetPath() const;
  void SetPath(CStdString path);
  CStdString GetCommand() const;
  void SetCommand(CStdString command);
  CStdString GetThumbPath() const;
  void SetThumbPath(CStdString thumbPath);
  bool GetIsFolder() const;
  void SetIsFolder(bool isFolder);
  bool GetIsAdult() const;
  void SetIsAdult(bool isAdult);
  CStdString GetCountry() const;
  void SetCountry(CStdString country);
  bool GetCountryAllow() const;
  void SetCountryAllow(bool countryAllow);
  bool IsReadOnly() const;
  void SetReadOnly(bool readOnly);

  bool IsNameLocalize() const;
  void SetIsNameLocalize(bool isNameLocalize);

  CStdString GetType() const;
  void SetType(CStdString type);
  CStdString GetBoxeeId() const;
  void SetBoxeeId(CStdString boxeeId);

  const CFileItemList* GetLinksList() const;

  void Launch();  

private:
  void Init();

  CStdString m_name;
  bool m_isNameLocalize;
  CStdString m_path;
  CStdString m_command;
  CStdString m_thumbPath;
  bool m_isFolder;
  bool m_isAdult;
  CStdString m_country;
  bool m_countryAllow;
  bool m_isReadOnly;

  CStdString m_type;
  CStdString m_boxeeId;

  CFileItemList* m_linksList;
};

class CBoxeeShortcutList
{
public:
  CBoxeeShortcutList();
  bool Load();
  bool Save();

  std::vector<CBoxeeShortcut>& GetItems();
  bool AddShortcut(const CBoxeeShortcut& cut);
  bool RemoveShortcut(const CBoxeeShortcut& cut);

  bool HasShortcutByPath(const CStdString& strPath);
  bool HasShortcutByCommand(const CStdString& strCommand);
  bool HasShortcutByBoxeeId(const CStdString& strBoxeeId);
  
  bool IsInShortcut(const CBoxeeShortcut& cut);

private:
  std::vector<CBoxeeShortcut> m_items;
};

#endif /* BOXEESHORTCUT_H_ */
