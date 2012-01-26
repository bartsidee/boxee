// Copyright © 2008 BOXEE. All rights reserved.
//
// C++ Interface: bxbgprocess
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#ifndef BOXEEBXBGPROCESS_H
#define BOXEEBXBGPROCESS_H

#include <SDL/SDL.h>
#include <vector>
#include <deque>
#include <map>
#include <string>

#ifdef _LINUX
  #include <pthread.h>
#endif

#ifdef _LINUX
#define MY_THREAD_ID  pthread_self()
#define THREAD_HANDLE pthread_t
#define THREAD_ID_TYPE pthread_t
#define SAME_THREAD(x,y) pthread_equal((x),(y))
#else
#define MY_THREAD_ID SDL_ThreadID()
#define THREAD_HANDLE SDL_Thread*
#define THREAD_ID_TYPE Uint32
#define SAME_THREAD(x,y) (SDL_GetThreadID(x) == (y))
#endif

namespace BOXEE
{

class BXBGJob
{
public:
  BXBGJob(const std::string& strDescription, bool bCanDelete = true);
  virtual ~BXBGJob();
  
  static int sId;
  
  void ExecuteJob();
  virtual void DoWork() = 0;
  virtual void PostDoWork(){};
  
  virtual bool CanDelete();
  void Pause();
  void Resume();
  void Cancel();
  void Lock();
  bool IsActive();
  int GetId();
  virtual bool Equals (const BXBGJob& other) const { return false; };

private:
  
  bool m_bActive;
  bool m_bCancelled;
  int m_iId;
  bool m_bCanDelete;
  std::string m_strDescription;
  SDL_sem	*m_pJobLock;
};

class BXDummyJob: public BXBGJob
{
public:
  virtual void DoWork() { }
};

/**
*/
class BXBGProcess
{
public:
  BXBGProcess(int iCapacity = -1);
  virtual ~BXBGProcess();
  
  bool Start(int nThreads, bool lazy = true);
  void Stop(int nTimeout=-1);
  void SignalStop();
  void Pause();
  void Resume();
  bool QueueJob(BXBGJob *pJob);
  bool QueueJobs(std::vector<BXBGJob*> vecJobs);
  bool QueueFront(BXBGJob *pJob);
  void ClearQueue();
  
  void SetName(std::string strName);
  std::string GetName();
  
  // Returns the number of jobs that are currently in the queue
  int GetNumJobs();
  int GetQueueSize() { return GetNumJobs(); }

  bool IsLazy();

  int GetNumOfJobsWithoutThread();
  void DecreaseNumOfJobsWithoutThread(int decreaseBy);
  int GetNumOfWorkingThread();

  bool IsRunning(); 
  
protected:
  THREAD_HANDLE Create();
  
  bool Lock();
  bool Unlock();
  
  static void* WorkerPThread( void *pParam );
  static int WorkerThread(void *pParam);
  
  void RemoveWorkerThread(THREAD_ID_TYPE thread);

  void CreateExecuteThread();

  std::string m_strName;
  
  bool m_bRunning;
  
  bool    m_bPaused;
  SDL_mutex  *m_pPauseLock;
  SDL_cond   *m_pPauseCond;
  
  SDL_sem 	*m_pJobs;
  SDL_sem   *m_pCapacity;
  SDL_mutex	*m_pQueueLock;
  
  std::vector< THREAD_HANDLE > m_workingThreads;
  std::deque<BXBGJob *> m_jobQueue;
  std::map<int, BXBGJob *> m_inProgressMap;

  bool m_lazy;
  int m_maxNumOfWorkingThreads;
  int m_numOfJobsWithoutThread;
};

}

#endif
