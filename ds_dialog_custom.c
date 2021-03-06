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
	DS_BASE_COLOR_TRANS_ON			= 1000,
	DS_BASE_COLOR_TRANS_E			= 1100,
	DS_BASE_COLOR_TRANS_O			= 1200,
	DS_BASE_COLOR_TRANS_ALPHA		= 1300,
	DS_BASE_COLOR_EDGE_USE_A		= 1400,
	DS_BASE_COLOR_EDGE_USE_O		= 1500,
	DS_BASE_COLOR_EDGE_SET			= 1600,
	DS_BASE_COLOR_VERTEX_SET		= 1700,
	DS_BASE_EDGE_ROUND				= 1800,
	DS_BASE_EDGE_BOX				= 1900,
	DS_BASE_EDGE_WIDTH				= 2000,
	DS_BASE_EDGE_HEIGHT				= 2100,
	DS_BASE_EDGE_OFFSET				= 2200,
	DS_BASE_VERTEX_SCALE			= 2300,
	DS_BASE_REPLICATE_FACE			= 2400,
	DS_BASE_REPLICATE_Z				= 2500,
	DS_BASE_REPLICATE_X				= 2600,
	DS_BASE_LABEL_FACE				= 2700,
	DS_BASE_LABEL_EDGE				= 2800,
	DS_BASE_LABEL_VERTEX			= 2900,
	DS_BASE_GEO_ORIENTATION			= 3000,
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
	L"Button",	L"Object",			DS_STATIC,		  3, 2, 91,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  //   3, 2, 75,38
	L"Button",	L"Draw",			DS_STATIC,		 96, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  //  82, 2, 40,38
	L"Button",	L"Color",			DS_STATIC,		139, 2,195,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 126, 2,199,38
	L"Button",	L"Face",			DS_STATIC,		139,13, 54,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 126,13, 55,38
	L"Button",	L"Transparency",	DS_STATIC,		195,13, 62,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 184,13, 62,38
	L"Button",	L"Edge",			DS_STATIC,		259,13, 44,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 249,13, 44,38
	L"Button",	L"Vertex",			DS_STATIC,		305,13, 29,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 296,13, 29,38
	L"Button",	L"Edge",			DS_STATIC,		336, 2,103,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 329, 2,106,38
	L"Button",	L"Vertex",			DS_STATIC,		441, 2, 31,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 438, 2, 31,38
	L"Button",	L"Replicate",		DS_STATIC,		474, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 472, 2, 41,38
	L"Button",	L"Labels",			DS_STATIC,		517, 2, 41,38,	WS_VISIBLE | WS_CHILD | BS_GROUPBOX | BS_CENTER,  // 517, 2, 41,38

	L"Static",	L"NAME",			DS_STATIC,		  8,22,65,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Vis",				DS_STATIC,		 55,22,10,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Geo",				DS_STATIC,		 70,22,20,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"F",				DS_STATIC,		102,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		114,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"V",				DS_STATIC,		126,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"E",				DS_STATIC,		146,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"A",				DS_STATIC,		157,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		168,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"On",				DS_STATIC,		199,22, 9,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		213,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		224,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"alpha",			DS_STATIC,		236,22,18,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"A",				DS_STATIC,		265,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"O",				DS_STATIC,		276,22, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"R",				DS_STATIC,		342,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"B",				DS_STATIC,		352,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Width",			DS_STATIC,		363,20,20,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Height",			DS_STATIC,		389,20,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Offset",			DS_STATIC,		414,20,22,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"Scale",			DS_STATIC,		447,20,18,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"1F",				DS_STATIC,		479,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"Z",				DS_STATIC,		493,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"X",				DS_STATIC,		503,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,

	L"Static",	L"F",				DS_STATIC,		524,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"E",				DS_STATIC,		535,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
	L"Static",	L"V",				DS_STATIC,		547,20, 8,8,	WS_VISIBLE | WS_CHILD | SS_LEFT,
};

int nDS_Fixed_Controls = sizeof(ds_obj_fixed) / sizeof(DS_OBJECT_CONTROL);

DS_OBJECT_CONTROL_EX ds_obj_variable[] = {
	(LPCWSTR)WC_STATIC,  L"default",DS_BASE_NAME				,	  8,32,40, 8,	WS_VISIBLE | WS_CHILD | SS_LEFT,								0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_ACTIVE					,	 55,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,

	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_FACE				,	100,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_EDGE				,	112,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_DRAW_VERTEX				,	124,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	// face color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_E		,	144,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,	0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_A		,	155,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_USE_O		,	166,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_FACE_SET			,	177,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// transparency
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_ON			,	199,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_E			,	211,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,	0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_TRANS_O			,	222,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_COLOR_TRANS_ALPHA		,	233,31,21,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// edge color
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_A		,	263,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_USE_O		,	274,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_EDGE_SET			,	286,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// vertex color 
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_COLOR_VERTEX_SET		,	313,32,13, 9,	WS_VISIBLE | WS_CHILD | WS_BORDER | BS_OWNERDRAW | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Edge round/box size
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_ROUND				,	340,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP | WS_GROUP,		0,
	(LPCWSTR)WC_BUTTON,	L"",	DS_BASE_EDGE_BOX				,	350,32,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTORADIOBUTTON | WS_TABSTOP,		0,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_WIDTH				,	361,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_HEIGHT				,	387,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_EDGE_OFFSET				,	412,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Vertex Scale
	(LPCWSTR)WC_EDIT,	L"",	DS_BASE_VERTEX_SCALE			,	445,30,24,12,	WS_VISIBLE | WS_CHILD | WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, WS_EX_CLIENTEDGE,
	// Replication 
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_FACE			,	479,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_Z				,	492,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_REPLICATE_X				,	502,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	// Replication 
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_FACE				,	521,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_EDGE				,	533,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,
	(LPCWSTR)WC_BUTTON,  L"",	DS_BASE_LABEL_VERTEX			,	545,31,10,10,	WS_VISIBLE | WS_CHILD | BS_AUTOCHECKBOX | WS_TABSTOP,			0,

	(LPCWSTR)WC_COMBOBOX,L"",	DS_BASE_GEO_ORIENTATION 		,	 65,31,25,200,	SS_SIMPLE | CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,			0,
}; 
//CBS_DROPDOWN | CBS_HASSTRINGS | WS_CHILD | WS_OVERLAPPED | WS_VISIBLE,

int nDS_Variable_Controls = sizeof(ds_obj_variable) / sizeof(DS_OBJECT_CONTROL_EX);

#include <tchar.h>

//-----------------------------------------------------------------------------
static void ds_fill_object_controls(HWND hWndDlg, int objID, DS_GEO_OBJECT *gobj, char *buffer)
//-----------------------------------------------------------------------------
{
	if (gobj->name[0])
	{
		SetDlgItemText(hWndDlg, DS_BASE_NAME + objID, gobj->name);
	}
	else if (gobj->filename)
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

	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_ON    + objID, BM_SETCHECK, (gobj->tAttr.onFlag ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_E     + objID, BM_SETCHECK, (gobj->tAttr.state & DS_COLOR_STATE_EXPLICIT ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_COLOR_TRANS_O     + objID, BM_SETCHECK, (gobj->tAttr.state & DS_COLOR_STATE_OVERRIDE ? BST_CHECKED : BST_UNCHECKED), 0);
	sprintf(buffer, "%.2f", gobj->tAttr.alpha);  SetDlgItemText(hWndDlg, DS_BASE_COLOR_TRANS_ALPHA + objID, buffer);

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

	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_FACE   + objID, BM_SETCHECK, (gobj->lFlags.face   ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_EDGE   + objID, BM_SETCHECK, (gobj->lFlags.edge   ? BST_CHECKED : BST_UNCHECKED), 0);
	SendDlgItemMessage(hWndDlg, DS_BASE_LABEL_VERTEX + objID, BM_SETCHECK, (gobj->lFlags.vertex ? BST_CHECKED : BST_UNCHECKED), 0);

	{
		char *geo_orientation[12]=
		{
			"IF - Icosahedron Face",
			"IE - Icosahedron Edge",
			"IV - Icosahedron Vertex",
			"OF - Octahedron Face",
			"OE - Octahedron Edge",
			"OV - Octahedron Vertex",
			"TF - Tetrahedron Face",
			"TE - Tetrahedron Edge",
			"TV - Tetrahedron Vertex",
			"CF - Cube Face",
			"CE - Cube Edge",
			"CV - Cube Vertex",
		};

		int i;
		for (i = 0; i < 12; ++i)
		{
			SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, (UINT)CB_ADDSTRING, (WPARAM)0, (LPARAM)geo_orientation[i]);
		}

		// Send the CB_SETCURSEL message to display an initial item 
		//  in the selection field  
		int		index;
		index = (gobj->geo_type - 1) * 3 + (2 - gobj->geo_orientation);
		SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, CB_SETCURSEL, (WPARAM)index, (LPARAM)0);
		SendDlgItemMessage(hWndDlg, DS_BASE_GEO_ORIENTATION + objID, CB_SETDROPPEDWIDTH, (WPARAM)132, (LPARAM)0);
	}
}

//-----------------------------------------------------------------------------
void static ds_draw_variable_controls(HWND hWndDlg, HFONT s_hFont, int yOffset, int *objID, int *bottom, int defaultObject )
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
		if (dsox->h < 40 && rect.bottom > *bottom)
			*bottom = rect.bottom;
		MapDialogRect(hWndDlg, &rect);

		if (!defaultObject || ( defaultObject && i != 1)) // don't create window for default visible checkbox
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
LRESULT CALLBACK ds_dlg_object_control(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam)
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

	switch (Msg) {
	case WM_INITDIALOG:
		logFont.lfHeight = -MulDiv(nFontSize, GetDeviceCaps(hdc, LOGPIXELSY), 72);
		logFont.lfWeight = FW_REGULAR;
		strcpy_s(logFont.lfFaceName, 32, fontName);
		s_hFont = CreateFontIndirect(&logFont);
		ReleaseDC(hWndDlg, hdc);

		int						i, nObj, bottom, maxBottom;
		DS_OBJECT_CONTROL		*dso;
		// do this for each geometry object  
		objID = 0;
		bottom = 0;

		// default object settings
		memcpy(&gobj_def, &ctx->defInputObj, sizeof(DS_GEO_INPUT_OBJECT));
		ds_draw_variable_controls(hWndDlg, s_hFont, yOffset, &objID, &bottom, 1);

		// loop thru real objects
		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			ds_draw_variable_controls( hWndDlg, s_hFont, yOffset, &objID, &bottom, 0);
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
/*
		objID = 0;

		// fill in the default
		ds_fill_object_controls(hWndDlg, objID, &gobj_def, buffer);

		LL_SetHead(ctx->gobjectq);
		while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{// send messages to all controls to update content
			ds_fill_object_controls( hWndDlg, ++objID,  gobj, buffer);
		}
*/
		EndPaint(hWndDlg, &ps);
		break;

	case WM_COMMAND:
//		switch (wParam)
//		{
//		case IDOK:
//			ctx->objControl = 0;
//			DestroyWindow(hWndDlg);
//
//		case IDCANCEL:
//			ctx->objControl = 0;
//			DestroyWindow(hWndDlg);
//		}

		control   = LOWORD(wParam); // the specific control that triggered the event
		category  = control / 100;	// control category
		category *= 100;
		objID     = control % 100;	// object ID 
		temp      = 0;

		if (control < 100 || control > 3100)
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
				case DS_BASE_EDGE_WIDTH:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.width,  1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_EDGE_HEIGHT:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.height, 1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_EDGE_OFFSET:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->eAttr.offset, 1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_VERTEX_SCALE:		ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->vAttr.scale,  1, 3, 0, 0.0, 1.0); break;
				case DS_BASE_COLOR_TRANS_ALPHA: ds_edit_text_update(pWnd, hWndDlg, control, buffer, (void *)&gobj->tAttr.alpha,  0, 2, 1, 0.0, 1.0); break;
				}
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

		case DS_BASE_COLOR_TRANS_ON: gobj->tAttr.onFlag = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_TRANS_E:  gobj->tAttr.state = DS_COLOR_STATE_EXPLICIT;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_TRANS_O:  gobj->tAttr.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_EDGE_USE_A: gobj->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_USE_O: gobj->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_COLOR_EDGE_SET:   clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.edge.color);   InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_COLOR_VERTEX_SET: clrUpdate = ds_general_color_dialog(hWndDlg, ctx, &gobj->cAttr.vertex.color); InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_EDGE_ROUND: gobj->eAttr.type = GEOMETRY_EDGE_ROUND;  InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_EDGE_BOX:   gobj->eAttr.type = GEOMETRY_EDGE_SQUARE; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_REPLICATE_FACE:	gobj->rAttr.oneFaceFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_Z:		gobj->rAttr.zRotationFlag = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_REPLICATE_X:		gobj->rAttr.xMirrorFlag   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_LABEL_FACE:		gobj->lFlags.face   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_LABEL_EDGE:		gobj->lFlags.edge   = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
		case DS_BASE_LABEL_VERTEX:		gobj->lFlags.vertex = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;

		case DS_BASE_GEO_ORIENTATION:	//gobj->lFlags.vertex = SendDlgItemMessage(hWndDlg, control, BM_GETCHECK, 0, 0) ? 1 : 0; InvalidateRect(pWnd, 0, 0); break;
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				int ItemIndex = SendDlgItemMessage(hWndDlg, control, (UINT)CB_GETCURSEL, (WPARAM)0, (LPARAM)0);
				// decode index in geometry and orientation
				gobj->geo_type = ItemIndex / 3 + 1;
				gobj->geo_orientation = 2 - ItemIndex % 3;
				InvalidateRect(pWnd, 0, 0);
			}
			break;
		}
		if (clrUpdate)
		{
			clrUpdate = 0;
			LPDRAWITEMSTRUCT lpdis = (DRAWITEMSTRUCT*)lParam;
			InvalidateRect((HWND)lParam, 0, 0);
		}
		break;

	case WM_DESTROY:
	case WM_CLOSE:
		if (ctx->objControl)
		{
			DestroyWindow(hWndDlg);
			ctx->objControl = 0;
		}
		break;

	case WM_DRAWITEM: // owner drawn 
		{
			LPDRAWITEMSTRUCT	lpdis = (DRAWITEMSTRUCT*)lParam;
			RECT				lpr;
			HBRUSH				hColorBrush;
			DS_COLOR			*clr = 0;

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
			case DS_BASE_COLOR_FACE_SET:   clr = &gobj->cAttr.face.color;		break;
			case DS_BASE_COLOR_EDGE_SET:   clr = &gobj->cAttr.edge.color;		break;
			case DS_BASE_COLOR_VERTEX_SET: clr = &gobj->cAttr.vertex.color;		break;
			}
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
		break;
	}

	return FALSE;
}
