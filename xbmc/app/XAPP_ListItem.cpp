#include "XAPP_ListItem.h"
#include "Util.h"
#include "Settings.h"
#include "FileItem.h"
#include "Tag.h"
#include "VideoInfoTag.h"
#include "StringUtils.h"
#include "AppManager.h"

static const char* MEDIA_TYPE_STRINGS[] =
{ 
  "MEDIA_UNKNOWN",
  "MEDIA_AUDIO_MUSIC", 
  "MEDIA_AUDIO_SPEECH",
  "MEDIA_AUDIO_RADIO",
  "MEDIA_AUDIO_OTHER", 
  "MEDIA_VIDEO_MUSIC_VIDEO", 
  "MEDIA_VIDEO_FEATURE_FILM", 
  "MEDIA_VIDEO_TRAILER", 
  "MEDIA_VIDEO_EPISODE", 
  "MEDIA_VIDEO_CLIP", 
  "MEDIA_VIDEO_OTHER",
  "MEDIA_PICTURE", 
  "MEDIA_FILE",
};
  
namespace XAPP
{

ListItem::ListItem(XAPP::ListItem::MediaType mediaType)
{
  m_fileItem.reset(new CFileItem());
  SetMediaType(mediaType);

  // set app id
  const char *strAppId = CAppManager::GetInstance().GetCurrentContextAppId();
  if (strAppId)
  {
    m_fileItem->SetProperty("appid", strAppId);
  }
}

ListItem::ListItem(CFileItemPtr item)
{
  m_fileItem = item;

  if (m_fileItem.get() == NULL)
    m_fileItem.reset(new CFileItem());

  const char *strAppId = CAppManager::GetInstance().GetCurrentContextAppId();
  if (strAppId)
  {
    m_fileItem->SetProperty("appid", strAppId);
  }
}

void ListItem::SetMediaType(XAPP::ListItem::MediaType mediaType)
{
  if (!m_fileItem)
    return;
  
  m_fileItem->SetProperty("media-type", mediaType);
  m_fileItem->SetProperty("media-type-str", MEDIA_TYPE_STRINGS[mediaType]);
  
  if (mediaType == XAPP::ListItem::MEDIA_AUDIO_MUSIC || 
      mediaType == XAPP::ListItem::MEDIA_AUDIO_SPEECH ||  
      mediaType == XAPP::ListItem::MEDIA_AUDIO_RADIO ||  
      mediaType == XAPP::ListItem::MEDIA_AUDIO_OTHER)
  {
    m_fileItem->SetProperty("ismusic", true);
    
    if (mediaType == XAPP::ListItem::MEDIA_AUDIO_RADIO)
    {
      m_fileItem->SetProperty("isradio", true);
    } 
  }
  else if (mediaType == XAPP::ListItem::MEDIA_VIDEO_MUSIC_VIDEO || 
      mediaType == XAPP::ListItem::MEDIA_VIDEO_FEATURE_FILM ||  
      mediaType == XAPP::ListItem::MEDIA_VIDEO_EPISODE ||  
      mediaType == XAPP::ListItem::MEDIA_VIDEO_CLIP ||  
      mediaType == XAPP::ListItem::MEDIA_VIDEO_OTHER ||  
      mediaType == XAPP::ListItem::MEDIA_VIDEO_TRAILER)
  {
    m_fileItem->SetProperty("isvideo", true);

    if (mediaType == XAPP::ListItem::MEDIA_VIDEO_FEATURE_FILM)
    {
      m_fileItem->SetProperty("ismovie", true);
    }
    
    if (mediaType == XAPP::ListItem::MEDIA_VIDEO_EPISODE)
    {
      m_fileItem->SetProperty("istvshow", true);        
    }

    if (mediaType == XAPP::ListItem::MEDIA_VIDEO_TRAILER)
    {
      m_fileItem->SetProperty("istrailer", true);        
    }    
  }
  else if (mediaType == XAPP::ListItem::MEDIA_PICTURE)
  {
    m_fileItem->SetProperty("ispicture", true);
  }
}

void ListItem::SetLabel(const std::string& label)
{
  if (m_fileItem)
  m_fileItem->SetLabel(label);
}

void ListItem::SetLabel2(const std::string& label2)
{
  if (m_fileItem)
  m_fileItem->SetLabel2(label2);
}

void ListItem::SetContentType(const std::string& contentType)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetContentType(contentType);  
  
  CStdString strContentType = contentType;
  if (strContentType.Left(6).Equals("video/") || 
      strContentType.Equals("application/x-shockwave-flash") ||
      strContentType.Equals("application/x-vnd.movenetworks.qm") ||
      strContentType.Equals("application/x-silverlight-2") ||
      strContentType.Equals("application/x-silverlight") )
  {
    m_fileItem->SetProperty("isvideo", true);
  }
  else if (strContentType.Left(6).Equals("audio/"))
  {
    m_fileItem->SetProperty("ismusic", true);
  }
  else if (strContentType.Left(6).Equals("image/"))
  {
    m_fileItem->SetProperty("ispicture", true);
  }  
}

void ListItem::SetPath(const std::string& path)
{
  if (!m_fileItem)
    return;

  m_fileItem->m_strPath = path;
  
  std::string strPrefix = path.substr(0, path.find_first_of(":"));
  if (strPrefix == "rss") 
  {
    // If this is an rss item, we treat it as another level in the directory
    m_fileItem->m_bIsFolder = true;
  }
  else if (strPrefix == "flash") 
  {
    m_fileItem->SetProperty("isvideo", true);
    m_fileItem->SetProperty("isinternetstream", true);
    m_fileItem->SetProperty("isflash", true);
  }
  else if (strPrefix == "http" || strPrefix == "mms")
  {
    m_fileItem->SetProperty("isinternetstream", true);    
  }

  // If no content type was set, try to guess whether it's video/audio/picture based on extension
  if (m_fileItem->GetContentType(false).size() == 0)
  {
    CStdString extension;
    CUtil::GetExtension(path, extension);

    if (extension.size() > 0 && g_stSettings.m_videoExtensions.Find(extension) != -1)
    {
      m_fileItem->SetProperty("isvideo", true);
    }  
    else if (extension.size() > 0 && g_stSettings.m_musicExtensions.Find(extension) != -1)
    {
      m_fileItem->SetProperty("ismusic", true);
    }
    else if (extension.size() > 0 && g_stSettings.m_pictureExtensions.Find(extension) != -1)
    {
      m_fileItem->SetProperty("ispicture", true);
    }
  }
}

void ListItem::SetTitle(const std::string& title)
{
  if (!m_fileItem)
    return;

  m_fileItem->m_strTitle = title;
  
  if (m_fileItem->HasProperty("ismusic"))
  {
    m_fileItem->GetMusicInfoTag()->SetTitle(title);
  }
  else if (m_fileItem->HasProperty("isvideo"))
  {
    m_fileItem->GetVideoInfoTag()->m_strTitle = title;
  }    
}

void ListItem::SetThumbnail(const std::string& thumbnail)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetThumbnailImage(thumbnail);
}

void ListItem::SetIcon(const std::string& icon)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetIconImage(icon);
}

void ListItem::SetTrackNumber(const int trackNumber)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetMusicInfoTag()->SetTrackNumber(trackNumber);  
  m_fileItem->SetProperty("hasInfo", true); 
}

void ListItem::SetArtist(const std::string& artist)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetMusicInfoTag()->SetArtist(artist);  
  m_fileItem->SetProperty("hasInfo", true); 
}

void ListItem::SetAlbum(const std::string& album)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetMusicInfoTag()->SetAlbum(album);
  m_fileItem->SetProperty("hasInfo", true); 
}

void ListItem::SetYear(int year)
{
  if (!m_fileItem)
    return;

  if (m_fileItem->HasProperty("ismusic"))
  {  
    m_fileItem->GetMusicInfoTag()->SetYear(year);
    m_fileItem->SetProperty("hasInfo", true); 
  } 
  else if (m_fileItem->HasProperty("isvideo"))
  {  
    m_fileItem->GetVideoInfoTag()->m_iYear = year;
    m_fileItem->SetProperty("hasInfo", true); 
  }
}

void ListItem::SetGenre(const std::string& genre)
{
  if (!m_fileItem)
    return;

  if (m_fileItem->HasProperty("ismusic"))
  {  
    m_fileItem->GetMusicInfoTag()->SetGenre(genre);
    m_fileItem->SetProperty("hasInfo", true); 
  }
  else if (m_fileItem->HasProperty("isvideo"))
  {
    m_fileItem->GetVideoInfoTag()->m_strGenre = genre;
    m_fileItem->SetProperty("hasInfo", true); 
  }
}

void ListItem::SetDirector(const std::string& director)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strDirector = director;
  m_fileItem->SetProperty("hasInfo", true); 
}

void ListItem::SetSize(long long size)
{
  if (!m_fileItem)
    return;

  m_fileItem->m_dwSize = size;
}

void ListItem::SetDuration(int seconds)
{
  if (!m_fileItem)
    return;

  if (m_fileItem->HasProperty("ismusic"))
  {  
    m_fileItem->GetMusicInfoTag()->SetDuration(seconds);
    m_fileItem->SetProperty("hasInfo", true);     
  }
  else if (m_fileItem->HasProperty("isvideo"))
  {
    m_fileItem->GetVideoInfoTag()->m_strRuntime.Format("%d", seconds);
    m_fileItem->SetProperty("hasInfo", true);     
  }  
}

void ListItem::SetStarRating(float rating)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_fRating = rating;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetViewCount(int viewCount)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("viewcount", viewCount);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetContentRating(const std::string& rating)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strMPAARating = rating;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetDescription(const std::string& description, bool isHTML)
{
  if (!m_fileItem)
    return;

  CStdString desriptionStr = description;
  
  desriptionStr.Replace("\\r", "");
  desriptionStr.Replace("\r", "");
  desriptionStr.Replace("\\n", "[CR]");
  desriptionStr.Replace("\n", "[CR]");

  if (isHTML)
  {
    desriptionStr.Replace("<br>", "[CR]");
    desriptionStr.Replace("<BR>", "[CR]");
    desriptionStr.Replace("<br/>", "[CR]");
    desriptionStr.Replace("<BR/>", "[CR]");    
    desriptionStr.Replace("<i>", "[I]");
    desriptionStr.Replace("<I>", "[I]");
    desriptionStr.Replace("</i>", "[/I]");
    desriptionStr.Replace("</I>", "[/I]");
    desriptionStr.Replace("<b>", "[B]");
    desriptionStr.Replace("<B>", "[B]");
    desriptionStr.Replace("</b>", "[/B]");
    desriptionStr.Replace("</B>", "[/B]");
    desriptionStr.Replace("</p>", "[CR]");
    desriptionStr.Replace("</P>", "[CR]");
    desriptionStr.Replace("&nbsp;", " ");
    
    // Remove any remaining HTML tags
    while (desriptionStr.Find("<") != -1)
    {
      int start = desriptionStr.Find("<");
      int end = desriptionStr.Find(">");
      if (end > start)
        desriptionStr.Delete(start, end-start+1);
      else
        desriptionStr.Delete(start, desriptionStr.GetLength() - start);
    }
    
    // Replace HTML escaped stuff 
    desriptionStr.Replace("&quot;", "\"");
    desriptionStr.Replace("&amp;", "&");
    desriptionStr.Replace("&nbsp;", " ");
    desriptionStr.Replace("&gt;", ">");
    desriptionStr.Replace("&lt;", "<");  
    
    desriptionStr.Trim();    
  }
  
  m_fileItem->SetProperty("description", desriptionStr);
  m_fileItem->SetProperty("hasInfo", true);   
  
  if (m_fileItem->HasProperty("isvideo"))
  {
    m_fileItem->GetVideoInfoTag()->m_strPlot = description;
  }
}

void ListItem::SetEpisode(int episode)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_iEpisode = episode;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetSeason(int season)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_iSeason = season;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetTVShowTitle(const std::string& title)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strShowTitle = title;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetComment(const std::string& comment)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetMusicInfoTag()->SetComment(comment);
}

void ListItem::SetDate(int year, int month, int day)
{
  if (!m_fileItem)
    return;

  CStdString dateStr;
  dateStr.Format("%04i-%02i-%02i", year, month, day);
  m_fileItem->SetProperty("releasedate", dateStr);
  m_fileItem->m_dateTime = CDateTime(year, month, day, 0, 0, 0);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetStudio(const std::string& studio)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strStudio = studio;
  m_fileItem->SetProperty("hasInfo", true); 
}

 void ListItem::Select(bool on)
 {
   if (!m_fileItem)
     return;

   m_fileItem->Select(on);
 }

void ListItem::SetAuthor(const std::string& author)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("author", author);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::AddCastAndRole(const std::string& name, const std::string& role)
{
  if (!m_fileItem)
    return;

  SActorInfo actor;
  actor.strName = name;
  actor.strRole = role;
  m_fileItem->GetVideoInfoTag()->m_cast.push_back(actor);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::AddCast(const std::string& name)
{
  if (!m_fileItem)
    return;

  SActorInfo actor;
  actor.strName = name;
  m_fileItem->GetVideoInfoTag()->m_cast.push_back(actor);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::ClearCastAndRole()
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_cast.clear();
}

void ListItem::SetWriter(const std::string& writer)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strWritingCredits = writer;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetTagLine(const std::string& tagLine)
{
  if (!m_fileItem)
    return;

  m_fileItem->GetVideoInfoTag()->m_strTagLine = tagLine;
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetProviderSource(const std::string& provider)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("provider_source", provider);  
}

void ListItem::SetKeywords(const std::string& keywords)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("keywords", keywords);
  m_fileItem->SetProperty("hasInfo", true);   
}

void ListItem::SetImage(int id, const std::string& url)
{
  if (!m_fileItem)
    return;

  CStdString propName;
  propName.Format("Image%d", id);
  m_fileItem->SetProperty(propName, url);  
}

void ListItem::AddAlternativePath(const std::string& label, const std::string& path, const std::string& contentType, const std::string& thumbUrl)
{
  if (!m_fileItem)
    return;

  bool found = false;
  int availableId = 0;
  CStdString propName;

  while (!found)
  {
    propName.Format("contextmenulabel-%d", availableId);

    if (m_fileItem->HasProperty(propName))
    {
      availableId++;
    }
    else
    {
      found = true; 
    }
  }
  
  propName.Format("contextmenulabel-%d", availableId);
  m_fileItem->SetProperty(propName, label);
  
  propName.Format("contextmenuurl-%d", availableId);
  m_fileItem->SetProperty(propName, path);

  propName.Format("contextmenuthumb-%d", availableId);
  m_fileItem->SetProperty(propName, thumbUrl);

  propName.Format("contextmenucontenttype-%d", availableId);
  m_fileItem->SetProperty(propName, contentType);
  
  propName.Format("contextmenupriority-%d", availableId);
  m_fileItem->SetProperty(propName, availableId);

  m_fileItem->SetProperty("NumOfCustomButtons", availableId + 1);
}

void ListItem::SetProperty(const std::string& key, const std::string& value)
{
  if (!m_fileItem)
    return;

  std::string customKey = "custom:";
  customKey += key;
  m_fileItem->SetProperty(customKey, value);
}

void ListItem::SetArbitratyProperty(const std::string& key, void* value)
{
  if (!m_fileItem)
    return;

  std::string customKey = "custom:";
  customKey += key;
  m_fileItem->SetProperty(customKey, (unsigned long) value);
}

void* ListItem::GetArbitratyProperty(const std::string& key)
{
  if (!m_fileItem)
    return NULL;

  std::string customKey = "custom:";
  customKey += key;
  return (void*) m_fileItem->GetPropertyULong(customKey);
}

std::string ListItem::GetLabel() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetLabel();
}

std::string ListItem::GetContentType() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetContentType();
}

std::string ListItem::GetPath() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->m_strPath;
}

std::string ListItem::GetTitle() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->m_strTitle;
}

std::string ListItem::GetThumbnail() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  if (m_fileItem->HasProperty("OriginalThumb"))
    return m_fileItem->GetProperty("OriginalThumb");
  else
    return m_fileItem->GetThumbnailImage();
}

std::string ListItem::GetIcon() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetIconImage();
}

int ListItem::GetTrackNumber() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetMusicInfoTag()->GetTrackNumber();
}

std::string ListItem::GetArtist() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  if (m_fileItem->HasMusicInfoTag())
  return m_fileItem->GetMusicInfoTag()->GetArtist();  
  if (m_fileItem->HasVideoInfoTag())
    return m_fileItem->GetVideoInfoTag()->m_strArtist;

  return StringUtils::EmptyString;
}

std::string ListItem::GetAlbum() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetMusicInfoTag()->GetAlbum();
}

int ListItem::GetYear() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetVideoInfoTag()->m_iYear;
}

std::string ListItem::GetDate()
{
  if (m_fileItem && m_fileItem->m_dateTime.IsValid())
  {
    return m_fileItem->m_dateTime.GetAsLocalizedDate();
  }
  
    return StringUtils::EmptyString;
  }

std::string ListItem::GetGenre() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetVideoInfoTag()->m_strGenre;
}

std::string ListItem::GetDirector() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetVideoInfoTag()->m_strDirector;
}

int ListItem::GetSize() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->m_dwSize;
}

std::string ListItem::GetSizeFormatted() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return StringUtils::SizeToString(m_fileItem->m_dwSize);
}

int ListItem::GetDuration() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetMusicInfoTag()->GetDuration();
}

std::string ListItem::GetDurationFormatted() const
{
  CStdString result;
  StringUtils::SecondsToTimeString(GetDuration(), result);
  return result;
}

float ListItem::GetStarRating() const
{
  if (!m_fileItem)
    return .0;

  return m_fileItem->GetVideoInfoTag()->m_fRating;
}

int ListItem::GetViewCount() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetPropertyInt("viewcount");
}

std::string ListItem::GetViewCountFormatted() const
{
  return StringUtils::FormatNumber(GetViewCount());
}

std::string ListItem::GetContentRating() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetVideoInfoTag()->m_strMPAARating;
}

std::string ListItem::GetDescription() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetProperty("description");
}

int ListItem::GetEpisode() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetVideoInfoTag()->m_iEpisode;
}

int ListItem::GetSeason() const
{
  if (!m_fileItem)
    return 0;

  return m_fileItem->GetVideoInfoTag()->m_iSeason;
}

std::string ListItem::GetTVShowTitle() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  return m_fileItem->GetVideoInfoTag()->m_strShowTitle;
}

std::string ListItem::GetComment() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetMusicInfoTag()->GetComment();
}

std::string ListItem::GetStudio() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetVideoInfoTag()->m_strStudio;
}

std::string ListItem::GetAuthor() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetProperty("author"); 
}

std::string ListItem::GetCastAndRole() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetVideoInfoTag()->GetCast(true);
}

std::string ListItem::GetCast() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetVideoInfoTag()->GetCast();
}

std::string ListItem::GetWriter() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetVideoInfoTag()->m_strWritingCredits;
}

std::string ListItem::GetTagLine() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetVideoInfoTag()->m_strTagLine;
}

std::string ListItem::GetProviderSource() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetProperty("provider_source");
}

std::string ListItem::GetKeywords() const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;
  return m_fileItem->GetProperty("keywords");
}

bool ListItem::HasProperty(const std::string& key) const
{
  if (!m_fileItem)
    return false;
  std::string customKey = "custom:";
  customKey += key;
  return m_fileItem->HasProperty(customKey);
}

std::string ListItem::GetProperty(const std::string& key) const
{ 
  if (!m_fileItem)
    return StringUtils::EmptyString;

  std::string customKey = "custom:";
  customKey += key;
  return m_fileItem->GetProperty(customKey);
}

CFileItemPtr ListItem::GetFileItem()
{
  return m_fileItem;
}

void ListItem::Dump()
{
  if (m_fileItem)
  m_fileItem->Dump();
}

bool ListItem::GetReportToServer()
{
  if (!m_fileItem)
    return false;
  return !m_fileItem->GetPropertyBOOL("dont-report");
}
  
void ListItem::SetReportToServer(bool reportToServer)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("dont-report", !reportToServer);
}

bool ListItem::GetResumable()
{
  if (!m_fileItem)
    return false;

  return m_fileItem->GetPropertyBOOL("resumable");
}
  
void ListItem::SetResumable(bool resumable)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("resumable", resumable);
}

bool ListItem::GetWatched()
{
  if (!m_fileItem)
    return false;

  return m_fileItem->GetPropertyBOOL("watched");
}

void ListItem::SetAddToHistory(bool addToHistory)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("add-to-history", addToHistory);
}

void ListItem::SetEnableRecommend(bool enabled)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("disable-recommend", !enabled);
}

void ListItem::SetEnableRate(bool enabled)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetProperty("disable-rate", !enabled);
}

void ListItem::SetExternalItem(ListItem externalItem)
{
  if (!m_fileItem)
    return;

  m_fileItem->SetExternalFileItem(externalItem.GetFileItem());
}
  
XAPP::ListItem::MediaType ListItem::GetMediaType() const
{
  if (!m_fileItem)
    return XAPP::ListItem::MEDIA_UNKNOWN;

  if (m_fileItem->HasProperty("media-type"))
  {
    return (XAPP::ListItem::MediaType) m_fileItem->GetPropertyInt("media-type");
  }
  else if (m_fileItem->HasProperty("isvideo"))
  {
    return XAPP::ListItem::MEDIA_VIDEO_OTHER;
  }
  else if (m_fileItem->HasProperty("ismusic"))
  {
    return XAPP::ListItem::MEDIA_AUDIO_OTHER;
  }
  else if (m_fileItem->HasProperty("ispicture"))
  {
    return XAPP::ListItem::MEDIA_PICTURE;
  }
  else
  {
    return XAPP::ListItem::MEDIA_UNKNOWN;
  }
}

bool ListItem::IsEnabledRecommend()
{
  if (!m_fileItem)
    return false;

  return !m_fileItem->GetPropertyBOOL("disable-recommend");
}

bool ListItem::IsEnabledRate()
{
  if (!m_fileItem)
    return false;

  return !m_fileItem->GetPropertyBOOL("disable-rate");
}

std::string ListItem::GetImage(int id) const
{
  if (!m_fileItem)
    return StringUtils::EmptyString;

  CStdString propName;
  propName.Format("Image%d", id);
  return m_fileItem->GetProperty(propName);    
}

void ListItem::SetMusicOSDButton(int id, const std::string thumbFocus, const std::string thumbNoFocus)
{
  if (!m_fileItem)
    return;

  char keyPrefix[32];
  sprintf(keyPrefix, "music-osd-ext-%d-", id);

  std::string key = keyPrefix;
  key += "on";
  m_fileItem->SetProperty(key, true);

  key = keyPrefix;
  key += "thumb-nofocus";
  m_fileItem->SetProperty(key, thumbNoFocus);

  key = keyPrefix;
  key += "thumb-focus";
  m_fileItem->SetProperty(key, thumbFocus);
}

void ListItem::ClearProperty(const std::string &strKey)
{
  if (!m_fileItem)
    return;

  std::string customKey = "custom:";
  customKey += strKey;
  m_fileItem->ClearProperty(customKey);
}

void ListItem::DeleteMusicOSDButton(int id)
{
  if (!m_fileItem)
    return;

  char keyPrefix[32];
  sprintf(keyPrefix, "music-osd-ext-%d-", id);

  std::string key = keyPrefix;
  key += "on";
  m_fileItem->ClearProperty(key);

  key = keyPrefix;
  key += "thumb-nofocus";
  m_fileItem->ClearProperty(key);

  key = keyPrefix;
  key += "thumb-focus";
  m_fileItem->ClearProperty(key);
}

void ListItem::SetCanShuffle(bool canShuffle)
{
  if (!m_fileItem)
    return;

  if (canShuffle)
  {
    if (m_fileItem->HasProperty("disable-shuffle"))
      m_fileItem->ClearProperty("disable-shuffle");
  }
  else
    m_fileItem->SetProperty("disable-shuffle", "true");
}

void ListItem::SetCanRepeat(bool canRepeat)
{
  if (!m_fileItem)
    return;

  if (canRepeat)
  {
    if (m_fileItem->HasProperty("disable-repeat"))
      m_fileItem->ClearProperty("disable-repeat");
  }
  else
    m_fileItem->SetProperty("disable-repeat", "true");
}

void ListItem::SetPauseOnSeek(bool pauseOnSeek)
{
  if (!m_fileItem)
    return;

  if (pauseOnSeek)
  {
    m_fileItem->SetProperty("pause-on-seek", "true");
  }
  else
  {
    if (m_fileItem->HasProperty("pause-on-seek"))
    {
      m_fileItem->ClearProperty("pause-on-seek");
    }
  }
}

}
