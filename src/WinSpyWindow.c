//
//	WinSpyWindow.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	All the window related functionality for the
//  main window (i.e. sizing, window layout etc)
//

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <tchar.h>

#include "resource.h"
#include "WinSpy.h"
#include "Utils.h"

#if (WINVER < 0x500)
#error "Please install latest Platform SDK or define WINVER >= 0x500"
#endif

//
//	Global variables, only used within this module.
//  It's just not worth putting them in a structure,
//  so I'll leave it like this for now
//
static SIZE duMinimized, szMinimized;
static SIZE duNormal,    szNormal;
static SIZE duExpanded,  szExpanded;

static SIZE szCurrent;			// current size of window!
static SIZE szLastMax;			// current NON-minimized (i.e. Normal or Expanded)
static SIZE szLastExp;			// the last expanded position

static int nLeftBorder;			// pixels between leftside + tab sheet
static int nBottomBorder;		// pixels between bottomside + tab

static BOOL fxMaxed, fyMaxed;	// Remember our sized state when a size/move starts
static UINT_PTR uHitTest;		// Keep track of if sizing or moving

//
//	These two variables help us to position WinSpy++
//  intelligently when it resizes.
//
UINT   uPinnedCorner = PINNED_TOPLEFT;	// which corner has been pinned
POINT  ptPinPos;						// coords of pinned corner

void RefreshTreeView(HWND hwndTree);

//
//	Added: Multimonitor support!!
//
typedef HMONITOR (WINAPI * MFR_PROC)(LPCRECT, DWORD);
static	MFR_PROC pMonitorFromRect = 0;

typedef BOOL (WINAPI * GMI_PROC)(HMONITOR, LPMONITORINFO);
static GMI_PROC pGetMonitorInfo = 0;

static BOOL fFindMultiMon = TRUE;

//	Get the dimensions of the work area that the specified WinRect resides in
void GetWorkArea(RECT *prcWinRect, RECT *prcWorkArea)
{
	HMONITOR	hMonitor;
	HMODULE		hUser32;
	MONITORINFO mi;

	if((hUser32 = GetModuleHandle(_T("USER32.DLL"))) == 0)
		return;
	
	// if we havn't already tried, 
	if(fFindMultiMon == TRUE)
	{
		pMonitorFromRect = (MFR_PROC)GetProcAddress(hUser32, "MonitorFromRect");
#ifdef UNICODE
		pGetMonitorInfo  = (GMI_PROC)GetProcAddress(hUser32, "GetMonitorInfoW");
#else
		pGetMonitorInfo  = (GMI_PROC)GetProcAddress(hUser32, "GetMonitorInfoA");
#endif

		fFindMultiMon = FALSE;
	}

	if(pMonitorFromRect && pGetMonitorInfo)
	{
		mi.cbSize = sizeof(mi);

		hMonitor = pMonitorFromRect(prcWinRect, MONITOR_DEFAULTTONEAREST);

		pGetMonitorInfo(hMonitor, &mi);
		CopyRect(prcWorkArea, &mi.rcWork);
	}
	else
	{
		SystemParametersInfo(SPI_GETWORKAREA, 0, prcWorkArea, FALSE);
	}
}

void ForceVisibleDisplay(HWND hwnd)
{
	RECT		rect;
	HMODULE		hUser32;

	GetWindowRect(hwnd, &rect);

	if ((hUser32 = GetModuleHandle(_T("USER32.DLL"))) == 0)
		return;
	
	pMonitorFromRect = (MFR_PROC)GetProcAddress(hUser32, "MonitorFromRect");

	if(pMonitorFromRect != 0)
	{
		if(NULL == pMonitorFromRect(&rect, MONITOR_DEFAULTTONULL))
		{
			// force window onto primary display if it is not visible
			rect.left %= GetSystemMetrics(SM_CXSCREEN);
			rect.top  %= GetSystemMetrics(SM_CYSCREEN);

			SetWindowPos(hwnd, 0, rect.left, rect.top, 0, 0, SWP_NOACTIVATE|SWP_NOZORDER|SWP_NOSIZE);
		}
	}
}

void GetPinnedPosition(HWND hwnd, POINT *pt)
{
	RECT rect;
	RECT rcDisplay;
	
	// 
	GetWindowRect(hwnd, &rect);

	// get 
//	SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDisplay, FALSE);
	GetWorkArea(&rect, &rcDisplay);

	uPinnedCorner = PINNED_NONE;

	if(rect.left + szLastExp.cx >= rcDisplay.right)
		uPinnedCorner |= PINNED_RIGHT;
	else
		uPinnedCorner |= PINNED_LEFT;

	if(rect.top + szLastExp.cy >= rcDisplay.bottom)
		uPinnedCorner |= PINNED_BOTTOM;
	else
		uPinnedCorner |= PINNED_TOP;
	
	if(fPinWindow == FALSE)
		uPinnedCorner = PINNED_TOPLEFT;

	switch(uPinnedCorner)
	{
	case PINNED_TOPLEFT:
		pt->x = rect.left;
		pt->y = rect.top;
		break;

	case PINNED_TOPRIGHT:
		pt->x = rect.right;
		pt->y = rect.top;
		break;

	case PINNED_BOTTOMRIGHT:
		pt->x = rect.right;
		pt->y = rect.bottom;
		break;

	case PINNED_BOTTOMLEFT:
		pt->x = rect.left;
		pt->y = rect.bottom;
		break;

	}

	//
	// Sanity check!!!
	//
	// If the window is in an expanded state, and it is
	// moved so that it's lower-right edge extends off the screen,
	// then when it is minimized, it will disappear (i.e. position
	// itself off-screen!). This check stops that
	//
	if(pt->x - szLastExp.cx < rcDisplay.left || pt->x >= rcDisplay.right)
	{
		pt->x = rect.left;
		uPinnedCorner &= ~PINNED_RIGHT;
	}

	if(pt->y - szLastExp.cy < rcDisplay.top || pt->y >= rcDisplay.bottom)
	{
		pt->y = rect.top;
		uPinnedCorner &= ~PINNED_BOTTOM;
	}
}

//
//	Return TRUE if the specified window is minimized to the taskbar.
//
BOOL IsMinimized(HWND hwnd)
{
	WINDOWPLACEMENT wp;

	ZeroMemory(&wp, sizeof(wp));
	wp.length = sizeof(wp);

	GetWindowPlacement(hwnd, &wp);

	return (wp.showCmd & SW_SHOWMINIMIZED) ? TRUE : FALSE;
}

//
//	hwnd       - window to calc
//	szDlgUnits - (input)  size in dialog units
//  szClient   - (output) size of client area in pixels
//	szWindow   - (output) total size of based on current settings
//
void CalcDlgWindowSize(HWND hwnd, SIZE *szDlgUnits, SIZE *szClient, SIZE *szWindow)
{
	RECT rect;
	DWORD dwStyle;
	DWORD dwStyleEx;

	// work out the size in pixels of our main window, by converting
	// from dialog units
	SetRect(&rect, 0, 0, szDlgUnits->cx, szDlgUnits->cy);
	MapDialogRect(hwnd, &rect);

	if(szClient)
	{
		szClient->cx = rect.right - rect.left;
		szClient->cy = rect.bottom - rect.top;
	}

	dwStyle   = GetWindowLong(hwnd, GWL_STYLE);
	dwStyleEx = GetWindowLong(hwnd, GWL_EXSTYLE);

	AdjustWindowRectEx(&rect, dwStyle, FALSE, dwStyleEx);

	if(szWindow)
	{
		szWindow->cx = rect.right - rect.left;
		szWindow->cy = rect.bottom - rect.top;
	}
}

//
// Position / size controls and main window based
// on current system metrics
//
void WinSpyDlg_SizeContents(HWND hwnd)
{
	int x,y,cx,cy;
	int i;
	RECT rect, rect1;
	HWND hwndTab;
	HWND hwndCtrl;

	int nPaneWidth;		// width of each dialog-pane
	int nPaneHeight;	// height of each dialog-pane
	int nActualPaneWidth; // what the tab-control is set to.

	int nTabWidth;
	int nTabHeight;

	int nDesiredTabWidth;

	// HARD-CODED sizes for each window layout.
	// These are DIALOG UNITS, so it's not too bad..
	duMinimized.cx = 254;
	duMinimized.cy = 28;//6;

	duNormal.cx    = duMinimized.cx;
	duNormal.cy    = 251;

	duExpanded.cx  = 432;//390;
	duExpanded.cy  = duNormal.cy;
	
	// work out the size (in pixels) of each window layout
	CalcDlgWindowSize(hwnd, &duMinimized, 0, &szMinimized);
	CalcDlgWindowSize(hwnd, &duNormal,    0, &szNormal);
	CalcDlgWindowSize(hwnd, &duExpanded,  0, &szExpanded);

	// resize to NORMAL layout (temporarily)
	SetWindowPos(hwnd, 0, 0, 0, szNormal.cx, szNormal.cy, SWP_SIZEONLY | SWP_NOREDRAW);

	// Locate main Property sheet control
	hwndTab = GetDlgItem(hwnd, IDC_TAB1);
	
	// Get SCREEN coords of tab control
	GetWindowRect(hwndTab, &rect);

	// Get SCREEN coords of dialog's CLIENT area
	GetClientRect(hwnd, &rect1);
	MapWindowPoints(hwnd, 0, (POINT *)&rect1, 2);

	// Now we know what the border is between TAB and left-side
	nLeftBorder   = rect.left - rect1.left;
	nBottomBorder = rect1.bottom - rect.bottom;
	
	nDesiredTabWidth = (rect1.right - rect1.left) - nLeftBorder * 2;

	//
	// Find out the size of the biggest dialog-tab-pane
	//
	SetRect(&rect, 0, 0, 0, 0);

	for(i = 0; i < NUMTABCONTROLITEMS; i++)
	{
		// Get tab-pane relative to parent (main) window
		GetClientRect(WinSpyTab[i].hwnd, &rect1);
		MapWindowPoints(WinSpyTab[i].hwnd, hwnd, (POINT *)&rect1, 2);

		// find biggest
		UnionRect(&rect, &rect, &rect1);
	}

	nPaneWidth  = rect.right - rect.left;
	nPaneHeight = rect.bottom - rect.top;

	// Resize the tab control based on this biggest rect
	SendMessage(hwndTab, TCM_ADJUSTRECT, TRUE, (LPARAM)&rect);
	
	nTabWidth  = rect.right-rect.left;
	nTabHeight = rect.bottom-rect.top;

	// Resize the tab control now we know how big it needs to be
	SetWindowPos(hwndTab, hwnd, 0,0, nDesiredTabWidth, nTabHeight, SWP_SIZEONLY);
	
	//
	// Tab control is now in place.
	// Now find out exactly where to position every
	// tab-pane. (We know how big they are, but we need
	// to find where to move them to).
	//
	GetWindowRect(hwndTab, &rect);
	ScreenToClient(hwnd, (POINT *)&rect.left);
	ScreenToClient(hwnd, (POINT *)&rect.right);

	SendMessage(hwndTab, TCM_ADJUSTRECT, FALSE, (LPARAM)&rect);
	
	x = rect.left;
	y = rect.top;
	cx = nPaneWidth;
	cy = nPaneHeight;

	nActualPaneWidth = rect.right-rect.left;
	
	// Center each dialog-tab in the tab control
	x += (nActualPaneWidth - nPaneWidth) / 2;

	// position each dialog in the right place
	for(i = 0; i < NUMTABCONTROLITEMS; i++)
	{
		SetWindowPos(WinSpyTab[i].hwnd, 	hwndTab, x, y, cx, cy, SWP_NOACTIVATE);
	}
	

	SetWindowPos(hwnd, 0, 0, 0, szMinimized.cx, szMinimized.cy, SWP_NOMOVE|SWP_NOZORDER);
	
	// Even though we are initially minimized, we want to
	// automatically expand to normal view the first time a
	// window is selected. 
	szCurrent = szMinimized;
	szLastMax = szNormal;
	szLastExp = szExpanded;

	SetWindowPos(hwndTab, //GetDlgItem(hwnd, IDC_MINIMIZE)
		HWND_BOTTOM, 0, 0, 0, 0, SWP_ZONLY);

	// Finally, move the little expand / shrink button
	// so it is right-aligned with the edge of the tab..
	hwndCtrl = GetDlgItem(hwnd, IDC_EXPAND);
	GetWindowRect(hwndCtrl, &rect);
	MapWindowPoints(0, hwnd, (POINT *)&rect, 2);

	x = nDesiredTabWidth + nLeftBorder - (rect.right-rect.left);
	y = rect.top;

	SetWindowPos(hwndCtrl, 0, x, y, 0, 0, SWP_MOVEONLY);
}

//
//	Retrieve current layout for main window
//
UINT GetWindowLayout(HWND hwnd)
{
	RECT rect;
	BOOL fxMaxed,  fyMaxed;

	GetWindowRect(hwnd, &rect);
	
	fyMaxed = GetRectHeight(&rect) > szMinimized.cy;
	fxMaxed = GetRectWidth(&rect) >= szExpanded.cx;

	if(fyMaxed == FALSE)
	{
		return WINSPY_MINIMIZED;
	}
	else
	{
		if(fxMaxed)
			return WINSPY_EXPANDED;
		else
			return WINSPY_NORMAL;
	}
}

//
//	Switch between minimized and non-minimized layouts
//
void ToggleWindowLayout(HWND hwnd)
{
	UINT layout = GetWindowLayout(hwnd);

	if(layout == WINSPY_MINIMIZED)
	{
		SetWindowLayout(hwnd, WINSPY_LASTMAX);
	}
	else
	{
		SetWindowLayout(hwnd, WINSPY_MINIMIZED);
	}
}

//
//	Switch to a specific layout.
//  Intelligently reposition the window if the new
//  layout won't fit on-screen.
//
void SetWindowLayout(HWND hwnd, UINT uLayout)
{
	DWORD dwSWPflags = SWP_NOZORDER|SWP_NOACTIVATE;

	SIZE   *psz;
	POINT  ptPos;

	// Decide which layout we are going to use
	switch(uLayout)
	{
	case WINSPY_MINIMIZED:
		psz = &szMinimized;
		break;

	case WINSPY_NORMAL:
		psz = &szNormal;
		break;

	case WINSPY_EXPANDED:
		psz = &szLastExp;
		break;

	default:
	case WINSPY_LASTMAX:
		psz = &szLastMax;
	}

	// Now work out where the top-left corner needs to
	// be, taking into account where the pinned-corner is
	switch(uPinnedCorner)
	{
	default:
	case PINNED_TOPLEFT:
		ptPos = ptPinPos;
		break;
		
	case PINNED_TOPRIGHT:
		ptPos.x = ptPinPos.x - psz->cx;
		ptPos.y = ptPinPos.y;
		break;
		
	case PINNED_BOTTOMRIGHT:
		ptPos.x = ptPinPos.x - psz->cx;
		ptPos.y = ptPinPos.y - psz->cy;
		break;
		
	case PINNED_BOTTOMLEFT:
		ptPos.x = ptPinPos.x;
		ptPos.y = ptPinPos.y - psz->cy;
		break;
		
	}

	// Switch into the new layout!
	SetWindowPos(hwnd, 0, ptPos.x, ptPos.y, psz->cx, psz->cy, dwSWPflags);
}


UINT WinSpyDlg_Size(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	int  cx, cy;
	HWND hwndCtrl;
	RECT rect;
	RECT rect2;

	if(wParam == SIZE_RESTORED || wParam == SIZE_MAXIMIZED)
	{
		cx = LOWORD(lParam);
		cy = HIWORD(lParam);

		// Resize the right-hand tab control so that
		// it fills the window
		hwndCtrl = GetDlgItem(hwnd, IDC_TAB2);
		GetWindowRect(hwndCtrl, &rect);
		ScreenToClient(hwnd, (POINT *)&rect.left);
		ScreenToClient(hwnd, (POINT *)&rect.right);

		MoveWindow(hwndCtrl, rect.left, rect.top, cx - rect.left - nLeftBorder, cy - rect.top - nBottomBorder, TRUE);

		GetWindowRect(hwndCtrl, &rect);
		ScreenToClient(hwnd, (POINT *)&rect.left);
		ScreenToClient(hwnd, (POINT *)&rect.right);
		rect.top++;

		// Work out the coords of the tab contents
		SendMessage(hwndCtrl, TCM_ADJUSTRECT, FALSE, (LPARAM)&rect);

		// Resize the tree control so that it fills the tab control.
		hwndCtrl = GetDlgItem(hwnd, IDC_TREE1);
		InflateRect(&rect, 1,1);
		MoveWindow(hwndCtrl, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, TRUE);

		// Position the size-grip
		{
			int width  = GetSystemMetrics(SM_CXVSCROLL);
			int height = GetSystemMetrics(SM_CYHSCROLL);

			GetClientRect(hwnd, &rect);

			MoveWindow(hwndSizer, rect.right-width,	rect.bottom-height,width,height,TRUE);

		}

		GetWindowRect(hwndPin, &rect2);
		OffsetRect(&rect2, -rect2.left, -rect2.top);

		// Position the pin toolbar
		//SetWindowPos(hwndPin,
		//	HWND_TOP, rect.right-rect2.right, 1, rect2.right, rect2.bottom, 0);
		MoveWindow(hwndPin, rect.right - rect2.right, 1, rect2.right, rect2.bottom, TRUE);
	}
	
	return 0;
}


//
//	Make sure that only the controls that
//  are visible through the current layout are actually enabled.
//  This prevents the user from Tabbing to controls that
//  are not visible
//
typedef struct
{
	UINT uCtrlId;
	BOOL fEnabled;
} CtrlEnable;

void EnableLayoutCtrls(HWND hwnd, UINT layout)
{
	int i;
	const int nNumCtrls = 9;

	CtrlEnable ctrl0[] = 
	{ 
		IDC_TAB1,		FALSE, 
		IDOK,			FALSE, 
		IDC_CAPTURE,	FALSE, 
		IDC_EXPAND,		FALSE,
		IDC_TAB2,		FALSE,
		IDC_TREE1,		FALSE,
		IDC_REFRESH,	FALSE,
		IDC_LOCATE,		FALSE,
		IDC_FLASH,		FALSE,
	};

	CtrlEnable ctrl1[] = 
	{ 
		IDC_TAB1,		TRUE, 
		IDOK,			TRUE, 
		IDC_CAPTURE,	TRUE, 
		IDC_EXPAND,		TRUE,
		IDC_TAB2,		FALSE,
		IDC_TREE1,		FALSE,
		IDC_REFRESH,	FALSE,
		IDC_LOCATE,		FALSE,
		IDC_FLASH,		FALSE,
	};

	CtrlEnable ctrl2[] = 
	{ 
		IDC_TAB1,		TRUE, 
		IDOK,			TRUE, 
		IDC_CAPTURE,	TRUE, 
		IDC_EXPAND,		TRUE,
		IDC_TAB2,		TRUE,
		IDC_TREE1,		TRUE,
		IDC_REFRESH,	TRUE,
		IDC_LOCATE,		TRUE,
		IDC_FLASH,		TRUE,
	};

	switch(layout)
	{
	case WINSPY_MINIMIZED:
		
		for(i = 0; i < NUMTABCONTROLITEMS; i++)
			EnableWindow(WinSpyTab[i].hwnd, FALSE);
		
		for(i = 0; i < nNumCtrls; i++)
			EnableDlgItem(hwnd, ctrl0[i].uCtrlId, ctrl0[i].fEnabled);

		break;
		
	case WINSPY_NORMAL:
		
		for(i = 0; i < NUMTABCONTROLITEMS; i++)
			EnableWindow(WinSpyTab[i].hwnd, TRUE);
		
		for(i = 0; i < nNumCtrls; i++)
			EnableDlgItem(hwnd, ctrl1[i].uCtrlId, ctrl1[i].fEnabled);
		
		break;
		
	case WINSPY_EXPANDED:
		
		for(i = 0; i < NUMTABCONTROLITEMS; i++)
			EnableWindow(WinSpyTab[i].hwnd, TRUE);
		
		for(i = 0; i < nNumCtrls; i++)
			EnableDlgItem(hwnd, ctrl2[i].uCtrlId, ctrl2[i].fEnabled);
		
		break;
		
	}

}

UINT WinSpyDlg_WindowPosChanged(HWND hwnd, WINDOWPOS *wp)
{
	UINT layout;
	HICON hIcon, hOld;
		
	static UINT oldlayout = -1;

	if(wp == 0)
		return 0;

	layout = GetWindowLayout(hwnd);

	// Detect if our size has changed
	if(!(wp->flags & SWP_NOSIZE))
	{
		if(layout == WINSPY_EXPANDED)
		{
			szLastExp.cx = wp->cx;
			szLastExp.cy = wp->cy;
		}

		szCurrent.cx = wp->cx;
		szCurrent.cy = wp->cy;

		if(layout != WINSPY_MINIMIZED)
		{
			szLastMax = szCurrent;
		}

		// Has the layout changed as a result?
		if(oldlayout != layout)
		{
			HWND  hwndExpand = GetDlgItem(hwnd, IDC_EXPAND);
			DWORD dwStyle = GetWindowLong(hwndExpand, GWL_STYLE);

			if(layout == WINSPY_NORMAL)
			{
				hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON16), IMAGE_ICON, 16, 16, 0);
				hOld = (HICON)SendDlgItemMessage(hwnd, IDC_EXPAND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

				DestroyIcon(hOld);

				SetWindowLong(hwndExpand, GWL_STYLE, dwStyle | BS_RIGHT);
				SetWindowText(hwndExpand, _T("&More"));
			}
			else
			{
				hIcon = LoadImage(hInst, MAKEINTRESOURCE(IDI_ICON18), IMAGE_ICON, 16, 16, 0);
				hOld = (HICON)SendDlgItemMessage(hwnd, IDC_EXPAND, BM_SETIMAGE, IMAGE_ICON, (LPARAM)hIcon);

				DestroyIcon(hOld);

				SetWindowLong(hwndExpand, GWL_STYLE, dwStyle & ~BS_RIGHT);
				SetWindowText(hwndExpand, _T("L&ess"));
			}
			
			SetSysMenuIconFromLayout(hwnd, layout);

			EnableLayoutCtrls(hwnd, layout);
		}
		
		oldlayout = layout;
	}
	
	// Has our Z-order changed?
	if(wp && !(wp->flags & SWP_NOZORDER))
	{
		DWORD dwStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	
		// Set the global flag (just so we can remember it in the registry)
		if(dwStyle & WS_EX_TOPMOST)
			fAlwaysOnTop = TRUE;
		else
			fAlwaysOnTop = FALSE;

		CheckSysMenu(hwnd, IDM_WINSPY_ONTOP, fAlwaysOnTop);
	}
	
	
	return 0;
}


// monitor the sizing rectangle so that the main window
// "snaps" to each of the 3 layouts

UINT WinSpyDlg_Sizing(HWND hwnd, UINT nSide, RECT *prc)
{
	int minx ;
	int miny;
	int maxy;
	int nWidthNew;
	int nHeightNew;
	
	minx = szMinimized.cx;
	miny = szMinimized.cy;
	maxy = szNormal.cy;
	
	nWidthNew   = prc->right - prc->left;
	nHeightNew  = prc->bottom - prc->top;
	
	if(fxMaxed == FALSE)
	{
		if(nWidthNew <= minx)
			nWidthNew = minx;
		
		if(nWidthNew > minx && nWidthNew < szExpanded.cx)
			nWidthNew = szExpanded.cx;
	}
	else
	{
		if(nWidthNew < szExpanded.cx)
			nWidthNew = minx;
		
	}
	
	if(fyMaxed == FALSE)
	{
		if(nHeightNew > miny)
		{
			nHeightNew = maxy;
		}
		
		if(nHeightNew <= miny)
		{
			nHeightNew = miny;
			nWidthNew  = minx;
		}
	}
	else
	{
		if(nHeightNew < maxy)
		{
			nHeightNew = miny;
			nWidthNew  = minx;
		}
		else
			nHeightNew = maxy;
	}
	
	// Adjust the rectangle's dimensions
	switch(nSide) 
	{
	case WMSZ_LEFT:    
		prc->left   = prc->right  - nWidthNew;  
		break;
		
	case WMSZ_TOP:     
		prc->top    = prc->bottom - nHeightNew; 
		//>
		prc->right  = prc->left   + nWidthNew;
		break;
		
	case WMSZ_RIGHT:   
		prc->right  = prc->left   + nWidthNew;  
		break;
		
	case WMSZ_BOTTOM:  
		prc->bottom = prc->top    + nHeightNew; 
		//>
		prc->right  = prc->left   + nWidthNew;
		break;
		
	case WMSZ_BOTTOMLEFT:
		prc->bottom = prc->top    + nHeightNew; 
		prc->left   = prc->right  - nWidthNew;  
		break;
		
	case WMSZ_BOTTOMRIGHT:
		prc->bottom = prc->top    + nHeightNew; 
		prc->right  = prc->left   + nWidthNew;  
		break;
		
	case WMSZ_TOPLEFT:
		prc->left   = prc->right  - nWidthNew;
		prc->top    = prc->bottom - nHeightNew;
		break;
		
	case WMSZ_TOPRIGHT:
		prc->top    = prc->bottom - nHeightNew;
		prc->right  = prc->left   + nWidthNew;  
		break;
	}
	
	return TRUE;
}

UINT WinSpyDlg_EnterSizeMove(HWND hwnd)
{
	RECT rect;
	GetWindowRect(hwnd, &rect);

	fyMaxed = (rect.bottom - rect.top > szMinimized.cy);
	fxMaxed = (rect.right-rect.left >= szExpanded.cx);

	return 0;
}

UINT WinSpyDlg_ExitSizeMove(HWND hwnd)
{
	RECT rect;
	UINT uLayout;
	
	static UINT uOldLayout = WINSPY_MINIMIZED;
	

	GetWindowRect(hwnd, &rect);
	
	szCurrent.cx = rect.right - rect.left;
	szCurrent.cy = rect.bottom - rect.top;
	
	fyMaxed = (szCurrent.cy > szMinimized.cy);
	fxMaxed = (szCurrent.cx >= szExpanded.cx);
	
	if(fyMaxed == FALSE)
	{
		uLayout = WINSPY_MINIMIZED;
	}
	else
	{
		if(fxMaxed)
			uLayout = WINSPY_EXPANDED;
		else
			uLayout = WINSPY_NORMAL;
		
		szLastMax = szCurrent;
	}
	
	SetSysMenuIconFromLayout(hwnd, uLayout);
	
	if(uLayout == WINSPY_EXPANDED && uOldLayout != WINSPY_EXPANDED)
		RefreshTreeView(GetDlgItem(hwnd, IDC_TREE1));
	
	// If the window was moved (ie. dragged by caption/client),
	// Then update our pinned corner position
	if(uHitTest == HTCAPTION)
		GetPinnedPosition(hwnd, &ptPinPos);

	uOldLayout = uLayout;

	return 0;
}

UINT_PTR WinSpyDlg_FullWindowDrag(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	uHitTest = DefWindowProc(hwnd, WM_NCHITTEST, wParam, lParam);
		
	// Allow full-window dragging
	if(fFullDragging &&	uHitTest == HTCLIENT) 
		uHitTest = HTCAPTION;

	SetWindowLongPtr(hwnd, DWLP_MSGRESULT, uHitTest);
	return uHitTest;
}

#define X_ZOOM_BORDER 8
#define Y_ZOOM_BORDER 8

BOOL WinSpy_ZoomTo(HWND hwnd, UINT uCorner)
{
	RECT rcDisplay;
	RECT rect;

	//SystemParametersInfo(SPI_GETWORKAREA, 0, &rcDisplay, FALSE);
	GetWindowRect(hwnd, &rect);
	GetWorkArea(&rect, &rcDisplay);

	switch(uCorner)
	{
	case PINNED_TOPLEFT:
		ptPinPos.x = rcDisplay.left + X_ZOOM_BORDER;
		ptPinPos.y = rcDisplay.top  + Y_ZOOM_BORDER;
		break;

	case PINNED_TOPRIGHT:
		ptPinPos.x = rcDisplay.right - X_ZOOM_BORDER;
		ptPinPos.y = rcDisplay.top   + Y_ZOOM_BORDER;
		break;

	case PINNED_BOTTOMRIGHT:
		ptPinPos.x = rcDisplay.right  - X_ZOOM_BORDER;
		ptPinPos.y = rcDisplay.bottom - Y_ZOOM_BORDER;
		break;

	case PINNED_BOTTOMLEFT:
		ptPinPos.x = rcDisplay.left   + X_ZOOM_BORDER;
		ptPinPos.y = rcDisplay.bottom - Y_ZOOM_BORDER;
		break;

	default:
		return FALSE;
	}

	SetPinState(TRUE);

	uPinnedCorner = uCorner;
	SetWindowLayout(hwnd, WINSPY_MINIMIZED);

	return TRUE;
}