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




#ifndef XBPERSISTENTTHREAD_H_
#define XBPERSISTENTTHREAD_H_

#include "XBPyThread.h"
#include "utils/Thread.h"
#include <deque>
#include <SDL/SDL.h>
#include <vector>
#include "ThreadPolicy.h"

typedef std::list<ThreadPolicy*> ThreadPolicyList;

class XBPython;

class XBPythonJob {
public:
  // specifies the whether the source is a filename or the script itself
  bool isFile;
  // Path of the file or script source, depending on the job type
  std::string strSource;
  std::string strPath;
  std::vector< std::string > argv;
};

class XBPyPersistentThread : public XBPyThread {
public:
  XBPyPersistentThread(XBPython *pExecuter, int id);
  virtual ~XBPyPersistentThread();
  
  virtual bool QueueScript(const std::string& pythonCode, const std::string& path, const std::vector<CStdString>& params);
  virtual bool QueueFile(const std::string& file, int argc, const char** argv);
  
  virtual void OnStartup();
  virtual void Process();
  virtual void OnExit();
  virtual void OnException() {}
  
  virtual void StopThread(bool mWait=true);
  virtual void SignalStop();
  
  void SetAppId(const CStdString &id);
  
  void CreatePolicy(const ThreadIdentifier threadId);
  void DeletePolicy(const ThreadIdentifier threadId);
  void SetPartnerId(const CStdString& partnerId);
  CStdString GetPartnerId();
  ThreadIdentifier GetCurrentThreadId();
  
protected:
  
  bool Lock();
  bool Unlock();
  
  std::string GetPythonPath(const std::string& strSourceDir = "");
  
  // script queue that should be executed by the thread
  std::deque<XBPythonJob> m_scriptQueue;
  
  CStdString m_strAppId;
  CStdString m_strPythonPath;
  // TODO: Move all synchronization to non SDL mechanisms
  
  SDL_sem   *m_pJobs;
  SDL_mutex *m_pQueueLock;
  
  // Python environment
  PyObject* main_module;
  PyObject* global_dict;
  
  PyThreadState *m_saveState;
  CStdString m_partnerId;
  ThreadIdentifier m_currentThreadId;

  bool m_enableSandbox;
};

#endif /* XBPERSISTENTTHREAD_H_ */
