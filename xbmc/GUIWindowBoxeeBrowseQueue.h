#ifndef GUIWINDOWBOXEEBROWSEQUEUE_H_
#define GUIWINDOWBOXEEBROWSEQUEUE_H_

#include "GUIWindowBoxeeBrowse.h"

class CQueueWindowState : public CBrowseWindowState
{
public:
  CQueueWindowState(CGUIWindow* pWindow);
  virtual void SortItems(CFileItemList &items);
};

class CGUIWindowBoxeeBrowseQueue : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseQueue();
	virtual ~CGUIWindowBoxeeBrowseQueue();

	virtual bool OnMessage(CGUIMessage& message);

protected:

	/**
	 * Creates the updated path that will be sent to BoxeeServerDirectory
	 * according to the current state of the buttons
	 */
	virtual CStdString CreatePath();

  virtual void SortItems(CFileItemList &items);

};

#endif /*GUIWINDOWBOXEEBROWSEQUEUE_H_*/
