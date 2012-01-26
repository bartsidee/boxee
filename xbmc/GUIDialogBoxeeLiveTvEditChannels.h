#ifndef GUIDIALOGBOXEELIVETVEDITCHANNELS_H
#define GUIDIALOGBOXEELIVETVEDITCHANNELS_H

#include "system.h"

#ifdef HAS_DVB

#include "GUIDialog.h"
#include <vector>

class CGUIDialogBoxeeLiveTvEditChannels : public CGUIDialog
{
public:

  CGUIDialogBoxeeLiveTvEditChannels();
  virtual ~CGUIDialogBoxeeLiveTvEditChannels();
  virtual bool OnMessage(CGUIMessage& message);

protected:
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  bool OnAction(const CAction& action);

private:
  bool LoadChannelsList();
  bool HandleClickedChannel();
  bool HandleClickedEditChannelButton();
  bool HandleClickedEditChannelList();
  bool LoadAvailableChannelList();
  void CloseScheduleList();

  bool m_dirty;
  bool m_scheduleLoaded;
  int  m_selectedChannelItem;

  typedef std::pair<std::string, std::string> Schedule;
  static bool ScheduleComparator(const Schedule &left, const Schedule &right);
  std::vector<Schedule> m_schedules;
};


#endif

#endif // GUIDIALOGBOXEELIVETVEDITCHANNELS_H

