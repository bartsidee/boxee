
#include "GUIDialogBoxeeChannelFilter.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "log.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxsourcesmanager.h"
#include "bxcscmanager.h"
#include "bxconfiguration.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "FileSystem/Directory.h"
#include "LocalizeStrings.h"
#include "GUIControlGroup.h"
#include "GUIFixedListContainer.h"

#define CHANNEL_LIST                   7025

#define OFFER_DELIMITER                ","
#define FREE_OFFER_STRING              "free"

using namespace BOXEE;

CGUIDialogBoxeeChannelFilter::CGUIDialogBoxeeChannelFilter() : CGUIDialog(WINDOW_DIALOG_BOXEE_CHANNEL_FILTER, "boxee_channel_filter.xml")
{

}

CGUIDialogBoxeeChannelFilter::~CGUIDialogBoxeeChannelFilter()
{

}

void CGUIDialogBoxeeChannelFilter::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  CStdString sourcesUrl = "boxee://sources/";
  int activeWindow = g_windowManager.GetActiveWindow();

  if (activeWindow == WINDOW_BOXEE_BROWSE_MOVIES)
  {
    sourcesUrl += "movies";
  }
  else
  {
    sourcesUrl += "shows";
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnInitWindow - build source list for [url=%s] (cf)",sourcesUrl.c_str());

  CFileItemList sourceList;
  DIRECTORY::CDirectory::GetDirectory(sourcesUrl,sourceList);

  if (sourceList.Size() < 1)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeChannelFilter::OnInitWindow - FAILED to get sources. [size=%d] (cf)",sourceList.Size());
    Close();
    return;
  }

  if (!LoadExcludedChannels())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeChannelFilter::OnInitWindow - FAILED to load excluded sources (cf)");
    Close();
    return;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnInitWindow - [NumOfSources=%d][NumOfExcludedSources=%zu] (cf)",sourceList.Size(),m_excludedChannels.size());

  CGUIMessage winmsgResetPremium(GUI_MSG_LABEL_RESET, GetID(), CHANNEL_LIST);
  OnMessage(winmsgResetPremium);

  CFileItemList channelItemList;
  CFileItemList freeChannelItemList;

  for (int i=0; i<sourceList.Size(); i++)
  {
    CFileItemPtr sourceItem = sourceList.Get(i);
    std::string sourceId = sourceItem->GetProperty("sourceid");
    std::string sourceOffer = sourceItem->GetProperty("sourceoffer");

    if (sourceOffer.empty())
    {
      CLog::Log(LOGWARNING,"CGUIDialogBoxeeChannelFilter::OnInitWindow - [%d/%d] - for sources [id=%s] the attribute offer is EMPTY. [offer=%s] (cf)",i+1,sourceList.Size(),sourceId.c_str(),sourceOffer.c_str());
      continue;
    }

    CFileItemPtr source(new CFileItem(*(sourceItem.get())));
    source->SetProperty("channelthumb",source->GetProperty("sourcethumb"));
    source->Select(m_excludedChannels.find(sourceId) == m_excludedChannels.end());

    if (stricmp(sourceOffer.c_str(),FREE_OFFER_STRING))
    {
      if (channelItemList.IsEmpty())
      {
        // add PREMIUM separator
        CFileItemPtr seperator(new CFileItem(g_localizeStrings.Get(53581)));
        seperator->SetProperty("isseparator",true);
        channelItemList.Add(seperator);
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnInitWindow - [%d/%d] - [sourceoffer=%s] -> adding PREMIUM source [id=%s] (cf)",i+1,sourceList.Size(),sourceOffer.c_str(),sourceId.c_str());
      channelItemList.Add(source);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnInitWindow - [%d/%d] - [sourceoffer=%s] -> adding FREE source [id=%s] (cf)",i+1,sourceList.Size(),sourceOffer.c_str(),sourceId.c_str());
      freeChannelItemList.Add(source);
    }
  }

  for (int i=0; i<freeChannelItemList.Size(); i++)
  {
    if (i == 0 && !channelItemList.IsEmpty())
    {
      // add FREE separator
      CFileItemPtr seperator(new CFileItem(g_localizeStrings.Get(53582)));
      seperator->SetProperty("isseparator",true);
      channelItemList.Add(seperator);
    }

    CFileItemPtr source(new CFileItem(*(freeChannelItemList.Get(i))));
    source->SetProperty("channelthumb",source->GetProperty("sourcethumb"));
    channelItemList.Add(source);
  }

  CGUIMessage msgBindToChannelList(GUI_MSG_LABEL_BIND, GetID(), CHANNEL_LIST, 0, 0, &channelItemList);
  OnMessage(msgBindToChannelList);

  SET_CONTROL_FOCUS(CHANNEL_LIST, 0);

  m_dirty = false;
}

void CGUIDialogBoxeeChannelFilter::OnDeinitWindow(int nextWindowID)
{
  CGUIDialog::OnDeinitWindow(nextWindowID);

  if (!m_dirty)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnDeinitWindow - [dirty=%d] -> no need to update (cf)",m_dirty);
    return;
  }

  std::string excludedChannelsStr;
  std::set<std::string>::iterator it = m_excludedChannels.begin();
  while(it != m_excludedChannels.end())
  {
    if (it != m_excludedChannels.begin())
    {
      excludedChannelsStr += OFFER_DELIMITER;
    }

    excludedChannelsStr += (*it);
    it++;
  }

  CPostExcludedSorcesRequestJob* postRequest = new CPostExcludedSorcesRequestJob(excludedChannelsStr);

  if (!postRequest)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeChannelFilter::OnDeinitWindow - FAILED to allocate CPostExcludedSorcesRequestJob object. [postData=%s] (cf)",excludedChannelsStr.c_str());
    return;
  }

  if (CUtil::RunInBG(postRequest) != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeChannelFilter::OnDeinitWindow - FAILED to execute CPostExcludedSorcesRequestJob. [postData=%s] (cf)",excludedChannelsStr.c_str());
    return;
  }
}

bool CGUIDialogBoxeeChannelFilter::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    return OnClick(message);
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeChannelFilter::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeChannelFilter::Show()
{
  CGUIDialogBoxeeChannelFilter* dlg = (CGUIDialogBoxeeChannelFilter*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_CHANNEL_FILTER);
  if (!dlg)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeChannelFilter::Show - FAILED to get WINDOW_DIALOG_BOXEE_CHANNEL_FILTER (cf)");
    return false;
  }

  dlg->DoModal();

  return true;
}

bool CGUIDialogBoxeeChannelFilter::OnClick(CGUIMessage& message)
{
  int controlId = message.GetSenderId();
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), controlId);
  g_windowManager.SendMessage(msg);
  int itemIndex = msg.GetParam1();

  CGUIMessage msg1(GUI_MSG_GET_ITEM, GetID(), controlId, itemIndex);
  OnMessage(msg1);

  CFileItemPtr clickedLinkFileItem = boost::static_pointer_cast<CFileItem>(msg1.GetItem());
  clickedLinkFileItem->Select(!clickedLinkFileItem->IsSelected());

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeChannelFilter::OnClick - after update item [label=%s][IsSelected=%d]. [index=%d][controlId=%d] (cf)",clickedLinkFileItem->GetLabel().c_str(),clickedLinkFileItem->IsSelected(),itemIndex,controlId);

  CStdString sourceid = clickedLinkFileItem->GetProperty("sourceid");
  bool isChannelInExcluded = (m_excludedChannels.find(sourceid) != m_excludedChannels.end());

  if (isChannelInExcluded)
  {
    // channel WAS excluded
    if (clickedLinkFileItem->IsSelected())
    {
      // set to show -> remove from excluded set
      m_excludedChannels.erase(sourceid);
    }
  }
  else
  {
    // channel WASN'T excluded
    if (!clickedLinkFileItem->IsSelected())
    {
      // set to NOT show -> insert to excluded set
      m_excludedChannels.insert(sourceid);
    }
  }

  m_dirty = true;

  return true;
}

bool CGUIDialogBoxeeChannelFilter::LoadExcludedChannels()
{
  m_excludedChannels.clear();

  std::string excludedChannels = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetExcludedSources();

  std::vector<CStdString> excludedChannelsVec;
  CUtil::Tokenize(excludedChannels,excludedChannelsVec,OFFER_DELIMITER);

  for (size_t i=0;i<excludedChannelsVec.size();i++)
  {
    m_excludedChannels.insert(excludedChannelsVec[i]);
  }

  return true;
}

CGUIDialogBoxeeChannelFilter::CPostExcludedSorcesRequestJob::CPostExcludedSorcesRequestJob(const std::string& excludedChannelsStr)
{
  m_excludedChannelsStr = excludedChannelsStr;
}

void CGUIDialogBoxeeChannelFilter::CPostExcludedSorcesRequestJob::Run()
{
  CLog::Log(LOGDEBUG,"CPostExcludedSorcesRequestJob::Run - enter function with [postData=%s] (cf)",m_excludedChannelsStr.c_str());

  BOXEE::BXCurl curl;
  std::string strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.ExcludedSourcesUrl","http://app.boxee.tv/api/excluded_sources");

  std::string excludedChannelsStr = "excluded_sources=" + m_excludedChannelsStr;

  CLog::Log(LOGDEBUG,"CPostExcludedSorcesRequestJob::Run - going to send [url=%s][postData=%s] (cf)",strUrl.c_str(),excludedChannelsStr.c_str());

  curl.HttpPostString(strUrl.c_str(),excludedChannelsStr);

  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().SetExcludedSources(m_excludedChannelsStr);
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateExcludedSourcesNow();

  int activeWindowId = g_windowManager.GetActiveWindow();
  CLog::Log(LOGDEBUG,"CPostExcludedSorcesRequestJob::Run - going to send GUI_MSG_UPDATE to [activeWindowId=%d] (cf)",activeWindowId);
  CGUIMessage refreshShowsMsg(GUI_MSG_UPDATE, WINDOW_DIALOG_BOXEE_CHANNEL_FILTER,0);
  g_windowManager.SendThreadMessage(refreshShowsMsg,activeWindowId);
}
