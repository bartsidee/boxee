#ifndef BXTUNER_H
#define BXTUNER_H

#include "system.h"

#ifdef HAS_DVB

#include <list>

#include "dvbchannel.h"
#include "FileSystem/File.h"
#include "FileSystem/IFile.h"
#include "Thread.h"
#include <boost/shared_ptr.hpp>
#include "dvbepgloader.h"
#include "dvbpatloader.h"

using namespace XFILE;

#define TUNER_FLAGS_BUDGET      0x0001 // use PID 0x200 to stream all PIDs
#define TUNER_FLAGS_FORCE_TUNE  0x0002 // force re-tune even if using the same frequency

class IDvbHelper;
class DvbTuner;

class DvbScanner : public CThread
{
public:
  typedef enum { SCAN_NOT_RUNNING, SCAN_STARTING, SCAN_RUNNING, SCAN_FAILED, SCAN_DONE, SCAN_CANCELED } ScanState;

  DvbScanner(DvbTuner* tuner);
  virtual void Process();
  void WaitForStarted();
  void Stop(bool bWait);
  int GetCurrentFrequencyCount() { return m_currentFreqCount; }
  int GetTotalFrequencyCount() { return m_totalFreqCount; }
  bool IsInitialScanDone() { return m_initalScanDone; }
  int GetCurrentFoundFrequencyCount() { return m_currentFoundFreqCount; }
  int GetTotalFoundFrequencyCount() { return m_totalFoundFreqCount; }
  ScanState GetState();

private:
  void SetState(ScanState state);

  ScanState m_state;
  DvbTuner* m_tuner;
  int m_totalFreqCount;
  int m_currentFreqCount;
  bool m_initalScanDone;
  int m_totalFoundFreqCount;
  int m_currentFoundFreqCount;
  CCriticalSection m_statusLock;

  pthread_mutex_t m_condLock;
  pthread_cond_t m_scannerStarted;
};

class IDvbStreamListener
{
public:
  IDvbStreamListener() {}
  virtual ~IDvbStreamListener() {}

  virtual void OnScanFinished(DvbScanner::ScanState result, const CStdString& channelsFile) = 0;
};

struct DvbTunerTechnicalInfo
{
  unsigned int signal;
  unsigned int snr;
  unsigned int ber;
  unsigned int ucb;
  unsigned int fe_status;
};

class DvbTuner
{
public:
  typedef enum { TUNER_IDLE, TUNER_SCANNING, TUNER_TUNED } TunerState;
  typedef enum { TUNER_TYPE_UNINITIALIZED, TUNER_TYPE_DVBT, TUNER_TYPE_ATSC, TUNER_TYPE_UNKNOWN } TunerType; // DVBT - Europe
  typedef enum { OFDM, QAM, VSB, UNKNOWN_MODULATION } ModulationType; // OFDM -- DVB-T, QAM - cable hooked up, VSB - antenna
  typedef enum { TUNING_STATE_IDLE, TUNING_STATE_TUNING, TUNING_STATE_TUNING_PROGRAM, TUNING_STATE_TUNED, TUNING_STATE_TUNE_FAILED } TuningState;
  typedef enum { DVB_FILTER_VIDEO, DVB_FILTER_AUDIO, DVB_FILTER_OTHER } DvbFilter;

  DvbTuner(int id, IDvbEpgListener* epgListener = NULL, int flags = 0);
  virtual ~DvbTuner();

  int  GetId() const    { return m_id; }
  void SetId(int id)    { m_id = id; }

  TunerState GetTunerState() const { return m_state; }
  TuningState GetTuningState() const;
  bool IsSignalOk();

  CStdString GetCountry()  const { return m_country; }
  void SetCountry(const CStdString &cc) { m_country = cc; }

  bool Tune(const DvbChannel &channel);
  void Untune();
  CStdString GetChannel() const { return m_currChannel; }

  void AddListener(IDvbStreamListener *listener);
  void RemoveListener(IDvbStreamListener *listener);
  int  Listeners() const { return m_listeners.size(); }

  TunerType GetTunerType();
  ModulationType GetModulationType() const { return m_modulationType; }
  void SetModulationType(ModulationType type) { m_modulationType = type; }

  void ScanFinished(bool bOk);

  bool StartScan();
  void CancelScan();
  DvbScanner::ScanState GetScanState() const;
  void OnScannerDone();

  int GetCurrentFrequencyScanCount();
  int GetTotalFrequencyScanCount();
  bool IsInitialScanDone();
  int GetCurrentFoundFrequencyCount();
  int GetTotalFoundFrequencyCount();

  int Read(char *buf, int size);

  void AddPidFilter(int pid, DvbTuner::DvbFilter type);
  void RemovePidFilter(int pid);
  void RemoveAllPidFilters();

  DvbTunerTechnicalInfo GetTechnicalInfo();

  int GetTuneFlags();

protected:
  int         m_id;
  bool        m_running;
  bool        m_drvLocked;
  TunerState  m_state;
  ModulationType m_modulationType;
  CStdString     m_country;
  CStdString     m_currChannel;
  DvbEpgLoader   m_epgLoader;
  DvbPatLoader   m_patLoader;

  IDvbHelper   *m_v4l;

  pthread_mutex_t  m_condLock;
  pthread_cond_t m_threadStarted;

  pthread_mutex_t      m_listenersLock;
  std::list<IDvbStreamListener *> m_listeners;
  DvbScanner    *m_scanner;
};

typedef boost::shared_ptr<DvbTuner> DvbTunerPtr;

#endif

#endif // BXTUNER_H
