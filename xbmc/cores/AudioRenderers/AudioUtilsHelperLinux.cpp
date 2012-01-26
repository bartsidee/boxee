#include "AudioUtilsHelperLinux.h"
#include "AudioUtils.h"
#include <math.h>
#include "GUISettings.h"

CAudioUtilsHelper::CAudioUtilsHelper(void)
{
  Init();
}

CAudioUtilsHelper::~CAudioUtilsHelper(void)
{
  Deinit();
}

bool CAudioUtilsHelper::Init()
{
  m_lAppVolumeDiff = abs(VOLUME_MINIMUM - VOLUME_MAXIMUM);
  m_lAppBaseVolume = (VOLUME_MINIMUM < VOLUME_MAXIMUM)? VOLUME_MINIMUM : VOLUME_MAXIMUM;

  m_mixer_fd = -1;

  if((m_mixer_fd = open("/dev/mixer", O_RDWR, 0)) == -1)
  {
    return false;
  }

  m_lSysVolumeDiff = 100;
  m_lSysBaseVolume = 1;

  return true;

}

bool CAudioUtilsHelper::Deinit()
{
  if (m_mixer_fd != -1)
  {
    close(m_mixer_fd);
    return true;
  }
  return false;
}

bool CAudioUtilsHelper::SetMasterVolume(long masterVolume)
{
  int setVol=-1;
  int newVolume=0;
  int rightVol=0,leftVol=0;

  newVolume = CastToSystemVolume(masterVolume); //check this

  rightVol = newVolume;
  leftVol = rightVol;

  setVol = (rightVol << 8) | leftVol;

  if(ioctl(m_mixer_fd, MIXER_WRITE(SOUND_MIXER_VOLUME), &setVol) == -1)
  {
    return false;
  }

  return true;
}

long CAudioUtilsHelper::GetMasterVolume()
{
  long AppVolume=0;

  int getVol=-1;

  if(ioctl(m_mixer_fd, MIXER_READ(SOUND_MIXER_VOLUME), &getVol) != -1)
  {
    AppVolume = (getVol >> 8);
  }

  return AppVolume;
}

long CAudioUtilsHelper::CastToSystemVolume(long AppVolume)
{
  long retVal=0;

  //should be generic for all OSs
  float relativeValue = GetAppRelativeVolume(AppVolume);
  retVal = (long) ((m_lSysVolumeDiff * relativeValue) + m_lSysBaseVolume);
  
  return retVal;
}

float CAudioUtilsHelper::GetSysRelativeVolume(long SysVolume)
{
  return (float)(abs(SysVolume - m_lSysBaseVolume) / (float) m_lSysVolumeDiff);
}

long CAudioUtilsHelper::CastToAppVolume(long SysVolume)
{
  long retVal=0;

  //should be generic for all OSs
  
  float relativeValue = GetSysRelativeVolume(SysVolume);
  retVal = (long) ((m_lAppVolumeDiff * relativeValue) + m_lAppBaseVolume);
  
  return retVal;
}

float CAudioUtilsHelper::GetAppRelativeVolume(long AppVolume)
{
  return (float)(abs(AppVolume - m_lAppBaseVolume) / (float)m_lAppVolumeDiff); //should be between 0 to 1
}
