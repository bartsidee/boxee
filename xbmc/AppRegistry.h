#ifndef APP_REGISTRY_H_
#define APP_REGISTRY_H_

#include <map>
#include <deque>
#include "AppDescriptor.h"
#include "utils/CriticalSection.h"

class CAppRegistry
{
public:
  CAppRegistry();
  virtual ~CAppRegistry();

  void Load(const CAppDescriptor& desc, bool persistent = false);
  const CStdString& Get(const CStdString& key);
  bool Has(const CStdString& key);
  void Set(const CStdString& key, const CStdString& value);
  void Set(const CStdString& key, const char* value);
  void PushBack(const CStdString& key, const CStdString& value, size_t limit);
  void PushFront(const CStdString& key, const CStdString& value, size_t limit);
  void Unset(const CStdString& key);
  CStdString Implode(const CStdString glue, const CStdString& key);  
  void Clear();

private:
  void Save();

  typedef std::deque<CStdString> RegistryValue;
  typedef std::deque<CStdString>::iterator RegistryValueIterator;
  typedef std::map<CStdString, RegistryValue> RegistryData;
  typedef std::map<CStdString, RegistryValue>::iterator RegistryDataIterator;
  RegistryData m_data;
  CStdString m_dir;
  CStdString m_fileName;
  
  CCriticalSection m_lock;
};

#endif /*APP_REGISTRY_H_*/
