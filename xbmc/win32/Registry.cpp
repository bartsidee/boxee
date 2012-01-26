///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#pragma once
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "system.h"
#include "Registry.h"

#ifdef _WIN32

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool IsWinNT()
{
  return true;
}

// Deletes a value from a given subkey and root
bool DeleteRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
    HKEY hKey;
    LONG lRes;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_SET_VALUE, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError((DWORD)lRes);
        return false;
    }

    lRes = RegDeleteValue(hKey, pszValue);

    RegCloseKey(hKey);

    if(lRes == ERROR_SUCCESS)
        return true;

    SetLastError(lRes);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////

int CreateRegKeyStructure(HKEY hKey, const char *sPath)
{
    char sDir[MAX_PATH];

    int iNameSz = (int)strlen(sPath);
	int iCount = 0;
	int iPos = 0;

    for(iPos = 0; iPos < iNameSz; iPos++)
    {
        if(sPath[iPos] == '\\' || sPath[iPos] == '/')
        {
            sDir[iPos] = '\0';
			if(CreateRegistryKey(hKey, sDir))
			{
				iCount++;
			}
        }

        sDir[iPos] = sPath[iPos];
    }

     sDir[iPos] = '\0';
     if(CreateRegistryKey(hKey, sDir))
	 {
		 iCount++;
	 }

	 return iCount;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Creates a key specified by pszSubKey - you can't create
// keys directly under HKEY_LOCAL_MACHINE in Windows NT or 2000
// just for an extra bit of info.
bool CreateRegistryKey(HKEY hKeyRoot, LPCTSTR pszSubKey)
{
    HKEY hKey;
    DWORD dwFunc;
    LONG  lRet;

    //------------------------------------------------------------------------------

    SECURITY_DESCRIPTOR SD;
    SECURITY_ATTRIBUTES SA;

    if(!InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION))
        return false;

    if(!SetSecurityDescriptorDacl(&SD, true, 0, false))
        return false;

    SA.nLength             = sizeof(SA);
    SA.lpSecurityDescriptor = &SD;
    SA.bInheritHandle      = false;

    //------------------------------------------------------------------------------

    lRet = RegCreateKeyEx(
        hKeyRoot,
        pszSubKey,
        0,
        (LPTSTR)NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        &SA,
        &hKey,
        &dwFunc
    );

    if(lRet == ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        hKey = (HKEY)NULL;
        return true;
    }

    SetLastError((DWORD)lRet);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool DeleteRegistryKey(HKEY hKeyRoot, LPCTSTR pszSubKey)
{
    DWORD dwRet = ERROR_SUCCESS;

    if(IsWinNT())
    {
        // WinNT/2K will not allow you to delete keys which have
        // subkeys/values inside them. MS's platform SDK tells you
        // to use the SHDeleteKey function in shlwapi.dll. This dll
        // is not available on NT platforms without IE 4.0 or later.
        // Because of this I first attempt to delete the key in the
        // hope that it is empty. If that is not possible I load shlwapi
        // and call the function in that. This prevents the app bombing
        // out if the dll can't be found.
        if(RegDeleteKey(hKeyRoot, pszSubKey) != ERROR_SUCCESS)
        {
            HINSTANCE hLibInst = LoadLibrary(_T("shlwapi.dll"));

            if(!hLibInst)
            {
                //throw ERROR_NO_SHLWAPI_DLL;
            }

            #if defined(UNICODE) || defined(_UNICODE)
            SHDELKEYPROC DeleteKeyRecursive = (SHDELKEYPROC)GetProcAddress(hLibInst, "SHDeleteKeyW");
            #else
            SHDELKEYPROC DeleteKeyRecursive = (SHDELKEYPROC)GetProcAddress(hLibInst, "SHDeleteKeyA");
            #endif

            if(!DeleteKeyRecursive)
            {
                FreeLibrary(hLibInst);
                //throw ERROR_NO_SHDELETEKEY;
            }

            dwRet = DeleteKeyRecursive(hKeyRoot, pszSubKey);

            FreeLibrary(hLibInst);
        }
    }
    else {
        // Windows 9x will allow RegDeleteKey to delete keys
        // even if they have subkeys/values.
        dwRet = RegDeleteKey(hKeyRoot, pszSubKey);
    }

    if(dwRet == ERROR_SUCCESS)
        return true;

    SetLastError(dwRet);
    return false;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Fetch a binary value. If the size specified by rdwSize is too small, rdwSize will
// be set to the correct size.
bool Get_BinaryRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, PVOID pBuffer, DWORD& rdwSize)
{
    HKEY  hKey;
    DWORD dwType = REG_BINARY;
    DWORD dwSize = rdwSize;
    LONG lRes   = 0;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_READ, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError((DWORD)lRes);
        return false;
    }

    lRes = RegQueryValueEx(hKey, pszValue, 0, &dwType, (LPBYTE)pBuffer, &dwSize);

    rdwSize = dwSize;
    RegCloseKey(hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    if(dwType != REG_BINARY)
    {
        //throw ERROR_WRONG_TYPE;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Fetch a little endian DWORD from the registry
//(see platform SDK "Registry Value Types")
bool Get_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned __int64 &u64Buff)
{
    HKEY hKey;
    DWORD dwType = REG_QWORD;
    DWORD dwSize = sizeof(u64Buff);
    LONG  lRes;

    u64Buff = 0;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_READ, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegQueryValueEx(hKey, pszValue, 0, &dwType, (LPBYTE)&u64Buff, &dwSize);

    RegCloseKey(hKey);

    if(dwType != REG_QWORD)
        //throw ERROR_WRONG_TYPE;

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Fetch a little endian DWORD from the registry
//(see platform SDK "Registry Value Types")
bool Get_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned long &ulBuff)
{
    HKEY hKey;
    DWORD dwType = REG_DWORD;
    DWORD dwSize = sizeof(ulBuff);
    LONG  lRes;

    ulBuff = 0;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_READ, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegQueryValueEx(hKey, pszValue, 0, &dwType, (LPBYTE)&ulBuff, &dwSize);

    RegCloseKey(hKey);

    if(dwType!=REG_DWORD)
        //throw ERROR_WRONG_TYPE;

    if(lRes!=ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Retrieve a string value. If the given buffer for the string is too small (specified
// by rdwSize), rdwSize is increased to the correct value. If the buffer is bigger than
// the retrieved string, rdwSize is set to the length of the string (in bytes) including
// the terminating null.
bool Get_StringRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR pszBuffer, DWORD& rdwSize)
{
    HKEY  hKey;
    LONG  lRes;
    DWORD dwType = KEY_ALL_ACCESS;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_READ, &hKey);
    if(lRes != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        SetLastError(lRes);
        return false;
    }

    lRes = RegQueryValueEx(hKey, pszValue, NULL, &dwType, (unsigned char*)pszBuffer, &rdwSize);
    if(lRes != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        SetLastError(lRes);
        return false;
    }

    lRes = RegCloseKey(hKey);
    if(lRes != ERROR_SUCCESS)
    {
        RegCloseKey(hKey);
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Writes a binary value to the registry
bool Set_BinaryRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, PVOID pData, DWORD dwSize)
{
    HKEY hKey;
    LONG lRes = 0;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_WRITE, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegSetValueEx(hKey, pszValue, 0, REG_BINARY, (unsigned char*)pData, dwSize);

    RegCloseKey(hKey);

    if(lRes!=ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Writes a DWORD value to the registry
bool Set_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned long ulValue)
{
    HKEY hKey;
    LONG lRes;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_WRITE, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegSetValueEx(hKey, pszValue, 0, REG_DWORD, (unsigned char*)&ulValue, sizeof(ulValue));

    RegCloseKey(hKey);

    if(lRes!=ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Writes a DWORD_PTR value to the registry
bool Set_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned __int64 u64Value)
{
    HKEY hKey;
    LONG lRes;

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_WRITE, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegSetValueEx(hKey, pszValue, 0, REG_QWORD, (LPBYTE)&u64Value, sizeof(u64Value));

    RegCloseKey(hKey);

    if(lRes!=ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

// Writes a string to the registry.
bool Set_StringRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR pszString)
{
    HKEY  hKey;
    LONG  lRes;
    DWORD dwSize = lstrlen(pszString) * sizeof(TCHAR);

    lRes = RegOpenKeyEx(hKeyRoot, pszSubKey, 0, KEY_WRITE, &hKey);

    if(lRes != ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    lRes = RegSetValueEx(hKey, pszValue, 0, REG_SZ, (unsigned char*)pszString, dwSize);

    RegCloseKey(hKey);

    if(lRes!=ERROR_SUCCESS)
    {
        SetLastError(lRes);
        return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////

bool Set_BOOLRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, bool bVal)
{
	DWORD dwVal = 0;

	if(bVal)
	{
		dwVal = 1;
	}

	return Set_DWORDRegistryValue(hKeyRoot, pszSubKey, pszValue, dwVal);
}

///////////////////////////////////////////////////////////////////////////////////////////

bool Get_BOOLRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue)
{
	DWORD dwVal = 0;

	Get_DWORDRegistryValue(hKeyRoot, pszSubKey, pszValue, dwVal);

	return (dwVal > 0);
}

///////////////////////////////////////////////////////////////////////////////////////////
#endif

