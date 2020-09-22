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

/* Start of HyperLink URL */
#define DS_HYPERLINK			2020
#define PROP_ORIGINAL_FONT      TEXT("_Hyperlink_Original_Font_")
#define PROP_ORIGINAL_PROC      TEXT("_Hyperlink_Original_Proc_")
#define PROP_STATIC_HYPERLINK   TEXT("_Hyperlink_From_Static_")
#define PROP_UNDERLINE_FONT     TEXT("_Hyperlink_Underline_Font_")
LRESULT CALLBACK _HyperlinkParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK _HyperlinkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
static void CreateHyperLink(HWND hwndControl);
/* End of HyperLink URL */

//-----------------------------------------------------------------------------
int ds_dlg_splash_screen(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	if (!ctx->program.firstTime)
		return 0;

	DialogBox(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG3), ctx->mainWindow, ds_dlg_about);

	// update the installation file
	FILE	*fp;
	char	buffer[512];

	sprintf(buffer, "%s\\state\\installation.txt", ctx->program.dataLocation);
	fopen_s(&fp, buffer, "r");
	if (fp)
	{
		fclose(fp);
		fopen_s(&fp, buffer, "w");
		fprintf(fp, "\"%s\"\n", ctx->program.exeLocation);
		fclose(fp);
	}
	return 0;
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_kbd_toggles(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
#define CLAMP_COLOR( color )	\
	if ( color < 0.0 )			\
		color = 0.0;			\
	else if ( color > 1.0 )		\
		color = 1.0;

//	char			buffer[256];
	HWND			pWnd;
	DS_CTX				*ctx;
	static HBITMAP	bmpSource[2];// = NULL;
	static HDC		hdcSource[2]; // = NULL;
	PAINTSTRUCT		ps;
	HDC				hdcDestination;
	HINSTANCE		hInst;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);
	hInst = GetModuleHandle(NULL);// returns HINSTANCE of the calling process

	switch (Msg) {
	case WM_INITDIALOG:
		{
			bmpSource[0] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, 0);
			bmpSource[1] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, 0);
			hdcSource[0] = CreateCompatibleDC(GetDC(0));
			SelectObject(hdcSource[0], bmpSource[0]);
			hdcSource[1] = CreateCompatibleDC(GetDC(0));
			SelectObject(hdcSource[1], bmpSource[1]);
		}
		break;

	case WM_PAINT:
		{
			hdcDestination = BeginPaint(hWndDlg, &ps);
			BitBlt(hdcDestination, 250, 15, 64, 64, hdcSource[1], 0, 0, SRCAND); // SRCCOPY);
			EndPaint(hWndDlg, &ps);
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			EndDialog(hWndDlg, 0);
			ctx->kbdToggle = 0;
		case IDCANCEL:
			EndDialog(hWndDlg, 0);
			ctx->kbdToggle = 0;
		}
		break;

	case WM_DESTROY:
		ctx->kbdToggle = 0;
		EndDialog(hWndDlg, 0);
		break;

	case WM_CLOSE:
		ctx->kbdToggle = 0;
		EndDialog(hWndDlg, 0);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_about(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
//	char			buffer[256];
	HWND			pWnd;
	DS_CTX			*ctx;
	static HBITMAP	bmpSource[2];// = NULL;
	static HDC		hdcSource[2]; // = NULL;
	PAINTSTRUCT		ps;
	HDC				hdcDestination;
	HINSTANCE		hInst;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);
	hInst = GetModuleHandle(NULL);// returns HINSTANCE of the calling process

	switch (Msg) {
	case WM_INITDIALOG:
		{
			RECT	mrect, arect;

			bmpSource[0] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, 0);
			bmpSource[1] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, 0);
			hdcSource[0] = CreateCompatibleDC(GetDC(0));
			SelectObject(hdcSource[0], bmpSource[0]);
			hdcSource[1] = CreateCompatibleDC(GetDC(0));
			SelectObject(hdcSource[1], bmpSource[1]);

			// place dialog window in the center of the main window 
			GetWindowRect(pWnd, &mrect);
			GetWindowRect(hWndDlg, &arect);
			int		hm, wm, ha, wa;
			hm = mrect.bottom - mrect.top;
			wm = mrect.right - mrect.left;
			ha = arect.bottom - arect.top;
			wa = arect.right - arect.left;
			MoveWindow(hWndDlg, mrect.left + (wm - wa) / 2, mrect.top + (hm - ha) / 2, wa, ha, 1);

			char	buffer[64];
			sprintf(buffer, "Version: %d.%d", ctx->version.major, ctx->version.minor);
			SetDlgItemText(hWndDlg, IDC_STATIC91, buffer);

			sprintf(buffer, "%d", ctx->opengl.samplesPerPixel);
			SetDlgItemText(hWndDlg, IDC_STATIC103, buffer);

			sprintf(buffer, "%s", ctx->executable.fullName);
			SetDlgItemText(hWndDlg, IDC_STATIC105, buffer);
		}
		CreateHyperLink(GetDlgItem(hWndDlg, IDC_STATIC104));
		break;

	case WM_PAINT:
		{
			hdcDestination = BeginPaint(hWndDlg, &ps);
			BitBlt(hdcDestination, 360, 15, 64, 64, hdcSource[1], 0, 0, SRCAND); // SRCCOPY);
			EndPaint(hWndDlg, &ps);
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:		EndDialog(hWndDlg, 0); InvalidateRect(pWnd, 0, 0); break;
		case IDCANCEL:	EndDialog(hWndDlg, 0); break;
		default:
			if (HIWORD(wParam) == BN_CLICKED) { //Buttons, checkboxs, labels, static labels clicked
				switch (LOWORD(wParam))
				{
				case IDC_STATIC104:
					char		url[1024];
					ds_build_url(ctx, url);
					ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
//
//					FILE *fp;
//					fopen_s(&fp, PROGRAM_DOCUMENTATION, "r");
//					if (fp)
//					{
//						fclose(fp);
//						ShellExecute(NULL, "open", PROGRAM_DOCUMENTATION_URL, NULL, NULL, SW_SHOWNORMAL);
//					}
//					else
//					{
//						char buffer[1024];
//						sprintf(buffer, "File <%s> not found.", PROGRAM_DOCUMENTATION);
//						MessageBox(ctx->mainWindow, buffer, "Error", 0);
//					}
					break;
				}
			}
		}
		break;

	case WM_DESTROY:
		break;

	case WM_CLOSE:
		EndDialog(hWndDlg, 0);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_splashscreen(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	//	char			buffer[256];
	HWND			pWnd;
	DS_CTX			*ctx;
	static HBITMAP	bmpSource[2];// = NULL;
	static HDC		hdcSource[2]; // = NULL;
	PAINTSTRUCT		ps;
	HDC				hdcDestination;
	HINSTANCE		hInst;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);
	hInst = GetModuleHandle(NULL);// returns HINSTANCE of the calling process

	switch (Msg) {
	case WM_INITDIALOG:
	{
		RECT	mrect, arect;

		bmpSource[0] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP2), IMAGE_BITMAP, 0, 0, 0);
		bmpSource[1] = (HBITMAP)LoadImageA(hInst, MAKEINTRESOURCEA(IDB_BITMAP1), IMAGE_BITMAP, 0, 0, 0);
		hdcSource[0] = CreateCompatibleDC(GetDC(0));
		SelectObject(hdcSource[0], bmpSource[0]);
		hdcSource[1] = CreateCompatibleDC(GetDC(0));
		SelectObject(hdcSource[1], bmpSource[1]);

		// place dialog window in the center of the main window 
		GetWindowRect(pWnd, &mrect);
		GetWindowRect(hWndDlg, &arect);
		int		hm, wm, ha, wa;
		hm = mrect.bottom - mrect.top;
		wm = mrect.right - mrect.left;
		ha = arect.bottom - arect.top;
		wa = arect.right - arect.left;
		MoveWindow(hWndDlg, mrect.left + (wm - wa) / 2, mrect.top + (hm - ha) / 2, wa, ha, 1);

//		char	buffer[64];
//		sprintf(buffer, "Version: %d.%d", ctx->version.major, ctx->version.minor);
//		SetDlgItemText(hWndDlg, IDC_STATIC91, buffer);
//
//		sprintf(buffer, "%d", ctx->opengl.samplesPerPixel);
//		SetDlgItemText(hWndDlg, IDC_STATIC103, buffer);
//
//		sprintf(buffer, "%s", ctx->executable.fullName);
//		SetDlgItemText(hWndDlg, IDC_STATIC105, buffer);
	}
//	CreateHyperLink(GetDlgItem(hWndDlg, IDC_STATIC104));
	break;

	case WM_PAINT:
	{
		hdcDestination = BeginPaint(hWndDlg, &ps);
		BitBlt(hdcDestination, 360, 15, 64, 64, hdcSource[1], 0, 0, SRCAND); // SRCCOPY);
		EndPaint(hWndDlg, &ps);
	}
	break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:		EndDialog(hWndDlg, 0); InvalidateRect(pWnd, 0, 0); break;
		case IDCANCEL:	EndDialog(hWndDlg, 0); break;
//		default:
//			if (HIWORD(wParam) == BN_CLICKED) { //Buttons, checkboxs, labels, static labels clicked
//				switch (LOWORD(wParam))
//				{
//				case IDC_STATIC104:
//					FILE *fp;
//					fopen_s(&fp, PROGRAM_DOCUMENTATION, "r");
//					if (fp)
//					{
//						fclose(fp);
//						ShellExecute(NULL, "open", PROGRAM_DOCUMENTATION_URL, NULL, NULL, SW_SHOWNORMAL);
//					}
//					else
//					{
//						char buffer[1024];
//						sprintf(buffer, "File <%s> not found.", PROGRAM_DOCUMENTATION);
//						MessageBox(ctx->mainWindow, buffer, "Error", 0);
//					}
//					break;
//				}
//			}
		}
		break;

	case WM_DESTROY:
		break;

	case WM_CLOSE:
		EndDialog(hWndDlg, 0);
		break;
	}

	return FALSE;
}

//-----------------------------------------------------------------------------
int ds_general_color_dialog(HWND hOwnerWnd, DS_CTX *ctx, DS_COLOR *clr)
//-----------------------------------------------------------------------------
{
//	General function to bring up the color picker dialog and select a color
//
	CHOOSECOLOR		cc;                 // common dialog box structure 
	static COLORREF acrCustClr[16];		// array of custom colors 
	DWORD			beforeColor;		// initial color selection

	// Initialize CHOOSECOLOR 
	ZeroMemory(&cc, sizeof(cc));
	cc.lStructSize = sizeof(cc);
	cc.hwndOwner = hOwnerWnd;
	cc.lpCustColors = (LPDWORD)acrCustClr;

	// initialize the current color (transform floating point to integer)
	beforeColor = (int)(clr->r * 255) | ((int)(clr->g * 255) << 8) | ((int)(clr->b * 255) << 16);

	// set the dialog color
	cc.rgbResult = beforeColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	ChooseColor(&cc); // bring up color dialog

	if (cc.rgbResult != beforeColor) // color has changed
	{
		// transition color information back to user
		clr->r = (float)((cc.rgbResult >>  0) & 0xff) / (float)255.0;
		clr->g = (float)((cc.rgbResult >>  8) & 0xff) / (float)255.0;
		clr->b = (float)((cc.rgbResult >> 16) & 0xff) / (float)255.0;
		return 1;
	}
	else
		return 0;
}

//-----------------------------------------------------------------------------
static void ds_dlg_spin_update(DS_CTX *ctx, HWND dlg, int control, int what, char *buffer)
//-----------------------------------------------------------------------------
{
	int		flag = 0;
	float	value;

	GetDlgItemText(dlg, control, buffer, 256);	// get text from edit control
	value = (float)atof(buffer);

	switch (what) {
	case 0: if (ctx->drawAdj.spin.dx != value) { ctx->drawAdj.spin.dx = value; flag = 1;} break;
	case 1: if (ctx->drawAdj.spin.dy != value) { ctx->drawAdj.spin.dy = value; flag = 1; } break;
	case 2: if (ctx->drawAdj.spin.dz != value) { ctx->drawAdj.spin.dz = value; flag = 1; } break;
	case 3: if (ctx->drawAdj.spin.timerMSec != value) { ctx->drawAdj.spin.timerMSec = value; flag = 1; } break;
	}
	if (flag && ctx->drawAdj.spin.spinState == ROTATE) // something is updated and spin is already enabled
	{
		if (ctx->drawAdj.spin.dx == 0 && ctx->drawAdj.spin.dy == 0 && ctx->drawAdj.spin.dz == 0)
		{
			KillTimer(ctx->mainWindow, 0);
		}
		else // then already running so need to reset
		{
			KillTimer(ctx->mainWindow, 0);
			SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
			InvalidateRect(ctx->mainWindow, 0, 0);
		}
	}
	if (flag)
	{
		if (what != 3) sprintf(buffer, "%.3f", value);
		else sprintf(buffer, "%d", (int)value);  
		SetDlgItemText(dlg, control, buffer);
	}
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_attributes(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	// Full scope attributes dialog - designed to be a floating non-modal dialog
	//
	static PAINTSTRUCT		ps;
#define CLAMP_COLOR( color )	\
	if ( color < 0.0 )			\
		color = 0.0;			\
	else if ( color > 1.0 )		\
		color = 1.0;

	char			buffer[256];
	HWND			pWnd;
	DS_CTX				*ctx;
	int				clrUpdate = 0;
	int				i;
	RECT			window;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);

	switch (Msg) {
	case WM_INITDIALOG:
	case WM_PAINT:
	{
			// Geometry Adjustments
			CheckRadioButton(hWndDlg, IDC_RADIO1, IDC_RADIO3, (ctx->geomAdj.polymode[0] == 0 ? IDC_RADIO1 : (ctx->geomAdj.polymode[0] == 1 ? IDC_RADIO2 : IDC_RADIO3)));
			CheckRadioButton(hWndDlg, IDC_RADIO4, IDC_RADIO6, (ctx->geomAdj.polymode[1] == 0 ? IDC_RADIO4 : (ctx->geomAdj.polymode[1] == 1 ? IDC_RADIO5 : IDC_RADIO6)));
			i = ctx->base_geometry.type;
			CheckRadioButton(hWndDlg, IDC_RADIO7, IDC_RADIO10, (i == GEOMETRY_ICOSAHEDRON ? IDC_RADIO7 : (i == GEOMETRY_OCTAHEDRON ? IDC_RADIO8 : (i == GEOMETRY_TETRAHEDRON ? IDC_RADIO9 : IDC_RADIO10))));
			i = ctx->geomAdj.orientation;
			CheckRadioButton(hWndDlg, IDC_RADIO11, IDC_RADIO13, (i == GEOMETRY_ORIENTATION_FACE ? IDC_RADIO11 : (i == GEOMETRY_ORIENTATION_EDGE ? IDC_RADIO12 : IDC_RADIO13)));
			SendDlgItemMessage(hWndDlg, IDC_CHECK18, BM_SETCHECK, (ctx->base_geometry.oneFaceFlag ? BST_CHECKED : BST_UNCHECKED), 0); // 
			SendDlgItemMessage(hWndDlg, IDC_CHECK19, BM_SETCHECK, (ctx->base_geometry.zRotFlag ? BST_CHECKED : BST_UNCHECKED), 0); // 
			SendDlgItemMessage(hWndDlg, IDC_CHECK20, BM_SETCHECK, (ctx->base_geometry.mirrorFlag ? BST_CHECKED : BST_UNCHECKED), 0); // 
			SendDlgItemMessage(hWndDlg, IDC_CHECK17, BM_SETCHECK, (ctx->clrCtl.reverseColorFlag ? BST_CHECKED : BST_UNCHECKED), 0); // 

			// Label settings
			CheckRadioButton(hWndDlg, IDC_RADIO56, IDC_RADIO60, ((int)ctx->label.edge.font   - 4 + IDC_RADIO56));
			CheckRadioButton(hWndDlg, IDC_RADIO61, IDC_RADIO65, ((int)ctx->label.vertex.font - 4 + IDC_RADIO61));
			CheckRadioButton(hWndDlg, IDC_RADIO71, IDC_RADIO75, ((int)ctx->label.face.font   - 4 + IDC_RADIO71));

//			i = ctx->geomAdj.drawWhat;
//			SendDlgItemMessage(hWndDlg, IDC_CHECK22, BM_SETCHECK, (i &  GEOMETRY_DRAW_TRIANGLES ? BST_CHECKED : BST_UNCHECKED), 0);
//			SendDlgItemMessage(hWndDlg, IDC_CHECK23, BM_SETCHECK, (i &  GEOMETRY_DRAW_EDGES     ? BST_CHECKED : BST_UNCHECKED), 0);
//			SendDlgItemMessage(hWndDlg, IDC_CHECK24, BM_SETCHECK, (i &  GEOMETRY_DRAW_VERTICES  ? BST_CHECKED : BST_UNCHECKED), 0);
//			sprintf(buffer, "%.3f", ctx->renderVertex.scale);  SetDlgItemText(hWndDlg, IDC_EDIT24, buffer);

			// Drawing Adjustments - version 2 (Check Boxes)
			SendDlgItemMessage(hWndDlg, IDC_CHECK1, BM_SETCHECK, (ctx->drawAdj.projection == GEOMETRY_PROJECTION_PERPSECTIVE ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK2, BM_SETCHECK, (ctx->drawAdj.normalizeFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK3, BM_SETCHECK, (ctx->drawAdj.circleFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK4, BM_SETCHECK, (ctx->drawAdj.fogFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK5, BM_SETCHECK, (ctx->drawAdj.axiiFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK24,BM_SETCHECK, (ctx->drawAdj.axiiLabelFlag ? BST_CHECKED : BST_UNCHECKED), 0); // label 
			SendDlgItemMessage(hWndDlg, IDC_CHECK6, BM_SETCHECK, (ctx->drawAdj.clipFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK7, BM_SETCHECK, (ctx->drawAdj.spin.spinState ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK8, BM_SETCHECK, (ctx->drawAdj.clipVisibleFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK9, BM_SETCHECK, (ctx->drawAdj.stereoCrossEyeFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
//			ctx->drawAdj.stereoCrossEyeFlag
			SendDlgItemMessage(hWndDlg, IDC_CHECK14, BM_SETCHECK, (ctx->drawAdj.stereoFlag ? BST_CHECKED : BST_UNCHECKED), 0); // normalize 
			SendDlgItemMessage(hWndDlg, IDC_CHECK43, BM_SETCHECK, (ctx->drawAdj.hiResFlag ? BST_CHECKED : BST_UNCHECKED), 0); // quality 
			sprintf(buffer, "%.4f", ctx->drawAdj.clipZIncrement); SetDlgItemText(hWndDlg, IDC_EDIT1, buffer);
			sprintf(buffer, "%.4f", ctx->drawAdj.clipZValue); SetDlgItemText(hWndDlg, IDC_EDIT17, buffer);
			sprintf(buffer, "%d", (int)ctx->drawAdj.spin.timerMSec);  SetDlgItemText(hWndDlg, IDC_EDIT2, buffer);
			sprintf(buffer, "%.3f", ctx->drawAdj.spin.dx);  SetDlgItemText(hWndDlg, IDC_EDIT7, buffer);
			sprintf(buffer, "%.3f", ctx->drawAdj.spin.dy);  SetDlgItemText(hWndDlg, IDC_EDIT8, buffer);
			sprintf(buffer, "%.3f", ctx->drawAdj.spin.dz);  SetDlgItemText(hWndDlg, IDC_EDIT23, buffer);
			sprintf(buffer, "%.3f", ctx->drawAdj.eyeSeparation);  SetDlgItemText(hWndDlg, IDC_EDIT25, buffer);
			GetWindowRect(pWnd, &window);
			{
				int	w, h;
				w = window.right - window.left - WINDOW_SIZE_OFFSET_WIDTH;
				h = window.bottom - window.top - WINDOW_SIZE_OFFSET_HEIGHT;
				sprintf(buffer, "%d", w);  SetDlgItemText(hWndDlg, IDC_EDIT9, buffer);
				sprintf(buffer, "%d", h);  SetDlgItemText(hWndDlg, IDC_EDIT10, buffer);
			}

			// Color Adjustments 
			sprintf(buffer, "%.2f", ctx->clrCtl.face.defaultColor.a);  SetDlgItemText(hWndDlg, IDC_EDIT54, buffer);
			SendDlgItemMessage(hWndDlg, IDC_CHECK15, BM_SETCHECK, (ctx->clrCtl.useLightingFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			sprintf(buffer, "%.3f", ctx->clrCtl.light.x);  SetDlgItemText(hWndDlg, IDC_EDIT20, buffer);
			sprintf(buffer, "%.3f", ctx->clrCtl.light.y);  SetDlgItemText(hWndDlg, IDC_EDIT21, buffer);
			sprintf(buffer, "%.3f", ctx->clrCtl.light.z);  SetDlgItemText(hWndDlg, IDC_EDIT22, buffer);

			// Edge Attributes - init
//			sprintf(buffer, "%.4f", ctx->eAttr.width);  SetDlgItemText(hWndDlg, IDC_EDIT3, buffer);
//			sprintf(buffer, "%.4f", ctx->eAttr.height); SetDlgItemText(hWndDlg, IDC_EDIT4, buffer);
//			sprintf(buffer, "%.4f", ctx->eAttr.offset); SetDlgItemText(hWndDlg, IDC_EDIT5, buffer);

			// Input Modifications - init
			SendDlgItemMessage(hWndDlg, IDC_CHECK8, BM_SETCHECK, (ctx->inputTrans.mirrorFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK12, BM_SETCHECK, (ctx->inputTrans.replicateFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK21, BM_SETCHECK, (ctx->inputTrans.transformFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK13, BM_SETCHECK, (ctx->inputTrans.guaFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK16, BM_SETCHECK, (ctx->inputTrans.guaResultsFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK25, BM_SETCHECK, (ctx->inputTrans.centerAndScaleFlag ? BST_CHECKED : BST_UNCHECKED), 0);

			sprintf(buffer, "%.4f", ctx->inputTrans.zAxis.x);  SetDlgItemText(hWndDlg, IDC_EDIT11, buffer);
			sprintf(buffer, "%.4f", ctx->inputTrans.zAxis.y);  SetDlgItemText(hWndDlg, IDC_EDIT12, buffer);
			sprintf(buffer, "%.4f", ctx->inputTrans.zAxis.z);  SetDlgItemText(hWndDlg, IDC_EDIT13, buffer);
			sprintf(buffer, "%.4f", ctx->inputTrans.yAxis.x);  SetDlgItemText(hWndDlg, IDC_EDIT14, buffer);
			sprintf(buffer, "%.4f", ctx->inputTrans.yAxis.y);  SetDlgItemText(hWndDlg, IDC_EDIT15, buffer);
			sprintf(buffer, "%.4f", ctx->inputTrans.yAxis.z);  SetDlgItemText(hWndDlg, IDC_EDIT16, buffer);

			// Image Capture 
			SetDlgItemText(hWndDlg, IDC_STATIC49, ctx->captureDir); // ctx->imageCapture.directory);
			SetDlgItemText(hWndDlg, IDC_EDIT6, ctx->png.basename);
			sprintf(buffer, "index: %05d", ctx->png.curFrame);     
			SetDlgItemText(hWndDlg, IDC_STATIC51, buffer);		
			SendDlgItemMessage(hWndDlg, IDC_CHECK10, BM_SETCHECK, (ctx->png.stateSaveFlag ? BST_CHECKED : BST_UNCHECKED), 0);
			SendDlgItemMessage(hWndDlg, IDC_CHECK76, BM_SETCHECK, (ctx->png.bwFlag ? BST_CHECKED : BST_UNCHECKED), 0);
		}
		break;

	case WM_COMMAND:
		{
			UINT uiID = LOWORD(wParam);
			UINT uiCode = HIWORD(wParam);   // spin_update ( ctx, what, buffer )  what = 0,1,2

			switch (uiCode){
			case EN_KILLFOCUS:
				switch (LOWORD(wParam)) {
				case IDC_EDIT1:  ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT1,  buffer, (void*)&ctx->drawAdj.clipZIncrement, 1, 4, 0, 0, 0); break;
				case IDC_EDIT2:  ds_dlg_spin_update( ctx, hWndDlg, IDC_EDIT2, 3, buffer);
				case IDC_EDIT3:  ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT3,  buffer, (void*)&ctx->eAttr.width , 1, 3, 0, 0, 0); break;
				case IDC_EDIT4:	 ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT4,  buffer, (void*)&ctx->eAttr.height, 1, 3, 0, 0, 0); break;
				case IDC_EDIT5:	 ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT5,  buffer, (void*)&ctx->eAttr.offset, 1, 3, 0, 0, 0); break;
				case IDC_EDIT6:	 GetDlgItemText(hWndDlg, IDC_EDIT6, buffer, 256);  strcpy(ctx->png.basename, buffer); break;
				case IDC_EDIT7:  ds_dlg_spin_update(ctx, hWndDlg, IDC_EDIT7, 0, buffer);
				case IDC_EDIT8:  ds_dlg_spin_update(ctx, hWndDlg, IDC_EDIT8, 1, buffer);
				case IDC_EDIT11: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT11, buffer, (void*)&ctx->inputTrans.zAxis.x, 1, 4, 0, 0, 0); break;
				case IDC_EDIT12: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT12, buffer, (void*)&ctx->inputTrans.zAxis.y, 1, 4, 0, 0, 0); break;
				case IDC_EDIT13: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT13, buffer, (void*)&ctx->inputTrans.zAxis.z, 1, 4, 0, 0, 0); break;
				case IDC_EDIT14: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT14, buffer, (void*)&ctx->inputTrans.yAxis.x, 1, 4, 0, 0, 0); break;
				case IDC_EDIT15: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT15, buffer, (void*)&ctx->inputTrans.yAxis.y, 1, 4, 0, 0, 0); break;
				case IDC_EDIT16: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT16, buffer, (void*)&ctx->inputTrans.yAxis.z, 1, 4, 0, 0, 0); break;
				case IDC_EDIT17: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT17, buffer, (void*)&ctx->drawAdj.clipZValue, 1, 4, 0, 0, 0); break;
				case IDC_EDIT20: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT20, buffer, (void*)&ctx->clrCtl.light.x, 1, 3, 0, 0, 0); break;
				case IDC_EDIT21: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT21, buffer, (void*)&ctx->clrCtl.light.y, 1, 3, 0, 0, 0); break;
				case IDC_EDIT22: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT22, buffer, (void*)&ctx->clrCtl.light.z, 1, 3, 0, 0, 0); break;
				case IDC_EDIT23:  ds_dlg_spin_update(ctx, hWndDlg, IDC_EDIT23, 2, buffer);
				case IDC_EDIT24: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT24, buffer, (void*)&ctx->renderVertex.scale, 1, 4, 0, 0, 0); break;
				case IDC_EDIT25: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT25, buffer, (void*)&ctx->drawAdj.eyeSeparation, 0, 3, 0, 0, 0); break;
				case IDC_EDIT54: ds_edit_text_update(pWnd, hWndDlg, IDC_EDIT54, buffer, (void*)&ctx->clrCtl.face.defaultColor.a, 0, 2, 1, 0.0, 1.0); break;
				}
				break;
			}
		}
		switch (wParam) { // on command 
		case IDC_RADIO1: ctx->geomAdj.polymode[0] = SendDlgItemMessage(hWndDlg, IDC_RADIO1, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_FILL : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;//; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO2: ctx->geomAdj.polymode[0] = SendDlgItemMessage(hWndDlg, IDC_RADIO2, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_LINE  : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO3: ctx->geomAdj.polymode[0] = SendDlgItemMessage(hWndDlg, IDC_RADIO3, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_POINT : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO4: ctx->geomAdj.polymode[1] = SendDlgItemMessage(hWndDlg, IDC_RADIO4, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_FILL  : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO5: ctx->geomAdj.polymode[1] = SendDlgItemMessage(hWndDlg, IDC_RADIO5, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_LINE  : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO6: ctx->geomAdj.polymode[1] = SendDlgItemMessage(hWndDlg, IDC_RADIO6, BM_GETCHECK, 0, 0) ? GEOMETRY_POLYMODE_POINT : GEOMETRY_POLYMODE_FILL; InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO7:  ctx->base_geometry.type = SendDlgItemMessage(hWndDlg, IDC_RADIO7, BM_GETCHECK, 0, 0) ? GEOMETRY_ICOSAHEDRON : GEOMETRY_ICOSAHEDRON; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO8:  ctx->base_geometry.type = SendDlgItemMessage(hWndDlg, IDC_RADIO8, BM_GETCHECK, 0, 0) ? GEOMETRY_OCTAHEDRON  : GEOMETRY_ICOSAHEDRON; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO9:  ctx->base_geometry.type = SendDlgItemMessage(hWndDlg, IDC_RADIO9, BM_GETCHECK, 0, 0) ? GEOMETRY_TETRAHEDRON : GEOMETRY_ICOSAHEDRON; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO10: ctx->base_geometry.type = SendDlgItemMessage(hWndDlg, IDC_RADIO10, BM_GETCHECK, 0, 0) ? GEOMETRY_CUBEHEDRON  : GEOMETRY_ICOSAHEDRON; InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO11: ctx->geomAdj.orientation = SendDlgItemMessage(hWndDlg, IDC_RADIO11, BM_GETCHECK, 0, 0) ? GEOMETRY_ORIENTATION_FACE : GEOMETRY_ORIENTATION_FACE; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO12: ctx->geomAdj.orientation = SendDlgItemMessage(hWndDlg, IDC_RADIO12, BM_GETCHECK, 0, 0) ? GEOMETRY_ORIENTATION_EDGE : GEOMETRY_ORIENTATION_FACE; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO13: ctx->geomAdj.orientation = SendDlgItemMessage(hWndDlg, IDC_RADIO13, BM_GETCHECK, 0, 0) ? GEOMETRY_ORIENTATION_VERTEX : GEOMETRY_ORIENTATION_FACE; InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO21: ctx->eAttr.type = SendDlgItemMessage(hWndDlg, IDC_RADIO21, BM_GETCHECK, 1, 0) ? 0 : 1; InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO22: ctx->eAttr.type = SendDlgItemMessage(hWndDlg, IDC_RADIO22, BM_GETCHECK, 1, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case IDC_BUTTON4: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &ctx->clrCtl.face.defaultColor); break;
		case IDC_BUTTON5: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &ctx->clrCtl.bkgClear); break;

		case IDC_CHECK1: ctx->drawAdj.projection = SendDlgItemMessage(hWndDlg, IDC_CHECK1, BM_GETCHECK, 0, 0) ? GEOMETRY_PROJECTION_PERPSECTIVE : GEOMETRY_PROJECTION_ORTHOGRAPHIC; ds_reshape(pWnd, ctx->window.width, ctx->window.height); InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK2: ctx->drawAdj.normalizeFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK2, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK3: ctx->drawAdj.circleFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK3, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK4: ctx->drawAdj.fogFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK4, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK5: ctx->drawAdj.axiiFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK5, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK24: ctx->drawAdj.axiiLabelFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK24, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK6: ctx->drawAdj.clipFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK6, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK7: ctx->drawAdj.spin.spinState = SendDlgItemMessage(hWndDlg, IDC_CHECK7, BM_GETCHECK, 0, 0) ? ROTATE : 0; InvalidateRect(pWnd, 0, 0); 
			if (!ctx->drawAdj.spin.spinState ) KillTimer(pWnd, 0);
			else SetTimer(pWnd, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
			break; 
		case IDC_CHECK8:  ctx->inputTrans.mirrorFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK8, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK9:  ctx->drawAdj.stereoCrossEyeFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK9, BM_GETCHECK, 0, 0) ? 1 : 0; ctx->drawAdj.stereoFlag ? InvalidateRect(pWnd, 0, 0) : 0; break;

		case IDC_CHECK10:  ctx->png.stateSaveFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK10, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK76:  ctx->png.bwFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK76, BM_GETCHECK, 0, 0) ? 1 : 0; break;

		case IDC_CHECK12: ctx->inputTrans.replicateFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK12, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK21: ctx->inputTrans.transformFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK21, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK13: ctx->inputTrans.guaFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK13, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK16: ctx->inputTrans.guaResultsFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK16, BM_GETCHECK, 0, 0) ? 1 : 0; break;
		case IDC_CHECK25: ctx->inputTrans.centerAndScaleFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK25, BM_GETCHECK, 0, 0) ? 1 : 0; break;

		case IDC_CHECK14: ctx->drawAdj.stereoFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK14, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0);ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height); break;
		case IDC_CHECK15: ctx->clrCtl.useLightingFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK15, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK17: ctx->clrCtl.reverseColorFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK17, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK18: ctx->base_geometry.oneFaceFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK18, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK19: ctx->base_geometry.zRotFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK19, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK20: ctx->base_geometry.mirrorFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK20, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case IDC_CHECK43: 
			ctx->drawAdj.hiResFlag = SendDlgItemMessage(hWndDlg, IDC_CHECK43, BM_GETCHECK, 0, 0) ? 1 : 0; 
			if (ctx->drawAdj.hiResFlag)
			{
				ctx->drawAdj.quality = &ctx->drawAdj.hiRes;
				ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjHiRes;
			}
			else
			{
				ctx->drawAdj.quality = &ctx->drawAdj.loRes;
				ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjLoRes;
			}
			InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO56: ctx->label.edge.font = GLUT_BITMAP_TIMES_ROMAN_10;	ds_label_update(&ctx->label.edge); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO57: ctx->label.edge.font = GLUT_BITMAP_TIMES_ROMAN_24;	ds_label_update(&ctx->label.edge); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO58: ctx->label.edge.font = GLUT_BITMAP_HELVETICA_10;		ds_label_update(&ctx->label.edge); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO59: ctx->label.edge.font = GLUT_BITMAP_HELVETICA_12;		ds_label_update(&ctx->label.edge); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO60: ctx->label.edge.font = GLUT_BITMAP_HELVETICA_18;		ds_label_update(&ctx->label.edge); InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO61: ctx->label.vertex.font = GLUT_BITMAP_TIMES_ROMAN_10;	ds_label_update(&ctx->label.vertex); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO62: ctx->label.vertex.font = GLUT_BITMAP_TIMES_ROMAN_24;	ds_label_update(&ctx->label.vertex); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO63: ctx->label.vertex.font = GLUT_BITMAP_HELVETICA_10;	ds_label_update(&ctx->label.vertex); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO64: ctx->label.vertex.font = GLUT_BITMAP_HELVETICA_12;	ds_label_update(&ctx->label.vertex); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO65: ctx->label.vertex.font = GLUT_BITMAP_HELVETICA_18;	ds_label_update(&ctx->label.vertex); InvalidateRect(pWnd, 0, 0); break;

		case IDC_RADIO71: ctx->label.face.font = GLUT_BITMAP_TIMES_ROMAN_10;	ds_label_update(&ctx->label.face); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO72: ctx->label.face.font = GLUT_BITMAP_TIMES_ROMAN_24;	ds_label_update(&ctx->label.face); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO73: ctx->label.face.font = GLUT_BITMAP_HELVETICA_10;		ds_label_update(&ctx->label.face); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO74: ctx->label.face.font = GLUT_BITMAP_HELVETICA_12;		ds_label_update(&ctx->label.face); InvalidateRect(pWnd, 0, 0); break;
		case IDC_RADIO75: ctx->label.face.font = GLUT_BITMAP_HELVETICA_18;		ds_label_update(&ctx->label.face); InvalidateRect(pWnd, 0, 0); break;

		case IDCANCEL:
			DestroyWindow(hWndDlg);
			break;

		case IDC_BUTTON1: ctx->trans[0] = ctx->trans[1] = ctx->trans[2] = 0; ctx->rot[0] = ctx->rot[1] = ctx->rot[2] = 0; mtx_set_unity(&ctx->matrix);ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height); InvalidateRect(pWnd, 0, 0); break; // reset rotation/translation
		case IDC_BUTTON7: ds_capture_image(ctx->mainWindow); InvalidateRect(hWndDlg, 0, 0);
			sprintf(buffer, "index: %05d", ctx->png.curFrame); SetDlgItemText(hWndDlg, IDC_STATIC51, buffer);break;

		case IDC_BUTTON6:
			{
				//Default folder set
				TCHAR szDir[MAX_PATH];
				BROWSEINFO bInfo;
				bInfo.hwndOwner = hWndDlg;
				bInfo.pidlRoot = NULL;
				bInfo.pszDisplayName = szDir; // Address of a buffer to receive the display name of the folder selected by the user
				bInfo.lpszTitle = "Select image capture folder"; // Title of the dialog
				bInfo.lParam = 0;
				bInfo.ulFlags = 0;
				bInfo.lpfn = NULL;
				bInfo.iImage = -1;

				LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
				if (lpItem != NULL)
				{
					SHGetPathFromIDList(lpItem, szDir);
					SetDlgItemText(hWndDlg, IDC_STATIC49, szDir);
//					strcpy(ctx->curWorkingDir, szDir); // save new current working directory
					strcpy(ctx->captureDir, szDir); // save new current working directory
					ds_build_dsf(&ctx->capDir, ctx->captureDir, 0); // update capture directory
//					SetCurrentDirectory(szDir);
				}
			}
			break;

		case IDC_BUTTON8:
			{
				GetWindowRect(pWnd, &window);
				int  w, h;
				GetDlgItemText(hWndDlg, IDC_EDIT9, buffer, 256); sscanf(buffer, "%d", &w);
				GetDlgItemText(hWndDlg, IDC_EDIT10, buffer, 256); sscanf(buffer, "%d", &h);
				w += WINDOW_SIZE_OFFSET_WIDTH;
				h += WINDOW_SIZE_OFFSET_HEIGHT;
				MoveWindow(pWnd, window.left, window.top, w, h, 1);// need to add extra
			}
			break;

		case IDC_BUTTON9: // reset frame counter
			ctx->png.curFrame = 0;
			sprintf(buffer, "index: %05d", ctx->png.curFrame);
			SetDlgItemText(hWndDlg, IDC_STATIC51, buffer);
			break;

		case IDC_BUTTON13: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &ctx->label.face.color); break;
		case IDC_BUTTON14: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &ctx->label.edge.color); break;
		case IDC_BUTTON15: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &ctx->label.vertex.color); break;
		}
		if (clrUpdate)
		{
			clrUpdate = 0;
			++ctx->clrCtl.updateFlag;
			InvalidateRect(ctx->mainWindow, 0, 0);
			LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)lParam;
			InvalidateRect((HWND)lParam, 0, 0);
		}
		break;

	case WM_DRAWITEM: // owner drawn 
		{
			LPDRAWITEMSTRUCT	lpdis = (DRAWITEMSTRUCT*)lParam;
			RECT				lpr;
			HBRUSH				hColorBrush;
			DS_COLOR			*clr;
			int					flag = 0;

			switch ((UINT)wParam) {
			case IDC_BUTTON2:  flag = 1;  clr = &ctx->clrCtl.line.override;			break;
			case IDC_BUTTON3:  flag = 1;  clr = &ctx->clrCtl.face.override;			break;
			case IDC_BUTTON4:  flag = 1;  clr = &ctx->clrCtl.face.defaultColor;		break;
			case IDC_BUTTON5:  flag = 1;  clr = &ctx->clrCtl.bkgClear;				break;
			case IDC_BUTTON12: flag = 1;  clr = &ctx->renderVertex.clr;				break;
			case IDC_BUTTON13: flag = 1;  clr = &ctx->label.face.color;				break;
			case IDC_BUTTON14: flag = 1;  clr = &ctx->label.edge.color;				break;
			case IDC_BUTTON15: flag = 1;  clr = &ctx->label.vertex.color;			break;
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

	case WM_DESTROY:
		ctx->attrControl = 0;
		DestroyWindow(hWndDlg);
		break;

	case WM_CLOSE:
		ctx->attrControl = 0;
		DestroyWindow(hWndDlg);
		break;
	}

	return FALSE; // FALSE;
}
//-----------------------------------------------------------------------------
char *ds_name_start(char *name)
//-----------------------------------------------------------------------------
{
	char c, *p=name;
	while (c = *p++)
		;
	p -= 2;
	while (c = *p--)
	{
		if (c == '/' || c == '\\')
			return p + 2;
	}
	return name;
}

//-----------------------------------------------------------------------------
int ds_set_object_state /*SetObjectState*/(DS_CTX *ctx, int index, int state)
//-----------------------------------------------------------------------------
{
	int					i = 0;
	DS_GEO_OBJECT		*gobj;

	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (i++ == index)
		{
			gobj->active = state;
			return 0;
		}
	}
	return 0;
}
//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_dlg_object_information (HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	HWND				pWnd, lWnd;
	DS_CTX					*ctx;
	int					clrUpdate = 0;
	int					i;
	static LVCOLUMN		column[6];
	static LVITEM		item;
	char				temp[256];
	DS_GEO_OBJECT		*gobj;

	pWnd = GetWindow(hWndDlg, GW_OWNER);
	ctx = (DS_CTX*)GetWindowLong(pWnd, GWL_USERDATA);
	lWnd = GetDlgItem(hWndDlg, IDC_LIST2);

	switch (Msg) {
	case WM_INITDIALOG:
		// Here we put the info on the Column headers
		// this is not data, only name of each header we like
		column[0].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[0].pszText = "Object";                            // First Header Text
		column[0].cx = 0x72;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 0, (LPARAM)&column[0]); // Insert/Show the coloum
		column[1].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[1].pszText = "#Faces";                            // First Header Text
		column[1].cx = 0x42;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 1, (LPARAM)&column[1]); // Insert/Show the coloum
		column[2].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[2].pszText = "#Vertices";                            // First Header Text
		column[2].cx = 0x42;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 2, (LPARAM)&column[2]); // Insert/Show the coloum
		column[3].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[3].pszText = "#Edges";                            // First Header Text
		column[3].cx = 0x42;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 3, (LPARAM)&column[3]); // Insert/Show the coloum
		column[4].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[4].pszText = "#UFaces";                            // First Header Text
		column[4].cx = 0x42;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 4, (LPARAM)&column[4]); // Insert/Show the coloum
		column[5].mask = LVCF_TEXT | LVCF_WIDTH | LVCF_SUBITEM;    // Type of mask
		column[5].pszText = "#UEdges";                            // First Header Text
		column[5].cx = 0x42;                                   // width of column	SendMessage(lWnd, LVM_INSERTCOLUMN, 0, &column[0]);
		SendMessage(lWnd, LVM_INSERTCOLUMN, 5, (LPARAM)&column[5]); // Insert/Show the coloum
//		break;
		SendMessage(lWnd, LVM_DELETEALLITEMS, 0, 0);
		i = 0;
		memset(&item, 0, sizeof(item)); // Zero struct's Members
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			item.mask = LVIF_TEXT;   // Text Style
			item.cchTextMax = 256; // Max size of test
			item.iItem = i;          // choose item  

			item.iSubItem = 0;       // Put in first column
			item.pszText = ds_name_start(gobj->filename); // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_INSERTITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 1;       // Put in first column
			sprintf(temp, "%d", gobj->nTri);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 2;       // Put in first coluom
			sprintf(temp, "%d", gobj->nVtx);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 3;       // Put in first coluom
			sprintf(temp, "%d", gobj->nEdge);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 4;       // Put in first coluom
			sprintf(temp, "%d", gobj->nUTri);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 5;       // Put in first coluom
			sprintf(temp, "%d", gobj->nUEdge);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview
			++i;
		}
		break;

	case WM_USER + 1000:
		SendMessage(lWnd, LVM_DELETEALLITEMS, 0, 0);
		i = 0;
		memset(&item, 0, sizeof(item)); // Zero struct's Members
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			item.mask = LVIF_TEXT;   // Text Style
			item.cchTextMax = 256; // Max size of test
			item.iItem = i;          // choose item  

			item.iSubItem = 0;       // Put in first column
			item.pszText = ds_name_start(gobj->filename); // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_INSERTITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 1;       // Put in first column
			sprintf(temp, "%d", gobj->nTri);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 2;       // Put in first coluom
			sprintf(temp, "%d", gobj->nVtx);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 3;       // Put in first coluom
			sprintf(temp, "%d", gobj->nEdge);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 4;       // Put in first coluom
			sprintf(temp, "%d", gobj->nUTri);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview

			item.iSubItem = 5;       // Put in first coluom
			sprintf(temp, "%d", gobj->nUEdge);
			item.pszText = temp; // Text to display (can be from a char variable) (Items)
			SendMessage(lWnd, LVM_SETITEM, 0, (LPARAM)&item); // Send to the Listview
			++i;
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDOK:
			ctx->objInfo = 0;
			DestroyWindow(hWndDlg);
			InvalidateRect(pWnd, 0, 0);
		case IDCANCEL:
			ctx->objInfo = 0;
			DestroyWindow(hWndDlg);
		}
		break;

	case WM_QUIT:
		ctx->objInfo = 0;
		DestroyWindow(hWndDlg);
		break;

	case WM_DESTROY:
		ctx->objInfo = 0;
		DestroyWindow(hWndDlg);
		break;

	case WM_CLOSE:
		ctx->objInfo = 0;
		DestroyWindow(hWndDlg);
		break;
	}

	return FALSE;
}

//
// The following code from web: https://stackoverflow.com/questions/7784975/c-win32-add-a-hyperlink-to-a-dialog
//
//-----------------------------------------------------------------------------
static void CreateHyperLink(HWND hwndControl)
//-----------------------------------------------------------------------------
{
	// Subclass the parent so we can color the controls as we desire.
	HWND hwndParent = GetParent(hwndControl);
	if (NULL != hwndParent)
	{
		WNDPROC pfnOrigProc = (WNDPROC)GetWindowLong(hwndParent, GWL_WNDPROC);
		if (pfnOrigProc != _HyperlinkParentProc)
		{
			SetProp(hwndParent, PROP_ORIGINAL_PROC, (HANDLE)pfnOrigProc);
			SetWindowLong(hwndParent, GWL_WNDPROC, (LONG)(WNDPROC)_HyperlinkParentProc);
		}
	}

	// Make sure the control will send notifications.
	DWORD dwStyle = GetWindowLong(hwndControl, GWL_STYLE);
	SetWindowLong(hwndControl, GWL_STYLE, dwStyle | SS_NOTIFY);

	// Subclass the existing control.
	WNDPROC pfnOrigProc = (WNDPROC)GetWindowLong(hwndControl, GWL_WNDPROC);
	SetProp(hwndControl, PROP_ORIGINAL_PROC, (HANDLE)pfnOrigProc);
	SetWindowLong(hwndControl, GWL_WNDPROC, (LONG)(WNDPROC)_HyperlinkProc);

	// Create an updated font by adding an underline.
	HFONT hOrigFont = (HFONT)SendMessage(hwndControl, WM_GETFONT, 0, 0);
	SetProp(hwndControl, PROP_ORIGINAL_FONT, (HANDLE)hOrigFont);

	LOGFONT lf;
	GetObject(hOrigFont, sizeof(lf), &lf);
	lf.lfUnderline = TRUE;

	HFONT hFont = CreateFontIndirect(&lf);
	SetProp(hwndControl, PROP_UNDERLINE_FONT, (HANDLE)hFont);

	// Set a flag on the control so we know what color it should be.
	SetProp(hwndControl, PROP_STATIC_HYPERLINK, (HANDLE)1);
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK _HyperlinkParentProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	WNDPROC pfnOrigProc = (WNDPROC)GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_CTLCOLORSTATIC:
	{
		HDC hdc = (HDC)wParam;
		HWND hwndCtl = (HWND)lParam;

		BOOL fHyperlink = (NULL != GetProp(hwndCtl, PROP_STATIC_HYPERLINK));
		if (fHyperlink)
		{
			LRESULT lr = CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
			SetTextColor(hdc, RGB(0, 0, 192));
			return lr;
		}

		break;
	}
	case WM_DESTROY:
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pfnOrigProc);
		RemoveProp(hwnd, PROP_ORIGINAL_PROC);
		break;
	}
	}
	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK _HyperlinkProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{
	WNDPROC pfnOrigProc = (WNDPROC)GetProp(hwnd, PROP_ORIGINAL_PROC);

	switch (message)
	{
	case WM_DESTROY:
	{
		SetWindowLong(hwnd, GWL_WNDPROC, (LONG)pfnOrigProc);
		RemoveProp(hwnd, PROP_ORIGINAL_PROC);

		HFONT hOrigFont = (HFONT)GetProp(hwnd, PROP_ORIGINAL_FONT);
		SendMessage(hwnd, WM_SETFONT, (WPARAM)hOrigFont, 0);
		RemoveProp(hwnd, PROP_ORIGINAL_FONT);

		HFONT hFont = (HFONT)GetProp(hwnd, PROP_UNDERLINE_FONT);
		DeleteObject(hFont);
		RemoveProp(hwnd, PROP_UNDERLINE_FONT);

		RemoveProp(hwnd, PROP_STATIC_HYPERLINK);

		break;
	}
	case WM_MOUSEMOVE:
	{
		if (GetCapture() != hwnd)
		{
			HFONT hFont = (HFONT)GetProp(hwnd, PROP_UNDERLINE_FONT);
			SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
			InvalidateRect(hwnd, NULL, FALSE);
			SetCapture(hwnd);
		}
		else
		{
			RECT rect;
			GetWindowRect(hwnd, &rect);

			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			ClientToScreen(hwnd, &pt);

			if (!PtInRect(&rect, pt))
			{
				HFONT hFont = (HFONT)GetProp(hwnd, PROP_ORIGINAL_FONT);
				SendMessage(hwnd, WM_SETFONT, (WPARAM)hFont, FALSE);
				InvalidateRect(hwnd, NULL, FALSE);
				ReleaseCapture();
			}
		}
		break;
	}
	case WM_SETCURSOR:
	{
		// Since IDC_HAND is not available on all operating systems,
		// we will load the arrow cursor if IDC_HAND is not present.
		HCURSOR hCursor = LoadCursor(NULL, IDC_HAND);
		if (NULL == hCursor)
			hCursor = LoadCursor(NULL, IDC_ARROW);
		SetCursor(hCursor);
		return TRUE;
	}
	}

	return CallWindowProc(pfnOrigProc, hwnd, message, wParam, lParam);
}