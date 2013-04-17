//
//	WinSpyDlgs.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Contains all the dialog box procedures for
//  each tab-pane dialog control.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINDOWS 0x400
#define _WIN32_WINNT 0x400

#include <windows.h>
#include <shellapi.h>
#include <tchar.h>
#include <commctrl.h>
#include "resource.h"
#include "WinSpy.h"
#include "BitmapButton.h"
#include "CaptureWindow.h"
#include "Utils.h"

void  MakeHyperlink     (HWND hwnd, UINT staticid, COLORREF crLink);
void  RemoveHyperlink	(HWND hwnd, UINT staticid);
void  GetRemoteInfo		(HWND hwnd);

TCHAR szWarning1[] = _T("Are you sure you want to close this process?");
TCHAR szWarning2[] = _T("WARNING: Terminating a process can cause undesired\r\n")\
					 _T("results including loss of data and system instability. The\r\n")\
					 _T("process will not be given the chance to save its state or\r\n")\
					 _T("data before it is terminated. Are you sure you want to\r\n")\
					 _T("terminate the process?");

extern TCHAR szPath[];

//
// save the tree-structure to clipboard?
//

//
//	Destroy specified window
//
/*BOOL RemoteDestroyWindow(HWND hwnd)
{
	DWORD pid, tid;
	HANDLE hThread;
	DWORD exitcode = FALSE;
	PVOID proc = GetProcAddress(GetModuleHandle(_T("user32.dll")), "DestroyWindow");
	int (WINAPI * ZwAlertResumeThread)(HANDLE, DWORD*);

	ZwAlertResumeThread = (PVOID)GetProcAddress(GetModuleHandle(_T("ntdll.dll")), "ZwAlertResumeThread");

	tid = GetWindowThreadProcessId(hwnd, &pid);

	hThread = OpenThread(THREAD_ALL_ACCESS, FALSE, tid);

	SuspendThread(hThread);
	//QueueUserAPC((PAPCFUNC)DestroyWindow, hThread, (ULONG_PTR)hwnd);
	QueueUserAPC((PAPCFUNC)proc, hThread, (ULONG_PTR)hwnd);
	ZwAlertResumeThread(hThread, 0);

	CloseHandle(hThread);
	

	return exitcode;
}*/

//
//
//
UINT WinSpy_PopupCommandHandler(HWND hwndDlg, UINT uCmdId, HWND hwndTarget)
{
	DWORD dwStyle, dwStyleEx;
	DWORD dwSWPflags;
	HWND  hwndZ;

	dwStyle   = GetWindowLong(hwndTarget, GWL_STYLE);
	dwStyleEx = GetWindowLong(hwndTarget, GWL_EXSTYLE);

	switch(uCmdId)		
	{
	// Show / Hide
	case IDM_POPUP_VISIBLE:
		
		dwSWPflags = SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_NOZORDER;

		if(dwStyle & WS_VISIBLE)
			dwSWPflags |= SWP_HIDEWINDOW;
		else
			dwSWPflags |= SWP_SHOWWINDOW;

		SetWindowPos(hwndTarget, 0, 0,0,0,0, dwSWPflags);
		
		return 0;
		
	// Enable / Disable
	case IDM_POPUP_ENABLED:
		EnableWindow(hwndTarget, (dwStyle & WS_DISABLED) ? TRUE : FALSE);
		return 0;
		
	// Ontop / Not ontop
	case IDM_POPUP_ONTOP:

		dwSWPflags = SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE;

		if(dwStyleEx & WS_EX_TOPMOST)
			hwndZ = HWND_NOTOPMOST;
		else
			hwndZ = HWND_TOPMOST;

		SetWindowPos(hwndTarget, hwndZ,	0,0,0,0, dwSWPflags);

		return 0;
		
	// Show the edit-size dialog
	case IDM_POPUP_SETPOS:
		
		ShowEditSizeDlg(hwndDlg, hwndTarget);
		return 0;

	case IDM_POPUP_TOFRONT:

		SetWindowPos(hwndTarget, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		return 0;

	case IDM_POPUP_TOBACK:
		SetWindowPos(hwndTarget, HWND_BOTTOM, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		return 0;

	// Close window
	case IDM_POPUP_CLOSE:
		PostMessage(hwndTarget, WM_CLOSE, 0, 0);
		return 0;

	//case IDM_POPUP_DESTROY:
	//	RemoteDestroyWindow(hwndTarget);
	//	return 0;

	// new for 1.6
	case IDM_POPUP_COPY:
		CaptureWindow(hwndDlg, hwndTarget);
		return 0;

	case IDM_POPUP_SAVE:
	//	SaveTreeStructure(hwndDlg, hwndTarget);
		return 0;
		
	default:
		return 0;
		
	}
}

//
//	Configure the popup menu
//
void WinSpy_SetupPopupMenu(HMENU hMenu, HWND hwndTarget)
{
	BOOL fParentVisible;
	BOOL fParentEnabled;

	DWORD dwStyle;
	DWORD dwStyleEx;

	dwStyle   = GetWindowLong(hwndTarget, GWL_STYLE);
	dwStyleEx = GetWindowLong(hwndTarget, GWL_EXSTYLE);

	if(GetParent(hwndTarget))
		fParentVisible = IsWindowVisible(GetParent(hwndTarget));
	else
		fParentVisible = TRUE;
	
	if(GetParent(hwndTarget))
		fParentEnabled = IsWindowEnabled(GetParent(hwndTarget));
	else
		fParentEnabled = TRUE;
	
	if(dwStyle & WS_VISIBLE)
		CheckMenuItem(hMenu, IDM_POPUP_VISIBLE, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, IDM_POPUP_VISIBLE, MF_BYCOMMAND | MF_UNCHECKED);
	
	if(dwStyle & WS_DISABLED)
		CheckMenuItem(hMenu, IDM_POPUP_ENABLED, MF_BYCOMMAND | MF_UNCHECKED);
	else
		CheckMenuItem(hMenu, IDM_POPUP_ENABLED, MF_BYCOMMAND | MF_CHECKED);
	
	if(dwStyleEx & WS_EX_TOPMOST)
		CheckMenuItem(hMenu, IDM_POPUP_ONTOP, MF_BYCOMMAND | MF_CHECKED);
	else
		CheckMenuItem(hMenu, IDM_POPUP_ONTOP, MF_BYCOMMAND | MF_UNCHECKED);
	
	EnableMenuItem(hMenu, IDM_POPUP_VISIBLE, MF_BYCOMMAND | (fParentVisible ? MF_ENABLED : MF_DISABLED|MF_GRAYED));
	EnableMenuItem(hMenu, IDM_POPUP_ONTOP,   MF_BYCOMMAND | (fParentVisible ? MF_ENABLED : MF_DISABLED|MF_GRAYED));
	EnableMenuItem(hMenu, IDM_POPUP_ENABLED, MF_BYCOMMAND | (fParentEnabled ? MF_ENABLED : MF_DISABLED|MF_GRAYED));
}

//
//	General tab
//
LRESULT CALLBACK GeneralDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND  hCtrl;
	TCHAR szCaption[256];
	HWND  hwndEdit1, hwndEdit2;
	HMENU hMenu, hPopup;
	RECT  rect;
	UINT  uCmd;

	switch(iMsg)
	{
	case WM_INITDIALOG:

		// Convert standard buttons into bitmapped-buttons
		MakeDlgBitmapButton(hwnd, IDC_HANDLE_MENU,	IDI_ICON10);
		MakeDlgBitmapButton(hwnd, IDC_EDITSIZE,		IDI_ICON5);
		MakeDlgBitmapButton(hwnd, IDC_SETCAPTION,	IDI_ICON17);

		MakeHyperlink(hwnd, IDC_WINDOWPROC, RGB(0,0,255));

		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		// added in 1.6: windowproc URL control
		case IDC_WINDOWPROC: 
			//RemoveHyperlink(hwnd, IDC_WINDOWPROC);
			//InvalidateRect(GetDlgItem(hwnd, IDC_WINDOWPROC), 0, 0);
			ShowDlgItem(hwnd, IDC_WINDOWPROC, SW_HIDE);
			ShowDlgItem(hwnd, IDC_WINDOWPROC2, SW_SHOW);
			GetRemoteInfo(spy_hCurWnd);
			SetClassInfo(spy_hCurWnd);
			return TRUE;

		case IDC_EDITSIZE:

			// Display the edit-size dialog
			ShowEditSizeDlg(hwnd, spy_hCurWnd);
			return TRUE;

		case IDC_HANDLE_MENU:

			// Show our popup menu under this button
			hCtrl = spy_hCurWnd;

			hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU2));
			hPopup = GetSubMenu(hMenu, 0);

			GetWindowRect((HWND)lParam, &rect);

			WinSpy_SetupPopupMenu(hPopup, hCtrl);

			uCmd = TrackPopupMenu(hPopup, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, 
				rect.right, rect.bottom, 0, hwnd, 0);

			WinSpy_PopupCommandHandler(hwnd, uCmd, hCtrl);
			SetGeneralInfo(hCtrl);

			DestroyMenu(hMenu);

			return TRUE;

		case IDC_SETCAPTION:

			// Set the target's window caption to the contents of the edit box
			hwndEdit1 = GetDlgItem(hwnd, IDC_CAPTION1);
			hwndEdit2 = GetDlgItem(hwnd, IDC_CAPTION2);

			// Show the combo box and hide the edit box
			if(IsWindowVisible(hwndEdit1))
			{
				// Copy the contents of the edit box to the combo box
				GetWindowText(hwndEdit1, szCaption, sizeof(szCaption) / sizeof(TCHAR));
				SetWindowText(hwndEdit2, szCaption);

				ShowWindow(hwndEdit2, SW_SHOW);
				ShowWindow(hwndEdit1, SW_HIDE);
				SetFocus(hwndEdit2);
			}

			hCtrl = (HWND)spy_hCurWnd;

			// get the original text and add it to the combo list
			GetWindowText(hCtrl, szCaption, sizeof(szCaption) / sizeof(TCHAR));

			SendMessage(hwndEdit2, CB_ADDSTRING, 0, (LPARAM)szCaption);

			// now see what the new caption is to be
			GetWindowText(hwndEdit2, szCaption, sizeof(szCaption) / sizeof(TCHAR));

			// set the text to the new string
			if(hCtrl != 0 && IsWindow(hCtrl))
				SendMessage(hCtrl, WM_SETTEXT, 0, (LPARAM)szCaption);

			return TRUE;
		}

		return FALSE;

	case WM_DRAWITEM:

		if(wParam == IDC_EDITSIZE || wParam == IDC_HANDLE_MENU || wParam == IDC_SETCAPTION)
			return DrawBitmapButton((DRAWITEMSTRUCT *)lParam);
		else
			break;

	}
	
	return FALSE;
}

//
//	Style tab
//
LRESULT CALLBACK StyleDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
	case WM_INITDIALOG:

		MakeDlgBitmapButton(hwnd, IDC_EDITSTYLE,   IDI_ICON5);
		MakeDlgBitmapButton(hwnd, IDC_EDITSTYLEEX, IDI_ICON5);

		return TRUE;

	case WM_MEASUREITEM:
		return FunkyList_MeasureItem(hwnd, (UINT)wParam, (MEASUREITEMSTRUCT *)lParam);

	case WM_DRAWITEM:

		if(wParam == IDC_LIST1 || wParam == IDC_LIST2)
		{
			return FunkyList_DrawItem(hwnd, (UINT)wParam, (DRAWITEMSTRUCT *)lParam);
		}
		else if(wParam == IDC_EDITSTYLE || wParam == IDC_EDITSTYLEEX)
		{
			return DrawBitmapButton((DRAWITEMSTRUCT *)lParam);
		}
		else
		{
			return FALSE;
		}

	case WM_COMMAND:
		
		switch(LOWORD(wParam))
		{
		case IDC_EDITSTYLE:
			ShowWindowStyleEditor(hwnd, spy_hCurWnd, FALSE);
			return TRUE;

		case IDC_EDITSTYLEEX:
			ShowWindowStyleEditor(hwnd, spy_hCurWnd, TRUE);
			return TRUE;

		default:
			break;
		}

		return FALSE;
	}
	return FALSE;
}

//
//	Window tab
//
LRESULT CALLBACK WindowDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND      hwndList1, hwndList2;
	LVCOLUMN  lvcol;
	RECT      rect;
	int       width;
	int       xs[] = { 64, 100, 140 };
	TCHAR     ach[10];
	NMITEMACTIVATE *nmatv;

	switch(iMsg)
	{
	case WM_INITDIALOG:

		hwndList1 = GetDlgItem(hwnd, IDC_LIST1);
		hwndList2 = GetDlgItem(hwnd, IDC_LIST2);

		// Full row select for both ListViews
		ListView_SetExtendedListViewStyle(hwndList1, LVS_EX_FULLROWSELECT);
		ListView_SetExtendedListViewStyle(hwndList2, LVS_EX_FULLROWSELECT);

		// See how much space we have for the header columns to
		// fit exactly into the dialog
		GetClientRect(hwndList1, &rect);
		width = rect.right;
		width -= GetSystemMetrics(SM_CXVSCROLL);

		lvcol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvcol.cx   = 64;
		lvcol.iSubItem = 0;
		lvcol.pszText = _T("Handle");
		ListView_InsertColumn(hwndList1, 0, &lvcol);
		ListView_InsertColumn(hwndList2, 0, &lvcol);
		width -= lvcol.cx;
		
		lvcol.pszText = _T("Class Name");
		lvcol.cx   = 100;
		ListView_InsertColumn(hwndList1, 1, &lvcol);
		ListView_InsertColumn(hwndList2, 1, &lvcol);
		width -= lvcol.cx;

		lvcol.pszText = _T("Window Text");
		lvcol.cx   = max(width, 64);
		ListView_InsertColumn(hwndList1, 2, &lvcol);
		ListView_InsertColumn(hwndList2, 2, &lvcol);

		// Make hyperlinks from our two static controls
		MakeHyperlink(hwnd, IDC_PARENT, RGB(0,0,255));
		MakeHyperlink(hwnd, IDC_OWNER,  RGB(0,0,255));

		return TRUE;

	case WM_NOTIFY:
		nmatv = (NMITEMACTIVATE *)lParam;

		if(nmatv->hdr.code == NM_DBLCLK)
		{
			ListView_GetItemText(nmatv->hdr.hwndFrom, nmatv->iItem, 0, ach, sizeof(ach) / sizeof(TCHAR));
			DisplayWindowInfo((HWND)_tstrtoib16(ach));
		}

		return FALSE;

	case WM_SYSCOLORCHANGE:
		ListView_SetBkColor(GetDlgItem(hwnd, IDC_LIST1), GetSysColor(COLOR_WINDOW));
		ListView_SetBkColor(GetDlgItem(hwnd, IDC_LIST2), GetSysColor(COLOR_WINDOW));
		return 0;

	// if clicked on one of the underlined static controls, then
	// display window info..
	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDC_PARENT: case IDC_OWNER: 
			GetDlgItemText(hwnd, LOWORD(wParam), ach, sizeof(ach)/sizeof(TCHAR));
			DisplayWindowInfo((HWND)_tstrtoib16(ach));
			return TRUE;
		}

		return FALSE;
	}
	return FALSE;
}

//
//	Properties tab
//
LRESULT CALLBACK PropertyDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HWND      hwndList1;
	LVCOLUMN  lvcol;
	RECT      rect;
	int       width;

	switch(iMsg)
	{
	case WM_INITDIALOG:

		// Full-row selection for the ListView
		hwndList1 = GetDlgItem(hwnd, IDC_LIST1);
		ListView_SetExtendedListViewStyle(hwndList1, LVS_EX_FULLROWSELECT);

		// Work out how big the header-items need to be 
		GetClientRect(hwndList1, &rect);
		width = rect.right;
		width -= GetSystemMetrics(SM_CXVSCROLL);

		// Insert "Handle" header-item
		lvcol.mask = LVCF_WIDTH | LVCF_TEXT | LVCF_SUBITEM;
		lvcol.cx   = 20+12*sizeof(LONG_PTR);
		lvcol.iSubItem = 0;
		lvcol.pszText = _T("Handle");
		ListView_InsertColumn(hwndList1, 0, &lvcol);
		width -= lvcol.cx;		
		
		// Insert "Property" header-item
		lvcol.pszText = _T("Property Name");
		lvcol.cx   = width;
		ListView_InsertColumn(hwndList1, 1, &lvcol);

		return TRUE;
	
	case WM_SYSCOLORCHANGE:

		// Need to react to system colour changes
		ListView_SetBkColor(GetDlgItem(hwnd, IDC_LIST1), GetSysColor(COLOR_WINDOW));
		return 0;
	}

	return FALSE;
}

//
// Class tab.
//
LRESULT CALLBACK ClassDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch(iMsg)
	{
	// Just make the class-name edit-box look normal, even
	// though it is read-only
	case WM_CTLCOLORSTATIC:

		if((HWND)lParam == GetDlgItem(hwnd, IDC_CLASSNAME))
		{
			SetTextColor((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor((HDC)wParam, GetSysColor(COLOR_WINDOW));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		}
		else
			return FALSE;
	}

	return FALSE;
}

//
//	Process Tab
//
LRESULT CALLBACK ProcessDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	HMENU  hMenu, hPopup;
	RECT   rect;
	UINT   uCmd;
	DWORD  dwThreadId;
	DWORD  dwProcessId;

	switch(iMsg)
	{
	case WM_INITDIALOG:
		MakeDlgBitmapButton(hwnd, IDC_PROCESS_MENU, IDI_ICON10);
		return TRUE;

	case WM_COMMAND:

		switch(LOWORD(wParam))
		{
		case IDC_PROCESS_MENU:

			// Load the menu
			dwThreadId = GetWindowThreadProcessId(spy_hCurWnd, &dwProcessId);

			hMenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
			hPopup = GetSubMenu(hMenu, 0);

			GetWindowRect((HWND)lParam, &rect);

			//
			// Display a popup menu underneath the close button
			//
			uCmd = TrackPopupMenu(hPopup, TPM_RIGHTALIGN | TPM_TOPALIGN | TPM_RETURNCMD, 
				rect.right, rect.bottom, 0, hwnd, 0);

			switch(uCmd)
			{
			case IDM_WINSPY_FINDEXE:
			
				{
					TCHAR szExplorer[MAX_PATH];
					TCHAR szPath[MAX_PATH];

					GetDlgItemText(hwnd, IDC_PROCESSPATH, szPath, sizeof(szPath)/sizeof(TCHAR));

					wsprintf(szExplorer, _T("/select,\"%s\""), szPath);
					ShellExecute(0, _T("open"), _T("explorer"), szExplorer, 0, SW_SHOW);

				}

				break;

			// Forcibly terminate!
			case IDM_WINSPY_TERMINATE:

				if(MessageBox(hwnd, szWarning2, szAppName, MB_YESNO|MB_ICONWARNING) == IDYES)
				{
					HANDLE hProcess;
					
					hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, dwProcessId);
					
					
					if(hProcess != 0)
					{
						TerminateProcess(hProcess, -1);
						CloseHandle(hProcess);
					}
					else
					{
						MessageBox(hwnd, _T("Invalid Process Id"), szAppName, MB_OK|MB_ICONWARNING);
					}
				}

				break;

			// Cleanly exit. Won't work if app. is hung
			case IDM_WINSPY_POSTQUIT:
				
				if(MessageBox(hwnd, szWarning1, szAppName, MB_YESNO|MB_ICONWARNING) == IDYES)
				{
					PostThreadMessage(dwThreadId, WM_QUIT, 0, 0);
				}
				
				break;

			}

			DestroyMenu(hMenu);
			return TRUE;

		}

		return FALSE;

	case WM_CTLCOLORSTATIC:

		if((HWND)lParam == GetDlgItem(hwnd, IDC_PROCESSNAME) ||
		   (HWND)lParam == GetDlgItem(hwnd, IDC_PROCESSPATH) )
		{
			SetTextColor ((HDC)wParam, GetSysColor(COLOR_WINDOWTEXT));
			SetBkColor   ((HDC)wParam, GetSysColor(COLOR_WINDOW));
			return (BOOL)GetSysColorBrush(COLOR_WINDOW);
		}


		return FALSE;

	case WM_DRAWITEM:
		return DrawBitmapButton((DRAWITEMSTRUCT *)lParam);
	}

	return FALSE;
}