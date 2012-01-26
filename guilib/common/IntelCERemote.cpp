#include "system.h"

#ifdef HAS_INTELCE

#include "IntelCERemote.h"

#include "log.h"
#include "ButtonTranslator.h"

CRemoteControl g_RemoteControl;

#define RC_DEVICE_NAME "intelce"

#define NOF_REMOTE_KEYS 46

struct RemoteKeyType
{
  int device;
  int number;
  const char* key;
} RemoteKeys[] =
{
    { 0x45, 0x12, "Power" },
    { 0x45, 0xe7, "Source" },
    { 0x38, 0x23, "Sleep" },
    { 0x45, 0xde, "Guide" },
    { 0x38, 0x0e, "VolUp" },
    { 0x38, 0x18, "Mute" },
    { 0x38, 0x21, "ChanUp" },
    { 0x38, 0x0f, "VolDown" },
    { 0x45, 0x34, "PIP" },
    { 0x38, 0x22, "ChanDown" },
    { 0x45, 0x14, "Stop" },
    { 0x45, 0x15, "Play" },
    { 0x45, 0x00, "Pause" },
    { 0x45, 0x19, "Rewind" },
    { 0x45, 0x2b, "Back" },
    { 0x45, 0x13, "Forward" },
    { 0x45, 0x23, "Replay" },
    { 0x45, 0x33, "Record" },
    { 0x45, 0x24, "Skip" },
    { 0x45, 0x80, "Up" },
    { 0x45, 0x51, "Left" },
    { 0x45, 0x21, "OK" },
    { 0x45, 0x4d, "Right" },
    { 0x45, 0x81, "Down" },
    { 0x45, 0x22, "Exit" },
    { 0x45, 0x16, "Info" },
    { 0x45, 0x30, "Keyboard" },
    { 0x45, 0x01, "One" },
    { 0x45, 0x02, "Two" },
    { 0x45, 0x03, "Three" },
    { 0x45, 0x39, "Favorite" },
    { 0x45, 0x04, "Four" },
    { 0x45, 0x05, "Five" },
    { 0x45, 0x06, "Six" },
    { 0x45, 0x2f, "Home" },
    { 0x45, 0x07, "Seven" },
    { 0x45, 0x08, "Eight" },
    { 0x45, 0x09, "Nine" },
    { 0x45, 0x32, "Setup" },
    { 0x45, 0x31, "BackSpace" },
    { 0x45, 0x0a, "Zero" },
    { 0x45, 0x25, "Enter" },
    { 0x45, 0x35, "Function1" },
    { 0x45, 0x36, "Function2" },
    { 0x45, 0x38, "Function4" },
    { 0x45, 0x37, "Function3" }
};

CRemoteControl::CRemoteControl()
{
  m_initialized = false;
  m_newData = false;
  m_pInterface = NULL;
  m_isHolding = false;
}

CRemoteControl::~CRemoteControl()
{
  Disconnect();
}

void CRemoteControl::setUsed(bool value)
{
}

void CRemoteControl::Reset()
{
}

void CRemoteControl::Disconnect() 
{
  if (m_pInterface != NULL)
  {
    delete m_pInterface;
    m_pInterface = NULL;
  }
} 

void CRemoteControl::setDeviceName(const CStdString& value)
{
}

const char* CRemoteControl::TranslateToButtonName(int device, int number)
{
  for (int i = 0; i < NOF_REMOTE_KEYS; i++)
  {
    if (RemoteKeys[i].device == device && RemoteKeys[i].number == number)
    {
      return RemoteKeys[i].key;
    }
  }

  return "Unknown";
}

void CRemoteControl::SetEvent(WORD button, bool isHolding)
{
  CSingleLock lock(m_lock);

  m_button = button;
  m_isHolding = isHolding;
  m_newData = true;
}

static int PICInterfaceCallBack( UINT8 type, UINT8 length, void* data, void* clientData)
{
  CRemoteControl* obj = (CRemoteControl*) clientData;

  UINT8* cdata = (UINT8*)data;

  if (type == LR_PIC_IR || type == LR_PIC_IR_REPEAT_START || type == LR_PIC_IR_REPEAT_STOP)
  {
    if (length != 3)
    {
      return 0;
    }

    bool isHolding = (type == LR_PIC_IR_REPEAT_START);
    PicBufferIR* pIR = PicBufferIR::unserialize( cdata );
    {
      const char* buttonName = CRemoteControl::TranslateToButtonName(pIR->irDevice, pIR->irNumber);
      WORD button = CButtonTranslator::GetInstance().TranslateLircRemoteString(RC_DEVICE_NAME, buttonName);
      if (button != 0)
      {
        obj->SetEvent(button, isHolding);
      }
    }
    delete pIR;
  }

  return 0;
}

void CRemoteControl::Initialize()
{
  int retCode;
  m_pInterface = new LR_PICInterface(PICInterfaceCallBack, this);
  if (m_pInterface == NULL)
  {
    CLog::Log(LOGERROR, "IR: problem with interface creation, exit" );
    return;
  }

  retCode = m_pInterface->Init((INT8*)("/dev/ttyS1"));
  if(retCode != PIC_SUCCESS)
  {
    CLog::Log(LOGERROR, "IR: problem with interface init" );
    return;
  }
/*
 * all IO Event functions are deprecated starting with SDK 21.2
 *
  retCode = m_pInterface->setIOTimerValue(1);
  if(retCode != PIC_SUCCESS)
  {
    CLog::Log(LOGERROR, "IR: problem with interface setIOTimerValue" );
    return;
  }
*/
  m_initialized = true;
}

void CRemoteControl::Update()
{
}

WORD CRemoteControl::GetButton()
{
  CSingleLock lock(m_lock);

  if (!m_newData)
    return 0;

  if (!m_isHolding)
    m_newData = false;

  return m_button;
}

bool CRemoteControl::IsHolding()
{
  CSingleLock lock(m_lock);
  return m_isHolding;
}

#endif
