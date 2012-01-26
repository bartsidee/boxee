#ifndef GUIWINDOWBOXEEBROWSESUBSCRIPTIONS_H_
#define GUIWINDOWBOXEEBROWSESUBSCRIPTIONS_H_

#include "GUIWindowBoxeeBrowse.h"

class CGUIWindowBoxeeBrowseSubscriptions : public CGUIWindowBoxeeBrowse
{
public:
  CGUIWindowBoxeeBrowseSubscriptions();
  virtual ~CGUIWindowBoxeeBrowseSubscriptions();

  bool OnClick(int iItem);

protected:

  /**
   * Creates the updated path that will be sent to BoxeeServerDirectory
   * according to the current state of the buttons
   */
//  virtual CStdString CreatePath();

};

#endif /*GUIWINDOWBOXEEBROWSESUBSCRIPTIONS_H_*/
