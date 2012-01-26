
#include "XAPP_LocalConfig.h"
#include "AppManager.h"

namespace XAPP
{

void LocalConfig::SetValue(const std::string& key, const std::string& value)
{
  CAppManager::GetInstance().GetRegistry().Set(key, value);
}

std::string LocalConfig::GetValue(const std::string& key)
{
  return CAppManager::GetInstance().GetRegistry().Get(key);
}

void LocalConfig::Reset(const std::string& key)
{
  CAppManager::GetInstance().GetRegistry().Unset(key);
}

void LocalConfig::ResetAll()
{
  CAppManager::GetInstance().GetRegistry().Clear();  
}

void LocalConfig::PushBackValue(const std::string& key, const std::string& value, const int limit)
{
  CAppManager::GetInstance().GetRegistry().PushBack(key, value, limit);
}

void LocalConfig::PushFrontValue(const std::string& key, const std::string& value, const int limit)
{
  CAppManager::GetInstance().GetRegistry().PushFront(key, value, limit);  
}

std::string LocalConfig::Implode(const std::string glue, const std::string& key)
{
  return CAppManager::GetInstance().GetRegistry().Implode(glue, key);    
}

}
