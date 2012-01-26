
#include "BoxeeShortcutsDirectory.h"
#include "Settings.h"
#include "LocalizeStrings.h"

using namespace DIRECTORY;

CBoxeeShortcutsDirectory::CBoxeeShortcutsDirectory()
{
}

CBoxeeShortcutsDirectory::~CBoxeeShortcutsDirectory()
{
}

bool CBoxeeShortcutsDirectory::GetDirectory(const CStdString & strPath, CFileItemList & items)
{
  std::vector<CBoxeeShortcut>& shortcuts = g_settings.GetShortcuts().GetItems();
  for (size_t i = 0; i < shortcuts.size(); i++)
  {
    CFileItem item = shortcuts[i].AsFileItem();
    
    if (item.GetPropertyBOOL("isLabelLocalize"))
    {
      item.SetLabel(g_localizeStrings.Get(atoi(item.GetLabel().c_str())));
    }
    
    CFileItemPtr pItem(new CFileItem(item));
    //pItem->Dump();
    items.Add(pItem);
  }
  
  return true;
}
