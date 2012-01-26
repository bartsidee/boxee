#ifndef GUIWINDOWBOXEEBROWSEPHOTOS_H_
#define GUIWINDOWBOXEEBROWSEPHOTOS_H_

#include "GUIWindowBoxeeBrowseLocal.h"
#include "GUIWindowBoxeeBrowseWithPanel.h"

class CPhotosWindowState : public CLocalBrowseWindowState
{
public:
  CPhotosWindowState(CGUIWindow* pWindow);

};

class CGUIWindowBoxeeBrowsePhotos : public CGUIWindowBoxeeBrowseWithPanel
{
public:
  CGUIWindowBoxeeBrowsePhotos();
  virtual ~CGUIWindowBoxeeBrowsePhotos();

  virtual void OnInitWindow();
  virtual bool OnBind(CGUIMessage& message);
  virtual bool ProcessPanelMessages(CGUIMessage& message);

private:

  void UpdateShortcutButton();

};

#endif /*GUIWINDOWBOXEEBROWSEPHOTOS_H_*/
