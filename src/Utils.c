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

// Needed for GetDpiForWindow() (Windows 10 1607+)
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif

#include <windows.h>
#include <tchar.h>
#include <malloc.h>
#include "Utils.h"

int atoi( const char *string );

//
//	Enable/Disable privilege with specified name (for current process)
//
BOOL EnablePrivilege(TCHAR *szPrivName, BOOL fEnable)
{
	TOKEN_PRIVILEGES tp;
	LUID	luid;
	HANDLE	hToken;

	if(!LookupPrivilegeValue(NULL, szPrivName, &luid))
		return FALSE;

	if(!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
		return FALSE;
	
	tp.PrivilegeCount			= 1;
	tp.Privileges[0].Luid		= luid;
	tp.Privileges[0].Attributes = fEnable ? SE_PRIVILEGE_ENABLED : 0;
	
	AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(tp), NULL, NULL);

	CloseHandle(hToken);

	return (GetLastError() == ERROR_SUCCESS);
}


BOOL EnableDebugPrivilege()
{
	return EnablePrivilege(SE_DEBUG_NAME, TRUE);
}


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
UINT_PTR _tstrtoib16(TCHAR *szHexStr)
{
	UINT_PTR  num = 0;

	TCHAR *hexptr = szHexStr;
	UINT_PTR  ch = *hexptr++;

	while(isxdigit((int)ch))
	{
		UINT_PTR x = ch - _T('0');
		if(x > 9 && x <= 42) x -= 7;		//A-Z
		else if(x > 42)   x -= 39;			//a-z
					
		num = (num << 4) | (x & 0x0f);
		ch = *hexptr++;
	}
	
	return num;
}

DWORD_PTR GetNumericValue(HWND hwnd, int base)
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

DWORD_PTR GetDlgItemBaseInt(HWND hwnd, UINT ctrlid, int base)
{
	return (DWORD_PTR)GetNumericValue(GetDlgItem(hwnd, ctrlid), base);
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

#pragma comment(lib, "version.lib")

//
//	Get the specified file-version information string from a file
//	
//	szItem	- version item string, e.g:
//		"FileDescription", "FileVersion", "InternalName", 
//		"ProductName", "ProductVersion", etc  (see MSDN for others)
//
TCHAR *GetVersionString(TCHAR *szFileName, TCHAR *szValue, TCHAR *szBuffer, ULONG nLength)
{
	DWORD  len;
	PVOID  ver;	
	DWORD  *codepage;
	TCHAR  fmt[0x40];
	PVOID  ptr = 0;
	BOOL   result = FALSE;
	
	szBuffer[0] = '\0';

	len = GetFileVersionInfoSize(szFileName, 0);

	if(len == 0 || (ver = malloc(len)) == 0)
		return NULL;

	if(GetFileVersionInfo(szFileName, 0, len, ver))
	{
		if(VerQueryValue(ver, TEXT("\\VarFileInfo\\Translation"), &codepage, &len))
		{
			wsprintf(fmt, TEXT("\\StringFileInfo\\%04x%04x\\%s"), (*codepage) & 0xFFFF, 
					(*codepage) >> 16, szValue);
			
			if(VerQueryValue(ver, fmt, &ptr, &len))
			{
				lstrcpyn(szBuffer, (TCHAR*)ptr, min(nLength, len));
				result = TRUE;
			}
		}
	}

	free(ver);
	return result ? szBuffer : NULL;
}


//
//	Compare Arch (32 or 64 bit) of our process with the process of the input window
//
BOOL ProcessArchMatches(HWND hwnd)
{
	static FARPROC fnIsWow64Process = NULL;
	static BOOL bIsWow64ProcessAbsents = FALSE;
	DWORD dwProcessId;
	HANDLE hProcess;
	BOOL bIsWow64Process;
	BOOL bSuccess;

	if(GetProcessorArchitecture() == PROCESSOR_ARCHITECTURE_INTEL)
		return TRUE;

	if(!fnIsWow64Process)
	{
		if(bIsWow64ProcessAbsents)
		{
#ifdef _WIN64
			return FALSE;
#else // ifndef _WIN64
			return TRUE;
#endif // _WIN64
		}

		fnIsWow64Process = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "IsWow64Process");
		if(!fnIsWow64Process)
		{
			bIsWow64ProcessAbsents = TRUE;

#ifdef _WIN64
			return FALSE;
#else // ifndef _WIN64
			return TRUE;
#endif // _WIN64
		}
	}

	GetWindowThreadProcessId(hwnd, &dwProcessId);

	hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, dwProcessId);
	if(!hProcess)
		return FALSE; // assume no match, to be on the safe side

	bSuccess = ((BOOL (WINAPI *)(HANDLE, PBOOL))fnIsWow64Process)(hProcess, &bIsWow64Process);

	CloseHandle(hProcess);

	if(bSuccess)
	{
#ifdef _WIN64
		return !bIsWow64Process;
#else // ifndef _WIN64
		return bIsWow64Process;
#endif // _WIN64
	}
	else
		return FALSE; // assume no match, to be on the safe side
}


//
// Assumes to support only PROCESSOR_ARCHITECTURE_INTEL and PROCESSOR_ARCHITECTURE_AMD64
//
WORD GetProcessorArchitecture()
{
#ifdef _WIN64
	return PROCESSOR_ARCHITECTURE_AMD64;
#else // ifndef _WIN64
	static WORD wProcessorArchitecture = PROCESSOR_ARCHITECTURE_UNKNOWN;

	if(wProcessorArchitecture == PROCESSOR_ARCHITECTURE_UNKNOWN)
	{
		FARPROC fnGetNativeSystemInfo = NULL;
		SYSTEM_INFO siSystemInfo;

		fnGetNativeSystemInfo = GetProcAddress(GetModuleHandle(TEXT("kernel32")), "GetNativeSystemInfo");

		if(fnGetNativeSystemInfo)
		{
			((VOID (WINAPI *)(LPSYSTEM_INFO))fnGetNativeSystemInfo)(&siSystemInfo);

			wProcessorArchitecture = siSystemInfo.wProcessorArchitecture;
		}
		else
			wProcessorArchitecture = PROCESSOR_ARCHITECTURE_INTEL;
	}

	return wProcessorArchitecture;
#endif // _WIN64
}

//
// DPI of the monitor hwnd currently lives on.
//
int GetWindowDpi(HWND hwnd)
{
	return GetDpiForWindow(hwnd);
}

//
// Scale a 96-dpi pixel value for hwnd's current monitor.
//
int DpiScale(HWND hwnd, int value)
{
	return MulDiv(value, GetWindowDpi(hwnd), USER_DEFAULT_SCREEN_DPI);
}

//
// Return a copy of hbmSrc scaled from 96 dpi up to the given dpi, using
// stretchMode (e.g. HALFTONE for plain bitmaps, COLORONCOLOR for bitmaps
// that will be used with a transparency mask - HALFTONE can blend mask-
// colored pixels into neighbouring ones at the edges, corrupting the mask).
// Returns hbmSrc itself (not a copy) when no scaling is needed - the
// caller should only free the result if it's different from hbmSrc.
//
HBITMAP CreateDpiScaledBitmap(HBITMAP hbmSrc, int dpi, int stretchMode)
{
	BITMAP bm;
	HDC hdcScreen, hdcSrc, hdcDst;
	HBITMAP hbmDst, hbmOldSrc, hbmOldDst;
	int cxNew, cyNew;

	if(dpi <= USER_DEFAULT_SCREEN_DPI || hbmSrc == NULL)
		return hbmSrc;

	if(!GetObject(hbmSrc, sizeof(bm), &bm))
		return hbmSrc;

	cxNew = MulDiv(bm.bmWidth,  dpi, USER_DEFAULT_SCREEN_DPI);
	cyNew = MulDiv(bm.bmHeight, dpi, USER_DEFAULT_SCREEN_DPI);

	hdcScreen = GetDC(0);
	hdcSrc    = CreateCompatibleDC(hdcScreen);
	hdcDst    = CreateCompatibleDC(hdcScreen);
	hbmDst    = CreateCompatibleBitmap(hdcScreen, cxNew, cyNew);

	if(hdcSrc == NULL || hdcDst == NULL || hbmDst == NULL)
	{
		if(hbmDst) DeleteObject(hbmDst);
		if(hdcSrc) DeleteDC(hdcSrc);
		if(hdcDst) DeleteDC(hdcDst);
		ReleaseDC(0, hdcScreen);
		return hbmSrc;
	}

	hbmOldSrc = SelectObject(hdcSrc, hbmSrc);
	hbmOldDst = SelectObject(hdcDst, hbmDst);

	SetStretchBltMode(hdcDst, stretchMode);
	SetBrushOrgEx(hdcDst, 0, 0, NULL);
	StretchBlt(hdcDst, 0, 0, cxNew, cyNew, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	SelectObject(hdcSrc, hbmOldSrc);
	SelectObject(hdcDst, hbmOldDst);

	DeleteDC(hdcSrc);
	DeleteDC(hdcDst);
	ReleaseDC(0, hdcScreen);

	return hbmDst;
}

//
// Like CreateDpiScaledBitmap, but for 32bpp per-pixel-alpha (premultiplied)
// bitmaps that will be drawn with AlphaBlend. The destination must be a
// real 32bpp DIB section - a screen-compatible bitmap doesn't reliably
// preserve the alpha channel's meaning through StretchBlt/AlphaBlend.
//
// Unlike CreateDpiScaledBitmap, this also scales DOWN (dpi < 96) - callers
// use that to fine-scale from whichever pre-baked size is the closest
// match for the real DPI (see FindTool.c's g_finderVariants), which may
// be slightly bigger than what's actually needed.
//
HBITMAP CreateDpiScaledAlphaBitmap(HBITMAP hbmSrc, int dpi)
{
	BITMAP bm;
	BITMAPINFO bmi = { 0 };
	HDC hdcScreen, hdcSrc, hdcDst;
	HBITMAP hbmDst, hbmOldSrc, hbmOldDst;
	void *pvBits;
	int cxNew, cyNew;

	if(dpi == USER_DEFAULT_SCREEN_DPI || hbmSrc == NULL)
		return hbmSrc;

	if(!GetObject(hbmSrc, sizeof(bm), &bm))
		return hbmSrc;

	cxNew = MulDiv(bm.bmWidth,  dpi, USER_DEFAULT_SCREEN_DPI);
	cyNew = MulDiv(bm.bmHeight, dpi, USER_DEFAULT_SCREEN_DPI);

	bmi.bmiHeader.biSize        = sizeof(bmi.bmiHeader);
	bmi.bmiHeader.biWidth       = cxNew;
	bmi.bmiHeader.biHeight      = -cyNew; // top-down
	bmi.bmiHeader.biPlanes      = 1;
	bmi.bmiHeader.biBitCount    = 32;
	bmi.bmiHeader.biCompression = BI_RGB;

	hdcScreen = GetDC(0);
	hbmDst    = CreateDIBSection(hdcScreen, &bmi, DIB_RGB_COLORS, &pvBits, NULL, 0);

	if(hbmDst == NULL)
	{
		ReleaseDC(0, hdcScreen);
		return hbmSrc;
	}

	hdcSrc = CreateCompatibleDC(hdcScreen);
	hdcDst = CreateCompatibleDC(hdcScreen);

	hbmOldSrc = SelectObject(hdcSrc, hbmSrc);
	hbmOldDst = SelectObject(hdcDst, hbmDst);

	// HALFTONE would corrupt the alpha channel here (GDI dithers all 4
	// bytes per pixel as if they were colour, with no idea the 4th is
	// alpha) - COLORONCOLOR does a plain per-pixel copy/replicate that
	// leaves the premultiplied alpha bytes intact.
	SetStretchBltMode(hdcDst, COLORONCOLOR);
	SetBrushOrgEx(hdcDst, 0, 0, NULL);
	StretchBlt(hdcDst, 0, 0, cxNew, cyNew, hdcSrc, 0, 0, bm.bmWidth, bm.bmHeight, SRCCOPY);

	SelectObject(hdcSrc, hbmOldSrc);
	SelectObject(hdcDst, hbmOldDst);

	DeleteDC(hdcSrc);
	DeleteDC(hdcDst);
	ReleaseDC(0, hdcScreen);

	return hbmDst;
}
