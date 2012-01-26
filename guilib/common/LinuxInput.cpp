#include "system.h"

#ifdef HAS_LIRC

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <linux/input.h>
#include "LinuxInput.h"
#include "ButtonTranslator.h"
#include "log.h"
#include "Settings.h"

CLinuxInput::CLinuxInput()
{
  m_fd = -1;
  m_bInitialized = false;
  m_skipHold = false;
  m_bNeedLongClick = false;
  m_lastScrollTime = 0;
  m_prevEventCode  = 0;
  m_prevEventValue = 0;
  Reset();
}

CLinuxInput::~CLinuxInput()
{
  if (m_fd != -1)
    close(m_fd);
}

void CLinuxInput::Reset()
{
  m_isHolding = false;
  m_bNeedLongClick = false;
  m_button = 0;
}

bool CLinuxInput::Initialize()
{
  // Open the socket from which we will receive the remote commands 
  m_fd = open(m_strDevice.c_str(), O_RDONLY | O_NONBLOCK);
  if (m_fd == -1)  
  {
    CLog::Log(LOGERROR, "LinuxInput %s: open device failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  // exclusive grab - so no input is sent to console
  if (ioctl(m_fd, EVIOCGRAB, 1) == -1)
  {
    CLog::Log(LOGERROR, "LinuxInput %s: cant grab exclusive: %s", __FUNCTION__, strerror(errno));
  }

  // Set the socket to non-blocking
  int opts = fcntl(m_fd,F_GETFL);
  if (opts == -1) 
  {
    CLog::Log(LOGERROR, "LinuxInput %s: fcntl(F_GETFL) failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }
	
  opts = (opts | O_NONBLOCK);
  if (fcntl(m_fd,F_SETFL,opts) == -1) 
  {
    CLog::Log(LOGERROR, "LinuxInput %s: fcntl(F_SETFL) failed: %s", __FUNCTION__, strerror(errno));
    return false;
  }

  m_bInitialized = true;
  return true;
}

void CLinuxInput::SetDeviceName(const CStdString &strName)
{
  m_strDevice = strName;
}

void CLinuxInput::Update()
{
  if (!m_bInitialized)
    return;

  Uint32 now = SDL_GetTicks(); 

  struct input_event event;

  // Read a line from the socket
  while ( read(m_fd, &event, sizeof(struct input_event)) != -1 )
  {
    if (event.type == EV_SYN)
      continue;

    bool bKeyUp = (event.value == 0); // hack - key up is marked as 0 in value (apple-ir)
    bool bNewKey = (event.value == 1); // apple remote
    bool bLongClick = (event.value == 2); // apple remote

    // CLog::Log(LOGDEBUG,"Remote, LinuxInput - got click <%d>, key-up: %d, new-key: %d, long click: %d. need long: %d. holding: %d", event.code, bKeyUp, bNewKey, bLongClick, m_bNeedLongClick, m_isHolding);

    CStdString buttonName;
    buttonName.Format("%d", event.code);

    /* tsella - very very ugly patch because vulkan does not want changes in ButtonTranslator
    /  if Wheel (8) or HWheel (6), and the event.value is 1, add 1 to the buttonName, effectively
    /  Wheel is 8 (value -1) and 9 (value 1), HWheel is 6 (value -1) and 7 (value 1).
    /
    /  cleaner approach: add ButtonTranslator TranslateLircWheel() with a wheel=positive|negative
    /                    hint in LircMap.xml
    */
    if (event.code == LINUX_INPUT_EVENT_WHEEL || event.code == LINUX_INPUT_EVENT_HWHEEL)
    {
      // if previous event happened more than LINUX_INPUT_SCROLL_SENSITIVITY milliseconds ago
      // and the last event code and event value were not the same
      bool bAllowScroll = (m_prevEventCode == event.code && m_prevEventValue == event.value);
      bAllowScroll = bAllowScroll || !(m_prevEventCode == LINUX_INPUT_EVENT_WHEEL || m_prevEventCode == LINUX_INPUT_EVENT_HWHEEL);
      if (now - m_lastScrollTime >= (unsigned int)LINUX_INPUT_SCROLL_SENSITIVITY && bAllowScroll)
      {
        if (event.value == 1)
          buttonName.Format("%d", event.code + 1);

        // CLog::Log(LOGDEBUG,"Remote, LinuxInput - is %sWheel, direction %s, tick diff %d", (event.code == LINUX_INPUT_EVENT_HWHEEL) ? "H" : "", (event.value == 1) ? "positive" : "negative", now - m_lastScrollTime);
        m_button = g_buttonTranslator.TranslateLircRemoteString(m_strDevice, buttonName);
        m_lastScrollTime = now;
      }
      m_prevEventCode  = event.code;
      m_prevEventValue = event.value;
      return;
    }
    m_prevEventCode  = event.code;
    m_prevEventValue = event.value;

    // when key up arrives we either ignore it (if not waiting for long click)
    // or  make the framework understand there's a click to process (long click)
    if (bKeyUp)
    {
      if (m_bNeedLongClick)
        m_button = bLongClick?g_buttonTranslator.TranslateLircLongClick(m_strDevice, buttonName):
                              g_buttonTranslator.TranslateLircRemoteString(m_strDevice, buttonName);
      m_bNeedLongClick = false;
      return;  
    }

    if (bNewKey)
    {
      if (!g_buttonTranslator.NeedsLircLongClick(m_strDevice, buttonName))
        m_button = g_buttonTranslator.TranslateLircRemoteString(m_strDevice, buttonName);
      else
        m_bNeedLongClick = true;

      m_firstClickTime = now;
      m_isHolding = false;
      m_skipHold = m_bNeedLongClick;
    }
    else if (m_bNeedLongClick && bLongClick && now - m_firstClickTime >= (unsigned int)g_advancedSettings.m_remoteRepeat)
    {
      m_button = g_buttonTranslator.TranslateLircLongClick(m_strDevice, buttonName);
    }
    else if (!m_bNeedLongClick && now - m_firstClickTime >= (unsigned int)g_advancedSettings.m_remoteRepeat && !m_skipHold)
    {
      m_button = g_buttonTranslator.TranslateLircRemoteString(m_strDevice, buttonName);
      m_isHolding = true;
    }
    else
    {
      m_isHolding = false;
      m_button = 0;
    }
  }
}

WORD CLinuxInput::GetButton()
{
  return m_button;
}

bool CLinuxInput::IsHolding()
{
  return m_isHolding;
}

#endif

