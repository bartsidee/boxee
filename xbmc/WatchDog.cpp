
#include "PlatformDefs.h"
#include "WatchDog.h"
#include "bxconfiguration.h"
#include "bxcurl.h"
#include "utils/log.h"
#include "FileSystem/VirtualDirectory.h"
#include "FileSystem/IDirectory.h"
#include "FileSystem/FactoryDirectory.h"
#include "StringUtils.h"
#include "SingleLock.h"
#include "Util.h"
#include "Application.h"
#include "bxexceptions.h"
#include "URL.h"
#include "lib/libBoxee/bxutils.h"
#include "lib/libBoxee/bxxmldocument.h"
#include "lib/libBoxee/bxcurl.h"
#include "lib/libBoxee/boxee.h"
#include "utils/GUIInfoManager.h"
#include "BoxeeUtils.h"
#include "BoxeeVersionUpdateManager.h"
#include "SystemInfo.h"
#include "SpecialProtocol.h"
#include "FileSystem/FileSmb.h"
#include "FileSystem/FileCurl.h"
#include "FileSystem/HTSPDirectory.h"
#include "FileSystem/DllLibCurl.h"
#include "FileSystem/CMythSession.h"
#include "UPnP.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "utils/Weather.h"
#include "GUIDialogOK.h"
#include "utils/GUIInfoManager.h"

#ifdef HAS_PYTHON
#include "lib/libPython/XBPython.h"
#endif

#define BXINFO_FILE_PATH "special://home/bxinfo.xml"

#define CHECK_INTERNET_CONNECTION_INTERVAL_IN_MS_HI_FREQ     10000       // 10 sec
#define CHECK_INTERNET_CONNECTION_INTERVAL_IN_MS_LOW_FREQ    (1*60000)   // 1 min

#define SEND_PING_TO_SERVER_INTERVAL_IN_MS                   (10*60000)  // 10 min
#define TEST_PATH_INTERVAL_IN_MS                             (1*60000)   // 1 min
#define PROCESS_SLOW_INTERVAL_IN_MS                          (1*60000)   // 1 min

WatchDog::WatchDog() : m_profileBaseSTM("wdProfileSTM",500), m_generalSTM("wdGeneralSTM",500)
{
  m_bConnectedToServer = true;
  m_bIsConnectToInternet = true;

  m_isStoped = true;
}

WatchDog::~WatchDog()
{
  m_generalSTM.Stop();

  if (m_pingJob)
  {
    delete m_pingJob;
    m_pingJob = NULL;
  }

  if (m_testPathJob)
  {
    delete m_testPathJob;
    m_testPathJob = NULL;
  }

  if (m_processSlowJob)
  {
    delete m_processSlowJob;
    m_processSlowJob = NULL;
  }

  if (m_checkInternetConnectionJob)
  {
    delete m_checkInternetConnectionJob;
    m_checkInternetConnectionJob = NULL;
  }
}

bool WatchDog::Start()
{
  CLog::Log(LOGDEBUG,"WatchDog::Start - Enter function. [isStoped=%d] (wd)",m_isStoped);

  bool succeeded = StartGeneralSTM();
  if(!succeeded)
  {
    CLog::Log(LOGERROR,"WatchDog::Start - FAILED to start GeneralScheduleTaskManager. [isStoped=%d] (wd)",m_isStoped);
    return false;
  }

  succeeded = StartProfileSTM();
  if(!succeeded)
  {
    CLog::Log(LOGERROR,"WatchDog::Start - FAILED to start ProfileScheduleTaskManager. [isStoped=%d] (wd)",m_isStoped);
    return false;
  }

  m_isStoped = false;

  CLog::Log(LOGDEBUG,"WatchDog::Start - Exit function. [isStoped=%d] (wd)",m_isStoped);

  return true;
}

bool WatchDog::StartGeneralSTM()
{
  if (m_generalSTM.IsRunning())
  {
    CLog::Log(LOGDEBUG,"WatchDog::StartGeneralSTM - GeneralScheduleTaskManager is already running. [isStoped=%d] (wd)",m_isStoped);
    return true;
  }

  bool succeeded = m_generalSTM.Start(1);
  if(!succeeded)
  {
    CLog::Log(LOGERROR,"WatchDog::StartGeneralSTM - FAILED to start GeneralScheduleTaskManager. [isStoped=%d] (wd)",m_isStoped);
    return false;
  }

  if (m_checkInternetConnectionJob)
  {
    delete m_checkInternetConnectionJob;
    m_checkInternetConnectionJob = NULL;
  }

  unsigned long checkInternetConnectionIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.CheckInternetConnectionLowInMs",CHECK_INTERNET_CONNECTION_INTERVAL_IN_MS_LOW_FREQ);
  m_checkInternetConnectionJob = new CCheckInternetConnectionJob(this,0,checkInternetConnectionIntervalInMS);
  if (m_checkInternetConnectionJob)
  {
    m_generalSTM.AddScheduleTask(m_checkInternetConnectionJob);
  }
  else
  {
    CLog::Log(LOGERROR,"WatchDog::StartGeneralSTM - FAILED to add CheckInternetConnectionJob to ScheduleTaskManager (wd)");
  }

  if (m_pingJob)
  {
    delete m_pingJob;
    m_pingJob = NULL;
  }

  unsigned long pingFreqIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.PingFreqInMs",SEND_PING_TO_SERVER_INTERVAL_IN_MS);
  m_pingJob = new CPingJob(this,0,pingFreqIntervalInMS);
  if (m_pingJob)
  {
    m_generalSTM.AddScheduleTask(m_pingJob);
  }
  else
  {
    CLog::Log(LOGERROR,"WatchDog::StartGeneralSTM - FAILED to add PingJob to ScheduleTaskManager (wd)");
  }

  return true;
}

bool WatchDog::StartProfileSTM()
{
  if (m_profileBaseSTM.IsRunning())
  {
    CLog::Log(LOGDEBUG,"WatchDog::StartProfileSTM - ProfileScheduleTaskManager is running. Going to stop it before start. [isStoped=%d] (wd)",m_isStoped);
    m_profileBaseSTM.Stop();
    CLog::Log(LOGDEBUG,"WatchDog::StartProfileSTM - ProfileScheduleTaskManager was stoped. [isStoped=%d] (wd)",m_isStoped);
  }

  bool succeeded = m_profileBaseSTM.Start(1);
  if(!succeeded)
  {
    CLog::Log(LOGERROR,"WatchDog::StartProfileSTM - FAILED to start ProfileScheduleTaskManager (wd)");
    return false;
  }

  if (m_testPathJob)
  {
    delete m_testPathJob;
    m_testPathJob = NULL;
  }

  unsigned long testPathIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.TestSourcesFreqInMs",TEST_PATH_INTERVAL_IN_MS);
  m_testPathJob = new CTestPathJob(this,0,testPathIntervalInMS);
  if (m_testPathJob)
  {
    m_profileBaseSTM.AddScheduleTask(m_testPathJob);
  }
  else
  {
    CLog::Log(LOGERROR,"WatchDog::StartProfileSTM - FAILED to add TestPathJob to ScheduleTaskManager (wd)");
  }

  if (m_processSlowJob)
  {
    delete m_processSlowJob;
    m_processSlowJob = NULL;
  }

  unsigned long processSlowIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.ProcessSlowFreqInMs",PROCESS_SLOW_INTERVAL_IN_MS);
  m_processSlowJob = new CProcessSlowJob(this,0,processSlowIntervalInMS);
  if (m_processSlowJob)
  {
    m_profileBaseSTM.AddScheduleTask(m_processSlowJob);
  }
  else
  {
    CLog::Log(LOGERROR,"WatchDog::StartProfileSTM - FAILED to add ProcessSlowJob to ScheduleTaskManager (wd)");
  }

  return true;
}

bool WatchDog::Stop()
{
  CLog::Log(LOGDEBUG,"WatchDog::Stop - Enter function. [isStoped=%d] (wd)",m_isStoped);

  bool succeeded = m_profileBaseSTM.Stop();
  if(!succeeded)
  {
    CLog::Log(LOGERROR,"WatchDog::Stop - FAILED to stop ProfileScheduleTaskManager. [isStoped=%d] (wd)",m_isStoped);
    return false;
  }

  m_isStoped = true;

  CLog::Log(LOGDEBUG,"WatchDog::Stop - Exit function. [isStoped=%d] (wd)",m_isStoped);

  return true;
}

bool WatchDog::IsStoped()
{
  return m_isStoped;
}

void WatchDog::AddListener(IWatchDogListener* pListener)
{
  CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_vecListeners.size(); i++)
  {
    if (m_vecListeners[i] == pListener)
      return;
  }
  
  m_vecListeners.push_back(pListener);
}

void WatchDog::RemoveListener(IWatchDogListener* pListener)
{
  CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_vecListeners.size(); i++)
  {
    if (m_vecListeners[i] == pListener)
    {
      m_vecListeners.erase(m_vecListeners.begin() + i);
      return;
    }
  }
}

void WatchDog::NotifyListeners(const CStdString &strPath, bool bAvailable)
{
  CSingleLock lock(m_lock);
  for (size_t i = 0; i < m_vecListeners.size(); i++)
  {
    if (m_vecListeners[i])
      m_vecListeners[i]->PathUpdate(strPath, bAvailable);
  }
}

void WatchDog::SetIsConnectedToServer(bool bIsConnectToServer)
{
  m_bConnectedToServer = bIsConnectToServer;
}

bool WatchDog::IsConnectedToServer()
{
  return m_bConnectedToServer;
}

void WatchDog::SetIsConnectedToInternet(bool bIsConnectToInternet)
{
  m_bIsConnectToInternet = bIsConnectToInternet;
}

bool WatchDog::IsConnectedToInternet()
{
  return m_bIsConnectToInternet;
}

void  WatchDog::ProcessSlow()
{
#if defined(_LINUX) && defined(HAS_FILESYSTEM_SMB)
  smb.CheckIfIdle();
#endif

  // check for any idle curl connections
  g_curlInterface.CheckIdle();

  // check for any idle myth sessions
  XFILE::CCMythSession::CheckIdle();

#ifdef HAS_FILESYSTEM_HTSP
  // check for any idle htsp sessions
  HTSP::CHTSPDirectorySession::CheckIdle();
#endif

#ifdef HAS_TIME_SERVER
  // check for any needed sntp update
  if(m_psntpClient && m_psntpClient->UpdateNeeded())
    m_psntpClient->Update();
#endif

  // update upnp server/renderer states
  if(CUPnP::IsInstantiated())
    CUPnP::GetInstance()->UpdateState();
}

void WatchDog::TestPath(const CStdString &strPath)
{
  CFileItem item;
  item.m_strPath = strPath;
  bool bAvailable = false;
  bool bHasInfo = false;

  if (item.IsInternetStream() || item.IsPlugin() || item.IsScript() || item.IsApp())
  {
    bAvailable = IsConnectedToInternet();
    bHasInfo = true;
  }
  else if (!CUtil::IsSmb(strPath) || !g_application.IsPlaying())
  {
    DIRECTORY::IDirectory* pDir = DIRECTORY::CFactoryDirectory::Create(strPath);
    if (pDir) 
    {
      bAvailable = pDir->Exists(strPath);
      bHasInfo = true;
      delete pDir;
    }
  }

  if (item.IsSmb() || item.IsHD())
  {
    CLog::Log(LOGDEBUG,"WatchDog::TestPath - [share=%s][available=%d][hasInfo=%d] (testpath)",strPath.c_str(), bAvailable, bHasInfo);
  }

  if (bHasInfo)
  {
    CSingleLock lock(m_lock);
    // If there was a change in a status, notify listeners
    if ( (bAvailable && m_mapPaths[strPath] != WD_AVAILABLE) || (!bAvailable && m_mapPaths[strPath] != WD_UNAVAILABLE) )
      NotifyListeners(strPath, bAvailable);
    
    m_mapPaths[strPath] = bAvailable?WD_AVAILABLE:WD_UNAVAILABLE;

    CLog::Log(LOGDEBUG,"WatchDog::TestPath - For [share=%s] set [available=%d] (testpath)",strPath.c_str(), m_mapPaths[strPath]);
  }
}

bool WatchDog::IsPathAvailable(const CStdString &pathToCheck, bool bDefault)
{
  CStdString strPath = _P(pathToCheck);

  CFileItem item;
  item.m_strPath = strPath;
  if (item.IsApp())
    return true;
  
  if (item.IsInternetStream())
    return g_application.IsConnectedToNet();
  
  CSingleLock lock(m_lock);
  
  CURL url1 (strPath);
  CStdString strUrl1;
  url1.GetURL(strUrl1);
  strUrl1 = BOXEE::BXUtils::RemoveSMBCredentials(strUrl1);
  strUrl1.ToLower();
  CUtil::RemoveSlashAtEnd(strUrl1);

  for (std::map<CStdString, PathStatus>::iterator iter=m_mapPaths.begin(); iter != m_mapPaths.end(); iter++)
  {
    CURL url2(iter->first);
    CStdString strUrl2;
    url2.GetURL(strUrl2);
    strUrl2 = BOXEE::BXUtils::RemoveSMBCredentials(strUrl2);
    strUrl2.ToLower();
    CUtil::RemoveSlashAtEnd(strUrl2);
    
    if(strUrl1.Find(strUrl2) >= 0)
    { 
      if (iter->second == WD_UNKNOWN)
      {
        return bDefault;
      }
      
      return (iter->second == WD_AVAILABLE);
    }
  }

  return bDefault; // we dont know...
}

bool WatchDog::IsPathWatched(const char *strPath)
{
  CSingleLock lock(m_lock);
  return m_mapPaths.find(strPath) != m_mapPaths.end();
}

void WatchDog::AddPathToWatch(const char *strPath)
{
  CSingleLock lock(m_lock);
  
  CStdString pathToWatch(strPath);
  pathToWatch = _P(pathToWatch);

  if (pathToWatch.IsEmpty())
    return;

  if (!IsPathWatched(pathToWatch))
  {
    m_mapPaths[pathToWatch] = (CUtil::IsSmb(pathToWatch) || CUtil::IsUPnP(pathToWatch))?WD_UNKNOWN:WD_AVAILABLE; // default is unknown only for network
  }
}

void WatchDog::RemovePathFromWatch(const char *strPath)
{
  CSingleLock lock(m_lock);
  
  CStdString pathToRemove(strPath);
  pathToRemove = _P(pathToRemove);

  CLog::Log(LOGDEBUG,"WATCHDOG: remove path [pathToRemove=%s] from watched. [OrgPath=%s]", pathToRemove.c_str(),strPath);
  
  if (!IsPathWatched(pathToRemove))
  {
    m_mapPaths.erase(pathToRemove);
  }
}

void WatchDog::CleanWatchedPaths()
{
  CSingleLock lock(m_lock);
  CLog::Log(LOGDEBUG,"WATCHDOG: clean watched paths");
  m_mapPaths.clear();
}

///////////////////////////////
// CProcessSlowJob functions //
///////////////////////////////

WatchDog::CProcessSlowJob::CProcessSlowJob(WatchDog* jobHandler, unsigned long executionDelayInMS, unsigned long repeatTaskIntervalInMS) : BoxeeScheduleTask("WatchDogProcessSlowTask",executionDelayInMS,repeatTaskIntervalInMS)
{
  m_jobHandler = jobHandler;
}

WatchDog::CProcessSlowJob::~CProcessSlowJob()
{

}

void WatchDog::CProcessSlowJob::DoWork()
{
  if (!m_jobHandler)
  {
    CLog::Log(LOGWARNING,"CProcessSlowJob::DoWork - Can't execute the job. [jobHandler=NULL] (wdpslow)");
    return;
  }

  CLog::Log(LOGWARNING,"CProcessSlowJob::DoWork - Enter function. [server=%d][internet=%d][IsStoped=%d] (wdpslow)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet(),m_jobHandler->IsStoped());

  if (!m_jobHandler->IsStoped())
  {
    m_jobHandler->ProcessSlow();
  }

  CLog::Log(LOGWARNING,"CProcessSlowJob::DoWork - Exit function. [server=%d][internet=%d] (wdpslow)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());
}

bool WatchDog::CProcessSlowJob::ShouldDelete()
{
  return false;
}

////////////////////////////
// CTestPathJob functions //
////////////////////////////

WatchDog::CTestPathJob::CTestPathJob(WatchDog* jobHandler, unsigned long executionDelayInMS, unsigned long repeatTaskIntervalInMS) : BoxeeScheduleTask("WatchDogTestPathTask",executionDelayInMS,repeatTaskIntervalInMS)
{
  m_jobHandler = jobHandler;
}

WatchDog::CTestPathJob::~CTestPathJob()
{

}

void WatchDog::CTestPathJob::DoWork()
{
  if (!m_jobHandler)
  {
    CLog::Log(LOGWARNING,"CTestPathJob::DoWork - Can't execute the job. [jobHandler=NULL] (testpath)");
    return;
  }

  CLog::Log(LOGDEBUG,"CTestPathJob::DoWork - Enter function. [server=%d][internet=%d] (testpath)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  // scan all shares
  CStdStringArray arr;

  // first copy share names from map to temp array so that we only lock the map shortly
  {
    CSingleLock lock(m_jobHandler->m_lock);
    for (std::map<CStdString, PathStatus>::iterator iter=m_jobHandler->m_mapPaths.begin(); iter != m_jobHandler->m_mapPaths.end(); iter++)
    {
      arr.push_back(iter->first);
    }
  }

  for (size_t n=0; n<arr.size() && !m_jobHandler->IsStoped(); n++)
  {
    CLog::Log(LOGDEBUG,"CTestPathJob::DoWork - [%d/%d] - Going to test [path=%s]. [server=%d][internet=%d] (testpath)",(int)n+1,(int)arr.size(),arr[n].c_str(),m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

    m_jobHandler->TestPath(arr[n]);
  }

  CLog::Log(LOGDEBUG,"CTestPathJob::DoWork - Exit function. [server=%d][internet=%d] (testpath)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());
}

bool WatchDog::CTestPathJob::ShouldDelete()
{
  return false;
}

////////////////////////
// CPingJob functions //
////////////////////////

WatchDog::CPingJob::CPingJob(WatchDog* jobHandler, unsigned long executionDelayInMS, unsigned long repeatTaskIntervalInMS) : BoxeeScheduleTask("WatchDogPingTask",executionDelayInMS,repeatTaskIntervalInMS)
{
  m_jobHandler = jobHandler;
}

WatchDog::CPingJob::~CPingJob()
{

}

void WatchDog::CPingJob::DoWork()
{
  if (!m_jobHandler)
  {
    CLog::Log(LOGWARNING,"CPingJob::DoWork - Can't execute the job. [jobHandler=NULL] (ping)");
    return;
  }

  CLog::Log(LOGDEBUG,"CPingJob::DoWork - Enter function. [server=%d][internet=%d] (ping)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());


  if (!m_jobHandler->IsConnectedToInternet())
  {
    CLog::Log(LOGDEBUG,"CPingJob::DoWork - There is no internet connection -> Don't try to send ping. [server=%d][internet=%d] (ping)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());
    return;
  }

  std::set<CStdString> allowedPaths;
  std::set<CStdString> allowedExt;

  allowedPaths.insert("special://xbmc");
  allowedPaths.insert("special://home");
  allowedPaths.insert("special://temp");

  allowedExt.insert(".xml");
  allowedExt.insert(".png");
  allowedExt.insert(".gif");
  allowedExt.insert(".jpg");
  allowedExt.insert(".tbn");

  // test boxee
  BOXEE::BXCurl curl;

  CStdString strClientPingVersion = "";
  CStdString pingUrl = "";

  bool succeed = InitPingRequest(pingUrl,strClientPingVersion);

  CLog::Log(LOGDEBUG,"CPingJob::DoWork - Call to InitPingRequest() returned [%d]. [strClientPingVersion=%s][pingUrl=%s] (ping)",succeed,strClientPingVersion.c_str(),pingUrl.c_str());

  if(!succeed)
  {
    CLog::Log(LOGERROR,"CPingJob::DoWork - Call to InitPingParams() FAILED. [retVal=%d] (ping)",succeed);
    return;
  }

  int nClientPingVersion = atoi(strClientPingVersion.c_str());

  CLog::Log(LOGWARNING,"CPingJob::DoWork - Going to check ping. [server=%d][internet=%d] (ping)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  std::string strResp = curl.HttpGetString(pingUrl, false);
  g_application.SetBoxeeServerIP(curl.GetServerIP());

  long retCode = curl.GetLastRetCode();
  CLog::Log(LOGWARNING,"CPingJob::DoWork - Check ping returned [retCode=%ld][Response-IsEmpty=%d]. [server=%d][internet=%d] (ping)",retCode,strResp.empty(),m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  m_jobHandler->SetIsConnectedToServer((retCode == 200));

  BOXEE::BXXMLDocument reader;
  if (strResp.empty())
  {
    CLog::Log(LOGDEBUG,"WatchDog::Process - Not handling server response to [pingUrl=%s] because it is empty (ping)",pingUrl.c_str());
    return;
  }

  if(!reader.LoadFromString(strResp))
  {
    CLog::Log(LOGERROR,"WatchDog::Process - Not handling server response to [pingUrl=%s] because failed to load it to BXXMLDocument (ping)",pingUrl.c_str());
    return;
  }

  TiXmlElement* root = reader.GetRoot();

  if(!root)
  {
    CLog::Log(LOGERROR,"WatchDog::Process - Failed to get root from BXXMLDocument of the ping response (ping)");
    return;
  }

  if((strcmp(root->Value(),"ping") != 0))
  {
    CLog::Log(LOGERROR,"WatchDog::Process - Failed to parse ping response because the root tag ISN'T <ping> (ping)");
    return;
  }

  CLog::Log(LOGDEBUG,"CPingJob::DoWork - The root tag <ping> was found. Going to parse the ping response (ping)");

  TiXmlElement* pingChildElem = NULL;
  pingChildElem = root->FirstChildElement();

  while (pingChildElem)
  {
    if (strcmp(pingChildElem->Value(), "cmds") == 0)
    {
      CLog::Log(LOGDEBUG,"CPingJob::DoWork - The <cmds> tag was found (ping)");

      CStdString strServerPingVersion = "";
      strServerPingVersion = pingChildElem->Attribute("ping_version");
      int nServerPingVersion = atoi(strServerPingVersion.c_str());

      if (nServerPingVersion > nClientPingVersion)
      {
        CLog::Log(LOGDEBUG,"CPingJob::DoWork - Because [nServerPingVersion=%d] > [%d=nClientPingVersion] going to parse the <cmds> tag (ping)",nServerPingVersion,nClientPingVersion);

        TiXmlElement* cmdsChildElem = NULL;
        cmdsChildElem = pingChildElem->FirstChildElement();

        while (cmdsChildElem)
        {
          if (strcmp(cmdsChildElem->Value(), "download") == 0)
          {
            CStdString url = cmdsChildElem->Attribute("url");
            CStdString local = cmdsChildElem->Attribute("local");
            CStdString hash = cmdsChildElem->Attribute("hash");

            CLog::Log(LOGDEBUG,"CPingJob::DoWork - Found <download> tag with attributes [url=%s][local=%s][hash=%s] (ping)",url.c_str(),local.c_str(),hash.c_str());

            if (!url.IsEmpty() && !local.IsEmpty())
            {
              CStdString strDir;
              CStdString strLocal = local;
              CStdString ext = CUtil::GetExtension(strLocal);
              CStdString strName = CUtil::GetFileName(strLocal);
              CUtil::GetDirectory(strLocal, strDir);
              if (allowedExt.find(ext.c_str()) != allowedExt.end() && allowedPaths.find(CUtil::GetSpecialPathPrefix(local).c_str()) != allowedPaths.end() && CUtil::CreateDirectoryEx(strDir))
              {
                CLog::Log(LOGDEBUG,"CPingJob::DoWork - Going to download [url=%s] (ping)",url.c_str());
                BoxeeUtils::SafeDownload(url.c_str(), strLocal.c_str(), hash);
              }
            }
          }
          else if (strcmp(cmdsChildElem->Value(), "remove") == 0)
          {
            CStdString local = cmdsChildElem->Attribute("path");

            CLog::Log(LOGDEBUG,"CPingJob::DoWork - Found <remove> tag with attributes [local=%s] (ping)",local.c_str());

            CStdString strLocal = local;
            CStdString ext = CUtil::GetExtension(strLocal);
            if (allowedExt.find(ext.c_str()) != allowedExt.end() && allowedPaths.find(CUtil::GetSpecialPathPrefix(local).c_str()) != allowedPaths.end())
            {
              CLog::Log(LOGDEBUG,"CPingJob::DoWork - Going to delete [strLocal=%s] (ping)",strLocal.c_str());
              ::DeleteFile(strLocal);
            }
          }

          cmdsChildElem = cmdsChildElem->NextSiblingElement();
        }

        if (!strServerPingVersion.IsEmpty())
        {
          // currently - version is the only info kept so we take a shortcut...
          TiXmlDocument *infoDoc = NULL;
          CStdString strInfoDoc = "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" ?><info><ping_version>"+strServerPingVersion+"</ping_version></info>";
          BOXEE::BXXMLDocument newinfo;
          if (newinfo.LoadFromString(strInfoDoc))
          {
            infoDoc = newinfo.GetRoot()->GetDocument();
          }

          if (infoDoc)
          {
            infoDoc->SaveFile(PTH_IC(BXINFO_FILE_PATH));
          }
        }
      }
      else
      {
        CLog::Log(LOGDEBUG,"CPingJob::DoWork - Because [nServerPingVersion=%d] <= [%d=nClientPingVersion] NOT going to parse the <cmds> tag (ping)",nServerPingVersion,nClientPingVersion);
      }
    }
    else if (strcmp(pingChildElem->Value(),"version_update") == 0)
    {
      CLog::Log(LOGDEBUG,"CPingJob::DoWork - The <version_update> tag was found (ping)");

      // Get the update type
      CStdString versionUpdateBuildNum = pingChildElem->Attribute("build-num");
      CStdString versionUpdateFilePath = pingChildElem->Attribute("update-descriptor-path");
      CStdString versionUpdateFileHash = pingChildElem->Attribute("update-descriptor-hash");

      CLog::Log(LOGDEBUG,"CPingJob::DoWork - <version_update> parameter has the following attributes [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

      if (!versionUpdateFilePath.IsEmpty() && !versionUpdateBuildNum.IsEmpty() && !versionUpdateFileHash.IsEmpty())
      {
        CLog::Log(LOGDEBUG,"CPingJob::DoWork - Going to call CBoxeeVersionUpdateManager::PrepareVersionUpdate() with [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());

        bool retVal = g_boxeeVersionUpdateManager.PrepareVersionUpdate(versionUpdateBuildNum,versionUpdateFilePath,versionUpdateFileHash);

        CLog::Log(LOGDEBUG,"CPingJob::DoWork - Call to CBoxeeVersionUpdateManager::PrepareVersionUpdate() with [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] returned [%d] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str(),retVal);
      }
      else
      {
        CLog::Log(LOGERROR,"CPingJob::DoWork - One of the attributes of <version_update> is empty. [build-num=%s][update-descriptor-path=%s][update-descriptor-hash=%s] (ping)(update)",versionUpdateBuildNum.c_str(),versionUpdateFilePath.c_str(),versionUpdateFileHash.c_str());
      }
    }

    pingChildElem = pingChildElem->NextSiblingElement();
  }

  CLog::Log(LOGDEBUG,"CPingJob::DoWork - Exit function. [server=%d][internet=%d] (ping)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());
}

bool WatchDog::CPingJob::InitPingRequest(CStdString& pingUrl,CStdString& strPingVersion)
{
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

  pingUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("WatchDog.TestServerUrl","http://app.boxee.tv/");

  if(g_application.GetInSlideshowScreensaver())
  {
    pingUrl += "idle/";
  }
  else
  {
    pingUrl += "ping/";
  }

  pingUrl += BoxeeUtils::GetPlatformStr();
  pingUrl += "/";

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

  pingUrl += boxeeCurrentVersion;

  pingUrl += "/";
  pingUrl += strPingVersion;

  return true;
}

bool WatchDog::CPingJob::ShouldDelete()
{
  return false;
}

///////////////////////////////////////////
// CCheckInternetConnectionJob functions //
///////////////////////////////////////////

WatchDog::CCheckInternetConnectionJob::CCheckInternetConnectionJob(WatchDog* jobHandler, unsigned long executionDelayInMS, unsigned long repeatTaskIntervalInMS) : BoxeeScheduleTask("WatchDogCheckInternetConnectionTask",executionDelayInMS,repeatTaskIntervalInMS)
{
  m_jobHandler = jobHandler;

  m_vecBoxeeServersUrls.clear();
  m_vecBoxeeServersUrls.push_back("http://0.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://1.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://2.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://3.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://4.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://5.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://6.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://7.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://8.ping.boxee.tv/");
  m_vecBoxeeServersUrls.push_back("http://9.ping.boxee.tv/");
}

WatchDog::CCheckInternetConnectionJob::~CCheckInternetConnectionJob()
{

}

void WatchDog::CCheckInternetConnectionJob::DoWork()
{
  if (!m_jobHandler)
  {
    CLog::Log(LOGWARNING,"CCheckInternetConnectionJob::DoWork - Can't execute the job. [jobHandler=NULL] (offline)");
    return;
  }

  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::DoWork - Enter function. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  bool cIsConnectToInternet = m_jobHandler->IsConnectedToInternet();

  bool nIsConnectedToInternet = CheckIsConnectedToInternet();

  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::DoWork - After check [cIsConnectToInternet=%d][nIsConnectedToInternet=%d]. [server=%d][internet=%d] (offline)",cIsConnectToInternet,nIsConnectedToInternet,m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  if (cIsConnectToInternet != nIsConnectedToInternet)
  {
    // set new IsConnectToInternet status
    m_jobHandler->SetIsConnectedToInternet(nIsConnectedToInternet);

    if (nIsConnectedToInternet)
    {
      // new IsConnectToInternet status is CONNECT

      HandleInternetConnectionRestore();
    }
    else
    {
      // new IsConnectToInternet status is NOT CONNECT

      HandleInternetConnectionLost();
    }
  }

  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::DoWork - Exit function. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());
}

bool WatchDog::CCheckInternetConnectionJob::CheckIsConnectedToInternet()
{
  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::CheckIsConnectedToInternet - Enter function. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  std::random_shuffle(m_vecBoxeeServersUrls.begin(), m_vecBoxeeServersUrls.end());

  for (int i=0; i<(int)m_vecBoxeeServersUrls.size(); i++)
  {
    CStdString serverUrl = m_vecBoxeeServersUrls[i];

    BOXEE::BXCurl curl;
    bool connect = curl.HttpHEAD(serverUrl.c_str());

    CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::CheckIsConnectedToInternet - Check [%s] return [%d]. [server=%d][internet=%d] (offline)",serverUrl.c_str(),connect,m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

    if (connect)
    {
      CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::CheckIsConnectedToInternet - Exit function and return TRUE. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

      return true;
    }
  }

  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::CheckIsConnectedToInternet - Exit function and return FALSE. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  return false;
}

bool WatchDog::CCheckInternetConnectionJob::HandleInternetConnectionRestore()
{
  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::HandleInternetConnectionRestore - Enter function. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  bool succeeded = true;

  // connection was restore -> set new interval for checking internet connection
  m_repeatTaskIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.CheckInternetConnectionLowInMs",CHECK_INTERNET_CONNECTION_INTERVAL_IN_MS_LOW_FREQ);

  // resume the media library
  BOXEE::Boxee::GetInstance().GetMetadataEngine().Resume();

  // update user list received from server now
  BOXEE::Boxee::GetInstance().GetBoxeeClientServerComManager().UpdateAllUserListsNow();

  if ((g_windowManager.GetActiveWindow() != WINDOW_LOGIN_SCREEN) && g_application.IsOfflineMode())
  {
    succeeded = LoginUserAfterInternetConnectionRestore();
  }

  return succeeded;
}

bool WatchDog::CCheckInternetConnectionJob::LoginUserAfterInternetConnectionRestore()
{
  BOXEE::BXObject userObj(false);

  int lastUsedProfileIndex = g_settings.m_iLastUsedProfileIndex;
  BOXEE::BXCredentials creds;
  creds.SetUserName(g_settings.m_vecProfiles[lastUsedProfileIndex].getID());
  creds.SetPassword(g_settings.m_vecProfiles[lastUsedProfileIndex].getLockCode());

  BOXEE::BXLoginStatus loginStatus = BOXEE::Boxee::GetInstance().Login(creds,userObj);

  if ((loginStatus != BOXEE::LOGIN_SUCCESS) || !userObj.IsValid())
  {
    CLog::Log(LOGWARNING,"CCheckInternetConnectionJob::LoginUserAfterInternetConnectionRestore - FAILED to login [loginStatus=%d] -> logout. [server=%d][internet=%d] (offline)",loginStatus,m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

    CGUIDialogOK* pDialog = (CGUIDialogOK*)g_windowManager.GetWindow(WINDOW_DIALOG_OK);
    pDialog->SetHeading(257);
    pDialog->SetLine(0, 53480);
    pDialog->SetLine(1, "");
    pDialog->SetLine(2, "");

    ThreadMessage tMsgNotify = {TMSG_DIALOG_DOMODAL, WINDOW_DIALOG_OK, g_windowManager.GetActiveWindow()};
    g_application.getApplicationMessenger().SendMessage(tMsgNotify, true);

    ThreadMessage tMsgLogout = {TMSG_LOGOUT};
    g_application.getApplicationMessenger().SendMessage(tMsgLogout, false);

    return false;
  }

  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::LoginUserAfterInternetConnectionRestore - Succeeded to login user [user=%s][pass=%s][loginStatus=%d] -> logout. [server=%d][internet=%d] (offline)(login)",creds.GetUserName().c_str(),creds.GetPassword().c_str(),loginStatus,m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  BoxeeUtils::UpdateProfile(lastUsedProfileIndex,userObj);

  g_application.SetOfflineMode(false);
  m_jobHandler->SetIsConnectedToServer(true);
  g_infoManager.LoginAfterConnectionRestoreWasDone();

  g_settings.m_vecProfiles[g_settings.m_iLastLoadedProfileIndex].setDate();
  g_settings.SaveProfiles(PROFILES_FILE);

  if (getenv("BX_COUNTRY_CODE"))
  {
    g_application.SetCountryCode(getenv("BX_COUNTRY_CODE"));
  }
  else
  {
    BOXEE::BXObject obj;
    BOXEE::Boxee::GetInstance().GetCurrentUser(obj);
    std::string cc = obj.GetValue("country");
    g_application.SetCountryCode(cc);
  }

  g_weatherManager.Refresh();

#ifdef HAS_PYTHON
  g_pythonParser.m_bLogin = true;
#endif

  CGUIMessage refreshHomeWinMsg(GUI_MSG_UPDATE, WINDOW_HOME, 0);
  g_windowManager.SendThreadMessage(refreshHomeWinMsg);

  return true;
}

bool WatchDog::CCheckInternetConnectionJob::HandleInternetConnectionLost()
{
  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::HandleInternetConnectionLost - Enter function. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  // connection was lost -> set new interval for checking internet connection
  m_repeatTaskIntervalInMS = BOXEE::BXConfiguration::GetInstance().GetIntParam("WatchDog.CheckInternetConnectionHiInMs",CHECK_INTERNET_CONNECTION_INTERVAL_IN_MS_HI_FREQ);

  // pause the media library
  BOXEE::Boxee::GetInstance().GetMetadataEngine().Pause();

  // no internet connection -> no srever connection
  m_jobHandler->SetIsConnectedToServer(false);

  return true;
}

bool WatchDog::CCheckInternetConnectionJob::ShouldDelete()
{
  CLog::Log(LOGDEBUG,"CCheckInternetConnectionJob::ShouldDelete - Return TRUE. [server=%d][internet=%d] (offline)",m_jobHandler->IsConnectedToServer(),m_jobHandler->IsConnectedToInternet());

  return false;
}

