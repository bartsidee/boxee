
#include "GUIWindowBoxeeBrowseHistory.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"
#include "GUIDialogBoxeeMediaAction.h"

using namespace std;
using namespace BOXEE;

CHistoryWindowState::CHistoryWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{

}

void CHistoryWindowState::SortItems(CFileItemList &items)
{
 // no sort required
}

CGUIWindowBoxeeBrowseHistory::CGUIWindowBoxeeBrowseHistory()
: CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_HISTORY, "boxee_browse_history.xml")
{
  SetWindowState(new CHistoryWindowState(this));
}

CGUIWindowBoxeeBrowseHistory::~CGUIWindowBoxeeBrowseHistory()
{
  
}

CStdString CGUIWindowBoxeeBrowseHistory::CreatePath()
{
  return "history://all";
}

bool CGUIWindowBoxeeBrowseHistory::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseHistory::OnClick - Enter function with [iItem=%d] (browse)",iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
  {
    return true;
  }

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseHistory::OnClick - For [iItem=%d] going to handle item [label=%s][path=%s] (browse)",iItem,item.GetLabel().c_str(),item.m_strPath.c_str());

  item.Dump();

  ///////////////////////////////////////////////////////////////////////////////////////////////////
  // for all history items we want to open DialogBoxeeMediaAction for the RemoveFromHistory button //
  ///////////////////////////////////////////////////////////////////////////////////////////////////

  return CGUIDialogBoxeeMediaAction::ShowAndGetInput(&item);
}

