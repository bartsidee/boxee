#ifndef _THREAD_POLICY_H_
#define _THREAD_POLICY_H_

#include <string>
#include <list>
#include <map>

#include <system.h>

typedef enum enumPolicyType
{
  FILE_SYSTEM = 0,
  SHARED_LIBRARY,
  DISALLOW,

  POLICY_TYPE_MAX

} PolicyType;
 

#define FILE_ACCESS_NONE	0
#define FILE_ACCESS_READ	(1<<1)
#define FILE_ACCESS_WRITE	(1<<2)
#define FILE_ACCESS_ALL	(FILE_ACCESS_READ | FILE_ACCESS_WRITE)


typedef struct tagFileRule
{
  std::string fileName;
  int accessMode;

} FileRule;

typedef std::list<FileRule*> FileRuleList;

typedef struct tagThreadPolicy
{
  PolicyType type;
  FileRuleList fileRuleList;

} ThreadPolicy;

typedef struct tagFileSystemItem
{
  const char* fileName;
  const char* accessMode;

} FileSystemItem;

extern "C" {

ThreadPolicy* TPAllocPolicy(ThreadIdentifier tid, PolicyType type);
void TPDeletePolicy(ThreadIdentifier tid, ThreadPolicy* policy);
bool TPAddPolicy(ThreadIdentifier tid, ThreadPolicy* policy);
bool TPApplyPolicy(ThreadIdentifier tid, PolicyType type, void* item, bool* safeToOpen);
bool TPInheritPolicy(ThreadIdentifier tid, ThreadIdentifier tidParent);
bool TPAddRuleToPolicy(ThreadIdentifier tid, ThreadPolicy* policy, const FileRule* rule);
bool TPDeleteRuleFromPolicy(ThreadIdentifier tid, ThreadPolicy* policy, const FileRule* rule);
void TPDisablePolicy(ThreadIdentifier tid, bool bEnable);
ThreadPolicy* TPGetPolicy(ThreadIdentifier tid, PolicyType type);
void TPReleasePolicy(ThreadIdentifier tid, ThreadPolicy* policy);
}




#endif
