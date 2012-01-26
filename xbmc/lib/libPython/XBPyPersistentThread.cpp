
#include "Python/Include/Python.h"
#include "Python/Include/osdefs.h"
#include "Python/Include/pystate.h"
#ifdef _LINUX
#include "XBPythonDll.h"
#endif
#include "Util.h"
#include "xbmcmodule/pyutil.h"
#include "XBPyPersistentThread.h"
#include "XBPython.h"
#include "FileSystem/SpecialProtocol.h"

#include "utils/log.h"

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

  //CreatePolicy(CThread::GetCurrentThreadId());
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

  //DeletePolicy(CThread::GetCurrentThreadId());
}

bool XBPyPersistentThread::QueueScript(const std::string& pythonCode,
    const std::string& path, int argc, const char** argv)
{
  if (!m_pJobs || !IsRunning() || !Lock())
    return false;

  XBPythonJob job;
  job.strSource = pythonCode;
  job.strPath = path;
  job.isFile = false;
  
  for (int i = 0; i < argc; i++)
  {
    job.argv.push_back(argv[i]); 
  }
  
  m_scriptQueue.push_back(job);

  Unlock();
	
  return (SDL_SemPost(m_pJobs) == 0);
}

bool XBPyPersistentThread::QueueFile(const std::string& file, int argc,
    const char** argv)
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
      
      // Copy the path to char* so it can be used with Python functions
      char path[1024];
      strcpy(path, strPath.c_str());
      
      int argc = job.argv.size();
      char** argv = new char*[argc];
      for (int i = 0; i < argc; i++)
      {
        //argv[i] = (char*) job.argv[i].c_str();
        argv[i] = new char[strlen(job.argv[i].c_str())+1];
        strcpy(argv[i], job.argv[i].c_str());
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

        m_InterpState->tstate_head = m_threadState;
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
  if (PyEval_ThreadsInitialized())
  {

    // 
    // File System policy - read only 
    // 
    ThreadPolicy *fsPolicy = TPAllocPolicy(threadId, FILE_SYSTEM);

    TPAddPolicy(threadId, fsPolicy);
  
    // 
    // Shared library policy
    // 
    ThreadPolicy *slPolicy = TPAllocPolicy(threadId,SHARED_LIBRARY);
    FileRule rule;

    rule.fileName = _P("special://xbmc/system/python");
    rule.accessMode = FILE_ACCESS_READ;

    TPAddRuleToPolicy(threadId, slPolicy, &rule);

    TPAddPolicy(threadId, slPolicy);

    //
    // ProcessExec policy
    //
    ThreadPolicy *pePolicy = TPAllocPolicy(threadId,PROCESS_EXEC);

    TPAddPolicy(threadId, pePolicy);

  }
}

void XBPyPersistentThread::DeletePolicy(const ThreadIdentifier threadId)
{
  TPDeletePolicy(threadId, NULL);
}

void XBPyPersistentThread::SetSecurityLevel(const CStdString& securityLevel)
{
  m_securityLevel = securityLevel;
}