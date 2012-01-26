
#include "GUIWindowBoxeeBrowseQueue.h"
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

CQueueWindowState::CQueueWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{

}

void CQueueWindowState::SortItems(CFileItemList &items)
{
 // no sort required
}

CGUIWindowBoxeeBrowseQueue::CGUIWindowBoxeeBrowseQueue() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_QUEUE, "boxee_browse_queue.xml")
{
  SetWindowState(new CQueueWindowState(this));
}

CGUIWindowBoxeeBrowseQueue::~CGUIWindowBoxeeBrowseQueue()
{
  
}

CStdString CGUIWindowBoxeeBrowseQueue::CreatePath()
{
  CStdString strPath = "feed://queue";

  CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvShows::CreatePath, created path = %s (browse)", strPath.c_str());

  return strPath;
}

void CGUIWindowBoxeeBrowseQueue::SortItems(CFileItemList &items)
{
  // Don't allow sort in this screen
}

bool CGUIWindowBoxeeBrowseQueue::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_UPDATE)
  {
    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseQueue::OnMessage - GUI_MSG_UPDATE - Enter function with [SenderId=%d] (queue)(browse)",message.GetSenderId());

    if (message.GetSenderId() != GetID())
    {
      return true;
    }

    bool clearCache = message.GetParam1();

    LOG(LOG_LEVEL_DEBUG,"CGUIWindowBoxeeBrowseQueue::OnMessage - GUI_MSG_UPDATE - Enter function with [clearCache=%d] (queue)(browse)",clearCache);

    if (clearCache)
    {
      g_directoryCache.ClearSubPaths("feed://queue/");
    }
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}
