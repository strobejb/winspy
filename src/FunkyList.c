//
//	WinSpy Finder Tool.
//
//  Copyright (c) 2002 by J Brown 
//  Freeware
//
//	Nice-looking owner-drawn list (used for style-lists).
//

#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <tchar.h>

//
//	Called from WM_MEASUREITEM
//
BOOL FunkyList_MeasureItem(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis)
{
	mis->itemHeight -= 2;
	return TRUE;
}

//
//	Super owner-drawn list!
//
//	All we do is draw the list normally, but with a couple of minor changes:
//
//	Each list item will have it's user-defined dataitem set to the value
//  of each style.
//
//  If this style is zero, this means that it is an implicit style, so
//  draw the whole line gray.
//
//  Also, at the end of every line, right-align the hex-values of each style  
//
BOOL FunkyList_DrawItem(HWND hwnd, UINT uCtrlId, DRAWITEMSTRUCT *dis)
{
	HWND  hwndList = GetDlgItem(hwnd, uCtrlId);
	TCHAR szText[60];
	DWORD dwStyle;

	COLORREF crFG = GetTextColor(dis->hDC);
	COLORREF crBG = GetBkColor(dis->hDC);

	switch(dis->itemAction)
	{
	case ODA_FOCUS:
		DrawFocusRect(dis->hDC, &dis->rcItem);
		break;

	case ODA_SELECT:
	case ODA_DRAWENTIRE:

		// get the text string to display, and the item state.
		SendMessage(hwndList, LB_GETTEXT, dis->itemID, (LONG)szText);
		dwStyle = (DWORD)dis->itemData;
	
		if((dis->itemState & ODS_SELECTED))
		{
			SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
			SetBkColor(dis->hDC,   GetSysColor(COLOR_HIGHLIGHT));
		}
		else
		{
			// Make the item greyed-out if the style is zero
			if(dwStyle == 0)
				SetTextColor(dis->hDC, GetSysColor(COLOR_3DSHADOW));
			else
				SetTextColor(dis->hDC, GetSysColor(COLOR_WINDOWTEXT));
			
			SetBkColor(dis->hDC, GetSysColor(COLOR_WINDOW));
		}

		//draw the item text first of all. The ExtTextOut function also
		//lets us draw a rectangle under the text, so we use this facility
		//to draw the whole line at once.
		ExtTextOut(dis->hDC, 
			dis->rcItem.left + 2, 
			dis->rcItem.top + 0, 
			ETO_OPAQUE, &dis->rcItem, szText, lstrlen(szText), 0);

		//Draw the style bytes
		if((dis->itemState & ODS_SELECTED))
			SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
		else
			SetTextColor(dis->hDC, GetSysColor(COLOR_3DSHADOW));

		wsprintf(szText, _T("%08X"), dwStyle);

		dis->rcItem.right -= 4;

		DrawText(dis->hDC, szText, -1, &dis->rcItem, DT_RIGHT|DT_SINGLELINE|DT_VCENTER);

		dis->rcItem.right += 4;

		SetTextColor(dis->hDC, crFG);
		SetBkColor(dis->hDC, crBG);

		if(dis->itemState & ODS_FOCUS)
			DrawFocusRect(dis->hDC, &dis->rcItem);

		break;
	}


	return TRUE;
}
