//
// C++ Implementation: bxbgprocess
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxbgprocess.h"
#include "logger.h"
#include <time.h>

namespace BOXEE {

int BXBGJob::sId = 1;

BXBGJob::BXBGJob(const std::string& strDescription, bool bCanDelete) : m_bCanDelete(bCanDelete)
{
	m_iId = sId++;
	m_bActive = true;
	m_pJobLock = SDL_CreateSemaphore(1);
	m_bCancelled = false;
	m_strDescription = strDescription;
}

BXBGJob::~BXBGJob()
{
	SDL_DestroySemaphore(m_pJobLock);
}

int BXBGJob::GetId() {
	return m_iId;
}

void BXBGJob::Pause() {
	//LOG(LOG_LEVEL_DEBUG,"PAUSE: Background job paused id = %d", GetId());
	if (!m_bActive) return; // already paused
	m_bActive = false;
	SDL_SemTryWait(m_pJobLock);
}

void BXBGJob::Resume() {
	//LOG(LOG_LEVEL_DEBUG,"PAUSE: Background job resumed id = %d", GetId());
	if (m_bActive) return; // already running
	m_bActive = true;
	SDL_SemPost(m_pJobLock);
}

void BXBGJob::Cancel()
{
  m_bCancelled = true;
}

void BXBGJob::Lock() 
{
	if (!m_bActive) 
  {
		//LOG(LOG_LEVEL_DEBUG,"PAUSE: Background job locked id = %d", GetId());
		SDL_SemWait(m_pJobLock);
	}
}

bool BXBGJob::IsActive() 
{
	return m_bActive;
}

bool BXBGJob::CanDelete()
{
  return m_bCanDelete;
}

void BXBGJob::ExecuteJob()
{
  if (m_bCancelled)
    return;

  //LOG(LOG_LEVEL_DEBUG,"BXBGJob::ExecuteJob, running job %s (bgjob)", m_strDescription.c_str());
  DoWork();
  
  PostDoWork();
}

BXBGProcess::BXBGProcess(int iCapacity) : m_bRunning(false), m_bPaused(false)
{
  m_pJobs = SDL_CreateSemaphore(0);
  m_pQueueLock = SDL_CreateMutex();
  m_pPauseLock = SDL_CreateMutex();
  m_pPauseCond = SDL_CreateCond();

  m_numOfJobsWithoutThread = 0;

  if (iCapacity > 0)
  {
    m_pCapacity = SDL_CreateSemaphore(iCapacity);
  }
  else
  {
    m_pCapacity = NULL;
  }

  if (!m_pJobs || !m_pQueueLock) {
    LOG(LOG_LEVEL_ERROR,"error creating sync objects for bg processing");
  }
}


BXBGProcess::~BXBGProcess()
{
   Stop();
  if (m_pJobs)
    SDL_DestroySemaphore(m_pJobs);

  if (m_pQueueLock)
    SDL_DestroyMutex(m_pQueueLock);

  if (m_pCapacity)
    SDL_DestroySemaphore(m_pCapacity);

  if (m_pPauseCond)
    SDL_DestroyCond(m_pPauseCond);

  if (m_pPauseLock)
    SDL_DestroyMutex(m_pPauseLock);
}

void BXBGProcess::SetName(std::string strName)
{
  m_strName = strName;
}

std::string BXBGProcess::GetName()
{
  return m_strName;
}

bool BXBGProcess::Lock() 
{
  if (!m_pQueueLock)
    return false;

  return (SDL_LockMutex(m_pQueueLock) == 0);
}

bool BXBGProcess::Unlock() 
{
  if (!m_pQueueLock)
    return false;

  return (SDL_UnlockMutex(m_pQueueLock) == 0);
}

bool BXBGProcess::Start(int nThreads, bool lazy) 
{
  if (!Lock())
    return false;

  if (m_bRunning) {
    LOG(LOG_LEVEL_WARNING,"already started");
    Unlock();
    return true;
  }

  m_bRunning = true;

  m_lazy = lazy;

  if(!m_lazy)
  {
    for (int nThread = 0; nThread < nThreads; nThread++)
      Create();

    LOG(LOG_LEVEL_DEBUG,"bg process [name=%s] initialized. [m_lazy=%d=FALSE] -> [%d] worker threads was created.",GetName().c_str(),m_lazy,(int)m_workingThreads.size());
  }
  else
  {
    m_maxNumOfWorkingThreads = nThreads;

    LOG(LOG_LEVEL_DEBUG,"bg process [name=%s] initialized. [m_lazy=%d=TRUE] -> MaxNumOfWorkingThreads was set to [%d]",GetName().c_str(),m_lazy,m_maxNumOfWorkingThreads);
  }

  Unlock();

  return true;
}

void BXBGProcess::RemoveWorkerThread(THREAD_ID_TYPE thread)
{
  Lock();

  std::vector< THREAD_HANDLE >::iterator iter = m_workingThreads.begin();
  while( iter != m_workingThreads.end() )
  {
    if( SAME_THREAD( *iter , thread ) )
  {
      m_workingThreads.erase(iter);
      break;
    }
    ++iter;
  }

  Unlock();
}

void BXBGProcess::SignalStop()
{
  m_bRunning = false;
}

void BXBGProcess::Stop(int nTimeout)
{
  m_bRunning = false;
  
  //printf("BXBGProcess, %s, asked to stop, this = %p. \n", m_strName.c_str(), this);

  // First resume all threads
  Resume();

  // This is done to allow all jobs that are waiting for the
  // capacity mutex to finish. Currently the number of iterations
  // was set to a hard coded large number
  // TODO: Count the actual number of tasks and update semaphore accordingly
  
  for (int i=0; m_pCapacity && i<10000; i++) 
  {
    SDL_SemPost(m_pCapacity);
  }
  
  Lock();

  for (unsigned int nThread=0; nThread<m_workingThreads.size(); nThread++) 
    SDL_SemPost(m_pJobs);

  for (unsigned int nItem=0; nItem < m_jobQueue.size(); nItem++) 
  {
    BXBGJob *pJob = m_jobQueue[nItem];
    if (pJob && pJob->CanDelete())
      delete pJob;
  }
  m_jobQueue.clear();
  
  Unlock();

  time_t start = time(NULL);
  
  do
  {
    Lock();
    size_t s = m_workingThreads.size();
    Unlock();

    if  (s == 0)
      break;
    
    if (nTimeout > 0 && time(NULL) - start >= nTimeout)
      break;

    SDL_Delay(500);
  } while(1);

  Lock();
  m_workingThreads.clear();
  Unlock();

}

void BXBGProcess::Pause()
{
  if (m_bPaused) return;

  Lock();
  std::map<int, BXBGJob*>::iterator it = m_inProgressMap.begin();
  for (;it != m_inProgressMap.end(); it++) {
	  BXBGJob *pJob = it->second;
	  if (pJob) {
		  pJob->Pause();
	  }
  }
  Unlock();

  // Lock the pause mutex
  SDL_LockMutex(m_pPauseLock);
  // Update the pause protected variable
  m_bPaused = true;
  // Unlock the mutex back
  SDL_UnlockMutex(m_pPauseLock);
}

void BXBGProcess::Resume()
{
  if (!m_bPaused) return;

  Lock();
  std::map<int, BXBGJob*>::iterator it = m_inProgressMap.begin();
  for (;it != m_inProgressMap.end(); it++) {
	  BXBGJob *pJob = it->second;
	  if (pJob) {
		  pJob->Resume();
	  }
  }
  Unlock();

  // Lock the pause mutex
  SDL_LockMutex(m_pPauseLock);
  // Update the pause protected variable
  m_bPaused = false;
  // Signal the condition variable so that the threads would resume working
  SDL_CondBroadcast(m_pPauseCond);
  // Unlock the mutex back
  SDL_UnlockMutex(m_pPauseLock);
}


bool BXBGProcess::QueueJob(BXBGJob *pJob)
{
  if (!m_bRunning) return false;

  if (m_pCapacity && SDL_SemWait(m_pCapacity) != 0)
      return false;

  if (!m_pJobs || !m_bRunning || !Lock())
    return false;

  bool bFoundSimilarJob = false;
  // lookup for a similar job in m_jobQueue
  for (std::deque<BXBGJob *>::iterator it = m_jobQueue.begin(); it != m_jobQueue.end() && (!bFoundSimilarJob) ; it++)
  {
    if ((*it)->Equals(*pJob))
    {
      (*it)->Cancel(); //cancel the old job, it won't do anything but delete itself
      m_jobQueue.insert(it,pJob); //insert the new job right after the one that we canceled..
      bFoundSimilarJob = true; //don't continue lookup
      LOG(LOG_LEVEL_WARNING,"processor %s found similar job (old_id=[%d], new id=[%s]), canceled the old one and added the new one right after it.",(*it)->GetId(),pJob->GetId());
    }
  }

  // if didn't find any other similar job, add the new job and continue as usual
  if (!bFoundSimilarJob)
  {
    m_jobQueue.push_back(pJob);

    if(m_lazy)
    {
      // case of lazy BXBGProcess -> create a thread for handling BXBGJob
      CreateExecuteThread();
    }
  }

  Unlock();

  return (SDL_SemPost(m_pJobs) == 0);
}

bool BXBGProcess::QueueJobs(std::vector<BXBGJob *> vecJobs) 
{  
  for (size_t i=0; m_pCapacity && m_bRunning && i<vecJobs.size(); i++)
    SDL_SemWait(m_pCapacity);
  
  if (!m_pJobs || !m_bRunning || !Lock())
    return false;
 
  m_jobQueue.insert(m_jobQueue.begin(), vecJobs.begin(), vecJobs.end());
  
  // Increase job counter
  bool bResult = true;
  for (size_t i = 0; i < vecJobs.size(); i++)
  {
    bResult &= (SDL_SemPost(m_pJobs) == 0);
    if(m_lazy)
      CreateExecuteThread();
  }
  
  Unlock();
  
  return bResult;
}
  
bool BXBGProcess::QueueFront(BXBGJob *pJob)
{
  if (!m_bRunning) return false;

  if (m_pCapacity && SDL_SemWait(m_pCapacity) != 0)
      return false;

  if (!m_pJobs || !m_bRunning || !Lock())
    return false;

  m_jobQueue.push_front(pJob);

  if(m_lazy)
  {
    // case of lazy BXBGProcess -> create a thread for handling BXBGJob
    CreateExecuteThread();
  }

  Unlock();

  return (SDL_SemPost(m_pJobs) == 0);
}

int BXBGProcess::GetNumJobs()
{
  return m_jobQueue.size();
}

bool BXBGProcess::IsLazy()
{
  return m_lazy;
}

int BXBGProcess::GetNumOfJobsWithoutThread()
{
  return m_numOfJobsWithoutThread;
}

void BXBGProcess::DecreaseNumOfJobsWithoutThread(int decreaseBy)
{
  m_numOfJobsWithoutThread -= decreaseBy;
}

int BXBGProcess::GetNumOfWorkingThread()
{
  return m_workingThreads.size();
}

void BXBGProcess::ClearQueue()
{
  if (!m_pJobs || !m_bRunning || !Lock())
      return;

  for (size_t i = 0; i < m_jobQueue.size(); i ++)
  {
    BXBGJob *pJob = m_jobQueue.front();
    pJob->Cancel();
    m_jobQueue.pop_front();
    m_jobQueue.push_back(pJob);
  }

  Unlock();
}

void* BXBGProcess::WorkerPThread( void* pParam )
{
  WorkerThread( pParam );
  return (void*)0;
}


int BXBGProcess::WorkerThread(void *pParam)
{
  if (!pParam)
    return 1;

  BXBGProcess *pCaller = (BXBGProcess *)pParam;

  // Get thread id for debugging
  //int iThreadId = SDL_ThreadID();

  // Initially lock the pause mutex
  bool bLazyDecrease = true;
  SDL_LockMutex(pCaller->m_pPauseLock);

  while (pCaller->m_bRunning)
  {
    if (pCaller->m_bPaused == false)
    {
      // Thread is not pause, perform work
      SDL_UnlockMutex(pCaller->m_pPauseLock);

      if (SDL_SemWait(pCaller->m_pJobs) == 0)
      {
        if (!pCaller->m_bPaused && pCaller->m_bRunning && pCaller->Lock())
        {
          BXBGJob *pJob = NULL;
          
          if (pCaller->m_jobQueue.size() > 0)
          {
            pJob = pCaller->m_jobQueue.front();
            pCaller->m_jobQueue.pop_front();
          }
          
          if (pJob)
          {
            // Add the job to the map of jobs in progress
            pCaller->m_inProgressMap[pJob->GetId()] = pJob;
          }
          
          pCaller->Unlock();

          if (pJob)
          {
            pJob->ExecuteJob();
		        pCaller->Lock();
            pCaller->m_inProgressMap.erase(pJob->GetId());
      			pCaller->Unlock();
            if (pCaller->m_pCapacity)
            SDL_SemPost(pCaller->m_pCapacity);
            if (pJob->CanDelete())
            {
              delete pJob;
            }
          }
        }
        else
        {
          //operation was paused, add the count we took because of SemWait
          SDL_SemPost(pCaller->m_pJobs);

          if(pCaller->IsLazy())
          {
            bLazyDecrease = false;
          }
        }
      }

      SDL_LockMutex(pCaller->m_pPauseLock);
    }
    else
    {
      // We are in the paused state, wait for resume
      if (pCaller->m_bRunning)
      {
        SDL_CondWait(pCaller->m_pPauseCond, pCaller->m_pPauseLock);
      }
    }

    if(pCaller->IsLazy())
    {
      pCaller->Lock();

      bool shouldBreak = false;
      int numOfJobsWithoutThread = pCaller->GetNumOfJobsWithoutThread();

      if(numOfJobsWithoutThread < 1)
      {
        shouldBreak = true;
        pCaller->RemoveWorkerThread(MY_THREAD_ID);
      }
      else
      {
        if (!bLazyDecrease)
        {
          pCaller->DecreaseNumOfJobsWithoutThread(1);
        }
      }

      bLazyDecrease = true;

      pCaller->Unlock();

      if(shouldBreak)
      {
        break;
      }
    }
  } // while

  SDL_UnlockMutex(pCaller->m_pPauseLock);

  pCaller->RemoveWorkerThread(MY_THREAD_ID);

  return 0;
}

THREAD_HANDLE BXBGProcess::Create()
{
  THREAD_HANDLE thread = NULL;
#ifdef _LINUX
  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&attr, 1024*1024);
  if( pthread_create( &thread, &attr, WorkerPThread, this ) == 0 )
  {
    m_workingThreads.push_back( thread );
  }
#else
  thread = SDL_CreateThread( WorkerThread, this );
  if( thread )
    m_workingThreads.push_back( thread );
#endif
  
  return thread;
}

void BXBGProcess::CreateExecuteThread()
{
  ///////////////////////////////////////////////////////////////////////
  // NOTE: call to this function assume that m_pQueueLock was acquired //
  ///////////////////////////////////////////////////////////////////////

  if((int)m_workingThreads.size() < m_maxNumOfWorkingThreads)
  {
    // case we can create new thread
    Create();

    // In case the processor is paused the m_numOfJobsWithoutThread should be incremented so that the job will be handled after resume
    if (m_bPaused)
    {
      m_numOfJobsWithoutThread++;
    }
  }
  else
  {
    // there are already MaxNumOfThreads -> no need to create new thread
    m_numOfJobsWithoutThread++;
  }
}

bool BXBGProcess::IsRunning()
{
  return m_bRunning;
}

}
