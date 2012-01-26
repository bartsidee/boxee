#include "AudioUtils.h"
#include "GUISettings.h"
#include "log.h"
#include <math.h>
#include "../../guilib/GUIAudioManager.h"

CAudioUtils::CAudioUtils(void)
{
}

CAudioUtils::~CAudioUtils(void)
{
}

unsigned int CAudioUtils::UpdateAppVolume()
{
  if (!g_guiSettings.GetBool("audiooutput.controlmastervolume"))
    return g_stSettings.m_nVolumeLevel;

  long nAppVolume;
  nAppVolume = m_SystemMasterVolume.GetMasterVolume();

  g_stSettings.m_nVolumeLevel = (long)((float)nAppVolume * 0.01f * (VOLUME_MAXIMUM - VOLUME_MINIMUM) + VOLUME_MINIMUM);
  g_application.m_guiDialogVolumeBar.Show();

  return g_stSettings.m_nVolumeLevel;
}

bool CAudioUtils::SetMasterVolume(long masterVolume)
{
  bool bOk = m_SystemMasterVolume.SetMasterVolume(masterVolume);
  if (bOk)
    g_stSettings.m_nVolumeLevel = masterVolume;
  return bOk;
}
