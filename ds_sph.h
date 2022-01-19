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
#define PROGRAM_VERSION_MINOR			9896 
#define PROGRAM_EXE_LOCATION			"ProgramFiles(x86)" // environment variable
//#define PROGRAM_DATA_LOCATION			"USERPROFILE"		// sample data location
#define PROGRAM_DATA_LOCATION			"PUBLIC"			// sample data location
#define PROGRAM_DOCUMENTATION			"/Documents/DisplaySphere/documentation/DisplaySphere Documentation.pdf"

#define GEOMETRY_ICOSAHEDRON			1
#define GEOMETRY_OCTAHEDRON				2
#define GEOMETRY_TETRAHEDRON			3
#define GEOMETRY_CUBEHEDRON				4
#define GEOMETRY_DODECAHEDRON			5

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
#define GEOMETRY_POLYMODE_CULL			3

#define DS_COLOR_STATE_EXPLICIT			1
#define DS_COLOR_STATE_AUTOMATIC		2
#define DS_COLOR_STATE_OVERRIDE			4	

#define GEOMETRY_PROJECTION_ORTHOGRAPHIC	1
#define GEOMETRY_PROJECTION_PERSPECTIVE		0

#define MAX_NUM_INPUT_FILES				8

#define COLOR_FORMAT_ZERO_TO_ONE		1
#define COLOR_FORMAT_255				2

#define COLOR_FACE_DEFAULT_RED			(float)1.0	//222 201 100
#define COLOR_FACE_DEFAULT_GRN			(float)0.0
#define COLOR_FACE_DEFAULT_BLU			(float)0.0

// greenish - 0x90e168
#define COLOR_FACE_OVERRIDE_RED			(float)(((0x90e168>>16)&0xff)/255.0) //	144,225,104 // green
#define COLOR_FACE_OVERRIDE_GRN			(float)(((0x90e168>> 8)&0xff)/255.0)
#define COLOR_FACE_OVERRIDE_BLU			(float)(((0x90e168>> 0)&0xff)/255.0)
#define COLOR_FACE_OVERRIDE_ALP			(float)1.0

// purplish - 0xdbb1cf
#define COLOR_EDGE_OVERRIDE_RED			(float)(((0xdbb1cf>>16)&0xff)/255.0) // purple
#define COLOR_EDGE_OVERRIDE_GRN			(float)(((0xdbb1cf>> 8)&0xff)/255.0)
#define COLOR_EDGE_OVERRIDE_BLU			(float)(((0xdbb1cf>> 0)&0xff)/255.0)

// bluish - 0x99d8c8
#define COLOR_VERTEX_OVERRIDE_RED		(float)(((0x99d8c8>>16)&0xff)/255.0) // blue
#define COLOR_VERTEX_OVERRIDE_GRN		(float)(((0x99d8c8>> 8)&0xff)/255.0)
#define COLOR_VERTEX_OVERRIDE_BLU		(float)(((0x99d8c8>> 0)&0xff)/255.0)

// bluish - 0x99d8c8
#define COLOR_CUT_OVERRIDE_RED			(float)(((0x9f9f9f>>16)&0xff)/255.0) // blue
#define COLOR_CUT_OVERRIDE_GRN			(float)(((0x9f9f9f>> 8)&0xff)/255.0)
#define COLOR_CUT_OVERRIDE_BLU			(float)(((0x9f9f9f>> 0)&0xff)/255.0)

#define ORTHODROME_STYLE_RIM					0
#define ORTHODROME_STYLE_DISC_TWO_SIDED			1
#define ORTHODROME_STYLE_DISC_ONE_SIDED			2
#define ORTHODROME_STYLE_SPHERICAL_SECTION		3

#define FACE_HOLE_STYLE_ROUND					0
#define FACE_HOLE_STYLE_POLYGONAL				1
#define FACE_HOLE_STYLE_ROUND_WITH_IN_DIMPLE	2
#define FACE_HOLE_STYLE_ROUND_WITH_OUT_DIMPLE	3


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

enum {
	DS_STATE_NULL = 0,
	DS_STATE_OPEN,
};

typedef struct {
	int			mode;					// current mode
	long		offset;					// file position
	int			resetFlag;				// reset flag used for write
	char		description[1024];		// text to associated with state save
	char		filename[1024];			// filename used
	int			playbackOption;			// playback option (manual, timed)
	int			objectCount;			// track 
	FILE		*fp;					// file pointer 
	DS_FILE		base, file;
	char		clrTblfilename[512];
	int			clrTblFlag;		
//	DS_FILE		dsf;					// pointer to structure
} DS_STATE_IO;

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
	int				nZRot;				// max number of Z rotations per face (3,4,5)
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
	int			spinState;
	float		dx;
	float		dy;
	float		dz;
	float		timerMSec;
} DS_SPIN;

/*
	Face options (new)
	Extrusion 
		double sided (close back side)
		height
		direction: radial or face normal
	Offset
		factor (outward > 1.0, inward < 1.0) 
	Hole
		Polygonal or round
		radius % of max (less than 1.0)
		Increment automatic

*/

typedef struct {
	int				enable;
	float			xOffset;	// applied as a subtraction from current raster position after multiplying by number of characters
	float			yOffset;
	void			*font;
	DS_COLOR		color;
} DS_LABEL; // _ATTRIBUTES;

typedef struct {							// new face control
	int			nSegments; 					// based on the angle of division
	int			*nSeg;						// integer array of nSegs per edge 
	float		*angle;						// array of subtended angles
	GUT_POINT	centroid;					// average center of polygon
	double		radius;						// max radius of hole
	GUT_VECTOR	normal;						// outside face normal for 
	GUT_VECTOR	rnormal; 					// inside face normal (reverse)
	GUT_VECTOR	*nml; 						// [nFaceVtx] explicit vertex normals 
	GUT_VECTOR	*no; 						// [nFaceVtx] outside normals for extrusion
	GUT_VECTOR	*ni;  						// [nSegments] inside normals for extrusion 
	GUT_POINT	*vto, *vti, *vbo, *vbi; 	// [nSegments * 4]
	GUT_POINT	**vct, **vcb; 				// [nFaceVtx] each - corner top and bottom 
} DS_FACE_DRAWABLE;

typedef struct {							// new face control
	GUT_VECTOR	*n; 						// all normals
	GUT_POINT	*v;							// all vertices 
	GUT_POINT	middle;
	GUT_VECTOR	normal;
} DS_EDGE_DRAWABLE;

typedef struct {	// new face control
	int			draw;
	struct {
		int		 enable; // 
		int		 bothSides; // draw both sides of face
		int		 direction; // radial (0) or normal (1)
		int		 holeOnly;
		double	 factor;
	} extrusion;
	struct {
		int		 enable; // 
		double	 factor; // positive numbers used only
	} offset;
	struct {
		int		 enable; // 
		double	 factor;	// positive numbers used only
	} scale;
	struct {
		int		 enable; // 
		int		 style; // round = 0, polygonal = 1, round w/in dimple = 2, round w/out dimple = 3, 
		double	 radius; // max at 0.95 min at 0.1
		double	 shallowness; // range of 0.3 to 1.5 
	} hole;
	struct
	{
		int		 style;
		int		 enable;
		int		 cutColorEnable;
		DS_COLOR cutColor;
		int		 dashEnable;
		double	 radius;
		double	 depth1;
		double	 depth2;
		double	 height;
	} orthodrome; // used with degenerate faces (1 vertex) that have normals
	DS_LABEL label;
} DS_FACE_ATTRIBUTES;

typedef struct {		// Triangle specific
	int					*vtx;	// unique vertex IDs
	int					*nml;	// unique normal IDs
	int					id;		// unique ID auto generated 
	int					nVtx;	// size of vertex array
	DS_COLOR			color;	// explict color
	DS_FACE_DRAWABLE	draw;	// drawable components
} DS_FACE; 

typedef struct {		// General version 
	int					*vtx;	// unique vertex IDs
	int					*nml;	// unique normal IDs
	int					id;		// unique ID auto generated 
	int					nVtx;	// size of vertex array
	DS_COLOR			color;	// explict color
	DS_FACE_DRAWABLE	draw;	// drawable components
} DS_POLYGON;

typedef struct {
	int			vtx[3]; // unique vertex IDs
} DS_VTRIANGLE;

typedef struct {
	int					vtx[2]; // unique vertex IDs
	int					id;		// unique ID auto generated 
	DS_EDGE_DRAWABLE	draw;	// drawable components
} DS_EDGE;

typedef struct {
	int					draw,		// draw flag
						type;		// GEOMETRY_EDGE_SQUARE/ROUND
	int					arcEnable;	// straight or arc form
	double				width,		// adjustment from radius position
						height,		// fraction of 1.0
						minLength,	// minimum length of all edges
						maxLength;	// maximum length of all edges
	struct {
				int		enable; // 
				double	factor;	// positive numbers used only
	} scale;
	struct {
				int		 enable; // 
				double	 factor; // positive numbers used only
	} offset;
	DS_LABEL			label;
} DS_EDGE_ATTRIBUTES;

typedef struct {
	int					draw;
	double				scale;	// fraction of 1.0
	struct {
		int				enable; // 
		double			factor; // positive numbers used only
	} offset;
	DS_LABEL			label;
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
	int cull[2]; // 0 - front, 1 - back
//	int geometry; // icosa
//	int orientation;
//	int drawWhat;
//	int replication; // number depends on geometry
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
	double				clipZValue;
	double				clipZIncrement;
	int					clipVisibleFlag;
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

//ypedef struct {
//	float			xOffset;	// applied as a subtraction from current raster position after multiplying by number of characters
//	float			yOffset;
//	void			*font;
//	DS_COLOR		color;
// DS_LABEL;

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
	DS_COLOR			clr;
} DS_OPENGL;

typedef struct {
	int							active;
	char						*filename;		// saved to compare with updates
	int							name[32];		// user specified name
	int							drawWhat;		// what part of the object to draw F 1 E 2 V 4
	DS_FACE_ATTRIBUTES			fAttr;			// face state for rendering
	DS_EDGE_ATTRIBUTES			eAttr;			// edge state for rendering
	DS_VERTEX_ATTRIBUTES		vAttr;			// vertex scale
	DS_COLOR_ATTRIBUTES			cAttr;			// face, edge, and vertex color control
	DS_REPLICATION_ATTRIBUTES	rAttr;			// replication flags
	DS_TRANSPARENCY_ATTRIBUTES	tAttr;			// transparency info
	DS_LABEL_FLAGS				lFlags;			// label flags
	int							geo_type;		// which of the base geometry
	int							geo_orientation; // base orientation
	DS_COLOR					faceDefault;
	HWND						attrDialog; // window for attribute manipulation 
} DS_GEO_INPUT_OBJECT;

typedef struct {
//================= SAME STRUCTURE AS GEO_INPUT_OBJECT
	int							active;
	char						*filename;	// saved to compare with updates
	int							name[32];	// user specified name
	int							drawWhat;	// what part of the object to draw F 1 E 2 V 4
	DS_FACE_ATTRIBUTES			fAttr;		// face state for rendering
	DS_EDGE_ATTRIBUTES			eAttr;		// edge state for rendering
	DS_VERTEX_ATTRIBUTES		vAttr;		// vertex scale
	DS_COLOR_ATTRIBUTES			cAttr;		// face, edge, and vertex color control
	DS_REPLICATION_ATTRIBUTES	rAttr;		// replication flags
	DS_TRANSPARENCY_ATTRIBUTES	tAttr;		// transparency info
	DS_LABEL_FLAGS				lFlags;		// label flags
	int							geo_type;	// which of the base geometry
	int							geo_orientation; // base orientation
	DS_COLOR					faceDefault;
	HWND						attrDialog; // window for attribute manipulation 
											//=====================================================
	MTX_VECTOR					*v_out;		// temp space for coordinate data to be used for transformations
	MTX_VECTOR					*n_out;		// temp space for coordinate data to be used for transformations
	GUT_POINT					*vtx;		// array of unique vertex coordinates
	GUT_POINT					*nml;		// array of unique normal coordinates
	DS_FACE						*tri;		// array of unique triangles/polygons
	DS_EDGE						*edge;		// array of unique edges
	int							nVtx,		// number of unique vertices in object
								nTri,		// number of unique triangles in object by vertex sets
								nEdge,		// number of unique edges in object by vertex pairs
								nUTri,		// number of unique triangles by unique edge length sets
								nUEdge;		// number of unique edges by length
	int							*vIndex;	// array of all vertex indices for all faces/polygons
	int							*nIndex;	// array of all normal indices for all faces/polygons
	int							nmlFlag;	// indicates normals for each subface or normals per vertex
	DS_COLOR_TABLE				*ctT,		// color table for triangles 
								*ctE,		// color table for edges
								*ctB;		// color table for edges & triangles 
	int							matrixID;	// identifying ID  ( ( object# << 8 ) | ( sequence & 0xff )
	DS_FILE						*dsf;		// file structure
	char						*faceMem;	// allocated memory for face options
	char						*edgeMem;	// allocated memory for edge options
	int							faceInit;	// flag for memory allocation
	int							edgeInit;	// flag for memory allocation
	DS_GEO_INPUT_OBJECT			restore;	// used to hold info during dialog changes
} DS_GEO_OBJECT;

//typedef struct {
//	int						id;			// used to sort matrix
//	MTX_MATRIX				mtx;		// model matrix
//	MTX_MATRIX				nmtx;		// normal matrix
//} DS_MATRIX_SORT;

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
	int						useLightingFlag;
	int						reverseColorFlag;
	GUT_POINT				position;
	int						ambientEnabled;
	int						diffuseEnabled;
	int						specularEnabled;
	double					ambientPercent;
	double					diffusePercent;
	double					specularPercent;
	double					matShininess;
	double					matSpecular;
} DS_LIGHTING;

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
	char					tempBuffer[512];	// used for command line
	char					*execDir;
	char					*internalDSS;
	int						ac;					// number of arguments to program
	int						relativeObjPathFlag;
	int						dssStateFlag;
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
	DS_GEO_OBJECT			defInputObj;	// objDefault;// defaultObject;
	DS_GEO_OBJECT			curInputObj;	// defaultObject;
	DS_INPUT_TRANSFORMATION	inputTrans;		// modify vertex data when read in
	DS_COLOR_TABLE_SET		cts;
	HWND					mainWindow;
	HWND					kbdToggle;
	HWND					attrControl;
	HWND					objInfo;
	//HWND					objControl;
	HWND					objAttributes;
	HWND					objDashboard;
	HWND					stateRecorderWin;
	HWND					statePlaybackWin;
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
	DS_FILE					clrTbl;
	DS_LABELS				label;
	DS_LIGHTING				lighting;
	DS_GEO_OBJECT			*curObjAttr;
	LL						*objAttrQueue; //  queue of dialog windows
	int						curRotationAxis;
	double					curRotationAngle;
	void					*cmdLineCtx;
	DS_STATE_IO				stateRead;		// .dss read state
	DS_STATE_IO				stateWrite;		// .dss write state
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
void ds_geo_edge_to_triangles_new(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *out, GUT_VECTOR *nml, int normalize, GUT_POINT *origin);
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
LRESULT CALLBACK ds_dlg_object_attributes(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_object_dashboard(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_state_recorder(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK ds_dlg_state_playback(HWND hWndDlg, UINT Msg, WPARAM wParam, LPARAM lParam);

// GEOMETRY UTILITY FUNCTIONS SPECIFIC TO DS
int  ds_geo_plane_from_points(GUT_POINT *a, GUT_POINT *b, GUT_POINT *c, GUT_PLANE *p);
void ds_geo_build_transform_matrix(DS_CTX *ctx);

// Label & Font Functions
void ds_label_set(DS_LABEL *label, void *font, DS_COLOR *color, int enable);
void ds_label_draw(DS_LABEL *label, float x, float y, float z, float nz, char *string);
//void ds_label_draw_id(DS_LABEL *label, float x, float y, float z, float nz, int id);
void ds_label_draw_id(DS_LABEL *label, float x, float y, float z, GUT_VECTOR *normal, int id, int lighting);
void ds_label_update(DS_LABEL *label); // reset offsets after font change

void ds_face_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
void ds_edge_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj);

//#define ERR_DEBUG
//#ifdef ERR_DEBUG
//int dbg_output(DS_CTX *ctx, char *string);
//#define DEBUG(string)	if(ctx->errFile) dbg_output(ctx,string);
//#endif
int ds_gl_render_vertex(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE *vt, int nVtx, int nTri, double scale, DS_COLOR *clr, int insideOutFlag);

#endif