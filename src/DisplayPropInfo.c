//
//	DisplayPropInfo.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//	void SetPropertyInfo(HWND hwnd)
//
//	Fill the properties-tab-pane with class info for the
//  specified window
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "WinSpy.h"

//
//	Called once for each window property
//
BOOL CALLBACK PropEnumProcEx(HWND hwnd, LPTSTR lpszString, HANDLE hData, ULONG_PTR dwUser)
{
	HWND   hwndList = (HWND)dwUser;
	TCHAR  ach[256];
	LVITEM lvitem;

	wsprintf(ach, szPtrFmt, hData);
	
	lvitem.mask = LVIF_TEXT | LVIF_IMAGE | LVIF_PARAM | LVIF_STATE;
	lvitem.iSubItem = 0;
	lvitem.pszText = ach;
	lvitem.iItem = 0;
	lvitem.state = 0;
	lvitem.stateMask = 0;
	lvitem.iImage = 0;

	ListView_InsertItem(hwndList, &lvitem);
	
	// check that lpszString is a valid string, and not an ATOM in disguise
	if(((DWORD_PTR)lpszString & 0xffff0000) == 0)
	{
		wsprintf(ach, _T("%08X (Atom)"), lpszString);
		ListView_SetItemText(hwndList, 0, 1, ach);
	}
	else
	{
		ListView_SetItemText(hwndList, 0, 1, lpszString);
	}

	return TRUE;
}

//
//	Display the window properties (SetProp API)
//
void EnumWindowProps(HWND hwnd, HWND hwndList)
{
	ListView_DeleteAllItems(hwndList);	
	EnumPropsEx(hwnd, PropEnumProcEx, (ULONG_PTR)hwndList);
}

void SetPropertyInfo(HWND hwnd)
{
	if(hwnd == 0) return;
	EnumWindowProps(hwnd, GetDlgItem(WinSpyTab[PROPERTY_TAB].hwnd, IDC_LIST1));
}

