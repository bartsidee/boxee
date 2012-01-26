#ifndef GUIDIALOGBOXEELIVETVCTX_H
#define GUIDIALOGBOXEELIVETVCTX_H

#include "system.h"

#ifdef HAS_DVB

#include "LiveTvModel.h"
#include "GUIDialog.h"
#include "FileItem.h"

class CGUIDialogBoxeeLiveTvCtx : public CGUIDialog
{
public:
  CGUIDialogBoxeeLiveTvCtx();
  virtual ~CGUIDialogBoxeeLiveTvCtx();
  virtual bool OnMessage(CGUIMessage& message);
  virtual bool OnAction(const CAction& action);
  virtual void Render();

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);

private:
  int LoadEPG();
  bool HandleClick(CGUIMessage& message);
  void HandleClickedChannel();
  bool RefreshEPG(int direction);

  void SwitchChannel(int channel_id);

  void LoadTimeSlot();

  CFileItemList m_listEPG;
  CFileItemPtr m_selectedChannel;

  CStopWatch    m_programWatchedTimer;

  LiveTvModel m_model;
};


#endif

#endif // GUIDIALOGBOXEELIVETVCTX_H

