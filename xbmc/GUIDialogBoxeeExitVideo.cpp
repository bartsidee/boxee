
#include "GUIDialogBoxeeExitVideo.h"
#include "GUIWindowManager.h"
#include "GUIRadioButtonControl.h"
#include "Settings.h"
#include "GUISettings.h"
#include "log.h"
#include "Application.h"
#include "MouseStat.h"

#define SAVE_CHOICE_RADIO_BUTTON  20
#define STAY_BUTTON               26
#define LEAVE_BUTTON              27
#define BACK_TO_BROWSER_BUTTON    28

CGUIDialogBoxeeExitVideo::CGUIDialogBoxeeExitVideo() : CGUIDialogBoxBase(WINDOW_DIALOG_BOXEE_EXIT_VIDEO, "boxee_exit_video.xml")
{
  m_stopVideo = true;
  m_dontShowDialog = false;
}

CGUIDialogBoxeeExitVideo::~CGUIDialogBoxeeExitVideo()
{

}

void CGUIDialogBoxeeExitVideo::OnInitWindow()
{
  CGUIDialog::OnInitWindow();

  if (g_application.IsPlayingLiveTV())
    m_settingsStr = "ota.showmessagewhenexit";
  else
    m_settingsStr = "myvideos.showmessagewhenexit";

  m_stopVideo = false;
  m_dontShowDialog = !g_guiSettings.GetBool(m_settingsStr);

  CGUIRadioButtonControl* pControl = (CGUIRadioButtonControl*)GetControl(SAVE_CHOICE_RADIO_BUTTON);
  if (pControl)
  {
    pControl->SetSelected(m_dontShowDialog);
  }

  SET_CONTROL_FOCUS(LEAVE_BUTTON,0);

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnInitWindow - [StopVideo=%d][saveChoice=%d][ShowMessageWhenExit=%d] (ev)",m_stopVideo,m_dontShowDialog,g_guiSettings.GetBool(m_settingsStr));
}

void CGUIDialogBoxeeExitVideo::OnDeinitWindow(int nextWindowID)
{
  if (g_guiSettings.GetBool(m_settingsStr) != !m_dontShowDialog)
  {
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnDeinitWindow - going to update [ShowMessageWhenExit=%d] to [DontShowDialog=%d] (ev)",g_guiSettings.GetBool(m_settingsStr), m_dontShowDialog);
    g_guiSettings.SetBool(m_settingsStr, !m_dontShowDialog);
    g_settings.Save();
  }

  CGUIDialogBoxBase::OnDeinitWindow(nextWindowID);
}

bool CGUIDialogBoxeeExitVideo::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() != GUI_MSG_CLICKED)
  {
    return CGUIDialogBoxBase::OnMessage(message);
  }

  switch(message.GetSenderId())
  {
    case SAVE_CHOICE_RADIO_BUTTON:
    {
      CGUIRadioButtonControl* pControl = (CGUIRadioButtonControl*)GetControl(SAVE_CHOICE_RADIO_BUTTON);
      if (pControl)
      {
        m_dontShowDialog = pControl->IsSelected();
      }
      else
      {
        m_dontShowDialog = false;
        CLog::Log(LOGERROR,"CGUIDialogBoxeeExitVideo::OnMessage - SAVE_CHOICE_RADIO_BUTTON - FAILED to get SAVE_CHOICE_RADIO_BUTTON. [DontShowDialog=%d] (ev)",m_dontShowDialog);
      }

      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnMessage - SAVE_CHOICE_RADIO_BUTTON - [DontShowDialog=%d]. [StopVideo=%d] (ev)",m_dontShowDialog,m_stopVideo);
      return true;
    }
    break;

    case STAY_BUTTON:
    {
      m_stopVideo = false;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnMessage - STAY_BUTTON - [StopVideo=%d] (ev)",m_stopVideo);
      Close();
      return true;
    }
    break;

    case LEAVE_BUTTON:
    {
      m_stopVideo = true;
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnMessage - LEAVE_BUTTON - [StopVideo=%d] (ev)",m_stopVideo);
      Close();
      return true;
    }
    break;
  }

  return CGUIDialogBoxBase::OnMessage(message);
}

bool CGUIDialogBoxeeExitVideo::OnAction(const CAction& action)
{
  if (action.id == ACTION_PREVIOUS_MENU)
  {
    m_stopVideo = false;
    m_dontShowDialog = false;
    CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::OnAction - ACTION_PREVIOUS_MENU - [StopVideo=%d] (ev)",m_stopVideo);
    Close();
    return true;
  }

  return CGUIDialogBoxBase::OnAction(action);
}

bool CGUIDialogBoxeeExitVideo::ShowAndGetInput()
{
  bool bMouseEnabled = g_Mouse.IsEnabledInSettings();
  g_Mouse.SetEnabled(false);

  CGUIDialogBoxeeExitVideo* dialog = (CGUIDialogBoxeeExitVideo*)g_windowManager.GetWindow(WINDOW_DIALOG_BOXEE_EXIT_VIDEO);
  if (!dialog)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeeExitVideo::ShowAndGetInput - FAILED to get WINDOW_DIALOG_BOXEE_EXIT_VIDEO. Return TRUE (ev)");
    g_Mouse.SetEnabled(bMouseEnabled);
    return true;
  }

  dialog->DoModal();

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeeExitVideo::ShowAndGetInput - Exit function. [StopVideo=%d][DontShowDialog=%d][ShowMessageWhenExit=%d] (ev)",dialog->m_stopVideo,dialog->m_dontShowDialog,g_guiSettings.GetBool(dialog->m_settingsStr));

  g_Mouse.SetEnabled(bMouseEnabled);
  return dialog->m_stopVideo;
}
