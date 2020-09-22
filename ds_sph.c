/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	File handles the program main functionality and control.
*/

#include <stdio.h>
#include <io.h>
#include <fcntl.h>
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
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

#define _WIN32_DCOM
//WINOLEAPI  CoInitializeEx(LPVOID pvReserved, DWORD dwCoInit);
LONG WINAPI WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LONG WINAPI WindowProc2(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
//void ds_reshape(HWND hWnd, int width, int height);

#define clamp(x) x = x > 360.0f ? x-360.0f : x < -360.0f ? x+=360.0f : x

//-----------------------------------------------------------------------------
static void ds_update ( DS_CTX *ctx, int state, int ox, int nx, int oy, int ny )
//-----------------------------------------------------------------------------
{
	int dx = ox - nx;
	int dy = ny - oy;

	switch(state) {
	case PAN:
		ctx->trans[0] -= dx / 100.0f;
		ctx->trans[1] -= dy / 100.0f;
		break;

	case ROTATE:
		ctx->rot[0] += (dy * 180.0f) / 500.0f;
		ctx->rot[1] -= (dx * 180.0f) / 500.0f;
		clamp(ctx->rot[0]);
		clamp(ctx->rot[1]);
		{
			MTX_MATRIX	mrx,
				mry,
				mm;

			mtx_create_rotation_matrix(&mrx, MTX_ROTATE_X_AXIS, DTR(ctx->rot[0]));
			mtx_multiply_matrix(&ctx->matrix, &mrx, &mm);
			mtx_create_rotation_matrix(&mry, MTX_ROTATE_Y_AXIS, DTR(ctx->rot[1]));
			mtx_multiply_matrix(&mm, &mry, &ctx->matrix);
		}
		ctx->rot[0] = ctx->rot[1] = ctx->rot[2] = 0;
		break;

	case ROTATE_Z:
		ctx->rot[2] += (-dy * 180.0f) / 500.0f;
		clamp(ctx->rot[2]);
		{
			MTX_MATRIX	mrx, mm;

			mtx_create_rotation_matrix(&mrx, MTX_ROTATE_Z_AXIS, DTR(ctx->rot[2]));
			mtx_multiply_matrix(&ctx->matrix, &mrx, &mm);
			ctx->matrix = mm;
		}
		ctx->rot[0] = ctx->rot[1] = ctx->rot[2] = 0;
		break;

	case 3:
	case 5:
//	case ZOOM:
		ctx->trans[2] -= (dx+dy) / 100.0f;
		ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height);
		break;
	}
}

//-----------------------------------------------------------------------------
static void ds_update3(DS_CTX *ctx, int dx, int dy, int dz )
//-----------------------------------------------------------------------------
{
	ctx->rot[0] += (float)(dy/2.0);
	ctx->rot[1] += (float)(dx/2.0);
	ctx->rot[2] += (float)(dz/2.0);
	clamp(ctx->rot[0]);
	clamp(ctx->rot[1]);
	clamp(ctx->rot[2]);
	{
		MTX_MATRIX	m_src, m_dst;

		mtx_create_rotation_matrix(&m_src, MTX_ROTATE_X_AXIS, DTR(ctx->rot[0]));
		mtx_multiply_matrix(&ctx->matrix, &m_src, &m_dst);
		mtx_create_rotation_matrix(&m_src, MTX_ROTATE_Y_AXIS, DTR(ctx->rot[1]));
		mtx_multiply_matrix(&m_dst, &m_src, &ctx->matrix);
		mtx_create_rotation_matrix(&m_src, MTX_ROTATE_Z_AXIS, DTR(ctx->rot[2]));
		mtx_multiply_matrix(&ctx->matrix, &m_src, &m_dst);
		ctx->matrix = m_dst;
	}
	ctx->rot[0] = ctx->rot[1] = ctx->rot[2] = 0;
}

//-----------------------------------------------------------------------------
static void ds_update2 ( DS_CTX *ctx, int state, double dx, double dy, double dz )
//-----------------------------------------------------------------------------
{
	switch (state) {
	case PAN:
		ctx->trans[0] -= (float)dx / 100.0f;
		ctx->trans[1] -= (float)dy / 100.0f;
		break;
	
	case ROTATE:
		ctx->rot[0] = (float)dx;
		ctx->rot[1] = (float)dy;
		ctx->rot[2] = (float)dz;
		{
			MTX_MATRIX	mrx,
						mry,
						mm;

			mtx_create_rotation_matrix(&mrx, MTX_ROTATE_X_AXIS, DTR(ctx->rot[0]));
			mtx_multiply_matrix(&ctx->matrix, &mrx, &mm);
			mtx_create_rotation_matrix(&mry, MTX_ROTATE_Y_AXIS, DTR(ctx->rot[1]));
			mtx_multiply_matrix(&mm, &mry, &ctx->matrix);
			mtx_create_rotation_matrix(&mry, MTX_ROTATE_Z_AXIS, DTR(ctx->rot[2]));
			mtx_multiply_matrix(&ctx->matrix, &mry, &mm);
			ctx->matrix = mm;
		}
		break;
	
	case 3:
	case 5:
//	case ZOOM:
		ctx->trans[2] -= ((float)dx + (float)dy) / 100.0f;
		ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height);
		break;
	}
}

//-----------------------------------------------------------------------------
void ds_reshape ( HWND hWnd, int width, int height )
//-----------------------------------------------------------------------------
{
	DS_CTX		*ctx;
	float	ar;
//	float	zoom; 

	ctx = (DS_CTX*)GetWindowLong ( hWnd, GWL_USERDATA );
	if (!ctx)
		return;

	ctx->window.width  = width;
	ctx->window.height = height;
	ar = (float)(width) / (float)height;

	glViewport( 0, 0, width, height );
    glMatrixMode ( GL_PROJECTION );
    glLoadIdentity ();
	if ( ctx->drawAdj.projection == GEOMETRY_PROJECTION_ORTHOGRAPHIC )
	{
		double	size = 1.045;
		double  zoom;
		//        left,  right,  bottom, top, nearVal, farVal
		zoom = -ctx->trans[2]*0.25 + 1;
		glOrtho(-size*ar*zoom , size*ar*zoom, -size*zoom, size*zoom, -100.00f, 100.0f);
	}
	else if (ctx->drawAdj.projection == GEOMETRY_PROJECTION_PERPSECTIVE )
	{
		float	f, 
				zn, 
				zf;
		GLfloat	m[16];

		// projective
		f = (float)( 1.0 / tan ( DTR( 15.0 ) ) );
		zn = 0.001f;
		zf = 100.0f;
		
		m[0] = f / ar;
		m[1] = 0.0f;
		m[2] = 0.0f;
		m[3] = 0.0f;

		m[4] = 0.0f;
		m[5] = f;
		m[6] = 0.0f;
		m[7] = 0.0f;

		m[8] = 0.0f;
		m[9] = 0.0f;
		m[10] = ( zf + zn ) / ( zn - zf );
		m[11] = -1.0f ;

		m[12] = 0.0f;
		m[13] = 0.0f;
		m[14] = ( 2.0f * zf * zn ) / ( zn - zf );
		m[15] = 0.0f; // 0.0f;

		glLoadMatrixf ( m );
	}
    glMatrixMode ( GL_MODELVIEW );
    glLoadIdentity ();
	glTranslatef(0.f, 0.f, -4.0f);
}

//-----------------------------------------------------------------------------
void ds_position_window(DS_CTX *ctx, HWND hWnd, int flag, RECT *rect)
//-----------------------------------------------------------------------------
{
	// only call when tool window is first visible 
	HMONITOR	monitor = MonitorFromWindow(ctx->mainWindow, MONITOR_DEFAULTTONEAREST);
	MONITORINFO	info;
	RECT		mrect, trect; // arect, trect;
	int			left, top;
	int			x_offset = 10, y_offset = 10;

	info.cbSize = sizeof(MONITORINFO);
	GetMonitorInfo(monitor, &info);

	int mon_w, main_w, tool_w; 
	int mon_h, main_h, tool_h;
	mon_w = info.rcMonitor.right - info.rcMonitor.left;
	mon_h = info.rcMonitor.bottom - info.rcMonitor.top;
	GetWindowRect(ctx->mainWindow, &mrect);
	main_w = mrect.right - mrect.left;
	main_h = mrect.bottom - mrect.top;
	GetWindowRect(hWnd, &trect);
	tool_w = trect.right - trect.left;
	tool_h = trect.bottom - trect.top;

	if (hWnd == ctx->attrControl)
	{
		top = mrect.top + y_offset;
		if (mrect.right + tool_w + x_offset < info.rcMonitor.right)
		{ // normal
			left = mrect.right + x_offset;
		}
		else
		{
			left = info.rcMonitor.right - tool_w - x_offset;
		}
		MoveWindow(hWnd, left, top, tool_w, tool_h, 1);
	}
	else if (hWnd == ctx->objControl)
	{
		left = mrect.left + x_offset;
		if (mrect.bottom + tool_h + y_offset < info.rcMonitor.bottom)
		{ // normal
			top = mrect.bottom + y_offset;
		}
		else
		{
			top = info.rcMonitor.bottom - tool_h - y_offset;
		}
		MoveWindow(hWnd, left, top, tool_w, tool_h, 1);
	}
}

//-----------------------------------------------------------------------------
void ds_set_render_quality(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	if (ctx->drawAdj.hiResFlag) // need to reset pointers
	{
		ctx->drawAdj.quality = &ctx->drawAdj.hiRes; // edge 
		ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjHiRes; // vertex
	}
	else
	{
		ctx->drawAdj.quality = &ctx->drawAdj.loRes; // edge
		ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjLoRes; // vertex
	}
}

//-----------------------------------------------------------------------------
static void ds_update_obj_control_window (DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	RECT	rect;
	// if window visible then save top,left position
	if (ctx->objControl)
	{
		GetWindowRect(ctx->objControl, &rect); // get existing rectangle
		DestroyWindow(ctx->objControl); //destroy existing
		ctx->objControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), ctx->mainWindow, ds_dlg_object_control);
		ShowWindow(ctx->objControl, SW_SHOW);// make window visible
		ds_position_window(ctx, ctx->objControl, 1, &rect); // move windows to appropriate spots - bottom or right of the current window 
	}
}

//-----------------------------------------------------------------------------
LONG WINAPI WindowProc ( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
//-----------------------------------------------------------------------------
{
    static PAINTSTRUCT	ps;
    static GLboolean	left  = GL_FALSE;	/* left button currently down? */
    static GLboolean	right = GL_FALSE;	/* right button currently down? */
    static GLuint		state   = 0;	/* mouse state flag */
    static int			omx = 0, omy=0, mx=0, my=0, keyState=0;
	DS_CTX					*ctx;
	static int			w, h; //WINDOW_DATA	before = ctx->window;
	static RECT			before;

	ctx = (DS_CTX*)GetWindowLong ( hWnd, GWL_USERDATA ); // user context has been stored in the window structure

	switch ( uMsg ) {
	case WM_CREATE:
		DragAcceptFiles ( hWnd, TRUE );
		break;

	case WM_SHOWWINDOW:
		// open tool windows 
		if (ctx && ctx->window.toolsVisible)
		{
			if (!ctx->attrControl)
			{
				ctx->attrControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG5), hWnd, ds_dlg_attributes);
				ShowWindow(ctx->attrControl, SW_SHOW);
				ds_position_window(ctx, ctx->attrControl, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
			if (!ctx->objControl)
			{
				ctx->objControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), hWnd, ds_dlg_object_control);
				ShowWindow(ctx->objControl, SW_SHOW);
				ds_position_window(ctx, ctx->objControl, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
		}
		InvalidateRect(hWnd, 0, 0);
		break;

	case WM_COMMAND: 
        // Test for the identifier of a command item. 
		switch (LOWORD(wParam)) {
		case ID_FILE_OPEN_POLY:
			ds_open_file_dialog(hWnd, ctx, 0);   // application-defined 
			//SetCurrentDirectory(ctx->curWorkingDir);
			ds_update_obj_control_window(ctx);
			InvalidateRect(hWnd, 0, 0);
			if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER+1000, 0, 0);
			break;

		case ID_FILE_OPEN_POLY_ADD:
			ctx->gobjAddFlag = 1;
			ds_open_file_dialog(hWnd, ctx, 0);   // application-defined 
			//SetCurrentDirectory(ctx->curWorkingDir);
			ds_update_obj_control_window(ctx);
			ctx->gobjAddFlag = 0;
			InvalidateRect(hWnd, 0, 0);
			if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER + 1000, 0, 0);
			break;

		case ID_FILE_READCOLORTABLES:
			ds_open_file_dialog(hWnd, ctx, 1);   // application-defined 
			//SetCurrentDirectory(ctx->curWorkingDir);
			InvalidateRect(hWnd, 0, 0);
			break;

//		case ID_FILE_REPEAT:
//			if (!ds_read_file_from_buffer(ctx)) {
//				//SetCurrentDirectory(ctx->curWorkingDir);
//				InvalidateRect(hWnd, 0, 0);
//				if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER + 1000, 0, 0);
//				if (ctx->objControl) SendMessage(ctx->objControl, WM_PAINT, 0, 0);//init
//			}
//			break;

		case ID_FILE_EXIT:
			ds_exit(ctx);
			exit(0);
			break;

		case ID_FILE_SAVESTATE: //ds_save_state(ctx, "C:/TEMP/ds_state.txt"); break;
			if (ds_write_file_dialog(hWnd, ctx, 0))
			{
				ds_save_state(ctx, ctx->filename); // "C:/TEMP/ds_state.txt");
				//SetCurrentDirectory(ctx->curWorkingDir);
			}
			break;

		case ID_FILE_RESTORESTATE: //ds_restore_state(ctx, "C:/TEMP/ds_state.txt"); break;
			ds_open_file_dialog(hWnd, ctx, 2);
			//SetCurrentDirectory(ctx->curWorkingDir);
			if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER + 1000, 0, 0);
			break;

		case ID_FILE_DEFAULTSTATE: //ds_restore_state(ctx, "C:/TEMP/ds_state.txt"); break;
			ds_pre_init2(ctx);
			SetWindowText(ctx->mainWindow, "Display Sphere");
			ds_post_init2(ctx);
			InvalidateRect(hWnd, 0, 0);
			break;

		case ID_FILE_ABOUT:
			DialogBox(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG1), hWnd, ds_dlg_about);
			break;
		
		case ID_FILE_ATTRIBUTECONTROL:
			if (!ctx->attrControl)
			{
				ctx->attrControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG5), hWnd, ds_dlg_attributes);
				ShowWindow(ctx->attrControl, SW_SHOW);
				ds_position_window(ctx, ctx->attrControl,0,0);
			}
			break;

		case ID_FILE_OBJECTCONTROL:
			if (!ctx->objControl)
			{
				ctx->objControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), hWnd, ds_dlg_object_control);
				ShowWindow(ctx->objControl, SW_SHOW);
				ds_position_window(ctx, ctx->objControl,0,0); // move windows to appropriate spots - bottom or right of the current window 
			}
			break;

		case ID_FILE_OBJECTINFORMATION:
			if (!ctx->objInfo)
			{
				ctx->objInfo = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG6), hWnd, ds_dlg_object_information);
				ShowWindow(ctx->objInfo, SW_SHOW);
//				ds_position_window(ctx, ctx->objInfo); // move windows to appropriate spots - bottom or right of the current window 
			}
			break;

		case ID_SHOW_TOGGLE:
			if (!ctx->kbdToggle)
			{
				ctx->kbdToggle = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG2), hWnd, ds_dlg_kbd_toggles);
				ShowWindow(ctx->kbdToggle, SW_SHOW);
			}
			break;
		case ID_FILE_HELP:
			{
				char		url[1024];
				ds_build_url(ctx, url);
				ShellExecute(NULL, "open", url, NULL, NULL, SW_SHOWNORMAL);
			}
			break;
		}
		break;

	case WM_DROPFILES:
		if ( GetKeyState(VK_SHIFT) & 0x1000 ) // check high order bit to see if key is down
			ctx->gobjAddFlag = 1; // return to default state
		ds_file_drag_and_drop( hWnd, (HDROP)wParam );
		SetCurrentDirectory(ctx->currentDir);
		InvalidateRect(hWnd, 0, 0);
		if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER + 1000, 0, 0);
		break;

	case WM_SIZE:
		ds_reshape( hWnd, LOWORD( lParam ), HIWORD( lParam ) );
		InvalidateRect(hWnd, 0, 0);
		if(ctx->attrControl) SendMessage(ctx->attrControl, WM_PAINT, 0, 0);//init
		break;

    case WM_KEYDOWN: 
		switch (wParam) {
		case VK_LEFT:		ds_update3(ctx, -1, 0, 0); InvalidateRect(hWnd, 0, 0); break;
		case VK_RIGHT:		ds_update3(ctx, 1, 0, 0); InvalidateRect(hWnd, 0, 0); break;
		case VK_UP:			ds_update3(ctx, 0, -1, 0); InvalidateRect(hWnd, 0, 0); break;
		case VK_DOWN:		ds_update3(ctx, 0, 1, 0); InvalidateRect(hWnd, 0, 0); break;
		case VK_PRIOR:		ds_update3(ctx, 0, 0, 1); InvalidateRect(hWnd, 0, 0); break;
		case VK_NEXT:		ds_update3(ctx, 0, 0, -1); InvalidateRect(hWnd, 0, 0); break;
		case 'O':
		case 'o':
			if (GetKeyState(VK_CONTROL) < 0) SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_OPEN_POLY, 0); break;
		case 'A':
		case 'a':
			if (GetKeyState(VK_CONTROL) < 0) SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_OPEN_POLY_ADD, 0); break;
		case 'I':
		case 'i':
			if (GetKeyState(VK_CONTROL) < 0) SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_OBJECTINFORMATION, 0); break;
		case 'H':
		case 'h':
			if (GetKeyState(VK_CONTROL) < 0) SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_HELP, 0); break;
		}
		break;

    case WM_CHAR:
		switch (wParam) {
		case '0': ctx->drawAdj.circleFlag = !ctx->drawAdj.circleFlag; break; /* 2 key */
		case 'A':
		case 'a': ctx->drawAdj.axiiFlag = !ctx->drawAdj.axiiFlag; break; // reset drawing of axii
		case 'B':
		case 'b': ctx->geomAdj.polymode[1] += 1; if (ctx->geomAdj.polymode[1] > 2) ctx->geomAdj.polymode[1] = 0; break;// back polymode
		case 'C':
		case 'c': ctx->drawAdj.clipFlag = !ctx->drawAdj.clipFlag; break;
		case 'E':
		case 'e':
			SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_OBJECTCONTROL, 0); break;
		case 'F':
		case 'f': ctx->geomAdj.polymode[0] += 1; if (ctx->geomAdj.polymode[0] > 2) ctx->geomAdj.polymode[0] = 0; break;// front polymode
		case 'G':
		case 'g': // change base geometry
			switch (ctx->base_geometry.type) {
			case GEOMETRY_ICOSAHEDRON: ctx->base_geometry.type = GEOMETRY_OCTAHEDRON;  break;
			case GEOMETRY_OCTAHEDRON:  ctx->base_geometry.type = GEOMETRY_TETRAHEDRON; break;
			case GEOMETRY_TETRAHEDRON: ctx->base_geometry.type = GEOMETRY_CUBEHEDRON;  break;
			case GEOMETRY_CUBEHEDRON:  ctx->base_geometry.type = GEOMETRY_ICOSAHEDRON; break;
			}
			ds_file_set_window_text(hWnd, ctx->filename);
			break;
		case 'H':
		case 'h': ctx->drawAdj.hiResFlag = !ctx->drawAdj.hiResFlag; ds_set_render_quality(ctx); break;
		case 'I':
		case 'i': ds_capture_image(hWnd); break;
		case 'K':
		case 'k':
			SendMessage(ctx->mainWindow, WM_COMMAND, ID_SHOW_TOGGLE, 0); break;
		case 'N':
		case 'n': ctx->drawAdj.normalizeFlag = ctx->drawAdj.normalizeFlag ? 0 : 1; break; //global_normalize = ctx->global_normalize ? 0 : 1; break;			/* n key */
		case 'O':
		case 'o': // change base geometry orientation
			switch (ctx->geomAdj.orientation) {
			case GEOMETRY_ORIENTATION_FACE: ctx->geomAdj.orientation = GEOMETRY_ORIENTATION_EDGE; break;
			case GEOMETRY_ORIENTATION_EDGE: ctx->geomAdj.orientation = GEOMETRY_ORIENTATION_VERTEX; break;
			case GEOMETRY_ORIENTATION_VERTEX: ctx->geomAdj.orientation = GEOMETRY_ORIENTATION_FACE; break;
			}
			ds_file_set_window_text(hWnd, ctx->filename);
			break;
		case 'P':
		case 'p': ctx->drawAdj.projection = !ctx->drawAdj.projection; ds_reshape(hWnd, ctx->window.width, ctx->window.height); break;	/* p key - switch from perspective to orthographic projection */
		case 'R':
		case 'r': ctx->trans[0] = ctx->trans[1] = ctx->trans[2] = 0; ctx->rot[0] = ctx->rot[1] = ctx->rot[2] = 0; mtx_set_unity(&ctx->matrix); ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height); break; // reset rotation/translation
		case 'S':
		case 's': if (ctx->drawAdj.spin.spinState) { ctx->drawAdj.spin.spinState = 0; KillTimer(hWnd, 0); }
				  else { ctx->drawAdj.spin.spinState = ROTATE; SetTimer(hWnd, 0, (int)ctx->drawAdj.spin.timerMSec, 0); } break; // reset spin of axii
		case 'W':
		case 'w':
			SendMessage(ctx->mainWindow, WM_COMMAND, ID_FILE_ATTRIBUTECONTROL, 0); break;
		case 'Z':
		case 'z': ctx->drawAdj.fogFlag  = ctx->drawAdj.fogFlag ? 0 : 1; break;
		case '_':
		case '-':
			if ( ctx->drawAdj.clipFlag )
			{
				ctx->drawAdj.clipZValue -= ctx->drawAdj.clipZIncrement; // clip.z_increment;
				if ( ctx->drawAdj.clipZValue > 1.0 )
					ctx->drawAdj.clipZValue = 1.0;
				else if ( ctx->drawAdj.clipZValue < -1.0 )
					ctx->drawAdj.clipZValue = -1.0;
			}
			break;
		case '+':
		case '=':
			if (ctx->drawAdj.clipFlag)
			{
				ctx->drawAdj.clipZValue += ctx->drawAdj.clipZIncrement; // clip.z_increment;
				if (ctx->drawAdj.clipZValue > 1.0)
					ctx->drawAdj.clipZValue = 1.0;
				else if (ctx->drawAdj.clipZValue < -1.0)
					ctx->drawAdj.clipZValue = -1.0;
			}
			break;
		case 27:			/* ESC key */
			PostQuitMessage(0);
			ds_exit(ctx);
			exit(0);
			break;

		case '%': // NOT DOCUMENTED
			ctx->relativeObjPathFlag = 1; break;
		}
		InvalidateRect(hWnd, 0, 0);
		if (ctx->attrControl) SendMessage(ctx->attrControl, WM_PAINT, 0, 0);//init
		break;

    case WM_LBUTTONDOWN:
    case WM_RBUTTONDOWN:
		/* if we don't set the capture we won't get mouse move
			   messages when the mouse moves outside the window. */
		SetCapture(hWnd);
//		keyState = GetKeyState(VK_SHIFT);
		mx = LOWORD(lParam);
		my = HIWORD(lParam);
		if (uMsg == WM_LBUTTONDOWN )
			state |= PAN;
		if (uMsg == WM_RBUTTONDOWN)
			state |= ( GetKeyState(VK_SHIFT) & 0x8000 )  ? ROTATE_Z : ROTATE;
		break; 

    case WM_LBUTTONUP:
    case WM_RBUTTONUP: /* remember to release the capture when we are finished. */
		ReleaseCapture();
		state = 0;
		keyState = 0;
		break;

    case WM_MOUSEMOVE:
		if ( state ) {
			omx = mx;
			omy = my;
			mx = LOWORD(lParam);
			my = HIWORD(lParam);
			/* Win32 is pretty braindead about the x, y position that
			   it returns when the mouse is off the left or top edge
			   of the window (due to them being unsigned). therefore,
			   roll the Win32's 0..2^16 pointer co-ord range to the
			   more amenable (and useful) 0..+/-2^15. */
			if ( mx & 1 << 15 )
				mx -= (1 << 16);
			if ( my & 1 << 15 )
				my -= (1 << 16);
			ds_update ( ctx, state, omx, mx, omy, my );
			InvalidateRect(hWnd, 0, 0);
		}
		break; 

    case WM_PALETTECHANGED:
		if ( hWnd == (HWND)wParam )
			break;
		/* fall through to WM_QUERYNEWPALETTE */
		break;

    case WM_QUERYNEWPALETTE:
		if ( ctx->hPalette ) {
			UnrealizeObject (ctx->hPalette );
			SelectPalette (ctx->hDC, ctx->hPalette, FALSE );
			RealizePalette (ctx->hDC );
		}
		break;

	case WM_CLOSE:
		PostQuitMessage(0);
		ds_exit(ctx);
		exit(0);
		break;

	case WM_PAINT:
		BeginPaint(hWnd, &ps);
		ds_display_control(ctx);
		EndPaint(hWnd, &ps);
		break;

	case WM_USER+100:
		if (ctx->png.nFrames)//|| ctx->png.singleFlag )
		{
			int		spinState = ctx->drawAdj.spin.spinState;

			ctx->drawAdj.spin.spinState = 0; // turn off spin 
			ds_capture_image(hWnd);
			ctx->drawAdj.spin.spinState = spinState; // restore spin

			--ctx->png.nFrames;
			if ( !ctx->png.nFrames )//|| ctx->png.singleFlag)
				PostQuitMessage(0); // all done
		}
		break;

	case WM_TIMER:
		if (ctx->drawAdj.spin.spinState)
		{
			if (ctx->png.nFrames)
			{
				ds_update2(ctx, ctx->drawAdj.spin.spinState, ctx->drawAdj.spin.dx, ctx->drawAdj.spin.dy, ctx->drawAdj.spin.dz); // ds_update transformation matrix
				InvalidateRect(hWnd, 0, 0);
				PostMessage(hWnd, WM_USER + 100, 0, 0);
			}
			else
			{
				ds_update2(ctx,ctx->drawAdj.spin.spinState, ctx->drawAdj.spin.dx, ctx->drawAdj.spin.dy, ctx->drawAdj.spin.dz); // ds_update transformation matrix
				InvalidateRect(hWnd, 0, 0);
			}
		}
		break;
    }

	return DefWindowProc ( hWnd, uMsg, wParam, lParam );
} 

//-----------------------------------------------------------------------------
int ds_command_line_options (DS_CTX *ctx, LPSTR lpszCmdLine)
//-----------------------------------------------------------------------------
{
	// decode command line options
	char	*p = (char*)lpszCmdLine;

	char	buffer[1024], curArg[128];
	char	*av[64];
	int		ac,
			error=0,
			nextAc,
			badArgIndex;

	// make copy of the command line
	strncpy(buffer, (char*)lpszCmdLine, 1024);

	// parse the command string
	ctx->ac = ac = ds_parse_lexeme(buffer, av, 64);
	nextAc = 0;
	curArg[0] = 0;

	ctx->errorInfo.count = 0;
	ds_command_line(ctx, ac, av, &error, &badArgIndex, &ctx->errorInfo); // NEED TO HANDLE ERRORS
	if (error)
	{
		char	buffer[128];
		int		i;
		buffer[0] = 0;
		strcat(buffer, "An error in the command line was encountered at: ");
		for (i = 0; i < ctx->errorInfo.count; ++i)
		{
			strcat(buffer, ctx->errorInfo.text[i]);
			strcat(buffer, ",");
		}
		strcat(buffer, ">");

		//badArgIndex = 0;
//		sprintf(buffer, "An error in the command line was encountered at <%s><%s>", av[badArgIndex], curArg);
		MessageBox(NULL, buffer, "Command Line Error", MB_OK);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int APIENTRY WinMain ( HINSTANCE hCurrentInst, HINSTANCE hPreviousInst, LPSTR lpszCmdLine, int nCmdShow )
//-----------------------------------------------------------------------------
{
	HGLRC	hRC;						/* opengl context */
	HWND	hWnd;						/* window */
	MSG		msg;						/* message */
	DWORD	buffer = PFD_DOUBLEBUFFER;	/* buffering type */
	BYTE	color  = PFD_TYPE_RGBA;		/* color type */
	static DS_CTX		ctx;
	int		method = 3; // 2;

	ds_pre_init(&ctx); // initialize DS_CTX structure prior to command line options being processed

	// process the command line
	ds_command_line_options(&ctx, lpszCmdLine);

	hWnd = ds_create_opengl_window(&ctx, "Display Sphere", ctx.window.start_x, ctx.window.start_y, ctx.window.width + WINDOW_SIZE_OFFSET_WIDTH, ctx.window.height + WINDOW_SIZE_OFFSET_HEIGHT);
	if ( hWnd == NULL )
		exit ( 1 );
	ctx.mainWindow = hWnd;

	SetWindowLong ( hWnd, GWL_USERDATA, (long)&ctx );  // attach context to window

	ctx.hDC = GetDC ( hWnd );
	hRC = wglCreateContext (ctx.hDC );
    wglMakeCurrent (ctx.hDC, hRC);

	ds_post_init(&ctx);// , polyhedron); // re-initialize neccessary items after window is created and command line is processed 

	if ( !ctx.ac && ctx.internalDSS )
		ds_process_restore_file(&ctx, ctx.internalDSS);

    ShowWindow(hWnd, nCmdShow);
		
	ds_dlg_splash_screen(&ctx);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if (   !IsDialogMessage(ctx.kbdToggle, &msg)
			&& !IsDialogMessage(ctx.attrControl, &msg)
			&& !IsDialogMessage(ctx.objControl, &msg)
			)
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

//	ReleaseDC ( ctx.hDC, hWnd );
	ReleaseDC ( hWnd, ctx.hDC );
	wglDeleteContext ( hRC );
    DestroyWindow ( hWnd );

    if ( ctx.hPalette )
		DeleteObject ( ctx.hPalette );

    return 0;
}

void *font = GLUT_BITMAP_HELVETICA_18;

//-----------------------------------------------------------------------------
void label(float x, float y, float z, float nz, char *string)
//-----------------------------------------------------------------------------
{
	int				len,
		i;
	static char		buf[36];
	static int		first = 1;
	static char		*av[2];
	static GLubyte	bitmap[10];

	if (first)
	{
		i = 0;
		first = 0;
		glutInit(&i, av);
	}

	if (nz < 0.6)
		return;

	glColor3f(0.0, 0.0, 0.0);
	glRasterPos3f(x, y, z);
	len = (int)strlen(string);
	glBitmap((GLsizei)0, (GLsizei)0, (GLfloat)0, (GLfloat)0, (GLfloat)(-len*5.0), (GLfloat)-7.0, &bitmap);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, string[i]);
//		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
	}
}

//-----------------------------------------------------------------------------
void textout ( float offset, float x, float y, float z, char *string )
//-----------------------------------------------------------------------------
{
	int				len,
					i;
	static char		buf[36];
	static int		first = 1;
	static char		*av[2];
	static GLubyte	bitmap[10];
	if (first)
	{
		i = 0;
		first = 0;
		glutInit(&i, av);
	}
	if ( fabs ( x ) <= 0.0000001 )
		x = 0;
	if ( fabs ( y ) <= 0.0000001 )
		y = 0;
	if ( fabs ( z ) <= 0.0000001 )
		z = 0;

	// format string
	sprintf ( buf, "%6.4f", z );

	x *= offset;
	y *= offset;
	z *= offset;

	glColor3f(0.0, 0.0, 0.0);
	glRasterPos3f( x, y, z );
	len = (int)strlen(string);
	glBitmap((GLsizei)0, (GLsizei)0, (GLfloat)0, (GLfloat)0, (GLfloat)(-len*5.0), (GLfloat)-7.0, &bitmap);
	for (i = 0; i < len; i++)
	{
		glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, string[i]);
	}

}

//--------------------------------------  ---------------------------------------
static int ds_file_drag_and_drop ( HWND hWnd, HDROP hdrop )
//-----------------------------------------------------------------------------
{
	// DESCRIPTION: Respond to a file drag and drop event.
	//
	// INPUT:
	//    hdrop  Drag and Drop context
	//
	char			buffer[256];
	unsigned int	count, index;
	DS_CTX				*ctx;

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);

	// determine if a file is available
	if ( count = DragQueryFile( hdrop, 0xFFFFFFFF, (LPSTR)buffer, (UINT)256 ) )
	{
		for (index = 0; index < count; ++index)
		{
			if (index)
				ctx->gobjAddFlag = 1;

			// get the new filename
			DragQueryFile(hdrop, index, (LPSTR)buffer, (UINT)256);

			// special check for state and color files
			{
				FILE	*fp;
				char	buf[256];
				char	*array[64];
				int		argCount;
				fopen_s(&fp, buffer, "r");
				if (fp)
				{
					buf[0] = 0;
					fgets(buf, 256, fp);
					argCount = ds_parse_lexeme(buf, array, 64); // split line into words
					if (argCount)
					{
						if (!strcmp(array[0], "DS_STATE"))
						{
							ctx->gobjAddFlag = 0;
							fclose(fp);
							return ds_process_restore_file(ctx, buffer);
						}
						else if (!strcmp(array[0], "DS_COLOR"))
						{
							ctx->gobjAddFlag = 0;
							fclose(fp);
							return ds_ctbl_process_color_table_file(&ctx->cts, buffer);
						}
					}
				}
				else
				{
					char buffer[128];
					sprintf(buffer, "Dragged file <%s> failed to open.", buffer);
					MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
				}
				fclose(fp);
			}
			// reset the menus
			ds_file_initialization (hWnd, buffer);
		}
	}

	// release system resources 
	DragFinish ( hdrop );
	ctx->gobjAddFlag = 0; // return to default state

	ds_update_obj_control_window(ctx);
	if (ctx->objInfo) SendMessage(ctx->objInfo, WM_USER+1000, 0, 0);
	return 0;
}

//-----------------------------------------------------------------------------
LRESULT CALLBACK ds_window_procedure(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
//-----------------------------------------------------------------------------
{

	switch (message) {
	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE) {
			PostQuitMessage(0);
		}
		break;
	case WM_CLOSE:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;       // message handled
}

//-----------------------------------------------------------------------------
ATOM ds_register_class(HINSTANCE hInstance)
//-----------------------------------------------------------------------------
{

	WNDCLASSEX wcex;
	ZeroMemory(&wcex, sizeof(wcex));
	wcex.cbSize = sizeof(wcex);
	wcex.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wcex.lpfnWndProc = ds_window_procedure; //  WindowProcedure;
//	wcex.lpfnWndProc = (WNDPROC)WindowProc;
	wcex.hInstance = hInstance;
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.lpszClassName = "Core";
//	hInstance = GetModuleHandle(NULL);
//	return RegisterClassEx(&wcex);

	wcex.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS;
	wcex.lpfnWndProc = (WNDPROC)WindowProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(NULL, (LPCSTR)IDI_ICON1);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = NULL;
	//exwc.lpszMenuName  = NULL;
	wcex.lpszMenuName = MAKEINTRESOURCE(IDR_MENU1);
//	wcex.lpszClassName = "OpenGL";
	wcex.hIconSm = LoadImage(hInstance,
		MAKEINTRESOURCE(IDI_ICON1),
		IMAGE_ICON,
		16,
		16,
		0);

	return RegisterClassEx(&wcex);
}

//-----------------------------------------------------------------------------
HWND ds_create_opengl_window(DS_CTX *ctx, char* title, int x, int y, int width, int height )
//-----------------------------------------------------------------------------
{
//	WNDCLASSEX				wc; //	wndclass ;

//	static HINSTANCE		hInstance = 0;

	if (!ctx->hInstance)
		ds_register_class(ctx->hInstance);

	/* only register the window class once - use hInstance as a flag. */
	HWND fakeWND = CreateWindow(
		"Core", "Fake Window",      // window class, title
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // style
		0, 0,                       // position x, y
		1, 1,                       // width, height
		NULL, NULL,                 // parent window, menu
		ctx->hInstance, NULL);           // instance, param

	HDC fakeDC = GetDC(fakeWND);        // Device Context
	PIXELFORMATDESCRIPTOR fakePFD;
	ZeroMemory(&fakePFD, sizeof(fakePFD));
	fakePFD.nSize		= sizeof(fakePFD);
	fakePFD.nVersion	= 1;
	fakePFD.dwFlags		= PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
	fakePFD.iPixelType	= PFD_TYPE_RGBA;
	fakePFD.cColorBits	= 32;
	fakePFD.cAlphaBits	= 8;
	fakePFD.cDepthBits	= 24;

	int fakePFDID = ChoosePixelFormat(fakeDC, &fakePFD);

	if (fakePFDID == 0) {
		MessageBox(NULL, (LPCSTR)"ChoosePixelFormat() failed.",0,MB_OK);
		return 0;
	}

	if (SetPixelFormat(fakeDC, fakePFDID, &fakePFD) == FALSE) {
		MessageBox((HWND)NULL, (LPCSTR)"SetPixelFormat() failed.", 0, MB_OK);
		return 0;
	}

	HGLRC fakeRC = wglCreateContext(fakeDC);    // Rendering Contex

	if (fakeRC == 0) {
		MessageBox((HWND)NULL, (LPCSTR)"wglCreateContext() failed.", 0, MB_OK);
		return 0;
	}

	if (wglMakeCurrent(fakeDC, fakeRC) == FALSE) {
		MessageBox((HWND)NULL, (LPCSTR)"wglMakeCurrent() failed.", 0, MB_OK);
		return 0;
	}

	PFNWGLCHOOSEPIXELFORMATARBPROC wglChoosePixelFormatARB = NULL;
	wglChoosePixelFormatARB = (PFNWGLCHOOSEPIXELFORMATARBPROC)wglGetProcAddress("wglChoosePixelFormatARB");
	if (wglChoosePixelFormatARB == NULL) {
		MessageBox((HWND)NULL, (LPCSTR)"wglGetProcAddress() failed.", 0, MB_OK);
		return 0;
	}

	PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = NULL;
	wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	if (wglCreateContextAttribsARB == NULL) {
		MessageBox((HWND)NULL,"wglGetProcAddress() failed.", 0, MB_OK);
		return 0;
	}

	HWND WND = CreateWindow(
		"Core", title,        // class name, window name  WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS | WS_CLIPCHILDREN
		WS_OVERLAPPEDWINDOW | WS_CAPTION | WS_SYSMENU | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, // style
//		config.posX, config.posY,       // posx, posy
//		config.width, config.height,    // width, height
		x, y,
		width, height,
		NULL, NULL,                     // parent window, menu
		ctx->hInstance, NULL);               // instance, param

	HDC			DC = GetDC(WND);
	int			pixelFormatID; 
	UINT		numFormats;
	int			status; 
	int			spp[] = { 32, 16, 12, 8, 4, 2, 1 };
	int			i, start;
	int	pixelAttribs[] = {
		WGL_DRAW_TO_WINDOW_ARB,		GL_TRUE,						// 0, 1
		WGL_SUPPORT_OPENGL_ARB,		GL_TRUE,						// 2, 3
		WGL_DOUBLE_BUFFER_ARB,		GL_TRUE,						// 4, 5
		WGL_PIXEL_TYPE_ARB,			WGL_TYPE_RGBA_ARB,				// 6, 7
		WGL_ACCELERATION_ARB,		WGL_FULL_ACCELERATION_ARB,		// 8, 9 
		WGL_COLOR_BITS_ARB,			32,								// 10, 11
		WGL_ALPHA_BITS_ARB,			8,								// 12
		WGL_DEPTH_BITS_ARB,			24,								// 14
		WGL_STENCIL_BITS_ARB,		8,								// 16
		WGL_SAMPLE_BUFFERS_ARB,		GL_TRUE,						// 18, 19 
		WGL_SAMPLES_ARB, ctx->opengl.samplesPerPixel,				// 20, 21
		0															// 22
	};

	// check range from possible setting
	if (ctx->opengl.samplesPerPixel <= 0 || ctx->opengl.samplesPerPixel > 32)
	{
		start = 0;
	}
	else
	{
		// determine where to start
		for (i = 0, start=0; i < sizeof(spp) / sizeof(int); ++i)
		{
			if (ctx->opengl.samplesPerPixel >= spp[i])
			{
				start = i;
				break;
			}
		}
	}

	// perform in a loop to find highest quality option on available hardware
	for (i=start; i<sizeof(spp)/sizeof(int); ++i)
	{
		pixelAttribs[21] = spp[i];
		status = wglChoosePixelFormatARB(DC, pixelAttribs, NULL, 1, &pixelFormatID, &numFormats);
		if (status != FALSE && numFormats != 0) {
			ctx->opengl.samplesPerPixel = spp[i]; // save the final version
			break;
		}
	}

	if (status == FALSE || numFormats == 0)
	{
		MessageBox((HWND)NULL,"wglChoosePixelFormatARB() failed.",0,MB_OK);
		return 0;
	}

	PIXELFORMATDESCRIPTOR PFD;
	DescribePixelFormat(DC, pixelFormatID, sizeof(PFD), &PFD);
	SetPixelFormat(DC, pixelFormatID, &PFD);

	const int major_min = 4, minor_min = 2; // 5;
	int  contextAttribs[] = {
		WGL_CONTEXT_MAJOR_VERSION_ARB, major_min,
		WGL_CONTEXT_MINOR_VERSION_ARB, minor_min,
		WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
		0
	};

	HGLRC RC = wglCreateContextAttribsARB(DC, 0, contextAttribs);
	if (RC == NULL) {
		MessageBox((HWND)NULL,"wglCreateContextAttribsARB() failed.", 0, MB_OK);
		return 0;
	}

	wglMakeCurrent(NULL, NULL);
	wglDeleteContext(fakeRC);
	ReleaseDC(fakeWND, fakeDC);
	DestroyWindow(fakeWND);
	if (!wglMakeCurrent(DC, RC)) {
		MessageBox((HWND)NULL,"wglMakeCurrent() failed.", 0, MB_OK);
		return 0;
	}

	ReleaseDC(WND, DC);
	return WND;
}

#include "common.h"
#include "bmphed.h"
#include "zlib.h"

BOOL write_png(char *fn, IMAGE *img);

//-----------------------------------------------------------------------------
int ds_capture_image(HWND hWnd)
//-----------------------------------------------------------------------------
{
//	ds_capture_image_black_and_white(hWnd); 
//	return 0;
		//	Captures a screenshot into a window and then saves it in a .bmp file.
	HDC			hdcWindow;
	HDC			hdcMemDC = NULL;
	HBITMAP		hbmWindow = NULL;
	BITMAP		bmpWindow;
	DS_CTX		*ctx;
	char		outputFilename[256];

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);

	// Retrieve the handle to a display device context for the client area of the window. 
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC)
	{
		MessageBox(hWnd, (LPCSTR)L"CreateCompatibleDC has failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	// Create a compatible bitmap from the Window DC
	hbmWindow = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmWindow)
	{
		MessageBox(hWnd, (LPCSTR)L"CreateCompatibleBitmap Failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmWindow);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0,
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		hdcWindow,
		0, 0,
		SRCCOPY))
	{
		MessageBox(hWnd, (LPCSTR)L"BitBlt has failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmWindow, sizeof(BITMAP), &bmpWindow);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize = sizeof(BITMAPINFOHEADER);
	bi.biWidth = bmpWindow.bmWidth;
	bi.biHeight = bmpWindow.bmHeight;
	bi.biPlanes = 1;
	bi.biBitCount = 32;
	bi.biCompression = BI_RGB;
	bi.biSizeImage = 0;
	bi.biXPelsPerMeter = 0;
	bi.biYPelsPerMeter = 0;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;

	DWORD dwBmpSize = ((bmpWindow.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpWindow.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	char *lpbitmap = (char *)GlobalLock(hDIB);

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(	hdcWindow, hbmWindow, 0,
				(UINT)bmpWindow.bmHeight,
				lpbitmap,
				(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	if (ctx->png.bwFlag) 
	{ // image conversion 
		int		w, h, i, n;
		unsigned char *r, *g, *b, gray;

		r = (unsigned char*)&lpbitmap[0];
		g = (unsigned char*)&lpbitmap[1];
		b = (unsigned char*)&lpbitmap[2];
		w = bi.biWidth; // = bmpWindow.bmWidth;
		h = bi.biHeight; // = bmpWindow.bmHeight;
		n = w * h;
		for (i = 0; i < n; ++i)
		{
			gray = (int)(*r * 0.3 + *g * 0.58 + *b * 0.11);
			*r = *b = *g = gray;
			r += 4;
			g += 4;
			b += 4;
		}
	}

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSize + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	{ // Call BMP to PNG library
		IMAGE	image;
		image.width  = bi.biWidth; // = bmpWindow.bmWidth;
		image.height = bi.biHeight; // = bmpWindow.bmHeight;

		image.pixdepth = 32;
		image.palnum   = 0;
		image.topdown  = 0;
		image.alpha    = 0;
		image.palette  = 0;
		//		image.rowbytes = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4;
		//		image.imgbytes = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * bi.biHeight; // lpbitmap;
		image.sigbit.red = image.sigbit.green = image.sigbit.blue = 8;
		image.sigbit.gray = image.sigbit.alpha = 8;
		image.rowbytes = ((DWORD)image.width * image.pixdepth + 31) / 32 * 4;
		image.imgbytes = image.rowbytes * image.height;
		image.rowptr = malloc((size_t)image.height * sizeof(BYTE *));
		image.bmpbits = lpbitmap; // malloc((size_t)image.imgbytes);

		// fill the row pointers
		BYTE *bp, **rp;
		LONG n;

		n = image.height;
		rp = image.rowptr;
		bp = image.bmpbits;

		bp += image.imgbytes;
		while (--n >= 0) {
			bp -= image.rowbytes;
			*(rp++) = bp;
		}

		if (strlen(ctx->captureDir))
			SetCurrentDirectory(ctx->captureDir);

		if (ctx->png.singleFlag) //do not add index
			sprintf(outputFilename, "%s.png", ctx->png.basename); // , ctx->png.curFrame++);
		else
			sprintf(outputFilename, "%s%05d.png", ctx->png.basename, ctx->png.curFrame);

		write_png(outputFilename, &image);
		free(image.rowptr);

		if (ctx->png.stateSaveFlag) //dump a state file
		{
			if (ctx->png.singleFlag) //do not add index
				sprintf(outputFilename, "%s.dss", ctx->png.basename); // , ctx->png.curFrame++);
			else
				sprintf(outputFilename, "%s%05d.dss", ctx->png.basename, ctx->png.curFrame);

			ds_save_state(ctx, outputFilename);
		}

		SetCurrentDirectory(ctx->currentDir);

		if (!ctx->png.singleFlag) //do not add index
			++ctx->png.curFrame;
	}

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
//	GlobalUnlock(hDIB_BW);
	GlobalFree(hDIB);
//	GlobalFree(hDIB_BW);

	done: //Clean up
	DeleteObject(hbmWindow);
	DeleteObject(hdcMemDC);
	ReleaseDC(hWnd, hdcWindow);

	return 0;
}

#ifdef ERR_DEBUG
int dbg_output(DS_CTX *ctx, char *string)
{
	fprintf(ctx->errFile, "%s\n", string);
		fflush(ctx->errFile);
	return 0;
}
#endif

//-----------------------------------------------------------------------------
int ds_capture_image_black_and_white(HWND hWnd)
//-----------------------------------------------------------------------------
{
	//	Captures a screenshot into a window and then saves it in a .bmp file.
	HDC			hdcWindow;
	HDC			hdcMemDC = NULL;
	HBITMAP		hbmWindow = NULL;
	BITMAP		bmpWindow;
	DS_CTX		*ctx;
	char		outputFilename[256];

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);

	// Retrieve the handle to a display device context for the client area of the window. 
	hdcWindow = GetDC(hWnd);

	// Create a compatible DC which is used in a BitBlt from the window DC
	hdcMemDC = CreateCompatibleDC(hdcWindow);

	if (!hdcMemDC)
	{
		MessageBox(hWnd, (LPCSTR)L"CreateCompatibleDC has failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Get the client area for size calculation
	RECT rcClient;
	GetClientRect(hWnd, &rcClient);

	// Create a compatible bitmap from the Window DC
	hbmWindow = CreateCompatibleBitmap(hdcWindow, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);

	if (!hbmWindow)
	{
		MessageBox(hWnd, (LPCSTR)L"CreateCompatibleBitmap Failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Select the compatible bitmap into the compatible memory DC.
	SelectObject(hdcMemDC, hbmWindow);

	// Bit block transfer into our compatible memory DC.
	if (!BitBlt(hdcMemDC,
		0, 0,
		rcClient.right - rcClient.left, rcClient.bottom - rcClient.top,
		hdcWindow,
		0, 0,
		SRCCOPY))
	{
		MessageBox(hWnd, (LPCSTR)L"BitBlt has failed", (LPCSTR)L"Failed", MB_OK);
		goto done;
	}

	// Get the BITMAP from the HBITMAP
	GetObject(hbmWindow, sizeof(BITMAP), &bmpWindow);

	BITMAPFILEHEADER   bmfHeader;
	BITMAPINFOHEADER   bi;

	bi.biSize			= sizeof(BITMAPINFOHEADER);
	bi.biWidth			= bmpWindow.bmWidth;
	bi.biHeight			= bmpWindow.bmHeight;
	bi.biPlanes			= 1;
	bi.biBitCount		= 32;
	bi.biCompression	= BI_RGB;
	bi.biSizeImage		= 0;
	bi.biXPelsPerMeter	= 0;
	bi.biYPelsPerMeter	= 0;
	bi.biClrUsed		= 0;
	bi.biClrImportant	= 0;

	DWORD dwBmpSize   = ((bmpWindow.bmWidth * bi.biBitCount + 31) / 32) * 4 * bmpWindow.bmHeight;
	DWORD dwBmpSizeBW = ((bmpWindow.bmWidth * bi.biBitCount +  7) /  8) * 4 * bmpWindow.bmHeight;

	// Starting with 32-bit Windows, GlobalAlloc and LocalAlloc are implemented as wrapper functions that 
	// call HeapAlloc using a handle to the process's default heap. Therefore, GlobalAlloc and LocalAlloc 
	// have greater overhead than HeapAlloc.
	HANDLE hDIB = GlobalAlloc(GHND, dwBmpSize);
	HANDLE hDIB_BW = GlobalAlloc(GHND, dwBmpSizeBW);
	char *lpbitmap = (char *)GlobalLock(hDIB);
	char *lpbitmapBW = (char *)GlobalLock(hDIB_BW);

	// Gets the "bits" from the bitmap and copies them into a buffer 
	// which is pointed to by lpbitmap.
	GetDIBits(hdcWindow, hbmWindow, 0,
		(UINT)bmpWindow.bmHeight,
		lpbitmap,
		(BITMAPINFO *)&bi, DIB_RGB_COLORS);

	if (1)
	{ // image conversion 
		int		w, h, i, n;
		unsigned char *r, *g, *b, gray, *a;

		a = (unsigned char*)&lpbitmapBW[0];
		r = (unsigned char*)&lpbitmap[0];
		g = (unsigned char*)&lpbitmap[1];
		b = (unsigned char*)&lpbitmap[2];
		w = bi.biWidth; // = bmpWindow.bmWidth;
		h = bi.biHeight; // = bmpWindow.bmHeight;
		n = w * h;
		for (i = 0; i < n; ++i)
		{
			gray = (int)(*r * 0.3 + *g * 0.58 + *b * 0.11);
			*a = gray;
			r += 4;
			g += 4;
			b += 4;
		}
	}

	// Add the size of the headers to the size of the bitmap to get the total file size
	DWORD dwSizeofDIB = dwBmpSizeBW + sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER);

	//Offset to where the actual bitmap bits start.
	bmfHeader.bfOffBits = (DWORD)sizeof(BITMAPFILEHEADER) + (DWORD)sizeof(BITMAPINFOHEADER);

	//Size of the file
	bmfHeader.bfSize = dwSizeofDIB;

	//bfType must always be BM for Bitmaps
	bmfHeader.bfType = 0x4D42; //BM   

	DWORD dwBytesWritten = 0;
	{ // Call BMP to PNG library
		IMAGE	image;
		image.width = bi.biWidth; // = bmpWindow.bmWidth;
		image.height = bi.biHeight; // = bmpWindow.bmHeight;

		image.pixdepth = 32;
		image.palnum =  0;
		image.topdown = 0;
		image.alpha = 0;
		image.palette = 0;
		//		image.rowbytes = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4;
		//		image.imgbytes = ((bi.biWidth * bi.biBitCount + 31) / 32) * 4 * bi.biHeight; // lpbitmap;
		image.sigbit.red = image.sigbit.green = image.sigbit.blue = 8;
		image.sigbit.gray = 8;
		image.sigbit.alpha = 8;
		image.rowbytes = ((DWORD)image.width * image.pixdepth + 7) / 8 * 4;
		image.imgbytes = image.rowbytes * image.height;
		image.rowptr = malloc((size_t)image.height * sizeof(BYTE *));
		image.bmpbits = lpbitmapBW; // malloc((size_t)image.imgbytes);

								  // fill the row pointers
		BYTE *bp, **rp;
		LONG n;

		n = image.height;
		rp = image.rowptr;
		bp = image.bmpbits;

		bp += image.imgbytes;
		while (--n >= 0) {
			bp -= image.rowbytes;
			*(rp++) = bp;
		}

		if (strlen(ctx->captureDir))
			SetCurrentDirectory(ctx->captureDir);

		if (ctx->png.singleFlag) //do not add index
			sprintf(outputFilename, "%s_b.png", ctx->png.basename); // , ctx->png.curFrame++);
		else
			sprintf(outputFilename, "%s%05d_b.png", ctx->png.basename, ctx->png.curFrame);

		write_png(outputFilename, &image);
		free(image.rowptr);

		if (ctx->png.stateSaveFlag) //dump a state file
		{
			if (ctx->png.singleFlag) //do not add index
				sprintf(outputFilename, "%s.dss", ctx->png.basename); // , ctx->png.curFrame++);
			else
				sprintf(outputFilename, "%s%05d.dss", ctx->png.basename, ctx->png.curFrame);

			ds_save_state(ctx, outputFilename);
		}

		SetCurrentDirectory(ctx->currentDir);

		if (!ctx->png.singleFlag) //do not add index
			++ctx->png.curFrame;
	}

	//Unlock and Free the DIB from the heap
	GlobalUnlock(hDIB);
	GlobalFree(hDIB);
	GlobalUnlock(hDIB_BW);
	GlobalFree(hDIB_BW);

done: //Clean up
	DeleteObject(hbmWindow);
	DeleteObject(hdcMemDC);
	ReleaseDC(hWnd, hdcWindow);

	return 0;
}