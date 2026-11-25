//
//	WinSpy.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Main implementation.
//
//	v 1.7.1	- moved system-menu items to appear before the Close item
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <objbase.h>

#pragma comment(lib, "Ole32.lib")

#include "resource.h"
#include "WinSpy.h"
#include "FindTool.h"
//#include "AggressiveOptimize.h"
#include "CaptureWindow.h"
#include "BitmapButton.h"
#include "Utils.h"
#include "WindowFromPointEx.h"

HWND		hwndPin;		// Toolbar with pin bitmap
HWND		hwndSizer;		// Sizing grip for bottom-right corner
HWND		hwndToolTip;	// tooltip for main window controls only
HINSTANCE	hInst;			// Current application instance

TCHAR szHexFmt[]	= _T("%08X");
TCHAR szPtrFmt[]	= _T("%08p");
TCHAR szAppName[]	= _T("WinSpy++");

//
//	Current window being spied on
//
HWND       spy_hCurWnd = 0;
WNDCLASSEX spy_WndClassEx;
WNDPROC    spy_WndProc;
BOOL       spy_fPassword = FALSE;   // is it a password (edit) control?
TCHAR      spy_szPassword[200];
TCHAR      spy_szClassName[70];


static TBBUTTON tbbPin[] = 
{
	{	0,	IDM_WINSPY_PIN,		TBSTATE_ENABLED, TBSTYLE_CHECK,  0, 0	},
};
							
#define IDC_PIN_TOOLBAR 2000 // must be unique, so check resource.h
#define TOOLBAR_PIN_STYLES  (TBSTYLE_FLAT |	WS_CHILD | WS_VISIBLE | \
						CCS_NOPARENTALIGN | CCS_NORESIZE | CCS_NODIVIDER)

DialogTab WinSpyTab[NUMTABCONTROLITEMS] =
{
	0, _T("General"),		IDD_TAB_GENERAL,	GeneralDlgProc,
	0, _T("Styles"),		IDD_TAB_STYLES,		StyleDlgProc,
	0, _T("Properties"),	IDD_TAB_PROPERTIES, PropertyDlgProc,
	0, _T("Class"),			IDD_TAB_CLASS,		ClassDlgProc,
	0, _T("Windows"),		IDD_TAB_WINDOWS,	WindowDlgProc,
	0, _T("Process"),		IDD_TAB_PROCESS,    ProcessDlgProc,
};

static int nCurrentTab = 0;
static BOOL fFirstSelection = TRUE;

#define DELAYED_CAPTURE_TIMER 1
#define CAPTURE_HOTKEY_ID 1
static ULONGLONG captureDeadline;
static UINT captureSeconds;
static BOOL captureHotkeyArmed;
static TCHAR capturePreviousCaption[160];

// The delayed snapshot lives in the tab controls. Track the original window
// separately so commands that need a live target can check its identity.
static HWND snapshotWindow;
static DWORD snapshotThread;
static DWORD snapshotProcess;

void GetRemoteInfo(HWND hwnd);

BOOL CanUseSelectedWindow(void)
{
	DWORD process = 0;

	if(!snapshotWindow || spy_hCurWnd != snapshotWindow)
		return IsWindow(spy_hCurWnd);

	return GetWindowThreadProcessId(snapshotWindow, &process) == snapshotThread &&
		process == snapshotProcess;
}

static void DisplayTabInfo(HWND hwnd, int tab)
{
	switch(tab)
	{
	case GENERAL_TAB:
		SetGeneralInfo(hwnd);
		break;

	case STYLE_TAB:
		SetStyleInfo(hwnd);
		break;

	case PROPERTY_TAB:
		SetPropertyInfo(hwnd);
		SetScrollbarInfo(hwnd);
		break;

	case CLASS_TAB:
		SetClassInfo(hwnd);
		break;

	case WINDOW_TAB:
		SetWindowInfo(hwnd);
		break;

	case PROCESS_TAB:
		SetProcessInfo(hwnd);
		break;
	}
}

static void DisplayCurrentTabInfo(HWND hwnd)
{
	if(!IsWindow(hwnd) || hwnd == snapshotWindow)
		return;

	if(nCurrentTab == CLASS_TAB)
		GetRemoteInfo(hwnd);

	DisplayTabInfo(hwnd, nCurrentTab);
}

//
//	Try to get class information normally - if
//  it's a private application class, then we need to 
//  do this remotely
//
void GetRemoteInfo(HWND hwnd)
{
	BOOL b;

	ZeroMemory(&spy_WndClassEx, sizeof(WNDCLASSEX));
	spy_WndClassEx.cbSize = sizeof(WNDCLASSEX);

	b = GetClassInfoEx(0, spy_szClassName, &spy_WndClassEx);

	//try to use the standard methods to get the window procedure
	//and class information. If that fails, then we have to inject
	//a remote thread into the window's process and call the functions
	//from there.
	if(spy_WndProc == 0 || b == FALSE || spy_fPassword)
	{
		//Remote Threads only available under Windows NT
		if(ProcessArchMatches(hwnd))
		{
			// doesn't work with debug info!!!!!!!!
			// make sure we never call this function unless we have 
			// a release build!!!
			GetRemoteWindowInfo(hwnd, &spy_WndClassEx, &spy_WndProc, spy_szPassword, 200);
		}
		else
		{
			SendMessage(hwnd, WM_GETTEXT, 200, (LPARAM)spy_szPassword);
		}
	}
}

// Read the basic information shared by all the tabs.
static void ReadWindowInfo(HWND hwnd)
{
	spy_hCurWnd = hwnd;

	spy_szClassName[0] = 0;
	GetClassName(hwnd, spy_szClassName, 70);

	if(IsWindowUnicode(hwnd))
		spy_WndProc = (WNDPROC)GetWindowLongPtrW(hwnd, GWLP_WNDPROC);
	else
		spy_WndProc = (WNDPROC)GetWindowLongPtrA(hwnd, GWLP_WNDPROC);

	// If an password-edit control, then we can
	// inject our thread to get the password text!
	if(lstrcmpi(spy_szClassName, _T("Edit")) == 0)
	{
		// If a password control
		DWORD dwStyle = GetWindowLong(hwnd, GWL_STYLE);

		if(dwStyle & ES_PASSWORD)
			spy_fPassword = TRUE;
		else
			spy_fPassword = FALSE;
	}
	else
		spy_fPassword = FALSE;
}

//
//	Top-level function for retrieving+displaying a window's
//  information (styles/class/properties etc)
//
void DisplayWindowInfo(HWND hwnd)
{
	if(!IsWindow(hwnd) || (hwnd == snapshotWindow && !CanUseSelectedWindow()))
		return;

	snapshotWindow = NULL;
	ClearCapturedGeneralInfo();
	ReadWindowInfo(hwnd);

	// Password edit controls need the remote text before General can show it.
	if(spy_fPassword && nCurrentTab != CLASS_TAB)
	{
		GetRemoteInfo(hwnd);
	}

	DisplayCurrentTabInfo(hwnd);
}

static void StopPendingCapture(HWND hwnd, BOOL restoreCaption)
{
	if(captureDeadline)
	{
		KillTimer(hwnd, DELAYED_CAPTURE_TIMER);
		captureDeadline = 0;
	}
	else if(captureHotkeyArmed)
	{
		UnregisterHotKey(hwnd, CAPTURE_HOTKEY_ID);
		captureHotkeyArmed = FALSE;
	}
	else
		return;

	if(restoreCaption)
		SetWindowText(hwnd, capturePreviousCaption);
}

static void SetCountdownCaption(HWND hwnd, UINT seconds)
{
	TCHAR caption[80];

	captureSeconds = seconds;
	wsprintf(caption, _T("%s - Capture in %u s"), szAppName, seconds);
	SetWindowText(hwnd, caption);
}

static void StartDelayedCapture(HWND hwnd, UINT seconds)
{
	if(!SetTimer(hwnd, DELAYED_CAPTURE_TIMER, 100, NULL))
	{
		MessageBox(hwnd, _T("Could not start the capture timer."), szAppName, MB_ICONWARNING);
		return;
	}

	GetWindowText(hwnd, capturePreviousCaption, ARRAYSIZE(capturePreviousCaption));
	captureDeadline = GetTickCount64() + seconds * 1000;
	SetCountdownCaption(hwnd, seconds);

	if(fMinimizeWinSpy)
		ShowWindow(hwnd, SW_MINIMIZE);
}

typedef struct
{
	UINT key;
	UINT modifiers;
	TCHAR text[96];
} CaptureHotkey;

static LRESULT CALLBACK CaptureHotkeyFieldProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR subclass, DWORD_PTR data)
{
	CaptureHotkey *hotkey = (CaptureHotkey *)data;
	HWND dialog = GetParent(hwnd);

	switch(msg)
	{
	case WM_GETDLGCODE:
		return DLGC_WANTALLKEYS;

	case WM_KEYDOWN:
	case WM_SYSKEYDOWN:
	{
		UINT modifiers = 0;
		TCHAR keyName[48];

		if(wParam == VK_CONTROL || wParam == VK_SHIFT || wParam == VK_MENU ||
			wParam == VK_LWIN || wParam == VK_RWIN || hotkey->key || (lParam & 0x40000000))
			return 0;

		if(GetKeyState(VK_CONTROL) < 0) modifiers |= MOD_CONTROL;
		if(GetKeyState(VK_SHIFT) < 0) modifiers |= MOD_SHIFT;
		if(GetKeyState(VK_MENU) < 0) modifiers |= MOD_ALT;
		if(GetKeyState(VK_LWIN) < 0 || GetKeyState(VK_RWIN) < 0) modifiers |= MOD_WIN;

		if(wParam == VK_ESCAPE && !modifiers)
		{
			EndDialog(dialog, IDCANCEL);
			return 0;
		}

		hotkey->key = (UINT)wParam;
		hotkey->modifiers = modifiers;
		if(!GetKeyNameText((LONG)lParam, keyName, ARRAYSIZE(keyName)))
			wsprintf(keyName, _T("Key 0x%02X"), hotkey->key);
		wsprintf(hotkey->text, _T("%s%s%s%s%s"),
			modifiers & MOD_CONTROL ? _T("Ctrl+") : _T(""),
			modifiers & MOD_SHIFT ? _T("Shift+") : _T(""),
			modifiers & MOD_ALT ? _T("Alt+") : _T(""),
			modifiers & MOD_WIN ? _T("Win+") : _T(""), keyName);
		SetWindowText(hwnd, hotkey->text);
		return 0;
	}

	case WM_KEYUP:
	case WM_SYSKEYUP:
		if(hotkey->key && wParam == hotkey->key)
		{
			UINT key = hotkey->key;
			hotkey->key = 0;
			// Register on release so the keystroke used to choose the
			// shortcut cannot also trigger the capture.
			if(RegisterHotKey(GetWindow(dialog, GW_OWNER), CAPTURE_HOTKEY_ID,
				hotkey->modifiers | MOD_NOREPEAT, key))
				EndDialog(dialog, IDOK);
			else
				SetDlgItemText(dialog, IDC_CAPTURE_HOTKEY_PROMPT,
					_T("Hotkey unavailable. Try again."));
		}
		return 0;

	case WM_CHAR:
	case WM_SYSCHAR:
		return 0;

	case WM_KILLFOCUS:
		hotkey->key = 0;
		break;

	case WM_NCDESTROY:
		RemoveWindowSubclass(hwnd, CaptureHotkeyFieldProc, subclass);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

static INT_PTR CALLBACK CaptureHotkeyDlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
	{
		HWND field = GetDlgItem(hwnd, IDC_CAPTURE_HOTKEY);
		if(!SetWindowSubclass(field, CaptureHotkeyFieldProc, 0, (DWORD_PTR)lParam))
			EndDialog(hwnd, IDCANCEL);
		else
			SetFocus(field);
		return FALSE;
	}

	case WM_COMMAND:
		if(LOWORD(wParam) == IDCANCEL)
		{
			EndDialog(hwnd, IDCANCEL);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

static void StartHotkeyCapture(HWND hwnd)
{
	CaptureHotkey hotkey = {0};
	TCHAR caption[160];

	if(DialogBoxParam(hInst, MAKEINTRESOURCE(IDD_CAPTURE_HOTKEY), hwnd,
		CaptureHotkeyDlgProc, (LPARAM)&hotkey) != IDOK)
		return;

	captureHotkeyArmed = TRUE;
	GetWindowText(hwnd, capturePreviousCaption, ARRAYSIZE(capturePreviousCaption));
	wsprintf(caption, _T("%s - Press %s to capture"), szAppName, hotkey.text);
	SetWindowText(hwnd, caption);

	if(fMinimizeWinSpy)
		ShowWindow(hwnd, SW_MINIMIZE);
}

void WinSpy_ShowFinderMenu(HWND hwnd, POINT pt)
{
	HMENU menu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_FINDER_MENU));
	HMENU popup;
	UINT command;

	if(!menu)
		return;

	// A pending capture must not fire in TrackPopupMenu's nested message loop.
	StopPendingCapture(hwnd, TRUE);
	popup = GetSubMenu(menu, 0);
	CheckMenuItem(popup, IDM_CAPTURE_MINIMIZE, fMinimizeWinSpy ? MF_CHECKED : MF_UNCHECKED);
	CheckMenuItem(popup, IDM_CAPTURE_HIDDEN, fShowHidden ? MF_CHECKED : MF_UNCHECKED);

	command = TrackPopupMenu(popup, TPM_RETURNCMD | TPM_RIGHTBUTTON,
		pt.x, pt.y, 0, hwnd, NULL);
	DestroyMenu(menu);

	switch(command)
	{
	case IDM_CAPTURE_DELAY_3:  StartDelayedCapture(hwnd, 3); break;
	case IDM_CAPTURE_DELAY_5:  StartDelayedCapture(hwnd, 5); break;
	case IDM_CAPTURE_DELAY_10: StartDelayedCapture(hwnd, 10); break;
	case IDM_CAPTURE_HOTKEY:   StartHotkeyCapture(hwnd); break;
	case IDM_WINSPY_OPTIONS:   ShowOptionsDlg(hwnd); break;
	case IDM_CAPTURE_MINIMIZE: fMinimizeWinSpy = !fMinimizeWinSpy; break;
	case IDM_CAPTURE_HIDDEN:   fShowHidden = !fShowHidden; break;
	}
}

static BOOL CaptureWindowAtPointer(HWND hwndMain)
{
	POINT pt;
	HWND hwnd;
	DWORD thread, process;
	int tab;

	if(!GetCursorPos(&pt))
		return FALSE;

	hwnd = WindowFromPointEx(pt, fShowHidden);
	if(!hwnd || GetAncestor(hwnd, GA_ROOTOWNER) == hwndMain)
		return FALSE;

	thread = GetWindowThreadProcessId(hwnd, &process);
	if(!thread)
		return FALSE;

	ReadWindowInfo(hwnd);
	snapshotWindow = hwnd;
	snapshotThread = thread;
	snapshotProcess = process;

	// Read local class data during acquisition. The live Class tab can
	// still use GetRemoteInfo on refresh to obtain private class details.
	ZeroMemory(&spy_WndClassEx, sizeof(spy_WndClassEx));
	spy_WndClassEx.style = GetClassLong(hwnd, GCL_STYLE);
	spy_WndClassEx.cbClsExtra = GetClassLong(hwnd, GCL_CBCLSEXTRA);
	spy_WndClassEx.cbWndExtra = GetClassLong(hwnd, GCL_CBWNDEXTRA);
	spy_WndClassEx.hInstance = (HINSTANCE)GetClassLongPtr(hwnd, GCLP_HMODULE);
	spy_szPassword[0] = 0;
	spy_fPassword = FALSE;

	// Sample every pane while the target still has focus. Later tab
	// switches show these values even if the window has since closed.
	CaptureGeneralInfo(hwnd);
	for(tab = GENERAL_TAB + 1; tab < NUMTABCONTROLITEMS; tab++)
		DisplayTabInfo(hwnd, tab);

	return TRUE;
}

static void FinishPointerCapture(HWND hwnd)
{
	BOOL captured, live;
	TCHAR caption[160];

	StopPendingCapture(hwnd, FALSE);
	captured = CaptureWindowAtPointer(hwnd);
	live = captured && CanUseSelectedWindow();

	if(captured && !live)
		SetWindowText(hwnd, _T("WinSpy++ - Target closed during capture"));
	else if(captured && fShowInCaption)
	{
		wsprintf(caption, _T("%s [%08X, %s]"), szAppName, snapshotWindow, spy_szClassName);
		SetWindowText(hwnd, caption);
	}
	else
		SetWindowText(hwnd, captured ? szAppName : capturePreviousCaption);

	if(IsIconic(hwnd))
		ShowWindow(hwnd, SW_RESTORE);

	if(captured)
	{
		if(GetWindowLayout(hwnd) == WINSPY_MINIMIZED)
			SetWindowLayout(hwnd, WINSPY_LASTMAX);
		fFirstSelection = FALSE;
		SetForegroundWindow(hwnd);
	}
	else
		MessageBox(hwnd, _T("No target window was available under the cursor."), szAppName, MB_ICONWARNING);
}

static void UpdateDelayedCapture(HWND hwnd)
{
	ULONGLONG now;

	if(!captureDeadline)
		return;

	now = GetTickCount64();
	if(now >= captureDeadline)
		FinishPointerCapture(hwnd);
	else
	{
		UINT seconds = (UINT)((captureDeadline - now + 999) / 1000);
		if(seconds != captureSeconds)
			SetCountdownCaption(hwnd, seconds);
	}
}

//
//	User-defined callback function for the Find Tool
//
UINT CALLBACK WndFindProc(HWND hwndTool, UINT uCode, HWND hwnd)
{
	HWND hwndMain = GetParent(hwndTool);

	TCHAR ach[90] = {0};
	TCHAR szClass[70] = {0};

	static HWND hwndLastTarget;
	static BOOL fOldShowHidden;
	static TCHAR dragPreviousCaption[160];

	switch(uCode)
	{
	case WFN_SELCHANGED:

		if(fShowInCaption)
		{
			GetClassName(hwnd, szClass, sizeof(szClass) / sizeof(TCHAR));
			wsprintf(ach, _T("%s [%08X, %s]"), szAppName, hwnd, szClass);
			SetWindowText(hwndMain, ach);
		}

		if(!snapshotWindow)
		{
			spy_hCurWnd = hwnd;
			spy_WndProc = 0;
		}
		PreviewGeneralInfo(hwnd);
		return 0;

	case WFN_BEGIN:

		StopPendingCapture(hwndMain, TRUE);
		hwndLastTarget = spy_hCurWnd;
		if(snapshotWindow)
			GetWindowText(hwndMain, dragPreviousCaption, ARRAYSIZE(dragPreviousCaption));

		if(!snapshotWindow)
			spy_hCurWnd = hwnd;

		if(fMinimizeWinSpy)
		{
			SetWindowLayout(hwndMain, WINSPY_MINIMIZED);
			InvalidateRect(hwndMain, 0, 0);
			UpdateWindow(hwndMain);
		}

		fOldShowHidden = fShowHidden;
		break;

	case WFN_CANCELLED:

		// Restore the current window + Fall through!
		spy_hCurWnd = hwndLastTarget;

		if(fShowInCaption && !snapshotWindow)
		{
			GetClassName(spy_hCurWnd, szClass, sizeof(szClass) / sizeof(TCHAR));

			wsprintf(ach, _T("%s [%08X, %s]"), szAppName, spy_hCurWnd, szClass);
			SetWindowText(hwndMain, ach);
		}

	case WFN_END:

		ShowWindow(hwndMain, SW_SHOW);			
	
		if(!snapshotWindow)
			DisplayWindowInfo(spy_hCurWnd);
		else if(uCode == WFN_END && IsWindow(hwnd) &&
			(hwnd != snapshotWindow || CanUseSelectedWindow()))
			DisplayWindowInfo(hwnd);
		else
		{
			SetWindowText(hwndMain, dragPreviousCaption);
			RestoreCapturedGeneralInfo();
		}

		if(fMinimizeWinSpy || fFirstSelection)
		{
			fFirstSelection = FALSE;
			SetWindowLayout(hwndMain, WINSPY_LASTMAX);
		}
		
		if(fMinimizeWinSpy)
		{
			InvalidateRect(hwndMain, 0, TRUE);
			InvalidateRect(WinSpyTab[nCurrentTab].hwnd, 0, TRUE);
		}

		fShowHidden = fOldShowHidden;
		break;

	case WFN_CTRL_UP:
		ToggleWindowLayout(hwndMain);
		break;

	case WFN_CTRL_DOWN:
		ToggleWindowLayout(hwndMain);
		break;

	case WFN_SHIFT_UP:
		SetWindowPos(hwndMain, 0,0,0,0, 0, SWP_SHOWONLY);
		fShowHidden = fOldShowHidden;
		break;

	case WFN_SHIFT_DOWN:
		SetWindowPos(hwndMain, 0,0,0,0, 0, SWP_HIDEONLY);
		fShowHidden = TRUE;
		break;

	}
	return 0;
}

//
// Check/Uncheck the specified item in the system menu
//
void CheckSysMenu(HWND hwnd, UINT uItemId, BOOL fChecked)
{
	HMENU hSysMenu;

	hSysMenu = GetSystemMenu(hwnd, FALSE);

	if(fChecked)
		CheckMenuItem(hSysMenu, uItemId, MF_CHECKED|MF_BYCOMMAND);
	else
		CheckMenuItem(hSysMenu, uItemId, MF_UNCHECKED|MF_BYCOMMAND);
}

//
// Is the specified item in system menu checked?
//
BOOL IsSysMenuChecked(HWND hwnd, UINT uItemId)
{
	HMENU hSysMenu;
	DWORD dwState;

	hSysMenu = GetSystemMenu(hwnd, FALSE);
	
	dwState = GetMenuState(hSysMenu, uItemId, MF_BYCOMMAND);
				
	return (dwState & MF_CHECKED) ? TRUE : FALSE;
}

//
//	Toggle the checked status for specified item
//
BOOL ToggleSysMenuCheck(HWND hwnd, UINT uItemId)
{
	if(IsSysMenuChecked(hwnd, uItemId))
	{
		CheckSysMenu(hwnd, uItemId, FALSE);
		return FALSE;
	}
	else
	{
		CheckSysMenu(hwnd, uItemId, TRUE);
		return TRUE;
	}
}

//
//	Determine the window layout and check/uncheck  the
//  maximized menu item accordingly
//
void SetSysMenuIconFromLayout(HWND hwnd, UINT layout)
{
	if(layout == WINSPY_MINIMIZED)
		CheckSysMenu(hwnd, SC_MAXIMIZE, FALSE);
	else
		CheckSysMenu(hwnd, SC_MAXIMIZE, TRUE);
}

//
//	Create a sizing grip for the lower-right corner
//
HWND CreateSizeGrip(HWND hwndDlg)
{
	HWND hwndSizeGrip;

	// Create a sizing grip for the lower-right corner
	hwndSizeGrip = CreateWindow(
		_T("Scrollbar"),
		_T(""), 
		WS_VISIBLE|WS_CHILD|SBS_SIZEGRIP|
		SBS_SIZEBOXBOTTOMRIGHTALIGN|WS_CLIPSIBLINGS, 
		0,0,20,20, 
		hwndDlg, 0, hInst, 0);

	return hwndSizeGrip;
}

//
//	Create a tooltip control, 
//
HWND CreateTooltip(HWND hwndDlg)
{
	HWND hwndTT;
	TOOLINFO ti;
	int i;
	BOOL fRet;

	struct CtrlTipsTag
	{
		UINT  uDlgId;	// -1 for main window, 0-n for tab dialogs
		UINT  uCtrlId;
		TCHAR szText[50];

	} CtrlTips[] = 
	{
		-1, IDC_DRAGGER,    _T("Window Finder Tool"),
		-1, IDC_PIN_TOOLBAR,_T("Keep On-Screen (F4)"),
		-1, IDC_FINDER_MENU,_T("Capture options"),
		-1, IDC_CAPTURE,    _T("Capture Current Window (Alt+C)"),
		-1, IDOK,			_T("Exit WinSpy++"),
		-1, IDC_EXPAND,     _T("Expand / Collapse (F3)"),
		-1, IDC_REFRESH,    _T("Refresh Window List (F6)"),
		-1, IDC_LOCATE,     _T("Locate Current Window"),
		-1, IDC_FLASH,      _T("Highlight Selected Window"),

		GENERAL_TAB, IDC_HANDLE_MENU,  _T("Window Commands"),
		GENERAL_TAB, IDC_SETCAPTION,   _T("Set Window Caption"),
		GENERAL_TAB, IDC_EDITSIZE,     _T("Adjust Window Position"),
		STYLE_TAB,	 IDC_EDITSTYLE,    _T("Edit Styles"),
		STYLE_TAB,   IDC_EDITSTYLEEX,  _T("Edit Extended Styles"),
		PROCESS_TAB, IDC_PROCESS_MENU, _T("Process Commands"),
		WINDOW_TAB,  IDC_PARENT,       _T("Parent Window"),
		WINDOW_TAB,  IDC_OWNER,        _T("Owner Window"),
	};

	// Create tooltip for main window
    hwndTT = CreateWindowEx(WS_EX_TOPMOST,
        TOOLTIPS_CLASS,
        NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,		
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        hwndDlg,
        NULL,
        hInst,
        NULL
        );

	//	
	//	Add tooltips to every control (above)
	//	
	for(i = 0; i < sizeof(CtrlTips) / sizeof(CtrlTips[0]); i++)
	{
		HWND hwnd;

		if(CtrlTips[i].uDlgId == -1)
			hwnd = hwndDlg;
		else
			hwnd = WinSpyTab[CtrlTips[i].uDlgId].hwnd;

		ti.cbSize   = sizeof(ti);
		ti.uFlags   = TTF_SUBCLASS | TTF_IDISHWND;
		ti.hwnd     = hwnd;
		ti.uId      = (UINT_PTR)GetDlgItem(hwnd, CtrlTips[i].uCtrlId);
		if(!ti.uId)
			continue;
		ti.hinst    = hInst;
		ti.lpszText = CtrlTips[i].szText;
		ti.lParam   = 0;
	
		fRet = (BOOL)SendMessage(hwndTT, TTM_ADDTOOL, 0, (LPARAM)&ti);
	}
	
	SendMessage(hwndTT, TTM_ACTIVATE, fEnableToolTips, 0);

	return hwndTT;
}

//
//	Create a toolbar with one button in it, for
//  the pin-button
//
HWND CreatePinToolbar(HWND hwndDlg)
{
	RECT       rect;
	HWND       hwndTB;
	HBITMAP    hBitmap, hBitmapScaled;
	HIMAGELIST hImageList;
	HDC        hdcScreen, hdcBmp;
	COLORREF   crMask;
	BITMAP     bmScaled;
	int        dpi, cx, cy;

	dpi = GetWindowDpi(hwndDlg);

	hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_PIN_BITMAP));

	// Classic toolbar bitmaps use their bottom-left pixel as the
	// transparency-mask colour - sample it rather than guess it.
	hdcScreen = GetDC(0);
	hdcBmp    = CreateCompatibleDC(hdcScreen);
	SelectObject(hdcBmp, hBitmap);
	crMask = GetPixel(hdcBmp, 0, 13);
	DeleteDC(hdcBmp);
	ReleaseDC(0, hdcScreen);

	// Scale the bitmap for the current dpi. COLORONCOLOR (not HALFTONE)
	// keeps that mask colour solid at the edges - HALFTONE would blend
	// it into the icon and corrupt the transparency.
	hBitmapScaled = CreateDpiScaledBitmap(hBitmap, dpi, COLORONCOLOR);

	// The bitmap is a horizontal strip of 2 frames (unpinned/pinned).
	// Derive the per-frame size from the actual scaled bitmap, rather
	// than scaling 15x14 independently, so it always divides evenly -
	// two separately-rounded MulDiv results aren't guaranteed to.
	GetObject(hBitmapScaled, sizeof(bmScaled), &bmScaled);
	cx = bmScaled.bmWidth / 2;
	cy = bmScaled.bmHeight;

	hImageList = ImageList_Create(cx, cy, ILC_COLOR32 | ILC_MASK, 2, 0);
	ImageList_AddMasked(hImageList, hBitmapScaled, crMask);

	if(hBitmapScaled != hBitmap) DeleteObject(hBitmapScaled);
	DeleteObject(hBitmap);

	// Create the toolbar with no buttons yet and no built-in bitmap -
	// TB_SETBUTTONSIZE only takes effect before any buttons are added,
	// so the button (tbbPin) is added afterwards via TB_ADDBUTTONS.
	hwndTB = CreateToolbarEx(
			hwndDlg,
			TOOLBAR_PIN_STYLES,				//,
			IDC_PIN_TOOLBAR,				//toolbar ID (don't need)
			0,								//no built-in bitmap load
			NULL,
			0,
			NULL,							//no buttons yet
			0,
			cx,cy,cx,cy,
			sizeof(TBBUTTON) );

	SendMessage(hwndTB, TB_SETIMAGELIST, 0, (LPARAM)hImageList);

	// TOOLBAR_PIN_STYLES includes CCS_NORESIZE, which per the documented
	// pattern means the toolbar won't infer button size from the image
	// list on its own - it must be set explicitly.
	SendMessage(hwndTB, TB_SETBITMAPSIZE, 0, MAKELPARAM(cx, cy));
	SendMessage(hwndTB, TB_SETBUTTONSIZE, 0, MAKELPARAM(cx, cy));

	SendMessage(hwndTB, TB_ADDBUTTONS, sizeof(tbbPin) / sizeof(tbbPin[0]), (LPARAM)tbbPin);

	// Find out how big the button is, so we can resize the
	// toolbar to fit perfectly
	SendMessage(hwndTB, TB_GETITEMRECT, 0, (LPARAM)&rect);

	SetWindowPos(hwndTB, HWND_TOP, 0,0, 
		rect.right-rect.left, 
		rect.bottom-rect.top, SWP_NOMOVE);

	// Setup the bitmap image
	SendMessage(hwndTB, TB_CHANGEBITMAP, IDM_WINSPY_PIN, 
		(LPARAM)MAKELPARAM(fPinWindow, 0)); 

	// Checked / Unchecked
	SendMessage(hwndTB, TB_CHECKBUTTON, IDM_WINSPY_PIN, MAKELONG(fPinWindow, 0));

	return hwndTB;
}

//
//	WM_INITDIALOG handler
//
BOOL WinSpy_InitDlg(HWND hwnd)
{
	HBITMAP hBmp1, hBmp2;
	HMENU   hSysMenu;
	int     i;
	HICON   hIcon;
	TCITEM  tcitem;
	HWND    hwndFinder = GetDlgItem(hwnd, IDC_DRAGGER);
	
	// Initialize the finder tool
	MakeFinderTool(hwndFinder, WndFindProc);
#ifdef WINSPY_FINDER_DROPDOWN
	// Keep one dialog template so the resource editor can round-trip it.
	SetWindowLong(hwndFinder, GWL_STYLE, GetWindowLong(hwndFinder, GWL_STYLE) | WS_TABSTOP);
	SetWindowText(hwndFinder, _T("Finder and capture options"));
	DestroyWindow(GetDlgItem(hwnd, IDC_FINDER_MENU));
	ShowDlgItem(hwnd, IDC_FINDER_HINT2, SW_HIDE);
#elif !defined(WINSPY_FINDER_TIMER)
	InitFinderCog(GetDlgItem(hwnd, IDC_FINDER_MENU));
#endif

	// Make the More>> button into a bitmap
	MakeDlgBitmapButton(hwnd, IDC_EXPAND, IDI_ICON16);
	
	hwndSizer   = CreateSizeGrip(hwnd);
	hwndPin     = CreatePinToolbar(hwnd);

	// Pin toolbar button is temporarily hidden from the UI - the app
	// always behaves as if pinned (see Options.c's fPinWindow override).
	// The toolbar itself is left fully intact so this is easy to revert.
	ShowWindow(hwndPin, SW_HIDE);

	// Load image lists etc
	InitGlobalWindowTree(GetDlgItem(hwnd, IDC_TREE1));
	
	// Create each dialog-tab pane,
	for(i = 0; i < NUMTABCONTROLITEMS; i++)
	{
		ZeroMemory(&tcitem, sizeof(tcitem));
		
		tcitem.mask = TCIF_TEXT;
		tcitem.pszText = (LPTSTR)WinSpyTab[i].szText;
		
		// Create the dialog pane
		WinSpyTab[i].hwnd = CreateDialog(hInst, 
			MAKEINTRESOURCE(WinSpyTab[i].id), hwnd, WinSpyTab[i].dlgproc);
		
		// Create the corresponding tab
		SendDlgItemMessage(hwnd, IDC_TAB1, TCM_INSERTITEM, i, (LPARAM)&tcitem);
		
		SetWindowText(WinSpyTab[i].hwnd, WinSpyTab[i].szText);
		
		// Make this dialog XP-theme aware!
		EnableDialogTheme(WinSpyTab[i].hwnd);
	}
	
	InitStockStyleLists();
	
	// Position our contents, work out how big the various
	// layouts are (depending on current system settings for border
	// width, titlebar height etc).
	WinSpyDlg_SizeContents(hwnd);

	// See what the registry settings are, and setup accordingly
	if(fSaveWinPos && ptPinPos.x != CW_USEDEFAULT && ptPinPos.y != CW_USEDEFAULT)
	{
		SetWindowLayout(hwnd, WINSPY_MINIMIZED);
	}
	else
	{
		RECT rect;
		GetWindowRect(hwnd, &rect);
		ptPinPos.x = rect.left;
		ptPinPos.y = rect.top;
	}

	// display the general tab
	SetWindowPos(WinSpyTab[0].hwnd, 0, 0, 0, 0, 0, SWP_SHOWONLY);

	//
	//	Customize this window's system menu by adding our own
	//  commands.
	//
	hSysMenu = GetSystemMenu(hwnd, FALSE);


	// add items *before* the close item
	InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_ABOUT,   _T("&About..."));
	//InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_OPTIONS, _T("&Options...\tAlt+Enter"));
	//InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING, IDM_WINSPY_HELP,    _T("&Help\tF1"));
	//InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR,           -1,                 _T(""));
	InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_ENABLED | MF_STRING	| 
		(fAlwaysOnTop ? MF_CHECKED : 0), IDM_WINSPY_ONTOP,   		 
		_T("Always On &Top\tShift+Y"));
	InsertMenu(hSysMenu, SC_CLOSE, MF_BYCOMMAND | MF_SEPARATOR,           -1,                 _T(""));

/*	AppendMenu(hSysMenu, MF_SEPARATOR,           -1,                 _T(""));
	AppendMenu(hSysMenu, MF_ENABLED | MF_STRING, IDM_WINSPY_ABOUT,   _T("&About"));
	AppendMenu(hSysMenu, MF_ENABLED | MF_STRING, IDM_WINSPY_OPTIONS, _T("&Options...\tAlt+Enter"));
	AppendMenu(hSysMenu, MF_ENABLED | MF_STRING, IDM_WINSPY_HELP,    _T("&Help\tF1"));
	AppendMenu(hSysMenu, MF_SEPARATOR,           -1,                 _T(""));
	AppendMenu(hSysMenu, MF_ENABLED | MF_STRING	| 
		(fAlwaysOnTop ? MF_CHECKED : 0), IDM_WINSPY_ONTOP,   		 
		_T("Always On &Top\tShift+Y"));*/
		
	// Change the Maximize item to a Toggle Layout item
	ModifyMenu(hSysMenu, SC_MAXIMIZE, MF_ENABLED|MF_STRING,SC_MAXIMIZE, 
		_T("&Toggle Layout\tF3"));
	
	// Change the bitmaps for the Maximize item
	hBmp1 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CHECK1));
	hBmp2 = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_CHECK2));
	SetMenuItemBitmaps(hSysMenu, SC_MAXIMIZE, MF_BYCOMMAND, hBmp1, hBmp2);
		
	// Set the dialog's Small Icon - this is what actually renders in the
	// titlebar corner / system menu button, independently of the taskbar
	// and Alt-Tab icon (ICON_BIG, below) - a hamburger reads better than
	// the app icon at this size and in this "click for a menu" role.
	hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON_SYSMENU), IMAGE_ICON, 16, 16, 0);
	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);

	// Set the dialog's Large Icon
	hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON1), IMAGE_ICON, 32, 32, 0);
	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);

	// Create tooltips after all other windows
	hwndToolTip = CreateTooltip(hwnd);

	ForceVisibleDisplay(hwnd);

	// Set focus to first item		
	return TRUE;
}

//
//	WM_NOTIFY handler
//
UINT WinSpyDlg_NotifyHandler(HWND hwnd, WPARAM wParam, NMHDR *hdr)
{
	NMTREEVIEW   *nmtv = (NMTREEVIEW *)hdr;
	TVHITTESTINFO hti;
	TVITEM        tvi;
	
	UINT   uCmd;
	HMENU  hMenu, hPopup;
	POINT  pt;
	
	switch(hdr->code)
	{
	// TabView selection has changed, so show appropriate tab-pane
	case TCN_SELCHANGE:
		
		ShowWindow(WinSpyTab[nCurrentTab].hwnd, SW_HIDE);
		
		nCurrentTab = TabCtrl_GetCurSel(hdr->hwndFrom);
		
		SetWindowPos(WinSpyTab[nCurrentTab].hwnd, HWND_TOP, 0,0,0,0, SWP_SHOWONLY);

		DisplayCurrentTabInfo(spy_hCurWnd);
		
		return TRUE;
		
	// TreeView has been right-clicked, so show the popup menu
	case NM_DBLCLK:
	case NM_RCLICK:
		
		// Find out where in the TreeView the mouse has been clicked		
		GetCursorPos(&pt);

		hti.pt = pt;
		ScreenToClient(hdr->hwndFrom, &hti.pt);
		
		// Find item which has been right-clicked on
		if(TreeView_HitTest(hdr->hwndFrom, &hti) && 
			(hti.flags & (TVHT_ONITEM|TVHT_ONITEMRIGHT) ))
		{
			// Now get the window handle, which is stored in the lParam
			// portion of the TVITEM structure..
			ZeroMemory(&tvi, sizeof(tvi));
			tvi.mask = TVIF_HANDLE | TVIF_PARAM;
			tvi.hItem = hti.hItem;
			
			TreeView_GetItem(hdr->hwndFrom, &tvi);
			
			if(hdr->code == NM_RCLICK)
			{
				hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU3));
				hPopup = GetSubMenu(hMenu, 0);

				WinSpy_SetupPopupMenu(hPopup, (HWND)tvi.lParam);
			
				// Show the menu
				uCmd = TrackPopupMenu(hPopup, TPM_RIGHTBUTTON | TPM_RETURNCMD, pt.x, pt.y, 0, hwnd, 0);
			
				// Act accordingly
				WinSpy_PopupCommandHandler(hwnd, uCmd, (HWND)tvi.lParam);

				DestroyMenu(hMenu);

				return TRUE;
			}
			/*else if(!(hti.flags & TVHT_ONITEMICON))
			{
				FlashWindowBorder((HWND)tvi.lParam, TRUE);
				
				// Return non-zero to prevent item from expanding when double-clicked
				SetWindowLong(hwnd, DWL_MSGRESULT, TRUE);
				return TRUE;
			}*/
		}
		
		return TRUE;

	case TVN_ITEMEXPANDING:

		if(nmtv->action == TVE_EXPAND)
			PopulateProcessWindowTree(hdr->hwndFrom, nmtv->itemNew.hItem);

		return FALSE;
		
	// TreeView selection has changed, so update the main window properties
	case TVN_SELCHANGED:
		
		if(IsWindowVisible(GetDlgItem(hwnd, IDC_TREE1)))
		{
			//Find the window handle stored in the TreeView item's lParam
			ZeroMemory(&tvi, sizeof(tvi));

			tvi.mask = TVIF_HANDLE | TVIF_PARAM;
			tvi.hItem = nmtv->itemNew.hItem;
			
			// Locating the captured window in the tree keeps its snapshot.
			if(TreeView_GetItem(hdr->hwndFrom, &tvi) && (HWND)tvi.lParam != snapshotWindow)
				DisplayWindowInfo((HWND)tvi.lParam);
		}
		
		return TRUE;
	}
	
	return 0;
}

//
//	WM_SYSCOLORCHANGE handler
//
BOOL WinSpyDlg_SysColorChange(HWND hwnd)
{
	int i;

	// forward this to all the dialogs - they can look after
	// their own controls
	for(i = 0; i < NUMTABCONTROLITEMS; i++)
		PostMessage(WinSpyTab[i].hwnd, WM_SYSCOLORCHANGE, 0, 0);

	// Set the treeview colours
	TreeView_SetBkColor(GetDlgItem(hwnd, IDC_TREE1), GetSysColor(COLOR_WINDOW));

	// Recreate toolbar, so it uses new colour scheme. TB_SETIMAGELIST
	// doesn't hand ownership of the image list to the toolbar, so it's
	// ours to destroy before the window that was using it goes away.
	ImageList_Destroy((HIMAGELIST)SendMessage(hwndPin, TB_GETIMAGELIST, 0, 0));
	DestroyWindow(hwndPin);

	hwndPin = CreatePinToolbar(hwnd);
	ShowWindow(hwndPin, SW_HIDE);

	// Send a WM_SIZE so that the pin toolbar gets repositioned
	SetWindowPos(hwnd, 0, 0, 0, 0, 0, 
		SWP_NOMOVE|SWP_NOSIZE|
		SWP_NOZORDER|SWP_NOACTIVATE|
		SWP_FRAMECHANGED);

	return TRUE;
}

#ifdef _DEBUG
void DumpRect(HWND hwnd)
{
	RECT  rect;
	TCHAR ach[80];
	GetWindowRect(hwnd, &rect);
	wsprintf(ach, _T("%d %d %d %d\n"), rect.left, rect.top, rect.right, rect.bottom);
	OutputDebugString(ach);

}
#endif

//
//	Dialog procedure for main window
//
INT_PTR WINAPI DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_INITDIALOG:
		return WinSpy_InitDlg(hwnd);

	case WM_CLOSE:
		ExitWinSpy(hwnd, 0); 
		return TRUE;

	case WM_DESTROY:
		StopPendingCapture(hwnd, FALSE);
		ClearCapturedGeneralInfo();
		DeInitGlobalWindowTree(GetDlgItem(hwnd, IDC_TREE1));
		return TRUE;

	case WM_SYSCOLORCHANGE:
		return WinSpyDlg_SysColorChange(hwnd);

	case WM_SYSCOMMAND:
		return WinSpyDlg_SysMenuHandler(hwnd, wParam, lParam);
		
	case WM_COMMAND:
		return WinSpyDlg_CommandHandler(hwnd, wParam, lParam);

	case WM_TIMER:
		if(wParam == DELAYED_CAPTURE_TIMER)
		{
			UpdateDelayedCapture(hwnd);
			return TRUE;
		}
		break;

	case WM_HOTKEY:
		if(wParam == CAPTURE_HOTKEY_ID && captureHotkeyArmed)
		{
			FinishPointerCapture(hwnd);
			return TRUE;
		}
		break;

	case WM_SIZE:
		return WinSpyDlg_Size(hwnd, wParam, lParam);
	
	case WM_SIZING:
		return WinSpyDlg_Sizing(hwnd, (UINT)wParam, (RECT *)lParam);

	case WM_NCHITTEST:
		return WinSpyDlg_FullWindowDrag(hwnd, wParam, lParam);

	case WM_ENTERSIZEMOVE:
		return WinSpyDlg_EnterSizeMove(hwnd);

	case WM_EXITSIZEMOVE:
		return WinSpyDlg_ExitSizeMove(hwnd);

	case WM_WINDOWPOSCHANGED:
		return WinSpyDlg_WindowPosChanged(hwnd, (WINDOWPOS *)lParam);

	case WM_NOTIFY:
		return WinSpyDlg_NotifyHandler(hwnd, wParam, (NMHDR *)lParam);

	case WM_DRAWITEM:
#ifndef WINSPY_FINDER_DROPDOWN
		if(wParam == IDC_FINDER_MENU)
		{
			DrawFinderMenuButton((DRAWITEMSTRUCT *)lParam);
			return TRUE;
		}
#endif
		return DrawBitmapButton((DRAWITEMSTRUCT *)lParam);

	// Update our layout based on new settings
	case WM_SETTINGCHANGE:
		WinSpyDlg_SizeContents(hwnd);
		return TRUE;
	}

	return FALSE;
}

//
//	The only reason I do this is to "obfuscate" the main
//  window. All the windows are just dialogs (#32770), but
//  I use this function to make a new dialog class with
//  a different name..no other reason.
//
//	Check the dialog resources to see how the new name
//  is specified. (MFC extensions must be turned off for the resource
//  to enable this feature).
//
void RegisterDialogClass(TCHAR szNewName[])
{
	WNDCLASSEX wc;

	// Get the class structure for the system dialog class
	wc.cbSize = sizeof(wc);
	GetClassInfoEx(0, _T("#32770"), &wc);

	// Make sure our new class does not conflict
	wc.style &= ~CS_GLOBALCLASS;

	// Register an identical dialog class, but with a new name!
	wc.lpszClassName = szNewName;

	RegisterClassEx(&wc);
}
//
//	This is where the fun begins
//
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
	HWND	hwndMain;
	HACCEL	hAccelTable;			
	MSG		msg;
	BOOL	fStartOnTop;

	INITCOMMONCONTROLSEX ice;
	hInst = hInstance;

	ice.dwSize = sizeof ice;
	ice.dwICC  = ICC_BAR_CLASSES      | ICC_TREEVIEW_CLASSES |
		         ICC_LISTVIEW_CLASSES | ICC_TAB_CLASSES      ;

	EnableDebugPrivilege();

	// Needed for WIC (LoadPNGImage, used by the finder tool and the
	// drag-select box overlay) - both silently fail to decode without it.
	CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);

	InitCommonControls();//Ex(&ice);

	RegisterDialogClass(_T("WinSpy"));
	RegisterDialogClass(_T("WinSpyPane"));

	LoadSettings();
	// Initial layout changes can update fAlwaysOnTop before the dialog is shown.
	fStartOnTop = fAlwaysOnTop;
	
	//DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DialogProc);

	hwndMain = CreateDialog(hInstance, MAKEINTRESOURCE(IDD_MAIN), 0, DialogProc);

	//Initialise the keyboard accelerators
	hAccelTable = LoadAccelerators(hInstance, MAKEINTRESOURCE(IDR_ACCELERATOR1));

	//
	// UPDATED (fix for Matrox CenterPOPUP feature :)
	//
	//	If we use ShowWindow, then my Matrox card automatically centers WinSpy
	//  on the current monitor (even if we restored WinSpy to it's position last
	//  time we ran). Therefore we use SetWindowPos to display the dialog, as
	//  Matrox don't seem to hook this in their display driver..
	//
	SetWindowPos(hwndMain, fStartOnTop ? HWND_TOPMOST : HWND_TOP, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
	
	while(GetMessage(&msg, NULL,0,0))
	{
		// Get the accelerator keys before IsDlgMsg gobbles them up!
		if(!TranslateAccelerator(hwndMain, hAccelTable, &msg))
		{
			// Let IsDialogMessage process TAB etc
			if(!IsDialogMessage(hwndMain, &msg))
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	SaveSettings();

	CoUninitialize();

	return 0;
}

void ExitWinSpy(HWND hwnd, UINT uCode)
{
	// Refresh the pinned-corner anchor from the window's actual final
	// state before it's destroyed (NOT after the message loop ends - by
	// then hwnd is already gone, and GetWindowRect on a destroyed handle
	// fails silently, leaving garbage that gets saved as the position).
	// During the session ptPinPos is only updated when a caption-drag
	// ends with the mouse still over the caption/client area (uHitTest
	// == HTCAPTION at that exact instant) - if the last drag's release
	// happened to land somewhere that didn't register as HTCAPTION, that
	// update gets silently skipped and a stale anchor from earlier would
	// get saved instead. This guarantees what's persisted always matches
	// reality, regardless of that.
	GetPinnedPosition(hwnd, &ptPinPos);

	DestroyWindow(hwnd);
	PostQuitMessage(uCode);
}
