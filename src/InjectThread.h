#ifndef INJECT_THREAD
#define INJECT_THREAD

#ifdef __cplusplus
extern "C" {
#endif

DWORD InjectRemoteThread(HWND hwnd, LPTHREAD_START_ROUTINE lpCode, DWORD cbCodeSize, LPVOID lpData, DWORD cbDataSize);

#ifdef __cplusplus
}
#endif

#endif