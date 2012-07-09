#ifndef BITMAPBUTTON_INCLUDED
#define BITMAPBUTTON_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

BOOL  DrawBitmapButton      (DRAWITEMSTRUCT *dis);

void  MakeBitmapButton		(HWND hwnd,                  UINT uIconId);
void  MakeDlgBitmapButton	(HWND hwndDlg, UINT uCtrlId, UINT uIconId);

#ifdef __cplusplus
}
#endif

#endif