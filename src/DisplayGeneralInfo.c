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

typedef struct
{
	HWND hwnd;
	TCHAR caption[256];
	TCHAR className[256];
	BOOL unicode, visible, enabled;
	DWORD style;
	RECT windowRect, clientRect;
	WNDPROC wndProc;
	LONG_PTR instance, userData, controlId;
	UINT numBytes;
	LONG_PTR *extraBytes;
} GeneralInfo;

static GeneralInfo capturedGeneralInfo;

static void ReadGeneralInfo(HWND hwnd, GeneralInfo *info, BOOL preview)
{
	UINT i;

	ZeroMemory(info, sizeof(*info));
	info->hwnd = hwnd;
	GetClassName(hwnd, info->className, ARRAYSIZE(info->className) - 12);
	info->unicode = IsWindowUnicode(hwnd);
	info->visible = IsWindowVisible(hwnd);
	info->enabled = IsWindowEnabled(hwnd);
	info->style = GetWindowLong(hwnd, GWL_STYLE);
	GetWindowRect(hwnd, &info->windowRect);
	if(GetClientRect(hwnd, &info->clientRect))
	{
		MapWindowPoints(hwnd, NULL, (POINT *)&info->clientRect, 2);
		OffsetRect(&info->clientRect, -info->windowRect.left, -info->windowRect.top);
	}

	info->wndProc = preview ? NULL : spy_WndProc;
	info->instance = GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	info->userData = GetWindowLongPtr(hwnd, GWLP_USERDATA);
	info->controlId = GetWindowLongPtr(hwnd, GWLP_ID);
	info->numBytes = GetClassLong(hwnd, GCL_CBWNDEXTRA);
	if(info->numBytes >= 4)
	{
		info->extraBytes = HeapAlloc(GetProcessHeap(), 0, (info->numBytes / 4) * sizeof(LONG_PTR));
		if(info->extraBytes)
		{
			for(i = 0; i < info->numBytes / 4; i++)
				info->extraBytes[i] = GetWindowLongPtr(hwnd, i * 4);
		}
	}

	if(!preview && spy_fPassword)
		lstrcpyn(info->caption, spy_szPassword, ARRAYSIZE(info->caption));
	else
	{
		DWORD_PTR result;
		// WM_GETTEXT also reads child control text across processes.
		SendMessageTimeout(hwnd, WM_GETTEXT, ARRAYSIZE(info->caption), (LPARAM)info->caption,
			SMTO_BLOCK | SMTO_ABORTIFHUNG | SMTO_ERRORONEXIT, 100, &result);
	}
}

static void DisplayGeneralInfo(const GeneralInfo *info)
{
	TCHAR	ach[256];
	HWND	hwndDlg = WinSpyTab[GENERAL_TAB].hwnd;
	RECT	rect;
	UINT	i;

	//handle
	wsprintf(ach, szHexFmt, info->hwnd);
	SetDlgItemText(hwndDlg, IDC_HANDLE, ach);

	//caption
	ShowDlgItem(hwndDlg, IDC_CAPTION1, SW_SHOW);
	ShowDlgItem(hwndDlg, IDC_CAPTION2, SW_HIDE);

	SendDlgItemMessage(hwndDlg, IDC_CAPTION2, CB_RESETCONTENT, 0, 0);

	SetDlgItemText(hwndDlg, IDC_CAPTION1, info->caption);
	SetDlgItemText(hwndDlg, IDC_CAPTION2, info->caption);

	//class name
	lstrcpy(ach, info->className);
	if(info->unicode)	lstrcat(ach, _T("  (Unicode)"));

	SetDlgItemText(hwndDlg, IDC_CLASS, ach);

	//style
	wsprintf(ach, szHexFmt, info->style);
	
	if(info->visible)	lstrcat(ach, _T("  (visible, "));
	else						lstrcat(ach, _T("  (hidden, "));

	if(info->enabled)	lstrcat(ach, _T("enabled)"));
	else						lstrcat(ach, _T("disabled)"));

	SetDlgItemText(hwndDlg, IDC_STYLE, ach);

	//rectangle
	rect = info->windowRect;

	wsprintf(ach, _T("(%d,%d) - (%d,%d)  -  %dx%d"), 
		rect.left,rect.top, rect.right,rect.bottom,
		(rect.right-rect.left), (rect.bottom-rect.top));

	SetDlgItemText(hwndDlg, IDC_RECTANGLE, ach);

	//client rect
	rect = info->clientRect;
	
	wsprintf(ach, _T("(%d,%d) - (%d,%d)  -  %dx%d"), 
		rect.left,rect.top, rect.right,rect.bottom,
		(rect.right-rect.left), (rect.bottom-rect.top));

	SetDlgItemText(hwndDlg, IDC_CLIENTRECT, ach);	

	//window procedure
	if(info->wndProc == 0)
	{
		wsprintf(ach, _T("N/A"));
		SetDlgItemText(hwndDlg, IDC_WINDOWPROC, ach);

		ShowDlgItem(hwndDlg, IDC_WINDOWPROC,  SW_SHOW);
		ShowDlgItem(hwndDlg, IDC_WINDOWPROC2, SW_HIDE);
	}
	else					
	{
		wsprintf(ach, szPtrFmt, info->wndProc);
		SetDlgItemText(hwndDlg, IDC_WINDOWPROC2, ach);

		ShowDlgItem(hwndDlg, IDC_WINDOWPROC,  SW_HIDE);
		ShowDlgItem(hwndDlg, IDC_WINDOWPROC2, SW_SHOW);
	}

	//instance handle
	wsprintf(ach, szPtrFmt, info->instance);
	SetDlgItemText(hwndDlg, IDC_INSTANCE, ach);

	//user data
	wsprintf(ach, szPtrFmt, info->userData);
	SetDlgItemText(hwndDlg, IDC_USERDATA, ach);

	//control ID
	wsprintf(ach, szPtrFmt, info->controlId);
	SetDlgItemText(hwndDlg, IDC_CONTROLID, ach);

	//extra window bytes
	SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_RESETCONTENT, 0, 0);	
	EnableDlgItem(hwndDlg, IDC_WINDOWBYTES, info->numBytes != 0);

	// Retrieve all the window bytes + add to combo box
	for(i = 0; i < info->numBytes; i += 4)
	{
		if(info->numBytes - i >= 4 && info->extraBytes)
			wsprintf(ach, _T("+%-8d  %08p"), i, info->extraBytes[i / 4]);
		else
			wsprintf(ach, _T("+%-8d  %s"), i, _T("(Unavailable)"));

		SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_ADDSTRING, 0, (LPARAM)ach);
	}

	SendDlgItemMessage(hwndDlg, IDC_WINDOWBYTES, CB_SETCURSEL, 0, 0);
}

static void FreeGeneralInfo(GeneralInfo *info)
{
	if(info->extraBytes)
		HeapFree(GetProcessHeap(), 0, info->extraBytes);
	info->extraBytes = NULL;
}

static void UpdateGeneralInfo(HWND hwnd, BOOL preview)
{
	GeneralInfo info;

	if(hwnd)
	{
		ReadGeneralInfo(hwnd, &info, preview);
		DisplayGeneralInfo(&info);
		FreeGeneralInfo(&info);
	}
}

void SetGeneralInfo(HWND hwnd)
{
	UpdateGeneralInfo(hwnd, FALSE);
}

void PreviewGeneralInfo(HWND hwnd)
{
	UpdateGeneralInfo(hwnd, TRUE);
}

void CaptureGeneralInfo(HWND hwnd)
{
	ClearCapturedGeneralInfo();
	ReadGeneralInfo(hwnd, &capturedGeneralInfo, FALSE);
	RestoreCapturedGeneralInfo();
}

void RestoreCapturedGeneralInfo(void)
{
	DisplayGeneralInfo(&capturedGeneralInfo);
}

void ClearCapturedGeneralInfo(void)
{
	FreeGeneralInfo(&capturedGeneralInfo);
}
