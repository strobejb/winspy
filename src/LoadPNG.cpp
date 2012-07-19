//
//  LoadPNG.cpp
//
//  www.catch22.net
//
//  Copyright (C) 2012 James Brown
//  Please refer to the file LICENCE.TXT for copying permission
//

#include <windows.h>
#include <wincodec.h>
#include <tchar.h>

#pragma comment(lib, "windowscodecs")

//#import "wincodec.idl" no_namespace 

// Creates a stream object initialized with the data from an executable resource.
IStream * CreateStreamOnResource(LPCTSTR lpName, LPCTSTR lpType)
{
    IStream * ipStream = NULL;

    // find the resource
    HRSRC hRes = FindResource(NULL, lpName, lpType);
	DWORD dwResourceSize;
	HGLOBAL hglbImage;
	LPVOID pvSourceResourceData;
	HGLOBAL hgblResourceData;
	LPVOID pvResourceData;

    if(hRes == NULL)
        goto Return;

    // load the resource
    dwResourceSize = SizeofResource(NULL, hRes);
    hglbImage = LoadResource(NULL, hRes);

    if (hglbImage == NULL)
        goto Return;

    // lock the resource, getting a pointer to its data
	pvSourceResourceData = LockResource(hglbImage);

    if (pvSourceResourceData == NULL)
        goto Return;

    // allocate memory to hold the resource data
	hgblResourceData = GlobalAlloc(GMEM_MOVEABLE, dwResourceSize);

    if (hgblResourceData == NULL)
        goto Return;

    // get a pointer to the allocated memory
    pvResourceData = GlobalLock(hgblResourceData);

    if (pvResourceData == NULL)
        goto FreeData;

    // copy the data from the resource to the new memory block
    CopyMemory(pvResourceData, pvSourceResourceData, dwResourceSize);

    GlobalUnlock(hgblResourceData);

    // create a stream on the HGLOBAL containing the data
	// Specify that the HGLOBAL will be automatically free'd on the last Release()
    if (SUCCEEDED(CreateStreamOnHGlobal(hgblResourceData, TRUE, &ipStream)))
        goto Return;

FreeData:

    // couldn't create stream; free the memory
    GlobalFree(hgblResourceData);

Return:

    // no need to unlock or free the resource
    return ipStream;
}

// Now that we have an IStream pointer to the data of the image, 
// we can use WIC to load that image. An important step in this 
// process is to use WICConvertBitmapSource to ensure that the image
// is in a 32bpp format suitable for direct conversion into a DIB. 
// This method assumes that the input image is in the PNG format; 
// for a splash screen, this is an excellent choice because it allows 
// an alpha channel as well as lossless compression of the source image. 
// (To make the splash screen image as small as possible, 
// I highly recommend the PNGOUT compression utility.)

// Loads a PNG image from the specified stream (using Windows Imaging Component).
IWICBitmapSource * LoadBitmapFromStream(IStream * ipImageStream)
{
    // initialize return value
    IWICBitmapSource * ipBitmap = NULL;

    // load WIC's PNG decoder
    IWICBitmapDecoder * ipDecoder = NULL;
	IID i = IID_IWICBitmapDecoder;

    if (FAILED(CoCreateInstance(CLSID_WICPngDecoder, NULL, CLSCTX_INPROC_SERVER, 
		i,//__uuidof(ipDecoder)
		 
		(void **)&ipDecoder)))
        goto Return;
 

    // load the PNG
    if (FAILED(ipDecoder->Initialize(ipImageStream, WICDecodeMetadataCacheOnLoad)))
        goto ReleaseDecoder;

    // check for the presence of the first frame in the bitmap
    UINT nFrameCount = 0;

    if (FAILED(ipDecoder->GetFrameCount(&nFrameCount)) || nFrameCount != 1)
        goto ReleaseDecoder;

    // load the first frame (i.e., the image)
    IWICBitmapFrameDecode * ipFrame = NULL;

    if (FAILED(ipDecoder->GetFrame(0, &ipFrame)))
        goto ReleaseDecoder;
 

    // convert the image to 32bpp BGRA format with pre-multiplied alpha
    //   (it may not be stored in that format natively in the PNG resource,
    //   but we need this format to create the DIB to use on-screen)
    WICConvertBitmapSource(GUID_WICPixelFormat32bppPBGRA, ipFrame, &ipBitmap);
    ipFrame->Release();

ReleaseDecoder:
    ipDecoder->Release();

Return:
    return ipBitmap;

}


// Creates a 32-bit DIB from the specified WIC bitmap.
HBITMAP CreateHBITMAP(IWICBitmapSource * ipBitmap, PVOID *bits)
{
    // initialize return value
    HBITMAP hbmp = NULL;

    // get image attributes and check for valid image
    UINT width = 0;
    UINT height = 0;

    if (FAILED(ipBitmap->GetSize(&width, &height)) || width == 0 || height == 0)
        goto Return;

    // prepare structure giving bitmap information (negative height indicates a top-down DIB)
    BITMAPINFO bminfo;

    ZeroMemory(&bminfo, sizeof(bminfo));

    bminfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bminfo.bmiHeader.biWidth = width;
    bminfo.bmiHeader.biHeight = -((LONG) height);
    bminfo.bmiHeader.biPlanes = 1;
    bminfo.bmiHeader.biBitCount = 32;
    bminfo.bmiHeader.biCompression = BI_RGB;

    // create a DIB section that can hold the image
    void * pvImageBits = NULL;

    HDC hdcScreen = GetDC(NULL);
    hbmp = CreateDIBSection(hdcScreen, &bminfo, DIB_RGB_COLORS, &pvImageBits, NULL, 0);
	*bits = pvImageBits;
    ReleaseDC(NULL, hdcScreen);

    if (hbmp == NULL)
        goto Return;

 
    // extract the image into the HBITMAP
    const UINT cbStride = width * 4;
    const UINT cbImage = cbStride * height;

    if (FAILED(ipBitmap->CopyPixels(NULL, cbStride, cbImage, static_cast<BYTE *>(pvImageBits))))
    {
        // couldn't extract image; delete HBITMAP
        DeleteObject(hbmp);

        hbmp = NULL;
    }

Return:
    return hbmp;
}

// Loads the PNG bitmap into a 32bit HBITMAP.
extern "C"
HBITMAP LoadPNGImage(UINT id, OUT VOID **bits)
{
    HBITMAP hbmpSplash = NULL;
 
    // load the PNG image data into a stream
    IStream * ipImageStream = CreateStreamOnResource(MAKEINTRESOURCE(id), _T("PNG"));

    if (ipImageStream == NULL)
        goto Return;

    // load the bitmap with WIC
    IWICBitmapSource * ipBitmap = LoadBitmapFromStream(ipImageStream);

    if (ipBitmap == NULL)
        goto ReleaseStream;

    // create a HBITMAP containing the image
    hbmpSplash = CreateHBITMAP(ipBitmap, bits);

    ipBitmap->Release();

ReleaseStream:

    ipImageStream->Release();

Return:
    return hbmpSplash;
}
