
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
#include "GUIDialogOK.h"
#include "utils/GUIInfoManager.h"
#include "ThumbLoader.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/Directory.h"
#include "BoxeeViewDatabase.h"
#include "SpecialProtocol.h"
#include "BoxeeItemLauncher.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "ItemLoader.h"
#include "GUIDialogBoxeeSortDropdown.h"
#include "GUIDialogBoxeeSearch.h"
#include "GUIWindowStateDatabase.h"
#include "FileItem.h"
#include "GUIDialogBoxeeMainMenu.h"
#include "GUIDialogBoxeeBrowseMenu.h"
#include "VideoInfoTag.h"
#include "UPnPDirectory.h" // for directory renaming
#include "GUIToggleButtonControl.h"
#include "SMBUtils.h"

// Mark the range of view controls
#define CONTROL_VIEW_START        50
#define CONTROL_VIEW_END          59

#define LABEL_WINDOW_TITLE 820
#define IMAGE_WINDOW_TITLE 810

#define SORT_BUTTON     8014

#define IS_ALLOW_FILTER 900

#define MAX_DISPLAY_PATH_LENGTH 40

using namespace std;
using namespace BOXEE;
using namespace DIRECTORY;

CCriticalSection CBrowseWindowStateHistory::m_historyLock;
std::map<int, std::stack<CBrowseStateHistoryItem*> > CBrowseWindowStateHistory::m_history;

CBrowseWindowSource::CBrowseWindowSource(const CStdString& strSourceId, const CStdString& strPath, int iWindow)
{
  m_strSourceId = strSourceId;
  m_strBasePath = strPath;
  m_iWindowID = iWindow;
  m_bActive = true;
  m_bReady = false;
  m_bDepleted = false;
  m_iPageSize = -1;
  m_iStart = 0;
  m_iMarker=0;
  m_iTotalItemsCount=0;
  m_vecFileItems.SetFastLookup(true, "itemid");
}

CBrowseWindowSource::~CBrowseWindowSource()
{
  m_vecFileItems.Clear();
}

bool CBrowseWindowSource::GetCredentials(CFileItemList& items)
{
  if (items.GetPropertyBOOL("getcredentials"))
  {

    CStdString translatedPath =  items.m_strPath;

    if (CUtil::IsSmb(translatedPath))
    {
      translatedPath = SMBUtils::TranslatePath(translatedPath);
    }

    g_passwordManager.SetSMBShare(translatedPath);
    if (g_passwordManager.GetSMBShareUserPassword())
    {
      /* must do this as our urlencoding for spaces is invalid for samba */
      /* and doing double url encoding will fail */
      /* curl doesn't decode / encode filename yet */
      CURI urlnew( g_passwordManager.GetSMBShare() );
      CStdString newPath = BoxeeUtils::URLEncode(urlnew);
      CUtil::UrlDecode(newPath); // we have to decode the url before it is sent to the loader

      m_strCurrentPath = newPath;

      // Get new list of items using provided credentials
      RequestItems(true);

      // Do not continue processing
      return false;
    }
    else
    {
      // could not get credentials that were required
      CLog::Log(LOGWARNING, "Could not get credentials for path = %s (browse)", items.m_strPath.c_str());
      return false;
    }
  }
  else
  {
    // no need for credentials
    return true;
  }
}

int CBrowseWindowSource::GetTotalItemsCount()
{
  return m_iTotalItemsCount;
}

void CBrowseWindowSource::BindItems(CFileItemList& items)
{
  CLog::Log(LOGDEBUG, "CBrowseWindowSource::BindItems, source id %s received %d items for path %s (browse)", m_strSourceId.c_str(), items.Size(), items.m_strPath.c_str());

  // Check if credentials are required
  if (!GetCredentials(items))
  {
    return;
  }

  // Handling paged sources
  if (m_iPageSize > 0)
  {
    // In case we have received zero items, we assume the source is depleted
    if (items.IsEmpty())
    {
      m_bDepleted = true;
      m_bReady = true; //the source is ready even though we received 0 items
      CLog::Log(LOGDEBUG,"CBrowseWindowSource::BindItems, source id=[%s] got no pages or items from server. (browse)", m_strSourceId.c_str());
      return;
    }

    m_vecFileItems.Append(items);

    m_iTotalItemsCount = items.GetPageContext().m_totalResults;

    BoxeeUtils::IndexItems(m_mapFileItemsIndex , items);

    m_iStart += items.Size();
  }
  else
  {
    //handle sources without paging
    m_vecFileItems.Assign(items);

    m_iTotalItemsCount = items.Size();

    BoxeeUtils::IndexItems(m_mapFileItemsIndex,m_vecFileItems);
    m_iMarker = 0; //those with paging should update this value
    m_bDepleted = true;
    CLog::Log(LOGDEBUG,"CBrowseWindowSource::BindItems, paging is disabled source id=[%s], [%d] item(s) received. (browse)", m_strSourceId.c_str(), m_vecFileItems.Size());
  }

  m_bReady = true;
}

void CBrowseWindowSource::UpdateItem(const CFileItem* pItem)
{
  CStdString itemId = pItem->GetProperty("itemid");
  CFileItemPtr pCurrentItem = m_mapFileItemsIndex[itemId];
  if (pCurrentItem)
  {
    // If the new item does not have a thumbnail, preserve the one we have now
    CStdString strPreviousIcon;
    strPreviousIcon = pCurrentItem->GetIconImage();

    // Copy the entire item
    *pCurrentItem = *pItem;

    // If the new item does not have an icon, restore the old one, unless it is a loading item
    if (!pCurrentItem->HasIcon() && !pCurrentItem->GetPropertyBOOL("IsLoading"))
    {
      pCurrentItem->SetIconImage(strPreviousIcon);
    }
  }
}

SourceItemState CBrowseWindowSource::PopItem(CFileItemPtr& item)
{
  SourceItemState retVal = NO_MORE_ITEMS;

  if (m_iMarker < m_vecFileItems.Size() )
  {
    item = m_vecFileItems.Get(m_iMarker);
    retVal = GOT_ITEMS;
    m_iMarker++;
  }
  else
  {
    if (m_iPageSize > 0 && !m_bDepleted)
    {
      RequestItems();
      retVal = WAITING_FOR_ITEMS;
    }
  }

  if (m_iMarker >= m_vecFileItems.Size())
  {
    //we've just returned the last item we had, DO NOT SKIP THIS STEP, its vital to mark the source as NOT ready.
    m_bReady = false;
    CLog::Log(LOGDEBUG,"CBrowseWindowSource::PopItem, all items popped for source id:['%s'], m_bReady:[%s],m_bDepleted:[%s],m_iPageSize:[%d] (merge)" ,m_strSourceId.c_str(),m_bReady?"true":"false",m_bDepleted?"true":"false",m_iPageSize);
  }

  return retVal;
}

SourceItemState CBrowseWindowSource::PeekItem(CFileItemPtr& item)
{
  SourceItemState retVal = NO_MORE_ITEMS;

  if (m_iMarker < m_vecFileItems.Size() )
  {
    item = m_vecFileItems.Get(m_iMarker);
    retVal = GOT_ITEMS;
  }
  else
  {
    if (m_iPageSize > 0 && !m_bDepleted)
    {
      RequestItems();
      retVal = WAITING_FOR_ITEMS;
    }
  }

  return retVal;
}

void CBrowseWindowSource::ClearItems()
{
  CLog::Log(LOGDEBUG, "CBrowseWindowSource::ClearItems, clear source, path = %s (browse)", m_strBasePath.c_str());
  m_vecFileItems.Clear();
  m_mapFileItemsIndex.clear();
  m_iTotalItemsCount = 0;
  m_iStart = 0;
  m_iMarker=0;
}

void CBrowseWindowSource::RequestItems(bool bUseCurPath)
{
  std::map < CStdString , CStdString > mapOptions;

  AddStateParameters(mapOptions);

  if( bUseCurPath )
  {
    m_strCurrentPath = m_strCurrentPath + BoxeeUtils::BuildParameterString(mapOptions);
  }
  else
  {
    m_strCurrentPath = m_strBasePath + BoxeeUtils::BuildParameterString(mapOptions);
  }

  m_bReady = false;
  m_bDepleted = false;

  CStdString pathNoPassword = m_strCurrentPath;
  CUtil::RemovePasswordFromPath(pathNoPassword);
  CLog::Log(LOGDEBUG,"CBrowseWindowSource::RequestItems - request item by source [basePath=%s][CurrentPath=%s] for [windowId=%d] (browse)",m_strBasePath.c_str(),pathNoPassword.c_str(),m_iWindowID);

  m_iRequestId = g_application.GetItemLoader().AddControl(m_iWindowID, 0, m_strCurrentPath, m_sourceSort.m_sortMethod, m_sourceSort.m_sortOrder);
}

void CBrowseWindowSource::AddStateParameters(std::map<CStdString, CStdString>& mapOptions)
{
  if (m_iPageSize > 0)
  {
    CStdString strCount;
    CStdString strStart;

    strCount.Format("%d",m_iPageSize);
    strStart.Format("%d",m_iStart);

    mapOptions["count"] = strCount;
    mapOptions["start"] = strStart;
  }
}

bool CBrowseWindowSource::IsPathRelevant(const CStdString& strPath)
{
  bool isPathRelevant = false;

  if (m_strCurrentPath == strPath)
  {
    isPathRelevant = true;
  }

  return isPathRelevant;
}

void CBrowseWindowSource::Reset()
{
  m_vecFileItems.Clear();
  m_mapFileItemsIndex.clear();
  m_iMarker = 0;
  m_iTotalItemsCount = 0;
}

void CBrowseWindowSource::SetFilter(const CStdString& strName, const CStdString strValue)
{
  m_mapFilters[strName] = strValue;
}

void CBrowseWindowSource::ClearFilter(const CStdString& strName)
{
  map<CStdString, CStdString>::iterator it = m_mapFilters.find(strName);
  if (it != m_mapFilters.end())
    m_mapFilters.erase(it);
}

void CBrowseWindowSource::SetSortMethod(const CBoxeeSort& bxSort)
{
  m_sourceSort = bxSort;
}

// //////////////////////////////////////////////////////////////
// Controller

CBrowseWindowSourceController::CBrowseWindowSourceController( IBrowseWindowView* pView , int pageSize)
{
  m_pView = pView;

  m_vecCurrentViewItems.SetFastLookup(true, "itemid");

  m_iPageSize = pageSize;

  m_bPendingBind = false;

  m_iTotalItemsCount = 0;
}

CBrowseWindowSourceController::~CBrowseWindowSourceController()
{
  SourcesMap::iterator it = m_sources.begin();

  while (it != m_sources.end())
  {
    delete it->second;
    it++;
  }
}

int CBrowseWindowSourceController::RemoveAllSources()
{
  int size = m_sources.size();

  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    delete it->second;

    it++;
  }

  m_sources.clear();
  return size;
}

CBrowseWindowSource* CBrowseWindowSourceController::GetSourceById(const CStdString& strSourceId)
{
  SourcesMap::iterator it = m_sources.find(strSourceId);
  if (it != m_sources.end())
  {
    return it->second;
  }
  return NULL;
}

void CBrowseWindowSourceController::ClearItems()
{
  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    it->second->ClearItems();
    it++;
  }
  m_mapViewItemsIndexByBoxeeId.clear();
  m_mapViewItemsIndexByItemId.clear();
  m_vecCurrentPageItems.Clear();
  m_vecCurrentViewItems.Clear();
}

void CBrowseWindowSourceController::ClearCachedSources()
{
  for (SourcesMap::iterator it = m_sources.begin(); it != m_sources.end(); it++)
  {
    it->second->ClearCachedItems();
    it++;
  }
}

void CBrowseWindowSourceController::Refresh()
{
  ClearItems();
  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    if (it->second->IsActive())
      it->second->RequestItems();
    it++;
  }
}

bool CBrowseWindowSourceController::SetNewSource(CBrowseWindowSource* source)
{
  ClearItems();
  RemoveAllSources();
  return AddSource(source);
}

void CBrowseWindowSourceController::ActivateAllSources(bool bActivate, bool bReset)
{
  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    if (bReset)
    {
      it->second->ClearItems();
    }

    it->second->Activate(bActivate);

    it++;
  }
}

void CBrowseWindowSourceController::ActivateSource(const CStdString& strSourceId, bool bActivate, bool bReset)
{

  SourcesMap::iterator it = m_sources.find(strSourceId);
  if (it != m_sources.end())
  {
    if (bReset)
    {
      it->second->ClearItems();
    }
    
    it->second->Activate(bActivate);
  }
}

bool CBrowseWindowSourceController::AddSource(CBrowseWindowSource* source)
{
  if (m_sources.find(source->GetSourceId())!=m_sources.end())
  {
    CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::AddSource source from the same base path already exists (path: %s) (browse)", source->GetBasePath().c_str());
    return false; // there should not be same source twice.
  }
  else
  {
    m_sources[source->GetSourceId()] = source;
    return true;
  }
}

bool CBrowseWindowSourceController::RemoveSource(const CStdString& strSourceId)
{
  SourcesMap::iterator it = m_sources.find(strSourceId);
  if (it != m_sources.end())
  {
    delete it->second;
    m_sources.erase(it);
    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CBrowseWindowSourceController::RemoveSource could not find id %s (browse)", strSourceId.c_str());
    return false;
  }
}

bool CBrowseWindowSourceController::IsPathRelevant(CFileItemList& items)
{
  if (items.HasProperty("requestId"))
  {
    int requestId = items.GetPropertyInt("requestId");
    return IsPathRelevant(items.m_strPath,requestId);
  }
  return false;
}

bool CBrowseWindowSourceController::IsPathRelevant(const CStdString& strPath, int requestId)
{
  for (SourcesMap::iterator it = m_sources.begin(); it != m_sources.end() ; it++)
  {
    if (it->second->IsPathRelevant(strPath) && it->second->IsActive() && requestId == it->second->GetRequestId())
    {
      return true;
    }
  }
  return false;
}

void CBrowseWindowSourceController::BindItems(CFileItemList& items)
{
  CStdString strPath = items.m_strPath;

  if (m_sources.empty())
  {
    CLog::Log(LOGERROR, "CBrowseWindowSourceController::BindItems - could not bind items from [path=%s] since list of sources is empty (browse)",strPath.c_str());
    return;
  }
  
  CLog::Log(LOGDEBUG,"CBrowseWindowSourceController::BindItems - received [%d] items for [path=%s] (browse)",items.Size(),strPath.c_str());

  // Find the source matching to the current path
  SourcesMap::iterator it = m_sources.begin();

  int requestId = items.GetPropertyInt("requestId");

  while (it != m_sources.end())
  {
    if (it->second->IsPathRelevant(strPath))
    {

      if (!it->second->IsActive())
      {
        CLog::Log(LOGWARNING,"CBrowseWindowSourceController::BindItems - not binding items from [path=%s] since source of [id=%s] and [path=%s] is NOT active [IsActive=%d] (browse)",strPath.c_str(),it->second->GetSourceId().c_str(),it->second->GetCurrentPath().c_str(),it->second->IsActive());
        return;
      }

      if (requestId != it->second->GetRequestId())
      {
        CLog::Log(LOGWARNING,"CBrowseWindowSourceController::BindItems - not binding items from [path=%s] since source of [id=%s] and [path=%s] request id mismatch, source request id:[%d] vs [%d] (browse)",strPath.c_str(),it->second->GetSourceId().c_str(),it->second->GetCurrentPath().c_str(),requestId,it->second->GetRequestId());
        return;
      }

      it->second->BindItems(items);
      break;
    }

    it++;
  }

  if (it == m_sources.end())
  {
    CLog::Log(LOGWARNING,"CBrowseWindowSourceController::BindItems - unable to bind items for [path=%s] (browse)",strPath.c_str());
    return;
  }

  ShowReadySources();
}

void CBrowseWindowSourceController::ShowReadySources()
{
  bool bReady = true;
  SourceItemState mergeResult=GOT_ITEMS;

  int totalItemCount = 0;
  int totalActiveSources = 0;

  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    if (it->second->IsActive()) //check only if its an active source
    {
      bReady &= it->second->IsReady();

      CLog::Log(LOGDEBUG,"CBrowseWindowSourceController::ShowReadySources, '%s' source %s ready. (merge)", it->second->GetSourceId().c_str(), it->second->IsReady()?"is":"is not");

      //calculate the amount of items
      totalItemCount += it->second->GetTotalItemsCount();
      totalActiveSources++;
    }

    it++;
  }

  if (bReady)
  {
    CLog::Log(LOGDEBUG,"CBrowseWindowSourceController::ShowReadySources, %d source(s) active ready, going to show items.. (merge)",totalActiveSources);
    m_iTotalItemsCount = totalItemCount;

    mergeResult = MergeSources();

    if (mergeResult != WAITING_FOR_ITEMS) //means we should wait for a bind to one of the sources
    {
      m_pView->ShowItems(m_vecCurrentViewItems,false);
    }
  }
}

void CBrowseWindowSourceController::UpdateItem(const CFileItem* pItem)
{
  CStdString itemId = pItem->GetProperty("itemid");
  CFileItemPtr pCurrentItem = m_mapViewItemsIndexByItemId[itemId];
  if (pCurrentItem)
  {
    // If the new item does not have a thumbnail, preserve the one we have now
    CStdString strPreviousIcon;
    strPreviousIcon = pCurrentItem->GetIconImage();

    // Copy the entire item
    *pCurrentItem = *pItem;

    // If the new item does not have an icon, restore the old one, unless it is a loading item
    if (!pCurrentItem->HasIcon() && !pCurrentItem->GetPropertyBOOL("IsLoading"))
    {
      pCurrentItem->SetIconImage(strPreviousIcon);
    }
  }

  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    it->second->UpdateItem(pItem);
    it++;
  }
}

SourceItemState CBrowseWindowSourceController::GetNextPage()
{
  SourceItemState mergeResult=NO_MORE_ITEMS;

  if (m_iPageSize != -1)
  {
    mergeResult = MergeSources();

    if (mergeResult != WAITING_FOR_ITEMS) //means we should wait for a bind to one of the sources
    {
      m_pView->ShowItems(m_vecCurrentViewItems,false);
    }
  }

  return mergeResult;
}


SourceItemState CBrowseWindowSourceController::MergeSources()
{
  m_vecCurrentPageItems.Clear();

  SourcesMap::iterator it;
  int iActiveCount=0;
  CBrowseWindowSource* lastActiveSource = NULL;

  if (m_iPageSize == DISABLE_PAGING)
  {
    //clear any leftovers from previous merge
    m_vecCurrentPageItems.Clear();
    m_vecCurrentViewItems.Clear();
  }

  for (it = m_sources.begin(); it != m_sources.end() ; it++)
  {
    if (it->second->IsActive())
    {
      iActiveCount++;
      lastActiveSource = it->second;
    }
  }

  if (iActiveCount < 1)
    return NO_MORE_ITEMS;

  // In case there is only one source, append it to the current page and view
  if (iActiveCount == 1)
  {
    if (lastActiveSource)
    {
      CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, processing single source id=[%s] (browse)(merge)",lastActiveSource->GetSourceId().c_str());
    }
    else
    {
      //should not happen - sanity
      CLog::Log(LOGERROR, "CBrowseWindowSourceController::MergeSources, found one active source but lastActiveSource is NULL (browse) (merge)");
      return NO_MORE_ITEMS;
    }

    int fillItemsCount = ((m_iPageSize==DISABLE_PAGING)?lastActiveSource->GetItemCount():m_iPageSize);

    // If source is not active return - sanity
    if (!lastActiveSource->IsActive())
    {
      CLog::Log(LOGERROR, "CBrowseWindowSourceController::MergeSources, source id=[%s] became inactive on merge attempt.(browse) (merge)", lastActiveSource->GetSourceId().c_str());
      return NO_MORE_ITEMS;
    }

    while (m_vecCurrentPageItems.Size() < fillItemsCount)
    {
      CFileItemPtr currentItem;
      SourceItemState sourceItemState = lastActiveSource->PopItem(currentItem);

      if (sourceItemState != GOT_ITEMS)
      {
        return sourceItemState;
      }

      // We assume source does not have duplicate items in within itself
      m_vecCurrentPageItems.Add(currentItem);
      m_vecCurrentViewItems.Add(currentItem);
      m_mapViewItemsIndexByItemId[currentItem->GetProperty("itemid")] = currentItem;
    }

    return GOT_ITEMS;
  }

  // The more complicated case of multiple sources that should be merged

  // Add all sources to the vector in order to only go over those that have items
  std::vector< CBrowseWindowSource*> vecSources;
  for (SourcesMap::iterator it = m_sources.begin() ; it != m_sources.end() ; it++)
  { 
    if (it->second->IsActive())
    {
      vecSources.push_back(it->second);
    }
  }
  
  CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, processing %d sources (browse) (merge)",(int)vecSources.size());
  while (!vecSources.empty() && (m_vecCurrentPageItems.Size() < m_iPageSize || m_iPageSize == -1))
  {
    int minIndex = -1;
    CFileItemPtr  minItem;

    //determine the item the should enter the vector by the sort method
    for (int i = 0; i < (int)vecSources.size(); i++)
    {
      CFileItemPtr  currentItem;
      SourceItemState sourceItemState = vecSources[i]->PeekItem(currentItem);

      if (sourceItemState == NO_MORE_ITEMS)
      {
        CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, source id= %s, path = %s is depleted (browse) (merge)", vecSources[i]->GetSourceId().c_str(), vecSources[i]->GetCurrentPath().c_str());
        vecSources.erase(vecSources.begin() + i);
        i--;
        continue;
      }
      else if (sourceItemState == WAITING_FOR_ITEMS)
      {
        CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, returning, source id= %d, path = %s requested more items from next page (browse) (merge)", i, vecSources[i]->GetCurrentPath().c_str());
        // Assumption: currently we don't have more than one source that should be paged
        return WAITING_FOR_ITEMS;  //need to go back to bind the items to the source
      }
      else
      {
        if (minIndex < 0)
        {
          minIndex = i;
          minItem = currentItem;
        }
        else
        {
          // Check whether current item is the minimal
          bool leftIsLower=true;

          /*
           *  these are the sorting function that the merging supports
           *  when we sort by title (ATOZ) we compare the items as usual and the local and remote should appear sorted by A to Z.
           *
           *  when we're sorting by popularity or release date, we can not rely on the local items since the data there might be outdated.
           *  so we're checking if the currentItem hold a remote item we insert that into the output map first.
           *  which means that all the local items that are not in the remote library will appear at the bottom of the list.
           *
           *  local items that can be found in the library should not be treated anyhow since the remote item already includes the local items links
           *  these local items should be popped out of the source and ignored.
           *
           */

          if (GetSortMethod().m_id == VIEW_SORT_METHOD_ATOZ)
          { //incase its sorting by title, we compare both items as usual
            leftIsLower = SSortFileItem::LabelAscendingExactNoCase(minItem,currentItem);
          }
          if (GetSortMethod().m_id == VIEW_SORT_METHOD_ZTOA)
          { //incase its sorting by title, we compare both items as usual
            leftIsLower = SSortFileItem::LabelDescendingExactNoCase(minItem,currentItem);
          }
          else if (GetSortMethod().m_id == VIEW_SORT_METHOD_RELEASE || GetSortMethod().m_id == VIEW_SORT_METHOD_RELEASE_REVERSE)
          { // on the local item there is no releasedate and popularity, so we take the remote item instead
            if (currentItem->GetPropertyBOOL("IsBoxeeServerItem"))
            {
              CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, detected remote item in : %s (merge)" , vecSources[i]->GetSourceId().c_str());
              leftIsLower = false;
            }
          }
          else if (GetSortMethod().m_id == VIEW_SORT_METHOD_POPULARITY)
          {
            if (currentItem->GetPropertyBOOL("IsBoxeeServerItem"))
            {
              CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, detected remote item in : %s (merge)" , vecSources[i]->GetSourceId().c_str());
              leftIsLower = false;
            }
          }
          else if (GetSortMethod().m_id == VIEW_SORT_METHOD_NEWEST_FIRST)
          {
            leftIsLower = SSortFileItem::EpisodesDateDescending(minItem,currentItem);
          }
          else if (GetSortMethod().m_id == VIEW_SORT_METHOD_NEWEST_LAST)
          {
            leftIsLower = SSortFileItem::EpisodesDateAscending(minItem,currentItem);
          }

          if (!leftIsLower)
          {
              // false if firstItem > secondItem
              minItem = currentItem;
              minIndex = i;
          }
        }
      }
    }

    if (minIndex < 0)
    {
      // All sources were depleted
      return NO_MORE_ITEMS;
    }

    // Now that we have the minItem, determine whether we can add it to the page
    CFileItemPtr nextItem;
    vecSources[minIndex]->PopItem(nextItem);

    CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, popping from source : %s (merge)" , vecSources[minIndex]->GetSourceId().c_str());

    CStdString strBoxeeId = nextItem->GetProperty("boxeeid");
    bool canAddItem = true;

    if (!strBoxeeId.IsEmpty())
    {
      // Lookup whether an item with the same BoxeeId already exists in the view
      FileItemIndex::iterator it = m_mapViewItemsIndexByBoxeeId.find(strBoxeeId);

      if (it != m_mapViewItemsIndexByBoxeeId.end())
      {
        // item with the same BoxeeId already exists in the view -> DON'T add item
        canAddItem = false;

        //make sure the item there is currently in the map is the remote item
        CFileItemPtr existingItem = it->second;

        CFileItemPtr importantItem = existingItem;
        CFileItemPtr otherItem;

        //determine the base item
        if (existingItem)
        {
          bool bCurrentItemIsRemote = existingItem->GetPropertyBOOL("IsBoxeeServerItem");
          bool bNextItemIsRemote = nextItem->GetPropertyBOOL("IsBoxeeServerItem");

          CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, conflict found (merge)");

          if (existingItem->GetPropertyBOOL("hasNFO"))
          {
            CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, using existing item as the important one since it has an NFO (new)(merge)");
            importantItem = existingItem;
            otherItem = nextItem;
          }
          else if (nextItem->GetPropertyBOOL("hasNFO"))
          {
            CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, using the new item as the important one since it has an NFO (new)(merge)");
            importantItem = nextItem;
            otherItem = existingItem;
          }
          else if (!bCurrentItemIsRemote && bNextItemIsRemote)
          {
            CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, using the existing item as the important one since it was sent by the server (new)(merge)");
            importantItem = nextItem;
            otherItem = existingItem;
          }
          else
          {
            CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, using the new item as the important one since it was sent by the server (new)(merge)");
            importantItem = existingItem;
            otherItem = nextItem;
          }
        }

        if (otherItem.get() != NULL)
        {
          //merge links of the one to the other

          const CFileItemList* otherLinks = otherItem->GetLinksList();
          if (otherLinks)
          {
            for (int i = 0 ; i < otherLinks->Size() ; i++)
            {
              importantItem->AddLink(otherLinks->Get(i));
            }
          }

          bool bHasBoxeeDBVideoId = otherItem->HasProperty("boxeeDBvideoId");

          *otherItem = *importantItem;

          *existingItem = *otherItem;

          existingItem->SetProperty("IsMergedItem",true);

          if (bHasBoxeeDBVideoId)
          {
            //needed to allow manual resolve "remove" option. TODO: Implement property merging
            existingItem->SetProperty("boxeeDBvideoId",otherItem->GetPropertyBOOL("boxeeDBvideoId"));
          }
        }
      }
    }
    else
    {
      if (nextItem->GetProperty("showid").IsEmpty())
      {
        // no BoxeeId or ShowId -> DON'T add item
        canAddItem = false;
      }
    }

    if (canAddItem)
    {
      CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources - adding item to page [label=%s][path=%s][strBoxeeId=%s] (browse)(merge)", nextItem->GetLabel().c_str(), nextItem->m_strPath.c_str(),strBoxeeId.c_str());
      // Item does not exist in the view, add it to the current page
      m_vecCurrentPageItems.Add(nextItem);
      m_vecCurrentViewItems.Add(nextItem);

      m_mapViewItemsIndexByItemId[nextItem->GetProperty("itemid")] = nextItem;

      // Also add it to the view index
      m_mapViewItemsIndexByBoxeeId[strBoxeeId] = nextItem;
    }
  }

  if (vecSources.empty())
  {
    CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, returning, all sources depleted (browse) (merge)");
    return NO_MORE_ITEMS;
  }
  else
  {
    CLog::Log(LOGDEBUG, "CBrowseWindowSourceController::MergeSources, returning, page full (browse) (merge)");
    return GOT_ITEMS;
  }
}

void CBrowseWindowSourceController::SetFilter(const CStdString& strName, const CStdString strValue)
{
  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    it->second->SetFilter(strName, strValue);
    it++;
  }
}

void CBrowseWindowSourceController::ClearFilter(const CStdString& strName)
{
  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    it->second->ClearFilter(strName);
    it++;
  }
}

void CBrowseWindowSourceController::SetSortMethod(const CBoxeeSort& bxSort)
{
  m_currentSort = bxSort;

  SourcesMap::iterator it = m_sources.begin();
  while (it != m_sources.end())
  {
    it->second->SetSortMethod(bxSort);
    it++;
  }
}


///////////////////////////////////////////////////////////////////////
// IMPLEMENTATION OF THE CBrowseWindowState ///////////////////////////

CBrowseWindowState::CBrowseWindowState(CGUIWindowBoxeeBrowse* pWindow) : m_sourceController(pWindow)
{
  m_iSearchDepth = 0;
  m_pWindow = pWindow;

  m_iSelectedItem = -1;

  m_browseWindowAllowFilter = new CBrowseWindowAllowFilter(IS_ALLOW_FILTER,"Item Allow Filter");

  m_bIsInitialized = false;

  m_storedSort = NULL;
}

CBrowseWindowState::~CBrowseWindowState()
{
  m_pWindow = NULL;
}

bool CBrowseWindowState::OnClick(CFileItem& item)
{
  return false;
}

void CBrowseWindowState::OnItemLoaded(const CFileItem* pItem)
{
  m_sourceController.UpdateItem(pItem);
}

void CBrowseWindowState::ResetDefaults()
{
  SetDefaultCategory();
  SetDefaultSort();
  SetDefaultView();
}

// TODO: Replace with correct constant
void CBrowseWindowState::SetDefaultView()
{
  m_iCurrentView = 0;
}

void CBrowseWindowState::SetDefaultSort()
{
  if (m_storedSort != NULL)
  {
    SetSort(*m_storedSort); //if there's a loaded sort from the db, set it
  }
  else if (!m_vecSortMethods.empty())
  {
    SetSort(m_vecSortMethods[0]); //try to set the first sort (default)
  }
  else
  {
    SetSort(CBoxeeSort(VIEW_SORT_METHOD_NONE,SORT_METHOD_NONE,SORT_ORDER_NONE,"","")); //there is not sort
  }
}

void CBrowseWindowState::SetDefaultCategory()
{
  SetCategory("no-category"); // load the sort and view for windows without category
}

void CBrowseWindowState::InitState()
{
  if (!m_bIsInitialized)
  {
    ResetDefaults();
    m_bIsInitialized = true;
  }
}

void CBrowseWindowState::SetCategory(const CStdString& strCategory)
{
  m_strSelectedCategory = strCategory;

  LoadState();
}

void CBrowseWindowState::SaveState()
{
  CGUIWindowStateDatabase statedb;

  statedb.SaveState(m_pWindow->GetID(),  m_strSelectedCategory, GetCurrentView() , m_sort.m_id , m_iSelectedItem );
}

bool CBrowseWindowState::LoadState()
{
  CGUIWindowStateDatabase statedb;
  int view;
  std::string SortId;
  int selectedItem;
  bool bRetVal = true;
  bRetVal = statedb.LoadState(m_pWindow->GetID(), m_strSelectedCategory,view,SortId,selectedItem);

  m_storedSort = NULL; //clear the stored sort, we don't need to delete the object because its passed by reference and held in m_vecSortMethods

  if (bRetVal)
  {
    m_iCurrentView = view; //the view will be applied by the GUIWindowBoxeeBrowse in ShowItems

    if (!SortId.empty())
    {
      CBoxeeSort* sort = GetSortById(SortId);

      if (sort != NULL)
      {
        m_storedSort = sort;
      }
    }
  }
  return bRetVal;
}

void CBrowseWindowState::Refresh(bool bResetSeleted)
{
  //m_pWindow->ClearFileItems();
  if (m_bIsInitialized)
    m_sourceController.Refresh();
}

void CBrowseWindowState::OnBind(CFileItemList& items)
{
  m_sourceController.BindItems(items);
}

void CBrowseWindowState::SetSelectedItem(int iSelectedItem)
{
  m_iSelectedItem = iSelectedItem;
}

int CBrowseWindowState::GetSelectedItem()
{
  if (m_iSelectedItem > -1)
  {
    return m_iSelectedItem;
  }
  else
  {
   return 0;
  }
}

void CBrowseWindowState::SetLabel(const CStdString& strLabel)
{
  m_strLabel = strLabel;
}

CStdString CBrowseWindowState::GetLabel()
{
  return m_strLabel;
}

bool CBrowseWindowState::OnSort()
{
  CFileItemList sorts;

  CStdString sortID = this->m_sort.m_id;

  for (size_t i = 0; i < m_vecSortMethods.size(); i++)
  {
    CFileItemPtr sortItem (new CFileItem(m_vecSortMethods[i].m_sortName));
    sortItem->SetProperty("value",(int) i);
    sorts.Add(sortItem);
    if (sortID == m_vecSortMethods[i].m_id)
       sorts.SetProperty("selectedSort",(int) i);
  }

  CStdString value;

  CGUIToggleButtonControl* pTButton = (CGUIToggleButtonControl*) m_pWindow->GetControl(SORT_BUTTON);
  if (pTButton)
  {
    CPoint buttonPos = pTButton->GetRenderPosition();
    pTButton->SetSelected(false);

    if (CGUIDialogBoxeeSortDropdown::Show(sorts,value,buttonPos.x,buttonPos.y))
    {
      SetSort(m_vecSortMethods[atoi(value)]);
      return true;
    }
  }

  return false;
}

void CBrowseWindowState::SetSort(const CBoxeeSort& sort)
{
  m_sort = sort;
  m_pWindow->SetProperty("sort-label", m_sort.m_sortName);

  m_sourceController.SetSortMethod(sort);
}

CBoxeeSort* CBrowseWindowState::GetSortById(const CStdString& id)
{
  std::vector<CBoxeeSort>::iterator it;

  for (it = m_vecSortMethods.begin() ; it != m_vecSortMethods.end() ; it++)
  {
    if (it->m_id == id)
    {
      return &(*it);
    }
  }
  return NULL;
}

/*
void CBrowseWindowState::SortItems(CFileItemList &items)
{
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
}
*/

// SEARCH ///////////////////////////////////////////

/*
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
  SaveWindowStateToDB();

  m_strSearchString = "";

  if (CGUIDialogBoxeeSearch::ShowAndGetInput(m_strSearchString, m_strSearchType, "Search", 929, 39, false, false))
  {
    CStdString searchLabel;
    searchLabel.Format("SEARCH RESULTS FOR: %s", m_strSearchString.c_str());
    m_pWindow->SetProperty("search-label", searchLabel);
    m_pWindow->SetProperty("searching", false);
    m_pWindow->SetProperty("search-set", true);

    // Open the search result dialog with search term
    CGUIDialogBoxeeSearchResults::Show(m_strSearchString);

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
    RestoreWindowStateFromDB();

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

*/

// SEARCH END ///////////////////////////////

//void CBrowseWindowState::ProcessItems(const CFileItemList& vecModelItems, CFileItemList& vecViewItems)
//{
//  vecViewItems.Clear();
//
//  if (vecModelItems.Size() == 0)
//    return;
//
//  for(int i = 0; i < vecModelItems.Size(); i++)
//  {
//    CFileItemPtr pItem = vecModelItems.Get(i);
//    bool bFilter = ApplyLocalFilter(pItem.get());
//    pItem->SetProperty("filter", bFilter);
//  }
//
//
//  // We copy all items first in order to preserve the original properties
//  // set by directory
//  vecViewItems.Copy(vecModelItems);
//
//  // Copy all items that have true filter property
//  for (int i = 0; i < vecViewItems.Size(); i++)
//  {
//    CFileItemPtr pItem = vecViewItems.Get(i);
//
//    if (!pItem->GetPropertyBOOL("filter"))
//    {
//      vecViewItems.Remove(i);
//      i--;
//    }
//  }
//
//  SortItems(vecViewItems);
//
//  // We need to set the "browsemode" property because it is used by the view state factory
//  // within the browse screen
//  vecViewItems.SetProperty("browsemode", m_strType);
//}

void CBrowseWindowState::OnPathChanged(CStdString strPath, bool bResetSelected)
{
}

void CBrowseWindowState::SetPathToDisplay(const CStdString& strPath)
{
  // dconti - the fiona UI does not use the 'displaypath' variable at all, but this is kept
  // consistent with CLocalBrowseWindowState::UpdateProperties()
  CStdString pathToDisplay = "";
  CStdString path = _P(strPath);

  if (CUtil::IsHD(path) || CUtil::IsSmb(path) || CUtil::IsUPnP(path))
  {
    // remove password
    CUtil::RemovePasswordFromPath(path);

    if( CUtil::IsUPnP(path) )
      CUPnPDirectory::GetFriendlyPath(path);
    // Shorten the path
    pathToDisplay = path;
    CUtil::MakeShortenPath(path,pathToDisplay, MAX_DISPLAY_PATH_LENGTH);
  }
  else
  {
    pathToDisplay = "";
  }

  m_pWindow->SetProperty("displaypath", pathToDisplay);
}

bool CBrowseWindowState::OnBack()
{
  CLog::Log(LOGDEBUG, "CBrowseWindowState::OnBack, default implementation, open main menu (browse)");

  CGUIDialogBoxeeBrowseMenu* pMenu = (CGUIDialogBoxeeBrowseMenu*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_BROWSE_MENU);
  pMenu->DoModal();
  return true;
}



void CBrowseWindowState::SetFilter(const CStdString& strName, const CStdString strValue)
{
  m_sourceController.SetFilter(strName, strValue);
}

void CBrowseWindowState::ClearFilter(const CStdString& strName)
{
  m_sourceController.ClearFilter(strName);
}

void CBrowseWindowState::SetCurrentView(int iCurrentView)
{
  m_iCurrentView = iCurrentView;
}

int CBrowseWindowState::GetCurrentView()
{
  return m_iCurrentView;
}

void CBrowseWindowState::SetPageSize(int iPageSize)
{
  m_sourceController.SetPageSize(iPageSize);
}

SourceItemState CBrowseWindowState::GetNextPage()
{
  return m_sourceController.GetNextPage();
}

////////////////////////////////////////////////////////////////////////////////////

void CBrowseWindowStateHistory::PushState(int id, CBrowseStateHistoryItem* item)
{
  CSingleLock lock(m_historyLock);

  std::map<int, std::stack<CBrowseStateHistoryItem*> >::iterator it = m_history.find(id);

  if (it != m_history.end())
  {
    item->m_level = m_history[id].size();
    (*it).second.push(item);
  }
  else
  {
    std::stack<CBrowseStateHistoryItem*> newStack;
    newStack.push(item);
    item->m_level = 0;
    m_history[id] = newStack;
  }
}

CBrowseStateHistoryItem* CBrowseWindowStateHistory::PopState(int id)
{
  CSingleLock lock(m_historyLock);

  CBrowseStateHistoryItem* state = NULL;

  std::map<int, std::stack<CBrowseStateHistoryItem*> >::iterator it = m_history.find(id);
  if (it != m_history.end())
  {
    if (!CBrowseWindowStateHistory::IsEmpty(id))
    {
      state = (*it).second.top();
      (*it).second.pop();
    }
    else
    {
      CLog::Log(LOGWARNING,"CBrowseWindowStateHistory::PopState - for [id=%d] history stack is EMPTY. [size=%d] (history)",id,CBrowseWindowStateHistory::Size(id));
    }
  }

  return state;
}

bool CBrowseWindowStateHistory::IsEmpty(int id)
{
  CSingleLock lock(m_historyLock);

  std::map<int, std::stack<CBrowseStateHistoryItem*> >::iterator it = m_history.find(id);
  if (it != m_history.end())
  {
    CLog::Log(LOGDEBUG,"CBrowseWindowStateHistory::IsEmpty - history for [id=%d] is %s (history)",id,(*it).second.empty() ? "EMPTY" : "NOT EMPTY");
    return (*it).second.empty();
  }
  return true;
}

int CBrowseWindowStateHistory::Size(int id)
{
  CSingleLock lock(m_historyLock);

  int size = -1;

  std::map<int, std::stack<CBrowseStateHistoryItem*> >::iterator it = m_history.find(id);
  if (it != m_history.end())
  {
    size = (int)((*it).second.size());
    CLog::Log(LOGDEBUG,"CBrowseWindowStateHistory::Size - history for [id=%d] is [size=%d] (history)",id,size);
  }

  return size;
}

void CBrowseWindowStateHistory::ResetHistory(int id)
{
  CSingleLock lock(m_historyLock);

  std::map<int, std::stack<CBrowseStateHistoryItem*> >::iterator it = m_history.find(id);
  if (it != m_history.end())
  {
    while (!(*it).second.empty())
    {
      CBrowseStateHistoryItem* item = (*it).second.top();
      delete item;
      (*it).second.pop();
    }
  }
}

CBrowseWindowStateWithHistory::CBrowseWindowStateWithHistory(CGUIWindowBoxeeBrowse *pWindow) : CBrowseWindowState(pWindow)
{
}

CBrowseWindowStateWithHistory::~CBrowseWindowStateWithHistory()
{
}

bool CBrowseWindowStateWithHistory::OnClick(CFileItem& item)
{
  // Default implementation, in case of a folder item drill down into it
  if ((item.m_bIsFolder) && (!item.IsPlayList()))
  {
    UpdateHistory();

    CLog::Log(LOGDEBUG, "CBrowseWindowState::OnClick, drill down into folder, path = %s (browse)", item.m_strPath.c_str());
    OnPathChanged(item.m_strPath, true);
    
    return true;
  }

  return false;
}

bool CBrowseWindowStateWithHistory::OnBack()
{
  CLog::Log(LOGDEBUG,"CBrowseWindowStateWithHistory::OnBack - enter (back)");

  if (!CBrowseWindowStateHistory::IsEmpty(m_pWindow->GetID()))
  {
    //just before restoring the state , save the current 
    //m_windowState->SaveWindowStateToDB();
    
    //pop the previous state and continue updating the path
    CBrowseStateHistoryItem* historyItem = CBrowseWindowStateHistory::PopState(m_pWindow->GetID());

    FromHistory(historyItem);

    if (historyItem)
    {
      delete historyItem;
    }

    Refresh(false);
    
    m_pWindow->SetSelectedItem(GetSelectedItem());

    m_pWindow->SetWindowTitle(GetLabel());

    return true;
  }

  return false;
}

void CBrowseWindowStateWithHistory::ResetHistory()
{
  CBrowseWindowStateHistory::ResetHistory(m_pWindow->GetID());
}


void CBrowseWindowStateWithHistory::UpdateHistory()
{  
  SetSelectedItem(m_pWindow->GetSelectedItem());

  CBrowseStateHistoryItem* historyItem = ToHistory();

  if (historyItem)
  {
    CBrowseWindowStateHistory::PushState(m_pWindow->GetID(), historyItem);
  }
}

CBrowseStateHistoryItem* CBrowseWindowStateWithHistory::ToHistory()
{
  CBrowseStateHistoryItem* historyItem = new CBrowseStateHistoryItem();

  // Save all sources
  SourcesMap sources = m_sourceController.GetSources();
  for (SourcesMap::iterator it = sources.begin(); it != sources.end(); it++)
  {
    CBrowseStateHistorySource source;

    source.m_strBasePath = it->second->GetBasePath();
    source.m_strSourceId = it->second->GetSourceId();

    historyItem->m_vecSources.push_back(source);
  }

  historyItem->m_iSelectedItem = m_iSelectedItem;

  return historyItem;
}

void CBrowseWindowStateWithHistory::FromHistory(CBrowseStateHistoryItem* historyItem)
{
  if (!historyItem) return;

  m_sourceController.RemoveAllSources();
  
  for (int i = 0; i < (int)historyItem->m_vecSources.size(); i++)
  {
    CStdString strSourceId = historyItem->m_vecSources[i].m_strSourceId;
    CStdString strBasePath = historyItem->m_vecSources[i].m_strBasePath;

    m_sourceController.AddSource(new CBrowseWindowSource(strSourceId, strBasePath, m_pWindow->GetID()));
  }

  m_iSelectedItem = historyItem->m_iSelectedItem;
}

void CBrowseWindowStateWithHistory::OnPathChanged(CStdString strPath, bool bResetSelected)
{
  SetPathToDisplay(strPath);

  if (bResetSelected)
      m_iSelectedItem = 0;

  Refresh();
}
