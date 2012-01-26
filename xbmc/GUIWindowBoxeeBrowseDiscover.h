#ifndef GUIWINDOWBOXEEBROWSEDISCOVER_H_
#define GUIWINDOWBOXEEBROWSEDISCOVER_H_

#include "GUIWindowBoxeeBrowse.h"

class CDiscoverWindowState : public CBrowseWindowState
{
public:
  CDiscoverWindowState(CGUIWindow* pWindow);
  virtual void SortItems(CFileItemList &items);
};

class CGUIWindowBoxeeBrowseDiscover : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseDiscover();
  virtual ~CGUIWindowBoxeeBrowseDiscover();
	
  virtual bool OnMessage(CGUIMessage& message);

protected:

	/**
	 * Creates the updated path that will be sent to BoxeeServerDirectory
	 * according to the current state of the buttons
	 */
	virtual CStdString CreatePath();

};

#endif /*GUIWINDOWBOXEEBROWSEDISCOVER_H_*/
