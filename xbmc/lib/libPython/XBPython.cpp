/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifdef _XBOX
#pragma comment(linker, "/merge:PY_TEXT=PYTHON")
#pragma comment(linker, "/merge:PY_DATA=PY_RW")
#pragma comment(linker, "/merge:PY_BSS=PY_RW")
#pragma comment(linker, "/merge:PY_RDATA=PYTHON")
#endif

// python.h should always be included first before any other includes

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
#include "Python/Include/Python.h"
#include "Python/Include/osdefs.h"
#endif
#include "cores/DllLoader/DllLoaderContainer.h"
#include "GUIPassword.h"

#include "XBPython.h"
#include "XBPythonDll.h"
#include "Settings.h"
#include "Profile.h"
#include "FileSystem/File.h"
#include "FileSystem/SpecialProtocol.h"
#include "utils/log.h"
#include "utils/SingleLock.h"

XBPython g_pythonParser;

#ifndef _LINUX
#define PYTHON_DLL "special://xbmc/system/python/python24.dll"
#else
#if defined(__APPLE__)
#if defined(__POWERPC__)
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-osx.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86-osx.so"
#endif
#elif defined(__x86_64__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-x86_64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-x86_64-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-x86_64-linux.so"
#endif
#elif defined(_POWERPC)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-powerpc-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-powerpc-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc-linux.so"
#endif
#elif defined(_POWERPC64)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-powerpc64-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-powerpc64-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-powerpc64-linux.so"
#endif
#elif defined(__arm__)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-arm-none-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-arm-none-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-arm-none-linux.so"
//#define PYTHON_DLL "/usr/lib/libpython2.4.so"
#endif
#elif defined(CANMORE)
#if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-i686-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-i686-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-i686-linux.so"
//#define PYTHON_DLL "/usr/lib/libpython2.4.so"
#endif
#else /* !__x86_64__ && !__powerpc__ */

#ifdef __x86_64__
#if (defined HAVE_LIBPYTHON2_6)
    #define PYTHON_DLL "special://xbmc/system/python/python26-x86_64-linux.so"
  #elif (defined HAVE_LIBPYTHON2_5)
    #define PYTHON_DLL "special://xbmc/system/python/python25-x86_64-linux.so"
  #else
    #define PYTHON_DLL "special://xbmc/system/python/python24-x86_64-linux.so"
  #endif
#else
  #if (defined HAVE_LIBPYTHON2_6)
#define PYTHON_DLL "special://xbmc/system/python/python26-i486-linux.so"
#elif (defined HAVE_LIBPYTHON2_5)
#define PYTHON_DLL "special://xbmc/system/python/python25-i486-linux.so"
#elif (defined CANMORE)
#define PYTHON_DLL "special://xbmc/system/python/python24-i686-linux.so"
#else
#define PYTHON_DLL "special://xbmc/system/python/python24-i486-linux.so"
#endif
#endif


#endif /* __x86_64__ */
#endif /* _LINUX */

extern "C" HMODULE __stdcall dllLoadLibraryA(LPCSTR file);
extern "C" BOOL __stdcall dllFreeLibrary(HINSTANCE hLibModule);

extern "C" {
  void InitXBMCModule(void);
  void InitXBMCTypes(void);
  void DeinitXBMCModule(void);
  void InitPluginModule(void);
  void InitPluginTypes(void);
  void DeinitPluginModule(void);
  void InitGUIModule(void);
  void InitGUITypes(void);
  void DeinitGUIModule(void);
// BOXEE  
  void init_mc(void);
// End BOXEE  
}

XBPython::XBPython()
{
  m_bInitialized = false;
  m_bStartup = false;
  m_bLogin = false;
  m_nextid = 0;
  m_mainThreadState = NULL;
  m_hEvent = CreateEvent(NULL, false, false, "pythonEvent");
  m_globalEvent = CreateEvent(NULL, false, false, "pythonGlobalEvent");
  m_ThreadId = (ThreadIdentifier)CThread::GetCurrentThreadId();
  m_vecPlayerCallbackList.clear();
  m_iDllScriptCounter = 0;
}

XBPython::~XBPython()
{
  CloseHandle(m_globalEvent);
}

// message all registered callbacks that xbmc stopped playing
void XBPython::OnPlayBackEnded(bool bError, const CStdString& error)
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackEnded(bError, error);
      it++;
    }
  }
}

// message all registered callbacks that we started playing
void XBPython::OnPlayBackStarted()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStarted();
      it++;
    }
  }
}

// message all registered callbacks that user stopped playing
void XBPython::OnPlayBackStopped()
{
  if (m_bInitialized)
  {
    PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
    while (it != m_vecPlayerCallbackList.end())
    {
      ((IPlayerCallback*)(*it))->OnPlayBackStopped();
      it++;
    }
  }
}

void XBPython::RegisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  m_vecPlayerCallbackList.push_back(pCallback);
}

void XBPython::UnregisterPythonPlayerCallBack(IPlayerCallback* pCallback)
{
  PlayerCallbackList::iterator it = m_vecPlayerCallbackList.begin();
  while (it != m_vecPlayerCallbackList.end())
  {
    if (*it == pCallback)
    {
      it = m_vecPlayerCallbackList.erase(it);
    }
    else 
      it++;
  }
}

/**
 * Check for file and print an error if needed
 */
bool XBPython::FileExist(const char* strFile)
{
  if (!strFile) 
    return false;

  if (!XFILE::CFile::Exists(strFile))
  {
    CLog::Log(LOGERROR, "Python: Cannot find '%s'", strFile);
    return false;
  }
  return true;
}

void XBPython::RegisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib) 
    return;

  CSingleLock lock(m_critSection);

  CLog::Log(LOGDEBUG,"%s, adding %s (%p)", __FUNCTION__, pLib->GetName(), (void*)pLib);
  m_extensions.push_back(pLib);
}

void XBPython::UnregisterExtensionLib(LibraryLoader *pLib)
{
  if (!pLib) 
    return;

  CSingleLock lock(m_critSection);
  CLog::Log(LOGDEBUG,"%s, removing %s (0x%p)", __FUNCTION__, pLib->GetName(), (void *)pLib);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
    if (*iter == pLib)
    {
      m_extensions.erase(iter);
      break;
    }
    iter++;
  }
}

void XBPython::UnloadExtensionLibs()
{
  CLog::Log(LOGDEBUG,"%s, clearing python extension libraries", __FUNCTION__);
  CSingleLock lock(m_critSection);
  PythonExtensionLibraries::iterator iter = m_extensions.begin();
  while (iter != m_extensions.end())
  {
    DllLoaderContainer::ReleaseModule(*iter);
    iter++;
  }
  m_extensions.clear();
}

void XBPython::InitializeInterpreter()
{
  InitXBMCModule(); // init xbmc modules
  InitPluginModule(); // init plugin modules
  InitGUIModule(); // init xbmcgui modules
// BOXEE  
  init_mc();
// End BOXEE
  // redirecting default output to debug console
  if (PyRun_SimpleString(""
      "import xbmc\n"
      "class xbmcout:\n"
      "	def write(self, data):\n"
      "		xbmc.output(data)\n"
      "	def close(self):\n"
      "		xbmc.output('.')\n"
      "	def flush(self):\n"
      "		xbmc.output('.')\n"
      "\n"
      "import sys\n"
// BOXEE
      "import mc\n"
// End BOXEE      
      "sys.stdout = xbmcout()\n"
      "sys.stderr = xbmcout()\n"
      "") == -1)
  {
    CLog::Log(LOGFATAL, "Python Initialize Error");
  }

  // Hack for getcwd
  if (PyRun_SimpleString(""
      "import os\n"
      "def getcwd_xbmc():\n"
      "  import __main__\n"
      "  return os.path.dirname(__main__.__file__)\n"
      "\n"
      "def chdir_xbmc(dir):\n"
      "  raise RuntimeError(\"os.chdir not supported in xbmc\")\n"
      "\n"
      "os_getcwd_original = os.getcwd\n"
      "os.getcwd          = xbmc.getcwd\n"
      "os.chdir_orignal   = os.chdir\n"
      "os.chdir           = chdir_xbmc\n"
      "") == -1)
  {
    CLog::Log(LOGFATAL, "Python Hack Initialize Error ");
  }

  if (PyRun_SimpleString(""
      "print '-->Python Interpreter Initialized<--'\n"
      "") == -1)
  {
    CLog::Log(LOGFATAL, "Python Hack Initialize Error ");
  }
}

void XBPython::DeInitializeInterpreter()
{
  DeinitXBMCModule(); 
  DeinitPluginModule(); 
  DeinitGUIModule();   
}

/**
 * Should be called before executing a script
 */
void XBPython::Initialize()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    return;
  }

  m_iDllScriptCounter++;
  CLog::Log(LOGINFO, "XBPython::Initialize, initializing python engine.");
    if (m_ThreadId == (ThreadIdentifier)CThread::GetCurrentThreadId())
    {
#ifdef USE_PYTHON_DLL
      m_pDll = DllLoaderContainer::LoadModule(PYTHON_DLL, NULL, true);

      if (!m_pDll || !python_load_dll(*m_pDll))
      {
        CLog::Log(LOGFATAL, "Python: error loading python dll");
        Finalize();
        return;
      }
#endif

      // first we check if all necessary files are installed
#ifndef _LINUX      
      if (!FileExist("special://xbmc/system/python/python24.zlib") ||
        !FileExist("special://xbmc/system/python/DLLs/_socket.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/_ssl.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/bz2.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/pyexpat.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/select.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/unicodedata.pyd") ||
        !FileExist("special://xbmc/system/python/DLLs/zlib.pyd"))
      {
        CLog::Log(LOGERROR, "Python: Missing files, unable to execute script");
        Finalize();
        return;
      }
#endif        

      CStdString path;
      path += PTH_IC("special://xbmc/system/python/local");
#if (!defined USE_EXTERNAL_PYTHON)
      path += DELIM;
      path += PTH_IC("special://xbmc/system/python/Lib");
      path += DELIM;
      path += PTH_IC("special://xbmc/system/python/python24.zip");
#ifdef _LINUX
    
      // Info about interesting python envvars available
      // at http://docs.python.org/using/cmdline.html#environment-variables
      
      // Required for python to find optimized code (pyo) files
      setenv("PYTHONOPTIMIZE", "1", 1);
      setenv("PYTHONHOME", _P("special://xbmc/system/python").c_str(), 1);      
      setenv("PYTHONPATH", path.c_str(), 1);
      //setenv("PYTHONDEBUG", "1", 1);
      //setenv("PYTHONINSPECT", "1", 1);
      //setenv("PYTHONVERBOSE", "1", 1);
      setenv("PYTHONCASEOK", "1", 1);
      CLog::Log(LOGDEBUG, "Python wrapper library linked with internal Python library");
#endif /* _LINUX */
#else
      /* PYTHONOPTIMIZE is set off intentionally when using external Python.
         Reason for this is because we cannot be sure what version of Python
         was used to compile the various Python object files (i.e. .pyo,
         .pyc, etc.). */
      setenv("PYTHONOPTIMIZE", "1", 1);
      //setenv("PYTHONDEBUG", "1", 1);
      //setenv("PYTHONINSPECT", "1", 1);
      //setenv("PYTHONVERBOSE", "1", 1);

#ifdef EXTERNAL_PYTHON_PREFIX
      path.Format("%s:%s:%s/plat-linux2:%s/lib-dynload:%s/site-packages:", EXTERNAL_PYTHON_ZIP,
          EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX);
      path += PTH_IC("special://xbmc/system/python/local");
#endif

#ifdef EXTERNAL_PYTHON_HOME
      setenv("PYTHONHOME", "/opt/local", 1);
#endif

      setenv("PYTHONPATH", path.c_str(), 1);

      setenv("PYTHONCASEOK", "1", 1); //This line should really be removed
      CLog::Log(LOGDEBUG, "Python wrapper library linked with system Python library");
#endif /* USE_EXTERNAL_PYTHON */

      Py_Initialize();
      PyEval_InitThreads();

      char* python_argv[1] = { (char*)"" } ;
      PySys_SetArgv(1, python_argv);

      InitXBMCTypes();
      InitGUITypes();
      InitPluginTypes();      

      m_mainThreadState = PyThreadState_Get();
      PyThreadState_Swap(m_mainThreadState);

      char *pathCopy = strdup(path.c_str());
      CLog::Log(LOGDEBUG,"using python path: <%s>", pathCopy);
      PySys_SetPath(pathCopy);
      free(pathCopy);
 
      InitializeInterpreter();
      PyThreadState_Swap(NULL);

      // release the lock
      PyEval_ReleaseLock();

      m_bInitialized = true;
      PulseEvent(m_hEvent);
    }
    else
    {
      // only the main thread should initialize python.
      m_iDllScriptCounter--;

      lock.Leave();
      WaitForSingleObject(m_hEvent, INFINITE);
      lock.Enter();
    }
  }

/**
 * Should be called when a script is finished
 */
void XBPython::Finalize()
{
  CSingleLock lock(m_critSection);
  // for linux - we never release the library. its loaded and stays in memory.
  if (m_iDllScriptCounter)
  m_iDllScriptCounter--;
  else
    CLog::Log(LOGERROR, "Python script counter attempted to become negative");
  if (m_iDllScriptCounter == 0 && m_bInitialized)
  {
    CLog::Log(LOGINFO, "Python, unloading python24.dll because no scripts are running anymore");
    
    PyEval_AcquireLock();
    PyThreadState_Swap(m_mainThreadState);
    
    Py_Finalize();
    PyEval_ReleaseLock();

    UnloadExtensionLibs();

    // first free all dlls loaded by python, after that python24.dll (this is done by UnloadPythonDlls
    DllLoaderContainer::UnloadPythonDlls();
#ifdef USE_PYTHON_DLL
    // we can't release it on windows, as this is done in UnloadPythonDlls() for win32 (see above).
    // The implementation for linux and os x needs looking at - UnloadPythonDlls() currently only searches for "python24.dll"
    DllLoaderContainer::ReleaseModule(m_pDll);
#endif    
    m_hModule = NULL;
    m_mainThreadState = NULL;
    m_bInitialized = false;
  }
}

void XBPython::FreeResources()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    // cleanup threads that are still running
    PyList::iterator it = m_vecPyList.begin();
    while (it != m_vecPyList.end())
    {
      PyElem inf = *it;
      m_vecPyList.erase(it);
      lock.Leave(); //unlock here because the python thread might lock when it exits
      delete inf.pyThread;
      lock.Enter();
      Finalize();
      it = m_vecPyList.begin();
    }
  }
  
  if (m_hEvent)
    CloseHandle(m_hEvent);
}

void XBPython::Process()
{
  CStdString strAutoExecPy;

  if (m_bStartup)
  {
    m_bStartup = false;

    // autoexec.py - userdata
    strAutoExecPy = "special://home/scripts/autoexec.py";

    if (XFILE::CFile::Exists(strAutoExecPy))
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no user autoexec.py (%s) found, skipping", __FUNCTION__, CSpecialProtocol::TranslatePath(strAutoExecPy).c_str());

    // autoexec.py - system
    strAutoExecPy = "special://xbmc/scripts/autoexec.py";

    if (XFILE::CFile::Exists(strAutoExecPy))
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no system autoexec.py (%s) found, skipping", __FUNCTION__, CSpecialProtocol::TranslatePath(strAutoExecPy).c_str());
  }

  if (m_bLogin)
  {
    m_bLogin = false;

    // autoexec.py - profile
    strAutoExecPy = "special://profile/scripts/autoexec.py";

    if (XFILE::CFile::Exists(strAutoExecPy))
      evalFile(strAutoExecPy);
    else
      CLog::Log(LOGDEBUG, "%s - no profile autoexec.py (%s) found, skipping", __FUNCTION__, CSpecialProtocol::TranslatePath(strAutoExecPy).c_str());
  }

  CSingleLock lock(m_critSection);

  if (m_bInitialized)
  {
    PyList::iterator it = m_vecPyList.begin();
    while (it != m_vecPyList.end())
    {
      //delete scripts which are done
      if (it->bDone)
      {
        delete it->pyThread;
        it = m_vecPyList.erase(it);
// BOXEE        
        //Finalize();
// End BOXEE
      }
      else ++it;
    }
  }
  }

int XBPython::evalFile(const char *src) { return evalFile(src, 0, NULL); }
// execute script, returns -1 if script doesn't exist
int XBPython::evalFile(const char *src, const unsigned int argc, const char ** argv)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
  {
    CLog::Log(LOGERROR, "Python script \"%s\" does not exist", CSpecialProtocol::TranslatePath(src).c_str());
    return -1;
  }

  Initialize();

  if (!m_bInitialized) return -1;

  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalFile(src);
  PyElem inf;
  inf.id = m_nextid;
  inf.bDone = false;
  inf.strFile = src;
  inf.pyThread = pyThread;

  m_vecPyList.push_back(inf);

  return m_nextid;
}

void XBPython::setDone(int id)
{
  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == id)
    {
      if (it->pyThread->isStopping())
        CLog::Log(LOGINFO, "Python script interrupted by user");
      else
        CLog::Log(LOGINFO, "Python script stopped");
      it->bDone = true;
    }
    ++it;
  }
}

void XBPython::stopScript(int id)
{
  CSingleExit ex(g_graphicsContext);
  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == id) {
      CLog::Log(LOGINFO, "Stopping script with id: %i", id);
      it->pyThread->stop();
      return;
    }
    ++it;
  }
}

PyThreadState *XBPython::getMainThreadState()
{
  CSingleLock lock(m_critSection);
  return m_mainThreadState;
}

int XBPython::ScriptsSize()
{
  CSingleLock lock(m_critSection);
  return m_vecPyList.size();
}

const char* XBPython::getFileName(int scriptId)
{
  const char* cFileName = NULL;

  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId) cFileName = it->strFile.c_str();
    ++it;
  }

  return cFileName;
}
  
int XBPython::getScriptId(const char* strFile)
{
  int iId = -1;

  CSingleLock lock(m_critSection);

  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (!stricmp(it->strFile.c_str(), strFile)) iId = it->id;
    ++it;
  }

  return iId;
}

bool XBPython::isRunning(int scriptId)
{
  bool bRunning = false;
  CSingleLock lock(m_critSection); 

  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId)	bRunning = true;
    ++it;
  }

  return bRunning;
}

bool XBPython::isStopping(int scriptId)
{
  bool bStopping = false;
  
  CSingleLock lock(m_critSection);
  PyList::iterator it = m_vecPyList.begin();
  while (it != m_vecPyList.end())
  {
    if (it->id == scriptId) bStopping = it->pyThread->isStopping();
    ++it;
  }
  
  return bStopping;
}

int XBPython::GetPythonScriptId(int scriptPosition)
{
  CSingleLock lock(m_critSection);
  return (int)m_vecPyList[scriptPosition].id;
}

void XBPython::PulseGlobalEvent()
{
  SetEvent(m_globalEvent);
}

void XBPython::WaitForEvent(HANDLE hEvent, unsigned int timeout)
{
  // wait for either this event our our global event
  HANDLE handles[2] = { hEvent, m_globalEvent };
  WaitForMultipleObjects(2, handles, FALSE, timeout);
  ResetEvent(m_globalEvent);
}

// BOXEE
PyThreadState* XBPython::CreateNewInterpreter()
{
  PyEval_AcquireLock();
  PyThreadState* contextState = Py_NewInterpreter();
  PyThreadState_Swap(contextState);
  InitializeInterpreter();
  PyThreadState_Swap(NULL);
  PyEval_ReleaseLock();
  
  return contextState;
}

XBPythonAppContext* XBPython::GetContext(const std::string& _contextId, const CStdString& partnerId = "")
{
  std::string contextId = _contextId;
  if (contextId == "") {
    contextId = "general";
  }
  
  XBPythonAppContext* appContext = NULL;
  
  // look up the context
  std::map<std::string, XBPythonAppContext*>::iterator it = m_contextMap.find(contextId);
  if (it == m_contextMap.end()) 
  {
    XBPythonAppContext* newContext = new XBPythonAppContext();
    if (contextId == "general") 
    {
      CLog::Log(LOGDEBUG,"XBPython::GetContext, no context for id = %s, creating new one (python)", contextId.c_str());
      m_nextid++;
      XBPyPersistentThread *pyThread = new XBPyPersistentThread(this, m_nextid);
      pyThread->Create();
      newContext->m_pyThread = pyThread;
    }
    else 
    {
      CLog::Log(LOGDEBUG,"XBPython::GetContext, no context for id = %s, creating new one (python)", contextId.c_str());
      m_nextid++;
      XBPyPersistentThread *pyThread = new XBPyPersistentThread(this, m_nextid);
      pyThread->SetAppId(contextId);
      pyThread->SetPartnerId(partnerId);
      pyThread->Create();
      newContext->m_pyThread = pyThread;
      
    }
    m_contextMap.insert(it, std::map<std::string, XBPythonAppContext*>::value_type(contextId, newContext));
    appContext = newContext;
  }
  else {
    CLog::Log(LOGDEBUG,"XBPython::GetContext, found context for id = %s (python)", contextId.c_str());
    appContext = it->second;
  }

  return appContext;
}

void XBPython::RemoveContext(const std::string& _contextId)
{
  std::string contextId = _contextId;
  if (contextId == "" || contextId == "general")
  {
    CLog::Log(LOGDEBUG,"XBPython::RemoveContext, unable to remove general context (python)");
    return;
  }

  // look up the context
  std::map<std::string, XBPythonAppContext*>::iterator it = m_contextMap.find(contextId);
  if (it == m_contextMap.end())
  {
    CLog::Log(LOGDEBUG,"XBPython::GetContext, unable to find context for id = %s (python)", contextId.c_str());
    return;
  }

  XBPythonAppContext* appContext = it->second;

  // Check when we can remove the context
  m_contextMap.erase(it);

  appContext->m_pyThread->StopThread();

  delete appContext->m_pyThread;
  delete appContext;
}


//int XBPython::evalStringInContext(const char *pythonCode, const std::string& path, const std::string& contextId , const CStdString& securityLevel, const unsigned int argc, const char ** argv)
int XBPython::evalStringInContext(const char *pythonCode, const std::string& path, const std::string& contextId , const CStdString& partnerId, const std::vector<CStdString>& params)
{
  CLog::Log(LOGDEBUG, "XBPython::evalStringInContext... contextId = %s (python)", contextId.c_str());
  
  Initialize();

  if (!m_bInitialized) {
    CLog::Log(LOGERROR, "XBPython::evalStringInContext, python not initialized (python)");
    return -1;
  }

  // Get thread state for context id, if context is not found a new one will be allocated
  XBPythonAppContext* context = GetContext(contextId, contextId);
  XBPyPersistentThread *pyThread = context->m_pyThread;

  if (!pyThread) {
    CLog::Log(LOGERROR, "XBPython::evalStringInContext, can not run script, no thread or context ");
    return -1;
  }
  
  pyThread->QueueScript(pythonCode, path, params);

  return m_nextid;
}

int XBPython::evalFileInContext(const char *src, const std::string& contextId, const CStdString& partnerId, const unsigned int argc, const char ** argv)
{
  CStdString srcStr = _P(src);
  CLog::Log(LOGDEBUG, "XBPython::evalFileInContext... context = %s, path = %s (python)", contextId.c_str(), srcStr.c_str());

  // return if file doesn't exist
  if (!XFILE::CFile::Exists(src))
    return -1;

   Initialize();

  if (!m_bInitialized) return -1;
  
  // Get thread state for context id, if context is not found a new one will be allocated
  XBPythonAppContext* context = GetContext(contextId, partnerId);
  XBPyPersistentThread *pyThread = context->m_pyThread;
  
  if (!pyThread) {
    CLog::Log(LOGERROR, "XBPython::evalFileInContext, can not run script, no thread or context ");
    return -1;
  }
  
  CLog::Log(LOGDEBUG, "XBPython::evalFileInContext, queue file: %s (python)", srcStr.c_str());
  pyThread->QueueFile(srcStr.c_str(), argc, argv);

  return m_nextid;
}

// execute script, returns -1 if script doesn't exist
int XBPython::evalString(const char *src, const unsigned int argc, const char ** argv)
{
  CLog::Log(LOGDEBUG, "XBPython::evalString (python)");
  CSingleLock lock(m_critSection);
  
  Initialize();

  if (!m_bInitialized) 
  {
    CLog::Log(LOGERROR, "XBPython::evalString, python not initialized (python)");
    return -1;
  }

  // Previous implementation would create a new thread for every script
  m_nextid++;
  XBPyThread *pyThread = new XBPyThread(this, m_nextid);
  if (argv != NULL)
    pyThread->setArgv(argc, argv);
  pyThread->evalString(src);
  
  PyElem inf;
  inf.id = m_nextid;
  inf.bDone = false;
  inf.strFile = "<string>";
  inf.pyThread = pyThread;

  m_vecPyList.push_back(inf);

  return m_nextid;
}

void XBPython::RemoveContextAll()
{
  CSingleLock lock(m_critSection);
  if (m_bInitialized)
  {
    std::map<std::string, XBPythonAppContext*>::iterator it = m_contextMap.begin();
    while (it != m_contextMap.end())
    {
      XBPythonAppContext* appContext = it->second;
      m_contextMap.erase(it);

      lock.Leave();
      
      appContext->m_pyThread->StopThread();
      
      lock.Enter();

      delete appContext->m_pyThread;
      delete appContext;
    
      it = m_contextMap.begin();
    }
  }
}

bool XBPython::HasPartnerThreadRunning(const CStdString& partnerId, const ThreadIdentifier& threadId)
{
  CSingleLock lock(m_critSection);
  bool bResult = false;

  if (m_bInitialized)
  {
    std::map<std::string, XBPythonAppContext*>::iterator it = m_contextMap.begin();
    while (it != m_contextMap.end())
    {
      XBPythonAppContext* appContext = it->second;
      XBPyPersistentThread* thread = appContext->m_pyThread;

      if(thread->GetCurrentThreadId() == threadId && thread->GetPartnerId() == partnerId)
      {
        bResult = true;
        break;
      }

      it++;
    }
  }

  return bResult;
}
// End BOXEE
