#ifndef GUIWINDOWBOXEELIVETV_H
#define GUIWINDOWBOXEELIVETV_H

#include "system.h"

#ifdef HAS_DVB

#include "LiveTvModel.h"
#include "GUIWindow.h"
#include "FileItem.h"

class CGUIWindowBoxeeLiveTv : public CGUIWindow
{
public:
  CGUIWindowBoxeeLiveTv();
  virtual ~CGUIWindowBoxeeLiveTv();
  virtual bool OnAction(const CAction &action);
  virtual bool OnMessage(CGUIMessage &message);
  virtual void Render();
  void RenderFullScreen();

  static bool SwitchChannel(int channel_id);

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  bool SwitchRelativeChannel(int direction);

private:
  void ShowQuickOSD(); // upper part
  void ShowOSD();  // lower part
  void ShowEPG();
  void ShowInfo();
  void ShowShare();
  void RenderCodecInfo();

  CStopWatch    m_programWatchedTimer;
  CStopWatch    m_osdTimer;
#ifdef ENABLE_AUTO_CHANNEL_SWITCH
  CStopWatch    m_autoChannelSwitch;
#endif

  bool          m_reportToServer;
};


#endif

#endif // GUIWINDOWBOXEELIVETV_H

