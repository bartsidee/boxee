///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Copyright © NetworkDLS 2002, All rights reserved
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF 
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO 
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A 
// PARTICULAR PURPOSE.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#ifndef _NSWFL_REGISTRY_H_
#define _NSWFL_REGISTRY_H_
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

typedef DWORD (__stdcall* SHDELKEYPROC)(HKEY, LPCTSTR);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int CreateRegKeyStructure(HKEY hKey, const char *sPath);
bool CreateRegistryKey(HKEY hKeyRoot, LPCTSTR pszSubKey);
bool DeleteRegistryKey(HKEY hKeyRoot, LPCTSTR pszSubKey);
bool DeleteRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue);
bool Get_BinaryRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, PVOID pBuffer, DWORD& rdwSize);
bool Get_BOOLRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue);
bool Get_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned long &ulBuff);
bool Get_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned __int64 &u64Buff);
bool Get_StringRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, LPTSTR pszBuffer, DWORD& rdwSize);
bool Set_BinaryRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, PVOID pData, DWORD dwSize);
bool Set_BOOLRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, bool bVal);
bool Set_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned long ulValue);
bool Set_DWORDRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, unsigned __int64 u64Buff);
bool Set_StringRegistryValue(HKEY hKeyRoot, LPCTSTR pszSubKey, LPCTSTR pszValue, LPCTSTR pszString);

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif

