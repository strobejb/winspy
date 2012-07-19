//
//	TabCtrlUtils.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Basic support to remove the flicker from a TAB control
//  when it gets resized. This is achieved by subclassing the
//  tab control and handling the WM_ERASEBKGND message, so
//  that only the necessary parts of the window are actually
//  painted. Needs updating if you use in an XP-themed APP.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>

//remove flicker from tab control when it is resized
static LRESULT CALLBACK NoFlickerTabProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	RECT rect;
	HDC hdc;
	int n;
	int width;

	//int bx, by;

	WNDPROC OldTabProc = (WNDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);

	switch(msg)
	{
	case WM_NCDESTROY:
		//do any de-init here..
		break;

	case WM_ERASEBKGND:
		hdc = (HDC)wParam;

		GetWindowRect(hwnd, &rect);
		OffsetRect(&rect, -rect.left, -rect.top);
		rect.top++;
		width = rect.right;

		//find work area of tab control
		TabCtrl_AdjustRect(hwnd, FALSE, (LPARAM)&rect);

		//bx = GetSystemMetrics(SM_CXEDGE);
		//by = GetSystemMetrics(SM_CYEDGE);

		//only redraw the area in-between the work area and
		//the 3d-look border around the edge.
		InflateRect(&rect, 1, 1);
		FrameRect(hdc, &rect, GetSysColorBrush(COLOR_BTNFACE));
		
		InflateRect(&rect, 1, 1);
		FrameRect(hdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

		// Get coords of last TAB. 
		n = TabCtrl_GetItemCount(hwnd);
		TabCtrl_GetItemRect(hwnd, n - 1, &rect);

		// Now fill the long horz rectangle to the right of the tab..
		rect.left  = rect.right + 2;
		rect.right = width;
		rect.top   = 0;
		FillRect(hdc, &rect, GetSysColorBrush(COLOR_BTNFACE));

		//prevent erasure of window
		return 1;
	}

	return CallWindowProc(OldTabProc, hwnd, msg, wParam, lParam);
}

BOOL RemoveTabCtrlFlicker(HWND hwndTab)
{
	//Subclass the tab control
	WNDPROC oldproc = (WNDPROC)SetWindowLongPtr(hwndTab, GWLP_WNDPROC, (LONG_PTR)NoFlickerTabProc);
	
	//Store the old window procedure
	SetWindowLongPtr(hwndTab, GWLP_USERDATA, (LONG_PTR)oldproc);
	
	return TRUE;
}