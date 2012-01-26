//
// C++ Implementation: bxbgprocess
//
// Description:
//
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "bxscheduletaskmanager.h"

#include "boxee.h"
#include "logger.h"
#include <time.h>

#include "utils/TimeUtils.h"

#ifdef _LINUX
#include "linux/XTimeUtils.h"
#endif

namespace BOXEE
{

//////////////////////////////
// CBoxeeScheduleTask Class //
//////////////////////////////

BoxeeScheduleTask::BoxeeScheduleTask(const std::string& name,unsigned long executionDelayInMS,bool repeat) : BXBGJob(name, !repeat)
{
  m_executionDelayInMS = executionDelayInMS;
  m_executionTimeInMS = CTimeUtils::GetTimeMS() + executionDelayInMS;
  m_repeatTaskIntervalInMS = executionDelayInMS;
  m_repeat = repeat;
  m_name = name;
  m_stmHandler = NULL;
}

BoxeeScheduleTask::BoxeeScheduleTask(const std::string& name,unsigned long firstExecutionDelayInMS,unsigned long repeatTaskIntervalInMS) : BXBGJob(name, (repeatTaskIntervalInMS == 0)?true:false)
{
  m_executionDelayInMS = firstExecutionDelayInMS;
  unsigned int currTime = CTimeUtils::GetTimeMS();
  m_executionTimeInMS = currTime + firstExecutionDelayInMS;
  m_repeatTaskIntervalInMS = repeatTaskIntervalInMS;
  m_repeat = (m_repeatTaskIntervalInMS > 0) ? true : false;
  m_name = name;
  m_stmHandler = NULL;
}

BoxeeScheduleTask::~BoxeeScheduleTask()
{

}

void BoxeeScheduleTask::PostDoWork()
{
  //LOG(LOG_LEVEL_DEBUG,"BoxeeScheduleTask::PostDoWork - Enter function. [name=%s][jobId=%d][repeat=%d][currentTime=%lu] (bstm)",m_name.c_str(),GetId(),m_repeat,CTimeUtils::GetTimeMS());

  if(IsTaskRepeatable())
  {
    if (!m_stmHandler)
    {
      LOG(LOG_LEVEL_ERROR,"BoxeeScheduleTask::PostDoWork - FAILED to queue back task [name=%s][jobId=%d][repeat=%d] since [stmHandler=NULL] (bstm)",m_name.c_str(),GetId(),IsTaskRepeatable());
    }
    else
    {
      //LOG(LOG_LEVEL_DEBUG,"BoxeeScheduleTask::PostDoWork - For [name=%s][jobId=%d][repeat=%d] going to reset and add again to BoxeeScheduleTaskManager (bstm)",m_name.c_str(),GetId(),IsTaskRepeatable());

      ResetTaskExecutionTimeForRepeat();
      m_stmHandler->AddScheduleTask(this);
    }
  }
}

unsigned long BoxeeScheduleTask::GetExecutionDelayInMS()
{
  return m_executionDelayInMS;
}

unsigned long BoxeeScheduleTask::GetExecutionTimeInMS()
{
  return m_executionTimeInMS;
}

unsigned long BoxeeScheduleTask::GetRepeatTaskIntervalInMS()
{
  return m_repeatTaskIntervalInMS;
}

bool BoxeeScheduleTask::IsTaskRepeatable()
{
  return m_repeat;
}

void BoxeeScheduleTask::SetTaskRepeatable(bool repeat)
{
  m_repeat = repeat;
}

bool BoxeeScheduleTask::CanDelete()
{
  return !IsTaskRepeatable();
}

void BoxeeScheduleTask::ResetTaskExecutionTimeForRepeat()
{
  m_executionTimeInMS = CTimeUtils::GetTimeMS() + m_repeatTaskIntervalInMS;

  if (m_executionDelayInMS == 0)
  {
    // in case executionDelayInMS=0 -> need reset it so it won't be execute again immediately
    m_executionDelayInMS = m_executionTimeInMS;
}
}

std::string BoxeeScheduleTask::GetName()
{
  return m_name;
}

void BoxeeScheduleTask::SetScheduleTaskManagerHandler(CBoxeeScheduleTaskManager* stmHandler)
{
  m_stmHandler = stmHandler;
}

CBoxeeScheduleTaskManager* BoxeeScheduleTask::GetScheduleTaskManagerHandler()
{
  return m_stmHandler;
}

/////////////////////////////////////
// CBoxeeScheduleTaskManager Class //
/////////////////////////////////////

CBoxeeScheduleTaskManager::CBoxeeScheduleTaskManager(const std::string& name, unsigned long minSleepTimeInMs)
{
  m_pListLock = SDL_CreateMutex();
  m_pRunningLock = SDL_CreateMutex();
  m_ScheduleProcessMutex = SDL_CreateMutex();
  m_ScheduleProcessCond = SDL_CreateCond();
  m_running = false;
  m_schedulerThread = NULL;  
  m_minSleepTimeInMs = minSleepTimeInMs;
  m_name = name;
}

CBoxeeScheduleTaskManager::~CBoxeeScheduleTaskManager()
{
  if (m_pListLock)
  {
    SDL_DestroyMutex(m_pListLock);
  }
  
  if (m_pRunningLock)
  {
    SDL_DestroyMutex(m_pRunningLock);
  }  
}

bool CBoxeeScheduleTaskManager::Start(int numOfExecutionThreads, bool lazy)
{
  if (!m_pListLock)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::Start - FAILED to create list MUTEX. [name=%s] (bstm)",m_name.c_str());
    return false;
  }

  if (!m_pRunningLock)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::Start - FAILED to create Initialize MUTEX. [name=%s] (bstm)",m_name.c_str());
    return false;
  }

  if (IsRunning())
  {
    LOG(LOG_LEVEL_WARNING,"CBoxeeScheduleTaskManager::Start - Already initialize. [name=%s] (login)",m_name.c_str());
    return true;
  }

  m_ScheduleProcessSemaphore = SDL_CreateSemaphore(0);


  m_scheduleTaskExecuter.SetName(m_name + "_exec");
  m_scheduleTaskExecuter.Start(numOfExecutionThreads, lazy);

  SDL_LockMutex(m_pRunningLock);
  
  m_running = true;

  m_schedulerThread = SDL_CreateThread(ScheduleTaskProcess, this);
  
  if (!m_schedulerThread)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::Start - FAILED to create Scheduler thread. [name=%s] (bstm)",m_name.c_str());
    SDL_UnlockMutex(m_pRunningLock);
    return false;    
  }

  SDL_UnlockMutex(m_pRunningLock);

  return true;
}

bool CBoxeeScheduleTaskManager::Stop()
{
  SDL_LockMutex(m_pRunningLock);

  m_running = false;

  SDL_UnlockMutex(m_pRunningLock);

  m_scheduleTaskExecuter.Stop();

  if (m_schedulerThread) 
  {
    SDL_SemPost(m_ScheduleProcessSemaphore);
    SDL_CondBroadcast(m_ScheduleProcessCond);
    SDL_WaitThread(m_schedulerThread, NULL);
    m_schedulerThread = NULL;
  }
  
  // Clean all of the BoxeeScheduleTask that left in the list
  std::list<BoxeeScheduleTask*>::iterator it;
  it = m_scheduleTaskList.begin();

  while(it != m_scheduleTaskList.end())
  {
    BoxeeScheduleTask* boxeeScheduleTask = *(it);
    it++;
    if (boxeeScheduleTask->ShouldDelete())
      delete boxeeScheduleTask;
  }
  
  m_scheduleTaskList.clear();
  
  SDL_DestroySemaphore(m_ScheduleProcessSemaphore);
  m_ScheduleProcessSemaphore = NULL;

  return true;
}

bool CBoxeeScheduleTaskManager::IsRunning()
{
  bool retVal = false;
  
  SDL_LockMutex(m_pRunningLock);
  
  retVal = m_running;
  
  SDL_UnlockMutex(m_pRunningLock);
  
  return retVal;
}

void CBoxeeScheduleTaskManager::RemoveScheduleTask(BoxeeScheduleTask* task)
{
  SDL_LockMutex(m_pListLock);
  std::list<BoxeeScheduleTask*>::iterator it = m_scheduleTaskList.begin();
  while(it != m_scheduleTaskList.end())
  {
    if (*it==task)
    {
      m_scheduleTaskList.erase(it);
      break;
    }
    it++;
  }  
  SDL_UnlockMutex(m_pListLock);
}
  
bool CBoxeeScheduleTaskManager::AddScheduleTask(BoxeeScheduleTask* newTask)
{
  if(!IsRunning())
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::AddScheduleTask - FAILED to add task because IsRunning() return FALSE. [name=%s] (bstm)",m_name.c_str());
    return false;    
  }
  
  if(!newTask)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::AddScheduleTask - Enter function with a NULL BoxeeScheduleTask object. [name=%s] (bstm)",m_name.c_str());
    return false;        
  }
  
  //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - Enter function with task [name=%s][jobId=%d][delay=%lu][executionTime=%lu]. [name=%s] (bstm)",newTask->GetName().c_str(),newTask->GetId(),newTask->GetExecutionDelayInMS(),newTask->GetExecutionTimeInMS(),m_name.c_str());

  if (newTask->IsTaskRepeatable())
  {
    // set ScheduleTaskManagerHandler for repeat
    newTask->SetScheduleTaskManagerHandler(this);
  }

  // In case of task delay for execution is 0 -> Execute it immediately
  if(newTask->GetExecutionDelayInMS() == 0)
  {
    //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - For task [name=%s][jobId=%d][delay=%lu=0] -> Going to execute it immediately. [name=%s] (bstm)",newTask->GetName().c_str(),newTask->GetId(),newTask->GetExecutionDelayInMS(),m_name.c_str());
    m_scheduleTaskExecuter.QueueJob(newTask);
    return true;
  }

  // Get the new task execution time
  unsigned long newTaskTimeToBeExecute = newTask->GetExecutionTimeInMS();
  
  SDL_LockMutex(m_pListLock);
  
  std::list<BoxeeScheduleTask*>::iterator it;
  it = m_scheduleTaskList.begin();
  bool taskWasInsertToList = false;
  bool taskWasInsertToListHead = true;
  int counter = 0;
  
  while(it != m_scheduleTaskList.end())
  {
    unsigned long listTaskExecuteTimeInMS = (*it)->GetExecutionTimeInMS();

    counter++;
    //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - [%d/%d] - compare if [newTaskTimeToBeExecute=%lu] < [%lu=listTaskExecuteTimeInMS] For task [name=%s][jobId=%d] (bstm)",counter,m_scheduleTaskList.size(),newTaskTimeToBeExecute,listTaskExecuteTimeInMS,newTask->GetName().c_str(),newTask->GetId());

    if(newTaskTimeToBeExecute < listTaskExecuteTimeInMS)
    {
      m_scheduleTaskList.insert(it,newTask);
      
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - Task [name=%s][jobId=%d][delay=%lu][executionTime=%lu] was insert to the list. [ScheduleTaskListSize=%d][WasInsertToListHead=%d]. [name=%s] (bstm)",newTask->GetName().c_str(),newTask->GetId(),newTask->GetExecutionDelayInMS(),newTask->GetExecutionTimeInMS(),m_scheduleTaskList.size(),taskWasInsertToListHead,m_name.c_str());

      taskWasInsertToList = true;
      break;
    }
    
    it++;
    
    // Not going to insert to the list head -> Mark it
    taskWasInsertToListHead = false;
  }
  
  // If task was not insert to list -> Its execution time is the latest -> push it to the back of the list
  if(!taskWasInsertToList)
  {
    m_scheduleTaskList.push_back(newTask);
    
    //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - Task [name=%s][jobId=%d][delay=%lu][executionTime=%lu] was pushed to the end of the list. [ScheduleTaskListSize=%d][WasInsertToListHead=%d]. [name=%s] (bstm)",newTask->GetName().c_str(),newTask->GetId(),newTask->GetExecutionDelayInMS(),newTask->GetExecutionTimeInMS(),m_scheduleTaskList.size(),taskWasInsertToListHead,m_name.c_str());
  }
  else
  {
    // task was inserted to the list -> check if it was insert to the list head 
    if(taskWasInsertToListHead)
    {
      // Task was insert to the list head -> Need to signal CondWaitTimeout
      
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::AddScheduleTask - Task [name=%s][jobId=%d] was insert to the list head. Going to signal. [name=%s] (bstm)",newTask->GetName().c_str(),newTask->GetId(),m_name.c_str());

      SDL_CondSignal(m_ScheduleProcessCond);
    }
  }

  SDL_SemPost(m_ScheduleProcessSemaphore);

  SDL_UnlockMutex(m_pListLock);
  
  return true;
}

int CBoxeeScheduleTaskManager::ScheduleTaskProcess(void *pParam)
{
  // Set the caller of this thread
  CBoxeeScheduleTaskManager* pCaller = (CBoxeeScheduleTaskManager*)pParam;

  if (!pCaller)
  {
    LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - pParam that was passed as parameter is NULL. Going to exit function and return [1]. [name=%s] (bstm)",pCaller->m_name.c_str());
    return 1;
  }

  BoxeeScheduleTask* firstTaskInList = NULL;
  
  //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - Going to try and start loop. [IsRunning=%d]. [name=%s] (bstm)",pCaller->IsRunning(),pCaller->m_name.c_str());

  while (pCaller->IsRunning())
  {
    // wait until there is a task in the list
    SDL_SemWait(pCaller->m_ScheduleProcessSemaphore);

    if(!pCaller->IsRunning())
    {
      // If after SemWait the thread ISN'T Initialize -> Thread was closed -> Call break in order to exit
      break;
    }
    
    SDL_LockMutex(pCaller->m_pListLock);

    if (pCaller->m_scheduleTaskList.empty())
    {
      // If there are no tasks in the list -> Call continue in order to wait for new tasks
      SDL_UnlockMutex(pCaller->m_pListLock);
      continue;
    }

    // Get the first BoxeeScheduleTask in the list
    firstTaskInList = (pCaller->m_scheduleTaskList).front();

    if(!firstTaskInList)
    {
      //LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - First task in the list is NULL -> continue. [name=%s] (bstm)",pCaller->m_name.c_str());

      pCaller->m_scheduleTaskList.pop_front();
      SDL_UnlockMutex(pCaller->m_pListLock);
      continue;
    }

    //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - Handling first task in the list [name=%s][jobId=%d]. [name=%s] (bstm)",firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),pCaller->m_name.c_str());

    int wokeUpReason = -1;
    
    unsigned long taskExecuteTimeInMS = firstTaskInList->GetExecutionTimeInMS();
    unsigned long currentTimeInMS = CTimeUtils::GetTimeMS();
    long delayToExecuteTimeInMS = taskExecuteTimeInMS - currentTimeInMS;
    
    if((delayToExecuteTimeInMS > 0) && (delayToExecuteTimeInMS > (long) pCaller->m_minSleepTimeInMs))
    {
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - Going to call CondWaitTimeout with [delay=%ld] for [name=%s][jobId=%d]. [taskExecuteTimeInMS=%lu][currentTimeInMS=%lu]. [ScheduleTaskListSize=%d]. [name=%s] (bstm)",delayToExecuteTimeInMS,firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),taskExecuteTimeInMS,currentTimeInMS,pCaller->m_scheduleTaskList.size(),pCaller->m_name.c_str());

      wokeUpReason = SDL_CondWaitTimeout(pCaller->m_ScheduleProcessCond, pCaller->m_pListLock, delayToExecuteTimeInMS);
      
      // After CondWaitTimeout -> Need to get the first BoxeeScheduleTask in the list
      firstTaskInList = (pCaller->m_scheduleTaskList).front();
      
      if(!firstTaskInList)
      {
        wokeUpReason = -1;
        
        //LOG(LOG_LEVEL_WARNING,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After CondWaitTimeout, the first CBoxeeScheduleTaskContainer in the list is NULL. wokeUpReason was set to [%d] for ERROR. [ScheduleTaskListSize=%d]. [name=%s] (bstm)",wokeUpReason,pCaller->m_scheduleTaskList.size(),pCaller->m_name.c_str());
      }
      //else
      //{
        //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After CondWaitTimeout, going to handle [name=%s][jobId=%d][taskExecuteTimeInMS=%lu]. [currentTimeInMS=%lu][ScheduleTaskListSize=%d]. [name=%s] (bstm)",firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),firstTaskInList->GetExecutionTimeInMS(),CTimeUtils::GetTimeMS(),pCaller->m_scheduleTaskList.size(),pCaller->m_name.c_str());
      //}
      }
    else
    {
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - [delay=%ld <= %lu] for task [name=%s][jobId=%d] -> Going to execute it NOW by set [wokeUpReason=SDL_MUTEX_TIMEDOUT]. [taskExecuteTimeInMS=%lu][currentTimeInMS=%lu]. [ScheduleTaskListSize=%d]. [name=%s] (bstm)",delayToExecuteTimeInMS,pCaller->m_minSleepTimeInMs,firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),taskExecuteTimeInMS,currentTimeInMS,pCaller->m_scheduleTaskList.size(),pCaller->m_name.c_str());
      
      wokeUpReason = SDL_MUTEX_TIMEDOUT;
    }
      
    if(wokeUpReason == 0)
    {
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After call to CondWaitTimeout with [name=%s][jobId=%d][delay=%lu]. Call returned [wokeUpReason=%d=SIGNAL]. [ScheduleTaskListSize=%d]. [name=%s] (bstm)",firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),delayToExecuteTimeInMS,wokeUpReason,pCaller->m_scheduleTaskList.size(),pCaller->m_name.c_str());

      // Woke up from SDL_CondWaitTimeout by SIGNAL -> A new task was added to the front -> Do nothing and a new CondWaitTimeout will be set
      // (release the pCaller->m_pListLock in order to give a chance to queue before going to set again the CondWaitTimeout)

      SDL_SemPost(pCaller->m_ScheduleProcessSemaphore);

      SDL_UnlockMutex(pCaller->m_pListLock);

      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - A new task was added to the front. Do nothing and loop again in order to set a new timeout. [name=%s] (bstm)",pCaller->m_name.c_str());
    }
    else if(wokeUpReason == SDL_MUTEX_TIMEDOUT)
    {
      // Woke up from SDL_CondWaitTimeout by TIMEOUT -> Need to execute the task
 
      firstTaskInList = (pCaller->m_scheduleTaskList).front();
      (pCaller->m_scheduleTaskList).pop_front();
      
      SDL_UnlockMutex(pCaller->m_pListLock);
      
      taskExecuteTimeInMS = firstTaskInList->GetExecutionTimeInMS();
      
      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After call to CondWaitTimeout with [delay=%lu]. Call returned [wokeUpReason=%d=SDL_MUTEX_TIMEDOUT]. [currentTime=%lu][taskExecuteTimeInMS=%lu][name=%s][jobId=%d]. [name=%s] (bstm)",delayToExecuteTimeInMS,wokeUpReason,CTimeUtils::GetTimeMS(),taskExecuteTimeInMS,firstTaskInList->GetName().c_str(),firstTaskInList->GetId(),pCaller->m_name.c_str());

      //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - Going to queue [name=%s][jobId=%d] for execution (bstm)",firstTaskInList->GetName().c_str(),firstTaskInList->GetId());

      pCaller->m_scheduleTaskExecuter.QueueJob(firstTaskInList);      
    }
    else
    {
      // Woke up from SDL_CondWaitTimeout by ERROR
      // (release the pCaller->m_pListLock in order to give a chance to queue before going to set again the CondWaitTimeout)

      SDL_SemPost(pCaller->m_ScheduleProcessSemaphore);

      SDL_UnlockMutex(pCaller->m_pListLock);

      LOG(LOG_LEVEL_ERROR,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After call to CondWaitTimeout with [delay=%lu]. Call returned [wokeUpReason=%d=ERROR]. Do nothing and loop again in order to set a new timeout. [name=%s] (bstm)",delayToExecuteTimeInMS,wokeUpReason,pCaller->m_name.c_str());
    } 
  }// while
  
  //LOG(LOG_LEVEL_DEBUG,"CBoxeeScheduleTaskManager::ScheduleTaskProcess - After exit loop. [IsRunning=%d]. Going to exit function and return [0]. [name=%s] (bstm)",pCaller->IsRunning(),pCaller->m_name.c_str());

  return 0;
}

}

