
#include "RssSourceManager.h"

#include "FileSystem/File.h"
#include "Settings.h"
#include "Util.h"
#include "FileItem.h"
#include "bxrssreader.h"
#include "VideoInfoTag.h"
#include "../utils/SingleLock.h"
#include "lib/libBoxee/bxexceptions.h"
#include "lib/libBoxee/bxfeedreader.h"
#include "lib/libBoxee/bxfeedfactory.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "MathUtils.h"
#include "lib/libBoxee/boxee.h"

using namespace std;

#define RSS_DEFAULT_CHECK_INTERVAL_SECONDS 300

CRssFeed::CRssFeed()
{
	m_LastRead = 0;
	m_Timestamp = 0;
	m_iIntervalSeconds = RSS_DEFAULT_CHECK_INTERVAL_SECONDS;
	m_iNewItems = 0;
}

CRssFeed::~CRssFeed()
{

}

bool CRssFeed::Init(const CStdString& strURL, const CStdString& originalURL) {

  m_originalURL = originalURL;
  m_strURL = strURL;

	CLog::Log(LOGDEBUG, "Initializing feed: %s", m_strURL.c_str());
	return true;
}

void CRssFeed::CheckFeed() 
{
	// Get the feed and parse it
  BOXEE::BXXMLDocument document;
  document.GetDocument().SetConvertToUtf8(true);
  bool result = document.LoadFromURL(m_strURL);

	if (!result)
	{
		CLog::Log(LOGERROR, "%s: Check failed, could not parse feed %s", __FUNCTION__, m_strURL.c_str());
		time(&m_Timestamp);
		m_Timestamp += m_iIntervalSeconds;
		return;
	}

  BOXEE::IBXFeedReader* reader = BOXEE::BXFeedFactory::Create(document);
  if (!reader)
  {
    CLog::Log(LOGERROR, "%s: Feed does not seem to be an RSS or Atom: %s", __FUNCTION__, m_strURL.c_str());
    return;
  }
  
	reader->Parse();

	// Retreive number of items in the list
	int numOfItems = reader->GetNumOfItems();

	// Loop over all items in the feed
	for (int i = 0; i < numOfItems; i++) 
	{
		// Check if the item is newer than the last read date
		string strPubDate = reader->GetItemPubDate(i);
		//    struct tm pubDate = {0};
		//    // TODO: Handle time zone
		//    strptime(strPubDate.c_str(), "%a, %d %b %Y %H:%M:%S", &pubDate);
		//    // Check the difference between the time of last check and time of the item
		//    if (difftime(mktime(&pubDate), m_LastRead) > 0) {
		//      m_iNewItems++;
		//    }
		if (difftime(reader->ParseDateTime(strPubDate), m_LastRead) > 0) 
		{
			m_iNewItems++;
		}
	}

	// Reset the timestamp to the next time the feed should be checked
	// Update the time to the current time first TODO: Check if this is ok
	time(&m_Timestamp);
	m_Timestamp += m_iIntervalSeconds;
	
	delete reader;
}

CStdString CRssFeed::CleanDescription(const CStdString& strDescription) 
{
	CStdString description = strDescription;

  description.Replace("<br>", "[CR]");
  description.Replace("<BR>", "[CR]");
  description.Replace("<br/>", "[CR]");
  description.Replace("<BR/>", "[CR]");
  description.Replace("<i>", "[I]");
  description.Replace("<I>", "[I]");
  description.Replace("</i>", "[/I]");
  description.Replace("</I>", "[/I]");
  description.Replace("<b>", "[B]");
  description.Replace("<B>", "[B]");
  description.Replace("</b>", "[/B]");
  description.Replace("</B>", "[/B]");
  description.Replace("</p>", "[CR]");
  description.Replace("</P>", "[CR]");
  description.Replace("&nbsp;", " ");
    
	while (description.Find("<") != -1)
	{
		int start = description.Find("<");
		int end = description.Find(">");
		if (end > start)
			description.Delete(start, end-start+1);
		else
			description.Delete(start, description.GetLength() - start);
	}
	description.Trim();

	description.Replace("\\&", "&");
	description.Replace("&quot;", "\"");
	description.Replace("&amp;", "&");
	description.Replace("&nbsp;", " ");
	description.Replace("&gt;", ">");
	description.Replace("&lt;", "<");
	
	int i;
	while ((i = description.Find("&#")) >= 0)
	{
    CStdString src = "&#";
    int radix = 10;
    
	  i += 2;
	  if (description[i] == 'x' || description[i] == 'X')
	  {
	    src += description[i];
	    i++;
	    radix = 16;
	  }
	  
	  CStdString numStr;
	  unsigned long num;
	  while (description[i] != ';' && numStr.length() <= 6)
	  {
	    numStr += description[i];
	    src += description[i];
	    i++;
	  }
	  
	  // doesn't make sense....abort
	  if (numStr.length() == 6)
	  {
	    break;
	  }
	  
	  src += ';';
	  char* end;
	  num = strtoul(numStr.c_str(), &end, radix);
	  
	  // doesn't make sense....abort
	  if (num == 0)
	  {
	    break;
	  }
	  	 
	  CStdStringW utf16;
	  utf16 += (wchar_t) num;
	  CStdStringA utf8;
	  g_charsetConverter.wToUTF8(utf16, utf8);
	  description.Replace(src, utf8);
	}

	return description;
}

bool CRssFeed::IsKnownContentType(CStdString contentType)
{
  return (contentType.Left(6).Equals("video/") ||
     contentType.Left(6).Equals("audio/") ||
     contentType.Left(6).Equals("image/") ||
     contentType.Equals("application/x-boxee") ||
     contentType.Equals("application/x-shockwave-flash") ||
     contentType.Equals("application/x-vnd.movenetworks.qm") ||
     contentType.Equals("application/x-silverlight-2") ||
     contentType.Equals("application/x-silverlight"));
}


bool CRssFeed::ParseItem(BOXEE::IBXFeedReader* reader, TiXmlElement* child, CFileItemPtr item, CRssFeedParseContext& parseContext)
{
  if (!child)
  {
    CLog::Log(LOGDEBUG,"CRssFeed::ParseItem - Enter with a NULL element. Return FALSE");
    return false;
  }

  CStdString elementValue = child->ValueStr();
  if (elementValue.CompareNoCase("item") != 0)
  {
    CLog::Log(LOGDEBUG,"CRssFeed::ParseItem - Element <%s> ISN'T <item>. Return FALSE",child->ValueStr().c_str());
    return false;
  }

  bool succeeded = ParseItemElements(reader, child, item, parseContext);

  return succeeded;
}

bool CRssFeed::ParseItemElements(BOXEE::IBXFeedReader* reader, TiXmlElement* child, CFileItemPtr item, CRssFeedParseContext& parseContext)
{
  if (!child)
  {
    CLog::Log(LOGDEBUG,"CRssFeed::ParseItemElements - Enter with a NULL element. Return FALSE");
    return false;
  }

  int alternativeLinks = 0;

  for(TiXmlElement* item_child = child->FirstChildElement(); item_child; item_child = item_child->NextSiblingElement())
  {
    //printf("parse: %s\n", item_child->Value());
    if (item_child->Value() && strcmp(item_child->Value(), "title") == 0) {
      if (item_child->GetText()) {
        CStdString dirtyStrTitle = item_child->GetText();
        CStdString strTitle = CleanDescription(dirtyStrTitle);
        CUtil::UrlDecode(strTitle);
        item->SetLabel(strTitle);
      }
    }
    else if (item_child->Value() && ((reader->GetFeedType() == BOXEE::FEED_TYPE_RSS && strcmp(item_child->Value(), "pubDate") == 0) ||
        (reader->GetFeedType() == BOXEE::FEED_TYPE_ATOM && strcmp(item_child->Value(), "published") == 0)))
    {
      const char *pubDate = item_child->GetText();
      if (!pubDate)
        pubDate = ""; // hack to protect from crash
      time_t date = reader->ParseDateTime(pubDate);
      if (date > 0)
      {
         CDateTime pubDate(date);
         item->SetProperty("releasedate", pubDate.GetAsddMMMYYYYDate());
         item->m_dateTime = pubDate;
         CLog::Log(LOGDEBUG,"DATEDATE, label = %s, text = %s, release date = %s", item->GetLabel().c_str(), item_child->GetText(), item->GetProperty("releasedate").c_str());
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "link") == 0))
    {
      CStdString strLink = "";      
      if (reader->GetFeedType() == BOXEE::FEED_TYPE_RSS) 
      {
        if (item_child->GetText())
        {
          strLink = item_child->GetText();
          strLink.Trim();
        }
      }
      else if (reader->GetFeedType() == BOXEE::FEED_TYPE_ATOM && item_child->Attribute("rel") && 
          stricmp(item_child->Attribute("rel"), "alternate") == 0) 
      {
        strLink = item_child->Attribute("href");
      }
      
      if (!strLink.IsEmpty())
      {
        CRssFeedPlaybackItem playbackItem;
        
        string strPrefix = strLink.substr(0, strLink.find_first_of(":"));
        if (strPrefix == "rss") 
        {
          // If this is an rss item, we treat it as another level in the directory
          playbackItem.isFolder = true;
          playbackItem.path = strLink;
          playbackItem.priority = CRssFeedPlaybackItem::BOXEE_KNOWN;
        }
        else if (strPrefix == "flash") 
        {
          playbackItem.path = strLink;
          playbackItem.isVideo = true;
          playbackItem.isInternetStream = true;
          playbackItem.isFlash = true;
          playbackItem.priority = CRssFeedPlaybackItem::BOXEE_KNOWN;
        }
        else if (strLink.Right(4) == ".xml")
        {
          //
          // hack. will work in *some* cases - and allow an rss or rss -like structure.
          //
          playbackItem.path = "rss" + strLink.substr((strLink+":").Find(":"));
          playbackItem.isFolder = true;
          playbackItem.priority = CRssFeedPlaybackItem::BOXEE_KNOWN;
        }
        else
        {          
          playbackItem.path = strLink;  
          playbackItem.priority = CRssFeedPlaybackItem::LINK;          
          if (reader->GetFeedType() == BOXEE::FEED_TYPE_ATOM && item_child->Attribute("type"))
          {
            playbackItem.contentType = item_child->Attribute("type");
          }          
        }
        
        if (playbackItem.path != "")
        {
          parseContext.Push(playbackItem);
        }
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "enclosure") == 0))
    {
      const char * url = item_child->Attribute("url");
      const char * content_type = item_child->Attribute("type");
      CRssFeedPlaybackItem playbackItem;
      
      if (url && content_type && IsKnownContentType(content_type)) 
      {
        playbackItem.path = url;
        playbackItem.contentType = content_type;
        playbackItem.priority = CRssFeedPlaybackItem::ENCLOSURE_URL_WITH_TYPE;
      }
      else if (url && IsPathToMedia(url)) 
      {
        playbackItem.path = url;
        playbackItem.priority = CRssFeedPlaybackItem::ENCLOSURE_URL_WITH_TYPE;
      }
      
      if (playbackItem.path != "")
      {
        parseContext.Push(playbackItem);
      }      
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "media:content") == 0))
    {
      if (HasBoxeeTypeAttribute(item_child))
      {
        AddLinkItem(item_child,item);
      }

      const char * url = item_child->Attribute("url");
      const char * content_type = item_child->Attribute("type");
      CRssFeedPlaybackItem playbackItem;

      if (url && content_type && IsKnownContentType(content_type))
      {
        playbackItem.path = url;
        playbackItem.contentType = content_type;
        playbackItem.priority = CRssFeedPlaybackItem::MEDIA_CONTENT_WITH_TYPE;
      }
      else if (url && IsPathToMedia(url))
      {
        playbackItem.path = url;
        playbackItem.priority = CRssFeedPlaybackItem::MEDIA_CONTENT_WITH_TYPE;
      }
      
      // Temp fix for init IsInternetStream property for HULU item (in carusel)
      // in order to be able to successfully send recommandation for it (021208)
      if (!playbackItem.path.IsEmpty() && playbackItem.path.Left(6) == "app://")
      {
        playbackItem.isInternetStream = true;
      }

      const char * duration = item_child->Attribute("duration");
      if (duration)
      {
        long durationLong = atol(duration);
        StringUtils::SecondsToTimeString(durationLong, playbackItem.duration);
      }
        
      const char * lang = item_child->Attribute("lang");
      if (lang)
      {
        playbackItem.lang = lang;
      }
      
      if (playbackItem.path != "")
      {
        parseContext.Push(playbackItem);
      }
      
      if (!item_child->NoChildren())
        ParseItemElements(reader, item_child, item, parseContext);
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:group") == 0))
    {
      ParseItemElements(reader, item_child, item, parseContext);
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:image") == 0))
    {
      if (item_child->GetText())
      {
        item->SetThumbnailImage(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:thumbnail") == 0))
    {
      const char * url = item_child->Attribute("url");
      if (url)
      {
        item->SetThumbnailImage(url);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:image") == 0) && item_child->GetText() && IsPathToThumbnail(item_child->GetText()))
    {
      item->SetThumbnailImage(item_child->GetText());
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:image") == 0))
    {
      const char * url = item_child->Attribute("href");
      if(url)
      {
        item->SetThumbnailImage(url);
      }
    }
    else if(reader->GetFeedType() == BOXEE::FEED_TYPE_RSS && item_child->Value() && (strcmp(item_child->Value(), "description") == 0))
    {
      if(item_child->GetText()) {
        CStdString dirtyDescription = item_child->GetText();
        CStdString description = CleanDescription(dirtyDescription);
        CUtil::UrlDecode(description);
        item->SetProperty("description", description);
      }
    }
    else if (reader->GetFeedType() == BOXEE::FEED_TYPE_ATOM && item_child->Value() &&
      (strcmp(item_child->Value(), "summary") == 0 || strcmp(item_child->Value(), "content") == 0))
    {
      if(item_child->GetText()) {
        CStdString dirtyDescription = item_child->GetText();
        CStdString description = CleanDescription(dirtyDescription);
        CUtil::UrlDecode(description);
        item->SetProperty("description", description);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:summary") == 0))
    {
      if(item_child->GetText()) {
        CStdString description = item_child->GetText();
        CUtil::UrlDecode(description);
        item->SetProperty("description", description);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:subtitle") == 0))
    {
      if(item_child->GetText()) {
        CStdString description = item_child->GetText();
        CUtil::UrlDecode(description);
        item->SetProperty("description", description);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:author") == 0))
    {
      if(item_child->GetText()) {
        CStdString strAuthor = item_child->GetText();
        CUtil::UrlDecode(strAuthor);
        item->SetProperty("author", strAuthor);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:duration") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strRuntime = item_child->GetText();
        item->SetProperty("duration", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "itunes:keywords") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("keywords", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:user_agent") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("useragent", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:content_type") == 0))
    {
      if(item_child->GetText()) {
        item->SetContentType(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:runtime") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strRuntime = item_child->GetText();
        item->SetProperty("duration", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:view-count") == 0 ||
            strcmp(item_child->Value(), "zv:views") == 0))
    {
      if (item_child->GetText()) {
        item->SetProperty("viewcount", item_child->GetText());
      }
    }    
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:episode") == 0 ||
            strcmp(item_child->Value(), "zv:episode") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_iEpisode = atoi(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:season") == 0 ||
            strcmp(item_child->Value(), "zv:season") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_iSeason = atoi(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:tv-show-title") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strShowTitle = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:release-date") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("releasedate", item_child->GetText());
        item->GetVideoInfoTag()->m_strFirstAired = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:release-year") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_iYear = atoi(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:imdb-id") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strIMDBNumber = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:show-id") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("showid", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:plot") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strPlot = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:plot-summary") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strPlotOutline = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:id") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("boxeeid", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:meta-provider") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("metaprovider", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:user-rating") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_iRating = atoi(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:popularity") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_iPopularity = atoi(item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:cast") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("cast", item_child->GetText());

        SActorInfo actor;
        std::vector<CStdString> names;
        std::vector<CStdString>::iterator it;
        CUtil::Tokenize(item_child->GetText(), names, ",");
        for( it = names.begin(); it != names.end(); it++ )
        {
        	actor.strName = (*it);
            item->GetVideoInfoTag()->m_cast.push_back(actor);
        }

      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:producers") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("producers", item_child->GetText());
      }
    }
    else if(strcmp(item_child->Value(), "provider-thumb") == 0)
    {
      if(item_child->GetText()) {
        item->SetProperty("providerthumb", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:directors") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("directors", item_child->GetText());
        item->GetVideoInfoTag()->m_strDirector = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "boxee:background") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("background", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:credit") == 0))
    {
      if (item_child->GetText())
      {
        const char *role = item_child->Attribute("role");
        if (role && stricmp(role, "director") == 0)
        {
          item->GetVideoInfoTag()->m_strDirector = item_child->GetText();
        }
        else if (role && stricmp(role, "actor") == 0)
        {
          SActorInfo actor;
          actor.strName = item_child->GetText();
          item->GetVideoInfoTag()->m_cast.push_back(actor);
        }
        else if (role && stricmp(role, "writer") == 0)
        {
          item->GetVideoInfoTag()->m_strWritingCredits = item_child->GetText();
        }
        else if (role && stricmp(role, "artist") == 0)
        {
          item->GetVideoInfoTag()->m_strArtist = item_child->GetText();
        }
        else if (role == NULL || (role && stricmp(role, "author") == 0))
        {
          CStdString strAuthor = item_child->GetText();
          CUtil::UrlDecode(strAuthor);
          item->SetProperty("author", item_child->GetText());
        }
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:rating") == 0))
    {
      const char *schema = item_child->Attribute("schema"); // this is a typo but for backward compatibility...
      if (!schema) schema = item_child->Attribute("scheme");

      if (schema && stricmp(schema, "urn:user") == 0  && item_child->GetText()) {
          item->GetVideoInfoTag()->m_fRating = atof(item_child->GetText());
      }
      else if (schema && stricmp(schema, "urn:mpaa") == 0 && item_child->GetText()) {
          item->GetVideoInfoTag()->m_strMPAARating = item_child->GetText();
          if (stricmp(item_child->GetText(), "x") == 0 || stricmp(item_child->GetText(), "nc17") == 0 || stricmp(item_child->GetText(), "nc-17") == 0)
          {
            item->SetAdult(true);
          }  
      }
      else if (schema && stricmp(schema, "urn:tv") == 0  && item_child->GetText()) {
          item->GetVideoInfoTag()->m_strMPAARating = item_child->GetText();
          if (stricmp(item_child->GetText(), "tv-ma") == 0 || stricmp(item_child->GetText(), "tvma") == 0)
          {
            item->SetAdult(true);
          }  
      }
      else if (schema && stricmp(schema, "urn:v-chip") == 0  && item_child->GetText()) {
          item->GetVideoInfoTag()->m_strMPAARating = item_child->GetText();
          if (stricmp(item_child->GetText(), "tv-ma") == 0 || stricmp(item_child->GetText(), "tvma") == 0)
          {
            item->SetAdult(true);
          }  
      }
      else if ((!schema || (schema && stricmp(schema, "urn:simple") == 0)) && item_child->GetText()) {
          if (stricmp(item_child->GetText(), "adult") == 0)
          {
            item->SetAdult(true);
          }
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:copyright") == 0))
    {
      if(item_child->GetText()) {
        item->GetVideoInfoTag()->m_strStudio = item_child->GetText();
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:restriction") == 0))
    {
      const char *restrictionType = item_child->Attribute("type");
      const char *relationshipType = item_child->Attribute("relationship");
      
      if (restrictionType && stricmp(restrictionType, "country") == 0 && 
          relationshipType && item_child->GetText()) 
      {        
        item->SetCountryRestriction(item_child->GetText(), stricmp(relationshipType, "allow") == 0);
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:keywords") == 0))
    {
      if(item_child->GetText()) {
        item->SetProperty("keywords", item_child->GetText());
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:description") == 0))
    {
      if (item_child->GetText()) {
        const char* type = item_child->Attribute("type");
        if (!type || (type && strcmp(type, "plain") == 0))
        {
          item->SetProperty("description", item_child->GetText());
        }
        else if (type && strcmp(type, "html") == 0)
        {
          CStdString dirtyDescription = item_child->GetText();
          CStdString strDescription = CleanDescription(dirtyDescription);
          CUtil::UrlDecode(strDescription);
          item->SetProperty("description", strDescription);
        }
      }
      else
      {
        item->SetProperty("description", "");
      }
    }
    else if(item_child->Value() && (strcmp(item_child->Value(), "media:category") == 0))
    {
      if (item_child->GetText())
      {
        const char *scheme = item_child->Attribute("scheme");

        if(scheme && strcmp(scheme, "urn:boxee:genre") == 0) {
          item->GetVideoInfoTag()->m_strGenre = item_child->GetText();
        }
        else if(scheme && strcmp(scheme, "urn:boxee:title-type") == 0) {
          const char* type = item_child->GetText();

          if (type)
          {
            item->SetProperty("boxee-mediatype", type);
          }

          if (strcmp(type, "tv") == 0)
          {
            item->SetProperty("istvshow", true);
          }
          else if (strcmp(type, "movie") == 0)
          {
            item->SetProperty("ismovie", true);
            item->SetProperty("rts-mediatype", "movie");
          }
          else if (strcmp(type, "episode") == 0)
          {
            item->SetProperty("isepisode", true);
            item->SetProperty("rts-mediatype", "tv_show");
          }
        }
        else if(scheme && strcmp(scheme, "urn:boxee:episode") == 0) {
          item->GetVideoInfoTag()->m_iEpisode = atoi(item_child->GetText());
        }
        else if(scheme && strcmp(scheme, "urn:boxee:season") == 0) {
          item->GetVideoInfoTag()->m_iSeason = atoi(item_child->GetText());
        }
        else if(scheme && strcmp(scheme, "urn:boxee:show-title") == 0) {
          item->GetVideoInfoTag()->m_strShowTitle = item_child->GetText();
        }        
        else if(scheme && strcmp(scheme, "urn:boxee:view-count") == 0) {
          item->SetProperty("viewcount", item_child->GetText());
        }  
        else if(scheme && strcmp(scheme, "urn:boxee:source") == 0) {
          item->SetProperty("provider_source", item_child->GetText());
        }            
      }
    }
    // <boxee:property name="isvideo" type="bool">true</boxee:property>
    else if (item_child->Value() && (strcmp(item_child->Value(), "boxee:property") == 0) && item_child->GetText() && item_child->Attribute("name"))
    {
      const char *val = item_child->GetText();
      const char *type = item_child->Attribute("type");
      const char *name = item_child->Attribute("name");
      //printf("Add property: %s(%s)=%s\n", val, type, name);
      if (!type) {
        item->SetProperty(name, val);
      }
      else if (strcmp(type, "bool") == 0) {
        if (strcmp(val, "true") == 0) item->SetProperty(name, true);
        else if (strcmp(val, "false") == 0) item->SetProperty(name, false);
      }
      else if (strcmp(type, "int") == 0) {
        item->SetProperty(name, atoi(val));
      }
      else if (strcmp(type, "float") == 0) {
        item->SetProperty(name, atof(val));
      }
      else if (strcmp(type, "string") == 0) {
        item->SetProperty(name, val);
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "yt:statistics") == 0))
    {
      if (item_child->Attribute("viewCount"))
      {
        item->SetProperty("viewcount", item_child->Attribute("viewCount"));
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "gd:rating") == 0))
    {
      if (item_child->Attribute("average"))
      {
          item->GetVideoInfoTag()->m_fRating = atof(item_child->Attribute("average")) * 2;
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "boxee:viewcount") == 0))
    {
      if (item_child->GetText())
      {
        item->SetProperty("viewcount", item_child->GetText());
      }
    }
    else if (item_child->Value() && (strcmp(item_child->Value(), "boxee:alternative-link") == 0))
    {
      if (item_child->Attribute("label")) {
        CStdString propName;
        propName.Format("contextmenulabel-%d", alternativeLinks);
        item->SetProperty(propName, item_child->Attribute("label"));
      }
      if (item_child->Attribute("url")) {
        CStdString propName;
        propName.Format("contextmenuurl-%d", alternativeLinks);
        item->SetProperty(propName, item_child->Attribute("url"));
      }
      if (item_child->Attribute("thumb")) {
        CStdString propName;
        propName.Format("contextmenuthumb-%d", alternativeLinks);
        item->SetProperty(propName, item_child->Attribute("thumb"));
      }
      CStdString propName;
      propName.Format("contextmenupriority-%d", alternativeLinks);
      item->SetProperty(propName, alternativeLinks);

      ++alternativeLinks;
    }
    else if (reader->GetFeedType() == BOXEE::FEED_TYPE_ATOM && item_child->Value() && (strcmp(item_child->Value(), "author") == 0) && item_child->FirstChild("name"))
    {
      CStdString strAuthor = item_child->FirstChildElement("name")->GetText();
      CUtil::UrlDecode(strAuthor);
      item->SetProperty("author", strAuthor);
    }

  } // for item

  if (alternativeLinks > 0)
  {
    item->SetProperty("NumOfCustomButtons", alternativeLinks);
  }
  
  if (!item->GetThumbnailImage().IsEmpty())
  {
    item->SetProperty("OriginalThumb",item->GetThumbnailImage());
  }
  
  // ugly hack
  // some rss feeds have reckless spaces and new-line characters in the middle of the link
  item->m_strPath.Replace("\n","");

  return true;
}

bool CRssFeed::ReadFeed() {

  // Remove all previous items
  EnterCriticalSection(m_ItemVectorLock);
  items.Clear();
  LeaveCriticalSection(m_ItemVectorLock);
  // Reset the number of new items
  m_iNewItems = 0;

  // Get the feed and parse it
  BOXEE::BXXMLDocument document;
  document.GetDocument().SetConvertToUtf8(true);
  bool result = false;
  
  // Check whether this is a local path
  if (CUtil::IsHD(m_strURL))
  {
    CLog::Log(LOGDEBUG, "%s - RSS, Loading feed from file", __FUNCTION__);
    result = document.LoadFromFile(m_strURL);
  }
  else
  {
    CLog::Log(LOGDEBUG, "%s - RSS, Loading feed from URL: %s", __FUNCTION__, m_strURL.c_str());
    
    document.SetCredentials(BOXEE::Boxee::GetInstance().GetCredentials());
    document.SetVerbose(BOXEE::Boxee::GetInstance().IsVerbose());

    result = document.LoadFromURL(m_strURL);
  }

  if (!result) 
  {
    // log as warning. the error could also be a 404 missing title at the server.
    CLog::Log(LOGWARNING, "%s - RSS Init failed, could not parse feed %s. Reason: %s, Row: %d Col: %d", __FUNCTION__, m_strURL.c_str(), document.GetErrorDesc(), document.GetErrorRow(), document.GetErrorCol());

    // Reset the timestamp to the next time the feed should be checked
    // Update the time to the current time first
    time(&m_Timestamp);
    m_LastRead = m_Timestamp;
    m_Timestamp += m_iIntervalSeconds;
    return false;
  }

  BOXEE::IBXFeedReader* reader = BOXEE::BXFeedFactory::Create(document);
  if (!reader)
  {
    CLog::Log(LOGERROR, "%s: Feed does not seem to be an RSS or Atom: %s", __FUNCTION__, m_strURL.c_str());
    return false;
  }
  
  TiXmlElement* root = document.GetRoot();

  // Load title
  m_strTitle = reader->GetChannelTitle();

  // Load description
  m_strDescription = reader->GetChannelDesc();

  // Get channel thumbnail
  CStdString strMediaThumbnail;
  if (IsPathToThumbnail(reader->GetChannelImageLink())) {
    m_strThumbnail = reader->GetChannelImageLink();
  }
  else if (IsPathToThumbnail(reader->QueryChannelAttribute("media:content/media:thumbnail", "url"))) {
    m_strThumbnail = (reader->QueryChannelAttribute("media:content/media:thumbnail", "url"));
  }
  else if(IsPathToThumbnail(reader->QueryChannel("boxee:image"))) {
    m_strThumbnail = (reader->QueryChannel("boxee:image"));
  }
  else if(IsPathToThumbnail(reader->QueryChannel("itunes:image"))) {
    // TODO: This case is most likely irrelevant
    m_strThumbnail = (reader->QueryChannel("itunes:image"));
  }
  else if(IsPathToThumbnail(reader->QueryChannelAttribute("itunes:image", "href"))) {
    m_strThumbnail = (reader->QueryChannelAttribute("itunes:image", "href"));
  }
  else {
    // no thumbnail for channel
  }

  std::string strBGImage = reader->QueryChannel("boxee:background-image");
  if (!strBGImage.empty())
    items.SetProperty("BrowseBackgroundImage", strBGImage.c_str());
  
  std::string strCacheExp = reader->QueryChannel("boxee:expiry");
  if (!strCacheExp.empty())
    items.SetProperty("CacheExpiry", strCacheExp.c_str());
  
  // Get the feed check interval from the feed if it exists
  if (reader->QueryChannelAttribute("boxee:interval", "val") != "") {
    m_iIntervalSeconds = atoi(reader->QueryChannelAttribute("boxee:interval", "val").c_str());
  }

  m_arrCategories.push_back(reader->QueryChannelAttribute("boxee:category", "name"));


  CLog::Log(LOGDEBUG, "Channel thumbnail: %s", m_strThumbnail.c_str());

  TiXmlElement* child = NULL;
  TiXmlElement* channel = root->FirstChildElement("channel");
  if (channel == NULL)
    channel = root;

  // Try to get <boxee:display> element
  TiXmlElement* boxeeDisplayElement = NULL;
  boxeeDisplayElement = channel->FirstChildElement("boxee:display");
  if(boxeeDisplayElement)
  {
    TiXmlPrinter printer;
    printer.SetStreamPrinting();
    boxeeDisplayElement->Accept(&printer);
    items.SetProperty("ServerViewAndSort",printer.CStr());

    CLog::Log(LOGDEBUG,"CRssFeed::ReadFeed - After setting property [ServerViewAndSort=%s] (vns)",(items.GetProperty("ServerViewAndSort")).c_str());
  }
  
  // Try to get opensearch information from openSearch:startIndex, openSearch:itemsPerPage and openSearch:totalResults
  CPageContext pageContext;
  std::string startIndex = reader->QueryChannel("openSearch:startIndex");
  if (!startIndex.empty())
    pageContext.m_startIndex = atoi(startIndex.c_str());

  std::string itemsPerPage = reader->QueryChannel("openSearch:itemsPerPage");
  if (!itemsPerPage.empty())
    pageContext.m_itemsPerPage = atoi(itemsPerPage.c_str());

  std::string totalResults = reader->QueryChannel("openSearch:totalResults");
  if (!totalResults.empty())
    pageContext.m_totalResults = atoi(totalResults.c_str());
  
  CLog::Log(LOGDEBUG,"CRssFeed::ReadFeed - open search, startIndex = %d, itemsPerPage = %d, totalResults = %d (paging)",  pageContext.m_startIndex, pageContext.m_itemsPerPage, pageContext.m_totalResults);
  
  items.SetPageContext(pageContext);

  CRssFeedParseContext parseContext;
  for (child = channel->FirstChildElement(reader->GetItemTag()); child; child = child->NextSiblingElement() )
  {
    parseContext.Clear();
    
    CFileItemPtr item (new CFileItem);
    bool succeeded = ParseItem(reader, child, item, parseContext);

    if (!succeeded)
    {
      continue;
    }

    if (parseContext.Size() > 0)
    {
      const CRssFeedPlaybackItem& playbackItem = parseContext.Top();
      item->m_strPath = playbackItem.path;
      if (playbackItem.contentType.length() > 0)
      {
        item->SetContentType(playbackItem.contentType);
      }
      if (playbackItem.isVideo)
      {
        item->SetProperty("isvideo", true);
      }
      if (playbackItem.isFlash)
      {
        item->SetProperty("isflash", true);
      }
      if (playbackItem.isFolder)
      {
        item->m_bIsFolder = true;
      }
      if (playbackItem.isInternetStream)
      {
        item->SetProperty("isinternetstream", true);
      }
      if (playbackItem.duration.length() > 0)
      {
        item->SetProperty("duration", playbackItem.duration);
      }
      if (playbackItem.lang.length() > 0)
      {
        item->SetContentLang(playbackItem.lang);
      }
    }

    if (item->GetPropertyBOOL("istvshow") && item->m_bIsFolder)
    {
      item->SetProperty("istvshowfolder", true);      
    }
    
    item->SetProperty("rsschanneltitle", m_strTitle);

    CStdString strContentType = item->GetContentType(false);
    if (strContentType.IsEmpty())
    {
      item->SetContentType("");
      item->SetProperty("NeedVerify",true);
      item->SetProperty("isvideo", true);
    }    
    else if (strContentType.Left(6).Equals("video/") || 
        strContentType.Equals("text/html") ||
        strContentType.Equals("application/x-shockwave-flash") ||
        strContentType.Equals("application/x-vnd.movenetworks.qm") ||
        strContentType.Equals("application/x-silverlight-2") ||
        strContentType.Equals("application/x-silverlight"))
    {
      item->SetProperty("isvideo", true);
    }
    else if (strContentType.Left(6).Equals("audio/"))
    {
      item->SetProperty("ismusic", true);
    }
    else if (strContentType.Left(6).Equals("image/"))
    {
      item->SetProperty("ispicture", true);
    }

    if (item->GetPropertyBOOL("isvideo"))
    {
      if (item->GetVideoInfoTag()->m_strTitle.IsEmpty())
        item->GetVideoInfoTag()->m_strTitle = item->GetLabel();

      if (item->GetVideoInfoTag()->m_strPlot.IsEmpty())
        item->GetVideoInfoTag()->m_strPlot = item->GetProperty("description");

      if (item->GetVideoInfoTag()->m_strPlotOutline.IsEmpty())
        item->GetVideoInfoTag()->m_strPlotOutline = item->GetProperty("description");

      if (item->GetVideoInfoTag()->m_strRuntime.IsEmpty())
        item->GetVideoInfoTag()->m_strRuntime = item->GetProperty("duration");
    }
    
    if (item->HasVideoInfoTag())
    {
      char rating[10];
      sprintf(rating, "%d", item->GetVideoInfoTag()->m_iRating);
      item->SetProperty("videorating", rating);

      if (item->GetVideoInfoTag()->m_iEpisode > 0 || !item->GetProperty("isepisode").IsEmpty()) // hack
      {
        item->SetProperty("istvshow","1");
      }
      else
      {
        item->SetProperty("ismovie","1");
      }
    }

    item->SetProperty("isrss", "1");
    if (m_arrCategories.size() > 0) 
    {
      item->SetProperty("category", m_arrCategories[0]);
    }

    // This property indicates that item has additional info that will be presented to the user
    item->SetProperty("hasInfo", true);
    item->SetProperty("title", item->GetLabel());
    
    // If the item has relative path, prepend the host name from the original RSS
    if (item->m_strPath[0] == '/')
    {
      CURL originalURL(m_originalURL);
      if (originalURL.GetProtocol() == "http")
      {
        CStdString path = "http://";
        path += originalURL.GetHostName();
        path += item->m_strPath;
        item->m_strPath = path;
      }
    }
    
    EnterCriticalSection(m_ItemVectorLock);
    items.Add(item);
    LeaveCriticalSection(m_ItemVectorLock);

    if (item->GetThumbnailImage().GetLength() == 0)
    {
      item->SetThumbnailImage(m_strThumbnail);
    }
  }

  // Reset the timestamp to the next time the feed should be checked
  // Update the time to the current time first TODO: Check if this is ok
  time(&m_Timestamp);
  m_LastRead = m_Timestamp;
  m_Timestamp += m_iIntervalSeconds;

  if (reader)
  {
    delete reader;
  }
  
  return true;
}

bool CRssFeed::IsPathToMedia(const CStdString& strPath ) {

	CStdString extension;
	CUtil::GetExtension(strPath, extension);

	if (extension.IsEmpty())
		return false;

	extension.ToLower();

	if (g_stSettings.m_videoExtensions.Find(extension) != -1)
		return true;

	if (g_stSettings.m_musicExtensions.Find(extension) != -1)
		return true;

	if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
		return true;

	return false;
}

bool CRssFeed::IsPathToThumbnail(const CStdString& strPath ) {

	// Currently just check if this is an image, maybe we will add some
	// other checks later

	CStdString extension;
	CUtil::GetExtension(strPath, extension);

	if (extension.IsEmpty())
		return false;

	extension.ToLower();

	if (g_stSettings.m_pictureExtensions.Find(extension) != -1)
		return true;

	return false;
}

void CRssFeed::SerializeToXml(TiXmlElement *pElement) {

	TiXmlElement * feedElement = new TiXmlElement("rssfeed");

	feedElement->SetAttribute("newItems", m_iNewItems);
	feedElement->SetAttribute("intervalSeconds", m_iIntervalSeconds);
	feedElement->SetAttribute("lastRead", (int)m_LastRead);

	pElement->LinkEndChild(feedElement);

	TiXmlElement * titleElement = new TiXmlElement("title");
	titleElement->LinkEndChild(new TiXmlText(m_strTitle));
	feedElement->LinkEndChild(titleElement);

	TiXmlElement * descriptionElement = new TiXmlElement("description");
	descriptionElement->LinkEndChild(new TiXmlText(m_strDescription));
	feedElement->LinkEndChild(descriptionElement);

	TiXmlElement * thumbnailElement = new TiXmlElement("thumbnail");
	thumbnailElement->LinkEndChild(new TiXmlText(m_strThumbnail));
	feedElement->LinkEndChild(thumbnailElement);

	TiXmlElement * urlElement = new TiXmlElement("url");
	urlElement->LinkEndChild(new TiXmlText(m_strURL));
	feedElement->LinkEndChild(urlElement);

}

void CRssFeed::LoadFromXml(TiXmlElement *pElement) {
	CStdString strValue;
	if (pElement)
	{
		strValue = pElement->Value();
	}
	if (strValue != "rssfeed")
	{
		CLog::Log(LOGERROR, "%s rssfeeds.xml file does not contain <rssfeeds>", __FUNCTION__);
	}

	TiXmlElement* titleElement = pElement->FirstChildElement("title");
	m_strTitle = titleElement->GetText();

	TiXmlElement* descriptionElement = pElement->FirstChildElement("description");
	m_strDescription = descriptionElement->GetText();

	TiXmlElement* thumbnailElement = pElement->FirstChildElement("thumbnail");
	m_strThumbnail = thumbnailElement->GetText();

	TiXmlElement* urlElement = pElement->FirstChildElement("url");
	m_strURL = urlElement->GetText();

	pElement->QueryIntAttribute("newItems", &m_iNewItems);
	pElement->QueryIntAttribute("intervalSeconds", &m_iIntervalSeconds);
	pElement->QueryIntAttribute("lastRead", (int*)&m_LastRead);
}

bool CRssFeed::HasBoxeeTypeAttribute(const TiXmlElement* item_child)
{
  if (item_child->Attribute("boxee:type"))
  {
    return true;
  }

  return false;
}

void CRssFeed::AddLinkItem(const TiXmlElement* item_child, CFileItemPtr item)
{
  CStdString stUrl = "";
  CStdString stBoxeeType = "";
  CStdString stType = "";
  CStdString stProvider = "";
  CStdString stProviderName = "";
  CStdString stProviderThumb = "";
  CStdString stTitle = "";
  CStdString stCountryAllow = "";
  CStdString strQualityLabel = "";
  bool bCountryRel = true;
  int  quality = 0;

  const char* title = item_child->Attribute("boxee:title");
  if (title)
  {
    stTitle = title;
  }

  const char* url = item_child->Attribute("url");
  if (url)
  {
    stUrl = url;
  }

  const char* type = item_child->Attribute("type");
  if (type)
  {
    stType = type;
  }

  const char* boxeeType = item_child->Attribute("boxee:type");
  if (boxeeType)
  {
    stBoxeeType = boxeeType;
  }

  const char* provider = item_child->Attribute("boxee:provider");
  if (provider)
  {
    stProvider = provider;
  }

  const char* providerName = item_child->Attribute("boxee:provider-name");
  if (providerName)
  {
    stProviderName = providerName;
  }

  const char* providerThumb = item_child->Attribute("boxee:provider-thumb");
  if (providerThumb)
  {
    stProviderThumb = providerThumb;
  }

  const char* countryAllow = item_child->Attribute("boxee:country-allow");
  if (countryAllow)
  {
    stCountryAllow = countryAllow;
    bCountryRel = true;
  }
  else
  {
    // no "boxee:country-allow" -> all countries ([country-allow=""][country-rel=FALSE])
    bCountryRel = false;
  }

  const char* qualityLabel = item_child->Attribute("boxee:quality-lbl");
  if (qualityLabel)
  {
    strQualityLabel = qualityLabel;
  }
  else
  {
    // no "boxee:quality-lbl" -> use default 480p
    strQualityLabel = "";
  }
  //in case that the quality attribute doesnt it will be 0
  item_child->Attribute((const char *)("boxee:quality"),&quality);


  // NOTE: countries in "boxee:country-allow" are the allow countries, so TRUE will be send in AddLink()
  item->AddLink(stTitle, stUrl, stType, CFileItem::GetLinkBoxeeTypeAsEnum(stBoxeeType), stProvider, stProviderName, stProviderThumb, stCountryAllow, bCountryRel, strQualityLabel,quality);
}

bool RssFeedSortByTimestamp(const CRssFeed* lhs, const CRssFeed* rhs)
{
	return lhs->GetTimestamp() < rhs->GetTimestamp();
}

CRssSourceManager::CRssSourceManager()
{
	m_WakeEvent = CreateEvent(NULL, FALSE /*manual*/, TRUE /*initial state*/, NULL);
}

CRssSourceManager::~CRssSourceManager()
{
	CSingleLock lock(m_FeedVectorLock);
	for (unsigned int i = 0; i < m_vecFeeds.size(); i++) {
		delete m_vecFeeds[i];
	}
	CloseHandle(m_WakeEvent);
}



CRssFeed* CRssSourceManager::AddFeed(const CStdString& strFeedURL) {

	CLog::Log(LOGINFO, "Trying to add RSS Feed: %s", strFeedURL.c_str());
	// Add the feed only if the feed with the same URL does not
	// already exist
	EnterCriticalSection(m_FeedVectorLock);
	for (unsigned int i=0; i < m_vecFeeds.size(); i++) {
		if (m_vecFeeds[i]->GetUrl() == strFeedURL)
		{
			CLog::Log(LOGINFO, "Found existing feed: %s", strFeedURL.c_str());
			LeaveCriticalSection(m_FeedVectorLock);
			return m_vecFeeds[i];
		}
	}

	LeaveCriticalSection(m_FeedVectorLock);

	CRssFeed* pFeed = new CRssFeed();
	pFeed->Init(strFeedURL, strFeedURL);

	CLog::Log(LOGINFO, "Created new feed: %s", strFeedURL.c_str());
	AddFeed(pFeed);

	Save();

	return pFeed;
}

void CRssSourceManager::AddFeed(CRssFeed* pFeed) {

	// Get the feed period from the feed and calculate the next time
	// it should be checked
	time_t now;
	time(&now);

	pFeed->SetTimestamp(now + pFeed->GetIntervalSeconds());

	// Add the new feed to the list
	EnterCriticalSection(m_FeedVectorLock);
	m_vecFeeds.push_back(pFeed);
	LeaveCriticalSection(m_FeedVectorLock);

	// Update the feed immediately by waking the thread
	SetEvent(m_WakeEvent);
}

bool CRssSourceManager::Save()
{
	TiXmlDocument doc;
	TiXmlDeclaration * decl = new TiXmlDeclaration( "1.0", "", "" );
	TiXmlElement * rootElement = new TiXmlElement( "rssfeeds" );

	CRssFeed* feed;
	EnterCriticalSection(m_FeedVectorLock);
	for (unsigned int i = 0; i < m_vecFeeds.size(); i++) {
		feed = m_vecFeeds[i];
		feed->SerializeToXml(rootElement);
	}
	LeaveCriticalSection(m_FeedVectorLock);

	doc.LinkEndChild( decl );
	doc.LinkEndChild( rootElement );

	CStdString strRssFeedsFilePath;
	CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"rssfeeds.xml",strRssFeedsFilePath);

	CLog::Log(LOGINFO, "Saving feeds to: %s", strRssFeedsFilePath.c_str());
	doc.SaveFile(strRssFeedsFilePath);
	CLog::Log(LOGINFO, "Feeds saved to: %s", strRssFeedsFilePath.c_str());

	return true;
}

bool CRssSourceManager::Init()
{

	CStdString strRssFeedsFilePath;

	CUtil::AddFileToFolder(g_settings.GetUserDataFolder(),"rssfeeds.xml",strRssFeedsFilePath);

	// Open the rssfeeds.xml file from the user space
	if (!XFILE::CFile::Exists(strRssFeedsFilePath))
	{
		// If the file does not exist the initialization can not continue
		CLog::Log(LOGWARNING, "%s rssfeeds.xml does not exist, RSS ticker will be disabled.", __FUNCTION__);
		return false;
	}

	TiXmlDocument xmlDoc;
	TiXmlElement *pRootElement= NULL;
	if (xmlDoc.LoadFile(strRssFeedsFilePath.c_str()))
	{
		pRootElement = xmlDoc.RootElement();
		CStdString strValue;
		if (pRootElement)
		{
			strValue = pRootElement->Value();
		}
		else
		{
		  CLog::Log(LOGERROR, "%s rssfeeds.xml root element is NULL", __FUNCTION__);
      return false;
		}

		if (strValue != "rssfeeds")
		{
			CLog::Log(LOGERROR, "%s rssfeeds.xml file does not contain <rssfeeds>", __FUNCTION__);
			return false;
		}
	}
	else
	{
		CLog::Log(LOGERROR, "%s Error loading %s: Line %d, %s", __FUNCTION__,
				strRssFeedsFilePath.c_str(), xmlDoc.ErrorRow(), xmlDoc.ErrorDesc());
		return false;
	}

	// Load feeds
	if (pRootElement)
	{
		// Go over all elements in the widget
		TiXmlNode * pChild;
		for ( pChild = pRootElement->FirstChild(); pChild != NULL; pChild = pChild->NextSibling())
		{
			int type = pChild->Type();

			switch ( type )
			{
			case TiXmlNode::ELEMENT: {
				string name = pChild->Value();
				if (name == "rssfeed") {
					CRssFeed* feed = new CRssFeed();
					feed->LoadFromXml((TiXmlElement*)pChild);
					EnterCriticalSection(m_FeedVectorLock);
					m_vecFeeds.push_back(feed);
					LeaveCriticalSection(m_FeedVectorLock);
				}
			}
			default:
				break;
			}
		}
	}

	// Start the thread

	//---> currently disabled!
	//CThread::Create(false, THREAD_MINSTACKSIZE);

	return true;
}

bool CRssSourceManager::Stop() {
  // Save is temporarily diabled, should be enabled if new item notifications are used
	//Save();
	SetEvent(m_WakeEvent);
	StopThread();
	CLog::Log(LOGINFO, "%s CRssSourceManager STOPPED !!!", __FUNCTION__);
	return true;
}

void CRssSourceManager::Process()
{
	time_t now;

	while (!m_bStop)
	{
		CLog::Log(LOGDEBUG, "Processing RSS Feeds");
		time(&now);

		// TODO: Consider rewriting the checking mechanism to work without sort

		time_t nextTime = now + RSS_DEFAULT_CHECK_INTERVAL_SECONDS;

		EnterCriticalSection(m_FeedVectorLock);
		for (unsigned int i = 0; i < m_vecFeeds.size() && !m_bStop; i++)
		{
			// If the timestamp of the feed is less than the current time, check it
			if (m_vecFeeds[i]->GetTimestamp()  < now) {
				CRssFeed* feed = m_vecFeeds[i];
				LeaveCriticalSection(m_FeedVectorLock);
				feed->CheckFeed();
				EnterCriticalSection(m_FeedVectorLock);
			}
		}
		LeaveCriticalSection(m_FeedVectorLock);

		EnterCriticalSection(m_FeedVectorLock);
		// Sort the vector according to timestamp
		if (m_vecFeeds.size() > 0) {
			std::sort(m_vecFeeds.begin(), m_vecFeeds.end(), RssFeedSortByTimestamp);
			if (m_vecFeeds[0]->GetTimestamp() > now)
			{
				nextTime = m_vecFeeds[0]->GetTimestamp();
			}
			else
			{
				nextTime = now;
			}
		}
		LeaveCriticalSection(m_FeedVectorLock);

		// Sleep until the next time
		CLog::Log(LOGDEBUG, "RSS Source Manager going to sleep for %ld seconds", nextTime - now);
		WaitForSingleObject(m_WakeEvent, (nextTime - now)* 1000 /*milliseconds*/);

	} // while
}

CRssFeedParseContext::CRssFeedParseContext()
{
  Clear();
}

void CRssFeedParseContext::Clear()
{
  while (!m_queue.empty())
  {
    m_queue.pop();
  }
}

void CRssFeedParseContext::Push(const CRssFeedPlaybackItem& item)
{
  m_queue.push(item);
}

const CRssFeedPlaybackItem& CRssFeedParseContext::Top()
{
  return m_queue.top();
}

size_t CRssFeedParseContext::Size()
{
  return m_queue.size();
}
