
#include "FilePlaylist.h"
#include "utils/log.h"
#include "PlayList.h"
#include "Util.h"
#include "PlayListFactory.h"
#include "bxcurl.h"
#include "Base64.h"
#include "bxutils.h"
#include "utils/SingleLock.h"
#include "utils/TimeUtils.h"
#include "Application.h"

#include <openssl/evp.h>

#define ACTION_STEP_IN_SEC                    60
#define ACTION_BIG_STEP_BACK_IN_SEC           600
#define HI_QUALITY_PARAMETER_VALUE            "1"
#define LOW_QUALITY_PARAMETER_VALUE           "0"
#define AUTO_QUALITY_PARAMETER_VALUE          "A"

#define WAIT_FOR_BUFFER_IN_MS                 200

using namespace XFILE;
using namespace PLAYLIST;
using namespace BOXEE;

CPlaylistData::CPlaylistData(PLAYLIST::CPlayList* playList)
{
  m_playList = playList;
  m_playlistBandwidth = 0;
  m_playlistLastPos = -1;
  m_targetDuration = 0;
  m_startDate = 0;
}

CPlaylistData::~CPlaylistData()
{
  if (m_playList)
  {
    delete m_playList;
    m_playList = NULL;
  }
}

BufferData::BufferData()
{
  m_buffer = NULL;
  m_bNeedResetDemuxer = false;
  m_nOriginPlaylist = -1;
  m_nDuration = 0;
  m_nBufferTime = 0;
  m_nSeq = 0;
}

BufferData::~BufferData()
{
  if (m_buffer)
    delete m_buffer;
}

CFilePlaylist::CFilePlaylist()
{
  m_isLive = false;
  m_nLastLoadedSeq = 0;
  m_nCurrPlaylist = 0;
  m_nReadAheadBuffers = 15; // max amount of files in queue
  m_eof = false;
  m_autoChooseQuality = false;
  m_startTime = 0;
  m_prerollDuration = 0;
  m_nLastBufferSwitch = time(NULL);
  m_inProgressBuffer = NULL;
  m_lastReportedTime = 0;
  m_nStartTime = 0;
}

CFilePlaylist::~CFilePlaylist()
{
  StopThread();

  for (size_t i=0; i<m_playlists.size();i++)
  {
    delete m_playlists[i];
  }
  m_playlists.clear();

  ClearBuffers();
}

void CFilePlaylist::SetReadAheadBuffers(int nBuffers)
{
  m_nReadAheadBuffers = nBuffers;
}

bool CFilePlaylist::IncQuality()
{
  CSingleLock lock(m_lock);
  if (m_nCurrPlaylist < (int)m_playlists.size() - 1)
  {
    m_nCurrPlaylist++;
    CLog::Log(LOGDEBUG,"%s to playlist %d. buffers: %d.", __FUNCTION__, m_nCurrPlaylist, m_buffersQueue.size());
    return true;
  }
  return false;
}

bool CFilePlaylist::DecQuality()
{
  CSingleLock lock(m_lock);
  if (m_nCurrPlaylist > 0)
  {
    // dont go too low.
    if (m_playlists[m_nCurrPlaylist-1]->m_playlistBandwidth < 800000)
      return false;
    
    m_nCurrPlaylist--;
    CLog::Log(LOGDEBUG,"%s to playlist %d. buffers: %d.", __FUNCTION__, m_nCurrPlaylist, m_buffersQueue.size());
    return true;
  }
  return false;
}

bool CFilePlaylist::IsEOF()
{
  return m_eof;
}

bool CFilePlaylist::ValidatePlaylist(CPlaylistData *pl)
{
  if (!pl)
    return false;
  
  if (pl->m_playlistLastPos == -1 && !pl->m_playList->Load(pl->m_playlistPath))
    return false;
  
  if (pl->m_playlistLastPos == -1 || pl->m_playlistLastPos >= pl->m_playList->size())
    pl->m_playlistLastPos = 0;
  
  if (pl->m_playList->size() <= 0 && ( !pl->m_playList->Load(pl->m_playlistPath) || pl->m_playList->size() <= 0) )
    return false;

  if (pl->m_playlistLastPos >= pl->m_playList->size())
    pl->m_playlistLastPos = 0; // reset after load (might been truncated)

  CFileItemPtr firstItem = (*pl->m_playList)[0];
  pl->m_targetDuration = BXUtils::StringToInt(firstItem->GetProperty("m3u8-targetDurationInSec").c_str());
  pl->m_startDate = BXUtils::StringToInt(firstItem->GetProperty("m3u8-startDate").c_str());
  
  CFileItemPtr lastItem = (*pl->m_playList)[pl->m_playList->size() - 1];
  unsigned int nLastSeq = lastItem->GetPropertyULong("m3u8-playlistSequenceNo");
  if (nLastSeq <= m_nLastLoadedSeq && pl->m_playList->CanAdd() && !pl->m_playList->Load(pl->m_playlistPath))
    return false;
  
  if (nLastSeq <= m_nLastLoadedSeq && !pl->m_playList->CanAdd())
  {
    m_eof = true;
    return true;
  }
  
  bool bFoundSeq = false;
  while (pl->m_playlistLastPos < pl->m_playList->size())
  {
    lastItem = (*pl->m_playList)[pl->m_playlistLastPos];
    nLastSeq = lastItem->GetPropertyULong("m3u8-playlistSequenceNo");
    if (nLastSeq >= m_nLastLoadedSeq)
    {
      bFoundSeq = true;
      break;
    }
    pl->m_playlistLastPos++;
  }
  
  if (!bFoundSeq)
    return false;
  
  // we are now on the last seq loaded. we should load the next item (if available)
  return (pl->m_playlistLastPos < pl->m_playList->size() - 1);
}

void CFilePlaylist::ReadAhead()
{
  CSingleLock lock(m_lock);
  if (!ValidatePlaylist(m_playlists[m_nCurrPlaylist]) || m_eof)
    return;
    
  int nCurrPlaylist = m_nCurrPlaylist;
  CPlaylistData* pl = m_playlists[m_nCurrPlaylist];
  pl->m_playlistLastPos++;
  CFileItemPtr item = (*pl->m_playList)[pl->m_playlistLastPos];
  m_nLastLoadedSeq = item->GetPropertyULong("m3u8-playlistSequenceNo");
  unsigned int workingOnSeq = m_nLastLoadedSeq; // if m_nLastLoadedSeq changes - it means there was a skip and need to terminate this transfer (block irrelevant)
  unsigned int nBufferTime = pl->m_playlistLastPos * pl->m_targetDuration;
  lock.Leave();
  
  CFile file;
  if (!file.Open(item->m_strPath, READ_NO_CACHE))
  {
    CLog::Log(LOGERROR,"CFilePlaylist::%s - FAILED to open file [%s]!",__FUNCTION__,item->m_strPath.c_str());
    return; // next ReadAhead will try again
  }
  
  if (m_bStop || workingOnSeq != m_nLastLoadedSeq)
  {
    file.Close();
    return;
  }
  
  EVP_CIPHER_CTX deCtx;
  EVP_CIPHER_CTX_init(&deCtx);
  
  CStdString strEncMethod = item->GetProperty("m3u8-encryptionMethod");
  strEncMethod.MakeLower();
  
  bool bEncrypted = (!strEncMethod.IsEmpty() && strEncMethod != "none");
  if (bEncrypted && strEncMethod != "aes-128")
  {
    CLog::Log(LOGERROR,"unsupported encryption method <%s>", strEncMethod.c_str());
    // still try to decode... what do we have to lose.
  }
  
  CStdString encryptKeyUri = item->GetProperty("m3u8-encryptKeyUri");
  
  lock.Enter();
  if (bEncrypted && m_encryptKeyUri != encryptKeyUri)
  {
    m_encryptKeyUri = encryptKeyUri;
    GetEncryptKey(encryptKeyUri, m_encryptKeyValue); // again - if it fails- we dont really have anything to do...
  }
  lock.Leave();

  if (bEncrypted)
  {
    CStdString encryptIv;
    unsigned char iv[16] = { 0 };
    if (!item->GetProperty("m3u8-encryptIv").IsEmpty())
    {
      // interpret as hex string 
      encryptIv = item->GetProperty("m3u8-encryptIv");
      encryptIv.MakeLower();
      if (encryptIv.size() > 2 && encryptIv[0] == '0' && encryptIv[1] == 'x')
        encryptIv = encryptIv.substr(2);
      
      char buf[64];
      memset(buf,0,64);
      memcpy(buf, encryptIv.c_str(), encryptIv.size());
      
      for (int i=0; i<16; i++)
        sscanf(buf + (i*2), "%02x", &iv[i]);
    }
    else if (!item->GetProperty("m3u8-playlistSequenceNo").IsEmpty())
    {
      encryptIv = item->GetProperty("m3u8-playlistSequenceNo");
      unsigned int seq = htonl(atoi(encryptIv.c_str()));
      memcpy(&iv[12], &seq, sizeof(unsigned int));
    }        
    EVP_DecryptInit_ex(&deCtx, EVP_aes_128_cbc(), NULL, (unsigned char*)m_encryptKeyValue.data(), iv);
  }
  
  static const int READ_CHUNK_SIZE=4096;
  
  int nSize = (int)file.GetLength();
  if (nSize < (100 * 1024)) // just sanity (we might not have length)
    nSize = 10 * 1024 * 1024; // 10M should be enough... but this should be smarter
  
  CRingBuffer *newBuffer = new CRingBuffer;
  newBuffer->Create(nSize + READ_CHUNK_SIZE); // CHUNK_SIZE spair bytes. not really required. just to be on the safe side
  
  lock.Enter();
  m_inProgressBuffer = newBuffer;
  lock.Leave();
  
  unsigned int nStartTiming = CTimeUtils::GetTimeMS();
  int nBytes = 0;
  char *buffer    = new char[READ_CHUNK_SIZE];
  char *decBuffer = new char[READ_CHUNK_SIZE*2];
    
  do
  {
    nBytes = file.Read(buffer, READ_CHUNK_SIZE);
    if (nBytes > 0)
    {
      if (bEncrypted)
      {
        int decBufferSize=READ_CHUNK_SIZE;
        if (!EVP_DecryptUpdate(&deCtx, (unsigned char*)decBuffer, &decBufferSize, (unsigned char*)buffer, nBytes))
        {
          CLog::Log(LOGERROR,"CFilePlaylist::CReadFileDataJob::GetEncryptKey - FAILED to decrypt data (rfdj)");
        }
        else
        {
          newBuffer->WriteBinary(decBuffer, decBufferSize);
        }
      }
      else
      {
        newBuffer->WriteBinary(buffer, nBytes);
      }
    }
  } while (!m_bStop && workingOnSeq == m_nLastLoadedSeq && nBytes > 0);
  unsigned int nEndTiming = CTimeUtils::GetTimeMS();
  
  if (bEncrypted)
  {
    int decBufferSize=READ_CHUNK_SIZE;
    if (EVP_DecryptFinal_ex(&deCtx, (unsigned char*)decBuffer, &decBufferSize))
      newBuffer->WriteBinary(decBuffer, decBufferSize);
  }
  
  EVP_CIPHER_CTX_cleanup(&deCtx);
  
  lock.Enter();
  m_inProgressBuffer = NULL;
  lock.Leave();
  
  delete [] buffer;
  delete [] decBuffer;
  
  if (m_bStop || workingOnSeq != m_nLastLoadedSeq) // check also if last seq changed (seek while we were reading)
  {
    // something went wrong
    CLog::Log(LOGDEBUG,"failed to read segment.");
    delete newBuffer;
    return;
  }
  
  if (newBuffer->GetMaxReadSize() == 0)
  {
    CLog::Log(LOGDEBUG,"buffer was already read while being retrieved");
    delete newBuffer;
    return;
  }
  
  lock.Enter();
  
  int nDuration = item->GetPropertyInt("m3u8-durationInSec") * 1000;

  BufferData *data = new BufferData;
  data->m_buffer=newBuffer;
  data->m_nOriginPlaylist = nCurrPlaylist;
  data->m_nDuration = nDuration / 1000;
  data->m_nBufferTime = nBufferTime;
  data->m_nSeq = workingOnSeq;
  m_buffersQueue.push_back(data);
  
  if (nEndTiming < nStartTiming)
  {
    // doesnt make sense- probably timer reset
    return;
  }
  
  int nDiff = nEndTiming - nStartTiming;
  CLog::Log(LOGDEBUG,"finished reading segment (%d sec). took %d millis.", nDuration, nDiff);
  
  //
  // estimate bw and switch to lower/higher bitrate stream if possible and required
  //
  if (!m_autoChooseQuality)
    return;
  
  if ((float)nDiff <= (float)nDuration * 0.25)
  {
    CLog::Log(LOGDEBUG,"incrementing quality. bw is good.");
    if (IncQuality())
      data->m_bNeedResetDemuxer = true; // mark that the next buffer will have different bitrate
  }
  else if ((float)nDiff >= (float)nDuration * 0.8)
  {
    CLog::Log(LOGDEBUG,"decrementing quality. bw is bad.");
    if (DecQuality())
      data->m_bNeedResetDemuxer = true; // mark that the next buffer will have different bitrate
  }
}

void CFilePlaylist::Process()
{
  if (!m_trackingUrl.IsEmpty())
  {
    BOXEE::BXCurl bxCurl;
    bxCurl.HttpGetString(m_trackingUrl, false);
    CLog::Log(LOGDEBUG, "sent tracking to <%s>", m_trackingUrl.c_str());
  }
  m_trackingUrl.clear();
  
  while (!m_bStop && !m_eof)
  {
    CSingleLock lock(m_lock);
    int nQueueSize = m_buffersQueue.size();
    lock.Leave();
    for (int i=0; !m_bStop && i < m_nReadAheadBuffers - nQueueSize; i++)
    {
      ReadAhead();
    }
    if (!m_bStop)
      Sleep(200);
  }
}

bool CFilePlaylist::Open(const CURL& url)
{
  CStdString strPath;
  url.GetURL(strPath);

  CLog::Log(LOGDEBUG,"CFilePlaylist::Open - Enter function with [strPath=%s] (fpl)",strPath.c_str());

  if (!ParsePath(strPath))
  {
    CLog::Log(LOGERROR,"CFilePlaylist::Open - FAILED to parse Playlist file path. [strPath=%s] (fpl)",strPath.c_str());
    return false;
  }

  CLog::Log(LOGDEBUG,"CFilePlaylist::Open - After ParsePath [PlaylistFilePath=%s][FingerprintBase64=%s][KeyServerParams=%s][bxOurl=%s][m_trackingUrl=%s][m_inningsIndex=%s][m_quality=%s][isLive=%d] (fpl)",m_playlistFilePath.c_str(),m_fingerprintBase64.c_str(),m_keyServerParams.c_str(),m_bxOurl.c_str(),m_trackingUrl.c_str(),m_inningsIndex.c_str(),m_quality.c_str(),m_isLive);

  PLAYLIST::CPlayList* playList = BuildPlaylist(m_playlistFilePath);
  if (!playList)
  {
    CLog::Log(LOGERROR,"CFilePlaylist::Open - FAILED to build playlist from [%s] (fpl)",m_playlistFilePath.c_str());
    return false;
  }

  if (playList->size() < 1)
  {
    delete playList;
    CLog::Log(LOGERROR,"CFilePlaylist::Open - FAILED to build playlist from [%s]. [size=%d] (fpl)",m_playlistFilePath.c_str(),playList->size());
    return false;
  }

  //////////////////////////////////////////////////////////////////
  // check if playlist is a m3u8 of different bandwidth playlist. //
  // is this is the right way to check ???                        //
  //////////////////////////////////////////////////////////////////

  CFileItemPtr item = (*playList)[0];

  if (item->IsPlayList() && !item->GetProperty("m3u8-bandwidth").IsEmpty())
  {
    CLog::Log(LOGDEBUG,"CFilePlaylist::Open - Playlist [%s] is playlist of playlists. Going to call BuildPlaylistByBandwidth() (fpl)",m_playlistFilePath.c_str());

    // this is a playlist of different bandwidth playlist -> build vector of playlist by bandwidth
    if (!BuildPlaylistByBandwidth(playList))
    {
      delete playList;
      CLog::Log(LOGERROR,"CFilePlaylist::Open - FAILED to build playlist by bandwidth for [%s] (fpl)",m_playlistFilePath.c_str());
      return false;
    }

    // no need for the original playlist of playlists
    delete playList;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CFilePlaylist::Open - Going to insert playlist [%s] to playlist vector (fpl)",m_playlistFilePath.c_str());

    if (!InsertPlaylistDataToList(item,playList))
    {
      delete playList;
      CLog::Log(LOGERROR,"CFilePlaylist::Open - FAILED to insert playlist [%s] (fpl)",m_playlistFilePath.c_str());
      return false;
    }
  }

  /////////////////////////////////////////
  // need to choose the current playlist //
  /////////////////////////////////////////

  if ((int)m_playlists.size() < 2)
  {
    // there is only 1 playlist file
    m_nCurrPlaylist = 0;
  }
  else
  {
    m_nCurrPlaylist = m_playlists.size() - 1;
    if (m_quality == AUTO_QUALITY_PARAMETER_VALUE)
      m_autoChooseQuality = true;
    
    if (m_quality != HI_QUALITY_PARAMETER_VALUE)
    {
      while (m_nCurrPlaylist > 0 && m_playlists[m_nCurrPlaylist]->m_playlistBandwidth > (1.5 * 1024 * 1024)) // huristic. we find the < 1.5mbps stream
        m_nCurrPlaylist--;
      CLog::Log(LOGDEBUG,"CFilePlaylist::Open - NORMAL quality was chosen. [quality=%s] (fpl)",m_quality.c_str());
    }
    else
    {
      CLog::Log(LOGDEBUG,"CFilePlaylist::Open - HI quality was chosen. [quality=%s] (fpl)",m_quality.c_str());
    }
  }

  ///////////////////////////////////
  // open first file from playlist //
  ///////////////////////////////////
  CPlaylistData* pl = m_playlists[m_nCurrPlaylist];
  
  if (!pl)
  {
    CLog::Log(LOGERROR,"%s playlist is null! open failed", __FUNCTION__);
    return false;
  }
  
  m_nLastLoadedSeq = 0;
  
  if (m_isLive)
  {
    if (ValidatePlaylist(pl))
    {
      if (pl->m_playList->size() == 0)
      {
        CLog::Log(LOGERROR,"invalid playlist (empty)");
        return false;
      }
            
      int nItemDuration = pl->m_targetDuration;
      int nItemsToSkip = 5;
      if (nItemDuration > 0)
        nItemsToSkip = 30 / nItemDuration; // 30 seconds behind
      int nItem = (int)(pl->m_playList->size()) - 1 - nItemsToSkip;
      if (nItem < 0)
        nItem = 0;
      pl->m_playlistLastPos = nItem;
      CFileItemPtr item = (*pl->m_playList)[nItem];
      m_nLastLoadedSeq = item->GetPropertyULong("m3u8-playlistSequenceNo");
      m_lastReportedTime = pl->m_playlistLastPos * pl->m_targetDuration;
      m_nStartTime = pl->m_playlistLastPos * pl->m_targetDuration;
    }
    else
    {
      CLog::Log(LOGERROR,"%s unable to verify playlist <%s>. open failed", __FUNCTION__, pl->m_playlistPath.c_str());
      return false;
    }
  }

  if (!m_preRollStrDuration.IsEmpty() && ValidatePlaylist(pl))
  {
    int nHours=0, nMin=0, nSec=0;
    sscanf(m_preRollStrDuration.c_str(), "%d:%d:%d",&nHours,&nMin,&nSec);
    int prerollEnd = nHours*3600 + nMin*60 + nSec;
    int prerollStart = pl->m_startDate;
    if (prerollEnd < prerollStart)
      prerollEnd += (24*60*60 - prerollStart); // past midnight. need to add the delta of prev day
    m_prerollDuration = prerollEnd - prerollStart;
    m_lastReportedTime -= m_prerollDuration;
    if (!m_isLive)
    {
      m_lastReportedTime = 0;
      m_nStartTime = m_prerollDuration; 
      SeekToTime(0); // will add m_prerollDuration internally     
    }
  }
  
  if (m_startTime > 0)
    SeekToTime(m_startTime);
  
  m_nLastBufferSwitch = time(NULL);
    
  Create();
  Sleep(1000);
  
  return true;
}

void CFilePlaylist::Close()
{
  CLog::Log(LOGDEBUG,"CFilePlaylist::Close - Enter function (fpl)");
  StopThread();
}

bool CFilePlaylist::Exists(const CURL& url)
{
  CStdString strPath;
  url.GetURL(strPath);

  CLog::Log(LOGDEBUG,"CFilePlaylist::Open - Enter function with [path=%s] (fpl)",strPath.c_str());

  return true;
}

int CFilePlaylist::Stat(const CURL& url, struct __stat64* buffer)
{
  CStdString strPath;
  url.GetURL(strPath);

  CLog::Log(LOGDEBUG,"CFilePlaylist::Stat - Not implemented. [path=%s] (fpl)",strPath.c_str());

  return -1;
}

unsigned int CFilePlaylist::Read(void* lpBuf, int64_t uiBufSize)
{
  return ReadData(lpBuf,uiBufSize);
}

void CFilePlaylist::SetPlayerTime()
{
  CAction action;
  action.id = ACTION_SET_VIDEO_TIME;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->OnAction(action);  
}

void CFilePlaylist::ResetDemuxer()
{
  if (!m_autoChooseQuality)
    return;
  
  CAction action;
  action.id = ACTION_RESET_DEMUXER;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->OnAction(action);  
}

void CFilePlaylist::ClearBuffers()
{
  CSingleLock lock(m_lock);
  if (m_buffersQueue.size() && m_buffersQueue.front()->m_nOriginPlaylist != m_nCurrPlaylist)
    ResetDemuxer();
  
  if (m_buffersQueue.size())
    m_nLastLoadedSeq = m_buffersQueue.front()->m_nSeq;

  while (m_buffersQueue.size())
  {
    delete m_buffersQueue.front();
    m_buffersQueue.pop_front();
  }  
  m_inProgressBuffer = NULL;
}

bool CFilePlaylist::OnAction(const CAction &action)
{
  static const int sSmallSkipSec = 30;
  static const int sBigSkipSec   = 300;

  CSingleLock lock(m_lock);
  if (m_nCurrPlaylist < 0 || m_nCurrPlaylist >= (int)m_playlists.size())
    return true;
  int nItemDuration = m_playlists[m_nCurrPlaylist]->m_targetDuration;
  if (nItemDuration <= 0)
    nItemDuration = sSmallSkipSec;
  lock.Leave();

  unsigned int nSmallSkipItems =  sSmallSkipSec / nItemDuration;
  unsigned int nBigSkipItems   =  sBigSkipSec   / nItemDuration;

  switch (action.id)
  {
    case ACTION_STEP_BACK:
      SeekToTime(m_lastReportedTime - sSmallSkipSec - nItemDuration);
      break;
      
    case ACTION_BIG_STEP_BACK:
      SeekToTime(m_lastReportedTime - sBigSkipSec - nItemDuration);
      break;

    case ACTION_STEP_FORWARD:
      SeekToTime(m_lastReportedTime + sSmallSkipSec);
      break;

    case ACTION_BIG_STEP_FORWARD:
      SeekToTime(m_lastReportedTime + sBigSkipSec);
      break;
      
    default:
      return false;
  }
  return true;
}

bool CFilePlaylist::SeekToTime(int nSecs)
{
  ClearBuffers();
  CSingleLock lock(m_lock);
  nSecs += m_prerollDuration;
  if (m_nCurrPlaylist < 0 || m_nCurrPlaylist >= (int)m_playlists.size())
    return false;
  
  CPlaylistData *pl = m_playlists[m_nCurrPlaylist];
  if (!ValidatePlaylist(pl))
    return false;
  
  if (!pl || pl->m_targetDuration <= 0 || !pl->m_playList || pl->m_playList->size() == 0)
    return false;
  
  int nIndex = nSecs / pl->m_targetDuration;
  if (nIndex >= pl->m_playList->size())
    nIndex = pl->m_playList->size() - 1;
  pl->m_playlistLastPos = nIndex - 1;
  if (pl->m_playlistLastPos < 0)
    pl->m_playlistLastPos = 0;
  CFileItemPtr item = (*pl->m_playList)[pl->m_playlistLastPos];
  m_nLastLoadedSeq = item->GetPropertyULong("m3u8-playlistSequenceNo");
  m_lastReportedTime = nSecs - m_prerollDuration;
  return true;
}

unsigned int CFilePlaylist::GetStartTime()
{
  return m_nStartTime - m_prerollDuration;
}

unsigned int CFilePlaylist::GetCurrentTime()
{
  CSingleLock lock(m_lock);
  if (m_buffersQueue.size() == 0)
    return m_lastReportedTime;
  
  int pos = m_buffersQueue.front()->m_nBufferTime + (time(NULL) - m_nLastBufferSwitch) - m_prerollDuration;
  if (pos < 0)
    pos = 0;
  
  m_lastReportedTime = pos;
  return (unsigned int)pos;
}

unsigned int CFilePlaylist::GetTotalTime()
{
  CSingleLock lock(m_lock);
  if (m_nCurrPlaylist < 0 || m_nCurrPlaylist >= (int)m_playlists.size())
    return 0;
  
  CPlaylistData *pl = m_playlists[m_nCurrPlaylist];
  if (!pl || pl->m_targetDuration <= 0 || !pl->m_playList || pl->m_playList->size() == 0 || pl->m_playlistLastPos <= 0 || pl->m_playList->CanAdd())
    return 0;
  
  return pl->m_playList->size() * pl->m_targetDuration - m_prerollDuration;
}

int64_t CFilePlaylist::Seek(int64_t iFilePosition, int iWhence)
{
  return -1;
}

int64_t CFilePlaylist::GetPosition()
{
  return -1;
}

int64_t CFilePlaylist::GetLength()
{
  return -1;
}

CStdString CFilePlaylist::GetContent()
{
  return "";
}

bool CFilePlaylist::ParsePath(const CStdString& strPath)
{
  CURL url(strPath);

  if (url.GetProtocol() != "playlist")
  {
    CLog::Log(LOGERROR,"CFilePlaylist::ParsePath - Enter function without playlist://. [strPath=%s] (fpl)",strPath.c_str());
    return false;
  }

  m_playlistFilePath = url.GetUrlWithoutOptions();
  if (m_playlistFilePath.IsEmpty())
  {
    m_playlistFilePath = strPath;
  }

  // remove playlist://
  m_playlistFilePath = m_playlistFilePath.substr(11);

  CUtil::UrlDecode(m_playlistFilePath);
  CUtil::RemoveSlashAtEnd(m_playlistFilePath);

  std::map<CStdString, CStdString> optionMap = url.GetOptionsAsMap();

  m_fingerprintBase64 = optionMap["stream-fingerprint"];
  m_bxOurl = optionMap["bx-ourl"];
  m_trackingUrl = optionMap["tracking-url"];
  m_inningsIndex = optionMap["innings-index"];
  m_keyServerParams = optionMap["stream-params"];
  m_quality = optionMap["quality"];
  m_startTime = atoi(optionMap["startTime"].c_str());
  m_preRollStrDuration = optionMap["startDate"];
  
  CStdString isLiveStr = optionMap["live"];
  if (isLiveStr == "1")
  {
    m_isLive = true;
  }
  else
  {
    m_isLive = false;
  }

  return true;
}

PLAYLIST::CPlayList* CFilePlaylist::BuildPlaylist(const CStdString& playlistPath, bool appendToPlaylist)
{
  CLog::Log(LOGDEBUG,"CFilePlaylist::BuildPlaylist - Enter function. [playlistPath=%s][appendToPlaylist=%d] (fpl)",playlistPath.c_str(),appendToPlaylist);

  PLAYLIST::CPlayList* playList = NULL;
  playList = CPlayListFactory::Create(playlistPath);

  if (!playList)
  {
    CLog::Log(LOGERROR,"CFilePlaylist::BuildPlaylist - FAILED to get playlist object for [%s]. [appendToPlaylist=%d] (fpl)",playlistPath.c_str(),appendToPlaylist);
    return playList;
  }

  bool succeeded = false;
  succeeded = playList->Load(playlistPath);

  if (!succeeded)
  {
    delete playList;
    CLog::Log(LOGERROR,"CFilePlaylist::BuildPlaylist - FAILED to load [%s]. [appendToPlaylist=%d] (fpl)",playlistPath.c_str(),appendToPlaylist);
    return NULL;
  }

  return playList;
}

bool CFilePlaylist::BuildPlaylistByBandwidth(PLAYLIST::CPlayList* playListOfPlaylists)
{
  if (!playListOfPlaylists)
  {
    CLog::Log(LOGERROR,"CFilePlaylist::BuildPlaylistByBandwidth - Enter function with a NULL playlist (fpl)");
    return false;
  }

  ////////////////////////////////
  // build each of the playlist //
  ////////////////////////////////

  for (int i=0; i<playListOfPlaylists->size(); i++)
  {
    CFileItemPtr playlistItem = (*playListOfPlaylists)[i];

    CStdString playlistPath = playlistItem->m_strPath;
    CStdString playlistBandwidthStr = playlistItem->GetProperty("m3u8-bandwidth");

    if (playlistItem->IsPlayList() && !playlistBandwidthStr.IsEmpty())
    {
      PLAYLIST::CPlayList* playList = NULL;

      playList = CPlayListFactory::Create(playlistPath);

      if (!playList)
      {
        CLog::Log(LOGERROR,"CFilePlaylist::BuildPlaylistByBandwidth - FAILED to get playlist object for [%s] (fpl)",playlistPath.c_str());
      }
      else if (!InsertPlaylistDataToList(playlistItem,playList))
      {
        delete playList;
        CLog::Log(LOGERROR,"CFilePlaylist::BuildPlaylistByBandwidth - FAILED to insert PlaylistData [%s] to vec (fpl)",playlistPath.c_str());
      }
    }
  }

  CLog::Log(LOGDEBUG,"CFilePlaylist::BuildPlaylistByBandwidth - Exit function after building [%d] playlists from [%s] (fpl)",(int)m_playlists.size(),playListOfPlaylists->GetName().c_str());

  return true;
}

bool CFilePlaylist::InsertPlaylistDataToList(CFileItemPtr newPlaylistItem, PLAYLIST::CPlayList* newPlaylist)
{
  if (!newPlaylist)
  {
    CLog::Log(LOGERROR,"CFilePlaylist::InsertPlaylistDataToList - Enter function with a NULL Playlist object (fpl)");
    return false;
  }

  CStdString playlistPath = newPlaylistItem->m_strPath;

  CPlaylistData* newPlaylistData = new CPlaylistData(newPlaylist);
  if (!newPlaylistData)
  {
    CLog::Log(LOGERROR,"CFilePlaylist::InsertPlaylistDataToList - FAILED to allocate PlaylistData object for [%s] (fpl)",playlistPath.c_str());
    return false;
  }

  int newPlaylistBandwidth = BXUtils::StringToInt(newPlaylistItem->GetProperty("m3u8-bandwidth"));
  newPlaylistData->m_playlistBandwidth = newPlaylistBandwidth;
  newPlaylistData->m_playlistPath = playlistPath;
  CStdString targetDuration = newPlaylistItem->GetProperty("m3u8-targetDurationInSec");  
  newPlaylistData->m_targetDuration = BXUtils::StringToInt(targetDuration.c_str());
  
  /////////////////////////////////////////
  // NOTE: currently adding by bandwidth //
  /////////////////////////////////////////

  bool newPlaylistDataWasAdded = false;
  std::vector<CPlaylistData*>::iterator it = m_playlists.begin();
  for (;it != m_playlists.end(); it++)
  {
    CPlaylistData* playlistData = *it;
    if (!playlistData)
      continue;

    int bandwidth = playlistData->m_playlistBandwidth;

    if (newPlaylistBandwidth < bandwidth)
    {
      m_playlists.insert(it,newPlaylistData);
      newPlaylistDataWasAdded = true;
      break;
    }
  }

  // if not added -> new playlist bandwidth is the bigger one -> add as last
  if (!newPlaylistDataWasAdded)
  {
    m_playlists.push_back(newPlaylistData);
  }

  return true;
}

void CFilePlaylist::NextBuffer()
{
  CSingleLock lock(m_lock);
  if (m_buffersQueue.size() == 0)
    return;

  m_lastReportedTime = m_buffersQueue.front()->m_nBufferTime - m_prerollDuration + m_buffersQueue.front()->m_nDuration;
  delete m_buffersQueue.front();
  m_buffersQueue.pop_front();
  m_nLastBufferSwitch = time(NULL);  

  CAction action;
  action.id = ACTION_SYNC_AV;
  if (g_application.m_pPlayer)
    g_application.m_pPlayer->OnAction(action);    
}

unsigned int CFilePlaylist::ReadData(void* lpBuf, int64_t uiBufSize)
{
  CSingleLock lock(m_lock);

  if (m_eof && m_buffersQueue.size() == 0)
    return 0;

  while (m_buffersQueue.size() == 0 && !m_bStop)
  {
    // we do not have any input.
    // wait for something to come 
    lock.Leave();
    Sleep(50);
    lock.Enter();
    
    if (m_buffersQueue.size() == 0 && m_inProgressBuffer && m_inProgressBuffer->GetMaxReadSize() > 0)
    {
      int nSize = m_inProgressBuffer->GetMaxReadSize();
      if (nSize > uiBufSize)
        nSize = (int)uiBufSize;
      m_inProgressBuffer->ReadBinary((char *)lpBuf,nSize);
      return nSize;
    }
  }
  
  if (m_bStop || m_buffersQueue.size() == 0)
    return 0;
  
  bool bNeedReset = false;
  int nRead = 0;
  CRingBuffer *rb = m_buffersQueue.front()->m_buffer;
  int nMaxRead = rb->GetMaxReadSize();
  if (nMaxRead >= uiBufSize)
  {
    rb->ReadBinary(((char*)lpBuf) + nRead, (int)uiBufSize);
    if (rb->GetMaxReadSize() == 0)
    {
      bNeedReset = m_buffersQueue.front()->m_bNeedResetDemuxer;
      NextBuffer();
    }
    nRead += (int)uiBufSize;
  }
  else
  {
    rb->ReadBinary(((char*)lpBuf) + nRead, nMaxRead);
    bNeedReset = m_buffersQueue.front()->m_bNeedResetDemuxer;
    NextBuffer();
    nRead += nMaxRead;
  }
  
  if (bNeedReset)
    ResetDemuxer();
  
  return nRead;
}

const CStdString& CFilePlaylist::GetFingerprintBase64()
{
  return m_fingerprintBase64;
}

const CStdString& CFilePlaylist::GetKeyServerParams()
{
  return m_keyServerParams;
}

bool CFilePlaylist::GetEncryptKey(const CStdString& encryptKeyUri, CStdString& encryptKeyValue)
{
  // check that we have needed parameters
  CStdString keyServerParams   = GetKeyServerParams();
  CStdString fingerprintBase64 = GetFingerprintBase64();

  CStdString urlStr = encryptKeyUri;
  if (encryptKeyUri.find("?") != CStdString::npos)
    urlStr += "&";
  else
    urlStr += "?";
  urlStr += keyServerParams;

  BOXEE::BXCurl bxCurl;
  CStdString encryptionKeyEncryptedBase64 = bxCurl.HttpGetString(urlStr, false);

  if (encryptionKeyEncryptedBase64.IsEmpty())
  {
    CLog::Log(LOGERROR,"CFilePlaylist::GetEncryptKey - FAILED to get encryptionKey from [%s] (rfdj)",urlStr.c_str());
    return false;
  }

  CStdString fingerprint = CBase64::Decode(fingerprintBase64);
  CStdString encryptionKeyEncrypted = CBase64::Decode(encryptionKeyEncryptedBase64);

  /////////////////////
  // decrypt the key //
  /////////////////////

  unsigned char decryptedKey[64] = { 0 };
  int decryptKeySize = 0;
  int finalDecryptKeySize = 0;
  unsigned char iv[16] = {0};
  EVP_CIPHER_CTX deCtx;

  EVP_CIPHER_CTX_init(&deCtx);
  if (!EVP_DecryptInit_ex(&deCtx, EVP_aes_128_cbc(), NULL, (unsigned char*)fingerprint.data(), iv))
  {
    CLog::Log(LOGERROR,"CFilePlaylist::CReadFileDataJob::GetEncryptKey - FAILED to initialize DecryptCtx (rfdj)");
    return false;
  }

  EVP_CIPHER_CTX_set_padding(&deCtx, 0);

  if (!EVP_DecryptUpdate(&deCtx, decryptedKey, &decryptKeySize, (unsigned char*)encryptionKeyEncrypted.data(), encryptionKeyEncrypted.size()))
  {
    CLog::Log(LOGERROR,"CFilePlaylist::CReadFileDataJob::GetEncryptKey - FAILED to decrypt data (rfdj)");
    return false;
  }

  if (!EVP_DecryptFinal_ex(&deCtx, decryptedKey, &finalDecryptKeySize))
  {
    CLog::Log(LOGERROR,"CFilePlaylist::CReadFileDataJob::GetEncryptKey - FAILED to finalize decrypt data (rfdj)");
    return false;
  }

  EVP_CIPHER_CTX_cleanup(&deCtx);

  encryptKeyValue.assign((const char*)decryptedKey, decryptKeySize);

  CLog::Log(LOGDEBUG,"CFilePlaylist::CReadFileDataJob::GetEncryptKey - Succeeded to decrypt the encryption key (rfdj)");

  return true;
}

