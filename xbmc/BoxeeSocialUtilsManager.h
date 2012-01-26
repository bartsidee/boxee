
#ifndef BOXEESOCIALUTILSMANAGER_H
#define BOXEESOCIALUTILSMANAGER_H

#include <string>
#include "GUIWindow.h"
#include "FileItem.h"
#include "lib/libjson/include/json/value.h"

#define FACEBOOK_SERVICE_ID                   "311"
#define TWITTER_SERVICE_ID                    "413"
#define TUMBLR_SERVICE_ID                     "323"

class CBoxeeSocialUtilsManager
{
public:
  CBoxeeSocialUtilsManager();
  virtual ~CBoxeeSocialUtilsManager();
  bool Initialize();
  void Reset();
  bool IsInitialized();

  CStdString GetConnectLink(const CStdString& serviceId);
  CStdString GetDisconnectLink(const CStdString& serviceId);

  bool IsConnected(const CStdString& serviceId, bool getDataFromServer = false);
  bool RequiresReconnect(const CStdString& serviceId, bool getDataFromServer = false);

  bool IsAnyConnected(bool getDataFromServer = false);

  void SetIsConnected(const CStdString& serviceId, bool isConnected);

private:
  struct SocialServiceInfo
  {
    SocialServiceInfo() { isConnected = false; requiresUpdate = false; }

    bool isConnected;
    bool requiresUpdate;
    std::string link;
    std::string externalLink;
    std::string disconnectLink;
  };

  bool GetStatusFromServer();

  CFileItemList m_servicesList;
  Json::Value m_jsonServiceList;

  std::map<CStdString, SocialServiceInfo> m_serviceMap;

  bool m_bIsInit;
};

#endif
