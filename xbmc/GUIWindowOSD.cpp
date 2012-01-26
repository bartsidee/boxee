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

#include "GUIWindowOSD.h"
#include "Application.h"
#include "GUIUserMessages.h"
#include "GUIWindowManager.h"
#include "MouseStat.h"
#include "LocalizeStrings.h"

//Boxee
#define CONTROL_RATE_BUTTON   601
#define CONTROL_RECOMMEND_BUTTON   602

#include "BoxeeUtils.h"
#include "GUIDialogBoxeeShare.h"
#include "GUIDialogBoxeeRate.h"
#include "Application.h"
#include "Util.h"
//end Boxee

CGUIWindowOSD::CGUIWindowOSD(void)
    : CGUIDialog(WINDOW_OSD, "VideoOSD.xml")
{
}

CGUIWindowOSD::~CGUIWindowOSD(void)
{
}

void CGUIWindowOSD::OnWindowLoaded()
{
  CFileItem &item = g_application.CurrentFileItem();

  // Boxee
  // workaround for not diaplaying "buy on amazon" on local media
  if ( !item.IsInternetStream() || item.IsType(".flv") )
  {
    SET_CONTROL_HIDDEN(407);
  }
  // end Boxee

  CGUIDialog::OnWindowLoaded();
  m_bRelativeCoords = true;
}

void CGUIWindowOSD::Render()
{
  if (m_autoClosing)
  {
    // check for movement of mouse or a submenu open
    if (g_Mouse.HasMoved() || g_windowManager.IsWindowActive(WINDOW_DIALOG_AUDIO_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_OSD_SETTINGS)
						   || g_windowManager.IsWindowActive(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_VIDEO_BOOKMARKS)
                           || g_windowManager.IsWindowActive(WINDOW_DIALOG_OSD_TELETEXT))
      SetAutoClose(3000);
  }
  CGUIDialog::Render();
}

bool CGUIWindowOSD::OnAction(const CAction &action)
{
  // keyboard or controller movement should prevent autoclosing
  if (action.id != ACTION_MOUSE && m_autoClosing)
    SetAutoClose(3000);

  if (action.id == ACTION_NEXT_ITEM || action.id == ACTION_PREV_ITEM)
  {
    // these could indicate next chapter if video supports it
    if (g_application.m_pPlayer != NULL && g_application.m_pPlayer->OnAction(action))
      return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIWindowOSD::OnMouse(const CPoint &point)
{
  if (g_Mouse.GetWheel())
  { // Mouse wheel
    int wheel = abs(g_Mouse.GetWheel());
    CAction action;
    action.amount1 = 0.5f * (float)wheel;
    action.id = g_Mouse.GetWheel() > 0 ? ACTION_ANALOG_SEEK_FORWARD : ACTION_ANALOG_SEEK_BACK;
    return g_application.OnAction(action);
  }
  if (g_Mouse.bClick[MOUSE_LEFT_BUTTON])
  { // pause
    CAction action;
    action.id = ACTION_PAUSE;
    return g_application.OnAction(action);
  }

  return CGUIDialog::OnMouse(point);
}

bool CGUIWindowOSD::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
// Boxee section
  case GUI_MSG_CLICKED:
    {
      unsigned int iControl = message.GetSenderId();
      if (iControl == CONTROL_RATE_BUTTON)
      {
        CFileItem &item = g_application.CurrentFileItem();
        bool bLike;
        if (CGUIDialogBoxeeRate::ShowAndGetInput(bLike))
        {
          BoxeeUtils::Rate(&item, bLike);
          g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_STAR, "", g_localizeStrings.Get(51034), 5000 , KAI_YELLOW_COLOR, KAI_GREY_COLOR);
        }
      }
      else if (iControl == CONTROL_RECOMMEND_BUTTON)
      {
        CGUIDialogBoxeeShare *pFriends = (CGUIDialogBoxeeShare *)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_SHARE);
        pFriends->DoModal();
      }
      return true;
    }
// end boxee section
  case GUI_MSG_VIDEO_MENU_STARTED:
    {
      // We have gone to the DVD menu, so close the OSD.
      Close();
    }
  case GUI_MSG_WINDOW_DEINIT:  // fired when OSD is hidden
    {
      // Remove our subdialogs if visible
      CGUIDialog *pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning())
        pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_AUDIO_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_VIDEO_BOOKMARKS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_OSD_TELETEXT);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
      pDialog = (CGUIDialog *)g_windowManager.GetWindow(WINDOW_DIALOG_SUBTITLE_OSD_SETTINGS);
      if (pDialog && pDialog->IsDialogRunning()) pDialog->Close(true);
    }
    break;
  }
  return CGUIDialog::OnMessage(message);
}

