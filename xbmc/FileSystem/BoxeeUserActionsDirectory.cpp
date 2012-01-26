
#include "BoxeeUserActionsDirectory.h"
#include "lib/libBoxee/boxee.h"
#include "FileSystem/DirectoryCache.h"
#include "BoxeeUtils.h"
#include "utils/log.h"
#include "lib/libBoxee/bxfriendslist.h"
#include "URL.h"
#include "lib/libBoxee/bxutils.h"
#include "Util.h"

using namespace BOXEE;

namespace DIRECTORY
{

CBoxeeUserActionsDirectory::CBoxeeUserActionsDirectory()
{

}

CBoxeeUserActionsDirectory::~CBoxeeUserActionsDirectory()
{

}

bool CBoxeeUserActionsDirectory::GetDirectory(const CStdString& strPath, CFileItemList &itemsList)
{
  CURI url(strPath);
  CStdString protocol = url.GetProtocol();
  CStdString hostname = url.GetHostName();
  CStdString filename = url.GetFileName();

  CLog::Log(LOGDEBUG,"Enter to CBoxeeUserActionsDirectory::GetDirectory with [strPath=%s][Protocol=%s][HostName=%s][FileName=%s] (ac)",strPath.c_str(),protocol.c_str(),hostname.c_str(),filename.c_str());

  if((protocol != "actions") || (hostname != "user"))
  {
    CLog::Log(LOGERROR,"The HostName [%s] received is not [actions] or the Protocol [%s] received is not [user]. [strPath=%s] (ac)",hostname.c_str(),protocol.c_str(),strPath.c_str());
    return false;
  }

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

      CLog::Log(LOGDEBUG,"After splitting [FileName=%s] by [?] delimiter got userName [%s] and params [%s] (ac)",filename.c_str(),userName.c_str(),params.c_str());
    }
    else if(arrDirAndParams.size() == 2)
    {
      // Case of one "?" delimiter

      userName = arrDirAndParams[0];
      CUtil::RemoveSlashAtEnd(userName);

      params = arrDirAndParams[1];

      CLog::Log(LOGDEBUG,"After splitting [FileName=%s] by [?] delimiter got userName [%s] and params [%s] (ac)",filename.c_str(),userName.c_str(),params.c_str());
    }
    else
    {
      // Case of more then one "?" delimiter

      CLog::Log(LOGERROR,"Faild to parse [FileName=%s] because there is more then one \"?\" characters (ac)",filename.c_str());
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
        CLog::Log(LOGDEBUG,"Retrieve [limit=%d] from params [%s] (ac)",limit,params.c_str());
      }
      else
      {
        CLog::Log(LOGERROR,"Failed to parse [%s] with BoxeeUtils::ParseBoxeeDbUrl. Going to exit (ac)",strPath.c_str());
        return true;
      }
    }
  }

  if(limit == 0)
  {
    CLog::Log(LOGDEBUG,"limit value is [%d] in [strPath=%s]. Going to return all of the user actions (ac)",limit,strPath.c_str());
    limit = (-1);
  }

  // retreive the FriendsList of a user

  BOXEE::BXBoxeeFeed userActionsList;
  bool bResult = false;

  bResult = BOXEE::Boxee::GetInstance().RetrieveUserActions(userActionsList,userName);

  CLog::Log(LOGDEBUG,"Retrieve [ActionsListSize=%d] for user [%s] (ac)",userActionsList.GetNumOfActions(),userName.c_str());

  if(bResult)
  {
    int numOfActionsToRetrieve = GetNumOfActionsToRetrieve(strPath,userActionsList.GetNumOfActions(),limit);

    int numOfActionsAdded = 0;

    for (int i=0; ((numOfActionsAdded < numOfActionsToRetrieve) && (i < userActionsList.GetNumOfActions())); i++)
    {
      BOXEE::BXGeneralMessage msg = userActionsList.GetAction(i);

      CStdString strDesc = BoxeeUtils::GetFormattedDesc(msg, false);

      CStdString strOriginalDesc = BoxeeUtils::StripDesc(strDesc);
      int nTimeStamp = msg.GetTimestamp();

      CFileItemPtr item ( new CFileItem(strOriginalDesc) );

      item->SetProperty("IsFeedItem", true);
      item->SetProperty("originaldesc",strOriginalDesc);

      item->SetProperty("time",BoxeeUtils::FormatTime(nTimeStamp).c_str());
      //item->SetUserData(new BOXEE::BXGeneralMessage(msg));
      
      // Go over all object in the item and set relevant properties
      for (int iObj = 0; iObj < msg.GetObjectCount(); iObj++) 
      {
        BoxeeUtils::ObjToFileItem(msg.GetObject(iObj), item.get());
      }

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
        //CLog::Log(LOGDEBUG,"CBoxeeUserActionsDirectory::GetDirectory - After setting property [referral=%s] in Item [label=%s]  (referral)",(item->GetProperty("referral")).c_str(),(item->GetLabel()).c_str());
      }

      itemsList.Add(item);

      numOfActionsAdded++;
     }
   }
  else
  {
    CLog::Log(LOGERROR,"Failed to retrieve the ActionsList of user [%s] (ac)",userName.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"Going to return [%d] actions out of the user [%s] ActionsList [ActionsListSize=%d] (ac)",itemsList.Size(),userName.c_str(),userActionsList.GetNumOfActions());

	return true;
}

int CBoxeeUserActionsDirectory::GetNumOfActionsToRetrieve(const CStdString& strPath, int numOfActions, int limit)
{
  // Check if there is a limit on the number of actions item to retrieve

  if(limit < 0)
  {
    CLog::Log(LOGDEBUG,"limit value is negative in [strPath=%s]. Going to return all of the user actions [%d] (ac)",strPath.c_str(),numOfActions);
    return numOfActions;
  }
  else if(limit > 0)
  {
    if(limit < numOfActions)
    {
      CLog::Log(LOGDEBUG,"The request was recived with limit [%d], which is less then the numOfActions is [%d]. Going to return the limit size (ac)",limit,numOfActions);
      return limit;
    }
    else
    {
      CLog::Log(LOGDEBUG,"The request was recived with limit [%d], which is more then the numOfActions is [%d]. Going to return all of the user actions (ac)",limit,numOfActions);
      return numOfActions;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"limit value in [strPath=%s] is [%d]. Going to return all of the user actions [%d] (ac)",strPath.c_str(),limit,numOfActions);
    return numOfActions;
  }
}

bool CBoxeeUserActionsDirectory::Exists(const char* strPath)
{
	// NOT IMPLEMENTED
	return true;
}

} // namespace


