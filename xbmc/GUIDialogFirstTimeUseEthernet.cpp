#include "GUIDialogFirstTimeUseEthernet.h"

#ifdef HAS_EMBEDDED

#include "GUIDialogOK2.h"
#include "log.h"

CGUIDialogFirstTimeUseEthernet::CGUIDialogFirstTimeUseEthernet() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_ETHERNET,"ftu_ethernet.xml","CGUIDialogFirstTimeUseEthernet")
{
  m_buttonSelected = 0;
}

CGUIDialogFirstTimeUseEthernet::~CGUIDialogFirstTimeUseEthernet()
{

}

void CGUIDialogFirstTimeUseEthernet::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  if (m_buttonSelected != 0)
  {
    switch(m_buttonSelected)
    {
    case AUTOMATIC_CONF:
    {
      SetSelectAUTOMATIC();
    }
    break;
    case MANUALLY_CONF:
    {
      SetSelectMANUALLY();
    }
    break;
    case SWITCH_TO_WIRELESS:
    {
      SetSelectSWITCH_TO_WIRELESS();
    }
    break;
    }
  }
}

bool CGUIDialogFirstTimeUseEthernet::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseEthernet::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (initbox)",senderId);

    switch (senderId)
    {
    case AUTOMATIC_CONF:
    {
      SetSelectAUTOMATIC();
      return true;
    }
    break;
    case MANUALLY_CONF:
    {
      SetSelectMANUALLY();
      return true;
    }
    break;
    case SWITCH_TO_WIRELESS:
    {
      SetSelectSWITCH_TO_WIRELESS();
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseEthernet::HandleClickNext()
{
  if (m_buttonSelected == 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseEthernet::HandleClickNext - No buton was chose (initbox)");

    CGUIDialogOK2::ShowAndGetInput(54669,54668);

    SET_CONTROL_FOCUS(AUTOMATIC_CONF, 0);

    return false;
  }

  return true;
}

bool CGUIDialogFirstTimeUseEthernet::HandleClickBack()
{
  // nothing to do

  return true;
}

void CGUIDialogFirstTimeUseEthernet::SetSelectAUTOMATIC()
{
  SET_CONTROL_SELECTED(GetID(), AUTOMATIC_CONF, true);
  SET_CONTROL_SELECTED(GetID(), MANUALLY_CONF, false);
  SET_CONTROL_SELECTED(GetID(), SWITCH_TO_WIRELESS, false);
  m_buttonSelected = AUTOMATIC_CONF;
}

void CGUIDialogFirstTimeUseEthernet::SetSelectMANUALLY()
{
  SET_CONTROL_SELECTED(GetID(), AUTOMATIC_CONF, false);
  SET_CONTROL_SELECTED(GetID(), MANUALLY_CONF, true);
  SET_CONTROL_SELECTED(GetID(), SWITCH_TO_WIRELESS, false);
  m_buttonSelected = MANUALLY_CONF;
}

void CGUIDialogFirstTimeUseEthernet::SetSelectSWITCH_TO_WIRELESS()
{
  SET_CONTROL_SELECTED(GetID(), AUTOMATIC_CONF, false);
  SET_CONTROL_SELECTED(GetID(), MANUALLY_CONF, false);
  SET_CONTROL_SELECTED(GetID(), SWITCH_TO_WIRELESS, true);
  m_buttonSelected = SWITCH_TO_WIRELESS;
}

int CGUIDialogFirstTimeUseEthernet::GetChoiceSelected()
{
  return m_buttonSelected;
}

#endif

