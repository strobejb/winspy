//
//	GetRemoteClassInfoEx.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//  BOOL GetRemoteClassInfoEx(HWND hwnd)
//
//  In order to retrieve private class information for a 
//  window in another process, we have to create
//  a remote thread in that process and call GetClassInfoEx from
//  there.
//
//  GetRemoteClassInfoEx uses the InjectRemoteThread call defined
//  in InjectThread.c
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "InjectThread.h"

typedef BOOL (WINAPI *PROCGETCLASSINFOEX) (HINSTANCE, LPTSTR, WNDCLASSEX*);
typedef LONG (WINAPI *PROCGETWINDOWLONG)  (HWND, int);
typedef int  (WINAPI *PROCGETWINDOWTEXT)  (HWND, LPTSTR, int);
typedef UINT (WINAPI *PROCSENDMESSAGETO)  (HWND, UINT, WPARAM, LPARAM, UINT, UINT, DWORD*);

//
//	Define a structure for the remote thread to use
//
typedef struct 
{
	PROCGETCLASSINFOEX fnGetClassInfoEx;
	PROCGETWINDOWLONG  fnGetWindowLong;
	PROCGETWINDOWTEXT  fnGetWindowText;
	PROCSENDMESSAGETO  fnSendMessageTimeout;

	HWND        hwnd;		//window we want to get class info for
	ATOM        atom;		//class atom of window
	HINSTANCE   hInst;
	
	TCHAR		szClassName[128];

	WNDCLASSEX  wcOutput;
	WNDPROC     wndproc;

	// Window text to retrieve
	TCHAR       szText[200];	// text (out)
	int         nTextSize;

} INJDATA;



// calls to the stack checking routine must be disabled
#pragma check_stack (off)

//
//	Thread to inject to remote process. Must not
//  make ANY calls to code in THIS process.
//
static DWORD WINAPI GetClassInfoExProc(LPVOID *pParam)
{
	INJDATA *pInjData = (INJDATA *)pParam;
	BOOL    fRet = 0;
	DWORD   dwResult;

	if(pInjData->fnGetWindowLong)
		pInjData->wndproc = (WNDPROC)pInjData->fnGetWindowLong(pInjData->hwnd, GWLP_WNDPROC);

	if(pInjData->fnGetClassInfoEx)
		fRet = pInjData->fnGetClassInfoEx(pInjData->hInst, (LPTSTR)pInjData->szClassName, &pInjData->wcOutput);

	//if(pInjData->fnGetWindowText)
	//	pInjData->fnGetWindowText(pInjData->hwnd, pInjData->szText, pInjData->nTextSize);

	if(pInjData->fnSendMessageTimeout)
	{
		// Nul-terminate in case the gettext fails
		pInjData->szText[0] = _T('\0');

		pInjData->fnSendMessageTimeout(pInjData->hwnd, WM_GETTEXT, 
				pInjData->nTextSize, (LPARAM)pInjData->szText, 
				SMTO_ABORTIFHUNG, 100, &dwResult);
	}

	return fRet;

}

static void AfterThreadProc(void) { }

#pragma check_stack 

BOOL GetRemoteWindowInfo(HWND hwnd, WNDCLASSEX *pClass, WNDPROC *pProc, TCHAR *pszText, int nTextLen)
{
	INJDATA InjData;
	HMODULE hUser32 = GetModuleHandle(__TEXT("User32"));
	BOOL    fReturn;

	DWORD   dwThreadId;

	// Calculate how many bytes the injected code takes
	DWORD cbCodeSize = ((BYTE *)(DWORD)AfterThreadProc - (BYTE *)(DWORD)GetClassInfoExProc);

	//
	// Setup the injection structure:
	//
	ZeroMemory(&InjData, sizeof(InjData));

	// Get pointers to the API calls we will be using in the remote thread
#ifdef UNICODE
	InjData.fnSendMessageTimeout = (PROCSENDMESSAGETO)  GetProcAddress(hUser32, "SendMessageTimeoutW");
	//InjData.fnGetWindowText  = (PROCGETWINDOWTEXT)  GetProcAddress(hUser32, "GetWindowTextW");
#else
	InjData.fnSendMessageTimeout = (PROCSENDMESSAGETO)  GetProcAddress(hUser32, "SendMessageTimeoutA");
	//InjData.fnGetWindowText  = (PROCGETWINDOWTEXT)  GetProcAddress(hUser32, "GetWindowTextA");
#endif
	
	if(IsWindowUnicode(hwnd))
	{
		InjData.fnGetWindowLong  = (PROCGETWINDOWLONG)  GetProcAddress(hUser32, "GetWindowLongW");
		InjData.fnGetClassInfoEx = (PROCGETCLASSINFOEX) GetProcAddress(hUser32, "GetClassInfoExW");

		GetClassNameW(hwnd, (WORD *)InjData.szClassName, sizeof(InjData.szClassName) / sizeof(WORD));
	}
	else
	{
		InjData.fnGetWindowLong  = (PROCGETWINDOWLONG)  GetProcAddress(hUser32, "GetWindowLongA");
		InjData.fnGetClassInfoEx = (PROCGETCLASSINFOEX) GetProcAddress(hUser32, "GetClassInfoExA");

		GetClassNameA(hwnd, (char *)InjData.szClassName, sizeof(InjData.szClassName) / sizeof(char));
	}

	// Setup the data the API calls will need
	InjData.hwnd      = (HWND)hwnd;
	InjData.atom      = (ATOM)GetClassLong(hwnd, GCW_ATOM);
	InjData.hInst     = (HINSTANCE)GetWindowLongPtr(hwnd, GWLP_HINSTANCE);
	InjData.wndproc   = 0;
	InjData.nTextSize = sizeof(InjData.szText) / sizeof(TCHAR);


	// if we are injecting into THIS process, DO NOT RUN GETWINDOWTEXT!!!
	// This is because GetWindowText uses SendMessage internally,
	// which will send (and wait for) a message to THIS PROCESS. Because
	// we wait for the remote thread to terminate, we cannot process any
	// messages, and we get locked up. Took me two days to realise this :-(
	dwThreadId = GetWindowThreadProcessId(hwnd, 0);

	if(dwThreadId == GetCurrentThreadId())
	{
		InjData.fnGetWindowText = 0;
	}
	
	//
	// Inject the GetClassInfoExProc function, and our InjData structure!
	//
	fReturn = InjectRemoteThread(hwnd, GetClassInfoExProc, cbCodeSize, &InjData, sizeof(InjData));

	if(fReturn == FALSE)
	{
		// Failed to retrieve class information!
		*pProc  = NULL;
		ZeroMemory(pClass, sizeof(WNDCLASSEX));
		pszText[0] = 0;
		return FALSE;
	}
	else
	{
		*pClass = InjData.wcOutput;
		*pProc  = InjData.wndproc;

		lstrcpyn(pszText, InjData.szText, 200);
		return TRUE;
	}
}
