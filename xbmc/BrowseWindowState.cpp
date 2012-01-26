
#include "Application.h"
#include "GUIWindowBoxeeBrowse.h"
#include "PlayListPlayer.h"
#include "BoxeeUtils.h"
#include "GUIDialogBoxeeVideoCtx.h"
#include "GUIDialogBoxeeAppCtx.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "MusicInfoTag.h"
#include "URL.h"
#include "GUIListContainer.h"
#include "utils/LabelFormatter.h"
#include "FileSystem/File.h"
#include "SkinInfo.h"
#include "lib/libPython/XBPython.h"
#include "GUIDialogOK.h"
#include "utils/GUIInfoManager.h"
#include "ThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/Directory.h"
#include "GUIBoxeeViewStateFactory.h"
#include "GUIImage.h"
#include "BoxeeViewDatabase.h"
#include "SpecialProtocol.h"
#include "BoxeeItemLauncher.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"
#include "GUIDialogBoxeeDropdown.h"
#include "GUIDialogBoxeeSearch.h"

#include <vector>

// Mark the range of view controls
#define CONTROL_VIEW_START        50
#define CONTROL_VIEW_END          59

#define LABEL_WINDOW_TITLE 820
#define IMAGE_WINDOW_TITLE 810

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

// IMPLEMENTATION OF THE CBrowseWindowState ///////////////////////////

CBrowseWindowState::CBrowseWindowState(CGUIWindow* pWindow)
{
  m_iSearchDepth = 0;
  m_pWindow = pWindow;

  m_displayPathMaxLength = 40;
}

CBrowseWindowState::~CBrowseWindowState()
{
  m_pWindow = NULL;
}

CStdString CBrowseWindowState::CreatePath()
{
  return m_configuration.m_strPath;
}

void CBrowseWindowState::Reset()
{
  m_iSearchDepth = 0;
  m_pWindow->SetProperty("search-set", false);

  SetSort(m_sort);
}

CStdString CBrowseWindowState::GetCurrentPath()
{
  return m_configuration.m_strPath;
}

void CBrowseWindowState::InitState(const CStdString& strPath)
{
  m_configuration = CBrowseWindowConfiguration::GetConfigurationByPath(strPath);
  m_configuration.m_strPath = strPath;
  UpdateWindowProperties();
}

void CBrowseWindowState::UpdateWindowProperties()
{
  if (m_pWindow)
  {
    m_pWindow->SetProperty("path", CUtil::GetTitleFromPath(_P(m_configuration.m_strPath)));
    SetPathToDisplay();
  }
}

void CBrowseWindowState::UpdateConfigurationByType(const CStdString& strType)
{
  m_configuration = CBrowseWindowConfiguration::GetConfigurationByType(strType);
}


void CBrowseWindowState::SetPath(const CStdString &strInitialPath)
{
  m_configuration.m_strPath = strInitialPath;
}

void CBrowseWindowState::SetSelectedItem(int iSelectedItem)
{
  m_configuration.m_iSelectedItem = iSelectedItem;
}

int CBrowseWindowState::GetSelectedItem()
{
  if (m_configuration.m_iSelectedItem > -1)
  {
    return m_configuration.m_iSelectedItem;
  }
  else
  {
   return 0;
  }
}

CStdString CBrowseWindowState::GetWindowStateType()
{
  return m_configuration.m_strType;
}

void CBrowseWindowState::SetBackgroundImage(const CStdString& strImage)
{
  m_configuration.m_strBackgroundImage = strImage;
}

CStdString CBrowseWindowState::GetBackgroundImage()
{
  return m_configuration.m_strBackgroundImage;
}

void CBrowseWindowState::SetLabel(const CStdString& strLabel)
{
  m_configuration.m_strLabel = strLabel;
}

CStdString CBrowseWindowState::GetLabel()
{
  return m_configuration.m_strLabel;
}

bool CBrowseWindowState::OnSort()
{
  CFileItemList sorts;

  for (size_t i = 0; i < m_vecSortMethods.size(); i++)
  {
    CFileItemPtr sortItem (new CFileItem(m_vecSortMethods[i].m_sortName));
    sortItem->SetProperty("value",(int) i);
    sorts.Add(sortItem);
  }

  CStdString value;
  if (CGUIDialogBoxeeDropdown::Show(sorts, "SORT", value))
  {
    SetSort(m_vecSortMethods[atoi(value)]);
    return true;
  }
  return false;
}

void CBrowseWindowState::SetSort(const CBoxeeSort& sort)
{
  m_sort = sort;
  m_pWindow->SetProperty("sort-label", m_sort.m_sortName);
}

void CBrowseWindowState::SortItems(CFileItemList &items)
{
//  CGUIWindowBoxeeBrowse* pWindow = (CGUIWindowBoxeeBrowse*)m_pWindow;
//  CGUIBoxeeViewState* pViewState = pWindow->GetViewState();

  if (m_sort.m_sortMethod != SORT_METHOD_NONE)
  {
    items.Sort(m_sort);
  }
  else if (items.HasProperty("preferredsortmethod") && items.GetPropertyInt("preferredsortmethod") != SORT_METHOD_NONE)
  {
    SORT_METHOD preferredSortMethod = (SORT_METHOD)items.GetPropertyInt("preferredsortmethod");
    SORT_ORDER preferredSortOrder = (SORT_ORDER)items.GetPropertyInt("preferredsortorder");

    items.Sort(preferredSortMethod, preferredSortOrder);
  }
  else
  {
    items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);
  }
//  else if (pViewState && pViewState->GetSortMethod() != SORT_METHOD_NONE)
//  {
//    CBoxeeSort boxeeSort;
//    bool retVal = pViewState->GetBoxeeSort(boxeeSort);
//
//    if(retVal == true)
//    {
//      CLog::Log(LOGDEBUG,"CGUIWindowBoxeeBrowse::SortItems - Going to set FileItemList [path=%s] with BoxeeSort [SortId=%s][SortMethod=%s=%d][SortOrder=%s=%d][m_sortName=%s][FolderPosition=%s] (vns) (browse)",(items.m_strPath).c_str(),(boxeeSort.m_id).c_str(),(CGUIBoxeeViewState::GetSortMethodAsString(boxeeSort.m_sortMethod)).c_str(),boxeeSort.m_sortMethod,(CGUIBoxeeViewState::GetSortOrderAsString(boxeeSort.m_sortOrder)).c_str(),boxeeSort.m_sortOrder,(boxeeSort.m_sortName).c_str(),(boxeeSort.m_folderPosition).c_str());
//      items.Sort(boxeeSort);
//    }
//  }
}

CStdString CBrowseWindowState::GetSearchString()
{
  CStdString searchStringEncoded = m_strSearchString;
  CUtil::URLEncode(searchStringEncoded);
  return searchStringEncoded;
}

bool CBrowseWindowState::OnSearchStart()
{
  m_iSearchDepth++;

  // going to search -> save window state
  SaveWindowState();

  m_strSearchString = "";
  if (CGUIDialogBoxeeSearch::ShowAndGetInput(m_strSearchString, m_strSearchType, "Search", false, false))
  {
    m_configuration.ClearActiveFilters();

    CStdString searchLabel;
    searchLabel.Format("SEARCH RESULTS FOR: %s", m_strSearchString.c_str());
    m_pWindow->SetProperty("search-label", searchLabel);
    m_pWindow->SetProperty("searching", false);
    m_pWindow->SetProperty("search-set", true);

    return true;
  }

  m_iSearchDepth--;

  if (m_iSearchDepth == 0)
    m_pWindow->SetProperty("searching", false);

  return false;
}

bool CBrowseWindowState::OnSearchEnd()
{
  if (m_iSearchDepth > 0)
  {
    RestoreWindowState();

    m_iSearchDepth = 0;
    m_pWindow->SetProperty("search-set", false);
    m_pWindow->SetProperty("search-label", "");

    return true;
  }

  return false;
}

void CBrowseWindowState::SetSearchType(const CStdString& strSearchType)
{
  m_strSearchType = strSearchType;
}

bool CBrowseWindowState::InSearchMode()
{
  return m_iSearchDepth > 0;
}

void CBrowseWindowState::SaveWindowState()
{
  // nothing to do here
}

void CBrowseWindowState::RestoreWindowState()
{
  // Restore sort
  SetSort(m_sort);
}

void CBrowseWindowState::ProcessItems(const CFileItemList& vecModelItems, CFileItemList& vecViewItems)
{
  vecViewItems.Clear();

  if (vecModelItems.Size() == 0)
    return;

  for(int i = 0; i < vecModelItems.Size(); i++)
  {
    CFileItemPtr pItem = vecModelItems.Get(i);
    bool bFilter = m_configuration.ApplyFilter(pItem.get());
    pItem->SetProperty("filter", bFilter);
  }


  // We copy all items first in order to preserve the original properties
  // set by directory
  vecViewItems.Copy(vecModelItems);

//  vecViewItems.m_strPath = vecModelItems.m_strPath;
//  vecViewItems.SetProperty("parentPath", vecModelItems.GetProperty("parentPath"));

  // Copy all items that have true filter property
  for (int i = 0; i < vecViewItems.Size(); i++)
  {
    CFileItemPtr pItem = vecViewItems.Get(i);

    if (!pItem->GetPropertyBOOL("filter"))
    {
      vecViewItems.Remove(i);
      i--;
    }
  }

  SortItems(vecViewItems);

  // We need to set the "browsemode" property because it is used by the view state factory
  // within the browse screen
  vecViewItems.SetProperty("browsemode", m_configuration.m_strType);
}

CBrowseWindowState* CBrowseWindowState::ToHistory()
{
  CBrowseWindowState* state = new CBrowseWindowState(m_pWindow);
  state->m_configuration = m_configuration;
  state->m_strInitialPath = m_strInitialPath;

  return state;
}

void CBrowseWindowState::FromHistory(CBrowseWindowState* state)
{
  m_configuration = state->m_configuration;
  m_strInitialPath = state->m_strInitialPath;
}

// CALLBACKS

void CBrowseWindowState::OnLoading()
{
  m_configuration.m_loaded = false;
}

void CBrowseWindowState::OnLoadFailed()
{
  m_configuration.m_loaded = false;
}

void CBrowseWindowState::OnLoaded()
{
  m_configuration.m_loaded = true;
}

void CBrowseWindowState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  m_configuration.m_strPath = strPath;

  m_pWindow->SetProperty("path", CUtil::GetTitleFromPath(_P(m_configuration.m_strPath)));
  SetPathToDisplay();

  if (bResetSelected)
      m_configuration.m_iSelectedItem = 0;

  UpdateFilters(strPath);
}

void CBrowseWindowState::UpdateFilters(const CStdString& strPath)
{
  CBrowseWindowConfiguration::UpdateFiltersByPath(strPath, m_configuration);
}

void CBrowseWindowState::SetPathToDisplay()
{
  CStdString pathToDisplay = "";
  CStdString path = _P(m_configuration.m_strPath);

  if (CUtil::IsHD(path) || CUtil::IsSmb(path) || CUtil::IsUPnP(path))
  {
    // remove password
    CUtil::RemovePasswordFromPath(path);

    // Shorten the path
    pathToDisplay = path;
    CUtil::MakeShortenPath(path,pathToDisplay,m_displayPathMaxLength);
  }
  else
  {
    pathToDisplay = "";
  }

  m_pWindow->SetProperty("displaypath", pathToDisplay);
}

// ///////////////////////////////////////////////////////////////////////

