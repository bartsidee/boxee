
#include "GUIWindowBoxeeBrowseSubscriptions.h"
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
#include "GUIWindowBoxeeBrowseTvEpisodes.h"

using namespace std;
using namespace BOXEE;

CGUIWindowBoxeeBrowseSubscriptions::CGUIWindowBoxeeBrowseSubscriptions()
: CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_SUBSCRIPTIONS, "boxee_browse_subscriptions.xml")
{

}

CGUIWindowBoxeeBrowseSubscriptions::~CGUIWindowBoxeeBrowseSubscriptions()
{

}

//CStdString CGUIWindowBoxeeBrowseSubscriptions::CreatePath()
//{
//  CStdString strPath;
//
//  strPath = "boxee://tvshows/myshows";
//
//  CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvShows::CreatePath, created path = %s (browse)", strPath.c_str());
//  return strPath;
//
//}

bool CGUIWindowBoxeeBrowseSubscriptions::OnClick(int iItem)
{
  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseSubscriptions::OnClick, item = %d (browse)", iItem);

  CFileItem item;

  if (!GetClickedItem(iItem, item))
    return true;

  item.Dump();

  if (item.GetPropertyBOOL("istvshowfolder"))
  {

    CStdString strPath;
    if (item.IsBoxeeDb())
    {
      // If clicked item, is boxeedb item (local only), get local episodes from the database
      CStdString strBoxeePath = item.m_strPath;

      if (CUtil::HasSlashAtEnd(strBoxeePath))
        CUtil::RemoveSlashAtEnd(strBoxeePath);

      CURI url(strBoxeePath);

      strPath = "boxee://tvshows/episodes?local=true&seriesId=";
      strPath += url.GetFileName();
    }
    else
    {
      // Get all episodes according to source filter
      strPath = "boxee://tvshows/episodes?seriesId=";
      strPath += item.GetProperty("boxeeid");
    }

    g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_TVEPISODES, strPath);
  }
  else
  {
    CGUIWindowBoxeeBrowse::OnClick(iItem);
  }

  return true;
}


