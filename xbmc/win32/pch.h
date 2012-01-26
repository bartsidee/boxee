//#define WIN32_MEMORY_LEAK_DETECT	// Uncomment this line if you want memory leak detection to be enabled under Windows.

#ifdef WIN32_MEMORY_LEAK_DETECT
#define _CRTDBG_MAP_ALLOC
#endif

#pragma once
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <windows.h>
#include <mmsystem.h>
#include <TCHAR.H>
#include <locale>
#include <comdef.h>
#define DIRECTINPUT_VERSION 0x0800
#include "DInput.h"
#include "DSound.h"
#include "D3D9.h"
#include "D3DX9.h"
#include "boost/shared_ptr.hpp"
#include "SDL\SDL.h"
// anything below here should be headers that very rarely (hopefully never)
// change yet are included almost everywhere.
#include "StdString.h"

#ifdef WIN32_MEMORY_LEAK_DETECT
#include <stdlib.h>
#include <crtdbg.h>

#ifdef _DEBUG
#ifndef NEW_INLINE_WORKAROUND
#define NEW_INLINE_WORKAROUND new ( _NORMAL_BLOCK , __FILE__ , __LINE__ )
#define new NEW_INLINE_WORKAROUND
#endif
#endif  // _DEBUG

#endif