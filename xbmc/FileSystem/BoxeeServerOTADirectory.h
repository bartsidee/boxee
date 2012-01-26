#ifndef CBOXEESERVEROTADIRECTORY_H_
#define CBOXEESERVEROTADIRECTORY_H_

#include "IDirectory.h"
#include "../../lib/libjson/include/json/value.h"
#include <vector>

namespace DIRECTORY
{

class CBoxeeServerOTADirectory : public IDirectory
{
public:
  CBoxeeServerOTADirectory();
  virtual ~CBoxeeServerOTADirectory();
  virtual bool GetDirectory(const CStdString& strPath, CFileItemList &items);
  virtual bool Exists(const char* strPath);
  virtual DIR_CACHE_TYPE GetCacheType(const CStdString& strPath) const;
  static bool HasOption(const std::map<CStdString, CStdString>& mapOptions, const CStdString& optionName, CStdString& optionValue);
  static void AddParametersToRemoteRequest(std::map<CStdString, CStdString>& mapRemoteOptions);

private:
  bool HandleLocationRequest(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  bool HandleEPGRequest(const CStdString& strRequest, std::map<CStdString, CStdString>& mapOptions, CFileItemList &items);
  long HandleJsonRequest(const std::string& url, Json::Value& response);
  int ParseJsonResponse(const Json::Value& jsonValue, CFileItemList& outputList);
};

}

#endif /*CBOXEESERVEROTADIRECTORY_H_*/
