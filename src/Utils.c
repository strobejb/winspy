//
//	Utils.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Lots of utility and general helper functions.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include "Utils.h"

int atoi( const char *string );

//
// Style helper functions
//
UINT AddStyle(HWND hwnd, UINT style)
{
	UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLong(hwnd, GWL_STYLE,  oldstyle | style);
	return oldstyle;
}

UINT AddDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
	return AddStyle(GetDlgItem(hwnd, nCtrlId), style);
}

UINT DelStyle(HWND hwnd, UINT style)
{
	UINT oldstyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLong(hwnd, GWL_STYLE, oldstyle & ~style);
	return oldstyle;
}

UINT DelDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style)
{
	return DelStyle(GetDlgItem(hwnd, nCtrlId), style);
}

BOOL EnableDlgItem(HWND hwnd, UINT nCtrlId, BOOL fEnabled)
{
	return EnableWindow(GetDlgItem(hwnd, nCtrlId), fEnabled);
}

BOOL ShowDlgItem(HWND hwnd, UINT nCtrlId, DWORD dwShowCmd)
{
	return ShowWindow(GetDlgItem(hwnd, nCtrlId), dwShowCmd);
}

int WINAPI GetRectHeight(RECT *rect)
{
	return rect->bottom - rect->top;
}

int WINAPI GetRectWidth(RECT *rect)
{
	return rect->right - rect->left;
}


//
//	Convert the specified string (with a hex-number in it)
//  into the equivalent hex-value
//
UINT _tstrtoib16(TCHAR *szHexStr)
{
	UINT  num = 0;

	TCHAR *hexptr = szHexStr;
	UINT  ch = *hexptr++;

	while(isxdigit(ch))
	{
		UINT x = ch - _T('0');
		if(x > 9 && x <= 42) x -= 7;		//A-Z
		else if(x > 42)   x -= 39;			//a-z
					
		num = (num << 4) | (x & 0x0f);
		ch = *hexptr++;
	}
	
	return num;
}

DWORD GetNumericValue(HWND hwnd, int base)
{
	TCHAR szAddressText[128];

	GetWindowText(hwnd, szAddressText, sizeof(szAddressText) / sizeof(TCHAR));

	switch(base)
	{
	case 1:
	case 16:			//base is currently hex
		return _tstrtoib16(szAddressText);

	case 0:
	case 10:			//base is currently decimal
		return _ttoi(szAddressText);

	default:
		return 0;
	}
}

DWORD GetDlgItemBaseInt(HWND hwnd, UINT ctrlid, int base)
{
	return (DWORD)GetNumericValue(GetDlgItem(hwnd, ctrlid), base);
}

//
//	Copied from uxtheme.h
//  If you have this new header, then delete these and
//  #include <uxtheme.h> instead!
//
#define ETDT_DISABLE        0x00000001
#define ETDT_ENABLE         0x00000002
#define ETDT_USETABTEXTURE  0x00000004
#define ETDT_ENABLETAB      (ETDT_ENABLE  | ETDT_USETABTEXTURE)

// 
typedef HRESULT (WINAPI * ETDTProc) (HWND, DWORD);

//
//	Try to call EnableThemeDialogTexture, if uxtheme.dll is present
//
BOOL EnableDialogTheme(HWND hwnd)
{
	HMODULE hUXTheme;
	ETDTProc fnEnableThemeDialogTexture;

	hUXTheme = LoadLibrary(_T("uxtheme.dll"));

	if(hUXTheme)
	{
		fnEnableThemeDialogTexture = 
			(ETDTProc)GetProcAddress(hUXTheme, "EnableThemeDialogTexture");

		if(fnEnableThemeDialogTexture)
		{
			fnEnableThemeDialogTexture(hwnd, ETDT_ENABLETAB);
			
			FreeLibrary(hUXTheme);
			return TRUE;
		}
		else
		{
			// Failed to locate API!
			FreeLibrary(hUXTheme);
			return FALSE;
		}
	}
	else
	{
		// Not running under XP? Just fail gracefully
		return FALSE;
	}
}
