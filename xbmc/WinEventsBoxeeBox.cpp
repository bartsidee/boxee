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

#if defined(_LINUX) && !defined(__APPLE__) && defined(HAS_EMBEDDED)

#include "WinEventsBoxeeBox.h"
#include "WinEvents.h"
#include "XBMC_events.h"
#include "XBMC_keysym.h"
#include "Application.h"
#include "MouseStat.h"
#include "log.h"
#include "common/LinuxInputDevices.h"

PHANDLE_EVENT_FUNC CWinEventsBase::m_pEventFunc = NULL;

bool m_initialized = false;
CLinuxInputDevices m_devices;

CWinEventsBoxeeBox::CWinEventsBoxeeBox()
{
}

static void InitLinuxDevices()
{
  if (m_initialized) return;

  m_devices.InitAvailable();

  m_initialized = true;
}

bool CWinEventsBoxeeBox::MessagePump()
{
  InitLinuxDevices();

  bool ret = false;
  XBMC_Event event;
  while (1)
  {
    event = m_devices.ReadEvent();
    if (event.type != XBMC_NOEVENT)
    {
      ret |= g_application.OnEvent(event);
    }
    else
    {
      break;
    }
  }

  return ret;
}

#endif
