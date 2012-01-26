
#include "XAPP_Label.h"
#include "Application.h"
#include "GUIWindowManager.h"

namespace XAPP
{

Label::Label(int windowId, int controlId) throw (AppException) : Control(windowId, controlId)
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (type != CGUIControl::GUICONTROL_LABEL)
      throw AppException("Control is not a Label");

//  // TODO: I am not sure this is necessary at all ??
//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (!control || control->GetControlType() != CGUIControl::GUICONTROL_LABEL)
//    {
//      throw AppException("Control is not a Label");
//    }
//  }
    }

void Label::SetLabel(const std::string& label)
{
  std::vector<CStdString> vec;
  ThreadMessage tMsg ( TMSG_SET_CONTROL_LABEL, m_windowId, m_controlId, label, vec, "", NULL, NULL );
  g_application.getApplicationMessenger().SendMessage(tMsg, false); 
}

std::string Label::GetLabel()
{
  CGUIMessage msg(GUI_MSG_GET_LABEL, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  return msg.GetLabel();
}

}
