#ifndef AUDIOUTILSHELPER_H
#define AUDIOUTILSHELPER_H

class CAudioUtilsHelper
{
public:
  bool  Init();
  bool  Deinit();
  bool  SetMasterVolume(long masterVolume);
  long  GetMasterVolume();

  CAudioUtilsHelper(void);
  virtual ~CAudioUtilsHelper(void);

  long  CastToSystemVolume(long AppVolume);
  float GetSysRelativeVolume(long SysVolume);

  float GetAppRelativeVolume(long AppVolume);
  long  CastToAppVolume(long SysVolume);

private:

  long m_lSysVolumeDiff;
  long m_lSysBaseVolume;
  
  long m_lAppVolumeDiff;
  long m_lAppBaseVolume;
};

#endif