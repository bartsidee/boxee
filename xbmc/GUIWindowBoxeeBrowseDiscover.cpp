
#include "GUIWindowBoxeeBrowseDiscover.h"
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
#include "DirectoryCache.h"

using namespace std;
using namespace BOXEE;

CDiscoverWindowState::CDiscoverWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{

}

void CDiscoverWindowState::SortItems(CFileItemList &items)
{
 // no sort required
}

CGUIWindowBoxeeBrowseDiscover::CGUIWindowBoxeeBrowseDiscover()
: CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_DISCOVER, "boxee_browse_discover.xml")
{
  SetWindowState(new CDiscoverWindowState(this));
}

CGUIWindowBoxeeBrowseDiscover::~CGUIWindowBoxeeBrowseDiscover()
{
  
}

CStdString CGUIWindowBoxeeBrowseDiscover::CreatePath()
{
  CStdString strPath = "feed://share";
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvShows::CreatePath, created path = %s (browse)", strPath.c_str());
  return strPath;

}

bool CGUIWindowBoxeeBrowseDiscover::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_UPDATE)
  {
    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseDiscover::OnMessage - GUI_MSG_UPDATE - Enter function with [SenderId=%d] (rec)(browse)",message.GetSenderId());

    if (message.GetSenderId() != GetID())
    {
      return true;
    }

    bool clearCache = message.GetParam1();

    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseDiscover::OnMessage - GUI_MSG_UPDATE - Enter function with [clearCache=%d] (rec)(browse)",clearCache);

    if (clearCache)
    {
      g_directoryCache.ClearSubPaths("feed://share/");
      g_directoryCache.ClearSubPaths("feed://recommend/");
    }
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

