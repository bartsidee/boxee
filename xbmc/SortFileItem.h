#pragma once
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

#include "utils/LabelFormatter.h"
#include <boost/shared_ptr.hpp>

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;

struct SSortFileItem
{
  // Sort by sort field
  static bool Ascending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool Descending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool IgnoreFoldersAscending(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool IgnoreFoldersDescending(const CFileItemPtr &left, const CFileItemPtr &right);

  // Fill in sort field
  static void ByLabel(CFileItemPtr &item);
  static void ByLabelNoThe(CFileItemPtr &item);
  static void ByFile(CFileItemPtr &item);
  static void ByFullPath(CFileItemPtr &item);
  static void ByDate(CFileItemPtr &item);
  static void BySize(CFileItemPtr &item);
  static void ByDriveType(CFileItemPtr &item);
  static void BySongTitle(CFileItemPtr &item);
  static void BySongTitleNoThe(CFileItemPtr &item);
  static void BySongAlbum(CFileItemPtr &item);
  static void BySongAlbumNoThe(CFileItemPtr &item);
  static void BySongArtist(CFileItemPtr &item);
  static void BySongArtistNoThe(CFileItemPtr &item);
  static void BySongTrackNum(CFileItemPtr &item);
  static void BySongDuration(CFileItemPtr &item);
  static void BySongRating(CFileItemPtr &item);
  static void ByLinkTitle(CFileItemPtr& item);

  static void ByProgramCount(CFileItemPtr &item);

  static void ByGenre(CFileItemPtr &item);
  static void ByYear(CFileItemPtr &item);

  static void ByMovieTitle(CFileItemPtr &item);
  static void ByMovieSortTitle(CFileItemPtr &item);
  static void ByMovieSortTitleNoThe(CFileItemPtr &item);
  static void ByMovieRating(CFileItemPtr &item);
  static void ByMovieRuntime(CFileItemPtr &item);
  static void ByMPAARating(CFileItemPtr &item);
  static void ByStudio(CFileItemPtr &item);
  static void ByStudioNoThe(CFileItemPtr &item);

  static void ByEpisodeNum(CFileItemPtr &item);
  static void ByProductionCode(CFileItemPtr &item);
	
  //Boxee
	
  static void ByDefault(CFileItemPtr& item);
  
  static void ByAppPopularity(CFileItemPtr& item);
  static void ByAppReleaseDate(CFileItemPtr& item);
  static void ByAppUsage(CFileItemPtr& item);
  static void ByAppLastUsedDate(CFileItemPtr& item);
  static void ByVideoQuality(CFileItemPtr& item);
  static void ByReleaseDate(CFileItemPtr& item);


  static bool AscendingFilesFirst(const CFileItemPtr &left, const CFileItemPtr &right);
  static bool DescendingFilesFirst(const CFileItemPtr &left, const CFileItemPtr &right);
  
	// Sorts by label, files before folders
	static bool LabelFilesFirstAscending(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool LabelFilesFirstDescending(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sort by label (exact) without regard to files and folders, "The" prefix ignored
	static bool LabelAscendingNoTheExact(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool LabelDescendingNoTheExact(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sort by label (exact) without regard to files and folders
	static bool LabelAscendingExact(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool LabelDescendingExact(const CFileItemPtr &left, const CFileItemPtr &right);

	// Sort by label (exact) without regard to files and folders
	static bool LabelAscendingExactNoCase(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool LabelDescendingExactNoCase(const CFileItemPtr &left, const CFileItemPtr &right);

	// Sort by label, putting the shares first
	static bool LabelAscendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool LabelDescendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sort by date, with shares first, followed by folders
	static bool DateAscendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool DateDescendingWithShares(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sort by date added, with shares first, followed by folders
	static bool DateAddedAscending(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool DateAddedDescending(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sort by date modified, with shares first, followed by folders
	static bool DateModifiedAscending(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool DateModifiedDescending(const CFileItemPtr &left, const CFileItemPtr &right);

	// Sorts folders by label ascending and item by date descending
	static bool RssItems(const CFileItemPtr &left, const CFileItemPtr &right);
	
	// Sorts search result items by their search count and label
	static bool SearchResultPopularity(const CFileItemPtr &left, const CFileItemPtr &right);

	// Sorts episodes
	static bool EpisodesDateAscending(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool EpisodesDateDescending(const CFileItemPtr &left, const CFileItemPtr &right);

	// sort shows and movies release date
	static bool ReleaseDateAscending(const CFileItemPtr &left, const CFileItemPtr &right);
	static bool ReleaseDateDescending(const CFileItemPtr &left, const CFileItemPtr &right);


  //Boxee
};

typedef enum {
  SORT_METHOD_NONE=0,
  SORT_METHOD_LABEL,
  SORT_METHOD_LABEL_IGNORE_THE,
  SORT_METHOD_DATE,
  SORT_METHOD_SIZE,
  SORT_METHOD_FILE,
  SORT_METHOD_DRIVE_TYPE,
  SORT_METHOD_TRACKNUM,
  SORT_METHOD_DURATION,
  SORT_METHOD_TITLE,
  SORT_METHOD_TITLE_IGNORE_THE,
  SORT_METHOD_ARTIST,
  SORT_METHOD_ARTIST_IGNORE_THE,
  SORT_METHOD_ALBUM,
  SORT_METHOD_ALBUM_IGNORE_THE,
  SORT_METHOD_GENRE,
  SORT_METHOD_YEAR,
  SORT_METHOD_VIDEO_RATING,
  SORT_METHOD_PROGRAM_COUNT,
  SORT_METHOD_PLAYLIST_ORDER,
  SORT_METHOD_EPISODE,
  SORT_METHOD_VIDEO_TITLE,
  SORT_METHOD_VIDEO_SORT_TITLE,
  SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE,
  SORT_METHOD_PRODUCTIONCODE,
  SORT_METHOD_SONG_RATING,
  SORT_METHOD_MPAA_RATING,
  SORT_METHOD_VIDEO_RUNTIME,
  SORT_METHOD_STUDIO,
  SORT_METHOD_STUDIO_IGNORE_THE,
  SORT_METHOD_FULLPATH,
  SORT_METHOD_LABEL_IGNORE_FOLDERS,
  SORT_METHOD_LASTPLAYED,
  SORT_METHOD_LISTENERS,
  SORT_METHOD_UNSORTED,
  //Boxee
  SORT_METHOD_LABEL_FILES_FIRST,
  SORT_METHOD_LABEL_IGNORE_THE_EXACT, //30
  SORT_METHOD_LABEL_EXACT,
  SORT_METHOD_LABEL_WITH_SHARES, //32
  SORT_METHOD_DATE_WITH_SHARES, // 33
  SORT_METHOD_DATE_ADDED, // 34
  SORT_METHOD_RSS_ITEMS, // 35
  SORT_METHOD_DEFAULT, // 36
  SORT_METHOD_APP_POPULARITY, // 37
  SORT_METHOD_APP_RELEASE_DATE, // 38
  SORT_METHOD_DATE_MODIFIED, // 39
  SORT_METHOD_APP_USAGE, // 40
  SORT_METHOD_APP_LAST_USED_DATE, //41
  SORT_METHOD_VIDEO_QUALITY,
  SORT_METHOD_RELEASE_DATE,
  SORT_METHOD_SEARCH_COUNT,
  SORT_METHOD_LINK_TITLE,
  //end Boxee
	
  SORT_METHOD_MAX,
} SORT_METHOD;

typedef enum {
  SORT_ORDER_NONE=0,
  SORT_ORDER_ASC,
  SORT_ORDER_DESC
} SORT_ORDER;

typedef struct
{
  SORT_METHOD m_sortMethod;
  int m_buttonLabel;
  LABEL_MASKS m_labelMasks;
} SORT_METHOD_DETAILS;

class CBoxeeView
{
public:

  CStdString m_id;
  int m_type;
};

class CBoxeeSort
{
 public:
  CBoxeeSort()
  {
    Reset();
  }

  CBoxeeSort(CStdString id, SORT_METHOD sortMethod, SORT_ORDER sortOrder, CStdString sortName, CStdString folderPosition)
  {
    m_id = id;
    m_sortMethod = sortMethod;
    m_sortOrder = sortOrder;
    m_sortName = sortName;
    m_folderPosition = folderPosition;
  }

  void Reset()
  {
    m_id = "";
    m_sortMethod = SORT_METHOD_NONE;
    m_sortOrder = SORT_ORDER_NONE;
  }

  CStdString m_id;
  SORT_METHOD m_sortMethod;
  SORT_ORDER m_sortOrder;
  CStdString m_sortName;
  CStdString m_folderPosition;
};

class CBoxeeViewState
{
 public:
   
   CBoxeeViewState(CBoxeeView boxeeView,CBoxeeSort boxeeSort,time_t changeTime)
  {
    m_boxeeView = boxeeView;
    m_boxeeSort = boxeeSort;
    m_changeTime = changeTime;    
  };
 
   CBoxeeViewState()
  {
    m_boxeeView.m_id = "1";
    m_boxeeView.m_type = 50;

    m_boxeeSort.m_id = "1";
    m_boxeeSort.m_sortMethod = SORT_METHOD_LABEL;  
    m_boxeeSort.m_sortOrder = SORT_ORDER_ASC;
    m_boxeeSort.m_sortName = "Name";
    m_boxeeSort.m_folderPosition = "start";

    m_changeTime = 0;
  };

  CBoxeeView m_boxeeView;
  CBoxeeSort m_boxeeSort;
  time_t m_changeTime;
};


