#include "AudioUtilsHelperWin32.h"
#include "AudioUtils.h"
#include "GUISettings.h"
#include <propidl.h>
#include <FunctionDiscoveryKeys_devpkey.h>


#ifndef _XBOX
extern HWND g_hWnd;
#endif


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
//general usage
  HRESULT hr=S_OK;
  OSVERSIONINFO osvi;
//used for winver > 6 control
  IMMDevice*                defaultDevice;
  IMMDeviceEnumerator*      deviceEnumerator = NULL;
  IMMDeviceCollection*      deviceCollection = NULL; 
  IPropertyStore *          pProps = NULL;
  LPWSTR                    pwszID = NULL;
  std::string               friendlyName;
  bool                      found = false;

  m_lAppVolumeDiff = abs(VOLUME_MINIMUM - VOLUME_MAXIMUM);
  m_lAppBaseVolume = (VOLUME_MINIMUM < VOLUME_MAXIMUM)? VOLUME_MINIMUM : VOLUME_MAXIMUM;


  bool retVal = true;

//init for winver > 6 control
  m_endpointVolume = NULL;

  ZeroMemory(&osvi, sizeof(OSVERSIONINFO));
  osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);

  GetVersionEx(&osvi);

  m_bIsWinVer6 = (osvi.dwMajorVersion >= 6);

  if (m_bIsWinVer6)
  {
    if (hr == S_OK)
    {
      deviceEnumerator = NULL;
      hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&deviceEnumerator);
      defaultDevice = NULL;

      if (hr == S_OK)
      {
        deviceEnumerator->EnumAudioEndpoints(eRender,DEVICE_STATEMASK_ALL,&deviceCollection);
        UINT  count;
        hr = deviceCollection->GetCount(&count);

        // Each loop prints the name of an endpoint device.
        for (ULONG i = 0; i < count; i++)
        {
            // Get pointer to endpoint number i.
            hr = deviceCollection->Item(i, &defaultDevice);
            
            // Get the endpoint ID string.
            hr = defaultDevice->GetId(&pwszID);
                        
            hr = defaultDevice->OpenPropertyStore(STGM_READ, &pProps);
            
            PROPVARIANT varName;
            // Initialize container for property value.
            PropVariantInit(&varName);

            // Get the endpoint's friendly-name property.
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varName);

            friendlyName.clear();

            convertLPWtoStdString(friendlyName,varName.pwszVal);
            
            // Print endpoint friendly name and endpoint ID.
           
            if (g_guiSettings.GetString("audiooutput.audiodevice") == friendlyName)
              found = true;

            CoTaskMemFree(pwszID);
            pwszID = NULL;
            PropVariantClear(&varName);
            SAFE_RELEASE(pProps)
              
            if (found)
              break;
            else
              SAFE_RELEASE(defaultDevice)
        }

        if (hr != S_OK || !found)//if getting the device specified in the UI didn't work 
          hr = deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);

        if (hr == S_OK)
        {
          deviceEnumerator->Release();
          deviceEnumerator = NULL;
       
          hr = defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&m_endpointVolume);
          
          if (hr == S_OK)
          {
            m_lSysBaseVolume = 0;
            m_lSysVolumeDiff = 1;
            defaultDevice->Release();
            defaultDevice = NULL;
          }
        }
      }
    }
  }
  else
  {
    //used for all the other
    int         lMinimum = 0;
    int         lMaximum = 0;
    UINT        NumMixers = 0;
    MIXERCAPS   mxcaps;

    ASSERT(m_hMixer == NULL);

    // get the number of mixer devices present in the system
    NumMixers = ::mixerGetNumDevs();

    m_hMixer = NULL;
    ::ZeroMemory(&mxcaps, sizeof(MIXERCAPS));

    // open the first mixer
    // A "mapper" for audio mixer devices does not currently exist.
    DWORD_PTR cb = reinterpret_cast<DWORD>(g_hWnd);

    //if (NumMixers != 0)
    for (int i = 0 ; i < NumMixers || retVal ; i++)
    { 
      if (retVal && ::mixerOpen(&m_hMixer,
                      i,
                      cb,
                      NULL,
                      MIXER_OBJECTF_MIXER | CALLBACK_WINDOW)
          != MMSYSERR_NOERROR)
      {
        retVal = false;
      }
  
      if (retVal && ::mixerGetDevCaps(reinterpret_cast<UINT>(m_hMixer),&mxcaps, sizeof(MIXERCAPS))!= MMSYSERR_NOERROR)
      {
        retVal = false;
      }

      if (g_guiSettings.GetString("audiooutput.audiodevice").Equals(mxcaps.szPname))//check if this is the device we want
      {
        break; //we found our device.
      }
      else if (NumMixers == 1) //the device strings does not match.
      { // its the only device anyway - no need to search, take whatever you can
        break;
      }
    }

    if (retVal && m_hMixer == NULL)
    {
      retVal = false;
    }

    if (retVal)
    {
      //after init lets find the master volume

      // get dwLineID
      MIXERLINE mxl;
      mxl.cbStruct = sizeof(MIXERLINE);
      mxl.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
      if (::mixerGetLineInfo(reinterpret_cast<HMIXEROBJ>(m_hMixer),
                             &mxl,
                             MIXER_OBJECTF_HMIXER |
                             MIXER_GETLINEINFOF_COMPONENTTYPE)
          != MMSYSERR_NOERROR)
      {
        retVal = false;
      }

      if (retVal)
      {
        // get dwControlID
        MIXERCONTROL mxc;
        MIXERLINECONTROLS mxlc;
        mxlc.cbStruct = sizeof(MIXERLINECONTROLS);
        mxlc.dwLineID = mxl.dwLineID;
        mxlc.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
        mxlc.cControls = 1;
        mxlc.cbmxctrl = sizeof(MIXERCONTROL);
        mxlc.pamxctrl = &mxc;
      

        if (::mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(m_hMixer),
                                   &mxlc,
                                   MIXER_OBJECTF_HMIXER |
                                   MIXER_GETLINECONTROLSF_ONEBYTYPE)
              != MMSYSERR_NOERROR)
        {
          retVal = false;
        }

        if (retVal)
        {
          // store dwControlID
          lMinimum = mxc.Bounds.lMinimum;
          lMaximum = mxc.Bounds.lMaximum;
          m_dwVolumeControlID = mxc.dwControlID;

          m_lSysVolumeDiff = abs(lMinimum - lMaximum);
          m_lSysBaseVolume = (lMinimum < lMaximum)? lMinimum : lMaximum;
        }
      }
    }
  }

  return retVal;
}

bool CAudioUtilsHelper::Deinit()
{
  bool retVal = true;

  if (m_bIsWinVer6)
  {
    SAFE_RELEASE(m_endpointVolume);
  }
  else
  {
    if (m_hMixer != NULL)
    {
        retVal = (::mixerClose(m_hMixer) == MMSYSERR_NOERROR);
        m_hMixer = NULL;
    }
  }
  return retVal;
}

bool CAudioUtilsHelper::SetMasterVolume(long masterVolume)
{
  bool retVal=true;
  float currentVolume = 0;
  
  if (m_bIsWinVer6 && m_endpointVolume) 
  {
    m_endpointVolume->SetMasterVolumeLevelScalar(GetAppRelativeVolume(masterVolume), NULL);
    retVal = true;
  }
  else
  {
    if (m_hMixer != NULL)
    {
      long lVal = CastToSystemVolume(masterVolume);

      MIXERCONTROLDETAILS_SIGNED mxcdVolume = { lVal };
      MIXERCONTROLDETAILS mxcd;
      mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
      mxcd.dwControlID = m_dwVolumeControlID;
      mxcd.cChannels = 1;
      mxcd.cMultipleItems = 0;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
      mxcd.paDetails = &mxcdVolume;
      
      if (::mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer),
                                   &mxcd,
                                   MIXER_OBJECTF_HMIXER |
                                   MIXER_SETCONTROLDETAILSF_VALUE)
              != MMSYSERR_NOERROR)
      {
          retVal = false;
      }
    }
    else
    {
      retVal = false;
    }
  }
  
  return retVal;
}

long CAudioUtilsHelper::GetMasterVolume()
{
  long AppVolume=0;

  MIXERCONTROLDETAILS_SIGNED mxcdVolume;
  MIXERCONTROLDETAILS mxcd;
  MMRESULT retVal=NO_ERROR;
  long SysVolume=0;
  float fAppVolume;

  if (!m_bIsWinVer6)
  {
    if (m_hMixer != NULL)
    {//read the mixer values

      mxcd.cbStruct = sizeof(MIXERCONTROLDETAILS);
      mxcd.dwControlID = m_dwVolumeControlID;
      mxcd.cChannels = 1;
      mxcd.cMultipleItems = 0;
      mxcd.cbDetails = sizeof(MIXERCONTROLDETAILS_SIGNED);
      mxcd.paDetails = &mxcdVolume;
      
      retVal = ::mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(m_hMixer),
                                   &mxcd,
                                   MIXER_OBJECTF_HMIXER |
                                   MIXER_SETCONTROLDETAILSF_VALUE);
      
      SysVolume = mxcdVolume.lValue;
      
      fAppVolume = GetSysRelativeVolume(SysVolume)*100;
    }
  }
  else
  {
    m_endpointVolume->GetMasterVolumeLevelScalar(&fAppVolume); //returns values between 0 to 1
    fAppVolume *= 100;
  }
  
  //we don't want to get stuck in mute situation..
  if (fAppVolume < 1)
  {
    if (fAppVolume > 0.5)
      AppVolume = 1;
    else
      AppVolume = 0;
  }
  else
    AppVolume = fAppVolume;

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

bool CAudioUtilsHelper::convertLPWtoStdString(std::string& s, const LPWSTR pw, UINT codepage)
{
    bool res = false;
    char* p = 0;
    int bsz;

    bsz = WideCharToMultiByte(codepage,
        0,
        pw,-1,
        0,0,
        0,0);
    if (bsz > 0) {
        p = new char[bsz];
        int rc = WideCharToMultiByte(codepage,
            0,
            pw,-1,
            p,bsz,
            0,0);
        if (rc != 0) {
            p[bsz-1] = 0;
            s = p;
            res = true;
        }
    }
    delete [] p;
    return res;
}