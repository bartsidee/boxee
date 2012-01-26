
#include "XAPP_Control.h"
#include "GUIMessage.h"
#include "GraphicContext.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "GUIWindowApp.h"

namespace XAPP
{
Control::Control(int windowId, int controlId) throw (AppException)
{
  m_windowId = windowId;
  m_controlId = controlId;
}

void Control::SetFocus()
{
    CGUIMessage msg(GUI_MSG_SETFOCUS, m_windowId, m_controlId, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
}

bool Control::HasFocus()
{
  CGUIMessage msg(GUI_MSG_GET_FOCUS, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  if (msg.GetParam1() == 1)
    return true;

  return false;

}

void Control::SetVisible(bool enabled)
{
  if (enabled)
  {
    CGUIMessage msg(GUI_MSG_VISIBLE, m_windowId, m_controlId, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_HIDDEN, m_windowId, m_controlId, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
  }
}

bool Control::IsVisible()
{
  CGUIMessage msg(GUI_MSG_GET_VISIBLE, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  if (msg.GetParam1() == 1)
    return true;

  return false;
}

void Control::SetEnabled(bool enabled)
{
  if (enabled)
  {
    CGUIMessage msg(GUI_MSG_ENABLED, m_windowId, m_controlId, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
  }
  else
  {
    CGUIMessage msg(GUI_MSG_DISABLED, m_windowId, m_controlId, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);
  }
}

bool Control::IsEnabled()
{
  CGUIMessage msg(GUI_MSG_GET_ENABLED, m_windowId, m_controlId);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(msg, m_windowId, true);

  if (msg.GetParam1() == 1)
    return true;

  return false;
}

/**
 * Get window id
 */
int Control::GetWindowId()
{
  return m_windowId;
}

/**
 * Get control id
 */
int Control::GetControlId()
{
  return m_controlId;
}

void Control::AddActionListener(ActionListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_windowId);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->AddActionListener(listener);
}

void Control::RemoveActionListener(ActionListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_windowId);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->RemoveActionListener(listener);
}

ActionListener::~ActionListener()
{
}

}
