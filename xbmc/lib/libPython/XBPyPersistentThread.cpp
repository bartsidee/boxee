#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif
#if (defined USE_EXTERNAL_PYTHON)
  #if (defined HAVE_LIBPYTHON2_6)
    #include <python2.6/Python.h>
    #include <python2.6/osdefs.h>
  #elif (defined HAVE_LIBPYTHON2_5)
    #include <python2.5/Python.h>
    #include <python2.5/osdefs.h>
  #elif (defined HAVE_LIBPYTHON2_4)
    #include <python2.4/Python.h>
    #include <python2.4/osdefs.h>
  #else
    #error "Could not determine version of Python to use."
  #endif
#else
#include "Python/Include/Python.h"
#include "Python/Include/osdefs.h"
#include "Python/Include/pystate.h"
#endif

#include "Util.h"
#include "xbmcmodule/pyutil.h"
#include "XBPyPersistentThread.h"
#include "XBPython.h"
#include "FileSystem/SpecialProtocol.h"

#include "utils/log.h"
#include "lib/libBoxee/boxee.h"

extern "C"
{
  int xbp_chdir(const char *dirname);
  char* dll_getenv(const char* szKey);
}

#ifdef _WIN32
extern "C" FILE *fopen_utf8(const char *_Filename, const char *_Mode);
#else
#define fopen_utf8 fopen
#endif

#define PY_PATH_SEP DELIM

XBPyPersistentThread::XBPyPersistentThread(XBPython *pExecuter, int id) : XBPyThread(pExecuter, id) 
{
  m_pJobs = SDL_CreateSemaphore(0);
  m_pQueueLock = SDL_CreateMutex();
  
  main_module = NULL;
  global_dict = NULL;
  m_saveState = NULL;
  m_enableSandbox = false;
  
}

XBPyPersistentThread::~XBPyPersistentThread()
{
  if (m_pJobs)
    SDL_DestroySemaphore(m_pJobs);

  if (m_pQueueLock)
    SDL_DestroyMutex(m_pQueueLock);
}

void XBPyPersistentThread::SetAppId(const CStdString &id)
{
  m_strAppId = id;
}

void XBPyPersistentThread::OnStartup()
{
  XBPyThread::OnStartup();

  // get the global lock
  PyEval_AcquireLock();

  m_threadState = Py_NewInterpreter();
  
  m_InterpState = m_threadState->interp;
  PyThreadState_Swap(m_threadState);
  
  m_pExecuter->InitializeInterpreter();
  
  // Get and save the main module dictionary
  char *m = strdup("__main__");
  main_module = PyImport_AddModule(m);
  global_dict = PyModule_GetDict(main_module);
  m_saveState = PyEval_SaveThread();
  free(m);

  m_strPythonPath = GetPythonPath();

#ifdef HAS_EMBEDDED
  CStdString enableSandbox = getenv("PYTHON_SANDBOX");
  if(!enableSandbox.empty())
  {
    m_enableSandbox = (enableSandbox.compare("1") == 0);
  }
#endif

  if(m_enableSandbox)
  {
    CLog::Log(LOGINFO, "XBPyPersistentThread::OnStartup: turning on Python Sandbox");
  }
}

bool XBPyPersistentThread::Lock()
{
  if (!m_pQueueLock)
    return false;

  return (SDL_LockMutex(m_pQueueLock) == 0);
}

bool XBPyPersistentThread::Unlock()
{
  if (!m_pQueueLock)
    return false;

  return (SDL_UnlockMutex(m_pQueueLock) == 0);
}

void XBPyPersistentThread::OnExit()
{
  XBPyThread::OnExit();
  m_done = true;
  m_pExecuter->setDone(m_id);
}

bool XBPyPersistentThread::QueueScript(const std::string& pythonCode, const std::string& path, const std::vector<CStdString>& params)
{
  if (!m_pJobs || !IsRunning() || !Lock())
    return false;

  XBPythonJob job;
  job.strSource = pythonCode;
  job.strPath = path;
  job.isFile = false;
  
  for (std::vector<CStdString>::const_iterator cit = params.begin() ; cit != params.end() ; cit++)
  {
    CLog::Log(LOGERROR, "XBPyPersistentThread::QueueScript, params");
    job.argv.push_back(*cit);
  }
  
  m_scriptQueue.push_back(job);

  Unlock();
	
  return (SDL_SemPost(m_pJobs) == 0);
}

bool XBPyPersistentThread::QueueFile(const std::string& file, int argc, const char** argv)
{
  if (!m_pJobs || !IsRunning() || !Lock())
    return false;

  XBPythonJob job;
  job.strSource = file;
  job.isFile = true;

  for (int i = 0; i < argc; i++)
  {
    job.argv.push_back(argv[i]); 
  }
    
  m_scriptQueue.push_back(job);

  Unlock();
	
  return (SDL_SemPost(m_pJobs) == 0);
}

std::string XBPyPersistentThread::GetPythonPath(const std::string& strSourceDir)
{
  std::string strPath;
  
  strPath = strSourceDir;
  
#ifndef _LINUX
  strPath += dll_getenv("PYTHONPATH");
#else
  strPath += PTH_IC("special://xbmc/system/python/lib");
  strPath += PY_PATH_SEP;
  strPath += PTH_IC("special://xbmc/system/python/local");
  strPath += PY_PATH_SEP;
#endif
  
  CLog::Log(LOGDEBUG,"%s - using python path <%s>", __FUNCTION__, strPath.c_str());
  
  return strPath; 
  
}

void XBPyPersistentThread::StopThread(bool mWait)
{
  CLog::Log(LOGERROR, "XBPyPersistentThread::StopThread, stop thread, mWait = %s (python)", mWait ? "true" : "false");
  m_bStop = true;
  SDL_SemPost(m_pJobs);
  XBPyThread::StopThread(mWait);
}

void XBPyPersistentThread::SignalStop()
{
  CLog::Log(LOGINFO, "XBPyPersistentThread::SignalStop, stop thread (python)");
  m_bStop = true;
}

void XBPyPersistentThread::Process()
{
  // Loop while thread is active
  while (IsRunning())
  {
    CLog::Log(LOGINFO, "XBPyPersistentThread::Process, iteration started (python)");
    // Lock the semaphore if possible, sleep on it otherwise
    if (SDL_SemWait(m_pJobs) == 0)
    {
      CLog::Log(LOGINFO, "XBPyPersistentThread::Process, running job (python)");
      if (!IsRunning())
      {
        CLog::Log(LOGINFO, "XBPyPersistentThread::Process, not running, return (python)");
        break;
      }
      
#if defined(_LINUX) && !defined(__APPLE__)
      m_currentThreadId = gettid();
      CreatePolicy(m_currentThreadId);
#else
      m_currentThreadId = CThread::GetCurrentThreadId();
      CreatePolicy(m_currentThreadId);
#endif
      XBPythonJob job;
      
      Lock();
      job = m_scriptQueue.front();
      m_scriptQueue.pop_front();
      Unlock();
      
      // Copy source into a temporary variable
      char * _source = new char[job.strSource.length()+1];
      strcpy(_source, job.strSource.c_str());
      
      char sourcedir[1024];
      sourcedir[0] = '\0';
      
      // get path from script file name and add python path's
      // this is used for python so it will search modules from script path first
      if (job.isFile)
      {
        strcpy(sourcedir, _source);
        
        char *p = strrchr(sourcedir, PATH_SEPARATOR_CHAR);
        *p = PY_PATH_SEP;
        *++p = 0;
      }
      else
      {
        if (job.strPath.length() > 0)
        {
          sprintf(sourcedir, "%s%c", job.strPath.c_str(), PY_PATH_SEP);
        }
      }
      
      std::string strSourceDir = sourcedir;
      std::string strPath = strSourceDir;
      strPath += m_strPythonPath;

#ifdef EXTERNAL_PYTHON_PREFIX
      CStdString strPath2;
      strPath2.Format("%s:%s:%s/plat-linux2:%s/lib-dynload:%s/site-packages:", EXTERNAL_PYTHON_ZIP,
          EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX, EXTERNAL_PYTHON_PREFIX);
      strPath += strPath2;
      strPath += PTH_IC("special://xbmc/system/python/local:");
      setenv("PYTHONPATH", strPath.c_str(), 1);
#endif

#ifdef EXTERNAL_PYTHON_HOME
      setenv("PYTHONHOME", "/opt/local", 1);
#endif

      // Copy the path to char* so it can be used with Python functions
      char path[1024];
      strcpy(path, strPath.c_str());
      
      int argc = job.argv.size();
      char** argv = new char*[argc];
      for (int i = 0; i < argc; i++)
      {
        CLog::Log(LOGDEBUG, "XBPyPersistentThread::Process, parameter (%d): %s (python)", i, job.argv[i].c_str());
        argv[i] = new char[(job.argv[i].size())+1];
        memset(argv[i], 0, job.argv[i].size()+1);
        strncpy(argv[i], job.argv[i].c_str(), job.argv[i].size());
      }
      
      PyEval_RestoreThread(m_saveState); 
      
      // set current directory and python's path.
      if (argc > 0 && argv != NULL)
      {
        PySys_SetArgv(argc, argv);
      }
      PySys_SetPath(path);
      // Remove the PY_PATH_SEP at the end
      sourcedir[strlen(sourcedir)-1] = 0;
      xbp_chdir(sourcedir);
      
      // set the app-id so we can retrieve it later
      if (!m_strAppId.IsEmpty())
      {
        PyObject* id = PyString_FromString(m_strAppId.c_str());
        if(id)
        {
          PySys_SetObject(const_cast<char*>("app-id"), id );
          Py_XDECREF(id);
        }
      }
      
      if (job.isFile)
      {
        // run script from file
        FILE *fp = fopen_utf8(_source, "r");
        if (fp)
        {
          try 
          {
            CLog::Log(LOGINFO, "XBPyPersistentThread::Process, run file, path = %s (python)", _source);
            if (PyRun_SimpleFile(fp, _source) == -1)
            {
              CLog::Log(LOGERROR, "Scriptresult: Error\n");
              if (PyErr_Occurred())
              {
                PyErr_Print();
                PyErr_Clear();
            }
            }
            else
              CLog::Log(LOGINFO, "Scriptresult: Succes\n");
          }
          catch (...)
          {
            CLog::Log(LOGERROR,"exception when running python script from <%s>", _source);
          }
          
          fclose(fp);
        }
        else
          CLog::Log(LOGERROR, "%s not found!\n", m_source);
      }
      else
      {
        //run script
        CLog::Log(LOGINFO, "XBPyPersistentThread::Process, run script, (python)");
        if (PyRun_String(_source, Py_file_input, global_dict, global_dict) == NULL)
        {
          CLog::Log(LOGERROR, "Scriptresult: Error\n");
          if (PyErr_Occurred())
          {
            PyErr_Print();
            PyErr_Clear();
        }
        }
        else
          CLog::Log(LOGINFO, "Scriptresult: Success\n");
      }
      
      m_saveState = PyEval_SaveThread(); 
      
      for (int i = 0; i < argc; i++)
      {
        delete [] argv[i];
      }
      
      delete [] argv;
      
      delete _source;
#if defined(_LINUX) && !defined(__APPLE__)
      DeletePolicy(gettid());
#else
      DeletePolicy(CThread::GetCurrentThreadId());
#endif
      if (!IsRunning())
      {
        CLog::Log(LOGINFO, "XBPyPersistentThread::Process, not running, return (python)");
        break;
      }

    } // if there are scripts in the queue
    
  } // while thread is running 
  
  if (m_threadState != NULL)
  {
  PyEval_RestoreThread(m_saveState);   
    PyThreadState_Swap(m_threadState);
  
    //
    // if threads are enabled we must kill the threads before destroying the interpreter 
    //
    if (PyEval_ThreadsInitialized())
    {
        //
        // Loop through all threads in the current interpreter and raise SystemExit
        //
        PyThreadState *curstate = m_InterpState->tstate_head;
        while (curstate != NULL && curstate->interp == m_InterpState)
        {
            PyThreadState_Swap(curstate);

            PyThreadState_SetAsyncExc(curstate->thread_id, PyExc_SystemExit);
            curstate = curstate->next;

            PyThreadState_Swap(NULL);
        }

        //m_InterpState->tstate_head = m_threadState;
    }

    PyThreadState_Swap(m_threadState);

    m_pExecuter->DeInitializeInterpreter();

    if (m_strAppId != "lastfm")
    {
      //
      // FIXME - Py_EndInterpreter() sometimes crashes/hangs
      // Its well known problem in Python interpreter
      // http://bugs.python.org/issue4236
      //

      //Py_EndInterpreter(m_threadState);
    }

    PyThreadState_Swap(NULL);
    PyEval_ReleaseLock();

    m_threadState = NULL;
  }
}

void XBPyPersistentThread::CreatePolicy(const ThreadIdentifier threadId)
{
  if (m_enableSandbox && PyEval_ThreadsInitialized())
  {

    CLog::Log(LOGDEBUG, "XBPyPersistentThread::CreatePolicy -thread id %p", threadId);
    // 
    // File System policy - no access to all except:
    //
    // r/w
    // userdata/apps/app_id
    // userdata/profiles/usename/apps/app_id
    // 
    // r/o
    // /opt/boxee/system/python
    // /opt/local/lib/python2.4
    //
    ThreadPolicy *fsPolicy = TPAllocPolicy(threadId, FILE_SYSTEM);
    FileRule rule;

    rule.fileName = _P("special://home/profiles/");
    rule.fileName += BOXEE::Boxee::GetInstance().GetCredentials().GetUserName();
    rule.fileName += "/apps/";
    rule.fileName += m_strAppId;
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = _P("special://home/apps/");
    rule.fileName += m_strAppId;
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "/tmp/boxee/";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "http://";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "https://";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "mms://";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "rss://";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

#ifdef HAS_EMBEDDED
    rule.fileName = "/opt/boxee";
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "/opt/local/lib/python2.4";
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "mc.py";
    rule.accessMode = FILE_ACCESS_READ;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "/etc/hosts";
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "/dev/urandom";
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = _P(g_settings.GetProfileUserDataFolder());
    rule.fileName += "/cache";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = _P(g_settings.GetProfileUserDataFolder());
    rule.fileName += "/playbackhistory.dmp";
    rule.accessMode = FILE_ACCESS_READ | FILE_ACCESS_WRITE;
    
    TPAddRuleToPolicy(threadId, fsPolicy, &rule);

    rule.fileName = "/opt/local/etc/mdns.allow";
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, fsPolicy, &rule);
#endif

    TPAddPolicy(threadId, fsPolicy);
    
    // 
    // Shared library policy
    // 
    ThreadPolicy *slPolicy = TPAllocPolicy(threadId,SHARED_LIBRARY);

    rule.fileName = _P("special://xbmc/system/python");
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, slPolicy, &rule);

    rule.fileName = _P("special://home/apps/");
    rule.fileName += m_strAppId;
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, slPolicy, &rule);

#ifdef HAS_EMBEDDED
    rule.fileName = "/opt/local/lib/python2.4";
    rule.accessMode = FILE_ACCESS_READ;
   	
    TPAddRuleToPolicy(threadId, slPolicy, &rule);
#endif

    TPAddPolicy(threadId, slPolicy);

    //
    // Disallow policy
    //
    ThreadPolicy *pePolicy = TPAllocPolicy(threadId, DISALLOW);

    TPAddPolicy(threadId, pePolicy);

    }
}

void XBPyPersistentThread::DeletePolicy(const ThreadIdentifier threadId)
{
  if(m_enableSandbox)
  {
    CLog::Log(LOGDEBUG, "XBPyPersistentThread::DeletePolicy -thread id %p", threadId);
    TPDeletePolicy(threadId, NULL);
  }
}
   
void XBPyPersistentThread::SetPartnerId(const CStdString& partnerId)
{
  m_partnerId = partnerId;
}

ThreadIdentifier XBPyPersistentThread::GetCurrentThreadId()
{
  return m_currentThreadId;
}

CStdString XBPyPersistentThread::GetPartnerId()
{
  return m_partnerId;
}
