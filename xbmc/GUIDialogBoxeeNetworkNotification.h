#ifndef GUIDIALOGBOXEENETWORKNOTIFICATION_H
#define GUIDIALOGBOXEENETWORKNOTIFICATION_H

#include "GUIDialogBoxBase.h"

class CGUIDialogBoxeeNetworkNotification : public CGUIDialogBoxBase
{
public:
  CGUIDialogBoxeeNetworkNotification();
  virtual ~CGUIDialogBoxeeNetworkNotification();

  virtual bool OnMessage(CGUIMessage& message);
  
  static void ShowAndGetInput(int heading, int line);
  static void ShowAndGetInput(const CStdString&  heading, const CStdString& line);

protected:

};

#endif

