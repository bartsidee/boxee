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

#include "GUIOperations.h"
#include "ApplicationMessenger.h"
#include "utils/log.h"
#include "GUIWindowManager.h"
#include "GUIUserMessages.h"
#include "GUIEditControl.h"
#include "Application.h"

using namespace Json;
using namespace JSONRPC;

JSON_STATUS CGUIOperations::TextFieldSet(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CStdString text = parameterObject["text"].asString();

  CGUIDialogKeyboard* pKeyboard = GetKeyboard();
  if (!pKeyboard)
    return FailedToExecute;
  else if (!pKeyboard->IsActive())
    return FailedToExecute;

  CGUIMessage message(GUI_MSG_LABEL2_SET, WINDOW_DIALOG_KEYBOARD, CTL_LABEL_EDIT);
  message.SetLabel(text);
  g_windowManager.SendThreadMessage(message);

  result["success"] = true;

  return OK;
}

JSON_STATUS CGUIOperations::KeyboardState(const CStdString &method, ITransportLayer *transport, IClient *client, const Value &parameterObject, Value &result)
{
  CGUIDialogKeyboard* pKeyboard = GetKeyboard();
  if (!pKeyboard)
    return FailedToExecute;

  if (pKeyboard->IsActive())
  {
    result["state"]["displayed"] = pKeyboard->IsActive();

    if (pKeyboard->IsActive())
    {
      result["state"]["password"] = (pKeyboard->IsHiddenInput());

      if (!pKeyboard->IsHiddenInput())
      {
        result["state"]["text"] = (pKeyboard->GetText());
      }
    }
  }
  else
  {
    int focusedWindowId = g_windowManager.GetFocusedWindow();
    CGUIWindow* focusedWindow = g_windowManager.GetWindow(focusedWindowId);
    if (!focusedWindow)
      return FailedToExecute;

    CGUIControl* focusedControl = focusedWindow->GetFocusedControl();
    if (focusedControl)
    {
      CGUIControl::GUICONTROLTYPES controlType = focusedControl->GetControlType();
      if (controlType == CGUIControl::GUICONTROL_EDIT)
      {
        result["state"]["displayed"] = true;

        CGUIEditControl* editControl = (CGUIEditControl*) focusedControl;
        result["state"]["password"] = (editControl->GetInputType() == CGUIEditControl::INPUT_TYPE_PASSWORD);

        if (editControl->GetInputType() != CGUIEditControl::INPUT_TYPE_PASSWORD)
        {
          result["state"]["text"] = editControl->GetLabel2();
        }
      }
      else
      {
        result["state"]["displayed"] = false;
      }
    }
    else
    {
      result["state"]["displayed"] = false;
    }
  }


  result["success"] = true;

  return OK;
}

JSON_STATUS CGUIOperations::NotificationShow(const CStdString &method, ITransportLayer *transport, IClient *client, const Json::Value &parameterObject, Json::Value &result)
{
  CStdString msg = parameterObject["msg"].asString();
  g_application.m_guiDialogKaiToast.QueueNotification("", "", msg, 5000);

  result["success"] = true;

  return OK;
}

CGUIDialogKeyboard* CGUIOperations::GetKeyboard()
{
  CGUIDialogKeyboard* pKeyboard;

  pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SEARCH);

  if (pKeyboard && pKeyboard->IsActive())
  {
    return pKeyboard;
  }

  pKeyboard = (CGUIDialogKeyboard*)g_windowManager.GetWindow(WINDOW_DIALOG_KEYBOARD);

  if (pKeyboard)
  {
    return pKeyboard;
  }

  return NULL;
}
