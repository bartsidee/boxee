#include "BoxeeBrowseMenuManager.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "FileItem.h"
#include "Resolution.h"
#include "SkinInfo.h"
#include "StringUtils.h"
#include "lib/libBoxee/bxgenresmanager.h"
#include "FileSystem/Directory.h"
#include "FileSystem/DirectoryCache.h"

#include <vector>

using namespace BOXEE;

#define BOXEE_BROWSE_MENU_TREE_FILE_NAME "boxee_browse_menu_tree.xml"

CBoxeeBrowseMenuManager::CBoxeeBrowseMenuManager()
{
  m_strPath = "";
  m_isInit = false;
}

CBoxeeBrowseMenuManager::~CBoxeeBrowseMenuManager()
{

}

CBoxeeBrowseMenuManager& CBoxeeBrowseMenuManager::GetInstance()
{
  static CBoxeeBrowseMenuManager boxeeBrowseMenuManager;
  return boxeeBrowseMenuManager;
}

bool CBoxeeBrowseMenuManager::Init(bool allowRebuild)
{
  if (!allowRebuild && m_isInit)
  {
    CLog::Log(LOGWARNING,"CBoxeeBrowseMenuManager::Init - [allowRebuild=%d=FALSE] and [isInit=%d=TRUE] therefore CAN'T build the menu. [MenuMapSize=%zu] (bm)",allowRebuild,m_isInit,m_menuMap.size());
    return false;
  }

  if (allowRebuild)
  {
    m_isInit = false;
  }

  if (!m_isInit)
  {
    m_isInit = BuildMenus();
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuManager::Init - after init [path=%s][NumOfMunus=%zu] (bm)",m_strPath.c_str(),m_menuMap.size());
  }

  return m_isInit;
}

bool CBoxeeBrowseMenuManager::BuildMenus()
{
  CSingleLock lock(m_menuMapLock);

  m_strPath = "";
  TiXmlDocument xmlDoc;

  RESOLUTION resToUse = RES_INVALID;
  m_strPath = g_SkinInfo.GetSkinPath(BOXEE_BROWSE_MENU_TREE_FILE_NAME,&resToUse);

  if (!xmlDoc.LoadFile(m_strPath))
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - FAILED to load XMl file [path=%s] (bm)",m_strPath.c_str());
    return false;
  }

  TiXmlElement* pRootElement = xmlDoc.RootElement();
  if (strcmp(pRootElement->Value(),"browse_menu"))
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - XMl file [path=%s] root element ISN'T <browse_menu> but <%s> (bm)",pRootElement->Value(),m_strPath.c_str());
    return false;
  }

  const TiXmlElement* pMenuElement = pRootElement->FirstChildElement("menu");
  if (!pMenuElement)
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - FAILED to find <menu> element. [path=%s] (bm)",m_strPath.c_str());
    return false;
  }

  m_menuMap.clear();

  while (pMenuElement)
  {
    if (strcmp(pMenuElement->Value(),"menu"))
    {
      CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuManager::BuildMenus - <%s> ISN'T <menu> element. continue to next <menu> element (bm)",pMenuElement->Value());
      pMenuElement = pMenuElement->NextSiblingElement("menu");
      continue;
    }

    if (!BuildMenu(*pMenuElement))
    {
      // ERROR log will be written from BuildMenu function
    }

    pMenuElement = pMenuElement->NextSiblingElement("menu");
  }

  return true;
}

bool CBoxeeBrowseMenuManager::BuildMenu(const TiXmlElement& pMenuElement)
{
  CStdString strMenuId = pMenuElement.Attribute("id");

  if (strMenuId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - <menu> element DOESN'T has \"id\" attribute. [path=%s] (bm)",m_strPath.c_str());
    return false;
  }

  if (m_menuMap.find(strMenuId) != m_menuMap.end())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - <menu> with [id=%s] already exist. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    return false;
  }

  CStdString strMenuLevel = pMenuElement.Attribute("level");

  /*
  if (strMenuLevel.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - <menu> element with [id=%s] DOESN'T has \"level\" attribute. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    //return false;
  }
  */

  CStdString strMenuTitle = pMenuElement.Attribute("title");

  if (strMenuTitle.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - <menu> element with [id=%s] DOESN'T has \"title\" attribute. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    //return false;
  }

  if (!StringUtils::IsNaturalNumber(strMenuTitle))
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - value of \"title\" attribute [%s] ISN'T numeric for <menu> element with [id=%s]. [path=%s] (bm)",strMenuTitle.c_str(),strMenuId.c_str(),m_strPath.c_str());
  }

  CFileItemPtr menuList = CFileItemPtr(new CFileItemList());
  int counter = 0;

  const TiXmlElement* pButtonElement = pMenuElement.FirstChildElement("button");
  while (pButtonElement)
  {
    CFileItemPtr buttonItem(new CFileItem());

    CStdString strButtonId = pButtonElement->Attribute("id");

    const TiXmlElement* pElement = pButtonElement->FirstChildElement();
    while (pElement)
    {
      const TiXmlNode* pValue = NULL;
      if (pElement->ValueStr() == "label")
      {
        pValue = pElement->FirstChild();
        if (pValue)
        {
          CStdString strLabel = pValue->ValueStr();
          if (StringUtils::IsNaturalNumber(pValue->ValueStr()))
          {
            strLabel = g_localizeStrings.Get(atoi(pValue->Value()));
          }

          buttonItem->SetLabel(strLabel);
        }
      }
      else if (pElement->ValueStr() == "command")
      {
        pValue = pElement->FirstChild();
        if (pValue)
        {
          buttonItem->m_strPath = pValue->ValueStr();
        }
      }
      else if (pElement->ValueStr() == "child")
      {
        pValue = pElement->FirstChild();
        if (pValue)
        {
          buttonItem->SetProperty("child",pValue->ValueStr());
        }
      }
      else if (pElement->ValueStr() == "clickable")
      {
        pValue = pElement->FirstChild();
        if (pValue)
        {
          buttonItem->SetProperty("isClickable",(strcmp(pValue->ValueStr().c_str(),"true") == 0) ? true : false);
        }
      }
      else if (pElement->ValueStr() == "window")
      {
        AddOpenInWindow(pElement,buttonItem);
      }
      else if (pElement->ValueStr() == "menu")
      {
        pValue = pElement->FirstChild();
        if (pValue)
        {
          buttonItem->SetProperty("updateMenu",true);
        }
      }

      pElement = pElement->NextSiblingElement();
    }

    buttonItem->SetProperty("id",strButtonId);
    buttonItem->SetProperty("isseparator",(strButtonId == "bt_separator"));

    if (!buttonItem->HasProperty("isClickable") && !buttonItem->m_strPath.IsEmpty())
    {
      buttonItem->SetProperty("isClickable",true);
    }

    counter++;
    CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuManager::BuildMenu - [%d] - adding button [label=%s][path=%s][id=%s][isseparator=%d] to menu [%s] (bm)",counter,buttonItem->GetLabel().c_str(),buttonItem->m_strPath.c_str(),buttonItem->GetProperty("id").c_str(),buttonItem->GetPropertyBOOL("isseparator"),strMenuId.c_str());
    ((CFileItemList*)menuList.get())->Add(buttonItem);

    pButtonElement = pButtonElement->NextSiblingElement("button");
  }

  const TiXmlElement* pPathElement = pMenuElement.FirstChildElement("path");

  if (((CFileItemList*)menuList.get())->IsEmpty() && (!pPathElement || pPathElement->ValueStr().empty()))
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::BuildMenus - FAILED to add <menu> with [id=%s] since [numOfButtons=%d=0] and no <path> element. [path=%s] (bm)",strMenuId.c_str(),((CFileItemList*)menuList.get())->Size(),m_strPath.c_str());
    return false;
  }

  menuList->SetProperty("id",strMenuId);
  menuList->SetProperty("level",strMenuLevel);
  menuList->SetProperty("title",g_localizeStrings.Get(atoi(strMenuTitle)));

  if (pPathElement && pPathElement->FirstChild())
  {
    menuList->m_strPath = pPathElement->FirstChild()->ValueStr();
  }

  const TiXmlElement* pParentElement = pMenuElement.FirstChildElement("parent");
  if (pParentElement && pParentElement->FirstChild())
  {
    menuList->SetProperty("parent",pParentElement->FirstChild()->ValueStr());
  }

  const TiXmlElement* pWindowElement = pMenuElement.FirstChildElement("window");
  if (pWindowElement && pWindowElement->FirstChild())
  {
    AddOpenInWindow(pWindowElement,menuList);
  }

  m_menuMap[strMenuId] = menuList;

  return true;
}

void CBoxeeBrowseMenuManager::ClearDynamicMenuButtons(const CStdString& strMenuId)
{
  CSingleLock lock(m_menuMapLock);

  std::map<CStdString,CFileItemPtr>::iterator it;
  CFileItemPtr menu;

  if (strMenuId.IsEmpty())
  {
    for(it=m_menuMap.begin();it != m_menuMap.end();++it)
    {
      menu = it->second;
      CStdString path = ((CFileItemList*)(menu.get()))->m_strPath;

      if (!path.IsEmpty())
      {
        // TMP LIMITATION: removing only buttons above the separator
        for (int i=0; i<((CFileItemList*)(menu.get()))->Size();i++)
        {
          if (((CFileItemList*)(menu.get()))->Get(i)->GetPropertyBOOL("isseparator"))
          {
            break;
          }

          ((CFileItemList*)(menu.get()))->Remove(i);
          i--;
        }

        ((CFileItemList*)(menu.get()))->SetProperty("IsFullyInitialized",false);

        CStdString menuContentPath = ((CFileItemList*)menu.get())->GetProperty("menuContentPath");

        if (!menuContentPath.IsEmpty())
        {
          g_directoryCache.ClearDirectory(menuContentPath);
        }
      }
    }

    return;
  }

  it = m_menuMap.find(strMenuId);

  if (it == m_menuMap.end())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::ClearMenu - FAILED to find menu [id=%s] in MenuMap. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    return;
  }

  menu = it->second;

  if (!menu.get())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::ClearMenu - got NULL menu for [id=%s]. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    return;
  }

  CStdString path = ((CFileItemList*)(menu.get()))->m_strPath;

  if (!path.IsEmpty())
  {
    // TMP LIMITATION: removing only buttons above the separator
    for (int i=0; i<((CFileItemList*)(menu.get()))->Size();i++)
    {
      if (((CFileItemList*)(menu.get()))->Get(i)->GetPropertyBOOL("isseparator"))
      {
        break;
      }

      ((CFileItemList*)(menu.get()))->Remove(i);
      i--;
    }

    ((CFileItemList*)(menu.get()))->SetProperty("IsFullyInitialized",false);

    CStdString menuContentPath = ((CFileItemList*)menu.get())->GetProperty("menuContentPath");

    if (!menuContentPath.IsEmpty())
    {
      g_directoryCache.ClearDirectory(menuContentPath);
    }
  }
}

CFileItemList* CBoxeeBrowseMenuManager::GetMenu(const CStdString& strMenuId)
{
  if (strMenuId.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::GetMenu - FAILED to get menu EMPTY [id=%s]. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
    return NULL;
  }

  CSingleLock lock(m_menuMapLock);

  CFileItemPtr menu;
  CStdString menuByPath = strMenuId;

  std::map<CStdString,CFileItemPtr>::iterator it = m_menuMap.find(strMenuId);

  if (it != m_menuMap.end())
  {
    ////////////////////////////////////////
    // going to get menu items by mennuId //
    ////////////////////////////////////////

    menu = it->second;

    if (!menu.get())
    {
      CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::GetMenu - got NULL menu for [id=%s]. [path=%s] (bm)",strMenuId.c_str(),m_strPath.c_str());
      return NULL;
    }

    if (((CFileItemList*)(menu.get()))->IsEmpty())
    //if (!menu->m_strPath.IsEmpty() || ((CFileItemList*)(menu.get()))->IsEmpty())
    {
      ////////////////////////////////////////////////
      // menu has NO items -> try get items by path //
      ////////////////////////////////////////////////
      menuByPath = menu->m_strPath;
    }
    else
    {
      ////////////////////
      // menu has items //
      ////////////////////

      if (menu->m_strPath.IsEmpty() || menu->GetPropertyBOOL("IsFullyInitialized"))
      {
        //////////////////////////////////////////////////////
        // menu has items and there is NO path -> return it //
        //////////////////////////////////////////////////////

        CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuManager::GetMenu - got menu with [size=%d] for [id=%s]. [path=%s] (bm)",((CFileItemList*)menu.get())->Size(),strMenuId.c_str(),m_strPath.c_str());
        return ((CFileItemList*)menu.get());
      }
      else
      {
        ////////////////////////////////////////////////////////
        // menu has items and also path -> try add path items //
        ////////////////////////////////////////////////////////

        menuByPath = menu->m_strPath;
      }
    }
  }
  else
  {
    menuByPath = strMenuId;
  }

  if (menuByPath.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::GetMenu - FAILED to get menu with EMPTY path [%s]. [path=%s] (bm)",menuByPath.c_str(),m_strPath.c_str());
    return NULL;
  }

  /////////////////////////////////////
  // going to get menu items by path //
  /////////////////////////////////////

  CFileItemList items;
  CStdString openInWindow = menu.get() ? menu->GetProperty("openInWindow") : "";
  GetMenuItemsByPath(menuByPath,items,openInWindow);

  if (items.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::GetMenu - got NO items [size=%d=0] for [path=%s]. [path=%s] (bm)",items.Size(),menuByPath.c_str(),m_strPath.c_str());
    return NULL;
  }

  CLog::Log(LOGDEBUG,"CBoxeeBrowseMenuManager::GetMenu - going to append [%d] items from [path=%s] to menu. [path=%s] (bm)",items.Size(),menuByPath.c_str(),m_strPath.c_str());

  if (!menu.get())
  {
    menu = CFileItemPtr(new CFileItemList());

    menu->SetProperty("id",menuByPath);
    //menu->SetProperty("title",g_localizeStrings.Get(atoi(strMenuTitle)));
  }

  if (((CFileItemList*)menu.get())->IsEmpty())
  {
    ((CFileItemList*)menu.get())->Append(items);
  }
  else
  {
    for (int i=0; i<items.Size(); i++)
    {
      ((CFileItemList*)menu.get())->AddFront(items.Get(i),i);
    }
  }

  menu->SetProperty("IsFullyInitialized",true);
  menu->m_strPath = menuByPath;
  menu->SetProperty("menuContentPath",items.m_strPath);

  m_menuMap[strMenuId] = menu;

  return ((CFileItemList*)menu.get());
}

bool CBoxeeBrowseMenuManager::GetFullMenuStructure(const CStdString& startMenuId,std::list<CFileItemList>& fullMenuStaructList)
{
  bool haveFullStructure = false;
  CStdString menuId = startMenuId;

  while (!haveFullStructure)
  {
    CFileItemList* menu = GetMenu(menuId);

    if (!menu)
    {
      CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::GetFullMenuStructure - FAILED to get menu for [id=%s] (bm)",menuId.c_str());
      haveFullStructure = true;
    }
    else
    {
      fullMenuStaructList.push_front(*menu);

      CStdString parentMenuId = menu->GetProperty("parent");
      if (parentMenuId.IsEmpty())
      {
        haveFullStructure = true;
      }
      else
      {
        menuId = parentMenuId;
      }
    }
  }

  return true;
}

bool CBoxeeBrowseMenuManager::AddOpenInWindow(const TiXmlElement* pElement,CFileItemPtr item)
{
  bool retVal = false;

  const TiXmlNode* pValue = pElement->FirstChild();

  if (pValue)
  {
    if (!StringUtils::IsNaturalNumber(pValue->ValueStr()))
    {
      CLog::Log(LOGERROR,"CBoxeeBrowseMenuManager::AddOpenInWindow - value of element <window> [%s] for [label=%s] ISN'T numeric. [path=%s] (bm)",pValue->ValueStr().c_str(),item->GetLabel().c_str(),item->m_strPath.c_str());
    }
    else
    {
      item->SetProperty("openInWindow",pValue->ValueStr());
      retVal = true;
    }
  }

  return retVal;
}

bool CBoxeeBrowseMenuManager::GetMenuItemsByPath(const CStdString& path, CFileItemList& items, const CStdString& openInWindow)
{
  bool retVal = false;

  retVal = DIRECTORY::CDirectory::GetDirectory(path, items);

  if (retVal && !openInWindow.IsEmpty())
  {
    for (int i=0; i<items.Size(); i++)
    {
      items[i]->SetProperty("openInWindow",openInWindow);
    }
  }

  return retVal;
}

