//
//	RegHelper.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Implements registry helper functions
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include "RegHelper.h"


LONG GetSettingBinary(HKEY hkey, TCHAR szKeyName[], void *buf, ULONG nNumBytes)
{
	DWORD type = REG_BINARY;
	ULONG len = nNumBytes;

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, szKeyName, 0, &type, (BYTE *)buf, &nNumBytes))
	{
		if(type != REG_BINARY) return 0;
		else return nNumBytes;
	}
	else
	{
		return 0;
	}
}

LONG GetSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nDefault)
{
	DWORD type;
	LONG value;
	ULONG len = sizeof(value);

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, szKeyName, 0, &type, (BYTE *)&value, &len))
	{
		if(type != REG_DWORD) return nDefault;
		return value;
	}
	else
	{
		return nDefault;
	}
}

BOOL GetSettingBool(HKEY hkey, TCHAR szKeyName[], BOOL nDefault)
{
	DWORD type;
	BOOL  value;
	ULONG len = sizeof(value);

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, szKeyName, 0, &type, (BYTE *)&value, &len))
	{
		if(type != REG_DWORD) return nDefault;
		return value != 0;
	}
	else
	{
		return nDefault;
	}
}

LONG GetSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szDefault[], TCHAR szReturnStr[], DWORD nSize)
{
	DWORD type = REG_SZ;
	TCHAR bigbuf[256];
	ULONG len = sizeof(bigbuf);

	if(ERROR_SUCCESS == RegQueryValueEx(hkey, szKeyName, 0, &type, (BYTE *)bigbuf, &len))
	{
		if(type != REG_SZ) return 0;
		memcpy(szReturnStr, bigbuf, len+sizeof(TCHAR));
		return len;
	}
	else
	{
		len = min(nSize, (DWORD)lstrlen(szDefault) * sizeof(TCHAR));
		memcpy(szReturnStr, szDefault, len+sizeof(TCHAR));
		return len;
	}
}

LONG WriteSettingInt(HKEY hkey, TCHAR szKeyName[], LONG nValue)
{
	return RegSetValueEx(hkey, szKeyName, 0, REG_DWORD, (BYTE *)&nValue, sizeof(nValue));
}

LONG WriteSettingBool(HKEY hkey, TCHAR szKeyName[], BOOL nValue)
{
	return RegSetValueEx(hkey, szKeyName, 0, REG_DWORD, (BYTE *)&nValue, sizeof(nValue));
}

LONG WriteSettingStr(HKEY hkey, TCHAR szKeyName[], TCHAR szString[])
{
	return RegSetValueEx(hkey, szKeyName, 0, REG_SZ, (BYTE *)szString, (lstrlen(szString) + 1) * sizeof(TCHAR));
}

LONG WriteSettingBinary(HKEY hkey, TCHAR szKeyName[], void *buf, UINT nNumBytes)
{
	return RegSetValueEx(hkey, szKeyName, 0, REG_BINARY, (BYTE *)buf, nNumBytes);
}