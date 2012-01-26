#ifndef FILE_PLAYLIST_H_
#define FILE_PLAYLIST_H_

#include "IFile.h"
#include "File.h"
#include "PlayList.h"
#include "RingBuffer.h"
#include "lib/libBoxee/bxbgprocess.h"
#include "utils/CriticalSection.h"
#include "utils/Thread.h"
#include "Key.h"

#include <vector>
#include <deque>

namespace XFILE
{

class CPlaylistData 
{
public:
  CPlaylistData(PLAYLIST::CPlayList* playList);
  virtual ~CPlaylistData();

  int m_playlistBandwidth;
  int m_playlistLastPos;
  int m_playlistLastPosTime;
  int m_playlistSavedSeqNo;
  int m_playlistSavedTimeStamp;
  int m_targetDuration;
  int m_startDate;
  bool m_forceReload;
  unsigned int m_playlistLastRefreshTime;
  unsigned int m_playlistRefreshInterval;
  unsigned int m_playlistTotalTime;
  CStdString m_playlistPath;
  PLAYLIST::CPlayList* m_playList;
};

class BufferData
{
public:
  CRingBuffer *m_buffer;
  bool         m_bNeedResetDemuxer;
  bool         m_bDiscontinuity;
  int          m_nOriginPlaylist;
  int          m_nDuration;
  unsigned int m_nBufferTime;
  unsigned int m_nSeq;
  unsigned int m_nTimeStamp;

  BufferData();
  ~BufferData();
};
  
class CFilePlaylist : public IFile, public CThread
{
public:

  CFilePlaylist();
  virtual ~CFilePlaylist();

  virtual bool Open(const CURI& url);
  virtual bool Exists(const CURI& url);
  virtual int Stat(const CURI& url, struct __stat64* buffer);
  virtual unsigned int Read(void* lpBuf, int64_t uiBufSize);
  virtual int64_t Seek(int64_t iFilePosition, int iWhence = SEEK_SET);
  virtual void Close();
  virtual int64_t GetPosition();
  virtual int64_t GetLength();

  virtual CStdString GetContent();

  bool OnAction(const CAction &action);

  const CStdString& GetFingerprintBase64();
  const CStdString& GetKeyServerParams();

  bool SeekToTime(int nSecs);
  int  GetSeekTime(bool bPlus, bool bLargeStep, int smallSeekSkipSecs = -1, int bigSeekSkipSecs = -1);

  bool SeekChapter(int chapterStartTime);
  
  unsigned int GetStartTime();   // in seconds
  unsigned int GetCurrentTime(); // in seconds
  unsigned int GetTotalTime();   // in seconds
  
  bool IsEOF();
  void SetReadAheadBuffers(int nBuffers);
  bool IncQuality();
  bool DecQuality();

  //
  // load exactly 1 segment to the queue (will calculate required segment according to the last seq)
  //
  void ReadAhead();
  
  //
  // make sure the playlist is loaded and tuned on the right index
  // 
  bool ValidatePlaylist(CPlaylistData *pl);

  bool IsPlayingLiveStream();
  int GetLiveStartTime();
  int GetCurrentTimecode();
  int64_t GetAvailableRead();
  bool CanSeek();
  bool CanPause();

protected:
  virtual void Process(); 
  bool ParsePath(const CStdString& strPath);
  void ClearBuffers();
  void ResetDemuxer();
  void SetPlayerTime();
  void NextBuffer();

  PLAYLIST::CPlayList* BuildPlaylist(const CStdString& playlistPath, bool appendToPlaylist = false);
  unsigned int ReadData(void* lpBuf, int64_t uiBufSize);
  bool BuildPlaylistByBandwidth(PLAYLIST::CPlayList* playListOfPlaylists);
  bool InsertPlaylistDataToList(CFileItemPtr item, PLAYLIST::CPlayList* playlist);
  bool BuildFilesToReadList(CFileItemList& fileToReadList, int& newCurrFileIndex, bool& isEndOfPlaylist);
  bool GetEncryptKey(const CStdString& encryptKeyUri, CStdString& encryptKeyValue);
  bool AdjustLastPosBySavedSequenceNo(CPlaylistData *pl);
  bool AdjustLastPosBySavedTimestamp(CPlaylistData *pl);
  bool ReadAheadPlaylist(CPlaylistData *pl);  

  std::vector<CPlaylistData*> m_playlists;
  int                         m_nCurrPlaylist;
  unsigned int                m_nLastLoadedSeq;
  unsigned int                m_nStartTime;
  int                         m_nReadAheadBuffers;
  int                         m_nLastBufferSwitch; 
  unsigned int                m_nLastTimeStamp;

  unsigned int m_lastReportedTime;
  unsigned int m_lastBitrateChange;
  int          m_averageBitrateShift;

  CStdString m_playlistFilePath;
  CStdString m_fingerprintBase64;
  CStdString m_keyServerParams;
  CStdString m_bxOurl;
  CStdString m_trackingUrl;
  CStdString m_inningsIndex;
  CStdString m_quality;
  bool m_isLive;
  bool m_canSeek;
  bool       m_eof;
  bool       m_autoChooseQuality;
  int        m_startTime;       // where to start - in seconds
  int        m_prerollDuration; // where the actual stream starts (begining of the playlist can be just junk or preroll slide)
  CStdString m_preRollStrDuration;

  std::map<CStdString, CStdString> m_encryptKeyMap;

  std::deque<BufferData*> m_buffersQueue;
  CRingBuffer* m_inProgressBuffer;
  CCriticalSection m_lock;
};

}
#endif /*FILE_PLAYLIST_H_*/
