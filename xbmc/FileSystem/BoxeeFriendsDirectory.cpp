
#include "BoxeeFriendsDirectory.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/DirectoryCache.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "lib/libBoxee/bxfriendslist.h"
#include "URL.h"
#include "lib/libBoxee/bxutils.h"
#include "Util.h"
#include "bxcscmanager.h"

using namespace BOXEE;

namespace DIRECTORY
{

CBoxeeFriendsDirectory::CBoxeeFriendsDirectory()
{
	//SetCacheDirectory(true);
}

CBoxeeFriendsDirectory::~CBoxeeFriendsDirectory()
{
}

bool CBoxeeFriendsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
	CURI url(strPath);
	CStdString protocol = url.GetProtocol();
  CStdString hostname = url.GetHostName();
  CStdString filename = url.GetFileName();

	CLog::Log(LOGDEBUG,"Enter to CBoxeeFriendsDirectory::GetDirectory with [strPath=%s] - [Protocol=%s][HostName=%s][FileName=%s] (fr)",strPath.c_str(),protocol.c_str(),hostname.c_str(),filename.c_str());

	if((protocol != "friends") || (hostname != "user"))
	{
		CLog::Log(LOGERROR,"The HostName [%s] received is not [friends] or the Protocol [%s] received is not [user]. [strPath=%s] (fr)",hostname.c_str(),protocol.c_str(),strPath.c_str());
		return false;
	}

	/*
	// If we have the items in the cache, return them
	if (g_directoryCache.GetDirectory(strPath, items))
    {
		CLog::Log(LOGDEBUG, "The FriendsList of [%s] already exist in the cache (fr)", strPath.c_str());
	    return true;
	}
	*/

  int limit = (-1);
  CStdString userName = "";
  CStdString params = "";

  if(filename.IsEmpty() == false)
  {
    // Split the filename by "?" in order to check if there is a limit parameter

    CStdStringArray arrDirAndParams;
    StringUtils::SplitString(filename, "?", arrDirAndParams);

    if(arrDirAndParams.size() == 1)
    {
      // Case of no "?" delimiter

      userName = arrDirAndParams[0];
      CUtil::RemoveSlashAtEnd(userName);

      CLog::Log(LOGDEBUG,"After splitting [FileName=%s] by [?] delimiter got userName [%s] and params [%s] (fr)",filename.c_str(),userName.c_str(),params.c_str());
    }
    else if(arrDirAndParams.size() == 2)
    {
      // Case of one "?" delimiter

      userName = arrDirAndParams[0];
      CUtil::RemoveSlashAtEnd(userName);

      params = arrDirAndParams[1];

      CLog::Log(LOGDEBUG,"After splitting [FileName=%s] by [?] delimiter got userName [%s] and params [%s] (fr)",filename.c_str(),userName.c_str(),params.c_str());
    }
    else
    {
      // Case of more then one "?" delimiter

      CLog::Log(LOGERROR,"Faild to parse [FileName=%s] because there is more then one \"?\" characters (fr)",filename.c_str());
      return false;
    }

    // In case there is params -> Get the limit param value
    if(params.IsEmpty() == false)
    {
      CStdString dir;
      CStdString file;
      std::map<std::string, std::string> mapParams;

      if( BoxeeUtils::ParseBoxeeDbUrl( strPath, dir, file, mapParams ) )
      {
        limit = BXUtils::StringToInt(mapParams["limit"]);
        CLog::Log(LOGDEBUG,"Retrieve [limit=%d] from params [%s] (fr)",limit,params.c_str());
      }
      else
      {
        CLog::Log(LOGERROR,"Failed to parse [%s] with BoxeeUtils::ParseBoxeeDbUrl. Going to exit (fr)",strPath.c_str());
        return true;
      }
    }
  }

  if(limit == 0)
  {
    CLog::Log(LOGDEBUG,"limit value is [%d] in [strPath=%s]. Going to return all of the user friends (fr)",limit,strPath.c_str());
    limit = (-1);
  }

  // retreive the FriendsList of a user

	BOXEE::BXFriendsList friendsList;
	bool bResult = false;

	// 041109 - At the moment we don't want to ask the server for the user FriendList
	//bResult = BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetFriends(friendsList,userName);

	CLog::Log(LOGDEBUG,"Retrieve [FriendsListSize=%d] for user [%s] (fr)",friendsList.GetCount(),userName.c_str());

	if (bResult)
	{
	  int numOfFriendsToRetrieve = GetNumOfFriendsToRetrieve(strPath,friendsList.GetCount(),limit);

		items.Clear();
		BOXEE::FriendsListIterator iter = friendsList.Iterate();

		int numOfFriendsAdded = 0;

		while ((numOfFriendsAdded < numOfFriendsToRetrieve) && (!iter.IsEndOfList()))
		{
			BOXEE::BXFriend aFriend = iter.GetFriend();

			CFileItemPtr item (new CFileItem());
			item->SetProperty("IsFriend", true);

			items.Add(item);
			numOfFriendsAdded++;

			item->SetLabel(aFriend.GetAttribute(FRIEND_ATTRIB_NAME));

			if (aFriend.GetActionCount() > 0)
			{
				BOXEE::BXGeneralMessage action = aFriend.GetAction(0);
                item->SetLabel2(BoxeeUtils::GetFormattedDesc(action,false));
				item->SetIconImage(BoxeeUtils::GetMessageThumb(action));
			}

			item->SetProperty("userid", aFriend.GetFriendId());
			item->SetProperty("presencethumb",BoxeeUtils::GetBoxeePresenceThumb(aFriend.GetAttribute(MSG_KEY_PRESENCE)).c_str());
			item->SetProperty("isonline",aFriend.GetAttribute(MSG_KEY_PRESENCE) == "online");
			item->SetProperty("userthumb",BoxeeUtils::AddCredentialsToLink(aFriend.GetAttribute(MSG_KEY_THUMB)).c_str());
			item->SetThumbnailImage(BoxeeUtils::AddCredentialsToLink(aFriend.GetAttribute(MSG_KEY_THUMB)));
			
			item->SetProperty("birthday", aFriend.GetAttribute(MSG_KEY_BDAY));		
			item->SetProperty("gender", aFriend.GetAttribute(MSG_KEY_GENDER));    
			item->SetProperty("location", aFriend.GetAttribute(MSG_KEY_LOCATION));    
			item->SetProperty("description", aFriend.GetAttribute(MSG_KEY_DESCRIPTION));    
			
			item->m_strPath = "friends://user/" + aFriend.GetFriendId();

			iter.Next();
		}
	}
	else
	{
		CLog::Log(LOGERROR,"Failed to retrieve the FriendsList of user [%s] (fr)",userName.c_str());
		return false;
	}
	
	// Sort friends by name
	items.Sort(SORT_METHOD_LABEL, SORT_ORDER_ASC);

	/*
	if (m_cacheDirectory)
	{
		CLog::Log(LOGDEBUG, "LIBRARY, FEED, LIB1: Boxee Feed Directory, adding to cache, path = %s", strPath.c_str());
		g_directoryCache.ClearDirectory(strPath);
		g_directoryCache.SetDirectory(strPath, items);
	}
	else {
		CLog::Log(LOGDEBUG, "LIBRARY, FEED, LIB1: Boxee Feed Directory, do not add to cache, path = %s", strPath.c_str());
	}
	*/

	CLog::Log(LOGDEBUG,"Going to return [%d] friends out of the user [%s] FriendsList [FriendsListSize=%d] (fr)",items.Size(),userName.c_str(),friendsList.GetCount());

	return true;
}

int CBoxeeFriendsDirectory::GetNumOfFriendsToRetrieve(const CStdString& strPath, int numOfFriends, int limit)
{
  // Check if there is a limit on the number of friends item to retrieve

  if(limit < 0)
  {
    CLog::Log(LOGDEBUG,"limit value is negative in [strPath=%s]. Going to return all of the user friends [%d] (fr)",strPath.c_str(),numOfFriends);
    return numOfFriends;
  }
  else if(limit > 0)
  {
    if(limit < numOfFriends)
    {
      CLog::Log(LOGDEBUG,"The request was recived with limit [%d], which is less then the numOfFriends is [%d]. Going to return the limit size [%d] (fr)",limit,numOfFriends,limit);
      return limit;
    }
    else
    {
      CLog::Log(LOGDEBUG,"The request was recived with limit [%d], which is more then the numOfFriends is [%d]. Going to return all of the user friends [%d] (fr)",limit,numOfFriends,numOfFriends);
      return numOfFriends;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"limit value in [strPath=%s] is [%d]. Going to return all of the user friends [%d] (fr)",strPath.c_str(),limit,numOfFriends);
    return numOfFriends;
  }
}

bool CBoxeeFriendsDirectory::Exists(const char* strPath)
{
	// NOT IMPLEMENTED
	return true;

}

} // namespace

