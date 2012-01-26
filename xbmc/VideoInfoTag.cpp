/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "VideoInfoTag.h"
#include "XMLUtils.h"
#include "LocalizeStrings.h"
#include "AdvancedSettings.h"
#include "utils/log.h"
#include "utils/CharsetConverter.h"
#include "Picture.h"
#include "utils/Variant.h"
#include "lib/libBoxee/bxutils.h"

#include <sstream>

using namespace std;

void CVideoInfoTag::Reset()
{
  m_strDirector = "";
  m_strWritingCredits = "";
  m_strGenre = "";
  m_strTagLine = "";
  m_strPlotOutline = "";
  m_strPlot = "";
  m_strPictureURL.Clear();
  m_strTitle = "";
  m_strOriginalTitle = "";
  m_strSortTitle = "";
  m_strVotes = "";
  m_cast.clear();
  m_strSet = "";
  m_strFile = "";
  m_strPath = "";
  m_strIMDBNumber = "";
  m_strMPAARating = "";
  m_strPremiered= "";
  m_strStatus= "";
  m_strProductionCode= "";
  m_strFirstAired= "";
  m_strStudio = "";
  m_strAlbum = "";
  m_strArtist = "";
  m_strTrailer = "";
  m_iTop250 = 0;
  m_iYear = 0;
  m_iSeason = -1;
  m_iEpisode = -1;
  m_iSpecialSortSeason = -1;
  m_iSpecialSortEpisode = -1;
  m_fRating = 0.0f;
  m_iDbId = -1;
  m_iFileId = -1;
  m_iBookmarkId = -1;
  m_iTrack = -1;
  m_fanart.m_xml = "";
  m_strRuntime = "";
  m_lastPlayed = "";
  m_streamDetails.Reset();
  m_playCount = 0;
  m_iRating = 0;
  m_strCountry="";
  m_strShowLink="";
  m_strNFOPath="";
  m_iNfoAccessTime=-1;
  m_iNfoModifiedTime=-1;
}

bool CVideoInfoTag::Save(TiXmlNode *node, const CStdString &tag, bool savePathInfo)
{
  if (!node) return false;

  // we start with a <tag> tag
  TiXmlElement movieElement(tag.c_str());
  TiXmlNode *movie = node->InsertEndChild(movieElement);

  if (!movie) return false;

  XMLUtils::SetString(movie, "title", m_strTitle);
  if (!m_strOriginalTitle.IsEmpty())
    XMLUtils::SetString(movie, "originaltitle", m_strOriginalTitle);
  if (!m_strSortTitle.IsEmpty())
    XMLUtils::SetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::SetFloat(movie, "rating", m_fRating);
  XMLUtils::SetInt(movie, "year", m_iYear);
  XMLUtils::SetInt(movie, "top250", m_iTop250);
  if (tag == "episodedetails" || tag == "tvshow")
  {
    XMLUtils::SetInt(movie, "season", m_iSeason);
    XMLUtils::SetInt(movie, "episode", m_iEpisode);
    XMLUtils::SetInt(movie, "displayseason",m_iSpecialSortSeason);
    XMLUtils::SetInt(movie, "displayepisode",m_iSpecialSortEpisode);
  }
  if (tag == "musicvideo")
  {
    XMLUtils::SetInt(movie, "track", m_iTrack);
    XMLUtils::SetString(movie, "album", m_strAlbum);
  }
  XMLUtils::SetString(movie, "votes", m_strVotes);
  XMLUtils::SetString(movie, "outline", m_strPlotOutline);
  XMLUtils::SetString(movie, "plot", m_strPlot);
  XMLUtils::SetString(movie, "tagline", m_strTagLine);
  XMLUtils::SetString(movie, "runtime", m_strRuntime);
  if (!m_strPictureURL.m_xml.empty())
  {
    TiXmlDocument doc;
    doc.Parse(m_strPictureURL.m_xml); 
    const TiXmlNode* thumb = doc.FirstChild("thumb");
    while (thumb)
    {
      movie->InsertEndChild(*thumb);
      thumb = thumb->NextSibling("thumb");
    }
  }
  if (m_fanart.m_xml.size())
  {
    TiXmlDocument doc;
    doc.Parse(m_fanart.m_xml);
    movie->InsertEndChild(*doc.RootElement());
  }
  XMLUtils::SetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::SetInt(movie, "playcount", m_playCount);
  XMLUtils::SetString(movie, "lastplayed", m_lastPlayed);
  if (savePathInfo)
  {
    XMLUtils::SetString(movie, "file", m_strFile);
    XMLUtils::SetString(movie, "path", m_strPath);
    XMLUtils::SetString(movie, "filenameandpath", m_strFileNameAndPath);
  }
  if (!m_strEpisodeGuide.IsEmpty())
    XMLUtils::SetString(movie, "episodeguide", m_strEpisodeGuide);

  XMLUtils::SetString(movie, "id", m_strIMDBNumber);
  XMLUtils::SetString(movie, "genre", m_strGenre);
  XMLUtils::SetString(movie, "set", m_strSet);
  XMLUtils::SetString(movie, "credits", m_strWritingCredits);
  XMLUtils::SetString(movie, "director", m_strDirector);
  XMLUtils::SetString(movie, "premiered", m_strPremiered);
  XMLUtils::SetString(movie, "status", m_strStatus);
  XMLUtils::SetString(movie, "code", m_strProductionCode);
  XMLUtils::SetString(movie, "aired", m_strFirstAired);
  XMLUtils::SetString(movie, "studio", m_strStudio);
  XMLUtils::SetString(movie, "trailer", m_strTrailer);

  if (m_streamDetails.HasItems())
  {
    // it goes fileinfo/streamdetails/[video|audio|subtitle]
    TiXmlElement fileinfo("fileinfo");
    TiXmlElement streamdetails("streamdetails");
    for (int iStream=1; iStream<=m_streamDetails.GetVideoStreamCount(); iStream++)
    {
      TiXmlElement stream("video");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetVideoCodec(iStream));
      XMLUtils::SetFloat(&stream, "aspect", m_streamDetails.GetVideoAspect(iStream));
      XMLUtils::SetInt(&stream, "width", m_streamDetails.GetVideoWidth(iStream));
      XMLUtils::SetInt(&stream, "height", m_streamDetails.GetVideoHeight(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetAudioStreamCount(); iStream++)
    {
      TiXmlElement stream("audio");
      XMLUtils::SetString(&stream, "codec", m_streamDetails.GetAudioCodec(iStream));
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetAudioLanguage(iStream));
      XMLUtils::SetInt(&stream, "channels", m_streamDetails.GetAudioChannels(iStream));
      streamdetails.InsertEndChild(stream);
    }
    for (int iStream=1; iStream<=m_streamDetails.GetSubtitleStreamCount(); iStream++)
    {
      TiXmlElement stream("subtitle");
      XMLUtils::SetString(&stream, "language", m_streamDetails.GetSubtitleLanguage(iStream));
      streamdetails.InsertEndChild(stream);
    }
    fileinfo.InsertEndChild(streamdetails);
    movie->InsertEndChild(fileinfo);
  }  /* if has stream details */

  // cast
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    // add a <actor> tag
    TiXmlElement cast("actor");
    TiXmlNode *node = movie->InsertEndChild(cast);
    TiXmlElement actor("name");
    TiXmlNode *actorNode = node->InsertEndChild(actor);
    TiXmlText name(it->strName);
    actorNode->InsertEndChild(name);
    TiXmlElement role("role");
    TiXmlNode *roleNode = node->InsertEndChild(role);
    TiXmlText character(it->strRole);
    roleNode->InsertEndChild(character);
    TiXmlElement thumb("thumb");
    TiXmlNode *thumbNode = node->InsertEndChild(thumb);
    TiXmlText th(it->thumbUrl.GetFirstThumb().m_url);
    thumbNode->InsertEndChild(th);
  }
  XMLUtils::SetString(movie, "artist", m_strArtist);

  return true;
}

bool CVideoInfoTag::Load(const TiXmlElement *movie, bool chained /* = false */)
{
  if (!movie) return false;

  // reset our details if we aren't chained.
  if (!chained) Reset();

/*  if (CStdString("Title").Equals(movie->Value())) // mymovies.xml
    ParseMyMovies(movie);
  else
  */
  ParseNative(movie);

  return true;
}

void CVideoInfoTag::Archive(CArchive& ar)
{
  if (ar.IsStoring())
  {
    // mark this as new-archive
    int nMark = -1;
    ar << nMark;
    
    int nDumpVersion = 3;
    ar << nDumpVersion;
    
    ar << m_strDirector;
    ar << m_strWritingCredits;
    ar << m_strGenre;
    ar << m_strTagLine;
    ar << m_strPlotOutline;
    ar << m_strPlot;
    ar << m_strPictureURL.m_spoof;
    ar << m_strPictureURL.m_xml;
    ar << m_fanart.m_xml;
    ar << m_strTitle;
    ar << m_strSortTitle;
    ar << m_strVotes;
    ar << m_strStudio;
    ar << m_strTrailer;
    ar << (int)m_cast.size();
    for (unsigned int i=0;i<m_cast.size();++i)
    {
      ar << m_cast[i].strName;
      ar << m_cast[i].strRole;
      ar << m_cast[i].thumbUrl.m_xml;
    }

    ar << m_strSet;
    ar << m_strRuntime;
    ar << m_strFile;
    ar << m_strPath;
    ar << m_strIMDBNumber;
    ar << m_strMPAARating;
    ar << m_strFileNameAndPath;
    ar << m_strOriginalTitle;
    ar << m_strEpisodeGuide;
    ar << m_strPremiered;
    ar << m_strStatus;
    ar << m_strProductionCode;
    ar << m_strFirstAired;
    ar << m_strShowTitle;
    ar << m_strAlbum;
    ar << m_strArtist;
    ar << m_playCount;
    ar << m_lastPlayed;
    ar << m_iTop250;
    ar << m_iYear;
    ar << m_iSeason;
    ar << m_iEpisode;
    ar << m_fRating;
    ar << m_iDbId;
    ar << m_iFileId;
    ar << m_iSpecialSortSeason;
    ar << m_iSpecialSortEpisode;
    ar << m_iBookmarkId;
    ar << m_iTrack;
    m_streamDetails.m_nVersion = nDumpVersion;
    ar << m_streamDetails;
  }
  else
  {
    int nMarker = 0;
    int nVersion = 0;
    CStdString strDirector;
    
    ar >> nMarker;
    if (nMarker == -1)
    {
      ar >> nVersion;
      ar >> strDirector;
    }
    else
    {
      int nSize = nMarker;
      if (nSize > 0)
      {
        char *buf = new char[nSize];
        memset(buf,0, nSize);
        char *ptr = buf;
        while (nSize)
        {
          ar >> (*ptr);
          ptr++;
          nSize--;
        }
        strDirector = buf;
        delete [] buf;
      }
    }
    
    m_strDirector = strDirector;
    ar >> m_strWritingCredits;
    ar >> m_strGenre;
    ar >> m_strTagLine;
    ar >> m_strPlotOutline;
    ar >> m_strPlot;
    ar >> m_strPictureURL.m_spoof;
    ar >> m_strPictureURL.m_xml;
    m_strPictureURL.Parse();
    ar >> m_fanart.m_xml;
    m_fanart.Unpack();
    ar >> m_strTitle;
    ar >> m_strSortTitle;
    ar >> m_strVotes;
    ar >> m_strStudio;
    ar >> m_strTrailer;
    int iCastSize;
    ar >> iCastSize;
    for (int i=0;i<iCastSize;++i)
    {
      SActorInfo info;
      ar >> info.strName;
      ar >> info.strRole;
      CStdString strXml;
      ar >> strXml;
      info.thumbUrl.ParseString(strXml);
      m_cast.push_back(info);
    }
    
    ar >> m_strSet;
    ar >> m_strRuntime;
    ar >> m_strFile;
    ar >> m_strPath;
    ar >> m_strIMDBNumber;
    ar >> m_strMPAARating;
    ar >> m_strFileNameAndPath;
    ar >> m_strOriginalTitle;
    ar >> m_strEpisodeGuide;
    ar >> m_strPremiered;
    ar >> m_strStatus;
    ar >> m_strProductionCode;
    ar >> m_strFirstAired;
    ar >> m_strShowTitle;
    ar >> m_strAlbum;
    ar >> m_strArtist;
    ar >> m_playCount;
    if (nVersion > 1)
      ar >> m_lastPlayed;
    ar >> m_iTop250;
    ar >> m_iYear;
    ar >> m_iSeason;
    ar >> m_iEpisode;
    ar >> m_fRating;
    ar >> m_iDbId;
    if (nVersion > 1)
      ar >> m_iFileId;
    ar >> m_iSpecialSortSeason;
    ar >> m_iSpecialSortEpisode;
    ar >> m_iBookmarkId;
    if (nVersion > 1)
      ar >> m_iTrack;
    if (nVersion > 1)
    {
      m_streamDetails.m_nVersion = nVersion;
      ar >> m_streamDetails;
    }
  }
}

void CVideoInfoTag::Serialize(CVariant& value)
{
  value["director"] = m_strDirector;
  value["writingcredits"] = m_strWritingCredits;
  value["genre"] = m_strGenre;
  value["tagline"] = m_strTagLine;
  value["plotoutline"] = m_strPlotOutline;
  value["plot"] = m_strPlot;
  value["title"] = m_strTitle;
  value["votes"] = m_strVotes;
  value["studio"] = m_strStudio;
  value["trailer"] = m_strTrailer;
  for (unsigned int i = 0; i < m_cast.size(); ++i)
  {
    value["cast"][i]["name"] = m_cast[i].strName;
    value["cast"][i]["role"] = m_cast[i].strRole;
  }
  value["set"] = m_strSet;
  value["runtime"] = m_strRuntime;
  value["file"] = m_strFile;
  value["path"] = m_strPath;
  value["imdbnumber"] = m_strIMDBNumber;
  value["filenameandpath"] = m_strFileNameAndPath;
  value["originaltitle"] = m_strOriginalTitle;
  value["episodeguide"] = m_strEpisodeGuide;
  value["premiered"] = m_strPremiered;
  value["status"] = m_strStatus;
  value["productioncode"] = m_strProductionCode;
  value["firstaired"] = m_strFirstAired;
  value["showtitle"] = m_strShowTitle;
  value["album"] = m_strAlbum;
  value["artist"] = m_strArtist;
  value["playcount"] = m_playCount;
  value["lastPlayed"] = m_lastPlayed;
  value["top250"] = m_iTop250;
  value["year"] = m_iYear;
  value["season"] = m_iSeason;
  value["episode"] = m_iEpisode;
  value["rating"] = m_fRating;
  value["dbid"] = m_iDbId;
  value["fileid"] = m_iFileId;
  value["track"] = m_iTrack;
  m_streamDetails.Serialize(value["streamDetails"]);
}

const CStdString CVideoInfoTag::GetCast(bool bIncludeRole /*= false*/) const
{
  CStdString strLabel;
  for (iCast it = m_cast.begin(); it != m_cast.end(); ++it)
  {
    CStdString character;
    if (it->strRole.IsEmpty() || !bIncludeRole)
      character.Format("%s\n", it->strName.c_str());
    else
      character.Format("%s %s %s\n", it->strName.c_str(), g_localizeStrings.Get(20347).c_str(), it->strRole.c_str());
    strLabel += character;
  }
  return strLabel.TrimRight("\n");
}

void CVideoInfoTag::ParseNative(const TiXmlElement* movie)
{
  XMLUtils::GetString(movie, "title", m_strTitle);
  XMLUtils::GetString(movie, "originaltitle", m_strOriginalTitle);
  XMLUtils::GetString(movie, "sorttitle", m_strSortTitle);
  XMLUtils::GetFloat(movie, "rating", m_fRating);
  XMLUtils::GetInt(movie, "rating", m_iRating);
  XMLUtils::GetInt(movie, "year", m_iYear);
  XMLUtils::GetInt(movie, "top250", m_iTop250);
  XMLUtils::GetInt(movie, "season", m_iSeason);
  XMLUtils::GetInt(movie, "episode", m_iEpisode);
  XMLUtils::GetInt(movie, "track", m_iTrack);
  XMLUtils::GetInt(movie, "displayseason", m_iSpecialSortSeason);
  XMLUtils::GetInt(movie, "displayepisode", m_iSpecialSortEpisode);
  int after=0;
  XMLUtils::GetInt(movie, "displayafterseason",after);
  if (after > 0)
  {
    m_iSpecialSortSeason = after;
    m_iSpecialSortEpisode = 0x1000; // should be more than any realistic episode number
  }
  XMLUtils::GetString(movie, "votes", m_strVotes);
  XMLUtils::GetString(movie, "outline", m_strPlotOutline);
  XMLUtils::GetString(movie, "plot", m_strPlot);
  XMLUtils::GetString(movie, "tagline", m_strTagLine);
  XMLUtils::GetString(movie, "runtime", m_strRuntime);

  int seconds=0;
  int minutes= BOXEE::BXUtils::StringToInt(m_strRuntime); //we assume its in xxx mins format

  if (m_strRuntime.Find(":") > 0)
  { // support for mm:ss
    std::vector<std::string> tokenizedRuntime = BOXEE::BXUtils::StringTokenize(m_strRuntime,":");

    if (tokenizedRuntime.size() == 2)
    {
      minutes = BOXEE::BXUtils::StringToInt(tokenizedRuntime[0]);
      seconds = BOXEE::BXUtils::StringToInt(tokenizedRuntime[1]);
    }
  }

  int timeInSeconds = (minutes*60+seconds);
  if (timeInSeconds > 0)
  {
    m_strRuntime = BOXEE::BXUtils::IntToString(timeInSeconds);
  }
  else
  { //if its 0 or negative, store it as "" to merge properly
    m_strRuntime.clear();
  }

  XMLUtils::GetString(movie, "mpaa", m_strMPAARating);
  XMLUtils::GetInt(movie, "playcount", m_playCount);
  XMLUtils::GetString(movie, "lastplayed", m_lastPlayed);
  XMLUtils::GetString(movie, "file", m_strFile);
  XMLUtils::GetString(movie, "path", m_strPath);
  XMLUtils::GetString(movie, "id", m_strIMDBNumber);
  XMLUtils::GetString(movie, "filenameandpath", m_strFileNameAndPath);
  XMLUtils::GetString(movie, "premiered", m_strPremiered);
  XMLUtils::GetString(movie, "status", m_strStatus);
  XMLUtils::GetString(movie, "code", m_strProductionCode);
  XMLUtils::GetString(movie, "aired", m_strFirstAired);
  XMLUtils::GetString(movie, "album", m_strAlbum);
  XMLUtils::GetString(movie, "trailer", m_strTrailer);

  const TiXmlElement *thumb = movie->FirstChildElement("thumb");
  while (thumb)
  {
    m_strPictureURL.ParseElement(thumb);
    thumb = thumb->NextSiblingElement("thumb");
  }

  XMLUtils::GetAdditiveString(movie,"genre",g_advancedSettings.m_videoItemSeparator,m_strGenre);
  XMLUtils::GetAdditiveString(movie,"country",g_advancedSettings.m_videoItemSeparator,m_strCountry);
  XMLUtils::GetAdditiveString(movie,"credits",g_advancedSettings.m_videoItemSeparator,m_strWritingCredits);
  XMLUtils::GetAdditiveString(movie,"director",g_advancedSettings.m_videoItemSeparator,m_strDirector);
  XMLUtils::GetAdditiveString(movie,"showlink",g_advancedSettings.m_videoItemSeparator,m_strShowLink);

  // cast
  const TiXmlElement* node = movie->FirstChildElement("actor");
  while (node)
  {
    const TiXmlNode *actor = node->FirstChild("name");
    if (actor && actor->FirstChild())
    {
      SActorInfo info;
      info.strName = actor->FirstChild()->Value();
      const TiXmlNode *roleNode = node->FirstChild("role");
      if (roleNode && roleNode->FirstChild())
        info.strRole = roleNode->FirstChild()->Value();
      const TiXmlElement* thumb = node->FirstChildElement("thumb");
      while (thumb)
      {
        info.thumbUrl.ParseElement(thumb);
        thumb = thumb->NextSiblingElement("thumb");
      }
      const char* clear=node->Attribute("clear");
      if (clear && stricmp(clear,"true"))
        m_cast.clear();
      m_cast.push_back(info);
    }
    node = node->NextSiblingElement("actor");
  }
  XMLUtils::GetAdditiveString(movie,"set",g_advancedSettings.m_videoItemSeparator,m_strSet);
  XMLUtils::GetAdditiveString(movie,"studio",g_advancedSettings.m_videoItemSeparator,m_strStudio);
  // artists
  node = movie->FirstChildElement("artist");
  while (node)
  {
    const TiXmlNode* pNode = node->FirstChild("name");
    const char* pValue=NULL;
    if (pNode && pNode->FirstChild())
      pValue = pNode->FirstChild()->Value();
    else if (node->FirstChild())
      pValue = node->FirstChild()->Value();
    if (pValue)
    {
      const char* clear=node->Attribute("clear");
      if (m_strArtist.IsEmpty() || (clear && stricmp(clear,"true")==0))
        m_strArtist += pValue;
      else
        m_strArtist += g_advancedSettings.m_videoItemSeparator + pValue;
    }
    node = node->NextSiblingElement("artist");
  }

  m_streamDetails.Reset();
  node = movie->FirstChildElement("fileinfo");
  if (node)
  {
    // Try to pull from fileinfo/streamdetails/[video|audio|subtitle] 
    const TiXmlNode *nodeStreamDetails = node->FirstChild("streamdetails");
    if (nodeStreamDetails)
    {
      const TiXmlNode *nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("audio", nodeDetail)))
      {
        CStreamDetailAudio *p = new CStreamDetailAudio();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        XMLUtils::GetInt(nodeDetail, "channels", p->m_iChannels);
        p->m_strCodec.MakeLower();
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("video", nodeDetail)))
      {
        CStreamDetailVideo *p = new CStreamDetailVideo();
        XMLUtils::GetString(nodeDetail, "codec", p->m_strCodec);
        XMLUtils::GetFloat(nodeDetail, "aspect", p->m_fAspect);
        XMLUtils::GetInt(nodeDetail, "width", p->m_iWidth);
        XMLUtils::GetInt(nodeDetail, "height", p->m_iHeight);
        XMLUtils::GetInt(nodeDetail, "durationinseconds", p->m_iDuration);
        p->m_strCodec.MakeLower();
        m_streamDetails.AddStream(p);
      }
      nodeDetail = NULL;
      while ((nodeDetail = nodeStreamDetails->IterateChildren("subtitle", nodeDetail)))
      {
        CStreamDetailSubtitle *p = new CStreamDetailSubtitle();
        XMLUtils::GetString(nodeDetail, "language", p->m_strLanguage);
        p->m_strLanguage.MakeLower();
        m_streamDetails.AddStream(p);
      }
    }
    m_streamDetails.DetermineBestStreams();
  }  /* if fileinfo */

  const TiXmlElement *epguide = movie->FirstChildElement("episodeguide");
  if (epguide)
  {
    if (epguide->FirstChild() && strncmp(epguide->FirstChild()->Value(),"<episodeguide>",14) == 0)
      m_strEpisodeGuide = epguide->FirstChild()->Value();
    else if (epguide->FirstChild() && strlen(epguide->FirstChild()->Value()) > 0)
    {
      stringstream stream;
      stream << *epguide;
      m_strEpisodeGuide = stream.str();
    }
  }

  // fanart
  const TiXmlElement *fanart = movie->FirstChildElement("fanart");
  if (fanart)
  {
    m_fanart.m_xml << *fanart;
    m_fanart.Unpack();
  }
}

void CVideoInfoTag::ParseMyMovies(const TiXmlElement *movie)
{
  XMLUtils::GetString(movie, "LocalTitle", m_strTitle);
  XMLUtils::GetString(movie, "OriginalTitle", m_strOriginalTitle);
  XMLUtils::GetString(movie, "SortTitle", m_strSortTitle);
  XMLUtils::GetFloat(movie, "Rating", m_fRating);
  XMLUtils::GetString(movie, "IMDB", m_strIMDBNumber);
  XMLUtils::GetInt(movie, "ProductionYear", m_iYear);
  int runtime = 0;
  XMLUtils::GetInt(movie, "RunningTime", runtime);
  m_strRuntime.Format("%i:%02d", runtime/60, runtime%60); // convert from minutes to hh:mm
  XMLUtils::GetString(movie, "TagLine", m_strTagLine);
  XMLUtils::GetString(movie, "Description", m_strPlot);
  if (m_strTagLine.IsEmpty())
  {
    if (m_strPlot.find("\r\n") > 0)
      m_strPlotOutline = m_strPlot.substr(0, m_strPlot.find("\r\n") - 1);
    else
    m_strPlotOutline = m_strPlot;
  }

  // thumb
  CStdString strTemp;
  const TiXmlNode *node = movie->FirstChild("Covers");
  while (node)
  {
    const TiXmlNode *front = node->FirstChild("Front");
    if (front && front->FirstChild())
    {
      strTemp = front->FirstChild()->Value();
      if (!strTemp.IsEmpty())
        m_strPictureURL.ParseString(strTemp);
    }
    node = node->NextSibling("Covers");
  }
  // genres
  node = movie->FirstChild("Genres");
  const TiXmlNode *genre = node ? node->FirstChildElement("Genre") : NULL;
  while (genre)
  {
    if (genre && genre->FirstChild())
    {
      strTemp = genre->FirstChild()->Value();
      if (m_strGenre.IsEmpty())
        m_strGenre = strTemp;
      else
        m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    genre = genre->NextSiblingElement("Genre");
  }
  // MyMovies categories to genres
  if (g_advancedSettings.m_bVideoLibraryMyMoviesCategoriesToGenres)
  {
    node = movie->FirstChild("Categories");
    const TiXmlNode *category = node ? node->FirstChildElement("Category") : NULL;
    while (category)
    {
      if (category && category->FirstChild())
      {
        strTemp = category->FirstChild()->Value();
        if (m_strGenre.IsEmpty())
          m_strGenre = strTemp;
        else
          m_strGenre += g_advancedSettings.m_videoItemSeparator+strTemp;
      }
      category = category->NextSiblingElement("Category");
    }
  }
  // studios
  node = movie->FirstChild("Studios");
  while (node)
  {
    const TiXmlNode *studio = node->FirstChild("Studio");
    if (studio && studio->FirstChild())
    {
      strTemp = studio->FirstChild()->Value();
      if (m_strStudio.IsEmpty())
        m_strStudio = strTemp;
      else
        m_strStudio += g_advancedSettings.m_videoItemSeparator+strTemp;
    }
    node = node->NextSibling("Studios");
  }
  // persons
  int personType = -1;
  node = movie->FirstChild("Persons");
  const TiXmlElement *element = node ? node->FirstChildElement("Person") : NULL;
  while (element)
  {
    element->Attribute("Type", &personType);
    const TiXmlNode *person = element->FirstChild("Name");
    if (person && person->FirstChild())
    {
      if (personType == 1) // actor
      {
        SActorInfo info;
        info.strName = person->FirstChild()->Value();
        const TiXmlNode *roleNode = element->FirstChild("Role");
        if (roleNode && roleNode->FirstChild())
          info.strRole = roleNode->FirstChild()->Value();
        m_cast.push_back(info);
      }
      else if (personType == 2) // director
      {
        strTemp = person->FirstChild()->Value();
        if (m_strDirector.IsEmpty())
          m_strDirector = strTemp;
        else
          m_strDirector += g_advancedSettings.m_videoItemSeparator+strTemp;
      }
    }
    element = element->NextSiblingElement("Person");
  }
}

void CVideoInfoTag::Dump() const
{
  CLog::Log(LOGDEBUG,"** Video Info Tag:");
  CLog::Log(LOGDEBUG,"Title: %s", m_strTitle.c_str());
  CLog::Log(LOGDEBUG,"Director: %s", m_strDirector.c_str());
  CLog::Log(LOGDEBUG,"Picture (spoof): %s", m_strPictureURL.m_spoof.c_str());
  CLog::Log(LOGDEBUG,"Picture (xml): %s", m_strPictureURL.m_xml.c_str());
  CLog::Log(LOGDEBUG,"Genre: %s", m_strGenre.c_str());
  CLog::Log(LOGDEBUG,"Trailer: %s", m_strTrailer.c_str());
  CLog::Log(LOGDEBUG,"Credits: %s", m_strWritingCredits.c_str());
  CLog::Log(LOGDEBUG,"Plot (80 chars): %s", m_strPlot.GetLength() > 80?m_strPlot.Mid(0,80).c_str():m_strPlot.c_str());
  CLog::Log(LOGDEBUG,"Plot-Outline (80 chars): %s", m_strPlotOutline.GetLength() > 80?m_strPlotOutline.Mid(0,80).c_str():m_strPlotOutline.c_str());
  CLog::Log(LOGDEBUG,"m_strTagLine: %s", m_strTagLine.c_str());
  CLog::Log(LOGDEBUG,"m_strVotes: %s", m_strVotes.c_str());
  CLog::Log(LOGDEBUG,"m_strStudio: %s", m_strStudio.c_str());
  CLog::Log(LOGDEBUG,"m_strRuntime: %s", m_strRuntime.c_str());
  CLog::Log(LOGDEBUG,"m_strFile: %s", m_strFile.c_str());
  CLog::Log(LOGDEBUG,"m_strPath: %s", m_strPath.c_str());
  CLog::Log(LOGDEBUG,"m_strIMDBNumber: %s", m_strIMDBNumber.c_str());
  CLog::Log(LOGDEBUG,"m_strMPAARating: %s", m_strMPAARating.c_str());
  CLog::Log(LOGDEBUG,"m_strFileNameAndPath: %s", m_strFileNameAndPath.c_str());
  CLog::Log(LOGDEBUG,"m_strOriginalTitle: %s", m_strOriginalTitle.c_str());
  CLog::Log(LOGDEBUG,"m_strEpisodeGuide: %s", m_strEpisodeGuide.c_str());
  CLog::Log(LOGDEBUG,"m_strPremiered: %s", m_strPremiered.c_str());
  CLog::Log(LOGDEBUG,"m_strStatus: %s", m_strStatus.c_str());
  CLog::Log(LOGDEBUG,"m_strProductionCode: %s", m_strProductionCode.c_str());
  CLog::Log(LOGDEBUG,"m_strFirstAired: %s", m_strFirstAired.c_str());
  CLog::Log(LOGDEBUG,"m_strShowTitle: %s", m_strShowTitle.c_str());
  CLog::Log(LOGDEBUG,"m_strAlbum: %s", m_strAlbum.c_str());
  CLog::Log(LOGDEBUG,"m_strArtist: %s", m_strArtist.c_str());
  CLog::Log(LOGDEBUG,"m_playCount: %d", m_playCount);
  CLog::Log(LOGDEBUG,"m_iTop250: %d", m_iTop250);
  CLog::Log(LOGDEBUG,"m_iYear: %d", m_iYear);
  CLog::Log(LOGDEBUG,"m_iSeason: %d", m_iSeason);
  CLog::Log(LOGDEBUG,"m_iEpisode: %d", m_iEpisode);
  CLog::Log(LOGDEBUG,"m_fRating: %f", m_fRating);
  CLog::Log(LOGDEBUG,"m_iRating: %d", m_iRating);
  CLog::Log(LOGDEBUG,"m_iDbId: %d", m_iDbId);
  CLog::Log(LOGDEBUG,"m_iSpecialSortSeason: %d", m_iSpecialSortSeason);
  CLog::Log(LOGDEBUG,"m_iSpecialSortEpisode: %d", m_iSpecialSortEpisode);
  CLog::Log(LOGDEBUG,"m_iBookmarkId: %d", m_iBookmarkId);

  CLog::Log(LOGDEBUG,"cast:");
  for (unsigned int i=0;i<m_cast.size();++i)
    CLog::Log(LOGDEBUG,"actor: %s. Role: %s. thumb: %s", m_cast[i].strName.c_str(), m_cast[i].strRole.c_str(), m_cast[i].thumbUrl.m_xml.c_str());

}

bool CVideoInfoTag::HasStreamDetails() const
{
  return m_streamDetails.HasItems();
}

bool CVideoInfoTag::IsEmpty() const
{
  return (m_strTitle.IsEmpty() &&
          m_strFile.IsEmpty() &&
          m_strPath.IsEmpty());
}

void CVideoInfoTag::MergeFieldsFrom(const CVideoInfoTag& other)
{
  if (!other.m_strDirector.IsEmpty() && m_strDirector.IsEmpty())
    m_strDirector = other.m_strDirector;

  if (!other.m_strWritingCredits.IsEmpty() && m_strWritingCredits.IsEmpty())
    m_strWritingCredits = other.m_strWritingCredits;

  if (!other.m_strGenre.IsEmpty() && m_strGenre.IsEmpty())
    m_strGenre = other.m_strGenre;

  if (!other.m_strTagLine.IsEmpty() && m_strTagLine.IsEmpty())
    m_strTagLine = other.m_strTagLine;

  if (!other.m_strPlotOutline.IsEmpty() && m_strPlotOutline.IsEmpty())
    m_strPlotOutline = other.m_strPlotOutline;

  if (!other.m_strPlot.IsEmpty() && m_strPlot.IsEmpty())
    m_strPlot = other.m_strPlot;

  if (!other.m_strTitle.IsEmpty() && m_strTitle.IsEmpty())
    m_strTitle = other.m_strTitle;

  if (!other.m_strVotes.IsEmpty() && m_strVotes.IsEmpty())
    m_strVotes = other.m_strVotes;

  if (!other.m_strStudio.IsEmpty() && m_strStudio.IsEmpty())
    m_strStudio = other.m_strStudio;

  if (!other.m_strTrailer.IsEmpty() && m_strTrailer.IsEmpty())
    m_strTrailer = other.m_strTrailer;

  if (!other.m_strSet.IsEmpty() && m_strSet.IsEmpty())
    m_strSet = other.m_strSet;

  if (!other.m_strRuntime.IsEmpty() && (m_strRuntime.IsEmpty() || BOXEE::BXUtils::StringToInt(m_strRuntime) <= 0))
    m_strRuntime = other.m_strRuntime;

  if (!other.m_strFile.IsEmpty() && m_strFile.IsEmpty())
    m_strFile = other.m_strFile;

  if (!other.m_strPath.IsEmpty() && m_strPath.IsEmpty())
    m_strPath = other.m_strPath;

  if (!other.m_strIMDBNumber.IsEmpty() && (m_strIMDBNumber.IsEmpty() || m_strIMDBNumber.substr(0,2).compare("tt")==0))
    m_strIMDBNumber = other.m_strIMDBNumber;

  if (!other.m_strFileNameAndPath.IsEmpty() && m_strFileNameAndPath.IsEmpty())
    m_strFileNameAndPath = other.m_strFileNameAndPath;

  if (!other.m_strOriginalTitle.IsEmpty() && m_strOriginalTitle.IsEmpty())
    m_strOriginalTitle = other.m_strOriginalTitle;

  if (!other.m_strEpisodeGuide.IsEmpty() && m_strEpisodeGuide.IsEmpty())
    m_strEpisodeGuide = other.m_strEpisodeGuide;

  if (!other.m_strPremiered.IsEmpty() && m_strPremiered.IsEmpty())
    m_strPremiered = other.m_strPremiered;

  if (!other.m_strStatus.IsEmpty() && m_strStatus.IsEmpty())
    m_strStatus = other.m_strStatus;

  if (!other.m_strProductionCode.IsEmpty() && m_strProductionCode.IsEmpty())
    m_strProductionCode = other.m_strProductionCode;

  if (!other.m_strFirstAired.IsEmpty() && m_strFirstAired.IsEmpty())
    m_strFirstAired = other.m_strFirstAired;

  if (!other.m_strShowTitle.IsEmpty() && m_strShowTitle.IsEmpty())
    m_strShowTitle = other.m_strShowTitle;

  if (!other.m_strAlbum.IsEmpty() && m_strAlbum.IsEmpty())
    m_strAlbum = other.m_strAlbum;

  if (!other.m_strArtist.IsEmpty() && m_strArtist.IsEmpty())
    m_strArtist = other.m_strArtist;

  if (!other.m_strShowTitle.IsEmpty() && m_strShowTitle.IsEmpty())
    m_strShowTitle = other.m_strShowTitle;

  if (!other.m_cast.empty() && (m_cast.size() == 0))
    m_cast = other.m_cast;

  if ((m_playCount == 0) && (m_playCount != other.m_playCount))
    m_playCount = other.m_playCount;

  if (!other.m_lastPlayed.IsEmpty() && m_lastPlayed.IsEmpty())
    m_lastPlayed = other.m_lastPlayed;

  if ((m_iTop250 == 0) && (m_iTop250 != other.m_iTop250))
    m_iTop250 = other.m_iTop250;

  if ((m_iYear == 0) && (m_iYear != other.m_iYear))
    m_iYear = other.m_iYear;

  if ((m_iSeason == -1) && (m_iSeason != other.m_iSeason))
    m_iSeason = other.m_iSeason;

  if ((m_iEpisode == -1) && (m_iEpisode != other.m_iEpisode))
    m_iEpisode = other.m_iEpisode;

  if ((m_fRating == 0.0f) && (m_fRating != other.m_fRating))
    m_fRating = other.m_fRating;

  if ((m_iRating == 0) && (m_iRating != other.m_iRating))
    m_iRating = other.m_iRating;

  if ((m_iDbId == -1) && (m_iDbId != other.m_iDbId))
    m_iDbId = other.m_iDbId;

  if ((m_iFileId == -1) && (m_iFileId != other.m_iFileId))
    m_iFileId = other.m_iFileId;

  if ((m_iTrack == -1) && (m_iTrack != other.m_iTrack))
    m_iTrack = other.m_iTrack;

  if (!other.m_strShowLink.IsEmpty() && m_strShowLink.IsEmpty())
    m_strShowLink = other.m_strShowLink;

  if (!other.m_strCountry.IsEmpty() && m_strCountry.IsEmpty())
    m_strCountry = other.m_strCountry;

  if (!other.m_strMPAARating.IsEmpty() && m_strMPAARating.IsEmpty())
    m_strMPAARating = other.m_strMPAARating;

  //if other tag has more valid values than ours, copy it
  if (other.m_rottenTomatoDetails.bValidAudienceDetails && other.m_rottenTomatoDetails.bValidCriticsDetails &&
      (!m_rottenTomatoDetails.bValidAudienceDetails || !m_rottenTomatoDetails.bValidCriticsDetails))
  {
    m_rottenTomatoDetails = other.m_rottenTomatoDetails;
  }
}
