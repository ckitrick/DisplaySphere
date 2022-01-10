/*
Copyright (C) 2020 Christopher J Kitrick

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
This group of functions is designed to handle all dialog boxes except for Object control.
All of these dialogs are defined in the ds_menu.rc file.
*/
#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glext.h>		/* OpenGL header file */
#include <GL/wglext.h>
#include <GL/glut.h>
#include <stdio.h>
#include <math.h>
#include <commdlg.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

//-----------------------------------------------------------------------------
//LRESULT CALLBACK ds_dlg_object_attributes(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------


//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_object_attributes(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	int					i, index;
	HWND				pWnd;
	DS_CTX				*ctx;
	PAINTSTRUCT			ps;
	HDC					hdcDestination;
	HINSTANCE			hInst;
	DS_GEO_OBJECT		*gobj;
	char				buffer[128];

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);

	hInst = GetModuleHandle(NULL);// returns HINSTANCE of the calling process
	gobj = (DS_CTX*)GetWindowLong(hWndDlg, GWL_USERDATA);

	if (Msg != WM_INITDIALOG && !gobj)
		return 0;
	else if (Msg == WM_INITDIALOG && !ctx)
		return 0;

	switch (Msg) {
	case WM_INITDIALOG:
		// save current object in window 
		gobj = ctx->curObjAttr;
		if (!gobj)
			return 0;
		SetWindowLong(hWndDlg, GWL_USERDATA, (long)ctx->curObjAttr);  // attach context to window
		if (gobj->name[0])
		{
			sprintf(buffer, "Object: %s", gobj->name); //SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, gobj->name);
		}
		else if (gobj->filename)
		{
			sprintf(buffer, "Object: %s", ds_name_start(gobj->filename)); //SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, ds_name_start(gobj->filename));
		}
		else
		{
			sprintf(buffer, "Object: %s", "default"); //SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, "default");
		}			
		SetWindowText(hWndDlg, buffer);
		gobj->restore = *(DS_GEO_INPUT_OBJECT*)gobj; // save the front end of the object state 
		// Load geometry section
		// combo box 1 - base
		static char *geo_base[5] =
		{
			"Icosahedron",
			"Octahedron",
			"Tetrahedron",
			"Cube",
			"Dodecahedron",
		};

		for (i = 0; i < 5; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO1, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)geo_base[i]);
		SendDlgItemMessage(hWndDlg, IDC_COMBO1, CB_SETCURSEL, (WPARAM)(gobj->geo_type-1), (LPARAM)0);
		// combo box 2 - orientation
		static char *geo_orientation[3] =
		{
			"vertex",
			"edge",
			"face",
		};

		for (i = 0; i < 3; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO3, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)geo_orientation[i]);
		SendDlgItemMessage(hWndDlg, IDC_COMBO3, CB_SETCURSEL, (WPARAM)gobj->geo_orientation, (LPARAM)0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK1, BM_SETCHECK, (gobj->rAttr.oneFaceFlag ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK2, BM_SETCHECK, (gobj->rAttr.zRotationFlag ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK3, BM_SETCHECK, (gobj->rAttr.xMirrorFlag ? BST_CHECKED : BST_UNCHECKED), 0);
		// Face 
//		SendDlgItemMessage(hWndDlg, DS_BASE_ACTIVE + objID, BM_SETCHECK, (gobj->active ? BST_CHECKED : BST_UNCHECKED), 0);
		// visible
//		SendDlgItemMessage(hWndDlg, IDC_CHECK4, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_TRIANGLES ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK4, BM_SETCHECK, (gobj->fAttr.draw ? BST_CHECKED : BST_UNCHECKED), 0);
		// combo box 3 - face color
		static char *color_option[3] =
		{
			"explicit",
			"automatic",
			"override",
		};

		for (i = 0; i < 3; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO4, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)color_option[i]);
		switch (gobj->cAttr.face.state) {
		case 1: SendDlgItemMessage(hWndDlg, IDC_COMBO4, CB_SETCURSEL, (WPARAM)0, 0); break;
		case 2: SendDlgItemMessage(hWndDlg, IDC_COMBO4, CB_SETCURSEL, (WPARAM)1, 0); break;
		case 4: SendDlgItemMessage(hWndDlg, IDC_COMBO4, CB_SETCURSEL, (WPARAM)2, 0); break;
		}
		// transparency		
		SendDlgItemMessage(hWndDlg, IDC_CHECK5, BM_SETCHECK, (gobj->tAttr.onFlag ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK30, BM_SETCHECK, (gobj->tAttr.state & DS_COLOR_STATE_OVERRIDE ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.2f", gobj->tAttr.alpha);  SetDlgItemText(hWndDlg, IDC_EDIT67, buffer);
		// scale
		SendDlgItemMessage(hWndDlg, IDC_CHECK86, BM_SETCHECK, (gobj->fAttr.scale.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->fAttr.scale.factor);  SetDlgItemText(hWndDlg, IDC_EDIT66, buffer);
		// offset
		SendDlgItemMessage(hWndDlg, IDC_CHECK79, BM_SETCHECK, (gobj->fAttr.offset.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->fAttr.offset.factor);  SetDlgItemText(hWndDlg, IDC_EDIT65, buffer);
		// extrusion
		SendDlgItemMessage(hWndDlg, IDC_CHECK80, BM_SETCHECK, (gobj->fAttr.extrusion.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK82, BM_SETCHECK, (gobj->fAttr.extrusion.bothSides ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK85, BM_SETCHECK, (gobj->fAttr.extrusion.holeOnly ? BST_CHECKED : BST_UNCHECKED), 0);
		static char *direction_option[2] =
		{
			"normal",
			"radial",
		};
		for (i = 0; i < 2; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO5, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)direction_option[i]);
		switch (gobj->fAttr.extrusion.direction) {
		case 0: SendDlgItemMessage(hWndDlg, IDC_COMBO5, CB_SETCURSEL, (WPARAM)0, 0); break;
		case 1: SendDlgItemMessage(hWndDlg, IDC_COMBO5, CB_SETCURSEL, (WPARAM)1, 0); break;
		}
		sprintf(buffer, "%.3f", gobj->fAttr.extrusion.factor);  SetDlgItemText(hWndDlg, IDC_EDIT3, buffer);
		// hole
		SendDlgItemMessage(hWndDlg, IDC_CHECK81, BM_SETCHECK, (gobj->fAttr.hole.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		static char *style_option[4] =
		{
			"round",
			"polygonal",
			"in dimple",
			"out dimple",
		};
		for (i = 0; i < 4; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO6, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)style_option[i]);
		SendDlgItemMessage(hWndDlg, IDC_COMBO6, CB_SETCURSEL, (WPARAM)gobj->fAttr.hole.style, 0);
//		switch (gobj->fAttr.hole.style) {
//		case 0: SendDlgItemMessage(hWndDlg, IDC_COMBO6, CB_SETCURSEL, (WPARAM)0, 0); break;
//		case 1: SendDlgItemMessage(hWndDlg, IDC_COMBO6, CB_SETCURSEL, (WPARAM)1, 0); break;
//		case 2: SendDlgItemMessage(hWndDlg, IDC_COMBO6, CB_SETCURSEL, (WPARAM)2, 0); break;
//		case 3: SendDlgItemMessage(hWndDlg, IDC_COMBO6, CB_SETCURSEL, (WPARAM)3, 0); break;
//		}
		sprintf(buffer, "%.2f", gobj->fAttr.hole.radius);  SetDlgItemText(hWndDlg, IDC_EDIT60, buffer);
		sprintf(buffer, "%.2f", gobj->fAttr.hole.shallowness);  SetDlgItemText(hWndDlg, IDC_EDIT74, buffer);
		// orthodrome
		// style section
		// combo box 8
		static char *orthodrome_base[4] =
		{
			"Rim",
			"Disc 2 sided",
			"Disc 1 sided",
			"Spherical sections",
		};

		for (i = 0; i < 4; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO8, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)orthodrome_base[i]);
		SendDlgItemMessage(hWndDlg, IDC_COMBO8, CB_SETCURSEL, (WPARAM)(gobj->fAttr.orthodrome.style), (LPARAM)0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK93, BM_SETCHECK, (gobj->fAttr.orthodrome.cutColorEnable ? BST_CHECKED : BST_UNCHECKED), 0);
		//		CheckRadioButton(hWndDlg, IDC_RADIO3, IDC_RADIO24, (gobj->fAttr.orthodrome.style == ORTHODROME_STYLE_RIM ? IDC_RADIO3 : ( gobj->fAttr.orthodrome.style == ORTHODROME_STYLE_DISC_ONE_SIDED ? IDC_RADIO23 : IDC_RADIO24)));
		SendDlgItemMessage(hWndDlg, IDC_CHECK31, BM_SETCHECK, (gobj->fAttr.orthodrome.enable ? BST_CHECKED : BST_UNCHECKED), 0);
//		SendDlgItemMessage(hWndDlg, IDC_CHECK88, BM_SETCHECK, (gobj->fAttr.orthodrome.autoRadiusEnable ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK87, BM_SETCHECK, (gobj->fAttr.orthodrome.dashEnable ? BST_CHECKED : BST_UNCHECKED), 0);
//		sprintf(buffer, "%.3f", gobj->fAttr.orthodrome.radius);  SetDlgItemText(hWndDlg, IDC_EDIT70, buffer);
		sprintf(buffer, "%.3f", gobj->fAttr.orthodrome.depth1);   SetDlgItemText(hWndDlg, IDC_EDIT68, buffer);
		sprintf(buffer, "%.3f", gobj->fAttr.orthodrome.depth2);   SetDlgItemText(hWndDlg, IDC_EDIT71, buffer);
		sprintf(buffer, "%.3f", gobj->fAttr.orthodrome.height);  SetDlgItemText(hWndDlg, IDC_EDIT69, buffer);
		// labels
		SendDlgItemMessage(hWndDlg, IDC_CHECK6, BM_SETCHECK, (gobj->fAttr.label.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		static char *font[5] =
		{
			"H10 - Helvetica 10",
			"H12 - Helvetica 12",
			"H18 - Helvetica 18",
			"T10 - Times Roman 10",
			"T24 - Times Roman 24",
		};

		for (i = 0; i < 5; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO9, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)font[i]);
		switch ((int)gobj->fAttr.label.font) {
		case GLUT_BITMAP_HELVETICA_10:		SendDlgItemMessage(hWndDlg, IDC_COMBO9, CB_SETCURSEL, (WPARAM)0, 0); break;
		case GLUT_BITMAP_HELVETICA_12:		SendDlgItemMessage(hWndDlg, IDC_COMBO9, CB_SETCURSEL, (WPARAM)1, 0); break;
		case GLUT_BITMAP_HELVETICA_18:		SendDlgItemMessage(hWndDlg, IDC_COMBO9, CB_SETCURSEL, (WPARAM)2, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_10:	SendDlgItemMessage(hWndDlg, IDC_COMBO9, CB_SETCURSEL, (WPARAM)3, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_24:	SendDlgItemMessage(hWndDlg, IDC_COMBO9, CB_SETCURSEL, (WPARAM)4, 0); break;
		}

		// Edge
		// visible
//		SendDlgItemMessage(hWndDlg, IDC_CHECK83, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_EDGES ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK83, BM_SETCHECK, (gobj->eAttr.draw ? BST_CHECKED : BST_UNCHECKED), 0);
		// combo box 3 - edge color
		static char *color_option2[2] =
		{
			"automatic",
			"override",
		};

		for (i = 0; i < 2; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO7, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)color_option2[i]);
		switch (gobj->cAttr.edge.state) {
//		case 1: SendDlgItemMessage(hWndDlg, IDC_COMBO4, CB_SETCURSEL, (WPARAM)0, 0); break;
		case 2: SendDlgItemMessage(hWndDlg, IDC_COMBO7, CB_SETCURSEL, (WPARAM)0, 0); break;
		case 4: SendDlgItemMessage(hWndDlg, IDC_COMBO7, CB_SETCURSEL, (WPARAM)1, 0); break;
		}
		// style 
		CheckRadioButton(hWndDlg, IDC_RADIO1, IDC_RADIO2, (gobj->eAttr.type == GEOMETRY_EDGE_ROUND ? IDC_RADIO1 : IDC_RADIO2 ));
		SendDlgItemMessage(hWndDlg, IDC_CHECK89, BM_SETCHECK, (gobj->eAttr.arcEnable ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK91, BM_SETCHECK, (gobj->eAttr.offset.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->eAttr.width);  SetDlgItemText(hWndDlg, IDC_EDIT61, buffer);
		sprintf(buffer, "%.3f", gobj->eAttr.height);  SetDlgItemText(hWndDlg, IDC_EDIT62, buffer);
		sprintf(buffer, "%.3f", gobj->eAttr.offset.factor);  SetDlgItemText(hWndDlg, IDC_EDIT63, buffer);
		// scale
		SendDlgItemMessage(hWndDlg, IDC_CHECK92, BM_SETCHECK, (gobj->eAttr.scale.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->eAttr.scale.factor);  SetDlgItemText(hWndDlg, IDC_EDIT73, buffer);
		// label
		SendDlgItemMessage(hWndDlg, IDC_CHECK43, BM_SETCHECK, (gobj->eAttr.label.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		for (i = 0; i < 5; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO10, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)font[i]);
		switch ((int)gobj->eAttr.label.font) {
		case GLUT_BITMAP_HELVETICA_10:		SendDlgItemMessage(hWndDlg, IDC_COMBO10, CB_SETCURSEL, (WPARAM)0, 0); break;
		case GLUT_BITMAP_HELVETICA_12:		SendDlgItemMessage(hWndDlg, IDC_COMBO10, CB_SETCURSEL, (WPARAM)1, 0); break;
		case GLUT_BITMAP_HELVETICA_18:		SendDlgItemMessage(hWndDlg, IDC_COMBO10, CB_SETCURSEL, (WPARAM)2, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_10:	SendDlgItemMessage(hWndDlg, IDC_COMBO10, CB_SETCURSEL, (WPARAM)3, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_24:	SendDlgItemMessage(hWndDlg, IDC_COMBO10, CB_SETCURSEL, (WPARAM)4, 0); break;
		}

		// Vertex ==============================================================================================
		// visible
//		SendDlgItemMessage(hWndDlg, IDC_CHECK84, BM_SETCHECK, (gobj->drawWhat & GEOMETRY_DRAW_VERTICES ? BST_CHECKED : BST_UNCHECKED), 0);
		SendDlgItemMessage(hWndDlg, IDC_CHECK84, BM_SETCHECK, (gobj->vAttr.draw ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->vAttr.scale);  SetDlgItemText(hWndDlg, IDC_EDIT64, buffer);
		SendDlgItemMessage(hWndDlg, IDC_CHECK90, BM_SETCHECK, (gobj->vAttr.offset.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		sprintf(buffer, "%.3f", gobj->vAttr.offset.factor);  SetDlgItemText(hWndDlg, IDC_EDIT72, buffer);
		SendDlgItemMessage(hWndDlg, IDC_CHECK76, BM_SETCHECK, (gobj->vAttr.label.enable ? BST_CHECKED : BST_UNCHECKED), 0);
		for (i = 0; i < 5; ++i) SendDlgItemMessage(hWndDlg, IDC_COMBO12, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)font[i]);
		switch ((int)gobj->vAttr.label.font) {
		case GLUT_BITMAP_HELVETICA_10:		SendDlgItemMessage(hWndDlg, IDC_COMBO12, CB_SETCURSEL, (WPARAM)0, 0); break;
		case GLUT_BITMAP_HELVETICA_12:		SendDlgItemMessage(hWndDlg, IDC_COMBO12, CB_SETCURSEL, (WPARAM)1, 0); break;
		case GLUT_BITMAP_HELVETICA_18:		SendDlgItemMessage(hWndDlg, IDC_COMBO12, CB_SETCURSEL, (WPARAM)2, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_10:	SendDlgItemMessage(hWndDlg, IDC_COMBO12, CB_SETCURSEL, (WPARAM)3, 0); break;
		case GLUT_BITMAP_TIMES_ROMAN_24:	SendDlgItemMessage(hWndDlg, IDC_COMBO12, CB_SETCURSEL, (WPARAM)4, 0); break;
		}

	case WM_PAINT:
		break;

	case WM_DRAWITEM: // owner drawn 
		{
			LPDRAWITEMSTRUCT	lpdis = (DRAWITEMSTRUCT*)lParam;
			RECT				lpr;
			HBRUSH				hColorBrush;
			DS_COLOR			*clr;
			int					flag = 0;

			switch ((UINT)wParam) {
			case IDC_BUTTON1:  flag = 1;  clr = &gobj->cAttr.face.color;	break;
			case IDC_BUTTON39: flag = 1;  clr = &gobj->fAttr.label.color;	break;
			case IDC_BUTTON16: flag = 1;  clr = &gobj->cAttr.edge.color;	break;
			case IDC_BUTTON40: flag = 1;  clr = &gobj->eAttr.label.color;	break;
			case IDC_BUTTON17: flag = 1;  clr = &gobj->cAttr.vertex.color;	break;
			case IDC_BUTTON41: flag = 1;  clr = &gobj->vAttr.label.color;	break;
			case IDC_BUTTON42: flag = 1;  clr = &gobj->fAttr.orthodrome.cutColor;	break;
			}
			if (flag)
			{
				hColorBrush = CreateSolidBrush(RGB((unsigned int)(clr->r * 255), (unsigned int)(clr->g * 255), (unsigned int)(clr->b * 255)));
				GetWindowRect(lpdis->hwndItem, &lpr);
				// convert to local coordinates
				lpr.right = lpr.right - lpr.left;
				lpr.left = 0;
				lpr.bottom = lpr.bottom - lpr.top;
				lpr.top = 0;
				FillRect(lpdis->hDC, &lpr, hColorBrush);
				//				return TRUE;
			}
		}
		break;

//	case WM_DESTROY:
	case WM_CLOSE:
//		EndDialog(hWndDlg, 0);
		gobj->attrDialog = 0;
		DestroyWindow(hWndDlg);
		break;

	case WM_COMMAND:
		//void ds_edit_text_update(HWND pWnd, HWND dlg, int control, char *buffer, void *vPtr, int doubleFlag, int nDigits, int clamp, double min, double max)
		switch (LOWORD(wParam)) { //66,65,3,60,61,62,63,64
		case IDOK:
		case IDCANCEL:
			if (LOWORD(wParam) == IDCANCEL)
			{
				*(DS_GEO_INPUT_OBJECT*)gobj = gobj->restore;  // restore the front end of the object state 
				ds_face_update(ctx, gobj);
				ds_edge_update(ctx, gobj);
			}
			EndDialog(hWndDlg, 0); InvalidateRect(pWnd, 0, 0);
			if (gobj->attrDialog)
			{
				gobj->attrDialog = 0;
				DestroyWindow(hWndDlg);
			}
			break;
		// GEOMETRY SECTION ================================================================================== 
		case IDC_COMBO1:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				gobj->geo_type = ItemIndex + 1;
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		case IDC_COMBO3:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				gobj->geo_orientation = ItemIndex;
				InvalidateRect(pWnd, 0, 0);
			}
			break;

		case IDC_CHECK1:	gobj->rAttr.oneFaceFlag = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK2:	gobj->rAttr.zRotationFlag = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK3:	gobj->rAttr.xMirrorFlag = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		// FACE SECTION =======================================================================================
//		case IDC_CHECK4:	gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_EDGES | GEOMETRY_DRAW_VERTICES)) | (SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_FACES : 0); InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK4:	gobj->fAttr.draw = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0); InvalidateRect(pWnd, 0, 0); break;
		case IDC_COMBO4:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->cAttr.face.state = DS_COLOR_STATE_EXPLICIT; break;
				case 1: gobj->cAttr.face.state = DS_COLOR_STATE_AUTOMATIC; break;
				case 2: gobj->cAttr.face.state = DS_COLOR_STATE_OVERRIDE; break;
				}
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		case IDC_BUTTON1:   /* clrUpdate = */ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.face.color); InvalidateRect((HWND)lParam, 0, 0); InvalidateRect(pWnd, 0, 0); break;
		case IDC_EDIT67: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->tAttr.alpha, 0, 3, 1, 0.0, 1.0);                               InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_CHECK5:    gobj->tAttr.onFlag = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); InvalidateRect(pWnd, 0, 0); break; // transparency
		case IDC_CHECK30:  if (SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0)) gobj->tAttr.state = DS_COLOR_STATE_OVERRIDE; else gobj->tAttr.state = DS_COLOR_STATE_EXPLICIT; InvalidateRect(pWnd, 0, 0); break;// transparency override
		case IDC_CHECK86: gobj->fAttr.scale.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// scale
		case IDC_CHECK79: gobj->fAttr.offset.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// offset
		case IDC_CHECK80: gobj->fAttr.extrusion.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion
		case IDC_CHECK82: gobj->fAttr.extrusion.bothSides = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion 2 sided
		case IDC_CHECK85: gobj->fAttr.extrusion.holeOnly = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion 2 sided
		case IDC_CHECK81: gobj->fAttr.hole.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion 2 sided


		case IDC_EDIT60: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.hole.radius,   1, 2, 1, 0.1, 0.95);  ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT74: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.hole.shallowness, 1, 2, 1, 0.3, 1.5);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT65: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.offset.factor, 1, 3, 1, -1.0, 1.0);   ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT66: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.scale.factor, 1, 3, 1, 0.0, 2.0);   ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT3:  if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.extrusion.factor, 1, 3, 1, 0.01, 0.95);   ds_face_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;

//		case IDC_RADIO3: gobj->fAttr.orthodrome.style = ORTHODROME_STYLE_RIM; InvalidateRect(pWnd, 0, 0); break;
//		case IDC_RADIO23: gobj->fAttr.orthodrome.style = ORTHODROME_STYLE_DISC_ONE_SIDED;  InvalidateRect(pWnd, 0, 0); break;
//		case IDC_RADIO24: gobj->fAttr.orthodrome.style = ORTHODROME_STYLE_DISC_TWO_SIDED;  InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK31: gobj->fAttr.orthodrome.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion 2 sided
		case IDC_CHECK93: gobj->fAttr.orthodrome.cutColorEnable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); InvalidateRect(ctx->mainWindow, 0, 0); break;// extrusion 2 sided
		case IDC_CHECK87: gobj->fAttr.orthodrome.dashEnable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; 
			InvalidateRect(pWnd, 0, 0); 
			InvalidateRect(ctx->mainWindow, 0, 0); 
			break;// extrusion 2 sided
//		case IDC_EDIT70: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.orthodrome.radius, 1, 3, 1, 0.1, 1.5);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT68: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.orthodrome.depth1, 1, 3, 1, 0.0, 0.3);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT69: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.orthodrome.height, 1, 3, 1, 0.005, 0.2);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT71: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->fAttr.orthodrome.depth2, 1, 3, 1, 0.0, 0.9);  InvalidateRect(ctx->mainWindow, 0, 0); } break;

		case IDC_COMBO5:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->fAttr.extrusion.direction = 0; break; // normal
				case 1: gobj->fAttr.extrusion.direction = 1; break; // radial
				}
				ds_face_update(ctx, gobj);  
				InvalidateRect(ctx->mainWindow, 0, 0);
			}
			break;
		case IDC_COMBO6:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				gobj->fAttr.hole.style = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				ds_face_update(ctx, gobj);
				InvalidateRect(ctx->mainWindow, 0, 0);
			}
			break;		
		case IDC_COMBO8:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				gobj->fAttr.orthodrome.style = ItemIndex;
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		case IDC_CHECK6:	gobj->fAttr.label.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_COMBO9: // label
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->fAttr.label.font = GLUT_BITMAP_HELVETICA_10; break;
				case 1: gobj->fAttr.label.font = GLUT_BITMAP_HELVETICA_12; break;
				case 2: gobj->fAttr.label.font = GLUT_BITMAP_HELVETICA_18; break;
				case 3: gobj->fAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_10; break;
				case 4: gobj->fAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_24; break;
				}
				InvalidateRect(ctx->mainWindow, 0, 0);
			}
			break;		
		case IDC_BUTTON39:
			ds_general_color_dialog(hWndDlg, ctx, &gobj->fAttr.label.color);
			InvalidateRect((HWND)lParam, 0, 0);
			InvalidateRect(pWnd, 0, 0);
			break;
		case IDC_BUTTON42:
			ds_general_color_dialog(hWndDlg, ctx, &gobj->fAttr.orthodrome.cutColor);
			InvalidateRect((HWND)lParam, 0, 0);
			InvalidateRect(pWnd, 0, 0);
			break;

			// EDGE SECTION ======================================================
//		case IDC_CHECK83: gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_VERTICES)) | (SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_EDGES : 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK83: gobj->eAttr.draw = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_COMBO7:
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC; break;
				case 1: gobj->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE; break;
				}
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		case IDC_BUTTON16:   /* clrUpdate = */ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.edge.color);   InvalidateRect((HWND)lParam, 0, 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO1: gobj->eAttr.type = GEOMETRY_EDGE_ROUND;  ds_edge_update(ctx, gobj);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO2: gobj->eAttr.type = GEOMETRY_EDGE_SQUARE; ds_edge_update(ctx, gobj);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_EDIT61: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->eAttr.width, 1, 2, 1, 0.01, 1.5);   ds_edge_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT62: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->eAttr.height, 1, 2, 1, 0.01, 1.0);  ds_edge_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT63: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->eAttr.offset.factor, 1, 2, 1, 0.0, 2.0);   ds_edge_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_CHECK43:	gobj->eAttr.label.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK89:	gobj->eAttr.arcEnable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK91:	gobj->eAttr.offset.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; ds_edge_update(ctx, gobj); InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK92:	gobj->eAttr.scale.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; ds_edge_update(ctx, gobj); InvalidateRect(pWnd, 0, 0); break;
		case IDC_EDIT73: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->eAttr.scale.factor, 1, 2, 1, 0.0, 2.0);   ds_edge_update(ctx, gobj);  InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_COMBO10: // label edge
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->eAttr.label.font = GLUT_BITMAP_HELVETICA_10; break;
				case 1: gobj->eAttr.label.font = GLUT_BITMAP_HELVETICA_12; break;
				case 2: gobj->eAttr.label.font = GLUT_BITMAP_HELVETICA_18; break;
				case 3: gobj->eAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_10; break;
				case 4: gobj->eAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_24; break;
				}
				InvalidateRect(ctx->mainWindow, 0, 0);
			}
			break;
		case IDC_BUTTON40:   
			ds_general_color_dialog(hWndDlg, ctx, &gobj->eAttr.label.color);   
			InvalidateRect((HWND)lParam, 0, 0);  
			InvalidateRect(pWnd, 0, 0); 
			break;

		// VERTEX ============================================================================================
//		case IDC_CHECK84: gobj->drawWhat = (gobj->drawWhat & (GEOMETRY_DRAW_FACES | GEOMETRY_DRAW_EDGES)) | (SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? GEOMETRY_DRAW_VERTICES : 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK84: gobj->vAttr.draw = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_BUTTON17:   /* clrUpdate = */ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.vertex.color);   InvalidateRect((HWND)lParam, 0, 0);  InvalidateRect(pWnd, 0, 0); break;
		case IDC_EDIT64: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->vAttr.scale, 1, 2, 1, 0.0, 1.0);                               InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_EDIT72: if (HIWORD(wParam) == EN_KILLFOCUS) { ds_edit_text_update(pWnd, hWndDlg, LOWORD(wParam), buffer, (void *)&gobj->vAttr.offset.factor, 1, 2, 1, 0.0, 1.0);                               InvalidateRect(ctx->mainWindow, 0, 0); } break;
		case IDC_CHECK76:	gobj->vAttr.label.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK90:	gobj->vAttr.offset.enable = SendDlgItemMessage(hWndDlg, LOWORD(wParam), BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_COMBO12: // label vertex
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				index = SendDlgItemMessage(hWndDlg, LOWORD(wParam), (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				switch (index) {
				case 0: gobj->vAttr.label.font = GLUT_BITMAP_HELVETICA_10; break;
				case 1: gobj->vAttr.label.font = GLUT_BITMAP_HELVETICA_12; break;
				case 2: gobj->vAttr.label.font = GLUT_BITMAP_HELVETICA_18; break;
				case 3: gobj->vAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_10; break;
				case 4: gobj->vAttr.label.font = GLUT_BITMAP_TIMES_ROMAN_24; break;
				}
				InvalidateRect(ctx->mainWindow, 0, 0);
			}
			break;
		case IDC_BUTTON41:   
			ds_general_color_dialog(hWndDlg, ctx, &gobj->vAttr.label.color);   
			InvalidateRect((HWND)lParam, 0, 0);  
			InvalidateRect(pWnd, 0, 0); 
			break;
		}
		break;
	}

	return FALSE;
}
