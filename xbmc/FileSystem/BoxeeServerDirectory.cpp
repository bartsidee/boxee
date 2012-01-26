
#include "StdString.h"
#include <map>
#include "PlatformDefs.h"
#include "BoxeeServerDirectory.h"
#include "Application.h"
#include "FileSystem/DirectoryCache.h"
#include "SpecialProtocol.h"
#include "URL.h"
#include "lib/libBoxee/boxee.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxsubscriptionsmanager.h"
#include "FileSystem/FileCurl.h"
#include "BoxeeUtils.h"
#include "Util.h"
#include "GUISettings.h"
#include "VideoInfoTag.h"

using namespace BOXEE;
using namespace XFILE;

namespace DIRECTORY
{

CBoxeeServerDirectory::CBoxeeServerDirectory()
{
  m_cacheDirectory = DIR_CACHE_ALWAYS;  // by default, caching is done.
}

CBoxeeServerDirectory::~CBoxeeServerDirectory()
{

}

bool CBoxeeServerDirectory::GetDirectory(const CStdString& strPath, CFileItemList &items)
{
  CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::GetDirectory, retreive contents for %s (bsd) (browse)", strPath.c_str());

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  CURL url(strPath);
  CStdString strProtocol = url.GetProtocol();
  std::map<CStdString, CStdString> mapOptions = url.GetOptionsAsMap();

  if (strProtocol.CompareNoCase("boxee") != 0)
    return false;

  CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::GetDirectory, GetHostName = %s, GetShareName = %s, GetDomain = %s, GetFileName = %s, GetOptions = %s (bsd) (browse)", 
      url.GetHostName().c_str(), url.GetShareName().c_str(), url.GetDomain().c_str(), url.GetFileName().c_str(), url.GetOptions().c_str());

  // The type of the request movies or tvshows
  CStdString strType = url.GetHostName();
  // The actual request
  CStdString strRequest = url.GetShareName();

  if (strType == "tvshows")
  {
    return HandleTvShows(strRequest, mapOptions, items);
  }
  else if (strType == "movies")
  {
    return HandleMovies(strRequest, mapOptions, items);
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::GetDirectory, unrecognized url %s (bsd) (browse)", strPath.c_str());
    return false;
  }

  return true;
}

bool CBoxeeServerDirectory::HandleTvShows(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  CFileItemList remoteItems;
  CFileItemList localItems;
  CFileItemList subscribedItems;

  std::map<CStdString, CStdString> mapRemoteOptions;
  std::map<CStdString, CStdString> mapLocalOptions;

  if (strRequest == "shows" || strRequest == "tv")
  {
    CStdString search;
    if (HasOption(mapOptions, "search", search))
    {
      CUtil::URLEncode(search);
      mapLocalOptions["search"] = search;
    }

    CStdString genre;
    if (HasOption(mapOptions, "genre", genre))
    {
      mapRemoteOptions["genre"] = genre;
      mapLocalOptions["genre"] = genre;
    }

    CStdString sort;
    if (HasOption(mapOptions, "sort", sort))
    {
      mapRemoteOptions["sort"] = sort;
    }

    CStdString free;
    if (HasOption(mapOptions, "free", free))
    {
      mapRemoteOptions["free"] = free;
    }

    CStdString prefix;
    if (HasOption(mapOptions, "prefix", prefix))
    {
      mapRemoteOptions["prefix"] = prefix;
      mapLocalOptions["prefix"] = prefix;
    }


    CStdString local = mapOptions["local"];
    if (local == "true")
    {
      // Get local items first
      CStdString localUrl = "boxeedb://tvshows/";
      localUrl += BoxeeUtils::BuildParameterString(mapLocalOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, get local items, url = %s (browse)", localUrl.c_str());
      CDirectory::GetDirectory(localUrl, localItems);
      MergeByBoxeeId(localItems, items);
    }

    CStdString subscribed = mapOptions["subscribed"];
    if (subscribed == "true")
    {
      CFileItemList subscribedItems;
      GetTvShowSubscriptions(subscribedItems);
      MergeByBoxeeId(subscribedItems, items);
    }

    // Check if remote items should be added as well (and we have internet connection)
    CStdString remote = mapOptions["remote"];
    if (remote == "true" &&  g_application.IsConnectedToInternet())
    {
      CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

      CStdString search;
      if (HasOption(mapOptions, "search", search))
      {
        CUtil::URLEncode(search);
        mapRemoteOptions["term"] = search;
        strBoxeeServerUrl += "/titles/search/tv";
      }
      else
      {
        strBoxeeServerUrl += "/titles/tv";
      }

      AddParametersToRemoteRequest(mapRemoteOptions);

      strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      //std::vector<std::string> vecServices;
      //Boxee::GetInstance().GetBoxeeClientServerComManager().GetServicesIds(vecServices);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, get remote items, url = %s (browse)(add)", strBoxeeServerUrl.c_str());

      CDirectory::GetDirectory(strBoxeeServerUrl, remoteItems);

      if (remoteItems.Size() == 0)
      {
        m_cacheDirectory = DIR_CACHE_NEVER;
      }

      MergeByBoxeeId(remoteItems, items);

    }

    return true;
  }
  else if (strRequest == "episodes")
  {
    CStdString seriesId = mapOptions["seriesId"];

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, episodes requested for show id = %s (browse)", seriesId.c_str());

    // get details of the TV show
    CFileItemList seriesItems;
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
    strBoxeeServerUrl += "/title/tv/";
    strBoxeeServerUrl += seriesId;

    CDirectory::GetDirectory(strBoxeeServerUrl, seriesItems);

    if (seriesItems.Size() == 1)
    {
      CFileItemPtr seriesItem = seriesItems.Get(0);
      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, got metadata for show id = %s, label = %s (browse)", seriesId.c_str(), seriesItem->GetLabel().c_str());

      items.SetProperty("serieslabel", seriesItem->GetLabel());
      items.SetProperty("seriesbackground", seriesItem->GetProperty("background"));
      items.SetProperty("seriesthumb", seriesItem->GetThumbnailImage());
    }

    // always set "seriesid" for unsubscribe, etc.
    items.SetProperty("seriesid", seriesId);

    CStdString local = mapOptions["local"];
    if (local == "true")
    {
      CStdString strBoxeePath = "boxeedb://episodes/";
      strBoxeePath += seriesId;
      strBoxeePath += "/";

      CDirectory::GetDirectory(strBoxeePath, localItems);
      MergeByBoxeeId(localItems, items);
    }

    CStdString remote = mapOptions["remote"];
    if (remote == "true" &&  g_application.IsConnectedToInternet())
    {
      strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
      strBoxeeServerUrl += "/title/";
      strBoxeeServerUrl += seriesId;
      strBoxeeServerUrl += "/episodes";

      AddParametersToRemoteRequest(mapRemoteOptions);

      strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, server url = %s (browse)(add)", strBoxeeServerUrl.c_str());

      CDirectory::GetDirectory(strBoxeeServerUrl, remoteItems);

      if (remoteItems.Size() == 0)
      {
        m_cacheDirectory = DIR_CACHE_NEVER;
      }

      if (local != "true")
      {
        // Get local paths only in case local episodes were not retrieved
        // Otherwise the local path will be added during merge
        for (int  i = 0; i < remoteItems.Size(); i++)
        {
          BoxeeUtils::GetLocalLinks(remoteItems.Get(i));
        }
      }

      MergeByBoxeeId(remoteItems, items);
    }

    // Filter our irrelevant seasons if necessary
    CFileItemList filteredItems;
    CStdString season;
    if (HasOption(mapOptions, "season", season))
    {
      int iSeason = atoi(season);
      for (int i = 0; i < items.Size(); i++)
      {
        CFileItemPtr item = items.Get(i);
        if (item->HasVideoInfoTag() && item->GetVideoInfoTag()->m_iSeason == iSeason)
        {
          filteredItems.Add(item);
        }
      }

      items.Clear();
      items.Append(filteredItems);
    }

    // add the right MediaType to the ReportToServer
    for (int i=0; i<items.Size(); i++)
    {
      items.Get(i)->SetProperty("rts-mediatype","tv_show");
    }

    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::HandleTvShows, unrecognized request type: %s (browse)", strRequest.c_str());
    return false;
  }
}

bool CBoxeeServerDirectory::HandleMovies(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  if (strRequest == "movies")
  {
    std::map<CStdString, CStdString> mapRemoteOptions;
    std::map<CStdString, CStdString> mapLocalOptions;
    CFileItemList remoteItems;
    CFileItemList localItems;

    CStdString genre;
    if (HasOption(mapOptions, "genre", genre))
    {
      mapRemoteOptions["genre"] = genre;
      mapLocalOptions["genre"] = genre;
    }

    CStdString sort;
    if (HasOption(mapOptions, "sort", sort))
    {
      mapRemoteOptions["sort"] = sort;
    }

	CStdString provider;
    if (HasOption(mapOptions, "provider", provider))
    {
      mapRemoteOptions["provider"] = provider;
    }

    CStdString free;
    if (HasOption(mapOptions, "free", free))
    {
      mapRemoteOptions["free"] = free;
    }

    CStdString prefix;
    if (HasOption(mapOptions, "prefix", prefix))
    {
      mapRemoteOptions["prefix"] = prefix;
      mapLocalOptions["prefix"] = prefix;
    }

    CStdString local = mapOptions["local"];
    if (local == "true")
    {
      CStdString search;
      if (HasOption(mapOptions, "search", search))
      {
        //CUtil::URLEncode(search);
        mapLocalOptions["search"] = search;
      }

      // Get local items first
      CStdString localUrl = "boxeedb://movies/";
      localUrl += BoxeeUtils::BuildParameterString(mapLocalOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::Movies, get local items, url = %s (browse)", localUrl.c_str());
      CDirectory::GetDirectory(localUrl, localItems);
      MergeByBoxeeId(localItems, items);
    }

    CStdString remote = mapOptions["remote"];
    if (remote == "true" &&  g_application.IsConnectedToInternet())
    {
      CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

      CStdString search;
      if (HasOption(mapOptions, "search", search))
      {
        CUtil::URLEncode(search);
        mapRemoteOptions["term"] = search;
        strBoxeeServerUrl += "/titles/search/movie";
      }
      else
      {
        strBoxeeServerUrl += "/titles/movie";
      }

      AddParametersToRemoteRequest(mapRemoteOptions);

      strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleMovies, server url = %s (browse)(add)", strBoxeeServerUrl.c_str());

      CDirectory::GetDirectory(strBoxeeServerUrl, remoteItems);

      if (remoteItems.Size() == 0)
      {
        m_cacheDirectory = DIR_CACHE_NEVER;
      }

      // TODO: Check whether this should be done here or as a background loader
      for (int  i = 0; i < remoteItems.Size(); i++)
      {
         BoxeeUtils::GetLocalLinks(remoteItems.Get(i));
      }

      MergeByBoxeeId(remoteItems, items);
    }

    // add the right MediaType to the ReportToServer
    for (int i=0; i<items.Size(); i++)
    {
      items.Get(i)->SetProperty("rts-mediatype","movie");
    }

    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::HandleTvShows, unrecognized request type: %s (browse)", strRequest.c_str());
    return false;
  }
}

bool CBoxeeServerDirectory::GetTvShowSubscriptions(CFileItemList& items)
{
  // Get a list of subscribed shows from the server
  std::vector<BXSubscriptionItem> vecSubscriptions;
  if (BoxeeUtils::GetSubscriptions(BOXEE::CSubscriptionType::TVSHOW_SUBSCRIPTION, vecSubscriptions))
  {
    for (size_t i = 0; i < vecSubscriptions.size(); i++)
    {
      BXSubscriptionItem subscriptionItem = vecSubscriptions[i];

      CFileItemPtr pItem ( new CFileItem(subscriptionItem.GetName()) );

      pItem->SetProperty("type", "tvshow");
      pItem->SetProperty("boxeeid", subscriptionItem.GetId());
      pItem->SetProperty("IsBoxeeServerItem", true);
      pItem->SetProperty("istvshowfolder", true);
      pItem->SetProperty("tvshow", true);

      items.Add(pItem);
    }
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::GetTvShowSubscriptions, could not get subscriptions (browse)");
  }
  return true;
}

bool CBoxeeServerDirectory::HasOption(const std::map<CStdString, CStdString>& mapOptions, const CStdString& optionName, CStdString& optionValue)
{
  std::map<CStdString, CStdString>::const_iterator lb = mapOptions.lower_bound(optionName);
  if(lb != mapOptions.end() && !(mapOptions.key_comp()(optionName, lb->first)))
  {
    optionValue = lb->second;
    return true;
  }
  return false;
}

bool CBoxeeServerDirectory::Exists(const char* strPath)
{
  // NOT IMPLEMENTED
  return true;

}

// Receives two lists, left and right and merges the left list into the right list
// Left list is destroyed in the process
void CBoxeeServerDirectory::MergeByBoxeeId(CFileItemList& left, CFileItemList& right)
{
  if (left.Size() == 0)
    return;

  if (right.Size() == 0)
  {
    right.Append(left);
    return;
  }

  // Initialize fast lookup for items by boxee id
  right.SetFastLookup(true, "boxeeid");

  for (int i = 0; i < left.Size(); i++)
  {
    CStdString leftId = left[i]->GetProperty("boxeeid");
    CFileItemPtr leftItem = left[i];

    CFileItemPtr rightItem = right.Get("boxeeid", leftId);

    if (rightItem.get() != NULL)
    {
      // we found a match
      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows, match found = %s (browse)", leftItem->GetLabel().c_str());

      // Get the links from the left item and add them to the right item
      const CFileItemList* linksFileItemList = leftItem->GetLinksList();

      if (linksFileItemList)
      {
        // Copy all links from local to the remote item
        for (int j = 0; j < linksFileItemList->Size(); j++)
        {
          CFileItemPtr link = linksFileItemList->Get(j);
          rightItem->AddLink(link->GetLabel(), link->m_strPath, link->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(link->GetProperty("link-boxeetype")), link->GetProperty("link-provider"), link->GetProperty("link-providername"), link->GetProperty("link-providerthumb"), link->GetProperty("link-countrycodes"), link->GetPropertyBOOL("link-countryrel"),link->GetProperty("quality-lbl"),link->GetPropertyInt("quality"));
        }
      }

      left.Remove(i);
      i--; // set the loop back
    }
  }

  // Append the remaining items to the right list
  right.Append(left);
}

void CBoxeeServerDirectory::AddParametersToRemoteRequest(std::map<CStdString, CStdString>& mapRemoteOptions)
{
  if (!CUtil::IsAdultAllowed())
  {
    mapRemoteOptions["adult"] = "no";
  }

  if (g_guiSettings.GetBool("filelists.filtergeoip"))
  {
    CStdString countryCode = g_application.GetCountryCode();

    if (!countryCode.IsEmpty())
    {
      mapRemoteOptions["geo"] = countryCode;
    }
  }
}

DIR_CACHE_TYPE CBoxeeServerDirectory::GetCacheType(const CStdString& strPath) const
{
  return m_cacheDirectory;
}

}

