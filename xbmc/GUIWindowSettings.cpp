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
#include "GUIWindowSettings.h"
#include "GUIWindowManager.h"
#include "Key.h"
#include "GUIBaseContainer.h"
#ifdef HAS_CREDITS
#include "Credits.h"
#endif
#include "GUIDialogPluginSettings.h"
#include "utils/log.h"
#include "boxee.h"
#include "bxconfiguration.h"
#include "bxversion.h"
#include "bxxmldocument.h"
#include "bxcurl.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "lib/libBoxee/bxcurl.h"
#include "GUIDialogOK2.h"
#include "GUIDialogBoxeeMessageScroll.h"
#include "GUIDialogProgress.h"
#include "LocalizeStrings.h"

#define CONTROL_CREDITS 12
#define CONTROL_LEGAL_BUTTON 54

CGUIWindowSettings::CGUIWindowSettings(void)
    : CGUIWindow(WINDOW_SETTINGS_MENU, "Settings.xml")
{
  m_iSelectedControl = 0;
  m_iSelectedItem = 0;
}

CGUIWindowSettings::~CGUIWindowSettings(void)
{
}

void CGUIWindowSettings::OnInitWindow()
{
  CGUIWindow::OnInitWindow();

  if (m_iSelectedControl != 0)
    SET_CONTROL_FOCUS(m_iSelectedControl, m_iSelectedItem);
}

bool CGUIWindowSettings::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.id == ACTION_PARENT_DIR)
  {
    g_windowManager.PreviousWindow();
    return true;
  }
  else if (action.id == ACTION_SELECT_ITEM)
  {
    CGUIControl *focusedControl = GetFocusedControl();
    if (focusedControl && focusedControl->IsContainer())
    {
      m_iSelectedControl = focusedControl->GetID();

      CGUIBaseContainer* container  = (CGUIBaseContainer*)focusedControl;
      m_iSelectedItem = container->GetSelectedItem();
    }
  }

  return CGUIWindow::OnAction(action);
}

bool CGUIWindowSettings::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
    {
      int iControl = message.GetSenderId();
      CLog::Log(LOGDEBUG,"CGUIWindowSettings::OnMessage - GUI_MSG_CLICKED - Enter function with [iControl=%d]",iControl);
      if (iControl == CONTROL_CREDITS)
      {
#ifdef HAS_CREDITS
        RunCredits();
#endif
        return true;
      }
      else if (iControl == CONTROL_LEGAL_BUTTON)
      {
        return ShowLegal();
      }
    }
    break;
  }

  return CGUIWindow::OnMessage(message);
}

bool CGUIWindowSettings::ShowLegal()
{
  if (m_legalString.IsEmpty())
  {
    CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
    if (progress)
    {
      progress->StartModal();
      progress->Progress();
    }

    // verify the username is available
    CStdString strUrl = BOXEE::BXConfiguration::GetInstance().GetStringParam("Boxee.Legal","http://app.boxee.tv/api/getlegal?page=legal");
    BOXEE::BXCurl curl;

    m_legalString = curl.HttpGetString(strUrl, false);

    if (progress)
    {
      progress->Close();
    }

    if (m_legalString.IsEmpty())
    {
      CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(53701), g_localizeStrings.Get(53431));
      return true;
    }
  }

  CGUIDialogBoxeeMessageScroll::ShowAndGetInput(g_localizeStrings.Get(53441), m_legalString);
  return true;
}
