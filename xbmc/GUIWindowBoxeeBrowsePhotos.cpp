
#include "GUIWindowBoxeeBrowsePhotos.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "GUIWindowManager.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "URL.h"
#include "Util.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "LocalizeStrings.h"
#include "Application.h"
#include "GUIWindowStateDatabase.h"
#include "FileSystem/SpecialProtocol.h"
#include "GUIWindowSlideShow.h"

using namespace std;
using namespace BOXEE;

// This button is only shown in case no albums present
// and user is suggested to browse apps instead
#define BUTTON_SHOW_APPS  131
#define BUTTON_BROWSE_SOURCES 7002

#define PHOTOS_THUMB_VIEW     53
#define PHOTOS_LINE_VIEW_LIST 54

#define ITEM_SUMMARY    9018
#define ITEM_SUMMARY_FLAG "item-summary"

#define SWITCH_VIEW_THUMBS   8001
#define SWITCH_VIEW_LIST   8002
#define SWITCH_VIEW_FLAG "show-thumbnails"

#define PHOTOS_BASE_PATH "boxeedb://pictures/"

#define ITEM_SUMMARY_FLAG "item-summary"
#define ITEM_COUNT_LABEL "item-summary-count"

#define INIT_SELECT_POS_IN_BROWSE_MENU 6

CPhotosSource::CPhotosSource(int iWindowID) : CBrowseWindowSource("photossource", PHOTOS_BASE_PATH, iWindowID) , m_filter(1,"photofilter")
{
}

CPhotosSource::~CPhotosSource()
{

}

void CPhotosSource::BindItems(CFileItemList& items)
{
  for (int i = 0 ; i < items.Size() ; i++)
  {
    if (!m_filter.Apply(&*items.Get(i)))
    {
      items.Remove(i);
      i--;
    }
  }

  CBrowseWindowSource::BindItems(items);
}

CPhotosWindowState::CPhotosWindowState(CGUIWindowBoxeeBrowse* pWindow) : CBrowseWindowStateWithHistory(pWindow)
{
  m_sourceController.AddSource(new CPhotosSource(pWindow->GetID()));

  m_pWindow->SetProperty("is-base-path",true);
}

void CPhotosWindowState::InitState()
{
  // Initialize sort vector
  m_vecSortMethods.clear();
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_ATOZ, SORT_METHOD_LABEL, SORT_ORDER_ASC, g_localizeStrings.Get(53505), "start"));
  m_vecSortMethods.push_back(CBoxeeSort(VIEW_SORT_METHOD_DATE, SORT_METHOD_DATE, SORT_ORDER_DESC, g_localizeStrings.Get(53539), "start"));

  CBrowseWindowStateWithHistory::InitState();
}


void CPhotosWindowState::SetDefaultView()
{
  m_iCurrentView = PHOTOS_THUMB_VIEW;
}

CStdString CPhotosWindowState::GetItemSummary()
{
  CStdString itemSummary = "";
  std::map<CStdString , CStdString> mapTitleItemValue;

  if (!m_sort.m_sortName.empty() && m_sort.m_id != VIEW_SORT_METHOD_ATOZ)
  {
    mapTitleItemValue["sort"] = m_sort.m_sortName;
  }

  mapTitleItemValue["media"] = g_localizeStrings.Get(1);

  if (!CUtil::ConstructStringFromTemplate(g_localizeStrings.Get(90004), mapTitleItemValue,itemSummary))
  {
    itemSummary = g_localizeStrings.Get(1);
    CLog::Log(LOGERROR,"CTvShowsWindowState::GetItemSummary, Error in Strings.xml for the current language [id=90004], the template is bad. (browse)");
  }

  return itemSummary;
}

void CPhotosWindowState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  CPhotosSource* source = new CPhotosSource(m_pWindow->GetID());

  source->SetBasePath(strPath);
  source->SetSortMethod(m_sourceController.GetSortMethod());
  
  m_sourceController.SetNewSource(source);

  m_pWindow->SetProperty("is-base-path",(strPath == PHOTOS_BASE_PATH));

  CBrowseWindowStateWithHistory::OnPathChanged(strPath,bResetSelected);
}


CFileItemList CPhotosWindowState::GetItems()
{
  SourcesMap mapSources = m_sourceController.GetSources();
  CFileItemList output;

  if (!mapSources.empty())
  {
    CBrowseWindowSource* firstSource = mapSources.begin()->second;
    if (firstSource)
    {
      output = firstSource->GetItemList();
    }
    else
    {
      CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, first source is null (browse)");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, no sources found (browse)");
  }

  return output;
}

void CPhotosWindowState::SetCurrentPath(const CStdString& path)
{
  // Return current path under assumption that Local browse screen has only one source
  SourcesMap mapSources = m_sourceController.GetSources();

  if (!mapSources.empty())
  {
    CBrowseWindowSource* firstSource = mapSources.begin()->second;
    if (firstSource)
    {
      firstSource->SetBasePath(path);
    }
    else
    {
      CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, first source is null (browse)");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, no sources found (browse)");
  }
}

CStdString CPhotosWindowState::GetCurrentPath()
{
  // Return current path under assumption that Local browse screen has only one source
  SourcesMap mapSources = m_sourceController.GetSources();

  if (!mapSources.empty())
  {
    CBrowseWindowSource* firstSource = mapSources.begin()->second;
    if (firstSource)
    {
      return firstSource->GetCurrentPath();
    }
    else
    {
      CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, first source is null (browse)");
    }
  }
  else
  {
    CLog::Log(LOGWARNING, "CPhotosWindowState::GetCurrentPath, no sources found (browse)");
  }

  return "";
}


void CPhotosWindowState::Refresh(bool bResetSelected)
{
  CBrowseWindowState::Refresh(bResetSelected);

  m_pWindow->SetProperty(ITEM_SUMMARY_FLAG,GetItemSummary());
}

bool CPhotosWindowState::OnBack()
{
  if (!CBrowseWindowStateWithHistory::OnBack())
  {
    CBrowseWindowState::OnBack();
  }

  return false;
}

void CPhotosWindowState::FromHistory(CBrowseStateHistoryItem* historyItem)
{
  CPhotosSource* source;
  if (!historyItem) return;

  m_sourceController.RemoveAllSources();

  for (size_t i = 0; i < historyItem->m_vecSources.size(); i++)
  {
    CStdString strSourceId = historyItem->m_vecSources[i].m_strSourceId;
    CStdString strBasePath = historyItem->m_vecSources[i].m_strBasePath;

    source = new CPhotosSource( m_pWindow->GetID() );

    if (strBasePath == PHOTOS_BASE_PATH)
    {
      m_pWindow->SetProperty("is-base-path",true);
    }

    source->SetBasePath(strBasePath);
    source->SetSourceId(strSourceId);
    source->SetSortMethod(m_sourceController.GetSortMethod());

    m_sourceController.AddSource(source);

    source = NULL;
  }

  m_iSelectedItem = historyItem->m_iSelectedItem;
}

CGUIWindowBoxeeBrowsePhotos::CGUIWindowBoxeeBrowsePhotos() : CGUIWindowBoxeeBrowse(WINDOW_BOXEE_BROWSE_PHOTOS, "boxee_browse_photos.xml")
{
  SetWindowState(new CPhotosWindowState(this));
}

CGUIWindowBoxeeBrowsePhotos::~CGUIWindowBoxeeBrowsePhotos()
{
}

bool CGUIWindowBoxeeBrowsePhotos::OnAction(const CAction &action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    if (GetPropertyBOOL("empty"))
    {
      OnBack();
      return true;
    }
  }
  break;
  }

  return CGUIWindowBoxeeBrowse::OnAction(action);
}

bool CGUIWindowBoxeeBrowsePhotos::OnClick(int iItem)
{
  CFileItem item;
  if (!GetClickedItem(iItem, item))
    return true;

  if (item.IsPicture())
  {
    CGUIWindowSlideShow *pSlideShow = (CGUIWindowSlideShow *)g_windowManager.GetWindow(WINDOW_SLIDESHOW);
    CFileItemList slideshowItems = ((CPhotosWindowState*)m_windowState)->GetItems();

    if (pSlideShow && !slideshowItems.IsEmpty())
    {
      if (g_application.IsPlayingVideo())
        g_application.StopPlaying();

      pSlideShow->Reset();
      pSlideShow->RunSlideShow(slideshowItems,false,false,item.m_strPath);
      return true;
    }
  }

  return CGUIWindowBoxeeBrowse::OnClick(iItem);
}

bool CGUIWindowBoxeeBrowsePhotos::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == SWITCH_VIEW_THUMBS || iControl == SWITCH_VIEW_LIST)
    {
      SetProperty(SWITCH_VIEW_FLAG, !GetPropertyBOOL(SWITCH_VIEW_FLAG));
      return true;
    }
    if (iControl == BUTTON_SHOW_APPS)
    {
      g_windowManager.ActivateWindow(WINDOW_BOXEE_BROWSE_APPS , "boxeeui://apps/?category=all&type=photos");
      return true;
    }
  }
  break;
  }
  
  return CGUIWindowBoxeeBrowse::OnMessage(message);
}   

void CGUIWindowBoxeeBrowsePhotos::ConfigureState(const CStdString& param)
{
  CGUIWindowBoxeeBrowse::ConfigureState(param);

  std::map<CStdString, CStdString> optionsMap;
  CURI properties(param);

  if (properties.GetProtocol().compare("boxeeui") == 0)
  {
    optionsMap = properties.GetOptionsAsMap();

    if (optionsMap.find("path") != optionsMap.end())
    {
      CStdString path = optionsMap["path"];

      ((CPhotosWindowState*)m_windowState)->SetCurrentPath(path);
    }
  }
}

CStdString CGUIWindowBoxeeBrowsePhotos::GetItemDescription()
{
  CStdString strPath = ((CPhotosWindowState*)m_windowState)->GetCurrentPath();
  CStdString translatedPath = "";

  if ((CUtil::IsHD(strPath) || CUtil::IsSmb(strPath) || CUtil::IsUPnP(strPath)) && !strPath.IsEmpty() &&  strPath != PHOTOS_BASE_PATH)
  {
    // Translate the path
    translatedPath = _P(strPath);

    CStdString shortPath = translatedPath;
    CUtil::MakeShortenPath(translatedPath,shortPath,60);
    translatedPath = shortPath;

    if (!translatedPath.IsEmpty())
    {
      CUtil::RemovePasswordFromPath(translatedPath);
    }
  }

  if (!translatedPath.IsEmpty())
    return translatedPath;
  else
    return " ";
}

void CGUIWindowBoxeeBrowsePhotos::GetStartMenusStructure(std::list<CFileItemList>& browseMenuLevelList)
{
  CStdString category = m_windowState->GetCategory();

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowsePhotos::GetStartMenusStructure - enter function [category=%s] (bm)",category.c_str());

  CBoxeeBrowseMenuManager::GetInstance().GetFullMenuStructure("mn_local_files",browseMenuLevelList);

  m_initSelectPosInBrowseMenu = INIT_SELECT_POS_IN_BROWSE_MENU;

  CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowsePhotos::GetStartMenusStructure - after set [browseMenuLevelListSize=%zu][m_initSelectPosInBrowseMenu=%d]. [category=%s] (bm)",browseMenuLevelList.size(),m_initSelectPosInBrowseMenu,category.c_str());

  return CGUIWindowBoxeeBrowse::GetStartMenusStructure(browseMenuLevelList);
}

