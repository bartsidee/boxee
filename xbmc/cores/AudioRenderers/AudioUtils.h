#ifndef __AUDIOUTILS_H__
#define __AUTIOUTILS_H__

#include "../../Settings.h"
#include "Application.h"

#ifdef _WIN32
#include "AudioUtilsHelperWin32.h"
#elif defined(_LINUX) && !defined(__APPLE__)
#include "AudioUtilsHelperLinux.h"
#elif defined(__APPLE__)
#include "AudioUtilsHelperApple.h"
#endif

class CAudioUtils
{
public:
  static CAudioUtils& GetInstance()
  {
    static CAudioUtils sAudioUtil;
    return sAudioUtil;
  }

  bool SetMasterVolume(long masterVolume);
  unsigned int UpdateAppVolume();

  virtual ~CAudioUtils(void);
private:
  
  CAudioUtilsHelper   m_SystemMasterVolume;

  CAudioUtils(void);
};

#endif
