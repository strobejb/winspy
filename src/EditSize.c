//
//	Edit Size Dialog.
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Just a simple dialog which allows you to edit a
//  window's size / position
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>

#include "resource.h"
#include "WinSpy.h"

//
//	rect - window coords
//
void SetupEdits(HWND hwndDlg, HWND hwndTarget, RECT *prect)
{
	DWORD dwStyle;
	RECT rect;

	CopyRect(&rect, prect);

	// Is this window a child control or not??
	dwStyle = GetWindowLong(hwndTarget, GWL_STYLE);

	// If this is a child window, then make it's coords
	// relative to it's parent.
	if(dwStyle & WS_CHILD)
		MapWindowPoints(NULL, GetParent(hwndTarget), (POINT *)&rect, 2);

	// Set the edit control's contents
	SetDlgItemInt(hwndDlg, IDC_EDITX, rect.left, TRUE);
	SetDlgItemInt(hwndDlg, IDC_EDITY, rect.top,  TRUE);
	SetDlgItemInt(hwndDlg, IDC_EDITW, rect.right-rect.left, TRUE);
	SetDlgItemInt(hwndDlg, IDC_EDITH, rect.bottom-rect.top, TRUE);
}

//
//	Set the target window's size/pos, based on the
//  contents of the edit boxes
//
void SetTargetPos(HWND hwndDlg, HWND hwndTarget)
{
	RECT rect;

	// We're using rect.bottom + rect.right as height/width
	rect.left   = GetDlgItemInt(hwndDlg, IDC_EDITX, 0, TRUE);
	rect.top    = GetDlgItemInt(hwndDlg, IDC_EDITY, 0, TRUE);
	rect.right  = GetDlgItemInt(hwndDlg, IDC_EDITW, 0, TRUE);
	rect.bottom = GetDlgItemInt(hwndDlg, IDC_EDITH, 0, TRUE);
	
	SetWindowPos(hwndTarget, 0, rect.left, rect.top, rect.right, rect.bottom,
		SWP_NOACTIVATE | SWP_NOZORDER);
}

//
//	Dialog procedure for the edit size window
//
BOOL CALLBACK EditSizeDlgProc(HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	RECT  rect;	

	static HWND hwndTarget;	// target window!
	static RECT rect0;		// original coords

	switch(iMsg)
	{
	case WM_INITDIALOG:

		hwndTarget = (HWND)lParam;

		GetWindowRect(hwndTarget, &rect0);
		CopyRect(&rect, &rect0);

		SetupEdits(hwnd, hwndTarget, &rect);

		// Set up the spin controls
		SendDlgItemMessage(hwnd, IDC_SPINX, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL , UD_MINVAL));
		SendDlgItemMessage(hwnd, IDC_SPINY, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL , UD_MINVAL));
		SendDlgItemMessage(hwnd, IDC_SPINW, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL , UD_MINVAL));
		SendDlgItemMessage(hwnd, IDC_SPINH, UDM_SETRANGE, 0, MAKELONG(UD_MAXVAL , UD_MINVAL));

		return TRUE;

	case WM_CLOSE:
		EndDialog(hwnd, 0);
		return TRUE;

	case WM_VSCROLL:

		SetTargetPos(hwnd, hwndTarget);

		// Get the window's coords again to see what happened
		GetWindowRect(hwndTarget, &rect);
		SetupEdits(hwnd, hwndTarget, &rect);

		return TRUE;

	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDC_RESET:
	
			// go back to the original coords
			SetupEdits(hwnd, hwndTarget, &rect0);
			SetTargetPos(hwnd, hwndTarget);

			// Get the window's coords again to see what happened
			GetWindowRect(hwndTarget, &rect);
			SetupEdits(hwnd, hwndTarget, &rect);

			return TRUE;

		case IDC_ADJUST:
			
			SetTargetPos(hwnd, hwndTarget);

			// Get the window's coords again to see what happened
			GetWindowRect(hwndTarget, &rect);
			SetupEdits(hwnd, hwndTarget, &rect);

			return TRUE;

		case IDCANCEL:
			EndDialog(hwnd, 0);
			return TRUE;

		}

		return FALSE;
	}

	return FALSE;
}


void ShowEditSizeDlg(HWND hwndParent, HWND hwndTarget)
{
	if(IsWindow(spy_hCurWnd))
	{
		DialogBoxParam(
			GetModuleHandle(0), 
			MAKEINTRESOURCE(IDD_ADJUSTWINPOS), 
			hwndParent, 
			EditSizeDlgProc, 
			(LPARAM)hwndTarget);

		SetGeneralInfo(hwndTarget);
	}
	else
	{
		MessageBox(hwndParent, 
			_T("Not a valid window"), 
			szAppName, 
			MB_OK | MB_ICONEXCLAMATION);
	}
}

