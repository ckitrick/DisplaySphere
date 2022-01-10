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
#include <time.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_cmd_line.h"

static char *font_name(void *id);

//-----------------------------------------------------------------------------
int ds_state_read(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	// This is the core routine to read state files all at once or in mulitple 
	// parts. Keeps track of where read is located if broken into sequences
	//
	// stateRead and stateWrite structures need to be be initialized at startup
	FILE					*fp;
	char					buffer[1024];
	char					*av[64];
	int						ac;
	int						consumedCount;
	char					*error;

	// check if read and write files match and both open 
	if (ctx->stateWrite.mode == DS_STATE_OPEN)
	{
		if (!strcmp(ctx->stateWrite.filename, filename))
		{
			// will effectively end any appends to the write file
			ctx->stateWrite.mode = DS_STATE_NULL;
//			if (ctx->stateWrite.dsf.fp)
//			{
//				fclose(ctx->stateWrite.dsf.fp);
//				ctx->stateWrite.dsf.fp = 0;
//			}
		}
		if (strcmp(ctx->stateRead.filename, filename))
		{
			// the previous open state file is different
			// will effectively end prior read file
			ctx->stateRead.mode = DS_STATE_NULL; // will cause a reset
//			if (ctx->stateRead.dsf.fp)
//			{
//				fclose(ctx->stateRead.dsf.fp);
//				ctx->stateRead.dsf.fp = 0;
//			}
		}
	}

	// SAVE CURRENT GLOBAL STATE THAT WILL BE CHECKED AFTER READ
	// saveGlobalState()
	// saveObjectState()

	// determine how to proceed if this is the first time or subsequent read
	if (ctx->stateRead.mode == DS_STATE_OPEN)
	{
		// file has already been opened before
		if (!(ctx->stateRead.fp = fopen(filename, "r")))
			return 1; // failure

		// move the file read position to last place used
		fseek(ctx->stateRead.fp, ctx->stateRead.offset, SEEK_SET);
		// global state and objects not modified
	}
	else //if (!ctx->stateRead.dsf)
	{
		// open file and move to correct position to continue reading
		if (!(ctx->stateRead.fp = fopen(filename, "r")))
			return 1; // failure

		ctx->stateRead.mode = DS_STATE_OPEN;

		strcpy(ctx->stateRead.filename, filename); // save for later

		// if recording then cause reset to happen on replay
		if (ctx->stateWrite.mode == DS_STATE_OPEN)
			ctx->stateWrite.resetFlag = 1; // used next time write enabled

		// NEED to reset all global state to default
		// NEED to destroy all current objects
	}

	// loop thru state file either until end or when marker is found
	while (fgets(buffer, 1024, fp))
	{
		// check for sequence indicator 
		if (!strcmp(av[0], "-state_marker"))
		{
			// record next position to start reading from
			ctx->stateRead.offset = ftell(ctx->stateRead.fp);
			// close file but don't destroy DS_FILE
			fclose(ctx->stateRead.fp);
			ctx->stateRead.fp = 0;
			goto MARKER;
		}
		else if (!strcmp(av[0], "-command_line"))
		{
			// nestd option not allowed so skip
			continue;
		}
		else
		{
			ac = 1;
			if( _ds_cmd_line_process(ctx->cmdLineCtx, ac, av, &consumedCount, &error))
			{
				return 1;
			}
		}
	}

	// END OF FILE condiition
	ctx->stateRead.mode = DS_STATE_NULL;
	fclose(ctx->stateRead.fp);
	ctx->stateRead.fp = 0;

MARKER:
	// close file and reset file pointer
	fclose(ctx->stateRead.fp);
	ctx->stateRead.fp = 0;
	// update any neccessary state
	// CHECK SAVED STATE TO UPDATE WINDOWS, ETC.
	// add new objects
	// update any existing objects
	// check fil/capture state

	return 0;
}

//-----------------------------------------------------------------------------
int ds_state_write(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	// This is the core routine to write state files all at once or in mulitple 
	// parts. Multiple writes are appended to the original filename.
	//
	// stateRead and stateWrite structures need to be be initialized at startup
	FILE	*fp;

	// if also in the middle of reading state and the read and write files are the same
	// then stop the state read
	if (ctx->stateRead.mode == DS_STATE_OPEN && !strcmp(filename, ctx->stateRead.filename))
	{
		ctx->stateRead.mode = DS_STATE_NULL;
		// NEED to update dialog buttons if window open
	}

	if (ctx->stateWrite.mode == DS_STATE_NULL)
	{
		// this is the first time state write is called
		strcpy(ctx->stateWrite.filename, filename); // copy filename
		if (ctx->stateWrite.filename)
		{
			int		i = strlen(ctx->stateWrite.filename);
			if (i > 4)
			{
				if (strcmp(&ctx->stateWrite.filename[i - 4], ".dss"))
					strcpy(&ctx->stateWrite.filename[i - 4], ".dss");
			}
		}
		ctx->stateWrite.fp = fopen(ctx->stateWrite.filename, "w");
		if (!ctx->stateWrite.fp)
		{
			char buffer[128];
			sprintf(buffer, "File <%s> failed to open.", ctx->stateWrite.filename);
			MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
			return 1;
		}

		// initialize 
		ds_state_save_init(ctx);
		
		// dump global state with header
		ds_state_save_header(ctx);
		// dump all object state
		ds_state_save_global(ctx);
		ctx->stateWrite.mode = DS_STATE_OPEN; // update mode
		// track how many objects have state written
		ctx->stateWrite.objectCount = LL_GetLength(ctx->gobjectq);
	}
	else
	{
		// set file position to append since this is not the first write
		ctx->stateWrite.fp = fopen(ctx->stateWrite.filename, "a"); // append
		if (!ctx->stateWrite.fp)
			return 1; // failure
		// check reset (caused by playback scenario)
		if (ctx->stateWrite.resetFlag)
		{
			// output '-reset' option
			fprintf(ctx->stateWrite.fp, "-reset\n");
			ctx->stateWrite.resetFlag = 0; // turn flag off
		}
		//
		// dump global state
		// dump existing object state (user -object_reference #)
		// dump new object state (use -object filename)
		// update current object count
		ctx->stateWrite.objectCount = LL_GetLength(ctx->gobjectq);
	}

	// close state file
	fclose(ctx->stateWrite.fp);
	ctx->stateWrite.fp = 0;

	return 0;
}

//-----------------------------------------------------------------------------
int ds_state_save_object(DS_CTX *ctx, DS_FILE *base, DS_GEO_OBJECT *gobj, int objectID, int referenceFlag)
//-----------------------------------------------------------------------------
{
	char		*p;
	FILE		*fp = ctx->stateWrite.fp;

	if (gobj->filename)
	{
		char	relativeFilename[1024];
		ds_cd_relative_filename(ctx, base, gobj->dsf, relativeFilename);

		fprintf(fp, "# Object settings for <%s>\n", gobj->dsf->nameOnly);
//		if (referenceFlag)
//			fprintf(fp, "-object_reference %d\n", objectID); 
//		else
			fprintf(fp, "-object \"%s\"\n", relativeFilename);
	}
	else
	{
		fprintf(fp, "# Default object settings\n");
	}
	if (gobj->filename)
	{
		fprintf(fp, "-visible %s\n", gobj->active ? "enable" : "disable");
	}
	else
		fprintf(fp, "-visible enable\n");

	// Geometry
	switch (gobj->geo_type) {
	case GEOMETRY_CUBEHEDRON:	p = "cube"; break;
	case GEOMETRY_ICOSAHEDRON:	p = "icosahedron"; break;
	case GEOMETRY_OCTAHEDRON:	p = "octahedron"; break;
	case GEOMETRY_TETRAHEDRON:	p = "tetrahedron"; break;
	case GEOMETRY_DODECAHEDRON:	p = "dodecahedron"; break;
	default:					p = "icosahedron";
	}
	fprintf(fp, "-geometry %s\n", p);

	//- orientation
	switch (gobj->geo_orientation) {
	case GEOMETRY_ORIENTATION_EDGE:		p = "edge"; break;
	case GEOMETRY_ORIENTATION_FACE:		p = "face"; break;
	case GEOMETRY_ORIENTATION_VERTEX:	p = "vertex"; break;
	default:							p = "face";
	}
	fprintf(fp, "-orientation %s\n", p);
	fprintf(fp, "-replicate_one_face %s\n", gobj->rAttr.oneFaceFlag ? "enable" : "disable");
	fprintf(fp, "-replicate_z_rotate %s\n", gobj->rAttr.zRotationFlag ? "enable" : "disable");
	fprintf(fp, "-replicate_x_mirror %s\n", gobj->rAttr.xMirrorFlag ? "enable" : "disable");

	// Faces
	fprintf(fp, "-face_draw %s\n", gobj->fAttr.draw ? "enable" : "disable");

	switch (gobj->cAttr.face.state) {
	case DS_COLOR_STATE_AUTOMATIC: fprintf(fp, "-face_color_use automatic\n"); break;
	case DS_COLOR_STATE_EXPLICIT:  fprintf(fp, "-face_color_use explicit\n"); break;
	case DS_COLOR_STATE_OVERRIDE:  fprintf(fp, "-face_color_use override\n"); break;
	}
	fprintf(fp, "-face_override_color %f %f %f %f\n", gobj->cAttr.face.color.r, gobj->cAttr.face.color.g, gobj->cAttr.face.color.b, gobj->tAttr.alpha);

	// extrusion
	fprintf(fp, "-face_extrude %s\n", gobj->fAttr.extrusion.enable ? "enable" : "disable");
	fprintf(fp, "-face_extrude_2sides %s\n", gobj->fAttr.extrusion.bothSides ? "enable" : "disable");
	fprintf(fp, "-face_extrude_direction %s\n", gobj->fAttr.extrusion.direction ? "radial" : "normal");
	fprintf(fp, "-face_extrude_height %f\n", gobj->fAttr.extrusion.factor);
	fprintf(fp, "-face_extrude_hole_only %s\n", gobj->fAttr.extrusion.holeOnly ? "enable" : "disable");
	// hole
	fprintf(fp, "-face_hole %s\n", gobj->fAttr.hole.enable ? "enable" : "disable");
	fprintf(fp, "-face_hole_type ", gobj->fAttr.hole.style ? "polygonal" : "round");
	switch (gobj->fAttr.hole.style) {
	case FACE_HOLE_STYLE_ROUND:	fprintf(fp, "round\n"); break;
	case FACE_HOLE_STYLE_POLYGONAL:	fprintf(fp, "polygonal\n"); break;
	case FACE_HOLE_STYLE_ROUND_WITH_IN_DIMPLE:	fprintf(fp, "round_in_dimple\n"); break;
	case FACE_HOLE_STYLE_ROUND_WITH_OUT_DIMPLE:	fprintf(fp, "round_out_dimple\n"); break;
	}
	fprintf(fp, "-face_hole_radius %.2f\n", gobj->fAttr.hole.radius);
	fprintf(fp, "-face_hole_dimple_height %.2f\n", gobj->fAttr.hole.shallowness);
	// scale
	fprintf(fp, "-face_scale %s\n", gobj->fAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-face_scale_value %f\n", gobj->fAttr.scale.factor);
	// offset
	fprintf(fp, "-face_offset %s\n", gobj->fAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-face_offset_value %f\n", gobj->fAttr.offset.factor);
	// transparency
	fprintf(fp, "-face_transparency %s\n", gobj->tAttr.onFlag ? "enable" : "disable");
	fprintf(fp, "-face_transparency_override %s\n", gobj->tAttr.state == DS_COLOR_STATE_OVERRIDE ? "enable" : "disable");
	fprintf(fp, "-face_transparency_alpha %f\n", gobj->tAttr.alpha);

	// great circles 
	fprintf(fp, "-face_great_circles %s\n", gobj->fAttr.orthodrome.enable ? "enable" : "disable");

	// face great circles & spherical sections type
	switch (gobj->fAttr.orthodrome.style) {
	case ORTHODROME_STYLE_RIM:					p = "rim"; break;
	case ORTHODROME_STYLE_DISC_TWO_SIDED:		p = "disc_2_sided"; break;
	case ORTHODROME_STYLE_DISC_ONE_SIDED:		p = "disc_1_sided"; break;
	case ORTHODROME_STYLE_SPHERICAL_SECTION:	p = "spherical_sections"; break;
	default:									p = "rim";
	}
	fprintf(fp, "-face_great_circle_type %s\n", p);

	// face great circles & spherical sections rim dash enable
	fprintf(fp, "-face_great_circle_dashed %s\n", gobj->fAttr.orthodrome.dashEnable ? "enable" : "disable");

	// face great circles & spherical sections rim depth 1
	fprintf(fp, "-face_great_circle_rim_depth1 %.2f\n", gobj->fAttr.orthodrome.depth1); // NEEED NEW  clamp values

																						// face great circles & spherical sections rim depth 2
	fprintf(fp, "-face_great_circle_rim_depth2 %.2f\n", gobj->fAttr.orthodrome.depth2);

	// face great circles & spherical sections rim  height
	fprintf(fp, "-face_great_circle_rim_height %.2f\n", gobj->fAttr.orthodrome.height);

	// face great circles & spherical sections seperate cut color enable
	fprintf(fp, "-face_great_circle_cut_color %s\n", gobj->fAttr.orthodrome.cutColorEnable ? "enable" : "disable");

	// face great circles & spherical sections seperate cut color 
	fprintf(fp, "-face_great_circle_cut_color_value %f %f %f\n", gobj->fAttr.orthodrome.cutColor.r, ctx->curInputObj.fAttr.orthodrome.cutColor.g, ctx->curInputObj.fAttr.orthodrome.cutColor.b);

	// label
	fprintf(fp, "-face_label %s\n", gobj->fAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-face_label_color %f %f %f\n", gobj->fAttr.label.color.r, gobj->fAttr.label.color.g, gobj->fAttr.label.color.b);
	fprintf(fp, "-face_label_font %s\n", font_name(gobj->fAttr.label.font));

	// Edges
	fprintf(fp, "-edge_draw %s\n", gobj->eAttr.draw ? "enable" : "disable");
	switch (gobj->cAttr.edge.state) {
	case DS_COLOR_STATE_AUTOMATIC: p = "automatic"; break;
	case DS_COLOR_STATE_EXPLICIT:  p = "explicit"; break;
	case DS_COLOR_STATE_OVERRIDE:  p = "override"; break;
	}
	fprintf(fp, "-edge_color_use %s\n", p);
	fprintf(fp, "-edge_override_color %f %f %f\n", gobj->cAttr.edge.color.r, gobj->cAttr.edge.color.g, gobj->cAttr.edge.color.b);

	// scale
	fprintf(fp, "-edge_scale %s\n", gobj->eAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-edge_scale_value %f\n", gobj->eAttr.scale.factor);

	// offset
	fprintf(fp, "-edge_offset %s\n", gobj->eAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-edge_offset_value %f\n", gobj->eAttr.offset.factor);

	// type
	fprintf(fp, "-edge_type %s\n", gobj->eAttr.type == GEOMETRY_EDGE_ROUND ? "cylindrical" : "square");
	fprintf(fp, "-edge_arc %d\n", gobj->eAttr.arcEnable);
	//	fprintf(fp, "-edge_scale %s\n"gobj->eAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-edge_width_height %f %f\n", gobj->eAttr.width, gobj->eAttr.height);

	// label
	fprintf(fp, "-edge_label %s\n", gobj->eAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-edge_label_color %f %f %f\n", gobj->eAttr.label.color.r, gobj->eAttr.label.color.g, gobj->eAttr.label.color.b);
	fprintf(fp, "-edge_label_font %s\n", font_name(gobj->eAttr.label.font));

	// Vertex
	fprintf(fp, "-vertex_draw %s\n", gobj->vAttr.draw ? "enable" : "disable");
	fprintf(fp, "-vertex_color %f %f %f\n", gobj->cAttr.vertex.color.r, gobj->cAttr.vertex.color.g, gobj->cAttr.vertex.color.b);

	// offset
	fprintf(fp, "-vertex_offset %s\n", gobj->vAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-vertex_offset_value %f\n", gobj->vAttr.offset.factor);
	fprintf(fp, "-vertex_scale %f\n", gobj->vAttr.scale);//.width, gobj->eAttr.height, gobj->eAttr.offset);
	
	// label
	fprintf(fp, "-vertex_label %s\n", gobj->vAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-vertex_label_color %f %f %f\n", gobj->vAttr.label.color.r, gobj->vAttr.label.color.g, gobj->vAttr.label.color.b);
	fprintf(fp, "-vertex_label_font %s\n", font_name(gobj->vAttr.label.font));

	return 0;
}

//-----------------------------------------------------------------------------
static char *font_name(void *id)
//-----------------------------------------------------------------------------
{
	switch ((int)id) {
	case 0x0004: return "T10"; break;
	case 0x0005: return "T24"; break;
	case 0x0006: return "H10"; break;
	case 0x0007: return "H12"; break;
	case 0x0008: return "H18"; break;
	default: return "T24";
	}
}

//-----------------------------------------------------------------------------
int ds_state_save_init(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	// first time set up of file structures used for relative filename references 
	ds_build_dsf(&ctx->stateWrite.base, ctx->stateWrite.filename, 1); // just the path
	ds_build_dsf(&ctx->stateWrite.file, ctx->stateWrite.filename, 0); // full name
	ctx->relativeObjPathFlag = 1; // always force
	return 0;
}

//-----------------------------------------------------------------------------
int ds_state_save_header(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	FILE	*fp = ctx->stateWrite.fp;
	char	*p;
	int		clrTblFlag = 1;
	char	clrTblFilename[512];
	int		i;

	// check if color table has been changed
	if (ctx->clrTbl.fullName) 
	{
		DS_FILE		clrTblFile;
		char		filename[512];
		char		newFilename[512];

		// color table has been updated so dump current tables
		strcpy(filename, ctx->stateWrite.filename);
		if (i = strlen(filename))
		{
			if (i > 4)
			{
				if (strcmp(&filename[i - 4], ".dsc"))
					strcpy(&filename[i - 4], ".dsc");
			}
		}
		ds_build_dsf(&clrTblFile, filename, 0); // full name
		ds_cd_relative_filename(ctx, &ctx->stateWrite.base, &ctx->stateWrite.clrTblfilename, newFilename);
		ctx->stateWrite.clrTblFlag = ds_ctbl_output_color_table_file(&ctx->cts, newFilename);
	}

	// header for drag and drop functionality 
	fprintf(ctx->stateWrite.fp, "DS_STATE\n");
	fprintf(ctx->stateWrite.fp, "# Progam:  %s\n", ctx->version.text);
	fprintf(ctx->stateWrite.fp, "# Version: %d.%d\n", ctx->version.major, ctx->version.minor);
	fprintf(ctx->stateWrite.fp, "# Current State: ");

	// Get current date/time, format is YYYY-MM-DD.HH:mm:ss	
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	fprintf(ctx->stateWrite.fp, "%s\n", buf);
}

//-----------------------------------------------------------------------------
int ds_state_save_global(DS_CTX *ctx) //, int clrTblFlag, char *clrTblFilename)
//-----------------------------------------------------------------------------
{	
	// write out all global state
	//
	FILE		*fp = ctx->stateWrite.fp;
	char		*p;
	int			clrTblFlag;
	char		*clrTblFilename;

	if (!fp)
		return 1;

	fprintf(fp, "# Global settings\n");

	{
		char	relativePath[1024];
		//	-cd

		//	-capd
		ds_cd_relative_filename(ctx, &ctx->stateWrite.base, &ctx->capDir, relativePath); // capDir is already a path only
		fprintf(fp, "-capture_directory \"%s\"\n", relativePath);
	}

	//- axis
	fprintf(fp, "-axis %s\n", ctx->drawAdj.axiiFlag ? "enable" : "disable");
	fprintf(fp, "-axis_label %s\n", ctx->drawAdj.axiiLabelFlag ? "enable" : "disable");

	//- circle
	fprintf(fp, "-circle %s\n", ctx->drawAdj.circleFlag ? "enable" : "disable");

	//- clip
	fprintf(fp, "-clip %s\n", ctx->drawAdj.clipFlag ? "enable" : "disable");
	fprintf(fp, "-clip_z_increment %f %f\n", ctx->drawAdj.clipZValue, ctx->drawAdj.clipZIncrement);

	//- clr_backgound
	fprintf(fp, "-background_color  %5.3f %5.3f %5.3f\n", ctx->clrCtl.bkgClear.r, ctx->clrCtl.bkgClear.g, ctx->clrCtl.bkgClear.b);
	//- clr_table
	if (!ctx->stateWrite.clrTblFlag)
		fprintf(fp, "-color_table \"%s\"\n", ctx->stateWrite.clrTblfilename);

	//- gl_back
	switch (ctx->geomAdj.polymode[1]) {
	case GEOMETRY_POLYMODE_FILL:	p = "fill"; break;
	case GEOMETRY_POLYMODE_LINE:	p = "line"; break;
	case GEOMETRY_POLYMODE_POINT:	p = "point"; break;
	}
	fprintf(fp, "-gl_back %s\n", p);
	fprintf(fp, "-gl_back_cull %s\n", ctx->geomAdj.cull[1] ? "enable" : "disable");

	//- gl_front
	switch (ctx->geomAdj.polymode[0]) {
	case GEOMETRY_POLYMODE_FILL:	p = "fill"; break;
	case GEOMETRY_POLYMODE_LINE:	p = "line"; break;
	case GEOMETRY_POLYMODE_POINT:	p = "point"; break;
	}
	fprintf(fp, "-gl_front %s\n", p);
	fprintf(fp, "-gl_front_cull %s\n", ctx->geomAdj.cull[0] ? "enable" : "disable");

	//- hires
	fprintf(fp, "-high_resolution %s\n", ctx->drawAdj.hiResFlag ? "enable" : "disable");

	//- image_state_save
	fprintf(fp, "-image_state_save %s\n", ctx->png.stateSaveFlag ? "enable" : "disable");

	//- light
	//- light
	fprintf(fp, "-lighting %s\n", ctx->lighting.useLightingFlag ? "enable" : "disable");
	fprintf(fp, "-light_ambient %s\n", ctx->lighting.ambientEnabled ? "enable" : "disable");
	fprintf(fp, "-light_diffuse %s\n", ctx->lighting.diffuseEnabled ? "enable" : "disable");
	fprintf(fp, "-light_specular %s\n", ctx->lighting.specularEnabled ? "enable" : "disable");

	fprintf(fp, "-light_position %f %f %f\n", ctx->lighting.position.x, ctx->lighting.position.y, ctx->lighting.position.z);
	fprintf(fp, "-light_ambient_value %f\n", ctx->lighting.ambientPercent);
	fprintf(fp, "-light_diffuse_value %f\n", ctx->lighting.diffusePercent);
	fprintf(fp, "-light_specular_value %f\n", ctx->lighting.specularPercent);

	// material 
	fprintf(fp, "-light_material_specular %f\n", ctx->lighting.matSpecular);
	fprintf(fp, "-light_material_shininess %f\n", ctx->lighting.matShininess);


	//- no_fog
	fprintf(fp, "-fog %s\n", ctx->drawAdj.fogFlag ? "enable" : "disable");

	//- normalize
	fprintf(fp, "-normalize %s\n", ctx->drawAdj.normalizeFlag ? "enable" : "disable");

	//- orthographic
	fprintf(fp, "-projection %s\n", ctx->drawAdj.projection == GEOMETRY_PROJECTION_PERSPECTIVE ? "perspective" : "orthographic");

	//- spin
	fprintf(fp, "-spin %s\n", ctx->drawAdj.spin.spinState ? "enable" : "disable");
	if (ctx->drawAdj.spin.spinState)
		fprintf(fp, "-spin_values %f %f %f %d\n", ctx->drawAdj.spin.dx, ctx->drawAdj.spin.dy, ctx->drawAdj.spin.dz, (int)ctx->drawAdj.spin.timerMSec);

	//- spp
	fprintf(fp, "-samples_per_pixel %d\n", ctx->opengl.samplesPerPixel);

	//- stereo
	fprintf(fp, "-stereo %s\n", ctx->drawAdj.stereoFlag ? "enable" : "disable");
	fprintf(fp, "-stereo_cross_eye %s\n", ctx->drawAdj.stereoCrossEyeFlag ? "enable" : "disable");
	fprintf(fp, "-stereo_angle %f\n", ctx->drawAdj.eyeSeparation);

	// -toolvis
	// open tool windows
	if (ctx->attrControl && ctx->objDashboard)
		fprintf(fp, "-tool_windows_visible enable\n");

	{ // get current window information from OS
		RECT	rect;
		int		width, height;
		GetWindowRect(ctx->mainWindow, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		//- win_wh
		fprintf(fp, "-window_width_height %d %d\n", width - WINDOW_SIZE_OFFSET_WIDTH, height - WINDOW_SIZE_OFFSET_HEIGHT);
		//- win_xy
		fprintf(fp, "-window_left_top %d %d\n", rect.left, rect.top);
	}

	// capture
	// film
	// image
	fprintf(fp, "# Capture settings\n");
	fprintf(fp, "-image_basename %s\n", ctx->png.basename);

	fprintf(fp, "# Input settings\n");
	// input
	//-in_cs
	fprintf(fp, "-input_center_scale %s\n", ctx->inputTrans.centerAndScaleFlag ? "enable" : "disable");

	//- in_transform
	fprintf(fp, "-input_transform %s\n", ctx->inputTrans.transformFlag ? "enable" : "disable");
	fprintf(fp, "-input_transform_axii %f %f %f %f %f %f\n",
		ctx->inputTrans.zAxis.x, ctx->inputTrans.zAxis.y, ctx->inputTrans.zAxis.z, ctx->inputTrans.yAxis.x, ctx->inputTrans.yAxis.y, ctx->inputTrans.yAxis.z);

	//- in_x_mirror
	fprintf(fp, "-input_x_mirror %s\n", ctx->inputTrans.mirrorFlag ? "enable" : "disable");
	//- in_z_rotate
	fprintf(fp, "-input_z_rotate %s\n", ctx->inputTrans.replicateFlag ? "enable" : "disable");

	//- unique
	fprintf(fp, "-unique %s\n", ctx->inputTrans.guaFlag ? "enable" : "disable");

	//- rot_matrix
	double	*d = &ctx->matrix.data.array[0];
	fprintf(fp, "-rotation_matrix %f %f %f %f %f %f %f %f %f\n", d[0], d[1], d[2], d[4], d[5], d[6], d[8], d[9], d[10]);

	//- txyz
	fprintf(fp, "-translate_xyz %f %f %f\n", ctx->trans[0], ctx->trans[1], ctx->trans[2]);

	//- udump
	fprintf(fp, "-unique_dump %s\n", ctx->inputTrans.guaResultsFlag ? "enable" : "disable");

	fprintf(fp, "# Object values\n");
	fprintf(fp, "-face_default_color %5.3f %5.3f %5.3f %5.3f\n", ctx->clrCtl.face.defaultColor.r, ctx->clrCtl.face.defaultColor.g, ctx->clrCtl.face.defaultColor.b, ctx->clrCtl.face.defaultColor.a);

	{
		DS_GEO_OBJECT			*obj;
		int						i = 0;

		// output default object
//		ds_save_object_state(ctx, &ctx->stateWrite.base, (DS_GEO_OBJECT*)&ctx->defInputObj, 0, 0);
		ds_state_save_object(ctx, &ctx->stateWrite.base, (DS_GEO_OBJECT*)&ctx->defInputObj, 0, 0);

		// write out individual object's state
		LL_SetHead(ctx->gobjectq);
		while (obj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
			if (i < ctx->stateWrite.objectCount)
				ds_state_save_object(ctx, &ctx->stateWrite.base, obj, i, 1); // use -object_reference
			else
				ds_state_save_object(ctx, &ctx->stateWrite.base, obj, i, 0); // use -object
			++i;
		}
	}

	return 0;
}