/*
 * XAPP_Textbox.cpp
 *
 *  Created on: Aug 4, 2011
 *      Author: shay
 */

#include "XAPP_Textbox.h"
#include "Application.h"
#include "GUIWindowManager.h"

namespace XAPP
{

Textbox::Textbox(int windowId, int controlId) throw (AppException) : Control(windowId, controlId)
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (type != CGUIControl::GUICONTROL_TEXTBOX)
    throw AppException("Control is not a Textbox");
}


void Textbox::SetText(const std::string& text)
{
  std::vector<CStdString> vec;
  ThreadMessage tMsg ( TMSG_SET_CONTROL_LABEL, m_windowId, m_controlId, text, vec, "", NULL, NULL );
  g_application.getApplicationMessenger().SendMessage(tMsg, false);
}

}
