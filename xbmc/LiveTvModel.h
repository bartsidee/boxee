#pragma once

#include "system.h"

#ifdef HAS_DVB

#include "StdString.h"
#include <vector>
#include <map>
#include <time.h>
#include "StringUtils.h"
#include "xbmc/cores/dvb/epgstore.h"

struct LiveTvModelChannel
{
  LiveTvModelChannel() { enabled = true; }
  int id;
  CStdString number;
  CStdString label;
  bool enabled;
};

struct LiveTvModelProgram
{
  LiveTvModelProgram() { Clear(); }

  void Clear()
  {
    isNow = false;
    info.Clear();
    social.Clear();
  }

  EpgProgramInfo info;
  bool isNow;
  EpgSocialInfo social;
};

typedef std::vector<std::vector<LiveTvModelProgram> > LiveTvModelProgramsType;
typedef std::vector<LiveTvModelChannel> LiveTvModelChannelsType;

class CFileItem;

class LiveTvModel
{
public:
  LiveTvModel();

  LiveTvModelProgramsType GetPrograms();
  LiveTvModelChannelsType GetChannels();

  void SetCurrentTimeSlot(const time_t& slot);
  void GetCurrentTimeSlot(time_t& startSlot, time_t& endSlot);
  bool HasNextTimeSlot();
  bool SetNextTimeSlot();
  bool HasPrevTimeSlot();
  bool SetPrevTimeSlot();
  bool IsCurrentTimeSlotFirst();

private:
  LiveTvModelProgramsType GetProgramsForSlot(time_t slot);
  time_t CalculateEndTimeSlot(time_t& startSlot);

  time_t             m_currentStartTimeSlot;
  time_t             m_currentEndTimeSlot;
};

#endif
