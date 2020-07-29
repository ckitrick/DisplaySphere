/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle most of the file access and control.
	Also includes some interaction between window titles and files.
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <direct.h>
#include <commdlg.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <matrix.h>
#include <geoutil.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_par.h"
#include <link.h>
#include <mem.h>
#include "ds_file.h"
#include "ds_gua.h"

//-----------------------------------------------------------------------------
int ds_file_set_window_text(HWND hWnd, char *name)
//-----------------------------------------------------------------------------
{
	DS_CTX		*ctx;

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);
	SetWindowText(hWnd, name);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_type_initialization ( DS_CTX *ctx, char *filename )
//-----------------------------------------------------------------------------
{
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_initialization( HWND hWnd, char *filename )
//-----------------------------------------------------------------------------
{
	char		c, *p;
	FILE		*fp;
	DS_CTX		*ctx;
//	char	buffer[128], buf1[24], buf2[12];

	ctx = (DS_CTX*)GetWindowLong ( hWnd, GWL_USERDATA );

	p = filename + ( strlen(filename ) - 1 );
	while (c = *p--)
	{
		if (c == '\\')
		{
			++p;
			break;
		}
	}

	fopen_s(&fp, filename, "r" );

	if (fp)
	{
		ds_file_set_window_text(hWnd, p + 1);
		ds_parse_file(ctx, fp, filename);
		fclose(fp);
	}

	return 0;
}

//-----------------------------------------------------------------------------
int ds_process_restore_file (DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	RECT		before;
	int			w, h;

	GetWindowRect(ctx->mainWindow, &before);
	w = before.right - before.left - WINDOW_SIZE_OFFSET_WIDTH;
	h = before.bottom - before.top - WINDOW_SIZE_OFFSET_HEIGHT;

	if (ds_restore_state(ctx, filename))
	{
		MessageBox(NULL, "State restoration contained errors.", 0, MB_OK);
		return 1;
	}
	else
	{
		if (ctx->window.start_x != before.left || ctx->window.start_y != before.top ||
			ctx->window.width != w || ctx->window.height != h)
		{
			ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height);
			MoveWindow(ctx->mainWindow, ctx->window.start_x, ctx->window.start_y, ctx->window.width + WINDOW_SIZE_OFFSET_WIDTH, ctx->window.height + WINDOW_SIZE_OFFSET_HEIGHT, 1); // fix size & position
		}
		if (ctx->window.toolsVisible)
		{
			if (!ctx->attrControl)
			{
				ctx->attrControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG5), ctx->mainWindow, ds_dlg_attributes);
				ShowWindow(ctx->attrControl, SW_SHOW);
				ds_position_window(ctx, ctx->attrControl, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
			if (!ctx->objControl)
			{
				ctx->objControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), ctx->mainWindow, ds_dlg_object_control);
				//			ctx->objControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG7), hWnd, DlgObjectControl5);
				ShowWindow(ctx->objControl, SW_SHOW);
				ds_position_window(ctx, ctx->objControl, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
		}
		InvalidateRect(ctx->mainWindow, 0, 0);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int ds_open_file_dialog (HWND hOwnerWnd, DS_CTX *ctx, int type)
//-----------------------------------------------------------------------------
{
	char			pFilter[] = "Geometry Data (*.off,*.spc)\0*.off;*.spc\0OFF Data (*.off)\0*.off;\0SPC Data (*.spc)\0*.spc;\0All Files (*.*)\0*.*\0\0";
	char			cFilter[] = "Color Table Files (*.txt)\0*.txt\0All Files (*.*)\0*.*\0\0";
	char			sFilter[] = "State Files (*.dss,*.txt)\0*.dss;*.txt\0All Files (*.*)\0*.*\0\0";
	char			*p;
	BOOL			bRes;
	OPENFILENAME	ofn;
	FILE			*fp;

	ctx->filename[0]      = 0; // null terminate
	ofn.lStructSize       = sizeof( OPENFILENAME );
	ofn.hwndOwner         = hOwnerWnd;
	ofn.hInstance         = 0; //hInstance;
	ofn.lpstrFilter       = !type ? pFilter : ( type==1 ? cFilter : sFilter );
	ofn.lpstrCustomFilter = 0;
	ofn.nMaxCustFilter    = 0;
	ofn.nFilterIndex      = 0;
	ofn.lpstrFile         = ctx->filename;
	ofn.nMaxFile          = _MAX_PATH;
	ofn.lpstrFileTitle    = 0;
	ofn.nMaxFileTitle     = _MAX_FNAME+_MAX_EXT;
	ofn.lpstrInitialDir   = 0;
	ofn.lpstrTitle        = 0;
	ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	ofn.nFileOffset       = 0;
	ofn.nFileExtension    = 0;
	ofn.lpstrDefExt       = "off";
	ofn.lCustData         = 0L;
	ofn.lpfnHook          = 0;
	ofn.lpTemplateName    = 0;

	bRes = GetOpenFileName ( &ofn );
	if (!bRes)
		return 0; // no file selected 

	if (!type) // geometry file
	{
		p = ctx->filename + (strlen(ctx->filename) - 1);
		while (*p != '\\')
		{
			--p;
		}
		ds_file_set_window_text(hOwnerWnd, p + 1);


		// repeat
		fopen_s(&fp,ctx->filename, "r");
		if (fp)
		{
			ds_file_type_initialization(ctx, p + 1);
			ds_parse_file(ctx, fp, ctx->filename);
		}
		fclose(fp);
	}
	else if (type == 1) // color tables 
	{
		if (ds_ctbl_process_color_table_file(&ctx->cts, ctx->filename))
			MessageBox(NULL, "Color table file read failed.", 0, MB_OK);
	}
	else if (type == 2) // sat tables 
	{
		return ds_process_restore_file(ctx, ctx->filename);
	}
	return 1;
}

//-----------------------------------------------------------------------------
int ds_write_file_dialog (HWND hOwnerWnd, DS_CTX *ctx, int type)
//-----------------------------------------------------------------------------
{
	char			sFilter[] = "State Files (*.dss)\0*.dss\0All Files (*.*)\0*.*\0\0";
	OPENFILENAME	ofn;

	ctx->filename[0] = 0; // null terminate
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hOwnerWnd;
	ofn.hInstance = 0; //hInstance;
	ofn.lpstrFilter = sFilter; // !type ? pFilter : (itype == 1 ? cFilter : sFilter);
	ofn.lpstrCustomFilter = 0;
	ofn.nMaxCustFilter = 0;
	ofn.nFilterIndex = 0;
	ofn.lpstrFile = ctx->filename;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrFileTitle = 0;
	ofn.nMaxFileTitle = _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir = 0;
	ofn.lpstrTitle = 0;
	ofn.Flags = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	ofn.nFileOffset = 0;
	ofn.nFileExtension = 0;
	ofn.lpstrDefExt = "off";
	ofn.lCustData = 0L;
	ofn.lpfnHook = 0;
	ofn.lpTemplateName = 0;

	return GetSaveFileName(&ofn);
}

//-----------------------------------------------------------------------------
int ds_read_file_from_buffer (DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	FILE	*fp;

	if ( !strlen ( ctx->filename ) )
		return 1;

	fopen_s(&fp, ctx->filename, "r");
	if (fp)
	{
		ds_parse_file(ctx, fp, ctx->filename);
		fclose(fp);
	}
	else
		MessageBox(NULL, "File read failed.", 0, MB_OK);

	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_drag_and_drop( HWND hWnd, HDROP hdrop )
//-----------------------------------------------------------------------------
{
	char			buffer[256];
	unsigned int	count, index;
	DS_CTX				*ctx;

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);

	// determine if a file is available
	if (count = DragQueryFileA(hdrop, 0xFFFFFFFF, (LPSTR)buffer, (UINT)256))
	{
		for (index = 0; index < count; ++index)
		{
			if (index)
				ctx->gobjAddFlag = 1;

			// get the new filename
			DragQueryFile(hdrop, index, (LPSTR)buffer, (UINT)256);

			// reset the menus
			ds_file_initialization(hWnd, buffer);// , 0);
	
			ctx->gobjAddFlag = 0;
		}
	}

	// release system resources 
	DragFinish ( hdrop );
	return 0;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_geo_object_find(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	DS_GEO_OBJECT	*gobj;

	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (!strcmp(gobj->filename, filename))
			return gobj;
	}

	return 0;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_geo_object_create(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	// initialize a geometric object
	DS_GEO_OBJECT	*gobj, *lgobj;

	gobj = malloc(sizeof(DS_GEO_OBJECT));
	if (!gobj)
		return 0;

	// memory allocated pointers 
	gobj->filename	= 0; 
	gobj->v_out		= 0;
	gobj->tri		= 0;
	gobj->edge		= 0;
	// assigned data/pointers
	gobj->nTri		= 0;
	gobj->nEdge		= 0;
	gobj->nVtx		= 0;
	gobj->ctT		= 0;
	gobj->ctE		= 0;
	gobj->ctB		= 0;
	gobj->vIndex    = 0;

	// inherit default settings
	gobj->active	= ctx->defInputObj.active;
	gobj->drawWhat	= ctx->defInputObj.drawWhat;
	gobj->eAttr		= ctx->defInputObj.eAttr;
	gobj->cAttr		= ctx->defInputObj.cAttr;
	gobj->vAttr		= ctx->defInputObj.vAttr;
	gobj->rAttr		= ctx->defInputObj.rAttr; 

	gobj->filename = malloc(strlen(filename) + 1);
	strcpy(gobj->filename, filename);

	if (!ctx->gobjAddFlag) // remove all the previous geometry
	{
		ctx->eAttr.maxLength = 0.0;
		ctx->eAttr.minLength = 1000000000.0;

		while (lgobj = (DS_GEO_OBJECT*)LL_RemoveHead(ctx->gobjectq))
		{	// free up allocated memory
			lgobj->filename ? free(lgobj->filename) : 0;
			lgobj->v_out	? free(lgobj->v_out)	: 0;
			lgobj->tri		? free(lgobj->tri)		: 0;
			lgobj->edge		? free(lgobj->edge)		: 0;
			lgobj->vIndex   ? free(lgobj->vIndex)   : 0;
			lgobj			? free ( lgobj )		: 0; // free self
		}
	}

	LL_AddTail(ctx->gobjectq, gobj); // add to queue

	return gobj;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_parse_file(DS_CTX *ctx, FILE *fp, char *filename)
//-----------------------------------------------------------------------------
{
	void			*gua;
	DS_GEO_OBJECT	*geo;
	int				resultsFlag = 0;
	int				fileType;
	int				guaFlag = 0;
	enum {
		FILE_OFF,
		FILE_SPC
	};

	// atempt to read OFF format first 
	fileType = gua_off_read(ctx, (void*)&gua, fp, (float*)&ctx->clrCtl.triangle.defaultColor);

	switch (fileType) {
	case FILE_OFF:
		guaFlag = 1; // normal GUA data created
		break;
	case FILE_SPC:
		if (ctx->inputTrans.guaFlag)
		{
			guaFlag = 1;// normal GUA data created
			gua_spc_read(ctx, (void*)&gua, fp, (float*)&ctx->clrCtl.triangle.defaultColor);
		}
		else
		{	// NGUA data created
			ngua_spc_read(ctx, (void*)&gua, fp, (float*)&ctx->clrCtl.triangle.defaultColor);
		}
	}

	geo = ds_geo_object_create(ctx, filename); // create, initialize, and add to queue
	if (guaFlag && ctx->inputTrans.guaResultsFlag)
	{

		int			i, n = 0;
		char		buffer[512];
		n = strlen(filename);
		for (i = n - 1; i >= 0; --i)
			if (filename[i] == '.')
			{
				sprintf(buffer, "%.*s-unique.txt", i, filename);
				break;
			}
		if (!i)
			sprintf(buffer, "%s-unique.txt", filename);
		gua_dump(ctx, gua, filename, buffer);
	}
	if ( guaFlag)
		gua_convert_to_object(ctx, gua, geo);
	else
		ngua_convert_to_object(ctx, gua, geo);

	return geo;
}
