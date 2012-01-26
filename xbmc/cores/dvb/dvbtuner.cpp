#include "utils/log.h"
#include "Util.h"
#include "dvbtuner.h"
#include <boost/foreach.hpp>
#include <time.h>
#include "SpecialProtocol.h"
#include "RegExp.h"
#include "dvbepgloader.h"
#include <poll.h>
#include "SingleLock.h"

#ifdef HAS_DVB

#include "v4lhelper.h"

DvbScanner::DvbScanner(DvbTuner* tuner) : m_tuner(tuner)
{
  pthread_mutex_init(&m_condLock, NULL);
  pthread_cond_init(&m_scannerStarted, NULL);

  m_totalFreqCount = 0;
  m_currentFreqCount = 0;

  SetState(SCAN_NOT_RUNNING);
}

void DvbScanner::Process()
{
  m_totalFreqCount = 0;
  m_currentFreqCount = 0;
  m_totalFoundFreqCount = 0;
  m_currentFoundFreqCount = 0;
  m_initalScanDone = false;

  SetState(SCAN_STARTING);

  CStdString cmd;
  FILE* f;
  char szBuffer[1024];

  CStdString frontendTypeStr = "t";
  switch (m_tuner->GetModulationType())
  {
  case DvbTuner::VSB:
    frontendTypeStr = "a -A 1";
    break;
  case DvbTuner::QAM:
    frontendTypeStr = "a -A 2";
    break;
  case DvbTuner::OFDM:
  case DvbTuner::UNKNOWN_MODULATION:
    frontendTypeStr = "t";
    break;
  }

  // First, do dry run
  cmd.Format("special://xbmc/w_scan -f %s -c %s -o7 -t3 -X -a %d -C UTF-8 -d", frontendTypeStr.c_str(), m_tuner->GetCountry(), m_tuner->GetId());
  cmd = _P(cmd);

  CLog::Log(LOGDEBUG, "DvbScanner::Process: dry run of w_scan: %s", cmd.c_str());

  f = popen(cmd.c_str(), "r");
  if (!f)
  {
    CLog::Log(LOGERROR, "DvbScanner::Process: dry run failed: %s", strerror(errno));
    SetState(SCAN_FAILED);
    pthread_cond_broadcast(&m_scannerStarted);
    m_tuner->OnScannerDone();
    return;
  }

  while (!feof(f) && !m_bStop)
  {
    fgets(szBuffer, sizeof(szBuffer), f);
    CLog::Log(LOGDEBUG, "DvbScanner::Process: dry run output: %s", szBuffer);
    if (strncmp(szBuffer, "COUNT:", 6) == 0)
    {
      sscanf(szBuffer, "COUNT: %d", &m_totalFreqCount);
      CLog::Log(LOGDEBUG, "DvbTuner: scanner (dry-run), found: %d frequencies", m_totalFreqCount);
      break;
    }
  }

  pclose(f);

  // sometimes real scan fails due unusable/locked frontend device
  // sleep here to make sure that dry scan frees frontend's resources
  Sleep(500);

  printf("\nDRY RUN COUNT: %d\n", m_totalFreqCount);

  if (m_bStop)
  {
    CLog::Log(LOGINFO, "DvbScanner::Process: scan canceled");
    SetState(SCAN_CANCELED);
    pthread_cond_broadcast(&m_scannerStarted);
    m_tuner->OnScannerDone();
    return;
  }

  if (m_totalFreqCount == 0)
  {
    CLog::Log(LOGERROR, "DvbScanner::Process: dry run failed since no frequecies were found");
    SetState(SCAN_FAILED);
    pthread_cond_broadcast(&m_scannerStarted);
    m_tuner->OnScannerDone();
    return;
  }

  CStdString channelsFile = _P("special://home/dvb/channels.conf");

  cmd.Format("special://xbmc/w_scan -f %s -c %s -o7 -t3 -X -a %d -C UTF-8 -E0 -R0 -g '%s' 2>&1", frontendTypeStr.c_str(), m_tuner->GetCountry(), m_tuner->GetId(), channelsFile.c_str());
  cmd = _P(cmd);

  CLog::Log(LOGDEBUG, "DvbScanner::Process: real run of w_scan: %s", cmd.c_str());

  f = popen(cmd.c_str(), "r");
  if (!f)
  {
    CLog::Log(LOGERROR, "DvbScanner::Process: real run failed: %s", strerror(errno));
    SetState(SCAN_FAILED);
    pthread_cond_broadcast(&m_scannerStarted);
    m_tuner->OnScannerDone();
    return;
  }

  SetState(SCAN_RUNNING);

  pthread_cond_broadcast(&m_scannerStarted);

  CRegExp regScanFreq;
  regScanFreq.RegComp("[0-9]+: .*\\(time: .*\\)");

  CRegExp regFreqTune;
  regFreqTune.RegComp("tune to:.+f=[0-9]+");

  CRegExp regFreqFound;
  regFreqFound.RegComp("f=[0-9]+");

  while (!feof(f) && !m_bStop)
  {
    printf("DvbScanner::Process: real run output: %s\n", szBuffer);
    CLog::Log(LOGDEBUG, "DvbScanner::Process: real run output: %s", szBuffer);
    fgets(szBuffer, sizeof(szBuffer), f);

    if (regScanFreq.RegFind(szBuffer) != -1)
    {
      m_currentFreqCount++;
      printf("**** LIVE COUNT: %d\n", m_currentFreqCount);
    }

    if (regFreqTune.RegFind(szBuffer) != -1)
    {
      m_initalScanDone = true;
      m_currentFoundFreqCount++;
      printf("**** CURRENT FREQ COUNT: %d\n", m_currentFoundFreqCount);
    }
    else if (regFreqFound.RegFind(szBuffer) != -1)
    {
      m_totalFoundFreqCount++;
      printf("**** TOTAL FREQ COUNT: %d\n", m_totalFoundFreqCount);
    }
  }

  if (m_bStop)
  {
    system("pkill w_scan");
  }

  pclose(f);

  if (m_bStop)
  {
    CLog::Log(LOGINFO, "DvbScanner::Process: scan canceled");
    SetState(SCAN_CANCELED);
  }
  else
  {
    SetState(SCAN_DONE);
  }

  m_tuner->OnScannerDone();
}

void DvbScanner::WaitForStarted()
{
  pthread_cond_wait(&m_scannerStarted, &m_condLock);
}

void DvbScanner::Stop(bool bWait)
{
  m_bStop = true;
}

void DvbScanner::SetState(ScanState state)
{
  CSingleLock lock(m_statusLock);
  m_state = state;
}

DvbScanner::ScanState DvbScanner::GetState()
{
  CSingleLock lock(m_statusLock);
  return m_state;
}

DvbTuner::DvbTuner(int id, IDvbEpgListener* epgListener, int flags)
{
  m_state = TUNER_IDLE;
  m_id = id;
  m_modulationType = UNKNOWN_MODULATION;
  m_country = "";
  m_running = false;
  m_scanner = new DvbScanner(this);
  m_currChannel = "";
  m_drvLocked = false;
  m_v4l = NULL;

#if defined(_LINUX) && !defined(__APPLE__)
  m_v4l = new V4LHelper(id, flags);
#endif

#if !defined(__APPLE__)
  if (epgListener)
    m_epgLoader.AddListener(epgListener);
#endif

  pthread_mutex_init(&m_condLock, NULL);
  pthread_mutex_init(&m_listenersLock, NULL);
  pthread_cond_init(&m_threadStarted, NULL);
}

DvbTuner::~DvbTuner()
{
  pthread_mutex_destroy(&m_condLock);
  pthread_mutex_destroy(&m_listenersLock);
  pthread_cond_destroy(&m_threadStarted);

  if (m_v4l)
    delete m_v4l;

  if (m_scanner)
    delete m_scanner;
}

void DvbTuner::AddListener(IDvbStreamListener *alistener)
{
  pthread_mutex_lock(&m_listenersLock);
  bool bExists = false;

  foreach (IDvbStreamListener* listener, m_listeners)
  {
    if (alistener == listener)
    {
      bExists = true;
      break;
    }
  }

  if (!bExists)
    m_listeners.push_back(alistener);

  pthread_mutex_unlock(&m_listenersLock);
}

void DvbTuner::RemoveListener(IDvbStreamListener *listener)
{
  pthread_mutex_lock(&m_listenersLock);
  m_listeners.remove(listener);
  pthread_mutex_unlock(&m_listenersLock);
}

bool DvbTuner::StartScan()
{
  if (m_country.length() == 0)
  {
    CLog::Log(LOGDEBUG, "DvbTuner: no country specified, aborting");
    return false;
  }

  if (m_v4l)
    m_v4l->Stop(); // we have to close the helper so that the device (frontend) will be available

  m_scanner->Create();
  m_scanner->WaitForStarted();

  if (m_scanner->GetState() != DvbScanner::SCAN_RUNNING)
    return false;

  m_state = TUNER_SCANNING;

  CLog::Log(LOGDEBUG, "DvbTuner: scanner started");

  return true;
}

void DvbTuner::CancelScan()
{
  m_scanner->Stop(false);
  m_state = TUNER_IDLE;
}

void DvbTuner::OnScannerDone()
{
  CLog::Log(LOGINFO, "DvbTuner::OnScannerDone: scanner done");

  CStdString channelsFile = _P("special://home/dvb/channels.conf");

  pthread_mutex_lock(&m_listenersLock);
  std::list<IDvbStreamListener *> tempListeners = m_listeners;
  pthread_mutex_unlock(&m_listenersLock);

  foreach (IDvbStreamListener* listener, tempListeners)
  {
    listener->OnScanFinished(m_scanner->GetState(), channelsFile);
  }

  m_state = TUNER_IDLE;
}

int DvbTuner::GetCurrentFrequencyScanCount()
{
  return m_scanner->GetCurrentFrequencyCount();
}

bool DvbTuner::IsInitialScanDone()
{
  return m_scanner->IsInitialScanDone();
}

int DvbTuner::GetCurrentFoundFrequencyCount()
{
  return m_scanner->GetCurrentFoundFrequencyCount();
}

int DvbTuner::GetTotalFoundFrequencyCount()
{
  return m_scanner->GetTotalFoundFrequencyCount();
}

int DvbTuner::GetTotalFrequencyScanCount()
{
  return m_scanner->GetTotalFrequencyCount();
}

DvbScanner::ScanState DvbTuner::GetScanState() const
{
  return m_scanner->GetState();
}

bool DvbTuner::Tune(const DvbChannel &channel)
{
  if (m_state == TUNER_SCANNING)
  {
    return false;
  }

  if (m_v4l && !m_v4l->Tune(channel))
  {
    CLog::Log(LOGERROR, "tuning failed\n");
    return false;
  }

  m_state = TUNER_IDLE;
  if (m_running)
  {
    m_running = false;
#ifndef __APPLE__
    wait(); // the thread will call NotifyStreamClosed
#endif
  }

  m_currChannel.clear();
  m_currChannel = channel.GetServiceName();

  return true;
}

void DvbTuner::Untune()
{
  if (m_state == TUNER_SCANNING)
  {
    return;
  }

  if (m_v4l)
  {
    m_v4l->Stop();
  }
}

DvbTuner::TunerType DvbTuner::GetTunerType()
{
  if (m_v4l)
  {
    return m_v4l->GetTunerType(m_id);
  }

  return TUNER_TYPE_UNKNOWN;
}

int DvbTuner::Read(char *buf, int size)
{
  if (m_v4l)
    return m_v4l->Read(buf, size);

  return -1;
}

DvbTuner::TuningState DvbTuner::GetTuningState() const
{
  if (m_v4l)
    return m_v4l->GetTuningState();
  return TUNING_STATE_IDLE;
}

bool DvbTuner::IsSignalOk()
{
  if (m_v4l)
    return m_v4l->IsSignalOk();
  return true;
}

void DvbTuner::AddPidFilter(int pid, DvbTuner::DvbFilter type)
{
  if (m_v4l)
     m_v4l->AddPidFilter(pid, type);
}

void DvbTuner::RemovePidFilter(int pid)
{
  if (m_v4l)
     m_v4l->RemovePidFilter(pid);
}

void DvbTuner::RemoveAllPidFilters()
{
  if (m_v4l)
    m_v4l->RemoveAllPidFilters();
}

DvbTunerTechnicalInfo DvbTuner::GetTechnicalInfo()
{
  return m_v4l->GetTechnicalInfo();
}

int DvbTuner::GetTuneFlags()
{
  if (m_v4l)
    return m_v4l->GetFlags();
  else
    return 0;
}

#endif
