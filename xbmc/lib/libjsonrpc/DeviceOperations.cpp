/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "DeviceOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "FileItem.h"
#include "Util.h"
#include "utils/log.h"
#include "GUIDialogBoxeePair.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CDeviceOperations::PairChallenge(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - enter function (hsrv)");

  CStdString deviceId = parameterObject["deviceid"].asString();
  CStdString deviceLabel = parameterObject.get("label","").asString();
  CStdString deviceType = parameterObject.get("type","").asString();
  CStdString deviceIcon = parameterObject.get("icon","").asString();
  CStdString deviceApplicationId = parameterObject.get("applicationid","").asString();

  if (g_application.GetBoxeeDeviceManager().IsPairDevice(deviceId))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to pair device because deviceId already paired. [deviceId=%s] (hsrv)",deviceId.c_str());
    return FailedToExecute;
  }

  CGUIDialogBoxeePair* pairDlg = (CGUIDialogBoxeePair*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAIR);
  if (pairDlg && pairDlg->IsActive())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to handle request. [deviceId=%s] (hsrv)",deviceId.c_str());
    return FailedToExecute;
  }

  CBoxeeDeviceItem* pDeviceItem = new CBoxeeDeviceItem();
  if (!pDeviceItem || !pDeviceItem->Initialize(deviceId, deviceLabel, deviceType, deviceIcon, deviceApplicationId))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - FAILED to initialize BoxeeDeviceItem. [deviceItem=%p][deviceId=%s] (hsrv)",pDeviceItem,deviceId.c_str());
    return FailedToExecute;
  }

  ThreadMessage tMsg(TMSG_SHOW_BOXEE_DEVICE_PAIR_DIALOG);
  tMsg.lpVoid = pDeviceItem;
  g_application.getApplicationMessenger().SendMessage(tMsg, false);

  result["success"] = true;

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingChallengeRequest - exit function. [retCode=OK] (hsrv)");

  return OK;
}

JSON_STATUS CDeviceOperations::PairResponse(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString deviceId = parameterObject["deviceid"].asString();
  CStdString code = parameterObject.get("code","").asString();

  CGUIDialogBoxeePair* pairDlg = (CGUIDialogBoxeePair*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_PAIR);
  if (!pairDlg || !pairDlg->IsActive())
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - FAILED to handle request. [deviceId=%s] (hsrv)",deviceId.c_str());
    return FailedToExecute;
  }

  if (!pairDlg->PairDevice(deviceId, code))
  {
    CLog::Log(LOGERROR,"CBoxeeApiHttpServer::HandlePairingResponseRequest - FAILED to pair device. [deviceId=%s] (hsrv)",deviceId.c_str());
    return FailedToExecute;
  }

  CGUIMessage refreshWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_SETTINGS_DEVICES, 0);
  g_windowManager.SendThreadMessage(refreshWinMsg);

  result["success"] = true;

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingResponseRequest - exit function. [retCode=OK] (hsrv)");

  return OK;
}

JSON_STATUS CDeviceOperations::Unpair(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingUnpairRequest - enter function (hsrv)");

  CStdString deviceId = parameterObject["deviceid"].asString();

  if (!g_application.GetBoxeeDeviceManager().UnPairDevice(deviceId))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - FAILED to unpair device. [deviceId=%s] (hsrv)",deviceId.c_str());
    return FailedToExecute;
  }

  CGUIMessage refreshWinMsg(GUI_MSG_UPDATE, WINDOW_BOXEE_SETTINGS_DEVICES, 0);
  g_windowManager.SendThreadMessage(refreshWinMsg);

  result["success"] = true;

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandlePairingUnpairRequest - exit function. [retCode=OK] (hsrv)");

  return OK;
}

JSON_STATUS CDeviceOperations::Connect(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleConnectRequest - enter function (hsrv)");

  CStdString deviceId = parameterObject["deviceid"].asString();

  if (!g_application.GetBoxeeDeviceManager().IsPairDevice(deviceId))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - FAILED to unpair device. [deviceId=%s] (hsrv)",deviceId.c_str());
    return BadPermission;
  }

  client->SetPermissionFlags(OPERATION_PERMISSION_ALL);
  client->SetAnnouncementFlags(ANNOUNCE_ALL);

  result["success"] = true;

  CLog::Log(LOGDEBUG,"CBoxeeApiHttpServer::HandleConnectRequest - exit function. [retCode=OK] (hsrv)");

  return OK;
}
