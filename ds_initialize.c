/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions handles pre and post initialization of program 
	state before and after command line option processing.
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
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

//-----------------------------------------------------------------------------
void ds_pre_init(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	// initialize all variables that rely on some initial value other than zero
	// clear memory

#ifdef ERR_DEBUG
	ctx->errFile = fopen("C:/TEMP/ds_dbg.txt", "w");
#else
	ctx->errFile = 0;
#endif

	ctx->hDC  = 0;				/* d1.1.1.	Option: -in_mirror 
The -in_mirror option enables X axis geometry duplication through mirroring during input processing. Disabled by default.
evice context */
	ctx->hPalette = 0;		/* custom palette (if needed) */
	ctx->hInstance = 0;

	// current directory 
	ctx->curWorkingDir[0] = 0;
	GetCurrentDirectory(512, ctx->curWorkingDir);

	ctx->drawAdj.axiiFlag = 0;
	ctx->drawAdj.fogFlag = 1;
	ctx->drawAdj.projection = GEOMETRY_PROJECTION_PERPSECTIVE; // = GEOMETRY_PROJECTION_ORTHOGRAPHIC)
	ctx->drawAdj.stereoFlag = 0;
	ctx->drawAdj.stereoCrossEyeFlag = 1;
	ctx->drawAdj.eyeSeparation = 2.5;

	ctx->drawAdj.clipFlag = 0;
	ctx->drawAdj.clipZValue = 0;
	ctx->drawAdj.clipZIncrement = (float)0.01;

	ctx->drawAdj.hiResFlag = 0;
	ctx->drawAdj.loRes.edgeNSeg = 6;
	ctx->drawAdj.loRes.sphereFrequency = 3;
	ctx->drawAdj.hiRes.edgeNSeg = 12;
	ctx->drawAdj.hiRes.sphereFrequency = 8;
	ctx->drawAdj.quality = &ctx->drawAdj.loRes;

	ds_base_geometry_create(ctx);

	ctx->base_geometry.type = GEOMETRY_ICOSAHEDRON;
	ctx->base_geometry.oneFaceFlag = 0;
	ctx->base_geometry.mirrorFlag = 0;
	ctx->base_geometry.zRotFlag = 0;

	ctx->gobjectq  = LL_Create();
	ctx->inputObjq = LL_Create();
	{
		DS_GEO_INPUT_OBJECT	*gio; // this is the default object configuration
		ctx->curInputObj = gio = &ctx->defInputObj; // objDefault;

		// object defaults: these are inheritied for each new object
		gio->active					= 1;
		gio->filename				= 0;
		gio->drawWhat				= GEOMETRY_DRAW_FACES;	 // what part of the object to draw F 1 E 2 V 4

		gio->eAttr.type				= GEOMETRY_EDGE_ROUND; // GEOMETRY_EDGE_SQUARE;
		gio->eAttr.height			= 0.02;
		gio->eAttr.width			= 0.065;
		gio->eAttr.offset			= 1.00;
		gio->eAttr.maxLength		= 0;
		gio->eAttr.minLength		= 100000000;

		gio->vAttr.scale			= 0.07; // default: vertex scale
		
		// default: face, edge, and vertex color control
		gio->cAttr.face.state		= DS_COLOR_STATE_EXPLICIT;  //DEFAULT=0 EXPLICIT=0, AUTOMATIC=1, OVERRIDE=2
		gio->cAttr.face.color.r		= (float)0;
		gio->cAttr.face.color.g		= (float)0.8;
		gio->cAttr.face.color.b		= (float)0;
		gio->cAttr.edge.state		= DS_COLOR_STATE_AUTOMATIC;
		gio->cAttr.edge.color.r		= (float)0;
		gio->cAttr.edge.color.g		= (float)0;
		gio->cAttr.edge.color.b		= (float)0.8;
		gio->cAttr.vertex.state		= DS_COLOR_STATE_AUTOMATIC;
		gio->cAttr.vertex.color.r	= (float)(214 / 255.0);
		gio->cAttr.vertex.color.g	= (float)(205 / 255.0);
		gio->cAttr.vertex.color.b	= (float)( 41 / 255.0);

		// default: replication flags
		gio->rAttr.oneFaceFlag		= 1;
		gio->rAttr.zRotationFlag	= 0;
		gio->rAttr.xMirrorFlag		= 0;
	}

//	ctx->filename[0]		= 0;
//	ctx->nInputFiles		= 0;
//	ctx->curWorkingDir[0]	= 0;

	// default window size & position 
	ctx->window.start_x			= 0;
	ctx->window.start_y			= 0;
	ctx->window.width			= 800;
	ctx->window.height			= 800;
	// windows 
	ctx->window.toolsVisible	= 0;
	ctx->attrControl			= 0;
	ctx->objInfo				= 0;

	// default capture name (single mode)
	ctx->png.basename[0] = 0;
	ctx->png.singleFlag = 0;
	ctx->png.nFrames = 0;
	ctx->png.stateSaveFlag = 1;
	ctx->png.bwFlag = 0;

	ctx->geomAdj.polymode[0]	= GEOMETRY_POLYMODE_FILL;
	ctx->geomAdj.polymode[1]	= GEOMETRY_POLYMODE_FILL;

	ctx->geomAdj.orientation	= GEOMETRY_ORIENTATION_FACE;

	// edge attributes
	ctx->eAttr.type				= GEOMETRY_EDGE_ROUND; // GEOMETRY_EDGE_SQUARE;
	ctx->eAttr.height			= 0.02;
	ctx->eAttr.width			= 0.065;
	ctx->eAttr.offset			= 1.00;
	ctx->eAttr.maxLength		= 0;
	ctx->eAttr.minLength		= 100000000;

	// draw attributes
	ctx->geomAdj.drawWhat		= GEOMETRY_DRAW_TRIANGLES;

	// color control
	ctx->clrCtl.useLightingFlag				= 1;
	ctx->clrCtl.light.x						= 0.5;
	ctx->clrCtl.light.y						= 0.5;
	ctx->clrCtl.light.z						= 2.0;
	ctx->clrCtl.light.w						= 1.0;
	ctx->clrCtl.bkgClear.r					= 1.0;
	ctx->clrCtl.bkgClear.g					= 1.0;
	ctx->clrCtl.bkgClear.b					= 1.0;
	ctx->clrCtl.line.flag					= 0; // override flag
	ctx->clrCtl.line.override.r				= 0.0;
	ctx->clrCtl.line.override.g				= 0.0;
	ctx->clrCtl.line.override.b				= 1.0; // blue
	ctx->clrCtl.line.defaultColor.r			= 0;
	ctx->clrCtl.line.defaultColor.g			= 0;
	ctx->clrCtl.line.defaultColor.b			= 0;
	ctx->clrCtl.triangle.flag				= 0; // override flag
	ctx->clrCtl.triangle.override.r			= 0.0;
	ctx->clrCtl.triangle.override.g			= 1.0; // green
	ctx->clrCtl.triangle.override.b			= 0.0;
	ctx->clrCtl.triangle.defaultColor.r		= 1.0; // red
	ctx->clrCtl.triangle.defaultColor.g		= 0;
	ctx->clrCtl.triangle.defaultColor.b		= 0;
	ctx->clrCtl.autoColor					= 0; // don't use auto generated unique colors by default
	ctx->clrCtl.user_color_table[0]			= 0; // empty string 
	ctx->clrCtl.reverseColorFlag			= 0;

	// input transformation 
	ctx->inputTrans.guaFlag = 1; // default
	ctx->inputTrans.guaResultsFlag = 0; // default
	ctx->inputTrans.zAxis.x = ctx->inputTrans.zAxis.y = ctx->inputTrans.zAxis.z = ctx->inputTrans.zAxis.w = 0;
	ctx->inputTrans.yAxis.x = ctx->inputTrans.yAxis.y = ctx->inputTrans.yAxis.z = ctx->inputTrans.yAxis.w = 0;
	ctx->inputTrans.replicateFlag = 0;
	ctx->inputTrans.mirrorFlag = 0;
	ctx->inputTrans.centerAndScaleFlag = 0; // default
	mtx_set_unity(&ctx->inputTrans.matrix[0]);
	mtx_create_rotation_matrix(&ctx->inputTrans.matrix[1], MTX_ROTATE_Z_AXIS, DTR(120.0));
	// matrices will be initialized during post init 

	// initialize the matrix
	mtx_set_unity(&ctx->matrix);
	ctx->rot[0] = ctx->rot[0] = ctx->rot[0] = 0;
	ctx->trans[0] = ctx->trans[1] = ctx->trans[2] = 0;

	// initialize color table set
	ctx->cts.nTables = 0;
	ctx->cts.head = ctx->cts.tail = 0;
	ds_clr_default_color_tables(ctx); // builds 4 tables (8,16,32,64)

	// initialize the render vertex
	ctx->renderVertex.scale = 0.07;
	ctx->renderVertex.clr.r = (float)(214 / 255.0); ctx->renderVertex.clr.g = (float)(205 / 255.0); ctx->renderVertex.clr.b = (float)(41/255.0);
	ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjLoRes;

	// opengl 
	ctx->opengl.samplesPerPixel = 0; // this will let program determine highest possible ( 8 )

	// spin initial
	ctx->drawAdj.spin.dx = ctx->drawAdj.spin.dy = ctx->drawAdj.spin.dz = ctx->drawAdj.spin.timerMSec = 0;
}

//-----------------------------------------------------------------------------
void ds_pre_init2(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	// initialize all variables that rely on some initial value other than zero
	// clear memory

//	ctx->hDC = 0;				/* d1.1.1.	Option: -in_mirror
//	ctx->hPalette = 0;		/* custom palette (if needed) */
//	ctx->hInstance = 0;

// current directory 
	GetCurrentDirectory(512, ctx->curWorkingDir);

	ctx->drawAdj.axiiFlag = 0;
	ctx->drawAdj.fogFlag = 1;
	ctx->drawAdj.projection = GEOMETRY_PROJECTION_PERPSECTIVE; // = GEOMETRY_PROJECTION_ORTHOGRAPHIC)
	ctx->drawAdj.stereoFlag = 0;
	ctx->drawAdj.stereoCrossEyeFlag = 1;
	ctx->drawAdj.eyeSeparation = 2.5;

	ctx->drawAdj.clipFlag = 0;
	ctx->drawAdj.clipZValue = 0;
	ctx->drawAdj.clipZIncrement = (float)0.01;

	ctx->drawAdj.hiResFlag = 0;
	ctx->drawAdj.loRes.edgeNSeg = 6;
	ctx->drawAdj.loRes.sphereFrequency = 3;
	ctx->drawAdj.hiRes.edgeNSeg = 12;
	ctx->drawAdj.hiRes.sphereFrequency = 8;
	ctx->drawAdj.quality = &ctx->drawAdj.loRes;

//	base_geometry_new_create(ctx);

	ctx->base_geometry.type = GEOMETRY_ICOSAHEDRON;
	ctx->base_geometry.oneFaceFlag = 0;
	ctx->base_geometry.mirrorFlag = 0;
	ctx->base_geometry.zRotFlag = 0;

	ctx->eAttr.maxLength = 0.0;
	ctx->eAttr.minLength = 1000000000.0;

	//	ctx->inputObjq = LL_Create();
	// remove all the previous geometry
	{
		DS_GEO_OBJECT	*lgobj;
		ctx->eAttr.maxLength = 0.0;
		ctx->eAttr.minLength = 1000000000.0;

		while (lgobj = (DS_GEO_OBJECT*)LL_RemoveHead(ctx->gobjectq))
		{	// free up allocated memory
			lgobj->filename ? free(lgobj->filename) : 0;
			lgobj->v_out ? free(lgobj->v_out) : 0;
			lgobj->tri ? free(lgobj->tri) : 0;
			lgobj->edge ? free(lgobj->edge) : 0;
			lgobj->vIndex ? free(lgobj->vIndex) : 0;
			lgobj ? free(lgobj) : 0; // free self
		}
	}
	{
		DS_GEO_INPUT_OBJECT	*gio; // this is the default object configuration
		ctx->curInputObj = gio = &ctx->defInputObj; // objDefault;

		// object defaults: these are inheritied for each new object
		gio->active = 1;
		gio->filename = 0;
		gio->drawWhat = GEOMETRY_DRAW_FACES;	 // what part of the object to draw F 1 E 2 V 4

		gio->eAttr.type = GEOMETRY_EDGE_ROUND; // GEOMETRY_EDGE_SQUARE;
		gio->eAttr.height = 0.02;
		gio->eAttr.width = 0.065;
		gio->eAttr.offset = 1.00;
		gio->eAttr.maxLength = 0;
		gio->eAttr.minLength = 100000000;

		gio->vAttr.scale = 0.07; // default: vertex scale

								 // default: face, edge, and vertex color control
		gio->cAttr.face.state = DS_COLOR_STATE_EXPLICIT;  //DEFAULT=0 EXPLICIT=0, AUTOMATIC=1, OVERRIDE=2
		gio->cAttr.face.color.r = (float)0;
		gio->cAttr.face.color.g = (float)0.8;
		gio->cAttr.face.color.b = (float)0;
		gio->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;
		gio->cAttr.edge.color.r = (float)0;
		gio->cAttr.edge.color.g = (float)0;
		gio->cAttr.edge.color.b = (float)0.8;
		gio->cAttr.vertex.state = DS_COLOR_STATE_AUTOMATIC;
		gio->cAttr.vertex.color.r = (float)(214 / 255.0);
		gio->cAttr.vertex.color.g = (float)(205 / 255.0);
		gio->cAttr.vertex.color.b = (float)( 41 / 255.0);

		// default: replication flags
		gio->rAttr.oneFaceFlag   = 1;
		gio->rAttr.zRotationFlag = 0;
		gio->rAttr.xMirrorFlag   = 0;
	}

	//	ctx->filename[0]		= 0;
	//	ctx->nInputFiles		= 0;
	//ctx->curWorkingDir[0] = 0;

	// default window size & position 
	ctx->window.start_x = 0;
	ctx->window.start_y = 0;
	ctx->window.width   = 800;
	ctx->window.height  = 800;
	// windows 
	ctx->window.toolsVisible = 0;
	if ( ctx->attrControl )
		DestroyWindow(ctx->attrControl); //destroy existing
	if (ctx->objInfo)
		DestroyWindow(ctx->objInfo); //destroy existing
	if (ctx->objControl)
		DestroyWindow(ctx->objControl); //destroy existing
	ctx->attrControl = 0;
	ctx->objControl = 0;
	ctx->objInfo = 0;

	// default capture name (single mode)
	ctx->png.basename[0] = 0;
	ctx->png.singleFlag = 0;
	ctx->png.nFrames = 0;
	ctx->png.stateSaveFlag = 1;
	ctx->png.bwFlag = 0;

	ctx->geomAdj.polymode[0] = GEOMETRY_POLYMODE_FILL;
	ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_FILL;

	ctx->geomAdj.orientation = GEOMETRY_ORIENTATION_FACE;

	// edge attributes
	ctx->eAttr.type = GEOMETRY_EDGE_ROUND; // GEOMETRY_EDGE_SQUARE;
	ctx->eAttr.height = 0.02;
	ctx->eAttr.width = 0.065;
	ctx->eAttr.offset = 1.00;
	ctx->eAttr.maxLength = 0;
	ctx->eAttr.minLength = 100000000;

	// draw attributes
	ctx->geomAdj.drawWhat = GEOMETRY_DRAW_TRIANGLES;

	// color control
	ctx->clrCtl.useLightingFlag = 1;
	ctx->clrCtl.light.x = 0.5;
	ctx->clrCtl.light.y = 0.5;
	ctx->clrCtl.light.z = 2.0;
	ctx->clrCtl.light.w = 1.0;
	ctx->clrCtl.bkgClear.r = 1.0;
	ctx->clrCtl.bkgClear.g = 1.0;
	ctx->clrCtl.bkgClear.b = 1.0;
	ctx->clrCtl.line.flag = 0; // override flag
	ctx->clrCtl.line.override.r = 0.0;
	ctx->clrCtl.line.override.g = 0.0;
	ctx->clrCtl.line.override.b = 1.0; // blue
	ctx->clrCtl.line.defaultColor.r = 0;
	ctx->clrCtl.line.defaultColor.g = 0;
	ctx->clrCtl.line.defaultColor.b = 0;
	ctx->clrCtl.triangle.flag = 0; // override flag
	ctx->clrCtl.triangle.override.r = 0.0;
	ctx->clrCtl.triangle.override.g = 1.0; // green
	ctx->clrCtl.triangle.override.b = 0.0;
	ctx->clrCtl.triangle.defaultColor.r = 1.0; // red
	ctx->clrCtl.triangle.defaultColor.g = 0;
	ctx->clrCtl.triangle.defaultColor.b = 0;
	ctx->clrCtl.autoColor = 0; // don't use auto generated unique colors by default
	ctx->clrCtl.user_color_table[0] = 0; // empty string 
	ctx->clrCtl.reverseColorFlag = 0;

	// input transformation 
	ctx->inputTrans.guaFlag = 1; // default
	ctx->inputTrans.guaResultsFlag = 0; // default
	ctx->inputTrans.zAxis.x = ctx->inputTrans.zAxis.y = ctx->inputTrans.zAxis.z = ctx->inputTrans.zAxis.w = 0;
	ctx->inputTrans.yAxis.x = ctx->inputTrans.yAxis.y = ctx->inputTrans.yAxis.z = ctx->inputTrans.yAxis.w = 0;
	ctx->inputTrans.replicateFlag = 0;
	ctx->inputTrans.mirrorFlag = 0;
	ctx->inputTrans.centerAndScaleFlag = 0; // default
	mtx_set_unity(&ctx->inputTrans.matrix[0]);
	mtx_create_rotation_matrix(&ctx->inputTrans.matrix[1], MTX_ROTATE_Z_AXIS, DTR(120.0));
	// matrices will be initialized during post init 

	// initialize the matrix
	mtx_set_unity(&ctx->matrix);
	ctx->rot[0] = ctx->rot[0] = ctx->rot[0] = 0;
	ctx->trans[0] = ctx->trans[1] = ctx->trans[2] = 0;

	// initialize color table set
//	ctx->cts.nTables = 0;
//	ctx->cts.head = ctx->cts.tail = 0;
//	clr_default_color_tables(ctx); // builds 4 tables (8,16,32,64)

								   // initialize the render vertex
	ctx->renderVertex.scale = 0.07;
	ctx->renderVertex.clr.r = (float)(214 / 255.0); ctx->renderVertex.clr.g = (float)(205 / 255.0); ctx->renderVertex.clr.b = (float)(41 / 255.0);
	ctx->renderVertex.vtxObj = &ctx->renderVertex.vtxObjLoRes;

	// opengl 
	ctx->opengl.samplesPerPixel = 0; // this will let program determine highest possible ( 8 )

	// spin initial
	ctx->drawAdj.spinFlag = 0;
	ctx->drawAdj.spin.dx = ctx->drawAdj.spin.dy = ctx->drawAdj.spin.dz = ctx->drawAdj.spin.timerMSec = 0;
}

//-----------------------------------------------------------------------------
void ds_post_init(DS_CTX *ctx) //, POLYHEDRON **poly)
//-----------------------------------------------------------------------------
{
	// Pre-Initialize all neccessary variables AFTER command line is processed
	FILE	*fp;
	GLfloat	fcolor[4] = { .5, .5, .5, 1.0 }; // gray
	int		captureMode = 0; // 1 = single, 2 = movie
	enum {
		CAPTURE_NONE,
		CAPTURE_IMAGE,
		CAPTURE_FILM,
	};

	if (!ctx->base_geometry.type)
		ctx->base_geometry.type = GEOMETRY_ICOSAHEDRON;


	if (!ctx->geomAdj.drawWhat)
		ctx->geomAdj.drawWhat = GEOMETRY_DRAW_TRIANGLES;

	if (!ctx->drawAdj.clipFlag) // not set by command line
	{
		ctx->drawAdj.clipVisibleFlag = 0; // clip.visible = 0;
		ctx->drawAdj.clipZValue = 0;
	}
//	glClearColor(0, 0, 0, 0);

	//	ctx->display_text = 0;
//	ctx->gobjectq = LL_Create();

	// enable depth testing with z
//	glEnable(GL_DEPTH_TEST);

	//	if (!ctx->matrixFlag) //
	//		mtx_set_unity(&ctx->matrix);

	if (strlen(ctx->png.basename)) // -capture option specified
	{
		if (!ctx->png.nFrames)
		{
			ctx->png.singleFlag = 1; // single capture
			ctx->png.nFrames = 1;
			captureMode = CAPTURE_IMAGE;
		}
		else
		{
			// movie
			ctx->png.nFrames = ctx->png.nFrames < 0 ? ctx->png.nFrames * -1 : ctx->png.nFrames;
			captureMode = CAPTURE_FILM;
		}
	}
	else
		strcpy(ctx->png.basename, "image"); // default capture filename

	switch (captureMode) {
	case CAPTURE_NONE: // no capture dependency - explicitly set bby user
		if (ctx->drawAdj.spin.dx != 0 || ctx->drawAdj.spin.dy != 0 || ctx->drawAdj.spin.dz != 0 || ctx->drawAdj.spin.timerMSec > 0) //|| ctx->drawAdj.spin.timerMSec != 0)
		{
			ctx->drawAdj.spin.spinState = ROTATE;
			SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
		}
		else
		{
			// default settings but spin is not enabled
			ctx->drawAdj.spin.dx = 0.5;
			ctx->drawAdj.spin.dy = 0.5;
			ctx->drawAdj.spin.dz = 0.0;
			ctx->drawAdj.spin.timerMSec = 100;
			ctx->drawAdj.spin.spinState = 0;
		}
		break;
	case CAPTURE_IMAGE: // single image capture - force spin and timer
		// when timer expires one image will be captured and the program will exit
		ctx->drawAdj.spin.dx = 0.5;
		ctx->drawAdj.spin.dy = 0.5;
		ctx->drawAdj.spin.dz = 0.0;
		ctx->drawAdj.spin.timerMSec = 500;
		ctx->drawAdj.spin.spinState = ROTATE;
		SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
		break;
	case CAPTURE_FILM: // movie - multiple frames
		ctx->drawAdj.spin.spinState = ROTATE; // enable spin regardless - even if user has not specified parameters
		ctx->drawAdj.spin.timerMSec = 500; // override
		SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
	}

	// input transformation 
	// shift data due to addressing (W is not used) 
	ctx->inputTrans.yAxis.z = ctx->inputTrans.yAxis.y;
	ctx->inputTrans.yAxis.y = ctx->inputTrans.yAxis.x;
	ctx->inputTrans.yAxis.x = ctx->inputTrans.zAxis.w;
	ctx->inputTrans.zAxis.w = ctx->inputTrans.yAxis.w = 1.0;
	if (ctx->inputTrans.zAxis.x != 0 || ctx->inputTrans.zAxis.y != 0 || ctx->inputTrans.zAxis.z != 0 ||
		ctx->inputTrans.yAxis.x != 0 || ctx->inputTrans.yAxis.y != 0 || ctx->inputTrans.yAxis.z != 0) // data has been provided
	{ // build transformation matrix
		ctx->inputTrans.transformFlag = 1;
		ds_geo_build_transform_matrix(ctx);
	}

	ds_set_render_quality(ctx);

	// process color table if defined
	ds_ctbl_process_color_table_file(&ctx->cts, ctx->clrCtl.user_color_table);

	DS_GEO_INPUT_OBJECT		*gio;
	DS_GEO_OBJECT			*gobj;

	ctx->gobjAddFlag = LL_GetLength(ctx->inputObjq)  ? 1 : 0;
	while (gio = (DS_GEO_INPUT_OBJECT*)LL_RemoveHead(ctx->inputObjq))
	{
		if (gio->filename && strlen(gio->filename))
		{
			fopen_s(&fp, gio->filename, "r");
			if (!fp)
				continue; // can't open file so skip
			gobj = ds_parse_file(ctx, fp, gio->filename);
			// transfer the settings
			fclose(fp);
			if ( gobj) 
			{
				char	*p;

				p = gio->filename + (strlen(gio->filename) - 1);

				// need to make sure you handle filenames that don't have a path
				while (p > gio->filename)
				{
					if (*p == '\\')
					{
						p = p + 1;
						break;
					}

					--p;
				}

				ds_file_set_window_text(ctx->mainWindow, p);

//				copy settings
				gobj->drawWhat	= gio->drawWhat;
				gobj->eAttr		= gio->eAttr;
				gobj->cAttr		= gio->cAttr;
				gobj->vAttr		= gio->vAttr;
				gobj->rAttr		= gio->rAttr;
			}
		}
		free(gio->filename);
		free(gio);
	}
	ctx->gobjAddFlag = 0; // add
}

//-----------------------------------------------------------------------------
void ds_post_init2(DS_CTX *ctx) //, POLYHEDRON **poly)
//-----------------------------------------------------------------------------
{
	// Pre-Initialize all neccessary variables AFTER command line is processed
	FILE	*fp;
	GLfloat	fcolor[4] = { .5, .5, .5, 1.0 }; // gray
	int		captureMode = 0; // 1 = single, 2 = movie
	enum {
		CAPTURE_NONE,
		CAPTURE_IMAGE,
		CAPTURE_FILM,
	};

	if (!ctx->base_geometry.type)
		ctx->base_geometry.type = GEOMETRY_ICOSAHEDRON;


	if (!ctx->geomAdj.drawWhat)
		ctx->geomAdj.drawWhat = GEOMETRY_DRAW_TRIANGLES;

	if (!ctx->drawAdj.clipFlag) // not set by command line
	{
		ctx->drawAdj.clipVisibleFlag = 0; // clip.visible = 0;
		ctx->drawAdj.clipZValue = 0;
	}

	if (strlen(ctx->png.basename)) // -capture option specified
	{
		if (!ctx->png.nFrames)
		{
			ctx->png.singleFlag = 1; // single capture
			ctx->png.nFrames = 1;
			captureMode = CAPTURE_IMAGE;
		}
		else
		{
			// movie
			ctx->png.nFrames = ctx->png.nFrames < 0 ? ctx->png.nFrames * -1 : ctx->png.nFrames;
			captureMode = CAPTURE_FILM;
		}
	}
	else
		strcpy(ctx->png.basename, "image"); // default capture filename

	switch (captureMode) {
	case CAPTURE_NONE: // no capture dependency - explicitly set bby user
		if (ctx->drawAdj.spin.dx != 0 || ctx->drawAdj.spin.dy != 0 || ctx->drawAdj.spin.dz != 0 || ctx->drawAdj.spin.timerMSec > 0) //|| ctx->drawAdj.spin.timerMSec != 0)
		{
			ctx->drawAdj.spin.spinState = ROTATE;
			SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
		}
		else
		{
			// default settings but spin is not enabled
			ctx->drawAdj.spin.dx = 0.5;
			ctx->drawAdj.spin.dy = 0.5;
			ctx->drawAdj.spin.dz = 0.0;
			ctx->drawAdj.spin.timerMSec = 100;
			ctx->drawAdj.spin.spinState = 0;
		}
		break;
	case CAPTURE_IMAGE: // single image capture - force spin and timer
						// when timer expires one image will be captured and the program will exit
		ctx->drawAdj.spin.dx = 0.5;
		ctx->drawAdj.spin.dy = 0.5;
		ctx->drawAdj.spin.dz = 0.0;
		ctx->drawAdj.spin.timerMSec = 500;
		ctx->drawAdj.spin.spinState = ROTATE;
		SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
		break;
	case CAPTURE_FILM: // movie - multiple frames
		ctx->drawAdj.spin.spinState = ROTATE; // enable spin regardless - even if user has not specified parameters
		ctx->drawAdj.spin.timerMSec = 500; // override
		SetTimer(ctx->mainWindow, 0, (int)ctx->drawAdj.spin.timerMSec, 0);
	}

	// input transformation 
	// shift data due to addressing (W is not used) 
	ctx->inputTrans.yAxis.z = ctx->inputTrans.yAxis.y;
	ctx->inputTrans.yAxis.y = ctx->inputTrans.yAxis.x;
	ctx->inputTrans.yAxis.x = ctx->inputTrans.zAxis.w;
	ctx->inputTrans.zAxis.w = ctx->inputTrans.yAxis.w = 1.0;
	if (ctx->inputTrans.zAxis.x != 0 || ctx->inputTrans.zAxis.y != 0 || ctx->inputTrans.zAxis.z != 0 ||
		ctx->inputTrans.yAxis.x != 0 || ctx->inputTrans.yAxis.y != 0 || ctx->inputTrans.yAxis.z != 0) // data has been provided
	{ // build transformation matrix
		ctx->inputTrans.transformFlag = 1;
		ds_geo_build_transform_matrix(ctx);
	}

	ds_set_render_quality(ctx);

	// process color table if defined
//	ctbl_process_color_table_file(&ctx->cts, ctx->clrCtl.user_color_table);

	DS_GEO_INPUT_OBJECT	*gio;
	DS_GEO_OBJECT			*gobj;
	ctx->gobjAddFlag = LL_GetLength(ctx->inputObjq) ? 1 : 0;
	while (gio = (DS_GEO_INPUT_OBJECT*)LL_RemoveHead(ctx->inputObjq))
	{
		if (gio->filename && strlen(gio->filename))
		{
			fopen_s(&fp, gio->filename, "r");
			if (!fp)
				continue; // can't open file so skip
			gobj = ds_parse_file(ctx, fp, gio->filename);
			// transfer the settings
			fclose(fp);
			if (gobj)
			{
				char	*p;

				p = gio->filename + (strlen(gio->filename) - 1);

				// need to make sure you handle filenames that don't have a path
				while (p > gio->filename)
				{
					if (*p == '\\')
					{
						p = p + 1;
						break;
					}

					--p;
				}

				ds_file_set_window_text(ctx->mainWindow, p);

				//	copy settings
				gobj->active = gio->active;
				gobj->drawWhat = gio->drawWhat;
				gobj->eAttr = gio->eAttr;
				gobj->cAttr = gio->cAttr;
				gobj->vAttr = gio->vAttr;
				gobj->rAttr = gio->rAttr;
			}
		}
		free(gio->filename);
		free(gio);

		ctx->gobjAddFlag = 1;
	}
	ctx->gobjAddFlag = 0; // add
}
