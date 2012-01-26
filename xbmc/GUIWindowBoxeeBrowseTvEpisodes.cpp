
#include "GUIWindowBoxeeBrowseTvEpisodes.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "BoxeeUtils.h"
#include "VideoInfoTag.h"
#include "URL.h"
#include "GUIDialogBoxeeDropdown.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "Application.h"
#include "GUIWindowManager.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_FULL_EPISODES 110
#define BUTTON_FULL_CLIPS 120

#define BUTTON_SEASONS   140
#define BUTTON_SUBSCRIBE 150
#define BUTTON_FREE      151
#define BUTTON_SHORTCUT  160

#define SEASONS_LIST 540

#define CUSTOM_FREE_FILTER 615

#define NOTIFICATION_APPEARANCE_IN_SEC   5000

#define TV_EPISODES_WINDOW_SHORTCUT_COMMAND   "ActivateWindow(10483,%s)"

#define NEWEST_SET_FLAG "newestfirst-set"
#define SEASONS_LABEL_FLAG "seasons-label"

#define FREE_ONLY_SET_FLAG "free-set"
#define FREE_ONLY_LABEL_FLAG "free-label"


void SubscribeJob::Run()
{
  if (m_bSubscribe)
  {
    m_bJobResult = BoxeeUtils::Subscribe(m_type, m_id, m_strShowTitle);
  }
  else
  {
    m_bJobResult = BoxeeUtils::Unsubscribe(m_type, m_id, m_strShowTitle);
  }
}

//////////////////////////////////////////////////////////////////////////////

CEpisodesWindowState::CEpisodesWindowState(CGUIWindow* pWindow) : CBrowseWindowState(pWindow)
{
  m_iSeason = -1;
  m_bFreeOnly= false;
  m_bSubscribed = false;
  m_bHasShortcut = false;
  m_bLocal = false;
  m_bRemote = false;

  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort("2", SORT_METHOD_EPISODE, SORT_ORDER_DESC, g_localizeStrings.Get(53529), ""));
  m_vecSortMethods.push_back(CBoxeeSort("1", SORT_METHOD_EPISODE, SORT_ORDER_ASC, g_localizeStrings.Get(53528), ""));

  SetSort(m_vecSortMethods[0]);
}

CStdString CEpisodesWindowState::CreatePath()
{
  CStdString strPath = "boxee://tvshows/episodes/";

  std::map<CStdString, CStdString> mapOptions;

  if (m_bLocal)
  {
    mapOptions["local"] = "true";
  }

  if (m_bRemote)
  {
    mapOptions["remote"] = "true";
  }

  mapOptions["seriesId"] =  CStdString(m_strShowId);

  if (m_iSeason > 0)
  {
    CStdString seasonOption;
    seasonOption.Format("%d",m_iSeason);
    mapOptions["season"] = seasonOption;
  }

  strPath += BoxeeUtils::BuildParameterString(mapOptions);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvEpisodes::CreatePath, created path = %s (browse)", strPath.c_str());
  return strPath;

}

void CEpisodesWindowState::InitState(const CStdString& strPath)
{
  Reset();
  CBrowseWindowState::InitState(strPath);

  m_strPath = strPath;

  if (!m_strPath.IsEmpty())
  {
    CURL url(m_strPath);

    std::map<CStdString, CStdString> mapOptions = url.GetOptionsAsMap();
    m_strShowId = mapOptions["seriesId"];

    CStdString local;
    m_bLocal = DIRECTORY::CBoxeeServerDirectory::HasOption(mapOptions,"local", local);

    CStdString remote;
    m_bRemote = DIRECTORY::CBoxeeServerDirectory::HasOption(mapOptions,"remote", remote);

    if (m_strShowId.IsEmpty())
    {
      CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage - GUI_MSG_WINDOW_INIT - no TV show id, should not happen (browse)");
      return;
    }

    // check subscription status
    if (BoxeeUtils::IsSubscribe(m_strShowId))
    {
      m_bSubscribed = true;
      m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53522));
    }
    else
    {
      m_bSubscribed = false;
      m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53521));
    }

    // check if item has shortcut
    CBoxeeShortcut cut;
    CStdString command;
    CStdString str = TV_EPISODES_WINDOW_SHORTCUT_COMMAND;
    command.Format(str.c_str(), m_strPath);
    cut.SetCommand(command);

    if (g_settings.GetShortcuts().IsInShortcut(cut))
    {
      m_bHasShortcut = true;
    }
    else
    {
      m_bHasShortcut = false;
    }
  }
}

bool CEpisodesWindowState::HasShortcut()
{
  return m_bHasShortcut;
}

bool CEpisodesWindowState::OnSeasons(std::set<int>& setSeasons)
{
  CFileItemList seasons;

  CFileItemPtr allSeasonsItem (new CFileItem(g_localizeStrings.Get(53508)));
  allSeasonsItem->SetProperty("value", -1);
  seasons.Add(allSeasonsItem);

  std::set<int>::iterator it = setSeasons.begin();
  while (it != setSeasons.end())
  {
    CStdString label;
    label.Format("SEASON %d", (int) *it);

    CFileItemPtr seasonItem (new CFileItem(label));
    seasonItem->SetProperty("value", *it);
    seasons.Add(seasonItem);
    it++;
  }

  CStdString value;
  if (CGUIDialogBoxeeDropdown::Show(seasons, g_localizeStrings.Get(53562), value))
  {
    m_iSeason = atoi(value);

    if (m_iSeason == -1)
    {
      m_pWindow->SetProperty(SEASONS_LABEL_FLAG, g_localizeStrings.Get(53508));
    }
    else
    {
      CStdString seasonLabel;
      seasonLabel.Format("SEASON %d", m_iSeason);
      m_pWindow->SetProperty(SEASONS_LABEL_FLAG, seasonLabel);
    }
    return true;
  }
  return false;
}

bool CEpisodesWindowState::OnSubscribe()
{
  SubscribeJob* job = new SubscribeJob(CSubscriptionType::TVSHOW_SUBSCRIPTION, m_strShowId, m_strShowTitle, !m_bSubscribed);

  // TODO: Check result and present error message
  CUtil::RunInBG(job);

  m_bSubscribed = !m_bSubscribed;

  if (m_bSubscribed)
  {
    g_application.m_guiDialogKaiToast.QueueNotification("", "",g_localizeStrings.Get(53523), NOTIFICATION_APPEARANCE_IN_SEC);
    m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53522));
  }
  else
  {
    g_application.m_guiDialogKaiToast.QueueNotification("", "",g_localizeStrings.Get(53524), NOTIFICATION_APPEARANCE_IN_SEC);
    m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53521));
  }

  return m_bSubscribed;
}

bool CEpisodesWindowState::OnShortcut()
{
  // Add currently path to all tv shows, unless it already exists?
  CBoxeeShortcut cut;
  cut.SetName(m_strShowTitle);

  CStdString command;
  CStdString str = TV_EPISODES_WINDOW_SHORTCUT_COMMAND;
  command.Format(str.c_str(), m_strPath);
  cut.SetCommand(command);

  cut.SetThumbPath(m_strShowThumb);
  cut.SetCountry("all");
  cut.SetCountryAllow(true);
  cut.SetReadOnly(false);

  if (!m_bHasShortcut)
  {
    // add to shortcut
    if (g_settings.GetShortcuts().AddShortcut(cut))
    {
      m_bHasShortcut = true;
    }
  }
  else
  {
    // remove from shortcut
    if (g_settings.GetShortcuts().RemoveShortcut(cut))
    {
      m_bHasShortcut = false;
    }
  }
  return m_bHasShortcut;
}

void CEpisodesWindowState::OnFree()
{
  SetFree(!m_bFreeOnly);
}

void CEpisodesWindowState::SetFree(bool bFree)
{
  m_bFreeOnly = bFree;
  m_pWindow->SetProperty(FREE_ONLY_SET_FLAG, m_bFreeOnly);

  //m_configuration.ClearActiveFilters();
  m_configuration.RemoveCustomFilter(CUSTOM_FREE_FILTER);
  if (m_bFreeOnly)
  {
    m_pWindow->SetProperty(FREE_ONLY_LABEL_FLAG, g_localizeStrings.Get(53526));
    m_configuration.AddCustomFilter(new CBrowseWindowTvEpisodeFreeFilter(CUSTOM_FREE_FILTER, "Tv Episode Free Filter", true));
  }
  else
  {
    m_pWindow->SetProperty(FREE_ONLY_LABEL_FLAG, "");
  }
}

void CEpisodesWindowState::Reset()
{
  CBrowseWindowState::Reset();
  m_strShowTitle = "";
  m_strShowId = "";
  m_strShowThumb = "";

  // Reset season filter
  m_iSeason = -1;
  m_pWindow->SetProperty(SEASONS_LABEL_FLAG, g_localizeStrings.Get(53508));

  SetFree(false);
}

void CEpisodesWindowState::SortItems(CFileItemList &items)
{
  // Remove previous separators
  for (int i = 0; i < items.Size() - 1; i++)
  {
    if (items.Get(i)->GetPropertyBOOL("isseparator"))
    {
      items.Remove(i--);
    }
  }

  items.Sort(m_sort);

  // Set the correct number of items as a window property
  m_pWindow->SetProperty("episode-count", items.Size());

  bool bHasSeasons = false;

  // Go over the vector and put season separators where needed
  for (int i = 0; i < items.Size() ; i++)
  {
    int currentSeason = items.Get(i)->GetVideoInfoTag()->m_iSeason;
    int nextSeason = 0;

    if  (i == items.Size() -1 )
    {
        if (currentSeason <= 0)
        {
         items.Get(i)->GetVideoInfoTag()->m_iEpisode = -1;
        }
        nextSeason = currentSeason;
    }
    else
    {
      nextSeason = items.Get(i+1)->GetVideoInfoTag()->m_iSeason;
    }

    //patch for local daily shows - currently daily shows are marked in the database as
    // season -2,  treat it as season 0.
    if (currentSeason == -2)
    {
      currentSeason = -1;
    }
    if (nextSeason == -2)
    {
      nextSeason = -1;
    }

    if (currentSeason <= 0)
    {
     items.Get(i)->GetVideoInfoTag()->m_iEpisode = -1;
    }

    CStdString seasonLabel;

    if (i == 0)
    {
      // Add separator for the first season in the list
      if (currentSeason > 0)
        seasonLabel.Format("Season %d", currentSeason);
      else
        seasonLabel = "Extras";
    }
    else if (currentSeason != nextSeason)
    {
      if (nextSeason > 0)
      {
        seasonLabel.Format("Season %d", nextSeason);
        bHasSeasons = true;
      }
      else
        seasonLabel = "Extras";
    }
    else
    {
      continue;
    }

    CFileItemPtr pItem ( new CFileItem(seasonLabel) );
    pItem->SetProperty("isseparator", true);

    items.AddFront(pItem,i == 0 ? i : i+1);
    if (i != 0)
      i++;

  }

  if (!bHasSeasons)
  {
    // there are no seasons, all items are under category other, in which case we want to remove it
    if (items.Size() > 0 && items.Get(0)->HasProperty("isseparator") && items.Get(0)->GetLabel() == "Extras")
    {
      items.Remove(0);

    }
  }
}

void CEpisodesWindowState::SetPath(const CStdString& strPath)
{
  m_strPath = strPath;
}

void CEpisodesWindowState::SetShowId(const CStdString& strShowId)
{
  m_strShowId = strShowId;
}

void CEpisodesWindowState::SetShowTitle(const CStdString& strShowTitle)
{
  m_strShowTitle = strShowTitle;
}

void CEpisodesWindowState::SetShowThumb(const CStdString& strShowThumb)
{
  m_strShowThumb = strShowThumb;
}

// ///////////////////////////////////////////////////////////////

CGUIWindowBoxeeBrowseTvEpisodes::CGUIWindowBoxeeBrowseTvEpisodes()
: CGUIWindowBoxeeBrowseWithPanel(WINDOW_BOXEE_BROWSE_TVEPISODES, "boxee_browse_tvepisodes.xml")
{
  SetWindowState(new CEpisodesWindowState(this));
}

CGUIWindowBoxeeBrowseTvEpisodes::~CGUIWindowBoxeeBrowseTvEpisodes()
{

}

void CGUIWindowBoxeeBrowseTvEpisodes::OnInitWindow()
{
  m_iLastControl = 52;
  ClearProperty("background");

  // reset the correct number of items as a window property
  SetProperty("episode-count", "");

  //m_windowState->Reset();

  CGUIWindowBoxeeBrowse::OnInitWindow();

  if (((CEpisodesWindowState*)m_windowState)->HasShortcut())
  {
    CStdString label = g_localizeStrings.Get(53716);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }
  else
  {
    CStdString label = g_localizeStrings.Get(53715);
    SET_CONTROL_LABEL(BUTTON_SHORTCUT, label.ToUpper());
  }

  CONTROL_DISABLE(BUTTON_SHORTCUT);
}

void CGUIWindowBoxeeBrowseTvEpisodes::OnDeinitWindow(int nextWindowID)
{
  CGUIWindowBoxeeBrowse::OnDeinitWindow(nextWindowID);
  m_setSeasons.clear();
}


bool CGUIWindowBoxeeBrowseTvEpisodes::OnMessage(CGUIMessage& message)
{
  if (ProcessPanelMessages(message)) {
    return true;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

// TODO: Take this function out to the helper class
bool CGUIWindowBoxeeBrowseTvEpisodes::ProcessPanelMessages(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage, GUI_MSG_CLICKED, control = %d (browse)", iControl);

   if (iControl == BUTTON_SUBSCRIBE)
    {
      ((CEpisodesWindowState*)m_windowState)->OnSubscribe();
      return true;
    }
    else if (iControl == BUTTON_SEASONS)
    {
      if (((CEpisodesWindowState*)m_windowState)->OnSeasons(m_setSeasons))
      {
        Refresh();
      }
      return true;
    }
    else if (iControl == BUTTON_SHORTCUT)
    {
      if (((CEpisodesWindowState*)m_windowState)->OnShortcut())
      {
        // shortcut was added
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53716));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53737), 5000);
      }
      else
      {
        // shortcut was removed
        SET_CONTROL_LABEL(BUTTON_SHORTCUT, g_localizeStrings.Get(53715));
        g_application.m_guiDialogKaiToast.QueueNotification("", "", g_localizeStrings.Get(53739), 5000);
      }

    }
    else if (iControl == BUTTON_FREE)
    {
      ((CEpisodesWindowState*)m_windowState)->OnFree();
      UpdateFileList(false);
    }

    // else - break from switch and return false
    break;
  } // case GUI_MSG_CLICKED

  } // switch

  return CGUIWindowBoxeeBrowseWithPanel::ProcessPanelMessages(message);
}

bool CGUIWindowBoxeeBrowseTvEpisodes::OnBind(CGUIMessage& message)
{
  if (message.GetPointer() && message.GetControlId() == 0)
  {
    CFileItemList *items = (CFileItemList *)message.GetPointer();

    ((CEpisodesWindowState*)m_windowState)->SetShowTitle(items->GetProperty("serieslabel"));
    ((CEpisodesWindowState*)m_windowState)->SetShowId(items->GetProperty("seriesid"));
    ((CEpisodesWindowState*)m_windowState)->SetShowThumb(items->GetProperty("seriesthumb"));

    SetProperty("title", items->GetProperty("serieslabel"));
    SetProperty("background", items->GetProperty("seriesbackground"));

    m_windowState->SetLabel(items->GetProperty("serieslabel"));

    CONTROL_ENABLE(BUTTON_SHORTCUT);

    // Extract seasons from received items
    if (m_setSeasons.empty())
      ExtractSeasons(items, m_setSeasons);

    CONTROL_ENABLE_ON_CONDITION(BUTTON_SEASONS, (m_setSeasons.size() > 1));
  }

  return CGUIWindowBoxeeBrowse::OnBind(message);
}

void CGUIWindowBoxeeBrowseTvEpisodes::OnBack()
{
  g_windowManager.PreviousWindow();
}

void CGUIWindowBoxeeBrowseTvEpisodes::ExtractSeasons(CFileItemList* items, std::set<int>& setSeasons)
{
  setSeasons.clear();

  // Get all items from the container
  for (int i = 0; i < items->Size(); i++)
  {
    CFileItemPtr item = items->Get(i);
    if (item->HasVideoInfoTag())
    {
      if (item->GetVideoInfoTag()->m_iSeason != -1)
      {
        setSeasons.insert(item->GetVideoInfoTag()->m_iSeason);
      }
    }
  }

  SetProperty(SEASONS_LABEL_FLAG, g_localizeStrings.Get(53508));
}



