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

// Basic filter interfaec
class CBrowseWindowFilter
{
public:
	CBrowseWindowFilter(int id, const CStdString& strName);
	virtual ~CBrowseWindowFilter();

	//void Activate(bool bActive) { m_bActive = bActive; }

	int GetId() { return m_iId; }
	CStdString GetName() { return m_strName; }
	//bool IsActive() { return m_bActive; }

private:
	int m_iId;
	CStdString m_strName;
	//bool m_bActive;
};

// The purpose of the local filter is to filter out items AFTER they have arrived to the browse screen
// based on their members and properties
// Once applied to a file item, the filter returns true if the item should be shown and false otherwise
class CBrowseWindowLocalFilter : public CBrowseWindowFilter
{
public:
  CBrowseWindowLocalFilter(int id, const CStdString& strName) : CBrowseWindowFilter(id, strName) {}
  virtual ~CBrowseWindowLocalFilter() {}
  virtual bool Apply(const CFileItem * pItem) = 0;
};

// The purpose of the remote filter is to provide options that should be passed as parameters to the server to filter out
// items BEFORE they arrive to the browse screen.
// Once applied, the filter appends the necessary set of options and their values that will be passed as parameters to the server
class CBrowseWindowFilterRemote : public CBrowseWindowFilter
{
public:
  CBrowseWindowFilterRemote(int id, const CStdString& strName) : CBrowseWindowFilter(id, strName) {}
  virtual bool Apply(std::map<CStdString, CStdString>& mapOptions) = 0;
};

/**
 * Defines filter that MUST HAVE a certain property
 */
class CBrowseWindowPropertyFilter : public CBrowseWindowLocalFilter
{
public:
	CBrowseWindowPropertyFilter(int id, const CStdString& strName, const CStdString& strProperty);
	virtual ~CBrowseWindowPropertyFilter() {}
	virtual bool Apply(const CFileItem * pItem);

	CStdString m_strProperty;
};

/**
 * Defines filter that check value of a certain property
 */
class CBrowseWindowPropertyValueFilter : public CBrowseWindowLocalFilter
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
class CBrowseWindowVideoGenreFilter : public CBrowseWindowLocalFilter
{
public:
	CBrowseWindowVideoGenreFilter(int id, const CStdString& strName, const CStdString& strGenre);
	virtual bool Apply(const CFileItem * pItem);

	CStdString m_strGenre;

};

class CBrowseWindowMediaItemFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowMediaItemFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowVideoFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowVideoFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowAudioFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowAudioFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowPictureFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowPictureFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem * pItem);
};

class CBrowseWindowAlbumGenreFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowAlbumGenreFilter(int id, const CStdString& strName, const CStdString& strGenreValue);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_strGenreValue;
};

class CBrowseWindowTvShowGenreFilter : public CBrowseWindowLocalFilter
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

//class CBrowseWindowFirstLetterFilter : public CBrowseWindowFilter
//{
//public:
//  CBrowseWindowFirstLetterFilter(int id, const CStdString& strName, const CStdString& strLetter);
//  virtual bool Apply(const CFileItem * pItem);
//
//  CStdString m_strLetter;
//};

class CBrowseWindowTvShowUnwatchedFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowTvShowUnwatchedFilter(int id, const CStdString& strName, bool bUnwatched);
  virtual bool Apply(const CFileItem * pItem);

  bool m_bUnwatched;
};

class CBrowseWindowLocalSourceFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowLocalSourceFilter(int id, const CStdString& strName, const CStdString& sourcePath);
  virtual bool Apply(const CFileItem * pItem);

  CStdString m_sourcePath;
};

class CBrowseWindowTvEpisodeFreeFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowTvEpisodeFreeFilter(int id, const CStdString& strName, bool bFreeOnly);
  virtual bool Apply(const CFileItem * pItem);

  bool m_bFreeOnly;
};

class CBrowseWindowAllowFilter : public CBrowseWindowLocalFilter
{
public:
  CBrowseWindowAllowFilter(int id, const CStdString& strName);
  virtual bool Apply(const CFileItem* pItem);
};

#endif /* BROWSEWINDOWFILTER_H_ */
