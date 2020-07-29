/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This module determines all the transformation matrices required for the four main polyhedron.
	Except for the cube all the matrices are automatically determined by mirroring vertices 
	around the spherical surface starting from the initial face.

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
#include <avl_new.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

typedef struct { // FACE
	int			vid[3];
	int			svid[3]; // sorted
	int			id;
	MTX_MATRIX	m_face;
} FACE;

typedef struct { // CONTEXT 
	FACE				f[21]; //max
	GUT_POINT			vdata[13]; // v[13]; // max
	GUT_POINT			*v; // v[13]; // max
	int					fq[21];
	int					nfq;
	int					nf;
	int					nv;
	int					nFcmp;
	int					nVcmp;
	int					nZRot;
	void				*favl;
	FACE				*fp; // face pointer
	MTX_MATRIX			m_mirror;
	MTX_MATRIX			m_zrot;
	MTX_MATRIX			m_edge;
	MTX_MATRIX			m_vertex;
} MATRIX_CONTEXT;

static void convert_context_2_geo_object_hi_res(MATRIX_CONTEXT *ctx, DS_GEO_OBJECT_VERTEX *g, int frequency);

//------------------------------------------------------------------------------------
static int insert_v ( MATRIX_CONTEXT *ctx, GUT_POINT *v, int *id)
//------------------------------------------------------------------------------------
{
	// This is a brute force vertex compare function that 
	// tests every vertex that is already in the set. Since
	// the total number of vertices involved is low a brute
	// force method is efficient enough.
	int  	i;
	++ctx->nVcmp;
	for (i = 0; i < ctx->nv; ++i)
	{
		if (fabs(ctx->v[i].x - v->x) > 0.00000007)
			continue;
		if (fabs(ctx->v[i].y - v->y) > 0.00000007)
			continue;
		if (fabs(ctx->v[i].z - v->z) > 0.00000007)
			continue;
		*id = i;
		return 0; // not unique 
	}
	*id = ctx->nv++;
	return 1; // unique
}

//--------------------------------------------------------------------------------
static int face_compare(MATRIX_CONTEXT *ctx, FACE *a, FACE *b)
//--------------------------------------------------------------------------------
{
	// This is face compare function. It compares the three vertex indices 
	// that define the face. The order of the vertex indices is always ordered.
	// The compare is used when performing an insert into the face AVL tree.
	// If all three vertex ID match we return 0 since the face is NOT unique.
	// When face is NOT unique we store a pointer to the matching face is in 
	// provided content structure.
	int	diff;

	++ctx->nFcmp; // statistic info

	diff = a->svid[0] - b->svid[0];
	if (diff) // unique
		return diff;
	diff = a->svid[1] - b->svid[1];
	if (diff) // unique
		return diff;
	diff = a->svid[2] - b->svid[2];
	if (diff) // unique
		return diff;

	ctx->fp = a; // save the matching node
	return 0;
}

//------------------------------------------------------------------------------------
static int insert_face ( MATRIX_CONTEXT *ctx, FACE *f, int *id )
//------------------------------------------------------------------------------------
{
	// This function is called when attempting to add a new face to 
	// face AVL tree. If the input face matches an already existing
	// face the existing face's ID is returned. Otherwise the originally
	// supplied id is unchanged.

	int		tmp;
	int		*vid;

	f->svid[0] = f->vid[0];
	f->svid[1] = f->vid[1];
	f->svid[2] = f->vid[2];
	vid = &f->svid[0];

	// sort the array of vertex IDs
	if (vid[1] < vid[0])
	{
		tmp = vid[0];
		vid[0] = vid[1];
		vid[1] = tmp;
	}
	if (vid[2] < vid[1])
	{
		tmp = vid[1];
		vid[1] = vid[2];
		vid[2] = tmp;
		if (vid[1] < vid[0])
		{
			tmp = vid[0];
			vid[0] = vid[1];
			vid[1] = tmp;
		}
	}

	if (avl_insert(ctx->favl, f)) // attempt to insert face
	{	// face was new
		*id = f->id;
		++ctx->nf;
		return 1;
	}
	else
	{
		// duplicate 
		*id = ctx->fp->id;
		return 0;
	}
}

//------------------------------------------------------------------------------------
static void init ( MATRIX_CONTEXT *ctx, GUT_POINT *v )
//------------------------------------------------------------------------------------
{
	// This is the common initialization routine for all
	// polyhedron except the cube. The only difference is 
	// the coordinates of the three supplied vertices.
	// We create the initial face and vertices, add them
	// to the face and vertex databases then proceed to 
	// process the face which will automatically branch
	// out to all the other remaining faces and vertices.
	//
	FACE	*f; //	f
	int		i, id;

	// create AVL tree for faces
	ctx->favl = avl_create(face_compare, (void*)ctx);
	ctx->nf = ctx->nv = ctx->nFcmp = ctx->nVcmp = ctx->nfq = 0;

	ctx->nZRot = 3;
	f = &ctx->f[0];
	f[0].vid[0] = 0;
	f[0].vid[1] = 1;
	f[0].vid[2] = 2;
	f[0].id = ctx->nf; 

	if (insert_face(ctx, f, &id))
	{
		ctx->fq[0] = id;
		ctx->nfq = 1;
	}
	for (i = 0; i < 3; ++i)
	{
		ctx->v[ctx->nv] = v[i];
		insert_v(ctx, &v[i], &id);
	}
}

//------------------------------------------------------------------------------------
static void fill(MATRIX_CONTEXT *ctx)
//------------------------------------------------------------------------------------
{
	// This function is called after the init() function.
	// It will continue branching off the initial face until all faces
	// and vertices have been created.
	// Once all the faces/vertices are available then the transformation
	// matrics can be derived. 
	//
	FACE 				*f;
	GUT_POINT			*v;
	GUT_POINT			o;
	int					i, vID, fID;
	GUT_PLANE			plane;

	o.x = o.y = o.z = 0;

	while (ctx->nfq) // process queue until none left
	{
		i = ctx->fq[--ctx->nfq]; // take top face from queue
		f = &ctx->f[i]; // take top face from queue

		//echo process face $f.id 
		for (i = 0; i < 3; ++i)
		{
			// create 3 mirror points
			v = &ctx->v[ctx->nv]; // use top of vertex array 

			// add to vertex data base
			gut_plane_from_points(&ctx->v[f->vid[i]], &ctx->v[f->vid[(i + 1) % 3]], &o, &plane);
			gut_mirror_point_to_plane(&plane, &ctx->v[f->vid[(i + 2) % 3]], v);

			// create mirror
			if (insert_v(ctx, v, &vID))
			{
				// if mirror point is unique then create new face and add to queue
				ctx->f[ctx->nf].vid[0] = f->vid[(i + 1) % 3];
				ctx->f[ctx->nf].vid[1] = f->vid[i];
				ctx->f[ctx->nf].vid[2] = vID;
				ctx->f[ctx->nf].id = ctx->nf;

				insert_face(ctx, &ctx->f[ctx->nf], &fID);
				ctx->fq[ctx->nfq++] = fID;
			}
			else
			{
				// vertex is not unique so need to insert possible face by vid comparison
				ctx->f[ctx->nf].vid[0] = f->vid[(i + 1) % 3];
				ctx->f[ctx->nf].vid[1] = f->vid[i];
				ctx->f[ctx->nf].vid[2] = vID;
				ctx->f[ctx->nf].id = ctx->nf;

				if (insert_face(ctx, &ctx->f[ctx->nf], &fID))
				{
					ctx->fq[ctx->nfq++] = fID; // add to work queue
				}
			}
		}
	}
}

//------------------------------------------------------------------------------------
static void matrix_edge(MATRIX_CONTEXT *ctx)
//------------------------------------------------------------------------------------
{
	// create matrix for edge orientation 
	FACE				*f;
	GUT_VECTOR			x, y, z;
	GUT_POINT			c;
	double				*m;

	f = &ctx->f[0]; // base face
	gut_mid_point(&ctx->v[f->vid[0]], &ctx->v[f->vid[1]], &c);

	gut_vector(&c, &ctx->v[f->vid[1]], &x);
	gut_normalize_vector(&x);
	z = *(GUT_VECTOR*)&c;
	gut_normalize_vector(&z);
	gut_cross_product(&z, &x, &y);

	m = (double*)&ctx->m_edge;
	// transformation matrix to global
	m[0]  = x.i;	m[1]  = y.i;	m[2]  = z.i;	m[3]  = 0;
	m[4]  = x.j;	m[5]  = y.j;	m[6]  = z.j;	m[7]  = 0;
	m[8]  = x.k;	m[9]  = y.k;	m[10] = z.k;	m[11] = 0;
	m[12] = 0;	    m[13] = 0;	    m[14] = 0;	    m[15] = 1;
}

//------------------------------------------------------------------------------------
static void matrix_vertex(MATRIX_CONTEXT *ctx)
//------------------------------------------------------------------------------------
{
	// create matrix for vertex orientation 
	FACE				*f;
	GUT_VECTOR			x, y, z;
//	GUT_POINT			c;
	double				*m;

	f = &ctx->f[0]; // base face
	z = *(GUT_VECTOR*)&ctx->v[f->vid[2]];
	gut_normalize_vector(&z);
	gut_vector(&ctx->v[f->vid[0]], &ctx->v[f->vid[1]], &x);
	gut_normalize_vector(&x);
	gut_cross_product(&z, &x, &y);
	m = (double*)&ctx->m_vertex;
	// transformation matrix to global
	m[0]  = x.i;	m[1]  = y.i;	m[2]  = z.i;	m[3]  = 0;
	m[4]  = x.j;	m[5]  = y.j;	m[6]  = z.j;	m[7]  = 0;
	m[8]  = x.k;	m[9]  = y.k;	m[10] = z.k;	m[11] = 0;
	m[12] = 0;	    m[13] = 0;	    m[14] = 0;	    m[15] = 1;
}

//------------------------------------------------------------------------------------
static void matrix_face(MATRIX_CONTEXT *ctx, FACE *f)
//------------------------------------------------------------------------------------
{
	// create matrix for each face - this is the standard base orientation
	GUT_VECTOR	x, y, z;
	GUT_POINT	c;
	double		*m;

	// FACE ORIENTATION - UNIQUE FOR EACH FACE
	gut_center_point(&ctx->v[f->vid[0]], &ctx->v[f->vid[1]], &ctx->v[f->vid[2]], &c);
	gut_vector(&c, &ctx->v[f->vid[2]], &y);
	gut_normalize_vector(&y);
	z = *(GUT_VECTOR*)&c;
	gut_normalize_vector(&z);
	gut_cross_product(&y, &z, &x);
	m = (double*)&f->m_face;

	// transformation matrix
	m[0]  = x.i;	m[1]  = y.i;	m[2]  = z.i;	m[3]  = 0;
	m[4]  = x.j;	m[5]  = y.j;	m[6]  = z.j;	m[7]  = 0;
	m[8]  = x.k;	m[9]  = y.k;	m[10] = z.k;	m[11] = 0;
	m[12] = 0;	    m[13] = 0;	    m[14] = 0;	    m[15] = 1;
	mtx_transpose_matrix((MTX_MATRIX*)&f->m_face);
}

//------------------------------------------------------------------------------------
static matrix_mirror(MATRIX_CONTEXT *ctx)
//------------------------------------------------------------------------------------
{
	// create matrix for face along edge from vertex 0 to vertex 1
	FACE				*f;
	GUT_VECTOR			x, y, z;
	GUT_POINT			c;
	double				m[16], n[16], r[16];

	f = &ctx->f[0]; // base face
	gut_mid_point(&ctx->v[f->vid[0]], &ctx->v[f->vid[1]], &c);

	gut_vector(&c, &ctx->v[f->vid[1]], &x);
	gut_normalize_vector(&x);
	z = *(GUT_VECTOR*)&c;
	gut_normalize_vector(&z);
	gut_cross_product(&z, &x, &y);

	// transformation matrix
	m[0]  = x.i;	m[1]  = y.i;	m[2]  = z.i;	m[3]  = 0;
	m[4]  = x.j;	m[5]  = y.j;	m[6]  = z.j;	m[7]  = 0;
	m[8]  = x.k;	m[9]  = y.k;	m[10] = z.k;	m[11] = 0;
	m[12] = 0;	    m[13] = 0;	    m[14] = 0;	    m[15] = 1;

	mtx_set_unity((MTX_MATRIX*)&n);
	n[0] = -1;
	mtx_multiply_matrix((MTX_MATRIX*)&m, (MTX_MATRIX*)&n, (MTX_MATRIX*)&r);
	mtx_transpose_matrix((MTX_MATRIX*)m);
	mtx_multiply_matrix((MTX_MATRIX*)&r, (MTX_MATRIX*)&m, &ctx->m_mirror);
}

//------------------------------------------------------------------------------------
static void create_cube(MATRIX_CONTEXT *ctx)
//------------------------------------------------------------------------------------
{
	// The cube is a special case.
	ctx->nf = 6;
	ctx->nZRot = 4;
	mtx_set_unity(&ctx->f[0].m_face);
	mtx_create_rotation_matrix(&ctx->f[1].m_face, MTX_ROTATE_Y_AXIS, DTR(90.0));
	mtx_create_rotation_matrix(&ctx->f[2].m_face, MTX_ROTATE_Y_AXIS, DTR(180.0));
	mtx_create_rotation_matrix(&ctx->f[3].m_face, MTX_ROTATE_Y_AXIS, DTR(-90.0));
	mtx_create_rotation_matrix(&ctx->f[4].m_face, MTX_ROTATE_X_AXIS, DTR(90.0));
	mtx_create_rotation_matrix(&ctx->f[5].m_face, MTX_ROTATE_X_AXIS, DTR(-90.0));

	mtx_create_rotation_matrix(&ctx->m_edge, MTX_ROTATE_X_AXIS, DTR(-45)); // used for mirroring 

	MTX_MATRIX	vtx[2]; 
	mtx_create_rotation_matrix(&vtx[0], MTX_ROTATE_Y_AXIS, DTR(45));
	mtx_create_rotation_matrix(&vtx[1], MTX_ROTATE_X_AXIS, asin(1/sqrt(3))); // ~35 degrees
	mtx_multiply_matrix(&vtx[0], &vtx[1], &ctx->m_vertex);

	vtx[0] = ctx->m_edge; // copy edge transformation
	mtx_set_unity(&ctx->m_mirror);
	ctx->m_mirror.data.array[0] = -1; // set the mirroring on x axis
	mtx_multiply_matrix(&vtx[0], &ctx->m_mirror, &vtx[1]); // composite edge and mirroring
	mtx_transpose_matrix(&vtx[0]); // modify edge to transform back into original position
	mtx_multiply_matrix(&vtx[0], &vtx[1], &ctx->m_mirror); // final composite mirror matrix

	mtx_create_rotation_matrix(&ctx->m_zrot, MTX_ROTATE_Z_AXIS, DTR(90.0));
}

//------------------------------------------------------------------------------------
static void create ( MATRIX_CONTEXT *ctx, GUT_POINT *v )
//------------------------------------------------------------------------------------
{
	init(ctx, v); // initialize the process
	fill(ctx); // create all the poly faces
	int		i;
	for (i = 0; i < ctx->nf; ++i) // create trasnformation matrix per face 
		matrix_face(ctx, &ctx->f[i]);

	matrix_edge(ctx); // create mirror matrix from face[0]
	matrix_vertex(ctx); // create mirror matrix from face[0]
	matrix_mirror(ctx); // create mirror matrix from face[0]
	mtx_create_rotation_matrix(&ctx->m_zrot, MTX_ROTATE_Z_AXIS, DTR(120.0));
}

//------------------------------------------------------------------------------------
static void convert_context_2_polyhedron(MATRIX_CONTEXT *ctx, DS_POLYHEDRON *poly)
//------------------------------------------------------------------------------------
{
	poly->nFaces = ctx->nf;
	poly->face = (MTX_MATRIX*)malloc(sizeof(MTX_MATRIX) * ctx->nf);
	poly->mirror = ctx->m_mirror;
	poly->edge = ctx->m_edge;
	poly->vertex = ctx->m_vertex;
	poly->zrotation = ctx->m_zrot;
	poly->nZRot = ctx->nZRot;

	int		i;
	for (i = 0; i < ctx->nf; ++i)
		poly->face[i] = *(MTX_MATRIX*)&ctx->f[i].m_face; // copy transform matrix 

	if ( ctx->favl )
		avl_destroy(ctx->favl,0); // done with tree
}

//------------------------------------------------------------------------------------
static void convert_context_2_geo_object(MATRIX_CONTEXT *ctx, DS_GEO_OBJECT_VERTEX *g)
//------------------------------------------------------------------------------------
{
	// create additional vertices (this is a {3,5+}3,0 configuration)
	int			i, j;
	int			vid[7];
	GUT_POINT	mid;
	g->nTri = ctx->nf*9;
	g->tri = (DS_VTRIANGLE*)malloc(sizeof(DS_VTRIANGLE)*g->nTri);
	for (i = 0, j=0; i < ctx->nf; ++i, j+=9)
	{
		gut_point_on_line(&ctx->v[ctx->f[i].vid[0]], &ctx->v[ctx->f[i].vid[1]], 1.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[0]))
			ctx->v[vid[0]] = mid;
		gut_point_on_line(&ctx->v[ctx->f[i].vid[0]], &ctx->v[ctx->f[i].vid[1]], 2.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[1]))
			ctx->v[vid[1]] = mid;
		gut_point_on_line(&ctx->v[ctx->f[i].vid[1]], &ctx->v[ctx->f[i].vid[2]], 1.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[2]))
			ctx->v[vid[2]] = mid;
		gut_point_on_line(&ctx->v[ctx->f[i].vid[1]], &ctx->v[ctx->f[i].vid[2]], 2.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[3]))
			ctx->v[vid[3]] = mid;
		gut_point_on_line(&ctx->v[ctx->f[i].vid[2]], &ctx->v[ctx->f[i].vid[0]], 1.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[4]))
			ctx->v[vid[4]] = mid;
		gut_point_on_line(&ctx->v[ctx->f[i].vid[2]], &ctx->v[ctx->f[i].vid[0]], 2.0 / 3.0, &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[5]))
			ctx->v[vid[5]] = mid;
		gut_center_point(&ctx->v[ctx->f[i].vid[0]], &ctx->v[ctx->f[i].vid[1]], &ctx->v[ctx->f[i].vid[2]], &mid);
		gut_normalize_point(&mid);
		if (insert_v(ctx, &mid, &vid[6]))
			ctx->v[vid[6]] = mid;

		g->tri[j + 0].vtx[0] = ctx->f[i].vid[0];	g->tri[j + 0].vtx[1] = vid[0]; g->tri[j + 0].vtx[2] = vid[5];
		g->tri[j + 1].vtx[0] = vid[0];				g->tri[j + 1].vtx[1] = vid[1]; g->tri[j + 1].vtx[2] = vid[6];
		g->tri[j + 2].vtx[0] = vid[5];				g->tri[j + 2].vtx[1] = vid[0]; g->tri[j + 2].vtx[2] = vid[6];

		g->tri[j + 3].vtx[0] = ctx->f[i].vid[1];	g->tri[j + 3].vtx[1] = vid[2]; g->tri[j + 3].vtx[2] = vid[1];
		g->tri[j + 4].vtx[0] = vid[2];				g->tri[j + 4].vtx[1] = vid[3]; g->tri[j + 4].vtx[2] = vid[6];
		g->tri[j + 5].vtx[0] = vid[1];				g->tri[j + 5].vtx[1] = vid[2]; g->tri[j + 5].vtx[2] = vid[6];

		g->tri[j + 6].vtx[0] = ctx->f[i].vid[2];	g->tri[j + 6].vtx[1] = vid[4]; g->tri[j + 6].vtx[2] = vid[3];
		g->tri[j + 7].vtx[0] = vid[4];				g->tri[j + 7].vtx[1] = vid[5]; g->tri[j + 7].vtx[2] = vid[6];
		g->tri[j + 8].vtx[0] = vid[3];				g->tri[j + 8].vtx[1] = vid[4]; g->tri[j + 8].vtx[2] = vid[6];

	}
	g->nVtx = ctx->nv;
	g->vtx = (GUT_POINT*)malloc(sizeof(GUT_POINT)*g->nVtx * 2);
	g->v_out = (MTX_VECTOR*)(g->vtx + ctx->nv);
	for (i = 0; i < ctx->nv; ++i)
	{
		g->vtx[i].x = ctx->v[i].x;
		g->vtx[i].y = ctx->v[i].y;
		g->vtx[i].z = ctx->v[i].z;
		g->vtx[i].w = 1;
	}
}

//------------------------------------------------------------------------------------
int ds_base_geometry_create ( DS_CTX *context )
//------------------------------------------------------------------------------------
{
	GUT_POINT			v[3];
	MATRIX_CONTEXT		ctx[4];
	DS_BASE_GEOMETRY	*base = &context->base_geometry;

	// initialize memory pointers
	ctx[0].v = &ctx[0].vdata[0];
	ctx[1].v = &ctx[1].vdata[0];
	ctx[2].v = &ctx[2].vdata[0];
	ctx[3].v = &ctx[3].vdata[0];

	// ICOSAHEDRON
	v[0].x = -0.525731112119133;  v[0].y = -0.303530999103343;  v[0].z = 0.794654472291766;	 v[0].w = 1;
	v[1].x =  0.525731112119133;  v[1].y = -0.303530999103343;  v[1].z = 0.794654472291766;	 v[1].w = 1;
	v[2].x =  0.000000000000000;  v[2].y =  0.607061998206686;  v[2].z = 0.794654472291766;	 v[2].w = 1;
	create(&ctx[0], &v[0]);
	// OCTAHEDRON
	v[0].x = -0.707106781186547;  v[0].y = -0.408248290463863;  v[0].z = 0.577350269189626;	 v[0].w = 1;
	v[1].x =  0.707106781186547;  v[1].y = -0.408248290463863;  v[1].z = 0.577350269189626;	 v[1].w = 1;
	v[2].x =  0.000000000000000;  v[2].y =  0.816496580927726;  v[2].z = 0.577350269189626;	 v[2].w = 1;
	create(&ctx[1], &v[0]);
	// TETRAHEDRON
	v[0].x = -0.816496580927726;  v[0].y = -0.471404520791032;  v[0].z = 0.333333333333333;	 v[0].w = 1;
	v[1].x =  0.816496580927726;  v[1].y = -0.471404520791032;  v[1].z = 0.333333333333333;	 v[1].w = 1;
	v[2].x =  0.000000000000000;  v[2].y =  0.942809041582063;  v[2].z = 0.333333333333333;	 v[2].w = 1;
	create(&ctx[2], &v[0]);

	ctx[3].favl = 0; // not needed
	create_cube(&ctx[3]);

	base->poly = (DS_POLYHEDRON*)malloc(sizeof(DS_POLYHEDRON) * 4);

	convert_context_2_polyhedron(&ctx[0], &base->poly[0]);
	convert_context_2_polyhedron(&ctx[1], &base->poly[1]);
	convert_context_2_polyhedron(&ctx[2], &base->poly[2]);
	convert_context_2_polyhedron(&ctx[3], &base->poly[3]);
	base->stack = mtx_create_stack(10, MTX_PRE_MULTIPLY);
	// create copy of icosahedron to be used for vertex rendering
	convert_context_2_geo_object_hi_res(&ctx[0], &context->renderVertex.vtxObjLoRes, context->drawAdj.loRes.sphereFrequency);
	convert_context_2_geo_object_hi_res(&ctx[0], &context->renderVertex.vtxObjHiRes, context->drawAdj.hiRes.sphereFrequency);

	return 0;
}

//------------------------------------------------------------------------------------
void ds_geometry_draw_init(DS_CTX *ctx, DS_GEO_OBJECT *gobj) //, BASE_GEOMETRY_NEW *base)
//------------------------------------------------------------------------------------
{
	DS_BASE_GEOMETRY	*base = &ctx->base_geometry;

	switch ( base->type ) {
	case GEOMETRY_ICOSAHEDRON://			1
		base->curPoly = &base->poly[0]; break;
	case GEOMETRY_OCTAHEDRON://			1
		base->curPoly = &base->poly[1]; break;
	case GEOMETRY_TETRAHEDRON://			1
		base->curPoly = &base->poly[2]; break;
	case GEOMETRY_CUBEHEDRON://			1
		base->curPoly = &base->poly[3]; break; // NOT CORRECT
	default:
		base->curPoly = &base->poly[0];
	}

	base->nZRot = base->curPoly->nZRot; // = ctx->nZ3; // BYPASS TEST
	base->maxNTransforms = (gobj->rAttr.zRotationFlag ? base->nZRot : 1) * (gobj->rAttr.oneFaceFlag ? 1 : base->curPoly->nFaces) * (gobj->rAttr.xMirrorFlag ? 2 : 1);
	base->faceRoll = (gobj->rAttr.zRotationFlag ? base->nZRot : 1) * (gobj->rAttr.xMirrorFlag ? 2 : 1);
	base->zRoll = 0;
	base->curFaceIndex = 0;
	base->curTransformIndex = 0; // reset
}

//------------------------------------------------------------------------------------
int ds_geometry_next_draw_transform(DS_CTX *ctx, DS_GEO_OBJECT *gobj, MTX_MATRIX **m, int *reverseOrder, int orientation )
//------------------------------------------------------------------------------------
{
	DS_BASE_GEOMETRY	*base = &ctx->base_geometry;
//	DS_POLYHEDRON		*poly;

	// check if we have reached max number 
	if (base->curTransformIndex == base->maxNTransforms)
		return 0; // finished

	// reset the stack
	mtx_stack_reset(base->stack);
	*reverseOrder = 0; //default

	// check for mirroring flag
	if (gobj->rAttr.xMirrorFlag) //(base->mirrorFlag)
	{
		if (base->mirrorRoll % 2)
		{
			mtx_push_matrix(base->stack, &base->curPoly->mirror);
			*reverseOrder = 1;
		}
		++base->mirrorRoll;
	}

	if (gobj->rAttr.zRotationFlag) // (base->zRotFlag) // needs to happen multiple times 
	{
		int		i;
		base->zRoll %= base->nZRot;

		for (i = 0; i < base->zRoll; ++i)
			mtx_push_matrix(base->stack, &base->curPoly->zrotation); // &base->zRotationMatrix);

		if (!gobj->rAttr.xMirrorFlag) //(!base->mirrorFlag)
			++base->zRoll;
		else if (gobj->rAttr.xMirrorFlag && !(base->mirrorRoll % 2)) //(base->mirrorFlag && !(base->mirrorRoll % 2))
			++base->zRoll;
	}

	// push the face matrix
	mtx_push_matrix(base->stack, &base->curPoly->face[base->curFaceIndex]);

	// re-orient if needed
	if ( orientation == GEOMETRY_ORIENTATION_EDGE) //edge
		mtx_push_matrix(base->stack, &base->curPoly->edge);
	else if ( orientation == GEOMETRY_ORIENTATION_VERTEX) //vertex
		mtx_push_matrix(base->stack, &base->curPoly->vertex);

	*m = mtx_get_stack_top(base->stack);

	++base->curTransformIndex;
	if ( !( base->curTransformIndex % base->faceRoll ) )
		++base->curFaceIndex;

	return 1;
}

//------------------------------------------------------------------------------------
static void convert_context_2_geo_object_hi_res(MATRIX_CONTEXT *ctx, DS_GEO_OBJECT_VERTEX *g, int frequency)
//------------------------------------------------------------------------------------
{
	// create additional vertices (this is a {3,5+}3,0 configuration)
	int				i, j, k, n, m, o, oo, T;
	int				ii, kk, maxID, maxTID;
	double			t;
	GUT_POINT		p[3];
	int				*vid; 
	int				nFacePerTri;
	int				nVtxPerFace;
	MATRIX_CONTEXT	v;  // requires additional memory to be allocated

	v.v = &v.vdata[0];
	nFacePerTri = frequency * frequency;
	nVtxPerFace = (nFacePerTri + 3 * frequency + 2) / 2;
	vid = (int*)malloc(sizeof(int)*nVtxPerFace); // temp space

	// total metrics
	T = frequency * frequency;	// class I
	g->nTri = T * ctx->nf;
	g->tri = (DS_VTRIANGLE*)malloc(sizeof(DS_VTRIANGLE)*g->nTri);
	g->nVtx = T * 10 + 2;
	g->vtx = (GUT_POINT*)malloc(sizeof(GUT_POINT)*g->nVtx * 2);
	g->v_out = (MTX_VECTOR*)(g->vtx + g->nVtx);
	v.v = (GUT_POINT*)g->vtx; // (VERTEX*)malloc(sizeof(VERTEX)*g->nVtx); // temp memory
	v.nv = 0; 
	v.nVcmp = 0;

	for (ii = 0, kk=0, maxID=0, maxTID=0; ii < ctx->nf; ++ii)
	{
		// create all the new vertices required
		for (i = 0, m = n = frequency, k = 0; i <= n; ++i, --m)
		{
			t = (i * 1.0) / n;
			gut_point_on_line(&ctx->v[ctx->f[ii].vid[0]], &ctx->v[ctx->f[ii].vid[2]], t, &p[0]); // end point
			gut_point_on_line(&ctx->v[ctx->f[ii].vid[1]], &ctx->v[ctx->f[ii].vid[2]], t, &p[1]); // end point
			for (j = 0; j <= m; ++j, ++k)
			{
				if (m)
					t = (j * 1.0) / m;
				else
					t = 0.0;
				gut_point_on_line(&p[0], &p[1], t, &p[2]); // new point on line
				p[2].w = 1;
				gut_normalize_point(&p[2]);
				if (insert_v(&v, &p[2], &vid[k]))
				{
					v.v[vid[k]] = p[2]; // need to copy new vertex data
				}
			}
		}

		n = frequency;
		m = frequency;
		o = 0;
		oo = o + m + 1;
//		k = 0;
		for (i = 0; i <= n; ++i)
		{
			for (j = 0; j < m; ++j)
			{
				g->tri[kk].vtx[0] = vid[o];
				g->tri[kk].vtx[1] = vid[o + 1];
				g->tri[kk].vtx[2] = vid[oo];
				++kk;

				if (j < m - 1)
				{
					g->tri[kk].vtx[0] = vid[o + 1];
					g->tri[kk].vtx[1] = vid[oo + 1];
					g->tri[kk].vtx[2] = vid[oo];
					++kk;
				}
				++o;
				++oo;
			}
			++o;
			oo = o + m;
			--m;
		}
	}

	free(vid);
}