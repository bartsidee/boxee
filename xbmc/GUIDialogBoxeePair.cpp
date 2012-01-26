#include "GUIDialogBoxeePair.h"
#include "GUIWindowManager.h"
#include "log.h"
#include "bxutils.h"
#include "Application.h"
#include "LocalizeStrings.h"

#define CONTROL_BUTTON  10
#define CONTROL_LABEL   43

#define BASE_PAIR_DIGIT_ICON "device_pairing_code_"
#define BASE_PAIR_DIGIT_PROPERY "pair-digit-"

CGUIDialogBoxeePair::CGUIDialogBoxeePair() : CGUIDialog(WINDOW_DIALOG_BOXEE_PAIR, "boxee_pair.xml")
{

}

CGUIDialogBoxeePair::~CGUIDialogBoxeePair()
{

}

void CGUIDialogBoxeePair::OnInitWindow()
{
  if (!m_deviceItem.IsInitialize())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::OnInitWindow - FAILED to initialize dialog. DeviceItem ISN'T initialize (bdm)");
    Close();
    return;
  }

  CGUIDialog::OnInitWindow();

  SET_CONTROL_LABEL(CONTROL_BUTTON,222);

  CStdString label;
  CStdString labelStr = g_localizeStrings.Get(55501);
  label.Format(labelStr.c_str(),m_deviceItem.GetDeviceLabel());
  SET_CONTROL_LABEL(CONTROL_LABEL,label);

  int passcode;
  srand((unsigned)time(0));
  passcode = rand() % 9000 + 1000;

  m_passcodeStr = BOXEE::BXUtils::IntToString(passcode);

  CGUIWindow* activeWindow = g_windowManager.GetWindow(g_windowManager.GetActiveWindow());
  if (!activeWindow)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::OnInitWindow - FAILED to initialize dialog. [activeWindow=%p] (bdm)",activeWindow);
    Close();
    return;
  }

  for (unsigned int i=0; i<m_passcodeStr.size(); i++)
  {
    CStdString basePairDigitIcon = BASE_PAIR_DIGIT_ICON;
    CStdString pairDigitIcon = basePairDigitIcon;
    pairDigitIcon += m_passcodeStr[i];
    pairDigitIcon += ".png";

    CStdString basePairDigitWindowProperty = BASE_PAIR_DIGIT_PROPERY;
    CStdString pairDigitWindowProperty = basePairDigitWindowProperty;
    pairDigitWindowProperty += BOXEE::BXUtils::IntToString(i);

    activeWindow->SetProperty(pairDigitWindowProperty,pairDigitIcon);
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePair::OnInitWindow - after set [passcode=%s] (bdm)",m_passcodeStr.c_str());
}

bool CGUIDialogBoxeePair::OnAction(const CAction& action)
{
  switch (action.id)
  {
  case ACTION_PARENT_DIR:
  case ACTION_PREVIOUS_MENU:
  {
    Close();
    return true;
  }
  }

  return CGUIDialog::OnAction(action);
}

bool CGUIDialogBoxeePair::OnMessage(CGUIMessage& message)
{
  switch ( message.GetMessage() )
  {
  case GUI_MSG_CLICKED:
  {
    int iControl = message.GetSenderId();
    if (iControl == CONTROL_BUTTON)
    {
      CLog::Log(LOGDEBUG,"CGUIDialogBoxeePair::OnMessage - GUI_MSG_CLICKED - going to close dialog (bdm)");
      Close();
      return true;
    }
  }
  break;
  }

  return CGUIDialog::OnMessage(message);
}

void CGUIDialogBoxeePair::Render()
{
  return CGUIDialog::Render();
}

void CGUIDialogBoxeePair::SetDeviceItem(CBoxeeDeviceItem* deviceItem)
{
  if (deviceItem)
  {
    m_deviceItem = *deviceItem;
  }
  else
  {
    m_deviceItem.Reset();
  }
}

CBoxeeDeviceItem CGUIDialogBoxeePair::GetDeviceItem()
{
  return m_deviceItem;
}

void CGUIDialogBoxeePair::Reset()
{
  m_deviceItem.Reset();
  m_passcodeStr = "";
}

bool CGUIDialogBoxeePair::PairDevice(const CStdString& deviceId, const CStdString& code)
{
  if (!m_deviceItem.IsInitialize())
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - DeviceItem ISN'T initialize. [deviceId=%s][code=%s] (bdm)",deviceId.c_str(),code.c_str());
    return false;
  }

  CStdString deviceIdToMatch = m_deviceItem.GetDeviceId();
  if (deviceId != deviceIdToMatch)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - [DeviceId=%s] DOESN'T match [%s=DeviceItemId]. [deviceId=%s][code=%s] (bdm)",deviceId.c_str(),deviceIdToMatch.c_str(),deviceId.c_str(),code.c_str());
    return false;
  }

  if (code != m_passcodeStr)
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - [code=%s] DOESN'T match [%s=passcode]. [deviceId=%s][code=%s] (bdm)",code.c_str(),m_passcodeStr.c_str(),deviceId.c_str(),code.c_str());

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_LABEL);
    msg.SetLabel(55502);
    g_windowManager.SendThreadMessage(msg);

    return false;
  }

  if (!g_application.GetBoxeeDeviceManager().PairDevice(m_deviceItem))
  {
    CLog::Log(LOGERROR,"CGUIDialogBoxeePair::PairDevice - FAILED to pair device. [deviceId=%s][code=%s] (bdm)",deviceId.c_str(),code.c_str());

    CGUIMessage msg(GUI_MSG_LABEL_SET, GetID(), CONTROL_LABEL);
    msg.SetLabel(55502);
    g_windowManager.SendThreadMessage(msg);

    return false;
  }

  CLog::Log(LOGDEBUG,"CGUIDialogBoxeePair::PairDevice - SUCCEEDED to pair device. [deviceId=%s][code=%s] (bdm)",deviceId.c_str(),code.c_str());

  CGUIMessage msgLabel(GUI_MSG_LABEL_SET, GetID(), CONTROL_LABEL);
  msgLabel.SetLabel(55503);
  g_windowManager.SendThreadMessage(msgLabel);

  CGUIMessage msgButton(GUI_MSG_LABEL_SET, GetID(), CONTROL_BUTTON);
  msgButton.SetLabel(20177);
  g_windowManager.SendThreadMessage(msgButton);

  return true;
}

