
#include "XAPP_ToggleButton.h"
#include "GUIInfoManager.h"
#include "GUIToggleButtonControl.h"
#include "Application.h"
#include "GUIWindowManager.h"

namespace XAPP
{

ToggleButton::ToggleButton(int windowId, int controlId) throw (AppException) : Control(windowId, controlId)
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (type != CGUIControl::GUICONTROL_TOGGLEBUTTON)
      throw AppException("Control is not a ToggleButton");

//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (!control || control->GetControlType() != CGUIControl::GUICONTROL_TOGGLEBUTTON)
//    {
//      throw AppException("Control is not a ToggleButton");
//    }
//  }
    }

void ToggleButton::SetLabel(const std::string& label)
{
  std::vector<CStdString> vec;
  ThreadMessage tMsg ( TMSG_SET_CONTROL_LABEL, m_windowId, m_controlId, label, vec, "", NULL, NULL );
  g_application.getApplicationMessenger().SendMessage(tMsg, true); 
}

void ToggleButton::SetSelected(bool selected)
{
  if (selected)
  {
    CGUIMessage message(GUI_MSG_SELECTED, m_windowId, m_controlId);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, true);
  }
  else
  {
    CGUIMessage message(GUI_MSG_DESELECTED, m_windowId, m_controlId);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, true);
  }
}

bool ToggleButton::IsSelected()
{
  CGUIMessage msg(GUI_MSG_ITEM_SELECTED, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  if (msg.GetParam1() == 1)
    return true;

  return false;

//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (control)
//    {
//      return ((CGUIToggleButtonControl*)control)->IsSelected();
//    }
//  }
//  return false;
}

}
