#pragma once

#include "GUIDialogFirstTimeUseBase.h"

#ifdef HAS_EMBEDDED

class CAudioOutputType
{
public:
  enum AudioOutputTypeEnums
  {
    NONE=0,
    HDMI=1,
    OPTICAL=2,
    ANALOG_STEREO=3,
    NUM_OF_AUDIO_OUTPUT_TYPES=4
  };
};

class CGUIDialogFirstTimeUseAudio : public CGUIDialogFirstTimeUseBase
{
public:
  
  CGUIDialogFirstTimeUseAudio();
  virtual ~CGUIDialogFirstTimeUseAudio();

  virtual bool OnMessage(CGUIMessage &message);

  bool IsFinishSetAudio();

protected:

  virtual void OnInitWindow();

  virtual bool HandleClickNext();
  virtual bool HandleClickBack();

  void SetSelectHDMI();
  void SetSelectOPTICAL();
  void SetSelectANALOG_STEREO();
  void SetSelectHAS_DDD();
  void SetSelectNO_DDD();
  void SetSelectDONT_KNOW_DDD();

  bool m_finishSetAudio;
  CAudioOutputType::AudioOutputTypeEnums m_audioOutputType;
  int m_hasDolbyDigitalDts;

};

#endif

