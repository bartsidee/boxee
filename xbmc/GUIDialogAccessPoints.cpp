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

#ifdef HAS_BOXEE_HAL

#include "GUIDialogAccessPoints.h"
#include "GUIDialogKeyboard.h"
#include "Application.h"
#include "FileItem.h"
#include "LocalizeStrings.h"
#include "GUIDialogWirelessAuthentication.h"
#include "GUIWindowManager.h"
#include "Util.h"
#include "log.h"

#define CONTROL_ACCESS_POINTS 3

CGUIDialogAccessPoints::CGUIDialogAccessPoints(void)
    : CGUIDialog(WINDOW_DIALOG_ACCESS_POINTS, "DialogAccessPoints.xml")
{
}

CGUIDialogAccessPoints::~CGUIDialogAccessPoints(void)
{}

bool CGUIDialogAccessPoints::OnAction(const CAction &action)
{
  if (action.id == ACTION_SELECT_ITEM)
  {
    CGUIMessage msg(GUI_MSG_ITEM_SELECTED, GetID(), CONTROL_ACCESS_POINTS);
    OnMessage(msg);
    int iItem = msg.GetParam1();

    if (iItem == (int) m_aps.size())
    {
      // Get AP name
      if (!CGUIDialogKeyboard::ShowAndGetInput(m_essId, g_localizeStrings.Get(789), false))
      {
        return true;
      }

      // Get authentication
      CGUIDialogWirelessAuthentication *authDialog = (CGUIDialogWirelessAuthentication *)g_windowManager.GetWindow(WINDOW_DIALOG_WIRELESS_AUTHENTICATION);
      authDialog->SetAuth(m_auth);
      authDialog->DoModal();

      if (!authDialog->WasItemSelected())
      {
        return true;
      }

      m_auth = authDialog->GetAuth();

      // Get password
      if (m_auth != AUTH_NONE)
      {
        CStdString savePassword = m_password;
        m_password = "";
        if (!CGUIDialogKeyboard::ShowAndGetInput(m_password, g_localizeStrings.Get(54687), false, true))
        {
          m_password = savePassword;
          return true;
        }
      }

      m_wasItemSelected = true;

      Close();
    }
    else
    {
       // Get passwordsystem/settingsmap.xml
       if (m_aps[iItem].secure)
       {
         CStdString savePassword = m_password;
         m_password = "";
         if (!CGUIDialogKeyboard::ShowAndGetInput(m_password, g_localizeStrings.Get(54687), false, true))
         {
           m_password = savePassword;
           return true;
         }
       }
       else
       {
         m_password = "";
       }

       m_essId = m_aps[iItem].ssid;
       m_auth = AUTH_DONTCARE;
       m_wasItemSelected = true;

       Close();

       return true;
    }
  }

  return CGUIDialog::OnAction(action);
}

void CGUIDialogAccessPoints::OnInitWindow()
{
  m_wasItemSelected = false;

  CGUIDialog::OnInitWindow();

  CGUIMessage msgReset(GUI_MSG_LABEL_RESET, GetID(), CONTROL_ACCESS_POINTS);
  OnMessage(msgReset);

  m_aps.clear();
  CWirelessScanBG* pJob = new CWirelessScanBG(m_aps);
  CUtil::RunInBG(pJob);

  int select = -1;
  for (int i = 0; i < (int) m_aps.size(); i++)
  {
    CFileItemPtr item(new CFileItem(m_aps[i].ssid));
    int q = m_aps[i].signal_strength;
    if (q <= 33) item->SetThumbnailImage("ap-signal1.png");
    else if (q <= 66) item->SetThumbnailImage("ap-signal2.png");
    else item->SetThumbnailImage("ap-signal3.png");

    if (m_aps[i].secure)
      item->SetIconImage("ap-lock.png");

    CLog::Log(LOGDEBUG,"CGUIDialogAccessPoints::OnInitWindow - [%d/%d] - going to add [label=%s][signal=%d=%s][IsSecure=%d=%s] (ap)",i+1,(int)m_aps.size(),item->GetLabel().c_str(),q,item->GetThumbnailImage().c_str(),m_aps[i].secure,item->GetIconImage().c_str());

    CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ACCESS_POINTS, 0, 0, item);
    OnMessage(msg);

    if (m_essId == m_aps[i].ssid)
    {
      select = i;
    }
  }

  CFileItemPtr item(new CFileItem(g_localizeStrings.Get(1047)));
  CLog::Log(LOGDEBUG,"CGUIDialogAccessPoints::OnInitWindow - going to add [label=%s] (ap)",item->GetLabel().c_str());
  CGUIMessage msg(GUI_MSG_LABEL_ADD, GetID(), CONTROL_ACCESS_POINTS, 0, 0, item);
  OnMessage(msg);

  if (select == -1)
  {
    select = (int) m_aps.size();
  }

  if (m_essId.size() > 0)
  {
    CGUIMessage msg2(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_ACCESS_POINTS, select);
    OnMessage(msg2);
  }
}

bool CGUIDialogAccessPoints::WasItemSelected()
{
  return m_wasItemSelected;
}

void CGUIDialogAccessPoints::SetEssId(CStdString essId)
{
  m_essId = essId;
}

void CGUIDialogAccessPoints::SetAuth(CHalWirelessAuthType auth)
{
  m_auth = auth;
}

CStdString CGUIDialogAccessPoints::GetEssId()
{
  return m_essId;
}

CStdString CGUIDialogAccessPoints::GetPassword()
{
  return m_password;
}

CHalWirelessAuthType CGUIDialogAccessPoints::GetAuth()
{
  return m_auth;
}

CWirelessScanBG::CWirelessScanBG(std::vector<CHalWirelessNetwork>& aps) : m_aps(aps)
{
}

void CWirelessScanBG::Run()
{
  CHalServicesFactory::GetInstance().SearchWireless(0, m_aps);
  m_bJobResult = true;
}

#endif

