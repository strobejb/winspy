//
//	DisplayScrollInfo.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//	void SetScrollInfo(HWND hwnd)
//
//	Fill the scrollbar-tab-pane with scrollbar info for the
//  specified window
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "WinSpy.h"

void SetScrollbarInfo(HWND hwnd)
{
	SCROLLINFO si;
	DWORD  dwStyle;
	int    bartype;
	TCHAR  ach[256];
	HWND   hwndDlg = WinSpyTab[PROPERTY_TAB].hwnd;

	if(hwnd == 0) return;

	si.cbSize = sizeof(SCROLLINFO);
	si.fMask  = SIF_ALL;

	dwStyle = GetWindowLong(hwnd, GWL_STYLE);

	bartype = SB_HORZ;

	GetClassName(hwnd, ach, sizeof(ach) / sizeof(TCHAR));
	
	if(lstrcmpi(ach, _T("ScrollBar")) == 0)
	{
		if((dwStyle & SBS_VERT) == 0)
			bartype = SB_CTL;
	
		SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Visible"));
	}
	else
	{
		if(dwStyle & WS_HSCROLL)
			SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Visible"));
		else
			SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Disabled"));
	}

	if(GetScrollInfo(hwnd, bartype, &si))
	{
		SetDlgItemInt(hwndDlg, IDC_HMIN, si.nMin, TRUE);
		SetDlgItemInt(hwndDlg, IDC_HMAX, si.nMax, TRUE);
		SetDlgItemInt(hwndDlg, IDC_HPOS, si.nPos, TRUE);	
		SetDlgItemInt(hwndDlg, IDC_HPAGE, si.nPage, TRUE);

		if(bartype == SB_HORZ)
		{
			if(dwStyle & WS_HSCROLL)
				SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Visible"));
			else
				SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Hidden"));
		}
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_HMIN, _T(""));
		SetDlgItemText(hwndDlg, IDC_HMAX, _T(""));
		SetDlgItemText(hwndDlg, IDC_HPOS, _T(""));
		SetDlgItemText(hwndDlg, IDC_HPAGE, _T(""));
		SetDlgItemText(hwndDlg, IDC_HSTATE, _T("Disabled"));
	}

	bartype = SB_VERT;

	if(lstrcmpi(ach, _T("ScrollBar")) == 0)
	{
		if((dwStyle & SBS_VERT) == SB_VERT)
			bartype = SB_CTL;
	
		SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Visible"));
	}
	else
	{
		if(dwStyle & WS_VSCROLL)
			SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Visible"));
		else
			SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Disabled"));
	}

	if(GetScrollInfo(hwnd, bartype, &si))
	{
		SetDlgItemInt(hwndDlg, IDC_VMIN, si.nMin, TRUE);
		SetDlgItemInt(hwndDlg, IDC_VMAX, si.nMax, TRUE);
		SetDlgItemInt(hwndDlg, IDC_VPOS, si.nPos, TRUE);	
		SetDlgItemInt(hwndDlg, IDC_VPAGE, si.nPage, TRUE);

		if(bartype == SB_VERT)
		{
			if(dwStyle & WS_VSCROLL)
				SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Visible"));
			else
				SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Hidden"));
		}
	}
	else
	{
		SetDlgItemText(hwndDlg, IDC_VMIN, _T(""));
		SetDlgItemText(hwndDlg, IDC_VMAX, _T(""));
		SetDlgItemText(hwndDlg, IDC_VPOS, _T(""));
		SetDlgItemText(hwndDlg, IDC_VPAGE, _T(""));
		SetDlgItemText(hwndDlg, IDC_VSTATE, _T("Disabled"));
	}
}
