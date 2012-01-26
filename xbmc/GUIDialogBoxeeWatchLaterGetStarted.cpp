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

#include "GUIDialogBoxeeWatchLaterGetStarted.h"
#include "FileSystem/BoxeeServerDirectory.h"
#include "lib/libBoxee/bxconfiguration.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "GUIDialogOK2.h"
#include "bxutils.h"
#include "Application.h"

using namespace std;
using namespace BOXEE;

#define BTN_GET_STARTED     7092
#define BTN_OFFLINE_DONE    7192

CGUIDialogBoxeeWatchLaterGetStarted::CGUIDialogBoxeeWatchLaterGetStarted(void)
    : CGUIDialog(WINDOW_DIALOG_BOXEE_GET_STARTED,"boxee_watch_later_get_started.xml")
{
}

CGUIDialogBoxeeWatchLaterGetStarted::~CGUIDialogBoxeeWatchLaterGetStarted(void)
{
}

void CGUIDialogBoxeeWatchLaterGetStarted::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (g_application.IsConnectedToInternet())
  {
    SET_CONTROL_FOCUS(BTN_GET_STARTED, 0);
  }
  else
  {
    SET_CONTROL_FOCUS(BTN_OFFLINE_DONE, 0);
  }

  m_bIsConfirmed = false;
}

bool CGUIDialogBoxeeWatchLaterGetStarted::OnAction(const CAction &action)
{
  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeWatchLaterGetStarted::OnMessage(CGUIMessage& message)
{
  int controlId = message.GetSenderId();

  if (message.GetMessage() == GUI_MSG_CLICKED)
  {
    if (controlId == BTN_GET_STARTED)
    {
      if (!HandleClickOnGetStartedButton())
      {
        // ERROR log will be written from HandleClickOnGetStartedButton()
      }
      m_bIsConfirmed = true;
      Close();
      return true;
    }

    if (controlId == BTN_OFFLINE_DONE && !g_application.IsConnectedToInternet())
    {
      m_bIsConfirmed = true;
      Close();
      return true;
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeWatchLaterGetStarted::HandleClickOnGetStartedButton()
{
  CStdString strUrl = BXConfiguration::GetInstance().GetURLParam("Boxee.WatchLater.GetStarted","https://app.boxee.tv/accountapi/tellmeabout/?about=watchlater");

  Json::Value jResponse;
  int retCode;

  /*
  ////////////////////////////
  // erez - FOR TESTING !!! //
  {
    CStdString strJson = "{\"success\":\"false\",\"message\":\"bla bla bla\"}";

    Json::Reader reader;

    if (!reader.parse(strJson, jResponse))
    {
      printf("OnCreateUserThruFacebook - FAILED to parse json");
      return false;
    }

    Json::Value::Members keys = jResponse.getMemberNames();
    for (size_t i = 0; i < keys.size(); i++)
    {
      printf("[%zu/%zu] - [%s=%s]\n",i+1,keys.size(),keys[i].c_str(),jResponse.get(keys[i],"WTF").asString().c_str());
    }
  }
  ////////////////////////////
  */

  Job_Result result = BOXEE::BXUtils::PerformJSONGetRequestInBG(strUrl,jResponse,retCode);

  if (result != JOB_SUCCEEDED)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeWatchLaterGetStarted::HandleClickOnGetStartedButton - call to server returned FAILED. [result=%d][retCode=%d] (queue)(browse)",result,retCode);
    return false;
  }

  CStdString successStr = "";
  if (!jResponse.isMember("success"))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeWatchLaterGetStarted::HandleClickOnGetStartedButton - FAILED to get parameter <success> from jResponse (queue)(browse)");
    return false;
  }
  else
  {
    successStr = jResponse["success"].asString();
  }

  CStdString messageStr = "";
  if (!jResponse.isMember("message"))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeWatchLaterGetStarted::HandleClickOnGetStartedButton - FAILED to get parameter <message> from jResponse (queue)(browse)");
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(257),g_localizeStrings.Get(81000));
    return false;
  }
  else
  {
    messageStr = jResponse["message"].asString();
  }

  successStr.ToLower();
  if (successStr == "true")
  {
    CStdString messageStruct = g_localizeStrings.Get(56048);
    CStdString messageDesc = "";
    messageDesc.Format(messageStruct.c_str(), messageStr);
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(56047),messageDesc);
    return true;
  }
  else
  {
    CGUIDialogOK2::ShowAndGetInput(g_localizeStrings.Get(257),g_localizeStrings.Get(81000));
    return false;
  }
}
