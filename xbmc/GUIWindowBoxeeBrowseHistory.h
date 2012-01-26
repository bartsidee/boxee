#ifndef GUIWINDOWBOXEEBROWSEHISTORY_H_
#define GUIWINDOWBOXEEBROWSEHISTORY_H_

#include "GUIWindowBoxeeBrowse.h"

class CHistoryWindowState : public CBrowseWindowState
{
public:
  CHistoryWindowState(CGUIWindowBoxeeBrowse* pWindow);

  void OnBind(CFileItemList& items);
};


class CGUIWindowBoxeeBrowseHistory : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseHistory();
  virtual ~CGUIWindowBoxeeBrowseHistory();

  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual bool OnClick(int iItem);
};

#endif /*GUIWINDOWBOXEEBROWSEHISTORY_H_*/
