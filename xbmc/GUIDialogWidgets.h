#ifndef CGUIDIALOGWIDGETS_H_
#define CGUIDIALOGWIDGETS_H_

#include "GUIDialog.h"

class CGUIDialogWidgets : public CGUIDialog
{
public:
	CGUIDialogWidgets();
	virtual ~CGUIDialogWidgets();
	
	virtual void OnInitWindow();
	virtual void Render();
	virtual bool OnMessage(CGUIMessage& message);
	virtual bool OnAction(const CAction &action);
	virtual void OnWindowLoaded();
	virtual void OnWindowUnload();
};

#endif /*CGUIDIALOGWIDGETS_H_*/
