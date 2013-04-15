//
//	CaptureWindow.c
//  Copyright (c) 2002 by J Brown. 
//  Portions Copyright (c) Microsoft Corporation. (from MSDN)
//	Freeware
//
//	void CaptureWindow(HWND hwndOwner, HWND hwnd)
//
//	hwndOwner  - handle to window that owns clipboard (in THIS process)
//  hwnd       - handle to any window to capture to clipboard
//
//	Two bitmaps objects will be placed on the clipboard
//	(1x DIB and 1x DDB)
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>

//
//  Define this to include DIB support. (Adds to code size)
//	Without this constant, only DDB bitmap will be added.
//
#define SUPPORT_DIBS

#ifdef SUPPORT_DIBS

#define PALVERSION   0x300

/* DIB macros */
#define IS_WIN30_DIB(lpbi)  ((*(LPDWORD)(lpbi)) == sizeof(BITMAPINFOHEADER))
#define WIDTHBYTES(bits)    (((bits) + 31) / 32 * 4)

static int PalEntriesOnDevice(HDC hdc)
{
   int nColors;

	// Find number of palette entries on this device
   nColors = GetDeviceCaps(hdc, SIZEPALETTE);

   if(nColors == 0)
      nColors = GetDeviceCaps(hdc, NUMCOLORS);

   return nColors;
}

static HPALETTE GetSystemPalette(HDC hdc)
{
   static HPALETTE hPal = 0;	// handle to a palette
   HANDLE hLogPal;				// handle to a logical palette
   LOGPALETTE *pLogPal;			// pointer to a logical palette
   int nColors;					// number of colors

   // Find out how many palette entries we want.
   nColors = PalEntriesOnDevice(hdc);   

   // Allocate room for the palette and lock it.
   hLogPal = GlobalAlloc(GHND, sizeof(LOGPALETTE) + nColors * sizeof(PALETTEENTRY));

   // if we didn't get a logical palette, return NULL
   if (!hLogPal) return NULL;

   // get a pointer to the logical palette
   pLogPal = (LPLOGPALETTE)GlobalLock(hLogPal);

   // set some important fields
   pLogPal->palVersion = PALVERSION;
   pLogPal->palNumEntries = nColors;

   //Copy the current system palette into our logical palette */
   GetSystemPaletteEntries(hdc, 0, nColors, (LPPALETTEENTRY)(pLogPal->palPalEntry));

   // Go ahead and create the palette.  Once it's created,
   hPal = CreatePalette(pLogPal);

   // clean up
   GlobalUnlock(hLogPal);
   GlobalFree(hLogPal);

   return hPal;
}

static WORD FAR DIBNumColors(LPSTR lpDIB)
{
	WORD wBitCount;  // DIB bit count
	
	if(IS_WIN30_DIB(lpDIB))
	{
		DWORD dwClrUsed;
		
		dwClrUsed = ((LPBITMAPINFOHEADER)lpDIB)->biClrUsed;
		if (dwClrUsed)
			return (WORD)dwClrUsed;
	}
	
	//  Calculate the number of colors in the color table based on
    // the number of bits per pixel for the DIB.
    //
	if (IS_WIN30_DIB(lpDIB))
		wBitCount = ((LPBITMAPINFOHEADER)lpDIB)->biBitCount;
	else
		wBitCount = ((LPBITMAPCOREHEADER)lpDIB)->bcBitCount;
	
	// return number of colors based on bits per pixel
	switch (wBitCount)
	{
	case 1:	return 2;
	case 4:	return 16;
	case 8:	return 256;
	default:return 0;
	}
}

static WORD FAR PaletteSize(LPSTR lpDIB)
{
   /* calculate the size required by the palette */
   if (IS_WIN30_DIB (lpDIB))
      return (DIBNumColors(lpDIB) * sizeof(RGBQUAD));
   else
      return (DIBNumColors(lpDIB) * sizeof(RGBTRIPLE));
}

static HANDLE BitmapToDIB(HBITMAP hBitmap, HPALETTE hPal)
{
   BITMAP bm;                   // bitmap structure
   BITMAPINFOHEADER bi;         // bitmap header
   BITMAPINFOHEADER FAR *lpbi;  // pointer to BITMAPINFOHEADER
   DWORD dwLen;                 // size of memory block
   HANDLE hDIB, h;              // handle to DIB, temp handle
   HDC hDC;                     // handle to DC
   WORD biBits;                 // bits per pixel

   /* check if bitmap handle is valid */

   if (!hBitmap)
      return NULL;

   /* fill in BITMAP structure, return NULL if it didn't work */
   if (!GetObject(hBitmap, sizeof(bm), (LPSTR)&bm))
      return NULL;

   /* if no palette is specified, use default palette */
   if (hPal == NULL)
      hPal = GetStockObject(DEFAULT_PALETTE);

   /* calculate bits per pixel */
   biBits = bm.bmPlanes * bm.bmBitsPixel;

   /* make sure bits per pixel is valid */
   if (biBits <= 1)
      biBits = 1;
   else if (biBits <= 4)
      biBits = 4;
   else if (biBits <= 8)
      biBits = 8;
   else /* if greater than 8-bit, force to 24-bit */
      biBits = 24;

   /* initialize BITMAPINFOHEADER */
   bi.biSize = sizeof(BITMAPINFOHEADER);
   bi.biWidth = bm.bmWidth;
   bi.biHeight = bm.bmHeight;
   bi.biPlanes = 1;
   bi.biBitCount = biBits;
   bi.biCompression = BI_RGB;
   bi.biSizeImage = 0;
   bi.biXPelsPerMeter = 0;
   bi.biYPelsPerMeter = 0;
   bi.biClrUsed = 0;
   bi.biClrImportant = 0;

   /* calculate size of memory block required to store BITMAPINFO */
   dwLen = bi.biSize + PaletteSize((LPSTR)&bi);

   /* get a DC */
   hDC = GetDC(NULL);

   /* select and realize our palette */
   hPal = SelectPalette(hDC, hPal, FALSE);
   RealizePalette(hDC);

   /* alloc memory block to store our bitmap */
   hDIB = GlobalAlloc(GHND, dwLen);

   /* if we couldn't get memory block */
   if (!hDIB)
   {
      /* clean up and return NULL */
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }

   /* lock memory and get pointer to it */
   lpbi = (VOID FAR *)GlobalLock(hDIB);

   /* use our bitmap info. to fill BITMAPINFOHEADER */
   *lpbi = bi;

   /*  call GetDIBits with a NULL lpBits param, so it will calculate the
    *  biSizeImage field for us
    */
   GetDIBits(hDC, hBitmap, 0, (WORD)bi.biHeight, NULL, (LPBITMAPINFO)lpbi,
         DIB_RGB_COLORS);

   /* get the info. returned by GetDIBits and unlock memory block */
   bi = *lpbi;
   GlobalUnlock(hDIB);

   /* if the driver did not fill in the biSizeImage field, make one up */
   if (bi.biSizeImage == 0)
      bi.biSizeImage = WIDTHBYTES((DWORD)bm.bmWidth * biBits) * bm.bmHeight;

   /* realloc the buffer big enough to hold all the bits */
   dwLen = bi.biSize + PaletteSize((LPSTR)&bi) + bi.biSizeImage;
   if (h = GlobalReAlloc(hDIB, dwLen, 0))
      hDIB = h;
   else
   {
      /* clean up and return NULL */
      GlobalFree(hDIB);
      hDIB = NULL;
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }

   /* lock memory block and get pointer to it */
   lpbi = (VOID FAR *)GlobalLock(hDIB);

   /*  call GetDIBits with a NON-NULL lpBits param, and actualy get the
    *  bits this time
    */
   if (GetDIBits(hDC, hBitmap, 0, (WORD)bi.biHeight, (LPSTR)lpbi + (WORD)lpbi
         ->biSize + PaletteSize((LPSTR)lpbi), (LPBITMAPINFO)lpbi,
         DIB_RGB_COLORS) == 0)
   {
      /* clean up and return NULL */
      GlobalUnlock(hDIB);
      hDIB = NULL;
      SelectPalette(hDC, hPal, TRUE);
      RealizePalette(hDC);
      ReleaseDC(NULL, hDC);
      return NULL;
   }
   bi = *lpbi;

   /* clean up */
   GlobalUnlock(hDIB);
   SelectPalette(hDC, hPal, TRUE);
   RealizePalette(hDC);
   ReleaseDC(NULL, hDC);

   /* return handle to the DIB */
   return hDIB;
}

#endif

BOOL CaptureWindow(HWND hwndOwner, HWND hwnd)
{
	RECT rect;
	HDC hdc, hdcMem, hdcOld;
	HBITMAP hBmp;
	HANDLE hDIB;
	
	HPALETTE hPal;

	int width, height;

	int RasterCapsScrn;
	int PaletteSizeScrn;

	GetWindowRect(hwnd, &rect);
	width = rect.right-rect.left;
	height = rect.bottom-rect.top;

	hdc = GetDC(0);

	hdcMem = CreateCompatibleDC(hdc);
	hBmp   = CreateCompatibleBitmap(hdc, width, height);

	hdcOld = SelectObject(hdcMem, hBmp);

	//copy the screen contents
	BitBlt(hdcMem, 0, 0, width, height, hdc, rect.left, rect.top, SRCCOPY);
	SelectObject(hdcMem, hdcOld);

	OpenClipboard(hwndOwner);
	EmptyClipboard();

#ifdef SUPPORT_DIBS
	//palette detection
	RasterCapsScrn  = GetDeviceCaps(hdc, RASTERCAPS);
	PaletteSizeScrn = GetDeviceCaps(hdc, SIZEPALETTE);
  
	if((RasterCapsScrn & RC_PALETTE) && (PaletteSizeScrn == 256))
		hPal = GetSystemPalette(hdc);
	else
		hPal = 0;

	hDIB = BitmapToDIB(hBmp, hPal);
	SetClipboardData(CF_DIB, hDIB);
#endif

	SetClipboardData(CF_BITMAP, hBmp);


	CloseClipboard();
	
	ReleaseDC(0, hdc);

	return TRUE;
}

