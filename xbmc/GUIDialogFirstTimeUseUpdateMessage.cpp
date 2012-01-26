#include "GUIDialogFirstTimeUseUpdateMessage.h"

#ifdef HAS_EMBEDDED

#include "InitializeBoxManager.h"
#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "LocalizeStrings.h"
#include "log.h"
#include "Settings.h"

#define UPDATE_MESSAGE_LABEL_CONTROL   258
#define UPDATE_BUTTON_CONTROL          250

CGUIDialogFirstTimeUseUpdateMessage::CGUIDialogFirstTimeUseUpdateMessage() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_UPDATE_MESSAGE,"ftu_update_message.xml","CGUIDialogFirstTimeUseUpdateMessage")
{
  m_message = "";
  m_buttonLabel = "";
}

CGUIDialogFirstTimeUseUpdateMessage::~CGUIDialogFirstTimeUseUpdateMessage()
{

}

void CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  m_message = "";
  m_buttonLabel = "";

  CUpdateVersionStatus::UpdateVersionStatusEnums updateVersionStatus = CInitializeBoxManager::GetInstance().GetUpdateStatus();

  switch(updateVersionStatus)
  {
  case CUpdateVersionStatus::NO_UPDATE:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=NO_UPDATE] -> Not handling (initbox)",(int)updateVersionStatus);
  }
  break;
  case CUpdateVersionStatus::HAS_UPDATE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=HAS_UPDATE] (initbox)",(int)updateVersionStatus);
    m_message = g_localizeStrings.Get(54693);
    m_buttonLabel = g_localizeStrings.Get(54692);
  }
  break;
  case CUpdateVersionStatus::UPDATING:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=UPDATING] -> Not handling (initbox)",(int)updateVersionStatus);
  }
  break;
  case CUpdateVersionStatus::UPDATE_SUCCEEDED:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=UPDATE_SUCCEEDED] (initbox)",(int)updateVersionStatus);
    m_message = g_localizeStrings.Get(54657);
    m_buttonLabel = g_localizeStrings.Get(54658);
  }
  break;
  case CUpdateVersionStatus::UPDATE_FAILED:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=UPDATE_FAILED] (initbox)",(int)updateVersionStatus);
    m_message = g_localizeStrings.Get(54656);
    m_buttonLabel = g_localizeStrings.Get(54655);
  }
  break;
  case CUpdateVersionStatus::REQUEST_UPGRADE:
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - Update status is [%d=REQUEST_UPGRADE] (initbox)",(int)updateVersionStatus);
    m_message = g_localizeStrings.Get(54659);
    SET_CONTROL_HIDDEN(UPDATE_BUTTON_CONTROL);
    SET_CONTROL_FOCUS(UPDATE_MESSAGE_LABEL_CONTROL,0);

    // mark that FTU was finished OK
    g_stSettings.m_doneFTU = true;
    g_settings.Save();

    CInitializeBoxManager::GetInstance().RequestUpgrade();
  }
  break;
  default:
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseUpdateMessage::OnInitWindow - UNKNOWN Update status [%d] -> Not handling (initbox)",(int)updateVersionStatus);
  }
  break;
  }

  ((CGUILabelControl*)GetControl(UPDATE_MESSAGE_LABEL_CONTROL))->SetLabel(m_message);
  ((CGUIButtonControl*)GetControl(UPDATE_BUTTON_CONTROL))->SetLabel(m_buttonLabel);
}

bool CGUIDialogFirstTimeUseUpdateMessage::OnAction(const CAction &action)
{
  if (action.id == ACTION_PREVIOUS_MENU || action.id == ACTION_PARENT_DIR)
  {
    // don't allow back in this dialog

    return true;
  }

  return CGUIDialogFirstTimeUseBase::OnAction(action);
}

bool CGUIDialogFirstTimeUseUpdateMessage::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    switch (senderId)
    {
    case UPDATE_BUTTON_CONTROL:
    {
      m_actionChoseEnum = CActionChose::NEXT;
      Close();
      return true;
    }
    break;
    default:
    {
      // do nothing
    }
    break;
    }
  }
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

void CGUIDialogFirstTimeUseUpdateMessage::SetMessage(const CStdString& message)
{
  m_message = message;
}

void CGUIDialogFirstTimeUseUpdateMessage::SetButtonLabel(const CStdString& buttonLabel)
{
  m_buttonLabel = buttonLabel;
}

bool CGUIDialogFirstTimeUseUpdateMessage::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseUpdateMessage::HandleClickBack()
{
  // nothing to do

  return true;
}

#endif

