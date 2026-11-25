#if !defined(WINSPY_FINDER_DROPDOWN) && !defined(WINSPY_FINDER_TIMER)

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <commctrl.h>
#include <objidl.h>
#include <gdiplus.h>

#include "FindTool.h"
#include "Utils.h"
#include "resource.h"

#pragma comment(lib, "gdiplus.lib")

using namespace Gdiplus;

static const WCHAR cogHotProperty[] = L"WinSpyFinderCogHot";

static LRESULT CALLBACK FinderCogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
	UINT_PTR subclassId, DWORD_PTR)
{
	switch(msg)
	{
	case WM_MOUSEMOVE:
		if(!GetPropW(hwnd, cogHotProperty))
		{
			TRACKMOUSEEVENT track = { sizeof(track), TME_LEAVE, hwnd };
			if(TrackMouseEvent(&track))
			{
				SetPropW(hwnd, cogHotProperty, reinterpret_cast<HANDLE>(1));
				InvalidateRect(hwnd, NULL, FALSE);
			}
		}
		break;

	case WM_MOUSELEAVE:
		if(RemovePropW(hwnd, cogHotProperty))
			InvalidateRect(hwnd, NULL, FALSE);
		break;

	case WM_NCDESTROY:
		RemovePropW(hwnd, cogHotProperty);
		RemoveWindowSubclass(hwnd, FinderCogProc, subclassId);
		break;
	}

	return DefSubclassProc(hwnd, msg, wParam, lParam);
}

void InitFinderCog(HWND hwnd)
{
	SetWindowSubclass(hwnd, FinderCogProc, 0, 0);
}

class CogGraphicsSession
{
public:
	CogGraphicsSession()
	{
		GdiplusStartupInput input;
		if(GdiplusStartup(&token, &input, NULL) != Ok)
			token = 0;
	}

	~CogGraphicsSession()
	{
		if(token)
			GdiplusShutdown(token);
	}

	ULONG_PTR token = 0;
};

static Color SystemColor(int index)
{
	const COLORREF color = GetSysColor(index);
	return Color(GetRValue(color), GetGValue(color), GetBValue(color));
}

void DrawFinderCog(const DRAWITEMSTRUCT *draw)
{
	HWND hwnd = draw->hwndItem;
	const RECT &rc = draw->rcItem;
	const bool pressed = (draw->itemState & ODS_SELECTED) != 0;
	const bool hot = GetPropW(hwnd, cogHotProperty) != NULL;
	HIGHCONTRAST contrast = { sizeof(contrast) };
	const bool highContrast = SystemParametersInfo(SPI_GETHIGHCONTRAST, sizeof(contrast), &contrast, 0) &&
		(contrast.dwFlags & HCF_HIGHCONTRASTON);

	static const CogGraphicsSession session;
	if((hot || pressed) && session.token)
	{
		const Color highlight = highContrast ? SystemColor(COLOR_HIGHLIGHT)
			: pressed ? Color(190, 213, 240) : Color(219, 233, 250);
		const REAL dpiScale = static_cast<REAL>(GetWindowDpi(hwnd)) / USER_DEFAULT_SCREEN_DPI;
		Graphics graphics(draw->hDC);
		graphics.SetSmoothingMode(SmoothingModeAntiAlias);
		graphics.TranslateTransform((rc.left + rc.right) / 2.0f, (rc.top + rc.bottom) / 2.0f);
		graphics.ScaleTransform(dpiScale, dpiScale);

		GraphicsPath background;
		background.AddArc(-15.0f, -12.0f, 10.0f, 10.0f, 180.0f, 90.0f);
		background.AddArc(5.0f, -12.0f, 10.0f, 10.0f, 270.0f, 90.0f);
		background.AddArc(5.0f, 2.0f, 10.0f, 10.0f, 0.0f, 90.0f);
		background.AddArc(-15.0f, 2.0f, 10.0f, 10.0f, 90.0f, 90.0f);
		background.CloseFigure();
		SolidBrush brush(highlight);
		graphics.FillPath(&brush, &background);
	}

	const int size = DpiScale(hwnd, 16);
	const int x = (rc.left + rc.right - size) / 2;
	const int y = (rc.top + rc.bottom - size) / 2;
	HICON icon = NULL;
	if(FAILED(LoadIconWithScaleDown(GetModuleHandleW(NULL), MAKEINTRESOURCEW(IDI_FINDER_COG),
		size, size, &icon)))
		return;

	if(highContrast)
	{
		HBRUSH ink = GetSysColorBrush(hot || pressed ? COLOR_HIGHLIGHTTEXT : COLOR_BTNTEXT);
		DrawState(draw->hDC, ink, NULL, reinterpret_cast<LPARAM>(icon), 0, x, y, size, size, DST_ICON | DSS_MONO);
	}
	else
		DrawIconEx(draw->hDC, x, y, icon, size, size, 0, NULL, DI_NORMAL);
	DestroyIcon(icon);
}

#endif
