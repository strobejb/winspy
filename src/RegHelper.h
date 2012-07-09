#ifndef _REGHELPER_INCLUDED
#define _REGHELPER_INCLUDED

#include <windows.h>

#ifdef __cplusplus
extern "C" {
#endif

LONG GetSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nDefault);
BOOL GetSettingBool(HKEY hkey, TCHAR szKeyName[], BOOL nDefault);
LONG GetSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szDefault[], TCHAR szReturnStr[], DWORD nSize);
LONG GetSettingBinary(HKEY hkey, TCHAR szKeyName[], void *buf, ULONG nNumBytes);

LONG WriteSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nValue);
LONG WriteSettingBool(HKEY hkey, TCHAR szKeyName[], BOOL nValue);
LONG WriteSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szString[]);
LONG WriteSettingBinary(HKEY hkey, TCHAR szKeyName[], void *buf, UINT nNumBytes);


#ifdef __cplusplus
}
#endif

#endif