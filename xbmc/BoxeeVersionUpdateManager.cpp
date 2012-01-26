
#include "BoxeeVersionUpdateManager.h"
#include "Util.h"
#include "utils/GUIInfoManager.h"
#include "BoxeeUtils.h"
#include "File.h"
#include "GUIDialogBoxeeUpdateProgress.h"
#include "GUIWindowManager.h"
#include <sys/stat.h>
#include "GUIDialogKeyboard.h"
#include "Application.h"
#include "Directory.h"
#include "GUIDialogOK2.h"
#include "GUIDialogBoxeeUpdateMessage.h"
#include "SpecialProtocol.h"
#include "utils/log.h"
#include "utils/SingleLock.h"
#include "LocalizeStrings.h"

#ifdef HAS_EMBEDDED
#include "GUISettings.h"
#include "GUIDialogProgress.h"
#include "HalServices.h"
#include "FileCurl.h"
#include "Util.h"
#include "TimeUtils.h"
#include "bxconfiguration.h"
#endif

#ifdef _WIN32
#include <io.h>
#include <process.h>
#endif

#define UPDATE_DIRECTORY_BASE_PATH _P("/download/upgrade/")
//#define UPDATE_DIRECTORY_BASE_PATH _P("special://home/boxee/packages/")
//#define UPDATE_DIRECTORY_BASE_PATH PTH_IC("C:\\Users\\Popeye\\AppData\\Roaming\\BOXEE\\userdata\\boxee\\packages\\")

#define BUFFER_SIZE 1024
#define READ_TIMEOUT 1000

#if defined(_LINUX) && !defined(__APPLE__)
#include <sys/types.h>
#include <sys/wait.h>
#endif

using namespace XFILE;
using namespace DIRECTORY;

class CBoxeeVersionUpdateManager g_boxeeVersionUpdateManager;

CBoxeeVersionUpdateManager::CBoxeeVersionUpdateManager()
{
  m_prepareBoxeeVerUpdateThread = NULL;

#ifdef _LINUX

  m_userPassword = "";
  
#else
  m_pid = NULL;
  m_sem = NULL;
  m_memFile = NULL;
  m_readHandle = NULL;
#endif
}

CBoxeeVersionUpdateManager::~CBoxeeVersionUpdateManager()
{
  
}

void CBoxeeVersionUpdateManager::reset()
{
#ifdef _LINUX

  m_userPassword = "";
  
#endif
  
  m_boxeeVerUpdateJob.reset();

  if(m_prepareBoxeeVerUpdateThread)
  {
    delete m_prepareBoxeeVerUpdateThread;
    m_prepareBoxeeVerUpdateThread = NULL;    
  }
}

bool CBoxeeVersionUpdateManager::ShouldInstallFromLocal(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash,const CStdString& directoryForUpdateLocalPath)
{
  bool retval = false;
  struct stat st;

  CStdString updateDirectory = directoryForUpdateLocalPath;
  CStdString tmpDirectoryForUpdateLocalPath = directoryForUpdateLocalPath;
  tmpDirectoryForUpdateLocalPath += "_tmp";
  
  if(stat(tmpDirectoryForUpdateLocalPath,&st) == 0)
  {
    updateDirectory = tmpDirectoryForUpdateLocalPath;
  }

  if(stat(updateDirectory,&st) == 0)
  {
    CBoxeeVersionUpdateJob boxeeVerUpdateJob;
    bool initSucceeded = boxeeVerUpdateJob.Init(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash,updateDirectory);
  
    if(initSucceeded)
    {

      CStdString updateFileLocalPath = boxeeVerUpdateJob.GetVersionUpdateFilePath();

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal- Going to parse [m_versionUpdateFilePath=%s] (update)",updateFileLocalPath.c_str());

      bool parseSucceeded = boxeeVerUpdateJob.ParseUpdateFile(updateFileLocalPath);

      if(parseSucceeded)
      {
         CVersionUpdateInfo& versionUpdateInfo = boxeeVerUpdateJob.GetVersionUpdateInfo();
         int numOfFilesToDownload = versionUpdateInfo.m_UpdateFilesToDownload.size();

         for(int i=0; i<numOfFilesToDownload; i++)
         {
           CUpdateFilesInfo& ufi = (versionUpdateInfo.m_UpdateFilesToDownload)[i];
           CStdString strName = CUtil::GetFileName(ufi.m_filePath);
           CStdString downloadFileLocalPath = updateDirectory;

           downloadFileLocalPath += "/";
           downloadFileLocalPath += strName;

           if(!XFILE::CFile::Exists(downloadFileLocalPath))
           {
              CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal- Local file [%s] is missing(update)",downloadFileLocalPath.c_str());
              retval = false;
              break;
           }
           else
           {            
             if(CUtil::MD5File(downloadFileLocalPath) != ufi.m_fileHash)
             {
               CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal- Local file [%s] is corrupted(update)",downloadFileLocalPath.c_str());
               retval = false;
               break;
             }
             
             CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal- Local file [%s] is OK(update)",downloadFileLocalPath.c_str());
             retval = true;             
           }           
         }

        if(retval == true)
        {
          if(updateDirectory != directoryForUpdateLocalPath)
          {
            int renameFailed = ::rename(updateDirectory,directoryForUpdateLocalPath);
           
            if(renameFailed)
            {
              CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal- Failed to rename [%s] ==> [%s]",updateDirectory.c_str(), directoryForUpdateLocalPath.c_str());
              CUtil::WipeDir(updateDirectory);
              retval = false;
            }
          }
        }
        else
        {  
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::ShouldInstallFromLocal - One of the installation files is missing or corrupted. Going to remove the update directory [%s] (update)",updateDirectory.c_str());
          CUtil::WipeDir(updateDirectory);
        }   
      }
    }
  }

  return retval;
}

bool CBoxeeVersionUpdateManager::PrepareVersionUpdate(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash)
{
  bool retVal = false;
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Enter function with [versionUpdateBuildNum=%s][versionUpdateFilePath=%s][versionUpdateFileHash=%s] (update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Going to call acquireVersionUpdateJobForInitializeUpdate() with [versionUpdateBuildNum=%s] (update)",versionUpdateBuildNum.c_str());

  if(m_boxeeVerUpdateJob.acquireVersionUpdateJobForInitializeUpdate(versionUpdateBuildNum))
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call to acquireVersionUpdateJobForInitializeUpdate() returned TRUE for new version [build-num=%s]. Going to check if update was already downloaded (update)",versionUpdateBuildNum.c_str());

    ////////////////////////////////////////////////////////////
    // Check if we already downloaded this version for update //
    ////////////////////////////////////////////////////////////

    CStdString directoryForUpdateLocalPath = UPDATE_DIRECTORY_BASE_PATH;
    directoryForUpdateLocalPath += versionUpdateBuildNum;
    
    if(ShouldInstallFromLocal(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash,directoryForUpdateLocalPath))
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - The directory for the update [%s] already exist [%s], therefore no need to download. Going to call PrepareVersionUpdateFromLocal() (update)",versionUpdateBuildNum.c_str(),directoryForUpdateLocalPath.c_str());
      
      bool prepareFromLoaclSucceeded = PrepareVersionUpdateFromLocal(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash,directoryForUpdateLocalPath);
      
      if(prepareFromLoaclSucceeded)
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call PrepareVersionUpdateFromLocal() returned [%d] (update)",prepareFromLoaclSucceeded);
        
        retVal = true;
      }
      else
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call PrepareVersionUpdateFromLocal() returned [%d] (update)",prepareFromLoaclSucceeded);

        reset();
      }
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - The directory for the update [%s] DOESN'T exist, therefore need to download. Going to call PrepareVersionUpdateFromRemote() (update)",versionUpdateBuildNum.c_str());
      
      bool prepareFromRemoteSucceeded = PrepareVersionUpdateFromRemote(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash);

      if(prepareFromRemoteSucceeded)
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call PrepareVersionUpdateFromRemote() returned [%d] (update)",prepareFromRemoteSucceeded);

        retVal = true;
      }
      else
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call PrepareVersionUpdateFromRemote() returned [%d] (update)",prepareFromRemoteSucceeded);

        reset();
      }
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdate - Call to acquireVersionUpdateJobForInitializeUpdate() returned FALSE for new version [build-num=%s] (update)",versionUpdateBuildNum.c_str());
  }
  
  return retVal;    
}

bool CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash,const CStdString& directoryForUpdateLocalPath)
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Enter function with [versionUpdateBuildNum=%s][versionUpdateFilePath=%s][versionUpdateFileHash=%s][directoryForUpdateLocalPath=%s] (update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str(),directoryForUpdateLocalPath.c_str());

  bool retVal = false;

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Going to call CBoxeeVersionUpdateJob::Init() (update)");

  bool initSucceeded = m_boxeeVerUpdateJob.Init(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash,directoryForUpdateLocalPath);
  
  if(initSucceeded)
  {
    VERSION_UPDATE_JOB_STATUS verUpdateJobStatus = m_boxeeVerUpdateJob.GetVersionUpdateJobStatus();
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Call CBoxeeVersionUpdateJob::Init() returned TRUE. VerUpdateJob status is [%d=%s] (update)",verUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(verUpdateJobStatus)).c_str());

    //////////////////////////////////////////////////////////////////////////////
    // Init of CVersionUpdateInfo was succeeded -> Need to parse UpdateFilePath //
    //////////////////////////////////////////////////////////////////////////////

    CStdString updateFileLocalPath = m_boxeeVerUpdateJob.GetVersionUpdateFilePath();

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Going to parse [m_versionUpdateFilePath=%s] (update)",updateFileLocalPath.c_str());

    bool parseSucceeded = m_boxeeVerUpdateJob.ParseUpdateFile(updateFileLocalPath);

    if(parseSucceeded)
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Succeed to parse [updateFileLocalPath=%s]. Going to call CBoxeeVersionUpdateJob::UpdateFilesPathToLocal() (update)",updateFileLocalPath.c_str());

      ////////////////////////////////////
      // Update file path to local ones //
      ////////////////////////////////////
      
      m_boxeeVerUpdateJob.UpdateFilesPathToLocal(directoryForUpdateLocalPath);
            
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - After call to CBoxeeVersionUpdateJob::UpdateFilesPathToLocal(). Going to call ReadReleaseNoteFile() (update)");

      ////////////////////////////
      // Read the release notes //
      ////////////////////////////
      
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Going to call ReadReleaseNoteFile() for reading the release notes file (update)");

      bool succeedToRead = m_boxeeVerUpdateJob.ReadReleaseNoteFile();
      
      if(succeedToRead)
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Call ReadReleaseNoteFile() was successful (update)");        
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Call ReadReleaseNoteFile() failed. No release notes will be shown (update)");
        (m_boxeeVerUpdateJob.GetVersionUpdateInfo()).m_UpdateNotesFileText = "";
      }        

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - After call to CBoxeeVersionUpdateJob::ReadReleaseNoteFile(). Going to set VersionUpdateJob Status to [VUJS_READY_FOR_UPDATE] (update)");

      m_boxeeVerUpdateJob.SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Going to set NewVersion flag in CGUIInfoManager as TRUE (update)");

      g_infoManager.SetHasNewVersion(true,m_boxeeVerUpdateJob.GetVersionUpdateInfo().GetVersionUpdateForce() == VUF_YES ? true : false);

#ifdef HAS_EMBEDDED
      m_boxeeVerUpdateJob.SetVersionUpdateDownloadStatus(VUDS_FINISHED);
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - After set [Status=VUDS_FINISHED] (update)");
#endif
  
      retVal = true;
    }
    else
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::PrepareVersionUpdateFromLocal - Failed to parse UpdateFile [updateFileLocalPath=%s]. Going to remove the UpdateVersion directory [%s] (update)",updateFileLocalPath.c_str(),directoryForUpdateLocalPath.c_str());
      
      CUtil::WipeDir(directoryForUpdateLocalPath);
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Call CBoxeeVersionUpdateJob::Init() returned FALSE. Going to remove the UpdateVersion directory [%s] (update)",directoryForUpdateLocalPath.c_str());
    
    /////////////////////////////////////////////////////////////////////////////////
    // Remove directory in order for next time update will be downloaded correctly //
    /////////////////////////////////////////////////////////////////////////////////
    
    CUtil::WipeDir(directoryForUpdateLocalPath);
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromLocal - Exit function and return [%d] (update)",retVal);

  return retVal;
}

bool CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote(const CStdString& versionUpdateBuildNum,const CStdString& versionUpdateFilePath,const CStdString& versionUpdateFileHash)
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - Enter function with [versionUpdateBuildNum=%s][versionUpdateFilePath=%s][versionUpdateType=%s]. Going to call Init() (update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

  bool retVal = false;
  
  bool initSucceeded = m_boxeeVerUpdateJob.Init(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash);
  
  if(initSucceeded)
  {
    VERSION_UPDATE_JOB_STATUS verUpdateJobStatus = m_boxeeVerUpdateJob.GetVersionUpdateJobStatus();
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - Call CBoxeeVersionUpdateJob::Init() returned TRUE. VerUpdateJob status is [%d=%s] (update)",verUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(verUpdateJobStatus)).c_str());

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    // Init of CVersionUpdateInfo was succeeded -> Open a new thread for handling the versionUpdateFilePath //
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
  
    if(m_prepareBoxeeVerUpdateThread)
    {                          
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - The PrepareForVerUpdateThread object exist. Going to delete it before handling <version_update> (update)");

      delete m_prepareBoxeeVerUpdateThread;
      m_prepareBoxeeVerUpdateThread = NULL;
    }
  
    m_prepareBoxeeVerUpdateThread = new CThread(&m_boxeeVerUpdateJob);
  
    if(m_prepareBoxeeVerUpdateThread)
    {        
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - A new PrepareForVerUpdateThread object was created. Going to start it by calling m_prepareVerUpdateThread->Create() (update)");

      m_prepareBoxeeVerUpdateThread->Create();
      
      retVal = true;
    }
    else
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - Failed to allocate CThread object for CBoxeeVersionUpdateJob (update)");
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PrepareVersionUpdateFromRemote - Call CBoxeeVersionUpdateJob::Init() returned FALSE (update)");        
  }
  
  return retVal;
}

bool CBoxeeVersionUpdateManager::PerformVersionUpdate()
{
  bool retVal = false;
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Enter function. Going to call acquireVersionUpdateJobForPerformUpdate() (update)");

  if(m_boxeeVerUpdateJob.acquireVersionUpdateJobForPerformUpdate())
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Call to acquireVersionUpdateJobForPerformUpdate() returned TRUE. VersionUpdateJobStatus is [%d=%s] (update)",m_boxeeVerUpdateJob.GetVersionUpdateJobStatus(),(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_boxeeVerUpdateJob.GetVersionUpdateJobStatus())).c_str());

#ifdef _LINUX
          
    /////////////////////////////
    // APPLE and LINUX section //
    /////////////////////////////

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Going to call PerformOsxVersionUpdate() (update)");

    retVal = PerformOsxVersionUpdate();
        
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Call to PerformOsxVersionUpdate() returned [%d] (update)",retVal);

#else

    /////////////////////
    // WINDOWS section //
    /////////////////////

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Going to call PerformWinVersionUpdate() (update)");
        
    retVal = PerformWinVersionUpdate();
        
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Call to PerformWinVersionUpdate() returned [%d] (update)",retVal);

#endif
      
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - Call to acquireVersionUpdateJobForPerformUpdate() returned FALSE. VersionUpdateJobStatus is [%d=%s] (update)",m_boxeeVerUpdateJob.GetVersionUpdateJobStatus(),(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_boxeeVerUpdateJob.GetVersionUpdateJobStatus())).c_str());
  }
  
  return retVal;
}

#ifdef _LINUX

bool CBoxeeVersionUpdateManager::PerformOsxVersionUpdate()
{
  bool retVal = false;
      
  //signal(SIGCHLD,SIG_IGN);

  if (pipe(m_readPipe) == -1)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - Call pipe(m_readPipe) failed (update)");            
    return false;    
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - Going to call fork() (update)");
    
  m_pid = fork();
           
  if(m_pid == -1)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - Create a child process for update failed. Going to change VersionUpdateJob status back to [VUJS_READY_FOR_UPDATE] (update)");

    m_boxeeVerUpdateJob.SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);
  }
  else if(m_pid == 0)
  {
    //////////////
    // In child //
    //////////////

    dup2(m_readPipe[1], fileno(stdout));
              
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - Enter the child process [m_pid=%d]. Going to get parentPID (update)",m_pid);

    pid_t parentPID = getppid();

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In child process, got [parentPID=%d]. Going to call setsid() (update)",parentPID);

    pid_t sid = setsid();
            
    if(sid < 0)
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In child process, call to setsid() returned [sid=%d] -> Exit (update)",sid); 
      exit(0);
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In child process, call to setsid() returned [sid=%d] (update)",sid);
                
      CVersionUpdateInfo& versionUpdateInfo = m_boxeeVerUpdateJob.GetVersionUpdateInfo(); 

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, got CVersionUpdateInfo object. [m_versionUpdateForce=%s][m_scriptToRunName=%s][m_UpdateNotesFileName=%s][m_UpdateVersionNum=%s][m_UpdateFilesToDownloadSize=%zu] (update)",(versionUpdateInfo.m_versionUpdateForce).c_str(),(versionUpdateInfo.m_scriptToRunName).c_str(),(versionUpdateInfo.m_UpdateNotesFileName).c_str(),(versionUpdateInfo.m_UpdateVersionNum).c_str(),(versionUpdateInfo.m_UpdateFilesToDownload).size());
              
      ////////////////////////////////////////////////////
      // Get the scriptToRun path for the execl command //
      ////////////////////////////////////////////////////
              
      CStdString scriptToRunPath = "";
                 
      for(size_t i=0;i<(versionUpdateInfo.m_UpdateFilesToDownload).size();i++)
      {
        CUpdateFilesInfo& ufi = (versionUpdateInfo.m_UpdateFilesToDownload)[i];

        CStdString strName = CUtil::GetFileName(ufi.m_filePath);
                   
        if((versionUpdateInfo.m_scriptToRunName) == strName)
        {
          scriptToRunPath += ufi.m_filePath;
          break;
        }
      }

      if(scriptToRunPath.IsEmpty())
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In child process, Failed to find scriptToRunPath [%s] -> Exit (update)",scriptToRunPath.c_str());
        exit(0);                
      }

      CStdString userPassword = g_boxeeVersionUpdateManager.GetUserPassword();

      CStdString scriptToRunLocationPath = "";
      CUtil::GetParentPath(scriptToRunPath,scriptToRunLocationPath);
                 
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, after set [scriptToRunPath=%s][scriptToRunLocationPath=%s] (update)",scriptToRunPath.c_str(),scriptToRunLocationPath.c_str());
                
      char parentPIDStr[8];
      sprintf(parentPIDStr,"%d",parentPID);

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, woke up. Going to call execl() with [scriptToRunPath=%s][scriptToRunName=%s][scriptToRunLocationPath=%s][userPassword=%s][parentPIDStr=%s] (update)",scriptToRunPath.c_str(),(versionUpdateInfo.m_scriptToRunName).c_str(),scriptToRunLocationPath.c_str(),userPassword.c_str(),parentPIDStr);

      int retVal = execl("/bin/sh",scriptToRunPath.c_str(),scriptToRunPath.c_str(),(versionUpdateInfo.m_scriptToRunName).c_str(),scriptToRunLocationPath.c_str(),userPassword.c_str(),parentPIDStr,NULL);
              
      if(retVal != 0)
      { 
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, call to execl with [scriptToRunPath=%s][scriptToRunName=%s][scriptToRunLocationPath=%s][userPassword=%s][parentPIDStr=%s] FAILED. [retVal=%d][errno=%d] (update)",scriptToRunPath.c_str(),(versionUpdateInfo.m_scriptToRunName).c_str(),scriptToRunLocationPath.c_str(),userPassword.c_str(),parentPIDStr,retVal,errno);
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, call to execl with [scriptToRunPath=%s][scriptToRunName=%s][scriptToRunLocationPath=%s][userPassword=%s][parentPIDStr=%s] succeeded. [retVal=%d] (update)",scriptToRunPath.c_str(),(versionUpdateInfo.m_scriptToRunName).c_str(),scriptToRunLocationPath.c_str(),userPassword.c_str(),parentPIDStr,retVal);          
      }

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the child process, going to exit... (update)");
         
      exit(0);              
    }
  }
  else
  {            
    ///////////////
    // In parent //
    ///////////////

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - Enter the parent process. Child pid is [m_pid=%d] and going to start a thread for reading the child process script outputs (update)",m_pid);

    // Going to start a thread reading the child process script outputs
    ListenToUpdateScriptJob ListenToUpdateScript(m_readPipe[0]);      
    CThread thread(&ListenToUpdateScript);

    thread.Create();

    // Going to show the UpdateProgressDialog
    CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);
    if (dialog)
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the parent process. Going to show CGUIDialogBoxeeUpdateProgress (update)");
      dialog->DoModal();
    }
    else
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the parent process. Failed to get CGUIDialogBoxeeUpdateProgress. (update)");        
    }

    /////////////////////////////////////////////////////////
    // If we reached here there was an error in the update //
    /////////////////////////////////////////////////////////

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the parent process. Returned from CGUIDialogBoxeeUpdateProgress which mean that there was an error [m_updateScriptMessage=%s] (update)",(ListenToUpdateScript.m_updateScriptMessage).c_str());

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In the parent process. Going to stop the thread for reading the child process script outputs (update)");
      
    thread.StopThread();
      
    ////////////////////////////////////////////
    // Notify the user about the update error //
    ////////////////////////////////////////////
      
    if(!(ListenToUpdateScript.m_updateScriptMessage).IsEmpty())
    {
      CStdString heading = g_localizeStrings.Get(53240);
      CStdString line = g_localizeStrings.Get(53241);
      line += "[CR][CR]";
      line += ListenToUpdateScript.m_updateScriptMessage;
        
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In parent process. Going to open CGUIDialogOK2 with [heading=%s][line=%s] (update)(test)",heading.c_str(),line.c_str());
        
      CGUIDialogOK2::ShowAndGetInput(heading,line);
    }

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformOsxVersionUpdate - In parent process. Going to close the pipes and going to change the VersionUpdateJob status to [VUJS_READY_FOR_UPDATE] (update)");

    // close the pipes
    if (m_readPipe[0] != -1)
    {
      close(m_readPipe[0]);
      m_readPipe[0] = -1;
    }

    if (m_readPipe[1] != -1)
    {
      close(m_readPipe[1]);
      m_readPipe[1] = -1;
    }
      
    m_boxeeVerUpdateJob.SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);
  }

  return retVal;
}

#endif

#ifdef _WIN32
bool CBoxeeVersionUpdateManager::PerformWinVersionUpdate()
{
  bool retVal = false;

  ////////////////////////////////////////////////////////////
  // Get the scriptToRun path for the CreateProcess command //
  ////////////////////////////////////////////////////////////
          
  CVersionUpdateInfo& versionUpdateInfo = m_boxeeVerUpdateJob.GetVersionUpdateInfo(); 

  CStdString scriptToRunPath = "";
                 
  for(size_t i=0;i<(versionUpdateInfo.m_UpdateFilesToDownload).size();i++)
  {
    CUpdateFilesInfo& ufi = (versionUpdateInfo.m_UpdateFilesToDownload)[i];

    CStdString strName = CUtil::GetFileName(ufi.m_filePath);
                   
    if((versionUpdateInfo.m_scriptToRunName) == strName)
    {
      scriptToRunPath += ufi.m_filePath;
      break;
    }
  }

  if(scriptToRunPath.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - Failed to find scriptToRunPath [%s] -> Exit (update)",scriptToRunPath.c_str());               
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - scriptToRunPath was set [%s] (update)",scriptToRunPath.c_str());
    
    HANDLE hOutputRead;
    HANDLE hInputWrite;
    
    SECURITY_ATTRIBUTES sa;
    sa.nLength= sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;
    
    if (!CreatePipe(&m_readHandle,&hInputWrite,&sa,0))
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - Call CreatePipe() for input pipe failed (update)");
      retVal = false;    
    }
    else
    {
      if (!CreatePipe(&hOutputRead,&m_outputHandle,&sa,0))
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - Call CreatePipe() for output pipe failed (update)");
        retVal = false;    
      }
      else
      {
        PROCESS_INFORMATION pi;
        STARTUPINFO si;

        ZeroMemory(&si,sizeof(STARTUPINFO));
        si.cb = sizeof(STARTUPINFO);
        si.dwFlags = STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
        si.hStdOutput = hInputWrite;
        si.hStdInput  = hOutputRead;
        si.hStdError  = GetStdHandle(STD_ERROR_HANDLE);
        si.wShowWindow = SW_SHOWNOACTIVATE;
        
        /////////////////////////////////////////////////
        // Build the command for runing the batch file //
        /////////////////////////////////////////////////
        
        CStdString scriptToRunLocationPath = "";
        CUtil::GetParentPath(scriptToRunPath,scriptToRunLocationPath);
        
        int boxeePID = ::getpid();
        char boxeePIDStr[8];
        sprintf(boxeePIDStr,"%d",boxeePID);
        
        scriptToRunPath = "\"" + scriptToRunPath;
        scriptToRunPath += "\"";
        scriptToRunPath += " ";
        scriptToRunPath += boxeePIDStr;
        scriptToRunPath += " \"";
        scriptToRunPath += scriptToRunLocationPath;
        scriptToRunPath += "\"";

        char cmd[4096];
        strcpy(cmd,scriptToRunPath);
        CStdString strWorkingPath = "";
        
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - Going to call CreateProcess() for [cmd=%s] (update)",cmd);

        STARTUPINFO startupInfo = {0};
        startupInfo.cb = sizeof(startupInfo);
        
        // Try to start the process
        BOOL createProcessSucceed = ::CreateProcess(NULL,cmd,NULL,NULL,TRUE,NORMAL_PRIORITY_CLASS | CREATE_NO_WINDOW,NULL,NULL,&si,&pi);
        
        if (createProcessSucceed)
        {
          //////////////////////
          // In Boxee process //
          //////////////////////
            
          CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - New process for update was created successfully. Going to start a thread with [m_readHandle=%p=%d] for reading the update process batch file outputs (update)",m_readHandle,m_readHandle);
          
          // Going to start a thread reading the child process script outputs
          ListenToUpdateScriptJob ListenToUpdateScript(m_readHandle);
          CThread thread(&ListenToUpdateScript);
          
          thread.Create();
          
          // Going to show the UpdateProgressDialog
          CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);
          
          if (dialog)
          {
            CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Going to show CGUIDialogBoxeeUpdateProgress (update)");
            dialog->DoModal();
          }
          else
          {
            CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Failed to get CGUIDialogBoxeeUpdateProgress. (update)");
          }
          
          /////////////////////////////////////////////////////////
          // If we reached here there was an error in the update //
          /////////////////////////////////////////////////////////
		  
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Returned from CGUIDialogBoxeeUpdateProgress which mean that there was an error [m_updateScriptMessage=%s] (update)",(ListenToUpdateScript.m_updateScriptMessage).c_str());

          CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Going to stop the thread for reading the child process script outputs (update)");
      
          thread.StopThread();
      
          ////////////////////////////////////////////
          // Notify the user about the update error //
          ////////////////////////////////////////////
      
          if(!(ListenToUpdateScript.m_updateScriptMessage).IsEmpty())
          {
            CStdString heading = g_localizeStrings.Get(53240);
            CStdString line = g_localizeStrings.Get(53241);
            line += "[CR][CR]";
            line += ListenToUpdateScript.m_updateScriptMessage;
        
            CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Going to open CGUIDialogOK2 with [heading=%s][line=%s] (update)(test)",heading.c_str(),line.c_str());
        
            CGUIDialogOK2::ShowAndGetInput(heading,line);
          }

          CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - In boxee process. Going to close the pipes and going to change the VersionUpdateJob status to [VUJS_READY_FOR_UPDATE] (update)");

          // close the pipes
          ::CloseHandle(pi.hThread);
          ::CloseHandle(pi.hProcess);
          ::CloseHandle(hInputWrite);
          ::CloseHandle(hOutputRead);

          m_boxeeVerUpdateJob.SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);
            
          retVal = false;
        }
        else
        {
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::PerformWinVersionUpdate - Call CreateProcess() failed. [cmd=%s] (update)",cmd);
          m_boxeeVerUpdateJob.SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);
        }
      }
    }
  }

  return retVal;
}
#endif

void CBoxeeVersionUpdateManager::ListenToUpdateScriptJob::Run()
{
  CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - Enter function (update)");

  int counter = 0;
  char buf[BUFFER_SIZE];
  memset(buf,0,BUFFER_SIZE);

#if defined(_LINUX)
  while(read(m_readPipe,buf,READ_TIMEOUT) > 0)
  {    
    CStdString str = buf;
    memset(buf,0,BUFFER_SIZE);
    counter++;

    if(AnalyzeUpdateScriptPrints(str))
    {
      // Script print was analyzed OK
    }
    else
    {
      // There was an error in the update script -> Return to Boxee
      
      CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - There was an error in the update script -> Return to Boxee (RunInBG)(update)");
      break;
    }
  }
  
#elif defined(_WIN32)
  
  CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - Going to enter while loop (loop)");
  int i=0;
  while(true)
  {
    i++;
    CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - [i=%d] Before WaitForSingleObject if (loop)",i);
    
    if(::WaitForSingleObject(m_readPipe,READ_TIMEOUT) == WAIT_OBJECT_0)
    {
      CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - [i=%d] Pass WaitForSingleObject if (loop)",i);

      memset(buf,0,BUFFER_SIZE);
      DWORD nOut=0;
      char *ptr = buf;
      
      int numOfBytesRead=0;
      do 
      {
        ::ReadFile(m_readPipe, ptr, 1, &nOut, NULL);
        numOfBytesRead++;
        ptr++;
      } while ((*(ptr-1) != '\n') && (numOfBytesRead < BUFFER_SIZE));

      counter++;

      CStdString str = buf;
      
      CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - [%d] After read line [str=%s] (loop)",counter,str.c_str());

      if(AnalyzeUpdateScriptPrints(str))
      {
        // Script print was analyzed OK
      }
      else
      {
        // There was an error in the update script -> Return to Boxee
        
        CLog::Log(LOGDEBUG,"ListenToUpdateScriptJob::Run - There was an error in the update script -> Return to Boxee (RunInBG)(update)");
        break;
      }
    }
  }
#endif

  /////////////////////////////////////////////////////////
  // If we reached here there was an error in the update //
  /////////////////////////////////////////////////////////

  /////////////////////////////////////////
  // Sleep so the user can see the error //
  /////////////////////////////////////////
  
  Sleep(2000);
  
  CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);
  if(dialog)
  {
    dialog->Close();
  }
}

bool CBoxeeVersionUpdateManager::ListenToUpdateScriptJob::AnalyzeUpdateScriptPrints(const CStdString& str)
{
  bool retVal = false;

  if(str.Find("SCRIPT") != (-1))
  {
    /////////////////////////////////////
    // Handle only outputs from script //
    /////////////////////////////////////
    
    if(str.Find("LOG:") != (-1))
    {
      m_updateScriptMessage = g_localizeStrings.Get(53205);

      CStdString label = "";
      CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);

      if(str.Find("CHECK_ROOT_PASSWORD") != (-1))
      {
        m_updateScriptMessage = g_localizeStrings.Get(53210);
      }
      else if(str.Find("ROOT_PASSWORD_VALID") != (-1))
      {
        m_updateScriptMessage = g_localizeStrings.Get(53211);
      }
      else if(str.Find("SHUTDOWN_BOXEE") != (-1))
      {
        m_updateScriptMessage = g_localizeStrings.Get(53213);
      }
      
      if(dialog && (!m_updateScriptMessage.IsEmpty()))
      {
        dialog->SetLabel(m_updateScriptMessage);
      }

	    retVal = true;
    }
    else if(str.Find("ACTION:") != (-1))
    {
      m_updateScriptMessage = g_localizeStrings.Get(53206);

      if(str.Find("SHUTDOWN_BOXEE") != (-1))
      {
        m_updateScriptMessage = g_localizeStrings.Get(53213);
        
        CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);
        if(dialog)
        {
          dialog->SetLabel(m_updateScriptMessage);
        }

        g_application.Stop();
        exit(0);
	    }
	
	    retVal = true;
    }
    else if(str.Find("ERROR:") != (-1))
    {
      m_updateScriptMessage = g_localizeStrings.Get(53242);
      m_updateScriptMessage += " ";

      CGUIDialogBoxeeUpdateProgress* dialog = (CGUIDialogBoxeeUpdateProgress*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_PROGRESS);

      if(str.Find("ROOT_PASSWORD_INVALID") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53212);       
      }
      else if(str.Find("ALREADY_ATTACHED") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53214);        
      }
      else if(str.Find("NO_INSTALLATION_FILE") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53215);        
      }
      else if(str.Find("ATTACHED_FAILED") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53216);        
      }
      else if(str.Find("FIND_ATTACH_FAILED") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53217);        
      }
      else if(str.Find("FIND_ATTACH_MPKG_FAILED") != (-1))
      {
        m_updateScriptMessage += g_localizeStrings.Get(53218);        
      }
      else
      {
        m_updateScriptMessage = g_localizeStrings.Get(53207);        
      }
     
      if(dialog && (!m_updateScriptMessage.IsEmpty()))
      {
        dialog->SetLabel(m_updateScriptMessage);
      }
    }  
  }
  else
  {
    ////////////////////////////////////////
    // Not a script output - Not handling //
    ////////////////////////////////////////

    retVal = true;
  }
  
  return retVal;
}

CBoxeeVersionUpdateJob& CBoxeeVersionUpdateManager::GetBoxeeVerUpdateJob()
{
  return m_boxeeVerUpdateJob;
}

#ifdef _LINUX

void CBoxeeVersionUpdateManager::SetUserPassword(CStdString userPassword)
{
  m_userPassword = userPassword;
}

const CStdString& CBoxeeVersionUpdateManager::GetUserPassword()
{
  return m_userPassword;
}

#endif

bool CBoxeeVersionUpdateManager::HandleUpdateVersionButton(bool inLoginScreen)
{
  bool retVal = false;

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Enter function. Going to call g_boxeeVersionUpdateManager.PerformVersionUpdate() (update)");
  
  CGUIDialogBoxeeUpdateMessage* dialog = (CGUIDialogBoxeeUpdateMessage*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_UPDATE_MESSAGE);
  
  if (dialog == false)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Failed to get CGUIDialogBoxeeUpdateMessage object (update)");
  }
  else
  {
    ///////////////////////////////////////////////////////
    // In case of inLoginScreen - Set show cancel button //
    ///////////////////////////////////////////////////////

    dialog->InLoginScreen(inLoginScreen);

    dialog->DoModal();

    if(dialog->IsConfirmed())
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Call to CGUIDialogBoxeeUpdateMessage returned with [Confirmed=TRUE] (update)");

#ifdef HAS_EMBEDDED

      IHalServices& client = CHalServicesFactory::GetInstance();

      client.RequestUpgrade();

      CGUIDialogProgress* dialog = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
      dialog->DoModal();

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Waiting for machine reboot (update)");
#else      
      CStdString scriptToRunName = (((g_boxeeVersionUpdateManager.GetBoxeeVerUpdateJob())).GetVersionUpdateInfo()).m_scriptToRunName;
      
      if(scriptToRunName.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - scriptToRunName is empty so NOT going to perform update (update)");
        retVal = true;
      }
      else
      {        
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - scriptToRunName is [%s]. Going to call g_boxeeVersionUpdateManager.PerformVersionUpdate() (update)",scriptToRunName.c_str());

        bool updateVerionSucceedded = g_boxeeVersionUpdateManager.PerformVersionUpdate();
        
        if(updateVerionSucceedded)
        {
          CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Call to g_boxeeVersionUpdateManager.PerformVersionUpdate() succeedded (update)");
        }
        else
        {
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - Call to g_boxeeVersionUpdateManager.PerformVersionUpdate() failed (update)");
        }
        
        retVal = updateVerionSucceedded;        
      }  
#endif
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleUpdateVersionButton - CGUIDialogBoxeeUpdateMessage was closed (update)");
      retVal = true;      
    }    
  }
  
  return retVal;
}

bool CBoxeeVersionUpdateManager::HandleVersionUpdate(const TiXmlElement* root, const TiXmlElement* updateChildElem, bool startOnDemand)
{
  bool retVal = false;

  if(strcmp(updateChildElem->Value(),"version_update") != 0)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::HandleVersionUpdate - <version_update> parameter is not found in child element [%s]",updateChildElem->Value());
    return false;   
  }

  // Get the update type
  CStdString versionUpdateBuildNum = updateChildElem->Attribute("build-num");
  CStdString versionUpdateFilePath = updateChildElem->Attribute("update-descriptor-path");
  CStdString versionUpdateFileHash = updateChildElem->Attribute("update-descriptor-hash");

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleVersionUpdate - <version_update> parameter has the following attributes [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

  if (!versionUpdateFilePath.IsEmpty() && !versionUpdateBuildNum.IsEmpty() && !versionUpdateFileHash.IsEmpty())
  {
    if(startOnDemand == false)
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleVersionUpdate - Going to call CBoxeeVersionUpdateManager::PrepareVersionUpdate() with [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

      retVal = PrepareVersionUpdate(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash);

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleVersionUpdate - Call to CBoxeeVersionUpdateManager::PrepareVersionUpdate() with [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] returned [%d] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str(),retVal);

    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::HandleVersionUpdate - Got on demand update, saving the attributes and waiting for start update command\n");

      m_savedOnDemandInfo.m_versionUpdateBuildNum = versionUpdateBuildNum;
      m_savedOnDemandInfo.m_versionUpdateFilePath = versionUpdateFilePath;
      m_savedOnDemandInfo.m_versionUpdateFileHash = versionUpdateFileHash;

      retVal = true;
    }

  }
  else
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::HandleVersionUpdate - One of the attributes of <version_update> is empty. [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());
  }

  return retVal;
}

#ifdef HAS_EMBEDDED
bool CBoxeeVersionUpdateManager::StartUpdate()
{
  if(m_savedOnDemandInfo.m_versionUpdateBuildNum.IsEmpty() ||  
     m_savedOnDemandInfo.m_versionUpdateFilePath.IsEmpty() || 
     m_savedOnDemandInfo.m_versionUpdateFileHash.IsEmpty())
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::StartUpdate - One of the attributes of <version_update> is empty. [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",m_savedOnDemandInfo.m_versionUpdateBuildNum.c_str(),m_savedOnDemandInfo.m_versionUpdateFilePath.c_str(),m_savedOnDemandInfo.m_versionUpdateFileHash.c_str());
    return false;
  }

  return PrepareVersionUpdate(m_savedOnDemandInfo.m_versionUpdateBuildNum,m_savedOnDemandInfo.m_versionUpdateFilePath,m_savedOnDemandInfo.m_versionUpdateFileHash);
}

bool CBoxeeVersionUpdateManager::GetDownloadInfo(CDownloadInfo& downloadInfo)
{
  return GetBoxeeVerUpdateJob().GetDownloadInfo(downloadInfo);
}

int CBoxeeVersionUpdateManager::InitCheckForUpdateRequest(CStdString& chkupdUrl,CStdString& strPingVersion)
{
#define BXINFO_FILE_PATH "special://home/bxinfo.xml"
  BOXEE::BXXMLDocument bxinfo;
  CStdString bxinfoFilePath = PTH_IC(BXINFO_FILE_PATH);
  
  //////////////////////////////////
  // Set the current ping version //
  //////////////////////////////////

  if (bxinfo.LoadFromFile(bxinfoFilePath))
  {
    TiXmlHandle handle(bxinfo.GetRoot());
    TiXmlElement* version = handle.FirstChild("ping_version").ToElement();
    
    if (version)
    {
      std::string strVersion = version->GetText();
      if (!strVersion.empty())
      {
        strPingVersion = strVersion;
      }
    }
  }
  
  if(strPingVersion.IsEmpty())
  {
    strPingVersion = "0";
    CLog::Log(LOGDEBUG,"CPingJob::InitPingRequest - Failed to get the PingVersion from [%s], so set the ping version to [%s] (ping)",bxinfoFilePath.c_str(),strPingVersion.c_str());
  }

  //////////////////////
  // Set the ping url //
  //////////////////////
  
  chkupdUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("WatchDog.TestServerUrl","http://app.boxee.tv/");

  chkupdUrl += "chkupd/";
  
  chkupdUrl += BoxeeUtils::GetPlatformStr();
  chkupdUrl += "/";
  
  CStdString boxeeCurrentVersion = g_infoManager.GetVersion();
  
  // For debug //
  /*
  if(boxeeCurrentVersion == "SVN")
  {
    boxeeCurrentVersion = g_localizeStrings.Get(53250);

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::PerformVersionUpdate - boxeeCurrentVersion is [SVN], so it was changed to [%s] (ping)",boxeeCurrentVersion.c_str());
  }
  */
  ///////////////
  
  chkupdUrl += boxeeCurrentVersion;
  
  chkupdUrl += "/";
  chkupdUrl += strPingVersion;
  

  CHalHardwareInfo hwInfo;
  CHalSoftwareInfo swInfo;
  IHalServices& client = CHalServicesFactory::GetInstance();

  if(client.GetHardwareInfo(hwInfo) && client.GetSoftwareInfo(swInfo))
  {
    chkupdUrl += "/";
    chkupdUrl += hwInfo.revision;

    chkupdUrl += "/";
    chkupdUrl += swInfo.regionSKU;

    chkupdUrl += "/";
    chkupdUrl += hwInfo.serialNumber;
  }
  else
  {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::InitCheckForUpdateRequest - Failed to retrieve device info\n");
  }

  if (g_guiSettings.GetBool("update.allow_beta"))
  {
    chkupdUrl += "/1";
  }
  else
  {
    chkupdUrl += "/0";
  }

  return true;
}

CStdString CBoxeeVersionUpdateManager::GetLastCheckedTime()
{
  return g_guiSettings.GetString("update.status");
}

int CBoxeeVersionUpdateManager::CheckForUpdate(bool& hasNewUpdate, CStdString& versionUpdateBuildNum)
{
  int retval = 0;

  if(g_application.IsConnectedToInternet() == false)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::CheckForUpdate - FAILED to check for update - no network connection (update)");
    return ENETDOWN;
  }

  hasNewUpdate = false;
  
  CStdString strClientPingVersion = "";
  CStdString chkupdUrl = "";
  BOXEE::BXCurl curl;

  InitCheckForUpdateRequest(chkupdUrl,strClientPingVersion);

  std::string strResp = curl.HttpGetString(chkupdUrl, false);

  CLog::Log(LOGWARNING,"CBoxeeVersionUpdateManager::CheckForUpdate - Check update for [chkupdUrl=%s] returned [Response-IsEmpty=%d] (update)",chkupdUrl.c_str(),strResp.empty());
  
  CDateTime time=CDateTime::GetCurrentDateTime();
  CStdString strLastUpdateTime = time.GetAsLocalizedDateTime(false, false);

  g_guiSettings.SetString("update.status",strLastUpdateTime);

  BOXEE::BXXMLDocument reader;
  if (strResp.empty())
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::CheckForUpdate - Not handling server response to [chkupdUrl=%s] because it is empty (update)",chkupdUrl.c_str());
    return 0;
  }
  if(!reader.LoadFromString(strResp))
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::CheckForUpdate - Not handling server response to [chkupdUrl=%s] because failed to load it to BXXMLDocument (update)",chkupdUrl.c_str());
    return EINVAL;
  }

  TiXmlElement* root = reader.GetRoot();

  if(!root)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::CheckForUpdate - Failed to get root from BXXMLDocument of the ping response (update)");
    return EINVAL;
  }

  if((strcmp(root->Value(),"ping") != 0))
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateManager::CheckForUpdate - Failed to parse ping response because the root tag ISN'T <ping> (update)");
    return EINVAL;
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::CheckForUpdate - The root tag <ping> was found. Going to parse the ping response (update)");

  TiXmlElement* pingChildElem = NULL;
  pingChildElem = root->FirstChildElement();

  while (pingChildElem)
  {
    if (strcmp(pingChildElem->Value(),"version_update") == 0)
    {

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateManager::CheckForUpdate - The <version_update> tag was found (update)");

      bool retval = HandleVersionUpdate(root, pingChildElem, true);
      
      if(retval == true)
      {
        hasNewUpdate = true;
        versionUpdateBuildNum = m_savedOnDemandInfo.m_versionUpdateBuildNum;
        retval = 0;
        break; 
      }

    }

    pingChildElem = pingChildElem->NextSiblingElement();
  }

  return retval;
}
#endif

//////////////////////////////////////
// CBoxeeVersionUpdateJob functions //
//////////////////////////////////////

CBoxeeVersionUpdateJob::CBoxeeVersionUpdateJob()
{
  m_VersionUpdateJobStatus = VUJS_IDLE;
  m_versionUpdateFilePath = "";
  m_versionUpdateBuildNum = "";
  m_versionUpdateInfo.reset();

#ifdef HAS_EMBEDDED
  m_TotalBytesToDownload = 0;
  m_TotalBytesDownloaded = 0;
  memset(&m_DownloadInfo, sizeof m_DownloadInfo, 0);
#endif
}

bool CBoxeeVersionUpdateJob::Init(const CStdString& versionUpdateBuildNum,const CStdString& updateFilePath,const CStdString& versionUpdateFileHash,const CStdString& directoryForUpdateLocalPath)
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Init - Enter function with [directoryForUpdateLocalPath=%s] (update)",directoryForUpdateLocalPath.c_str());

  m_versionUpdateBuildNum = versionUpdateBuildNum;
  m_versionUpdateFileHash = versionUpdateFileHash;
  
  if(directoryForUpdateLocalPath.IsEmpty())
  {
    m_versionUpdateFilePath = updateFilePath;    
  }
  else
  {
    CStdString strFileName = CUtil::GetFileName(updateFilePath);
    
    m_versionUpdateFilePath = directoryForUpdateLocalPath;
    m_versionUpdateFilePath += "/";
    m_versionUpdateFilePath += strFileName;
  }
  
  m_versionUpdateInfo.reset();

  SetVersionUpdateJobStatus(VUJS_INITIALIZED);

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Init - Exit function after setting [m_versionUpdateBuildNum=%s][m_versionUpdateFilePath=%s][m_versionUpdateFileHash=%s][VersionUpdateJobStatus=%d=%s] (update)",m_versionUpdateBuildNum.c_str(),m_versionUpdateFilePath.c_str(),m_versionUpdateFileHash.c_str(),m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

  return true;
}

void CBoxeeVersionUpdateJob::reset()
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::reset - Enter function. [m_VersionUpdateJobStatus=%d=%s] (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

  m_versionUpdateFilePath = "";
  m_versionUpdateBuildNum = "";
  
  SetVersionUpdateJobStatus(VUJS_IDLE);

  m_versionUpdateInfo.reset();
#ifdef HAS_EMBEDDED
  m_TotalBytesToDownload = 0;
  m_TotalBytesDownloaded = 0;
  memset(&m_DownloadInfo, sizeof m_DownloadInfo, 0);
#endif
}

void CBoxeeVersionUpdateJob::SetVersionUpdateJobStatus(VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus)
{
  CSingleLock lock(m_versionUpdateJobStatusLock);
  
  m_VersionUpdateJobStatus = versionUpdateJobStatus;
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::SetVersionUpdateJobStatus - VersionUpdateJobStatus status was changed to [%d=%s] (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());
}

VERSION_UPDATE_JOB_STATUS CBoxeeVersionUpdateJob::GetVersionUpdateJobStatus()
{
  VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus;
  
  EnterCriticalSection(m_versionUpdateJobStatusLock);
  
  versionUpdateJobStatus = m_VersionUpdateJobStatus;
  
  LeaveCriticalSection(m_versionUpdateJobStatusLock);

  return versionUpdateJobStatus;
}

bool CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate(const CStdString& newVersionForUpdate)
{
  CSingleLock lock(m_versionUpdateJobStatusLock);
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Enter function with [newVersionForUpdate=%s]. VersionUpdateJob current status is [%d=%s] (update)",newVersionForUpdate.c_str(),m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

  ////////////////////////////////////////////////////
  // Need to check if the can update to new version //
  ////////////////////////////////////////////////////

  if(m_VersionUpdateJobStatus == VUJS_IDLE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Enter [m_VersionUpdateJobStatus=VUJS_IDLE] block (update)");

    // No update version was initialize yet -> Check only if the new version is newer then the current version

    CStdString boxeeNewVersion = newVersionForUpdate;
    CStdString boxeeCurrentVersion = g_infoManager.GetVersion();
      
    // For debug //
    /*
    if(boxeeCurrentVersion == "SVN")
    {
      boxeeCurrentVersion = g_localizeStrings.Get(53250);

      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - boxeeCurrentVersion is [SVN], so it was changed to [%s] (update)",boxeeCurrentVersion.c_str());
    }
    */
    //////////

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Going to call IsThisNewVersion() for [boxeeCurrentVersion=%s][newVersionForUpdate=%s] (update)",boxeeCurrentVersion.c_str(),boxeeNewVersion.c_str());

    if(IsThisNewVersion(boxeeCurrentVersion,boxeeNewVersion))
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Call IsThisNewVersion() for [boxeeCurrentVersion=%s][newVersionForUpdate=%s] returned TRUE. Going to change VersionUpdateJob status to [VUJS_READY_FOR_INITIALIZE] (update)",boxeeCurrentVersion.c_str(),boxeeNewVersion.c_str());

      SetVersionUpdateJobStatus(VUJS_READY_FOR_INITIALIZE);

      return true;      
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Call to IsThisNewVersion() for [boxeeCurrentVersion=%s][newVersionForUpdate=%s] returned FALSE (update)",boxeeCurrentVersion.c_str(),boxeeNewVersion.c_str());
      
      return false;
    }
  }
  else if(m_VersionUpdateJobStatus == VUJS_READY_FOR_UPDATE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Enter [m_VersionUpdateJobStatus=VUJS_READY_FOR_UPDATE] block (update)");

    // VersionUpdateJob object is already initialized -> Check if the new version is newer then the VersionUpdateJob initialized version

    CStdString boxeeNewVersion = newVersionForUpdate;
    CStdString VersionUpdateJobInitializedVersion = m_versionUpdateBuildNum;

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Going to call IsThisNewVersion() for [VersionUpdateJobInitializedVersion=%s][newVersionForUpdate=%s] (update)",VersionUpdateJobInitializedVersion.c_str(),boxeeNewVersion.c_str());

    if(IsThisNewVersion(VersionUpdateJobInitializedVersion,boxeeNewVersion))
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Call IsThisNewVersion() for [VersionUpdateJobInitializedVersion=%s][newVersionForUpdate=%s] returned TRUE. Going to remove the current UpdateVersion directory and change VersionUpdateJob status to [VUJS_READY_FOR_INITIALIZE] (update)",VersionUpdateJobInitializedVersion.c_str(),boxeeNewVersion.c_str());

      CStdString currentUpdateVersionDirectory = UPDATE_DIRECTORY_BASE_PATH;
      currentUpdateVersionDirectory += m_versionUpdateBuildNum;

      CUtil::WipeDir(currentUpdateVersionDirectory);

      SetVersionUpdateJobStatus(VUJS_READY_FOR_INITIALIZE);

      return true;
    }
    else
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - Call to IsThisNewVersion() for [VersionUpdateJobInitializedVersion=%s][newVersionForUpdate=%s] returned FALSE. Going to return FALSE. [m_versionUpdateBuildNum=%s][m_versionUpdateFilePath=%s][m_versionUpdateFileHash=%s][VersionUpdateJobStatus=%d=%s] (update)",VersionUpdateJobInitializedVersion.c_str(),boxeeNewVersion.c_str(),m_versionUpdateBuildNum.c_str(),m_versionUpdateFilePath.c_str(),m_versionUpdateFileHash.c_str(),m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());
      
      return false;
    }
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForInitializeUpdate - VersionUpdateJob current status is [m_VersionUpdateJobStatus=%d=%s] so update can't be done (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

    return false;
  }
}

bool CBoxeeVersionUpdateJob::acquireVersionUpdateJobForPerformUpdate()
{
  CSingleLock lock(m_versionUpdateJobStatusLock);

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForPerformUpdate - Enter function. [m_VersionUpdateJobStatus=%d=%s] (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

  if(m_VersionUpdateJobStatus == VUJS_READY_FOR_UPDATE)
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForPerformUpdate - VersionUpdateJob current status is [m_VersionUpdateJobStatus=%d=%s]. Going to change it to [VUJS_ACQUIRE_FOR_UPDATE] (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

    SetVersionUpdateJobStatus(VUJS_ACQUIRE_FOR_UPDATE);

    return true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::acquireVersionUpdateJobForPerformUpdate - VersionUpdateJob current status is [m_VersionUpdateJobStatus=%d=%s], so it can't be acquire for update (update)",m_VersionUpdateJobStatus,(CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(m_VersionUpdateJobStatus)).c_str());

    return false;
  }
}

const CStdString CBoxeeVersionUpdateJob::GetVersionUpdateJobStatusAsString(VERSION_UPDATE_JOB_STATUS versionUpdateJobStatus)
{
  switch(versionUpdateJobStatus)
  {
  case VUJS_IDLE:
    return "VUJS_IDLE";
    break;
  case VUJS_READY_FOR_INITIALIZE:
    return "VUJS_READY_FOR_INITIALIZE";
    break;
  case VUJS_INITIALIZED:
    return "VUJS_INITIALIZED";
    break;
  case VUJS_WORKING:
    return "VUJS_WORKING";
    break;
  case VUJS_READY_FOR_UPDATE:
    return "VUJS_READY_FOR_UPDATE";
    break;
  case VUJS_ACQUIRE_FOR_UPDATE:
    return "VUJS_ACQUIRE_FOR_UPDATE";
    break;
  case VUJS_UPDATING:
    return "VUJS_UPDATING";
    break;
  default:
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::GetVersionUpdateJobAsString - Failed to convert [versionUpdateJobStatus=%d] to ENUM. Going to return [VUJS_ERROR] (update)",versionUpdateJobStatus);
    return "VUJS_ERROR";
    break;
  }
}

bool CBoxeeVersionUpdateJob::IsThisNewVersion(CStdString currentVersion,CStdString newVersion)
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::IsThisNewVersion - Enter function with [currentVersion=%s][newVersion=%s] (update)",currentVersion.c_str(),newVersion.c_str());
  
  bool isThisNewVersion = false;
  
  if (CUtil::VersionCompare(newVersion, currentVersion) > 0)
  {
    isThisNewVersion = true;
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::IsThisNewVersion - For [currentVersion=%s][newVersion=%s] going to return [isThisNewVersion=%d] (update)",currentVersion.c_str(),newVersion.c_str(),isThisNewVersion);
  
  return isThisNewVersion;
}

#ifdef HAS_EMBEDDED
void CBoxeeVersionUpdateJob::SetVersionUpdateDownloadStatus(VERSION_UPDATE_DOWNLOAD_STATUS versionUpdateDownloadStatus)
{
  CSingleLock lock(m_downloadInfoLock);
  
  m_DownloadInfo.m_Status = versionUpdateDownloadStatus;
}

VERSION_UPDATE_DOWNLOAD_STATUS CBoxeeVersionUpdateJob::GetVersionUpdateDownloadStatus()
{
  CSingleLock lock(m_downloadInfoLock);

  return m_DownloadInfo.m_Status;
}

bool CBoxeeVersionUpdateJob::GetDownloadInfo(CDownloadInfo& downloadInfo)
{
  CSingleLock lock(m_downloadInfoLock);
  
  downloadInfo = m_DownloadInfo;

  return true;
}

bool CBoxeeVersionUpdateJob::SafeDownloadWithProgress(const CStdString& url, const CStdString& target, const CStdString& hash,bool isLast)
{
  CFileCurl http;
  CFile targetFile;
  CStdString targetPath = target;
  char* buf = NULL;
  int bufSize = m_TotalBytesToDownload/100;
  bool bOk = false;

  if(CUtil::IsSpecial(targetPath))
  {
    targetPath = _P(targetPath);
  }

  buf = new char[bufSize];
  
  if(buf == NULL)
  {
     CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Unable to allocate memory [%d] bytes (update)", bufSize);
     return false;
  }

  do
  {
    int bytesRead = 0;
    
    bOk = http.Open(url);
    if(!bOk)
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Unable to open url [%s] (update)", url.c_str());
      break;
    }

    bOk = targetFile.OpenForWrite(targetPath, true);
    if(!bOk)
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Unable to open target file [%s] (update)", targetPath.c_str());
      break;
    }

    double prevPrintDownloadProgress = 0.0;
    while((bytesRead = http.Read(buf, bufSize)) != 0)
    {
      int bytesWritten = targetFile.Write(buf, bytesRead);
      double transferRateBytesPerMS = 0.0;

      if((bytesWritten < 0) || (bytesRead != bytesWritten))
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Error writting file [%s] (update)", target.c_str());
        bOk = false;
        break;
      }

      m_TotalBytesDownloaded += bytesWritten;

      transferRateBytesPerMS = (double)m_TotalBytesDownloaded/(double)((CTimeUtils::GetTimeMS() - m_DownloadStartTime));

      EnterCriticalSection(m_downloadInfoLock);

      if(m_TotalBytesToDownload)
      {
        m_DownloadInfo.m_CurrentDownloadProgress = (double)((double)m_TotalBytesDownloaded/(double)m_TotalBytesToDownload)*100.0;
      }

      if(transferRateBytesPerMS >= 1.0)
      {      
        m_DownloadInfo.m_EstimatedTimeLeftMS = (unsigned int)(m_TotalBytesToDownload - m_TotalBytesDownloaded)/(int)transferRateBytesPerMS;
      }

      if (m_DownloadInfo.m_Status != VUDS_DOWNLOADING)
      {
        SetVersionUpdateDownloadStatus(VUDS_DOWNLOADING);
      }

      LeaveCriticalSection(m_downloadInfoLock);

      if (m_DownloadInfo.m_CurrentDownloadProgress - prevPrintDownloadProgress >= 1.0)
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Downloading file [%s], total progress [%.2f%%], time left [%dms] (update)", url.c_str(), m_DownloadInfo.m_CurrentDownloadProgress, m_DownloadInfo.m_EstimatedTimeLeftMS);
        prevPrintDownloadProgress = m_DownloadInfo.m_CurrentDownloadProgress;
      }
    }

    if (isLast)
    {
      SetVersionUpdateDownloadStatus(VUDS_POST_DOWNLOADING);
    }

    targetFile.Close();
    http.Close();

    if(bOk)
    {
      if(hash != CUtil::MD5File(targetPath))
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::SafeDownloadWithProgress - Wrong MD5 file [%s] (update)", targetPath.c_str());
        ::DeleteFile(targetPath);
        bOk = false;
      }
    }

  } while(false);

  if(buf)
  {
    free(buf);
  }

  return bOk;  
}
#endif

void CBoxeeVersionUpdateJob::Run()
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Enter function [m_versionUpdateBuildNum=%s][m_versionUpdateFilePath=%s][m_VersionUpdateJobStatus=%d] (update)",m_versionUpdateBuildNum.c_str(),m_versionUpdateFilePath.c_str(),m_VersionUpdateJobStatus);
  
  SetVersionUpdateJobStatus(VUJS_WORKING);
  
#ifdef HAS_EMBEDDED
  m_DownloadStartTime = CTimeUtils::GetTimeMS();
  SetVersionUpdateDownloadStatus(VUDS_PRE_DOWNLOADING);
#endif

  ///////////////////////////////////////////////////
  // Clean the download dir first                  //
  // this is a must in occasions where an optional //
  // version was downloaded before and there's a   //
  // new version which will not have space on the  //
  // NAND.                                         //
  ///////////////////////////////////////////////////

  CUtil::WipeDir(UPDATE_DIRECTORY_BASE_PATH);
  CUtil::CreateDirectoryEx(UPDATE_DIRECTORY_BASE_PATH);

  /////////////////////////////////////////////////
  // Create a tmp directory for the update files //
  /////////////////////////////////////////////////
  
  CStdString tmpDirectoryForUpdate = UPDATE_DIRECTORY_BASE_PATH;
  tmpDirectoryForUpdate += m_versionUpdateBuildNum;
  tmpDirectoryForUpdate += "_tmp";
  bool tmpDirWasCreated = false;
  tmpDirWasCreated = CUtil::CreateDirectoryEx(tmpDirectoryForUpdate);
/*
#if defined(_LINUX)
  tmpDirWasCreated = CreateDirectory(tmpDirectoryForUpdate.c_str(),NULL);
#else
  fprintf(stderr,"In CBoxeeVersionUpdateJob::Run - Going to call CDirectory::Create with [%s]\n",tmpDirectoryForUpdate.c_str());
  tmpDirWasCreated = CUtil::CreateDirectoryEx(tmpDirectoryForUpdate);
  fprintf(stderr,"In CBoxeeVersionUpdateJob::Run - Going to call CDirectory::Create with [%s] returned [%d]\n",tmpDirectoryForUpdate.c_str(),tmpDirWasCreated);
#endif
*/

  if(tmpDirWasCreated == false)
  {
#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Call to CreateDirectory with [path=%s] failed. Going to exit (update)",tmpDirectoryForUpdate.c_str());
    reset();
    return;
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Tmp directory [path=%s] for the update files was created (update)",tmpDirectoryForUpdate.c_str());

  ////////////////////////////////////////
  // Download the m_updateFilePath file //
  ////////////////////////////////////////

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Try to download [m_versionUpdateFilePath=%s] (update)",m_versionUpdateFilePath.c_str());

  CStdString strName = CUtil::GetFileName(m_versionUpdateFilePath);
  CStdString updateFileLocalPath = tmpDirectoryForUpdate;
  updateFileLocalPath += "/";
  updateFileLocalPath += strName;
  
  bool succeedDownload = BoxeeUtils::SafeDownload(m_versionUpdateFilePath, updateFileLocalPath,m_versionUpdateFileHash);

  if(!succeedDownload)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to download file [path=%s] to [updateFileLocalPath=%s]. Going to remove the tmp directory [%s] and exit (update)",m_versionUpdateFilePath.c_str(),updateFileLocalPath.c_str(),tmpDirectoryForUpdate.c_str());

#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

    CUtil::WipeDir(tmpDirectoryForUpdate);
    reset();
    return;
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - [m_versionUpdateFilePath=%s] was downloaded successfully to [updateFileLocalPath=%s] (update)",m_versionUpdateFilePath.c_str(),updateFileLocalPath.c_str());
  
  CStdString orgPath = m_versionUpdateFilePath;
  m_versionUpdateFilePath = updateFileLocalPath;

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - m_versionUpdateFilePath was updated from [%s] to [%s]. Going to parse it (update)",orgPath.c_str(),m_versionUpdateFilePath.c_str());

  /////////////////////////////////////
  // Parse the m_updateFilePath file //
  /////////////////////////////////////

  bool parseSucceeded = false;
  parseSucceeded = ParseUpdateFile(updateFileLocalPath);
  
  if(parseSucceeded == false)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to parse [updateFileLocalPath=%s]. Going to remove the tmp directory [%s] and exit (update)",updateFileLocalPath.c_str(),tmpDirectoryForUpdate.c_str());
    
#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

    CUtil::WipeDir(tmpDirectoryForUpdate);
    reset();
    return;    
  }

  ///////////////////////////////////////////////////////////////////////////////
  // Download the files that was under the <download> in m_updateFilePath file //
  ///////////////////////////////////////////////////////////////////////////////

  int numOfFilesToDownload = (m_versionUpdateInfo.m_UpdateFilesToDownload).size();
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - [updateFileLocalPath=%s] was parsed successfully. Going to download [%d] files that was under <download> element (update)",updateFileLocalPath.c_str(),numOfFilesToDownload);

  if(numOfFilesToDownload > 0)
  {
    bool releaseNotesFileNameWasFound = false;
#ifndef HAS_EMBEDDED
    bool scriptToRunPermissionsWasChanged = false;
#endif

    for(int i=0;i<numOfFilesToDownload;i++)
    {
      CUpdateFilesInfo& ufi = (m_versionUpdateInfo.m_UpdateFilesToDownload)[i];

      CStdString strName = CUtil::GetFileName(ufi.m_filePath);
      
      CStdString downloadFileLocalPath = tmpDirectoryForUpdate;
      downloadFileLocalPath += "/";
      downloadFileLocalPath += strName;
     
      if(!XFILE::CFile::Exists(downloadFileLocalPath))
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - [%d/%d] Going to call SafeDownload for file [path=%s][hash=%s] and place it at [target=%s] (update)",i+1,numOfFilesToDownload,(ufi.m_filePath).c_str(),(ufi.m_fileHash).c_str(),downloadFileLocalPath.c_str());

#ifdef HAS_EMBEDDED
        bool isLast;
        (i == numOfFilesToDownload-1) ? isLast = true : isLast = false;
        bool succeed = SafeDownloadWithProgress(ufi.m_filePath, downloadFileLocalPath, ufi.m_fileHash, isLast);
#else
        bool succeed = BoxeeUtils::SafeDownload((ufi.m_filePath).c_str(), downloadFileLocalPath.c_str(),ufi.m_fileHash);
#endif
        if(!succeed)
        {
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - [%d/%d] Failed to download file [path=%s] to [target=%s]. Going to remove the tmp directory [%s] and exit (update)",i+1,numOfFilesToDownload,(ufi.m_filePath).c_str(),downloadFileLocalPath.c_str(),tmpDirectoryForUpdate.c_str());
         
#ifdef HAS_EMBEDDED
          SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif 
          CUtil::WipeDir(tmpDirectoryForUpdate);
          reset();
          return;
        }
        else
        {
          CStdString orgPath = ufi.m_filePath;
          ufi.m_filePath = downloadFileLocalPath;
          CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - [%d/%d] Succeeded to download file [path=%s] to [target=%s]. In CUpdateFilesInfo the file path was changed from [%s] to [%s] (update)",i+1,numOfFilesToDownload,orgPath.c_str(),downloadFileLocalPath.c_str(),orgPath.c_str(),(ufi.m_filePath).c_str());
        }
      }
      else
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - [%d/%d] No need to call SafeDownload for file [path=%s][hash=%s] and place it at [target=%s] because it is already exist (update)",i+1,numOfFilesToDownload,(ufi.m_filePath).c_str(),(ufi.m_fileHash).c_str(),downloadFileLocalPath.c_str());
      }
      
      if(strName == m_versionUpdateInfo.m_UpdateNotesFileName)
      {
        releaseNotesFileNameWasFound = true;      
      }

#ifndef HAS_EMBEDDED      
      if(strName == m_versionUpdateInfo.m_scriptToRunName)
      {
        //////////////////////////////////////////////////////////
        // Try to change the ScriptToRunName script permissions //
        //////////////////////////////////////////////////////////
        
  #if defined(_LINUX)
        if(chmod(downloadFileLocalPath,0777) != 0)
  #elif defined(_WIN32)
        if(_chmod(downloadFileLocalPath,0777) != 0)
  #endif
        {
#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

          CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to change the installtion script [path=%s] permissions [errno=%d]. Going to remove the tmp directory [%s] and exit (update)",downloadFileLocalPath.c_str(),errno,tmpDirectoryForUpdate.c_str());

          CUtil::WipeDir(tmpDirectoryForUpdate);
          reset();
          return;          
        }
        
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - The installtion script [path=%s] permissions was changed (update)",downloadFileLocalPath.c_str());
        
        scriptToRunPermissionsWasChanged = true;
      }
#endif
    }

    if((!(m_versionUpdateInfo.m_UpdateNotesFileName).IsEmpty()) && (releaseNotesFileNameWasFound == false))
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - The updateNotesFileName [%s] ISN'T the name of one of the files that was downloaded (update)",(m_versionUpdateInfo.m_UpdateNotesFileName).c_str());
    }

#ifndef HAS_EMBEDDED  
    if((!(m_versionUpdateInfo.m_scriptToRunName).IsEmpty()) && (scriptToRunPermissionsWasChanged == false))
    {
#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

      CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - The installation script name [%s] ISN'T one of the files that were downloaded. Going to remove the tmp directory [%s] and exit (update)",(m_versionUpdateInfo.m_scriptToRunName).c_str(),tmpDirectoryForUpdate.c_str());
      
      CUtil::WipeDir(tmpDirectoryForUpdate);
      reset();
      return;              
    }
#endif    
  }

  /////////////////////////////////////////////////////////////////////////////////////
  // After downloading all of the files to the tmp directory -> Rename the directory //
  /////////////////////////////////////////////////////////////////////////////////////

  CStdString directoryForUpdate = UPDATE_DIRECTORY_BASE_PATH;
  directoryForUpdate += m_versionUpdateBuildNum;
  int renameFailed = ::rename(tmpDirectoryForUpdate.c_str(),directoryForUpdate);
  if (renameFailed)
  {
#ifdef HAS_EMBEDDED
    SetVersionUpdateDownloadStatus(VUDS_FAILED);
#endif

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to rename directory of update files from [%s] to [%s]. Going to remove the tmp directory and exit (update)",tmpDirectoryForUpdate.c_str(),directoryForUpdate.c_str());

    CUtil::WipeDir(tmpDirectoryForUpdate);
    reset();
    return;
  }

#ifdef HAS_EMBEDDED
  /////////////////////////////////////////////////////////////////////////////////////
  // Create two symlinks: one points on update directory and second on image file   //
  ////////////////////////////////////////////////////////////////////////////////////

  CStdString directoryForUpdateSymlink = UPDATE_DIRECTORY_BASE_PATH;
  directoryForUpdateSymlink += "current";
  ::unlink(directoryForUpdateSymlink);
  int symlinkFailed = ::symlink(directoryForUpdate, directoryForUpdateSymlink);
  if(symlinkFailed)
  {
    SetVersionUpdateDownloadStatus(VUDS_FAILED);

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to create symbolic link [%s] -> [%s]. Going to remove the update directory and exit (update)", directoryForUpdateSymlink.c_str(),directoryForUpdate.c_str());

    CUtil::WipeDir(directoryForUpdate);
    reset();
    return;
  }

  CStdString imageFile = directoryForUpdate;
  imageFile += "/" + m_versionUpdateInfo.m_ImageFile;
  CStdString imageFileSymlink = directoryForUpdate;
  imageFileSymlink += "/image";
  ::unlink(imageFileSymlink);
  symlinkFailed = ::symlink(imageFile, imageFileSymlink);
  if(symlinkFailed)
  {
    SetVersionUpdateDownloadStatus(VUDS_FAILED);

    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::Run - Failed to create symbolic link [%s] to [%s]. Going to remove the update directory and exit (update)",imageFile.c_str(),imageFileSymlink.c_str());

    CUtil::WipeDir(directoryForUpdate);
    reset();
    return;    
  }
  
  CStdString versionFileName = directoryForUpdate;
  versionFileName += "/version";
  CFile versionFile;
  versionFile.OpenForWrite(versionFileName, true);
  versionFile.Write(m_versionUpdateBuildNum.c_str(), m_versionUpdateBuildNum.length());
  versionFile.Close();
#endif

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Directory of update files was renamed from [%s] to [%s]. Going to call UpdateFilesPathToLocal() (update)",tmpDirectoryForUpdate.c_str(),directoryForUpdate.c_str());

  ///////////////////////////////////////////////////
  // Going to update downloaded file path to local //
  ///////////////////////////////////////////////////
  
  UpdateFilesPathToLocal(directoryForUpdate);
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - After call to UpdateFilesPathToLocal(). Going to call ReadReleaseNoteFile() for reading the release notes (update)");

  ////////////////////////////
  // Read the release notes //
  ////////////////////////////
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Going to call ReadReleaseNoteFile() for reading the release notes file (update)");

  bool succeedToRead = ReadReleaseNoteFile();
  
  if(succeedToRead)
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Call ReadReleaseNoteFile() was successful (update)");        
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Call ReadReleaseNoteFile() failed. No release notes will be shown (update)");
    m_versionUpdateInfo.m_UpdateNotesFileText = "";
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - After call to ReadReleaseNoteFile(). Going to change VersionUpdateJobStatus to VUJS_READY_FOR_UPDATE (update)");

  SetVersionUpdateJobStatus(VUJS_READY_FOR_UPDATE);

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::Run - Going to set NewVersion flag in CGUIInfoManager as TRUE (update)");

  g_infoManager.SetHasNewVersion(true,m_versionUpdateInfo.GetVersionUpdateForce() == VUF_YES ? true : false);

#ifdef HAS_EMBEDDED
  SetVersionUpdateDownloadStatus(VUDS_FINISHED);
#endif

  return; 
}

bool CBoxeeVersionUpdateJob::ParseUpdateFile(const CStdString& updateFileLocalPath)
{
  bool retVal = false;

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Enter function with [updateFileLocalPath=%s]. Going to load file to BXXMLDocument (update)",updateFileLocalPath.c_str());

  BOXEE::BXXMLDocument reader;
  bool loadSucceeded = reader.LoadFromFile(updateFileLocalPath);
  
  if(!loadSucceeded)
  {
    CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - Failed to load file [updateFileLocalPath=%s] to BXXMLDocument (update)",updateFileLocalPath.c_str());
  }
  else
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Succeeded to load file [updateFileLocalPath=%s] to BXXMLDocument. Going to parse it (update)",updateFileLocalPath.c_str());

    TiXmlElement* root = reader.GetRoot();

    if (root)
    {
      if((strcmp(root->Value(),"update") == 0))
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - The root element <update> was found. Going to parse [updateFileLocalPath=%s] (update)",updateFileLocalPath.c_str());
        
        m_versionUpdateInfo.m_versionUpdateForce = root->Attribute("force");

        if((m_versionUpdateInfo.m_versionUpdateForce).IsEmpty())
        {
          CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - The [force] attribute in <update> is empty (update)");
        }      
        else
        {
          CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under <update> element parse [force=%s] (update)",(m_versionUpdateInfo.m_versionUpdateForce).c_str());
          
          root = root->FirstChildElement();

          bool foundDownload = false;
          bool foundUpdateNotes = false;
          bool foundScriptToRun = false;
          bool foundVersion = false;
          bool emptyAttribute = false;
          bool foundImageFile = false;
          
#ifdef HAS_EMBEDDED
          m_TotalBytesToDownload = 0;
#endif

          while (root)
          {
            if (strcmp(root->Value(),"download") == 0)
            {
              (m_versionUpdateInfo.m_UpdateFilesToDownload).clear();
             
              TiXmlElement* fileElem;
              fileElem = root->FirstChildElement("file");
              
              int filesCounter = 0;
              
              while(fileElem)
              {
                filesCounter++;
                
                CUpdateFilesInfo ufi;
                
                ufi.m_filePath = fileElem->Attribute("url");
                ufi.m_fileHash = fileElem->Attribute("hash");
#ifdef HAS_EMBEDDED
                ufi.m_fileSize = fileElem->Attribute("size");
                m_TotalBytesToDownload += atoi(ufi.m_fileSize);
#endif
                (m_versionUpdateInfo.m_UpdateFilesToDownload).push_back(ufi);

                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under element <download> parse file [%d] - [path=%s][hash=%s] (update)",filesCounter,(ufi.m_filePath).c_str(),(ufi.m_fileHash).c_str());
                
                fileElem = fileElem->NextSiblingElement("file");
              }

              int numOfFilesToDownload = (m_versionUpdateInfo.m_UpdateFilesToDownload).size();
              
              CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under element <download> parse [%d] files (update)",numOfFilesToDownload);

              foundDownload = true;
            }
            else if (strcmp(root->Value(),"update-notes-file") == 0)
            {
              m_versionUpdateInfo.m_UpdateNotesFileName = root->Attribute("name");
              
              if((m_versionUpdateInfo.m_UpdateNotesFileName).IsEmpty())
              {
                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - The [name] attribute in <update-notes> is empty -> break (update)");
                emptyAttribute = true;
                break;
              }
              else
              {
                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under <update-notes-file> element parse [name=%s] (update)",(m_versionUpdateInfo.m_UpdateNotesFileName).c_str());
                foundUpdateNotes = true;
              }
            }
            else if (strcmp(root->Value(),"script-to-run") == 0)
            {
              m_versionUpdateInfo.m_scriptToRunName = root->Attribute("name");
              
              if((m_versionUpdateInfo.m_scriptToRunName).IsEmpty())
              {
                CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - The [name] attribute in <script-to-run> is empty -> break (update)");
                emptyAttribute = true;
                break;
              }
              else
              {
                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under <script-to-run> element parse [name=%s] (update)",(m_versionUpdateInfo.m_scriptToRunName).c_str());
                foundScriptToRun = true;
              }
            }
            else if (strcmp(root->Value(),"version") == 0)
            {
              m_versionUpdateInfo.m_UpdateVersionNum = root->Attribute("value");
              
              if((m_versionUpdateInfo.m_UpdateVersionNum).IsEmpty())
              {
                CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - The [value] attribute in <version> is empty -> break (update)");
                emptyAttribute = true;
                break;
              }
              else
              {
                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under <version> element parse [value=%s] (update)",(m_versionUpdateInfo.m_UpdateVersionNum).c_str());
                foundVersion = true;
              }
            }
#ifdef HAS_EMBEDDED
            else if (strcmp(root->Value(),"image-file") == 0)
            {
              m_versionUpdateInfo.m_ImageFile = root->Attribute("name");
              if((m_versionUpdateInfo.m_ImageFile).IsEmpty())
              {
                CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - The [value] attribute in <image-file> is empty -> break (update)");
                emptyAttribute = true;
                break;
              }
              else
              {
                CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Under <image-file> element parse [value=%s] (update)",(m_versionUpdateInfo.m_UpdateVersionNum).c_str());
                foundImageFile = true;
              }
            
            }
#endif
            root = root->NextSiblingElement();
          }
          
          if(emptyAttribute)
          {
            CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - Parse of [updateFileLocalPath=%s] failed because one of the attribute was empty (update)",updateFileLocalPath.c_str());            
          }
          else
          {
            CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - After parse of [updateFileLocalPath=%s]. [foundDownload=%d][foundUpdateNotes=%d][foundScriptToRun=%d][foundVersion=%d][foundImageFile=%d] (update)",updateFileLocalPath.c_str(),foundDownload,foundUpdateNotes,foundScriptToRun,foundVersion,foundImageFile);
            retVal = true;
          }
        }
      }
      else
      {
        CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - The root tag ISN'T <update> but [%s] in [updateFileLocalPath=%s] (update)",root->Value(),updateFileLocalPath.c_str());
      }
    }
    else
    {
      CLog::Log(LOGERROR,"CBoxeeVersionUpdateJob::ParseUpdateFile - Failed to get root from BXXMLDocument of [updateFileLocalPath=%s] (update)",updateFileLocalPath.c_str());         
    }
  }
  
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ParseUpdateFile - Exit function. Going to return [retVal=%d]. [updateFileLocalPath=%s] (update)",retVal,updateFileLocalPath.c_str());

  return retVal;
}

void CBoxeeVersionUpdateJob::UpdateFilesPathToLocal(const CStdString& directoryForUpdateLocalPath)
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::UpdateFilesPathToLocal - Enter function with [directoryForUpdateLocalPath=%s] (update)",directoryForUpdateLocalPath.c_str());

  for(size_t i=0;i<(m_versionUpdateInfo.m_UpdateFilesToDownload).size();i++)
  {
    CUpdateFilesInfo& ufi = (m_versionUpdateInfo.m_UpdateFilesToDownload)[i];
    
    CStdString orgPath = ufi.m_filePath;
    
    CStdString strName = CUtil::GetFileName(orgPath);
    ufi.m_filePath = directoryForUpdateLocalPath;
    ufi.m_filePath += PATH_SEPARATOR_STRING;
    ufi.m_filePath += strName;

    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::UpdateFilesPathToLocal - [%zu/%zu] file path was changed from [%s] to [%s] (update)",i+1,(m_versionUpdateInfo.m_UpdateFilesToDownload).size(),orgPath.c_str(),(ufi.m_filePath).c_str());    
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::UpdateFilesPathToLocal - Exit function (update)");
}

bool CBoxeeVersionUpdateJob::ReadReleaseNoteFile()
{
  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Enter function. Going to find the release notes file (update)");

  bool retVal = false;
  bool releaseNotesFileNameWasFound = false;  

  for(size_t i=0;i<(m_versionUpdateInfo.m_UpdateFilesToDownload).size();i++)
  {
    CUpdateFilesInfo& ufi = (m_versionUpdateInfo.m_UpdateFilesToDownload)[i];
    CStdString strName = CUtil::GetFileName(ufi.m_filePath);
    
    if(strName == m_versionUpdateInfo.m_UpdateNotesFileName)
    {
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Release notes file was found [path=%s] (update)",(ufi.m_filePath).c_str());

      releaseNotesFileNameWasFound = true;
      
      CFile releaseNotesFile;
      if (!releaseNotesFile.Open(ufi.m_filePath,false))
      {
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Failed to open release notes file [path=%s] (update)",(ufi.m_filePath).c_str());
      }
      else
      {
        char szLine[4096];

        m_versionUpdateInfo.m_UpdateNotesFileText = "";
        
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Going to read release notes from file [path=%s][size=%lld] (update)",(ufi.m_filePath).c_str(),releaseNotesFile.GetLength());

        while (releaseNotesFile.ReadString(szLine, 1024))
        {
          m_versionUpdateInfo.m_UpdateNotesFileText += szLine;
        }
        
        CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Finish reading release notes from file [path=%s] (update)",(ufi.m_filePath).c_str());
        
        retVal = true;
      }
      
      CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Going to close the release notes file [path=%s] (update)",(ufi.m_filePath).c_str());

      releaseNotesFile.Close();      
    }
  }
  
  if(releaseNotesFileNameWasFound == false)
  {
    CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Release notes file WASN'T found [name=%s] (update)",(m_versionUpdateInfo.m_UpdateNotesFileName).c_str());
  }

  CLog::Log(LOGDEBUG,"CBoxeeVersionUpdateJob::ReadReleaseNoteFile - Exit function. Going to return [%d] (update)",retVal);

  return retVal;
}

CVersionUpdateInfo& CBoxeeVersionUpdateJob::GetVersionUpdateInfo()
{
  return m_versionUpdateInfo;
}

//////////////////////////////////
// CVersionUpdateInfo functions //
//////////////////////////////////

CVersionUpdateInfo::CVersionUpdateInfo()
{
  m_UpdateFilesToDownload.clear();
  m_versionUpdateForce = "No";
  m_scriptToRunName = "";
  m_UpdateNotesFileName = "";
  m_UpdateNotesFileText = "";
  m_UpdateVersionNum = "";  
}

void CVersionUpdateInfo::reset()
{
  m_UpdateFilesToDownload.clear();
  m_versionUpdateForce = "No";
  m_scriptToRunName = "";
  m_UpdateNotesFileName = "";
  m_UpdateNotesFileText = "";
  m_UpdateVersionNum = "";
}

VERSION_UPDATE_FORCE CVersionUpdateInfo::GetVersionUpdateForce()
{
  VERSION_UPDATE_FORCE versionUpdateForceEnum = VUF_NO;
  
  CStdString versionUpdateForceStr = m_versionUpdateForce;
  versionUpdateForceStr.ToLower();
  
  if(versionUpdateForceStr == "yes")
  {
    versionUpdateForceEnum = VUF_YES;
  }
  else if(versionUpdateForceStr == "no")
  {
    versionUpdateForceEnum = VUF_NO;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CVersionUpdateInfo::GetVersionUpdateForceAsEnum - Failed to convert [%s] to enum. Going to set update as VUF_NO (update)",m_versionUpdateForce.c_str());
  }
  
  return versionUpdateForceEnum;
}

VERSION_UPDATE_FORCE CVersionUpdateInfo::GetVersionUpdateForceAsEnum(const CStdString& versionUpdateForce)
{
  VERSION_UPDATE_FORCE versionUpdateForceEnum = VUF_NO;
  
  CStdString versionUpdateForceStr = versionUpdateForce;
  versionUpdateForceStr.ToLower();
  
  if(versionUpdateForceStr == "yes")
  {
    versionUpdateForceEnum = VUF_YES;
  }
  else if(versionUpdateForceStr == "no")
  {
    versionUpdateForceEnum = VUF_NO;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CVersionUpdateInfo::GetVersionUpdateForceAsEnum - Failed to convert [%s] to enum. Going to set update as VUF_NO (update)",versionUpdateForce.c_str());
  }
  
  return versionUpdateForceEnum;
}
