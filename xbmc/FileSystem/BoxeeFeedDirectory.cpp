
#include "BoxeeFeedDirectory.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxmessages.h"
#include "FileSystem/DirectoryCache.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "URL.h"
#include "bxcscmanager.h"
#include "LocalizeStrings.h"
#include "RssSourceManager.h"
#include "Util.h"

#define QUEUE_MAX_SIZE 250

#define BOXEE_SOURCE "boxee"
#define BOOKLET_SOURCE "booklet"
#define SUBSCRIPTION_SOURCE "subscription"

using namespace BOXEE;

#undef GetObject

namespace DIRECTORY
{

CBoxeeFeedDirectory::CBoxeeFeedDirectory()
{
  
}

CBoxeeFeedDirectory::~CBoxeeFeedDirectory()
{
  
}

bool CBoxeeFeedDirectory::GetDirectory(const CStdString& strPath,CFileItemList &items)
{
  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetDirectory - Enter function with [strPath=%s] (bf)",strPath.c_str());
	
	CStdString strParams;
	CStdString strDir;
	CStdString strFile;
	std::map<std::string, std::string> mapParams;
	
	// Parse boxeedb url
	if (!BoxeeUtils::ParseBoxeeDbUrl(strPath, strDir, strFile, mapParams))
	{
    CLog::Log(LOGERROR,"CBoxeeFeedDirectory::GetDirectory - FAILED to parse [path=%s] (bf)",strPath.c_str());
	  return false;
	}
	
  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetDirectory - [strPath=%s] was parsed to [dir=%s][file=%s] (bf)",strPath.c_str(),strDir.c_str(), strFile.c_str());
  
  bool retVal = false;
  
  if (strDir == "recommend" || strDir == "share")
  {
    retVal = HandleRequestWithLimit("recommend",items,mapParams);
  }
  else if (strDir == "queue")
  {
    retVal = HandleQueueRequest(items,mapParams);
  }
  else if (strDir == "featured")
  {
    retVal = HandleRequestWithLimit("featured",items,mapParams);
  }
  else if (strDir == "activity")
 	{
    CLog::Log(LOGERROR,"CBoxeeFeedDirectory::GetDirectory - [%s] is not being handle by this class (bf)",strPath.c_str());
    //retVal = HandleRequestWithLimit("activity",items,listItems,mapParams);
 	}
  else if (strDir == "user" || strDir == "user") 
  {
 	  //////////////////////////////////////////////////////////////////////////////
    // bUser is not supported via this class, use boxee user activity directory //
 	  //////////////////////////////////////////////////////////////////////////////
    CLog::Log(LOGERROR,"CBoxeeFeedDirectory::GetDirectory - [%s] is not being handle by this class (bf)",strPath.c_str());
    //retVal = HandleRequestWithLimit("user",items,listItems,mapParams);
 	}
 	else 
  {
    CLog::Log(LOGERROR,"CBoxeeFeedDirectory::GetDirectory - Unrecognized feed path [%s]. Exit function and return FALSE (bf)",strPath.c_str());
 	}
    
  CLog::Log( LOGDEBUG, "Going to return list with size [%d] for type [%s] (bf)",items.Size(), strDir.c_str() );
      
	return retVal;
}


bool CBoxeeFeedDirectory::HandleRequestWithLimit(const CStdString& typeToRetrieveForLog,CFileItemList& items,std::map<std::string, std::string>& mapParams, bool sortByDate)
{   
  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - Going to retrieve objects from the server (bf)",typeToRetrieveForLog.c_str());
  
  bool bResult = GetItemsFromServer(typeToRetrieveForLog, mapParams);
  
  if (bResult)
  {
    CFileItemList listItems;

    bool succeeded = ConvertServerResponse(typeToRetrieveForLog,listItems);
    
    if(succeeded)
    {
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - Call to HandleServerResponse returned [listItemsSize=%d] (bf)",typeToRetrieveForLog.c_str(),listItems.Size());
      
      if (sortByDate)
      {
        // Sort by date
        listItems.Sort(SORT_METHOD_DATE, SORT_ORDER_DESC);
      }
        
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - After sort the retrieve items. Going to copy for return (bf)",typeToRetrieveForLog.c_str());
        
      // Set the limit for item to retrieve
      int limit = BOXEE::BXUtils::StringToInt(mapParams["limit"]);
      if (limit <= 0)
      {
        limit = 10000;
      }

      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - After setting [limit=%d] (bf)",typeToRetrieveForLog.c_str(),limit);

      // Copy
      if(listItems.Size() <= limit)
      {
        CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - [NumOfItems=%d] is smaller or equal to [limit=%d], therefore going to copy all of the items (bf)",typeToRetrieveForLog.c_str(),listItems.Size(),limit);
        items = listItems;
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - [NumOfItems=%d] is more then [limit=%d], therefore going to copy only [%d] items (bf)",typeToRetrieveForLog.c_str(),listItems.Size(),limit,limit);
          
        for(int i=0; i<limit; i++)
        {
          items.Add(listItems.Get(i));
        }
      }
    }
    else
    {
      // Error log will be written in HandleServerResponse()
      return true;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleRequestWithLimit - [%s] - FAILED to retrieve objects from the server (bf)",typeToRetrieveForLog.c_str());
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeQueueDirectory::HandleRequestWithLimit - Exit function and returning list with size [%d] (bf)",items.Size());

  return true;
}

bool CBoxeeFeedDirectory::HandleQueueRequest(CFileItemList& items,std::map<std::string, std::string>& mapParams)
{
  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - Going to retrieve objects from the server (bq)");
  
  // reset ValidQueueSize before handling
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().SetValidQueueSize(0);

  bool bResult = GetItemsFromServer("queue", mapParams);
  
  if (bResult)
  {
    CFileItemList listItems;

    bool succeeded = ConvertServerResponse("queue",listItems);
    
    if(succeeded)
    {
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - Call to HandleServerResponse returned [listItemsSize=%d] (bq)",listItems.Size());
              
      int count = BXUtils::StringToInt(mapParams["count"]);
      CStdString lastItemId = mapParams["last"];
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - After setting [count=%d][lastItemId=%s] (bq)",count,lastItemId.c_str());
      
      if(count <= 0)
      {
        CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - [count=%d <= 0] as parameter, therefore going to return all of the queue (bq)",count);
        count = QUEUE_MAX_SIZE;
      }

      int i=0;
      
      if(!lastItemId.IsEmpty())
      {
        i = GetTheLastItemIndexById(lastItemId);
        
        if(i < 0)
        {
          CLog::Log(LOGDEBUG,"CBoxeeQueueDirectory::GetDirectory - Exit function and returning list with size [%d] (bq)",items.Size());

          // function will return TRUE because the skin will handle the 0 items

          return true;
        }
      }

      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - Going to set [ValidQueueSize=%d] (bq)",listItems.Size());
      BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().SetValidQueueSize(listItems.Size());

      CLog::Log(LOGDEBUG,"CBoxeeQueueDirectory::GetDirectory - Going to add queue items. [i=%d][count=%d][listItemsSize=%d] (bq)",i,count,listItems.Size());

      for (;(i<listItems.Size()) && (count > 0); i++)
      {
        CFileItemPtr item = listItems.Get(i);
        items.Add(item);
        count--;
      }      
    }
    else
    {
      // Error log will be written in HandleServerResponse()
      return true;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::HandleQueueRequest - [queue] - FAILED to retrieve objects from the server (bq)");

    // function will return TRUE because the skin will handle the 0 items
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeQueueDirectory::HandleQueueRequest - Exit function and returning list with size [%d] (bq)",items.Size());

  return true;
}

bool CBoxeeFeedDirectory::ConvertServerResponse(const CStdString& typeToRetrieveForLog,CFileItemList& listItems)
{
  int numOfItems = m_boxeeFeed.GetNumOfActions();

  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::ConvertServerResponse - [%s] - Enter function after retrieve from server [numOfItemsFromServer=%d] for type [%s]. [timestamp=%lu] (bf)",typeToRetrieveForLog.c_str(),numOfItems,typeToRetrieveForLog.c_str(),m_boxeeFeed.GetTimeStamp());
 
/*
  //we're using Empty screen messages, no need to check for empty lists
  //this is a fix for BOXEE-6278 and many other screens that have empty screens
  if(numOfItems == 0)
  {
    CLog::Log(LOGWARNING,"CBoxeeFeedDirectory::ConvertServerResponse - [%s] - Got from server [numOfItemsFromServer=%d=0] for type [%s], therefore going to return FALSE (bf)",typeToRetrieveForLog.c_str(),numOfItems,typeToRetrieveForLog.c_str());
    return false;
  }
*/

  for (int i=0; i < numOfItems; i++)
  {
    BOXEE::BXGeneralMessage msg = m_boxeeFeed.GetAction(i);
      
    CStdString strDesc = BoxeeUtils::GetFormattedDesc(msg, false);
    CStdString strOriginalDesc = BoxeeUtils::StripDesc(strDesc);
    int nTimeStamp = msg.GetTimestamp();
        
    CFileItemPtr item(new CFileItem);
        
    item->SetProperty("IsFeedItem", true);
    item->SetProperty("feeddesc",strOriginalDesc);
    item->SetProperty("originaldesc",strOriginalDesc);
    item->SetProperty("formatteddesc",strDesc);
    item->SetProperty("timestamp",nTimeStamp);
    item->m_dateTime = CDateTime(nTimeStamp);
        
    bool itemIsValid = true;

    // Go over all object in the item and set relevant properties
    for (int iObj = 0; iObj < msg.GetObjectCount(); iObj++) 
    {
      BXObject obj = msg.GetObject(iObj);

      if ((msg.GetMessageType() == MSG_ACTION_TYPE_QUEUE) && (obj.HasValue(MSG_KEY_CLIENT_ID)))
      {
        // in case it is a QUEUE message and the object has MSG_KEY_CLIENT_ID value -> need to take the item from DB

        CStdString strClientId = obj.GetValue(MSG_KEY_CLIENT_ID);
        CStdString strLabel;
        CStdString strPath;
        CStdString strThumbPath;

        BXMetadataEngine& MDE = BOXEE::Boxee::GetInstance().GetMetadataEngine();
        bool succeeded = MDE.GetQueueItem(strClientId, strLabel, strPath, strThumbPath);

        if (succeeded)
        {
          item->SetLabel(strLabel);
          item->m_strPath = strPath;
          item->SetThumbnailImage(strThumbPath);
          item->SetProperty("clientid",strClientId);
        }
        else
        {
          CLog::Log(LOGWARNING,"CBoxeeFeedDirectory::ConvertServerResponse - [%s] - FAILED to get from DB item for [client_id=%s]. Continue to next object (bf)(queue)",typeToRetrieveForLog.c_str(),strClientId.c_str());
          itemIsValid = false;
          break;
        }
      }
      else
      {
        BoxeeUtils::ObjToFileItem(obj, item.get());
      }
    }
        
    if (!itemIsValid)
    {
      item.reset();
      continue;
    }
        
    // ObjToFileItem initializes a property called feedId which is used
    // in conjunction with the message type to create a path for a feed 
    // item if it has not been set yet
    if (item->m_strPath == "")
    {
      item->m_strPath = "feed://";
      item->m_strPath += item->GetProperty("user_id");
      item->m_strPath += "_";
      item->m_strPath += msg.GetMessageType();
      item->m_strPath += "_";
      item->m_strPath += msg.GetReferral();
    }

    // We set this property in order to allow this file to be 
    // handled correctly by the DirectoryCache mechanism
    item->SetProperty("cacheId", item->m_strPath);
        
    CStdString strThumb = BoxeeUtils::GetMessageThumb(msg);
    item->SetProperty("actionicon", strThumb);
        
    if(msg.GetMessageType() == MSG_ACTION_TYPE_LISTEN)
    {
      item->SetProperty("ismusic", true);
    }
    else if(msg.GetMessageType() == MSG_ACTION_TYPE_WATCH)
    {
      item->SetProperty("isvideo", true);
    }

    if((msg.GetReferral()).empty() == false)
    {
      item->SetProperty("referral",msg.GetReferral());
    }
        
    if((msg.GetSource()).empty() == false)
    {
      CStdString feedSourceStr = msg.GetSource();
      item->SetProperty("feedsource",feedSourceStr);
      item->SetProperty("feedsource-lower",feedSourceStr.ToLower());
    }

    CStdString thumbsUp = msg.GetValue(MSG_KEY_LIKE);
    if (!thumbsUp.IsEmpty())
    {
      item->SetProperty("thumbsUp",thumbsUp);
    }

    //CStdString userText = msg.GetValue(MSG_KEY_USER_TEXT);
    CStdString userText = msg.GetValue("userTxt");
    if (!userText.IsEmpty())
    {
      item->SetProperty("user_message",userText);
    }

    // clean description
    if (!item->GetProperty("description").IsEmpty())
    {
      CStdString dirtyDescription = item->GetProperty("description");
      CStdString description = CRssFeed::CleanDescription(dirtyDescription);
      CUtil::UrlDecode(description);
      item->SetProperty("description", description);
    }

    item->SetProperty("FeedTypeItem", msg.GetMessageType());

    bool IsAllowedPath = IsAllowed(item->m_strPath);
    bool IsAllowedItem = item->IsAllowed();
        
    bool IsAllowedAppItem = true;
    if (item->IsApp())
    {
      CStdString appId = item->GetProperty("appid");
      if (!appId.IsEmpty())
      {
        IsAllowedAppItem = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().IsAppIdInAppBoxApplicationsList(appId);
      }
    }

    bool filePassMask = (IsAllowedPath && IsAllowedItem && IsAllowedAppItem);
        
    if(filePassMask)
    {
      UpdateItemProperties(msg,item);
      listItems.Add(item);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::ConvertServerResponse - [%s] - [%d/%d] - Not adding item [label=%s][path=%s] - [IsAllowedPath=%d][IsAllowedItem=%d][IsAllowedAppItem=%d] (bf)",typeToRetrieveForLog.c_str(),i+1,numOfItems,item->GetLabel().c_str(),item->m_strPath.c_str(),IsAllowedPath,IsAllowedItem,IsAllowedAppItem);
    }
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::ConvertServerResponse - [%s] - Exit function after build [listItemsSize=%d] for type [%s]. [numOfItemsFromServer=%d] (bf)",typeToRetrieveForLog.c_str(),listItems.Size(),typeToRetrieveForLog.c_str(),numOfItems);

  return true;
}

bool CBoxeeFeedDirectory::GetItemsFromServer(const CStdString& typeToRetrieveForLog, std::map<std::string, std::string>& mapParams)
{
  bool retVal = false;
  
  if(typeToRetrieveForLog == "recommend")
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Going to call BoxeeClientServerComManager::GetRecommendationsList() (rec)");

    retVal = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetRecommendations(m_boxeeFeed);
    
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Call to BoxeeClientServerComManager::GetRecommendationsList() returned [retVal=%d][BoxeeFeedSize=%d] (rec)",retVal,m_boxeeFeed.GetNumOfActions());
  }
  else if(typeToRetrieveForLog == "queue")
  {
    CStdString queueType = mapParams["type"];

    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Going to call BoxeeClientServerComManager::GetQueueList() with [queueType=%s] (queue)",queueType.c_str());

    retVal = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetQueue(m_boxeeFeed,CQueueItemsType::GetQueueItemTypeAsEnum(queueType));
    
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Call to BoxeeClientServerComManager::GetQueueList() returned [retVal=%d][BoxeeFeedSize=%d] (queue)",retVal,m_boxeeFeed.GetNumOfActions());
  }
  else if(typeToRetrieveForLog == "featured")
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Going to call BoxeeClientServerComManager::GetFeaturedList() (queue)");

    retVal = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetFeatured(m_boxeeFeed);
    
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetItemsFromServer - Call to BoxeeClientServerComManager::GetFeaturedList() returned [retVal=%d][BoxeeFeedSize=%d] (queue)",retVal,m_boxeeFeed.GetNumOfActions());
  }
  else if(typeToRetrieveForLog == "apps")
  {
    CLog::Log(LOGWARNING,"CBoxeeFeedDirectory::GetItemsFromServer - NOT supported for type [%s] (bf)",typeToRetrieveForLog.c_str());
  }  
  else if(typeToRetrieveForLog == "subscriptions")
  {
    CLog::Log(LOGWARNING,"CBoxeeFeedDirectory::GetItemsFromServer - NOT supported for type [%s] (bf)",typeToRetrieveForLog.c_str());
  }
  else if(typeToRetrieveForLog == "activity")
  {
    retVal = BOXEE::Boxee::GetInstance().GetBoxeeFeed(m_boxeeFeed);    
  }
  else if(typeToRetrieveForLog == "user")
  {
    retVal = BOXEE::Boxee::GetInstance().GetBoxeeFeed(m_boxeeFeed);
  }
  else
  {
    CLog::Log(LOGWARNING,"CBoxeeFeedDirectory::GetItemsFromServer - FAILED to analyze Type [%s] (bf)",typeToRetrieveForLog.c_str());
  }
  
  return retVal;
}

int CBoxeeFeedDirectory::GetTheLastItemIndexById(const CStdString& lastItemId)
{
  int numOfItems = m_boxeeFeed.GetNumOfActions();

  for (int i=0; i < numOfItems; i++)
  {
    const BOXEE::BXGeneralMessage& msg = m_boxeeFeed.GetAction(i);
    
    if(msg.GetValue("last") == lastItemId)
    {
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::GetTheLastItemIndexById - Exit function and returning [index=%d]. [numOfItems=%d] (bf)",i,numOfItems);
      return i;
    }
  }

  CLog::Log(LOGERROR,"CBoxeeFeedDirectory::GetTheLastItemIndexById - FAILED to find item with [lastItemId=%s]. Exit function and returning [index=-1]. [numOfItems=%d] (bf)",lastItemId.c_str(),numOfItems);

  return -1;
}

bool CBoxeeFeedDirectory::Exists(const char* strPath)
{
	// NOT IMPLEMENTED
	return true;
}

void CBoxeeFeedDirectory::UpdateItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item)
{
  CStdString messageType = msg.GetMessageType();

  if (messageType == MSG_ACTION_TYPE_QUEUE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateItemProperties - handling message [type=%s]. call UpdateQueueItemProperties (bf)(queue)",messageType.c_str());
    return UpdateQueueItemProperties(msg,item);
  }
  else if (messageType == MSG_ACTION_TYPE_RECOMMEND || messageType == MSG_ACTION_TYPE_SHARE || messageType == MSG_ACTION_TYPE_RATE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateItemProperties - handling message [type=%s]. call UpdateDiscoverItemProperties (bf)(rec)",messageType.c_str());
    return UpdateDiscoverItemProperties(msg,item);
  }
  else
  {
    //CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateItemProperties - NOT handling message [type=%s] (bf)",messageType.c_str());
  }
}

void CBoxeeFeedDirectory::UpdateQueueItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item)
{
  if (msg.GetMessageType() != MSG_ACTION_TYPE_QUEUE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateQueueItemProperties - message type [%s] ISN'T [%s] (bf)(queue)",msg.GetMessageType().c_str(),MSG_ACTION_TYPE_QUEUE);
    return;
  }

  CStdString showId = item->GetProperty("showid");
  if (!showId.IsEmpty() && BoxeeUtils::IsSubscribe(showId))
  {
    item->SetProperty("IsSubscribe",true);
  }

  CStdString source = msg.GetSource();

  CStdString addToQueueDesc = "";

  if (source == SUBSCRIPTION_SOURCE)
  {
    addToQueueDesc += g_localizeStrings.Get(57006);
  }
  else
  {
    addToQueueDesc = g_localizeStrings.Get(57000);

    if (source == BOOKLET_SOURCE)
    {
      addToQueueDesc += g_localizeStrings.Get(57001);
    }
    else if (source == BOXEE_SOURCE)
    {
      addToQueueDesc += g_localizeStrings.Get(57002);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateQueueItemProperties - FAILED to handle queue message with [source=%s] (bf)(queue)",source.c_str());
      return;
    }

    addToQueueDesc += BoxeeUtils::GetTimeAddedToDirectoryLabel(msg.GetTimestamp()).ToLower();
  }

  item->SetProperty("addToQueueDesc",addToQueueDesc);
}

void CBoxeeFeedDirectory::UpdateDiscoverItemProperties(const BOXEE::BXGeneralMessage& msg,CFileItemPtr item)
{
  CStdString messageType = msg.GetMessageType();

  if (messageType != MSG_ACTION_TYPE_RECOMMEND && messageType != MSG_ACTION_TYPE_SHARE && messageType != MSG_ACTION_TYPE_RATE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeFeedDirectory::UpdateDiscoverItemProperties - message type [%s] ISN'T [%s] or [%s] or [%s] (bf)(rec)",msg.GetMessageType().c_str(),MSG_ACTION_TYPE_RECOMMEND,MSG_ACTION_TYPE_SHARE,MSG_ACTION_TYPE_RATE);
    return;
  }

  CStdString addToDiscoverDesc = BoxeeUtils::GetTimeAddedToDirectoryLabel(msg.GetTimestamp());

  item->SetProperty("addToDiscoverDesc",addToDiscoverDesc);
}

} // namespace

