//
//	WindowFromPointEx.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	HWND WindowFromPointEx(POINT pt)
//
//	Provides a better implementation of WindowFromPoint.
//  This function can return any window under the mouse,
//  including controls nested inside group-boxes, nested
//  dialogs etc.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

typedef struct
{
	POINT pt;
	BOOL showHidden;
	HWND best;
	ULONGLONG area;
} ChildSearch;

//
//	Callback function used with FindBestChild
//
static BOOL CALLBACK FindBestChildProc(HWND hwnd, LPARAM lParam)
{
	RECT  rect;
	ChildSearch *search = (ChildSearch *)lParam;

	if(GetWindowRect(hwnd, &rect) && PtInRect(&rect, search->pt) &&
		(search->showHidden || IsWindowVisible(hwnd)))
	{
		ULONGLONG area = (ULONGLONG)(rect.right - rect.left) * (rect.bottom - rect.top);
	
		// if this child window is smaller than the
		// current "best", then choose this one
		if(area < search->area)
		{
			search->area = area;
			search->best = hwnd;
		}
	}

	return TRUE;
}

//
//	The problem:
//
//	WindowFromPoint API is not very good. It cannot cope
//  with odd window arrangements, i.e. a group-box in a dialog
//  may contain a few check-boxes. These check-boxes are not
//  children of the groupbox, but are at the same "level" in the
//  window-hierachy. WindowFromPoint will just return the
//  first available window it finds which encompasses the mouse
//  (i.e. the group-box), but will NOT be able to detect the contents.
//
//	Solution:
//
//	We use WindowFromPoint to start us off, and then step back one
//  level (i.e. from the parent of what WindowFromPoint returned).
//
//	Once we have this window, we enumerate ALL children of this window
//  ourselves, and find the one that best fits under the mouse - 
//  the smallest window that fits, in fact.
//
//	I've tested this on alot of different apps, and it seems
//  to work flawlessly - in fact, I havn't found a situation yet
//  that this method doesn't work on.....we'll see!
//
//  Inputs:
//
//	hwndFound - window found with WindowFromPoint
//	pt        - coordinates of mouse, in screen coords
//				(i.e. same coords used with WindowFromPoint)
//
static HWND FindBestChild(HWND hwndFound, POINT pt, BOOL fShowHidden)
{
	HWND hwnd = GetParent(hwndFound);
	DWORD dwStyle;
	ChildSearch search = { pt, fShowHidden, NULL, (ULONGLONG)-1 };
	
	dwStyle = GetWindowLong(hwndFound, GWL_STYLE);

	// The original window might already be a top-level window,
	// so we don't want to start at *it's* parent
	if(hwnd == 0 || (dwStyle & WS_POPUP))
		hwnd = hwndFound;

	// Enumerate EVERY child window.
	//
	//	Note to reader:
	//
	//	You can get some real interesting effects if you set
	//  hwnd = GetDesktopWindow()
	//  fShowHidden = TRUE
	//  ...experiment!!
	//
	if(IsWindow(hwnd))
		EnumChildWindows(hwnd, FindBestChildProc, (LPARAM)&search);

	return IsWindow(search.best) ? search.best : hwnd;
}

//
//	Find window under specified point (screen coordinates)
//
HWND WindowFromPointEx(POINT pt, BOOL fShowHidden)
{
	HWND hWndPoint;
	
	//
	// First of all find the parent window under the mouse
	// We are working in SCREEN coordinates	
	// 
	hWndPoint = WindowFromPoint(pt);
	
	if(hWndPoint == 0)
		return 0;

	// WindowFromPoint is not too accurate. There is quite likely
	// another window under the mouse.
	hWndPoint = FindBestChild(hWndPoint, pt, fShowHidden);
	
	//if we don't allow hidden windows, then return the parent
	if(!fShowHidden)
	{
		while(hWndPoint && !IsWindowVisible(hWndPoint))
			hWndPoint = GetParent(hWndPoint);
	}

	return IsWindow(hWndPoint) ? hWndPoint : NULL;
}

