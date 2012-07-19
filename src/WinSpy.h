#ifndef WINSPY_INCLUDED
#define WINSPY_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include <windows.h>
#include <commctrl.h>
#include <tchar.h>

#define IDM_WINSPY_ABOUT	100

//
//	Define a structure for each property page in
//  the main window
//
typedef struct 
{
	HWND    hwnd;
	LPCTSTR szText;
	UINT    id;
	WNDPROC dlgproc;
} DialogTab;

extern DialogTab WinSpyTab[];

#define GENERAL_TAB        0
#define STYLE_TAB          1
#define PROPERTY_TAB       2
#define CLASS_TAB          3
#define WINDOW_TAB         4
#define PROCESS_TAB	       5
#define NUMTABCONTROLITEMS 6

//
//	Simple style-lookup
//
typedef struct 
{
	UINT    style;
	LPCTSTR szName;

} StyleLookupType;

//
//	Extended Style table. 1 per window class
//
typedef struct 
{
	DWORD   style;		// Single window style
	LPCTSTR name;		// Textual name of style

	DWORD   cmp_mask;	// If zero, then -style- is treated as a single bit-field
						// Otherwise, cmp_mask is used to make sure that
						// ALL the bits in the mask match -style-

	DWORD   depends;	// Style is only valid if includes these styles
	DWORD   excludes;	// Style is only valid if these aren't set

} StyleLookupEx;

//
//	Use this helper-macro to fill in the first two members
//  of the style structures.
//
//  e.g. STYLE_(WS_CHILD)  ->  WS_CHILD, "WS_CHILD"
//
#define STYLE_(style) (UINT)style, _T(#style)

//
//	Use this structure to list each window class with its
//	associated style table
//
typedef struct 
{
	LPCTSTR        szClassName;
	StyleLookupEx  *stylelist;
	DWORD			dwData;
} ClassStyleLookup;

#define WINLIST_INCLUDE_HANDLE	1		// handle, classname
#define WINLIST_INCLUDE_CLASS   2		// classname, caption
#define WINLIST_INCLUDE_ALL		3		// handle, caption, classname

//
//	Useful functions!
//
BOOL FunkyList_MeasureItem(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis);
BOOL FunkyList_DrawItem   (HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis);

//
//	WinSpy layout functions
//
void ToggleWindowLayout(HWND hwnd);
void SetWindowLayout(HWND hwnd, UINT uLayout);
UINT GetWindowLayout(HWND hwnd);
void ForceVisibleDisplay(HWND hwnd);

#define WINSPY_MINIMIZED 1
#define WINSPY_NORMAL    2
#define WINSPY_EXPANDED  3
#define WINSPY_LASTMAX	 4	// Only use with SetWindowLayout 
							// (chooses between normal/expanded)

//
//	WinSpy message handler functions
//

UINT WinSpyDlg_Size				(HWND hwnd, WPARAM wParam, LPARAM lParam);
UINT WinSpyDlg_Sizing			(HWND hwnd, UINT nSide, RECT *prc);
UINT WinSpyDlg_WindowPosChanged	(HWND hwnd, WINDOWPOS *wp);
UINT WinSpyDlg_EnterSizeMove	(HWND hwnd);
UINT WinSpyDlg_ExitSizeMove		(HWND hwnd);
UINT_PTR WinSpyDlg_FullWindowDrag	(HWND hwnd, WPARAM wParam, LPARAM lParam);

UINT WinSpyDlg_CommandHandler(HWND hwnd, WPARAM wParam, LPARAM lParam);
UINT WinSpyDlg_SysMenuHandler(HWND hwnd, WPARAM wParam, LPARAM lParam);

void WinSpyDlg_SizeContents     (HWND hwnd);

UINT WinSpy_PopupCommandHandler (HWND hwndDlg, UINT uCmdId, HWND hwndTarget);
void WinSpy_SetupPopupMenu      (HMENU hMenu, HWND hwndTarget);


int WINAPI GetRectWidth (RECT *rect);
int WINAPI GetRectHeight(RECT *rect);

BOOL IsMinimized(HWND hwnd);

//
//	Dialog box procedures for each dialog tab.
//
LRESULT CALLBACK GeneralDlgProc	 (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK StyleDlgProc	 (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WindowDlgProc	 (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK PropertyDlgProc (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ProcessDlgProc	 (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ClassDlgProc	 (HWND hwnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

// Top-level
void DisplayWindowInfo  (HWND hwnd);

void SetWindowInfo		(HWND hwnd);
void SetClassInfo		(HWND hwnd);
void SetStyleInfo		(HWND hwnd);
void SetGeneralInfo		(HWND hwnd);
void SetScrollbarInfo	(HWND hwnd);
void SetPropertyInfo	(HWND hwnd);
void SetProcessInfo		(HWND hwnd);

void ExitWinSpy			(HWND hwnd, UINT uCode);

//
//	Menu and System Menu functions
//
//
void CheckSysMenu(HWND hwnd, UINT uItemId, BOOL fChecked);
void SetSysMenuIconFromLayout(HWND hwnd, UINT layout);

void ShowEditSizeDlg		(HWND hwndParent, HWND hwndTarget);
void ShowWindowStyleEditor  (HWND hwndParent, HWND hwndTarget, BOOL fExtended);
void ShowOptionsDlg			(HWND hwndParent);

void LoadSettings(void);
void SaveSettings(void);

BOOL GetRemoteWindowInfo(HWND hwnd, WNDCLASSEX *pClass, 
						 WNDPROC *pProc, TCHAR *pszText, int nTextLen);

BOOL RemoveTabCtrlFlicker(HWND hwndTab);

void VerboseClassName(TCHAR ach[]);

void RefreshTreeView(HWND hwndTree);
void InitGlobalWindowTree(HWND hwnd);
void DeInitGlobalWindowTree(HWND hwnd);
void InitStockStyleLists();
BOOL GetProcessNameByPid(DWORD dwProcessId, TCHAR szName[], DWORD nNameSize, TCHAR szPath[], DWORD nPathSize);

//
//	Pinned-window support
//
void GetPinnedPosition(HWND, POINT *);
BOOL WinSpy_ZoomTo(HWND hwnd, UINT uCorner);
void SetPinState(BOOL fPinned);

//
//	Pinned-window constants
//
#define PINNED_TOPLEFT      0		// notice the bit-positions:
#define PINNED_TOPRIGHT     1		// (bit 0 -> 0 = left, 1 = right)
#define PINNED_BOTTOMRIGHT  3		// (bit 1 -> 0 = top,  1 = bottom)
#define PINNED_BOTTOMLEFT   2

#define PINNED_NONE			PINNED_TOPLEFT
#define PINNED_LEFT			0
#define PINNED_RIGHT		1
#define PINNED_TOP			0
#define PINNED_BOTTOM		2

//
//	Global variables!! These just control WinSpy behaviour
//
extern BOOL fAlwaysOnTop;
extern BOOL fClassThenText;
extern BOOL fEnableToolTips;
extern BOOL fFullDragging;
extern BOOL fMinimizeWinSpy;
extern BOOL fPinWindow;
extern BOOL fShowDimmed;
extern BOOL fShowHidden;
extern BOOL fShowInCaption;
extern BOOL fSaveWinPos;
extern UINT uTreeInclude;

extern POINT ptPinPos;
extern UINT  uPinnedCorner;

//
//	Application global variables
//
extern HINSTANCE hInst;

extern TCHAR szHexFmt[];
extern TCHAR szAppName[];
extern TCHAR szVerString[];

extern HWND	 hwndPin;		// Toolbar with pin bitmap
extern HWND	 hwndSizer;		// Sizing grip for bottom-right corner
extern HWND	 hwndToolTip;	// tooltip for main window controls only

//
//	Spy-window globals
//	
//
extern HWND       spy_hCurWnd;
extern WNDCLASSEX spy_WndClassEx;
extern WNDPROC    spy_WndProc;
extern BOOL       spy_fPassword;
extern TCHAR      spy_szPassword[];
extern TCHAR      spy_szClassName[];

//
//	Useful SetWindowPos constants (saves space!)
//
#define SWP_SIZEONLY  (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE)
#define SWP_MOVEONLY  (SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE)
#define SWP_ZONLY     (SWP_NOSIZE | SWP_NOMOVE   | SWP_NOACTIVATE)
#define SWP_SHOWONLY  (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_SHOWWINDOW)
#define SWP_HIDEONLY  (SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_NOSIZE | SWP_HIDEWINDOW)

#ifdef __cplusplus
}
#endif

#endif