#ifndef __BOXEEVERSIONUPDATEMANAGER__H__
#define __BOXEEVERSIONUPDATEMANAGER__H__

#include "Thread.h"
#include "utils/CriticalSection.h"
#include "lib/libBoxee/bxxmldocument.h"
#include "StdString.h"

typedef enum {VUF_NO,VUF_YES,VUF_NUM_OF_TYPES} VERSION_UPDATE_FORCE;
typedef enum {VUJS_IDLE,VUJS_READY_FOR_INITIALIZE,VUJS_INITIALIZED,VUJS_WORKING,VUJS_READY_FOR_UPDATE,VUJS_ACQUIRE_FOR_UPDATE,VUJS_UPDATING,VUJS_NUM_OF_STATUS} VERSION_UPDATE_JOB_STATUS;

class CUpdateFilesInfo
{
public:
  CStdString m_filePath;
  CStdString m_fileHash;
};

class CVersionUpdateInfo
{
public:
  CVersionUpdateInfo();
  virtual ~CVersionUpdateInfo(){};

  static VERSION_UPDATE_FORCE GetVersionUpdateForceAsEnum(const CStdString& updateType);
  VERSION_UPDATE_FORCE GetVersionUpdateForce();

  void reset();

  CStdString m_versionUpdateForce;
  std::vector<CUpdateFilesInfo> m_UpdateFilesToDownload;
  CStdString m_scriptToRunName;
  CStdString m_UpdateNotesFileName;
  CStdString m_UpdateNotesFileText;
  CStdString m_UpdateVersionNum;
};

class CBoxeeVersionUpdateJob : public IRunnable
{
public:
  CBoxeeVersionUpdateJob();
  virtual ~CBoxeeVersionUpdateJob(){};
  
  bool Init(const CStdString& versionUpdateBuildNum,const CStdString& updateFilePath,const CStdString& versionUpdateFileHash,const CStdString& directoryForUpdateLocalPath = "");

  void reset();
  
  virtual void Run();
  
  const CStdString& GetVersionUpdateFilePath(){return m_versionUpdateFilePath;};
  
  const CStdString& GetVersionUpdateBuildNum(){return m_versionUpdateBuildNum;};
  
  void SetVersionUpdateJobStatus(VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus);
  VERSION_UPDATE_JOB_STATUS GetVersionUpdateJobStatus();
  static const CStdString GetVersionUpdateJobStatusAsString(VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus);
  
  CVersionUpdateInfo& GetVersionUpdateInfo();

  bool acquireVersionUpdateJobForInitializeUpdate(const CStdString& newVersion);
  bool acquireVersionUpdateJobForPerformUpdate();

  bool ParseUpdateFile(const CStdString& updateFileLocalPath);

  void UpdateFilesPathToLocal(const CStdString& directoryForUpdateLocalPath);

  bool ReadReleaseNoteFile();

private:

  bool IsThisNewVersion(CStdString currentVersion,CStdString newVersion);

  VERSION_UPDATE_JOB_STATUS m_VersionUpdateJobStatus;
  CCriticalSection m_versionUpdateJobStatusLock;

  CVersionUpdateInfo m_versionUpdateInfo;

  CStdString m_versionUpdateDirectoryPath;

  CStdString m_versionUpdateBuildNum;  
  CStdString m_versionUpdateFilePath;
  CStdString m_versionUpdateFileHash;
};

class CBoxeeVersionUpdateManager
{
public:
  CBoxeeVersionUpdateManager();
  virtual ~CBoxeeVersionUpdateManager();
  
  void reset();
  
  bool PrepareVersionUpdate(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash);
  bool PerformVersionUpdate();
  
  CBoxeeVersionUpdateJob& GetBoxeeVerUpdateJob();
  
#ifdef _LINUX
  void SetUserPassword(CStdString userPassword);
  const CStdString& GetUserPassword();
#endif

  static bool HandleUpdateVersionButton(bool inLoginScreen = false);
  
private:

  bool PrepareVersionUpdateFromLocal(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash,const CStdString& directoryForUpdateLocalPath);
  bool PrepareVersionUpdateFromRemote(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash);

#ifdef _LINUX
  bool PerformOsxVersionUpdate();
#else
  bool PerformWinVersionUpdate();
#endif
  
  static CCriticalSection m_versionUpdateRunScriptLock;

  CBoxeeVersionUpdateJob m_boxeeVerUpdateJob;

  CThread* m_prepareBoxeeVerUpdateThread;
  
  class ListenToUpdateScriptJob : public IRunnable
  {
  public:
#ifdef _LINUX
    ListenToUpdateScriptJob(int readPipe) : m_readPipe(readPipe),m_updateScriptMessage(""){};
    int m_readPipe;
#else
    ListenToUpdateScriptJob(HANDLE readPipe) : m_readPipe(readPipe){};
    HANDLE m_readPipe;
#endif
    
    virtual void Run();
    
    bool AnalyzeUpdateScriptPrints(const CStdString& str);
    
    CStdString m_updateScriptMessage;
  };

#ifdef _LINUX  
  pid_t m_pid;
  int m_readPipe[2];
  CStdString m_userPassword;
#else
  HANDLE m_pid;
  HANDLE m_sem;
  HANDLE m_memFile;
  HANDLE m_readHandle;           // pipe
  HANDLE m_outputHandle;           // pipe
  OVERLAPPED m_overlapped;
#endif
  
};

extern class CBoxeeVersionUpdateManager g_boxeeVersionUpdateManager;

#endif //__BOXEEVERSIONUPDATEMANAGER__H__
