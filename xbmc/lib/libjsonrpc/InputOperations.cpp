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

#include "InputOperations.h"
#include "Application.h"
#include "ApplicationMessenger.h"
#include "utils/log.h"
#include "GUIWindowManager.h"
#include "MouseStat.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CInputOperations::MouseMovement(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  int deltax = parameterObject["deltax"].asInt();
  int deltay = parameterObject["deltay"].asInt();

  ThreadMessage tMsg(TMSG_SEND_MOVE, deltax, deltay);
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::MouseClick(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  if (!g_Mouse.IsEnabled())
    return FailedToExecute;

  ThreadMessage tMsg(TMSG_SEND_KEY, KEY_BUTTON_A);
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::NavigationState(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  result["state"]["keys-enabled"] = true;
  result["state"]["mouse-enabled"] = g_Mouse.IsEnabled();

  result["success"] = true;

  return OK;
}

JSON_STATUS CInputOperations::Left(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg(TMSG_SEND_KEY, KEY_BUTTON_DPAD_LEFT);
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Right(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg( TMSG_SEND_KEY, KEY_BUTTON_DPAD_RIGHT );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Down(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg ( TMSG_SEND_KEY, KEY_BUTTON_DPAD_DOWN );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Up(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg ( TMSG_SEND_KEY, KEY_BUTTON_DPAD_UP );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Select(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg ( TMSG_SEND_KEY, KEY_BUTTON_A );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Back(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  ThreadMessage tMsg ( TMSG_SEND_KEY, KEY_BUTTON_BACK );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
  return ACK;
}

JSON_STATUS CInputOperations::Home(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  g_application.getApplicationMessenger().ActivateWindow(WINDOW_HOME, std::vector<CStdString>(), false);
  return ACK;
}
