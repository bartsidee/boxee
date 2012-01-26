#include "dvbmanager.h"

#ifdef HAS_DVB

#include <boost/foreach.hpp>

#include "Util.h"
#include "utils/log.h"
#include "Directory.h"
#include "Application.h"
#include "LocalizeStrings.h"
#include "SingleLock.h"
#include "epgstore.h"
#include "GUISettings.h"

#include "json/value.h"
#include "json/reader.h"
#include "json/writer.h"

#include "bxconfiguration.h"
#include "bxutils.h"

#include "../lib/libBoxee/bxutils.h"
#include "../lib/libBoxee/bxconfiguration.h"
#include "HalServices.h"
#include "LiveTvModel.h"

using namespace BOXEE;

#define USE_BUDGET_MODE                 // stream using PID 0x200
//#define FORCE_TUNE_ON_CHANNEL_CHANGE    // force re-tuning even if frequencies are the same

DVBManager::DVBManager()
{
  m_bIsRunning = false;
  m_dongleInserted = false;
  m_lastEpgCheckTime = 0;
}

DVBManager &DVBManager::GetInstance()
{
  static DVBManager instance;
  return instance;
}

void DVBManager::ClearScanner()
{
  CLog::Log(LOGDEBUG, "DVBManager: clearing scanner...");
  m_scanningTuner.reset();
}

bool DVBManager::Start()
{
  if(m_bIsRunning)
    return true;

  CSingleLock lock(m_lock);

  std::vector<CHalDvbDevice> dvbDevices;
  if (CHalServicesFactory::GetInstance().GetAllDvbDevices(dvbDevices))
  {
    if (dvbDevices.size() > 0)
    {
      foreach (CHalDvbDevice device, dvbDevices)
      {
        if (device.ready)
          OnDvbDongleReady(device.instance);
        else if (device.connected)
          OnDvbDongleInserted();
      }
    }
  }

#if defined(_LINUX) && !defined(HAS_EMBEDDED)
  if (CFile::Exists("/dev/dvb/adapter0/frontend0"))
  {
    OnDvbDongleReady(0);
  }
#endif

#if 0
  // for testing purposes
  OnDvbDongleReady(0);
#endif

#ifdef HAS_SERVER_OTA
  if (g_guiSettings.GetBool("ota.scanned"))
  {
    m_channels.LoadChannelsFromServer();

    // For testing purposes uncomment the following to use the channels.conf
    //m_channels.Clear();

    if (!m_channels.HasChannels())
    {
      CLog::Log(LOGWARNING, "No channels found on server. Re-submitting.");

      // Oops, no channels on the server. Let's try to load the scanned channels and
      // store them on the server.
      if (!m_channels.LoadScannedChannels())
      {
        CLog::Log(LOGERROR, "No local channels found. The user will need to re-scan");

        g_guiSettings.SetBool("ota.scanned", false);
        g_settings.Save();

        return false;
      }

      CLog::Log(LOGINFO, "Loaded %d local channels", (int) m_channels.Size());

      if (!m_channels.SaveChannelsToServer())
      {
        CLog::Log(LOGERROR, "Error saving at server");
        m_channels.Clear();
        return false;
      }
    }

    CLog::Log(LOGINFO, "Loaded %d server channels", (int) m_channels.Size());
  }

  m_epgServerLoader.Create();
#else
  if (g_guiSettings.GetBool("ota.scanned"))
  {
    m_channels.LoadScannedChannels();
  }
#endif

  if (m_channels.Size() > 0)
  {
    SetCurrentChannel(m_channels.GetChannelByIndex(0));
  }

  m_bIsRunning = true;

  return true;
}

void DVBManager::OnDvbDongleInserted()
{
  CSingleLock lock(m_lock);

  m_dongleInserted = true;
}

void DVBManager::OnDvbDongleReady(int adapterId)
{
  CSingleLock lock(m_lock);

  // add new tuner
  bool found = false;

  foreach (DvbTunerPtr tuner, m_tuners)
  {
    if (tuner->GetId() == adapterId)
    {
      found = true;
      break;
    }
  }

  int flags = 0;

#ifdef USE_BUDGET_MODE
  flags |= TUNER_FLAGS_BUDGET;
#endif

#ifdef FORCE_TUNE_ON_CHANNEL_CHANGE
  flags |= TUNER_FLAGS_FORCE_TUNE;
#endif

  if (!found)
  {
    DvbTunerPtr tuner(new DvbTuner(adapterId, this, flags));
    m_tuners.push_back(tuner);
  }

  m_dongleInserted = true;
}

void DVBManager::OnDvbDongleRemoved(int adapterId)
{
  for(std::vector<DvbTunerPtr>::iterator it = m_tuners.begin(); it != m_tuners.end(); )
  {
    DvbTunerPtr tuner = (*it);
    int id = tuner->GetId();
    if (id == adapterId)
    {
      it = m_tuners.erase(it);
    }
    else
    {
      ++it;
    }
  }

  m_dongleInserted = false;
}

void DVBManager::Stop()
{
  CSingleLock lock(m_lock);
  m_tuners.clear();
  m_channels.Clear();
  m_epgServerLoader.StopThread();

  m_bIsRunning = false;
}

bool DVBManager::StartScan()
{
  CSingleLock lock(m_lock);

  foreach (DvbTunerPtr tuner, m_tuners)
  {
    if (tuner->GetTunerState() == DvbTuner::TUNER_IDLE)
    {
      m_scanningTuner = tuner; // hold a reference until scan is over
      tuner->SetCountry(g_guiSettings.GetString("ota.countrycode"));
      if (g_guiSettings.GetString("ota.countrycode") == "US" ||
          g_guiSettings.GetString("ota.countrycode") == "CA")
      {
        if (g_guiSettings.GetBool("ota.selectedcable"))
          tuner->SetModulationType(DvbTuner::QAM);
        else
          tuner->SetModulationType(DvbTuner::VSB);
      }
      else if (tuner->GetTunerType() == DvbTuner::TUNER_TYPE_ATSC)
      {
        tuner->SetModulationType(DvbTuner::VSB);
      }
      else
      {
        tuner->SetModulationType(DvbTuner::OFDM);
      }
      tuner->AddListener(this);
      tuner->StartScan();
      return true;
    }
  }

  return false;
}

bool DVBManager::CancelScan()
{
  if (!IsScanning())
    return true;

  m_scanningTuner->CancelScan();

  return true;
}

bool DVBManager::IsScanning()
{
  return (m_scanningTuner.use_count() > 0);
}

void DVBManager::OnScanFinished(DvbScanner::ScanState result, const CStdString& channelsFile)
{
  CLog::Log(LOGINFO, "DVBManager::OnScanFinished: result=%d", result);

  m_scanningTuner->RemoveListener(this);

  // In case of invalid scan..
  if (result != DvbScanner::SCAN_DONE)
  {
    return;
  }

  // When scanning complete, load channels and show a message
  m_channels.LoadScannedChannels();

#ifdef HAS_SERVER_OTA
  if (!m_channels.SaveChannelsToServer())
  {
    CLog::Log(LOGERROR, "Error storing at server");
    return;
  }
#endif

  if (m_channels.Size() > 0)
  {
    SetCurrentChannel(m_channels.GetChannelByIndex(0));
    m_epgServerLoader.RequestLoad();

    g_guiSettings.SetBool("ota.scanned", true);
    g_settings.Save();
  }
}

unsigned int DVBManager::GetScanPercent()
{
  // If not scanning, it's empty
  if (!IsScanning())
  {
    return 0;
  }

  // If scanning is done, show progress bar full
  if (m_scanningTuner->GetScanState() == DvbScanner::SCAN_DONE)
  {
    return 100;
  }

  // Scanning that we can count will be up to 80%. The remaining 20% is for getting the channel names, and we can't know it in advance.
  int percent = (int) (((float) m_scanningTuner->GetCurrentFrequencyScanCount()) / m_scanningTuner->GetTotalFrequencyScanCount() * 50.0f);

  if (m_scanningTuner->IsInitialScanDone())
  {
    percent += (int) (((float) m_scanningTuner->GetCurrentFoundFrequencyCount()) / m_scanningTuner->GetTotalFoundFrequencyCount() * 50.0f);
  }

  return percent;
}

DvbScanner::ScanState DVBManager::GetScanState()
{
  if (m_scanningTuner.get() == NULL)
  {
    return DvbScanner::SCAN_NOT_RUNNING;
  }

  return m_scanningTuner->GetScanState();
}

bool DVBManager::IsTuned()
{
  if (m_tuners.size() == 0)
    return false;

  bool result = (m_tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNED);
//  printf("DVBManager::IsTuned: %d state is %d \n", result, (int) m_tuners[0]->GetTuningState());
  return result;
}

bool DVBManager::IsTuning()
{
  if (m_tuners.size() == 0)
    return false;

  bool result = (m_tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNING ||
      m_tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNING_PROGRAM);
//  printf("DVBManager::IsTuning: %d\n", result);
  return result;
}

bool DVBManager::IsTuningFailed()
{
  if (m_tuners.size() == 0)
    return false;

  bool result = (m_tuners[0]->GetTuningState() == DvbTuner::TUNING_STATE_TUNE_FAILED);
//  printf("DVBManager::IsTuningFailed: %d\n", result);
  return result;
}

bool DVBManager::IsSignalOk()
{
  if (m_tuners.size() == 0)
    return false;

  bool result = (m_tuners[0]->IsSignalOk());
//  printf("DVBManager::IsSignalOk: %d\n", result);
  return result;
}

void DVBManager::OnEpgNewInfo(const DvbEpgInfo& info)
{
  /* Find the channel for this epg data */
  CStdString channelId;
  DvbChannelPtr ch = m_channels.GetChannelByDvbTriplet(info.networkId, info.tsId, info.serviceId);
  if (ch.get() == NULL)
  {
    // We cannot find by dvb triplet. let's try by service id only...
    ch = m_channels.GetChannel(info.serviceId);
    if (ch.get() == NULL)
    {
      CLog::Log(LOGERROR, "OnEpgNewInfo: cannot find channel for epg info: on=%d s=%d ts=%d\n", info.networkId, info.serviceId, info.tsId);
      return;
    }

    channelId = ch->GetServiceName();
  }
  else
  {
    channelId = ch->GetDvbTriplet();
  }

  void* tx = NULL;
  try
  {
    /* Add events */
    tx = EpgStore::GetInstance().StartEventsTransaction();

    foreach (DvbEpgEvent e, info.events)
    {
      CStdString idStr;
      idStr.Format("%d", e.id);

      EpgProgramInfo info;
      info.id = idStr;
      info.channelId = channelId;
      info.start = e.startTime;
      info.end = e.startTime + e.duration;
      info.title = e.name;
      info.synopsis = e.text + "\n" + e.extra;
      info.rating = e.rating;

      EpgStore::GetInstance().InsertEvent(tx, EPG_SOURCE_ID_OTA, info);
    }

    EpgStore::GetInstance().CommitEventsTransaction(tx);
  }
  catch (std::exception& ex)
  {
    if (tx)
      EpgStore::GetInstance().RollbackEventsTransaction(tx);
  }
}

bool DVBManager::ReportCurrentChannelToServerInBG()
{
#ifdef HAS_SERVER_OTA
  DvbChannelPtr currentChannel = DVBManager::GetInstance().GetCurrentChannel();
  if (!currentChannel.get())
    return false;

  std::string url = BXConfiguration::GetInstance().GetStringParam("Boxee.ReportWatchedProgram", "http://app.boxee.tv/epg/watch");

  std::string body = "";
  if (!currentChannel->GetServerId().IsEmpty())
  {
    body += "channel_id=";
    body += BXUtils::URLEncode(currentChannel->GetServerId());
  }
  else if (!currentChannel->GetServiceName().IsEmpty())
  {
    body += "service=";
    body += BXUtils::URLEncode(currentChannel->GetServiceName());
  }
  else
  {
    // There's no id -- nothing really to report
    return false;
  }

  // Append: country, zip code and provider id, if we know them
  body += "&country=";
  body += g_guiSettings.GetString("ota.countrycode");

  if (!g_guiSettings.GetString("ota.zipcode").IsEmpty())
  {
    body += "&postal_code=";
    body += g_guiSettings.GetString("ota.zipcode");
  }

  CLog::Log(LOGINFO, "Notify watch to server: url=[%s] body=[%s]", url.c_str(), body.c_str());

  std::string response;
  int retCode;

  if (!BXUtils::PerformPostRequestInBG(url, body, response, retCode, true))
  {
    CLog::Log(LOGERROR, "error reporting watched to server. Return code: %d", retCode);
    return false;
  }
#endif

  return true;
}

void DVBManager::SetCurrentChannel(DvbChannelPtr channel)
{
  CSingleLock lock(m_lockCurrentProgram);

  m_currentChannel = channel;
  m_currentProgram.Clear();
  m_lastEpgCheckTime = 0;

  if (m_currentChannel.get() == NULL)
    return;

  CStdString id = m_currentChannel->GetChannelId();
  if (id.IsEmpty())
    return;

  std::vector<EpgProgramInfo> infos = EpgStore::GetInstance().QueryByChannelNow(id);
  if (infos.size() == 0)
    return;

  m_currentProgram.isNow = true;
  m_currentProgram.info = infos[0];
  EpgStore::GetInstance().GetChannelSocialInfo(m_currentProgram.info.id, m_currentChannel->GetServiceName(), m_currentProgram.social);
}

LiveTvModelProgram DVBManager::GetCurrentProgram()
{
  CSingleLock lock(m_lockCurrentProgram);

  time_t now = time(NULL);

  if (now - m_lastEpgCheckTime > 60)
  {
    m_lastEpgCheckTime = now;

    m_currentProgram.Clear();

    CStdString id = m_currentChannel->GetChannelId();
    if (id.IsEmpty())
      return m_currentProgram;

    std::vector<EpgProgramInfo> infos = EpgStore::GetInstance().QueryByChannelNow(id);
    if (infos.size() == 0)
      return m_currentProgram;

    m_currentProgram.isNow = true;
    m_currentProgram.info = infos[0];
    EpgStore::GetInstance().GetChannelSocialInfo(m_currentProgram.info.id, m_currentChannel->GetServiceName(), m_currentProgram.social);
  }

  return m_currentProgram;
}

#endif
