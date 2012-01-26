
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
#include "GUIUserMessages.h"
#include "boxee.h"
#include "GUIDialogBoxeeChannelFilter.h"
#include "GUIListContainer.h"

using namespace std;
using namespace BOXEE;

#define BUTTON_FULL_EPISODES 110
#define BUTTON_FULL_CLIPS 120

#define BUTTON_SEASONS 140
#define BUTTON_SUBSCRIBE_ON   8004
#define BUTTON_SUBSCRIBE_OFF  8005
#define BUTTON_SUBSCRIBE_FLAG "is-favorite"
#define BUTTON_FREE 151

#define EPISODES_LIST 52
#define SEASONS_LIST 6020
#define BUTTON_OVERVIEW   8006

#define FREE_FILTER_ID   615
#define FREE_FILTER_NAME "episodefilterfree"

#define NOTIFICATION_APPEARANCE_IN_SEC   5000

#define NEWEST_SET_FLAG "newestfirst-set"

#define FREE_ONLY_SET_FLAG "free-set"
#define FREE_ONLY_LABEL_FLAG "free-label"
#define SEASONS_DROPDOWN  "seasons-drop"
#define SEASONS_LABEL     "seasons-label"

#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define ITEM_COUNT_LABEL "item-summary-count"

#define ALL_SEASONS_ID       -3
#define DAILY_SEASON_ID      -2
#define EXTRAS_SEASON_ID     -1

#define HIDDEN_TVSHOW_CONTAINER     5000

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

CLocalEpisodesSource::CLocalEpisodesSource(int iWindowID) : CBrowseWindowSource("localepisodessource", "boxeedb://episodes/", iWindowID)
{

}

CLocalEpisodesSource::~CLocalEpisodesSource(){}

void CLocalEpisodesSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  mapOptions["local"] = "true";
  mapOptions["remote"] = "true";

  CStdString strShowId = m_strShowId;
  CUtil::URLEncode(strShowId);
  mapOptions["seriesId"] = strShowId;

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

void CLocalEpisodesSource::BindItems(CFileItemList &items)
{
  //FILTER OUT BY SEASON
  if (m_mapFilters.size() > 0 && m_mapFilters.find("season") != m_mapFilters.end())
  {
    CStdString seasonNumber = m_mapFilters["season"];

    int iSeason = atoi(seasonNumber.c_str());

    if(iSeason == 0)
      iSeason = EXTRAS_SEASON_ID;

    if (iSeason >= EXTRAS_SEASON_ID /*season id == -1*/)
    {
      for (int i = 0 ; i < items.Size() ; i++)
      {
        int iItemSeason = items.Get(i)->GetVideoInfoTag()->m_iSeason;

        if(iItemSeason == 0)
          iItemSeason = EXTRAS_SEASON_ID;

        //items[i]->Dump();

        if (iItemSeason != iSeason)
          items.Remove(i--);
      }
    }
  }

  CBrowseWindowSource::BindItems(items);
}


//////////////////////////////////////////////////////////////////////////////

CRemoteEpisodesSource::CRemoteEpisodesSource(int iWindowID) : CBrowseWindowSource("remoteepisodessource", "boxee://tvshows/episodes/", iWindowID)
{

}

CRemoteEpisodesSource::~CRemoteEpisodesSource(){}

void CRemoteEpisodesSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  mapOptions["local"] = "true";
  mapOptions["remote"] = "true";

  CStdString strShowId = m_strShowId;
  CUtil::URLEncode(strShowId);
  mapOptions["seriesId"] = strShowId;

  CStdString excludedChannels = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetExcludedSources();

  if (!excludedChannels.IsEmpty())
  {
    if (!m_allowProviders.IsEmpty())
    {
      std::vector<CStdString> excludedChannelsVec;
      CUtil::Tokenize(m_allowProviders,excludedChannelsVec,",");

      for (size_t i=0; i<excludedChannelsVec.size(); i++)
      {
        excludedChannels.Replace(excludedChannelsVec[i],"");
      }

      excludedChannels.Replace(",,",",");

      if (!excludedChannels.IsEmpty() && excludedChannels.GetAt(0) == ',')
      {
        excludedChannels.Delete(0);
      }
    }

    CUtil::URLEncode(excludedChannels);
    mapOptions["provider_exclude"] = excludedChannels;
  }

  CBrowseWindowSource::AddStateParameters(mapOptions);
}

void CRemoteEpisodesSource::BindItems(CFileItemList &items)
{
  //FILTER OUT BY SEASON
  if (m_mapFilters.size() > 0 && m_mapFilters.find("season") != m_mapFilters.end())
  {
    CStdString seasonNumber = m_mapFilters["season"];

    int iSeason = atoi(seasonNumber.c_str());

    if (iSeason >= EXTRAS_SEASON_ID /*season id == -1*/)
    {
      for (int i = 0 ; i < items.Size() ; i++)
      {
        int iItemSeason = items.Get(i)->GetVideoInfoTag()->m_iSeason;

        //items[i]->Dump();

        if (iItemSeason != iSeason)
          items.Remove(i--);
      }
    }
  }

  CBrowseWindowSource::BindItems(items);
}

//////////////////////////////////////////////////////////////////////////////

CEpisodesWindowState::CEpisodesWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_iSeason = ALL_SEASONS_ID;
  m_bFreeOnly= false;
  m_bSubscribed = false;

  m_sourceController.RemoveAllSources();

  m_sourceController.AddSource(new CRemoteEpisodesSource(m_pWindow->GetID()));
  m_sourceController.AddSource(new CLocalEpisodesSource(m_pWindow->GetID()));

  // Initialize sort vector
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_NEWEST_FIRST, SORT_METHOD_EPISODE, SORT_ORDER_DESC, g_localizeStrings.Get(53529), ""));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_NEWEST_LAST, SORT_METHOD_EPISODE, SORT_ORDER_ASC, g_localizeStrings.Get(53528), ""));
}

void CEpisodesWindowState::SetDefaultView()
{
  m_iCurrentView = THUMB_VIEW_LIST;
}

void CEpisodesWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());

  UpdateSubscription();
}

bool CEpisodesWindowState::OnSeasons(std::set<int>& setSeasons)
{
  CFileItemList seasons;

  CFileItemPtr allSeasonsItem (new CFileItem(g_localizeStrings.Get(53508)));
  allSeasonsItem->SetProperty("value", ALL_SEASONS_ID);
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
  CGUIDialogBoxeeDropdown *dialog = (CGUIDialogBoxeeDropdown *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_DROPDOWN);

  if (dialog && dialog->Show(seasons, g_localizeStrings.Get(53562), value, 25,1))
  {
    SetSeason(value);
    return true;
  }

  return false;
}


void CEpisodesWindowState::OnFree()
{
  SetFree(!m_bFreeOnly);
}

void CEpisodesWindowState::SetSeason(const CStdString& strSeasonId)
{
  m_iSeason = atoi(strSeasonId);

  if (m_iSeason == ALL_SEASONS_ID)
  {
    m_pWindow->SetProperty(SEASONS_LABEL, g_localizeStrings.Get(53508));
    m_sourceController.ClearFilter("season");
  }
  else
  {
    CStdString seasonLabel;
    seasonLabel.Format("SEASON %d", m_iSeason);

    m_sourceController.SetFilter("season",strSeasonId);

    m_pWindow->SetProperty(SEASONS_LABEL, seasonLabel);
  }
}

int CEpisodesWindowState::GetSeason()
{
  return m_iSeason;
}

CStdString CEpisodesWindowState::GetItemSummary()
{
  CStdString itemSummary = "";

  itemSummary += m_strShowTitle;
/*  itemSummary += " ";

  itemSummary += g_localizeStrings.Get(20373);
  itemSummary += " " + m_iSeason;
  itemSummary += " ";

  if (!m_sort.m_sortName.empty() && m_sort.m_id != VIEW_SORT_METHOD_ATOZ)
  {//show the sort only if its not A TO Z
    strSummary += m_sort.m_sortName;
    strSummary += " ";
  }*/

  return itemSummary;
}

void CEpisodesWindowState::SetFree(bool bFree)
{
  m_bFreeOnly = bFree;
  m_pWindow->SetProperty(FREE_ONLY_SET_FLAG, m_bFreeOnly);

  //ALEX: Check what can be done with this filter
  // Remove previous free filter
  //RemoveLocalFilter(FREE_FILTER_NAME);

  if (m_bFreeOnly)
  {
    m_pWindow->SetProperty(FREE_ONLY_LABEL_FLAG, g_localizeStrings.Get(53526));
    //AddLocalFilter(new CBrowseWindowTvEpisodeFreeFilter(FREE_FILTER_ID, FREE_FILTER_NAME, true));
  }
  else
  {
    m_pWindow->SetProperty(FREE_ONLY_LABEL_FLAG, "");
  }
}


bool CEpisodesWindowState::OnSubscribe()
{
  SubscribeJob* job = new SubscribeJob(CSubscriptionType::TVSHOW_SUBSCRIPTION, m_strShowId, m_strShowTitle, !m_bSubscribed);

  // TODO: Check result and present error message
  if (CUtil::RunInBG(job) == JOB_SUCCEEDED)
  {
    m_bSubscribed = !m_bSubscribed;
  }

  if (m_bSubscribed)
    m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53522));
  else
    m_pWindow->SetProperty("subscribe", g_localizeStrings.Get(53521));

  return m_bSubscribed;
}


void CEpisodesWindowState::SetShowId(const CStdString& strShowId)
{
  m_strShowId = strShowId;

  CRemoteEpisodesSource* remoteSource = (CRemoteEpisodesSource*)m_sourceController.GetSourceById("remoteepisodessource");
  if (remoteSource)
  {
    remoteSource->m_strShowId = m_strShowId;

    //avoid searching online if its a user generated tv show
    if (m_strShowId.find("local-",0) == std::string::npos)
    {
      remoteSource->Activate(true);
    }
    else
    {
      remoteSource->Activate(false);
    }
  }

  CLocalEpisodesSource* localSource = (CLocalEpisodesSource*)m_sourceController.GetSourceById("localepisodessource");
  if (localSource)
  {
    localSource->m_strShowId = m_strShowId;
  }
}

void CEpisodesWindowState::SetShowTitle(const CStdString& strShowTitle)
{
  m_strShowTitle = strShowTitle;
}

void CEpisodesWindowState::SetShowThumb(const CStdString& strShowThumb)
{
  m_strShowThumb = strShowThumb;
}

void CEpisodesWindowState::SetAllowProviders(const CStdString& strAllowProviders)
{
  m_allowProviders = strAllowProviders;

  CRemoteEpisodesSource* remoteSource = (CRemoteEpisodesSource*)m_sourceController.GetSourceById("remoteepisodessource");
  if (remoteSource)
  {
    remoteSource->m_allowProviders = m_allowProviders;
  }
}

bool CEpisodesWindowState::UpdateSubscription()
{
  m_bSubscribed = BoxeeUtils::IsSubscribe(m_strShowId);
  m_pWindow->SetProperty(BUTTON_SUBSCRIBE_FLAG , m_bSubscribed);

  return m_bSubscribed;
}

// ///////////////////////////////////////////////////////////////

CGUIWindowBoxeeBrowseTvEpisodes::CGUIWindowBoxeeBrowseTvEpisodes() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_TVEPISODES, "boxee_browse_tvepisodes.xml")
{
  m_strItemDescription = g_localizeStrings.Get(90048);
  SetWindowState(new CEpisodesWindowState(this));

  m_hasBrowseMenu = false;
}

CGUIWindowBoxeeBrowseTvEpisodes::~CGUIWindowBoxeeBrowseTvEpisodes()
{

}

void CGUIWindowBoxeeBrowseTvEpisodes::OnInitWindow()
{
  ClearProperty("background");

  // reset the correct number of items as a window property
  SetProperty("episode-count", "");

  CGUIWindowBoxeeBrowse::OnInitWindow();

  m_originalItemCount = 0;

  CGUIMessage msgResetSeasonslList(GUI_MSG_LABEL_RESET, GetID(), SEASONS_LIST);
  OnMessage(msgResetSeasonslList);

  CGUIMessage msgResetTvShowItem(GUI_MSG_LABEL_RESET, GetID(), HIDDEN_TVSHOW_CONTAINER);
  OnMessage(msgResetTvShowItem);

  CFileItemPtr itemPtr(new CFileItem(m_tvShowItem));
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), HIDDEN_TVSHOW_CONTAINER, 0, 0, itemPtr);
  g_windowManager.SendThreadMessage(winmsg);
}

void CGUIWindowBoxeeBrowseTvEpisodes::OnDeinitWindow(int nextWindowID)
{
  //clear the seasons before we release the window.. so that if we launch the next window and get the response, it won't insert it together with this window session seasons
  m_setSeasons.clear();
  CGUIWindowBoxeeBrowse::OnDeinitWindow(nextWindowID);
}

bool CGUIWindowBoxeeBrowseTvEpisodes::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    OnBack();
    return true;
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowseTvEpisodes::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_WINDOW_DEINIT:
    {
      m_setSeasons.clear();
    }
    break;
    case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();

      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage, GUI_MSG_CLICKED, control = %d (browse)", iControl);

      if (iControl == BUTTON_SUBSCRIBE_ON || iControl == BUTTON_SUBSCRIBE_OFF)
      {
        ((CEpisodesWindowState*)m_windowState)->OnSubscribe();

        SetProperty(BUTTON_SUBSCRIBE_FLAG , !GetPropertyBOOL(BUTTON_SUBSCRIBE_FLAG));

        return true;
      }
      else if (iControl == BROWSE_SETTINGS)
      {
        return CGUIDialogBoxeeChannelFilter::Show();
      }
      else if (iControl == SEASONS_LIST)
      {
        return HandleClickOnSeasonList();
      }
      else if (iControl == BUTTON_OVERVIEW)
      {
        HandleClickOnOverviewButton();
      }
    }
    break;
    case GUI_MSG_UPDATE_ITEM:
    {
      CStdString param = message.GetStringParam();

      if (param == "favorite update")
      {
        ((CEpisodesWindowState*)m_windowState)->UpdateSubscription();
        return true;
      }
      else if (param == "MarkAsWatched")
      {
        if (message.GetSenderId() == WINDOW_BOXEE_BROWSE_TVEPISODES)
        {
          CStdString itemId = message.GetStringParam(1);

          if (itemId.IsEmpty())
          {
            CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since itemId is EMPTY. [itemId=%s] (browse)",itemId.c_str());
            return false;
          }

          CStdString watchedType = message.GetStringParam(2);

          if (watchedType.IsEmpty())
          {
            CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since watchedType is EMPTY. [watchedType=%s] (browse)",watchedType.c_str());
            return false;
          }

          CFileItemPtr pItem = m_viewItemsIndex[itemId];
          if (!pItem.get())
          {
            CLog::Log(LOGWARNING,"CGUIWindowBoxeeBrowseTvEpisodes::OnMessage - GUI_MSG_UPDATE_ITEM - FAILED to mark item as watched since there is NO item for [itemId=%s] (browse)",itemId.c_str());
            return false;
          }

          pItem->SetProperty("watched", (watchedType == "1") ? true : false);

          ((CEpisodesWindowState*)m_windowState)->Refresh(true);

          return true;
        }
      }
    }
    break;
  } // switch

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

void CGUIWindowBoxeeBrowseTvEpisodes::ExtractSeasons(CFileItemList* episodeList)
{
  // Get all items from the container
  for (int i = 0; i < episodeList->Size(); i++)
  {
    CFileItemPtr item = episodeList->Get(i);
    if (item->HasVideoInfoTag())
    {
      int season = item->GetVideoInfoTag()->m_iSeason;
      if(season == 0)
      {
        season = EXTRAS_SEASON_ID;
      }
      m_setSeasons.insert(season);
    }
  }

  SetProperty(SEASONS_LABEL, g_localizeStrings.Get(53508));

  CFileItemList seasonList;

  CFileItemPtr allSeasonsItem (new CFileItem(g_localizeStrings.Get(53508)));
  allSeasonsItem->SetProperty("value", ALL_SEASONS_ID);
  seasonList.Add(allSeasonsItem);

  int selectSeasonIndex = ALL_SEASONS_ID;
  std::set<int>::iterator it = m_setSeasons.begin();
  while (it != m_setSeasons.end())
  {
    int season = (int) *it;
    CStdString label;

    //if the show contains only season == EXTRAS_SEASON_ID assume show is daily
    if(season != EXTRAS_SEASON_ID || m_setSeasons.size() > 1)
    {
      //Extras id
      if(season == EXTRAS_SEASON_ID)
        label = g_localizeStrings.Get(20435);
      else
        label.Format(g_localizeStrings.Get(20358), season);

      CFileItemPtr seasonItem (new CFileItem(label));
      seasonItem->SetProperty("value", *it);

      if (season == ((CEpisodesWindowState*)m_windowState)->GetSeason())
      {
        seasonItem->Select(true);
        selectSeasonIndex = seasonList.Size();
      }

      seasonList.Add(seasonItem);
      it++;
    }
    else
      break;
  }

  if ((selectSeasonIndex == ALL_SEASONS_ID) && !seasonList.IsEmpty())
  {
    seasonList.Get(0)->Select(true);
  }

  CGUIMessage msgBindToSeasonslList(GUI_MSG_LABEL_BIND, GetID(), SEASONS_LIST, 0, 0, &seasonList);
  OnMessage(msgBindToSeasonslList);

  CONTROL_SELECT_ITEM(SEASONS_LIST,selectSeasonIndex);
}

void CGUIWindowBoxeeBrowseTvEpisodes::OnBack()
{
  m_tvShowItem.Reset();
  g_windowManager.PreviousWindow();
}


void CGUIWindowBoxeeBrowseTvEpisodes::ShowItems(CFileItemList& list, bool append)
{
  //the items that will actually appear on the screen, after sorting and filtering.
  m_setSeasons.clear(); //clear out the old seasons..
  ExtractSeasons(&list);

  AddSeparators(list);

  CGUIWindowBoxeeBrowse::ShowItems(list, append);
}

CStdString CGUIWindowBoxeeBrowseTvEpisodes::GetItemDescription()
{
  // Set the correct number of items as a window property
  SetProperty("episode-count", m_originalItemCount); //needed?

  CStdString strItemCount="";

  strItemCount.Format("%d %s", m_originalItemCount, m_strItemDescription.c_str());

  return strItemCount;
}

void CGUIWindowBoxeeBrowseTvEpisodes::AddSeparators(CFileItemList &items)
{
  // Remove previous separators
  for (int i = 0; i < items.Size() - 1; i++)
  {
    if (items.Get(i)->GetPropertyBOOL("isseparator"))
    {
      items.Remove(i--);
    }
  }

  m_originalItemCount = items.Size(); //used in GetItemDescription

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

    //patch for extra/specials, we get from the server -1 in the season, treat it as 0 so that all the extras will be in the same group
    if (currentSeason == -1)
    {
      currentSeason = 0;
    }
    if (nextSeason == -1)
    {
      nextSeason = 0;
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
        seasonLabel.Format("%s %d", g_localizeStrings.Get(20373), currentSeason);
      else
        seasonLabel = g_localizeStrings.Get(20435);
    }
    else if (currentSeason != nextSeason)
    {
      if (nextSeason > 0)
      {
        seasonLabel.Format("%s %d", g_localizeStrings.Get(20373), nextSeason);
        bHasSeasons = true;
      }
      else
      {
        seasonLabel = g_localizeStrings.Get(20435);
      }
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
    // there are no seasons, all items are under category other, in which case we want to change it to All
    if (items.Size() > 0 && items.Get(0)->HasProperty("isseparator") && items.Get(0)->GetLabel() == g_localizeStrings.Get(20435) && m_setSeasons.size() == 1)
    {
      items.Get(0)->SetLabel(g_localizeStrings.Get(53509));
    }
  }
}

void CGUIWindowBoxeeBrowseTvEpisodes::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.size() > 0)
    {
      if (optionsMap.find("title")!=optionsMap.end())
      {
        CStdString strTitle = optionsMap["title"];
        ((CEpisodesWindowState*)m_windowState)->SetShowTitle(strTitle);
      }

      if (optionsMap.find("seriesId")!=optionsMap.end())
      {
        ((CEpisodesWindowState*)m_windowState)->SetShowId(optionsMap["seriesId"]);
      }

      if (optionsMap.find("season")!=optionsMap.end())
      {
        ((CEpisodesWindowState*)m_windowState)->SetSeason(optionsMap["season"]);
      }
      else
      {
        ((CEpisodesWindowState*)m_windowState)->SetSeason("-3"); //ALL_SEASONS
      }

      CStdString strAllowProviders = "";
      if (optionsMap.find("allowProviders") != optionsMap.end())
      {
        strAllowProviders = optionsMap["allowProviders"];
      }
      ((CEpisodesWindowState*)m_windowState)->SetAllowProviders(strAllowProviders);
    }
  }
  else
  {
    //just set the id as it used to be on default season
    ((CEpisodesWindowState*)m_windowState)->SetShowId(param);
    ((CEpisodesWindowState*)m_windowState)->SetSeason("-1"); //EXTRAS
  }
}

bool CGUIWindowBoxeeBrowseTvEpisodes::HandleClickOnSeasonList()
{
  CGUIListContainer* pListcontainer = (CGUIListContainer*)GetControl(SEASONS_LIST);

  if (!pListcontainer)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvEpisodes::HandleClickOnSeasonList - FAILED to get container [SEASONS_LIST=%d] (browse)",SEASONS_LIST);
    return false;
  }

  CGUIListItemPtr selectedListItem = pListcontainer->GetSelectedItemPtr();
  CFileItem* selectedFileItem = (CFileItem*) selectedListItem.get();

  if (!selectedFileItem)
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseTvEpisodes::HandleClickOnSeasonList - FAILED to get selected item from [SEASONS_LIST=%d] (browse)",SEASONS_LIST);
    return false;
  }

  g_settings.SetSkinString(g_settings.TranslateSkinString("EpisodeViewMode"),"Episodes");

  CStdString seasonNumStr = selectedFileItem->GetProperty("value");

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseTvEpisodes::HandleClickOnSeasonList - going to set season to [%s] (browse)",seasonNumStr.c_str());
  ((CEpisodesWindowState*)m_windowState)->SetSeason(seasonNumStr);

  ((CEpisodesWindowState*)m_windowState)->Refresh(true);

  // set focus to the current view
  SET_CONTROL_FOCUS(EPISODES_LIST,0);

  return true;
}

bool CGUIWindowBoxeeBrowseTvEpisodes::HandleClickOnOverviewButton()
{
  CGUIMessage winmsg(GUI_MSG_UNMARK_ALL_ITEMS, GetID(), SEASONS_LIST);
  OnMessage(winmsg);

  return true;
}

void CGUIWindowBoxeeBrowseTvEpisodes::SetTvShowItem(const CFileItem& tvShowItem)
{
  m_tvShowItem = tvShowItem;
}
