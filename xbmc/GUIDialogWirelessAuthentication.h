#ifndef GUI_DIALOG_WIRELESS_AUTHENTICATION
#define GUI_DIALOG_WIRELESS_AUTHENTICATION

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

#pragma once

#include "system.h"

#ifdef HAS_BOXEE_HAL

#include <vector>
#include "GUIDialog.h"
#include "HalServices.h"

class CGUIDialogWirelessAuthentication : public CGUIDialog
{
public:
  CGUIDialogWirelessAuthentication(void);
  virtual ~CGUIDialogWirelessAuthentication(void);
  virtual void OnInitWindow();
  virtual bool OnAction(const CAction &action);

  bool WasItemSelected();

  CHalWirelessAuthType GetAuth();
  void SetAuth(CHalWirelessAuthType auth);

private:
  CHalWirelessAuthType m_auth;
  bool m_wasItemSelected;
};

#endif

#endif

