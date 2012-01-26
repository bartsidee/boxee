#include "RemoteControl.h"
#include "ButtonTranslator.h"
#include "Settings.h"
#include "windows.h"
#include <strsafe.h>
#include <SDL/SDL_syswm.h>
#include "utils/log.h"

#ifdef _WIN32
extern HWND g_hWnd;
#endif

CWinRemoteControl g_BoxeeRemoteControl;

CWinRemoteControl::CWinRemoteControl()
{
  m_bInitialized = false;
  m_isConnecting = false;

  Reset();

  // Initialize key map from remote raw input to IR Server Suite code
  m_keyMapping[140290] = "31708"; // Back
  m_keyMapping[176640] = "31720"; // Rec
  m_keyMapping[176128] = "31721"; // Play 02b000
  m_keyMapping[177152] = "31722"; // Rewind 
  m_keyMapping[176896] = "31723"; // FFrwrd 02b300
  m_keyMapping[177920] = "31718"; // Stop
  m_keyMapping[177664] = "31716"; // Begin
  m_keyMapping[177408] = "31717"; // End
  m_keyMapping[176384] = "31719"; // Pause
  m_keyMapping[196861] = "31730"; // Menu

}

CWinRemoteControl::~CWinRemoteControl()
{
}

void CWinRemoteControl::Reset()
{
  m_isHolding = false;
  m_button = 0;
}

void CWinRemoteControl::Update()
{
}

void ShowLastError()
{
  LPVOID lpMsgBuf;
	LPVOID lpDisplayBuf;
	LPTSTR lpszFunction = "";
	DWORD dw = GetLastError(); 

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | 
		FORMAT_MESSAGE_FROM_SYSTEM |
		FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		dw,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR) &lpMsgBuf,
		0, NULL );

	lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT, 
		(lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR)); 

	StringCchPrintf((LPTSTR)lpDisplayBuf, 
		LocalSize(lpDisplayBuf) / sizeof(TCHAR),
		TEXT("%s failed with error %d: %s"), 
		lpszFunction, dw, lpMsgBuf); 

	MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK); 

	LocalFree(lpMsgBuf);
	LocalFree(lpDisplayBuf);

}

HHOOK m_hook = NULL;

LRESULT CALLBACK GetMsgProc(int nCode, WPARAM wParam, LPARAM lParam) 
{ 
    if (nCode < 0) // do not process message 
        return CallNextHookEx(m_hook, nCode, wParam, lParam); 

	PMSG msg = (PMSG)lParam;

	if (msg->message == WM_INPUT)
	{
		WPARAM inputCode = GET_RAWINPUT_CODE_WPARAM(msg->wParam);
			if ((inputCode != RIM_INPUT) && (inputCode != RIM_INPUTSINK)) {
			return -1;
		}

		UINT size;
		if (GetRawInputData((HRAWINPUT)msg->lParam, RID_INPUT, NULL, &size, sizeof(RAWINPUTHEADER)) == -1) {
			return -1;
		}
 
		PRAWINPUT input = (PRAWINPUT)malloc(size);
        if (GetRawInputData( (HRAWINPUT)msg->lParam, RID_INPUT, input, &size, sizeof(RAWINPUTHEADER) ) != size)
        {
           free(input);
           return -1;
        }

		BYTE *data = input->data.hid.bRawData;

		DWORD key = 0;

		key = ((static_cast<DWORD>(data[0]) << 16) | data[1] << 8 | data[2]);

    CLog::Log( LOGERROR, "GetMsgProc key =  %ld", key );
		g_BoxeeRemoteControl.SetButton(key);

		free(input); 

	}
    return CallNextHookEx(m_hook, nCode, wParam, lParam); 
} 


// This function regsiters to receive raw input from the remote control
// and sets up a windows hook to receive the WM_INPUT messages
// since the main message handling occurs inside the SDL implementation
bool CWinRemoteControl::Initialize()
{
	//SDL_SysWMinfo wmInfo;
	//SDL_VERSION(&wmInfo.version);
	//int te = SDL_GetWMInfo(&wmInfo );
	HWND hWnd = g_hWnd; //wmInfo.window;

	UINT nInputDevices; 
	if (GetRawInputDeviceList(NULL, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) != 0) 
	{ 
		return false; 
	} 

	RAWINPUTDEVICELIST* rawinputDeviceList = new RAWINPUTDEVICELIST[nInputDevices];

	UINT ntmp = nInputDevices; 
	if (GetRawInputDeviceList(rawinputDeviceList, &nInputDevices, sizeof(RAWINPUTDEVICELIST)) != ntmp) {
		delete [] rawinputDeviceList;
		return false; 
	} 

	for (UINT i = 0; i < nInputDevices; ++i) 
	{ 
		if (rawinputDeviceList[i].dwType == RIM_TYPEHID)
		{
			// Get device info
			RID_DEVICE_INFO devinforemote = { sizeof devinforemote, }; 
			UINT sz = sizeof devinforemote; 
			if ((int)GetRawInputDeviceInfo(rawinputDeviceList[i].hDevice, RIDI_DEVICEINFO, &devinforemote, &sz) < 0) { 
				return false; 
			} 
			
			// The constants represent two magic numbers: 0xffbc and 0x88
			if (devinforemote.hid.usUsage == 136 && devinforemote.hid.usUsagePage == 65468) 
			{

				RAWINPUTDEVICE rid[3];
				rid[0].usUsagePage = 0xFFBC;      // adds HID remote control
				rid[0].usUsage = 0x88;
				rid[0].dwFlags = RIDEV_INPUTSINK;
				rid[0].hwndTarget = hWnd;
				
				// TODO: Check whether this is required
				rid[1].usUsagePage = 0x0C;      // adds HID remote control
				rid[1].usUsage = 0x01;
				rid[1].dwFlags = RIDEV_INPUTSINK;
				rid[1].hwndTarget = hWnd;

				rid[2].usUsagePage = 0x0C;      // adds HID remote control
				rid[2].usUsage = 0x80;
				rid[2].dwFlags = RIDEV_INPUTSINK;
				rid[2].hwndTarget = hWnd;

				if (!RegisterRawInputDevices(rid, (UINT)3, (UINT) sizeof(rid[0])))
				{
					//ShowLastError();
				    return false;
				}
			}
		}
	}

	delete [] rawinputDeviceList;

	// Register windows hook
	m_hook = SetWindowsHookEx( WH_GETMESSAGE, GetMsgProc, (HINSTANCE) m_instance, GetCurrentThreadId()); 
	if (m_hook == NULL) {
		//ShowLastError();
		return false;
	}

	return true;
}

void CWinRemoteControl::SetButton(DWORD buttonCode)
{
//#if defined(HAS_LIRC) || defined(HAS_IRSERVERSUITE)
	std::map<DWORD,const char*>::iterator it = m_keyMapping.find(buttonCode);
	if (it != m_keyMapping.end()) 
	{
		m_button = CButtonTranslator::GetInstance().TranslateLircRemoteString("Microsoft MCE", it->second);
	}
//#endif
}

WORD CWinRemoteControl::GetButton()
{
  WORD button = m_button;
  m_button = 0;
  return button;
}

bool CWinRemoteControl::IsHolding()
{
  return m_isHolding;
}


