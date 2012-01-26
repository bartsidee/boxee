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

#include "GUIDialogWirelessAuthentication.h"

#ifdef HAS_BOXEE_HAL

#include "Application.h"
#include "FileItem.h"
#include "LocalizeStrings.h"

#define CONTROL_AUTHS 3

static int nofAuths = 5;
static CHalWirelessAuthType auths[] = { AUTH_NONE, AUTH_WEP_PASSPHRASE, AUTH_WEP_KEY, AUTH_WPAPSK, AUTH_WPA2PSK };
static const char* authStrs[] = { "None", "WEP 128-bit passphrase", "WEP 40-bit key", "WPA Personal", "WPA2 Personal" };

CGUIDialogWirelessAuthentication::CGUIDialogWirelessAuthentication(void)
    : CGUIDialog(WINDOW_DIALOG_WIRELESS_AUTHENTICATION, "DialogWirelessAuthentication.xml")
{
}

CGUIDialogWirelessAuthentication::~CGUIDialogWirelessAuthentication(void)
{}

bool CGUIDialogWirelessAuthentication::OnAction(const CAction &action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_AUTHS);
    OnMessage(msg);
    int iItem = msg.GetParam1();
    m_auth = auths[iItem];
    m_wasItemSelected = true;

    Close();

    return true;
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogWirelessAuthentication::OnInitWindow()
{
  m_wasItemSelected = false;

  CGUIDialog::OnInitWindow();

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_AUTHS);
  OnMessage(msgReset);

  int select = -1;
  for (int i = 0; i < nofAuths; i++)
  {
    CFileItemPtr item(new CFileItem(authStrs[i]));

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_AUTHS, 0, 0, item);
    OnMessage(msg);

    if (auths[i] == (int) m_auth)
    {
      select = i;
    }
  }

  if (select != -1)
  {
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_AUTHS, select);
    OnMessage(msg2);
  }
}

bool CGUIDialogWirelessAuthentication::WasItemSelected()
{
  return m_wasItemSelected;
}

void CGUIDialogWirelessAuthentication::SetAuth(CHalWirelessAuthType auth)
{
  m_auth = auth;
}

CHalWirelessAuthType CGUIDialogWirelessAuthentication::GetAuth()
{
  return m_auth;
}

#endif

