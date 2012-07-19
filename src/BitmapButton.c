//
//	BitmapButton.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//	void MakeBitmapButton(HWND hwnd, UINT uIconId)
//
//  Converts the specified button into an owner-drawn button
//	(supports a small icon + text to the right)
//
//	 hwnd    - handle to button
//   uIconId - icon resource ID (loaded from THIS module)
//
//
//  BOOL DrawBitmapButton(DRAWITEMSTRUCT *dis)
//
//	You must call this when the parent (dialog?) window receives a
//  WM_DRAWITEM for the button.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN
#define _WIN32_WINNT 0x0501

#include <windows.h>
#include <tchar.h>
#include <uxtheme.h>
#include <tmschema.h>
#include "BitmapButton.h"

#pragma comment(lib,    "Uxtheme.lib")
#pragma comment(lib,    "Delayimp.lib")
//#pragma comment(linker, "/delayload:Uxtheme.dll")

BOOL	g_fThemeApiAvailable = FALSE;

HTHEME _OpenThemeData(HWND hwnd, LPCWSTR pszClassList)
{
	if(g_fThemeApiAvailable)
		return OpenThemeData(hwnd, pszClassList);
	else
		return NULL;
}

HRESULT _CloseThemeData(HTHEME hTheme)
{
	if(g_fThemeApiAvailable)
		return CloseThemeData(hTheme);
	else
		return E_FAIL;
}

#ifndef ODS_NOFOCUSRECT
#define ODS_NOFOCUSRECT     0x0200
#endif

#ifndef DT_HIDEPREFIX
#define DT_HIDEPREFIX		0x100000
#endif

#define X_ICON_BORDER	3	// 3 pixels

//
//	Subclass procedure for an owner-drawn button.
//  All this does is to re-enable double-click behaviour for
//  an owner-drawn button.
//
static LRESULT CALLBACK BBProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	WNDPROC oldproc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	TRACKMOUSEEVENT tme = { sizeof(tme) };

	static BOOL mouseOver;
	POINT pt;
	RECT  rect;
	
	switch(msg)
	{
	case WM_LBUTTONDBLCLK:
		msg = WM_LBUTTONDOWN;
		break;

	case WM_MOUSEMOVE:
		
		if(!mouseOver)
		{
			SetTimer(hwnd, 0, 15, 0);
			mouseOver = FALSE;
		}
		break;

	case WM_TIMER:

		GetCursorPos(&pt);
		ScreenToClient(hwnd, &pt);
		GetClientRect(hwnd, &rect);
		
		if(PtInRect(&rect, pt))
		{
			if(!mouseOver)
			{
				mouseOver = TRUE;
				InvalidateRect(hwnd, 0, 0);
			}
		}
		else
		{
			mouseOver = FALSE;
			KillTimer(hwnd, 0);
			InvalidateRect(hwnd, 0, 0);
		}
	
		return 0;

	// Under Win2000 / XP, Windows sends a strange message
	// to dialog controls, whenever the ALT key is pressed
	// for the first time (i.e. to show focus rect / & prefixes etc).
	// msg = 0x0128, wParam = 0x00030003, lParam = 0
	case 0x0128:
		InvalidateRect(hwnd, 0, 0);
		break;
	}

	return CallWindowProc(oldproc, hwnd, msg, wParam, lParam); 
}

//BOOL DrawThemedBitmapButton(DRAWITEMSTRUCT *dis)
/*BOOL DrawBitmapButton0(DRAWITEMSTRUCT *dis)
{
	//HTHEME hTheme = GetWindowTheme(dis->hwndItem, "Button");
	HTHEME hTheme = _OpenThemeData(dis->hwndItem, L"Button");
	DWORD state;

	if(dis->itemState & ODA_FOCUS)
		;

	if(dis->itemState & ODS_SELECTED)
		state = PBS_PRESSED;
	else if(dis->itemState & ODS_HOTLIGHT)
		state = PBS_HOT;
	else
		state = PBS_NORMAL;

	DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, state, &dis->rcItem, 0);

	_CloseThemeData(hTheme);

	return TRUE;
}*/

//
//	Call this function whenever you get a WM_DRAWITEM in the parent dialog..
//
BOOL DrawBitmapButton(DRAWITEMSTRUCT *dis)
{
	RECT rect;			// Drawing rectangle
	POINT pt;

	int ix, iy;			// Icon offset
	int bx, by;			// border sizes
	int sxIcon, syIcon;	// Icon size
	int xoff, yoff;		// 
	
	TCHAR szText[100];
	int   nTextLen;
	
	HICON hIcon;
	DWORD dwStyle = GetWindowLong(dis->hwndItem, GWL_STYLE);

	DWORD dwDTflags = DT_CENTER | DT_SINGLELINE | DT_VCENTER;
	BOOL  fRightAlign;
	
	// XP/Vista theme support
	DWORD dwThemeFlags;
	HTHEME hTheme;
	BOOL   fDrawThemed = g_fThemeApiAvailable;

	if(dis->itemState & ODS_NOFOCUSRECT)
		dwDTflags |= DT_HIDEPREFIX;

	fRightAlign = (dwStyle & BS_RIGHT) ? TRUE : FALSE;

	// do the theme thing
	hTheme = _OpenThemeData(dis->hwndItem, L"Button");

	switch(dis->itemAction)
	{
	// We need to redraw the whole button, no
	// matter what DRAWITEM event we receive..
	case ODA_FOCUS:
	case ODA_SELECT:
	case ODA_DRAWENTIRE:
	
		// Retrieve button text
		GetWindowText(dis->hwndItem, szText, sizeof(szText) / sizeof(TCHAR));

		nTextLen = lstrlen(szText);

		// Retrieve button icon
		hIcon = (HICON)SendMessage(dis->hwndItem, BM_GETIMAGE, IMAGE_ICON, 0);

		// Find icon dimensions
		sxIcon = 16;
		syIcon = 16;

		CopyRect(&rect, &dis->rcItem);
		GetCursorPos(&pt);
		ScreenToClient(dis->hwndItem, &pt);

		if(PtInRect(&rect, pt))
			dis->itemState |= ODS_HOTLIGHT;
	
		// border dimensions
		bx = 2;
		by = 2;
		
		// icon offsets
		if(nTextLen == 0)
		{
			// center the image if no text
			ix = (rect.right - rect.left - sxIcon) / 2;
		}
		else
		{
			if(fRightAlign)
				ix = rect.right - bx - X_ICON_BORDER - sxIcon;
			else
				ix = rect.left + bx + X_ICON_BORDER;
		}
	
		// center image vertically
		iy = (rect.bottom-rect.top - syIcon) / 2;

		InflateRect(&rect, -5, -5);

		// Draw a single-line black border around the button
		if(hTheme == NULL && (dis->itemState & (ODS_FOCUS | ODS_DEFAULT)))
		{
			FrameRect(dis->hDC, &dis->rcItem, GetStockObject(BLACK_BRUSH));
			InflateRect(&dis->rcItem, -1, -1);
		}

		if(dis->itemState & ODS_FOCUS)
			dwThemeFlags = PBS_DEFAULTED;
		if(dis->itemState & ODS_DISABLED)
			dwThemeFlags = PBS_DISABLED;
		else if(dis->itemState & ODS_SELECTED)
			dwThemeFlags = PBS_PRESSED;
		else if(dis->itemState & ODS_HOTLIGHT)
			dwThemeFlags = PBS_HOT;
		else if(dis->itemState & ODS_DEFAULT)
			dwThemeFlags = PBS_DEFAULTED;
		else
			dwThemeFlags = PBS_NORMAL;

		// Button is DOWN
		if(dis->itemState & ODS_SELECTED)
		{
			// Draw a button
			if(hTheme != NULL)
				DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, dwThemeFlags, &dis->rcItem, 0);
			else
				DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH | DFCS_PUSHED | DFCS_FLAT);
			
			
			// Offset contents to make it look "pressed"
			if(hTheme == NULL)
			{
				OffsetRect(&rect, 1, 1);
				xoff = yoff = 1;
			}
			else
				xoff = yoff = 0;
		}
		// Button is UP
		else
		{
			//
			if(hTheme != NULL)
				DrawThemeBackground(hTheme, dis->hDC, BP_PUSHBUTTON, dwThemeFlags, &dis->rcItem, 0);
			else
				DrawFrameControl(dis->hDC, &dis->rcItem, DFC_BUTTON, DFCS_BUTTONPUSH);

			xoff = yoff = 0;
		}

		// Draw the icon
		DrawIconEx(dis->hDC, ix + xoff, iy + yoff, hIcon, sxIcon, syIcon, 0, 0, DI_NORMAL);

		// Adjust position of window text 
		if(fRightAlign)
		{
			rect.left  += bx + X_ICON_BORDER;
			rect.right -= sxIcon + bx + X_ICON_BORDER;
		}
		else
		{
			rect.right -= bx + X_ICON_BORDER;
			rect.left  += sxIcon + bx + X_ICON_BORDER;
		}
		
		// Draw the text
		OffsetRect(&rect, 0, -1);
		SetBkMode(dis->hDC, TRANSPARENT);
		DrawText(dis->hDC, szText, -1, &rect, dwDTflags);
		OffsetRect(&rect, 0, 1);

		// Draw the focus rectangle (only if text present)
		if((dis->itemState & ODS_FOCUS) && nTextLen > 0)
		{
			if(!(dis->itemState & ODS_NOFOCUSRECT))
			{	
				// Get a "fresh" copy of the button rectangle
				CopyRect(&rect, &dis->rcItem);
				
				if(fRightAlign)
					rect.right -= sxIcon + bx;
				else
					rect.left += sxIcon + bx + 2;

				InflateRect(&rect, -3, -3);
				DrawFocusRect(dis->hDC, &rect);
			}
		}

		break;
	}

	_CloseThemeData(hTheme);
	return TRUE;
}

//
//	Convert the specified button into an owner-drawn button.
//  The button does NOT need owner-draw or icon styles set
//  in the resource editor - this function sets these
//  styles automatically
//
void MakeBitmapButton(HWND hwnd, UINT uIconId)
{
	WNDPROC oldproc;
	DWORD   dwStyle;

	HICON hIcon = (HICON)LoadImage(GetModuleHandle(0),
		MAKEINTRESOURCE(uIconId), IMAGE_ICON, 16, 16, 0);

	// Add on BS_ICON and BS_OWNERDRAW styles
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	SetWindowLong(hwnd, GWL_STYLE, dwStyle | BS_ICON | BS_OWNERDRAW);

	// Assign icon to the button
	SendMessage(hwnd, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

	// Subclass (to reenable double-clicks)
	oldproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)BBProc);

	// Store old procedure
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)oldproc);

	if(g_fThemeApiAvailable)
		SetWindowTheme(hwnd, L"explorer", NULL);
}

//
//	Just a helper function really
//
void MakeDlgBitmapButton(HWND hwndDlg, UINT uCtrlId, UINT uIconId)
{
	if(GetModuleHandle(_T("uxtheme.dll")))
		g_fThemeApiAvailable = TRUE;
	else
		g_fThemeApiAvailable = FALSE;

	MakeBitmapButton(GetDlgItem(hwndDlg, uCtrlId), uIconId);
}
