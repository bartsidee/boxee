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

#include "GUIDialogBoxeeMakeBoxeeSocial.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogOK2.h"
#include "bxutils.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define FACEBOOK_BUTTON      250
#define TWITTER_BUTTON       251
#define TUMBLR_BUTTON        252
#define DONE_BUTTON          7092
#define OFFLINE_DONE_BUTTON  7192

CGUIDialogBoxeeMakeBoxeeSocial::CGUIDialogBoxeeMakeBoxeeSocial(void)
    : CGUIDialog(WINDOW_DIALOG_BOXEE_MAKE_SOCIAL,"boxee_make_social.xml")
{
}

CGUIDialogBoxeeMakeBoxeeSocial::~CGUIDialogBoxeeMakeBoxeeSocial(void)
{
}

void CGUIDialogBoxeeMakeBoxeeSocial::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (g_application.IsConnectedToInternet())
  {
    SET_CONTROL_FOCUS(FACEBOOK_BUTTON, 0);
  }
  else
  {
    SET_CONTROL_FOCUS(OFFLINE_DONE_BUTTON, 0);
  }

  m_bIsConfirmed = false;
}

bool CGUIDialogBoxeeMakeBoxeeSocial::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeMakeBoxeeSocial::OnMessage(CGUIMessage& message)
{
  if(message.GetMessage() == GUI_MSG_CLICKED)
  {
    m_bIsConfirmed = true;
    if(message.GetSenderId() == FACEBOOK_BUTTON)
    {
      g_application.GetBoxeeSocialUtilsUIManager().HandleUISocialUtilConnect(FACEBOOK_SERVICE_ID);
      return true;
    }
    if(message.GetSenderId() == TWITTER_BUTTON)
    {
      g_application.GetBoxeeSocialUtilsUIManager().HandleUISocialUtilConnect(TWITTER_SERVICE_ID);
      return true;
    }
    if(message.GetSenderId() == TUMBLR_BUTTON)
    {
      g_application.GetBoxeeSocialUtilsUIManager().HandleUISocialUtilConnect(TUMBLR_SERVICE_ID);
      return true;
    }
    if(message.GetSenderId() == DONE_BUTTON || (message.GetSenderId() == OFFLINE_DONE_BUTTON && !g_application.IsConnectedToInternet()))
    {
      Close();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}
