// CxImageCrtDll.cpp : Defines the entry point for the CxImageCrtDll application.
//

#include "stdcrt.h"

#ifdef _WIN32
#define DLL_EXPORT __declspec(dllexport)
#else
#define DLL_EXPORT
#endif

extern "C"
{

BOOL DLL_EXPORT APIENTRY DllMain( HANDLE /*hModule*/, 
                       DWORD  /*ul_reason_for_call*/, 
                       LPVOID /*lpReserved*/
					 )
{
    return TRUE;
}

};
