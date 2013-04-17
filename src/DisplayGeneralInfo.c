//
//	DisplayGeneralInfo.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//	void SetGeneralInfo(HWND hwnd)
//
//	Fill the general-tab-pane with general info for the
//  specified window
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "WinSpy.h"
#include "Utils.h"

void RemoveHyperlink(HWND hwnd, UINT staticid);
void MakeHyperlink(HWND hwnd, UINT staticid, COLORREF crLink);


void SetGeneralInfo(HWND hwnd)
{
	TCHAR	ach[256];
	HWND	hwndDlg = WinSpyTab[GENERAL_TAB].hwnd;
	RECT	rect;
	int		x1, y1;
	int		i, numbytes;

	if(hwnd == 0) return;

	//handle
	wsprintf(ach, szHexFmt, hwnd);
	SetDlgItemText(hwndDlg, IDC_HANDLE, ach);

	//caption
	ShowDlgItem(hwndDlg, IDC_CAPTION1, SW_SHOW);
	ShowDlgItem(hwndDlg, IDC_CAPTION2, SW_HIDE);

	SendDlgItemMessage(hwndDlg, IDC_CAPTION2, CB_RESETCONTENT, 0, 0);

	// SendMessage is better than GetWindowText, 
	// because it gets text of children in other processes
	if(spy_fPassword == FALSE)
	{
		DWORD_PTR dwResult;

		ach[0] = 0;

		SendMessageTimeout(hwnd, WM_GETTEXT, sizeof(ach) / sizeof(TCHAR), (LPARAM)ach,
			SMTO_ABORTIFHUNG, 100, &dwResult); 

		SetDlgItemText(hwndDlg, IDC_CAPTION1, ach);	// edit box
		SetDlgItemText(hwndDlg, IDC_CAPTION2, ach);	// combo box
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_CAPTION1, spy_szPassword);	// edit box
		SetDlgItemText(hwndDlg, IDC_CAPTION2, spy_szPassword);	// combo box
	}

	//class name
	GetClassName(hwnd, ach, sizeof(ach)/sizeof(TCHAR));

	if(IsWindowUnicode(hwnd))	lstrcat(ach, _T("  (Unicode)"));

	SetDlgItemText(hwndDlg, IDC_CLASS, ach);

	//style
	wsprintf(ach, szHexFmt, GetWindowLong(hwnd, GWL_STYLE));
	
	if(IsWindowVisible(hwnd))	lstrcat(ach, _T("  (visible, "));
	else						lstrcat(ach, _T("  (hidden, "));

	if(IsWindowEnabled(hwnd))	lstrcat(ach, _T("enabled)"));
	else						lstrcat(ach, _T("disabled)"));

	SetDlgItemText(hwndDlg, IDC_STYLE, ach);

	//rectangle
	GetWindowRect(hwnd, &rect);
	x1 = rect.left;
	y1 = rect.top;

	wsprintf(ach, _T("(%d,%d) - (%d,%d)  -  %dx%d"), 
		rect.left,rect.top, rect.right,rect.bottom,
		(rect.right-rect.left), (rect.bottom-rect.top));

	SetDlgItemText(hwndDlg, IDC_RECTANGLE, ach);

	//client rect
	GetClientRect(hwnd, &rect);
	MapWindowPoints(hwnd, 0, (POINT *)&rect, 2);
	x1 = rect.left-x1;
	y1 = rect.top-y1;

	OffsetRect(&rect, -rect.left, -rect.top);
	OffsetRect(&rect, x1, y1);
	
	wsprintf(ach, _T("(%d,%d) - (%d,%d)  -  %dx%d"), 
		rect.left,rect.top, rect.right,rect.bottom,
		(rect.right-rect.left), (rect.bottom-rect.top));

	SetDlgItemText(hwndDlg, IDC_CLIENTRECT, ach);	

	//restored rect
	/*GetWindowPlacement(hwnd, &wp);
	wsprintf(ach, _T("(%d,%d) - (%d,%d)  -  %dx%d"), 
		wp.rcNormalPosition.left, wp.rcNormalPosition.top,
		wp.rcNormalPosition.right, wp.rcNormalPosition.bottom,
		(wp.rcNormalPosition.right-wp.rcNormalPosition.left),
		(wp.rcNormalPosition.bottom-wp.rcNormalPosition.top));

	SetDlgItemText(hwndDlg, IDC_RESTOREDRECT, ach);*/

	//window procedure
	if(spy_WndProc == 0)
	{
		wsprintf(ach, _T("N/A"));
		SetDlgItemText(hwndDlg, IDC_WINDOWPROC, ach);

		ShowDlgItem(hwndDlg, IDC_WINDOWPROC,  SW_SHOW);
		ShowDlgItem(hwndDlg, IDC_WINDOWPROC2, SW_HIDE);
	}
	else					
	{
		wsprintf(ach, szPtrFmt, spy_WndProc);
		SetDlgItemText(hwndDlg, IDC_WINDOWPROC2, ach);

		ShowDlgItem(hwndDlg, IDC_WINDOWPROC,  SW_HIDE);
		ShowDlgItem(hwndDlg, IDC_WINDOWPROC2, SW_SHOW);
	}

	//instance handle
	wsprintf(ach, szPtrFmt, GetWindowLongPtr(hwnd, GWLP_HINSTANCE));
	SetDlgItemText(hwndDlg, IDC_INSTANCE, ach);

	//user data
	wsprintf(ach, szPtrFmt, GetWindowLongPtr(hwnd, GWLP_USERDATA));
	SetDlgItemText(hwndDlg, IDC_USERDATA, ach);

	//control ID
	wsprintf(ach, szPtrFmt, GetWindowLongPtr(hwnd, GWLP_ID));
	SetDlgItemText(hwndDlg, IDC_CONTROLID, ach);

	//extra window bytes
	numbytes = GetClassLong(hwnd, GCL_CBWNDEXTRA);
	i = 0;

	SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_RESETCONTENT, 0, 0);	
	EnableDlgItem(hwndDlg, IDC_WINDOWBYTES, numbytes != 0);

	// Retrieve all the window bytes + add to combo box
	while(numbytes != 0)
	{
		if(numbytes >= 4)
			wsprintf(ach, _T("+%-8d  %08p"), i, GetWindowLongPtr(hwnd, i));
		else
			wsprintf(ach, _T("+%-8d  %s"), i, _T("(Unavailable)"));

		i += 4;
		numbytes = max(numbytes - 4, 0);

		SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_ADDSTRING, 0, (LPARAM)ach);
	}

	SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_SETCURSEL, 0, 0);
}
