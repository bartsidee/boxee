#pragma once

#include <winsock2.h>
#include <map>

class CWinRemoteControl
{
public:
  CWinRemoteControl();
  ~CWinRemoteControl();
 
  bool Initialize();
  void Disconnect() {};
  void Reset();
  void Update();
  WORD GetButton();
  bool IsHolding();
  
  void SetButton(DWORD button);

  void SetInstance(HINSTANCE instance) {
	  m_instance = instance;
  }

private:
  WORD  m_button;
  bool  m_isHolding;
  bool  m_bInitialized;
  bool  m_bRemoteFound;

  std::map<DWORD, const char*> m_keyMapping;

  HINSTANCE m_instance;

  bool m_isConnecting;
  CStdString m_deviceName;
  CStdString m_keyCode;
};

extern CWinRemoteControl g_BoxeeRemoteControl;
