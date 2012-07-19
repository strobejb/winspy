#ifndef UTILS_INCLUDED
#define UTILS_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

UINT AddStyle(HWND hwnd, UINT style);
UINT AddDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style);
UINT DelStyle(HWND hwnd, UINT style);
UINT DelDlgItemStyle(HWND hwnd, UINT nCtrlId, UINT style);
BOOL EnableDlgItem(HWND hwnd, UINT nCtrlId, BOOL fEnabled);
BOOL ShowDlgItem(HWND hwnd, UINT nCtrlId, DWORD dwShowCmd);

int WINAPI GetRectHeight(RECT *rect);
int WINAPI GetRectWidth(RECT *rect);

DWORD GetDlgItemBaseInt(HWND hwnd, UINT ctrlid, int base);
UINT _tstrtoib16(TCHAR *szHexStr);
BOOL EnableDialogTheme(HWND hwnd);

BOOL EnableDebugPrivilege();

#ifdef __cplusplus
}
#endif

#endif