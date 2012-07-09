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
#include "RegHelper.h"
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

#define REG_BASESTR  _T("Software\\Catch22\\WinSpy++ 1.5")

static TCHAR szRegLoc[] = REG_BASESTR;

void LoadSettings(void)
{
	HKEY hkey;

	RegCreateKeyEx(HKEY_CURRENT_USER, szRegLoc, 0, _T(""), 0, KEY_READ, NULL, &hkey, NULL);
	
	fSaveWinPos     = GetSettingBool(hkey, _T("SavePosition"),		TRUE);
	fAlwaysOnTop    = GetSettingBool(hkey, _T("AlwaysOnTop"),		FALSE);
	fMinimizeWinSpy = GetSettingBool(hkey, _T("MinimizeWinSpy"),	TRUE);
	fFullDragging   = GetSettingBool(hkey, _T("FullDragging"),		TRUE);
	fShowHidden     = GetSettingBool(hkey, _T("ShowHidden"),		FALSE);
	fShowDimmed     = GetSettingBool(hkey, _T("ShowDimmed"),		TRUE);
	fClassThenText  = GetSettingBool(hkey, _T("ClassThenText"),     TRUE);
	fPinWindow      = GetSettingBool(hkey, _T("PinWindow"),         FALSE);
	fShowInCaption  = GetSettingBool(hkey, _T("ShowInCaption"),     TRUE);
	fEnableToolTips = GetSettingBool(hkey, _T("EnableToolTips"),	FALSE);
	uTreeInclude    = GetSettingInt (hkey, _T("TreeItems"), WINLIST_INCLUDE_ALL);

	uPinnedCorner   = GetSettingInt (hkey, _T("PinCorner"),         0);

	ptPinPos.x      = GetSettingInt(hkey,  _T("xpos"), CW_USEDEFAULT);
	ptPinPos.y      = GetSettingInt(hkey,  _T("ypos"), CW_USEDEFAULT);

	RegCloseKey(hkey);
}

void SaveSettings(void)
{
	HKEY hkey;

	RegCreateKeyEx(HKEY_CURRENT_USER, szRegLoc, 0, _T(""), 0, KEY_WRITE, NULL, &hkey, NULL);

	WriteSettingBool(hkey, _T("SavePosition"),		fSaveWinPos);
	WriteSettingBool(hkey, _T("AlwaysOnTop"),		fAlwaysOnTop);
	WriteSettingBool(hkey, _T("MinimizeWinSpy"),	fMinimizeWinSpy);
	WriteSettingBool(hkey, _T("FullDragging"),		fFullDragging);
	WriteSettingBool(hkey, _T("ShowHidden"),		fShowHidden);
	WriteSettingBool(hkey, _T("ShowDimmed"),		fShowDimmed);
	WriteSettingBool(hkey, _T("ClassThenText"),     fClassThenText);
	WriteSettingBool(hkey, _T("PinWindow"),         fPinWindow);
	WriteSettingBool(hkey, _T("ShowInCaption"),     fShowInCaption);
	WriteSettingBool(hkey, _T("EnableToolTips"),    fEnableToolTips);
	WriteSettingInt (hkey, _T("TreeItems"),         uTreeInclude);
	WriteSettingInt (hkey, _T("PinCorner"),         uPinnedCorner);
	
	WriteSettingInt (hkey, _T("xpos"),              ptPinPos.x);
	WriteSettingInt (hkey, _T("ypos"),              ptPinPos.y);

	RegCloseKey(hkey);
}

BOOL CALLBACK OptionsDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
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