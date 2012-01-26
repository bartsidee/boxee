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
#include "SortFileItem.h"
#include "GUIBaseContainer.h"
#include "MediaSource.h"

class CViewState; // forward
class CFileItemList;

class CGUIViewState
{
public:
  virtual ~CGUIViewState();
  static CGUIViewState* GetViewState(int windowId, const CFileItemList& items);

  void SetViewAsControl(int viewAsControl);
  void SaveViewAsControl(int viewAsControl);
  int GetViewAsControl() const;

  SORT_METHOD SetNextSortMethod(int direction = 1);
  void SetCurrentSortMethod(int method);
  SORT_METHOD GetSortMethod() const;
  int GetSortMethodLabel() const;
  void GetSortMethodLabelMasks(LABEL_MASKS& masks) const;
  void GetSortMethods(std::vector< std::pair<int,int> > &sortMethods) const;

  void SetSortMethod(SORT_METHOD sortMethod);
  void SetSortOrder(SORT_ORDER sortOrder) ;

  SORT_ORDER SetNextSortOrder();
  SORT_ORDER GetSortOrder() const { return m_sortOrder; };
  SORT_ORDER GetDisplaySortOrder() const;
  virtual bool HideExtensions();
  virtual bool HideParentDirItems();
  virtual bool DisableAddSourceButtons();
  virtual int GetPlaylist();
  const CStdString& GetPlaylistDirectory();
  void SetPlaylistDirectory(const CStdString& strDirectory);
  bool IsCurrentPlaylistDirectory(const CStdString& strDirectory);
  virtual bool UnrollArchives();
  virtual bool AutoPlayNextItem();
  virtual CStdString GetLockType();
  virtual CStdString GetExtensions();
  virtual VECSOURCES& GetSources();

  virtual void SaveViewState()=0;

protected:
  //friend class CGUIMediaWindow;
  CGUIViewState(const CFileItemList& items);  // no direct object creation, use GetViewState()
  virtual void SaveViewToDb(const CStdString &path, int windowID, CViewState *viewState = NULL);
  void LoadViewState(const CStdString &path, int windowID);

  void AddSortMethod(SORT_METHOD sortMethod, int buttonLabel, LABEL_MASKS labelmasks);
  const CFileItemList& m_items;

  static VECSOURCES m_sources;

  int m_currentViewAsControl;

  std::vector<SORT_METHOD_DETAILS> m_sortMethods;
  int m_currentSortMethod;

  SORT_ORDER m_sortOrder;

  static CStdString m_strPlaylistDirectory;
};

class CGUIViewStateGeneral : public CGUIViewState
{
public:
  CGUIViewStateGeneral(const CFileItemList& items);

protected:
  virtual void SaveViewState() {};
};

class CGUIViewStateFromItems : public CGUIViewState
{
public:
  CGUIViewStateFromItems(const CFileItemList& items);

protected:
  virtual void SaveViewState();
};

class CGUIViewStateBoxeeBrowse : public CGUIViewState
{
public:
	CGUIViewStateBoxeeBrowse(const CFileItemList& items);

protected:
  virtual void SaveViewState();
  virtual void LoadViewState(const CFileItemList& items, int windowID);
};

class CGUIViewStateBoxeeBrowseVideo : public CGUIViewStateBoxeeBrowse
{
public:
	CGUIViewStateBoxeeBrowseVideo(const CFileItemList& items);

};

//class CGUIViewStateBoxeeBrowseMusic : public CGUIViewStateBoxeeBrowse
//{
//public:
//	CGUIViewStateBoxeeBrowseMusic(const CFileItemList& items);
//
//};

class CGUIViewStateBoxeeBrowsePictures : public CGUIViewStateBoxeeBrowse
{
public:
	CGUIViewStateBoxeeBrowsePictures(const CFileItemList& items);

};

class CGUIViewStateBoxeeBrowseOther : public CGUIViewStateBoxeeBrowse
{
public:
	CGUIViewStateBoxeeBrowseOther(const CFileItemList& items);

};

// Metods: Label 
// Default Sort: Label, Ascending.
// Default view: 3 rows (52) 
// Comment: 'Ignore the' flag enabled
class CGUIViewStateBoxee1 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee1(const CFileItemList& items);

};

// Metods: Date w/Shares and Label w/Shares 
// Default Sort: Label, Ascending.
// Default view: 3 rows (52)
// Comment: 'Ignore the' flag disabled
class CGUIViewStateBoxee2 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee2(const CFileItemList& items);

};

// Metods: Date w/Shares and Label w/Shares 
// Default Sort: Date, Descending.
// Default view: list w/Preview (50)
// Comment: 'Ignore the' flag disabled
class CGUIViewStateBoxee3 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee3(const CFileItemList& items);

};

// Metods: Label only 
// Default Sort: Label, Ascending.
// Default view: details (53)
// Comment: 'Ignore the' flag enabled
class CGUIViewStateBoxee4 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee4(const CFileItemList& items);

};

// Metods: Label only 
// Default Sort: Label, Ascending.
// Default view: list w/Preview (50)
// Comment: 'Ignore the' flag disabled
class CGUIViewStateBoxee5 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee5(const CFileItemList& items);

};

// Metods: Label and Date  
// Default Sort: Label, Ascending.
// Default view: 3 rows w/Preview (51)
// Comment: 'Ignore the' flag disabled
class CGUIViewStateBoxee6 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee6(const CFileItemList& items);

};

/// Metods: Date w/Shares and Label w/Shares and Episodes
// Default Sort: Episode, Ascending
// Default view: list w/Preview (50)
// Comment: 'Ignore the' flag disabled
class CGUIViewStateBoxee7 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee7(const CFileItemList& items);

};

// Metods: Label only 
// Default Sort: Label, Ascending.
// Default view: details (53)
// Comment: 'Ignore the' flag enabled
class CGUIViewStateBoxee8 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee8(const CFileItemList& items);

};

// Metods: No sort
// Default Sort: No sort
// Default view: line (50)
// Comment: 'Ignore the' flag enabled
class CGUIViewStateBoxee9 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee9(const CFileItemList& items);

};


// Metods: Label only 
// Default Sort: Label, Ascending.
// Default view: 3row w/preview (51)
// Comment: 'Ignore the' flag enabled
class CGUIViewStateBoxee10 : public CGUIViewStateBoxeeBrowse
{
public:
  CGUIViewStateBoxee10(const CFileItemList& items);

};



