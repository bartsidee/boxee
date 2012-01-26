#ifndef GUIWINDOWBOXEEBROWSEHISTORY_H_
#define GUIWINDOWBOXEEBROWSEHISTORY_H_

#include "GUIWindowBoxeeBrowse.h"

class CHistoryWindowState : public CBrowseWindowState
{
public:
  CHistoryWindowState(CGUIWindow* pWindow);
  virtual void SortItems(CFileItemList &items);
};


class CGUIWindowBoxeeBrowseHistory : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseHistory();
	virtual ~CGUIWindowBoxeeBrowseHistory();

protected:

	virtual CStdString CreatePath();
	virtual bool OnClick(int iItem);

};

#endif /*GUIWINDOWBOXEEBROWSEHISTORY_H_*/
