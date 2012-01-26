#ifndef BOXEEWINDOWFTUBACKGROUND_H
#define BOXEEWINDOWFTUBACKGROUND_H

#include "GUIWindow.h"

class CGUIWindowFirstTimeUseBackground : public CGUIWindow
{
public:
  CGUIWindowFirstTimeUseBackground(void);
  virtual ~CGUIWindowFirstTimeUseBackground(void);

  virtual bool OnAction(const CAction &action);

protected:

};

#endif
