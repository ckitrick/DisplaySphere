/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle the fully custom Object control dialog.
	The details of the Object Control dialog is NOT defined in the ds_menu.rc file.
*/
#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glext.h>		/* OpenGL header file */
#include <GL/wglext.h>
#include <stdio.h>
#include <math.h>
#include <commdlg.h>
#include <commctrl.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

char *ds_name_start(char *name);
enum {
	DS_STATIC = 10,
	DS_BASE_VISIBLE = 100,
	DS_BASE_ATTRIBUTES = 200,
	DS_BASE_OBJECT_NAME = 300,
};


typedef struct {
	LPCWSTR		className;
	LPCWSTR		text;
	int			id;
	int			x, y, w, h; // position and size 
	int			style;
} DS_OBJECT_CONTROL;

typedef struct {
	LPCWSTR		className;
	LPCWSTR		text;
	int			id;
	int			x, y, w, h; // position and size 
	DWORD		style, exStyle;
} DS_OBJECT_CONTROL_EX;



DS_OBJECT_CONTROL ds_obj_fixed[] = {
	L"Static",	L"Visible",			DS_STATIC,		  5, 5, 30,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,  //   3, 2, 75,38
	L"Static",	L"Attributes",		DS_STATIC,		 30, 5, 30,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,  //  82, 2, 40,38
	L"Static",	L"Name",			DS_STATIC,		 75, 5,195,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,  // 126, 2,199,38
};

int nDS_Fixed_Controls = sizeof(ds_obj_fixed) / sizeof(DS_OBJECT_CONTROL);

DS_OBJECT_CONTROL_EX ds_obj_variable[] = {
	(LPCWSTR)WC_BUTTON,  L"",		DS_BASE_VISIBLE					,	  8,20,10, 8,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | SS_LEFT,								0,
	(LPCWSTR)WC_BUTTON,  "change",	DS_BASE_ATTRIBUTES				,	 30,20,30,10,	WS_VISIBLE | WS_CHILD,								0,
	(LPCWSTR)WC_STATIC,  L"",		DS_BASE_OBJECT_NAME				,	 75,20,195,10,	WS_VISIBLE | WS_CHILD | SS_LEFT,												0,
};

int nDS_Variable_Controls = sizeof(ds_obj_variable) / sizeof(DS_OBJECT_CONTROL_EX);

#include <tchar.h>

//-----------------------------------------------------------------------------
static void ds_fill_object_controls(HWND hWndDlg, int objID, DS_GEO_OBJECT *gobj, char *buffer)
//-----------------------------------------------------------------------------
{
	SendDlgItemMessage(hWndDlg, DS_BASE_VISIBLE + objID, BM_SETCHECK, (gobj->active ? BST_CHECKED : BST_UNCHECKED), 0);

	if (gobj->name[0])
	{
		SetDlgItemText(hWndDlg, DS_BASE_OBJECT_NAME + objID, gobj->name);
	}
	else if (gobj->filename)
	{
		SetDlgItemText(hWndDlg, DS_BASE_OBJECT_NAME + objID, ds_name_start(gobj->filename));
	}
	else
	{
		SetDlgItemText(hWndDlg, DS_BASE_OBJECT_NAME + objID, "default");
	}

//	SendDlgItemMessage(hWndDlg, DS_BASE_ATTRIBUTES + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_TRIANGLES ? BST_CHECKED : BST_UNCHECKED), 0);
}

//-----------------------------------------------------------------------------
void static ds_draw_variable_controls(HWND hWndDlg, HFONT s_hFont, int yOffset, int *objID, int *bottom, int defaultObject)
//-----------------------------------------------------------------------------
{
	int						i;
	DS_OBJECT_CONTROL_EX	*dsox;
	HWND					hEdit;
	RECT					rect;

	// do this for each geometry object  
	*bottom = 0;

	for (i = 0, dsox = ds_obj_variable; i < nDS_Variable_Controls; ++i, ++dsox)
	{
		rect.top = dsox->y + yOffset * *objID;
		rect.left = dsox->x;
		rect.right = dsox->x + dsox->w;
		rect.bottom = dsox->y + dsox->h + yOffset * *objID;
//		if (dsox->h < 40 && rect.bottom > *bottom)
		MapDialogRect(hWndDlg, &rect);
		if (rect.bottom > *bottom)
			*bottom = rect.bottom;

		if (!defaultObject || (defaultObject && i != 0)) // don't create window for default visible checkbox
		{
			hEdit = CreateWindowExA(dsox->exStyle | WS_EX_TOOLWINDOW, (LPCSTR)dsox->className, (LPCSTR)dsox->text, dsox->style,
				//			hEdit = CreateWindowExA(dsox->exStyle, (LPCSTR)dsox->className, (LPCSTR)dsox->text, dsox->style,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hWndDlg,
				(HMENU)(dsox->id + *objID), GetModuleHandle(NULL), NULL);
			if (hEdit)
				SendMessage(hEdit, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
		}
	}
	++*objID;
}

//-----------------------------------------------------------------------------
void ds_edit_text_update(HWND pWnd, HWND dlg, int control, char *buffer, void *vPtr, int doubleFlag, int nDigits, int clamp, double min, double max)
//-----------------------------------------------------------------------------
{
	// Helper function 
	double		value;
	GetDlgItemText(dlg, control, buffer, 256);	// get text from edit control
	value = atof(buffer);						// extract number from text
	if (clamp)									// check for clamping
		if (value < min)		value = min;
		else if (value > max)	value = max;
	sprintf(buffer, "%.*f", nDigits, value);	// reformat answer
	if (doubleFlag)	*((double*)vPtr) = value;	// copy value to appropriate address
	else			*((float*)vPtr) = (float)value;
	SetDlgItemText(dlg, control, buffer);		// reset control
	InvalidateRect(pWnd, 0, 0);					// update main window
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_object_dashboard(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	HWND					pWnd;
	DS_CTX					*ctx;
	static HFONT			s_hFont = NULL;
	HWND					hEdit;
	const TCHAR*			fontName = _T("Microsoft Sans Serif");
	const long				nFontSize = 8;
	LOGFONT					logFont = { 0 };
	HDC						hdc = GetDC(hWndDlg);
	int						yOffset = 15;
	RECT					rect;
	DS_GEO_OBJECT			*gobj;
	int						temp, clrUpdate;
	char					buffer[128];
	int						control, category, objID;
	static DS_GEO_OBJECT	gobj_def;
	static PAINTSTRUCT		ps;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);

	// display all objects including default with 
	switch (Msg) {
	case WM_INITDIALOG:
		// draw header
		// draw special default line
		// loop through all objects and display visible status, property button, name 
		if (!ctx)
			MessageBox(pWnd, "Dashboad error", "HELP", 0);
		logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		logFont.lfWeight = FW_REGULAR;
		strcpy_s(logFont.lfFaceName, 32, fontName);
		s_hFont = CreateFontIndirect(&logFont);
		ReleaseDC(hWndDlg, hdc);

		int						i, nObj, bottom, maxBottom;
		DS_OBJECT_CONTROL		*dso;
		// do this for each geometry object  
		objID = 0;
		bottom = 15;

		// default object settings
		memcpy(&gobj_def, &ctx->defInputObj, sizeof(DS_GEO_INPUT_OBJECT));
		ds_draw_variable_controls(hWndDlg, s_hFont, yOffset, &objID, &bottom, 1);

		// loop thru real objects
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			ds_draw_variable_controls(hWndDlg, s_hFont, yOffset, &objID, &bottom, 0);
		}
 
		nObj = LL_GetLength(ctx->gobjectq) + 1; // add default
		maxBottom = bottom - 5;
		for (i = 0, dso = ds_obj_fixed; i < nDS_Fixed_Controls; ++i, ++dso)
		{
			rect.top = dso->y;
			rect.left = dso->x;
			rect.right = dso->x + dso->w;
			rect.bottom = dso->y + dso->h;
//			if (dso->style & BS_GROUPBOX)
//			{
//				rect.bottom = bottom + 5;
//			}
			MapDialogRect(hWndDlg, &rect);
//			if (rect.bottom > maxBottom)
//				maxBottom = rect.bottom;
			hEdit = CreateWindowW((LPCWSTR)dso->className, dso->text, dso->style,
				rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,
				hWndDlg, (HMENU)dso->id, GetModuleHandle(NULL), NULL);
			SendMessage(hEdit, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
		}
		{ // resize dialog to match the size based on the number of objects
			RECT			window;
			GetWindowRect(hWndDlg, &window);
			int  w, h;
			w = window.right - window.left;
			h = window.bottom - window.top;
			h = maxBottom;
			h += WINDOW_SIZE_OFFSET_HEIGHT;
			MoveWindow(hWndDlg, window.left, window.top, w, h, 1);// need to add extra
		}
		objID = 0;

		// fill in the default
		ds_fill_object_controls(hWndDlg, objID, &gobj_def, buffer);

		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{// send messages to all controls to update content
			ds_fill_object_controls(hWndDlg, ++objID, gobj, buffer);
		}
		break;

	case WM_PAINT:
		BeginPaint(hWndDlg, &ps);
		EndPaint(hWndDlg, &ps);
		break;

	case WM_COMMAND:
		if (!ctx)
			MessageBox(pWnd, "Dashboad error", "HELP", 0);
		control = LOWORD(wParam); // the specific control that triggered the event
		category = control / 100;	// control category
		category *= 100;
		objID = control % 100;	// object ID 
		temp = 0;

		if (control < 100 || control >= 400)
			return FALSE;

		if (objID < 0 || objID > LL_GetLength(ctx->gobjectq))
			return FALSE;
		else if (!objID)
			gobj = (DS_GEO_OBJECT*)&ctx->defInputObj;
		else
		{
			temp = 1;
			LL_SetHead(ctx->gobjectq);
			while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
			{
				if (temp == objID) break;
				++temp;
			}
		}
		if (!gobj)
			break; // done since we didn't find an object

		switch (category) {
		case DS_BASE_VISIBLE: gobj->active = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_ATTRIBUTES: 
			if (!gobj->attrDialog)
			{
				ctx->curObjAttr = gobj; // save and assign to dialog when it gets created
				gobj->attrDialog = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG7), ctx->mainWindow, ds_dlg_object_attributes);
				ShowWindow(gobj->attrDialog, SW_SHOW);
				ds_position_window(ctx, gobj->attrDialog, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
			break;
		}
		break;

//	case WM_DESTROY:
//		if (ctx->objDashboard)
//		{
//			ctx->objDashboard = 0;
//		}
//		DestroyWindow(hWndDlg);
//		break;

	case WM_CLOSE:
		if (!ctx)
			MessageBox(pWnd, "Dashboad error", "HELP", 0);
		else if (ctx->objDashboard)
			ctx->objDashboard = 0;
		DestroyWindow(hWndDlg);
		break;
	}

	return FALSE;
//	return DefWindowProc(hWndDlg, Msg, wParam, lParam);
}
