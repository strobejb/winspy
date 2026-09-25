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
//	Windows pre-fills mis->itemHeight with a guess that doesn't reliably
//	track the listbox's actual (DPI-scaled, for a per-monitor-aware app)
//	font, so measure it directly instead - otherwise the row height can
//	end up smaller than the text that's about to be drawn into it.
//
BOOL FunkyList_MeasureItem(HWND hwnd, UINT uCtrlId, MEASUREITEMSTRUCT *mis)
{
	HWND       hwndList = GetDlgItem(hwnd, uCtrlId);
	HFONT      hFont = (HFONT)SendMessage(hwndList, WM_GETFONT, 0, 0);
	HDC        hdc = GetDC(hwndList);
	HFONT      hOldFont;
	TEXTMETRIC tm;

	hOldFont = hFont ? (HFONT)SelectObject(hdc, hFont) : NULL;
	GetTextMetrics(hdc, &tm);
	if(hOldFont) SelectObject(hdc, hOldFont);
	ReleaseDC(hwndList, hdc);

	// A little extra breathing room beyond the bare font metrics, sized
	// as a fraction of the (already DPI-scaled) font height so it stays
	// proportionally correct at any DPI instead of a fixed pixel count.
	mis->itemHeight = tm.tmHeight + tm.tmExternalLeading + tm.tmHeight / 4;
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
		SendMessage(hwndList, LB_GETTEXT, dis->itemID, (LONG_PTR)szText);
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

		// Fill the whole row's background first (an ExtTextOut with no
		// string still does the ETO_OPAQUE fill), then draw the item
		// text vertically centred in it - drawing text flush against
		// rcItem.top left more padding above the text than below it.
		ExtTextOut(dis->hDC,
			dis->rcItem.left,
			dis->rcItem.top,
			ETO_OPAQUE, &dis->rcItem, NULL, 0, 0);

		{
			RECT rcText = dis->rcItem;
			rcText.left += 2;
			DrawText(dis->hDC, szText, -1, &rcText, DT_LEFT|DT_SINGLELINE|DT_VCENTER|DT_NOPREFIX);
		}

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
