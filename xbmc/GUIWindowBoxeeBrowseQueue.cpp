
#include "GUIWindowBoxeeBrowseQueue.h"
#include "GUIDialogBoxeeWatchLaterGetStarted.h"
#include "GUIWindowStateDatabase.h"
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
#include "bxutils.h"
#include "FileItem.h"
#include "lib/libBoxee/boxee.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define GET_ALL_QUEUE_PATH "feed://queue"

#define QUEUE_VIEW  52
#define BTN_GET_STARTED     7092
#define EMPTY_STATE_LABEL   7094

#define ITEM_SUMMARY_FLAG "item-summary"

CQueueSource::CQueueSource(int iWindowID) : CBrowseWindowSource("queuesource", "feed://queue", iWindowID)
{

}

CQueueSource::~CQueueSource()
{

}

void CQueueSource::AddStateParameters(std::map <CStdString, CStdString>& mapOptions)
{
  if (!m_mapFilters["type"].empty())
  {
    CStdString queueType = m_mapFilters["type"];
    CUtil::URLEncode(queueType);
    mapOptions["type"] = queueType;
  }
}

CQueueWindowState::CQueueWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowState(pWindow)
{
  m_sourceController.SetNewSource(new CQueueSource(pWindow->GetID()));
}

void CQueueWindowState::SetDefaultView()
{
  m_iCurrentView = QUEUE_VIEW;
}

void CQueueWindowState::SetQueueType(const CStdString& queueType)
{
  m_sourceController.SetFilter("type",queueType);
  m_queueTypeStr = queueType;
}

CStdString CQueueWindowState::GetQueueType()
{
  return m_queueTypeStr;
}

void CQueueWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());
}

CStdString CQueueWindowState::GetItemSummary()
{
  CStdString itemSummary = "";

  BOXEE::CQueueItemsType::QueueItemsTypeEnums queueTypeEnum = BOXEE::CQueueItemsType::GetQueueItemTypeAsEnum(m_queueTypeStr);

  switch (queueTypeEnum)
  {
  case BOXEE::CQueueItemsType::QIT_ALL:
  {
    itemSummary = g_localizeStrings.Get(53509);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_CLIP:
  {
    itemSummary = g_localizeStrings.Get(33016);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_TVSHOW:
  {
    itemSummary = g_localizeStrings.Get(53912);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_MOVIE:
  {
    itemSummary = g_localizeStrings.Get(53913);
  }
  break;
  default:
  {
    // do nothing
  }
  }

  itemSummary += " ";
  itemSummary += g_localizeStrings.Get(57010);

  return itemSummary;
}


CGUIWindowBoxeeBrowseQueue::CGUIWindowBoxeeBrowseQueue() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_QUEUE, "boxee_browse_queue.xml")
{
  SetWindowState(new CQueueWindowState(this));
}

CGUIWindowBoxeeBrowseQueue::~CGUIWindowBoxeeBrowseQueue()
{

}

bool CGUIWindowBoxeeBrowseQueue::WatchLaterGetStarted()
{
  CGUIDialogBoxeeWatchLaterGetStarted* pGetStarted = (CGUIDialogBoxeeWatchLaterGetStarted*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_GET_STARTED);
  if(pGetStarted)
  {
    pGetStarted->DoModal();

    if(pGetStarted->m_bIsConfirmed || !g_application.IsConnectedToInternet())
    {
      return true;
    }
  }
  return false;
}

void CGUIWindowBoxeeBrowseQueue::LaunchWatchLater()
{
  int size = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetQueueSize(CQueueItemsType::QIT_ALL);

  if(size == 0)
  {
    CGUIWindowStateDatabase stateDB;
    CStdString isFirstTimeUser;
    bool settingExists = stateDB.GetUserSetting("firstTimeWatchLaterRequest", isFirstTimeUser);
    if(!settingExists)
    {
      stateDB.SetUserSetting("firstTimeWatchLaterRequest","true");
      isFirstTimeUser = "true";
    }
    if(isFirstTimeUser == "true")
    {
      if(!WatchLaterGetStarted())
      {
        return;
      }
      stateDB.SetUserSetting("firstTimeWatchLaterRequest","false");
    }
  }
  g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_QUEUE,"boxeeui://queue/?type=clip");
}

bool CGUIWindowBoxeeBrowseQueue::OnAction(const CAction &action)
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

bool CGUIWindowBoxeeBrowseQueue::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_UPDATE:
  {
    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::OnMessage - GUI_MSG_UPDATE - Enter function with [SenderId=%d] (queue)(browse)",message.GetSenderId());

    if (message.GetSenderId() != GetID())
    {
      return true;
    }

    bool clearCache = message.GetParam1();

    CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::OnMessage - GUI_MSG_UPDATE - Enter function with [clearCache=%d] (queue)(browse)",clearCache);

    if (clearCache)
    {
      g_directoryCache.ClearSubPaths("feed://queue/");
    }
  }
  break;
  case GUI_MSG_CLICKED:
  {
    if (OnClick(message))
    {
      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::OnMessage - GUI_MSG_CLICKED - click was handled -> RETURN (queue)(browse)");
      return true;
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnMessage(message);
}

bool CGUIWindowBoxeeBrowseQueue::OnClick(CGUIMessage& message)
{
  int controlId = message.GetSenderId();

  if (controlId == BTN_GET_STARTED)
  {

  }

  return false;
}

void CGUIWindowBoxeeBrowseQueue::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.find("type") != optionsMap.end())
    {
      CStdString queueType = optionsMap["type"];

      ((CQueueWindowState*)m_windowState)->SetQueueType(queueType);

      SetProperty("is-category-movie",(CQueueItemsType::GetQueueItemTypeAsEnum(queueType) == CQueueItemsType::QIT_MOVIE));
    }
  }
}

void CGUIWindowBoxeeBrowseQueue::ShowItems(CFileItemList& list, bool append)
{
  CGUIWindowBoxeeBrowse::ShowItems(list,append);

  bool isEmpty = GetPropertyBOOL("empty");
  if (isEmpty)
  {
    CStdString currentMenuLevelStr = g_settings.GetSkinString(g_settings.TranslateSkinString("activemenulevel"));
    int iCurrentMenuLevel = atoi(currentMenuLevelStr.c_str());
    SET_CONTROL_FOCUS((BROWSE_MENU_LIST_LEVEL_START_ID + iCurrentMenuLevel - 1),0);
  }
}

CStdString CGUIWindowBoxeeBrowseQueue::GetItemDescription()
{
  bool isOne = (m_windowState->GetTotalItemCount() == 1);

  CStdString queueTypeStr="";

  BOXEE::CQueueItemsType::QueueItemsTypeEnums queueTypeEnum = BOXEE::CQueueItemsType::GetQueueItemTypeAsEnum(((CQueueWindowState*)m_windowState)->GetQueueType());

  switch (queueTypeEnum)
  {
  case BOXEE::CQueueItemsType::QIT_ALL:
  {
    queueTypeStr = isOne ? g_localizeStrings.Get(90051) : g_localizeStrings.Get(90050);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_CLIP:
  {
    queueTypeStr = isOne ? g_localizeStrings.Get(53771) : g_localizeStrings.Get(33016);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_TVSHOW:
  {
    queueTypeStr = isOne ? g_localizeStrings.Get(20359) : g_localizeStrings.Get(90048);
  }
  break;
  case BOXEE::CQueueItemsType::QIT_MOVIE:
  {
    queueTypeStr = isOne ? g_localizeStrings.Get(20338) : g_localizeStrings.Get(90041);
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CGUIWindowBoxeeBrowseQueue::GetItemDescription - FAILED to handle queue type [%s=%d] (bm)",((CQueueWindowState*)m_windowState)->GetQueueType().c_str(),queueTypeEnum);
  }
  }

  CStdString strItemCount="";
  strItemCount.Format("%d %s", m_windowState->GetTotalItemCount() , queueTypeStr.c_str());

  return strItemCount;
}

void CGUIWindowBoxeeBrowseQueue::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();
  CStdString queueType = ((CQueueWindowState*)m_windowState)->GetQueueType();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::GetStartMenusStructure - enter function [category=%s][queueType=%s] (bm)",category.c_str(),queueType.c_str());

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_watch_later",browseMenuLevelList);

  m_initSelectPosInBrowseMenu = (int)CQueueItemsType::GetQueueItemTypeAsEnum(queueType);

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu][m_initSelectPosInBrowseMenu=%d]. [category=%s] (bm)",browseMenuLevelList.size(),m_initSelectPosInBrowseMenu,category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);

  //CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowseQueue::GetStartMenusStructure - exit function with [browseMenuLevelStackSize=%zu]. [category=%s] (bm)",browseMenuLevelStack.size(),category.c_str());
}

