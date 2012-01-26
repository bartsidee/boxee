
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
#include "SortFileItem.h"

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
  CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::GetDirectory -  enter function with [path=%s] (browse)(bsd)", strPath.c_str());

  m_cacheDirectory = DIR_CACHE_ALWAYS;

  CURI url(strPath);
  CStdString strProtocol = url.GetProtocol();
  std::map<CStdString, CStdString> mapOptions = url.GetOptionsAsMap(); // NOTICE: the URL options are decoded here

  if (strProtocol.CompareNoCase("boxee") != 0)
    return false;

  CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::GetDirectory - [HostName=%s][ShareName=%s][Domain=%s][FileName=%s][Options=%s] (browse)(bsd)", url.GetHostName().c_str(), url.GetShareName().c_str(), url.GetDomain().c_str(), url.GetFileName().c_str(), url.GetOptions().c_str());

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
  else if (strType == "clips")
  {
    return HandleClips(strRequest, mapOptions, items);
  }
  else if (strType == "sources")
  {
    return HandleSources(strRequest, mapOptions, items);
  }
  else if (strType == "subscriptions")
  {
    return GetTvShowSubscriptions(items);
  }
  else if (strType == "trailers")
  {
    return HandleTrailers(strRequest, mapOptions, items);
  }
  else if (strType == "search")
  {
    return HandleGlobalSearch(strRequest, mapOptions, items);
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::GetDirectory - unrecognized url [%s] (browse)(bsd)", strPath.c_str());
    return false;
  }

  return true;
}

bool CBoxeeServerDirectory::HandleGlobalSearch(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  std::map<CStdString, CStdString> mapRemoteOptions = mapOptions;

  if (g_application.IsConnectedToInternet())
  {
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.GlobalSearch","http://res.boxee.tv/titles/search/all");

    CStdString searchTerm;
    if (HasOption(mapOptions, "term", searchTerm))
    {
      CUtil::URLEncode(searchTerm);
      mapRemoteOptions["term"] = searchTerm;
    }
    else
    {
      return false;
    }

    AddParametersToRemoteRequest(mapRemoteOptions);

    strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleGlobalSearch - server [url=%s] (browse)(add)", strBoxeeServerUrl.c_str());

    CDirectory::GetDirectory(strBoxeeServerUrl, items);

    if (items.Size() == 0)
    {
      m_cacheDirectory = DIR_CACHE_NEVER;
    }
  }

  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr pItem (items.Get(i));

    pItem->SetProperty("infoLoaded",true);
    pItem->SetProperty("IsSearchItem", true);
  }

  return true;
}

bool CBoxeeServerDirectory::HandleSources(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  std::vector<BXSourcesItem> sourcesVec;

  if (strRequest == "shows")
  {
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetTvSources(sourcesVec);
  }
  else if (strRequest == "movies")
  {
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetMovieSources(sourcesVec);
  }
  else if (strRequest == "all")
  {
    BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().GetSources(sourcesVec);
  }
  else
  {
    return false;
  }

  for (std::vector<BXSourcesItem>::iterator it = sourcesVec.begin() ; it != sourcesVec.end() ; ++it )
  {
    CFileItemPtr pItem ( new CFileItem(it->GetSourceName()) );

    pItem->SetThumbnailImage(it->GetSourceThumb());

    if (!it->GetSourceGeo().empty())
    {
      pItem->SetProperty("country-codes" , it->GetSourceGeo());
    }
    else
    {
      pItem->SetProperty("country-codes" , "all");
    }

    pItem->SetProperty("country-rel" , true);
    pItem->SetProperty("type", it->GetSourceType());
    pItem->SetProperty("sourceid", it->GetSourceId());
    pItem->SetProperty("sourcename", it->GetSourceName());
    pItem->SetProperty("sourcetype", it->GetSourceType());
    pItem->SetProperty("sourcegeo", it->GetSourceGeo());
    pItem->SetProperty("sourcethumb", it->GetSourceThumb());
    pItem->SetThumbnailImage(it->GetSourceThumb());
    pItem->SetProperty("sourcepremium", it->GetSourcePremium());
    pItem->SetProperty("sourceoffer", it->GetSourceOffer());

    if (pItem->IsAllowed())
    {
      items.Add(pItem);
      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleSources - appending item [%d] of [%d] (bsd)",items.Size(),(int)sourcesVec.size());
    }
  }

  return true;
}

bool CBoxeeServerDirectory::HandleTvShows(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  //CFileItemList subscribedItems;

  std::map<CStdString, CStdString> mapRemoteOptions = mapOptions;

  if (strRequest == "shows" || strRequest == "tv")
  {
    if (g_application.IsConnectedToInternet())
    {
      CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

      CStdString searchTerm;
      if (HasOption(mapOptions, "term", searchTerm))
      {
        CUtil::URLEncode(searchTerm);
        mapRemoteOptions["term"] = searchTerm;
        strBoxeeServerUrl += "/titles/search/tv";
      }
      else
      {
        strBoxeeServerUrl += "/titles/tv";
      }

      AddParametersToRemoteRequest(mapRemoteOptions);

      strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows - get remote items [url=%s] (browse)(bsd)", strBoxeeServerUrl.c_str());

      CDirectory::GetDirectory(strBoxeeServerUrl, items);

      if (items.Size() == 0)
      {
        m_cacheDirectory = DIR_CACHE_NEVER;
      }

      for (int i=0; i<items.Size(); i++)
      {
        CFileItemPtr pItem (items.Get(i));

        pItem->SetProperty("infoLoaded",true);
        pItem->SetProperty("rts-mediatype","tv_show");
        pItem->SetProperty("IsBoxeeServerItem", true);
      }
    }

    return true;
  }
  else if (strRequest == "episodes")
  {
    CStdString seriesId = mapOptions["seriesId"];

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows - episodes requested for [showId=%s] (browse)(bsd)", seriesId.c_str());

    // get details of the TV show
    CFileItemList seriesItems;
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
    strBoxeeServerUrl += "/title/tv/";
    strBoxeeServerUrl += seriesId;

    CDirectory::GetDirectory(strBoxeeServerUrl, seriesItems);

    if (seriesItems.Size() == 1)
    {
      CFileItemPtr seriesItem = seriesItems.Get(0);
      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows - got metadata for [showId=%s][label=%s] (browse)(bsd)", seriesId.c_str(), seriesItem->GetLabel().c_str());

      items.SetProperty("serieslabel", seriesItem->GetLabel());
      items.SetProperty("seriesbackground", seriesItem->GetProperty("background"));
      items.SetProperty("seriesthumb", seriesItem->GetThumbnailImage());
    }

    // always set "seriesid" for unsubscribe, etc.
    items.SetProperty("seriesid", seriesId);


    if (g_application.IsConnectedToInternet())
    {
      strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");
      strBoxeeServerUrl += "/title/";
      strBoxeeServerUrl += seriesId;
      strBoxeeServerUrl += "/episodes";

      AddParametersToRemoteRequest(mapRemoteOptions);

      strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows - server [url=%s] (browse)(add)", strBoxeeServerUrl.c_str());

      CDirectory::GetDirectory(strBoxeeServerUrl, items);

      if (items.Size() == 0)
      {
        m_cacheDirectory = DIR_CACHE_NEVER;
      }
    }

    // TODO: Season filter should be moved to the source
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

    for (int i=0; i<items.Size(); i++)
    {
      CFileItemPtr pItem (items.Get(i));

      pItem->SetProperty("infoLoaded",true);
      pItem->SetProperty("rts-mediatype","tv_show");
      pItem->SetProperty("IsBoxeeServerItem", true);
    }

    return true;
  }
  else
  {
    CLog::Log(LOGERROR, "CBoxeeServerDirectory::HandleTvShows - unrecognized request [type=%s] (browse)(bsd)", strRequest.c_str());
    return false;
  }
}

bool CBoxeeServerDirectory::HandleMovies(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  std::map<CStdString, CStdString> mapRemoteOptions = mapOptions;

  if (g_application.IsConnectedToInternet())
  {
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

    CStdString searchTerm;
    if (HasOption(mapOptions, "term", searchTerm))
    {
      CUtil::URLEncode(searchTerm);
      mapRemoteOptions["term"] = searchTerm;
      strBoxeeServerUrl += "/titles/search/movie";
    }
    else
    {
      strBoxeeServerUrl += "/titles/movie";
    }

    AddParametersToRemoteRequest(mapRemoteOptions);

    strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleMovies - server [url=%s] (browse)(add)", strBoxeeServerUrl.c_str());

    CDirectory::GetDirectory(strBoxeeServerUrl, items);

    if (items.Size() == 0)
    {
      m_cacheDirectory = DIR_CACHE_NEVER;
    }

  }

  CBoxeeSort trailerSort("trailer_label_sort",SORT_METHOD_LINK_TITLE,SORT_ORDER_ASC,"trailer_label","");

  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr pItem (items.Get(i));

    pItem->SetProperty("infoLoaded",true);
    pItem->SetProperty("rts-mediatype","movie");
    pItem->SetProperty("IsBoxeeServerItem", true);
    pItem->SortLinkList(trailerSort);
  }

  return true;

}

bool CBoxeeServerDirectory::HandleTrailers(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  std::map<CStdString, CStdString> mapRemoteOptions = mapOptions;

  if (g_application.IsConnectedToInternet())
  {
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

    CStdString searchTerm;
    if (HasOption(mapOptions, "term", searchTerm))
    {
      CUtil::URLEncode(searchTerm);
      mapRemoteOptions["term"] = searchTerm;
      strBoxeeServerUrl += "/titles/search/trailer";
    }
    else
    {
      strBoxeeServerUrl += "/titles/trailers";
    }

    AddParametersToRemoteRequest(mapRemoteOptions);

    strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTrailers - server [url=%s] (browse)(add)", strBoxeeServerUrl.c_str());

    CDirectory::GetDirectory(strBoxeeServerUrl, items);
/*
    CRssFeed loader;
    loader.Init("/home/haim/Downloads/trailers.xml","/home/haim/Downloads/trailers.xml");
    loader.ReadFeed();
    loader.GetItemList(items);
*/
    if (items.Size() == 0)
    {
      m_cacheDirectory = DIR_CACHE_NEVER;
    }

  }

  CBoxeeSort trailerSort("trailer_label_sort",SORT_METHOD_LINK_TITLE,SORT_ORDER_ASC,"trailer_label","");

  for (int i=0; i<items.Size(); i++)
  {
    CFileItemPtr pItem = items.Get(i);

    pItem->SetProperty("IsTrailer",true);
    pItem->SetProperty("infoLoaded",true);
    pItem->SetProperty("rts-mediatype","movie");
    pItem->SetProperty("IsBoxeeServerItem", true);
    pItem->SortLinkList(trailerSort);
  }

  return true;

}

bool CBoxeeServerDirectory::HandleClips(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items)
{
  std::map<CStdString, CStdString> mapRemoteOptions = mapOptions;

  if (g_application.IsConnectedToInternet())
  {
    CStdString strBoxeeServerUrl = BXConfiguration::GetInstance().GetStringParam("Boxee.Resolver.Server","http://res.boxee.tv");

    CStdString searchTerm;
    if (HasOption(mapOptions, "term", searchTerm))
    {
      CUtil::URLEncode(searchTerm);
      mapRemoteOptions["term"] = searchTerm;
      strBoxeeServerUrl += "/titles/search/clips";
    }
    else
    {
      strBoxeeServerUrl += "/titles/clip";
    }

    AddParametersToRemoteRequest(mapRemoteOptions);

    strBoxeeServerUrl += BoxeeUtils::BuildParameterString(mapRemoteOptions);

    CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleClips - server [url=%s] (browse)(add)", strBoxeeServerUrl.c_str());

    CDirectory::GetDirectory(strBoxeeServerUrl, items);

    if (items.Size() == 0)
    {
      m_cacheDirectory = DIR_CACHE_NEVER;
    }
  }

  // add the right MediaType to the ReportToServer
  for (int i=0; i<items.Size(); i++)
  {
    items.Get(i)->SetProperty("rts-mediatype","clip");
  }

  return true;

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
      pItem->SetProperty("IsTVShow",true);
      pItem->SetProperty("tvshow", true);

      items.Add(pItem);
    }
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
      CLog::Log(LOGDEBUG, "CBoxeeServerDirectory::HandleTvShows - match found [%s] (browse)(bsd)", leftItem->GetLabel().c_str());

      // Get the links from the left item and add them to the right item
      const CFileItemList* linksFileItemList = leftItem->GetLinksList();

      if (linksFileItemList)
      {
        // Copy all links from local to the remote item
        for (int j = 0; j < linksFileItemList->Size(); j++)
        {
          CFileItemPtr link = linksFileItemList->Get(j);
          rightItem->AddLink(link->GetLabel(), link->m_strPath, link->GetContentType(true), CFileItem::GetLinkBoxeeTypeAsEnum(link->GetProperty("link-boxeetype")), link->GetProperty("link-provider"), link->GetProperty("link-providername"), link->GetProperty("link-providerthumb"), link->GetProperty("link-countrycodes"), link->GetPropertyBOOL("link-countryrel"),link->GetProperty("quality-lbl"),link->GetPropertyInt("quality"), CFileItem::GetLinkBoxeeOfferAsEnum(link->GetProperty("link-boxeeoffer")), link->GetProperty("link-productslist"));
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
  else
  {
    mapRemoteOptions["adult"] = "yes";
  }

  if (g_guiSettings.GetBool("filelists.filtergeoip2"))
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

