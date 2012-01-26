#include <string>

#include "utils/SingleLock.h"
#include "utils/log.h"
#include "URL.h"
#include "SpecialProtocol.h"

#include "ThreadPolicy.h"

#ifdef _WIN32
#define __func__ __FUNCTION__
#endif

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
  bool disabled;

}ThreadPolicyInt;


typedef std::list<ThreadPolicyInt*> ThreadPolicyList;
typedef std::list<ThreadPolicyInt*>::iterator ThreadPolicyIter;
typedef std::list<FileRule*>::iterator FileRuleIter;

static std::map<ThreadIdentifier, ThreadPolicyList> g_mapPolicies;
static CCriticalSection g_treadPolicyLock;

static FileRule defaultRules[] = 
{

  { "default:", FILE_ACCESS_NONE },// FILE_SYSTEM - prohibited to access files
  { "default:", FILE_ACCESS_NONE },// SHARED_LIBRARY - prohibited to load dlls
  { "default:", FILE_ACCESS_NONE },// DISALLOW - everything is prohibited
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
  const char* pmode = item->accessMode;
  FileRuleList& fileRuleList = policy->fileRuleList;
  FileRule *defaultRule = NULL;
  bool bRuleApplied = false;
  std::string fileName = _P(CURI::ValidatePath(item->fileName));

#if defined(HAS_EMBEDDED) && !defined(__APPLE__) 
  CStdString str = fileName;
  str.Replace("/.", "//.");
  fileName = str;
#endif
 
  CStdString reason;

  for(FileRuleIter it = fileRuleList.begin(); it != fileRuleList.end(); ++it)
  {   
    FileRule* rule = *it;
    std::string ruleFilename = CURI::ValidatePath(rule->fileName);
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
      if (bRuleApplied && *safeToOpen == false)
        reason.Format("failed on rule: %s", ruleFilename.c_str());
      break;
    }
  }       

  //
  // No specific policy was found - apply default policy
  //
  if (defaultRule && !bRuleApplied)
  {
    bRuleApplied = TPAppyFileRule(defaultRule, pmode, safeToOpen);   
    if (bRuleApplied && *safeToOpen == false)
      reason.Format("failed on default rule: %s", defaultRule->fileName.c_str());
  }
  
  if (bRuleApplied && *safeToOpen == false)
  {
    CLog::Log(LOGINFO, "%s - blocking access to file: thread id %p, file %s, mode %s. reason: %s",
              __func__, (void*) tid, fileName.c_str(), item->accessMode, reason.c_str());
  }

  return bRuleApplied;
}

static bool TPHandleDisallow(ThreadIdentifier tid, ThreadPolicy* policy, void* dummy, bool* safeToOpen)
{
  *safeToOpen = false;

  CLog::Log(LOGDEBUG, "%s - blocking disallow: thread is %p", __func__, (void*) tid);

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
  policyInt->disabled = 0;

  if(type == FILE_SYSTEM)
    policyInt->handler = (PolicyHandlerFunc)TPHandleFileSystemPolicy;
  else if(type == SHARED_LIBRARY)
    policyInt->handler = (PolicyHandlerFunc)TPHandleFileSystemPolicy;
  else if(type == DISALLOW)
    policyInt->handler = (PolicyHandlerFunc)TPHandleDisallow;

  policy->type = type;
  policy->fileRuleList.push_back(&defaultRules[type]);

  return policy;
}

void 
TPDeletePolicy(ThreadIdentifier tid, ThreadPolicy* policy)
{
  CSingleLock lock(g_treadPolicyLock);
  ThreadPolicyList& policyList = g_mapPolicies[tid];

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
  CSingleLock lock(g_treadPolicyLock);
  ThreadPolicyList& policyList = g_mapPolicies[tid];
  bool bDuplicatePolicy = false;

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
  CSingleLock lock(g_treadPolicyLock);

  if(g_mapPolicies.size() == 0)
   return false;

  ThreadPolicyList& policyList = g_mapPolicies[tid];
  bool bApplied = false;

  if(policyList.empty())
    return false;	

  if(policyList.front()->disabled)
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
  CSingleLock lock(g_treadPolicyLock);

  if(g_mapPolicies.size() == 0)
   return false;

  ThreadPolicyList& parentPolicyList = g_mapPolicies[tidParent];
  bool bInhereted = true;

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
  CSingleLock lock(g_treadPolicyLock);

  FileRule* theRule = new FileRule;
  
  theRule->fileName = rule->fileName;
  theRule->accessMode = rule->accessMode;

  policy->fileRuleList.push_back(theRule);

  return true;
}

bool
TPDeleteRuleFromPolicy(ThreadIdentifier tid, ThreadPolicy* policy, const FileRule* rule)
{
  CSingleLock lock(g_treadPolicyLock);
  bool bDeleted = false;

  if(!policy || !rule)
    return false;

  FileRuleList& fileRuleList = policy->fileRuleList;
  for(FileRuleIter it = fileRuleList.begin(); it != fileRuleList.end(); ++it)
  {
    FileRule* rule = *it;
    std::string ruleFilename = CURI::ValidatePath(rule->fileName);

    if(ruleFilename == rule->fileName)
    {
      fileRuleList.erase(it);
      bDeleted = true;
      break;
    }
  }

  return bDeleted;
}

void
TPDisablePolicy(ThreadIdentifier tid, bool bEnable)
{
  CSingleLock lock(g_treadPolicyLock);
  ThreadPolicyList& policyList = g_mapPolicies[tid];

  if(policyList.empty())
    return;

  policyList.front()->disabled = !bEnable;
}

ThreadPolicy*
TPGetPolicy(ThreadIdentifier tid, PolicyType type)
{
  CSingleLock lock(g_treadPolicyLock);
  ThreadPolicyList& policyList = g_mapPolicies[tid];

  if(policyList.empty())
    return NULL;

  for(ThreadPolicyIter it = policyList.begin(); it != policyList.end(); ++it)
  {
    ThreadPolicyInt* policyInt = *it;
    ThreadPolicy* policy = &policyInt->policy;

    if(policy->type == type)
    {
      ++policyInt->refCount;
      return policy;
    }
  }

  return NULL;
}

void
TPReleasePolicy(ThreadIdentifier tid, ThreadPolicy* policy)
{
  CSingleLock lock(g_treadPolicyLock);

  if (policy == NULL)
    return;

  ThreadPolicyInt* policyInt = CONTAINING_RECORD(policy, ThreadPolicyInt, policy);

  if (policyInt->refCount - 1 == 0)
  {
    TPDeletePolicy(tid, policy);
  }
}
