#ifndef FILESCANNER_H_
#define FILESCANNER_H_

#include <string>
#include <vector>

#include <time.h>
#include "lib/libBoxee/bxmetadataengine.h"
#include <SDL/SDL.h>
#include "FileItem.h"
#include "utils/Thread.h"
#include "Settings.h"
#include <set>

class CMetadataResolver;

#define FOLDER_STATUS_NONE      1 // the folder can not be added as share (path like sources://all)
#define FOLDER_STATUS_NOT_SHARE 2 // the folder is not on any share
#define FOLDER_STATUS_ON_SHARE  3 // the folder under a  non private share
#define FOLDER_STATUS_PRIVATE   4 // the folder under a private share (for either videos or music)
#define FOLDER_STATUS_SCANNING  5 // the folder is being queued for scanning

/**
 * File scanner that goes over all files it can find
 * on all shares and reports them to the Boxee metadata engine
 */
class CFileScanner : public CThread
{
public:
  CFileScanner();
  virtual ~CFileScanner();

  bool Init();
  void Start();
  void Stop();
  void Pause();
  void Resume();
  void CheckPause();
	
	void AddUserPath(const CStdString& strPath);
	bool IsScanning(const CStdString& _strPath);
		
  int GetFolderStatus(const CStdString& strPath, std::vector<CStdString>& vecTypes);
		
  void InformRemoveShare(const CStdString sharePath);

  static bool ShowSourcesStatusKaiDialog(int messageId, const CStdString& sourceName, const CStdString& sourcePath = "", const CStdString& sourceType = "");

private:

  void InitSourcesOnStart();

  void Reset();
	
  void InterruptibleSleep(unsigned int milliseconds);

  void Process();
  
  void ScanShares(VECSOURCES * pVecShares, const CStdString& strShareType);
  void ScanShare(CMediaSource* pShare, const CStdString& strShareType);
  void ScanMediaFolder(CStdString& strPath, const CStdString& strShareType, const CStdString& strSharePath);
  void SynchronizeFolderAndDatabase(const CStdString& strPath, const CStdString& strShareType,  const CStdString& strSharePath, CFileItemList &folderitems);
  void CleanMediaFolders(const CStdString& strType, VECSOURCES& vecShares);
  void CleanChildFolders(const CStdString& strSharePath, const CStdString& strPath);
  bool ShouldScan(const CMediaSource* pShare);
  
  bool PathInPrivateShare(const CStdString& strPath);
  bool IsScannable(const CStdString& strPath);
  
  void UpdateScanLabel(const CStdString& path);

  // Local cache that maps folder path to the last modification time
  std::map<std::string, time_t> m_mapScannedFolders;
    
  // black list of paths we do not scan
  std::set<CStdString> m_noScanPaths;
  std::set<CStdString> m_PrivatePaths;
  
  // User paths, is the list of paths that the user has requested to be rescanned manually
  CCriticalSection m_lock;
  std::set<std::pair<CStdString, CStdString> > m_setUserPaths;

  bool    m_bPaused;
  SDL_mutex  *m_pPauseLock;
  SDL_cond   *m_pPauseCond;

  BOXEE::BXMetadataEngine* m_MDE; 
  CMetadataResolver* m_pResolver;

  bool m_bExtendedLog;
  
  CCriticalSection m_shareLock;
  CStdString       m_currenlyScannedShare;
  bool             m_currentShareValid;

  HANDLE           m_hStopEvent;
};

#endif /*FILESCANNER_H_*/
