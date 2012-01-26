#include "GUIDialogFirstTimeUseNetworkMessage.h"

#ifdef HAS_EMBEDDED

#include "GUILabelControl.h"
#include "GUIButtonControl.h"
#include "Application.h"
#include "LocalizeStrings.h"
#include "log.h"
#include "InitializeBoxManager.h"

CGUIDialogFirstTimeUseNetworkMessage::CGUIDialogFirstTimeUseNetworkMessage() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_NETWORK_MESSAGE,"ftu_network_message.xml","CGUIDialogFirstTimeUseNetworkMessage")
{
  m_buttonClicked = 0;
}

CGUIDialogFirstTimeUseNetworkMessage::~CGUIDialogFirstTimeUseNetworkMessage()
{

}

void CGUIDialogFirstTimeUseNetworkMessage::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  CGUILabelControl* labelControl = ((CGUILabelControl*)GetControl(NETWORK_MESSAGE_LABEL_CONTROL));

  if (labelControl)
  {
    labelControl->SetLabel(m_message);
  }

  m_buttonClicked = 0;

  if (CInitializeBoxManager::GetInstance().IsConnectViaEthernet())
  {
    m_switchButtonLabel = g_localizeStrings.Get(54699);
    m_switchToType = SWITCH_TO_WIRELESS_BUTTON;
    SET_CONTROL_VISIBLE(ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL);
  }
  else
  {
    m_switchButtonLabel = g_localizeStrings.Get(54729);
    m_switchToType = SWITCH_TO_ETHETNET_BUTTON;
    SET_CONTROL_HIDDEN(ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL);
  }

  CGUIButtonControl* buttonControl = ((CGUIButtonControl*)GetControl(SWITCH_CONNECTION_TYPE_BUTTON_CONTROL));

  if (buttonControl)
  {
    CStdString label = "[B]" + m_switchButtonLabel + "[/B]";
    buttonControl->SetLabel(label);
  }
}

bool CGUIDialogFirstTimeUseNetworkMessage::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    switch (senderId)
    {
    case TRY_AGAIN_BUTTON_CONTROL:
    {
      m_buttonClicked = TRY_AGAIN_BUTTON_CONTROL;
      m_actionChoseEnum = CActionChose::NEXT;
      Close();
      return true;
    }
    break;
    case SWITCH_CONNECTION_TYPE_BUTTON_CONTROL:
    {
      m_buttonClicked = SWITCH_CONNECTION_TYPE_BUTTON_CONTROL;
      m_actionChoseEnum = CActionChose::NEXT;
      Close();
      return true;
    }
    break;
    case ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL:
    {
      m_buttonClicked = ADJUST_NETWORK_SETTINGS_BUTTON_CONTROL;
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

bool CGUIDialogFirstTimeUseNetworkMessage::HandleClickNext()
{
  // nothing to do

  return true;
}

bool CGUIDialogFirstTimeUseNetworkMessage::HandleClickBack()
{
  // nothing to do

  return true;
}

void CGUIDialogFirstTimeUseNetworkMessage::SetMessage(const CStdString& message)
{
  m_message = message;
}

int CGUIDialogFirstTimeUseNetworkMessage::GetButtonClicked()
{
  return m_buttonClicked;
}

int CGUIDialogFirstTimeUseNetworkMessage::GetSwitchToType()
{
  return m_switchToType;
}

#endif

