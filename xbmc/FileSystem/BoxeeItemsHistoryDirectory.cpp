
#include "BoxeeItemsHistoryDirectory.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxobject.h"
#include "FileSystem/DirectoryCache.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "URL.h"
#include "Application.h"
#include "lib/libBoxee/bxutils.h"
#include "MusicInfoTag.h"
#include "PictureThumbLoader.h"
#include "Picture.h"
#include "Util.h"
#include "File.h"
#include "LocalizeStrings.h"
#include "BoxeeItemsHistory.h"

using namespace MUSIC_INFO;
using namespace BOXEE;

namespace DIRECTORY
{

CBoxeeItemsHistoryDirectory::CBoxeeItemsHistoryDirectory()
{

}

CBoxeeItemsHistoryDirectory::~CBoxeeItemsHistoryDirectory()
{

}

bool CBoxeeItemsHistoryDirectory::GetDirectory(const CStdString& strPath, CFileItemList &itemsList)
{
  g_application.GetBoxeeItemsHistoryList().WaitForHistoryReady(INFINITE);

  // Check if request if for "history" protocol
  if (strPath.Left(10).Equals("history://") == false)
  {
    CLog::Log(LOGERROR, "Trying to retrieve files history with invalid name [%s] - (bh)", strPath.c_str());
    return false;
  }
    
  CBoxeeMediaTypes::BoxeeMediaTypesEnums typeToRetrieve = GetTypeToRetrieve(strPath);
    
  // Check if type to retrieve is valid
  if(typeToRetrieve == CBoxeeMediaTypes::NONE)
  {
    CLog::Log(LOGERROR, "Failed to get type to retrieve from [%s] (bh)", strPath.c_str());
    return false;
  }
    
  // Get a duplicated fileHistoryList from CApplication
  CFileItemList itemsHistoryList;
    
  CLog::Log(LOGDEBUG,"Going to get the ItemsHistoryList (bh)");
    
  g_application.GetBoxeeItemsHistoryList().GetFilesHistory(itemsHistoryList);
    
  CLog::Log(LOGDEBUG,"ItemsHistoryList with [size=%d] was received (bh)",itemsHistoryList.Size());
    
  if(itemsHistoryList.Size() <= 0)
  {
    CLog::Log(LOGDEBUG,"The ItemsHistoryList contains [%d] files, therefore there are no history files to return (bh)",itemsHistoryList.Size());
    return true;
  }
    
  int numOfHistoryFilesToRetrieve = itemsHistoryList.Size();
    
	CLog::Log(LOGDEBUG,"After parsing request [%s]. Going to retrieve media for type [%s] - (bh)",strPath.c_str(),GetBoxeeMediaTypeEnumAsString(typeToRetrieve));
    
	// Check if there is a limit on the number of history item to retrieve
	CStdString strDir;
	CStdString strFile;
	std::map<std::string, std::string> mapParams;
    
  if( BoxeeUtils::ParseBoxeeDbUrl( strPath, strDir, strFile, mapParams ) )
	{
	  // Get the limit
    int limit = BXUtils::StringToInt(mapParams["limit"]);
      
    CLog::Log(LOGDEBUG,"The limit that was read from [strPath=%s] is [%d] - (bh)",strPath.c_str(),limit);
    if(limit > 0)
    {
      if(limit < numOfHistoryFilesToRetrieve)
      {
        CLog::Log(LOGDEBUG,"The request was recived with limit [%d] and the HistoryFiles list size is [%d], therefore going to return the limit size - (bh)",limit,numOfHistoryFilesToRetrieve);
        numOfHistoryFilesToRetrieve = limit;
      }
    }
    else if(limit == 0)
    {
      CLog::Log(LOGDEBUG,"The limit that was read from [strPath=%s] is [%d], either it exist as [limit=%d] or doesn't, therefore going to return all of the file that qualify - (bh)",strPath.c_str(),limit,limit);
    }
    else
    {
      CLog::Log(LOGERROR,"Received limit with negative value [strPath=%s], therefore going to return all of the file that qualify - (bh)",strPath.c_str());
    }
	}
  else
  {
    CLog::Log(LOGERROR,"Failed to parse [strPath=%s] by BoxeeUtils::ParseBoxeeDbUrl - (bh)",strPath.c_str());
    return false;
  }
    
	for(int i=0; i<itemsHistoryList.Size(); i++)
	{
    CFileItemPtr item = itemsHistoryList.Get(i);
      
    if(item != NULL)
    {
      bool fileMatch = DoesFileMatchForRetrieve(item.get(),typeToRetrieve);

      if(fileMatch == true)
      {
        // Create the Suitable item that will be entered to the list
        CFileItemPtr newItem ( new CFileItem(*item) );

        // If the item has no thumb, try to put its "HasAutoThumb", "OriginalThumb" or "ParentThumb" ones
        if(!newItem->HasThumbnail() || (CUtil::IsHD(newItem->GetThumbnailImage()) && !XFILE::CFile::Exists(newItem->GetThumbnailImage())) )
        {
          // This is needed in order to get the item to be loaded by the background loader
          newItem->SetProperty("IsLoaded", false);
            
          if(newItem->GetPropertyBOOL("HasAutoThumb"))
          {
            newItem->SetThumbnailImage(newItem->GetProperty("AutoThumbImage"));
          }
          else if(newItem->HasProperty("OriginalThumb") && !newItem->GetProperty("OriginalThumb").IsEmpty())
          {
            newItem->SetThumbnailImage(newItem->GetProperty("OriginalThumb"));
          }
          else if(newItem->HasProperty("ParentThumb"))
          {
            newItem->SetThumbnailImage(newItem->GetProperty("ParentThumb"));
          }
          else
          {
            // There are no "HasAutoThumb", "OriginalThumb" or "ParentThumb" -> Do nothing
          }

          // We don't want to show "defaultloading.gif" in the HistoryList
          if(newItem->GetThumbnailImage() == "defaultloading.gif")
          {
            newItem->SetThumbnailImage("");
              
            CLog::Log(LOGDEBUG,"CBoxeeItemsHistoryDirectory::GetDirectory - Because the thumb of the history item [%s] was [defaultloading.gif], it was changed to [thumb=%s] in order to show the DefaultThumb (bh)",(newItem->m_strPath).c_str(),(newItem->GetThumbnailImage()).c_str());
          }
        }

        UpdateHistoryItemProperties(newItem);

        itemsList.Add(newItem);
          
        if(itemsList.Size() == numOfHistoryFilesToRetrieve)
        {
          break;
        }
      }
    }
    else
    {
      CLog::Log(LOGERROR,"Got NULL from the FilesHistoryLIst for item [%d]. Continue to next item - (bh)",i);
      continue;
    }
	}

	itemsList.SetProperty("empty",itemsHistoryList.GetPropertyBOOL("empty"));
    
	CLog::Log(LOGDEBUG,"Going to return a HistoryList with size [%d] - (bh)",itemsList.Size());
    
	return true;
}

bool CBoxeeItemsHistoryDirectory::Exists(const char* strPath)
{
	// NOT IMPLEMENTED
	return true;
}

/////////////////////////////////////////////
///////				Private functions				///////
/////////////////////////////////////////////

void CBoxeeItemsHistoryDirectory::UpdateHistoryItemProperties(CFileItemPtr historyItem)
{
  if (historyItem->GetPropertyBOOL("isseparator"))
  {
    return;
  }

  CStdString addToHistoryDesc = "";
  CStdString historyItemType = historyItem->GetProperty("MediaType");

  if (historyItemType == "video" || historyItemType == "picture")
  {
    addToHistoryDesc = g_localizeStrings.Get(57040);
  }
  else
  {
    addToHistoryDesc = g_localizeStrings.Get(57041);
  }

  int tmAddToHistory = historyItem->GetPropertyInt("AddedToHistoryListTime");

  addToHistoryDesc += BoxeeUtils::GetTimeAddedToDirectoryLabel(tmAddToHistory).ToLower();

  historyItem->SetProperty("addToHistoryDesc",addToHistoryDesc);
}

CBoxeeMediaTypes::BoxeeMediaTypesEnums CBoxeeItemsHistoryDirectory::GetTypeToRetrieve(const CStdString& strPath)
{
	CBoxeeMediaTypes::BoxeeMediaTypesEnums typeToRetrieve = CBoxeeMediaTypes::NONE;

	// Analyze the url and determime which actions should be included
	CURI url(strPath);
	CStdString typeToRetrieveStr = url.GetHostName();

	if(typeToRetrieveStr == "all")
	{
		typeToRetrieve = CBoxeeMediaTypes::ALL;
	}
	else if(typeToRetrieveStr == "video")
	{
		typeToRetrieve = CBoxeeMediaTypes::VIDEO;
	}
	else if(typeToRetrieveStr == "audio")
	{
		typeToRetrieve = CBoxeeMediaTypes::AUDIO;
	}
	else if(typeToRetrieveStr == "picture")
	{
		typeToRetrieve = CBoxeeMediaTypes::PICTURE;
	}
	else
	{
		// Error log will be written out of this function
	}

	return typeToRetrieve;
}

bool CBoxeeItemsHistoryDirectory::DoesFileMatchForRetrieve(CFileItem* item,CBoxeeMediaTypes::BoxeeMediaTypesEnums typeToRetrieve)
{
	bool fileMatch = false;

	switch(typeToRetrieve)
	{
	case CBoxeeMediaTypes::ALL:
		fileMatch = true;
		break;
	case CBoxeeMediaTypes::VIDEO:
		if((item->IsVideo()) || (item->IsRSS()))
		{
			fileMatch = true;
		}
		break;
	case CBoxeeMediaTypes::AUDIO:
		if((item->IsAudio()) || (item->GetPropertyBOOL("isMusicFolder")) || (item->IsLastFM()) || (item->IsShoutCast()))
		{
			fileMatch = true;
		}
		break;
	case CBoxeeMediaTypes::PICTURE:
		if((item->IsPicture()) || (item->GetPropertyBOOL("isPictureFolder")))
		{
			fileMatch = true;
		}
		break;
	default:
		CLog::Log(LOGERROR,"Invalid type to retrieve [%s]. Should have been deected earlier - (bh)",GetBoxeeMediaTypeEnumAsString(typeToRetrieve));
	}

	return fileMatch;
}

const char* CBoxeeItemsHistoryDirectory::GetBoxeeMediaTypeEnumAsString(CBoxeeMediaTypes::BoxeeMediaTypesEnums boxeeMediaTypeEnum)
{
	switch(boxeeMediaTypeEnum)
	{
	case CBoxeeMediaTypes::ALL:
	  return "ALL";
	case CBoxeeMediaTypes::VIDEO:
	  return "VIDEO";
	case CBoxeeMediaTypes::AUDIO:
	  return "AUDIO";
	case CBoxeeMediaTypes::PICTURE:
	  return "PICTURE";
	case CBoxeeMediaTypes::NONE:
	  return "NONE";
	default:
		CLog::Log(LOGERROR,"Failed to convert enum [%d] to string MediaType. Return NONE - (bh)",boxeeMediaTypeEnum);
	  return "NONE";
	}
}

} // namespace


