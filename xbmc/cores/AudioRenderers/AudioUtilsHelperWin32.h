#ifndef AUDIOUTILSHELPER_H
#define AUDIOUTILSHELPER_H


#include "system.h"
#include <windows.h>
#include <commctrl.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>

#include <endpointvolume.h>

class CAudioUtilsHelper
{
public:
  bool Init();
  bool Deinit();
  bool SetMasterVolume(long masterVolume);
  long GetMasterVolume();

  CAudioUtilsHelper(void);
  virtual ~CAudioUtilsHelper(void);

  long  CastToSystemVolume(long AppVolume);
  float GetSysRelativeVolume(long SysVolume);

  float GetAppRelativeVolume(long AppVolume);
  long  CastToAppVolume(long SysVolume);

private:
  static bool convertLPWtoStdString(std::string& s, const LPWSTR pw, UINT codepage = CP_ACP);

  IAudioEndpointVolume*         m_endpointVolume; //used for winver > 6
  
  HMIXER                        m_hMixer; //used for winver < 6
  DWORD                         m_dwVolumeControlID;

  bool                          m_bIsWinVer6; //if true, winver > 6

  long m_lSysVolumeDiff;
  long m_lSysBaseVolume;
  
  long m_lAppVolumeDiff;
  long m_lAppBaseVolume;
};

#endif