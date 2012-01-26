
#include "XAPP_Edit.h"
#include "Application.h"
#include "GUIEditControl.h"
#include "GUIWindowManager.h"

namespace XAPP
{

Edit::Edit(int windowId, int controlId) throw (AppException) : Control(windowId, controlId) 
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (type != CGUIControl::GUICONTROL_EDIT)
    throw AppException("Control is not a Edit");

//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (!control || control->GetControlType() != CGUIControl::GUICONTROL_EDIT)
//    {
//      throw AppException("Control is not an Edit");
//    }
//  }
    }

void Edit::SetText(const std::string& text)
{
  CGUIMessage msg(GUI_MSG_LABEL2_SET, m_windowId, m_controlId);
  msg.SetLabel(text);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (control)
//    {
//      ((CGUIEditControl*)control)->SetLabel2(text);
//    }
//  }
    }

std::string Edit::GetText()
{
  CGUIMessage msg(GUI_MSG_GET_LABEL2, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  return msg.GetLabel();

//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (control)
//    {
//      return ((CGUIEditControl*)control)->GetLabel2();
//    }
//  }
//  return "";
    }

}
