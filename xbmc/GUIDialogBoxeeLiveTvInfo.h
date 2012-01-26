#ifndef GUIDIALOGBOXEELIVETVINFO_H
#define GUIDIALOGBOXEELIVETVINFO_H

#include "system.h"

#ifdef HAS_DVB

#include "LiveTvModel.h"
#include "GUIDialog.h"
#include "FileItem.h"

class CGUIDialogBoxeeLiveTvInfo : public CGUIDialog
{
public:
  CGUIDialogBoxeeLiveTvInfo();
  void SetProgram(CFileItemPtr item);

  virtual ~CGUIDialogBoxeeLiveTvInfo();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  bool IsRequestTune() { return m_requestTune; }

protected:
  virtual void OnInitWindow();

  CFileItemPtr m_item;
  bool m_requestTune;
};


#endif

#endif // GUIDIALOGBOXEELIVETVINFO_H
