//
//	InjectThread.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//  InjectThread uses the CreateRemoteThread API call
//  (under NT/2000/XP) to inject a piece of binary code
//  into the process which owns the specified window.
//
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

#include "InjectThread.h"

#define INJECT_PRIVELIDGE (PROCESS_CREATE_THREAD|PROCESS_QUERY_INFORMATION|PROCESS_VM_OPERATION|PROCESS_VM_READ|PROCESS_VM_WRITE)

typedef PVOID (WINAPI * VA_EX_PROC)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
typedef PVOID (WINAPI * VF_EX_PROC)(HANDLE, LPVOID, SIZE_T, DWORD);

//
//	Inject a thread into the process which owns the specified window.
//
//	lpCode     - address of function to inject. (See CreateRemoteThread)
//  cbCodeSize - size (in bytes) of the function
//
//	lpData     - address of a user-defined structure to be passed to the injected thread
//  cbDataSize - size (in bytes) of the structure
//
//  The user-defined structure is also injected into the target process' address space.
//  When the thread terminates, the structure is read back from the process.
//
DWORD InjectRemoteThread(HWND hwnd, LPTHREAD_START_ROUTINE lpCode, DWORD cbCodeSize, LPVOID lpData, DWORD cbDataSize)
{
	DWORD  dwProcessId;			//id of remote process
	DWORD  dwThreadId;			//id of the thread in remote process 
	HANDLE hProcess;			//handle to the remote process
	
	HANDLE hRemoteThread = 0;	//handle to the injected thread
	DWORD  dwRemoteThreadId =0;	//ID of the injected thread
	
	SIZE_T dwWritten = 0;		// Number of bytes written to the remote process
	SIZE_T dwRead = 0;
	DWORD  dwExitCode;
	
	void *pRemoteData = 0;
	
	VA_EX_PROC pVirtualAllocEx; 
	VF_EX_PROC pVirtualFreeEx;

	// The address to which code will be copied in the remote process
	DWORD *pdwRemoteCode;

	const int cbCodeSizeAligned = (cbCodeSize + (sizeof(LONG_PTR)-1)) & ~ (sizeof(LONG_PTR)-1);
	
	// Find the process ID of the process which created the specified window
	dwThreadId = GetWindowThreadProcessId(hwnd, &dwProcessId);
	
	// Open the remote process so we can allocate some memory in it
	hProcess = OpenProcess(INJECT_PRIVELIDGE, FALSE, dwProcessId);

	//Accessed denied????!!!!!!!!!
	if(hProcess == 0)
	{
		return FALSE;
	}

	// Allocate enough memory in the remote process's address space
	// to hold the binary image of our injection thread, and
	// a copy of the INJTHREADINFO structure
	pVirtualAllocEx = (VA_EX_PROC)GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")), "VirtualAllocEx");
	pVirtualFreeEx  = (VF_EX_PROC)GetProcAddress(GetModuleHandle(_T("KERNEL32.DLL")), "VirtualFreeEx");

	if(pVirtualAllocEx == 0 || pVirtualFreeEx == 0)
		return FALSE;

	pdwRemoteCode = pVirtualAllocEx(hProcess, 0, cbCodeSizeAligned + cbDataSize, MEM_COMMIT|MEM_RESERVE, PAGE_EXECUTE_READWRITE);

	if(pdwRemoteCode == 0)
		return FALSE;

	// Write a copy of our injection thread into the remote process
	WriteProcessMemory(hProcess, pdwRemoteCode, lpCode, (SIZE_T)cbCodeSize, &dwWritten);

	// Write a copy of the INJTHREAD to the remote process. This structure
	// MUST start on a 32bit/64bit boundary
	pRemoteData = (void *)((BYTE *)pdwRemoteCode + cbCodeSizeAligned);
	
	// Put DATA in the remote thread's memory block
	WriteProcessMemory(hProcess, pRemoteData, lpData, cbDataSize, &dwWritten);

	// Create the remote thread!!!
#ifndef _DEBUG
	hRemoteThread = CreateRemoteThread(hProcess, NULL, 0, 
		(LPTHREAD_START_ROUTINE)pdwRemoteCode, pRemoteData, 0, &dwRemoteThreadId);
#endif

	// Wait for the thread to terminate
	WaitForSingleObject(hRemoteThread, INFINITE);
	
	// Read the user-structure back again
	if(!ReadProcessMemory(hProcess, pRemoteData, lpData, cbDataSize, &dwRead))
	{
		//an error occurred
	}
	
	GetExitCodeThread(hRemoteThread, &dwExitCode);
	
	CloseHandle(hRemoteThread);
	
	// Free the memory in the remote process
	pVirtualFreeEx(hProcess, pdwRemoteCode, 0, MEM_RELEASE);
	CloseHandle(hProcess);

	return dwExitCode;
}