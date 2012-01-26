#include "GUIDialogFirstTimeUseAudio.h"

#ifdef HAS_EMBEDDED

#include "GUIWindowManager.h"
#include "GUIDialogOK2.h"
#include "GUIDialogYesNo2.h"
#include "log.h"

#define HAS_HDMI 261
#define HAS_OPTICAL 262
#define HAS_ANALOG_STEREO 263
#define HAS_DDD 271
#define NO_DDD 272
#define DONT_KNOW_DDD 273

CGUIDialogFirstTimeUseAudio::CGUIDialogFirstTimeUseAudio() : CGUIDialogFirstTimeUseBase(WINDOW_DIALOG_FTU_AUDIO,"ftu_audio.xml","CGUIDialogFirstTimeUseAudio")
{
  m_finishSetAudio = false;
  m_audioOutputType = CAudioOutputType::NONE;
  m_hasDolbyDigitalDts = 0;
}

CGUIDialogFirstTimeUseAudio::~CGUIDialogFirstTimeUseAudio()
{

}

void CGUIDialogFirstTimeUseAudio::OnInitWindow()
{
  CGUIDialogFirstTimeUseBase::OnInitWindow();

  m_finishSetAudio = false;

  if (m_audioOutputType != CAudioOutputType::NONE)
  {
    switch(m_audioOutputType)
    {
    case CAudioOutputType::HDMI:
    {
      SetSelectHDMI();
    }
    break;
    case CAudioOutputType::OPTICAL:
    {
      SetSelectOPTICAL();
    }
    break;
    case CAudioOutputType::ANALOG_STEREO:
    {
      SetSelectANALOG_STEREO();
    }
    break;
    }
  }

  if (m_hasDolbyDigitalDts != 0)
  {
    switch(m_hasDolbyDigitalDts)
    {
    case HAS_DDD:
    {
      SetSelectHAS_DDD();
    }
    break;
    case NO_DDD:
    {
      SetSelectNO_DDD();
    }
    break;
    case DONT_KNOW_DDD:
    {
      SetSelectDONT_KNOW_DDD();
    }
    break;
    }
  }
}

bool CGUIDialogFirstTimeUseAudio::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
  case GUI_MSG_CLICKED:
  {
    int senderId = message.GetSenderId();

    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseAudio::OnMessage - GUI_MSG_CLICKED - [buttonId=%d] (initbox)",senderId);

    switch (senderId)
    {
    case HAS_HDMI:
    {
      SetSelectHDMI();
      return true;
    }
    break;
    case HAS_OPTICAL:
    {
      SetSelectOPTICAL();
      return true;
    }
    break;
    case HAS_ANALOG_STEREO:
    {
      SetSelectANALOG_STEREO();
      return true;
    }
    break;
    case HAS_DDD:
    {
      SetSelectHAS_DDD();
      return true;
    }
    break;
    case NO_DDD:
    {
      SetSelectNO_DDD();
      return true;
    }
    break;
    case DONT_KNOW_DDD:
    {
      SetSelectDONT_KNOW_DDD();
      return true;
    }
    break;
    }
  }
  break;
  }

  return CGUIDialogFirstTimeUseBase::OnMessage(message);
}

bool CGUIDialogFirstTimeUseAudio::IsFinishSetAudio()
{
  return m_finishSetAudio;
}

bool CGUIDialogFirstTimeUseAudio::HandleClickNext()
{
  if (m_audioOutputType == CAudioOutputType::NONE)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseAudio::HandleClickNext - [AudioOutputType=%d] wasn't set (initbox)",m_audioOutputType);

    CGUIDialogOK2::ShowAndGetInput(54647,54648);

    SET_CONTROL_FOCUS(HAS_HDMI, 0);

    return false;
  }

  if (m_hasDolbyDigitalDts == 0)
  {
    CLog::Log(LOGERROR,"CGUIDialogFirstTimeUseAudio::HandleClickNext - [HasDolbyDigitalDts=%d] wasn't set (initbox)",m_hasDolbyDigitalDts);

    CGUIDialogOK2::ShowAndGetInput(54647,54649);

    SET_CONTROL_FOCUS(HAS_DDD, 0);

    return false;
  }

  // TODO: run audio test

  if (CGUIDialogYesNo2::ShowAndGetInput(54647,54651,106,107))
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseAudio::HandleClickNext - Res is OK (initbox)");
    m_finishSetAudio = true;
  }
  else
  {
    CLog::Log(LOGDEBUG,"CGUIDialogFirstTimeUseAudio::HandleClickNext - Res NOT OK. Show dialog again (initbox)");
    m_finishSetAudio = false;
  }

  return m_finishSetAudio;
}

bool CGUIDialogFirstTimeUseAudio::HandleClickBack()
{
  // nothing to do

  return true;
}

void CGUIDialogFirstTimeUseAudio::SetSelectHDMI()
{
  SET_CONTROL_SELECTED(GetID(), HAS_HDMI, true);
  SET_CONTROL_SELECTED(GetID(), HAS_OPTICAL, false);
  SET_CONTROL_SELECTED(GetID(), HAS_ANALOG_STEREO, false);
  m_audioOutputType = CAudioOutputType::HDMI;
}

void CGUIDialogFirstTimeUseAudio::SetSelectOPTICAL()
{
  SET_CONTROL_SELECTED(GetID(), HAS_HDMI, false);
  SET_CONTROL_SELECTED(GetID(), HAS_OPTICAL, true);
  SET_CONTROL_SELECTED(GetID(), HAS_ANALOG_STEREO, false);
  m_audioOutputType = CAudioOutputType::OPTICAL;
}

void CGUIDialogFirstTimeUseAudio::SetSelectANALOG_STEREO()
{
  SET_CONTROL_SELECTED(GetID(), HAS_HDMI, false);
  SET_CONTROL_SELECTED(GetID(), HAS_OPTICAL, false);
  SET_CONTROL_SELECTED(GetID(), HAS_ANALOG_STEREO, true);
  m_audioOutputType = CAudioOutputType::ANALOG_STEREO;
}

void CGUIDialogFirstTimeUseAudio::SetSelectHAS_DDD()
{
  SET_CONTROL_SELECTED(GetID(), HAS_DDD, true);
  SET_CONTROL_SELECTED(GetID(), NO_DDD, false);
  SET_CONTROL_SELECTED(GetID(), DONT_KNOW_DDD, false);
  m_hasDolbyDigitalDts = HAS_DDD;
}

void CGUIDialogFirstTimeUseAudio::SetSelectNO_DDD()
{
  SET_CONTROL_SELECTED(GetID(), HAS_DDD, false);
  SET_CONTROL_SELECTED(GetID(), NO_DDD, true);
  SET_CONTROL_SELECTED(GetID(), DONT_KNOW_DDD, false);
  m_hasDolbyDigitalDts = NO_DDD;
}

void CGUIDialogFirstTimeUseAudio::SetSelectDONT_KNOW_DDD()
{
  SET_CONTROL_SELECTED(GetID(), HAS_DDD, false);
  SET_CONTROL_SELECTED(GetID(), NO_DDD, false);
  SET_CONTROL_SELECTED(GetID(), DONT_KNOW_DDD, true);
  m_hasDolbyDigitalDts = DONT_KNOW_DDD;
}

#endif
