#ifndef DVBMANAGER_H
#define DVBMANAGER_H

#include "system.h"

#ifdef HAS_DVB

#include <map>
#include "dvbtuner.h"
#include "dvbchannels.h"
#include "epgserverloader.h"
#include "epgstore.h"
#include "LiveTvModel.h"

class DVBManager : public IDvbStreamListener, public IDvbEpgListener
{

public:
  static DVBManager &GetInstance();

  bool Start();
  void Stop();

  bool StartScan();
  bool CancelScan();
  bool IsScanning();
  unsigned int GetScanPercent(); // between 0-100
  DvbScanner::ScanState GetScanState();

  virtual void OnScanFinished(DvbScanner::ScanState result, const CStdString& channelsFile);
  void ClearScanner();

  DvbChannelPtr GetCurrentChannel() { return m_currentChannel; }
  void SetCurrentChannel(DvbChannelPtr channel);

  std::vector<DvbTunerPtr> GetTuners() { return m_tuners; }
  DvbChannels& GetChannels() { return m_channels; }

  bool IsTuning();
  bool IsTuned();
  bool IsTuningFailed();
  bool IsSignalOk();

  void OnDvbDongleInserted();
  void OnDvbDongleReady(int adapterId);
  void OnDvbDongleRemoved(int adapterId);

  virtual void OnEpgNewInfo(const DvbEpgInfo& info);

  bool LoadResolvedChannels();
  bool SaveResolvedChannels();
  bool ResolveChannels(CStdString providerName);

  bool IsDongleInserted() { return m_dongleInserted; }

  EpgServerLoader& GetEpgServerLoader() { return m_epgServerLoader; }
  bool ReportCurrentChannelToServerInBG();

  LiveTvModelProgram GetCurrentProgram();

protected:
  DVBManager();

  typedef std::map<int64_t, DvbEpgEvent> DvbEpgEvents;

  CCriticalSection m_lock;
  CCriticalSection m_lockCurrentProgram;
  DvbTunerPtr  m_scanningTuner; // keep a reference for the scanner to prevent it being deleted in the middle
  std::vector<DvbTunerPtr> m_tuners;
  DvbChannels m_channels;
  EpgServerLoader m_epgServerLoader;
  DvbChannelPtr m_currentChannel;
  LiveTvModelProgram m_currentProgram;
  bool m_bIsRunning;
  bool m_dongleInserted;

  time_t m_lastEpgCheckTime;
  time_t m_lastTechnicalDataCheckTime;
};

#endif

#endif // DVBMANAGER_H
