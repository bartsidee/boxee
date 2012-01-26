/*
 * BrowseWindowFilter.h
 *
 *  Created on: May 20, 2008
 *      Author: alex
 */

#ifndef BROWSEWINDOWFILTER_H_
#define BROWSEWINDOWFILTER_H_

#include "StdString.h"
#include "FileItem.h"
#include <vector>

#define FILTER_ALL		0
#define FILTER_MOVIE	1
#define FILTER_TV_SHOW	2
#define FILTER_MOVIE_HD  3

#define FILTER_VIDEO 4
#define FILTER_AUDIO 5
#define FILTER_PICTURE 6

#define FILTER_APPS_VIDEO 7
#define FILTER_APPS_MUSIC 8
#define FILTER_APPS_PICTURES 9
#define FILTER_APPS_GENERAL 10

#define FILTER_FOLDER_MEDIA_ITEM     49

//Genre -> Action, Adventure, Animation, Biography, Comedy, Crime,
//Documentary, Drama, Family, Fantasy, Film-Noir, Game Show, History,
//Horror, Music, Musical, Mystery, News, Reality-TV, Romance, Sci-Fi,
//Short, Sport, Talk-Show, Thriller, War, Western
#define FILTER_MOVIE_GENRE_ACTION     50
#define FILTER_MOVIE_GENRE_ADVENTURE  51
#define FILTER_MOVIE_GENRE_ANIMATION  52
#define FILTER_MOVIE_GENRE_BIOGRAPHY  53
#define FILTER_MOVIE_GENRE_COMEDY     54
#define FILTER_MOVIE_GENRE_CRIME      55
#define FILTER_MOVIE_GENRE_DOCUMENTARY  56
#define FILTER_MOVIE_GENRE_DRAMA      57
#define FILTER_MOVIE_GENRE_FAMILY     58
#define FILTER_MOVIE_GENRE_FANTASY    59
#define FILTER_MOVIE_GENRE_FILM_NOIR  60
#define FILTER_MOVIE_GENRE_GAME_SHOW  61
#define FILTER_MOVIE_GENRE_HISTORY    62
#define FILTER_MOVIE_GENRE_HORROR     63
#define FILTER_MOVIE_GENRE_MUSIC      64
#define FILTER_MOVIE_GENRE_MUSICAL    65
#define FILTER_MOVIE_GENRE_MYSTERY    66
#define FILTER_MOVIE_GENRE_NEWS       67
#define FILTER_MOVIE_GENRE_REALITY    68
#define FILTER_MOVIE_GENRE_ROMANCE    69
#define FILTER_MOVIE_GENRE_SCI_FI     70
#define FILTER_MOVIE_GENRE_SHORT      71
#define FILTER_MOVIE_GENRE_SPORT      72
#define FILTER_MOVIE_GENRE_TALK_SHOW  73
#define FILTER_MOVIE_GENRE_THRILLER   74
#define FILTER_MOVIE_GENRE_WAR        75
#define FILTER_MOVIE_GENRE_WESTERN    76

// Release Year -> This Year, Last Year, 00's, 90's, 80's, 70's, 60's, 50's
#define FILTER_MOVIE_RELEASE_00       90
#define FILTER_MOVIE_RELEASE_90       91
#define FILTER_MOVIE_RELEASE_80       92
#define FILTER_MOVIE_RELEASE_70       93
#define FILTER_MOVIE_RELEASE_60       94
#define FILTER_MOVIE_RELEASE_50       95
#define FILTER_MOVIE_RELEASE_THIS_YEAR 96
#define FILTER_MOVIE_RELEASE_LAST_YEAR 97

//Rating -> *, **, ***, ****, *****
#define FILTER_MOVIE_RATING_ONE       101
#define FILTER_MOVIE_RATING_TWO       102
#define FILTER_MOVIE_RATING_THREE     103
#define FILTER_MOVIE_RATING_FOUR      104
#define FILTER_MOVIE_RATING_FIVE      105

//MPAA Rating –> G, PG, PG-13, R, NC-17, X
#define FILTER_MOVIE_RATING_MPAA_G    111
#define FILTER_MOVIE_RATING_MPAA_PG   112
#define FILTER_MOVIE_RATING_MPAA_PG13 113
#define FILTER_MOVIE_RATING_MPAA_R    114
#define FILTER_MOVIE_RATING_MPAA_NC17 115
#define FILTER_MOVIE_RATING_MPAA_X    116
#define FILTER_MOVIE_GENRE            120
#define FILTER_ALBUM_GENRE            121

#define FILTER_MAX		500

// Container (category) for filters
// Limited to two level nesting
// Used for groups of filters such as Genre (which holds specific filters for individual genres)
class CBrowseWindowFilterContainer
{
public:
	//CBrowseWindowFilterContainer() {}
	CBrowseWindowFilterContainer(int iFilterId) {m_iFilterId = iFilterId;}
	CStdString m_strName;
	int m_iFilterId;

	std::vector<int> m_filters;
};

class CBrowseWindowFilter
{
public:
	int m_iId;
	CStdString m_strName;

	CBrowseWindowFilter();
	CBrowseWindowFilter(int id, const CStdString& strName);
	virtual ~CBrowseWindowFilter();

	// Base implementation, by default all items are visible
	virtual bool Apply(const CFileItem * pItem) {return true;}



};

/**
 * Defines filter that MUST HAVE a certain property
 */
class CBrowseWindowPropertyFilter : public CBrowseWindowFilter
{
public:
	CBrowseWindowPropertyFilter(int id, const CStdString& strName, const CStdString& strProperty);
	virtual bool Apply(const CFileItem * pItem);

	CStdString m_strProperty;
};

/**
 * Defines filter that check value of a certain property
 */
class CBrowseWindowPropertyValueFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowPropertyValueFilter(int id, const CStdString& strName, const CStdString& strProperty, const CStdString& strPropertyValue);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strProperty;
  CStdString m_strPropertyValue;
};

/**
 * Defines filter that checks the genre of an video item
 */
class CBrowseWindowVideoGenreFilter : public CBrowseWindowFilter
{
public:
	CBrowseWindowVideoGenreFilter(int id, const CStdString& strName, const CStdString& strGenre);
	virtual bool Apply(const CFileItem * pItem);

	CStdString m_strGenre;

};

class CBrowseWindowMediaItemFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowMediaItemFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowVideoFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowVideoFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowAudioFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowAudioFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowPictureFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowPictureFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowAlbumGenreFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowAlbumGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strGenreValue;
};

class CBrowseWindowTvShowGenreFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowTvShowGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strGenreValue;
};

class CBrowseWindowTvShowSourceFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowTvShowSourceFilter(int id, const CStdString& strName, const CStdString& strSourceValue);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strSourceValue;
};

class CBrowseWindowFirstLetterFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowFirstLetterFilter(int id, const CStdString& strName, const CStdString& strLetter);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strLetter;
};

class CBrowseWindowTvShowUnwatchedFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowTvShowUnwatchedFilter(int id, const CStdString& strName, bool bUnwatched);
  virtual bool Apply(const CFileItem * pItem);

  bool m_bUnwatched;
};

class CBrowseWindowTvEpisodeFreeFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowTvEpisodeFreeFilter(int id, const CStdString& strName, bool bFreeOnly);
  virtual bool Apply(const CFileItem * pItem);

  bool m_bFreeOnly;
};

class CBrowseWindowAllowFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowAllowFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem* pItem);
};

#endif /* BROWSEWINDOWFILTER_H_ */
