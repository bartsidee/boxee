#ifndef GUIWINDOWBOXEEBROWSEWITHPANEL_H_
#define GUIWINDOWBOXEEBROWSEWITHPANEL_H_

#include "GUIWindowBoxeeBrowse.h"

class CGUIWindowBoxeeBrowseWithPanel : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseWithPanel(DWORD dwID, const CStdString &xmlFile);
	virtual ~CGUIWindowBoxeeBrowseWithPanel();

	virtual void OnInitWindow();
	virtual void OnDeinitWindow(int nextWindowID);
	virtual bool OnMessage(CGUIMessage& message);
	virtual bool ProcessPanelMessages(CGUIMessage& message);

	void ShowPanel();

private:
	bool m_bShowPanel;

};

#endif /*GUIWINDOWBOXEEBROWSEWITHPANEL_H_*/
