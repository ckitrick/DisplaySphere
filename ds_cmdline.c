/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to provide specific argument handling 
	for DisplaySphere command line options. 
	Also handles save and retore functionality.
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
#include <string.h>
#include <math.h>
#include <time.h>
#include <commdlg.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"
#include "ds_cmdline_lib.h"

enum OPTION { // need to be in the same order as the options
	OP_ACTIVE,
	OP_AXIS,
	OP_AXIS_LABEL,
	OP_CAPTURE_DIRECTORY,
	OP_CD,
//	OP_CD_AFTER,
	OP_CIRCLE,
	OP_CLIP,
	OP_CLR_BACKGROUND,
	OP_CLR_E_SET,
	OP_CLR_E_USE,
	OP_CLR_F_DEFAULT,
	OP_CLR_F_SET,
	OP_CLR_F_USE,
	OP_CLR_T_ON,
	OP_CLR_T_SET,
	OP_CLR_T_USE,
	OP_CLR_TABLE,
	OP_CLR_V_SET,
	OP_COMMAND_LINE,
	OP_DRAW,
	OP_EDGE_PARAM,
	OP_EDGE_TYPE,
	OP_FILM,
	OP_GEOMETRY,
	OP_GL_BACK,
	OP_GL_FRONT,
	OP_HELP,
	OP_HIRES,
	OP_IMAGE,
	OP_IMAGE_BASENAME,
	OP_IN_CS,
	OP_IN_TRANSFORM,
	OP_IN_X_MIRROR,
	OP_IN_Z_ROTATE,
	OP_INACTIVE,
	OP_LABEL_EDGE_COLOR,
	OP_LABEL_EDGE_FONT,
	OP_LABEL_EDGE_ON,
	OP_LABEL_FACE_COLOR,
	OP_LABEL_FACE_FONT,
	OP_LABEL_FACE_ON,
	OP_LABEL_VERTEX_COLOR,
	OP_LABEL_VERTEX_FONT,
	OP_LABEL_VERTEX_ON,
	OP_LIGHT,
	OP_NO_FOG,
	OP_NO_IMAGE_STATE_SAVE,
	OP_NO_LIGHTING,
	OP_NO_UNIQUE,
	OP_NORMALIZE,
	OP_OBJECT,
	OP_ORIENTATION,
	OP_ORTHOGRAPHIC,
	OP_REPLICATE,
	OP_ROP,
	OP_ROT_MATRIX,
	OP_RX,
	OP_RY,
	OP_RZ,
	OP_SPIN,
	OP_SPP,
	OP_STEREO,
	OP_STEREO_ANGLE,
	OP_STEREO_NO_CROSS,
	OP_TOOLVIS,
	OP_TXYZ,
	OP_UDUMP,
	OP_VERTEX_SCALE,
	OP_WIN_WH,
	OP_WIN_XY,
};

//======================================== -back 
ARGUMENT	arg_back[] = {
	"fill",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_FILL,  0,	"",	/// back 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_FILL;
	"line",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_LINE,  0,	"",	///, 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_LINE;
	"point",	"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_POINT, 0,	"",	///	, 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_POINT;
};
ARGUMENT_SET	set_back[] = {
	sizeof(arg_back) / sizeof(ARGUMENT), arg_back,			// top level
};

ARGUMENT	arg_front[] = {
	"fill",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_FILL,  0,	"",	/// back 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_FILL;
	"line",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_LINE,  0,	"",	///, 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_LINE;
	"point",	"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_POLYMODE_POINT, 0,	"",	///	, 1)) ctx->geomAdj.polymode[1] = GEOMETRY_POLYMODE_POINT;
};
ARGUMENT_SET	set_front[] = {
	sizeof(arg_front) / sizeof(ARGUMENT), arg_front,			// top level
};

ATYPE	type_film[] = { ATYPE_STRING, ATYPE_INTEGER };
ADDR	addr_film[] = { 0, 0, };

ARGUMENT	arg_geometry[] = {
	"cube",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_CUBEHEDRON,  0,	"",	/// geometry
	"icosa",	"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_ICOSAHEDRON, 0,	"",	///
	"octa",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_OCTAHEDRON,  0,	"",	///
	"tetra",	"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_TETRAHEDRON, 0,	"",	///
};
ARGUMENT_SET	set_geometry[] = {
	sizeof(arg_geometry) / sizeof(ARGUMENT), arg_geometry,		// top level
};
ARGUMENT	arg_orientation[] = {
	"edge",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_ORIENTATION_EDGE,	0,	"",	/// orientation
	"face",		"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_ORIENTATION_FACE,	0,	"",	///
	"vertex",	"", 1, 0, (int*)ATYPE_SET_EXPLICIT, (int*)GEOMETRY_ORIENTATION_VERTEX,  0,	"",	///
};
ARGUMENT_SET	set_orientation[] = {
	sizeof(arg_orientation) / sizeof(ARGUMENT), arg_orientation,		// top level
};

ARGUMENT	arg_main[] = {  // pre-sorted alphabetically
	"-active",				"-act",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"set current object to be active",
	"-axis",				"-ax",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable axis",
	"-axis_label",			"-axl",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"display axis labels",
	"-capture_directory",	"-capd",	0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set capture directory",
	"-cd",					"-cd",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set working directory",
	"-circle",				"-cir",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable circle mode",
	"-clip",				"-clip",	0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"enable clippling and set z and increment (z, increment)",
	"-clr_background",		"-cbs",		6,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"change the background color (r, g, b)",
	"-clr_e_set",			"-ces",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"set current object edge override color (r, g, b)",
	"-clr_e_use",			"-ceu",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set which color to use for current object edges (A|O)",
	"-clr_f_default",		"-cfds",	0,  (ATYPE_ARRAY | 4),	(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"set default color to assign to faces (r, g, b)",
	"-clr_f_set",			"-cfs",		0,  (ATYPE_ARRAY | 4),	(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"set current object face override color (r, g, b)",
	"-clr_f_use",			"-cfu",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set which color to use for current object faces (E|A|O)",
	"-clr_t_on",			"-cto",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable transparency for faces",
	"-clr_t_set",			"-cts",		0,	1,					(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"set override alpha value for transparent",
	"-clr_t_use",			"-ctu",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set which alpha to use for current object faces (E|O)",
	"-clr_table",			"-ct",		0,  1,					(int*)ATYPE_STRING,			(int*)0, 0,						"color table filename to read (filename)",
	"-clr_v_set",			"-cvs",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT_CLAMP,	(int*)0, 0,						"set current object vertex color (r, g, b)",
	"-command_line",		"-cl",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"process command line file",
	"-draw",				"-dr",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set which components to draw on current object (FEV)",
	"-e_param",				"-ep",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_DOUBLE,			(int*)0, 0,						"set edge parameters for the current object (width, height, offset)",
	"-e_type",				"-et",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set edge type for current object (box|round)",
	"-film",				"-film",	0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, (void*)addr_film,		"enable film mode ( #frames, base_filename )",
	"-geometry",			"-geo",		0,	1,					(int*)ATYPE_SUB_ARGUMENT,	(int*)0, (void*)&set_geometry,	"set base geometry (icosa|octa|cube|tetra)",
	"-gl_back",				"-glb",		0,	1,					(int*)ATYPE_SUB_ARGUMENT,	(int*)0, (void*)&set_back,		"set OpenGL mode for back facing polygons (fill|line|point)",
	"-gl_front",			"-glf",		0,	1,					(int*)ATYPE_SUB_ARGUMENT,	(int*)0, (void*)&set_front,		"set OpenGL mode for front facing polygons (fill|line|point)",
	"-help",				"-help",	0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"display command line options",
	"-hires",				"-hi",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable high resolution rendering for edges and vertices",
	"-image",				"-im",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"enable image capture mode (filename)",
	"-image_basename",		"-ibn",		0,  1,					(int*)ATYPE_STRING,			(int*)0, 0,						"set image capture basename (filename)",
	"-in_cs",				"-incs",	0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable input center and scaling",
	"-in_transform",		"-int",		0,  (ATYPE_ARRAY | 6),	(int*)ATYPE_DOUBLE,			(int*)0, 0,						"set and enable input rotation transformation (Zx, Zy, Zz, Yx, Yy, Yz)",
	"-in_x_mirror",			"-inx",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable input x axis mirroring",
	"-in_z_rotate",			"-inz",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable input z axis repetition",
	"-inactive",			"-ina",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)0, 0,						"set current object to be inactive",
	"-label_e_clr",			"-lec",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT,			(int*)0, 0,						"set the color of edge labels",
	"-label_e_font",		"-lef",		0,	0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set the edge label font",
	"-label_e_on",			"-leo",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"display object's edge label",
	"-label_f_clr",			"-lfc",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT,			(int*)0, 0,						"set the color of face labels",
	"-label_f_font",		"-lff",		0,	0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set the face label font",
	"-label_f_on",			"-lfo",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"display object's face label",
	"-label_v_clr",			"-lvc",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT,			(int*)0, 0,						"set the color of vertex labels",
	"-label_v_font",		"-lvf",		0,	0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set the vertex label font",
	"-label_v_on",			"-lvo",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"display object's vertex label",
	"-light",				"-li",		0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_DOUBLE,			(int*)0, 0,						"change the position of the light (x, y, z)",
	"-no_fog",				"-nf",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)0, 0,						"disable fog",
	"-no_image_state_save",	"-nimss",	0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)0, 0,						"disable image capture mode to save state at same time(filename)",
	"-no_lighting",			"-nl",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)0, 0,						"disable lighting",
	"-no_unique",			"-nu",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"disable unique processing on input",
	"-normalize",			"-no",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable normalization of all vertices",
	"-o",					"-o",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set current input object (filename.off|.spc)",
	"-orientation",			"-ori",		0,	1,					(int*)ATYPE_SUB_ARGUMENT,	(int*)0, (void*)&set_orientation,	"change default orientation (face|edge|vertex)",
	"-orthographic",		"-ort",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)GEOMETRY_PROJECTION_ORTHOGRAPHIC, 0,	"enable orthographic projection",
	"-replicate",			"-rep",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set current object replication options (01ZX)",
	"-rop",					"-rop",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable relative object path in state file",
	"-rot_matrix",			"-rm",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set the rotation matrix",
	"-rx",					"-rx",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set initial x axis rotation (angle)",
	"-ry",					"-ry",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set initial y axis rotation (angle)",
	"-rz",					"-rz",		0,  0,					(int*)ATYPE_USER_FUNCTION,	(int*)0, 0,						"set initial z axis rotation (angle)",
	"-spin",				"-spin",	0,  (ATYPE_ARRAY | 4),	(int*)ATYPE_FLOAT,			(int*)0, 0,						"enable spin mode and set parameters (dx, dy, dz, mSec)",
	"-spp",					"-spp",		0,  1,					(int*)ATYPE_INTEGER,		(int*)0, 0,						"set samples per pixel for rendering (#)",
	"-stereo",				"-st",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable stereo rendering - split screen",
	"-stereo_angle",		"-sta",		0,  1,					(int*)ATYPE_FLOAT,			(int*)0, 0,						"set the stereo eye seperation angle (angle)",
	"-stereo_no_cross",		"-stnc",	0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"disable cross-eye mode for stereo",
	"-toolvis",				"-tv",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"make tool windows visible at startup",
	"-txyz",				"-txyz",	0,  (ATYPE_ARRAY | 3),	(int*)ATYPE_FLOAT,			(int*)0, 0,						"set initial translation (x, y, z)",
	"-udump",				"-ud",		0,  0,					(int*)ATYPE_SET_EXPLICIT,	(int*)1, 0,						"enable automatic dump of unique processing results after object input",
	"-v_scale",				"-vs",		0,  1,					(int*)ATYPE_DOUBLE,			(int*)0, 0,						"set current object vertex scale (scale)",
	"-win_wh",				"-wwh",		0,  (ATYPE_ARRAY | 2),	(int*)ATYPE_INTEGER,		(int*)0, 0,						"set initial rendering window width and height (w, h)",
	"-win_xy",				"-wxy",		0,  (ATYPE_ARRAY | 2),	(int*)ATYPE_INTEGER,		(int*)0, 0,						"set initial rendering window left top corner position (x, y)",
};

ARGUMENT_SUBSTITUTE arg_main_substitute[]={
/*	OP_CLR_BACKGROUND,		*/  "-act",			"-act",
/*	OP_CLR_BACKGROUND,		*/  "-ax",			"-axis",
/*	OP_CLR_BACKGROUND,		*/  "-axl",			"-axis_label",
/*	OP_CD,					*/  "-capd",		"-capture_directory",
/*	OP_CLR_BACKGROUND,		*/  "-cbs",			"-clr_background",
/*	OP_CD,					*/  "-cd",			"-cd",
/*	OP_CLR_E_SET,			*/  "-ces",			"-clr_e_set",
/*	OP_CLR_E_USE,			*/  "-ceu",			"-clr_e_use",
/*	OP_CLR_F_DEFAULT,		*/  "-cfds",		"-clr_f_default",
/*	OP_CLR_F_SET,			*/  "-cfs",			"-clr_f_set",
/*	OP_CLR_F_USE,			*/  "-cfu",			"-clr_f_use",
/*	OP_CIRCLE,				*/  "-cir",			"-circle",
/*	OP_COMMAND_LINE,		*/  "-cl",			"-command_line",
/*	OP_CLIP,				*/  "-clip",		"-clip",
/*	OP_CLR_TABLE,			*/  "-ct",			"-clr_table",
/*	OP_CLR_TABLE,			*/  "-cto",			"-clr_t_on",
/*	OP_CLR_TABLE,			*/  "-cts",			"-clr_t_set",
/*	OP_CLR_TABLE,			*/  "-ctu",			"-clr_t_use",
/*	OP_CLR_V_SET,			*/  "-cvs",			"-clr_v_set",
/*	OP_DRAW,				*/  "-dr",			"-draw",
/*	OP_EDGE_PARAM,			*/  "-ep",			"-e_param",
/*	OP_EDGE_TYPE,			*/  "-et",			"-e_type",
/*	OP_FILM,				*/  "-film",		"-film",
/*	OP_GEOMETRY,			*/  "-geo",			"-geometry",
/*	OP_GL_BACK,				*/  "-glb",			"-gl_back",
/*	OP_GL_FRONT,			*/  "-glf",			"-gl_front",
/*	OP_HELP,				*/  "-help",		"-help",
/*	OP_HIRES,				*/  "-hi",			"-hires",
/*	OP_IMAGE,				*/  "-ibn",			"-image_basename",
/*	OP_IMAGE,				*/  "-im",			"-image",
/*	OP_INACTIVE,			*/  "-ina",			"-inactive",
/*	OP_IN_CS,				*/  "-incs",		"-in_cs",
/*	OP_IN_X_MIRROR,			*/  "-int",			"-in_transform",
/*	OP_IN_Z_ROTATE,			*/  "-inx",			"-in_x_mirror",
/*	OP_IN_TRANSFORM,		*/  "-inz",			"-in_z_rotate",
/*--------------------------*/	"-lec",			"-label_e_clr",
/*--------------------------*/	"-lef",			"-label_e_font",
/*--------------------------*/	"-leo",			"-label_e_on",
/*--------------------------*/	"-lfc",			"-label_f_clr",
/*--------------------------*/	"-lff",			"-label_f_font",
/*--------------------------*/	"-lfo",			"-label_f_on",
/*	OP_LIGHT,				*/  "-li",			"-light",
/*--------------------------*/	"-lvc",			"-label_v_clr",
/*--------------------------*/	"-lvf",			"-label_v_font",
/*--------------------------*/	"-lvo",			"-label_v_on",
/*	OP_NO_FOG,				*/  "-nf",			"-no_fog",
/*	OP_NO_IMAGE_STATE_SAVE,	*/  "-nimss",		"-no_image_state_save",
/*	OP_NO_LIGHTING,			*/  "-nl",			"-no_lighting",
/*	OP_NORMALIZE,			*/  "-no",			"-normalize",
/*	OP_NO_UNIQUE,			*/  "-nu",			"-no_unique",
/*	OP_OBJECT,				*/  "-o",			"-o",
/*	OP_ORIENTATION,			*/  "-ori",			"-orientation",
/*	OP_ORTHOGRAPHIC,		*/  "-ort",			"-orthographic",
/*	OP_REPLICATE,			*/  "-rep",			"-replicate",
/*	OP_REPLICATE,			*/  "-rm",			"-rot_matrix",
/*	OP_ROP,					*/  "-rop",			"-rop",
/*	OP_RX,					*/  "-rx",			"-rx",
/*	OP_RY,					*/  "-ry",			"-ry",
/*	OP_RZ,					*/  "-rz",			"-rz",
/*	OP_SPIN,				*/  "-spin",		"-spin",
/*	OP_SPP,					*/  "-spp",			"-spp",
/*	OP_STEREO,				*/  "-st",			"-stereo",
/*	OP_STEREO_ANGLE,		*/  "-sta",			"-stereo_angle",
/*	OP_STEREO_NO_CROSS,		*/  "-stnc",		"-stereo_no_cross",
/*	OP_TOOLVIS,				*/  "-tv",			"-toolvis",
/*	OP_TXYZ,				*/  "-txyz",		"-txyz",
/*	OP_UDUMP,				*/  "-ud",			"-udump",
/*	OP_VERTEX_SCALE,		*/  "-vs",			"-v_scale",
/*	OP_WIN_WH,				*/  "-wwh",			"-win_wh",
/*	OP_WIN_XY,				*/  "-wxy",			"-win_xy",
};

ARGUMENT_SET	set_main[] = {
	sizeof(arg_main) / sizeof(ARGUMENT), arg_main,			// top level
};

ARGUMENT_SUBSTITUTE_SET	set_main_substitute[] = {
	sizeof(arg_main_substitute) / sizeof(ARGUMENT_SUBSTITUTE), arg_main_substitute,			// top level
};

//-----------------------------------------------------------------------------
int ds_filename_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
//	int		i;
	DS_CTX		*ctx;
	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	// CREATE A NEW INPUT OBJECT
	DS_GEO_INPUT_OBJECT	*gio = (DS_GEO_INPUT_OBJECT*)malloc(sizeof(DS_GEO_INPUT_OBJECT));
	if (!gio) return 1;

	LL_AddTail(ctx->inputObjq, gio); // add to queue
	ctx->curInputObj = gio; // make it the current object

	// inherit the default input object 
	memcpy(gio, &ctx->defInputObj, sizeof(DS_GEO_INPUT_OBJECT));

	// replace the name
	gio->filename = (char*)malloc(strlen(av[*currentArgIndex]) + 1);
	strcpy(gio->filename, av[*currentArgIndex]);

	++*currentArgIndex; // move forward by one

	// update the addresses
	arg_main[OP_CLR_E_SET].addr		= (void*)&ctx->curInputObj->cAttr.edge.color;
	arg_main[OP_CLR_F_SET].addr		= (void*)&ctx->curInputObj->cAttr.face.color;
	arg_main[OP_CLR_V_SET].addr		= (void*)&ctx->curInputObj->cAttr.vertex.color;
	arg_main[OP_EDGE_PARAM].addr	= (void*)&ctx->curInputObj->eAttr.width;
	arg_main[OP_ACTIVE].addr		= (void*)&ctx->curInputObj->active;
	arg_main[OP_INACTIVE].addr		= (void*)&ctx->curInputObj->active;
	arg_main[OP_VERTEX_SCALE].addr	= (void*)&ctx->curInputObj->vAttr.scale;
	arg_main[OP_CLR_T_ON].addr		= (void*)&ctx->curInputObj->tAttr.onFlag;
	arg_main[OP_CLR_T_SET].addr		= (void*)&ctx->curInputObj->tAttr.alpha;
	arg_main[OP_CLR_F_DEFAULT].addr = (void*)&ctx->curInputObj->faceDefault;

	arg_main[OP_LABEL_EDGE_ON].addr   = (void*)&ctx->curInputObj->lFlags.edge;				// need to set index & flag correctly afterwards
	arg_main[OP_LABEL_FACE_ON].addr   = (void*)&ctx->curInputObj->lFlags.face;				// need to set index & flag correctly afterwards
	arg_main[OP_LABEL_VERTEX_ON].addr = (void*)&ctx->curInputObj->lFlags.vertex;			// need to set index & flag correctly afterwards

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_font_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	//	int		i;
	DS_CTX		*ctx;
	DS_LABEL	*label;

//	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (!(label = (DS_LABEL*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	if (!stricmp(av[*currentArgIndex], "H10"))      label->font = GLUT_BITMAP_HELVETICA_10;
	else if (!stricmp(av[*currentArgIndex], "H12")) label->font = GLUT_BITMAP_HELVETICA_12;
	else if (!stricmp(av[*currentArgIndex], "H18")) label->font = GLUT_BITMAP_HELVETICA_18;
	else if (!stricmp(av[*currentArgIndex], "T10")) label->font = GLUT_BITMAP_TIMES_ROMAN_10;
	else if (!stricmp(av[*currentArgIndex], "T24")) label->font = GLUT_BITMAP_TIMES_ROMAN_24;

	++*currentArgIndex; // move forward by one

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_rotation(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error, int axis)
//-----------------------------------------------------------------------------
{
//	int		i;
	DS_CTX		*ctx;
	double	rotation;
	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	// get rotation value
	rotation = atof(av[*currentArgIndex]);
	++*currentArgIndex; // move forward by one

	if (rotation != 0)
	{
		MTX_MATRIX	mr, mm;

		ctx->matrixFlag = 1;
		mtx_create_rotation_matrix(&mr, axis, DTR(rotation));
		mtx_multiply_matrix(&ctx->matrix, &mr, &mm);
		ctx->matrix = mm;
	}

	return 0;
}
//-----------------------------------------------------------------------------
static int ds_rot_x_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	return ds_rotation(arg, currentArgIndex, maxNArgs, av, error, MTX_ROTATE_X_AXIS);
}
//-----------------------------------------------------------------------------
static int ds_rot_y_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	return ds_rotation(arg, currentArgIndex, maxNArgs, av, error, MTX_ROTATE_Y_AXIS);
}
//-----------------------------------------------------------------------------
static int ds_rot_z_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	return ds_rotation(arg, currentArgIndex, maxNArgs, av, error, MTX_ROTATE_Z_AXIS);
}

//-----------------------------------------------------------------------------
static int ds_clip_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
//	int		i;
	DS_CTX		*ctx;
	double	zposition;
	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	// get z position value
	zposition = atof(av[*currentArgIndex]);
	++*currentArgIndex; // move forward by one

	ctx->drawAdj.clipFlag = 1;
	ctx->drawAdj.clipZValue = zposition;

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_help_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	DS_CTX			*ctx;
	int			i, len;
	DWORD		nBytes;
	ARGUMENT	*a;
	char		buffer[1024];

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }

	AllocConsole();
	ctx->handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	ctx->handle_in = GetStdHandle(STD_INPUT_HANDLE);

	len = sprintf_s(buffer, sizeof(buffer), "%s\n", ctx->version.text);
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);
	len = sprintf_s(buffer, sizeof(buffer), "Version - %d.%d\n\n", ctx->version.major, ctx->version.minor );
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);

	len = sprintf_s(buffer, sizeof(buffer), "Command line options\n\n" );
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);

	len = sprintf_s(buffer, sizeof(buffer), " %-20s %-9s %s\n", "Option", "Alternate", "Description");
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);
	len = sprintf_s(buffer, sizeof(buffer), " %-20s %-9s %s\n", "-----------------", "---------", "-------------------------");
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);

	for (i = 0, a=set_main->argument; i < set_main->nArguments; ++i, ++a)
	{
		len = sprintf_s(buffer, sizeof(buffer), " %-20s (%-7s) %s\n", a->text, a->alt, a->description);
		WriteFile(ctx->handle_out, buffer, len, NULL, NULL);
	}

	len = sprintf_s(buffer, sizeof(buffer), "\nHit return to exit or c to continue: ");
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);
	{

		char buffer[256];
		ReadFile(ctx->handle_in, buffer, 1, &nBytes, 0);
		if ( buffer[0] != 'c')
			exit(0);
		FreeConsole();
	}
	return 0;
}

//-----------------------------------------------------------------------------
static int ds_draw_what_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;
	int					drawWhat = 0;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	while (c = *p++)
	{
		switch (c) {
		case 'f':  drawWhat |= GEOMETRY_DRAW_TRIANGLES;	break;
		case 'e':  drawWhat |= GEOMETRY_DRAW_EDGES;		break;
		case 'v':  drawWhat |= GEOMETRY_DRAW_VERTICES;		break;
		case 'F':  drawWhat |= GEOMETRY_DRAW_TRIANGLES;	break;
		case 'E':  drawWhat |= GEOMETRY_DRAW_EDGES;		break;
		case 'V':  drawWhat |= GEOMETRY_DRAW_VERTICES;		break;
		}
	}

	if (drawWhat) // don't update if not set correctly
		gio->drawWhat = drawWhat; // reset

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_obj_clr_edge_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	switch (c = *p) {
	case 'a':  gio->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;	break;
	case 'A':  gio->cAttr.edge.state = DS_COLOR_STATE_AUTOMATIC;	break;
	case 'o':  gio->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE;	break;
	case 'O':  gio->cAttr.edge.state = DS_COLOR_STATE_OVERRIDE;	break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_obj_clr_face_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	switch (c = *p) {
	case 'a':  gio->cAttr.face.state = DS_COLOR_STATE_AUTOMATIC;	break;
	case 'A':  gio->cAttr.face.state = DS_COLOR_STATE_AUTOMATIC;	break;
	case 'e':  gio->cAttr.face.state = DS_COLOR_STATE_EXPLICIT;		break;
	case 'E':  gio->cAttr.face.state = DS_COLOR_STATE_EXPLICIT;		break;
	case 'o':  gio->cAttr.face.state = DS_COLOR_STATE_OVERRIDE;		break;
	case 'O':  gio->cAttr.face.state = DS_COLOR_STATE_OVERRIDE;		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_obj_clr_transparency_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	switch (c = *p) {
	case 'e':  gio->tAttr.state = DS_COLOR_STATE_EXPLICIT;		break;
	case 'E':  gio->tAttr.state = DS_COLOR_STATE_EXPLICIT;		break;
	case 'o':  gio->tAttr.state = DS_COLOR_STATE_OVERRIDE;		break;
	case 'O':  gio->tAttr.state = DS_COLOR_STATE_OVERRIDE;		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_object_e_type_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	switch (c = *p) {
	case 'R':
	case 'r':
		gio->eAttr.type = GEOMETRY_EDGE_ROUND;
		break;
	case 'B':
	case 'b':
		gio->eAttr.type = GEOMETRY_EDGE_SQUARE;
		break;
	}

	return 0;
}

//-----------------------------------------------------------------------------
static int ds_current_directory_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				*p; // c, *p;
	DS_CTX				*ctx;
	int					status; 

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error
															 //	// directory change - prior to reading any external files
	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one after getting path
	
	if (strlen(p))
	{
		ds_build_dsf(&ctx->curDir, p, 0);
		strcpy(ctx->currentDir, ctx->curDir.fullName);
//		ds_build_dsf(&ctx->curDir, ctx->currentDir, 0);

		strcpy(ctx->curWorkingDir, p);
		status = SetCurrentDirectory(ctx->currentDir) ? 0 : 1;
	//	status = SetCurrentDirectory(ctx->curWorkingDir) ? 0 : 1;
	//	GetCurrentDirectory(512, ctx->curWorkingDir);
		GetCurrentDirectory(512, ctx->currentDir);
		return status;
	}
	else
		return 1;
}

//-----------------------------------------------------------------------------
static int ds_capture_directory_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				*p; // c, *p;
	DS_CTX				*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error
															 //	// directory change - prior to reading any external files
	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one after getting path

	if (strlen(p))
	{
		ds_build_dsf(&ctx->capDir, p, 0);
		strcpy(ctx->captureDir, ctx->capDir.fullName);
		return 0;
	}
	else
		return 1;
}

//-----------------------------------------------------------------------------
static int ds_image_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				*p, *q; // c, *p;
	DS_CTX				*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	if (!strcmp(arg->alt, "-film"))
	{
		if (*currentArgIndex + 1 >= maxNArgs) { ++*error; return 1; }//not enough arguments

		p = av[*currentArgIndex];
		++*currentArgIndex; // move forward by one after getting image name
		strcpy(ctx->png.basename, p);
		ctx->png.enabled = 1;

		q = av[*currentArgIndex];
		++*currentArgIndex; // move forward by one after getting image name
		ctx->png.nFrames = atoi(q);
	}
	else
	{
		p = av[*currentArgIndex];
		++*currentArgIndex; // move forward by one after getting image name

		strcpy(ctx->png.basename, p);
		ctx->png.nFrames = 1;
		ctx->png.enabled = 1;
	}
	return 0;
}

//-----------------------------------------------------------------------------
static int ds_obj_replicate_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	DS_GEO_INPUT_OBJECT	*gio = ctx->curInputObj;
	if (!gio)
		return 1;

	p = av[*currentArgIndex];
	++*currentArgIndex; // move forward by one

	while (c = *p++)
	{
		switch (c) {
		case '0':  gio->rAttr.oneFaceFlag = 0;	break;
		case '1':  gio->rAttr.oneFaceFlag = 1;	break;
		case 'x':
		case 'X':
			gio->rAttr.xMirrorFlag = 1;
			break;
		case 'z':
		case 'Z':
			gio->rAttr.zRotationFlag = 1;
			break;
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
static int ds_rot_matrix_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error)
//-----------------------------------------------------------------------------
{
//	char				c, *p;
	DS_CTX					*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error

	if (*currentArgIndex + 8 >= maxNArgs) { ++*error; return 1; }//not enough arguments

	// load rotation matrix 9 numbers - row order
	mtx_set_unity(&ctx->matrix);
	ctx->matrix.data.row_column[0][0] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[0][1] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[0][2] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[1][0] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[1][1] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[1][2] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[2][0] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[2][1] = atof(av[(*currentArgIndex)++]);
	ctx->matrix.data.row_column[2][2] = atof(av[(*currentArgIndex)++]);

	return 0;
}

//-----------------------------------------------------------------------------
ds_command_line_arg_handler(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex, DS_ERROR *errInfo)
//-----------------------------------------------------------------------------
{
	char		*p; // c, *p;
	DS_CTX		*ctx;

	if (!(ctx = (DS_CTX*)arg->data)) { ++*error; return 1; }
	if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error
															
	p = av[*currentArgIndex]; // filename
	++*currentArgIndex; // move forward by one

	FILE				*fp;
	int					currentIndex = 0;
	char				buffer[1024];
	char				*array[64];
	int					argCount;// , argIndex;
	ARGUMENT_SUBSTITUTE	*arg_sub;

	// open file
	// read each line
	// discard comments
	// for each real line call parse components and call arg_decode
	//

	fopen_s(&fp, p, "r"); // open file
	if (!fp)
	{
		char buffer[128];
		sprintf(buffer, "File <%s> failed to open.", p);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
		return 1;
	}

	while (fgets(buffer, 1024, fp)) // read each line
	{
		argCount = ds_parse_lexeme(buffer, array, 64); // split line into words

		if (!strncmp(array[0], "#", 1) || !strncmp(array[0], "//", 2)) // simple check for comment
			continue; // comment line

		if (!strcmp(array[0], "DS_STATE")) // File type
			continue; // header

		if (!strcmp(array[0], "-command_line") || !strcmp(array[0], "-cl")) // simple check to avoid recursive condition
			continue;

		// look for alternate short form - and replace if found
		if (arg_sub = arg_find_substitute(set_main_substitute, array[0]))
		{
			array[0] = arg_sub->longForm;
		}

		currentIndex = 0;
		// process one argument
		if (arg_decode(set_main, &currentIndex, argCount, array, error, argIndex, errInfo))
		{
			// reset the argIndex
			*currentArgIndex -= 2; // move backward by two
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	return 0;
}

//-----------------------------------------------------------------------------
int cmd_line_init(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	// assign pointers that can't be done at compile time
	ARGUMENT	*arg;

	arg = arg_back;
	arg[0].addr = (void*)&ctx->geomAdj.polymode[1];
	arg[1].addr = (void*)&ctx->geomAdj.polymode[1];
	arg[2].addr = (void*)&ctx->geomAdj.polymode[1];
	arg = arg_front;											//"-front", 0, 1, atype + 0, adata + 0, addr + 0,    // set front facing draw mode
	arg[0].addr = (void*)&ctx->geomAdj.polymode[0];
	arg[1].addr = (void*)&ctx->geomAdj.polymode[0];
	arg[2].addr = (void*)&ctx->geomAdj.polymode[0];
	arg = arg_geometry;											//"-geometry" set base geometry
	arg[0].addr = (void*)&ctx->base_geometry.type; //cube
	arg[1].addr = (void*)&ctx->base_geometry.type; //icos
	arg[2].addr = (void*)&ctx->base_geometry.type; //octa
	arg[3].addr = (void*)&ctx->base_geometry.type; //tetra
	arg = arg_orientation;											//"-orientation
	arg[0].addr = (void*)&ctx->geomAdj.orientation;
	arg[1].addr = (void*)&ctx->geomAdj.orientation;
	arg[2].addr = (void*)&ctx->geomAdj.orientation;
	
	addr_film[0] = (void*)ctx->png.basename;			// need to set index & flag correctly afterwards
	addr_film[1] = (void*)&ctx->png.nFrames;			// need to set index & flag correctly afterwards

	arg = arg_main;
	// GLOBALS ====================================================
	arg[OP_AXIS].addr				= (void*)&ctx->drawAdj.axiiFlag;			// -ax axis
	arg[OP_AXIS_LABEL].addr			= (void*)&ctx->drawAdj.axiiLabelFlag;		// -axl axis

	arg[OP_CAPTURE_DIRECTORY].addr	= (void*)&ds_capture_directory_arg_handler;			// "-cd set current working directory  
	arg[OP_CAPTURE_DIRECTORY].data	= (void*)ctx;												// 
	
	arg[OP_CD].addr				= (void*)&ds_current_directory_arg_handler;			// "-cd set current working directory  
	arg[OP_CD].data				= (void*)ctx;												// 

	arg[OP_CIRCLE].addr			= (void*)&ctx->drawAdj.circleFlag;				// "-cd set current working directory  
	arg[OP_CLIP].addr			= (void*)&ds_clip_arg_handler;				// "-cd set current working directory  
	arg[OP_CLIP].data			= (void*)ctx;									// 
	arg[OP_CLR_F_DEFAULT].addr	= (void*)&ctx->clrCtl.face.defaultColor;	// treat as float array
	arg[OP_CLR_BACKGROUND].addr = (void*)&ctx->clrCtl.bkgClear;					// treat as float array
	arg[OP_CLR_TABLE].addr		= (void*)&ctx->clrCtl.user_color_table;			// filename
	arg[OP_COMMAND_LINE].addr	= (void*)&ds_command_line_arg_handler;		// NEED FUNCTION
	arg[OP_COMMAND_LINE].data	= (void*)ctx;
	//	arg[OP_GEOMETRY].addr		= 0; // need to be parsed
//	arg[OP_GL_BACK].addr		= 0
//	arg[OP_GL_FRONT].addr		= 0
	arg[OP_HELP].addr			= (void*)&ds_help_arg_handler;				// NEED FUNCTION
	arg[OP_HELP].data			= (void*)ctx;
	arg[OP_HIRES].addr			= (void*)&ctx->drawAdj.hiResFlag;				// -high resolution
	arg[OP_FILM].addr			= (void*)ds_image_arg_handler;					// need to set index & flag correctly afterwards
	arg[OP_FILM].data			= (void*)ctx;									// need to set index & flag correctly afterwards
	arg[OP_IMAGE].addr			= (void*)ds_image_arg_handler;					// need to set index & flag correctly afterwards
	arg[OP_IMAGE].data			= (void*)ctx;									// need to set index & flag correctly afterwards
	arg[OP_IMAGE_BASENAME].addr	= (void*)ctx->png.basename;						// just set name
//	arg[OP_NO_IMAGE_STATE_SAVE].addr = (void*)&ctx->png.stateSaveFlag;				// need to set index & flag correctly afterwards

	arg[OP_LABEL_EDGE_COLOR].addr = (void*)&ctx->label.edge.color;	// treat as float array
	arg[OP_LABEL_EDGE_FONT].addr = (void*)&ds_font_arg_handler; //&gio->active;
	arg[OP_LABEL_EDGE_FONT].data = (void*)&ctx->label.edge;

	arg[OP_LABEL_FACE_COLOR].addr = (void*)&ctx->label.face.color;	// treat as float array
	arg[OP_LABEL_FACE_FONT].addr = (void*)&ds_font_arg_handler; //&gio->active;
	arg[OP_LABEL_FACE_FONT].data = (void*)&ctx->label.face;

	arg[OP_LABEL_VERTEX_COLOR].addr = (void*)&ctx->label.vertex.color;	// treat as float array
	arg[OP_LABEL_VERTEX_FONT].addr = (void*)&ds_font_arg_handler; //&gio->active;
	arg[OP_LABEL_VERTEX_FONT].data = (void*)&ctx->label.vertex;

	arg[OP_LIGHT].addr			= (void*)&ctx->clrCtl.light;					// -light
	arg[OP_NO_FOG].addr			= (void*)&ctx->drawAdj.fogFlag;					// -nofog
	arg[OP_NO_IMAGE_STATE_SAVE].addr = (void*)&ctx->png.stateSaveFlag;				// need to set index & flag correctly afterwards
	arg[OP_NO_LIGHTING].addr	= (void*)&ctx->clrCtl.useLightingFlag;			// -nolighting
	arg[OP_NORMALIZE].addr		= (void*)&ctx->drawAdj.normalizeFlag;			// -normal
//	arg[OP_ORIENTATION].addr	= 0;
	arg[OP_ORTHOGRAPHIC].addr	= (void*)&ctx->drawAdj.projection;				// change default
	arg[OP_ROP].addr			= (void*)&ctx->relativeObjPathFlag;				// change relative path
	arg[OP_SPP].addr			= (void*)&ctx->opengl.samplesPerPixel;			// -spp
	arg[OP_SPIN].addr			= (void*)&ctx->drawAdj.spin;					// -spin
	arg[OP_STEREO].addr			= (void*)&ctx->drawAdj.stereoFlag;				// -stereo
	arg[OP_STEREO_ANGLE].addr	= (void*)&ctx->drawAdj.eyeSeparation;
	arg[OP_STEREO_NO_CROSS].addr= (void*)&ctx->drawAdj.stereoCrossEyeFlag;
	arg[OP_TOOLVIS].addr		= (void*)&ctx->window.toolsVisible;				// -toolvis
	arg[OP_WIN_WH].addr			= (void*)&ctx->window.width;					// -win_wh
	arg[OP_WIN_XY].addr			= (void*)&ctx->window.start_x;					// -win_xy

//	INPUT MODIFICATIONS =================
	arg[OP_IN_CS].addr			= (void*)&ctx->inputTrans.centerAndScaleFlag;	// -in_cs
	arg[OP_IN_X_MIRROR].addr	= (void*)&ctx->inputTrans.mirrorFlag;			// -in_x_mirror
	arg[OP_IN_Z_ROTATE].addr	= (void*)&ctx->inputTrans.replicateFlag;		// -in_z_replicate
	arg[OP_IN_TRANSFORM].addr	= (void*)&ctx->inputTrans.zAxis;				// -ztran
	arg[OP_NO_UNIQUE].addr		= (void*)&ctx->inputTrans.guaFlag;				// -nogua
	arg[OP_ROT_MATRIX].addr		= (void*)&ds_rot_matrix_arg_handler;			// 
	arg[OP_ROT_MATRIX].data		= (void*)ctx;									// 
	arg[OP_RX].addr				= (void*)&ds_rot_x_arg_handler;				// 
	arg[OP_RX].data				= (void*)ctx;									// 
	arg[OP_RY].addr				= (void*)&ds_rot_y_arg_handler;				// 
	arg[OP_RY].data				= (void*)ctx;									// 
	arg[OP_RZ].addr				= (void*)&ds_rot_z_arg_handler;				// 
	arg[OP_RZ].data				= (void*)ctx;									// 
	arg[OP_TXYZ].addr			= (void*)&ctx->trans;							// -xyztran
	arg[OP_UDUMP].addr			= (void*)&ctx->inputTrans.guaResultsFlag;		// -unique_dump
	arg[OP_UDUMP].addr			= (void*)&ctx->inputTrans.guaResultsFlag;		// -unique_dump


// OBJECT PARAMETERS =================
	arg[OP_CLR_E_SET].addr		= (void*)&ctx->defInputObj.cAttr.edge.color; //  &gio->cAttr.edge.color;
	arg[OP_CLR_E_USE].addr		= (void*)&ds_obj_clr_edge_arg_handler; // NEED FUNCTION
	arg[OP_CLR_E_USE].data		= (void*)ctx;

//	arg[OP_CLR_F_DEFAULT].addr	= (void*)&ctx->clrCtl.face.defaultColor; // keep it here
	arg[OP_CLR_F_DEFAULT].addr	= (void*)&ctx->defInputObj.faceDefault;

	arg[OP_CLR_F_SET].addr		= (void*)&ctx->defInputObj.cAttr.face.color; //&gio->cAttr.face.color;
	arg[OP_CLR_F_USE].addr		= (void*)&ds_obj_clr_face_arg_handler; // NEED FUNCTION
	arg[OP_CLR_F_USE].data		= (void*)ctx;

	arg[OP_CLR_T_ON].addr		= (void*)&ctx->defInputObj.tAttr.onFlag; 
	arg[OP_CLR_T_SET].addr		= (void*)&ctx->defInputObj.tAttr.alpha;
	arg[OP_CLR_T_USE].addr		= (void*)&ds_obj_clr_transparency_arg_handler; // NEED FUNCTION
	arg[OP_CLR_T_USE].data		= (void*)ctx;

	arg[OP_CLR_V_SET].addr		= (void*)&ctx->defInputObj.cAttr.vertex.color; //&gio->cAttr.vertex.color;

	arg[OP_DRAW].addr			= (void*)&ds_draw_what_arg_handler; // NEED FUNCTION
	arg[OP_DRAW].data			= (void*)ctx;	
 
	arg[OP_EDGE_PARAM].addr		= (void*)&ctx->defInputObj.eAttr.width; //&gio->eAttr.width;

	arg[OP_EDGE_TYPE].addr		= (void*)&ds_object_e_type_arg_handler;
	arg[OP_EDGE_TYPE].data		= (void*)ctx;

	arg[OP_CLR_F_USE].addr		= (void*)&ds_obj_clr_face_arg_handler; // NEED FUNCTION
	arg[OP_CLR_F_USE].data		= (void*)ctx;

	arg[OP_ACTIVE].addr			= (void*)&ctx->defInputObj.active; //&gio->active;
	arg[OP_INACTIVE].addr		= (void*)&ctx->defInputObj.active; //&gio->active;

	arg[OP_LABEL_EDGE_ON].addr    = (void*)&ctx->defInputObj.lFlags.edge;				// need to set index & flag correctly afterwards
	arg[OP_LABEL_FACE_ON].addr    = (void*)&ctx->defInputObj.lFlags.face;				// need to set index & flag correctly afterwards
	arg[OP_LABEL_VERTEX_ON].addr  = (void*)&ctx->defInputObj.lFlags.vertex;				// need to set index & flag correctly afterwards

	arg[OP_OBJECT].addr			= (void*)&ds_filename_arg_handler;
	arg[OP_OBJECT].data			= (void*)ctx;

	arg[OP_REPLICATE].addr		= (void*)&ds_obj_replicate_arg_handler; // NEED FUNCTION
	arg[OP_REPLICATE].data		= (void*)ctx;

	arg[OP_VERTEX_SCALE].addr = (void*)&ctx->curInputObj->vAttr.scale; //&gio->vAttr.scale;

	return 0;
}

//-----------------------------------------------------------------------------
int ds_command_line(DS_CTX *ctx, int ac, char **av, int *error, int *argIndex, DS_ERROR *errInfo)
//-----------------------------------------------------------------------------
{
	int					currentIndex = 0;
	ARGUMENT_SUBSTITUTE *arg_sub;
	int					status = 0;

	cmd_line_init(ctx); // initialize any pointers

	while (currentIndex < ac)
	{	
		// look for alternate short form - and replace if found
		if (arg_sub = arg_find_substitute(set_main_substitute, av[currentIndex]))
		{
			av[currentIndex] = arg_sub->longForm;
		}
		if(arg_decode(set_main, &currentIndex, ac, av, error, argIndex, errInfo))
			break;
	}
	return 0;
}

//-----------------------------------------------------------------------------
int ds_restore_state(DS_CTX *ctx, char *filename, DS_ERROR *errInfo)
//-----------------------------------------------------------------------------
{
	int					currentArgIndex = 1;
	int					error = 0;
	int					status = 0;
	int					argIndex;

	char	*av[] = { "-command_line", filename };

	ds_pre_init2(ctx); // reset everything to the default and delete any existing objects
	cmd_line_init(ctx); // re-initialize any pointers
	ds_command_line_arg_handler(&set_main[0].argument[0], &currentArgIndex, 2, av, &error, &argIndex, errInfo); // process 
	ds_post_init2(ctx); // post command line updates
	return error;
}

//-----------------------------------------------------------------------------
int ds_save_object_state(DS_CTX *ctx, FILE *fp, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	if (gobj->filename)
	{
		char	relativeFilename[1024];
/* DEBUG
		{
			int			i;
			DS_FILE		*dsf = &ctx->executable;
			fprintf ( fp, "# executable: " );
			for (i = 0; i < dsf->count; ++i)
				fprintf(fp, "%s/", &dsf->splitName[dsf->word[i]]);
			fprintf(fp, "\n");
			dsf = gobj->dsf;
			fprintf ( fp, "# object    : ");
			for (i = 0; i < dsf->count; ++i)
				fprintf(fp, "%s/", &dsf->splitName[dsf->word[i]]);
			fprintf(fp, "\n");
		}
*/ 
//		ds_cd_relative_filename( &ctx->executable, gobj->dsf, relativeFilename);
		ds_cd_relative_filename(ctx, &ctx->pgmData, gobj->dsf, relativeFilename);

		fprintf(fp, "# Object settings for <%s>\n", gobj->dsf->nameOnly );
		fprintf(fp, "-o \"%s\"\n", relativeFilename);
	}
	else
	{
		fprintf(fp, "# Default object settings\n");
	}
	if (!gobj->active) fprintf(fp, "-inactive\n");
	else fprintf(fp, "-active\n");

	if (gobj->drawWhat != GEOMETRY_DRAW_NONE)
	{
		fprintf(fp, "-draw ");
		if (gobj->drawWhat & GEOMETRY_DRAW_TRIANGLES) fprintf(fp, "f");
		if (gobj->drawWhat & GEOMETRY_DRAW_EDGES)     fprintf(fp, "e");
		if (gobj->drawWhat & GEOMETRY_DRAW_VERTICES)  fprintf(fp, "v");
		fprintf(fp, "\n");
	}

	fprintf(fp, "-clr_f_use ");
	switch(gobj->cAttr.face.state) {
	case DS_COLOR_STATE_AUTOMATIC: fprintf(fp, "a\n"); break;
	case DS_COLOR_STATE_EXPLICIT:  fprintf(fp, "e\n"); break;
	case DS_COLOR_STATE_OVERRIDE:  fprintf(fp, "o\n"); break;
	}
	fprintf(fp, "-clr_f_set %5.3f %5.3f %5.3f %5.3f\n", gobj->cAttr.face.color.r, gobj->cAttr.face.color.g, gobj->cAttr.face.color.b, gobj->cAttr.face.color.a);

	fprintf(fp, "-clr_e_use ");
	switch (gobj->cAttr.edge.state) {
	case DS_COLOR_STATE_AUTOMATIC: fprintf(fp, "a\n"); break;
	case DS_COLOR_STATE_EXPLICIT:  fprintf(fp, "e\n"); break;
	case DS_COLOR_STATE_OVERRIDE:  fprintf(fp, "o\n"); break;
	}
	fprintf(fp, "-clr_e_set %5.3f %5.3f %5.3f\n", gobj->cAttr.edge.color.r, gobj->cAttr.edge.color.g, gobj->cAttr.edge.color.b);
	fprintf(fp, "-clr_v_set %5.3f %5.3f %5.3f\n", gobj->cAttr.vertex.color.r, gobj->cAttr.vertex.color.g, gobj->cAttr.vertex.color.b);


	//gobj->eAttr;
	fprintf(fp, "-e_type %s\n", gobj->eAttr.type == GEOMETRY_EDGE_ROUND ? "round" : "box" );
	fprintf(fp, "-e_param %f %f %f\n", gobj->eAttr.width, gobj->eAttr.height, gobj->eAttr.offset);

	//gobj->vAttr;
	fprintf(fp, "-v_scale %f\n", gobj->vAttr.scale);//.width, gobj->eAttr.height, gobj->eAttr.offset);

	//gobj->rAttr;
	fprintf(fp, "-replicate "); //  , gobj->rAttr.oneFaceFlag, .width, gobj->eAttr.height, gobj->eAttr.offset);
	fprintf(fp, "%c", gobj->rAttr.oneFaceFlag ? '1' : '0');
	if (gobj->rAttr.xMirrorFlag) fprintf(fp, "X");
	if (gobj->rAttr.zRotationFlag) fprintf(fp, "Z");
	fprintf(fp, "\n");

	// gobj->tAttr
	fprintf(fp, "-clr_t_set %5.3f\n", gobj->tAttr.alpha); //  , gobj->rAttr.oneFaceFlag, .width, gobj->eAttr.height, gobj->eAttr.offset);
	fprintf(fp, "-clr_t_use ");
	switch (gobj->tAttr.state) {
	case DS_COLOR_STATE_AUTOMATIC: fprintf(fp, "a\n"); break;
	case DS_COLOR_STATE_EXPLICIT:  fprintf(fp, "e\n"); break;
	case DS_COLOR_STATE_OVERRIDE:  fprintf(fp, "o\n"); break;
	}
	if (gobj->tAttr.onFlag) fprintf(fp, "-clr_t_on\n");

	// gobj label flags
	if (gobj->lFlags.edge)   fprintf(fp, "-label_e_on\n");
	if (gobj->lFlags.face)   fprintf(fp, "-label_f_on\n");
	if (gobj->lFlags.vertex) fprintf(fp, "-label_v_on\n");

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
int ds_save_state(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	FILE	*fp;
	char	*p;

	if (filename)
	{
		int		i = strlen(filename);
		if (i > 4)
		{
			if (strcmp(&filename[i - 4], ".dss"))
				strcpy(&filename[i - 4], ".dss");
		}
	}
	fopen_s(&fp,filename, "w");
	if (!fp)
	{
		char buffer[128];
		sprintf(buffer, "File <%s> failed to open.", filename);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
		return 1;
	}

	// header for drag and drop functionality 
	fprintf(fp, "DS_STATE\n");
	fprintf(fp, "# Progam:  %s\n", ctx->version.text);
	fprintf(fp, "# Version: %d.%d\n", ctx->version.major, ctx->version.minor);
	fprintf(fp, "# Current State: ");

	// Get current date/time, format is YYYY-MM-DD.HH:mm:ss	
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);
	fprintf(fp, "%s\n", buf);


	// global
	fprintf(fp, "# Global settings\n");

	{
		char	relativePath[1024];
		//	-cd
		ds_build_dsf(&ctx->curDir, ctx->currentDir, 0); // update DSF
		ds_cd_relative_filename(ctx, &ctx->pgmData, &ctx->curDir, relativePath);
		fprintf(fp, "-cd \"%s\"\n", relativePath);

		//	-capd
		ds_cd_relative_filename(ctx, &ctx->pgmData, &ctx->capDir, relativePath);
		fprintf(fp, "-capture_directory \"%s\"\n", relativePath);
	}

//- axis
	if (ctx->drawAdj.axiiFlag) fprintf(fp, "-axis\n");
	if (ctx->drawAdj.axiiLabelFlag) fprintf(fp, "-axis_label\n");
	//- circle
	if (ctx->drawAdj.circleFlag) fprintf(fp, "-circle\n");
	//- clip
	if (ctx->drawAdj.clipFlag) fprintf(fp, "-clip %f %f\n", ctx->drawAdj.clipZValue, ctx->drawAdj.clipZIncrement);
	//- clr_backgound
	fprintf(fp, "-clr_background %5.3f %5.3f %5.3f\n", ctx->clrCtl.bkgClear.r, ctx->clrCtl.bkgClear.g, ctx->clrCtl.bkgClear.b);
	//- clr_table

	//- geometry
	switch (ctx->base_geometry.type) {
	case GEOMETRY_CUBEHEDRON:	p = arg_geometry[0].text; break;
	case GEOMETRY_ICOSAHEDRON:	p = arg_geometry[1].text; break;
	case GEOMETRY_OCTAHEDRON:	p = arg_geometry[2].text; break;
	case GEOMETRY_TETRAHEDRON:	p = arg_geometry[3].text; break;
	}
	fprintf(fp, "-geometry %s\n", p);

	//- gl_back
	switch (ctx->geomAdj.polymode[1]) {
	case GEOMETRY_POLYMODE_FILL:	p = arg_back[0].text; break;
	case GEOMETRY_POLYMODE_LINE:	p = arg_back[1].text; break;
	case GEOMETRY_POLYMODE_POINT:	p = arg_back[2].text; break;
	}
	fprintf(fp, "-gl_back %s\n", p);

	//- gl_front
	switch (ctx->geomAdj.polymode[0]) {
	case GEOMETRY_POLYMODE_FILL:	p = arg_front[0].text; break;
	case GEOMETRY_POLYMODE_LINE:	p = arg_front[1].text; break;
	case GEOMETRY_POLYMODE_POINT:	p = arg_front[2].text; break;
	}
	fprintf(fp, "-gl_front %s\n", p);

	// open tool windows
	if (ctx->attrControl && ctx->objControl)
		fprintf(fp, "-toolvis\n");

	//- hires
	if (ctx->drawAdj.hiResFlag) fprintf(fp, "-hires\n");

	//- image_state_save
	if (!ctx->png.stateSaveFlag) fprintf(fp, "-no_image_state_save\n");

	// labels 
	fprintf(fp, "-label_e_clr %.3f %.3f %.3f\n", ctx->label.edge.color.r, ctx->label.edge.color.g, ctx->label.edge.color.b);
	fprintf(fp, "-label_e_font %s\n", font_name(ctx->label.edge.font));
	fprintf(fp, "-label_f_clr %.3f %.3f %.3f\n", ctx->label.face.color.r, ctx->label.face.color.g, ctx->label.face.color.b);
	fprintf(fp, "-label_f_font %s\n", font_name(ctx->label.face.font));
	fprintf(fp, "-label_v_clr %.3f %.3f %.3f\n", ctx->label.vertex.color.r, ctx->label.vertex.color.g, ctx->label.vertex.color.b);
	fprintf(fp, "-label_v_font %s\n", font_name(ctx->label.vertex.font));

	//- light
	//- light
	fprintf(fp, "-light %f %f %f\n", ctx->clrCtl.light.x, ctx->clrCtl.light.y, ctx->clrCtl.light.z );

	//- no_fog
	if (!ctx->drawAdj.fogFlag) fprintf(fp, "-no_fog\n");

	//- no_lighting
	if (!ctx->clrCtl.useLightingFlag) fprintf(fp, "-no_lighting\n");

	//- normalize
	if (ctx->drawAdj.normalizeFlag) fprintf(fp, "-normalize\n");

	//- orientation
	switch (ctx->geomAdj.orientation) {
	case GEOMETRY_ORIENTATION_EDGE:   p = arg_orientation[0].text; break;
	case GEOMETRY_ORIENTATION_FACE:   p = arg_orientation[1].text; break;
	case GEOMETRY_ORIENTATION_VERTEX: p = arg_orientation[2].text; break;
	}
	fprintf(fp, "-orientation %s\n", p);

	//- orthographic
	if (ctx->drawAdj.projection == GEOMETRY_PROJECTION_ORTHOGRAPHIC ) fprintf(fp, "-orthographic\n");

	//- spin
	if (ctx->drawAdj.spin.spinState)
	{
		fprintf(fp, "-spin %f %f %f %d\n", ctx->drawAdj.spin.dx, ctx->drawAdj.spin.dy, ctx->drawAdj.spin.dz, (int)ctx->drawAdj.spin.timerMSec );
	}

	//- spp
	fprintf(fp, "-spp %d\n", ctx->opengl.samplesPerPixel);

	//- stereo
	if (ctx->drawAdj.stereoFlag)
	{
		fprintf(fp, "-stereo\n");
		fprintf(fp, "-stereo_angle %f\n", ctx->drawAdj.eyeSeparation);
	}
	if (!ctx->drawAdj.stereoCrossEyeFlag) fprintf(fp, "-stereo_no_cross\n" );

	// -toolvis
	if(ctx->attrControl && ctx->objControl) fprintf(fp, "-toolvis\n");
//	if(ctx->window.toolsVisible) fprintf(fp, "-toolvis\n");

	{ // get current window information from OS
		RECT	rect;
		int		width, height;
		GetWindowRect(ctx->mainWindow, &rect);
		width = rect.right - rect.left;
		height = rect.bottom - rect.top;
		//- win_wh
		fprintf(fp, "-win_wh %d %d\n", width-WINDOW_SIZE_OFFSET_WIDTH, height- WINDOW_SIZE_OFFSET_HEIGHT);
		//- win_xy
		fprintf(fp, "-win_xy %d %d\n", rect.left, rect.top);
	}

	// capture
	// film
	// image
	fprintf(fp, "# Capture settings\n");
	fprintf(fp, "-image_basename %s\n", ctx->png.basename);
	if(!ctx->png.stateSaveFlag)	fprintf(fp, "-no_image_state_save\n");

	fprintf(fp, "# Input settings\n");
	// input
	//-in_cs
	if ( ctx->inputTrans.centerAndScaleFlag) fprintf(fp, "-in_cs\n");

	//- in_transform
	if (ctx->inputTrans.transformFlag) fprintf(fp, "-in_transform %f %f %f %f %f %f\n",
		ctx->inputTrans.zAxis.x, ctx->inputTrans.zAxis.y, ctx->inputTrans.zAxis.z, ctx->inputTrans.yAxis.x, ctx->inputTrans.yAxis.y, ctx->inputTrans.yAxis.z );

	//- in_x_mirror
	if (ctx->inputTrans.mirrorFlag) fprintf(fp, "-in_x_mirror\n");
	//- in_z_rotate
	if (ctx->inputTrans.replicateFlag) fprintf(fp, "-in_z_rotate\n");

	//- no_unique
	if (!ctx->inputTrans.guaFlag) fprintf(fp, "-no_unique\n");

	//- rot_matrix
	//- rx
	//- ry
	//- rz
	double	*d = &ctx->matrix.data.array[0];
	fprintf(fp, "-rot_matrix %f %f %f %f %f %f %f %f %f\n", d[0], d[1], d[2], d[4], d[5], d[6], d[8], d[9], d[10]);

	//- txyz
	fprintf(fp, "-txyz %f %f %f\n", ctx->trans[0], ctx->trans[1], ctx->trans[2]);

	//- udump
	if (ctx->inputTrans.guaResultsFlag) fprintf(fp, "-udump\n");
	
	fprintf(fp, "# Object values\n");
	fprintf(fp, "-clr_f_default %5.3f %5.3f %5.3f %5.3f\n", ctx->clrCtl.face.defaultColor.r, ctx->clrCtl.face.defaultColor.g, ctx->clrCtl.face.defaultColor.b, ctx->clrCtl.face.defaultColor.a);
	{
		DS_GEO_OBJECT			*obj;

		ds_save_object_state(ctx, fp, (DS_GEO_OBJECT*)&ctx->defInputObj);

		LL_SetHead(ctx->gobjectq);
		while (obj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
		{
//			gobj = (DS_GEO_INPUT_OBJECT*)obj;
			ds_save_object_state(ctx, fp, obj);
		}
	}

	fclose(fp);

	return 0;
}