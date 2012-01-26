#ifndef BOXEEBROWSEMENUMANAGER_H_
#define BOXEEBROWSEMENUMANAGER_H_

#include "../guilib/tinyXML/tinyxml.h"
#include "StdString.h"
#include "FileItem.h"

#include <map>
#include <list>

class CBoxeeBrowseMenuManager
{
public:

  CBoxeeBrowseMenuManager();
  virtual ~CBoxeeBrowseMenuManager();

  static CBoxeeBrowseMenuManager& GetInstance();

  bool Init(bool allowRebuild = false);

  CFileItemList* GetMenu(const CStdString& strMenuId);
  void ClearDynamicMenuButtons(const CStdString& strMenuId = "");
  bool GetFullMenuStructure(const CStdString& startMenuId,std::list<CFileItemList>& fullMenuStaructList);

private:

  bool BuildMenus();
  bool BuildMenu(const TiXmlElement& pMenuElement);
  bool GetMenuItemsByPath(const CStdString& path, CFileItemList& items, const CStdString& openInWindow);

  bool AddOpenInWindow(const TiXmlElement* pElement,CFileItemPtr buttonItem);

  CStdString m_strPath;

  std::map<CStdString,CFileItemPtr> m_menuMap;
  CCriticalSection m_menuMapLock;

  bool m_isInit;
};

#endif
