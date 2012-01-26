/*
 * Many concepts and protocol specification in this code are taken
 * from Airplayer. https://github.com/PascalW/Airplayer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "AirPlayServer.h"

#ifdef HAS_AIRPLAY

#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include "plist/plist++.h"
#include "log.h"
#include "SingleLock.h"
#include "StdString.h"
#include "FileItem.h"
#include "Application.h"
#include "Util.h"
#include "URL.h"
#include "Zeroconf.h"
#include "../lib/libBoxee/bxcurl.h"
#include "StringUtils.h"

#define RECEIVEBUFFER 1024

#define AIRPLAY_STATUS_OK 200
#define AIRPLAY_STATUS_SWITCHING_PROTOCOLS 101
#define AIRPLAY_STATUS_NOT_IMPLEMENTED 501

CAirPlayServer *CAirPlayServer::ServerInstance = NULL;

#define PLAYBACK_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"\
"<plist version=\"1.0\">"\
"<dict>"\
"<key>duration</key>"\
"<real>%f</real>"\
"<key>loadedTimeRanges</key>"\
"<array>"\
"    <dict>"\
"        <key>duration</key>"\
"        <real>%f</real>"\
"        <key>start</key>"\
"        <real>0.0</real>"\
"    </dict>"\
"</array>"\
"<key>playbackBufferEmpty</key>"\
"<true/>"\
"<key>playbackBufferFull</key>"\
"<false/>"\
"<key>playbackLikelyToKeepUp</key>"\
"<true/>"\
"<key>position</key>"\
"<real>%f</real>"\
"<key>rate</key>"\
"<real>%d</real>"\
"<key>readyToPlay</key>"\
"<true/>"\
"<key>seekableTimeRanges</key>"\
"<array>"\
"    <dict>"\
"        <key>duration</key>"\
"        <real>%f</real>"\
"        <key>start</key>"\
"        <real>0.0</real>"\
"    </dict>"\
"</array>"\
"</dict>"\
"</plist>"

#if 0
#define SLIDESHOW_FEATURES "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"\
"<plist version=\"1.0\">"\
"<dict>"\
"<key>themes</key>"\
"<array>"\
"    <dict>"\
"        <key>key</key>"\
"        <string>KenBurns</string>"\
"        <key>name</key>"\
"        <string>Ken Burns</string>"\
"        <key>transitions</key>"\
"        <array>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>None</string>"\
"                <key>name</key>"\
"                <string>None</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Cube</string>"\
"                <key>name</key>"\
"                <string>Cube</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Dissolve</string>"\
"                <key>name</key>"\
"                <string>Dissolve</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Droplet</string>"\
"                <key>name</key>"\
"                <string>Droplet</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>FadeThruColor</string>"\
"                <key>name</key>"\
"                <string>Fade Through White</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Flip</string>"\
"                <key>name</key>"\
"                <string>Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>TileFlip</string>"\
"                <key>name</key>"\
"                <string>Mosaic Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>MoveIn</string>"\
"                <key>name</key>"\
"                <string>Move In</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>PageFlip</string>"\
"                <key>name</key>"\
"                <string>Page Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Push</string>"\
"                <key>name</key>"\
"                <string>Push</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Reveal</string>"\
"                <key>name</key>"\
"                <string>Reveal</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Twirl</string>"\
"                <key>name</key>"\
"                <string>Twirl</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Wipe</string>"\
"                <key>name</key>"\
"                <string>Wipe</string>"\
"            </dict>"\
"        </array>"\
"    </dict>"\
"    <dict>"\
"        <key>key</key>"\
"        <string>Origami</string>"\
"        <key>name</key>"\
"        <string>Origami</string>"\
"    </dict>"\
"    <dict>"\
"        <key>key</key>"\
"        <string>Reflections</string>"\
"        <key>name</key>"\
"        <string>Reflections</string>"\
"    </dict>"\
"    <dict>"\
"        <key>key</key>"\
"        <string>Snapshots</string>"\
"        <key>name</key>"\
"        <string>Snapshots</string>"\
"    </dict>"\
"    <dict>"\
"        <key>key</key>"\
"        <string>Classic</string>"\
"        <key>name</key>"\
"        <string>Classic</string>"\
"        <key>transitions</key>"\
"        <array>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>None</string>"\
"                <key>name</key>"\
"                <string>None</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                   <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Cube</string>"\
"                <key>name</key>"\
"                <string>Cube</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Dissolve</string>"\
"                <key>name</key>"\
"                <string>Dissolve</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Droplet</string>"\
"                <key>name</key>"\
"                <string>Droplet</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>FadeThruColor</string>"\
"                <key>name</key>"\
"                <string>Fade Through White</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Flip</string>"\
"                <key>name</key>"\
"                <string>Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>TileFlip</string>"\
"                <key>name</key>"\
"                <string>Mosaic Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>MoveIn</string>"\
"                <key>name</key>"\
"                <string>Move In</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>PageFlip</string>"\
"                <key>name</key>"\
"                <string>Page Flip</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Push</string>"\
"                <key>name</key>"\
"                <string>Push</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Reveal</string>"\
"                <key>name</key>"\
"                <string>Reveal</string>"\
"            </dict>"\
"            <dict>"\
"                <key>key</key>"\
"                <string>Twirl</string>"\
"                <key>name</key>"\
"                <string>Twirl</string>"\
"            </dict>"\
"            <dict>"\
"                <key>directions</key>"\
"                <array>"\
"                    <string>up</string>"\
"                    <string>down</string>"\
"                    <string>left</string>"\
"                    <string>down</string>"\
"                </array>"\
"                <key>key</key>"\
"                <string>Wipe</string>"\
"                <key>name</key>"\
"                <string>Wipe</string>"\
"            </dict>"\
"        </array>"\
"    </dict>"\
"</array>"\
"</dict>"\
"</plist>"
#endif

#define SERVER_INFO  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>"\
"<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">"\
"<plist version=\"1.0\">"\
"<dict>"\
"<key>deviceid</key>"\
"<string>58:55:CA:06:BD:9E</string>"\
"<key>features</key>"\
"<integer>119</integer>"\
"<key>model</key>"\
"<string>AppleTV2,1</string>"\
"<key>protovers</key>"\
"<string>1.0</string>"\
"<key>srcvers</key>"\
"<string>101.28</string>"\
"</dict>"\
"</plist>"

bool CAirPlayServer::StartServer(int port, bool nonlocal)
{
  StopServer(true);

  ServerInstance = new CAirPlayServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    CNetworkInterfacePtr net = g_application.getNetwork().GetInterfaceByName("eth0");
    if (!net.get())
      net = g_application.getNetwork().GetInterfaceByName("en0");

    CStdString macAddress;

    if (net.get())
    {
      macAddress = net->GetMacAddress();
    }
    else
    {
      macAddress = "00:01:02:03:04:05";
    }

    std::map<std::string, std::string> txt;
    txt["deviceid"] = macAddress.c_str();
    txt["features"] = "0x77";
    txt["model"] = "AppleTV2,1";
    txt["srcvers"] = "101.28";
    CZeroconf::GetInstance()->PublishService("servers.airplay", "_airplay._tcp", "Boxee", port, txt);

    ServerInstance->Create();
    return true;
  }
  else
    return false;
}

void CAirPlayServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    CZeroconf::GetInstance()->RemoveService("servers.airplay");

    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }
  }
}

CAirPlayServer::CAirPlayServer(int port, bool nonlocal)
{
  m_port = port;
  m_nonlocal = nonlocal;
  m_ServerSocket = -1;
}

void CAirPlayServer::Process()
{
  m_bStop = false;

  while (!m_bStop)
  {
    int             max_fd = 0;
    fd_set          rfds;
    struct timeval  to     = {1, 0};
    FD_ZERO(&rfds);

    FD_SET(m_ServerSocket, &rfds);
    max_fd = m_ServerSocket;

    for (unsigned int i = 0; i < m_connections.size(); i++)
    {
      FD_SET(m_connections[i].m_socket, &rfds);
      if (m_connections[i].m_socket > max_fd)
        max_fd = m_connections[i].m_socket;
    }

    int res = select(max_fd+1, &rfds, NULL, NULL, &to);
    if (res < 0)
    {
      CLog::Log(LOGERROR, "AIRPLAY Server: Select failed: %s", strerror(errno));
      Sleep(1000);
      Initialize();
    }
    else if (res > 0)
    {
      for (int i = m_connections.size() - 1; i >= 0; i--)
      {
        int socket = m_connections[i].m_socket;
        if (FD_ISSET(socket, &rfds))
        {
          char buffer[RECEIVEBUFFER] = {};
          int  nread = 0;
          nread = recv(socket, (char*)&buffer, RECEIVEBUFFER, 0);
          if (nread > 0)
          {
            m_connections[i].PushBuffer(this, buffer, nread);
          }
          if (nread <= 0)
          {
            CLog::Log(LOGINFO, "AIRPLAY Server: Disconnection detected");
            m_connections[i].Disconnect();
            m_connections.erase(m_connections.begin() + i);
          }
        }
      }

      if (FD_ISSET(m_ServerSocket, &rfds))
      {
        CLog::Log(LOGINFO, "AIRPLAY Server: New connection detected");
        CTCPClient newconnection;
        newconnection.m_socket = accept(m_ServerSocket, &newconnection.m_cliaddr, &newconnection.m_addrlen);

        if (newconnection.m_socket < 0)
          CLog::Log(LOGERROR, "AIRPLAY Server: Accept of new connection failed: %s", strerror(errno));
        else
        {
          CLog::Log(LOGINFO, "AIRPLAY Server: New connection added");
          m_connections.push_back(newconnection);
        }
      }
    }
  }

  Deinitialize();
}

bool CAirPlayServer::Initialize()
{
  Deinitialize();

  struct sockaddr_in myaddr;

  myaddr.sin_family = AF_INET;
  myaddr.sin_port = htons(m_port);

  if (m_nonlocal)
    myaddr.sin_addr.s_addr = INADDR_ANY;
  else
    inet_pton(AF_INET, "127.0.0.1", &myaddr.sin_addr.s_addr);

  m_ServerSocket = socket(PF_INET, SOCK_STREAM, 0);
  int opt = 1;
  setsockopt(m_ServerSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof (opt));

  if (m_ServerSocket < 0)
  {

    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to create serversocket");
    return false;
  }

  if (bind(m_ServerSocket, (struct sockaddr*)&myaddr, sizeof myaddr) < 0)
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to bind serversocket");
    close(m_ServerSocket);
    return false;
  }
  
  if (listen(m_ServerSocket, 10) < 0)
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: Failed to set listen");
    close(m_ServerSocket);
    return false;
  }

  CLog::Log(LOGINFO, "AIRPLAY Server: Successfully initialized");
  return true;
}

void CAirPlayServer::Deinitialize()
{
  for (unsigned int i = 0; i < m_connections.size(); i++)
    m_connections[i].Disconnect();

  m_connections.clear();

  if (m_ServerSocket > 0)
  {
    shutdown(m_ServerSocket, SHUT_RDWR);
    close(m_ServerSocket);
    m_ServerSocket = -1;
  }
}

CAirPlayServer::CTCPClient::CTCPClient()
{
  m_socket = -1;
  m_httpParser = new HttpParser();

  m_addrlen = sizeof(struct sockaddr);
}

CAirPlayServer::CTCPClient::CTCPClient(const CTCPClient& client)
{
  Copy(client);
}

CAirPlayServer::CTCPClient& CAirPlayServer::CTCPClient::operator=(const CTCPClient& client)
{
  Copy(client);
  return *this;
}

void CAirPlayServer::CTCPClient::PushBuffer(CAirPlayServer *host, const char *buffer, int length)
{
  HttpParser::status_t status = m_httpParser->addBytes(buffer, length);

  if (status == HttpParser::Done)
  {
    // Parse the request
    CStdString responseHeader;
    CStdString responseBody;
    int status = ProcessRequest(responseHeader, responseBody);
    CStdString statusMsg = "OK";
    if (status == AIRPLAY_STATUS_NOT_IMPLEMENTED) statusMsg = "Not Implemented";
    else if (status == AIRPLAY_STATUS_SWITCHING_PROTOCOLS) statusMsg = "Switching Protocols";

    // Prepare the response
    CStdString response;
    const time_t ltime = time(NULL);
    char *date = asctime(gmtime(&ltime)); //Fri, 17 Dec 2010 11:18:01 GMT;
    date[strlen(date) - 1] = '\0'; // remove \n
    response.Format("HTTP/1.1 %d %s\nDate: %s\r\n", status, statusMsg.c_str(), date);
    if (responseHeader.size() > 0) response += responseHeader;
    if (responseBody.size() > 0)
    {
      response.Format("%sContent-Length: %d\r\n", response.c_str(), responseBody.size());
    }
    response += "\r\n";
    if (responseBody.size() > 0)
    {
      response += responseBody;
    }

    // Send the response
    send(m_socket, response.c_str(), response.size(), 0);

    // We need a new parser...
    delete m_httpParser;
    m_httpParser = new HttpParser;
  }
}

void CAirPlayServer::CTCPClient::Disconnect()
{
  if (m_socket > 0)
  {
    CSingleLock lock (m_critSection);
    shutdown(m_socket, SHUT_RDWR);
    close(m_socket);
    m_socket = -1;
    if (m_httpParser)
      delete m_httpParser;
  }
}

void CAirPlayServer::CTCPClient::Copy(const CTCPClient& client)
{
  m_socket            = client.m_socket;
  m_cliaddr           = client.m_cliaddr;
  m_addrlen           = client.m_addrlen;
  m_httpParser        = client.m_httpParser;
}

int CAirPlayServer::CTCPClient::ProcessRequest(CStdString& responseHeader, CStdString& responseBody)
{
  CStdString method = (m_httpParser->getMethod() ? m_httpParser->getMethod() : "");
  CStdString uri = (m_httpParser->getUri() ? m_httpParser->getUri() : "");
  CStdString queryString = (m_httpParser->getQueryString() ? m_httpParser->getQueryString() : "");
  CStdString body = (m_httpParser->getBody() ? m_httpParser->getBody() : "");
  CStdString contentType = (m_httpParser->getValue("content-type") ? m_httpParser->getValue("content-type") : "");

  int status = AIRPLAY_STATUS_OK;

  int startQs = uri.Find('?');
  if (startQs != -1)
  {
    uri = uri.Left(startQs);
  }

//  printf("method = %s uri = %s ct = %s qs = %s\n body=%s", method.c_str(), uri.c_str(), contentType.c_str(), queryString.c_str(), body.c_str());

  // The reverse command is the first command sent by Airplay,
  // it's a handshake.
  if (uri == "/reverse")
  {
    status = AIRPLAY_STATUS_SWITCHING_PROTOCOLS;
    responseHeader = "Upgrade: PTTH/1.0\r\nConnection: Upgrade\r\n";
  }

  // The rate command is used to play/pause media.
  // A value argument should be supplied which indicates media should be played or paused.
  // 0.000000 => pause
  // 1.000000 => play
  else if (uri == "/rate")
  {
      const char* found = strstr(queryString.c_str(), "value=");
      int rate = found ? (int)(atof(found + strlen("value=")) + 0.5f) : 0;

      if (rate == 0)
      {
        if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && !g_application.m_pPlayer->IsPaused())
        {
          g_application.getApplicationMessenger().MediaPause();
        }
      }
      else
      {
        if (g_application.m_pPlayer && g_application.m_pPlayer->IsPlaying() && g_application.m_pPlayer->IsPaused())
        {
          g_application.getApplicationMessenger().MediaPause();
        }
      }
  }

  // Contains a header like format in the request body which should contain a
  // Content-Location and optionally a Start-Position
  else if (uri == "/play")
  {
    CStdString location;
    float position = 0.0;

    if (contentType == "application/x-apple-binary-plist")
    {
      plist_t plist = NULL;
      plist_from_bin(m_httpParser->getBody(), m_httpParser->getContentLength(), &plist);
      if (plist)
      {
        plist_t node;

        node = plist_dict_get_item(plist, "Start-Position");
        if (node)
        {
          double val;
          plist_get_real_val(node, &val);
          position = val;
        }

        node = plist_dict_get_item(plist, "Content-Location");
        if (node)
        {
          char* val = NULL;
          plist_get_string_val(node, &val);
          if (val)
            location = val;
        }
      }
      else
      {
        CLog::Log(LOGERROR, "Error parsing plist");
      }
    }
    else
    {
      // Get URL to play
      int start = body.Find("Content-Location: ");
      if (start == -1)
        return AIRPLAY_STATUS_NOT_IMPLEMENTED;
      start += strlen("Content-Location: ");
      int end = body.Find('\n', start);
      location = body.Mid(start, end - start);

      start = body.Find("Start-Position");
      if (start != -1)
      {
        start += strlen("Start-Position: ");
        int end = body.Find('\n', start);
        CStdString positionStr = body.Mid(start, end - start);
        position = atof(positionStr.c_str());
      }
    }

    CURI url(location);
    CStdString fileName = url.GetFileName();
    if (CUtil::GetExtension(fileName) == ".m3u8")
    {
      CUtil::URLEncode(location);
      location = CStdString("playlist://") + location;
    }

    CFileItem fileToPlay(location, false);
    fileToPlay.SetProperty("StartPercent", position);

    if (CUtil::GetExtension(fileName) != ".m3u8")
    {
      fileToPlay.SetContentType("video/mp4");
      fileToPlay.SetExtraInfo("User-Agent: iTunes-AppleTV/4.1");

      if(location.Find("trailers.apple.com") != -1)
      {
        CStdString width;
        std::map<CStdString, CStdString> optionMap = url.GetOptionsAsMap();

        width = optionMap["width"];

        if(atoi(width.c_str()) < 720)
        {
          BOXEE::BXCurl curl;
          CStdStringArray urlsArray;
          std::string strTrailers;
          CStdString matchedUrl;

          strTrailers = curl.HttpGetString(location.c_str());

          StringUtils::SplitString(strTrailers, "http://", urlsArray);

          for(int i=0; i<urlsArray.size(); i++)
          {
            if(urlsArray[i].Find("h" + width) != -1)
            {
              matchedUrl.Format("http://%s", urlsArray[i]);
              break;
            }
          }

          if(matchedUrl.IsEmpty())
            matchedUrl = urlsArray[urlsArray.size() -1];

          fileToPlay.m_strPath = matchedUrl;
        }

        fileToPlay.SetExtraInfo("User-Agent: QuickTime/7.6.2");
      }
      else
      {
        std::map<CStdString, CStdString> optionMap = url.GetOptionsAsMap();
        if(!optionMap["bxs"].IsEmpty() && !optionMap["at"].IsEmpty() && !optionMap["rq"].IsEmpty())
        {
          fileToPlay.m_strPath = url.GetUrlWithoutOptions();
        }
      }
    }

    g_application.getApplicationMessenger().MediaPlay(fileToPlay);
  }

  // Used to perform seeking (POST request) and to retrieve current player position (GET request).
  else if (uri == "/scrub")
  {
    if (method == "GET")
    {
      if (g_application.m_pPlayer && g_application.m_pPlayer->GetTotalTime())
      {
        float position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
        responseBody.Format("duration: %d\r\nposition: %f", g_application.m_pPlayer->GetTotalTime(), position);
      }
    }
    else
    {
      const char* found = strstr(queryString.c_str(), "position=");
      if (found && g_application.m_pPlayer)
      {
        __int64 position = (__int64) (atof(found + strlen("position=")) * 1000.0);
        g_application.m_pPlayer->SeekTime(position);
      }
    }
  }

  // Sent when media playback should be stopped
  else if (uri == "/stop")
  {
    g_application.getApplicationMessenger().MediaStop();
  }

  // RAW JPEG data is contained in the request body
  else if (uri == "/photo")
  {
    if (m_httpParser->getContentLength() > 0)
    {
      FILE *fp = fopen("/tmp/airplay_photo.jpg", "wb");
      if (fp)
      {
        fwrite(m_httpParser->getBody(), 1, m_httpParser->getContentLength(), fp);
        fclose(fp);
        g_application.getApplicationMessenger().PictureShow("/tmp/airplay_photo.jpg");
      }
    }
  }

  else if (uri == "/playback-info")
  {
    float position = 0.0f;
    float duration = 0.0f;
    bool playing = false;

    if (g_application.m_pPlayer && g_application.m_pPlayer->GetTotalTime())
    {
      playing = true;
      position = ((float) g_application.m_pPlayer->GetTime()) / 1000;
      duration = (float) g_application.m_pPlayer->GetTotalTime();
    }

    responseBody.Format(PLAYBACK_INFO, duration, duration, position, (playing ? 1 : 0), duration);
    responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
  }

  else if (uri == "/server-info")
  {
    responseBody = SERVER_INFO;
    responseHeader = "Content-Type: text/x-apple-plist+xml\r\n";
  }

  else if (uri == "/slideshow-features")
  {
    // Ignore for now.
  }

  else if (uri == "/authorize")
  {
    // DRM, ignore for now.
  }

  else if (uri == "/setProperty")
  {
    // Ignore for now
  }

  else if (uri == "/getProperty")
  {
    // Ignore for now
  }

  else
  {
    CLog::Log(LOGERROR, "AIRPLAY Server: unhandled request [%s]\n", uri.c_str());
    status = AIRPLAY_STATUS_NOT_IMPLEMENTED;
  }

  return status;
}

#endif
