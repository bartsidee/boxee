/*
 * GUIDialogBoxeeApplicationAction.cpp
 *
 *  Created on: Feb 24, 2009
 *      Author: yuvalt
 */


#include "GUIDialogBoxeeApplicationAction.h"
#include "GUIWindowManager.h"
#include "Application.h"
#include "BoxeeMediaSourceList.h"
#include "URL.h"
#include "GUIDialogPluginSettings.h"
#include "AppManager.h"
#include "GUIDialogOK2.h"
#include "Util.h"
#ifdef HAS_LASTFM
#include "LastFmManager.h"
#endif
#include "GUIDialogProgress.h"
#include "BoxeeUtils.h"
#include "lib/libBoxee/boxee.h"
#include "utils/log.h"
#include "LocalizeStrings.h"
#include "ItemLoader.h"

#define UPGRADE_BUTTON   20
#define SETTINGS_BUTTON  21
#define REMOVE_BUTTON    22
#define THE_CONTAINER    5000

using namespace BOXEE;

CGUIDialogBoxeeApplicationAction::CGUIDialogBoxeeApplicationAction()
: CGUIDialog(WINDOW_BOXEE_DIALOG_APPLICATION_ACTION, "boxee_application_action.xml")
{
  m_appWasRemoved = false;
}

CGUIDialogBoxeeApplicationAction::~CGUIDialogBoxeeApplicationAction()
{
  // TODO Auto-generated destructor stub
}

void CGUIDialogBoxeeApplicationAction::SetAppItem(CFileItemPtr item)
{
  m_item = item;
  m_itemList.Clear();
  m_itemList.Add(item);
}

void CGUIDialogBoxeeApplicationAction::OnInitWindow()
{
  CGUIDialog::OnInitWindow();
  
  CGUIMessage winmsg1(GUI_MSG_LABEL_RESET, GetID(), 5000);
  g_windowManager.SendMessage(winmsg1);
  
  CGUIMessage winmsg(GUI_MSG_LABEL_ADD, GetID(), 5000, 0, 0, m_item);
  g_windowManager.SendMessage(winmsg);
  
  SET_CONTROL_FOCUS(REMOVE_BUTTON, 0);
    
  DWORD focus = REMOVE_BUTTON;
  
  if (m_item->GetPropertyBOOL("app-hasupgrade"))
  {
    SET_CONTROL_VISIBLE(UPGRADE_BUTTON); 
    focus = UPGRADE_BUTTON;
  }
  else
  {
    SET_CONTROL_HIDDEN(UPGRADE_BUTTON);
  }

  if (m_item->GetPropertyBOOL("app-hassettings"))
  {
    SET_CONTROL_VISIBLE(SETTINGS_BUTTON);
    if (!m_item->GetPropertyBOOL("app-hasupgrade"))
    {
      focus = SETTINGS_BUTTON;      
    }
  }
  else
  {
    SET_CONTROL_HIDDEN(SETTINGS_BUTTON);
  }

  SET_CONTROL_VISIBLE(REMOVE_BUTTON);
  SET_CONTROL_FOCUS(focus, 0);      

  m_appWasRemoved = false;

  g_application.GetItemLoader().AddControl(GetID(), THE_CONTAINER, m_itemList);
}

bool CGUIDialogBoxeeApplicationAction::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
    case GUI_MSG_CLICKED:
    {    
      DWORD senderId = message.GetSenderId();

      if (senderId == REMOVE_BUTTON)
      {
        if (!m_item->HasProperty("appid"))
        {
          CLog::Log(LOGERROR,"CGUIDialogBoxeeApplicationAction::OnMessage - FAILED to remove application because there is no [appid]. [ItemLabel=%s] (bapps)",m_item->GetLabel().c_str());
        }
        else
        {
          CStdString appId = m_item->GetProperty("appid");

          CLog::Log(LOGDEBUG,"CGUIDialogBoxeeApplicationAction::OnMessage - Going to remove application with [appid=%s]. [ItemLabel=%s] (bapps)",appId.c_str(),m_item->GetLabel().c_str());

          CGUIDialogProgress* progress = (CGUIDialogProgress *)g_windowManager.GetWindow(WINDOW_DIALOG_PROGRESS);
          if (progress)
          {
            progress->StartModal();
            progress->Progress();
          }

          // report to the server about the removed app
          BoxeeUtils::ReportRemoveApp(appId);

          if (progress)
          {
            progress->Close();
          }

          m_appWasRemoved = true;

          g_application.m_guiDialogKaiToast.QueueNotification(CGUIDialogKaiToast::ICON_MINUS, "", g_localizeStrings.Get(51039), 5000 , KAI_GREEN_COLOR , KAI_GREEN_COLOR);
        }
        Close();
        return true;
      }
      else if (senderId == SETTINGS_BUTTON)
      {
        CURI url(m_item->m_strPath);

        if (url.GetProtocol() == "plugin")
        {
          CGUIDialogPluginSettings* pDlgSettings = (CGUIDialogPluginSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
          if (pDlgSettings)
          {
            Close();
            pDlgSettings->ShowAndGetInput(url);
          }
        }
        if (url.GetProtocol() == "app")
        {
          CStdString path2 = "plugin://";
          path2 += m_item->GetProperty("app-media");
          path2 += "/";
          path2 += url.GetHostName();
          CURI url2(path2);
          
          CGUIDialogPluginSettings* pDlgSettings = (CGUIDialogPluginSettings*)g_windowManager.GetWindow(WINDOW_DIALOG_PLUGIN_SETTINGS);
          if (pDlgSettings)
          {
            Close();
            pDlgSettings->ShowAndGetInput(url2);
          }
        }
#ifdef HAS_LASTFM
        else if (url.GetProtocol() == "lastfm")
        {
          Close();
          CLastFmManager::ShowLastFMSettings();
        }
#endif
        return true;
      }
      else if (senderId == UPGRADE_BUTTON)
      {
        CURI url(m_item->m_strPath);    
        InstallOrUpgradeAppBG* job = new InstallOrUpgradeAppBG(url.GetHostName(), false, true);
        if (CUtil::RunInBG(job) != JOB_SUCCEEDED)
        {
          CGUIDialogOK2::ShowAndGetInput(52039, 52018);
        }
        Close();
        return true;
      }
    }
  }

  return CGUIDialog::OnMessage(message);
}

bool CGUIDialogBoxeeApplicationAction::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    Close();
    return true;
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeeApplicationAction::WasAppRemoved()
{
  return m_appWasRemoved;
}

