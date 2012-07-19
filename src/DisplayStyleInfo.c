//
//	DisplayStyleInfo.c
//  Copyright (c) 2002 by J Brown 
//	Freeware
//
//  void SetStyleInfo(HWND hwnd)
//
//	 Fill the style-tab-pane with style info for the
//   specified window
//
//  void FillStyleLists(HWND hwndTarget, HWND hwndStyleList, HWND hwndExStyleList, 
//					BOOL fAllStyles, BOOL fExtControl)
//
//	 Fill the two list-boxes (hwndStyleList and hwndExStyleList)
//   with the appropriate styles for the specified target window.
//
//	 hwndTarget      - window to find styles for
//	 hwndStyleList   - listbox to receive standard styles
//   hwndExStyleList - listbox to receive extended styles
//  
//   fAllStyles      - FALSE - just adds the styles that are set
//                     TRUE  - adds ALL possible styles, but
//                             only selects those that are set
//
//   fExtControl     - include control-specific extended styles
//                     (i.e. LVS_EX_xxx styles, not present in
//                     the extended window styles
//
//
//	v1.6.1 - fixed small bug thanks to Holger Stenger
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>
#include <richedit.h>
#include "resource.h"
#include "WinSpy.h"

StyleLookupEx WindowStyles[] = 
{
	STYLE_(WS_OVERLAPPEDWINDOW),	0, -1, (WS_POPUP|WS_CHILD),  
	STYLE_(WS_POPUPWINDOW),			WS_POPUPWINDOW, -1, 0, 

	STYLE_(WS_OVERLAPPED),			0, -1, (WS_POPUP|WS_CHILD),	//0x00000000
	STYLE_(WS_POPUP),				0, -1, 0,					//0x80000000
	STYLE_(WS_CHILD),				0, -1, 0,					//0x40000000
	STYLE_(WS_MINIMIZE),			0, -1, 0,					//0x20000000
	STYLE_(WS_VISIBLE),				0, -1, 0,					//0x10000000
	STYLE_(WS_DISABLED),			0, -1, 0,					//0x08000000
	STYLE_(WS_CLIPSIBLINGS),		0, -1, 0,					//0x04000000
	STYLE_(WS_CLIPCHILDREN),		0, -1, 0,					//0x02000000
	STYLE_(WS_MAXIMIZE),			0, -1, 0,					//0x01000000

	STYLE_(WS_CAPTION),				0, -1, 0,					//0x00C00000

	//(BORDER|CAPTION)
	STYLE_(WS_DLGFRAME),			0, -1, 0,					//0x00400000
	
	STYLE_(WS_BORDER),				0, -1, 0,					//0x00800000
	
	STYLE_(WS_VSCROLL),				0, -1, 0,					//0x00200000
	STYLE_(WS_HSCROLL),				0, -1, 0,					//0x00100000
	STYLE_(WS_SYSMENU),				0, -1, 0,					//0x00080000
	STYLE_(WS_THICKFRAME),			0, -1, 0,					//0x00040000
	STYLE_(WS_GROUP),				0, -1, 0,					//0x00020000
	STYLE_(WS_TABSTOP),				0, -1, 0,					//0x00010000

	STYLE_(WS_MINIMIZEBOX),			0, WS_POPUPWINDOW|WS_OVERLAPPEDWINDOW|WS_CAPTION, 0, //0x00020000
	STYLE_(WS_MAXIMIZEBOX),			0, WS_POPUPWINDOW|WS_OVERLAPPEDWINDOW|WS_CAPTION, 0, //0x00010000

	-1, _T(""), -1, -1, -1
};

// Dialog box styles (class = #32770)
StyleLookupEx DialogStyles[] = 
{
	STYLE_(DS_ABSALIGN),			0, -1, 0,			//0x00000001
	STYLE_(DS_SYSMODAL),			0, -1, 0,			//0x00000002
	STYLE_(DS_LOCALEDIT),			0, -1, 0,			//0x00000020
	STYLE_(DS_SETFONT),				0, -1, 0,			//0x00000040
	STYLE_(DS_MODALFRAME),			0, -1, 0,			//0x00000080
	STYLE_(DS_NOIDLEMSG),			0, -1, 0,			//0x00000100
	STYLE_(DS_SETFOREGROUND),		0, -1, 0,			//0x00000200

#if(WINVER >= 0x0400)

	STYLE_(DS_3DLOOK),				0, -1, 0,			//0x00000004
	STYLE_(DS_FIXEDSYS),			0, -1, 0,			//0x00000008
	STYLE_(DS_NOFAILCREATE),		0, -1, 0,			//0x00000010
	STYLE_(DS_CONTROL),				0, -1, 0,			//0x00000400
	STYLE_(DS_CENTER),				0, -1, 0,			//0x00000800
	STYLE_(DS_CENTERMOUSE),			0, -1, 0,			//0x00001000
	STYLE_(DS_CONTEXTHELP),			0, -1, 0,			//0x00002000

#endif

	-1, _T(""), -1, -1, -1
};

// Button styles (Button)
StyleLookupEx ButtonStyles[] = 
{
	STYLE_(BS_PUSHBUTTON),			0,   -1, BS_DEFPUSHBUTTON|BS_CHECKBOX|BS_AUTOCHECKBOX|BS_RADIOBUTTON|BS_GROUPBOX|BS_AUTORADIOBUTTON, 
	STYLE_(BS_DEFPUSHBUTTON),		0xf, -1, 0,			//0x0001
	STYLE_(BS_CHECKBOX),			0xf, -1, 0,			//0x0002
	STYLE_(BS_AUTOCHECKBOX),		0xf, -1, 0,			//0x0003
	STYLE_(BS_RADIOBUTTON),			0xf, -1, 0,			//0x0004
	STYLE_(BS_3STATE),				0xf, -1, 0,			//0x0005
	STYLE_(BS_AUTO3STATE),			0xf, -1, 0,			//0x0006
	STYLE_(BS_GROUPBOX),			0xf, -1, 0,			//0x0007
	STYLE_(BS_USERBUTTON),			0xf, -1, 0,			//0x0008
	STYLE_(BS_AUTORADIOBUTTON),		0xf, -1, 0,			//0x0009
	STYLE_(BS_OWNERDRAW),			0xf, -1, 0,			//0x000B
	STYLE_(BS_LEFTTEXT),			0,   -1, 0,			//0x0020

	//winver >= 4.0 (index 42)
	STYLE_(BS_TEXT),				0, -1, (BS_ICON|BS_BITMAP|BS_AUTOCHECKBOX|BS_AUTORADIOBUTTON|BS_CHECKBOX|BS_RADIOBUTTON),//0x00000000
	STYLE_(BS_ICON),				0, -1, 0,			//0x0040	
	STYLE_(BS_BITMAP),				0, -1, 0,			//0x0080
	STYLE_(BS_LEFT),				0, -1, 0,			//0x0100
	STYLE_(BS_RIGHT),				0, -1, 0,			//0x0200
	STYLE_(BS_CENTER),				0, -1, 0,			//0x0300
	STYLE_(BS_TOP),					0, -1, 0,			//0x0400
	STYLE_(BS_BOTTOM),				0, -1, 0,			//0x0800
	STYLE_(BS_VCENTER),				0, -1, 0,			//0x0C00
	STYLE_(BS_PUSHLIKE),			0, -1, 0,			//0x1000
	STYLE_(BS_MULTILINE),			0, -1, 0,			//0x2000
	STYLE_(BS_NOTIFY),				0, -1, 0,			//0x4000
	STYLE_(BS_FLAT),				0, -1, 0,			//0x8000
	STYLE_(BS_RIGHTBUTTON),			0, -1, 0,			//BS_LEFTTEXT

	-1, _T(""), -1, -1, -1
};

// Edit styles (Edit)
StyleLookupEx EditStyles[] = 
{
	STYLE_(ES_LEFT),				0, -1, (ES_CENTER|ES_RIGHT),	//0x0000
	STYLE_(ES_CENTER),				0, -1, 0,						//0x0001
	STYLE_(ES_RIGHT),				0, -1, 0,						//0x0002
	STYLE_(ES_MULTILINE),			0, -1, 0,						//0x0004
	STYLE_(ES_UPPERCASE),			0, -1, 0,						//0x0008
	STYLE_(ES_LOWERCASE),			0, -1, 0,						//0x0010
	STYLE_(ES_PASSWORD),			0, -1, 0,						//0x0020
	STYLE_(ES_AUTOVSCROLL),			0, -1, 0,						//0x0040
	STYLE_(ES_AUTOHSCROLL),			0, -1, 0,						//0x0080
	STYLE_(ES_NOHIDESEL),			0, -1, 0,						//0x0100
	STYLE_(ES_OEMCONVERT),			0, -1, 0,						//0x0400
	STYLE_(ES_READONLY),			0, -1, 0,						//0x0800
	STYLE_(ES_WANTRETURN),			0, -1, 0,						//0x1000
	STYLE_(ES_NUMBER),				0, -1, 0,						//0x2000	

	-1, _T(""), -1, -1, -1
};

StyleLookupEx RichedStyles[] = 
{
	// Standard edit control styles
	STYLE_(ES_LEFT),				0, -1, (ES_CENTER|ES_RIGHT),	//0x0000
	STYLE_(ES_CENTER),				0, -1, 0,						//0x0001
	STYLE_(ES_RIGHT),				0, -1, 0,						//0x0002
	STYLE_(ES_MULTILINE),			0, -1, 0,						//0x0004
	//STYLE_(ES_UPPERCASE),			0, -1, 0,						//0x0008
	//STYLE_(ES_LOWERCASE),			0, -1, 0,						//0x0010
	STYLE_(ES_PASSWORD),			0, -1, 0,						//0x0020
	STYLE_(ES_AUTOVSCROLL),			0, -1, 0,						//0x0040
	STYLE_(ES_AUTOHSCROLL),			0, -1, 0,						//0x0080
	STYLE_(ES_NOHIDESEL),			0, -1, 0,						//0x0100
	//STYLE_(ES_OEMCONVERT),		0, -1, 0,						//0x0400
	STYLE_(ES_READONLY),			0, -1, 0,						//0x0800
	STYLE_(ES_WANTRETURN),			0, -1, 0,						//0x1000
	STYLE_(ES_NUMBER),				0, -1, 0,						//0x2000	

	// Addition Rich Edit control styles

	STYLE_(ES_SAVESEL),				0, -1, 0,				//0x00008000
	STYLE_(ES_SUNKEN),				0, -1, 0,				//0x00004000
	STYLE_(ES_DISABLENOSCROLL),		0, -1, 0,				//0x00002000
	STYLE_(ES_SELECTIONBAR),		0, -1, 0,				//0x01000000
	STYLE_(ES_NOOLEDRAGDROP),		0, -1, 0,				//0x00000008

	-1, _T(""), -1, -1, -1

};

// Combo box styles (combobox)
StyleLookupEx ComboStyles[] = 
{
	STYLE_(CBS_SIMPLE),				0x3, -1, 0,		//0x0001
	STYLE_(CBS_DROPDOWN),			0x3, -1, 0,		//0x0002
	STYLE_(CBS_DROPDOWNLIST),		0x3, -1, 0,		//0x0003
	STYLE_(CBS_OWNERDRAWFIXED),		0, -1, 0,		//0x0010
	STYLE_(CBS_OWNERDRAWVARIABLE),	0, -1, 0,		//0x0020
	STYLE_(CBS_AUTOHSCROLL),		0, -1, 0,		//0x0040
	STYLE_(CBS_OEMCONVERT),			0, -1, 0,		//0x0080
	STYLE_(CBS_SORT),				0, -1, 0,		//0x0100
	STYLE_(CBS_HASSTRINGS),			0, -1, 0,		//0x0200
	STYLE_(CBS_NOINTEGRALHEIGHT),	0, -1, 0,		//0x0400
	STYLE_(CBS_DISABLENOSCROLL),	0, -1, 0,		//0x0800
	
#if(WINVER >= 0x0400)
	STYLE_(CBS_UPPERCASE),			0, -1, 0,		//0x1000
	STYLE_(CBS_LOWERCASE),			0, -1, 0,		//0x2000
#endif

	-1, _T(""), -1, -1, -1
};

// Listbox styles (Listbox)
StyleLookupEx ListBoxStyles[] = 
{
	STYLE_(LBS_NOTIFY),				0, -1, 0,		//0x0001
	STYLE_(LBS_SORT),				0, -1, 0,		//0x0002
	STYLE_(LBS_NOREDRAW),			0, -1, 0,		//0x0004
	STYLE_(LBS_MULTIPLESEL),		0, -1, 0,		//0x0008
	STYLE_(LBS_OWNERDRAWFIXED),		0, -1, 0,		//0x0010
	STYLE_(LBS_OWNERDRAWVARIABLE),	0, -1, 0,		//0x0020
	STYLE_(LBS_HASSTRINGS),			0, -1, 0,		//0x0040
	STYLE_(LBS_USETABSTOPS),		0, -1, 0,		//0x0080
	STYLE_(LBS_NOINTEGRALHEIGHT),	0, -1, 0,		//0x0100
	STYLE_(LBS_MULTICOLUMN),		0, -1, 0,		//0x0200
	STYLE_(LBS_WANTKEYBOARDINPUT),	0, -1, 0,		//0x0400
	STYLE_(LBS_EXTENDEDSEL),		0, -1, 0,		//0x0800
	STYLE_(LBS_DISABLENOSCROLL),	0, -1, 0,		//0x1000
	STYLE_(LBS_NODATA),				0, -1, 0,		//0x2000
	STYLE_(LBS_NOSEL),				0, -1, 0,		//0x4000

	-1, _T(""), -1, -1, -1
};

// Scrollbar control styles (Scrollbar)
StyleLookupEx ScrollbarStyles[] = 
{
	STYLE_(SBS_TOPALIGN),					0, SBS_HORZ, 0,								//0x0002
	STYLE_(SBS_LEFTALIGN),					0, SBS_VERT, 0,								//0x0002
	STYLE_(SBS_BOTTOMALIGN),				0, SBS_HORZ|SBS_SIZEBOX|SBS_SIZEGRIP, 0,	//0x0004
	STYLE_(SBS_RIGHTALIGN),					0, SBS_VERT|SBS_SIZEBOX|SBS_SIZEGRIP, 0,	//0x0004
	STYLE_(SBS_HORZ),						0, -1, SBS_VERT|SBS_SIZEBOX|SBS_SIZEGRIP,	//0x0000
	STYLE_(SBS_VERT),						0, -1, SBS_SIZEBOX|SBS_SIZEGRIP,			//0x0001
	STYLE_(SBS_SIZEBOXTOPLEFTALIGN),		0, SBS_SIZEBOX|SBS_SIZEGRIP, 0,				//0x0002
	STYLE_(SBS_SIZEBOXBOTTOMRIGHTALIGN),	0, SBS_SIZEBOX|SBS_SIZEGRIP, 0,				//0x0004
	STYLE_(SBS_SIZEBOX),					0, -1, 0,									//0x0008
	STYLE_(SBS_SIZEGRIP),					0, -1, 0,									//0x0010

	-1, _T(""), -1, -1, -1
};

// Static control styles (Static)
StyleLookupEx StaticStyles[] = 
{
	STYLE_(SS_LEFT),				0x1f, -1,SS_CENTER|SS_RIGHT,//0x0000
	STYLE_(SS_CENTER),				0x1f, -1, 0,				//0x0001
	STYLE_(SS_RIGHT),				0x1f, -1, 0,				//0x0002
	STYLE_(SS_ICON),				0x1f, -1, 0,				//0x0003
	STYLE_(SS_BLACKRECT),			0x1f, -1, 0,				//0x0004
	STYLE_(SS_GRAYRECT),			0x1f, -1, 0,				//0x0005
	STYLE_(SS_WHITERECT),			0x1f, -1, 0,				//0x0006
	STYLE_(SS_BLACKFRAME),			0x1f, -1, 0,				//0x0007
	STYLE_(SS_GRAYFRAME),			0x1f, -1, 0,				//0x0008
	STYLE_(SS_WHITEFRAME),			0x1f, -1, 0,				//0x0009
	STYLE_(SS_USERITEM),			0x1f, -1, 0,				//0x000A
	STYLE_(SS_SIMPLE),				0x1f, -1, 0,				//0x000B
	STYLE_(SS_LEFTNOWORDWRAP),		0x1f, -1, 0,				//0x000C

	STYLE_(SS_OWNERDRAW),			0x1f, -1, 0,				//0x000D
	STYLE_(SS_BITMAP),				0x1f, -1, 0,				//0x000E
	STYLE_(SS_ENHMETAFILE),			0x1f, -1, 0,				//0x000F
	STYLE_(SS_ETCHEDHORZ),			0x1f, -1, 0,				//0x0010
	STYLE_(SS_ETCHEDVERT),			0x1f, -1, 0,				//0x0011
	STYLE_(SS_ETCHEDFRAME),			0x1f, -1, 0,				//0x0012
	STYLE_(SS_TYPEMASK),			0x1f, -1, 0,				//0x001F
	STYLE_(SS_NOPREFIX),			0,    -1, 0,				//0x0080
	
	STYLE_(SS_NOTIFY),				0,    -1, 0,				//0x0100
	STYLE_(SS_CENTERIMAGE),			0,    -1, 0,				//0x0200
	STYLE_(SS_RIGHTJUST),			0,    -1, 0,				//0x0400
	STYLE_(SS_REALSIZEIMAGE),		0,    -1, 0,				//0x0800
	STYLE_(SS_SUNKEN),				0,    -1, 0,				//0x1000
	STYLE_(SS_ENDELLIPSIS),			0,    -1, 0,				//0x4000
	STYLE_(SS_PATHELLIPSIS),		0,    -1, 0,				//0x8000
	STYLE_(SS_WORDELLIPSIS),		0,    -1, 0,				//0xC000
	STYLE_(SS_ELLIPSISMASK),		0,    -1, 0,				//0xC000

	-1, _T(""), -1, -1, -1
};

//	Standard Common controls styles	
StyleLookupEx CommCtrlList[] = 
{
	STYLE_(CCS_TOP),				0x3, -1, 0,			//0x0001
	STYLE_(CCS_NOMOVEY),			0x3, -1, 0,			//0x0002
	STYLE_(CCS_BOTTOM),				0x3, -1, 0,			//0x0003
	STYLE_(CCS_NORESIZE),			0,	 -1, 0,			//0x0004
	STYLE_(CCS_NOPARENTALIGN),		0,   -1, 0,			//0x0008
	
	STYLE_(CCS_ADJUSTABLE),			0,   -1, 0,			//0x0020
	STYLE_(CCS_NODIVIDER),			0,   -1, 0,			//0x0040

#if (_WIN32_IE >= 0x0300)
	STYLE_(CCS_VERT),				0,   -1, 0,			//0x0080
#endif

	-1, _T(""), -1, -1, -1
};

//  DragList - uses same styles as listview

// Header control (SysHeader32)
StyleLookupEx HeaderStyles[] = 
{
	STYLE_(HDS_HORZ),				0, -1, 16,			//0x0000
	STYLE_(HDS_BUTTONS),			0, -1, 0,			//0x0002

#if (_WIN32_IE >= 0x0300)
	STYLE_(HDS_HOTTRACK),			0, -1, 0,			//0x0004
	STYLE_(HDS_DRAGDROP),			0, -1, 0,			//0x0040
	STYLE_(HDS_FULLDRAG),			0, -1, 0,			//0x0080
#endif

	STYLE_(HDS_HIDDEN),				0, -1, 0,			//0x0008

#if (_WIN32_IE >= 0x0500)
	STYLE_(HDS_FILTERBAR),			0, -1, 0,			//0x0100
#endif

	-1, _T(""), -1, -1, -1
};

// Listview (SysListView32)
StyleLookupEx ListViewStyles[] = 
{
	STYLE_(LVS_ICON),		LVS_TYPEMASK, -1, LVS_REPORT|LVS_SMALLICON|LVS_LIST, //0x0000
	STYLE_(LVS_REPORT),		LVS_TYPEMASK, -1, 0,		//0x0001
	STYLE_(LVS_SMALLICON),	LVS_TYPEMASK, -1, 0,		//0x0002
	STYLE_(LVS_LIST),		LVS_TYPEMASK, -1, 0,		//0x0003

	STYLE_(LVS_SINGLESEL),			0,   -1, 0,		//0x0004
	STYLE_(LVS_SHOWSELALWAYS),		0,   -1, 0,		//0x0008
	STYLE_(LVS_SORTASCENDING),		0,   -1, 0,		//0x0010
	STYLE_(LVS_SORTDESCENDING),		0,   -1, 0,		//0x0020
	STYLE_(LVS_SHAREIMAGELISTS),	0,   -1, 0,		//0x0040
	STYLE_(LVS_NOLABELWRAP),		0,   -1, 0,		//0x0080
	STYLE_(LVS_AUTOARRANGE),		0,   -1, 0,		//0x0100
	STYLE_(LVS_EDITLABELS),			0,   -1, 0,		//0x0200

#if (_WIN32_IE >= 0x0300)
	STYLE_(LVS_OWNERDATA),			0, -1, 0,		//0x1000
#endif

	STYLE_(LVS_NOSCROLL),			0, -1, 0,		//0x2000

	STYLE_(LVS_ALIGNTOP),			0, -1, 0,		//0x0000
	STYLE_(LVS_ALIGNLEFT),	LVS_ALIGNMASK, -1, 0,	//0x0800

	//STYLE_(LVS_ALIGNMASK),			0, -1, 0,		//0x0c00
	//STYLE_(LVS_TYPESTYLEMASK),		0, -1, 0,		//0xfc00

	STYLE_(LVS_OWNERDRAWFIXED),		0, -1, 0,		//0x0400
	STYLE_(LVS_NOCOLUMNHEADER),		0, -1, 0,		//0x4000
	STYLE_(LVS_NOSORTHEADER),		0, -1, 0,		//0x8000

	-1, _T(""), -1, -1, -1
};

// Toolbar control (ToolbarWindow32)
StyleLookupEx ToolbarStyles[] = 
{
	STYLE_(TBSTYLE_TOOLTIPS),		0, -1, 0,		//0x0100
	STYLE_(TBSTYLE_WRAPABLE),		0, -1, 0,		//0x0200
	STYLE_(TBSTYLE_ALTDRAG),		0, -1, 0,		//0x0400

#if (_WIN32_IE >= 0x0300)
	STYLE_(TBSTYLE_FLAT),			0, -1, 0,		//0x0800
	STYLE_(TBSTYLE_LIST),			0, -1, 0,		//0x1000
	STYLE_(TBSTYLE_CUSTOMERASE),	0, -1, 0,		//0x2000
#endif

#if (_WIN32_IE >= 0x0400)
	STYLE_(TBSTYLE_REGISTERDROP),	0, -1, 0,		//0x4000
	STYLE_(TBSTYLE_TRANSPARENT),	0, -1, 0,		//0x8000
#endif

	-1, _T(""), -1, -1, -1
};

// Rebar control (RebarControl32)
StyleLookupEx RebarStyles[] = 
{
#if (_WIN32_IE >= 0x0400)
	STYLE_(RBS_TOOLTIPS),			0, -1, 0,		//0x0100
	STYLE_(RBS_VARHEIGHT),			0, -1, 0,		//0x0200
	STYLE_(RBS_BANDBORDERS),		0, -1, 0,		//0x0400
	STYLE_(RBS_FIXEDORDER),			0, -1, 0,		//0x0800
	STYLE_(RBS_REGISTERDROP),		0, -1, 0,		//0x1000
	STYLE_(RBS_AUTOSIZE),			0, -1, 0,		//0x2000
	STYLE_(RBS_VERTICALGRIPPER),	0, -1, 0,		//0x4000
	STYLE_(RBS_DBLCLKTOGGLE),		0, -1, 0,		//0x8000
#endif

	-1, _T(""), -1, -1, -1
};

// Track Bar control (msctls_trackbar32)
StyleLookupEx TrackbarStyles[] = 
{
	STYLE_(TBS_AUTOTICKS),			0xf, -1, 0,				//0x0001
	STYLE_(TBS_VERT),				0xf, -1, 0,				//0x0002
	STYLE_(TBS_HORZ),				0xf, -1, TBS_VERT,		//0x0000
	STYLE_(TBS_TOP),				0xf, -1, 0,				//0x0004
	STYLE_(TBS_BOTTOM),				0xf, -1, TBS_TOP,		//0x0000
	STYLE_(TBS_LEFT),				0xf, -1, 0,				//0x0004
	STYLE_(TBS_RIGHT),				0xf, -1, TBS_LEFT,		//0x0000
	STYLE_(TBS_BOTH),				0xf, -1, 0,				//0x0008

	STYLE_(TBS_NOTICKS),			0, -1, 0,				//0x0010
	STYLE_(TBS_ENABLESELRANGE),		0, -1, 0,				//0x0020
	STYLE_(TBS_FIXEDLENGTH),		0, -1, 0,				//0x0040
	STYLE_(TBS_NOTHUMB),			0, -1, 0,				//0x0080

#if (_WIN32_IE >= 0x0300)
	STYLE_(TBS_TOOLTIPS),			0, -1, 0,				//0x0100
#endif

#if (_WIN32_IE >= 0x0500)
	STYLE_(TBS_REVERSED),			0, -1, 0,				//0x0200  
#endif

	-1, _T(""), -1, -1, -1
};

// Treeview (SysTreeView32)
StyleLookupEx TreeViewStyles[] = 
{
	STYLE_(TVS_HASBUTTONS),			0, -1, 0,			//0x0001
	STYLE_(TVS_HASLINES),			0, -1, 0,			//0x0002
	STYLE_(TVS_LINESATROOT),		0, -1, 0,			//0x0004
	STYLE_(TVS_EDITLABELS),			0, -1, 0,			//0x0008
	STYLE_(TVS_DISABLEDRAGDROP),	0, -1, 0,			//0x0010
	STYLE_(TVS_SHOWSELALWAYS),		0, -1, 0,			//0x0020

#if (_WIN32_IE >= 0x0300)
	STYLE_(TVS_RTLREADING),			0, -1, 0,			//0x0040
	STYLE_(TVS_NOTOOLTIPS),			0, -1, 0,			//0x0080
	STYLE_(TVS_CHECKBOXES),			0, -1, 0,			//0x0100
	STYLE_(TVS_TRACKSELECT),		0, -1, 0,			//0x0200

#if (_WIN32_IE >= 0x0400)
	STYLE_(TVS_SINGLEEXPAND),		0, -1, 0,			//0x0400
	STYLE_(TVS_INFOTIP),			0, -1, 0,			//0x0800
	STYLE_(TVS_FULLROWSELECT),		0, -1, 0,			//0x1000
	STYLE_(TVS_NOSCROLL),			0, -1, 0,			//0x2000
	STYLE_(TVS_NONEVENHEIGHT),		0, -1, 0,			//0x4000

#if (_WIN32_IE >= 0x500)
	STYLE_(TVS_NOHSCROLL),			0, -1, 0,			//0x8000

#endif
#endif
#endif

	-1, _T(""), -1, -1, -1
};

// Tooltips (tooltips_class32)
StyleLookupEx ToolTipStyles[] = 
{
	STYLE_(TTS_ALWAYSTIP),			0, -1, 0,			//0x01
	STYLE_(TTS_NOPREFIX),			0, -1, 0,			//0x02

#if (_WIN32_IE >= 0x0500)
	STYLE_(TTS_NOANIMATE),			0, -1, 0,			//0x10
	STYLE_(TTS_NOFADE),				0, -1, 0,			//0x20
	STYLE_(TTS_BALLOON),			0, -1, 0,			//0x40
#endif

	-1, _T(""), -1, -1, -1
};

// Statusbar (msctls_statusbar32)
StyleLookupEx StatusBarStyles[] = 
{
	STYLE_(SBARS_SIZEGRIP),			0, -1, 0,			//0x0100

#if (_WIN32_IE >= 0x0400)
	STYLE_(SBT_TOOLTIPS),			0, -1, 0,			//0x0800
#endif

	-1, _T(""), -1, -1, -1
};

// Updown control
StyleLookupEx UpDownStyles[] = 
{
	STYLE_(UDS_WRAP),				0, -1, 0,			//0x0001
	STYLE_(UDS_SETBUDDYINT),		0, -1, 0,			//0x0002
	STYLE_(UDS_ALIGNRIGHT),			0, -1, 0,			//0x0004
	STYLE_(UDS_ALIGNLEFT),			0, -1, 0,			//0x0008
	STYLE_(UDS_AUTOBUDDY),			0, -1, 0,			//0x0010
	STYLE_(UDS_ARROWKEYS),			0, -1, 0,			//0x0020
	STYLE_(UDS_HORZ),				0, -1, 0,			//0x0040
	STYLE_(UDS_NOTHOUSANDS),		0, -1, 0,			//0x0080

#if (_WIN32_IE >= 0x0300)
	STYLE_(UDS_HOTTRACK),			0, -1, 0,			//0x0100
#endif

	-1, _T(""), -1, -1, -1
};

// Progress control (msctls_progress32)
StyleLookupEx ProgressStyles[] = 
{
#if (_WIN32_IE >= 0x0300)
	STYLE_(PBS_SMOOTH),				0, -1, 0,			//0x01
	STYLE_(PBS_VERTICAL),			0, -1, 0,			//0x04
#endif

	-1, _T(""), -1, -1, -1
};

// Tab control (SysTabControl32)
StyleLookupEx TabStyles[] = 
{
#if (_WIN32_IE >= 0x0300)
	STYLE_(TCS_SCROLLOPPOSITE),		0, -1, 0,			//0x0001   // assumes multiline tab
	STYLE_(TCS_BOTTOM),				0, TCS_VERTICAL, 0,	//0x0002
	STYLE_(TCS_RIGHT),				0, -1, 0,			//0x0002
	STYLE_(TCS_MULTISELECT),		0, -1, 0,			//0x0004  
#endif
#if (_WIN32_IE >= 0x0400)
	STYLE_(TCS_FLATBUTTONS),		0, -1, 0,			//0x0008
#endif
	STYLE_(TCS_FORCEICONLEFT),		0, -1, 0,			//0x0010
	STYLE_(TCS_FORCELABELLEFT),		0, -1, 0,			//0x0020
#if (_WIN32_IE >= 0x0300)
	STYLE_(TCS_HOTTRACK),			0, -1, 0,			//0x0040
	STYLE_(TCS_VERTICAL),			0, -1, 0,			//0x0080
#endif
	STYLE_(TCS_TABS),				0, -1,TCS_BUTTONS,	//0x0000
	STYLE_(TCS_BUTTONS),			0, -1, 0,			//0x0100
	STYLE_(TCS_SINGLELINE),			0, -1,TCS_MULTILINE,//0x0000
	STYLE_(TCS_MULTILINE),			0, -1, 0,			//0x0200
	STYLE_(TCS_RIGHTJUSTIFY),		0, -1, -1,			//0x0000
	STYLE_(TCS_FIXEDWIDTH),			0, -1, 0,			//0x0400
	STYLE_(TCS_RAGGEDRIGHT),		0, -1, 0,			//0x0800
	STYLE_(TCS_FOCUSONBUTTONDOWN),	0, -1, 0,			//0x1000
	STYLE_(TCS_OWNERDRAWFIXED),		0, -1, 0,			//0x2000
	STYLE_(TCS_TOOLTIPS),			0, -1, 0,			//0x4000
	STYLE_(TCS_FOCUSNEVER),			0, -1, 0,			//0x8000

	-1, _T(""), -1, -1, -1
};

// Animation control (SysAnimate32)
StyleLookupEx AnimateStyles[] = 
{
	STYLE_(ACS_CENTER),				0, -1, 0,			//0x0001
	STYLE_(ACS_TRANSPARENT),		0, -1, 0,			//0x0002
	STYLE_(ACS_AUTOPLAY),			0, -1, 0,			//0x0004

#if (_WIN32_IE >= 0x0300)
	STYLE_(ACS_TIMER),				0, -1, 0,			//0x0008
#endif

	-1, _T(""), -1, -1, -1
};

// Month-calendar control (SysMonthCal32)
StyleLookupEx MonthCalStyles[] =
{
	STYLE_(MCS_DAYSTATE),			0, -1, 0,			//0x0001
	STYLE_(MCS_MULTISELECT),		0, -1, 0,			//0x0002
	STYLE_(MCS_WEEKNUMBERS),		0, -1, 0,			//0x0004

#if (_WIN32_IE >= 0x0400)
	STYLE_(MCS_NOTODAYCIRCLE),		0, -1, 0,			//0x0008
	STYLE_(MCS_NOTODAY),			0, -1, 0,			//0x0010
#endif

	-1, _T(""), -1, -1, -1
};

// Date-Time picker (SysDateTimePick32)
StyleLookupEx DateTimeStyles[] = 
{
	STYLE_(DTS_UPDOWN),				0, -1, 0,			//0x0001
	STYLE_(DTS_SHOWNONE),			0, -1, 0,			//0x0002
	STYLE_(DTS_SHORTDATEFORMAT),	0, -1, DTS_LONGDATEFORMAT,//0x0000
	STYLE_(DTS_LONGDATEFORMAT),		0, -1, 0,			//0x0004 

#if (_WIN32_IE >= 0x500)
	STYLE_(DTS_SHORTDATECENTURYFORMAT),	0, -1, 0,		//0x000C
#endif

	STYLE_(DTS_TIMEFORMAT),			0, -1, 0,			//0x0009 
	STYLE_(DTS_APPCANPARSE),		0, -1, 0,			//0x0010 
	STYLE_(DTS_RIGHTALIGN),			0, -1, 0,			//0x0020 

	-1, _T(""), -1, -1, -1
};

// Pager control (SysPager)
StyleLookupEx PagerStyles[] = 
{
	//Pager control
	STYLE_(PGS_VERT),				0, -1, PGS_HORZ,	//0x0000
	STYLE_(PGS_HORZ),				0, -1, 0,			//0x0001
	STYLE_(PGS_AUTOSCROLL),			0, -1, 0,			//0x0002
	STYLE_(PGS_DRAGNDROP),			0, -1, 0,			//0x0004

	-1, _T(""), -1, -1, -1
};

// Extended window styles (for all windows)
StyleLookupEx StyleExList[] = 
{
	STYLE_(WS_EX_DLGMODALFRAME),	0, -1, 0,	//0x00000001L
	STYLE_(WS_EX_NOPARENTNOTIFY),	0, -1, 0,	//0x00000004L
	STYLE_(WS_EX_TOPMOST),			0, -1, 0,	//0x00000008L
	STYLE_(WS_EX_ACCEPTFILES),		0, -1, 0,	//0x00000010L
	STYLE_(WS_EX_TRANSPARENT),		0, -1, 0,	//0x00000020L

#if(WINVER >= 0x0400)

	STYLE_(WS_EX_MDICHILD),			0, -1, 0,	//0x00000040L
	STYLE_(WS_EX_TOOLWINDOW),		0, -1, 0,	//0x00000080L
	STYLE_(WS_EX_WINDOWEDGE),		0, -1, 0,	//0x00000100L
	STYLE_(WS_EX_CLIENTEDGE),		0, -1, 0,	//0x00000200L
	STYLE_(WS_EX_CONTEXTHELP),		0, -1, 0,	//0x00000400L

	STYLE_(WS_EX_LEFT),				0, -1, (WS_EX_RIGHT),		//0x00000000L
	STYLE_(WS_EX_RIGHT),			0, -1, 0,	//0x00001000L
	STYLE_(WS_EX_LTRREADING),		0, -1, (WS_EX_RTLREADING),	//0x00000000L
	STYLE_(WS_EX_RTLREADING),		0, -1, 0,	//0x00002000L
	STYLE_(WS_EX_LEFTSCROLLBAR),	0, -1, 0,	//0x00004000L
	STYLE_(WS_EX_RIGHTSCROLLBAR),	0, -1, (WS_EX_LEFTSCROLLBAR),//0x00000000L

	STYLE_(WS_EX_CONTROLPARENT),	0, -1, 0,	//0x00010000L
	STYLE_(WS_EX_STATICEDGE),		0, -1, 0,	//0x00020000L
	STYLE_(WS_EX_APPWINDOW),		0, -1, 0,	//0x00040000L

	STYLE_(WS_EX_OVERLAPPEDWINDOW),	0, -1, 0,	//(WS_EX_WINDOWEDGE | WS_EX_CLIENTEDGE)
	//STYLE_(WS_EX_PALETTEWINDOW),	0, -1, 0,	//(WS_EX_WINDOWEDGE | WS_EX_TOOLWINDOW | WS_EX_TOPMOST)

#endif

	-1, _T(""), -1, -1, -1
};

// ListView extended styles
StyleLookupEx ListViewExStyles[] = 
{
	//ListView control styles
	STYLE_(LVS_EX_GRIDLINES),			0, -1, 0,		//0x00000001
	STYLE_(LVS_EX_SUBITEMIMAGES),		0, -1, 0,		//0x00000002
	STYLE_(LVS_EX_CHECKBOXES),			0, -1, 0,		//0x00000004
	STYLE_(LVS_EX_TRACKSELECT),			0, -1, 0,		//0x00000008
	STYLE_(LVS_EX_HEADERDRAGDROP),		0, -1, 0,		//0x00000010
	STYLE_(LVS_EX_FULLROWSELECT),		0, -1, 0,		//0x00000020
	STYLE_(LVS_EX_ONECLICKACTIVATE),	0, -1, 0,		//0x00000040
	STYLE_(LVS_EX_TWOCLICKACTIVATE),	0, -1, 0,		//0x00000080
#if (_WIN32_IE >= 0x0400)
	STYLE_(LVS_EX_FLATSB),				0, -1, 0,		//0x00000100
	STYLE_(LVS_EX_REGIONAL),			0, -1, 0,		//0x00000200
	STYLE_(LVS_EX_INFOTIP),				0, -1, 0,		//0x00000400
	STYLE_(LVS_EX_UNDERLINEHOT),		0, -1, 0,		//0x00000800
	STYLE_(LVS_EX_UNDERLINECOLD),		0, -1, 0,		//0x00001000
	STYLE_(LVS_EX_MULTIWORKAREAS),		0, -1, 0,		//0x00002000
#endif
#if (_WIN32_IE >= 0x0500)
	STYLE_(LVS_EX_LABELTIP),			0, -1, 0,		//0x00004000
#endif

	-1, _T(""), -1, -1, -1
};

// ComboBoxEx extended styles	
StyleLookupEx ComboBoxExStyles[] = 
{
	STYLE_(CBES_EX_NOEDITIMAGE),		0, -1, 0,	//0x00000001
	STYLE_(CBES_EX_NOEDITIMAGEINDENT),	0, -1, 0,	//0x00000002
	STYLE_(CBES_EX_PATHWORDBREAKPROC),	0, -1, 0,	//0x00000004

#if(_WIN32_IE >= 0x0400)
	STYLE_(CBES_EX_NOSIZELIMIT),		0, -1, 0,	//0x00000008
	STYLE_(CBES_EX_CASESENSITIVE),		0, -1, 0,	//0x00000010
#endif

	-1, _T(""), -1, -1, -1
};

// Tab control extended styles
StyleLookupEx TabCtrlExStyles[] = 
{
	STYLE_(TCS_EX_FLATSEPARATORS),			0, -1, 0,	//0x00000001
	STYLE_(TCS_EX_REGISTERDROP),			0, -1, 0,	//0x00000002
	-1, _T(""), -1, -1, -1
};

// Toolbar extended styles
StyleLookupEx ToolBarExStyles[] =
{
#if (_WIN32_IE >= 0x0400)
	STYLE_(TBSTYLE_EX_DRAWDDARROWS),		0, -1, 0,	//0x0001

#if (_WIN32_IE >= 0x0501)
	STYLE_(TBSTYLE_EX_MIXEDBUTTONS),		0, -1, 0,	//0x0008
	STYLE_(TBSTYLE_EX_HIDECLIPPEDBUTTONS),	0, -1, 0,	//0x0010

#endif
#endif

	-1, _T(""), -1, -1, -1
};

// Support RichEdit Event masks!!!
StyleLookupEx RichedEventMask[] =
{
	STYLE_(ENM_NONE),				0, -1,-1,	//0x00000000
	STYLE_(ENM_CHANGE),				0, -1, 0,	//0x00000001
	STYLE_(ENM_UPDATE),				0, -1, 0,	//0x00000002
	STYLE_(ENM_SCROLL),				0, -1, 0,	//0x00000004
	STYLE_(ENM_KEYEVENTS),			0, -1, 0,	//0x00010000
	STYLE_(ENM_MOUSEEVENTS),		0, -1, 0,	//0x00020000
	STYLE_(ENM_REQUESTRESIZE),		0, -1, 0,	//0x00040000
	STYLE_(ENM_SELCHANGE),			0, -1, 0,	//0x00080000
	STYLE_(ENM_DROPFILES),			0, -1, 0,	//0x00100000
	STYLE_(ENM_PROTECTED),			0, -1, 0,	//0x00200000
	STYLE_(ENM_CORRECTTEXT),		0, -1, 0,	//0x00400000		// PenWin specific
	STYLE_(ENM_SCROLLEVENTS),		0, -1, 0,	//0x00000008
	STYLE_(ENM_DRAGDROPDONE),		0, -1, 0,	//0x00000010

	// Far East specific notification mask
	STYLE_(ENM_IMECHANGE),			0, -1, 0,	//0x00800000		// unused by RE2.0
	STYLE_(ENM_LANGCHANGE),			0, -1, 0,	//0x01000000
	STYLE_(ENM_OBJECTPOSITIONS),	0, -1, 0,	//0x02000000
	STYLE_(ENM_LINK),				0, -1, 0,	//0x04000000

	-1, _T(""), -1, -1, -1
};

//
//	Lookup table which matches window classnames to style-lists
//
ClassStyleLookup StandardControls[] = 
{
	_T("#32770"), 				DialogStyles,		0,
	_T("Button"),				ButtonStyles,		0,
	_T("ComboBox"),				ComboStyles,		0,
	_T("Edit"),					EditStyles,			0,
	_T("ListBox"),				ListBoxStyles,		0,

	_T("RICHEDIT"),				RichedStyles,		0,
	_T("RichEdit20A"),			RichedStyles,		0,
	_T("RichEdit20W"),			RichedStyles,		0,
	
	_T("Scrollbar"),			ScrollbarStyles,	0,
	_T("Static"),				StaticStyles,		0,

	_T("SysAnimate32"),			AnimateStyles,		0,
	_T("ComboBoxEx"),			ComboStyles,		0,	//(Just a normal combobox)
	_T("SysDateTimePick32"),	DateTimeStyles,		0,
	_T("DragList"),				ListBoxStyles,		0,	//(Just a normal list)
	_T("SysHeader32"),			HeaderStyles,		0,
	//"SysIPAddress32",			IPAddressStyles,	0,	(NO STYLES)
	_T("SysListView32"),		ListViewStyles,		0,
	_T("SysMonthCal32"),		MonthCalStyles,		0,
	_T("SysPager"),				PagerStyles,		0,
	_T("msctls_progress32"),	ProgressStyles,		0,
	_T("RebarWindow32"),		RebarStyles,		0,
	_T("msctls_statusbar32"),	StatusBarStyles,	0,
	//"SysLink",				SysLinkStyles,		0,  (DO IT!)
	_T("SysTabControl32"),		TabStyles,			0,
	_T("ToolbarWindow32"),		ToolbarStyles,		0,
	_T("tooltips_class32"),		ToolTipStyles,		0,
	_T("msctls_trackbar32"),	TrackbarStyles,		0,
	_T("SysTreeView32"),		TreeViewStyles,		0,
	_T("msctls_updown32"),		UpDownStyles,		0,

	_T(""),						0,					0,
};

// Classes which use the CCS_xxx styles
ClassStyleLookup CustomControls[] =
{	
	_T("msctls_statusbar32"),	CommCtrlList,		0,
	_T("RebarWindow32"),		CommCtrlList,		0,
	_T("ToolbarWindow32"),		CommCtrlList,		0,
	_T("SysHeader32"),			CommCtrlList,		0,

	_T(""),						0,					0,
};

// Classes which have extended window styles
ClassStyleLookup ExtendedControls[] =
{
	_T("SysTabControl32"),		TabCtrlExStyles,	TCM_GETEXTENDEDSTYLE,
	_T("ToolbarWindow32"),		ToolBarExStyles,	TB_GETEXTENDEDSTYLE,
	_T("ComboBox"),				ComboBoxExStyles,	CBEM_GETEXTENDEDSTYLE,
	_T("SysListView32"),		ListViewExStyles,	LVM_GETEXTENDEDLISTVIEWSTYLE,
	_T("RICHEDIT"),				RichedEventMask,    EM_GETEVENTMASK,
	_T("RichEdit20A"),			RichedEventMask,    EM_GETEVENTMASK,
	_T("RichEdit20W"),			RichedEventMask,    EM_GETEVENTMASK,

	_T(""),						0,					0,
};

//
// Match the window classname to a
//
// pClassList - a lookup table of classname / matching stylelist
// 
//
StyleLookupEx *FindStyleList(ClassStyleLookup *pClassList, TCHAR *szClassName, DWORD *pdwData)
{
	int i;

	for(i = 0; pClassList[i].stylelist != 0; i++)
	{
		if(lstrcmpi(szClassName, pClassList[i].szClassName) == 0)
		{
			if(pdwData) *pdwData = pClassList[i].dwData;
			return pClassList[i].stylelist;
		}
	}

	return 0;
}

#define NUM_CLASS_STYLELISTS	(sizeof(ClassStyleList) / sizeof(ClassStyleList[0]))

//
//	Find all the styles that match from the specified list
//
//	StyleList  - list of styles
//  hwndList   - listbox to add styles to
//  dwStyle    - style for the target window
//  fAllStyles - 
//
DWORD EnumStyles(StyleLookupEx *StyleList, HWND hwndList, DWORD dwStyle, BOOL fAllStyles)
{
	// Remember what the style is before we start modifying it
	DWORD dwOrig = dwStyle;
	
	int            i, idx;
	BOOL           fAddIt;
	StyleLookupEx *pStyle;

	//
	//	Loop through all of the styles that we know about
	//	Check each style against our window's one, to see
	//  if it is set or not
	//
	for(i = 0; StyleList[i].style != -1; i++)
	{
		fAddIt = FALSE;

		pStyle = &StyleList[i];

		// This style needs a mask to detect if it is set - 
		// i.e. the style doesn't just use one bit to decide if it is on/off.
		if(pStyle->cmp_mask != 0)
		{
			//if((StyleList[i].depends & dwStyle) != dwStyle)
			//	continue;

			// Style is valid if the excludes styles are not set
			if(pStyle->excludes != 0 && (pStyle->excludes & (dwOrig & pStyle->cmp_mask)) == 0)
				fAddIt = TRUE;

			// If the style matches exactly (when masked)
			if(pStyle->style != 0 && (pStyle->cmp_mask & dwStyle) == pStyle->style)
				fAddIt = TRUE;
		}
		else
		{
			// Style is valid when 
			if(pStyle->excludes != 0 && (pStyle->excludes & dwOrig) == 0)
				fAddIt = TRUE;

			// If style matches exactly (all bits are set
			if(pStyle->style != 0 && (pStyle->style & dwStyle) == pStyle->style)
				fAddIt = TRUE;
				
			// If no bits are set, then we have to skip it
			else if(pStyle->style != 0 && (pStyle->style & dwStyle) == 0)
				fAddIt = FALSE;
			
			// If this style depends on others being set..
			if(dwStyle &&  (pStyle->depends & dwStyle) == 0)
				fAddIt = FALSE;
		}

		// Now add the style..
		if(fAddIt == TRUE || fAllStyles)
		{
			// We've added this style, so remove it to stop it appearing again
			if(fAddIt)
				dwStyle &= ~ (pStyle->style);
			
			// Add to list, and set the list's extra item data to the style's value
			idx = (int)SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)pStyle->name);
			SendMessage(hwndList, LB_SETITEMDATA, idx, pStyle->style);

			if(fAllStyles)
				SendMessage(hwndList, LB_SETSEL, fAddIt, idx);
		}
	}

	// return the style. This will be zero if we decoded all the bits
	// that were set, or non-zero if there are still bits left
	return dwStyle;
}

//
//	This function takes HWNDs of two ListBoxes, which we will fill
//  with the style strings that are set for the specified window
//
//	Either hwndStyleList or hwndExStyleList can be NULL
//
void FillStyleLists(HWND hwndTarget, HWND hwndStyleList, HWND hwndExStyleList, BOOL fAllStyles, BOOL fExtControl)
{
	TCHAR szClassName[256];
	DWORD dwStyle;
	DWORD dwStyleEx;
	DWORD dwMessage;

	StyleLookupEx *StyleList;

	//window class
	GetClassName(hwndTarget, szClassName, sizeof(szClassName) / sizeof(TCHAR));

	//normal window styles
	dwStyle = GetWindowLong(hwndTarget, GWL_STYLE);
	
	if(hwndStyleList != 0)
	{
		// Empty the list
		SendMessage(hwndStyleList, LB_RESETCONTENT, 0, 0);
		
		// enumerate the standard window styles, for any window no 
		// matter what class it might be
		dwStyle = EnumStyles(WindowStyles, hwndStyleList, dwStyle, fAllStyles);
		
		// if the window class is one we know about, then see if we
		// can decode any more style bits
		// enumerate the custom control styles
		StyleList = FindStyleList(StandardControls, szClassName, 0);
		if(StyleList != 0)
			dwStyle = EnumStyles(StyleList, hwndStyleList, dwStyle, fAllStyles);
		
		// does the window support the CCS_xxx styles (custom control styles)
		StyleList = FindStyleList(CustomControls, szClassName, 0);
		if(StyleList != 0)
			dwStyle = EnumStyles(StyleList, hwndStyleList, dwStyle, fAllStyles);

		// if there are still style bits set in the window style,
		// then there is something that we can't decode. Just display
		// a single HEX entry at the end of the list.
		if(dwStyle != 0)
		{
			int idx;
			TCHAR ach[10];
			
			wsprintf(ach, szHexFmt, dwStyle);
			idx = (int)SendMessage(hwndStyleList, LB_ADDSTRING, 0, (LPARAM)ach);
			SendMessage(hwndStyleList, LB_SETITEMDATA, idx, dwStyle);

			if(fAllStyles)
				SendMessage(hwndStyleList, LB_SETSEL, TRUE, idx);
		}
		
	}

	// Extended window styles
	if(hwndExStyleList != 0)
	{
		// Empty the list
		SendMessage(hwndExStyleList, LB_RESETCONTENT, 0, 0);
		
		// Find extended styles
		dwStyleEx = GetWindowLong(hwndTarget, GWL_EXSTYLE);
		
		EnumStyles(StyleExList, hwndExStyleList, dwStyleEx, fAllStyles);
		
		// Does this window use any custom control extended styles???
		// If it does, then dwMessage will contain the message identifier to send
		// to the window to retrieve them
		if(fExtControl)
		{
			StyleList = FindStyleList(ExtendedControls, szClassName, &dwMessage);

			// Add them if required
			if(StyleList != 0)
			{
				dwStyleEx = (DWORD)SendMessage(hwndTarget, dwMessage, 0, 0);
				EnumStyles(StyleList, hwndExStyleList, dwStyleEx, fAllStyles);
			}
		}
	}
}

//
//	Update the Style tab with styles for specified window
//
void SetStyleInfo(HWND hwnd)
{
	TCHAR ach[12];
	DWORD dwStyle;

	HWND hwndDlg = WinSpyTab[STYLE_TAB].hwnd;
	HWND hwndStyle, hwndStyleEx;

	if(hwnd == 0) return;

	// Display the window style in static label
	dwStyle = GetWindowLong(hwnd, GWL_STYLE);
	wsprintf(ach, szHexFmt, dwStyle);
	SetDlgItemText(hwndDlg, IDC_STYLE, ach);

	// Display the extended window style in static label
	dwStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
	wsprintf(ach, szHexFmt, dwStyle);
	SetDlgItemText(hwndDlg, IDC_STYLEEX, ach);
	
	// Find handles to standard and extended style lists
	hwndStyle   = GetDlgItem(hwndDlg, IDC_LIST1);
	hwndStyleEx = GetDlgItem(hwndDlg, IDC_LIST2);

	// Fill both lists with their styles!
	FillStyleLists(hwnd, hwndStyle, hwndStyleEx, FALSE, TRUE);
}
