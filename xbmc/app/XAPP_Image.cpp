
#include "XAPP_Image.h"
#include "Application.h"
#include "GUIWindowManager.h"

namespace XAPP
{
Image::Image(int windowId, int controlId) throw (AppException) : Control(windowId, controlId)
{
  CGUIMessage msg(GUI_MSG_GET_TYPE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  CGUIControl::GUICONTROLTYPES type = (CGUIControl::GUICONTROLTYPES)msg.GetParam1();

  if (!(type == CGUIControl::GUICONTROL_IMAGE || type == CGUIControl::GUICONTROL_BORDEREDIMAGE || type == CGUIControl::GUICONTROL_LARGE_IMAGE))
    throw AppException("Control is not a Image");


//  CGUIWindow* pWindow = g_windowManager.GetWindow(m_windowId);
//  if (pWindow)
//  {
//    CGUIControl *control = (CGUIControl *)pWindow->GetControl(m_controlId);
//    if (!control ||
//        !(control->GetControlType() == CGUIControl::GUICONTROL_IMAGE ||
//          control->GetControlType() ==  CGUIControl::GUICONTROL_BORDEREDIMAGE ||
//          control->GetControlType() == CGUIControl::GUICONTROL_LARGE_IMAGE))
//    {
//      throw AppException("Control is not an Image");
//    }
//  }
    }

void Image::SetTexture(const std::string& url)
{
  CGUIMessage message(GUI_MSG_SET_FILENAME, m_windowId, m_controlId);
  message.SetStringParam(url);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_windowId, true);
}

}
