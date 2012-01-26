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
#ifndef BOXEESCHEDULETASKMANAGER_H
#define BOXEESCHEDULETASKMANAGER_H

#include "bxbgprocess.h"

#include <SDL/SDL.h>
#include <list>

namespace BOXEE
{

class BXItem
{
public:
  BXItem(const std::string& type, const std::string& id, const std::string& name) : m_type(type),m_id(id),m_name(name){};
  virtual ~BXItem(){};
  
  std::string GetType(){return m_type;};
  std::string GetId(){return m_id;};
  std::string GetName(){return m_name;};

private:
  
  std::string m_type;
  std::string m_id;
  std::string m_name;
};

class CBoxeeScheduleTaskManager;

class BoxeeScheduleTask : public BXBGJob
{
public:
  BoxeeScheduleTask(const std::string& name,unsigned long executionDelayInMS,bool repeat=false);
  BoxeeScheduleTask(const std::string& name,unsigned long firstExecutionDelayInMS,unsigned long repeatTaskIntervalInMS = 0);
  virtual ~BoxeeScheduleTask();

  unsigned long GetExecutionDelayInMS();
  unsigned long GetExecutionTimeInMS();
  unsigned long GetRepeatTaskIntervalInMS();

  void SetScheduleTaskManagerHandler(CBoxeeScheduleTaskManager* stmHandler);
  CBoxeeScheduleTaskManager* GetScheduleTaskManagerHandler();

  bool IsTaskRepeatable();
  
  std::string GetName();
  
  virtual void PostDoWork();
  virtual bool ShouldDelete() { return true; }
  
  virtual bool CanDelete();

protected:
  
  void SetTaskRepeatable(bool repeat);
  void ResetTaskExecutionTimeForRepeat();

  unsigned long m_executionDelayInMS;
  unsigned long m_executionTimeInMS;
  unsigned long m_repeatTaskIntervalInMS;
  bool m_repeat;
  std::string m_name;

  CBoxeeScheduleTaskManager* m_stmHandler;

private:

};

class CBoxeeScheduleTaskManager
{
public:
  CBoxeeScheduleTaskManager(const std::string& name, unsigned long minSleepTimeInMs = 50);
  virtual ~CBoxeeScheduleTaskManager();

  bool Start(int numOfExecutionThreads, bool lazy = true);
  bool Stop();
  
  bool AddScheduleTask(BoxeeScheduleTask* task);
  void RemoveScheduleTask(BoxeeScheduleTask* task);

  bool IsRunning();
  
protected:

  static int ScheduleTaskProcess(void* pParam);
  SDL_mutex* m_ScheduleProcessMutex;
  SDL_cond*  m_ScheduleProcessCond;
  SDL_sem* m_ScheduleProcessSemaphore;

  SDL_Thread* m_schedulerThread;
	std::list<BoxeeScheduleTask*> m_scheduleTaskList;
  SDL_mutex* m_pListLock;
  
  BXBGProcess m_scheduleTaskExecuter;
  
  SDL_mutex* m_pRunningLock;
  bool m_running;

  std::string m_name;
  unsigned long m_minSleepTimeInMs;
};

//extern CBoxeeScheduleTaskManager g_boxeeScheduleTaskManager;

} // namespace

#endif
