
#ifndef GUIDIALOGBOXEERSSFEEDINFO_H_
#define GUIDIALOGBOXEERSSFEEDINFO_H_

#include "GUIDialog.h"

class CGUIDialogBoxeeRssFeedInfo : public CGUIDialog 
{
public:
  CGUIDialogBoxeeRssFeedInfo();
  virtual ~CGUIDialogBoxeeRssFeedInfo();
  
  bool OnAction(const CAction &action);
  bool OnMessage(CGUIMessage& message);
};

#endif /* GUIDIALOGBOXEERSSFEEDINFO_H_ */
