//
//	WinSpy Finder Tool.
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	This is a standalone file which implements
//	a "Finder Tool" similar to that used in Spy++
//
//	There are two functions you must use:
//
//	1. BOOL MakeFinderTool(HWND hwnd, WNDFINDPROC wfp)
//
//     hwnd  - handle to a STATIC control to base the tool around.
//             MakeFinderTool converts this control to the correct
//             style, adds the bitmaps and mouse support etc.
//
//     wfn   - Event callback function. Must not be zero.
//
//     Return values: 
//             TRUE for success, FALSE for failure
//
//
//  2. UINT CALLBACK WndFindProc(HWND hwndTool, UINT uCode, HWND hwnd)
//
//     This is a callback function that you supply when using 
//     MakeFinderTool. This callback can be executed for a number
//     different events - described by uCode.
//
//     hwndTool - handle to the finder tool
//
//     hwnd  - handle to the window which has been found.
//    
//     uCode - describes the event. Can be one of the following values.
//           
//             WFN_BEGIN        : tool is about to become active.
//             WFN_SELCHANGING  : sent when tool moves from window-window.
//             WFN_SELCHANGED   : sent when final window is selected.
//             WFN_CANCELLED    : Tool cancelled. hwnd is not valid (0)
//      
//     Return values:
//             Return value is only checked for WFN_BEGIN. Return 0 (zero)
//             to continue, -1 to prevent tool from being used. Otherwise,
//             return 0 (zero) for all other messages
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <uxtheme.h>
#include "FindTool.h"
#include "resource.h"
#include "WinSpy.h"
#include "Utils.h"

#pragma comment(lib, "Msimg32.lib")
#pragma comment(lib, "Uxtheme.lib")

HBITMAP LoadPNGImage(UINT id, void **bits);

#define INVERT_BORDER 3

// Property names used to stash a per-control, DPI-scaled copy of the
// finder-tool bitmaps on the static control itself, plus which of the
// two (possibly scaled) bitmaps is currently on display.
#define PROP_DRAG1_SCALED   _T("WinSpyDrag1Scaled")
#define PROP_DRAG2_SCALED   _T("WinSpyDrag2Scaled")
#define PROP_CURRENT_BITMAP _T("WinSpyDragCurrent")

HWND WindowFromPointEx(POINT pt, BOOL fShowHidden);
void CaptureWindow(HWND hwndParent, HWND hwnd);

HWND ShowTransWindow(HWND);
void ShowSel(HWND);
void HideSel(HWND);

static BOOL fTransSel = TRUE;
static HWND hwndTransPanel = 0;

static LONG    lRefCount = 0;

static HCURSOR hOldCursor;
static HHOOK   draghook = 0;
static HWND    draghookhwnd = 0;

//
// The finder-tool bitmaps are baked at all 7 standard Windows scaling
// presets (100/125/150/175/200/225/250% of the 96dpi base) - almost every
// real monitor's DPI is set to one of these, so PickAndScaleFinderVariant
// below finds an exact match and no runtime GDI stretch is needed at all.
// Only a genuinely custom/non-standard DPI falls back to scaling from the
// nearest baked size.
//
#define NUM_FINDER_VARIANTS 7

static const int  g_finderVariantScales[NUM_FINDER_VARIANTS] = { 100, 125, 150, 175, 200, 225, 250 };
static const UINT g_finderVariantIds1[NUM_FINDER_VARIANTS] =
	{ IDB_DRAGTOOL1_100, IDB_DRAGTOOL1_125, IDB_DRAGTOOL1_150, IDB_DRAGTOOL1_175, IDB_DRAGTOOL1_200, IDB_DRAGTOOL1_225, IDB_DRAGTOOL1_250 };
static const UINT g_finderVariantIds2[NUM_FINDER_VARIANTS] =
	{ IDB_DRAGTOOL2_100, IDB_DRAGTOOL2_125, IDB_DRAGTOOL2_150, IDB_DRAGTOOL2_175, IDB_DRAGTOOL2_200, IDB_DRAGTOOL2_225, IDB_DRAGTOOL2_250 };

static HBITMAP hBitmapDrag1[NUM_FINDER_VARIANTS];
static HBITMAP hBitmapDrag2[NUM_FINDER_VARIANTS];
static HCURSOR hCursor;

//is the finder-tool being dragged??
static BOOL fDragging = FALSE;

// Old window procedure...?
static WNDPROC oldstaticproc;


static HWND hwndCurrent;

//
//	Invert the specified window's border
//
void InvertWindow(HWND hwnd, BOOL fShowHidden)
{
	RECT rect;
	RECT rect2;
	RECT rectc;
	HDC hdc;
	int x1,y1;

	int border = INVERT_BORDER;
	
	if(hwnd == 0)
		return;

	//window rectangle (screen coords)
	GetWindowRect(hwnd, &rect);
	
	//client rectangle (screen coords)
	GetClientRect(hwnd, &rectc);
	ClientToScreen(hwnd, (POINT *)&rectc.left);
	ClientToScreen(hwnd, (POINT *)&rectc.right);
	//MapWindowPoints(hwnd, 0, (POINT *)&rectc, 2);

	x1 = rect.left;
	y1 = rect.top;
	OffsetRect(&rect, -x1, -y1);
	OffsetRect(&rectc, -x1, -y1);
				
	if(rect.bottom - border * 2 < 0)
		border = 1;

	if(rect.right - border * 2 < 0)
		border = 1;

	if(fShowHidden == TRUE)
		hwnd = 0;

	hdc = GetWindowDC(hwnd);
	
	if(hdc == 0)
		return;

	//top edge
	//border = rectc.top-rect.top;
	SetRect(&rect2, 0,0,rect.right, border);
	if(fShowHidden == TRUE) OffsetRect(&rect2, x1, y1);
	InvertRect(hdc, &rect2);
	
	//left edge
	//border = rectc.left-rect.left;
	SetRect(&rect2, 0,border,border, rect.bottom);
	if(fShowHidden == TRUE) OffsetRect(&rect2, x1, y1);
	InvertRect(hdc, &rect2);

	//right edge
	//border = rect.right-rectc.right;
	SetRect(&rect2, border,rect.bottom-border,rect.right, rect.bottom);
	if(fShowHidden == TRUE) OffsetRect(&rect2, x1, y1);
	InvertRect(hdc, &rect2);
	
	//bottom edge
	//border = rect.bottom-rectc.bottom;
	SetRect(&rect2, rect.right-border, border,rect.right, rect.bottom-border);
	if(fShowHidden == TRUE) OffsetRect(&rect2, x1, y1);
	InvertRect(hdc, &rect2);


	ReleaseDC(hwnd, hdc);
}

void FlashWindowBorder(HWND hwnd, BOOL fShowHidden)
{
	int i;

	for(i = 0; i < 3 * 2; i++)
	{
		InvertWindow(hwnd, fShowHidden);
		Sleep(100);
	}
}

// Whether uxtheme.dll is actually loaded, so DrawThemeParentBackground can
// be called safely (it's delay-loaded - calling it with no uxtheme.dll
// present would raise a delay-load SEH exception). Same check BitmapButton.c
// uses for its own theme calls.
static BOOL fThemeApiAvailable;

void LoadFinderResources()
{
	int i;
	void *pvBits;

	for(i = 0; i < NUM_FINDER_VARIANTS; i++)
	{
		hBitmapDrag1[i] = LoadPNGImage(g_finderVariantIds1[i], &pvBits);
		hBitmapDrag2[i] = LoadPNGImage(g_finderVariantIds2[i], &pvBits);
	}

	hCursor = LoadCursor(GetModuleHandle(0), MAKEINTRESOURCE(IDC_CURSOR1));

	fThemeApiAvailable = (GetModuleHandle(_T("uxtheme.dll")) != NULL);
}

//
// Pick the baked-in size variant whose nominal scale is closest to the
// control's actual DPI, then finish the job with a (usually small) GDI
// resize (CreateDpiScaledAlphaBitmap, in Utils.c) - much crisper than
// always stretching from the 100% master. *pfOwned reports whether the
// result is a fresh bitmap the caller must free, as opposed to one of
// the permanent baked bitmaps in the variants[] array.
//
static HBITMAP PickAndScaleFinderVariant(HBITMAP *variants, int dpi, BOOL *pfOwned)
{
	int targetScale = MulDiv(dpi, 100, USER_DEFAULT_SCREEN_DPI);
	int i, best = 0, bestDelta = targetScale - g_finderVariantScales[0];
	HBITMAP hbmBase, hbmScaled;

	if(bestDelta < 0) bestDelta = -bestDelta;

	for(i = 1; i < NUM_FINDER_VARIANTS; i++)
	{
		int delta = targetScale - g_finderVariantScales[i];
		if(delta < 0) delta = -delta;

		if(delta < bestDelta)
		{
			bestDelta = delta;
			best = i;
		}
	}

	hbmBase   = variants[best];
	hbmScaled = CreateDpiScaledAlphaBitmap(hbmBase, MulDiv(dpi, 100, g_finderVariantScales[best]));

	*pfOwned = (hbmScaled != hbmBase);
	return hbmScaled;
}

//
// Set the finder-tool bitmap on a control, picking and scaling the best
// variant for the control's current monitor DPI, and caching the result
// as a window property so it isn't recreated on every drag/drop. The
// control paints itself (see StaticProc's WM_PAINT) - this just records
// which bitmap is now current and invalidates it.
//
static void SetFinderBitmap(HWND hwnd, LPCTSTR propName, HBITMAP *variants)
{
	HBITMAP hbmScaled = (HBITMAP)GetProp(hwnd, propName);

	if(hbmScaled == NULL)
	{
		BOOL fOwned;

		hbmScaled = PickAndScaleFinderVariant(variants, GetWindowDpi(hwnd), &fOwned);

		// Only cache (and later free) a genuinely new bitmap - a scaled
		// copy the caller must own, as opposed to one of the permanent
		// baked bitmaps owned by LoadFinderResources/FreeFinderResources.
		if(fOwned)
			SetProp(hwnd, propName, (HANDLE)hbmScaled);
	}

	SetProp(hwnd, PROP_CURRENT_BITMAP, (HANDLE)hbmScaled);
	InvalidateRect(hwnd, NULL, FALSE);
}

#ifdef WINSPY_FINDER_DROPDOWN
static BOOL dropdownHot, dropdownPressed;

static BOOL HasFinderDropdown(HWND hwnd)
{
	return GetDlgItem(GetParent(hwnd), IDC_FINDER_HINT) != NULL;
}

static int FinderDropdownWidth(HWND hwnd)
{
	return DpiScale(hwnd, 15);
}

static RECT FinderDropdownBounds(HWND hwnd)
{
	RECT rect;
	GetClientRect(hwnd, &rect);
	rect.left = rect.right - FinderDropdownWidth(hwnd);
	return rect;
}

static void SetFinderDropdownHot(HWND hwnd, BOOL hot)
{
	if(hot == dropdownHot)
		return;

	dropdownHot = hot;
	if(hot)
	{
		TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd };
		TrackMouseEvent(&track);
	}
	InvalidateRect(hwnd, NULL, FALSE);
}

static void OpenFinderDropdown(HWND hwnd)
{
	RECT rect = FinderDropdownBounds(hwnd);
	POINT pt = { rect.left, rect.bottom };
	ClientToScreen(hwnd, &pt);
	WinSpy_ShowFinderMenu(GetParent(hwnd), pt);
}

static void DrawExtendedFinder(HWND hwnd, HDC hdc, HDC source, const BITMAP *bm, BLENDFUNCTION blend)
{
	int rightFrame = DpiScale(hwnd, 3);
	int edge = bm->bmWidth - rightFrame;
	int extension = FinderDropdownWidth(hwnd);
	int top = MulDiv(bm->bmHeight, 6, 28);
	int bottom = bm->bmHeight - DpiScale(hwnd, 3);
	int cx = bm->bmWidth + (extension - rightFrame) / 2;
	int cy = (top + bottom) / 2;
	int half = DpiScale(hwnd, 4);
	RECT body = { bm->bmWidth + 1, top, bm->bmWidth + extension - rightFrame, bottom };
	POINT arrow[] = { {cx - half, cy - half / 2}, {cx + half, cy - half / 2}, {cx, cy + half / 2} };
	HGDIOBJ oldPen, oldBrush;
	COLORREF fill = RGB(232, 238, 245);

	// Keep the original pixels around the crosshair. Move the rounded
	// right edge out and continue the blue title bar with a column of pixels.
	AlphaBlend(hdc, 0, 0, edge, bm->bmHeight, source, 0, 0, edge, bm->bmHeight, blend);
	AlphaBlend(hdc, edge, 0, extension, bm->bmHeight, source, edge - 1, 0, 1, bm->bmHeight, blend);
	AlphaBlend(hdc, edge + extension, 0, rightFrame, bm->bmHeight,
		source, edge, 0, rightFrame, bm->bmHeight, blend);

	if(dropdownHot)
		fill = dropdownPressed ? RGB(181, 207, 240) : RGB(204, 223, 247);
	SetDCBrushColor(hdc, fill);
	FillRect(hdc, &body, GetStockObject(DC_BRUSH));

	oldPen = SelectObject(hdc, GetStockObject(DC_PEN));
	SetDCPenColor(hdc, RGB(151, 177, 213));
	MoveToEx(hdc, bm->bmWidth, top, NULL);
	LineTo(hdc, bm->bmWidth, bottom);

	SelectObject(hdc, GetStockObject(NULL_PEN));
	oldBrush = SelectObject(hdc, GetStockObject(DC_BRUSH));
	SetDCBrushColor(hdc, RGB(35, 53, 75));
	Polygon(hdc, arrow, ARRAYSIZE(arrow));
	SelectObject(hdc, oldPen);
	SelectObject(hdc, oldBrush);

	if(GetFocus() == hwnd && !(SendMessage(hwnd, WM_QUERYUISTATE, 0, 0) & UISF_HIDEFOCUS))
	{
		InflateRect(&body, -DpiScale(hwnd, 1), -DpiScale(hwnd, 2));
		DrawFocusRect(hdc, &body);
	}
}
#endif

static void LayoutFinderMenu(HWND hwndFinder)
{
	HWND hwndDlg = GetParent(hwndFinder);
	HWND hwndHint = GetDlgItem(hwndDlg, IDC_FINDER_HINT);
	HBITMAP hbm = (HBITMAP)GetProp(hwndFinder, PROP_CURRENT_BITMAP);
	BITMAP bm;
	RECT rect, hint;
	int finderWidth;
#ifndef WINSPY_FINDER_DROPDOWN
	HWND hwndMenu = GetDlgItem(hwndDlg, IDC_FINDER_MENU);
	RECT menu;
#endif

	if(!hwndHint || !GetObject(hbm, sizeof(bm), &bm))
		return;

	GetWindowRect(hwndFinder, &rect);
	MapWindowPoints(NULL, hwndDlg, (POINT *)&rect, 2);
	finderWidth = bm.bmWidth;
#ifdef WINSPY_FINDER_DROPDOWN
	finderWidth += FinderDropdownWidth(hwndFinder);
#else
	GetWindowRect(hwndMenu, &menu);
	MapWindowPoints(NULL, hwndDlg, (POINT *)&menu, 2);
	SetWindowPos(hwndMenu, NULL, menu.left, rect.top, menu.right - menu.left, bm.bmHeight,
		SWP_NOZORDER | SWP_NOACTIVATE);
#endif
	SetWindowPos(hwndFinder, NULL, 0, 0, finderWidth, bm.bmHeight, SWP_SIZEONLY);

	GetWindowRect(hwndHint, &hint);
	MapWindowPoints(NULL, hwndDlg, (POINT *)&hint, 2);
	SetWindowPos(hwndHint, NULL, rect.left + finderWidth + DpiScale(hwndFinder, 8), hint.top,
		0, 0, SWP_MOVEONLY);
}

#ifndef WINSPY_FINDER_DROPDOWN
void DrawFinderMenuButton(const DRAWITEMSTRUCT *draw)
{
	HWND hwnd = draw->hwndItem;
	HDC hdc = draw->hDC;
	RECT rc = draw->rcItem;

	if(!fThemeApiAvailable || FAILED(DrawThemeParentBackground(hwnd, hdc, &rc)))
		FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

#ifdef WINSPY_FINDER_TIMER
	{
		int offset = (draw->itemState & ODS_SELECTED) != 0;
		int cx = (rc.left + rc.right) / 2 + offset;
		int cy = (rc.top + rc.bottom) / 2 + offset;
		int radius = DpiScale(hwnd, 7);
		int stroke = DpiScale(hwnd, 2);
		int clockX = cx - DpiScale(hwnd, 5);
		int arrowX = cx + DpiScale(hwnd, 8);
		POINT arrow[] = {
			{arrowX - stroke, cy - 1}, {arrowX + stroke, cy - 1}, {arrowX, cy + stroke}
		};
		HPEN pen = CreatePen(PS_SOLID, stroke, GetSysColor(COLOR_BTNTEXT));

		HGDIOBJ oldPen = SelectObject(hdc, pen);
		HGDIOBJ oldBrush = SelectObject(hdc, GetStockObject(HOLLOW_BRUSH));
		Ellipse(hdc, clockX - radius, cy - radius, clockX + radius + 1, cy + radius + 1);
		MoveToEx(hdc, clockX, cy - radius, NULL);
		LineTo(hdc, clockX, cy - radius - stroke * 2);
		MoveToEx(hdc, clockX - stroke, cy - radius - stroke * 2, NULL);
		LineTo(hdc, clockX + stroke + 1, cy - radius - stroke * 2);
		MoveToEx(hdc, clockX, cy - stroke * 2, NULL);
		LineTo(hdc, clockX, cy);
		LineTo(hdc, clockX + stroke * 2, cy);
		SelectObject(hdc, GetStockObject(NULL_PEN));
		SelectObject(hdc, GetSysColorBrush(COLOR_BTNTEXT));
		Polygon(hdc, arrow, ARRAYSIZE(arrow));
		SelectObject(hdc, oldPen);
		SelectObject(hdc, oldBrush);
		DeleteObject(pen);
	}
#else
	DrawFinderCog(draw);
#endif

	if((draw->itemState & (ODS_FOCUS | ODS_NOFOCUSRECT)) == ODS_FOCUS)
	{
		InflateRect(&rc, -DpiScale(hwnd, 3), -DpiScale(hwnd, 3));
		DrawFocusRect(hdc, &rc);
	}
}
#endif
static void ClearScaledFinderBitmaps(HWND hwnd)
{
	HBITMAP hbm1 = (HBITMAP)RemoveProp(hwnd, PROP_DRAG1_SCALED);
	HBITMAP hbm2 = (HBITMAP)RemoveProp(hwnd, PROP_DRAG2_SCALED);

	RemoveProp(hwnd, PROP_CURRENT_BITMAP);
	if(hbm1) DeleteObject(hbm1);
	if(hbm2) DeleteObject(hbm2);
}

void FreeFinderResources()
{
	int i;

	for(i = 0; i < NUM_FINDER_VARIANTS; i++)
	{
		DeleteObject(hBitmapDrag1[i]);
		DeleteObject(hBitmapDrag2[i]);
	}

	DestroyCursor(hCursor);
}

WNDFINDPROC GetWndFindProc(HWND hwnd)
{
	return (WNDFINDPROC)GetWindowLongPtr(hwnd, GWLP_USERDATA);
}

UINT FireWndFindNotify(HWND hwndTool, UINT uCode, HWND hwnd)
{
	WNDFINDPROC wfp = GetWndFindProc(hwndTool);

	if(wfp != 0)
		return wfp(hwndTool, uCode, hwnd);
	else
		return 0;
}

LRESULT EndFindToolDrag(HWND hwnd, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent;

	hwndParent = GetParent(hwnd);
	
	//InvertWindow(hwndCurrent, fShowHidden);
	HideSel(hwndCurrent);
	ReleaseCapture();
	SetCursor(hOldCursor);

	// Remove keyboard hook. This is done even if the user presses ESC
	UnhookWindowsHookEx(draghook);
	

	fDragging = FALSE;
	SetFinderBitmap(hwnd, PROP_DRAG1_SCALED, hBitmapDrag1);

	return 0;
}

// Keyboard hook for the Finder Tool.
// This hook just monitors the ESCAPE key
static LRESULT CALLBACK draghookproc(int code, WPARAM wParam, LPARAM lParam)
{
	ULONG state = (ULONG)lParam;
	static int count;

	if(code < 0) 
		return CallNextHookEx(draghook, code, wParam, lParam);

	switch(wParam)
	{
	case VK_ESCAPE:
	
		if(!(state & 0x80000000))
		{
			//don't let the current window procedure process a VK_ESCAPE, 
			//because we want it to cancel the mouse capture
			PostMessage(draghookhwnd, WM_CANCELMODE, 0, 0);
			return -1;
		}

		break;

	case VK_SHIFT:
		
		if(state & 0x80000000)
		{
			//InvertWindow(hwndCurrent, fShowHidden);
			HideSel(hwndCurrent);
			FireWndFindNotify(draghookhwnd, WFN_SHIFT_UP, 0);
			//InvertWindow(hwndCurrent, fShowHidden);
			ShowSel(hwndCurrent);
		}
		else
		{
			if(!(state & 0x40000000))
			{
				//InvertWindow(hwndCurrent, fShowHidden);
				HideSel(hwndCurrent);
				FireWndFindNotify(draghookhwnd, WFN_SHIFT_DOWN, 0);
				//InvertWindow(hwndCurrent, fShowHidden);
				ShowSel(hwndCurrent);
			}
		}

		return -1;

	case VK_CONTROL:

		if(state & 0x80000000)
		{
			//InvertWindow(hwndCurrent, fShowHidden);
			HideSel(hwndCurrent);
			FireWndFindNotify(draghookhwnd, WFN_CTRL_UP, 0);
			//InvertWindow(hwndCurrent, fShowHidden);
			ShowSel(hwndCurrent);
		}
		else
		{
			if(!(state & 0x40000000))
			{
				//InvertWindow(hwndCurrent, fShowHidden);
				HideSel(hwndCurrent);
				FireWndFindNotify(draghookhwnd, WFN_CTRL_DOWN, 0);
				//InvertWindow(hwndCurrent, fShowHidden);
				ShowSel(hwndCurrent);
			}
		}
		
		return -1;
	}

	// Test to see if a key is pressed for first time
	if(!(state & 0xC0000000))
	{
		// Find ASCII character
		UINT ch = MapVirtualKey((UINT)wParam, 2);

		if(ch == _T('c') || ch == _T('C'))
		{
			//InvertWindow(hwndCurrent, fShowHidden);
			HideSel(hwndCurrent);
			CaptureWindow(GetParent(draghookhwnd), hwndCurrent);
			//InvertWindow(hwndCurrent, fShowHidden);
			ShowSel(hwndCurrent);
			return -1;
		}
	}

	return CallNextHookEx(draghook, code, wParam, lParam);
}

void ShowSel(HWND hwnd)
{
	if(fTransSel)
	{
		hwndTransPanel = ShowTransWindow(hwnd);

		if(hwndTransPanel == 0)
		{
			fTransSel = FALSE;
			InvertWindow(hwnd, fShowHidden);
		}
	}
	else
	{			
		InvertWindow(hwnd, fShowHidden);
	}
}

void HideSel(HWND hwnd)
{
	if(fTransSel)
	{
		DestroyWindow(hwndTransPanel);
		hwndTransPanel = 0;
	}
	else
	{			
		InvertWindow(hwnd, fShowHidden);
	}
}


LRESULT CALLBACK StaticProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	HWND hwndParent;
	POINT pt;

	static POINT ptLast;

	switch(msg)
	{
	case WM_LBUTTONDBLCLK:
	case WM_LBUTTONDOWN:

		ptLast.x = (short)LOWORD(lParam);
		ptLast.y = (short)HIWORD(lParam);

#ifdef WINSPY_FINDER_DROPDOWN
		if(HasFinderDropdown(hwnd))
		{
			RECT dropdown = FinderDropdownBounds(hwnd);
			if(PtInRect(&dropdown, ptLast))
			{
				SetFocus(hwnd);
				SetFinderDropdownHot(hwnd, TRUE);
				dropdownPressed = TRUE;
				SetCapture(hwnd);
				InvalidateRect(hwnd, &dropdown, FALSE);
				return 0;
			}
			SetFinderDropdownHot(hwnd, FALSE);
		}
#endif
		// Ask the callback function if we want to proceed
		if(FireWndFindNotify(hwnd, WFN_BEGIN, 0) == -1)
		{
			return 0;
		}

		fDragging = TRUE;

		SetFinderBitmap(hwnd, PROP_DRAG2_SCALED, hBitmapDrag2);

		hwndParent = GetParent(hwnd);
		hwndCurrent = hwnd;
		
		ShowSel(hwndCurrent);
		
		SetCapture(hwnd);
		hOldCursor = SetCursor(hCursor);

		// Install keyboard hook to trap ESCAPE key
		// We could just set the focus to this window to receive
		// normal keyboard messages - however, we don't want to
		// steal focus from current window when we use the drag tool,
		// so a hook is a stealthier way to monitor key presses
		draghookhwnd = hwnd;
		draghook     = SetWindowsHookEx(WH_KEYBOARD, draghookproc, GetModuleHandle(0), 0); 

		// Current window has changed
		FireWndFindNotify(hwnd, WFN_SELCHANGED, hwndCurrent);

		return 0;

	case WM_MOUSEMOVE:

		pt.x = (short)LOWORD(lParam);
		pt.y = (short)HIWORD(lParam);

#ifdef WINSPY_FINDER_DROPDOWN
		if(!fDragging && HasFinderDropdown(hwnd))
		{
			RECT dropdown = FinderDropdownBounds(hwnd);
			SetFinderDropdownHot(hwnd, PtInRect(&dropdown, pt));
			return 0;
		}
#endif
		if(fDragging == TRUE && ptLast.x != pt.x && ptLast.y != pt.y)
		{
			//MoveFindTool(hwnd, wParam, lParam);

			HWND hWndPoint;
			
			ptLast = pt;
			ClientToScreen(hwnd, (POINT *)&pt);

			hWndPoint = WindowFromPointEx(pt, fShowHidden);

			if(hWndPoint == 0)
				return 0;

			if(hWndPoint != hwndCurrent)
			{
				HideSel(hwndCurrent);
				//InvertWindow(hwndCurrent, fShowHidden);

				FireWndFindNotify(hwnd, WFN_SELCHANGED, hWndPoint);
				//InvertWindow(hWndPoint, fShowHidden);
				ShowSel(hWndPoint);

				hwndCurrent = hWndPoint;
			}
		}
		return 0;

	case WM_LBUTTONUP:

#ifdef WINSPY_FINDER_DROPDOWN
		if(dropdownPressed && HasFinderDropdown(hwnd))
		{
			RECT dropdown = FinderDropdownBounds(hwnd);
			POINT click = { (short)LOWORD(lParam), (short)HIWORD(lParam) };
			dropdownPressed = FALSE;
			ReleaseCapture();
			SetFinderDropdownHot(hwnd, FALSE);
			InvalidateRect(hwnd, &dropdown, FALSE);
			if(PtInRect(&dropdown, click))
				OpenFinderDropdown(hwnd);
			return 0;
		}
#endif
		// Mouse has been release, so end the find-tool
		if(fDragging == TRUE)
		{
			fDragging = FALSE;

			EndFindToolDrag(hwnd, wParam, lParam);
			FireWndFindNotify(hwnd, WFN_END, hwndCurrent);
		}

		return 0;

	// Sent from the keyboard hook
	case WM_CANCELMODE:
		
#ifdef WINSPY_FINDER_DROPDOWN
		if(dropdownPressed && HasFinderDropdown(hwnd))
			ReleaseCapture();
#endif
		// User has pressed ESCAPE, so cancel the find-tool
		if(fDragging == TRUE)
		{
			fDragging = FALSE;

			EndFindToolDrag(hwnd, wParam, lParam);
			FireWndFindNotify(hwnd, WFN_CANCELLED, 0);
		}

		return 0;

#ifdef WINSPY_FINDER_DROPDOWN
	case WM_CAPTURECHANGED:
		if(dropdownPressed && HasFinderDropdown(hwnd))
		{
			dropdownPressed = FALSE;
			InvalidateRect(hwnd, NULL, FALSE);
		}
		break;

	case WM_MOUSELEAVE:
		if(HasFinderDropdown(hwnd))
			SetFinderDropdownHot(hwnd, FALSE);
		break;

	case WM_GETDLGCODE:
		if(HasFinderDropdown(hwnd) && (wParam == VK_SPACE || wParam == VK_RETURN))
			return DLGC_WANTMESSAGE;
		break;

	case WM_KEYDOWN:
		if(HasFinderDropdown(hwnd) && !fDragging &&
			(wParam == VK_SPACE || wParam == VK_RETURN) && !(lParam & 0x40000000))
		{
			OpenFinderDropdown(hwnd);
			return 0;
		}
		break;

	case WM_SETFOCUS:
	case WM_KILLFOCUS:
	case WM_UPDATEUISTATE:
		if(HasFinderDropdown(hwnd))
			InvalidateRect(hwnd, NULL, FALSE);
		break;
#endif

	case WM_ERASEBKGND:

		// Background is painted as part of WM_PAINT, right before the
		// alpha-blended bitmap - avoids a double-paint/flicker here.
		return 1;

	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);
		RECT rc;
		HBITMAP hbmCur = (HBITMAP)GetProp(hwnd, PROP_CURRENT_BITMAP);

		GetClientRect(hwnd, &rc);

		// Paint whatever the parent would really have drawn here first,
		// so the bitmap's transparent/rounded-corner pixels blend with
		// the *actual* themed background (tab page, dialog, dark mode,
		// etc.) instead of a hardcoded guess at its colour.
		if(!fThemeApiAvailable || FAILED(DrawThemeParentBackground(hwnd, hdc, &rc)))
			FillRect(hdc, &rc, GetSysColorBrush(COLOR_BTNFACE));

		if(hbmCur != NULL)
		{
			BITMAP bm;
			HDC hdcMem = CreateCompatibleDC(hdc);
			HBITMAP hbmOld = SelectObject(hdcMem, hbmCur);
			BLENDFUNCTION bf = { AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

			GetObject(hbmCur, sizeof(bm), &bm);
#ifdef WINSPY_FINDER_DROPDOWN
			if(HasFinderDropdown(hwnd))
				DrawExtendedFinder(hwnd, hdc, hdcMem, &bm, bf);
			else
#endif
			AlphaBlend(hdc, 0, 0, bm.bmWidth, bm.bmHeight, hdcMem, 0, 0, bm.bmWidth, bm.bmHeight, bf);

			SelectObject(hdcMem, hbmOld);
			DeleteDC(hdcMem);
		}

		EndPaint(hwnd, &ps);
		return 0;
	}

	case WM_DPICHANGED_AFTERPARENT:
		ClearScaledFinderBitmaps(hwnd);
		if(fDragging && draghookhwnd == hwnd)
			SetFinderBitmap(hwnd, PROP_DRAG2_SCALED, hBitmapDrag2);
		else
			SetFinderBitmap(hwnd, PROP_DRAG1_SCALED, hBitmapDrag1);
		LayoutFinderMenu(hwnd);
		break;

	case WM_NCDESTROY:
		ClearScaledFinderBitmaps(hwnd);
		// When the last finder tool has been destroyed, free
		// up all the resources
		if(InterlockedDecrement(&lRefCount) == 0)
		{
			FreeFinderResources();
		}

		break;
	}

	return CallWindowProc(oldstaticproc, hwnd, msg, wParam, lParam);
}


BOOL MakeFinderTool(HWND hwnd, WNDFINDPROC wfp)
{
	DWORD dwStyle;
	
	// If this is the first finder tool, then load
	// the bitmap and mouse-cursor resources
	if(InterlockedIncrement(&lRefCount) == 1)
	{
		LoadFinderResources();
	}

	// Apply styles to make this a picture control
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	
	// Turn OFF styles we don't want
	dwStyle &= ~(SS_RIGHT | SS_CENTER | SS_CENTERIMAGE);
	dwStyle &= ~(SS_ICON | SS_SIMPLE | SS_LEFTNOWORDWRAP);
	dwStyle &= ~(SS_BITMAP);

	// Turn ON styles we must have - the control paints itself entirely
	// (see StaticProc's WM_PAINT), so it no longer needs SS_BITMAP.
	dwStyle |= SS_NOTIFY;

	// Now apply them..
	SetWindowLong(hwnd, GWL_STYLE, dwStyle);
	
	// Set the default bitmap, scaled for this control's monitor DPI
	SetFinderBitmap(hwnd, PROP_DRAG1_SCALED, hBitmapDrag1);

	// Set the callback for this control
	SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)wfp);
	
	// Subclass the static control
	oldstaticproc = (WNDPROC)SetWindowLongPtr(hwnd, GWLP_WNDPROC, (LONG_PTR)StaticProc);
	LayoutFinderMenu(hwnd);

	return TRUE;
}

