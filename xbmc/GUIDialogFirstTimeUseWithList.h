#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

#include "FileItem.h"

#define LIST_CTRL 240

class CGUIDialogFirstTimeUseWithList : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseWithList(int id, const CStdString& xmlFile, const CStdString& derivedClassStr);
  virtual ~CGUIDialogFirstTimeUseWithList();

  virtual bool OnMessage(CGUIMessage &message);

  bool HasSelectedItem();
  CFileItemPtr GetSelectedItem();
  int GetSelectedIndex();

protected:

  virtual void OnInitWindow();

  virtual bool FillListOnInit() = 0;
  virtual bool HandleListChoice() = 0;

  CFileItemPtr m_selectedItem;
  CFileItemList m_listItems;
  int m_selectedIndex;

};

#endif

