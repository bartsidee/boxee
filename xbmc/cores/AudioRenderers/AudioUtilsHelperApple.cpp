#include "AudioUtilsHelperApple.h"
#include "AudioUtils.h"
#include "GUISettings.h"
#include "CoreAudio.h"
#include "utils/log.h"

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
  // TODO: need to set AppVolume
  m_lAppVolumeDiff = abs(VOLUME_MINIMUM - VOLUME_MAXIMUM);
  m_lAppBaseVolume = (VOLUME_MINIMUM < VOLUME_MAXIMUM)? VOLUME_MINIMUM : VOLUME_MAXIMUM;

  m_lSysVolumeDiff = 100;
  m_lSysBaseVolume = 1;

  return true;
}

bool CAudioUtilsHelper::Deinit()
{
  return true;
}

bool CAudioUtilsHelper::SetMasterVolume(long masterVolume)
{
 OSStatus err;
  UInt32 size;
  AudioDeviceID deviceId = 0;

  float volume = GetAppRelativeVolume(masterVolume);

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::SetMasterVolume - Convert [masterVolume=%ld] to [volume=%4.2f] (au)",masterVolume,volume);

  deviceId = CCoreAudioHardware::GetDefaultOutputDevice();

  if (!deviceId)
  {
    CLog::Log(LOGERROR,"CAudioUtilsHelper::SetMasterVolume - FAILED to get default output device. [masterVolume=%ld] (au)",masterVolume);
    return false;
  }

  CCoreAudioDevice device(deviceId);
  CStdString deviceName;
  device.GetName(deviceName, device.GetId());

  // try set master-channel
  Boolean canSetMasterChannel = false;
  size = sizeof canSetMasterChannel;
  err = AudioDeviceGetPropertyInfo(deviceId, 0, false, kAudioDevicePropertyVolumeScalar, &size, &canSetMasterChannel);

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::SetMasterVolume - Check MasterChannel for device [id=0x%04x][name=%s] returned [err=%d][canSet=%d]. [masterVolume=%ld] (au)",(unsigned int)device.GetId(),deviceName.c_str(),(bool)err,canSetMasterChannel,masterVolume);

  if(!err && canSetMasterChannel)
  {
    size = sizeof volume;
    err = AudioDeviceSetProperty(deviceId, 0, 0, false, kAudioDevicePropertyVolumeScalar, size, &volume);

    if (!err)
    {
      CLog::Log(LOGDEBUG,"CAudioUtilsHelper::SetMasterVolume - Set new volume to device [id=0x%04x][name=%s] MasterChannel (au)",(unsigned int)device.GetId(),deviceName.c_str());
      return true;
    }
  }

  UInt32 totalNumOfChannels = CCoreAudioDevice::GetTotalOutputChannels(deviceId);

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::SetMasterVolume - Device [id=0x%04x][name=%s] has [TotalNumOfChannels=%u]. [masterVolume=%ld] (au)",(unsigned int)device.GetId(),deviceName.c_str(),totalNumOfChannels,masterVolume);

  if (totalNumOfChannels < 1)
  {
    CLog::Log(LOGERROR,"CAudioUtilsHelper::SetMasterVolume - FAILED to set volume because [TotalNumOfChannels=%u]. [masterVolume=%ld] (au)",totalNumOfChannels,masterVolume);
    return false;
  }

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::SetMasterVolume - Going to set new volume to device [id=0x%04x][name=%s] channels. [TotalNumOfChannels=%u] (au)",(unsigned int)device.GetId(),deviceName.c_str(),totalNumOfChannels);

  for (UInt32 iChannel=1; iChannel<=totalNumOfChannels; iChannel++)
  {
    size = sizeof(volume);
    err = AudioDeviceSetProperty(deviceId, 0, iChannel, false, kAudioDevicePropertyVolumeScalar, size, &volume);

    if (err)
    {
      CLog::Log(LOGERROR,"CAudioUtilsHelper::SetMasterVolume - FAILED to set volume for channel [%u/%u]. [volume=%4.2f] (au)",iChannel,totalNumOfChannels,volume);
    }
  }

  return true;
}

long CAudioUtilsHelper::GetMasterVolume()
{
  long AppVolume=0;
  OSStatus err;
  UInt32 size;
  AudioDeviceID deviceId = 0;

  deviceId = CCoreAudioHardware::GetDefaultOutputDevice();

  if (!deviceId)
  {
    CLog::Log(LOGERROR,"CAudioUtilsHelper::UpdateAppVolume - FAILED to get default output device (au)");
    return 0;
  }

  CCoreAudioDevice device(deviceId);
  CStdString deviceName;
  device.GetName(deviceName, device.GetId());

  UInt32 totalNumOfChannels = CCoreAudioDevice::GetTotalOutputChannels(deviceId);

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::UpdateAppVolume - Device [id=0x%04x][name=%s] has [TotalNumOfChannels=%u] (au)",(unsigned int)device.GetId(),deviceName.c_str(),totalNumOfChannels);

  float volume = 0.0;
  float chVolume = 0.0;

  for (UInt32 iChannel=1; iChannel<=totalNumOfChannels; iChannel++)
  {
    size = sizeof(chVolume);
    err = AudioDeviceGetProperty(deviceId, iChannel, false, kAudioDevicePropertyVolumeScalar, &size, &chVolume);

    if (!err)
    {
      if (chVolume > volume)
      {
        volume = chVolume;
      }
    }
  }

  long sysVolume = volume * 100;

  int nCurrVol = g_application.GetVolume();
  if (abs(nCurrVol - sysVolume) < 3)
  {
    return nCurrVol;
  }

  AppVolume = sysVolume;

  CLog::Log(LOGDEBUG,"CAudioUtilsHelper::UpdateAppVolume - After set [AppVolume=%ld] from device [id=0x%04x][name=%s]. [volume=%4.2f][sysVolume=%ld] (au)",AppVolume,(unsigned int)device.GetId(),deviceName.c_str(),volume,sysVolume);

  return AppVolume;
}

long CAudioUtilsHelper::CastToSystemVolume(long AppVolume)
{
  long retVal=0;

  //should be generic for all OSs
  float relativeValue = GetAppRelativeVolume(AppVolume);
  retVal = (m_lSysVolumeDiff * relativeValue) + m_lSysBaseVolume;
  
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
  retVal = (m_lAppVolumeDiff * relativeValue) + m_lAppBaseVolume;
  
  return retVal;
}

float CAudioUtilsHelper::GetAppRelativeVolume(long AppVolume)
{
  return (float)(abs(AppVolume - m_lAppBaseVolume) / (float)m_lAppVolumeDiff); //should be between 0 to 1
}
