
#include "BoxeeItemsHistory.h"
#include "Util.h"
#include "bxutils.h"
#include "MusicInfoTag.h"
//#include "XTimeUtils.h"
#include "VideoInfoTag.h"
#include "SpecialProtocol.h"
#include "utils/log.h"
#include "GUIUserMessages.h"
#include "LocalizeStrings.h"
#include "utils/SingleLock.h"

using namespace std;

#define BOXEE_FILES_HISTORY_SIZE 500
#define ITEMS_HISTORY_FILE_PATH "special://profile/playbackhistory.dmp"

using namespace BOXEE;

CBoxeeItemsHistory::CBoxeeItemsHistory()
{
  m_historyReadyEvent = CreateEvent(NULL, TRUE, FALSE, "");
}

CBoxeeItemsHistory::~CBoxeeItemsHistory()
{
  CloseHandle(m_historyReadyEvent);
}

bool CBoxeeItemsHistory::WaitForHistoryReady(DWORD dwTimeout)
{
  return WaitForSingleObject(m_historyReadyEvent, dwTimeout) == WAIT_OBJECT_0;
}

bool CBoxeeItemsHistory::AddItemToHistory(const CFileItem& item,CStdString historyId)
{
  if (item.IsAdult())
  {
    return false;
  }

  CSingleLock lock(m_itemsHistorylock);
  
  CFileItem *ptr = CreateItemForHistoryList(item);  
  if(ptr == NULL)
  {
    CLog::Log(LOGERROR,"Failed to enter item [%s] to ItemsHistoryList (bh)",(item.m_strPath).c_str());
    return false;
  }

  CFileItemPtr historyItem (ptr);
  
  if(historyId.Equals("") == true)
  {
    historyId = _P(historyItem->m_strPath);
  }

  historyItem->SetProperty("historyId",historyId);
  
  time_t tmNow = time(NULL);
  historyItem->SetProperty("AddedToHistoryListTime",(int)tmNow);
  
  CLog::Log(LOGDEBUG,"Wants to add item [%s] with [historyId=%s][timestamp=%d] to the ItemsHistoryList (bh)",(item.m_strPath).c_str(),historyId.c_str(),(int)tmNow);
  
  std::set<CStdString>::iterator it;
  it=m_historyIdsOfItemsInHistoryList.find(historyId);
  
  if(it == m_historyIdsOfItemsInHistoryList.end())
  {
    // Item was is not in the itemsHistoryList -> Need to add it
    
    CLog::Log(LOGDEBUG,"historyId [%s] was NOT found in the SET (bh)",historyId.c_str());
    
    // Need to check if the FilesHistory id full.
    // If so remove the old item before adding new item
    if(m_itemsHistoryList.Size() == BOXEE_FILES_HISTORY_SIZE)
    {
      CLog::Log(LOGDEBUG,"ItemsHistoryList size [%d] is equal to BOXEE_FILES_HISTORY_SIZE [%d]. Going to remove oldest item - (bh)",m_itemsHistoryList.Size(),BOXEE_FILES_HISTORY_SIZE);
      
      // Remove the oldest item in the FilesHistory list
      CFileItemPtr itemToRemove = m_itemsHistoryList.Get(BOXEE_FILES_HISTORY_SIZE-1);
      m_historyIdsOfItemsInHistoryList.erase(itemToRemove->GetProperty("historyId"));
      m_itemsHistoryList.Remove(BOXEE_FILES_HISTORY_SIZE-1);
    }
    
    m_itemsHistoryList.AddFront(historyItem,0);
    m_historyIdsOfItemsInHistoryList.insert(historyId);
    
    CLog::Log(LOGDEBUG,"Item [%s] with [historyId=%s] was added to the ItemsHistoryList. ItemsHistoryList size is [%d] - (bh)",(item.m_strPath).c_str(),historyId.c_str(),m_itemsHistoryList.Size());
  }
  else
  {
    // Item is already in the map -> Need to put it at the front
    
    CLog::Log(LOGDEBUG,"HistoryId [%s] was found in the SET (bh)",historyId.c_str());
    
    bool itemWasMoved = false;
    
    CFileItemPtr item = m_itemsHistoryList.Get(0);
    CStdString itemHistoryId = item->GetProperty("historyId");
    
    if(itemHistoryId.Equals(historyId))
    {
      CLog::Log(LOGDEBUG,"Item with [historyId=%s] is already at the front of the ItemsHistoryList. ItemsHistoryList size is [%d] (bh)",historyId.c_str(),m_itemsHistoryList.Size());
      
      if(((item->GetThumbnailImage() == "defaultloading.gif") || (item->GetThumbnailImage() == "")) && (historyItem->GetThumbnailImage() != "defaultloading.gif"))
      {
        CLog::Log(LOGDEBUG,"Because the thumb of the history item is [%s], it will be update to the new item one [%s] (bh)",(item->GetThumbnailImage()).c_str(),(historyItem->GetThumbnailImage()).c_str());
        item->SetThumbnailImage(historyItem->GetThumbnailImage());
      }
    }
    else
    {
      for(int i=1;i<m_itemsHistoryList.Size();i++)
      {
        item = m_itemsHistoryList.Get(i);
        itemHistoryId = item->GetProperty("historyId");
        
        if(itemHistoryId.Equals(historyId))
        {
          // Remove from ItemsHistoryList
          m_itemsHistoryList.Remove(i);
          
          // Insert the duplicate at the front of the ItemsHistoryList
          m_itemsHistoryList.AddFront(historyItem,0);
          
          itemWasMoved = true;
          
          CLog::Log(LOGDEBUG,"Item with [historyId=%s] was moved from pos [%d] to the front of the ItemsHistoryList. ItemsHistoryList size is [%d] (bh)",historyId.c_str(),i,m_itemsHistoryList.Size());
          
          break;
        }
      }
      
      if(itemWasMoved == false)
      {
        // item was not found in the ItemsHistoryList
        // although itd HistoryId is in the SET -> need to add the item to the front of the list
        
        CLog::Log(LOGERROR,"Although the [historyId=%s] was found in the SET, item for that id WASN'T found in the ItemsHistoryList. Going to insert the item to the front of the ItemsHistoryList (bh)",historyId.c_str());
        
        // Insert the duplicate at the front of the ItemsHistoryList
        m_itemsHistoryList.AddFront(historyItem,0);
      }
    }
  }
  
  // Save the FilesHistory to file
  SaveItemsHistory();
  
  return true;
}

CFileItem* CBoxeeItemsHistory::CreateItemForHistoryList(const CFileItem& item)
{
  CFileItem* historyItem = NULL;
  
  if(item.IsPicture() || item.GetPropertyBOOL("IsPicture"))
  {
    // Case of picture

    if (item.IsInternetStream())
    {
      historyItem = new CFileItem(item);
    }
    else
    {
      historyItem = new CFileItem(BXUtils::GetParentPath(item.m_strPath), true);
      historyItem->SetProperty("isPictureFolder",true);
      historyItem->SetLabel(BXUtils::GetFolderName(historyItem->m_strPath));
    }

    historyItem->SetProperty("MediaType","picture");

    CLog::Log(LOGDEBUG,"A picture item for HistoryList was created (bh)");

  }
  else if(item.GetPropertyBOOL("isalbum"))
  {
    // Case of audio that was played from the library mode

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","music");

    CLog::Log(LOGDEBUG,"An audio item (album) for HistoryList was created (bh)");
  }
  else if(item.IsLastFM() || item.GetPropertyBOOL("IsLastFM"))
  {
    // Case of audio that was played from the library mode

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","music");
    if (item.HasProperty("station") && item.GetProperty("station") != "")
    {
      historyItem->SetLabel(item.GetProperty("station"));
    }

    CLog::Log(LOGDEBUG,"An audio item (lastFM) for HistoryList was created (bh)");
  }
  else if(item.IsShoutCast() || item.GetPropertyBOOL("IsShoutCast"))
  {
    // Case of audio that was played from the library mode

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","music");

    CLog::Log(LOGDEBUG,"An audio item (ShoutCast) for HistoryList was created (bh)");
  }
  else if(item.IsAudio() || item.GetPropertyBOOL("IsMusic"))
  {
    // Case of audio that was played from the folder mode

    historyItem = new CFileItem();

    historyItem->m_strPath = BXUtils::GetParentPath(item.m_strPath);
    if (!item.IsInternetStream())
    {
      historyItem->m_bIsFolder = true;
      historyItem->SetProperty("isMusicFolder",true);
      historyItem->SetLabel(BXUtils::GetFolderName(historyItem->m_strPath));
    }
    else
    {
      *historyItem = item;
      if (item.HasProperty("parentPath")) {
        historyItem->SetProperty("parentfolder", item.GetProperty("parentPath"));
      }
      historyItem->SetProperty("IsInternetStream", true);
      historyItem->SetProperty("IsMusic", true);
      if (item.IsRSS())
        historyItem->SetProperty("IsRss", true);
    }
    historyItem->SetProperty("MediaType","music");
    CLog::Log(LOGDEBUG,"An audio item for HistoryList was created (bh)");
  }
  else if(item.IsVideo() || item.GetPropertyBOOL("IsVideo"))
  {
    // Case of video

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","video");
    
    if (item.IsInternetStream())
    {
      // Save the parent path property for the purpose of retrieving the channel
      if (item.HasProperty("parentPath")) {
        historyItem->SetProperty("parentfolder", item.GetProperty("parentPath"));
      }
    }

    CLog::Log(LOGDEBUG,"A video item for HistoryList was created (bh)");
  }
  else if ((item.m_bIsFolder) && (item.GetPropertyBOOL("isDvdFolder")))
  {
    // Case of DVD folder

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","video");

    CLog::Log(LOGDEBUG,"A DVD folder item for HistoryList was created (bh)");
  }
  else if(item.IsPlugin() || item.GetPropertyBOOL("IsPlugin"))
  {
    // Case of Plugin Folder

    historyItem = new CFileItem(item);
    historyItem->SetProperty("MediaType","video");

    CLog::Log(LOGDEBUG,"An Plugin Folder (YouTube) for HistoryList was created (bh)");
  }
  else
  {
    CLog::Log(LOGERROR,"Failed to identify item [%s] type (bh)",(item.m_strPath).c_str());
  }

  if(historyItem != NULL)
  {
    historyItem->SetProperty("ishistory", true);

    historyItem->SetProperty("originWindowId",item.GetProperty("windowId"));

    if(historyItem->HasProperty("IsFeedItem"))
    {
      // This code is needed in order cause the HistoryItem to arrive to the right ThumbLoader
      // (and not to the BoxeeFeedItemsLoader) for successful loading.
      historyItem->SetProperty("IsFeedItem",false);      
    }
    
    // If there is a "provider_source" property -> set a "play_provider_label" proeprty in order to show "Play on [provider]" in CGUIDialogBoxeeMediaAction
    if((!historyItem->HasProperty("play_provider_label")) && (historyItem->HasProperty("provider_source")))
    {
      CStdString playProviderLabel = "Play on ";
      playProviderLabel += historyItem->GetProperty("provider_source");
      historyItem->SetProperty("play_provider_label",playProviderLabel);
      
      CLog::Log(LOGDEBUG,"Proeprty [play_provider_label=%s] was added to the HistoryItem since it contains [provider_source=%s] (bh)",(historyItem->GetProperty("play_provider_label")).c_str(),(historyItem->GetProperty("provider_source")).c_str());
    }
    else
    {
      CLog::Log(LOGWARNING,"Not creating a play_provider_label property because [play_provider_label=%s][provider_source=%s] (bh)",(historyItem->GetProperty("play_provider_label")).c_str(),(historyItem->GetProperty("provider_source")).c_str());
    }
  }
  
  return historyItem;
}

bool CBoxeeItemsHistory::RemoveItemFromHistory(const CFileItem& historyItem)
{
  CSingleLock lock(m_itemsHistorylock);

  CStdString historyIdToRemove = historyItem.GetProperty("historyId");

  CLog::Log(LOGDEBUG,"Wants to remove item with [historyId=%s] from the ItemsHistoryList (bh)",historyIdToRemove.c_str());

  std::set<CStdString>::iterator it;
  it=m_historyIdsOfItemsInHistoryList.find(historyIdToRemove);

  if(it == m_historyIdsOfItemsInHistoryList.end())
  {
    // Item with the requested historyId is not in the ItemsHistoryList set

    CLog::Log(LOGERROR,"HistoryId [%s] WASN'T found in the SET so it doesn't in the ItemsHistoryList and can't be removed (bh)",historyIdToRemove.c_str());
    return false;
  }
  else
  {
    // Item with the requested historyId is in the ItemsHistoryList -> Need to remove it

     CLog::Log(LOGDEBUG,"HistoryId [%s] was found in the SET (bh)",historyIdToRemove.c_str());

     bool itemWasRemoved = false;

     for(int i=0;i<m_itemsHistoryList.Size();i++)
     {
       CFileItemPtr item = m_itemsHistoryList.Get(i);
       CStdString itemHistoryId = item->GetProperty("historyId");

       if(itemHistoryId.Equals(historyIdToRemove))
       {
         // Remove from ItemsHistoryList
         m_itemsHistoryList.Remove(i);

         // Remove from the SET
         m_historyIdsOfItemsInHistoryList.erase(historyIdToRemove);

         itemWasRemoved = true;

         CLog::Log(LOGDEBUG,"Item with [historyId=%s] was removed from the ItemsHistoryList. ItemsHistoryList size is [%d] (bh)",historyIdToRemove.c_str(),m_itemsHistoryList.Size());

         // Save the FilesHistory to file
         SaveItemsHistory();
       }
     }

     if(itemWasRemoved == false)
     {
       // Need to remove the HistoryId from the SET
       m_historyIdsOfItemsInHistoryList.erase(historyIdToRemove);

       CLog::Log(LOGERROR,"Although HistoryId [%s] was found in the SET, item with that HistoryId doesn't appear in the ItemsHistoryList so it can't be removed. The HistoryId [%s] was removed from the SET (bh)",historyIdToRemove.c_str(),historyIdToRemove.c_str());

       return false;
     }

     return true;
  }
}

bool CBoxeeItemsHistory::SaveItemsHistory()
{
  //////////////////////////////////////////////////
  // No lock because the function is being called //
  // from addItemToHistory() which lock           //
  //////////////////////////////////////////////////

  bool retVal = false;

  retVal = m_itemsHistoryList.Save(_P(ITEMS_HISTORY_FILE_PATH));

  if(retVal == false)
  {
    CLog::Log(LOGERROR,"Failed to save the list of HistoryFile to file (bh)");
  }
  else
  {
    CLog::Log(LOGDEBUG,"ItemsHistoryList was saved to disk with size [%d] (bh)",m_itemsHistoryList.Size());
  }

  return retVal;
}

bool CBoxeeItemsHistory::LoadItemsHistory()
{
  m_itemsHistoryList.Clear();
  m_historyIdsOfItemsInHistoryList.clear();

  bool retVal = m_itemsHistoryList.Load(_P(ITEMS_HISTORY_FILE_PATH));

  if(retVal == false)
  {
    CLog::Log(LOGERROR,"Failed to load the ItemsHistoryList from file (bh)");

    m_itemsHistoryList.Clear();
    m_historyIdsOfItemsInHistoryList.clear();
  }
  else
  {
    for(int i=0; i<m_itemsHistoryList.Size(); i++)
    {
      CFileItemPtr historyItem = m_itemsHistoryList.Get(i);

      if(historyItem != NULL)
      {
        m_historyIdsOfItemsInHistoryList.insert(historyItem->GetProperty("historyId"));
      }
      else
      {
        CLog::Log(LOGERROR,"Failed to read item from the ItemsHistoryList during load. ItemsHistoryList will be reset (bh)");

        m_itemsHistoryList.Clear();
        m_historyIdsOfItemsInHistoryList.clear();

        retVal = false;

        break;
      }
    }
  }

  SetEvent(m_historyReadyEvent);

  CLog::Log(LOGDEBUG,"After loading of ItemsHistoryList from file [size=%d]. Event was sent (bh)", m_itemsHistoryList.Size());

  return retVal;
}

bool CBoxeeItemsHistory::GetFilesHistory(CFileItemList& itemsHistoryList)
{
  CSingleLock lock(m_itemsHistorylock);

  itemsHistoryList.Append(m_itemsHistoryList);

  return true;
}



