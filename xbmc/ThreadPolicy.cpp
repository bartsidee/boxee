#include <string>

#include "utils/SingleLock.h"
#include "utils/log.h"
#include "URL.h"

#include "ThreadPolicy.h"

#ifndef CONTAINING_RECORD

#define CONTAINING_RECORD(address, type, field) \
    ((type *)( \
    (char*)(address) - \
    (uintptr_t)(&((type *)0)->field)))

#endif

typedef bool (*PolicyHandlerFunc)(ThreadIdentifier, ThreadPolicy*, void*, bool*);

typedef struct tagThreadPolicyInt
{
  ThreadPolicy policy;
  unsigned int refCount;
  PolicyHandlerFunc handler;

}ThreadPolicyInt;


typedef std::list<ThreadPolicyInt*> ThreadPolicyList;
typedef std::list<ThreadPolicyInt*>::iterator ThreadPolicyIter;
typedef std::list<FileRule*>::iterator FileRuleIter;

static std::map<ThreadIdentifier, ThreadPolicyList> g_mapPolicies;
static CCriticalSection g_treadPolicyLock;

static FileRule defaultRules[] = 
{

  { "default:", FILE_ACCESS_READ },// FILE_SYSTEM - readonly for all files
  { "default:", FILE_ACCESS_NONE },// SHARED_LIBRARY - prohibited to load dlls
  { "default:", FILE_ACCESS_NONE },// PROCESS_EXEC - prohibited to exec processes

};

static void
TPFreePolicyInt(ThreadPolicyInt* policyInt)
{
  if(--policyInt->refCount == 0)
  {
    ThreadPolicy* policy = &policyInt->policy;

    std::list<FileRule*>::iterator fileRuleIt = policy->fileRuleList.begin();
    while (fileRuleIt != policy->fileRuleList.end())
    {
        FileRule* rule = *fileRuleIt;
              
        fileRuleIt = policy->fileRuleList.erase(fileRuleIt);

        if(rule->fileName.compare("default:"))
          delete rule;
    }

    delete policyInt;
  }
}

static bool 
TPAppyFileRule(FileRule *rule, const char *pmode, bool* safeToOpen)
{
  bool bRuleApplied = false;

  char access = *pmode;
  switch(access)
  {
    case 'w':
    {
      *safeToOpen = (rule->accessMode >= FILE_ACCESS_WRITE);

      bRuleApplied = true;

      break;
    }

    case 'r':
    {
      *safeToOpen = (rule->accessMode >= FILE_ACCESS_READ);

      bRuleApplied = true;

      break;
    }

  }

  return bRuleApplied;
}

static bool 
TPHandleFileSystemPolicy(ThreadIdentifier tid, ThreadPolicy* policy, FileSystemItem* item, bool* safeToOpen)
{
  const char* strFilename = item->fileName;
  const char* pmode = item->accessMode;
  FileRuleList& fileRuleList = policy->fileRuleList;
  FileRule *defaultRule = NULL;
  bool bRuleApplied = false;

  for(FileRuleIter it = fileRuleList.begin(); it != fileRuleList.end(); ++it)
  {   
    FileRule* rule = *it;
    std::string ruleFilename = CURL::ValidatePath(rule->fileName);
    std::string fileName = CURL::ValidatePath(strFilename);
    size_t found = std::string::npos;

    if(!ruleFilename.compare("default:"))
    {
      defaultRule = rule;
    }
    else
    {
      found = fileName.find(ruleFilename);
    }

    if(found != std::string::npos)
    {
      bRuleApplied = TPAppyFileRule(rule, pmode, safeToOpen);

      break;
    }
  }       

  //
  // No specific policy was found - apply default policy
  //
  if(defaultRule && !bRuleApplied)
  {
    bRuleApplied = TPAppyFileRule(defaultRule, pmode, safeToOpen);   

  }

  CLog::Log(LOGDEBUG, "%s: policy: thread id %ud, file %s, mode %s, safeToOpen %s", 
              __FUNCTION__, (unsigned int)tid, item->fileName, item->accessMode, *safeToOpen ? "true" : "false");

  return bRuleApplied;
}
 
static bool TPHandleProcessExec(ThreadIdentifier tid, ThreadPolicy* policy, char* processName, bool* safeToOpen)
{
  *safeToOpen = false;

  return true;
}

ThreadPolicy* TPAllocPolicy(ThreadIdentifier tid, PolicyType type)
{
  if(type < 0 && type > POLICY_TYPE_MAX)
    return NULL;

  ThreadPolicyInt* policyInt = new ThreadPolicyInt;
  ThreadPolicy* policy = &policyInt->policy;

  policyInt->refCount = 1;
  policyInt->handler = NULL;

  if(type == FILE_SYSTEM)
    policyInt->handler = (PolicyHandlerFunc)TPHandleFileSystemPolicy;
  else if(type == SHARED_LIBRARY)
    policyInt->handler = (PolicyHandlerFunc)TPHandleFileSystemPolicy;
  else if(type == PROCESS_EXEC)
    policyInt->handler = (PolicyHandlerFunc)TPHandleProcessExec;

  policy->type = type;
  policy->fileRuleList.push_back(&defaultRules[type]);

  return policy;
}

void 
TPDeletePolicy(ThreadIdentifier tid, ThreadPolicy* policy)
{
  ThreadPolicyList& policyList = g_mapPolicies[tid];
  CSingleLock lock(g_treadPolicyLock);

  if(policyList.empty())
    return;	

  if(policy == NULL)
  {
    ThreadPolicyIter threadPolicyListIt = policyList.begin();
    while (threadPolicyListIt != policyList.end()) 
    { 
      ThreadPolicyInt* currPolicyInt = *threadPolicyListIt;
      
      threadPolicyListIt = policyList.erase(threadPolicyListIt);  

      TPFreePolicyInt(currPolicyInt);     
    }
  }
  else
  {
    ThreadPolicyInt* policyInt = CONTAINING_RECORD(policy, ThreadPolicyInt, policy);

    policyList.remove(policyInt);

    TPFreePolicyInt(policyInt);
  }
}

bool 
TPAddPolicy(ThreadIdentifier tid, ThreadPolicy* policy)
{
  ThreadPolicyList& policyList = g_mapPolicies[tid];
  bool bDuplicatePolicy = false;
  CSingleLock lock(g_treadPolicyLock);

  for(ThreadPolicyIter it = policyList.begin(); it != policyList.end(); ++it)
  {
    ThreadPolicyInt* currPolicyInt = *it;
    ThreadPolicy* currPolicy = &currPolicyInt->policy;
    
    if(currPolicy == policy)
    {
      bDuplicatePolicy = true;
      break;
    }	
  }

  if(bDuplicatePolicy == false)
  {    
     ThreadPolicyInt* policyInt = CONTAINING_RECORD(policy, ThreadPolicyInt, policy);

     policyList.push_back(policyInt);
  }
  
  return !bDuplicatePolicy;
}

bool 
TPApplyPolicy(ThreadIdentifier tid, PolicyType type, void* item, bool* safeToOpen)
{
  ThreadPolicyList& policyList = g_mapPolicies[tid];
  bool bApplied = false;

  if(policyList.empty())
    return false;	

  *safeToOpen = true;

  for(ThreadPolicyIter it = policyList.begin(); it != policyList.end() && !bApplied; ++it)
  {
    ThreadPolicyInt* policyInt = *it;
    ThreadPolicy* policy = &policyInt->policy;
    
    if(policy->type == type && policyInt->handler)
    {
      bApplied = policyInt->handler(tid, policy, item, safeToOpen);
    }
  }

  return bApplied;
} 

bool 
TPInheritPolicy(ThreadIdentifier tid, ThreadIdentifier tidParent)
{
  ThreadPolicyList parentPolicyList = g_mapPolicies[tidParent];
  bool bInhereted = true;
  CSingleLock lock(g_treadPolicyLock);
 
  if(parentPolicyList.empty())
    return false;	

  for(ThreadPolicyIter it = parentPolicyList.begin(); it != parentPolicyList.end(); ++it)
  {
    ThreadPolicyInt* policyInt = *it;

    ++policyInt->refCount;
    g_mapPolicies[tid].push_back(policyInt);

  }  

  return bInhereted;
}

bool 
TPAddRuleToPolicy(ThreadIdentifier tid, ThreadPolicy* policy, const FileRule* rule)
{
  ThreadPolicyList& policyList = g_mapPolicies[tid];

  if(policyList.empty())
    return false;	

  FileRule* theRule = new FileRule;
  
  theRule->fileName = rule->fileName;
  theRule->accessMode = rule->accessMode;

  policy->fileRuleList.push_back(theRule);

  return true;
}
