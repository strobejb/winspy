//
//	Options.c
//
//  Copyright (c) 2002 by J Brown
//  Freeware
//
//	Implements the Options dialog for WinSpy
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "WinSpy.h"
#include "resource.h"

BOOL fSaveWinPos;
BOOL fAlwaysOnTop;
BOOL fMinimizeWinSpy;
BOOL fFullDragging;
BOOL fShowHidden;
BOOL fShowDimmed;
UINT uTreeInclude;
BOOL fClassThenText;
BOOL fPinWindow;
BOOL fShowInCaption;
BOOL fEnableToolTips;

extern POINT ptPinPos;
extern UINT  uPinnedCorner;

extern HWND hwndToolTip;

#define INI_SECTION  _T("Settings")
#define INI_FILENAME _T("winspy.ini")

static TCHAR szIniPath[MAX_PATH];

//
// Decide where winspy.ini lives: next to the exe if one is already there
// (portable mode), otherwise %AppData%\Catch22\WinSpy\winspy.ini.
//
static void ResolveIniPath(void)
{
	TCHAR *ptr;
	TCHAR szAppData[MAX_PATH];
	TCHAR szDir[MAX_PATH];

	GetModuleFileName(NULL, szIniPath, MAX_PATH);

	ptr = szIniPath + lstrlen(szIniPath);

	while(ptr > szIniPath && *(ptr - 1) != _T('\\'))
		ptr--;

	lstrcpy(ptr, INI_FILENAME);

	if(GetFileAttributes(szIniPath) != INVALID_FILE_ATTRIBUTES)
		return;   // portable mode: winspy.ini already exists next to the exe

	if(GetEnvironmentVariable(_T("APPDATA"), szAppData, MAX_PATH))
	{
		wsprintf(szDir, _T("%s\\Catch22"), szAppData);
		CreateDirectory(szDir, NULL);

		wsprintf(szDir, _T("%s\\Catch22\\WinSpy"), szAppData);
		CreateDirectory(szDir, NULL);

		wsprintf(szIniPath, _T("%s\\%s"), szDir, INI_FILENAME);
	}
}

static void WriteIniInt(TCHAR szKeyName[], LONG nValue)
{
	TCHAR ach[16];

	wsprintf(ach, _T("%d"), nValue);
	WritePrivateProfileString(INI_SECTION, szKeyName, ach, szIniPath);
}

void LoadSettings(void)
{
	ResolveIniPath();

	fSaveWinPos     = GetPrivateProfileInt(INI_SECTION, _T("SavePosition"),   TRUE,  szIniPath) != 0;
	fAlwaysOnTop    = GetPrivateProfileInt(INI_SECTION, _T("AlwaysOnTop"),    FALSE, szIniPath) != 0;
	fMinimizeWinSpy = GetPrivateProfileInt(INI_SECTION, _T("MinimizeWinSpy"), TRUE,  szIniPath) != 0;
	fFullDragging   = GetPrivateProfileInt(INI_SECTION, _T("FullDragging"),   TRUE,  szIniPath) != 0;
	fShowHidden     = GetPrivateProfileInt(INI_SECTION, _T("ShowHidden"),     FALSE, szIniPath) != 0;
	fShowDimmed     = GetPrivateProfileInt(INI_SECTION, _T("ShowDimmed"),     TRUE,  szIniPath) != 0;
	fClassThenText  = GetPrivateProfileInt(INI_SECTION, _T("ClassThenText"),  TRUE,  szIniPath) != 0;
	fPinWindow      = GetPrivateProfileInt(INI_SECTION, _T("PinWindow"),      FALSE, szIniPath) != 0;
	fShowInCaption  = GetPrivateProfileInt(INI_SECTION, _T("ShowInCaption"),  TRUE,  szIniPath) != 0;
	fEnableToolTips = GetPrivateProfileInt(INI_SECTION, _T("EnableToolTips"), FALSE, szIniPath) != 0;
	uTreeInclude    = GetPrivateProfileInt(INI_SECTION, _T("TreeItems"), WINLIST_INCLUDE_ALL, szIniPath);

	uPinnedCorner   = GetPrivateProfileInt(INI_SECTION, _T("PinCorner"), 0, szIniPath);

	ptPinPos.x      = GetPrivateProfileInt(INI_SECTION, _T("xpos"), CW_USEDEFAULT, szIniPath);
	ptPinPos.y      = GetPrivateProfileInt(INI_SECTION, _T("ypos"), CW_USEDEFAULT, szIniPath);
}

void SaveSettings(void)
{
	WriteIniInt(_T("SavePosition"),   fSaveWinPos);
	WriteIniInt(_T("AlwaysOnTop"),    fAlwaysOnTop);
	WriteIniInt(_T("MinimizeWinSpy"), fMinimizeWinSpy);
	WriteIniInt(_T("FullDragging"),   fFullDragging);
	WriteIniInt(_T("ShowHidden"),     fShowHidden);
	WriteIniInt(_T("ShowDimmed"),     fShowDimmed);
	WriteIniInt(_T("ClassThenText"),  fClassThenText);
	WriteIniInt(_T("PinWindow"),      fPinWindow);
	WriteIniInt(_T("ShowInCaption"),  fShowInCaption);
	WriteIniInt(_T("EnableToolTips"), fEnableToolTips);
	WriteIniInt(_T("TreeItems"),      uTreeInclude);
	WriteIniInt(_T("PinCorner"),      uPinnedCorner);

	WriteIniInt(_T("xpos"),           ptPinPos.x);
	WriteIniInt(_T("ypos"),           ptPinPos.y);
}

INT_PTR CALLBACK OptionsDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndTarget;

	switch(iMsg)
	{
	case WM_INITDIALOG:
		CheckDlgButton(hwnd, IDC_OPTIONS_SAVEPOS,	    fSaveWinPos);
		CheckDlgButton(hwnd, IDC_OPTIONS_FULLDRAG,	    fFullDragging);
		CheckDlgButton(hwnd, IDC_OPTIONS_DIR,		    fClassThenText);
		CheckDlgButton(hwnd, IDC_OPTIONS_SHOWHIDDEN,    fShowDimmed);
		CheckDlgButton(hwnd, IDC_OPTIONS_SHOWINCAPTION, fShowInCaption);
		CheckDlgButton(hwnd, IDC_OPTIONS_TOOLTIPS,		fEnableToolTips);

		CheckDlgButton(hwnd, IDC_OPTIONS_INCHANDLE,
			(uTreeInclude & WINLIST_INCLUDE_HANDLE) ? TRUE : FALSE);

		CheckDlgButton(hwnd, IDC_OPTIONS_INCCLASS,
			(uTreeInclude & WINLIST_INCLUDE_CLASS)  ? TRUE : FALSE);

		return TRUE;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:

			fSaveWinPos     = IsDlgButtonChecked(hwnd, IDC_OPTIONS_SAVEPOS);
			fFullDragging   = IsDlgButtonChecked(hwnd, IDC_OPTIONS_FULLDRAG);
			fClassThenText  = IsDlgButtonChecked(hwnd, IDC_OPTIONS_DIR);
			fShowDimmed     = IsDlgButtonChecked(hwnd, IDC_OPTIONS_SHOWHIDDEN);
			fShowInCaption  = IsDlgButtonChecked(hwnd, IDC_OPTIONS_SHOWINCAPTION);
			fEnableToolTips = IsDlgButtonChecked(hwnd, IDC_OPTIONS_TOOLTIPS);

			uTreeInclude   = 0;

			if(IsDlgButtonChecked(hwnd, IDC_OPTIONS_INCHANDLE))
				uTreeInclude |= WINLIST_INCLUDE_HANDLE;

			if(IsDlgButtonChecked(hwnd, IDC_OPTIONS_INCCLASS))
				uTreeInclude |= WINLIST_INCLUDE_CLASS;

			EndDialog(hwnd, 0);
			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;
		}

		return FALSE;
	}

	return FALSE;
}


void ShowOptionsDlg(HWND hwndParent)
{
	DialogBox(GetModuleHandle(0), MAKEINTRESOURCE(IDD_OPTIONS), hwndParent, OptionsDlgProc);

	if(!fShowInCaption)
	{
		SetWindowText(hwndParent, szAppName);
	}

	SendMessage(hwndToolTip, TTM_ACTIVATE, fEnableToolTips, 0);
}
