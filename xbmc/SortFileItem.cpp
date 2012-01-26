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

#include "AdvancedSettings.h"
#include "SortFileItem.h"
#include "StringUtils.h"
#include "VideoInfoTag.h"
#include "MusicInfoTag.h"
#include "FileItem.h"
#include "URL.h"
#include "utils/log.h"
#include "lib/libBoxee/bxutils.h"

#define RETURN_IF_NULL(x,y) if ((x) == NULL) { CLog::Log(LOGWARNING, "%s, sort item is null", __FUNCTION__); return y; }
#define IGNORE_PREFIX_CHARACTERS "'("

inline int StartsWithToken(const CStdString& strLabel)
{
  for (unsigned int i=0;i<g_advancedSettings.m_vecTokens.size();++i)
  {
    if (g_advancedSettings.m_vecTokens[i].size() < strLabel.size() &&
        strnicmp(g_advancedSettings.m_vecTokens[i].c_str(), strLabel.c_str(), g_advancedSettings.m_vecTokens[i].size()) == 0)
      return g_advancedSettings.m_vecTokens[i].size();
  }
  return 0;
}

bool SSortFileItem::Ascending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::Descending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
  return left->m_bIsFolder;
}

bool SSortFileItem::IgnoreFoldersAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
}

bool SSortFileItem::IgnoreFoldersDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
}


bool SSortFileItem::ReleaseDateAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  if (!left->HasProperty("releasedateVal") || !right->HasProperty("releasedateVal"))
    return false;

  CStdString releaseDateEpochLeft = left->GetProperty("releasedateVal");
  CStdString releaseDateEpochRight = right->GetProperty("releasedateVal");

  long leftDate  = (long) BOXEE::BXUtils::StringToUnsignedLong(releaseDateEpochLeft);
  long rightDate = (long) BOXEE::BXUtils::StringToUnsignedLong(releaseDateEpochRight);

  if (leftDate < rightDate)
    return true;
  else
    return false;
}

bool SSortFileItem::ReleaseDateDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  if (!left->HasProperty("releasedateVal") || !right->HasProperty("releasedateVal"))
    return false;

  CStdString releaseDateEpochLeft = left->GetProperty("releasedateVal");
  CStdString releaseDateEpochRight = right->GetProperty("releasedateVal");

  long leftDate  = (long) BOXEE::BXUtils::StringToUnsignedLong(releaseDateEpochLeft);
  long rightDate = (long) BOXEE::BXUtils::StringToUnsignedLong(releaseDateEpochRight);

  if (leftDate > rightDate)
    return true;
  else
    return false;
}

void SSortFileItem::ByLabel(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetLabel());
}

void SSortFileItem::ByLabelNoThe(CFileItemPtr &item)
{
  if (!item) return;

  CStdString itemLabel = item->GetLabel();
  size_t itemLabelPos = itemLabel.find_first_not_of(IGNORE_PREFIX_CHARACTERS);

  if (itemLabelPos > 0)
    itemLabel = itemLabel.Mid(itemLabelPos);

  item->SetSortLabel(itemLabel.Mid(StartsWithToken(itemLabel)));
}

void SSortFileItem::ByFile(CFileItemPtr &item)
{
  if (!item) return;

  CURI url(item->m_strPath);
  CStdString label;
  label.Format("%s %d", url.GetFileNameWithoutPath().c_str(), item->m_lStartOffset);
  item->SetSortLabel(label);
}

void SSortFileItem::ByFullPath(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%s %d", item->m_strPath, item->m_lStartOffset);
  item->SetSortLabel(label);
}

void SSortFileItem::ByDate(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%s %s", item->m_dateTime.GetAsDBDateTime().c_str(), item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::BySize(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%"PRId64, item->m_dwSize);
  item->SetSortLabel(label);
}

void SSortFileItem::ByDriveType(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  label.Format("%d %s", item->m_iDriveType, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::BySongTitle(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetMusicInfoTag()->GetTitle());
}

void SSortFileItem::BySongTitleNoThe(CFileItemPtr &item)
{
  if (!item) return;
  int start = StartsWithToken(item->GetMusicInfoTag()->GetTitle());
  item->SetSortLabel(item->GetMusicInfoTag()->GetTitle().Mid(start));
}

void SSortFileItem::BySongAlbum(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strAlbum;

  CStdString artist;
  if (item->HasMusicInfoTag())
    artist = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    artist = item->GetVideoInfoTag()->m_strArtist;
  label += " " + artist;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::ByLinkTitle(CFileItemPtr& item)
{
  if (!item) return;
  item->SetSortLabel(item->GetProperty("link-title"));
}

void SSortFileItem::BySongAlbumNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strAlbum;
  label = label.Mid(StartsWithToken(label));

  CStdString artist;
  if (item->HasMusicInfoTag())
    artist = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    artist = item->GetVideoInfoTag()->m_strArtist;
  artist = artist.Mid(StartsWithToken(artist));
  label += " " + artist;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongArtist(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strArtist;

  if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
  {
    int year = 0;
    if (item->HasMusicInfoTag())
      year = item->GetMusicInfoTag()->GetYear();
    else if (item->HasVideoInfoTag())
      year = item->GetVideoInfoTag()->m_iYear;
    label.AppendFormat(" %i", year);
  }

  CStdString album;
  if (item->HasMusicInfoTag())
    album = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    album = item->GetVideoInfoTag()->m_strAlbum;
  label += " " + album;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongArtistNoThe(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label = item->GetMusicInfoTag()->GetArtist();
  else if (item->HasVideoInfoTag())
    label = item->GetVideoInfoTag()->m_strArtist;
  label = label.Mid(StartsWithToken(label));

  if (g_advancedSettings.m_bMusicLibraryAlbumsSortByArtistThenYear)
  {
    int year = 0;
    if (item->HasMusicInfoTag())
      year = item->GetMusicInfoTag()->GetYear();
    else if (item->HasVideoInfoTag())
      year = item->GetVideoInfoTag()->m_iYear;
    label.AppendFormat(" %i", year);
  }

  CStdString album;
  if (item->HasMusicInfoTag())
    album = item->GetMusicInfoTag()->GetAlbum();
  else if (item->HasVideoInfoTag())
    album = item->GetVideoInfoTag()->m_strAlbum;
  album = album.Mid(StartsWithToken(album));
  label += " " + album;

  if (item->HasMusicInfoTag())
    label.AppendFormat(" %i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());

  item->SetSortLabel(label);
}

void SSortFileItem::BySongTrackNum(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (item->HasMusicInfoTag())
  label.Format("%i", item->GetMusicInfoTag()->GetTrackAndDiskNumber());
  if (item->HasVideoInfoTag())
    label.Format("%i", item->GetVideoInfoTag()->m_iTrack);
  item->SetSortLabel(label);
}

void SSortFileItem::BySongDuration(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->GetMusicInfoTag()->GetDuration());
  item->SetSortLabel(label);
}

void SSortFileItem::BySongRating(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%c %s", item->GetMusicInfoTag()->GetRating(), item->GetMusicInfoTag()->GetTitle().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByProgramCount(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%i", item->m_iprogramCount);
  item->SetSortLabel(label);
}

void SSortFileItem::ByGenre(CFileItemPtr &item)
{
  if (!item) return;

  if (item->HasMusicInfoTag())
    item->SetSortLabel(item->GetMusicInfoTag()->GetGenre());
  else
    item->SetSortLabel(item->GetVideoInfoTag()->m_strGenre);
}

void SSortFileItem::ByYear(CFileItemPtr &item)
{
  if (!item) return;

  CStdString label;
  if (item->HasMusicInfoTag())
    label.Format("%i %s", item->GetMusicInfoTag()->GetYear(), item->GetLabel().c_str());
  else
    label.Format("%s %s %i %s", item->GetVideoInfoTag()->m_strPremiered.c_str(), item->GetVideoInfoTag()->m_strFirstAired, item->GetVideoInfoTag()->m_iYear, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByMovieTitle(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strTitle);
}

void SSortFileItem::ByMovieSortTitle(CFileItemPtr &item)
{
  if (!item) return;
  if (!item->GetVideoInfoTag()->m_strSortTitle.IsEmpty())
    item->SetSortLabel(item->GetVideoInfoTag()->m_strSortTitle);
  else
    item->SetSortLabel(item->GetVideoInfoTag()->m_strTitle);
}

void SSortFileItem::ByMovieSortTitleNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  if (!item->GetVideoInfoTag()->m_strSortTitle.IsEmpty())
    label = item->GetVideoInfoTag()->m_strSortTitle;
  else
    label = item->GetVideoInfoTag()->m_strTitle;
  label = label.Mid(StartsWithToken(label));
  item->SetSortLabel(label);
}

void SSortFileItem::ByMovieRating(CFileItemPtr &item)
{
  if (!item) return;
  CStdString label;
  label.Format("%f %s", item->GetVideoInfoTag()->m_fRating, item->GetLabel().c_str());
  item->SetSortLabel(label);
}

void SSortFileItem::ByMovieRuntime(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strRuntime);
}

void SSortFileItem::ByMPAARating(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strMPAARating + " " + item->GetLabel());
}

void SSortFileItem::ByStudio(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strStudio);
}

void SSortFileItem::ByStudioNoThe(CFileItemPtr &item)
{
  if (!item) return;
  CStdString studio = item->GetVideoInfoTag()->m_strStudio;
  item->SetSortLabel(studio.Mid(StartsWithToken(studio)));
}

void SSortFileItem::ByEpisodeNum(CFileItemPtr &item)
{
  if (!item) return;

  if (!item->HasVideoInfoTag())
  {
    SSortFileItem::ByDate(item);
    return;
  }

  const CVideoInfoTag *tag = item->GetVideoInfoTag();

  // we calculate an offset number based on the episode's
  // sort season and episode values. in addition
  // we include specials 'episode' numbers to get proper
  // sorting of multiple specials in a row. each
  // of these are given their particular ranges to semi-ensure uniqueness.
  // theoretical problem: if a show has > 128 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^8-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  unsigned int num;
  if (tag->m_iSpecialSortEpisode > 0)
    num = (tag->m_iSpecialSortSeason<<24)+(tag->m_iSpecialSortEpisode<<8)-(128-tag->m_iEpisode);
  else if (tag->m_iSeason < 0 && tag->m_iEpisode < 0)
  {
    SSortFileItem::ByDate(item);
    return;
  }
  else
    num = (tag->m_iSeason<<24)+(tag->m_iEpisode<<8);

  // check filename as there can be duplicates now
  CURI file(tag->m_strFileNameAndPath);
  CStdString label;
  label.Format("%u %s", num, file.GetFileName().c_str());
  item->SetSortLabel(label);

}

void SSortFileItem::ByProductionCode(CFileItemPtr &item)
{
  if (!item) return;
  item->SetSortLabel(item->GetVideoInfoTag()->m_strProductionCode);
}

// Boxee

void SSortFileItem::ByDefault(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }
  
  item->SetSortLabel(item->GetProperty("DefaultSortLabel"));
}

void SSortFileItem::ByAppPopularity(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  item->SetSortLabel(item->GetProperty("app-popularity"));
}

void SSortFileItem::ByAppUsage(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  item->SetSortLabel(item->GetProperty("app-usage"));
}

void SSortFileItem::ByAppLastUsedDate(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  item->SetSortLabel(item->GetProperty("app-last-used"));
}

void SSortFileItem::ByAppReleaseDate(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  item->SetSortLabel(item->GetProperty("app-releasedate"));
}

void SSortFileItem::ByVideoQuality(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  item->SetSortLabel(item->GetProperty("quality"));
}

void SSortFileItem::ByReleaseDate(CFileItemPtr& item)
{
  if (!item)
  {
    return;
  }

  long releaseDateEpoch = (long) item->GetPropertyULong("releasedateVal");

  CStdString strReleaseDateEpoch = BOXEE::BXUtils::LongToString(releaseDateEpoch);

  item->SetSortLabel(strReleaseDateEpoch);
}

bool SSortFileItem::RssItems(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	
	if (left->m_bIsFolder == right->m_bIsFolder)
	{ // same category
		if (left->m_bIsFolder) {
			// Both are folder, sort by label
			return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
		}
		else 
		{
			// both are items, sort by date
			if ( left->m_dateTime < right->m_dateTime ) return false;
			if ( left->m_dateTime > right->m_dateTime ) return true;
			// dates are the same, sort by label in reverse (as default sort
			// method is descending for date, and ascending for label)
			return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
		}
	}
	return left->m_bIsFolder;
	
}

bool SSortFileItem::SearchResultPopularity(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  return (left->GetPropertyInt("searchCount") > right->GetPropertyInt("searchCount"));
  /*
  CStdString leftLabel;
  CStdString rightLabel;

  leftLabel.Format("%d", left->GetPropertyInt("searchCount"), left->GetLabel().c_str());
  rightLabel.Format("%d", right->GetPropertyInt("searchCount"), right->GetLabel().c_str());

  bool result = StringUtils::AlphaNumericCompare(leftLabel.c_str(), rightLabel.c_str()) > 0;

  CLog::Log(LOGDEBUG,"left: %s, right: %s, result: %d",leftLabel.c_str(),rightLabel.c_str(), result);

  return result;*/
}

bool SSortFileItem::EpisodesDateAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  if (!left->HasVideoInfoTag() && !right->HasVideoInfoTag())
  {
    if ( left->m_dateTime < right->m_dateTime ) return true;
    if ( left->m_dateTime > right->m_dateTime ) return false;
  }

  const CVideoInfoTag *tagLeft = left->GetVideoInfoTag();
  const CVideoInfoTag *tagRight = right->GetVideoInfoTag();

  // we calculate an offset number based on the episode's
  // sort season and episode values. in addition
  // we include specials 'episode' numbers to get proper
  // sorting of multiple specials in a row. each
  // of these are given their particular ranges to semi-ensure uniqueness.
  // theoretical problem: if a show has > 128 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^8-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  unsigned int numLeft=0, numRight=0;
  if (tagLeft->m_iSpecialSortEpisode > 0 || tagRight->m_iSpecialSortEpisode > 0)
  {
    numLeft = (tagLeft->m_iSpecialSortSeason<<24)+(tagLeft->m_iSpecialSortEpisode<<8)-(128-tagLeft->m_iEpisode);
    numRight = (tagRight->m_iSpecialSortSeason<<24)+(tagRight->m_iSpecialSortEpisode<<8)-(128-tagRight->m_iEpisode);
  }
  else if ((tagLeft->m_iSeason < 0 && tagLeft->m_iEpisode < 0) || (tagRight->m_iSeason < 0 && tagRight->m_iEpisode < 0))
  {
    if ( left->m_dateTime < right->m_dateTime ) return true;
    if ( left->m_dateTime > right->m_dateTime ) return false;
  }
  else
  {
    numLeft = (tagLeft->m_iSeason<<24)+(tagLeft->m_iEpisode<<8);
    numRight = (tagRight->m_iSeason<<24)+(tagRight->m_iEpisode<<8);
  }

  if ( numLeft < numRight )
    return true;
  else
    return false;
}

bool SSortFileItem::EpisodesDateDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  if (!left->HasVideoInfoTag() && !right->HasVideoInfoTag())
  {
    if ( left->m_dateTime < right->m_dateTime ) return false;
    if ( left->m_dateTime > right->m_dateTime ) return true;
  }

  const CVideoInfoTag *tagLeft = left->GetVideoInfoTag();
  const CVideoInfoTag *tagRight = right->GetVideoInfoTag();

  // we calculate an offset number based on the episode's
  // sort season and episode values. in addition
  // we include specials 'episode' numbers to get proper
  // sorting of multiple specials in a row. each
  // of these are given their particular ranges to semi-ensure uniqueness.
  // theoretical problem: if a show has > 128 specials and two of these are placed
  // after each other they will sort backwards. if a show has > 2^8-1 seasons
  // or if a season has > 2^16-1 episodes strange things will happen (overflow)
  unsigned int numLeft=0, numRight=0;
  if (tagLeft->m_iSpecialSortEpisode > 0 || tagRight->m_iSpecialSortEpisode > 0)
  {
    numLeft = (tagLeft->m_iSpecialSortSeason<<24)+(tagLeft->m_iSpecialSortEpisode<<8)-(128-tagLeft->m_iEpisode);
    numRight = (tagRight->m_iSpecialSortSeason<<24)+(tagRight->m_iSpecialSortEpisode<<8)-(128-tagRight->m_iEpisode);
  }
  else if ((tagLeft->m_iSeason < 0 && tagLeft->m_iEpisode < 0) || (tagRight->m_iSeason < 0 && tagRight->m_iEpisode < 0))
  {
    if ( left->m_dateTime < right->m_dateTime ) return false;
    if ( left->m_dateTime > right->m_dateTime ) return true;
  }
  else
  {
    numLeft = (tagLeft->m_iSeason<<24)+(tagLeft->m_iEpisode<<8);
    numRight = (tagRight->m_iSeason<<24)+(tagRight->m_iEpisode<<8);
  }

  if ( numLeft < numRight )
    return false;
  else
    return true;
}


bool SSortFileItem::DateAscendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	//	time_t leftTime;
	//	left->m_dateTime.GetAsTime(leftTime);
	//	time_t rightTime;
	//	right->m_dateTime.GetAsTime(rightTime);
	//CLog::Log(LOGDEBUG,"%s, compare items by date (left = %u, right = %u)", __FUNCTION__, leftTime, rightTime);
	
	// ignore the ".." item - that should always be on top
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
	{ // same category
		if (left->m_bIsFolder == right->m_bIsFolder)
		{ // same category
			if ( left->m_dateTime < right->m_dateTime ) return true;
			if ( left->m_dateTime > right->m_dateTime ) return false;
			// dates are the same, sort by label in reverse (as default sort
			// method is descending for date, and ascending for label)
			return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
		}
		return left->m_bIsFolder;
	}
	return left->GetPropertyBOOL("isshare");
}


bool SSortFileItem::DateAddedAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	time_t leftTime = left->GetPropertyULong("dateadded");
	time_t rightTime = right->GetPropertyULong("dateadded");
	
	// ignore the ".." item - that should always be on top
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
	{ // same category
		if ( leftTime < rightTime ) return true;
		if ( leftTime > rightTime ) return false;
		// dates are the same, sort by label in reverse (as default sort
		// method is descending for date, and ascending for label)
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
	}
	return left->GetPropertyBOOL("isshare");
}

bool SSortFileItem::DateAddedDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	time_t leftTime = left->GetPropertyULong("dateadded");
	time_t rightTime = right->GetPropertyULong("dateadded");
	
	//CLog::Log(LOGDEBUG,"%s, compare items by date (left = %u, right = %u)", __FUNCTION__, leftTime, rightTime);
	
	// ignore the ".." item - that should always be on top
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
	{ // same category
		if ( leftTime < rightTime ) return false;
		if ( leftTime > rightTime ) return true;
		// dates are the same, sort by label in reverse (as default sort
		// method is descending for date, and ascending for label)
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
	}
	return left->GetPropertyBOOL("isshare");
}

bool SSortFileItem::DateModifiedAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  time_t leftTime = left->GetPropertyULong("datemodified");
  time_t rightTime = right->GetPropertyULong("datemodified");

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
  { // same category
    if ( leftTime < rightTime ) return true;
    if ( leftTime > rightTime ) return false;
    // dates are the same, sort by label in reverse (as default sort
    // method is descending for date, and ascending for label)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) > 0;
  }
  return left->GetPropertyBOOL("isshare");
}

bool SSortFileItem::DateModifiedDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  time_t leftTime = left->GetPropertyULong("datemodified");
  time_t rightTime = right->GetPropertyULong("datemodified");

  //CLog::Log(LOGDEBUG,"%s, compare items by date (left = %u, right = %u)", __FUNCTION__, leftTime, rightTime);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
  { // same category
    if ( leftTime < rightTime ) return false;
    if ( leftTime > rightTime ) return true;
    // dates are the same, sort by label in reverse (as default sort
    // method is descending for date, and ascending for label)
    return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
  }
  return left->GetPropertyBOOL("isshare");
}


bool SSortFileItem::DateDescendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	time_t leftTime;
	left->m_dateTime.GetAsTime(leftTime);
	time_t rightTime;
	right->m_dateTime.GetAsTime(rightTime);
	//CLog::Log(LOGDEBUG,"%s, compare items by date (left = %u, right = %u)", __FUNCTION__, leftTime, rightTime);
	
	// ignore the ".." item - that should always be on top
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
	{ // same category
		if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare"))
		{ // same category
			if (left->m_bIsFolder == right->m_bIsFolder)
			{
				if ( left->m_dateTime < right->m_dateTime ) return false;
				if ( left->m_dateTime > right->m_dateTime ) return true;
				// dates are the same, sort by label in reverse (as default sort
				// method is descending for date, and ascending for label)
				return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(), right->GetLabel().c_str()) < 0;
			}
			return left->m_bIsFolder;
		}
	}
	return left->GetPropertyBOOL("isshare");
}

bool SSortFileItem::LabelFilesFirstAscending(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	// only if they're both folders or both files do we do the full comparison
	if (left->m_bIsFolder == right->m_bIsFolder)
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) < 0;
	return !left->m_bIsFolder;
}

bool SSortFileItem::LabelAscendingExact(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	
	return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) < 0;
}

bool SSortFileItem::LabelAscendingExactNoCase(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;

  CStdString strLeft = left->GetLabel();
  CStdString strRight = right->GetLabel();

  strLeft = strLeft.ToLower();
  strRight = strRight.ToLower();

  return StringUtils::AlphaNumericCompare(strLeft.c_str(),strRight.c_str()) < 0;
}

bool SSortFileItem::LabelAscendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	
	// only if the're both shares we do the full comparison
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare")) {
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) < 0;
	}
	return left->GetPropertyBOOL("isshare");
	
}

bool SSortFileItem::LabelAscendingNoTheExact(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	// only if they're both folders or both files do we do the full comparison
	//	if (left->m_bIsFolder == right->m_bIsFolder)
	//	{
	char *l = (char *)left->GetLabel().c_str();
	char *r = (char *)right->GetLabel().c_str();
	l += StartsWithToken(left->GetLabel());
	r += StartsWithToken(right->GetLabel());
	
	return StringUtils::AlphaNumericCompare(l, r) < 0;
	//	}
	//return left->m_bIsFolder;
}

bool SSortFileItem::LabelFilesFirstDescending(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	// only if they're both folders or both files do we do the full comparison
	if (left->m_bIsFolder == right->m_bIsFolder)
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) > 0;
	
	return !left->m_bIsFolder;
}

bool SSortFileItem::LabelDescendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	
	// only if the're both shares we do the full comparison
	if (left->GetPropertyBOOL("isshare") == right->GetPropertyBOOL("isshare")) {
		return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) > 0;
	}
	return left->GetPropertyBOOL("isshare");
}

bool SSortFileItem::LabelDescendingExact(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	
	return StringUtils::AlphaNumericCompare(left->GetLabel().c_str(),right->GetLabel().c_str()) > 0;
}

bool SSortFileItem::LabelDescendingExactNoCase(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // special items
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;

  CStdString strLeft = left->GetLabel();
  CStdString strRight = right->GetLabel();

  strLeft = strLeft.ToLower();
  strRight = strRight.ToLower();

  return StringUtils::AlphaNumericCompare(strLeft.c_str(),strRight.c_str()) > 0;
}

bool SSortFileItem::LabelDescendingNoTheExact(const CFileItemPtr &left, const CFileItemPtr &right)
{
	// sanity
	RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);
	
	// special items
	if (left->IsParentFolder()) return true;
	if (right->IsParentFolder()) return false;
	// only if they're both folders or both files do we do the full comparison
	//	if (left->m_bIsFolder == right->m_bIsFolder)
	//	{
	char *l = (char *)left->GetLabel().c_str();
	char *r = (char *)right->GetLabel().c_str();
	l += StartsWithToken(left->GetLabel());
	r += StartsWithToken(right->GetLabel());
	
	return StringUtils::AlphaNumericCompare(l, r) > 0;
	//	}
	//	return left->m_bIsFolder;
}

bool SSortFileItem::AscendingFilesFirst(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) < 0;
  return !left->m_bIsFolder;
}

bool SSortFileItem::DescendingFilesFirst(const CFileItemPtr &left, const CFileItemPtr &right)
{
  // sanity
  RETURN_IF_NULL(left,false); RETURN_IF_NULL(right,false);

  // ignore the ".." item - that should always be on top
  if (left->IsParentFolder()) return true;
  if (right->IsParentFolder()) return false;
  if (left->m_bIsFolder == right->m_bIsFolder)
    return StringUtils::AlphaNumericCompare(left->GetSortLabel().c_str(),right->GetSortLabel().c_str()) > 0;
  return !left->m_bIsFolder;
}



