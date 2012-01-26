#ifndef AUDIOUTILSHELPER_H
#define AUDIOUTILSHELPER_H

#include <fcntl.h> //open
#if !defined(__APPLE__)
#include <linux/soundcard.h> //mixer stuff
#endif
#include <sys/ioctl.h>

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
  int m_mixer_fd;

  long m_lSysVolumeDiff;
  long m_lSysBaseVolume;
  
  long m_lAppVolumeDiff;
  long m_lAppBaseVolume;
};

#endif

