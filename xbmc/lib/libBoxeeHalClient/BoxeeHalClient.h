#ifndef BOXEEHALCLIENT_H
#define BOXEEHALCLIENT_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <vector>
#include <map>

#include "Common.h"
#include "xbmc/utils/Thread.h"

enum NOTIFICATION_TYPE
{
  NO_NOTIFICATION = 0x00000000,
  NOTIFY_ETHERNET = 0x00000001,
  NOTIFY_WIRELESS = 0x00000002,
  NOTIFY_POWER    = 0x00000004,
  NOTIFY_INPUT    = 0x00000008,
  NOTIFY_STORAGE  = 0x00000010,
  NOTIFY_ALL      = 0xFFFFFFFF
};

class CBoxeeHalClientListener
{
public:
  CBoxeeHalClientListener(NOTIFICATION_TYPE eType = NOTIFY_ALL) :
    m_eType(eType)
  {
  }

  operator NOTIFICATION_TYPE() const
  {
    return m_eType;
  }

  virtual bool Notify(const std::string& notificationStr) = 0;

  virtual ~CBoxeeHalClientListener()
  {
  }

private:
  NOTIFICATION_TYPE m_eType;
};

#ifndef BOXEE_STANDALONE_HAL_CLIENT
class CBoxeeHalClient: public CThread
#else
class CBoxeeHalClient
#endif
{
public:
  bool SendRequest( const std::string& strClass, const std::string& strMethod, StringMap* pParams, std::string& response );
  CBoxeeHalClient();
  virtual ~CBoxeeHalClient();
  bool AddListener( CBoxeeHalClientListener* listener );
  void FormatResponse( const std::string& response, StringMap& map ) const;

#ifndef BOXEE_STANDALONE_HAL_CLIENT
protected:
#endif
  virtual void Process();

private:
  bool IsMessageRelevant( const std::string& strNotificationClass, NOTIFICATION_TYPE m_eType ) const;
  void NotifyListeners( const std::vector<std::string>& strNotifications ) const;
  bool ProcessNotification(bool bExpectHeader) const;
  void CreateMutex();
  bool CreateSocket(int &fd);
  bool ReadNotificationBody( std::vector<std::string>& strNotification , bool bExpectHeader) const;
  bool RegisterNotifications();
  int m_skfd;
  void FormatRequest( const std::string& strClass, const std::string& strMethod, StringMap* pParams, std::string &strRequest );

  bool ReadResponse( std::string& strResponse, int skfd ) const;
  std::vector< CBoxeeHalClientListener* > m_vListeners;
#ifndef BOXEE_STANDALONE_HAL_CLIENT
  CCriticalSection m_lock;
#endif
};


#endif

