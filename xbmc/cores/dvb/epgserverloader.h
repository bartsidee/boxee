#pragma once

#include "system.h"

#ifdef HAS_DVB

#include "Thread.h"
#include <time.h>
#include "StdString.h"
#include <json/json.h>

class EpgNotificationListener
{
public:
  virtual ~EpgNotificationListener() {};
  virtual void OnEpgProgramsChange(uint64_t mintime, uint64_t maxtime) = 0;
};

class EpgServerLoader : public EpgNotificationListener, public CThread
{
public:
  EpgServerLoader();

  virtual void Process();
  virtual void OnEpgProgramsChange(uint64_t mintime, uint64_t maxtime) {}
  virtual void NotifyEpgVisibilityChange(bool visible);

  void RequestLoad() { m_requestLoad = true; }

private:
  bool PullEPG();
  bool PullSocial();
  std::string TimeToString(time_t t);
  time_t StringToTime(std::string str);

  bool m_epgVisible;
  bool m_visibilityChange;
  bool m_requestLoad;

  time_t m_lastFullEpgPull;
  time_t m_lastSocialPull;
};

#endif
