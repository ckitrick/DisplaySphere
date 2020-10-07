/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifndef DISP_SPH_HEADER
#define DISP_SPH_HEADER

#define PROGRAM_NAME					"DisplaySphere" 
#define PROGRAM_VERSION_MAJOR			0	 
#define PROGRAM_VERSION_MINOR			985 
#define PROGRAM_EXE_LOCATION			"ProgramFiles(x86)" // environment variable
//#define PROGRAM_DATA_LOCATION			"USERPROFILE"		// sample data location
#define PROGRAM_DATA_LOCATION			"PUBLIC"			// sample data location
#define PROGRAM_DOCUMENTATION			"/Documents/DisplaySphere/documentation/DisplaySphere Documentation.pdf"

#define GEOMETRY_ICOSAHEDRON			1
#define GEOMETRY_OCTAHEDRON				2
#define GEOMETRY_TETRAHEDRON			3
#define GEOMETRY_CUBEHEDRON				4

#define GEOMETRY_ORIENTATION_FACE		2
#define GEOMETRY_ORIENTATION_EDGE		1
#define GEOMETRY_ORIENTATION_VERTEX		0

#define WINDOW_SIZE_OFFSET_WIDTH		16	// add these numbers to window size to ensure canvas is the requested size
#define WINDOW_SIZE_OFFSET_HEIGHT		59

#define GEOMETRY_DRAW_NONE				0
#define GEOMETRY_DRAW_FACES				1
#define GEOMETRY_DRAW_TRIANGLES			1
#define GEOMETRY_DRAW_EDGES				2
#define GEOMETRY_DRAW_VERTICES			4
#define GEOMETRY_DRAW_ALL				7
#define GEOMETRY_DRAW_BOTH				7

#define GEOMETRY_EDGE_SQUARE			2
#define GEOMETRY_EDGE_ROUND				1

#define GEOMETRY_REPLICATE_ONCE			1
#define GEOMETRY_REPLICATE_MULTIPLE		0

#define GEOMETRY_POLYMODE_FILL			0
#define GEOMETRY_POLYMODE_LINE			1
#define GEOMETRY_POLYMODE_POINT			2

#define DS_COLOR_STATE_EXPLICIT			1
#define DS_COLOR_STATE_AUTOMATIC		2
#define DS_COLOR_STATE_OVERRIDE			4	

#define GEOMETRY_PROJECTION_ORTHOGRAPHIC	1
#define GEOMETRY_PROJECTION_PERPSECTIVE		0

#define MAX_NUM_INPUT_FILES				8

#define COLOR_FORMAT_ZERO_TO_ONE		1
#define COLOR_FORMAT_255				2

#include <geoutil.h>
#include "ds_color.h"

typedef struct {
	char	*exeLocation;
	char	*dataLocation;
	int		firstTime;
} DS_PROGRAM;

typedef struct {
	int		major;
	int		minor;
	char	*text;
} DS_VERSION;

typedef struct {
	char		*userName;		// filename supplied by user
	char		*fullName;		// full path of filename
	char		*splitName;		// copy of full path
	char		*nameOnly;
	int			*word;			// array of indices into splitname 
	int			count;			// size of array
	FILE		*fp;			// file pointer
} DS_FILE;

enum { 
    PAN = 1,			/* pan state bit */
    ROTATE,				/* rotate state bits */
	ROTATE_Z=4,
    ZOOM				/* zoom state bit */
};

typedef struct {
	int		count;
	char	text[5][64];
} DS_ERROR;

typedef struct {
		int			start_x,
					start_y,
					width,
					height;
		int			toolsVisible;
} DS_WINDOW_DATA;

typedef struct {
	DS_COLOR	color;
	GUT_POINT	vertex[3];
} DS_SPHERE_DATA;

typedef struct {
	int		axis;
	double	angle;
} DS_ROT_PAIR;

typedef struct {
	int			n_rotations;
	DS_ROT_PAIR	*pair;
	MTX_MATRIX	matrix;
	int			initialized;
	int			orientation;
} DS_ROT_SET;

typedef struct {
	int				nFaces;
	MTX_MATRIX		*face; // transformation matrix - unique to each face
	MTX_MATRIX		mirror;	// mirror matrix - unique to each polyhedron
	MTX_MATRIX		edge;
	MTX_MATRIX		vertex;
	MTX_MATRIX		zrotation;
	int				nZRot;
} DS_POLYHEDRON;

typedef struct {
	int				type;
	int				mirrorFlag;			// apply mirroring flag
	int				mirrorRoll;			// current mirror status
	int				zRotFlag;			// apply z rotation flag
	int				zRoll;				// used to control the number of z rotations to apply
	int				faceRoll;			// used to control how often to change the curFaceIndex
	int				nZRot;				// max number of Z rotations per face (3 or 4)
	int				oneFaceFlag;		// only transform one polyhedron face
	int				curFaceIndex;		// current face index
	int				curPolyIndex;		// current polyhedron index (0,1,2,3)
	int				curTransformIndex;	// current transform
	int				maxFaceIndex;		// max number of faces to transform
	int				maxNTransforms;		// total number of transformations to apply
	DS_POLYHEDRON	*poly;
	DS_POLYHEDRON	*curPoly;
	MTX_STACK		*stack;
} DS_BASE_GEOMETRY;

typedef struct {
	GUT_POINT	vertex[3];
	DS_COLOR		color;
} DS_VTX_CLR;

typedef struct {
	char		basename[256];
	int			enabled,
				singleFlag,
				nFrames,
				curFrame,
				stateSaveFlag,
				bwFlag;
} DS_CAPTURE;

typedef struct {
	float		dx;
	float		dy;
	float		dz;
	float		timerMSec;
	int			spinState;
} DS_SPIN;

typedef struct {	// Triangle specific
	int			*vtx;	// unique vertex IDs
	int			id;		// unique ID auto generated 
	int			nVtx;	// size of vertex array
	DS_COLOR	color;	// explict color
} DS_FACE; 

typedef struct {	// General version 
	int			*vtx;	// unique vertex IDs
	int			id;		// unique ID auto generated 
	int			nVtx;	// size of vertex array
	DS_COLOR	color;	// explict color
} DS_POLYGON;

typedef struct {
	int			vtx[3]; // unique vertex IDs
} DS_VTRIANGLE;

typedef struct {
	int			vtx[2]; // unique vertex IDs
	int			id;		// unique ID auto generated 
} DS_EDGE;

typedef struct {
	int			type;		// GEOMETRY_EDGE_SQUARE/ROUND
	double		width,		// adjustment from radius position
				height,		// fraction of 1.0
				offset,		// fraction of 1.0
				minLength,	// minimum length of all edges
				maxLength;	// maximum length of all edges
} DS_EDGE_ATTRIBUTES;

typedef struct {
	double				scale;
} DS_VERTEX_ATTRIBUTES;

typedef struct {
	int				oneFaceFlag;
	int				zRotationFlag;
	int				xMirrorFlag;
} DS_REPLICATION_ATTRIBUTES;

typedef struct {
	int					state;
	DS_COLOR			color;
} DS_COLOR_STATE;

typedef struct {
	DS_COLOR_STATE		face;
	DS_COLOR_STATE		edge;
	DS_COLOR_STATE		vertex;
} DS_COLOR_ATTRIBUTES;

typedef struct {
	int					state;		// explicit/default(0.5) or override 
	float				alpha;		// override alpha value
	int					onFlag;		// use 
} DS_TRANSPARENCY_ATTRIBUTES;

typedef struct {
	int	polymode[2]; //0 - front, 1 - back
	int geometry; // icosa
	int orientation;
	int drawWhat;
	int replication; // number depends on geometry
} DS_GEOMETRY_ADJUSTMENTS;

typedef struct {
	int		sphereFrequency;
	int		edgeNSeg;
} DS_RENDER_QUALITY;

typedef struct {
	int					projection;
	int					stereoFlag;
	int					stereoCrossEyeFlag;
	int					normalizeFlag;
	int					circleFlag;
	int					fogFlag;
	int					axiiFlag;
	int					axiiLabelFlag;
	int					clipFlag;
	double				clipZIncrement;
	int					clipVisibleFlag;
	double				clipZValue;
	float				eyeSeparation;
	DS_SPIN				spin;
	int					spinFlag;
	double				matrix[16];
	int					hiResFlag;
	DS_RENDER_QUALITY	loRes;
	DS_RENDER_QUALITY	hiRes;
	DS_RENDER_QUALITY	*quality;
} DS_DRAWING_ADJUSTMENTS;

typedef struct {
	int			guaFlag,
				guaResultsFlag;
	GUT_POINT	zAxis, yAxis, xAxis; // x axis will be determined
	int			transformFlag,
				replicateFlag,
				mirrorFlag;
	int			centerAndScaleFlag;
	MTX_MATRIX	matrix[2]; // 0 - rotation for ZY, 1 - rotation for replication (120 degrees) 
} DS_INPUT_TRANSFORMATION;

typedef struct {
	int			flag;			// override flag
	DS_COLOR	override,		// geometry forced to this color if override flag set
				defaultColor;	// assigned to objects without explicit color
} DS_COLOR_SET;

typedef struct {
	int				updateFlag;
	int				autoColor; // only applies to triangles
	int				forceWhat; // used at init with command line
	DS_COLOR_SET	line,
					face;// triangle;
	DS_COLOR		bkgClear;
	int				useLightingFlag;
	int				reverseColorFlag;
	GUT_POINT		light;
	char			user_color_table[128];
} DS_COLOR_CONTROL;

typedef struct {
	float			xOffset;	// applied as a subtraction from current raster position after multiplying by number of characters
	float			yOffset;
	void			*font;
	DS_COLOR		color;
} DS_LABEL;

typedef struct {
	int				face;	// controls drawing
	int				edge;	// controls drawing
	int				vertex;	// controls drawing
} DS_LABEL_FLAGS;

typedef struct {
	DS_LABEL		face;
	DS_LABEL		edge;
	DS_LABEL		vertex;
	DS_LABEL		axii;
} DS_LABELS;

typedef struct {
	MTX_VECTOR			*v_out;		// temp space for coordinate data to be used for transformations
	GUT_POINT			*vtx;		// array of unique vertex coordinates
	DS_VTRIANGLE		*tri;		// array of unique triangles
	int					nVtx,		// number of unique vertices in object
						nTri;		// number of unique triangles in object by vertex sets
} DS_GEO_OBJECT_VERTEX;

typedef struct {
	double					scale;
	DS_COLOR				clr;
	DS_GEO_OBJECT_VERTEX	vtxObjLoRes;
	DS_GEO_OBJECT_VERTEX	vtxObjHiRes;
	DS_GEO_OBJECT_VERTEX	*vtxObj;
} DS_RENDER_VERTEX;

typedef struct {
	int					samplesPerPixel;
	DS_COLOR				clr;
} DS_OPENGL;

typedef struct {
	int							active;
	char						*filename;	// saved to compare with updates
	int							drawWhat;	// what part of the object to draw F 1 E 2 V 4
	DS_EDGE_ATTRIBUTES			eAttr;		// edge state for rendering
	DS_VERTEX_ATTRIBUTES		vAttr;		// vertex scale
	DS_COLOR_ATTRIBUTES			cAttr;		// face, edge, and vertex color control
	DS_REPLICATION_ATTRIBUTES	rAttr;		// replication flags
	DS_TRANSPARENCY_ATTRIBUTES	tAttr;		// transparency info
	DS_LABEL_FLAGS				lFlags;		// label flags
	DS_COLOR					faceDefault;
} DS_GEO_INPUT_OBJECT;

typedef struct {
//================= SAME STRUCTURE AS GEO_INPUT_OBJECT
	int							active;
	char						*filename;	// saved to compare with updates
	int							drawWhat;	// what part of the object to draw F 1 E 2 V 4
	DS_EDGE_ATTRIBUTES			eAttr;		// edge state for rendering
	DS_VERTEX_ATTRIBUTES		vAttr;		// vertex scale
	DS_COLOR_ATTRIBUTES			cAttr;		// face, edge, and vertex color control
	DS_REPLICATION_ATTRIBUTES	rAttr;		// replication flags
	DS_TRANSPARENCY_ATTRIBUTES	tAttr;		// transparency info
	DS_LABEL_FLAGS				lFlags;		// label flags
											//=====================================================
	MTX_VECTOR					*v_out;		// temp space for coordinate data to be used for transformations
	GUT_POINT					*vtx;		// array of unique vertex coordinates
	DS_FACE						*tri;		// array of unique triangles/polygons
	DS_EDGE						*edge;		// array of unique edges
	int							nVtx,		// number of unique vertices in object
								nTri,		// number of unique triangles in object by vertex sets
								nEdge,		// number of unique edges in object by vertex pairs
								nUTri,		// number of unique triangles by unique edge length sets
								nUEdge;		// number of unique edges by length
	int							*vIndex;	// array of all vertex indices for all faces/polygons
	DS_COLOR_TABLE				*ctT,		// color table for triangles 
								*ctE,		// color table for edges
								*ctB;		// color table for edges & triangles 
	int							matrixID;	// identifying ID  ( ( object# << 8 ) | ( sequence & 0xff )
	DS_FILE						*dsf;		// file structure
} DS_GEO_OBJECT;

typedef struct {
	int						id;			// used to sort matrix
	MTX_MATRIX				mtx;		// model matrix
} DS_MATRIX_SORT;

typedef struct {
	GUT_POINT				centroid;	// used to sort by z value
	DS_GEO_OBJECT			*object;	// associated object for attributes
	DS_FACE					*face;		// specific face
	DS_MATRIX_SORT			*ms;		// model sort matrix
	int						reverseOrder;
	LL						*queue;		// potential queue of faces with the same centroid
	float					alpha;
} DS_FACE_SORT;

typedef struct {
	void					*mtx_set;
	void					*fs_set;
	void					*avlZSort;		// sort face tree
	void					*avlMtxSort;	// sort matrix tree
	DS_FACE_SORT			*fs_match;
	DS_MATRIX_SORT			*mtx_match;
	MTX_MATRIX				*curMatrix;
	DS_GEO_OBJECT			*curGeoObject;
} DS_TRANSPARENCY;

typedef struct {
	DS_PROGRAM				program;
	DS_VERSION				version;
	DS_WINDOW_DATA			window;
	char					filename[512];		// used for color table
	char					curWorkingDir[512];	// used for capture
	char					curFileDir[512];	// used for opening and saving files
//	char					cwdAfter[512];
	char					currentDir[512];	// -cd
	char					captureDir[512];	// -capd
	char					*execDir;
	char					*internalDSS;
	int						ac;					// number of arguments to program
	int						relativeObjPathFlag;
	int						matrixFlag;
	DS_RENDER_VERTEX		renderVertex;
	int						gobjAddFlag;		// allow objects to be added rather than be replaced
	DS_BASE_GEOMETRY		base_geometry;
	LL						*gobjectq;
	LL						*inputObjq;
	float					trans[3];			// current translation 
	float					rot[3];				// current rotation 
	float					init_rot[3];
	GUT_POINT				origin;				// post transform origin
	MTX_MATRIX				matrix;
	DS_COLOR_CONTROL		clrCtl;
	DS_DRAWING_ADJUSTMENTS	drawAdj;
	DS_GEOMETRY_ADJUSTMENTS	geomAdj;
	DS_EDGE_ATTRIBUTES		eAttr;			// global edge attributes for display
	DS_CAPTURE				png;		
	DS_GEO_INPUT_OBJECT		defInputObj;	// objDefault;// defaultObject;
	DS_GEO_INPUT_OBJECT		*curInputObj;	// defaultObject;
	DS_INPUT_TRANSFORMATION	inputTrans;		// modify vertex data when read in
	DS_COLOR_TABLE_SET		cts;
	HWND					mainWindow;
	HWND					kbdToggle;
	HWND					attrControl;
	HWND					objInfo;
	HWND					objControl;
	HDC						hDC;			// device context
	HPALETTE				hPalette;		// custom palette (if needed) 
	HINSTANCE				hInstance;			
	DS_OPENGL				opengl;
	HANDLE					handle_out;
	HANDLE					handle_in;
	FILE					*errFile;
	DS_ERROR				errorInfo;
	DS_TRANSPARENCY			transparency;	// transparency context
	DS_FILE					executable;
	DS_FILE					pgmData;
	DS_FILE					curDir;
	DS_FILE					capDir;
	DS_LABELS				label;
} DS_CTX;


// FUNCTION DEFINITIONS
void ds_exit(DS_CTX *ctx);
void ds_start_up_initialization(DS_CTX *ctx);
void ds_pre_init(DS_CTX *ctx);
void ds_post_init(DS_CTX *ctx);
void ds_pre_init2(DS_CTX *ctx);
void ds_post_init2(DS_CTX *ctx);
void ds_draw_geometry(DS_CTX *ctx);
void ds_set_render_quality(DS_CTX *ctx);
int  ds_save_state(DS_CTX *ctx, char *filename);
int  ds_restore_state(DS_CTX *ctx, char *filename, DS_ERROR *errInfo);
void ds_position_window(DS_CTX *ctx, HWND hWnd, int flag, RECT *rect);
int  ds_general_color_dialog(HWND hOwnerWnd, DS_CTX *ctx, DS_COLOR *clr);
int  ds_command_line(DS_CTX *ctx, int ac, char **av, int *error, int *argIndex, DS_ERROR *errInfo);
DS_GEO_OBJECT *ds_parse_file(DS_CTX *ctx, DS_FILE *dsf);
HWND ds_create_opengl_window(DS_CTX *ctx, char* title, int x, int y, int width, int height);
int  ds_capture_image(HWND hWnd);
void ds_reshape(HWND hWnd, int width, int height);
void ds_geometry_draw_init(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
int ds_geometry_next_draw_transform(DS_CTX *ctx, DS_GEO_OBJECT *gobj, MTX_MATRIX **m, int *reverseOrder, int orientation);
void ds_geo_edge_to_triangles_hex(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *pa, GUT_POINT *pb, GUT_VECTOR *n, int normalize, GUT_POINT *origin, int nSeg);
void draw_triangle_hex(DS_CTX *ctx, GUT_POINT *p, GUT_VECTOR *n, DS_COLOR *clr, int i, int j, int k, int nSeg);
void ds_geo_edge_to_triangles(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *out, int normalize, GUT_POINT *origin);
void ds_geo_edge_to_triangles_hex_axii(DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *p, GUT_VECTOR *x, GUT_VECTOR *y, GUT_VECTOR *z, GUT_VECTOR *n, int normalize, GUT_POINT *origin);
int ds_base_geometry_create(DS_CTX *context);
int ds_file_set_window_text(HWND window, char *p);
void ds_clr_default_color_tables(DS_CTX *ctx); // builds 4 tables (8,16,32,64)
//void ds_display(DS_CTX *ctx);
void ds_display_control(DS_CTX *ctx);
int ds_process_restore_file(DS_CTX *ctx, char *filename);
int ds_transparent_face_compare(void *passThru, void *av, void *bv);
int ds_transparent_matrix_compare(void *passThru, void *av, void *bv);
void ds_edit_text_update(HWND pWnd, HWND dlg, int control, char *buffer, void *vPtr, int doubleFlag, int nDigits, int clamp, double min, double max);
DS_FILE *ds_build_dsf(DS_FILE *dsf, char *buffer, int pathFlag);
int ds_filename_env_var(char *original, char *modified);
int ds_dlg_splash_screen(DS_CTX *ctx);

LRESULT CALLBACK ds_dlg_kbd_toggles (HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_about (HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_attributes (HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_object_information (HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_object_control(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

// GEOMETRY UTILITY FUNCTIONS SPECIFIC TO DS
int  ds_geo_plane_from_points(GUT_POINT *a, GUT_POINT *b, GUT_POINT *c, GUT_PLANE *p);
void ds_geo_build_transform_matrix(DS_CTX *ctx);

// Label & Font Functions
void ds_label_set(DS_LABEL *label, void *font, DS_COLOR *color);
void ds_label_draw(DS_LABEL *label, float x, float y, float z, float nz, char *string);
void ds_label_draw_id(DS_LABEL *label, float x, float y, float z, float nz, int id);
void ds_label_update(DS_LABEL *label); // reset offsets after font change

//#define ERR_DEBUG
//#ifdef ERR_DEBUG
//int dbg_output(DS_CTX *ctx, char *string);
//#define DEBUG(string)	if(ctx->errFile) dbg_output(ctx,string);
//#endif

#endif