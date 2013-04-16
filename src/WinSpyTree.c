//
//	WinSpyTree.c
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Populate the treeview control on the main 
//  window with the system window hierarchy.
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <commctrl.h>
#include <shellapi.h>
#include <malloc.h>

#include "resource.h"
#include "WinSpy.h"

#pragma comment(lib, "comctl32.lib")

static BOOL       fShowNoTitle = FALSE;
static HIMAGELIST hImgList = 0;


//
//	Treeview image indices
//
#define DESKTOP_IMAGE	  0			// general images indices
#define WINDOW_IMAGE	  1
#define DIALOG_IMAGE	  2
#define CHILD_IMAGE		  3
#define POPUP_IMAGE		  4
#define CONTROL_START     5			// where the control images start
#define NUM_CLASS_BITMAPS 35		// (35 for visible, another 35 for invisible windows)

//
//	Use this structure+variables to help us popuplate the treeview
//
#define MAX_WINDOW_DEPTH 500

typedef struct
{
	HTREEITEM hRoot;
	HWND	  hwnd;

}  WinStackType;

typedef struct
{
	DWORD		 dwProcessId;
	HTREEITEM    hRoot;			    //Main root. Not used?

	WinStackType windowStack[MAX_WINDOW_DEPTH];
	int          nWindowZ;			//Current position in the window stack

} WinProc;

static WinProc *WinStackList;
int WinStackCount;

//static WinStackType WindowStack[MAX_WINDOW_DEPTH];
//static int          nWindowZ = 0;		//Current position in the window stack
//static HTREEITEM    hRoot;			    //Main root. Not used?

//
//	Define a lookup table, of windowclass to image index
//
typedef struct
{
	LPCTSTR szName;			// Class name
	int   index;			// Index into image list
	ATOM  atom;             // (Unused) Might use for fast lookups. 

    DWORD  dwAdjustStyles;	// Only valid if one of these styles is set
							// Default = 0 (don't care)

	DWORD  dwMask;			// Compare

}  ClassImageLookup;

ClassImageLookup ClassImage[] = 
{
	_T("#32770"),               0,  0, 0, 0,
	_T("Button"),               4,  0, BS_GROUPBOX,			0xF,
	_T("Button"),				2,  0, BS_CHECKBOX,			0xF,
	_T("Button"),				2,  0, BS_AUTOCHECKBOX,		0xF,
	_T("Button"),				2,  0, BS_AUTO3STATE,		0xF,
	_T("Button"),				2,  0, BS_3STATE,			0xF,
	_T("Button"),				3,  0, BS_RADIOBUTTON,		0xF,
	_T("Button"),				3,  0, BS_AUTORADIOBUTTON,	0xF,
	_T("Button"),               1,  0, 0, 0,	// (default push-button)
	_T("ComboBox"),             5,  0, 0, 0,
	_T("Edit"),                 6,  0, 0, 0,
	_T("ListBox"),              7,  0, 0, 0,
	
	_T("RICHEDIT"),				8,  0, 0, 0,
	_T("RichEdit20A"),			8,  0, 0, 0,
	_T("RichEdit20W"),			8,  0, 0, 0,

	_T("Scrollbar"),			9,  0, SBS_VERT, 0,
	_T("Scrollbar"),			11, 0, SBS_SIZEBOX | SBS_SIZEGRIP, 0,
	_T("Scrollbar"),			10, 0, 0, 0,  // (default horizontal)
	_T("Static"),				12, 0, 0, 0,

	_T("SysAnimate32"),			13, 0, 0, 0,
	_T("SysDateTimePick32"),	14, 0, 0, 0,
	_T("SysHeader32"),			15, 0, 0, 0,
	_T("IPAddress"),			16, 0, 0, 0,
	_T("SysListView32"),		17, 0, 0, 0,
	_T("SysMonthCal32"),		18, 0, 0, 0,
	_T("SysPager"),				19, 0, 0, 0,
	_T("msctls_progress32"),	20, 0, 0, 0,
	_T("ReBarWindow32"),		21, 0, 0, 0,
	_T("msctls_statusbar32"),	22, 0, 0, 0,
	_T("SysLink"),				23, 0, 0, 0,
	_T("SysTabControl32"),		24, 0, 0, 0,
	_T("ToolbarWindow32"),		25, 0, 0, 0,
	_T("tooltips_class32"),		26, 0, 0, 0,
	_T("msctls_trackbar32"),	27, 0, 0, 0,
	_T("SysTreeView32"),		28, 0, 0, 0,
	_T("msctls_updown32"),		29, 0, 0, 0,

	_T(""), 0, 0, 0,
};

//
//	Not used at present (doesn't work!)
//
//  Could be used to perform fast classname lookups, if we
//  precalculate all the class atoms, then all we need to
//  do is find the class-atom for a window, then match it in
//  our lookup table above (instead of doing slow string-searches).
//
void InitAtomList()
{
	int		i;
	ATOM    atom;

	INITCOMMONCONTROLSEX ice;

	ice.dwSize = sizeof(ice);
	ice.dwICC  = ICC_COOL_CLASSES ;//-1;	//all classes

	i=InitCommonControlsEx(&ice);

	for(i = 0; ClassImage[i].szName[0] != 0; i++)
	{
		atom = GlobalFindAtom(ClassImage[i].szName);
		
		ClassImage[i].atom = atom;
	}
}

//
//	Find the image index (in TreeView imagelist), given a
//  window classname. dwStyle lets us differentiate further
//  when we find a match.
//
int IconFromClassName(TCHAR *szName, DWORD dwStyle)
{
	int i = 0;

	while(ClassImage[i].szName[0] != _T('\0'))
	{
		if(lstrcmpi(ClassImage[i].szName, szName) == 0)
		{
			DWORD dwMask = ClassImage[i].dwMask;

			if(ClassImage[i].dwAdjustStyles != 0)
			{
				if(dwMask != 0)
				{
					if(ClassImage[i].dwAdjustStyles == (dwStyle & dwMask)) 
						return  (ClassImage[i].index + CONTROL_START);
				}
				else
				{
					if(ClassImage[i].dwAdjustStyles & dwStyle)
						return  (ClassImage[i].index + CONTROL_START);
				}

			}

			if(ClassImage[i].dwAdjustStyles == 0)
				return  (ClassImage[i].index + CONTROL_START);
		}

		i++;
	}
	
	return -1;
}

#define MAX_VERBOSE_LEN 22
#define MAX_CLASS_LEN   40
#define MAX_WINTEXT_LEN 200

#define MIN_FORMAT_LEN  (32 + MAX_VERBOSE_LEN + MAX_CLASS_LEN + MAX_WINTEXT_LEN)

//
//	szTotal must be MIN_FORMAT_LEN characters
//
int FormatWindowText(HWND hwnd, TCHAR szTotal[])
{
	static TCHAR szClass[MAX_CLASS_LEN + MAX_VERBOSE_LEN];
	int idx;
	TCHAR *pszCaption;
	DWORD dwStyle;
	int len;
	DWORD_PTR dwResult;

	//
	// Window handle in hex format
	//
	if(uTreeInclude & WINLIST_INCLUDE_HANDLE)
	{
		wsprintf(szTotal, _T("%08X  "), hwnd);
	}
	else
	{
		szTotal[0] = _T('\0');
	}

	//
	// Window class name
	//
	GetClassName(hwnd, szClass, MAX_CLASS_LEN);

	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	idx = IconFromClassName(szClass, dwStyle);

	if(uTreeInclude & WINLIST_INCLUDE_CLASS)
	{
		VerboseClassName(szClass);

		if(fClassThenText)
		{
			lstrcat(szTotal, szClass);
			lstrcat(szTotal, _T("  "));
		}
	}
	else
	{	
		szClass[0] = _T('\0');
	}	

	len = lstrlen(szTotal);
	pszCaption = szTotal + len;

	*pszCaption++ = _T('\"');
	*pszCaption   = _T('\0');

	// Window title, enclosed in quotes
	// if(GetWindowText(hwnd, temp, sizeof(temp) / sizeof(TCHAR)))
	SendMessageTimeout(
		hwnd, 
		WM_GETTEXT, 
		MAX_WINTEXT_LEN, 
		(LPARAM)pszCaption,
		SMTO_ABORTIFHUNG, 100, &dwResult);
	//{
		// add on the last quote
		lstrcat(pszCaption, _T("\""));
	/*}
	else if(fShowNoTitle)
	{
		lstrcpy(pszCaption, _T("<no title>"));
	}
	else
	{
		lstrcpy(pszCaption, _T("\"\""));
	}*/

	if(!fClassThenText)
	{
		lstrcat(szTotal, _T("  "));
		lstrcat(szTotal, szClass);
	}

	return idx;
}

//
//
//
WinProc *GetProcessWindowStack(HWND hwndTree, HWND hwnd)
{
	DWORD			pid;
	int				i;
	TVINSERTSTRUCT	tv;
	TCHAR			ach[MIN_FORMAT_LEN];
	TCHAR			name[100] = _T("");
	TCHAR			path[MAX_PATH] = _T("");
	SHFILEINFO		shfi = { 0 };
	HIMAGELIST		hImgList;
	HTREEITEM		hRoot;

	GetWindowThreadProcessId(hwnd, &pid);

	//
	// look for an existing process/window stack:
	//
	for(i = 0; i < WinStackCount; i++)
	{
		if(WinStackList[i].dwProcessId == pid)
			return &WinStackList[i];
	}


	//
	// couldn't find one - build a new one instead
	//
	GetProcessNameByPid(pid, name, 100, path, MAX_PATH);
	wsprintf(ach, _T("%s  (%d)"), name, pid);

	SHGetFileInfo(path, 0, &shfi, sizeof(shfi), SHGFI_SMALLICON | SHGFI_ICON);
	hImgList = TreeView_GetImageList(hwndTree, TVSIL_NORMAL);
	
	
	// Add the root item
	tv.hParent              = TVI_ROOT;
	tv.hInsertAfter         = TVI_LAST;
	tv.item.mask            = TVIF_STATE|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tv.item.state           = 0;//TVIS_EXPANDED;
	tv.item.stateMask       = 0;//TVIS_EXPANDED;
	tv.item.pszText         = ach;
	tv.item.cchTextMax      = lstrlen(ach);
	tv.item.iImage          = ImageList_AddIcon(hImgList, shfi.hIcon);//DESKTOP_IMAGE;
	tv.item.iSelectedImage  = tv.item.iImage;
	tv.item.lParam          = (LPARAM)GetDesktopWindow();

	hRoot = TreeView_InsertItem(hwndTree, &tv);
	WinStackList[WinStackCount].hRoot		= hRoot;//TVI_ROOT ;
	WinStackList[WinStackCount].dwProcessId = pid;
	WinStackList[WinStackCount].nWindowZ	= 1;
	WinStackList[WinStackCount].windowStack[0].hRoot = hRoot;
	WinStackList[WinStackCount].windowStack[0].hwnd  = 0;

	return &WinStackList[WinStackCount++];
}

//
// Callback function which is called once for every window in
// the system. We have to work out whereabouts in the treeview
// to insert each window
//
BOOL CALLBACK AllWindowProc(HWND hwnd, LPARAM lParam)
{
	HWND hwndTree = (HWND)lParam;
	
	static TCHAR szTotal[MIN_FORMAT_LEN];
	
	int i, idx;

	// Style is used to decide which bitmap to display in the tree
	UINT uStyle = GetWindowLong(hwnd, GWL_STYLE);

	// Need to know the current window's parent, so we know
	// where to insert this window
	HWND  hwndParent = GetParent(hwnd);

	// Keep track of the last window to be inserted, so
	// we know the z-order of the current window
	static HTREEITEM hTreeLast;
	static HWND		 hwndLast;

	TVINSERTSTRUCT tv;

	//
	//	
	//
	//
	WinProc *winProc = GetProcessWindowStack(hwndTree, hwnd);
	WinStackType *WindowStack = winProc->windowStack;


	idx = FormatWindowText(hwnd, szTotal);
	
	// Prepare the TVINSERTSTRUCT object
	ZeroMemory(&tv, sizeof(tv));
	tv.hParent         = winProc->hRoot;
	tv.hInsertAfter    = TVI_LAST;
	tv.item.mask       = TVIF_TEXT | TVIF_IMAGE | TVIF_SELECTEDIMAGE | TVIF_PARAM;
	tv.item.pszText    = szTotal;
	tv.item.cchTextMax = lstrlen(szTotal); 
	tv.item.lParam     = (LPARAM)(hwnd);

	//
	// set the image, depending on what type of window we have
	//
	if(uStyle & WS_CHILD)
	{
		// child windows (edit boxes, list boxes etc)
		tv.item.iImage  = CHILD_IMAGE;
		tv.hInsertAfter = TVI_LAST;
	}
	else if((uStyle & WS_POPUPWINDOW) == WS_POPUPWINDOW)
	{
		// dialog boxes
		tv.item.iImage  = DIALOG_IMAGE;
		tv.hInsertAfter = TVI_FIRST;
	}
	else if(uStyle & WS_POPUP)
	{
		// popup windows (tooltips etc)
		tv.item.iImage = POPUP_IMAGE;
		tv.hInsertAfter = TVI_LAST;
	}
	else
	{
		// anything else must be a top-level window
		tv.item.iImage = WINDOW_IMAGE;
		tv.hInsertAfter = TVI_FIRST;
	}

	if(idx != -1)
		tv.item.iImage = idx;

	if(fShowDimmed && !IsWindowVisible(hwnd) && hwnd != hwndTree)
		tv.item.iImage += NUM_CLASS_BITMAPS;

	//set the selected bitmap to be the same
	tv.item.iSelectedImage = tv.item.iImage;
	
	//
	// Decide where to place this item
	//
	//	If this window is in a different Z-order than the last one, then
	//  we need to either start a sub-hierarchy (if it is a child),
	//	or return back up the existing hierarchy.
	//
	if(winProc->nWindowZ > 0 && hwndParent != WindowStack[winProc->nWindowZ - 1].hwnd)
	{
		//we have another child window
		if(GetParent(hwnd) == hwndLast)
		{
			//make a new parent stack entry
			WindowStack[winProc->nWindowZ].hRoot = hTreeLast;
			WindowStack[winProc->nWindowZ].hwnd  = hwndParent;
			
			if(winProc->nWindowZ < MAX_WINDOW_DEPTH-1) 
				winProc->nWindowZ++;

			tv.hParent = hTreeLast;
		}
		//moving back?????
		else
		{
			//search for this window's parent in the stack so
			//we know where to insert this window under
			for(i = 0; i < winProc->nWindowZ; i++)
			{
				if(WindowStack[i].hwnd == hwndParent)
				{
					winProc->nWindowZ = i + 1;
					tv.hParent = WindowStack[i].hRoot;
				}
			}
		}
	}
	// otherwise, this window is a sibling to the last one, so just append
	// it to the treeview, in the same "z-order"
	else
	{
		tv.hParent = WindowStack[winProc->nWindowZ - 1].hRoot;
	}
	
	// Finally add the node
	hTreeLast = TreeView_InsertItem(hwndTree, &tv);
	hwndLast  = hwnd;
	
	return TRUE;
}

//
//	Populate the treeview control by using EnumChildWindows,
//  starting from the desktop window
//	HWND - handle to the dialog containing the tree
//
void FillGlobalWindowTree(HWND hwnd)
{
	HWND hwndTree = GetDlgItem(hwnd, IDC_TREE1);
	TVINSERTSTRUCT tv;
	static TCHAR ach[MIN_FORMAT_LEN];

	FormatWindowText(GetDesktopWindow(), ach);
	
	//Add the root item
	tv.hParent              = TVI_ROOT;
	tv.hInsertAfter         = TVI_LAST;
	tv.item.mask            = TVIF_STATE|TVIF_TEXT|TVIF_IMAGE|TVIF_SELECTEDIMAGE|TVIF_PARAM;
	tv.item.state           = TVIS_EXPANDED;
	tv.item.stateMask       = TVIS_EXPANDED;
	tv.item.pszText         = ach;
	tv.item.cchTextMax      = lstrlen(ach);
	tv.item.iImage          = DESKTOP_IMAGE;
	tv.item.iSelectedImage  = DESKTOP_IMAGE;
	tv.item.lParam          = (LPARAM)GetDesktopWindow();
	//tv.itemex.iIntegral = 1;

	//hRoot = TreeView_InsertItem(hwndTree, &tv);
	
	//WindowStack[0].hRoot = hRoot;
	//WindowStack[0].hwnd = 0;

	//nWindowZ = 1;

	// EnumChildWindows does the hard work for us
	// 
	EnumChildWindows(GetDesktopWindow(), AllWindowProc, (LPARAM)hwndTree);
}

//
//	Initialize the TreeView resource
//
void InitGlobalWindowTree(HWND hwndTree)
{
	HBITMAP hBitmap;
	TCITEM  tcitem;
	HWND    hwndTab;

	//only need to create the image list once.
	if(hImgList == 0)
	{
		// Create an empty image list
		hImgList = ImageList_Create(16,16,ILC_COLOR32 /*ILC_COLORDDB*/|ILC_MASK,NUM_CLASS_BITMAPS,8);
	
		// Load our bitmap and add it to the image list
		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_WINDOW_VISIBLE));
		ImageList_AddMasked(hImgList,hBitmap,RGB(255,0,255));
		DeleteObject(hBitmap);

		hBitmap = LoadBitmap(hInst, MAKEINTRESOURCE(IDB_WINDOW_INVISIBLE));
		ImageList_AddMasked(hImgList,hBitmap,RGB(255,0,255));
		DeleteObject(hBitmap);
	
		// Assign the image list to the treeview control
		TreeView_SetImageList(hwndTree, hImgList, TVSIL_NORMAL);
	}

	//add an item to the tab control
	ZeroMemory(&tcitem, sizeof(tcitem));
	tcitem.mask = TCIF_TEXT;
	tcitem.pszText = _T("All Windows");

	hwndTab = GetDlgItem(GetParent(hwndTree), IDC_TAB2);
	SendMessage(hwndTab, TCM_INSERTITEM, 0, (LPARAM)&tcitem);

	//subclass the tab control to remove flicker whilst it is resized
	RemoveTabCtrlFlicker(hwndTab); 

	//InitAtomList();
}

//
//	Clean up TreeView resources
//
void DeInitGlobalWindowTree(HWND hwndTree)
{
	TreeView_SetImageList(hwndTree, 0, TVSIL_NORMAL);
	ImageList_Destroy(hImgList);
}

//
//	Find the specified window (hwndTarget) in the TreeView.
//  Set hItem = NULL to start.
//
HTREEITEM FindTreeItemByHwnd(HWND hwndTree, HWND hwndTarget, HTREEITEM hItem)
{
	if(!hwndTarget)
		return NULL;

	// Start at root if necessary
	if(hItem == NULL)
		hItem = (HTREEITEM)SendMessage(hwndTree, TVM_GETNEXTITEM, TVGN_ROOT, 0);

	while(hItem != NULL)
	{
		TVITEM item;

		item.hItem  = hItem;
		item.mask   = TVIF_PARAM | TVIF_CHILDREN;
		item.lParam = (LPARAM)hwndTarget;
		
		// Search!
		SendMessage(hwndTree, TVM_GETITEM, 0, (LPARAM)&item);

		// Did we find it??
		if(item.lParam == (LPARAM)hwndTarget)
			return hItem;

		if(item.cChildren != 0)
		{
			// Recursively traverse child items.
            HTREEITEM hItemFound, hItemChild;

            hItemChild = (HTREEITEM)SendMessage(hwndTree, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)hItem);

            hItemFound = FindTreeItemByHwnd(hwndTree, hwndTarget, hItemChild);

            // Did we find it?
            if (hItemFound != NULL)
                return hItemFound;
        }

        // Go to next sibling item.
        hItem = (HTREEITEM)SendMessage(hwndTree, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)hItem);
    }

    // Not found.
    return NULL;
}

//
//	Update the TreeView with current window list
//
void RefreshTreeView(HWND hwndTree)
{
	DWORD dwStyle;

	WinStackList  = (WinProc*)malloc(1000 * sizeof(WinStackList[0]));
	WinStackCount = 0;

	EnableWindow(hwndTree, TRUE);
		
	dwStyle = GetWindowLong(hwndTree, GWL_STYLE);
		
	// We need to hide the treeview temporarily (turn OFF WS_VISIBLE)
	SendMessage(hwndTree, WM_SETREDRAW, FALSE, 0);
	SetWindowLong(hwndTree, GWL_STYLE, dwStyle & ~WS_VISIBLE);
		
	TreeView_DeleteAllItems(hwndTree);
		
	FillGlobalWindowTree(GetParent(hwndTree));

	
	SendMessage(hwndTree, WM_SETREDRAW, TRUE, 0);
	dwStyle = GetWindowLong(hwndTree, GWL_STYLE);
	SetWindowLong(hwndTree, GWL_STYLE, dwStyle | WS_VISIBLE);

		
	SetWindowPos(hwndTree, 0, 0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|
		SWP_NOZORDER|SWP_NOACTIVATE|SWP_DRAWFRAME);

	InvalidateRect(hwndTree, 0, TRUE);

	//free(WinStackList);
	WinStackList = 0;
}