//
//	DisplayWindowInfo.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//  void SetWindowInfo(HWND hwnd)
//
//	Fill the window-tab-pane with list of child+siblings
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "WinSpy.h"

static BOOL CALLBACK ChildWindowProc(HWND hwnd, LPARAM lParam)
{
	TCHAR  ach[256];
	TCHAR  cname[256];
	TCHAR  wname[256];
	LVITEM lvitem;
	
	//only display 1st generation (1-deep) children - 
	//(don't display child windows of child windows)
	if(GetParent(hwnd) == spy_hCurWnd)
	{
		GetClassName(hwnd, cname, sizeof(cname) / sizeof(TCHAR));
		GetWindowText(hwnd, wname, sizeof(wname) / sizeof(TCHAR));
		wsprintf(ach, szHexFmt, hwnd);

		lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
		lvitem.iSubItem = 0;
		lvitem.pszText = ach;
		lvitem.iItem = 0;
		lvitem.state = 0;
		lvitem.stateMask = 0;
		lvitem.iImage = 0;

		ListView_InsertItem((HWND)lParam, &lvitem);
		ListView_SetItemText((HWND)lParam, 0, 1, cname);
		ListView_SetItemText((HWND)lParam, 0, 2, wname);
	}	
	return TRUE;
}

static BOOL CALLBACK SiblingWindowProc(HWND hwnd, LPARAM lParam)
{
	TCHAR  ach[256];
	TCHAR  cname[256];
	TCHAR  wname[256];
	LVITEM lvitem;
		
	//sibling windows must share the same parent
	if(spy_hCurWnd != hwnd && GetParent(hwnd) == GetParent(spy_hCurWnd))
	{
		GetClassName(hwnd, cname, sizeof(cname) / sizeof(TCHAR));
		GetWindowText(hwnd, wname, sizeof(wname) / sizeof(TCHAR));
		wsprintf(ach, szHexFmt, hwnd);

		lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
		lvitem.iSubItem = 0;
		lvitem.pszText = ach;
		lvitem.iItem = 0;
		lvitem.state = 0;
		lvitem.stateMask = 0;
		lvitem.iImage = 0;

		ListView_InsertItem((HWND)lParam, &lvitem);
		ListView_SetItemText((HWND)lParam, 0, 1, cname);
		ListView_SetItemText((HWND)lParam, 0, 2, wname);
	}	

	return TRUE;
}

//
//	Get a list of all Child + Siblings for the specified window - 
//  Update the Windows tab accordingly
//
void SetWindowInfo(HWND hwnd)
{
	TCHAR ach[10];

	HWND hwndList1 = GetDlgItem(WinSpyTab[WINDOW_TAB].hwnd, IDC_LIST1);
	HWND hwndList2 = GetDlgItem(WinSpyTab[WINDOW_TAB].hwnd, IDC_LIST2);

	if(hwnd == 0) return;

	ListView_DeleteAllItems(hwndList1);
	ListView_DeleteAllItems(hwndList2);

	// Get all children of the window
	EnumChildWindows(hwnd, ChildWindowProc, (LPARAM)hwndList1);

	// Get children of it's PARENT (i.e, it's siblings!)
	EnumChildWindows(GetParent(hwnd), SiblingWindowProc, (LPARAM)hwndList2);

	// Set the Parent hyperlink
	wsprintf(ach, szHexFmt, GetParent(hwnd));
	SetDlgItemText(WinSpyTab[WINDOW_TAB].hwnd, IDC_PARENT, ach);

	// Set the Owner hyperlink
	wsprintf(ach, szHexFmt, GetWindow(hwnd, GW_OWNER));
	SetDlgItemText(WinSpyTab[WINDOW_TAB].hwnd, IDC_OWNER, ach);
}
