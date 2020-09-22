/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle rendering operations  
*/
#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glut.h>		/* OpenGL header file */
#include <GL/glext.h>		/* OpenGL header file */
#include <GL/wglext.h>
#include <stdio.h>
#include <math.h>
#include <commdlg.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <mem.h>
#include <link.h>
#include <avl_new.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

void ds_display(DS_CTX *ctx, int swap, int clear);
int ds_gl_render_axii(DS_CTX *ctx, GUT_POINT *p, GUT_POINT *out, GUT_POINT *origin);
//int ds_gl_render_vertex(DS_CTX *ctx, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE	*vt, int nVtx, int nTri, double scale, DS_COLOR *clr);
int ds_gl_render_edge(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *va, GUT_POINT *vb, GUT_POINT *p, GUT_POINT *origin, GUT_POINT *out, GUT_VECTOR *normal, DS_COLOR *clr, int nSeg);
int ds_gl_render_vertex(DS_CTX *ctx, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE	*vt, int nVtx, int nTri, double scale, DS_COLOR *clr, GUT_POINT *origin);

void ds_draw_geometry_transparency(DS_CTX *ctx);
void textout(float offset, float x, float y, float z, char *string);
void label(float x, float y, float z, float zn, char *string);


//-----------------------------------------------------------------------------
void ds_normalize_point(GUT_POINT *origin, GUT_POINT *p)
//-----------------------------------------------------------------------------
{
	// normalize a point in relation to a known origin (not required to be 0,0,0)
	double	dx = (p->x - origin->x), dy = (p->y - origin->y), dz = (p->z - origin->z);
	double	d = sqrt(dx*dx + dy*dy + dz*dz);
	p->x = origin->x + dx / d;
	p->y = origin->y + dy / d;
	p->z = origin->z + dz / d;
}

//-----------------------------------------------------------------------------
void ds_reshape_stereo(DS_CTX *ctx, int eye)
//-----------------------------------------------------------------------------
{
	float	ar;
//	float	zoom;
	int		xs, ys, width, height;

	width = ctx->window.width / 2;
	height = ctx->window.height;
	if (!eye)
	{
		xs = ys = 0;
	}
	else
	{
		xs = width;
		ys = 0;
	}
	ar = (float)(width) / (float)height;

	glViewport(xs, 0, width, height);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	if (ctx->drawAdj.projection == GEOMETRY_PROJECTION_ORTHOGRAPHIC)
	{
		double	size = 1.045;
		double  zoom;
		//        left,  right,  bottom, top, nearVal, farVal
		zoom = -ctx->trans[2] * 0.25 + 1;
		glOrtho(-size*ar*zoom, size*ar*zoom, -size*zoom, size*zoom, -100.00f, 100.0f);
	}
	else if (ctx->drawAdj.projection == GEOMETRY_PROJECTION_PERPSECTIVE)
	{
		float	f,
			zn,
			zf;
		GLfloat	m[16];

		// projective
		f = (float)(1.0 / tan(DTR(15.0)));
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
		m[10] = (zf + zn) / (zn - zf);
		m[11] = -1.0f;

		m[12] = 0.0f;
		m[13] = 0.0f;
		m[14] = (2.0f * zf * zn) / (zn - zf);
		m[15] = 0.0f; // 0.0f;

		glLoadMatrixf(m);
	}
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glTranslatef(0.f, 0.f, -4.0f);
	{
		MTX_MATRIX	mry, mm;
		double		angle = ctx->drawAdj.stereoCrossEyeFlag ? ctx->drawAdj.eyeSeparation : -ctx->drawAdj.eyeSeparation;
		double		rotation = !eye ? -angle : angle;

		mtx_create_rotation_matrix(&mry, MTX_ROTATE_Y_AXIS, DTR(rotation));
		mtx_multiply_matrix(&ctx->matrix, &mry, &mm);
		ctx->matrix = mm;
	}
}
//-----------------------------------------------------------------------------------
void ds_display_control(DS_CTX *ctx)
//-----------------------------------------------------------------------------------
{	
	int	stereo = 1;

	if (ctx->drawAdj.stereoFlag)
	{
		MTX_MATRIX	copy = ctx->matrix;
		ds_reshape_stereo(ctx, 0);
		ds_display(ctx, 0, 1);
		ctx->matrix = copy;
		ds_reshape_stereo(ctx, 1);
		ds_display(ctx, 1, 0);
		ctx->matrix = copy;
	}
	else
	{
		ds_display(ctx, 1, 1);
	}
}

//-----------------------------------------------------------------------------------
void ds_display ( DS_CTX *ctx, int swap, int clear )
//-----------------------------------------------------------------------------------
{
	static GLfloat mat_specular[4] = { (float)0.0, (float)0.5, (float)1.0, (float)1.0 };
	static GLfloat mat_shininess = 50.0;
	static GLfloat light_position[4];// = { (float)0.5, (float)0.5, (float)2.0, (float)0.0 };
	static GLfloat light_ambient[4] = { (float)0.1, (float)0.1, (float)0.1, (float)1.0 };
	static GLfloat light_diffuse[4] = { (float)0.3, (float)0.3, (float)0.3, (float)1.0 };
	static GLfloat light_specular[4] = { (float)0.0, (float)0.0, (float)0.0, (float)1.0 };
	static GLdouble eqn[4] = { 0.0, 1.0, 0.0, 0.0 };
	static GLdouble eqn2[4] = { 1.0, 0.0, 0.0, 0.0 };
	static MTX_VECTOR clip_in = { 0.0, 0.0, 1.0, 0.0 }, clip_out;
	static GLfloat	density = (float)0.9;
	static GLfloat fogStart = 4.0, fogEnd = 4.5;
	static GLfloat fogColor[4];
	static GLfloat fogStartCoord[4] = { 0.0,0.0,0.0,1.0 };
	static GUT_POINT		pt;
	static GUT_PLANE		pl;
	static GUT_VECTOR		n;


	glShadeModel(GL_SMOOTH);          // Use Smooth shading ( This is the Default so we dont actually have to set it) 
	if (ctx->drawAdj.fogFlag)//global_fog)
	{
		fogColor[0] = (float)(ctx->clrCtl.bkgClear.r * 0.90);
		fogColor[1] = (float)(ctx->clrCtl.bkgClear.g * 0.90);
		fogColor[2] = (float)(ctx->clrCtl.bkgClear.b * 0.90);
		fogColor[3] = (float)1.0;
		glEnable(GL_FOG);
		glFogi(GL_FOG_MODE, GL_LINEAR);// GL_EXP2); // GL_LINEAR);
		glFogfv(GL_FOG_COLOR, fogColor);
		glFogf(GL_FOG_DENSITY, density);
		// transform the start and end points of fog

		fogStart = (float)( 4.0 - ctx->trans[2] );
		fogEnd = (float)( 4.5 - ctx->trans[2] );
//		fogStart = (float)(4.0);// -ctx->trans[2]);
//		fogEnd = (float)(4.5); // -ctx->trans[2]);
		glFogf(GL_FOG_START, fogStart);
		glFogf(GL_FOG_END, fogEnd);
		glHint(GL_FOG_HINT, GL_NICEST);
	}
	else
	{
		glDisable(GL_FOG);
	}

	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, &mat_shininess);
	glDisable(GL_LIGHTING);
	if (ctx->clrCtl.useLightingFlag)
	{
		glEnable(GL_LIGHTING);
		light_position[0] = (float)ctx->clrCtl.light.x;
		light_position[1] = (float)ctx->clrCtl.light.y;
		light_position[2] = (float)ctx->clrCtl.light.z;
		light_position[3] = 0; // ctx->clrCtl.light.w;
		glLightfv(GL_LIGHT0, GL_POSITION, light_position);
		glLightfv(GL_LIGHT0, GL_AMBIENT, light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_specular);
		glEnable(GL_LIGHT0);
	}

	glColorMaterial(GL_FRONT, GL_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glDepthFunc(GL_LESS);
	glEnable(GL_DEPTH_TEST);

	if (ctx->drawAdj.clipFlag) //->clip.active )
	{
		pt.x = 0;
		pt.y = 0;
		pt.z = ctx->drawAdj.clipZValue; // ctx->clip.z_value;
		n.i = 0;
		n.j = 0;
		n.k = 1;
		n.l = 1;
		gut_plane_from_point_normal(&pt, &n, &pl);

		glEnable(GL_CLIP_PLANE0);
		clip_in.data.ijkl[0] = pl.A;
		clip_in.data.ijkl[1] = pl.B;
		clip_in.data.ijkl[2] = pl.C;
		clip_in.data.ijkl[3] = pl.D;
		
		glPushMatrix();
		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
		glMatrixMode(GL_MODELVIEW);
		glMultMatrixd((double*)&ctx->matrix);
		glClipPlane(GL_CLIP_PLANE0, (const double*)&clip_in);
		glPopMatrix();
/* 
{
			MTX_MATRIX	a, b, c;
//			mtx_set_unity(&a);
			mtx_create_translation_matrix(&a, (double)ctx->trans[0], (double)ctx->trans[1], (double)ctx->trans[2]);

//			mtx_multiply_matrix(a, &ctx->matrix, &c);
			mtx_multiply_matrix(&ctx->matrix, &a, &b);
//			mtx_multiply_matrix(&a, &ctx->matrix, &b);
			mtx_vector_multiply(1, &clip_in, &clip_out, &b); // transform the vertices once
//			glClipPlane(GL_CLIP_PLANE0, (const double*)&clip_out);
			glPushMatrix();
			glMatrixMode(GL_MODELVIEW);
			glClipPlane(GL_CLIP_PLANE0, (const double*)&clip_out);
			glPopMatrix();
		}

//		myx
//		mtx_multiply_matrix(mp, &ctx->matrix, &mtx);
//		mtx_multiply_matrix(&mtx, &rmtx, &tmtx);
//		void mtx_vector_multiply( 1, (MTX_VECTOR *)&clip_in, (MTX_VECTOR *)&clip_out, MTX_MATRIX *m);


////		glPushMatrix();
////		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
//		{
//			glMatrixMode(GL_MODELVIEW);
////			glMultMatrixd((double*)&ctx->matrix);
//			glClipPlane(GL_CLIP_PLANE0, (const double*)&clip_in);
//		}
//		glPopMatrix();
*/
	}
	else
	{
		glDisable(GL_CLIP_PLANE0);
	}

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);

	if (clear)
	{
		//	glClearColor((float)ctx->clear.r, (float)ctx->clear.g, (float)ctx->clear.b, (float)0.0);
		glClearColor((float)ctx->clrCtl.bkgClear.r, (float)ctx->clrCtl.bkgClear.g, (float)ctx->clrCtl.bkgClear.b, (float)0.0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

//	glEnable(GL_ALPHA_TEST);
//	glEnable(GL_BLEND);
//	glDepthMask(GL_FALSE);
//	glBlendFunc(GL_SRC_ALPHA, GL_ONE);

	glPushMatrix();
//	glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
	{
		glMatrixMode(GL_MODELVIEW);
//		glMultMatrixd((double*)&ctx->matrix);
	}

	//--------------------------------------------------------------------
	glDisable(GL_ALPHA_TEST);

	glColor3f(1.0f, 0.0f, 0.0f);

	switch (ctx->geomAdj.polymode[0]) {
	case GEOMETRY_POLYMODE_FILL:   glPolygonMode(GL_FRONT, GL_FILL); break;
	case GEOMETRY_POLYMODE_LINE:   glPolygonMode(GL_FRONT, GL_LINE); break;
	case GEOMETRY_POLYMODE_POINT:  glPolygonMode(GL_FRONT, GL_POINT); break;
	default: glPolygonMode(GL_FRONT, GL_FILL);
	}

	switch (ctx->geomAdj.polymode[1]) {
	case GEOMETRY_POLYMODE_FILL:  glPolygonMode(GL_BACK, GL_FILL); break;
	case GEOMETRY_POLYMODE_LINE:  glPolygonMode(GL_BACK, GL_LINE); break;
	case GEOMETRY_POLYMODE_POINT: glPolygonMode(GL_BACK, GL_POINT); break;
	default: glPolygonMode(GL_FRONT, GL_FILL);
	}

	ds_draw_geometry(ctx);

	//--------------------------------------------------------------------

	glPopMatrix();
	glFlush();
	if (swap)
	{
		SwapBuffers(ctx->hDC);			/* nop if singlebuffered */
	}
}

//-----------------------------------------------------------------------------
int ds_draw_circle_segment(GUT_POINT *a, GUT_POINT *b, GUT_POINT *c, GUT_VECTOR *n, DS_COLOR *clr, GUT_POINT *origin)
//-----------------------------------------------------------------------------
{
	// determine if a,b, or c is not on the sphere
	double		distance[3];
	GUT_POINT	*p[3];

	gut_distance_from_point_to_point(a, origin, &distance[0]);
	gut_distance_from_point_to_point(b, origin, &distance[1]);
	gut_distance_from_point_to_point(c, origin, &distance[2]);

	if (fabs(distance[0] - 1.0) > 0.00001)
	{
		p[0] = a;
		p[1] = b;
		p[2] = c;
	}
	else if (fabs(distance[1] - 1.0) > 0.00001)
	{
		p[0] = b;
		p[1] = c;
		p[2] = a;
	}
	else if (fabs(distance[2] - 1.0) > 0.00001)
	{
		p[0] = c;
		p[1] = a;
		p[2] = b;
	}
	else
	{
		return 1;
	}

	{
		GUT_VECTOR	v[2];
		double		angle,
			t,
			tInc;
		GUT_POINT	d,
			e;
		int			i,
			nInc;

		gut_vector(p[0], p[1], &v[0]);
		gut_vector(p[0], p[2], &v[1]);

		gut_normalize_vector(&v[0]);
		gut_normalize_vector(&v[1]);

		gut_dot_product(&v[1], &v[0], &angle);

		angle = acos(angle);

		t = angle / DTR(10.0);
		if (t < 1.0)
			nInc = 1;
		else if (t < 2.0)
			nInc = 2;
		else
			nInc = (int)t;

		tInc = 1.0 / nInc;

		gut_distance_from_point_to_point(p[0], p[1], &distance[0]);

		for (i = 0, d = *p[1], t = tInc; i<nInc; ++i, t += tInc)
		{
			gut_vector(p[0], &d, &v[0]);
			distance[1] = sqrt(v[0].i * v[0].i + v[0].j * v[0].j + v[0].k * v[0].k);
			distance[2] = distance[1] / distance[0];
			d.x = p[0]->x + v[0].i / distance[2];
			d.y = p[0]->y + v[0].j / distance[2];
			d.z = p[0]->z + v[0].k / distance[2];

			gut_parametric_point(p[1], p[2], &e, t);
			gut_vector(p[0], &e, &v[0]);
			distance[1] = sqrt(v[0].i * v[0].i + v[0].j * v[0].j + v[0].k * v[0].k);
			distance[2] = distance[1] / distance[0];
			e.x = p[0]->x + v[0].i / distance[2];
			e.y = p[0]->y + v[0].j / distance[2];
			e.z = p[0]->z + v[0].k / distance[2];
			glBegin(GL_TRIANGLES);
			glNormal3f((float)n->i * 3, (float)n->j * 3, (float)n->k * 3);
//			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glColor4f((float)clr->r, (float)clr->g, (float)clr->b, (float)clr->a);
			glVertex3f((float)p[0]->x, (float)p[0]->y, (float)p[0]->z);
			glVertex3f((float)d.x, (float)d.y, (float)d.z);
			glVertex3f((float)e.x, (float)e.y, (float)e.z);
			glEnd();
			d = e;
		}
	}

	return 0;
}

//-----------------------------------------------------------------------------
void ds_draw_triangle(DS_CTX *ctx, GUT_POINT *a, GUT_POINT *b, GUT_POINT *c, DS_COLOR *clr) //DS_COLOR *ctbl, int id)
//-----------------------------------------------------------------------------
{
	static GUT_VECTOR	vab, vbc, normal;

	// determine face normal from cross product
	gut_vector(a, b, &vab);
	gut_vector(b, c, &vbc);
	gut_cross_product(&vab, &vbc, &normal);
	gut_normalize_point((GUT_POINT*)&normal);

	glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
	glColor3f((float)clr->r, (float)clr->g, (float)clr->b);

	glVertex3f((float)a->x, (float)a->y, (float)a->z);
	glVertex3f((float)b->x, (float)b->y, (float)b->z);
	glVertex3f((float)c->x, (float)c->y, (float)c->z);
}

//-----------------------------------------------------------------------------
void ds_draw_triangle_hex(DS_CTX *ctx, GUT_POINT *p, GUT_VECTOR *n, DS_COLOR *clr, int i, int j, int k, int nSeg )
//-----------------------------------------------------------------------------
{
	// determine face normal from cross product
	glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
	glNormal3f((float)n[i % nSeg].i * 3, (float)n[i % nSeg].j * 3, (float)n[i % nSeg].k * 3);
	glVertex3f((float)p[i].x, (float)p[i].y, (float)p[i].z);
	glNormal3f((float)n[j % nSeg].i * 3, (float)n[j % nSeg].j * 3, (float)n[j % nSeg].k * 3);
	glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
	glNormal3f((float)n[k % nSeg].i * 3, (float)n[k % nSeg].j * 3, (float)n[k % nSeg].k * 3);
	glVertex3f((float)p[k].x, (float)p[k].y, (float)p[k].z);
}

//-----------------------------------------------------------------------------
int ds_gl_render_vertex(DS_CTX *ctx, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE	*vt, int nVtx, int nTri, double scale, DS_COLOR *clr, GUT_POINT *origin)
//-----------------------------------------------------------------------------
{
	// RENDER A SINGLE VERTEX WITH THE SPECIFIED DS_COLOR
	int					j;
//	static VTRIANGLE	*vt;
	static GUT_POINT	v;
//	int					nTri;

	v = *vtx;

	// check for special flag to re-normalize
	if (ctx->drawAdj.normalizeFlag)
		ds_normalize_point(origin, &v);

	// modify vertex object coordinates to be at this global position
	for (j = 0; j < nVtx; ++j) //ctx->renderVertex.vtxObj.nVtx; ++j)
	{
		vd[j].x = vs[j].x * scale + v.x;
		vd[j].y = vs[j].y * scale + v.y;
		vd[j].z = vs[j].z * scale + v.z;
		vd[j].w = 1;
	}
	// make OpenGL calls
	glBegin(GL_TRIANGLES);
	for (j = 0; j < nTri; ++j, ++vt)
	{
		glNormal3f((float)vs[vt->vtx[0]].x * 3, (float)vs[vt->vtx[0]].y * 3, (float)vs[vt->vtx[0]].z * 3);
		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		glVertex3f((float)vd[vt->vtx[0]].x, (float)vd[vt->vtx[0]].y, (float)vd[vt->vtx[0]].z);// ++v;

		glNormal3f((float)vs[vt->vtx[1]].x * 3, (float)vs[vt->vtx[1]].y * 3, (float)vs[vt->vtx[1]].z * 3);
		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		glVertex3f((float)vd[vt->vtx[1]].x, (float)vd[vt->vtx[1]].y, (float)vd[vt->vtx[1]].z);// ++v;

		glNormal3f((float)vs[vt->vtx[2]].x * 3, (float)vs[vt->vtx[2]].y * 3, (float)vs[vt->vtx[2]].z * 3);
		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		glVertex3f((float)vd[vt->vtx[2]].x, (float)vd[vt->vtx[2]].y, (float)vd[vt->vtx[2]].z);// ++v;
	}
	glEnd();
	return 0;
}

//-----------------------------------------------------------------------------
int ds_gl_render_edge(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *va, GUT_POINT *vb, GUT_POINT *p, GUT_POINT *origin, GUT_POINT *out, GUT_VECTOR *normal, DS_COLOR *clr, int nSeg)
//-----------------------------------------------------------------------------
{
	glBegin(GL_TRIANGLES);
	// copy vertex data to new variables
	p[0] = *va;
	p[1] = *vb;

	// check for special flag to re-normalize
	if (ctx->drawAdj.normalizeFlag)//if (ctx->global_normalize)
	{
		ds_normalize_point(origin, &p[0]);
		ds_normalize_point(origin, &p[1]);
	}

	if (gobj->eAttr.type == GEOMETRY_EDGE_SQUARE)
	{
		ds_geo_edge_to_triangles(ctx, &gobj->eAttr, &p[0], &p[1], out, ctx->drawAdj.normalizeFlag, origin);

		ds_draw_triangle(ctx, &out[0], &out[1], &out[2], clr); 
		ds_draw_triangle(ctx, &out[2], &out[3], &out[0], clr); 
		if (ctx->eAttr.height != 0.0)
		{
			ds_draw_triangle(ctx, &out[0], &out[4], &out[5], clr); 
			ds_draw_triangle(ctx, &out[5], &out[1], &out[0], clr); 
			ds_draw_triangle(ctx, &out[2], &out[6], &out[7], clr); 
			ds_draw_triangle(ctx, &out[7], &out[3], &out[2], clr); 

			ds_draw_triangle(ctx, &out[4], &out[7], &out[6], clr);
			ds_draw_triangle(ctx, &out[6], &out[5], &out[4], clr);
		}
	}
	else
	{
		static int i, j;
		ds_geo_edge_to_triangles_hex(ctx, &gobj->eAttr, &p[0], &p[1], out+0, out+nSeg, normal, ctx->drawAdj.normalizeFlag, origin, nSeg);

		for (i = 0, j=nSeg; i < nSeg; ++i, ++j)
		{
			ds_draw_triangle_hex(ctx, out, normal, clr, i, j, (j + 1) % nSeg + nSeg, nSeg);
			ds_draw_triangle_hex(ctx, out, normal, clr, (j + 1) % nSeg + nSeg, (i + 1) % nSeg, i, nSeg);
		}
	}
	glEnd();
	return 0;
}

//-----------------------------------------------------------------------------
int ds_gl_render_axii(DS_CTX *ctx, GUT_POINT *p, GUT_POINT *out, GUT_POINT *origin )
//-----------------------------------------------------------------------------
{
	double	width = ctx->eAttr.width;
	static DS_COLOR		red = { 1,0,0 }, grn = { 0,1,0 }, blu = { 0,0,1 };
	static GUT_VECTOR	normal[6], x = { 1,0,0 }, y = { 0, 1, 0 }, z = { 0,0,1 };
	static DS_COLOR		*clr;
	glBegin(GL_TRIANGLES);
	// copy vertex data to new variables
	ctx->eAttr.width = 0.01;
	p[0].x = -1.05; p[0].y = 0; p[0].z = 0; p[1].x = 1.05; p[1].y = 0; p[1].z = 0;
	clr = &red;
	x.i *= -1.0;
	ds_geo_edge_to_triangles_hex_axii(&ctx->eAttr, &p[0], &p[1], out, &y, &x, &z, normal, ctx->drawAdj.normalizeFlag, origin);
	ds_draw_triangle_hex(ctx, out, normal, clr,  0,  6,  7, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  7,  1,  0, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  1,  7,  8, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  8,  2,  1, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  2,  8,  9, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  9,  3,  2, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  3,  9, 10, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr, 10,  4,  3, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  4, 10, 11, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr, 11,  5,  4, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  5, 11,  6, 6 );
	ds_draw_triangle_hex(ctx, out, normal, clr,  6,  0,  5, 6 );
	x.i *= -1.0;
	p[0].x = 0; p[0].y = -1.05; p[0].z = 0; p[1].x = 0; p[1].y = 1.05; p[1].z = 0;
	clr = &grn;
	ds_geo_edge_to_triangles_hex_axii(&ctx->eAttr, &p[0], &p[1], out, &x, &y, &z, normal, ctx->drawAdj.normalizeFlag, origin);
	ds_draw_triangle_hex(ctx, out, normal, clr, 0, 6, 7, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 7, 1, 0, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 1, 7, 8, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 8, 2, 1, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 2, 8, 9, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 9, 3, 2, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 3, 9, 10, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 10, 4, 3, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 4, 10, 11, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 11, 5, 4, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 5, 11, 6, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 6, 0, 5, 6);
	p[0].x = 0; p[0].y = 0; p[0].z = -1.05; p[1].x = 0; p[1].y = 0; p[1].z = 1.05;
	clr = &blu;
	ds_geo_edge_to_triangles_hex_axii(&ctx->eAttr, &p[0], &p[1], out, &y, &z, &x, normal, ctx->drawAdj.normalizeFlag, origin);
	ds_draw_triangle_hex(ctx, out, normal, clr, 0, 6, 7, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 7, 1, 0, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 1, 7, 8, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 8, 2, 1, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 2, 8, 9, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 9, 3, 2, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 3, 9, 10, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 10, 4, 3, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 4, 10, 11, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 11, 5, 4, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 5, 11, 6, 6);
	ds_draw_triangle_hex(ctx, out, normal, clr, 6, 0, 5, 6);
	ctx->eAttr.width = width;
	glEnd();
	if (ctx->drawAdj.axiiLabelFlag)
	{
		ds_label_draw(&ctx->label.axii, (float)1.1, (float)0.0, (float)0.0, (float)1.0, "+X");
		ds_label_draw(&ctx->label.axii, (float)0.0, (float)1.1, (float)0.0, (float)1.0, "+Y");
		ds_label_draw(&ctx->label.axii, (float)0.0, (float)0.0, (float)1.1, (float)1.0, "+Z");
	}
	return 0;
}

//-----------------------------------------------------------------------------
int ds_transparent_face_compare(void *passThru, void *av, void *bv)
//-----------------------------------------------------------------------------
{
	// compare two sorted faces by their z value
	DS_FACE_SORT	*a=(DS_FACE_SORT*)av, *b= (DS_FACE_SORT*)bv;

	double diff = a->centroid.z - b->centroid.z;

	if (diff > 0.0)
		return 1;
	else if (diff < 0.0)
		return -1;
	else
	{
		// ad to existing node
		if (!a->queue)
			a->queue = LL_Create();
		LL_AddTail(a->queue, bv);

		return 0;
	}

	return 0;
}

//-----------------------------------------------------------------------------
int ds_transparent_matrix_compare(void *passThru, void *av, void *bv)
//-----------------------------------------------------------------------------
{
	// compare two matrices by their ID value
	DS_MATRIX_SORT	*a = (DS_MATRIX_SORT*)av, *b = (DS_MATRIX_SORT*)bv;

	int		diff = a->id - b->id;

	if (diff)
		return diff;
	else if (diff < 0.0) // save match 
		((DS_CTX*)passThru)->transparency.mtx_match = a;

	return 0;
}

//-----------------------------------------------------------------------------
int ds_blend_sort_free_face(DS_CTX *ctx, DS_FACE_SORT *fs)
//-----------------------------------------------------------------------------
{
	if (fs->queue)
	{
		DS_FACE_SORT	*fs2;
		LL_SetHead(fs->queue);
		while (fs2 = (DS_FACE_SORT*)LL_GetNext(fs->queue))
		{
			ds_blend_sort_free_face(ctx, fs2);
		}
		LL_Destroy(fs->queue);
	}
	mem_free(ctx->transparency.fs_set, (void*)fs);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_blend_sort_free_matrix(DS_CTX *ctx, DS_MATRIX_SORT *matrix)
//-----------------------------------------------------------------------------
{
	mem_free(ctx->transparency.mtx_set, (void*)matrix);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_blend_draw_face(DS_CTX *ctx, DS_FACE_SORT *fs)
//-----------------------------------------------------------------------------
{
	DS_GEO_OBJECT			*gobj = fs->object;
	DS_COLOR				*clr, color;
	DS_FACE					*face = fs->face;
	int						reverseOrder = fs->reverseOrder;
	int						j, k;
	static MTX_VECTOR		*v;
	static GUT_POINT		p[32];
	GUT_VECTOR				normal, vab, vbc;

	if (fs->queue) // there are more with the same Z value so process them first
	{
		DS_FACE_SORT	*fs2;
		LL_SetHead(fs->queue);
		while (fs2 = (DS_FACE_SORT*)LL_GetNext(fs->queue))
			ds_blend_draw_face(ctx, fs2);
	}

	v = gobj->v_out; // ppinter to array of object's vertices 

	if ( fs->ms->id != gobj->matrixID )
	{
		// redo the model matrix on the data
		mtx_vector_multiply(gobj->nVtx, (MTX_VECTOR*)gobj->vtx, gobj->v_out, &fs->ms->mtx); // transform the vertices once
		// update which matrix is associated with the transformed vertices
		gobj->matrixID = fs->ms->id;
	}

	// set up and draw
	switch (gobj->cAttr.face.state) {
	case DS_COLOR_STATE_EXPLICIT:	clr = &face->color;								break;
	case DS_COLOR_STATE_AUTOMATIC:	ds_ctbl_get_color(gobj->ctT, face->id, &clr);	break;
	case DS_COLOR_STATE_OVERRIDE:	clr = &gobj->cAttr.face.color;					break;
	}

	if (!reverseOrder) // copy vertex data to new variables
		for (j = 0; j<face->nVtx; ++j) p[j] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];
	else
		for (k = 0, j = face->nVtx - 1; j >= 0; --j, ++k) p[k] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];

	// check for special flag to re-normalize
	if (ctx->drawAdj.normalizeFlag)//if (ctx->global_normalize)
		for (j = 0; j < face->nVtx; ++j) ds_normalize_point(&ctx->origin, &p[j]);

	// determine face normal from cross product
	gut_vector(&p[0], &p[1], &vab);
	gut_vector(&p[1], &p[2], &vbc);
	gut_cross_product(&vab, &vbc, &normal);
	gut_normalize_point((GUT_POINT*)&normal);

	if (face->nVtx == 3 && ctx->drawAdj.circleFlag)
	{
		color = *clr;
		color.a = fs->alpha;
		ds_draw_circle_segment(&p[0], &p[1], &p[2], &normal, &color, &ctx->origin);
	}
	else
	{
		glBegin(GL_TRIANGLES);
		for (j = 0; j < face->nVtx; ++j)
		{
			if (j >= 2)
			{
				// determine face normal from cross product
				gut_vector(&p[0], &p[j - 1], &vab);
				gut_vector(&p[j - 1], &p[j], &vbc);
				gut_cross_product(&vab, &vbc, &normal);
				gut_normalize_point((GUT_POINT*)&normal);
				glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
				glColor4f((float)clr->r, (float)clr->g, (float)clr->b, (float)fs->alpha);
				glVertex3f((float)p[0].x, (float)p[0].y, (float)p[0].z);
				glVertex3f((float)p[j - 1].x, (float)p[j - 1].y, (float)p[j - 1].z);
				glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
			}
		}
		glEnd();
	}
	return 0;
}

//-----------------------------------------------------------------------------
ds_blend_sort_init(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	// free up any left over faces 
	int		nNodes, nLevels;
	avl_info(ctx->transparency.avlZSort, &nNodes, &nLevels); // Generate information on the specified AVL tree
	if ( nNodes )
		avl_traverse_ltr(ctx->transparency.avlZSort, (void*)ctx, ds_blend_sort_free_face); // Traverse the tree from left to right - if user function returns non-zero traversal is stopped and value returned
	avl_destroy(ctx->transparency.avlZSort, 0);

	// free up any matrices
	avl_info(ctx->transparency.avlMtxSort, &nNodes, &nLevels); // Generate information on the specified AVL tree
	if (nNodes)
		avl_traverse_ltr(ctx->transparency.avlMtxSort, (void*)ctx, ds_blend_sort_free_matrix); // Traverse the tree from left to right - if user function returns non-zero traversal is stopped and value returned
	avl_destroy(ctx->transparency.avlMtxSort, 0);

	ctx->transparency.avlZSort = avl_create(ds_transparent_face_compare, (void*)ctx);
	ctx->transparency.avlMtxSort = avl_create(ds_transparent_matrix_compare, (void*)ctx);

	return 0;
}

//-----------------------------------------------------------------------------
int ds_blend_sort_one_face(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, MTX_MATRIX *mtx, int reverseOrder, int matrixID, float alpha)
//-----------------------------------------------------------------------------
{
	GUT_POINT		*v;
	GUT_POINT		centroid;
	int				i;
	DS_FACE_SORT	*fs;
	DS_MATRIX_SORT	ms, *msp; 

	// add object to sort tree if there is transparency
	// process face if a polygon
	if (face->nVtx < 3)
		return 0;

	// compute the centroid for sorting
	for (i = 0, centroid.x = centroid.y = centroid.z = 0, v=(GUT_POINT*)gobj->v_out; i < face->nVtx; ++i)
	{
		centroid.x += v[face->vtx[i]].x;
		centroid.y += v[face->vtx[i]].y;
		centroid.z += v[face->vtx[i]].z;
	}
	centroid.x /= face->nVtx;
	centroid.y /= face->nVtx;
	centroid.z /= face->nVtx;

	// allocate a face node
	{
		fs = (DS_FACE_SORT*)mem_malloc(ctx->transparency.fs_set);// next available slot
	}

	// fill the node
	fs->centroid		= centroid;
	fs->object			= gobj; // need reference
	fs->face			= face;
	fs->reverseOrder	= reverseOrder;
	fs->queue			= 0; // by default there is no queue to hold faces with the same Z value
	fs->ms				= 0; // matrix associated with this face's vertex data
	fs->alpha			= alpha; // save corrected alpha

	// check to see if matrix is already in tree
	ms.id = matrixID;
	if (!avl_find(ctx->transparency.avlMtxSort, &ms, &msp))
	{
		msp      = (DS_MATRIX_SORT*)mem_malloc(ctx->transparency.mtx_set);
		msp->id  = matrixID;
		msp->mtx = *mtx;
		fs->ms   = msp;
		avl_insert(ctx->transparency.avlMtxSort, msp);
	}
	else
	{
		fs->ms = msp;
	}

	avl_insert(ctx->transparency.avlZSort, fs);
		
	return 1;  // number of faces added to the sort
}

//-----------------------------------------------------------------------------
void ds_draw_geometry(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	static int				i, j, k, reverseOrder;
	static MTX_MATRIX		*mp, mtx, rmtx, tmtx;
	static MTX_VECTOR		*v;
	static GUT_POINT		p[32];
	static GUT_POINT		cntr;
	double					maxZ;
	static GUT_VECTOR		vab, vbc, normal;
	static DS_POLYHEDRON	*poly;
	static DS_GEO_OBJECT	*gobj;
	static DS_FACE			*face;
	static DS_COLOR			*clr, color;
	static DS_EDGE			*edge;
	static GUT_POINT		out[24]; // edge triangle points 
	static GUT_POINT		origin[2] = { 0,0,0,1 };
	int						nSeg = 12;
	static GUT_VECTOR		edgeNormal[24];
	int						objectID = 0, matrixID = 0, transformID = 0;
	int						transparentFlag, skipFaceFlag = 0;
	float					alpha;
//	char					buf[12];

	if (ctx->drawAdj.axiiFlag)
	{
		glPushMatrix();
		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
		glMatrixMode(GL_MODELVIEW);
		glMultMatrixd((double*)&ctx->matrix);
		ds_gl_render_axii(ctx, p, out, &origin[0]);
		glPopMatrix();
	}

	mtx_create_translation_matrix(&rmtx, (double)ctx->trans[0], (double)ctx->trans[1], (double)ctx->trans[2]);
	mtx_vector_multiply(1, (MTX_VECTOR*)&origin[0], (MTX_VECTOR*)&origin[1], &rmtx); // transform the vertices once

	ds_blend_sort_init(ctx); // initialize transparency 
		
	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (!gobj->active) continue;

		if (!gobj->v_out) gobj->v_out = malloc(sizeof(MTX_VECTOR) * gobj->nVtx); // alocate memory once 

		transformID = 0;
		ds_geometry_draw_init(ctx, gobj); // initialize transformations unque to each object
		while (ds_geometry_next_draw_transform(ctx, gobj, &mp, &reverseOrder, ctx->geomAdj.orientation))
		{
			gobj->matrixID = matrixID = (objectID << 8) | (transformID++ & 0xff); // associate the current matrix with the object

			mtx_multiply_matrix(mp, &ctx->matrix, &mtx);
			mtx_multiply_matrix(&mtx, &rmtx, &tmtx);
			mtx_vector_multiply(gobj->nVtx, (MTX_VECTOR*)gobj->vtx, gobj->v_out, &tmtx); // transform the vertices once

			if (gobj->nTri && (gobj->drawWhat & GEOMETRY_DRAW_FACES)) // faces
			{
				for (i = 0, v = gobj->v_out, face = gobj->tri; i < gobj->nTri; ++i, ++face)
				{
					switch (gobj->cAttr.face.state) {
					case DS_COLOR_STATE_EXPLICIT: clr = &face->color; break;
					case DS_COLOR_STATE_AUTOMATIC:ds_ctbl_get_color(gobj->ctT, face->id, &clr);  break;
					case DS_COLOR_STATE_OVERRIDE: clr = &gobj->cAttr.face.color; break;
					}
					if (transparentFlag = gobj->tAttr.onFlag)
					{
						switch (gobj->tAttr.state) {
						case DS_COLOR_STATE_EXPLICIT: alpha = clr->a; break; // face->color.a;		break;
						case DS_COLOR_STATE_OVERRIDE: alpha = gobj->tAttr.alpha;	break;
						}
						if (alpha == 0.0)
							skipFaceFlag = 1; // nothing to draw
						else if (alpha == 1.0)
							transparentFlag = 0; // fully opaque
					}
					switch (face->nVtx) {
					case 1: // degenerate vertices
						double		scale;
						if (ctx->eAttr.maxLength > 0.0)		scale = gobj->vAttr.scale * ctx->eAttr.maxLength;
						else								scale = gobj->vAttr.scale;
						ds_gl_render_vertex(ctx, (GUT_POINT*)&v[face->vtx[0]], ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, &origin[1]);
						if (gobj->lFlags.face)
						{
							cntr = *(GUT_POINT*)&v[face->vtx[0]];
							normal.i = cntr.x - origin[1].x;
							normal.j = cntr.y - origin[1].y;
							normal.k = cntr.z - origin[1].z;
							gut_normalize_vector(&normal);
							glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
							ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + scale * 1.25, (float)normal.k, face->id);
						}
						break;
					case 2: // degenerate edges
						ds_gl_render_edge(ctx, gobj, (GUT_POINT*)&v[face->vtx[0]].data.xyzw[0], (GUT_POINT*)&v[face->vtx[1]].data.xyzw[0], p, &origin[1], out, edgeNormal, clr, ctx->drawAdj.quality->edgeNSeg);
						if (gobj->lFlags.face)
						{
							gut_mid_point((GUT_POINT*)&v[face->vtx[0]].data.xyzw[0], (GUT_POINT*)&v[face->vtx[1]].data.xyzw[0], &cntr);
//							gut_mid_point((GUT_POINT*)&v[edge->vtx[0]].data.xyzw, (GUT_POINT*)&v[edge->vtx[1]].data.xyzw, &cntr);
							normal.i = cntr.x - origin[1].x;
							normal.j = cntr.y - origin[1].y;
							normal.k = cntr.z - origin[1].z;
							gut_normalize_vector(&normal);
							glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
							ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, (float)normal.k, face->id);
						}
						break;
					default: // normal polygon faces
						if (transparentFlag) //gobj->cAttr.face.transparencyFlag)
						{
							ds_blend_sort_one_face(ctx, gobj, face, &tmtx, reverseOrder, matrixID, alpha);  // add face to transparency context
						}
						else if ( !skipFaceFlag ) // draw
						{
							if (!reverseOrder) // copy vertex data to new variables
								for (j = 0, cntr.x=0, cntr.y = 0, cntr.z=0, maxZ=(-1000); j < face->nVtx; ++j)
								{
									p[j] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];
									cntr.x += p[j].x; cntr.y += p[j].y; cntr.z += p[j].z;
//									maxZ = p[j].z > maxZ ? p[j].z : maxZ;
								}
							else
								for (k = 0, j = face->nVtx - 1, cntr.x = 0, cntr.y = 0, cntr.z = 0,maxZ = (-1000); j >= 0; --j, ++k)
								{
									p[k] = *(GUT_POINT*)&v[face->vtx[j]].data.xyzw[0];
									cntr.x += p[k].x; cntr.y += p[k].y; cntr.z += p[k].z;
//									maxZ = p[k].z > maxZ ? p[k].z : maxZ;
								}
							cntr.x /= face->nVtx;
							cntr.y /= face->nVtx;
							cntr.z /= face->nVtx;
							// check for special flag to re-normalize
							if (ctx->drawAdj.normalizeFlag)//if (ctx->global_normalize)
								for (j = 0; j < face->nVtx; ++j) ds_normalize_point(&origin[1],&p[j]);

							// determine face normal from cross product
							gut_vector(&p[0], &p[1], &vab);
							gut_vector(&p[1], &p[2], &vbc);
							gut_cross_product(&vab, &vbc, &normal);
							gut_normalize_point((GUT_POINT*)&normal);

							if (face->nVtx == 3 && ctx->drawAdj.circleFlag)
							{
								//							glBegin(GL_TRIANGLES);
								color = *clr;
								color.a = 1.0;
								ds_draw_circle_segment(&p[0], &p[1], &p[2], &normal, clr, &origin[1]);
							}
							else
							{
								glBegin(GL_TRIANGLES);
								for (j = 0; j < face->nVtx; ++j)
								{
									if (j >= 2)
									{
										// determine face normal from cross product
										gut_vector(&p[0], &p[j - 1], &vab);
										gut_vector(&p[j - 1], &p[j], &vbc);
										gut_cross_product(&vab, &vbc, &normal);
										gut_normalize_point((GUT_POINT*)&normal);
										glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
										glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
										glVertex3f((float)p[0].x, (float)p[0].y, (float)p[0].z);
										glVertex3f((float)p[j - 1].x, (float)p[j - 1].y, (float)p[j - 1].z);
										glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
									}
								}
								glEnd();
							}
							if (gobj->lFlags.face) 
								ds_label_draw_id(&ctx->label.face, (float)cntr.x, (float)cntr.y, (float)cntr.z + 0.03, (float)normal.k, face->id);
						}
					}
				}
			}

			if (gobj->nEdge && (gobj->drawWhat & GEOMETRY_DRAW_EDGES))//0x2)) //ctx->tri_edge_mode == 1 || ctx->tri_edge_mode == 2))
			{
				for (i = 0, v = gobj->v_out, edge = gobj->edge; i < gobj->nEdge; ++i, ++edge)
				{
					switch (gobj->cAttr.edge.state) {
					case DS_COLOR_STATE_EXPLICIT: clr = &face->color; break;
					case DS_COLOR_STATE_AUTOMATIC:ds_ctbl_get_color(gobj->ctE, edge->id, &clr);  break;
					case DS_COLOR_STATE_OVERRIDE: clr = &gobj->cAttr.edge.color; break;// &ctx->clrCtl.triangle.override; break;
					}

					ds_gl_render_edge(ctx, gobj, (GUT_POINT*)&v[edge->vtx[0]].data.xyzw, (GUT_POINT*)&v[edge->vtx[1]].data.xyzw, p, &origin[1], out, edgeNormal, clr,
						ctx->drawAdj.quality->edgeNSeg);

					if (gobj->lFlags.edge)
					{
						gut_mid_point((GUT_POINT*)&v[edge->vtx[0]].data.xyzw, (GUT_POINT*)&v[edge->vtx[1]].data.xyzw, &cntr);
						normal.i = cntr.x - origin[1].x;
						normal.j = cntr.y - origin[1].y;
						normal.k = cntr.z - origin[1].z;
						gut_normalize_vector(&normal);
						glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
						ds_label_draw_id(&ctx->label.edge, (float)cntr.x, (float)cntr.y, (float)cntr.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, (float)normal.k, edge->id);
					}
				}
			}

			if (gobj->drawWhat & GEOMETRY_DRAW_VERTICES)//0x4) // render vertices if required
			{
				double		scale;

				clr = &gobj->cAttr.vertex.color;// ctx->renderVertex.clr;

				if (ctx->eAttr.maxLength > 0.0)	scale = gobj->vAttr.scale * ctx->eAttr.maxLength; // 0.03;
				else								scale = gobj->vAttr.scale; // 0.03;

				for (i = 0, v = gobj->v_out; i < gobj->nVtx; ++i, ++face, ++v)
				{
					ds_gl_render_vertex(ctx, (GUT_POINT*)v, (GUT_POINT*)ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, &origin[1]);

					if (gobj->lFlags.vertex)
					{
						cntr = *(GUT_POINT*)v;
						normal.i = cntr.x - origin[1].x;
						normal.j = cntr.y - origin[1].y;
						normal.k = cntr.z - origin[1].z;
						gut_normalize_vector(&normal);
						glNormal3f((float)normal.i * 3, (float)normal.j * 3, (float)normal.k * 3);
						ds_label_draw_id(&ctx->label.vertex, (float)cntr.x, (float)cntr.y, (float)cntr.z + scale * 1.25, (float)normal.k, i);
					}
				}
			}
		}

		++objectID;
	}

	{ // processing of transparent faces
		int		nNodes, nLevels;
		avl_info(ctx->transparency.avlZSort, &nNodes, &nLevels);
		if (nNodes)
		{
			ctx->origin = origin[1];
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);
			glDepthMask(GL_TRUE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			// traverse the avl tree of faces - back to front z ordered 
			avl_traverse_ltr(ctx->transparency.avlZSort, (void*)ctx, ds_blend_draw_face);
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
		}
	}

	glBegin(GL_TRIANGLES);

	if (ctx->drawAdj.clipFlag && ctx->drawAdj.clipVisibleFlag)
	{
		glNormal3f((float)0, (float)0, (float)3);

		glColor3f((float)1, (float)1, (float)0);
		glVertex3f((float)1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)1.05, (float)1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float)1.05, (float)ctx->drawAdj.clipZValue);

		glVertex3f((float)-1.05, (float)1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
	}

	glEnd();
}
