/*
 * Many concepts and protocol specification in this code are taken
 * from Shairport, by James Laird.
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

#include "AirTunesServer.h"

#ifdef HAS_AIRTUNES

#include "log.h"
#include "StdString.h"
#include "Zeroconf.h"
#include "lib/shairport/shairport.h"
#include "Application.h"

CAirTunesServer *CAirTunesServer::ServerInstance = NULL;
CStdString CAirTunesServer::m_macAddress;

bool CAirTunesServer::StartServer(int port, bool nonlocal)
{
  bool success = false;

  CNetworkInterfacePtr net = g_application.getNetwork().GetInterfaceByName("eth0");
  if (!net.get())
    net = g_application.getNetwork().GetInterfaceByName("en0");

  if (net.get())
  {
    m_macAddress = net->GetMacAddress();
    m_macAddress.Replace(":","");

    while (m_macAddress.size() < 12)
    {
      m_macAddress = CStdString("0") + m_macAddress;
    }
  }
  else
  {
    m_macAddress = "000102030405";
  }

  ServerInstance = new CAirTunesServer(port, nonlocal);
  if (ServerInstance->Initialize())
  {
    ServerInstance->Create();
    success = true;
  }

  if (success)
  {
    CStdString appName;
    appName.Format("%s@Boxee", m_macAddress.c_str());

    std::map<std::string, std::string> txt;
    txt["cn"] = "0,1";
    txt["ch"] = "2";
    txt["ek"] = "1";
    txt["et"] = "0,1";
    txt["sv"] = "false";
    txt["tp"] = "UDP";
    txt["sm"] = "false";
    txt["ss"] = "16";
    txt["sr"] = "44100";
    txt["pw"] = "false";
    txt["vn"] = "3";
    txt["txtvers"] = "1";

    CZeroconf::GetInstance()->PublishService("servers.airtunes", "_raop._tcp", appName, port, txt);
  }

  return success;
}

void CAirTunesServer::StopServer(bool bWait)
{
  if (ServerInstance)
  {
    ServerInstance->Deinitialize();
    ServerInstance->StopThread(bWait);
    if (bWait)
    {
      delete ServerInstance;
      ServerInstance = NULL;
    }

    CZeroconf::GetInstance()->RemoveService("servers.airtunes");
  }
}

CAirTunesServer::CAirTunesServer(int port, bool nonlocal)
{
  m_port = port;
}

void CAirTunesServer::Process()
{
  m_bStop = false;

  while (!m_bStop && shairport_is_running())
  {
    shairport_loop();
  }
}

bool CAirTunesServer::Initialize()
{
  Deinitialize();

  CStdString hwStr;
  hwStr.Format("--mac=%s", m_macAddress.c_str());
  const char* argv[] = { "--apname=boxee", "--server_port=5000", (const char*) hwStr.c_str(), NULL };
  shairport_main(2, (char**) argv);

  return true;
}

void CAirTunesServer::Deinitialize()
{
  shairport_exit();
}

#endif
