/*
* All Rights Reserved, Boxee.tv
*/

#ifndef V4LHELPER_H
#define V4LHELPER_H

#include "system.h"

#ifdef HAS_DVB

#include "dvbchannel.h"
#include "dvbtuner.h"
#include <map>

#if defined(_LINUX) && !defined(__APPLE__)
#include <linux/dvb/frontend.h>
#endif

class IDvbHelper
{
public:
  IDvbHelper(int tunerId, int flags = 0) { };
  virtual ~IDvbHelper() { };

  virtual bool Tune(const DvbChannel &channel) { return true; }
  virtual bool Stop() { return true; }
  virtual DvbTuner::TunerType GetTunerType(int tunerId) { return DvbTuner::TUNER_TYPE_UNKNOWN; }
  virtual DvbTuner::TuningState GetTuningState() { return DvbTuner::TUNING_STATE_IDLE; }
  virtual bool IsSignalOk() { return false; }
  virtual int Read(char * buf, int size) { return -1; }
  virtual void AddPidFilter(int pid, DvbTuner::DvbFilter type) { }
  virtual void RemovePidFilter(int pid) { }
  virtual DvbTunerTechnicalInfo GetTechnicalInfo() { return DvbTunerTechnicalInfo(); }
  virtual int GetFlags() { return 0; }
  virtual void RemoveAllPidFilters() {}
};

#if defined(_LINUX) && !defined(__APPLE__)
class V4LHelper : public IDvbHelper
{
public:
  V4LHelper(int tunerId, int flags = 0);
  virtual ~V4LHelper();

  virtual bool Tune(const DvbChannel &channel);
  virtual bool Stop();
  virtual DvbTuner::TunerType GetTunerType(int tunerId);
  virtual DvbTuner::TuningState GetTuningState();
  virtual int Read(char * buf, int size);
  virtual bool IsSignalOk();

  virtual void AddPidFilter(int pid, DvbTuner::DvbFilter type);
  virtual void RemovePidFilter(int pid);
  virtual void RemoveAllPidFilters();

  virtual DvbTunerTechnicalInfo GetTechnicalInfo();
  virtual int GetFlags() { return m_nFlags; }

protected:
  bool OpenDVR();
  void CloseDVR();

  bool OpenFrontEnd();
  void CloseFrontEnd();


  void CloseAll();

  bool IsTuneDone(int tmout);
  bool Tune(struct dvb_frontend_parameters& frontend);

  uint16_t GetSignal();
  uint16_t GetSNR();
  uint16_t GetBER();
  uint16_t GetUCB();

  void ClosePID(int pid);

  std::map<int, int> m_pidHandles;
  int         m_frontendHandle;
  int         m_dvr;
  int         m_tunerId;

  CStdString  m_lastFreq;

  CCriticalSection m_lock;
  DvbTuner::TunerType m_type;
  std::map<CStdString, int> m_trValue; //  translate the string in channels.conf to v4l value

  DvbTuner::TuningState m_tuningState;
  bool m_signalOk;
  int m_lastSignalCheckTicks;
  struct dvb_frontend_parameters m_frontend_settings;

  int m_nFlags;
};

#endif

#endif

#endif // V4LHELPER_H
