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
	DS_STATIC_GROUP_OBJECT = 50,
	DS_STATIC_GROUP_DRAW,
	DS_STATIC_GROUP_COLOR_OVERRIDE,
	DS_STATIC_GROUP_EDGE,
	DS_STATIC_GROUP_VERTEX,

	DS_STATIC_TEXT_VIS,
	DS_STATIC_TEXT_DRAW_FACE,
	DS_STATIC_TEXT_DRAW_EDGE,
	DS_STATIC_TEXT_DRAW_VERTEX,

	DS_STATIC_TEXT_COLOR_OVERRIDE_FACE,
	DS_STATIC_TEXT_COLOR_OVERRIDE_EDGE,
	DS_STATIC_TEXT_COLOR_OVERRIDE_VERTEX,
	DS_STATIC_TEXT_COLOR_SELECT_FACE,
	DS_STATIC_TEXT_COLOR_SELECT_EDGE,
	DS_STATIC_TEXT_COLOR_SELECT_VERTEX,

	DS_STATIC_TEXT_EDGE_ROUND,
	DS_STATIC_TEXT_EDGE_BOX,
	DS_STATIC_TEXT_EDGE_WIDTH,
	DS_STATIC_TEXT_EDGE_HEIGHT,
	DS_STATIC_TEXT_EDGE_OFFSET,

	DS_STATIC_TEXT_VERTEX_SCALE,

	DS_BASE_NAME					= 100,
	DS_BASE_ACTIVE					= 200,
	DS_BASE_DRAW_FACE				= 300,
	DS_BASE_DRAW_EDGE				= 400,
	DS_BASE_DRAW_VERTEX				= 500,
	DS_BASE_COLOR_FACE_USE_E		= 600,
	DS_BASE_COLOR_FACE_USE_A		= 700,
	DS_BASE_COLOR_FACE_USE_O		= 800,
	DS_BASE_COLOR_FACE_SET			= 900,
	DS_BASE_COLOR_EDGE_USE_A		= 1000,
	DS_BASE_COLOR_EDGE_USE_O		= 1100,
	DS_BASE_COLOR_EDGE_SET			= 1200,
	DS_BASE_COLOR_VERTEX_SET		= 1300,
	DS_BASE_EDGE_ROUND				= 1400,
	DS_BASE_EDGE_BOX				= 1500,
	DS_BASE_EDGE_WIDTH				= 1600,
	DS_BASE_EDGE_HEIGHT				= 1700,
	DS_BASE_EDGE_OFFSET				= 1800,
	DS_BASE_VERTEX_SCALE			= 1900,
	DS_BASE_REPLICATE_FACE			= 2000,
	DS_BASE_REPLICATE_Z				= 2100,
	DS_BASE_REPLICATE_X				= 2200,
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
	L"Button",	L"Object",			DS_STATIC,		  3, 3, 74,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Draw",			DS_STATIC,		 81, 3, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Color",			DS_STATIC,		126, 3,130,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Face",			DS_STATIC,		126,14, 54,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Edge",			DS_STATIC,		182,14, 42,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Vertex",			DS_STATIC,		226,14, 30,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Edge",			DS_STATIC,		259, 3,127,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Vertex",			DS_STATIC,		390, 3, 38,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Button",	L"Replication",		DS_STATIC,		431, 3, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,
	L"Static",	L"name",			DS_STATIC,		  9,23,50,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Vis",				DS_STATIC,		 63,23,10,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"F",				DS_STATIC,		 87,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		 99,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"V",				DS_STATIC,		111,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		132,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"A",				DS_STATIC,		143,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		154,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"A",				DS_STATIC,		188,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		199,24, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"R",				DS_STATIC,		266,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"B",				DS_STATIC,		278,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Width",			DS_STATIC,		292,22,20,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Height",			DS_STATIC,		324,22,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Offset",			DS_STATIC,		358,22,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Scale",			DS_STATIC,		400,22,18,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"1F",				DS_STATIC,		436,23, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Z",				DS_STATIC,		450,23, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"X",				DS_STATIC,		462,23, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
};

int nDS_Fixed_Controls = sizeof(ds_obj_fixed) / sizeof(DS_OBJECT_CONTROL);

DS_OBJECT_CONTROL_EX ds_obj_variable[] = {
	(LPCWSTR)WC_STATIC,  L"",	DS_BASE_NAME					,	  9,32,50, 8,	WS_VISIBLE | WS_CHILD | SS_LEFT,								0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_ACTIVE					,	 63,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_FACE				,	 85,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_EDGE				,	 97,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_VERTEX				,	109,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	// face color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_E		,	130,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,	0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_A		,	141,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_O		,	152,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_SET			,	163,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// edge color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_A		,	186,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_O		,	197,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_SET			,	208,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// vertex color 
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_VERTEX_SET		,	234,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Edge round/box size
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_ROUND				,	264,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_BOX				,	275,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_WIDTH				,	287,30,30,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_HEIGHT				,	320,30,30,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_OFFSET				,	354,30,30,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Vertex Scale
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_VERTEX_SCALE			,	394,30,30,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Replication 
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_FACE			,	435,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_Z				,	447,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_X				,	459,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
};

int nDS_Variable_Controls = sizeof(ds_obj_variable) / sizeof(DS_OBJECT_CONTROL_EX);

#include <tchar.h>

//-----------------------------------------------------------------------------
static void ds_fill_object_controls(HWND hWndDlg, int objID, DS_GEO_OBJECT *gobj, char *buffer)
//-----------------------------------------------------------------------------
{
	if (gobj->filename)
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, ds_name_start(gobj->filename));
	}
	else
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, "default");
	}
	SendDlgItemMessage(hWndDlg, DS_BASE_ACTIVE + objID, BM_SETCHECK, (gobj->active ? BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_FACE   + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_TRIANGLES ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_EDGE   + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_EDGES ?     BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_DRAW_VERTEX + objID, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_VERTICES ?  BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_E + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_EXPLICIT ?  BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_A + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_AUTOMATIC ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_FACE_USE_O + objID, BM_SETCHECK, (gobj->cAttr.face.state & DS_COLOR_STATE_OVERRIDE ?  BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_EDGE_USE_A + objID, BM_SETCHECK, (gobj->cAttr.edge.state & DS_COLOR_STATE_AUTOMATIC ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_EDGE_USE_O + objID, BM_SETCHECK, (gobj->cAttr.edge.state & DS_COLOR_STATE_OVERRIDE ?  BST_CHECKED : BST_UNCHECKED), 0);

	SendDlgItemMessage(hWndDlg, DS_BASE_EDGE_ROUND + objID, BM_SETCHECK, (gobj->eAttr.type & GEOMETRY_EDGE_ROUND ?  BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_EDGE_BOX   + objID, BM_SETCHECK, (gobj->eAttr.type & GEOMETRY_EDGE_SQUARE ? BST_CHECKED : BST_UNCHECKED), 0);

	sprintf(buffer, "%.3f", gobj->eAttr.width);  SetDlgItemText(hWndDlg, DS_BASE_EDGE_WIDTH   + objID, buffer);
	sprintf(buffer, "%.3f", gobj->eAttr.height); SetDlgItemText(hWndDlg, DS_BASE_EDGE_HEIGHT  + objID, buffer);
	sprintf(buffer, "%.3f", gobj->eAttr.offset); SetDlgItemText(hWndDlg, DS_BASE_EDGE_OFFSET  + objID, buffer);
	sprintf(buffer, "%.3f", gobj->vAttr.scale);  SetDlgItemText(hWndDlg, DS_BASE_VERTEX_SCALE + objID, buffer);

	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_FACE + objID, BM_SETCHECK, (gobj->rAttr.oneFaceFlag ?   BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_Z    + objID, BM_SETCHECK, (gobj->rAttr.zRotationFlag ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_REPLICATE_X    + objID, BM_SETCHECK, (gobj->rAttr.xMirrorFlag ?   BST_CHECKED : BST_UNCHECKED), 0);
}

//-----------------------------------------------------------------------------
void static ds_draw_variable_controls(HWND hWndDlg, HFONT s_hFont, int yOffset, int *objID, int *bottom )
//-----------------------------------------------------------------------------
{
	int						i;
//	DS_OBJECT_CONTROL		*dso;
	DS_OBJECT_CONTROL_EX	*dsox;
	HWND					hEdit;
	RECT					rect; 

	// do this for each geometry object  
	//objID = 0;
	*bottom = 0;

	for (i = 0, dsox = ds_obj_variable; i < nDS_Variable_Controls; ++i, ++dsox)
	{
		rect.top = dsox->y + yOffset * *objID;
		rect.left = dsox->x;
		rect.right = dsox->x + dsox->w;
		rect.bottom = dsox->y + dsox->h + yOffset * *objID;
		if (rect.bottom > *bottom)
			*bottom = rect.bottom;
		MapDialogRect(hWndDlg, &rect);

		hEdit = CreateWindowExA(dsox->exStyle | WS_EX_TOOLWINDOW, (LPCSTR)dsox->className, (LPCSTR)dsox->text, dsox->style,
			rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top, hWndDlg,
			(HMENU)(dsox->id + *objID), GetModuleHandle(NULL), NULL);
		if (hEdit)
			SendMessage(hEdit, WM_SETFONT, (WPARAM)s_hFont, (LPARAM)MAKELONG(TRUE, 0));
	}
	++*objID;
}

//-----------------------------------------------------------------------------
//LRESULT CALLBACK DlgObjectControl(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
LRESULT CALLBACK ds_dlg_object_control(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	HWND					pWnd;
	DS_CTX						*ctx;
//	LONG_PTR				lp;
	static HFONT			s_hFont = NULL;
//	HFONT					hfDefault;
	HWND					hEdit;
	const TCHAR*			fontName = _T("Microsoft Sans Serif");
	const long				nFontSize = 8;
	LOGFONT					logFont = { 0 };
	HDC						hdc = GetDC(hWndDlg);
	int						yOffset = 15;
	RECT					rect; // , urect;
//	DWORD					error;
	DS_GEO_OBJECT			*gobj;
	int						temp, clrUpdate;
	char					buffer[128]; 
	int						control, category, objID;
	static DS_GEO_OBJECT	gobj_def;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);

	switch (Msg) {
	case WM_INITDIALOG:
		logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		logFont.lfWeight = FW_REGULAR;
		strcpy_s(logFont.lfFaceName, 32, fontName);
		s_hFont = CreateFontIndirect(&logFont);
		ReleaseDC(hWndDlg, hdc);

		int						i, nObj, bottom, maxBottom;
		DS_OBJECT_CONTROL		*dso;
//		DS_OBJECT_CONTROL_EX	*dsox;
		// do this for each geometry object  
		objID = 0;
		bottom = 0;

		// default object settings
		memcpy(&gobj_def, &ctx->defInputObj, sizeof(DS_GEO_INPUT_OBJECT));
		ds_draw_variable_controls(hWndDlg, s_hFont, yOffset, &objID, &bottom);

		// loop thru real objects
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			ds_draw_variable_controls( hWndDlg, s_hFont, yOffset, &objID, &bottom);
		}
		nObj = LL_GetLength(ctx->gobjectq) + 1; // add default
		maxBottom = 0;
		for (i = 0, dso = ds_obj_fixed; i < nDS_Fixed_Controls; ++i, ++dso)
		{
			rect.top = dso->y;
			rect.left = dso->x;
			rect.right = dso->x + dso->w;
			rect.bottom = dso->y + dso->h;
			if (dso->style & BS_GROUPBOX)
			{
				rect.bottom = bottom + 5;
			}
			MapDialogRect(hWndDlg, &rect);
			if (rect.bottom > maxBottom)
				maxBottom = rect.bottom;
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
			h = maxBottom - 10;
			h += WINDOW_SIZE_OFFSET_HEIGHT;
			MoveWindow(hWndDlg, window.left, window.top, w, h, 1);// need to add extra
		}
		break;

	case WM_PAINT:
		// get update rect
//		GetUpdateRect(hWndDlg, &urect, 0);

		objID = 0;

		// fill in the default
		ds_fill_object_controls(hWndDlg, objID, &gobj_def, buffer);

		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{// send messages to all controls to update content
			ds_fill_object_controls( hWndDlg, ++objID,  gobj, buffer);
		}
//		return TRUE;
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			ctx->objControl = 0;
			DestroyWindow(hWndDlg);
//			return TRUE;
		case IDCANCEL:
			ctx->objControl = 0;
			DestroyWindow(hWndDlg);
//			return TRUE;
		}

		control   = LOWORD(wParam); // the specific control that triggered the event
		category  = control / 100;	// control category
		category *= 100;
		objID     = control % 100;	// object ID 
		temp      = 0;

		if (control < 100 || control > 2300)
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

		{
			UINT uiID = LOWORD(wParam);
			UINT uiCode = HIWORD(wParam);   // spin_update ( ctx, what, buffer )  what = 0,1,2

			switch (uiCode) {
			case EN_KILLFOCUS:
				switch (category) {
				case DS_BASE_EDGE_WIDTH:   GetDlgItemText(hWndDlg, control, buffer, 256); sscanf(buffer, "%lf", &gobj->eAttr.width);  InvalidateRect(pWnd, 0, 0); break;
				case DS_BASE_EDGE_HEIGHT:  GetDlgItemText(hWndDlg, control, buffer, 256); sscanf(buffer, "%lf", &gobj->eAttr.height); InvalidateRect(pWnd, 0, 0); break;
				case DS_BASE_EDGE_OFFSET:  GetDlgItemText(hWndDlg, control, buffer, 256); sscanf(buffer, "%lf", &gobj->eAttr.offset); InvalidateRect(pWnd, 0, 0); break;
				case DS_BASE_VERTEX_SCALE: GetDlgItemText(hWndDlg, control, buffer, 256); sscanf(buffer, "%lf", &gobj->vAttr.scale);  InvalidateRect(pWnd, 0, 0); break;
				}
//				return TRUE;
				break;
			}
		}
		clrUpdate = 0;
		switch ( category ) {
		case DS_BASE_NAME: break;

		case DS_BASE_ACTIVE: gobj->active = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_DRAW_FACE:   gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_EDGES | GEOMETRY_DRAW_VERTICES)) | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_FACES    : 0);  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_DRAW_EDGE:   gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_VERTICES)) | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_EDGES    : 0);  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_DRAW_VERTEX: gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_EDGES))    | ( SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_VERTICES : 0);  InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_FACE_USE_E: gobj->cAttr.face.state = DS_COLOR_STATE_EXPLICIT;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_USE_A: gobj->cAttr.face.state = DS_COLOR_STATE_AUTOMATIC; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_USE_O: gobj->cAttr.face.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_FACE_SET:   clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.face.color);   InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_EDGE_USE_A: gobj->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_USE_O: gobj->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_SET:   clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.edge.color);   InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_VERTEX_SET: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.vertex.color); InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_EDGE_ROUND: gobj->eAttr.type = GEOMETRY_EDGE_ROUND;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_EDGE_BOX:   gobj->eAttr.type = GEOMETRY_EDGE_SQUARE; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_REPLICATE_FACE:	gobj->rAttr.oneFaceFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_Z:		gobj->rAttr.zRotationFlag = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_X:		gobj->rAttr.xMirrorFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		}
		if (clrUpdate)
		{
			clrUpdate = 0;
//			++ctx->clrCtl.updateFlag;
//			InvalidateRect(ctx->mainWindow, 0, 0);
			LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)lParam;
			InvalidateRect((HWND)lParam, 0, 0);
//			return TRUE;
		}
//		return TRUE;
		break;

	case WM_DESTROY:
		DestroyWindow(hWndDlg);
		ctx->objControl = 0;
//		return TRUE;
		break;

	case WM_CLOSE:
		DestroyWindow(hWndDlg);
		ctx->objControl = 0;
//		return TRUE;
		break;

	case WM_DRAWITEM: // owner drawn 
		{
			LPDRAWITEMSTRUCT	lpdis = (DRAWITEMSTRUCT*)lParam;
			RECT				lpr;
			HBRUSH				hColorBrush;
			DS_COLOR			*clr = 0;
//			int					flag = 0;

			control = LOWORD(wParam);   // the specific control that triggered the event
			category = control / 100;	// control category
			category *= 100;
			objID = control % 100;	    // object ID 
			temp = 0;
			if (control < 100 || control > 2300)
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
			if (!gobj) break;;

			switch (category) {
			case DS_BASE_COLOR_FACE_SET:   /* flag = 1; */ clr = &gobj->cAttr.face.color;		break;
			case DS_BASE_COLOR_EDGE_SET:   /* flag = 1; */ clr = &gobj->cAttr.edge.color;		break;
			case DS_BASE_COLOR_VERTEX_SET: /* flag = 1; */ clr = &gobj->cAttr.vertex.color;		break;
			}
//			if (flag)
			if (clr)
			{
				hColorBrush = CreateSolidBrush(RGB((unsigned int)(clr->r * 255), (unsigned int)(clr->g * 255), (unsigned int)(clr->b * 255)));
				GetWindowRect(lpdis->hwndItem, &lpr);
				// convert to local coordinates
				lpr.right  = lpr.right - lpr.left;
				lpr.left   = 0;
				lpr.bottom = lpr.bottom - lpr.top;
				lpr.top    = 0;
				FillRect(lpdis->hDC, &lpr, hColorBrush);
			}
		}
//		return TRUE;
		break;
	}

	return FALSE;
}
