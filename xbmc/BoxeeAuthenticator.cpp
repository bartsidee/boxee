

#if defined(_LINUX)
#include <dlfcn.h>
#else
#include <windows.h>
#endif

#include "StdString.h"
#include "BoxeeAuthenticator.h"
#include "bxconfiguration.h"
#include "bxcurl.h"
#include "GUIInfoManager.h"
#include "SystemInfo.h"
#include "Util.h"
#include "File.h"
#include "SpecialProtocol.h"
#include "utils/log.h"
#include "LocalizeStrings.h"

#ifdef _WINDOWS
#define FW_LOAD_PROC GetProcAddress
#else
#define FW_LOAD_PROC dlsym
#endif

#define AUTH_MIN_VALID_VERSION 4

BoxeeAuthenticator::BoxeeAuthenticator()
{
  m_DllHandle = NULL;
  _AuthInit = NULL;
  _AuthApp = NULL;
  _AuthMain = NULL;
  _AuthFree = NULL;
  _AuthVersion = NULL;
}

BoxeeAuthenticator::~BoxeeAuthenticator()
{
  if (m_DllHandle)
  {
    UnloadDll();
  }
}

bool BoxeeAuthenticator::Init()
{
  CStdString serverUrlPrefix = BOXEE::BXConfiguration::GetInstance().GetURLParam("Boxee.Server","http://app.boxee.tv");
  CStdString version = g_infoManager.GetVersion();
  CStdString platform;
#if defined(_LINUX) && !defined(__APPLE__)
  platform = "linux";
#elif defined(__APPLE__)
  if(CSysInfo::IsAppleTV())
  {
    platform = "atv";
  }
  else
  {
    platform = "mac";
  }
#elif defined(_WIN32)
  platform = "win";
#endif
  CStdString pathToCookieJar = BOXEE::BXCurl::GetCookieJar();
  CStdString pathToAppsDir = _P("special://home/apps/");
  
  CStdString strUrl = serverUrlPrefix;
  strUrl += "/ping?bxlibauth_ver=";
  
  if (LoadDll())
  {    
    const char* currentVersion = _AuthVersion();
    CLog::Log(LOGDEBUG, "Checking for new authenticator version. current: %s", currentVersion);
    
    strUrl += currentVersion;

    UnloadDll();
  }
  else
  {
    CLog::Log(LOGDEBUG, "No authenticator dll found. downloading");
    strUrl += "0";
  }

  BOXEE::BXCurl curl;
  if (!curl.HttpHEAD(strUrl))
  {
    return false;
  }
  
  if (curl.GetLastRetCode() == 200)
  {
    CLog::Log(LOGDEBUG, "Downloading authenticator dll");
    
    // Download the file
    CStdString TempFileName = "special://temp/AuthenticatorDll.bin";
    TempFileName = _P(TempFileName);
    
    if (!curl.HttpDownloadFile(strUrl, TempFileName, ""))
    {
      CLog::Log(LOGERROR, "Unable to download authenticator dll");
      XFILE::CFile::Delete(TempFileName);
      return false;
    }
    
    // Rename the downloaded file
    CStdString DllFileName = GetDllFilename();
    XFILE::CFile::Delete(DllFileName);
    XFILE::CFile::Rename(TempFileName, DllFileName);
  }
  
  if (LoadDll())
  {
    const char* currentVersion = _AuthVersion();
    CLog::Log(LOGDEBUG, "Using authenticator version %s", currentVersion);

    _AuthInit(platform.c_str(), version.c_str(), pathToCookieJar.c_str(), pathToAppsDir.c_str(), serverUrlPrefix.c_str());
    
    return true;
  }
  else
  {
    CLog::Log(LOGWARNING, "Cannot load authenticator dll");
    
    return false;
  }
}

CStdString BoxeeAuthenticator::GetDllFilename()
{
  CStdString result;
  result = "special://home/system/bxauth-";
  
  CStdString platform = CUtil::GetPlatform();
  if (platform == "linux")
  {
#ifdef __x86_64__
    result += "x86_64-linux.so";
#else
    result += "i486-linux.so";
#endif

  }
  else if (platform == "win")
  {
    result += "win32.dll";
  }
  else
  {
    result += "osx.so";
  }

  result = _P(result);
  
  return result;
}

CStdString BoxeeAuthenticator::AuthenticateApp(const CStdString& appId)
{
  if (IsDllLoaded() && _AuthApp)
  {
    char *appToken = _AuthApp(appId.c_str());
    CStdString result = appToken;
    _AuthFree(appToken);
    return result;
  }
  else
  {
    return "";
  }
}

bool BoxeeAuthenticator::AuthenticateMain()
{
  if (IsDllLoaded())
  {
    return _AuthMain();
  }
  else
  {
    return false;
  }
}

bool BoxeeAuthenticator::LoadDll()
{
  if (m_DllHandle)
  {
    UnloadDll();
  }

  CStdString dllFileName = GetDllFilename();
  CLog::Log(LOGDEBUG,"BoxeeAuthenticator::LoadDll - dllFileName was set [%s] (ba)",dllFileName.c_str());

#ifdef _LINUX
  m_DllHandle = dlopen(dllFileName.c_str(), RTLD_LAZY);
  if (!m_DllHandle)
  {
    CLog::Log(LOGWARNING,"BoxeeAuthenticator::LoadDll - FAILED to open [dllFileName=%s] (ba). Boxee authenticator disabled.",dllFileName.c_str());
    return false;
  }
#else
  m_DllHandle = LoadLibrary(dllFileName.c_str());
  if (!m_DllHandle)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - FAILED to LoadLibrary [dllFileName=%s] (ba)",dllFileName.c_str());
    return false;
  }
#endif
  _AuthVersion = (BXAuthVersionType) FW_LOAD_PROC(m_DllHandle, "BXVersion"); 
  if (_AuthVersion == NULL)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error loading func BXVersion. [dllFileName=%s] (ba)",dllFileName.c_str());
    UnloadDll();
    return false;
  }
    
  if (atoi(_AuthVersion()) < AUTH_MIN_VALID_VERSION)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error - bxauth version is too old, not loading authenticator [dllFileName=%s, ver: %s] (ba)",dllFileName.c_str(), _AuthVersion());
    UnloadDll();
    return false;
  }

  _AuthInit = (BXAuthInitType) FW_LOAD_PROC(m_DllHandle, "BXAuthInit"); 
  if (_AuthInit == NULL)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error loading func BXAuthInit. [dllFileName=%s] (ba)",dllFileName.c_str());
    UnloadDll();
    return false;
  }
  
  _AuthApp = (BXAuthAppType) FW_LOAD_PROC(m_DllHandle, "BXAuthApp"); 
  if (_AuthApp == NULL)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error loading func BXAuthApp. [dllFileName=%s] (ba)",dllFileName.c_str());
    UnloadDll();
    return false;
  }

  _AuthMain = (BXAuthMainType) FW_LOAD_PROC(m_DllHandle, "BXAuthMain"); 
  if (_AuthMain == NULL)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error loading func BXAuthMain. [dllFileName=%s] (ba)",dllFileName.c_str());
    UnloadDll();
    return false;
  }
  
  _AuthFree = (BXAuthFreeType) FW_LOAD_PROC(m_DllHandle, "BXFree"); 
  if (_AuthFree == NULL)
  {
    CLog::Log(LOGERROR,"BoxeeAuthenticator::LoadDll - Error loading func BXFree. [dllFileName=%s] (ba)",dllFileName.c_str());
    UnloadDll();
    return false;
  }  

  return true;
}

void BoxeeAuthenticator::UnloadDll()
{
  if (m_DllHandle)
  {
#ifdef _LINUX
    dlclose(m_DllHandle);
#else
    FreeLibrary(m_DllHandle);
#endif
  }

  m_DllHandle = NULL;  
  _AuthInit = NULL;
  _AuthApp = NULL;
  _AuthMain = NULL;
  _AuthFree = NULL ;
  _AuthVersion = NULL;
}

bool BoxeeAuthenticator::IsDllLoaded()
{
  return (m_DllHandle != NULL);
}
