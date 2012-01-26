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

#include "GUIDialogBoxeeEject.h"
#define CONTROL_HEADING       340
#define CONTROL_LIST          341

CGUIDialogBoxeeEject::CGUIDialogBoxeeEject(void)
    : CGUIDialogSelect(WINDOW_DIALOG_BOXEE_EJECT,"boxee_dialog_eject.xml")
{
  m_bWasSelected = false;
}

CGUIDialogBoxeeEject::~CGUIDialogBoxeeEject(void)
{
}

bool CGUIDialogBoxeeEject::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      if (CONTROL_LIST == iControl)
      {
        m_bWasSelected = true;
      }
    }
    break;
  }

  return CGUIDialogSelect::OnMessage(message);
}
