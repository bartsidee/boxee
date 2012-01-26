#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sstream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "BoxeeHalClient.h"
#include "Functions.h"
#include "JsonWrapper.h"
#include "utils/SingleLock.h"
#include "Util.h"

#define BUFSIZE 4096

static const char* NOTIFICATIONS_REGISTRATION = "GET /notifications HTTP/1.1\r\n\r\n";

#ifdef BOXEE_STANDALONE_HAL_CLIENT

int static RunNotificationsThread(void* pClient)
{
  if(!pClient)
  return 0;

  CBoxeeHalClient* pHalClient = (CBoxeeHalClient*)pClient;
  pHalClient->Process();

  return 0;
}

#endif

void CBoxeeHalClient::Process()
{
  bool expect_header = true;
  m_skfd = -1;
  while (true)
  {
    if (m_skfd < 0)
    {
      expect_header = true;
      if (!CreateSocket(m_skfd))
      {
        m_skfd = -1;
        sleep(1);
        continue;
      }
      else
      {
        if (!RegisterNotifications())
        {
          m_skfd = -1;
          continue;
        }
      }
    }

    fd_set fdRead;

    FD_ZERO(&fdRead);
    FD_SET(m_skfd, &fdRead);
    int rc = select(m_skfd + 1, &fdRead, NULL, NULL, NULL);

    if (rc < 0)
    {
      close(m_skfd);
      m_skfd = -1;
      continue;
    }
    else if (rc == 0)
    {
      continue;
    }

    if (!ProcessNotification(expect_header))
    {
      close(m_skfd);
      m_skfd = -1;
    }
    else
    {
      expect_header = false;
    }

  }
}

void CBoxeeHalClient::FormatRequest(const std::string& strClass,
    const std::string& strMethod, StringMap* pParams, std::string &strRequest)
{
  // request template is: GET /<class>.<method>?<param1_name>=<param1_value>&<param2_name>=<param2_value>,... HTTP/1.1
  std::stringstream request;

  request << strClass;
  if (strMethod != "")
    request << "." << strMethod;

  if (pParams && !pParams->empty())
  {
    StringMap::const_iterator iter = pParams->begin();
    StringMap::const_iterator endIter = pParams->end();
    CStdString name = (*iter).first;
    CUtil::URLEncode(name);
    CStdString value = (*iter).second;
    CUtil::URLEncode(value);

    request << "?" << name << "=" << value;
    ++iter;
    for (; iter != endIter; ++iter)
    {
      name = (*iter).first;
      CUtil::URLEncode(name);
      value = (*iter).second;
      CUtil::URLEncode(value);
      request << "&" <<  name << "=" << value;
    }
  }

  strRequest = request.str();
  CStdString tmpStr = strRequest;
  strRequest = tmpStr;

  strRequest = "GET /" + strRequest;

  strRequest = strRequest + " HTTP/1.1\r\n\r\n";
}

bool CBoxeeHalClient::SendRequest(const std::string& strClass,
    const std::string& strMethod, StringMap* pParams, std::string& response)
{
  ssize_t nBytes = 0;
  std::string strRequest;
  int reqfd;

  if (!CreateSocket(reqfd))
    return false;
  FormatRequest(strClass, strMethod, pParams, strRequest);

  while (!strRequest.empty())
  {
    nBytes = write(reqfd, strRequest.c_str(), strRequest.size());
    if (nBytes == -1)
    {
      close(reqfd);
      return false;
    }

    strRequest.erase(0, nBytes);
  }

  if (!ReadResponse(response, reqfd))
  {
    close(reqfd);
    return false;
  }

  close(reqfd);

  return true;
}

bool CBoxeeHalClient::ReadResponse(std::string& strResponse, int skfd) const
{
  char szBuffer[BUFSIZE];
  ssize_t nBytes = 0;

  memset(&szBuffer[0], 0, BUFSIZE * sizeof(char));
  nBytes = read(skfd, &szBuffer[0], BUFSIZE);

  // first find response length
  char* cl = strstr(szBuffer, "Content-Length: ");

  if (!cl)
    return false;

  char* eol = strstr(cl, "\r\n");

  if (!eol)
    return false;

  cl += strlen("Content-Length: ");

  char* pszLength = (char*) calloc(eol - cl + 1, sizeof(char));

  strncpy(pszLength, cl, eol - cl);

  int length = atoi(pszLength);
  char* eoh = strstr(szBuffer, "\r\n\r\n");

  strResponse.assign(eoh + strlen("\r\n\r\n"), length);

  return true;
}

/**
 * class CBoxeeHalClient
 **/
CBoxeeHalClient::CBoxeeHalClient() :
  m_skfd(0)
{
#ifndef BOXEE_STANDALONE_HAL_CLIENT
  Create();
#else
  pthread_t __newthread;
  pthread_create(&__newthread, NULL, (void*(*)(void*))RunNotificationsThread, (void *)this);
#endif
}

CBoxeeHalClient::~CBoxeeHalClient()
{
  if (m_skfd > 0)
    close(m_skfd);
}

bool CBoxeeHalClient::CreateSocket(int &skfd)
{
  skfd = socket(AF_INET, SOCK_STREAM, 0);
  if (skfd < 0)
    return false;

  struct sockaddr_in server_addr;

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(BOXEE_HAL_PORT);
  server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
  bzero(&(server_addr.sin_zero), 8);

  socklen_t addrlen = sizeof(struct sockaddr);
  int rc = connect(skfd, (struct sockaddr*) &server_addr, addrlen);
  if (rc == -1)
  {
    close(skfd);
    skfd = -1;
  }
  return (rc == 0);
}

bool CBoxeeHalClient::RegisterNotifications()
{
  ssize_t rc = write(m_skfd, NOTIFICATIONS_REGISTRATION, strlen(
      NOTIFICATIONS_REGISTRATION));

  if (rc == -1)
  {
    close(m_skfd);
    m_skfd = -1;
    return false;
  }

  std::string strNotification;
  return true;
}

bool CBoxeeHalClient::ProcessNotification(bool bExpectHeader) const
{
  std::vector<std::string> strNotifications;

  if (!ReadNotificationBody(strNotifications, bExpectHeader))
    return false;

  if (!bExpectHeader)
    NotifyListeners(strNotifications);

  return true;
}

bool CBoxeeHalClient::ReadNotificationBody(std::vector<std::string>& strNotifications,
    bool bExpectHeader) const
{
  strNotifications.clear();

  char szBuffer[BUFSIZE] = { 0 };
  ssize_t nBytes = 0;

  memset(&szBuffer[0], 0, BUFSIZE * sizeof(char));
  nBytes = read(m_skfd, &szBuffer[0], BUFSIZE);

  char* buf = szBuffer;

  while (nBytes > 0)
  {
    if (strcmp(buf, NOTIFICATION_REGISTRATION_RESPONSE) == 0
        && bExpectHeader)
    {
      return true;
    }

    char* eol = strstr(buf, "\r\n");
    if (!eol)
    {
      break;
    }

    char length[3]; // allowing up to 0xff chars
    memset(&length[0], 0, sizeof(length));
    int nLength = 0;

    strncpy(length, buf, eol - buf);
    sscanf(buf, "%x", &nLength);

    CStdString strNotification;
    if (nLength != 0)
    {
      strNotification.assign(eol + 2, nLength - 1);
      char* newBuf = (eol + 2) + (nLength - 1) + 2 /* \r\n */;
      nBytes -= (newBuf - buf);
      buf = newBuf;
    }
    else
    {
      strNotification.clear();
      break;
    }

    strNotifications.push_back(strNotification);
  }

  return (strNotifications.size() > 0);
}

void CBoxeeHalClient::NotifyListeners(const std::vector<std::string>& strNotifications) const
{
  StringMap map;

#ifndef BOXEE_STANDALONE_HAL_CLIENT
  CSingleLock lock(m_lock);
#endif

  for (size_t i = 0; i < strNotifications.size(); i++)
  {
    std::vector<CBoxeeHalClientListener*>::const_iterator iter = m_vListeners.begin();
    std::vector<CBoxeeHalClientListener*>::const_iterator endIter = m_vListeners.end();

    for (; iter != endIter; ++iter)
    {
      (*iter)->Notify(strNotifications[i]);
    }
  }
}

bool CBoxeeHalClient::IsMessageRelevant(
    const std::string& strNotificationClass, NOTIFICATION_TYPE eType) const
{
  if (eType == NOTIFY_ALL)
    return true;
  else if ((eType & NOTIFY_ETHERNET) && strNotificationClass == "ethernet")
    return true;
  else if ((eType & NOTIFY_WIRELESS) && strNotificationClass == "wireless")
    return true;
  else if ((eType & NOTIFY_POWER) && strNotificationClass == "power")
    return true;
  else if ((eType & NOTIFY_INPUT) && strNotificationClass == "input")
    return true;
  else if ((eType & NOTIFY_STORAGE) && strNotificationClass == "storage")
    return true;
  else
    return false;
}

bool CBoxeeHalClient::AddListener(CBoxeeHalClientListener* listener)
{
#ifndef BOXEE_STANDALONE_HAL_CLIENT
  CSingleLock lock(m_lock);
#endif

  m_vListeners.push_back(listener);
  return true;
}
