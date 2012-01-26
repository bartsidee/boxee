
#include "GUIWindowManager.h"
#include "XAPP_Window.h"
#include "Application.h"
#include "GUIWindowApp.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIUserMessages.h"
#include "GUISettings.h"

namespace XAPP
{

Window::Window(int id) throw (AppException)
{
  m_window = g_windowManager.GetWindow(id);
  if (m_window == NULL)
  {
    std::string s = "Window not found: ";
    s += id;
    throw AppException(s.c_str());
  }
  
  m_id = id;
}

Control Window::GetControl(int id) throw (AppException)
{
  return Control(m_id, id);
}

Label Window::GetLabel(int id) throw (AppException)
{
  return Label(m_id, id);  
}

Textbox Window::GetTextbox(int id) throw (AppException)
{
  return Textbox(m_id, id);
}

List Window::GetList(int id) throw (AppException)
{
  return List(m_id, id);  
}

ToggleButton Window::GetToggleButton(int id) throw (AppException)
{
  return ToggleButton(m_id, id);
}

Button Window::GetButton(int id) throw (AppException)
{
  return Button(m_id, id);
}

Image Window::GetImage(int id) throw (AppException)
{
  return Image(m_id, id);
}

Edit Window::GetEdit(int id) throw (AppException)
{
  return Edit(m_id, id);
}

void Window::PushState()
{
  CGUIMessage message(GUI_MSG_SAVE_STATE, m_id, 0);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_id, true);
}

void Window::PopState(bool restoreState)
{
  if (restoreState)
  {
    CGUIMessage message(GUI_MSG_RESTORE_STATE, m_id, 0, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_id, true);      
  }
  else
  {
    CGUIMessage message(GUI_MSG_RESET_STATE, m_id, 0, 0);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_id, true);          
  }
}

void Window::PopToState(int remainInStack)
{
  CGUIMessage message(GUI_MSG_RESET_STATE, m_id, 0, remainInStack);
  g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_id, true);          
}

void Window::ClearStateStack(bool restoreState)
{
  if (restoreState)
  {
    CGUIMessage message(GUI_MSG_RESTORE_STATE, m_id, 0, 1);
    g_application.getApplicationMessenger().SendGUIMessageToWindow(message, m_id, true);      
  }
  else
  {
    ((CGUIWindowApp*) m_window)->ClearStateStack();
  }
}

/**
 * Get window id
 */
int Window::GetWindowId()
{
  return m_id;
}

WindowListener::~WindowListener()
{
}

void Window::AddWindowListener(WindowListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_id);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->AddWindowListener(listener);
}

void Window::RemoveWindowListener(WindowListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_id);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->RemoveWindowListener(listener);
}

void Window::AddKeyListener(KeyListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_id);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->AddKeyListener(listener);
}

void Window::RemoveKeyListener(KeyListener* listener)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_id);
  if (!window)
    return;

  ((CGUIWindowApp*) window)->RemoveKeyListener(listener);
}

void Window::SetProperty(const std::string &strKey, const std::string &strValue)
{
  CGUIWindow* window = g_windowManager.GetWindow(m_id);
  if (!window)
    return;

  window->SetProperty(strKey, strValue);
}

}
