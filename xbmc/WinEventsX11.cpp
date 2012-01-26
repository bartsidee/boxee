/*
*      Copyright (C) 2005-2008 Team XBMC
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

#include "system.h"
#include "WinEvents.h"
#include "Application.h"
#include "XBMC_vkeys.h"
#include "MouseStat.h"
#include "WindowingFactory.h"

#ifdef HAS_X11

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

static bool GetKeyboardEvent(XEvent event, XBMC_Event& xbmcEvent)
{
  KeySym keySym = XLookupKeysym(&event.xkey, 0);

  if (keySym > 127)
  {
    switch (keySym)
    {
    case XK_Escape: xbmcEvent.key.keysym.sym = XBMCK_ESCAPE; break;
    case XK_BackSpace: xbmcEvent.key.keysym.sym = XBMCK_BACKSPACE; break;
    case XK_KP_Tab: case XK_Tab: xbmcEvent.key.keysym.sym = XBMCK_TAB; break;
    case XK_Return: xbmcEvent.key.keysym.sym = XBMCK_RETURN; break;
    case XK_Delete: xbmcEvent.key.keysym.sym = XBMCK_DELETE; break;
    case XK_KP_Home: case XK_Home: xbmcEvent.key.keysym.sym = XBMCK_HOME; break;
    case XK_KP_Left: case XK_Left: xbmcEvent.key.keysym.sym = XBMCK_LEFT; break;
    case XK_KP_Up: case XK_Up: xbmcEvent.key.keysym.sym = XBMCK_UP; break;
    case XK_KP_Right: case XK_Right: xbmcEvent.key.keysym.sym = XBMCK_RIGHT; break;
    case XK_KP_Page_Up: case XK_Page_Up: xbmcEvent.key.keysym.sym = XBMCK_PAGEUP; break;
    case XK_KP_Page_Down: case XK_Page_Down: xbmcEvent.key.keysym.sym = XBMCK_PAGEDOWN; break;
    case XK_KP_Down: case XK_Down: xbmcEvent.key.keysym.sym = XBMCK_DOWN; break;
    case XK_KP_End: case XK_End: xbmcEvent.key.keysym.sym = XBMCK_END; break;
    case XK_KP_Enter: xbmcEvent.key.keysym.sym = XBMCK_KP_ENTER; break;
    case XK_KP_Equal: xbmcEvent.key.keysym.sym = XBMCK_KP_EQUALS; break;
    case XK_KP_Multiply: xbmcEvent.key.keysym.sym = XBMCK_KP_MULTIPLY; break;
    case XK_KP_Add: xbmcEvent.key.keysym.sym = XBMCK_KP_PLUS; break;
    case XK_KP_Subtract: xbmcEvent.key.keysym.sym = XBMCK_KP_MINUS; break;
    case XK_KP_Divide: xbmcEvent.key.keysym.sym = XBMCK_KP_DIVIDE; break;
    case XK_F1: xbmcEvent.key.keysym.sym = XBMCK_F1; break;
    case XK_F2: xbmcEvent.key.keysym.sym = XBMCK_F2; break;
    case XK_F3: xbmcEvent.key.keysym.sym = XBMCK_F3; break;
    case XK_F4: xbmcEvent.key.keysym.sym = XBMCK_F4; break;
    case XK_F5: xbmcEvent.key.keysym.sym = XBMCK_F5; break;
    case XK_F6: xbmcEvent.key.keysym.sym = XBMCK_F6; break;
    case XK_F7: xbmcEvent.key.keysym.sym = XBMCK_F7; break;
    case XK_F8: xbmcEvent.key.keysym.sym = XBMCK_F8; break;
    case XK_F9: xbmcEvent.key.keysym.sym = XBMCK_F9; break;
    case XK_F10: xbmcEvent.key.keysym.sym = XBMCK_F10; break;
    case XK_F11: xbmcEvent.key.keysym.sym = XBMCK_F11; break;
    case XK_F12: xbmcEvent.key.keysym.sym = XBMCK_F12; break;
    case XK_Menu: xbmcEvent.key.keysym.sym = XBMCK_MENU; break;
    case XK_Control_L: xbmcEvent.key.keysym.sym = XBMCK_LCTRL; break;
    case XK_Control_R: xbmcEvent.key.keysym.sym = XBMCK_RCTRL; break;
    case XK_Shift_L: xbmcEvent.key.keysym.sym = XBMCK_LSHIFT; break;
    case XK_Shift_R: xbmcEvent.key.keysym.sym = XBMCK_RSHIFT; break;
    case XK_Meta_L: xbmcEvent.key.keysym.sym = XBMCK_LMETA; break;
    case XK_Meta_R: xbmcEvent.key.keysym.sym = XBMCK_RMETA; break;
    case XK_Alt_L: xbmcEvent.key.keysym.sym = XBMCK_LALT; break;
    case XK_Alt_R: xbmcEvent.key.keysym.sym = XBMCK_RALT; break;
    default: xbmcEvent.key.keysym.sym = XBMCK_UNKNOWN; break;
    }
  }
  else
  {
    xbmcEvent.key.keysym.sym = (XBMCKey) keySym;
    xbmcEvent.key.keysym.unicode = keySym;
  }

  return true;
}

static bool GetMouseButtonEvent(XEvent event, XBMC_Event& xbmcEvent)
{
  xbmcEvent.button.x = event.xbutton.x;
  xbmcEvent.button.y = event.xbutton.y;
  xbmcEvent.button.button = event.xbutton.button;

  return true;
}

static bool GetMouseMotionEvent(XEvent event, XBMC_Event& xbmcEvent)
{
  xbmcEvent.motion.x = event.xmotion.x;
  xbmcEvent.motion.y = event.xmotion.y;
  xbmcEvent.motion.state = 0;
  return true;
}

bool CWinEventsX11::MessagePump()
{ 
  Display* display = g_Windowing.GetDisplay();
  XEvent event;
  bool ret = false;
  
  while (XPending(display) > 0)
  {
    XNextEvent(display, &event);
    switch(event.type)
    {    
    case ClientMessage:
      if (event.xclient.data.l[0] == XInternAtom(display, "WM_DELETE_WINDOW", False))
      {
        if (!g_application.m_bStop) g_application.getApplicationMessenger().Quit();
      }
      break;

    case FocusIn:
      g_application.m_AppFocused = false;
      g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
      break;

    case FocusOut:
      g_application.m_AppFocused = true;
      g_Windowing.NotifyAppFocusChange(g_application.m_AppFocused);
      break;
      
    case VisibilityNotify:
      g_application.m_AppActive = (event.xvisibility.state != VisibilityFullyObscured);
      g_Windowing.NotifyAppActiveChange(g_application.m_AppActive);
      break;

    case KeyPress:
    {
      // process any platform specific shortcuts before handing off to XBMC
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(XBMC_Event));
      if (!GetKeyboardEvent(event, newEvent))
        continue;      //newEvent.resize.type = event.resize.type;
      newEvent.type = XBMC_KEYDOWN;

      // don't handle any more messages in the queue until we've handled keydown,
      // if a keyup is in the queue it will reset the keypress before it is handled.
      ret |= g_application.OnEvent(newEvent);
      break;
    }
      
    case KeyRelease:
    {
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(XBMC_Event));
      if (!GetKeyboardEvent(event, newEvent))
        continue;
      newEvent.type = XBMC_KEYUP;

      ret |= g_application.OnEvent(newEvent);
      break;
    }
    
    case ButtonPress:
    {
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(XBMC_Event));
      if (!GetMouseButtonEvent(event, newEvent))
        continue;
      newEvent.button.state = XBMC_PRESSED;
      newEvent.type = XBMC_MOUSEBUTTONDOWN;
      ret |= g_application.OnEvent(newEvent);
      break;
    }

    case ButtonRelease:
    {
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(XBMC_Event));
      if (!GetMouseButtonEvent(event, newEvent))
        continue;
      newEvent.button.state = XBMC_RELEASED;
      newEvent.type = XBMC_MOUSEBUTTONUP;
      ret |= g_application.OnEvent(newEvent);
      break;
    }

    case MotionNotify:
    {
      XBMC_Event newEvent;
      memset(&newEvent, 0, sizeof(XBMC_Event));
      if (!GetMouseMotionEvent(event, newEvent))
        continue;
      newEvent.type = XBMC_MOUSEMOTION;
      ret |= g_application.OnEvent(newEvent);
      break;
    }

    case ConfigureNotify:
    {
      XBMC_Event newEvent;
      newEvent.type = XBMC_VIDEORESIZE;
      newEvent.resize.w = event.xconfigure.width;
      newEvent.resize.h = event.xconfigure.height;
      ret |= g_application.OnEvent(newEvent);
      break;
    }
    }
  }
  
  return ret;
}

#endif
