#ifndef __WATCHDOG__H__
#define __WATCHDOG__H__

#include "Settings.h"
#include <map>

#include "utils/CriticalSection.h"

#include "lib/libBoxee/bxscheduletaskmanager.h"

class IWatchDogListener
{
public:
  virtual ~IWatchDogListener() { }
  virtual void PathUpdate(const CStdString &strPath, bool bAvailable) = 0;
};

typedef enum { WD_UNAVAILABLE=0, WD_AVAILABLE, WD_UNKNOWN } PathStatus;

class WatchDog
{
public:
  WatchDog();
  virtual ~WatchDog();

  bool Start();
  bool StartGeneralSTM();
  bool StartProfileSTM();
  bool Stop();

  bool IsStoped();

  bool IsPathAvailable(const CStdString &pathToCheck, bool bDefault=true); // default will be returned if availability is unknown

  void SetIsConnectedToServer(bool bIsConnectToServer);
  bool IsConnectedToServer();

  void SetIsConnectedToInternet(bool bIsConnectToInternet);
  void SetConnectionToInternetChanged();
  bool IsConnectedToInternet(bool checkNow = false);

  void AddPathToWatch(const char *strPath);
  void RemovePathFromWatch(const char *strPath);
  void CleanWatchedPaths();
  bool IsPathWatched(const char *strPath);
  
  void AddListener(IWatchDogListener* pListener);
  void RemoveListener(IWatchDogListener* pListener);
  void NotifyListeners(const CStdString &strPath, bool bAvailable);

protected:

  bool CheckIsConnectedToInternet(bool fromCheckInternetJob = false);

  void  TestPath(const CStdString &strPath);
  void  ProcessSlow();
  
  bool m_bConnectedToServer;
  bool m_bIsConnectToInternet;
  bool m_bConnectionToInternetChanged;
  
  bool m_isStoped;

  CCriticalSection m_lock;
  std::map<CStdString, PathStatus> m_mapPaths;
  std::vector<IWatchDogListener*> m_vecListeners;

  BOXEE::CBoxeeScheduleTaskManager m_profileBaseSTM;
  BOXEE::CBoxeeScheduleTaskManager m_generalSTM;

private:

  class CPingJob : public BOXEE::BoxeeScheduleTask
  {
  public:
    CPingJob(WatchDog* jobHandler,unsigned long executionDelayInMS,unsigned long repeatTaskIntervalInMS);
    virtual ~CPingJob();
    virtual void DoWork();

    virtual bool ShouldDelete();

  protected:

    bool InitPingRequest(CStdString& pingUrl,CStdString& strPingVersion);

    WatchDog* m_jobHandler;
  };

  CPingJob* m_pingJob;

#ifdef HAS_EMBEDDED
 
  class CChkupdJob : public BOXEE::BoxeeScheduleTask
  {
  public:
    CChkupdJob(WatchDog* jobHandler,unsigned long executionDelayInMS,unsigned long repeatTaskIntervalInMS);
    virtual ~CChkupdJob();
    virtual void DoWork();

    virtual bool ShouldDelete();

  protected:

    WatchDog* m_jobHandler;
  };

  CChkupdJob* m_chkupdJob;

#endif

  class CTestPathJob : public BOXEE::BoxeeScheduleTask
  {
  public:
    CTestPathJob(WatchDog* jobHandler,unsigned long executionDelayInMS,unsigned long repeatTaskIntervalInMS);
    virtual ~CTestPathJob();
    virtual void DoWork();

    virtual bool ShouldDelete();

  protected:

    WatchDog* m_jobHandler;
  };

  CTestPathJob* m_testPathJob;

  class CProcessSlowJob : public BOXEE::BoxeeScheduleTask
  {
  public:
    CProcessSlowJob(WatchDog* jobHandler,unsigned long executionDelayInMS,unsigned long repeatTaskIntervalInMS);
    virtual ~CProcessSlowJob();
    virtual void DoWork();

    virtual bool ShouldDelete();

  protected:

    WatchDog* m_jobHandler;
  };

  CProcessSlowJob* m_processSlowJob;

  class CCheckInternetConnectionJob : public BOXEE::BoxeeScheduleTask
  {
  public:
    CCheckInternetConnectionJob(WatchDog* jobHandler,unsigned long executionDelayInMS,unsigned long repeatTaskIntervalInMS);
    virtual ~CCheckInternetConnectionJob();
    virtual void DoWork();

    virtual bool ShouldDelete();

  protected:

    bool HandleInternetConnectionRestore();
    bool HandleInternetConnectionLost();
    bool RefreshCountry();

    bool LoginUserAfterInternetConnectionRestore();

    WatchDog* m_jobHandler;
};

  CCheckInternetConnectionJob* m_checkInternetConnectionJob;

  CCriticalSection m_ServersUrlsVecLock;
  std::vector<CStdString> m_vecBoxeeServersUrls;
};

#endif //__WATCHDOG__H__
