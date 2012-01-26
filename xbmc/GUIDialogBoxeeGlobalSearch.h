#ifndef GUIDIALOGBOXEEGLOBALSEARCH_H_
#define GUIDIALOGBOXEEGLOBALSEARCH_H_

#include "GUIDialogKeyboard.h"

class CGUIDialogBoxeeGlobalSearch : public CGUIDialogKeyboard
{

public:

  CGUIDialogBoxeeGlobalSearch(void);
  virtual ~CGUIDialogBoxeeGlobalSearch(void);

  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

  static bool ShowAndGetInput(CStdString& aTextString, const CStdString &strHeading, bool allowEmptyResult, bool hiddenInput = false);

private:

};

#endif /*GUIDIALOGBOXEEGLOBALSEARCH_H_*/

