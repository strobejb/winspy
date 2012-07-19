//
//  DockTransPanel.c
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include "resource.h"

int WINAPI GetRectWidth(RECT *);
int WINAPI GetRectHeight(RECT *);

HBITMAP LoadPNGImage(UINT id, void **bits);

#define WC_TRANSWINDOW	TEXT("TransWindow")
#define DOCKRECT_TYPE_TRANS  0
#define DOCKRECT_TYPE_SHADED 1
#define DOCKRECT_TYPE_THICK  2

RECT spritemap[2][5] = 
{
	{
		{ 0, 2, 54, 34 },
		{ 129, 1, 182, 33 },
		{ 256, 1, 288, 54 },
		{ 385, 1, 417, 54 },
		{ 0,0,0,0},
	},
	{
		{ 4, 80, 51, 112 },		// bottom
		{ 132, 80, 179, 111 },	// top
		{ 256, 80, 290,127 },	// left
		{ 384,80, 416,127 },	// right
		{ 0, 140, 128,180 },//74, 172 },
	}
};


/*
void PreMultiplyRGBChannels(HBITMAP hBmp, LPBYTE pBitmapBits)
{
	BITMAP bmpInfo={0};
	int x, y;

	// pre-multiply rgb channels with alpha channel
	for (y = 0; y<bmpInfo.bmHeight; ++y)
	{
		BYTE *pPixel= pBitmapBits + bmpInfo.bmWidth * 4 * y;

		for (x = 0; x < bmpInfo.bmWidth; ++x)
		{
			pPixel[0]= pPixel[0]*pPixel[3]/255;
			pPixel[1]= pPixel[1]*pPixel[3]/255;
			pPixel[2]= pPixel[2]*pPixel[3]/255;

			pPixel += 4;
		}
	}
}
*/

HBITMAP MakeDockPanelBitmap(RECT *rect)
{
	int width  = GetRectWidth(rect);
	int height = GetRectHeight(rect);
	
	DWORD *pdwBox;

	static HBITMAP hbmBox, hbm;
	HDC		hdcBox, hdcDIB, hdcSrc;
	HANDLE	hOldBox, hOldDIB;

	RECT *sprite = 0;
	POINT pos = { 0 };

	// 32bpp bitmap
	BITMAPINFOHEADER bih = { sizeof(bih) };
	
	bih.biWidth			= width;
	bih.biHeight		= height;
	bih.biPlanes		= 1;
	bih.biBitCount		= 32;
	bih.biCompression	= BI_RGB;
	bih.biSizeImage		= 0;

    hdcSrc = GetDC(0);

	if(hbmBox == 0)
	{
		hbmBox	= LoadPNGImage(IDB_SELBOX, (void **)&pdwBox);
	}

	hdcBox	= CreateCompatibleDC(hdcSrc);
	hOldBox	= SelectObject(hdcBox, hbmBox);

	hbm		= CreateDIBSection(hdcSrc, (BITMAPINFO *)&bih, DIB_RGB_COLORS, (void**)&pdwBox, 0, 0);
	hdcDIB	= CreateCompatibleDC(hdcSrc);
	hOldDIB	= SelectObject(hdcDIB, hbm);

    if(1)//type & DOCKRECT_TYPE_THICK)
    {
	    // corners
	    BitBlt(hdcDIB, 0, 0, 32, 32, hdcBox, 0, 0, SRCCOPY);
	    BitBlt(hdcDIB, width - 32, 0, 32, 32, hdcBox, 32, 0, SRCCOPY);
	    BitBlt(hdcDIB, 0, height-32, 32, 32, hdcBox, 0, 32, SRCCOPY);
	    BitBlt(hdcDIB, width-32, height-32, 32, 32, hdcBox, 32, 32, SRCCOPY);

	    // sides
	    StretchBlt(hdcDIB, 0, 32, 32, height-64, hdcBox, 0,32,32,1,SRCCOPY);
	    StretchBlt(hdcDIB, width-32, 32, 32, height-64, hdcBox, 32,32,32,1,SRCCOPY);
	    StretchBlt(hdcDIB, 32, 0, width-64, 32, hdcBox, 32,0,1,32,SRCCOPY);
	    StretchBlt(hdcDIB, 32, height-32, width-64, 32, hdcBox, 32,32,1,32,SRCCOPY);

        //if(type & DOCKRECT_TYPE_SHADED)
        {
        	// middle
	        StretchBlt(hdcDIB, 32, 32, width-64, height-64, hdcBox, 32,32,1,1,SRCCOPY);
        }
    }
    /*else if(type & DOCKRECT_TYPE_SHADED)
    {
        StretchBlt(hdcDIB, 0, 0, width, height, hdcBox, 32,32,1,1,SRCCOPY);
    }*/


	SelectObject(hdcDIB,	hOldDIB);
	SelectObject(hdcBox,	hOldBox);

	DeleteDC(hdcBox);
	DeleteDC(hdcDIB);

	ReleaseDC(0, hdcSrc);
	return hbm;
}

void UpdatePanelTrans(HWND hwndPanel, RECT *rect)
{
	POINT ptZero = { 0, 0 };
	COLORREF crKey = RGB(0,0,0);

	BYTE SourceConstantAlpha = 220;//255;
	BLENDFUNCTION blendPixelFunction= { AC_SRC_OVER, 0, SourceConstantAlpha, AC_SRC_ALPHA };
	RECT rect2;
	//rect->right = rect->left + 316;
	//rect->bottom = rect->top + 382;

	POINT pt = { rect->left, rect->top };
	SIZE sz = { rect->right-rect->left, rect->bottom-rect->top};
	
	HDC hdcSrc = GetDC(0);
	HDC hdcMem = CreateCompatibleDC(hdcSrc);
	HBITMAP hbm;
	HANDLE hold;

	GetClientRect(hwndPanel, &rect2);
	hbm  = MakeDockPanelBitmap(rect);
	hold = SelectObject(hdcMem, hbm);

	//FillRect(hdcMem, &rect, GetSysColorBrush(COLOR_HIGHLIGHT));
	//SetWindowLongPtr(hwndPanel, GWL_EXSTYLE, GetWindowLongPtr(hwndPanel, GWL_EXSTYLE) | WS_EX_LAYERED);
	
	UpdateLayeredWindow(hwndPanel, 
		hdcSrc,
		&pt, //pos
		&sz, //size
		hdcMem,
		&ptZero,
		crKey,
		&blendPixelFunction,
		ULW_ALPHA);

	SelectObject(hdcMem, hold);
	DeleteDC(hdcMem);
	ReleaseDC(0, hdcSrc);
}

// 
//	Very simple window-procedure for the transparent window
//  all the drawing happens via the DOCKPANEL WM_TIMER, 
//  and calls to UpdateLayeredWindow with a transparent PNG graphic
//
LRESULT CALLBACK TransWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_NCHITTEST:
		return HTTRANSPARENT;
	}

	return DefWindowProc(hwnd, msg, wParam, lParam);
}

ATOM InitTrans()
{
	WNDCLASSEX wc = { sizeof(wc) };

	wc.style		 = 0;
	wc.lpszClassName = WC_TRANSWINDOW;
	wc.lpfnWndProc   = TransWndProc;

	return RegisterClassEx(&wc);
}


HWND ShowTransWindow(HWND hwnd)//, RECT *rect)
{
	HWND hwndTransPanel;

	RECT r,rect;
	GetWindowRect(hwnd, &r);
	rect = r;

	InitTrans();

	hwndTransPanel = CreateWindowEx(
		WS_EX_TOOLWINDOW|WS_EX_LAYERED,
		WC_TRANSWINDOW, 
		0, 
		WS_POPUP,
		r.left, r.top, 
		r.right-r.left,
		r.bottom-r.top, 
		0, 0,0, &rect);

	UpdatePanelTrans(hwndTransPanel, &r);

	SetWindowPos(hwndTransPanel, HWND_TOPMOST, 
		0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);

	//SetWindowPos(hwnd, HWND_TOP, 0, 0, 0, 0, SWP_NOMOVE|SWP_NOSIZE|SWP_NOACTIVATE|SWP_SHOWWINDOW);
	return hwndTransPanel;
}