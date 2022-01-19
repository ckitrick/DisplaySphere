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
int ds_gl_render_edge(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *va, GUT_POINT *vb, GUT_POINT *p, GUT_POINT *origin, GUT_POINT *out, GUT_VECTOR *normal, DS_COLOR *clr, int nSeg);
int ds_gl_render_vertex(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE	*vt, int nVtx, int nTri, double scale, DS_COLOR *clr, int insideOutFlag);

void ds_draw_geometry_transparency(DS_CTX *ctx);
void textout(float offset, float x, float y, float z, char *string);
void label(float x, float y, float z, float zn, char *string);

void ds_face_initialize(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
void ds_face_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
void ds_face_draw(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, DS_COLOR *clr, float alpha);
void ds_edge_initialize(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
void ds_edge_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj);
void ds_edge_draw(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_EDGE *edge, DS_COLOR *clr);
void ds_geo_edge_to_triangles_arc(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, DS_COLOR *clr);
void ds_geo_edge_to_triangles_hex_arc(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, DS_COLOR *clr);

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
	else if (ctx->drawAdj.projection == GEOMETRY_PROJECTION_PERSPECTIVE)
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
	// https://www.khronos.org/registry/OpenGL-Refpages/gl2.1/xhtml/glLight.xml
	//
	static MTX_MATRIX	mtx;
	static GLfloat		light_position[4];
	static GUT_POINT	light_pos;
	static GLfloat		light_ambient[4]  = { (float)0.4, (float)0.4, (float)0.4, (float)1.0 };
	static GLfloat		light_diffuse[4]  = { (float)0.8, (float)0.8, (float)0.8, (float)1.0 };
	static GLfloat		light_specular[4] = { (float)0.5, (float)0.5, (float)0.5, (float)1.0 };
	static GLfloat		mat_shininess = 50; // 100.0; //50
	static GLfloat		mat_ambient[4] = { (float)0.0, (float)0.0, (float)0.0, (float)1.0 };
	static GLfloat		mat_diffuse[4] = { (float)0.7, (float)0.7, (float)0.7, (float)1.0 };
	static GLfloat		mat_specular[4] = { (float)1.0, (float)1.0, (float)1.0, (float)1.0 };

	static GLdouble		eqn[4] = { 0.0, 1.0, 0.0, 0.0 };
	static GLdouble		eqn2[4] = { 1.0, 0.0, 0.0, 0.0 };
	static MTX_VECTOR	clip_in = { 0.0, 0.0, 1.0, 0.0 }, clip_out;
	static GLfloat		density = (float)0.9;
	static GLfloat		fogStart = 4.0, fogEnd = 4.5;
	static GLfloat		fogColor[4];
	static GLfloat		fogStartCoord[4] = { 0.0,0.0,0.0,1.0 };
	static GUT_POINT	pt;
	static GUT_PLANE	pl;
	static GUT_VECTOR	n;

	if (ctx->lighting.ambientEnabled)
		light_ambient[0] = light_ambient[1] = light_ambient[2] = (float)ctx->lighting.ambientPercent;
	else
		light_ambient[0] = light_ambient[1] = light_ambient[2] = (float)0.0;
	if (ctx->lighting.diffuseEnabled)
		light_diffuse[0] = light_diffuse[1] = light_diffuse[2] = (float)ctx->lighting.diffusePercent;
	else
		light_diffuse[0] = light_diffuse[1] = light_diffuse[2] = (float)0.0;
	if (ctx->lighting.specularEnabled)
		light_specular[0] = light_specular[1] = light_specular[2] = (float)ctx->lighting.specularPercent;
	else
		light_specular[0] = light_specular[1] = light_specular[2] = (float)0.0;
	mat_specular[0] = mat_specular[1] = mat_specular[2] = (float)ctx->lighting.matSpecular;

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
		glFogf(GL_FOG_START, fogStart);
		glFogf(GL_FOG_END, fogEnd);
		glHint(GL_FOG_HINT, GL_NICEST);
	}
	else
	{
		glDisable(GL_FOG);
	}

	mat_shininess = (float)ctx->lighting.matShininess;
	glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
	glMaterialfv(GL_FRONT, GL_SHININESS, &mat_shininess);
	glDisable(GL_LIGHTING);
	if (ctx->clrCtl.useLightingFlag)
	{
		glEnable(GL_LIGHTING);
		mtx_create_translation_matrix(&mtx, (double)ctx->trans[0], (double)ctx->trans[1], (double)ctx->trans[2]);
		mtx_vector_multiply(1, (MTX_VECTOR*)&ctx->lighting.position, (MTX_VECTOR*)&light_pos, &mtx); // transform the vertices once
		light_position[0] = (float)light_pos.x;
		light_position[1] = (float)light_pos.y;
		light_position[2] = (float)light_pos.z;
		light_position[3] = 1.0;
		glLightfv(GL_LIGHT0, GL_POSITION,		light_position);
		glLightfv(GL_LIGHT0, GL_AMBIENT,		light_ambient);
		glLightfv(GL_LIGHT0, GL_DIFFUSE,		light_diffuse);
		glLightfv(GL_LIGHT0, GL_SPECULAR,		light_specular);
		glEnable(GL_LIGHT0);
	}

	glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_NORMALIZE);

	glEnable(GL_DEPTH_TEST);	// Hidden surface removal
	glFrontFace(GL_CCW);		// Counter clock-wise polygons face out
	if (ctx->geomAdj.cull[0] || ctx->geomAdj.cull[1])
	{
		if (ctx->geomAdj.cull[0] && ctx->geomAdj.cull[1]) glCullFace(GL_FRONT_AND_BACK);
		else if (ctx->geomAdj.cull[0]) glCullFace(GL_FRONT);
		else glCullFace(GL_BACK);
		glEnable(GL_CULL_FACE);
	}
	else
		glDisable(GL_CULL_FACE);

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
		glMatrixMode(GL_MODELVIEW);
		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
		glMultMatrixd((double*)&ctx->matrix);
		glClipPlane(GL_CLIP_PLANE0, (const double*)&clip_in);
		glPopMatrix();
	}
	else
	{
		glDisable(GL_CLIP_PLANE0);
	}

	glDisable(GL_LINE_SMOOTH);
	glDisable(GL_BLEND);

	if (clear)
	{
		glClearColor((float)ctx->clrCtl.bkgClear.r, (float)ctx->clrCtl.bkgClear.g, (float)ctx->clrCtl.bkgClear.b, (float)0.0);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}

	glPushMatrix();
	glMatrixMode(GL_MODELVIEW);

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
	default: glPolygonMode(GL_BACK, GL_FILL);
	}

	// draw the user defined objects
	ds_draw_geometry(ctx);

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

		t = angle / DTR(5.0);
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
			glNormal3f((float)n->i, (float)n->j, (float)n->k);
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

	glNormal3f((float)normal.i, (float)normal.j, (float)normal.k);
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
	glNormal3f((float)n[i % nSeg].i, (float)n[i % nSeg].j, (float)n[i % nSeg].k);
	glVertex3f((float)p[i].x, (float)p[i].y, (float)p[i].z);
	glNormal3f((float)n[j % nSeg].i, (float)n[j % nSeg].j, (float)n[j % nSeg].k);
	glVertex3f((float)p[j].x, (float)p[j].y, (float)p[j].z);
	glNormal3f((float)n[k % nSeg].i, (float)n[k % nSeg].j, (float)n[k % nSeg].k);
	glVertex3f((float)p[k].x, (float)p[k].y, (float)p[k].z);
}

//-----------------------------------------------------------------------------
int ds_gl_render_vertex(DS_CTX *ctx, DS_GEO_OBJECT *gobj, GUT_POINT *vtx, GUT_POINT *vs, GUT_POINT *vd, DS_VTRIANGLE *vt, int nVtx, int nTri, double scale, DS_COLOR *clr, int insideOutFlag) //GUT_POINT *origin)
//-----------------------------------------------------------------------------
{
	// Need to know if vertex is clipped and inward or outward
	// if ( clipped )
	//		if ( inward )
	//			change to CW order 
	//			flip normal direction
	//		else ( outward )
	//			
	// RENDER A SINGLE VERTEX WITH THE SPECIFIED DS_COLOR
	int					j;
	static GUT_POINT	v;
//
//	GUT_POINT	pt;
//	GUT_VECTOR	n;
//	GUT_PLANE	pl;
//	MTX_VECTOR	clip_in = { 0.0, 0.0, 1.0, 0.0 };
	float		s = -1.0;

//		glPushMatrix();
//		glMatrixMode(GL_MODELVIEW);
//		glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
//		glMultMatrixd((double*)&ctx->matrix);
//		glPopMatrix();

	// push matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	v = *vtx; // make copy of vertex		  
	if (ctx->drawAdj.normalizeFlag) // check for special flag to re-normalize
		gut_normalize_point(&v);
	if (gobj->vAttr.offset.enable)
	{
		GUT_POINT	vv; 
		vv = v;
		gut_normalize_point(&vv);
		v.x += ctx->eAttr.maxLength * gobj->vAttr.offset.factor * vv.x;
		v.y += ctx->eAttr.maxLength * gobj->vAttr.offset.factor * vv.y;
		v.z += ctx->eAttr.maxLength * gobj->vAttr.offset.factor * vv.z;
	}
	glTranslatef((float)v.x, (float)v.y, (float)v.z); // scale matrix
	glScalef((float)scale, (float)scale, (float)scale); // scale matrix


	if (insideOutFlag)
	{
		// change CCW flag
		// reverse normal direction
		glFrontFace(GL_CW);		// set reverse order 
		s = -1;
		glBegin(GL_TRIANGLES);
		for (j = 0; j < nTri; ++j, ++vt)
		{
			glNormal3f((float)vs[vt->vtx[0]].x*s, (float)vs[vt->vtx[0]].y*s, (float)vs[vt->vtx[0]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[0]].x, (float)vs[vt->vtx[0]].y, (float)vs[vt->vtx[0]].z);

			glNormal3f((float)vs[vt->vtx[1]].x*s, (float)vs[vt->vtx[1]].y*s, (float)vs[vt->vtx[1]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[1]].x, (float)vs[vt->vtx[1]].y, (float)vs[vt->vtx[1]].z);

			glNormal3f((float)vs[vt->vtx[2]].x*s, (float)vs[vt->vtx[2]].y*s, (float)vs[vt->vtx[2]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[2]].x, (float)vs[vt->vtx[2]].y, (float)vs[vt->vtx[2]].z);
		}
		glEnd();
		glFrontFace(GL_CCW);		// set normal order 
	}
	else
	{
		// draw
		// make OpenGL calls
		glBegin(GL_TRIANGLES);
		s = 1;
		for (j = 0; j < nTri; ++j, ++vt)
		{
			glNormal3f((float)vs[vt->vtx[0]].x*s, (float)vs[vt->vtx[0]].y*s, (float)vs[vt->vtx[0]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[0]].x, (float)vs[vt->vtx[0]].y, (float)vs[vt->vtx[0]].z);

			glNormal3f((float)vs[vt->vtx[1]].x*s, (float)vs[vt->vtx[1]].y*s, (float)vs[vt->vtx[1]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[1]].x, (float)vs[vt->vtx[1]].y, (float)vs[vt->vtx[1]].z);

			glNormal3f((float)vs[vt->vtx[2]].x*s, (float)vs[vt->vtx[2]].y*s, (float)vs[vt->vtx[2]].z*s);
			glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
			glVertex3f((float)vs[vt->vtx[2]].x, (float)vs[vt->vtx[2]].y, (float)vs[vt->vtx[2]].z);
		}
		glEnd();
	}
	glPopMatrix(); // return to prior state

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
		// top
		ds_draw_triangle(ctx, &out[0], &out[4], &out[5], clr);
		ds_draw_triangle(ctx, &out[5], &out[1], &out[0], clr);
		if (ctx->eAttr.height != 0.0)
		{
			// bottom
			ds_draw_triangle(ctx, &out[2], &out[6], &out[7], clr);
			ds_draw_triangle(ctx, &out[7], &out[3], &out[2], clr);
			// sides
			ds_draw_triangle(ctx, &out[0], &out[3], &out[7], clr);
			ds_draw_triangle(ctx, &out[7], &out[4], &out[0], clr);
			ds_draw_triangle(ctx, &out[5], &out[6], &out[2], clr);
			ds_draw_triangle(ctx, &out[2], &out[1], &out[5], clr);
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

//	int		diff = (int)a->mtx - (int)b->mtx;
//
//	if (diff)
//		return diff;
//	else if (diff < 0.0) // save match 
//		((DS_CTX*)passThru)->transparency.mtx_match = a;

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
	static MTX_VECTOR		*nml;
	static GUT_POINT		p[32];
	static GUT_POINT		n[32];
	GUT_VECTOR				normal, vab, vbc;

	if (fs->queue) // there are more with the same Z value so process them first
	{
		DS_FACE_SORT	*fs2;
		LL_SetHead(fs->queue);
		while (fs2 = (DS_FACE_SORT*)LL_GetNext(fs->queue))
			ds_blend_draw_face(ctx, fs2);
	}

	// set up and draw
	switch (gobj->cAttr.face.state) {
	case DS_COLOR_STATE_EXPLICIT:	clr = &face->color;								break;
	case DS_COLOR_STATE_AUTOMATIC:	ds_ctbl_get_color(gobj->ctT, face->id, &clr);	break;
	case DS_COLOR_STATE_OVERRIDE:	clr = &gobj->cAttr.face.color;					break;
	}

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd((double*)&fs->ms->mtx);
//	glMultMatrixd((double*)fs->ms->mtx); // new version 

	if (reverseOrder)
		glFrontFace(GL_CW);		// change default order 
	ds_face_draw(ctx, gobj, face, clr, fs->alpha);
	if (reverseOrder)
		glFrontFace(GL_CCW);	// switch back order 
	if (gobj->fAttr.label.enable)
		ds_label_draw_id(&gobj->fAttr.label, (float)face->draw.centroid.x, (float)face->draw.centroid.y, (float)face->draw.centroid.z + 0.03, &face->draw.normal, face->id, ctx->clrCtl.useLightingFlag);

	glPopMatrix();

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
int ds_blend_sort_one_face(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, MTX_MATRIX *mtx, MTX_MATRIX *nmtx, int reverseOrder, int matrixID, float alpha)
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

	// need to get matrix and compute correct z 
	GLdouble matrix[16];
	glGetDoublev(GL_MODELVIEW_MATRIX, &matrix);

	mtx_vector_multiply(1, (MTX_VECTOR*)&face->draw.centroid, (MTX_VECTOR*)&centroid, &matrix); // transform the vertices once

	// allocate a face node
	{
		fs = (DS_FACE_SORT*)mem_malloc(ctx->transparency.fs_set);// next available slot
	}

	// fill the node
	fs->centroid		= centroid; // correct sorting value //face->draw.centroid;
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
		msp       = (DS_MATRIX_SORT*)mem_malloc(ctx->transparency.mtx_set);
		msp->id   = matrixID;
		msp->mtx  = *mtx; // save pointer
//		msp->mtx  = mtx;
//		msp->nmtx = *nmtx;
		fs->ms    = msp;
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
	static MTX_MATRIX		*mp, mtx, rmtx, tmtx, nmtx;
	static MTX_VECTOR		*v;
	static MTX_VECTOR		*nml;
	static GUT_POINT		p[32];
	static GUT_POINT		q[32];
	static GUT_POINT		n[32];
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
	float					scale;
	//	char					buf[12];


	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
	glMultMatrixd((double*)&ctx->matrix);

	if (ctx->drawAdj.axiiFlag)
	{
		ds_gl_render_axii(ctx, p, out, &origin[0]);
	}

	mtx_create_translation_matrix(&rmtx, (double)ctx->trans[0], (double)ctx->trans[1], (double)ctx->trans[2]);
	mtx_vector_multiply(1, (MTX_VECTOR*)&origin[0], (MTX_VECTOR*)&origin[1], &rmtx); // transform the vertices once

	ds_blend_sort_init(ctx); // initialize transparency 

	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (!gobj->active) continue;

		transformID = 0;
		ds_geometry_draw_init(ctx, gobj); // initialize transformations unque to each object

		while (ds_geometry_next_draw_transform(ctx, gobj, &mp, &reverseOrder, gobj->geo_orientation))
		{
			gobj->matrixID = matrixID = (objectID << 8) | (transformID++ & 0xff); // associate the current matrix with the object

			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glMultMatrixd((double*)mp);

			for (i = 0, v = gobj->vtx, nml = gobj->nml, face = gobj->tri; i < gobj->nTri; ++i, ++face)
			{
				if (gobj->nTri && (gobj->fAttr.draw)) // faces
				{
					// color initialization
					switch (gobj->cAttr.face.state) {
					case DS_COLOR_STATE_EXPLICIT:	
						clr = &face->color;								
						alpha = gobj->tAttr.state == DS_COLOR_STATE_OVERRIDE ? gobj->tAttr.alpha : clr->a;
						break;
					case DS_COLOR_STATE_AUTOMATIC:	
						ds_ctbl_get_color(gobj->ctT, face->id, &clr);	
						alpha = gobj->tAttr.state == DS_COLOR_STATE_OVERRIDE ? gobj->tAttr.alpha : gobj->faceDefault.a;
						break;
					case DS_COLOR_STATE_OVERRIDE:	
						clr = &gobj->cAttr.face.color;
						alpha = gobj->tAttr.alpha;
						break;
					}
					// need to check for great circles & spherical sections here
					if ( gobj->nml && gobj->fAttr.orthodrome.enable ) 
					{
						int style = gobj->fAttr.orthodrome.style;
						if (style == ORTHODROME_STYLE_SPHERICAL_SECTION && face->nVtx > 5)
							style = ORTHODROME_STYLE_RIM;
						if (style != ORTHODROME_STYLE_SPHERICAL_SECTION)
							for (j = 0; j < face->nVtx; ++j)
								ds_gl_render_great_circle(ctx, gobj, clr, (GUT_POINT*)&v[face->vtx[j]], (GUT_VECTOR*)&gobj->nml[face->nml[j]]);
						else
							ds_gl_spherical_section(ctx, gobj, face, clr);
					}
					else 
					{
						if (transparentFlag = gobj->tAttr.onFlag)
						{
							if (alpha == 0.0)
								skipFaceFlag = 1; // nothing to draw
							else if (alpha == 1.0)
								transparentFlag = 0; // fully opaque
						}
						switch (face->nVtx) {
						case 1: // degenerate vertices
							if (ctx->eAttr.maxLength > 0.0)		scale = gobj->vAttr.scale * ctx->eAttr.maxLength;
							else								scale = gobj->vAttr.scale;
							if (!gobj->nmlFlag || !gobj->fAttr.orthodrome.enable)
								ds_gl_render_vertex(ctx, gobj, (GUT_POINT*)&v[face->vtx[0]], ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, 0);
							else
								ds_gl_render_great_circle(ctx, gobj, clr, (GUT_POINT*)&v[face->vtx[0]], (GUT_VECTOR*)&gobj->nml[face->nml[0]]);
							if (gobj->lFlags.face)
							{
								ds_label_draw_id(&gobj->fAttr.label, (float)face->draw.centroid.x, (float)face->draw.centroid.y, (float)face->draw.centroid.z + scale * 1.25, &face->draw.normal, face->id, ctx->clrCtl.useLightingFlag);
							}
							break;
						case 2: // degenerate edges

							ds_gl_render_edge(ctx, gobj, (GUT_POINT*)&v[face->vtx[0]].data.xyzw[0], (GUT_POINT*)&v[face->vtx[1]].data.xyzw[0], p, &origin[0], out, edgeNormal, clr, ctx->drawAdj.quality->edgeNSeg);
							if (gobj->lFlags.face)
							{
								ds_label_draw_id(&gobj->fAttr.label, (float)face->draw.centroid.x, (float)face->draw.centroid.y, (float)face->draw.centroid.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, &face->draw.normal, face->id, ctx->clrCtl.useLightingFlag);
							}
							break;
						default:
							if (transparentFlag) //gobj->cAttr.face.transparencyFlag)
							{
								ds_blend_sort_one_face(ctx, gobj, face, mp, &mtx, reverseOrder, matrixID, alpha);  // add face to transparency context
							}
							else if (!skipFaceFlag) // draw
							{
								if (reverseOrder)
									glFrontFace(GL_CW);		// change default order 
								ds_face_draw(ctx, gobj, face, clr, gobj->faceDefault.a);
								if (reverseOrder)
									glFrontFace(GL_CCW);	// switch back order 
								if (gobj->fAttr.label.enable)
									ds_label_draw_id(&gobj->fAttr.label, (float)face->draw.centroid.x, (float)face->draw.centroid.y, (float)face->draw.centroid.z + 0.03, &face->draw.normal, face->id, ctx->clrCtl.useLightingFlag);
							}
						}
					}
				}
			}

			if (gobj->nEdge && gobj->eAttr.draw)
			{
				if (reverseOrder)
					glFrontFace(GL_CW);		// change default order 
				for (i = 0, v = gobj->v_out, edge = gobj->edge; i < gobj->nEdge; ++i, ++edge)
				{
					switch (gobj->cAttr.edge.state) {
					case DS_COLOR_STATE_EXPLICIT: clr = &face->color; break;
					case DS_COLOR_STATE_AUTOMATIC:ds_ctbl_get_color(gobj->ctE, edge->id, &clr);  break;
					case DS_COLOR_STATE_OVERRIDE: clr = &gobj->cAttr.edge.color; break;
					}

					ds_edge_draw(ctx, gobj, edge, clr);

					if (gobj->eAttr.label.enable)
					{
						ds_label_draw_id(&gobj->eAttr.label, (float)edge->draw.middle.x, (float)edge->draw.middle.y, (float)edge->draw.middle.z + ctx->eAttr.maxLength * gobj->eAttr.width * 1.5, &edge->draw.normal, edge->id, ctx->clrCtl.useLightingFlag);
					}
				}
				if (reverseOrder)
					glFrontFace(GL_CCW);	// switch back order 
			}

			if (gobj->vAttr.draw) // render vertices if required
			{
				double		scale;

				clr = &gobj->cAttr.vertex.color;

				if (ctx->eAttr.maxLength > 0.0)	scale = gobj->vAttr.scale * ctx->eAttr.maxLength;
				else								scale = gobj->vAttr.scale;
				v = gobj->vtx;
				if (reverseOrder)
					glFrontFace(GL_CW);		// change default order 
				for (i = 0, v = gobj->vtx; i < gobj->nVtx; ++i, ++v)
				{
					ds_gl_render_vertex(ctx, gobj, (GUT_POINT*)v, (GUT_POINT*)ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, scale, clr, 0);

					if (gobj->vAttr.label.enable)
					{
						cntr = *(GUT_POINT*)v;
						ds_label_draw_id(&gobj->vAttr.label, (float)cntr.x, (float)cntr.y, (float)cntr.z + scale * 1.25, &cntr, i, ctx->clrCtl.useLightingFlag);
					}
				}
				if (reverseOrder)
					glFrontFace(GL_CCW);	// switch back order 
			}
			glPopMatrix();
		}
		++objectID;
	}
	glPopMatrix();

	{ // processing of transparent faces
		int		nNodes, nLevels;
		avl_info(ctx->transparency.avlZSort, &nNodes, &nLevels);
		if (nNodes)
		{
			// reset the modelview matrix
			glMatrixMode(GL_MODELVIEW);
			glPushMatrix();
			glTranslatef(ctx->trans[0], ctx->trans[1], ctx->trans[2]);
			glMultMatrixd((double*)&ctx->matrix);

			// set up transparent blending
			glEnable(GL_ALPHA_TEST);
			glEnable(GL_BLEND);
			glDepthMask(GL_FALSE);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			// traverse the avl tree of faces - back to front z ordered 
			avl_traverse_rtl(ctx->transparency.avlZSort, (void*)ctx, ds_blend_draw_face);
			// restore regular rendering without blending
			glDisable(GL_ALPHA_TEST);
			glDisable(GL_BLEND);
			glDepthMask(GL_TRUE);

			glPopMatrix();
		}
	}

	glBegin(GL_TRIANGLES);

	if (ctx->drawAdj.clipFlag && ctx->drawAdj.clipVisibleFlag)
	{
		glNormal3f((float)0, (float)0, (float)1.0);

		glColor3f((float)1, (float)1, (float)0);
		glVertex3f((float) 1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float) 1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);

		glVertex3f((float)-1.05, (float) 1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float)-1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
		glVertex3f((float) 1.05, (float)-1.05, (float)ctx->drawAdj.clipZValue);
	}

	glEnd();
}

//-----------------------------------------------------------------------------
void ds_face_initialize(DS_CTX *ctx, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	// this function should only be called once when object is created
	// All memory associated with all features enabled is allocated and assigned
	//
	GUT_POINT				*v;
	int						i, j, k, size;
	DS_FACE					*face;
	DS_FACE_DRAWABLE		*draw;
	GUT_POINT				*vMem;
	GUT_VECTOR				*nMem;
	int						nSegments = 360 / 10; // max 
	GUT_VECTOR				*nml, vec[3];
	double					angle, distance;
	GUT_LINE				line;
	char					*mem, *curMem; 

	if (gobj->faceInit)
		return;
	gobj->faceInit = 1; // mark as completed

	// compute the total amount of memory to allocate for the object's face data
	for (i = 0, size = 0, face = gobj->tri; i < gobj->nTri; ++i, ++face)
	{
//		if (face->nVtx < 3)
//			continue;

		// compute the total memory requirement for the face in bytes
		size += sizeof(GUT_POINT)*nSegments * 4; // vto[]/vti[]/vbo[]/vbi[] - all vertices needed for holes based on the max number of segments
		size += sizeof(GUT_VECTOR)*face->nVtx * 2 + sizeof(GUT_VECTOR)*nSegments; // ni[]/no[] - all normals needed for extrusion
		size += sizeof(int)*face->nVtx; // nSeg[] - hold the #divisions for each polygon edge for the hole
		size += sizeof(float)*face->nVtx; // angle[] - hold the #divisions for each polygon edge for the hole
		size += sizeof(GUT_POINT*)*face->nVtx * 2; // vct[]/vcb[] - pointers to corner top and bottom vertices
		size += sizeof(GUT_POINT*)*face->nVtx; //nml[] - pointers to vertex normals
	}
	gobj->faceMem = curMem = mem = (char*)malloc(size); // allocate all memory
	if (!mem)
	{
		char buffer[128];
		sprintf(buffer, "Memory allocation failed - program must exit/abort.");
		MessageBox(ctx->mainWindow, buffer, "Mem Allocation Failure", MB_OK);
		abort();
	}

	// intialize each face and its pointers
	for (i = 0, v = gobj->vtx, nml = gobj->nml, face = gobj->tri; i < gobj->nTri; ++i, ++face)
	{
//		if (face->nVtx >= 3)
		{
			// assign pointers to allocated mem and move the pointer forward
			face->draw.no    = (GUT_VECTOR*)curMem; curMem += sizeof(GUT_VECTOR) * face->nVtx;
			face->draw.ni    = (GUT_VECTOR*)curMem; curMem += sizeof(GUT_VECTOR) * nSegments;
			face->draw.vct   = (GUT_POINT**)curMem; curMem += sizeof(GUT_POINT*) * face->nVtx;
			face->draw.vcb   = (GUT_POINT**)curMem; curMem += sizeof(GUT_POINT*) * face->nVtx;
			face->draw.vto   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
			face->draw.vti   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
			face->draw.vbo   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
			face->draw.vbi   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
			face->draw.nml   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * face->nVtx;
			face->draw.nSeg  = (int*)curMem;        curMem += sizeof(int) * face->nVtx;
			face->draw.angle = (float*)curMem;      curMem += sizeof(float) * face->nVtx;
		}

		// reset 
		face->draw.nSegments = 0;

		// find centroid of polygon
		face->draw.centroid.x = face->draw.centroid.y = face->draw.centroid.z = 0;  // initialize

		for (j = 0; j < face->nVtx; ++j)
		{
			face->draw.centroid.x += v[face->vtx[j]].x;
			face->draw.centroid.y += v[face->vtx[j]].y;
			face->draw.centroid.z += v[face->vtx[j]].z;
			if (gobj->nmlFlag) // copy explicit normals per vertex
				face->draw.nml[j] = *(GUT_VECTOR*)&nml[face->nml[j]];
		}
		face->draw.centroid.w = 1;
		face->draw.centroid.x /= face->nVtx;
		face->draw.centroid.y /= face->nVtx;
		face->draw.centroid.z /= face->nVtx;
		
		if (face->nVtx < 3)
		{
			face->draw.normal = *(GUT_VECTOR*)&face->draw.centroid;
			gut_normalize_vector(&face->draw.normal);
			continue;
		}

		if (face->nVtx < 3)
			continue; // don't consume memory for degenerate faces 

		// continue with regular faces 

//		// assign pointers to allocated mem and move the pointer forward
//		face->draw.no    = (GUT_VECTOR*)curMem; curMem += sizeof(GUT_VECTOR) * face->nVtx;
//		face->draw.ni    = (GUT_VECTOR*)curMem; curMem += sizeof(GUT_VECTOR) * nSegments;
//		face->draw.vct   = (GUT_POINT**)curMem; curMem += sizeof(GUT_POINT*) * face->nVtx;
//		face->draw.vcb   = (GUT_POINT**)curMem; curMem += sizeof(GUT_POINT*) * face->nVtx;
//		face->draw.vto   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
//		face->draw.vti   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
//		face->draw.vbo   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
//		face->draw.vbi   = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * nSegments;
//		face->draw.nSeg  = (int*)curMem;        curMem += sizeof(int) * face->nVtx;
//		face->draw.angle = (float*)curMem;      curMem += sizeof(float) * face->nVtx;

		// determine the number of segments per edge
		for (j = 0; j < face->nVtx; ++j)
		{
			k = (j + 1) % face->nVtx;
			// compute the angle between the vertices along the face edge
			gut_vector(&face->draw.centroid, &v[face->vtx[j]], &vec[0]);
			gut_vector(&face->draw.centroid, &v[face->vtx[k]], &vec[1]);
			gut_normalize_vector(&vec[0]);
			gut_normalize_vector(&vec[1]);
			gut_dot_product(&vec[0], &vec[1], &angle);
			face->draw.angle[j] = angle = acos(angle);
			face->draw.nSeg[j] = angle / DTR(10.0); // (10.0 * 3.141592654 / 180.0); // integer value 
			if(!face->draw.nSeg[j])
				face->draw.nSeg[j] = 1; // minimum
			// accumulate total number of segments
			face->draw.nSegments += face->draw.nSeg[j];
		}

		// find the smallest distance between the centroid and all the polygon edges
		for (j = 0, face->draw.radius = 100000000; j < face->nVtx; ++j)
		{
			k = (j + 1) % face->nVtx;
			line.p1 = v[face->vtx[j]];
			line.p2 = v[face->vtx[k]];
			gut_find_distance_from_point_to_line(&line, &face->draw.centroid, &distance);
			if (distance < face->draw.radius)
				face->draw.radius = distance;
		}

		// find face normals outside and inside
		gut_cross_product_from_triangle_points(&v[face->vtx[0]], &v[face->vtx[1]], &face->draw.centroid,  &face->draw.normal);
		gut_normalize_vector(&face->draw.normal);
		face->draw.rnormal.i = face->draw.normal.i * -1;
		face->draw.rnormal.j = face->draw.normal.j * -1;
		face->draw.rnormal.k = face->draw.normal.k * -1;
	}
}

//-----------------------------------------------------------------------------
void ds_face_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	// this function is called when a face attribute is changed
	// all the geometry information (vertices & normals) is updated
	//
	GUT_POINT				*v;
	int						i, j, k, l, size, cnt[4];
	DS_FACE					*face;
	DS_FACE_DRAWABLE		*draw;
	GUT_POINT				*vMem;
	GUT_VECTOR				*nMem;
	int						nSegments = 36; // 360 / 10; // 10 degrees 
	GUT_VECTOR				*nml;
	GUT_POINT				centroid, p[32], q[2];
	double					t, d[3], radius;
	double					ang, angInc, otherAngle, phi;
	GUT_VECTOR				vec[2], normal;

	// update drawable face components based on attributes

	// 1) make a copy of the original face vertices
	// 2) shrink or expand vertex position from the centroid
	// 3) offset the vertex positions
	// 4) create hole vertices
	// 5) create extrusion vertices
	if (gobj == &ctx->defInputObj)
		return;

	for (i = 0, v = gobj->vtx, nml = gobj->nml, face = gobj->tri; i < gobj->nTri; ++i, ++face)
	{
		if (face->nVtx < 3) // skip degenerate faces
			continue;

		// make local copy of radius and adjust as necessary
		radius = face->draw.radius;
		if (gobj->fAttr.scale.enable)
			radius *= gobj->fAttr.scale.factor;
		if (gobj->fAttr.hole.enable)
			radius *= gobj->fAttr.hole.radius;

		// copy face vertices into local buffer and modify as neccesary
		for (j = 0; j < face->nVtx; ++j)
		{
			// copy
			p[j] = v[face->vtx[j]];

			// normalize
			if(ctx->drawAdj.normalizeFlag)
				gut_normalize_point(&p[j]);

			// shrink or expand position in relation to the centroid
			if (gobj->fAttr.scale.enable)
			{
				gut_parametric_point(&face->draw.centroid, &p[j], &p[j], gobj->fAttr.scale.factor);
			}

			// offset position in the direction of the face normal
			if (gobj->fAttr.offset.enable)
			{
				p[j].x = p[j].x + face->draw.normal.i * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
				p[j].y = p[j].y + face->draw.normal.j * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
				p[j].z = p[j].z + face->draw.normal.k * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
			}
		}

		if (gobj->fAttr.offset.enable) // adjust position of centroid
		{
			centroid.x = face->draw.centroid.x + face->draw.normal.i * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
			centroid.y = face->draw.centroid.y + face->draw.normal.j * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
			centroid.z = face->draw.centroid.z + face->draw.normal.k * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
		}
		else
			centroid = face->draw.centroid;


		cnt[3] = cnt[2] = cnt[1] = cnt[0] = 0;
		for (j = 0; j < face->nVtx; ++j)
		{
			k = (j + 1) % face->nVtx;

			// save the corner of the top outside
			face->draw.vct[j] = &face->draw.vto[cnt[0]];
			if (gobj->nmlFlag) // copy explicit normals per vertex
				face->draw.no[j] = *(GUT_VECTOR*)&gobj->nml[face->nml[j]];

			if (gobj->fAttr.hole.enable)
			{
				if (gobj->fAttr.hole.style != FACE_HOLE_STYLE_POLYGONAL) // round
				{
					// create additional vertices between current vertex and next vertex
					// 1) outer edge
					for (t = l = 0; l < face->draw.nSeg[j]; ++l, t += 1.0 / face->draw.nSeg[j])
					{
						gut_parametric_point(&p[j], &p[k], &face->draw.vto[cnt[0]++], t);
					}

					// 2) inner edge
					gut_distance_from_point_to_point(&centroid, &p[j], &d[0]);
					gut_distance_from_point_to_point(&centroid, &p[k], &d[1]);
					gut_parametric_point(&centroid, &p[j], &q[0], radius / d[0]);
					gut_parametric_point(&centroid, &p[k], &q[1], radius / d[1]);
					gut_distance_from_point_to_point(&q[0], &q[1], &d[0]);
					angInc = face->draw.angle[j] / face->draw.nSeg[j];
					otherAngle = (DTR(180.0) - face->draw.angle[j]) / 2.0;
					for (l = 0; l < face->draw.nSeg[j]; ++l)
					{
						ang = angInc * l;
						if (!l)
							t = 0;
						else
						{
							phi = DTR(180.0) - ang - otherAngle;
							d[1] = sin(ang)*radius / sin(phi);
							t = d[1] / d[0];
						}
						//gut_parametric_point(&p[j], &p[k], &face->draw.vti[cnt[1]], t);
						gut_parametric_point(&q[0], &q[1], &p[31], t);
						//gut_distance_from_point_to_point(&face->draw.centroid, &face->draw.vti[cnt[1]], &d[2]);
						gut_distance_from_point_to_point(&centroid, &p[31], &d[2]);
						gut_parametric_point(&centroid, &p[31], &face->draw.vti[cnt[1]], radius / d[2]);
						++cnt[1];
					}
					face->draw.nSegments = cnt[0];
				}
				else // polygonal
				{
					// 2) inner edge
					face->draw.nSegments = face->nVtx;
					face->draw.vto[cnt[0]++] = p[j];
					gut_distance_from_point_to_point(&centroid, &p[j], &d[0]);
					gut_parametric_point(&centroid, &p[j], &face->draw.vti[cnt[1]++], gobj->fAttr.hole.radius);
				}
			}
			else
			{
				// just copy existing data
//				if (gobj->nmlFlag) // copy explicit normals per vertex
//					face->draw.no[cnt[0]] = *(GUT_VECTOR*)&gobj->nml[face->nml[j]];
				face->draw.vto[cnt[0]++] = p[j];
			}
		}

		// extrusion
		if (gobj->fAttr.extrusion.enable)
		{
			// extrude existing vertices
			// 1) bottom outer edge
			for (j = 0; j < cnt[0]; ++j)
			{
				if (!gobj->fAttr.extrusion.direction) // normal direction
				{
					gut_scale_vector(&face->draw.rnormal, gobj->fAttr.extrusion.factor * ctx->eAttr.maxLength, &normal);
					face->draw.vbo[j].x = face->draw.vto[j].x + normal.i;
					face->draw.vbo[j].y = face->draw.vto[j].y + normal.j;
					face->draw.vbo[j].z = face->draw.vto[j].z + normal.k;
				}
				else // radial
				{
					vec[0] = *(GUT_VECTOR*)&face->draw.vto[j];
					gut_normalize_vector(&vec[0]);
					gut_scale_vector(&vec[0], gobj->fAttr.extrusion.factor * ctx->eAttr.maxLength * -1.0, &normal);
					face->draw.vbo[j].x = face->draw.vto[j].x + normal.i;
					face->draw.vbo[j].y = face->draw.vto[j].y + normal.j;
					face->draw.vbo[j].z = face->draw.vto[j].z + normal.k;
				}
			}
			// 2) bottom inner edge
			for (j = 0; j < cnt[1]; ++j)
			{
				gut_scale_vector(&face->draw.rnormal, gobj->fAttr.extrusion.factor * ctx->eAttr.maxLength, &normal);
				face->draw.vbi[j].x = face->draw.vti[j].x + normal.i;
				face->draw.vbi[j].y = face->draw.vti[j].y + normal.j;
				face->draw.vbi[j].z = face->draw.vti[j].z + normal.k;
			}

			// normal generation
			// outside extrusion
			// fill corner addresses
			for (j = 0; j < face->nVtx; ++j)
			{
				k = face->draw.vct[j] - face->draw.vto;
				face->draw.vcb[j] = &face->draw.vbo[k];
			}
			// use corners
			for (j = 0; j < face->nVtx; ++j)
			{
				k = (j + 1) % face->nVtx;
				// normal
				gut_cross_product_from_triangle_points(face->draw.vct[j], face->draw.vcb[j], face->draw.vct[k], &face->draw.no[j]);
				gut_normalize_vector(&face->draw.no[j]);
			}
			// NORMALS FOR POLYGONAL INTERNAL FACES SHOULD BE DIFFERENT NOT SMOOTH
			// inside extrusion
			// use direction to centroid at each inner vertex
			if (!gobj->fAttr.hole.style) // round
			{
				for (j = 0; j < cnt[1]; ++j)
				{
					// normal
					gut_vector(&face->draw.vti[j], &face->draw.centroid, &face->draw.ni[j]);
					gut_normalize_vector(&face->draw.ni[j]);
				}
			}
			else
			{
				// use corners
				for (j = 0; j < face->nVtx; ++j)
				{
					k = (j + 1) % face->nVtx;
					// normal
					gut_cross_product_from_triangle_points(&face->draw.vti[j], &face->draw.vti[k], &face->draw.vbi[k], &face->draw.ni[j]);
					gut_normalize_vector(&face->draw.no[j]);
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void ds_face_draw(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, DS_COLOR *clr, float alpha)
//-----------------------------------------------------------------------------
{
	// this function is called when a face attribute is changed
	//
	int						i, j, k, l, m;

	if (face->nVtx == 3 && ctx->drawAdj.circleFlag)
	{
		DS_COLOR	color;
		GUT_POINT	origin = { 0,0,0,0 };
		color = *clr;
		color.a = alpha;
		//		face->vto[face->draw.vct[0]]
		//			ds_draw_circle_segment(&p[0], &p[1], &p[2], &normal, clr, &origin[1]);
		ds_draw_circle_segment(face->draw.vct[0], face->draw.vct[1], face->draw.vct[2], &face->draw.normal, &color, &origin);
		return;
	}

	// always draw top of polygon
	if (!gobj->fAttr.hole.enable) // simple case with no hole 
	{
		for (j = 2; j < face->nVtx; ++j) // nVtx == nSegments
		{
			if (gobj->nmlFlag)
			{
				// use normals per vertex
				glBegin(GL_TRIANGLES);
				glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
//				glNormal3f((float)face->draw.no[0].i, (float)face->draw.no[0].j, (float)face->draw.no[0].k);
				glNormal3f((float)face->draw.nml[0].i, (float)face->draw.nml[0].j, (float)face->draw.nml[0].k);
				glVertex3f((float)face->draw.vto[0].x, (float)face->draw.vto[0].y, (float)face->draw.vto[0].z);
//				glNormal3f((float)face->draw.no[j - 1].i, (float)face->draw.no[j - 1].j, (float)face->draw.no[j - 1].k);
				glNormal3f((float)face->draw.nml[j-1].i, (float)face->draw.nml[j-1].j, (float)face->draw.nml[j-1].k);
				glVertex3f((float)face->draw.vto[j - 1].x, (float)face->draw.vto[j - 1].y, (float)face->draw.vto[j - 1].z);
//				glNormal3f((float)face->draw.no[j].i, (float)face->draw.no[j].j, (float)face->draw.no[j].k);
				glNormal3f((float)face->draw.nml[j].i, (float)face->draw.nml[j].j, (float)face->draw.nml[j].k);
				glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
				glEnd();
			}
			else
			{
				// use face normal
				glBegin(GL_TRIANGLES);
				glNormal3f((float)face->draw.normal.i, (float)face->draw.normal.j, (float)face->draw.normal.k);
				glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
				glVertex3f((float)face->draw.vto[0].x, (float)face->draw.vto[0].y, (float)face->draw.vto[0].z);
				glVertex3f((float)face->draw.vto[j - 1].x, (float)face->draw.vto[j - 1].y, (float)face->draw.vto[j - 1].z);
				glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
				glEnd();
			}
		}
	}
	else // draw hole
	{
		// hole style is not relevant - all hole data is same format/sequence INCIRREC
	//	if (!gobj->fAttr.hole.style) // round
		for (j = 0; j < face->draw.nSegments; ++j)
		{
			k = (j + 1) % face->draw.nSegments;
			glBegin(GL_TRIANGLES);
			// use face normal
			glNormal3f((float)face->draw.normal.i, (float)face->draw.normal.j, (float)face->draw.normal.k);
			glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
			// top tri 1  vto-j, vto-k, vti-k
			glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
			glVertex3f((float)face->draw.vto[k].x, (float)face->draw.vto[k].y, (float)face->draw.vto[k].z);
			glVertex3f((float)face->draw.vti[k].x, (float)face->draw.vti[k].y, (float)face->draw.vti[k].z);
			// top tri 2  vto-j, vti-k, vti-j
			glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
			glVertex3f((float)face->draw.vti[k].x, (float)face->draw.vti[k].y, (float)face->draw.vti[k].z);
			glVertex3f((float)face->draw.vti[j].x, (float)face->draw.vti[j].y, (float)face->draw.vti[j].z);
			glEnd();
		}
		if (gobj->fAttr.hole.style > FACE_HOLE_STYLE_POLYGONAL)
			ds_gl_spherical_bubble(ctx, gobj, face, clr);
	}

	if (gobj->fAttr.extrusion.enable)
	{
		if (gobj->fAttr.hole.enable)
		{
			// inside extrusion
			if (!gobj->fAttr.hole.style) // round
			{
				for (j = 0; j < face->draw.nSegments; ++j)
				{
					k = (j + 1) % face->draw.nSegments;
					glBegin(GL_TRIANGLES);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);

					glNormal3f((float)face->draw.ni[j].i, (float)face->draw.ni[j].j, (float)face->draw.ni[j].k);
					glVertex3f((float)face->draw.vti[j].x, (float)face->draw.vti[j].y, (float)face->draw.vti[j].z);
					glNormal3f((float)face->draw.ni[k].i, (float)face->draw.ni[k].j, (float)face->draw.ni[k].k);
					glVertex3f((float)face->draw.vti[k].x, (float)face->draw.vti[k].y, (float)face->draw.vti[k].z);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);

					glNormal3f((float)face->draw.ni[j].i, (float)face->draw.ni[j].j, (float)face->draw.ni[j].k);
					glVertex3f((float)face->draw.vti[j].x, (float)face->draw.vti[j].y, (float)face->draw.vti[j].z);
					glNormal3f((float)face->draw.ni[k].i, (float)face->draw.ni[k].j, (float)face->draw.ni[k].k);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);
					glNormal3f((float)face->draw.ni[j].i, (float)face->draw.ni[j].j, (float)face->draw.ni[j].k);
					glVertex3f((float)face->draw.vbi[j].x, (float)face->draw.vbi[j].y, (float)face->draw.vbi[j].z);
					glEnd();
				}
			}
			else
			{
				for (j = 0; j < face->draw.nSegments; ++j)
				{
					k = (j + 1) % face->draw.nSegments;
					glBegin(GL_TRIANGLES);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);

					glNormal3f((float)face->draw.ni[j].i, (float)face->draw.ni[j].j, (float)face->draw.ni[j].k);
					glVertex3f((float)face->draw.vti[j].x, (float)face->draw.vti[j].y, (float)face->draw.vti[j].z);
					glVertex3f((float)face->draw.vti[k].x, (float)face->draw.vti[k].y, (float)face->draw.vti[k].z);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);

					glNormal3f((float)face->draw.ni[j].i, (float)face->draw.ni[j].j, (float)face->draw.ni[j].k);
					glVertex3f((float)face->draw.vti[j].x, (float)face->draw.vti[j].y, (float)face->draw.vti[j].z);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);
					glVertex3f((float)face->draw.vbi[j].x, (float)face->draw.vbi[j].y, (float)face->draw.vbi[j].z);
					glEnd();
				}
			}
			if (gobj->fAttr.extrusion.holeOnly)
				return;
			// outside extrusion
			if (!gobj->fAttr.extrusion.direction) // normal direction
			{
				for (j = 0; j < face->nVtx; ++j)
				{
					k = (j + 1) % face->nVtx;
					// use face normal
					glBegin(GL_TRIANGLES);
					glNormal3f((float)face->draw.no[j].i, (float)face->draw.no[j].j, (float)face->draw.no[j].k);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
					glVertex3f((float)face->draw.vct[j]->x, (float)face->draw.vct[j]->y, (float)face->draw.vct[j]->z);
					glVertex3f((float)face->draw.vcb[j]->x, (float)face->draw.vcb[j]->y, (float)face->draw.vcb[j]->z);
					glVertex3f((float)face->draw.vcb[k]->x, (float)face->draw.vcb[k]->y, (float)face->draw.vcb[k]->z);

					glVertex3f((float)face->draw.vct[j]->x, (float)face->draw.vct[j]->y, (float)face->draw.vct[j]->z);
					glVertex3f((float)face->draw.vcb[k]->x, (float)face->draw.vcb[k]->y, (float)face->draw.vcb[k]->z);
					glVertex3f((float)face->draw.vct[k]->x, (float)face->draw.vct[k]->y, (float)face->draw.vct[k]->z);
					glEnd();
				}
			}
			else // radial
			{
				for (j = 0, l = m = 0; j < face->draw.nSegments; ++j)
				{
					k = (j + 1) % face->draw.nSegments;
					glBegin(GL_TRIANGLES);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);

					if (&face->draw.vto[j] == face->draw.vct[l]) // l = 1 
					{
						m = l;
						++l;
						l = l % face->nVtx;
					}
					glNormal3f((float)face->draw.no[m].i, (float)face->draw.no[m].j, (float)face->draw.no[m].k);

					glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
					glVertex3f((float)face->draw.vbo[j].x, (float)face->draw.vbo[j].y, (float)face->draw.vbo[j].z);
					glVertex3f((float)face->draw.vbo[k].x, (float)face->draw.vbo[k].y, (float)face->draw.vbo[k].z);

					glVertex3f((float)face->draw.vto[j].x, (float)face->draw.vto[j].y, (float)face->draw.vto[j].z);
					glVertex3f((float)face->draw.vbo[k].x, (float)face->draw.vbo[k].y, (float)face->draw.vbo[k].z);
					glVertex3f((float)face->draw.vto[k].x, (float)face->draw.vto[k].y, (float)face->draw.vto[k].z);

					glEnd();
				}
			}
		}
		else // no hole
		{
			// outside extrusion
			for (j = 0; j < face->nVtx; ++j)
			{
				k = (j + 1) % face->nVtx;
				// use face normal
				glBegin(GL_TRIANGLES);
				glNormal3f((float)face->draw.no[j].i, (float)face->draw.no[j].j, (float)face->draw.no[j].k);
				glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
				glVertex3f((float)face->draw.vct[j]->x, (float)face->draw.vct[j]->y, (float)face->draw.vct[j]->z);
				glVertex3f((float)face->draw.vcb[j]->x, (float)face->draw.vcb[j]->y, (float)face->draw.vcb[j]->z);
				glVertex3f((float)face->draw.vcb[k]->x, (float)face->draw.vcb[k]->y, (float)face->draw.vcb[k]->z);

				glVertex3f((float)face->draw.vct[j]->x, (float)face->draw.vct[j]->y, (float)face->draw.vct[j]->z);
				glVertex3f((float)face->draw.vcb[k]->x, (float)face->draw.vcb[k]->y, (float)face->draw.vcb[k]->z);
				glVertex3f((float)face->draw.vct[k]->x, (float)face->draw.vct[k]->y, (float)face->draw.vct[k]->z);
				glEnd();
			}
		}

		if (gobj->fAttr.extrusion.bothSides)
		{
			if (gobj->fAttr.hole.enable) {
				// backside extrusion
				for (j = 0; j < face->draw.nSegments; ++j)
				{
					k = (j + 1) % face->draw.nSegments;
					glBegin(GL_TRIANGLES);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);

					glNormal3f((float)face->draw.rnormal.i, (float)face->draw.rnormal.j, (float)face->draw.rnormal.k);
					glVertex3f((float)face->draw.vbo[j].x, (float)face->draw.vbo[j].y, (float)face->draw.vbo[j].z);
					glVertex3f((float)face->draw.vbi[j].x, (float)face->draw.vbi[j].y, (float)face->draw.vbi[j].z);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);

					glVertex3f((float)face->draw.vbo[j].x, (float)face->draw.vbo[j].y, (float)face->draw.vbo[j].z);
					glVertex3f((float)face->draw.vbi[k].x, (float)face->draw.vbi[k].y, (float)face->draw.vbi[k].z);
					glVertex3f((float)face->draw.vbo[k].x, (float)face->draw.vbo[k].y, (float)face->draw.vbo[k].z);

					glEnd();
				}
			}
			else // simple
			{
				for (j = 2; j < face->nVtx; ++j) // nVtx == nSegments
				{
					// use face normal
					glBegin(GL_TRIANGLES);
					glNormal3f((float)face->draw.rnormal.i, (float)face->draw.rnormal.j, (float)face->draw.rnormal.k);
					glColor4f((float)clr->r, (float)clr->g, (float)clr->b, alpha);
					glVertex3f((float)face->draw.vbo[0].x, (float)face->draw.vbo[0].y, (float)face->draw.vbo[0].z);
					glVertex3f((float)face->draw.vbo[j].x, (float)face->draw.vbo[j].y, (float)face->draw.vbo[j].z);
					glVertex3f((float)face->draw.vbo[j - 1].x, (float)face->draw.vbo[j - 1].y, (float)face->draw.vbo[j - 1].z);
					glEnd();
				}
			}
		}
	}
}

//-----------------------------------------------------------------------------
void ds_edge_initialize(DS_CTX *ctx, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	// this function should only be called once when object is created
	// All memory associated with all edge features enabled is allocated and assigned
	//
	GUT_POINT				*v;
	int						i, j, k, size;
	DS_EDGE					*edge;
	DS_FACE_DRAWABLE		*draw;
	GUT_POINT				*vMem;
	GUT_VECTOR				*nMem;
	int						nSegments = 360 / 10; // max 
	GUT_VECTOR				*nml, vec[3];
	double					angle, distance;
	GUT_LINE				line;
	char					*mem, *curMem;

	if (gobj->edgeInit)
		return;
	gobj->edgeInit = 1; // mark as completed
//	gobj->eAttr.label.enable = 1;
//	gobj->eAttr.label.font = GLUT_BITMAP_HELVETICA_12;

	// max memory size = nEdges * 24 vertices + 12 normals
	// compute the total memory requirement for the edges in bytes
	size  = 0;
	size += sizeof(GUT_POINT) * 24; 
	size += sizeof(GUT_VECTOR) * 12;
	size *= gobj->nEdge;
	gobj->edgeMem = curMem = mem = (char*)malloc(size); // allocate all memory

	// assign pointers for each edge
	for (i = 0, size = 0, edge = gobj->edge; i < gobj->nEdge; ++i, ++edge)
	{
		edge->draw.n = (GUT_VECTOR*)curMem; curMem += sizeof(GUT_VECTOR) * 12;
		edge->draw.v = (GUT_POINT*)curMem;  curMem += sizeof(GUT_POINT) * 24;
	}
}

//-----------------------------------------------------------------------------
void ds_edge_update(DS_CTX *ctx, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	// this function is called when a face attribute is changed
	// all the geometry information (vertices & normals) is updated
	//
	GUT_POINT		*v = gobj->vtx;
	DS_EDGE			*edge;
	GUT_POINT		p[2], origin = { 0,0,0 };
	int				i, nSeg = ctx->drawAdj.quality->edgeNSeg;

	if (gobj == &ctx->defInputObj)
		return;

	for (i = 0, edge = gobj->edge; i < gobj->nEdge; ++i, ++edge)
	{
		// copy vertex data to new variables
		p[0] = v[edge->vtx[0]];
		p[1] = v[edge->vtx[1]];

		// check for special flag to re-normalize
		if (ctx->drawAdj.normalizeFlag)//if (ctx->global_normalize)
		{
			gut_normalize_point(&p[0]);
			gut_normalize_point(&p[1]);
		}

		gut_mid_point((GUT_POINT*)&p[0], (GUT_POINT*)&p[1], &edge->draw.middle);
		edge->draw.normal = *(GUT_VECTOR*)&edge->draw.middle;
		gut_normalize_vector(&edge->draw.normal);

		if (gobj->eAttr.type == GEOMETRY_EDGE_SQUARE)
		{
			ds_geo_edge_to_triangles_new(ctx, &gobj->eAttr, &p[0], &p[1], edge->draw.v, edge->draw.n, ctx->drawAdj.normalizeFlag, &origin);
		}
		else
		{
			ds_geo_edge_to_triangles_hex(ctx, &gobj->eAttr, &p[0], &p[1], edge->draw.v + 0, edge->draw.v + nSeg, edge->draw.n, ctx->drawAdj.normalizeFlag, &origin, nSeg);
		}
	}
}

//-----------------------------------------------------------------------------
void ds_edge_draw(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_EDGE *edge, DS_COLOR *clr)
//-----------------------------------------------------------------------------
{
	//glBegin(GL_TRIANGLES);
	if (gobj->eAttr.type == GEOMETRY_EDGE_SQUARE)
	{
		if (gobj->eAttr.arcEnable)
		{
			// generate geometry on the fly
			ds_geo_edge_to_triangles_arc(ctx, &gobj->eAttr, &gobj->vtx[edge->vtx[0]], &gobj->vtx[edge->vtx[1]], clr);
		}
		else if (ctx->eAttr.height != 0.0)
		{
//			ds_render_edge(ctx, &gobj->eAttr, &gobj->vtx[edge->vtx[0]], &gobj->vtx[edge->vtx[1]], clr);
			static int	i, j, k, l, m;
			int			nSeg = 4;

			glBegin(GL_TRIANGLES);
			for (i = 0, j = nSeg; i < nSeg; ++i, ++j)
			{
				glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
				glNormal3f((float)edge->draw.n[i % nSeg].i, (float)edge->draw.n[i % nSeg].j, (float)edge->draw.n[i % nSeg].k);
				glVertex3f((float)edge->draw.v[i].x, (float)edge->draw.v[i].y, (float)edge->draw.v[i].z);
				glVertex3f((float)edge->draw.v[j].x, (float)edge->draw.v[j].y, (float)edge->draw.v[j].z);
				k = ( j + 1 ) % nSeg + nSeg;
				l = (i + 1) % nSeg;
				glVertex3f((float)edge->draw.v[k].x, (float)edge->draw.v[k].y, (float)edge->draw.v[k].z);
				glVertex3f((float)edge->draw.v[k].x, (float)edge->draw.v[k].y, (float)edge->draw.v[k].z);
				glVertex3f((float)edge->draw.v[l].x, (float)edge->draw.v[l].y, (float)edge->draw.v[l].z);
				glVertex3f((float)edge->draw.v[i].x, (float)edge->draw.v[i].y, (float)edge->draw.v[i].z);
			}
			glEnd();
		}
	}
	else
	{
		if (gobj->eAttr.arcEnable)
		{
			// generate geometry on the fly
			ds_geo_edge_to_triangles_hex_arc(ctx, &gobj->eAttr, &gobj->vtx[edge->vtx[0]], &gobj->vtx[edge->vtx[1]], clr);
		}
		else
		{
			static int	i, j, k, l;
			int			nSeg = ctx->drawAdj.quality->edgeNSeg;
			glBegin(GL_TRIANGLES);
			for (i = 0, j = nSeg; i < nSeg; ++i, ++j)
			{
				glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
				glNormal3f((float)edge->draw.n[i].i, (float)edge->draw.n[i].j, (float)edge->draw.n[i].k);
				glVertex3f((float)edge->draw.v[i].x, (float)edge->draw.v[i].y, (float)edge->draw.v[i].z);
				glNormal3f((float)edge->draw.n[i].i, (float)edge->draw.n[i].j, (float)edge->draw.n[i].k);
				glVertex3f((float)edge->draw.v[j].x, (float)edge->draw.v[j].y, (float)edge->draw.v[j].z);
				k = (j + 1) % nSeg + nSeg;
				l = (i + 1) % nSeg;
				glNormal3f((float)edge->draw.n[l].i, (float)edge->draw.n[l].j, (float)edge->draw.n[l].k);
				glVertex3f((float)edge->draw.v[k].x, (float)edge->draw.v[k].y, (float)edge->draw.v[k].z);

				glNormal3f((float)edge->draw.n[l].i, (float)edge->draw.n[l].j, (float)edge->draw.n[l].k);
				glVertex3f((float)edge->draw.v[k].x, (float)edge->draw.v[k].y, (float)edge->draw.v[k].z);
				glNormal3f((float)edge->draw.n[l].i, (float)edge->draw.n[l].j, (float)edge->draw.n[l].k);
				glVertex3f((float)edge->draw.v[l].x, (float)edge->draw.v[l].y, (float)edge->draw.v[l].z);
				glNormal3f((float)edge->draw.n[i].i, (float)edge->draw.n[i].j, (float)edge->draw.n[i].k);
				glVertex3f((float)edge->draw.v[i].x, (float)edge->draw.v[i].y, (float)edge->draw.v[i].z);
			}
			glEnd();
		}
	}
}
//-----------------------------------------------------------------------------
ds_find_great_circle_matrix(DS_CTX *ctx, GUT_POINT *p, GUT_VECTOR *n, double *m, double radius)
//-----------------------------------------------------------------------------
{
	// Generate the correct orientation matrix based on the normal
	// p = position coordinate
	// n = normal at position
	int				i, j, k;
	GUT_VECTOR		x, y, z, nn;
	double			zero = 0.00000001;

	// first - check size of normal components
	if (fabs(n->i) < zero)
		i = 0;
	else if (n->i > 0)
		i = 1;
	else
		i = -1;

	if (fabs(n->j) < zero)
		j = 0;
	else if (n->j > 0)
		j = 1;
	else
		j = -1;

	if (fabs(n->k) < zero)
		k = 0;
	else if (n->k > 0)
		k = 1;
	else
		k = -1;

	// n is the original normal
	// its size will be a radius 
	radius = sqrt(n->i * n->i + n->j * n->j + n->k * n->k);
					// 
	nn = *n;
	gut_normalize_vector(&nn);

	if (i == 1 && j == 0 && k == 0)
	{
		if (nn.i > 0)
		{
			x.i = 0, x.j = 1, x.k = 0;
			y.i = 0, y.j = 0, y.k = 1;
			z.i = 1, z.j = 0, z.k = 0;
		}
		else
		{
			x.i = 0, x.j = 0, x.k = -1;
			y.i = 0, y.j = -1, y.k = 0;
			z.i = -1, z.j = 0, z.k = 0;
		}
	}
	else if (i == -1 && j == 0 && k == 0)
	{
		x.i = 0, x.j = 0, x.k = 1;
		y.i = 0, y.j = 1, y.k = 0;
		z.i = -1, z.j = 0, z.k = 0;
	}
	else if (i == 0 && j == 1 && k == 0)
	{
		x.i = 0, x.j = 0, x.k = 1;
		y.i = 1, y.j = 0, y.k = 0;
		z.i = 0, z.j = 1, z.k = 0;
	}
	else if (i == 0 && j == -1 && k == 0)
	{
		x.i = 1, x.j = 0, x.k = 0;
		y.i = 0, y.j = 0, y.k = 1;
		z.i = 0, z.j = -1, z.k = 0;
	}
	else if (i == 0 && j == 0 && k == 1)
	{
		x.i = 1, x.j = 0, x.k = 0;
		y.i = 0, y.j = 1, y.k = 0;
		z.i = 0, z.j = 0, z.k = 1;
	}
	else if (i == 0 && j == 0 && k == -1)
	{
		x.i = 0, x.j = 1, x.k = 0;
		y.i = 1, y.j = 0, y.k = 0;
		z.i = 0, z.j = 0, z.k = -1;
		nn = z;
	}
	else if (i != 0 && j != 0 && k == 0)
	{
		z = nn;
		x.i = -nn.j, x.j = nn.i, x.k = 0;
		gut_normalize_vector(&x);
		gut_cross_product(&nn, &x, &y);
	}
	else if (i == 0 && j != 0 && k != 0)
	{
		z = nn;
		x.i = -nn.j, x.j = nn.i, x.k = 0;
		gut_normalize_vector(&x);
		gut_cross_product(&nn, &x, &y);
	}
	else if (i != 0 && j == 0 && k != 0)
	{
		z = nn;
		x.i = -nn.j, x.j = nn.i, x.k = 0;
		gut_normalize_vector(&x);
		gut_cross_product(&nn, &x, &y);
	}
	else
	{
		z = nn;
		x.i = -nn.j, x.j = nn.i, x.k = 0;
		gut_normalize_vector(&x);
		gut_cross_product(&nn, &x, &y);
	}

	//build rotation matrix from vectors
	if (0)
	{
		m[0] = x.i,  m[1] = y.i, m[2] = z.i, m[3] = 0;
		m[4] = x.j,  m[5] = y.j, m[6] = z.j, m[7] = 0;
		m[8] = x.k,  m[9] = y.k, m[10] = z.k, m[11] = 0;
		m[12] = p->x,  m[13] = p->y, m[14] = p->z, m[15] = 1;
	}
	else
	{
		m[0] = x.i,  m[1] = x.j, m[2] = x.k, m[3] = 0;
		m[4] = y.i,  m[5] = y.j, m[6] = y.k, m[7] = 0;
		m[8] = z.i,  m[9] = z.j, m[10] = z.k, m[11] = 0;
		m[12] = p->x,  m[13] = p->y, m[14] = p->z, m[15] = 1;
	}
}
//-----------------------------------------------------------------------------
ds_gl_render_great_circle(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_COLOR *clr, GUT_POINT *vertex, GUT_VECTOR *normal)
//-----------------------------------------------------------------------------
{
	// draw a greater or lesser circle at vertex position in direction perpindicular to normal
	// option 1
	// circle radius is defined
	// circle radius is based on vertex position (lesser circle) (R=1,d, r = sqrt(R^2+d^2))
	// circle segmentation is object global
	// size of ring is object global
	// create profile (triangle)
	// move to correct position on x axis (r)
	// sweep profile on z axis in number of segments (mod 8)
	// rotate entire ring based on normal
	// draw segments

	// geometry of circle belongs to object (one copy)
	// only generate the data once
	//
	static GUT_POINT	v[720], vtx, origin = { 0,0,0,1 };
	static GUT_VECTOR	n[720], nml;
	GUT_PLANE			pl;
	double				depth1 = gobj->fAttr.orthodrome.depth1, depth2 = gobj->fAttr.orthodrome.depth2, height = gobj->fAttr.orthodrome.height, ainc, h;
	int					i, j, k, l, kk, ll, kbase, lbase, q; //, nseg, NSEG
	int					nseg = 5, NSEG = 144;
	MTX_MATRIX			m, orientation;
	double				distance, radius;
	int					style=gobj->fAttr.orthodrome.style;

	// need to perform offset and scaling 
	// get face vertex & normal
	vtx = *vertex;
	nml = *normal;

	// move the vertex by the offset 
	if (gobj->fAttr.offset.enable)
	{
		vtx.x += nml.i * gobj->fAttr.offset.factor;
		vtx.y += nml.j * gobj->fAttr.offset.factor;
		vtx.z += nml.k * gobj->fAttr.offset.factor;
	}
	// determine plane equation at vertex with given normal
	gut_plane_from_point_normal(&vtx, &nml, &pl);

	// determine the point on the plane closest to the origin
	// it may be different than the user provided point
	gut_find_point_plane_intersection(&pl, &origin, &v[0]);
	// copy back corrected vertex at center of circle
	vtx = v[0];

	// determine plane equation at vertex with given normal
	ds_find_great_circle_matrix(ctx, &vtx, &nml, &orientation, 1.0);

//	// determine the point on the plane closest to the origin
//	// it may be different than the user provided point
//	gut_find_point_plane_intersection(&orientation, &vtx, &v[0]);
//	// copy back && reset the orientation matrix
//	vtx = v[0];
//	orientation.data.array[12] = vtx.x;
//	orientation.data.array[13] = vtx.y;
//	orientation.data.array[14] = vtx.z;

	distance = sqrt(vtx.x * vtx.x + vtx.y * vtx.y + vtx.z * vtx.z);
	radius = sqrt(1.0 - vtx.x * vtx.x - vtx.y * vtx.y - vtx.z * vtx.z);
	if (gobj->fAttr.scale.enable)
		radius *= gobj->fAttr.scale.factor;
	ainc = asin(distance);

	// set depth1 to zero if two sided disc to avoid geometry issues with small circles
	depth1 = style == ORTHODROME_STYLE_DISC_TWO_SIDED ? 0 : depth1;

	// rim - use width & height
	// one sided disc - use center rim only
	// two sided disc - force width to zero 
	// create geometry
	// storage: vertex[size], normal[size]
	// size = 3 * NSEG
	// create the base geometry
	v[0].x = 0,			v[0].y = 0,		v[0].z = 0, v[0].w = 1.0;
	v[1].x = -depth1,	v[1].y = 0,		v[1].z =  height / 2.0, v[1].w = 1.0;
	v[2].x = -depth2,	v[2].y = 0,		v[2].z =  height / 2.0, v[2].w = 1.0;
	v[3].x = -depth2,	v[3].y = 0,		v[3].z = -height / 2.0, v[2].w = 1.0;
	v[4].x = -depth1,	v[4].y = 0,		v[4].z = -height / 2.0, v[2].w = 1.0;
	n[0].i = height,	n[0].j = 0,		n[0].k = depth1;	
	n[1].i =  0.0,		n[1].j = 0,		n[1].k =  1.0;		
	n[2].i = -1.0,		n[2].j = 0,		n[2].k =  0.0;		
	n[3].i =  0.0,		n[3].j = 0,		n[3].k = -1.0;		
	n[4].i = height,	n[4].j = 0,		n[4].k = -depth1;	
	gut_normalize_vector(&n[0]);
	gut_normalize_vector(&n[4]);
	if (ainc != 0.0)
	{
		double	s = sin(ainc), c = cos(ainc), x, z;
		GUT_POINT	t = *vertex;
		gut_normalize_point(&t);
		// need to compare the distance from point to origin versus the point with the normal added
		t.x += normal->i;
		t.y += normal->j;
		t.z += normal->k;

		if (fabs(t.x) < 0.001 && fabs(t.y) < 0.001 &&fabs(t.z) < 0.001)
		{
			s = sin(-ainc);
			c = cos(-ainc);
		}
		for (i = 0; i < 5; ++i)
		{
			x = v[i].x*c - v[i].z*s;
			z = v[i].x*s + v[i].z*c;
			v[i].x = x;
			v[i].z = z;
			x = n[i].i*c - n[i].k*s;
			z = n[i].i*s + n[i].k*c;
			n[i].i = x;
			n[i].k = z;
		}
	}
	h = fabs(v[1].z);
	v[0].x += radius; // move out along the x axis 
	v[1].x += radius; // move out along the x axis 
	v[2].x += radius; // move out along the x axis 
	v[3].x += radius; // move out along the x axis 
	v[4].x += radius; // move out along the x axis 
	// sweep around z axis
	ainc = DTR(360.0) / NSEG;
	mtx_create_rotation_matrix(&m, MTX_ROTATE_Z_AXIS, ainc); //MTX_MATRIX *m, int axis, double angle )
	for (i = 0, j = 0, k = nseg; i < NSEG - 1; ++i, j += nseg, k += nseg)
	{
		mtx_vector_multiply(nseg, (MTX_VECTOR*)&v[j], (MTX_VECTOR*)&v[k], &m);
		mtx_vector_multiply(nseg, (MTX_VECTOR*)&n[j], (MTX_VECTOR*)&n[k], &m);
	}
//	ds_find_great_circle_matrix(ctx, vertex, normal, &orientation, 1.0);
//	ds_find_great_circle_matrix(ctx, &vtx, &nml, &orientation, 1.0);
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glMultMatrixd((GLdouble*)&orientation);

	// push required matrices 
	// scale overall (for lesser circles)
	// rotation matrix for orientation
	//
	// render geometry
	unsigned char	pattern;
	if (gobj->fAttr.orthodrome.dashEnable)
		pattern = 1 | 1 << 2 | 1 << 4 | 1 << 6; // oxff
	else
		pattern = 0xff;
//	glBegin(GL_TRIANGLES);
	glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
	switch (style) {
	case ORTHODROME_STYLE_RIM:
		glBegin(GL_TRIANGLES);
		if (1)//(depth1 != 0.0)
		{
			for (q = 0, i = 0, lbase = nseg, kbase = 0; i < NSEG; ++i, lbase += nseg, kbase += nseg)
			{
				if (q > 7)
					q = 0;
				if (i == NSEG - 1)
				{
					lbase = 0;
				}
				if (!((1 << q++) & pattern))
					continue;
				for (j = 0, k = kbase, kk = kbase + 1, l = lbase, ll = lbase + 1; j < nseg; ++j, ++k, ++l, ++kk, ++ll)
				{
					if (j == nseg - 1)
					{
						kk = kbase;
						ll = lbase;
					}
					// triangle 1
					glNormal3f((float)n[k].i, (float)n[k].j, (float)n[k].k);
					glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
					glNormal3f((float)n[l].i, (float)n[l].j, (float)n[l].k);
					glVertex3f((float)v[l].x, (float)v[l].y, (float)v[l].z);
					glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
					// triangle 2
					glNormal3f((float)n[l].i, (float)n[l].j, (float)n[l].k);
					glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
					glNormal3f((float)n[k].i, (float)n[k].j, (float)n[k].k);
					glVertex3f((float)v[kk].x, (float)v[kk].y, (float)v[kk].z);
					glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
				}
			}
		}
		else
		{
			for (q = 0, i = 0, lbase = nseg, kbase = 0; i < NSEG; ++i, lbase += nseg, kbase += nseg)
			{
				if (q > 7)
					q = 0;
				if (i == NSEG - 1)
				{
					lbase = 0;
				}
				if (!((1 << q++) & pattern))
					continue;
				for (j = 0, k = kbase, kk = kbase + 1, l = lbase, ll = lbase + 1; j < nseg; ++j, ++k, ++l, ++kk, ++ll)
				{
					if (j == nseg - 1)
					{
						kk = kbase;
						ll = lbase;
					}
					if (j != 1)
						continue;
					// triangle 1
					glNormal3f((float)-n[k].i, (float)-n[k].j, (float)-n[k].k);
					glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
					glNormal3f((float)-n[l].i, (float)-n[l].j, (float)-n[l].k);
					glVertex3f((float)v[l].x, (float)v[l].y, (float)v[l].z);
					glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
					// triangle 2
					glNormal3f((float)-n[l].i, (float)-n[l].j, (float)-n[l].k);
					glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
					glNormal3f((float)-n[k].i, (float)-n[k].j, (float)-n[k].k);
					glVertex3f((float)v[kk].x, (float)v[kk].y, (float)v[kk].z);
					glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
				}
			}
		}
		glEnd();
		break;
	case ORTHODROME_STYLE_DISC_ONE_SIDED:
		// draw single disc at exact center
		// disregard depth & height
		//
		//
		glBegin(GL_TRIANGLES);
		glNormal3f((float)0, (float)0, (float)1.0);
		for (i = 1, j = 0, k = 5; i < NSEG; ++i, j += 5, k += 5)
		{
			// draw v[j], origin, v[k]
			glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
			glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
			glVertex3f((float)0, (float)0, (float)0.0);// (float)height / 2);
		}
		glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
		glVertex3f((float)v[0].x, (float)v[0].y, (float)v[0].z);
		glVertex3f((float)0, (float)0, (float)0);
		glEnd();
		break;
	case ORTHODROME_STYLE_DISC_TWO_SIDED:
		// draw double disc at upper and lower points  center
		glBegin(GL_TRIANGLES);
		glNormal3f((float)0, (float)0, (float)-1.0);
		for (i = 1, j = 4, k = 9; i < NSEG; ++i, j += 5, k += 5)
		{
			glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
			glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
			glVertex3f((float)0, (float)0, (float)-h);
		}
		glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
		glVertex3f((float)v[9].x, (float)v[9].y, (float)v[9].z);
		glVertex3f((float)0, (float)0, (float)-h);
		glNormal3f((float)0, (float)0, (float)1.0);
		for (i = 1, j = 1, k = 6; i < NSEG; ++i, j += 5, k += 5)
		{
			glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
			glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
			glVertex3f((float)0, (float)0, (float)h);
		}
		glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
		glVertex3f((float)v[6].x, (float)v[6].y, (float)v[6].z);
		glVertex3f((float)0, (float)0, (float)h);
		for (q = 0, i = 0, lbase = nseg, kbase = 0; i < NSEG; ++i, lbase += nseg, kbase += nseg)
		{
			if (q > 7)
				q = 0;
			if (i == NSEG - 1)
			{
				lbase = 0;
			}
			if (!((1 << q++) & pattern))
				continue;
			for (j = 0, k = kbase, kk = kbase + 1, l = lbase, ll = lbase + 1; j < nseg; ++j, ++k, ++l, ++kk, ++ll)
			{
				if (j == nseg - 1)
				{
					kk = kbase;
					ll = lbase;
				}
				if (j != 0 && j != 4)
					continue;
				// triangle 1
				glNormal3f((float)n[k].i, (float)n[k].j, (float)n[k].k);
				glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
				glNormal3f((float)n[l].i, (float)n[l].j, (float)n[l].k);
				glVertex3f((float)v[l].x, (float)v[l].y, (float)v[l].z);
				glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
				// triangle 2
				glNormal3f((float)n[l].i, (float)n[l].j, (float)n[l].k);
				glVertex3f((float)v[ll].x, (float)v[ll].y, (float)v[ll].z);
				glNormal3f((float)n[k].i, (float)n[k].j, (float)n[k].k);
				glVertex3f((float)v[kk].x, (float)v[kk].y, (float)v[kk].z);
				glVertex3f((float)v[k].x, (float)v[k].y, (float)v[k].z);
			}
		}
		glEnd();
		break;
	}
//	glEnd();
	glPopMatrix();
	return;
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_arc(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, DS_COLOR *clr)
//-----------------------------------------------------------------------------------
{
	// create new points that represent triangles around edge
	GUT_PLANE			pl;
	GUT_VECTOR			v, vr, line, no[2], ni[2];
	double				d;
	GUT_POINT			aa0, aa, bb, o = { 0,0,0,1 }, c, f;
	static GUT_POINT	origin = { 0,0,0,1 };
	GUT_POINT			out[32];
	int					nArcSeg, offsetFlag=0;

	// copy input vertices
	aa = *a;
	bb = *b;
	// normalizing is a requirement for the arc
	gut_normalize_point(&aa);
	gut_normalize_point(&bb);
	aa0 = aa;
	// the line will be used to generate intermediate vertices
	gut_vector(&aa, &bb, &line);
	// distance is used to determine the angle of the line and how many intermediate vertices required
	gut_distance_from_point_to_point(&aa, &bb, &d);
	d = asin(d / 2.0);
	nArcSeg = d / DTR(2.0); // 2 degrees is the increment
	// Divide the line in 4 or more segments
	nArcSeg = nArcSeg < 4 ? 4 : nArcSeg;
	d = ctx->eAttr.maxLength;
	// determine the face (edge & origin) normal 
	gut_plane_from_points(&origin, &bb, &aa, &pl); // the plane of the edge and the origin (used for side normal)
	d *= eAttr->width / 2; // scale the edge length
	v.i = pl.A * d; // scale the normal vector
	v.j = pl.B * d;
	v.k = pl.C * d;
	vr.i = -1 * v.i; // reverse the direction
	vr.j = -1 * v.j;
	vr.k = -1 * v.k;
	no[0] = *(GUT_VECTOR*)&aa; // outward normal
	ni[0].i = -1 * no[0].i; // reverse the direction
	ni[0].j = -1 * no[0].j;
	ni[0].k = -1 * no[0].k;

	// determine offset factor
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		offsetFlag = 1;
		gut_pt_on_line_closest_to_pt(&aa, &bb, &o, &c);
		gut_normalize_point(&c);
		f.x = c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		f.y = c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		f.z = c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
	}
	if (eAttr->scale.enable && eAttr->scale.factor != 0.0)
	{
		GUT_POINT	ta, tb;
		double		f = 1 - eAttr->scale.factor;
		gut_parametric_point(&aa, &bb, &ta, f / 2.0);
		gut_parametric_point(&aa, &bb, &tb, 1 - f / 2.0);
		aa = ta;
		bb = tb;
		gut_normalize_point(&aa);
		gut_normalize_point(&bb);
		// the line will be used to generate intermediate vertices
		gut_vector(&aa, &bb, &line);
		aa0 = aa;
	}

	// create first set of vertices
	gut_point_plus_vector(&aa, &v, &out[0]);
	gut_point_plus_vector(&aa, &vr, &out[1]);
	d = (1 - eAttr->height);
	gut_scale_pt_from_origin(&origin, &out[0], &out[3], d);
	gut_scale_pt_from_origin(&origin, &out[1], &out[2], d);
	if (offsetFlag)
	{	// move the points radially (in or out)
		out[0].x += f.x;		out[0].y += f.y;		out[0].z += f.z;
		out[1].x += f.x;		out[1].y += f.y;		out[1].z += f.z;
		out[2].x += f.x;		out[2].y += f.y;		out[2].z += f.z;
		out[3].x += f.x;		out[3].y += f.y;		out[3].z += f.z;
	}

	// start loop
	int			q;
	double		t;
	glBegin(GL_TRIANGLES);
	for (q = 0, t = 1.0 / (float)nArcSeg; q < nArcSeg; t += 1.0 / (float)nArcSeg, ++q)
	{
		bb.x = aa0.x + line.i * t;
		bb.y = aa0.y + line.j * t;
		bb.z = aa0.z + line.k * t;
		gut_normalize_point(&bb);
		no[1] = *(GUT_VECTOR*)&bb;
		ni[1].i = -1 * no[1].i; // reverse the direction
		ni[1].j = -1 * no[1].j;
		ni[1].k = -1 * no[1].k;
		// create next set of vertices
		gut_point_plus_vector(&bb, &v, &out[4]);
		gut_point_plus_vector(&bb, &vr, &out[5]);
		d = (1 - eAttr->height);
		gut_scale_pt_from_origin(&origin, &out[4], &out[7], d);
		gut_scale_pt_from_origin(&origin, &out[5], &out[6], d);
		if (offsetFlag)
		{	// move the points radially (in or out)
			out[4].x += f.x;		out[4].y += f.y;		out[4].z += f.z;
			out[5].x += f.x;		out[5].y += f.y;		out[5].z += f.z;
			out[6].x += f.x;		out[6].y += f.y;		out[6].z += f.z;
			out[7].x += f.x;		out[7].y += f.y;		out[7].z += f.z;
		}

		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		// top
		// draw 5 1 0 - top 
		// draw 1 5 6 - side
		glNormal3f((float)no[0].i, (float)no[0].j, (float)no[0].k);
		glVertex3f((float)out[0].x, (float)out[0].y, (float)out[0].z);
		glNormal3f((float)no[1].i, (float)no[1].j, (float)no[1].k);
		glVertex3f((float)out[4].x, (float)out[4].y, (float)out[4].z);
		glVertex3f((float)out[5].x, (float)out[5].y, (float)out[5].z);
		glNormal3f((float)no[1].i, (float)no[1].j, (float)no[1].k);
		glVertex3f((float)out[5].x, (float)out[5].y, (float)out[5].z);
		glNormal3f((float)no[0].i, (float)no[0].j, (float)no[0].k);
		glVertex3f((float)out[1].x, (float)out[1].y, (float)out[1].z);
		glVertex3f((float)out[0].x, (float)out[0].y, (float)out[0].z);

		// side 
		// draw 1 5 6 - side
		// draw 6 2 1 - side 
		glNormal3f((float)vr.i, (float)vr.j, (float)vr.k);
		glVertex3f((float)out[1].x, (float)out[1].y, (float)out[1].z);
		glVertex3f((float)out[5].x, (float)out[5].y, (float)out[5].z);
		glVertex3f((float)out[6].x, (float)out[6].y, (float)out[6].z);
		glVertex3f((float)out[6].x, (float)out[6].y, (float)out[6].z);
		glVertex3f((float)out[2].x, (float)out[2].y, (float)out[2].z);
		glVertex3f((float)out[1].x, (float)out[1].y, (float)out[1].z);

		// bottom 
		// draw 2 6 7 - bottom
		// draw 7 3 2 - bottom
		glNormal3f((float)ni[0].i, (float)ni[0].j, (float)ni[0].k);
		glVertex3f((float)out[2].x, (float)out[2].y, (float)out[2].z);
		glNormal3f((float)ni[1].i, (float)ni[1].j, (float)ni[1].k);
		glVertex3f((float)out[6].x, (float)out[6].y, (float)out[6].z);
		glVertex3f((float)out[7].x, (float)out[7].y, (float)out[7].z);
		glNormal3f((float)ni[1].i, (float)ni[1].j, (float)ni[1].k);
		glVertex3f((float)out[7].x, (float)out[7].y, (float)out[7].z);
		glNormal3f((float)ni[0].i, (float)ni[0].j, (float)ni[0].k);
		glVertex3f((float)out[3].x, (float)out[3].y, (float)out[3].z);
		glVertex3f((float)out[2].x, (float)out[2].y, (float)out[2].z);

		// side 
		// draw 3 7 4 - side
		// draw 4 0 3 - side
		glNormal3f((float)v.i, (float)v.j, (float)v.k);
		glVertex3f((float)out[3].x, (float)out[3].y, (float)out[3].z);
		glVertex3f((float)out[7].x, (float)out[7].y, (float)out[7].z);
		glVertex3f((float)out[4].x, (float)out[4].y, (float)out[4].z);
		glVertex3f((float)out[4].x, (float)out[4].y, (float)out[4].z);
		glVertex3f((float)out[0].x, (float)out[0].y, (float)out[0].z);
		glVertex3f((float)out[3].x, (float)out[3].y, (float)out[3].z);

		// copy
		out[0] = out[4];
		out[1] = out[5];
		out[2] = out[6];
		out[3] = out[7];
		no[0] = no[1];
		ni[0] = ni[1];
	}
	glEnd();
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_hex_arc(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, DS_COLOR *clr)
//-----------------------------------------------------------------------------------
{
	// a & b are the endpoints of the line
	// p[] is the array of new endpoints arranged in 2 pairs of 6 or 12 vertices surrounding each end point
	// n[] is the normals to apply to each new endpoint when drawing
	// normalize is a flag 
	// origin is the local origin point
	//
	// create normals
	GUT_VECTOR	z, x, h, w, tmp; //  r, tmp;
	GUT_PLANE	pl;
	double		rad, d;
	int			i, j, k, l, nArcSeg, offsetFlag=0;
	double		angle, aInc;
	GUT_POINT	aa, aa0, bb, pa[12], pb[12], o = { 0,0,0,1 }, c, f;
	GUT_VECTOR	na[12], nb[12], line;
	int			nSeg = ctx->drawAdj.quality->edgeNSeg;
	int			q;
	double		t;

	// normalize a & b
	aa0 = aa = *a;
	bb = *b;
	gut_normalize_point(&aa);
	gut_normalize_point(&bb);
	gut_vector(&aa, &bb, &line);
	gut_distance_from_point_to_point(&aa, &bb, &d);
	angle = asin(d / 2.0);
	nArcSeg = angle / DTR(2.0);
	nArcSeg = nArcSeg < 4 ? 4 : nArcSeg;
	gut_plane_from_points(&o, &aa, &bb, &pl); // the plane of the edge and the origin (used for side normal)
	x.i = -pl.A; // scale the normal vector (x)
	x.j = -pl.B;
	x.k = -pl.C;
	gut_normalize_vector(&x);
	z = *(GUT_VECTOR*)&aa;

	// determine offset factor
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		offsetFlag = 1;
		gut_pt_on_line_closest_to_pt(&aa, &bb, &o, &c);
		gut_normalize_point(&c);
		f.x = c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		f.y = c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		f.z = c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
	}
	if (eAttr->scale.enable && eAttr->scale.factor != 0.0)
	{
		GUT_POINT	ta, tb;
		double		f = 1 - eAttr->scale.factor;
		gut_parametric_point(&aa, &bb, &ta, f / 2.0);
		gut_parametric_point(&aa, &bb, &tb, 1 - f / 2.0);
		aa = ta;
		bb = tb;
		gut_normalize_point(&aa);
		gut_normalize_point(&bb);
		// the line will be used to generate intermediate vertices
		gut_vector(&aa, &bb, &line);
		aa0 = aa;
	}

	// create points 
	d = ctx->eAttr.maxLength;
	rad = d * eAttr->width / 2;

	for (i = 0, angle = 0, aInc = 360.0 / nSeg; i < nSeg; ++i, angle += aInc)
	{
		gut_scale_vector(&z, rad * sin(DTR(angle)), &h);
		gut_scale_vector(&x, rad * cos(DTR(angle)), &w);
		gut_add_vector(&h, &w, &tmp);
		gut_point_plus_vector((GUT_POINT*)&aa, &tmp, &pa[i]);
		if (offsetFlag)
		{
			pa[i].x += f.x;
			pa[i].y += f.y;
			pa[i].z += f.z;
		}
		na[i] = tmp;
		gut_normalize_vector(&na[i]);
	}

	// do all subsequent 
	// start loop
	glBegin(GL_TRIANGLES);
	for (q = 0, t = 1.0/(float)nArcSeg; q < nArcSeg; t += 1.0/(float)nArcSeg, ++q)
	{
		bb.x = aa0.x + line.i * t;
		bb.y = aa0.y + line.j * t;
		bb.z = aa0.z + line.k * t;
		gut_normalize_point(&bb);
		z = *(GUT_VECTOR*)&bb;
//		if (eAttr->offset.factor != 1.0)
//		{	// move the points radially (in or out)
//			d = eAttr->offset.factor;
//			gut_scale_pt_from_origin(&o, &bb, &bb, d);
//		}

		for (i = 0, angle = 0, aInc = 360.0 / nSeg; i < nSeg; ++i, angle += aInc)
		{
			gut_scale_vector(&z, rad * sin(DTR(angle)), &h);
			gut_scale_vector(&x, rad * cos(DTR(angle)), &w);
			gut_add_vector(&h, &w, &tmp);
			gut_point_plus_vector((GUT_POINT*)&bb, &tmp, &pb[i]);
			if (offsetFlag)
			{
				pb[i].x += f.x;
				pb[i].y += f.y;
				pb[i].z += f.z;
			}
			nb[i] = tmp;
			gut_normalize_vector(&nb[i]);
		}

		// draw		
		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		static int	i, j, k, l;

		for (i = 0, k = 0; i < nSeg; ++i, ++k)
		{
			l = (k + 1) % nSeg;
			j = (i + 1) % nSeg;
			glNormal3f((float)na[i].i, (float)na[i].j, (float)na[i].k);
			glVertex3f((float)pa[i].x, (float)pa[i].y, (float)pa[i].z);
			glNormal3f((float)nb[k].i, (float)nb[k].j, (float)nb[k].k);
			glVertex3f((float)pb[k].x, (float)pb[k].y, (float)pb[k].z);
			glNormal3f((float)nb[l].i, (float)nb[l].j, (float)nb[l].k);
			glVertex3f((float)pb[l].x, (float)pb[l].y, (float)pb[l].z);

			glNormal3f((float)nb[l].i, (float)nb[l].j, (float)nb[l].k);
			glVertex3f((float)pb[l].x, (float)pb[l].y, (float)pb[l].z);
			glNormal3f((float)na[j].i, (float)na[j].j, (float)na[j].k);
			glVertex3f((float)pa[j].x, (float)pa[j].y, (float)pa[j].z);
			glNormal3f((float)na[i].i, (float)na[i].j, (float)na[i].k);
			glVertex3f((float)pa[i].x, (float)pa[i].y, (float)pa[i].z);
		}

		aa = bb;
		memcpy(&pa[0], &pb[0], sizeof(GUT_POINT) * 12);
		memcpy(&na[0], &nb[0], sizeof(GUT_VECTOR) * 12);
	}
	glEnd();
}

//-----------------------------------------------------------------------------
int ds_gl_spherical_section(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, DS_COLOR *clr)
//-----------------------------------------------------------------------------
{
	GUT_POINT	vtx, v[360], origin = { 0,0,0,1 }, v2;
	GUT_VECTOR	n, nml;
	GUT_PLANE	pl;
	int			i, j;
	int			planeID[6] = { GL_CLIP_PLANE1, GL_CLIP_PLANE2, GL_CLIP_PLANE3, GL_CLIP_PLANE4, GL_CLIP_PLANE5 };
	int			nseg = 180, nPlanes = 0;
	double		d[4], distance, radius[6], ainc;
	double		s, c, x, y;
	double		orientation[16];

	if (face->nVtx > 6) // skip degenerate faces
		return 0;

	s = sin(DTR(2.0));
	c = cos(DTR(2.0));

	// special 
	// set up a clip plane for each face vertex based on its position and normal
	for (i = 0; i < face->nVtx; ++i)
	{
		// get face vertex & normal
		vtx = gobj->vtx[face->vtx[i]];
		n = *(GUT_VECTOR*)&gobj->nml[face->nml[i]];

		// move the vertex by the offset 
		if (gobj->fAttr.offset.enable)
		{
			vtx.x += n.i * gobj->fAttr.offset.factor;
			vtx.y += n.j * gobj->fAttr.offset.factor;
			vtx.z += n.k * gobj->fAttr.offset.factor;
		}
		if (gobj->fAttr.scale.enable)
		{
			vtx.x *= gobj->fAttr.scale.factor;
			vtx.y *= gobj->fAttr.scale.factor;
			vtx.z *= gobj->fAttr.scale.factor;
		}
		// determine plane equation at vertex with given normal
		gut_plane_from_point_normal(&vtx, &n, &pl);

//		ds_find_great_circle_matrix(ctx, &vtx, &nml, &orientation, 1.0);

		// determine the point on the plane closest to the origin
		// it may be different than the user provided point
		gut_find_point_plane_intersection(&pl, &origin, &v2);
		// copy back && reset the orientation matrix
		vtx = v2;
//		orientation[12] = vtx.x;
//		orientation[13] = vtx.y;
//		orientation[14] = vtx.z;

		// determine size of clip radius
		// radius modified by scale 
		distance = sqrt(vtx.x * vtx.x + vtx.y * vtx.y + vtx.z * vtx.z);
		radius[i] = gobj->fAttr.scale.enable ? gobj->fAttr.scale.factor : 1;
		radius[i] = sqrt(radius[i] * radius[i] - distance * distance);
//		radius = sqrt(1.0 - distance * distance);
		ainc = asin(distance);

		// set up plane equation
		// reverse sign of normal
		n.i *= -1;
		n.j *= -1;
		n.k *= -1;
		gut_plane_from_point_normal(&vtx, &n, &pl);
		d[0] = pl.A;
		d[1] = pl.B;
		d[2] = pl.C;
		d[3] = pl.D;
		// enable PLANE_N
		glClipPlane(planeID[nPlanes], (const double*)&d);
		glEnable(planeID[nPlanes]);
		++nPlanes;
	}

	// create geometry
	// storage: vertex[size], normal[size]
	// size = 3 * NSEG
	// create the base geometry of a unit radius circle
	v[0].x = 1.0, v[0].y = 0, v[0].z = 0, v[0].w = 1.0;
	for (i = 0, j = 1; j < nseg; ++j, ++i)
	{
		x = ( v[i].x*c - v[i].y*s );
		y = ( v[i].x*s + v[i].y*c );
		v[j].x = x;
		v[j].y = y;
		v[j].z = 0;
		v[j].w = 1.0;
	}

	// draw all vertices
	for (i = 0; i < face->nVtx; ++i)
	{
		// get face vertex & normal
		vtx = gobj->vtx[face->vtx[i]];
		n = *(GUT_VECTOR*)&gobj->nml[face->nml[i]];
		// move the vertex by the offset 
		if (gobj->fAttr.offset.enable)
		{
			vtx.x += n.i * gobj->fAttr.offset.factor;
			vtx.y += n.j * gobj->fAttr.offset.factor;
			vtx.z += n.k * gobj->fAttr.offset.factor;
		}
		if (gobj->fAttr.scale.enable)
		{
			vtx.x *= gobj->fAttr.scale.factor;
			vtx.y *= gobj->fAttr.scale.factor;
			vtx.z *= gobj->fAttr.scale.factor;
		}
		// determine plane equation at vertex with given normal
		gut_plane_from_point_normal(&vtx, &n, &pl);
//		ds_find_great_circle_matrix(ctx, &vtx, &nml, &orientation, 1.0);

		// determine the point on the plane closest to the origin
		// it may be different than the user provided point
		gut_find_point_plane_intersection(&pl, &origin, &v2);
		// copy back && reset the orientation matrix
		vtx = v2;
//		orientation[12] = vtx.x;
//		orientation[13] = vtx.y;
//		orientation[14] = vtx.z;

		glDisable(planeID[i]);
		// create orientation based on vertex & normal
		ds_find_great_circle_matrix(ctx, &vtx, &n, &orientation, 1.0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glMultMatrixd((GLdouble*)&orientation);
		glScalef((float)radius[i], (float)radius[i], (float)1.0);
		glBegin(GL_POLYGON);
		glNormal3f((float)0, (float)0, (float)1.0);
		if(gobj->fAttr.orthodrome.cutColorEnable)
			glColor3f((GLfloat)gobj->fAttr.orthodrome.cutColor.r, (GLfloat)gobj->fAttr.orthodrome.cutColor.g, (GLfloat)gobj->fAttr.orthodrome.cutColor.b);
		else
			glColor3f((GLfloat)clr->r, (GLfloat)clr->g, (GLfloat)clr->b);
		for (j = 0; j < nseg; ++j)
		{
			glVertex3f((float)v[j].x, (float)v[j].y, (float)v[j].z);
		}
		glEnd();
		glPopMatrix();
		glEnable(planeID[i]);
	}

	// render sphere
	radius[0] = gobj->fAttr.scale.enable ? gobj->fAttr.scale.factor : 1;
	ds_gl_render_vertex(ctx, gobj, &origin, ctx->renderVertex.vtxObj->vtx, (GUT_POINT*)ctx->renderVertex.vtxObj->v_out, ctx->renderVertex.vtxObj->tri, ctx->renderVertex.vtxObj->nVtx, ctx->renderVertex.vtxObj->nTri, radius[0], clr, 0);

	// disable all the clip planes
	for (i = 0; i < nPlanes; ++i)
	{
		glDisable(planeID[i]);
	}

	return 0;
}

//-----------------------------------------------------------------------------
int ds_gl_spherical_bubble(DS_CTX *ctx, DS_GEO_OBJECT *gobj, DS_FACE *face, DS_COLOR *clr)
//-----------------------------------------------------------------------------
{
	GUT_POINT	v, c; // , origin = { 0,0,0,1 };
	GUT_VECTOR	n;// , nml;
	GUT_PLANE	pl;
	double		d[4], radius, Radius, fraction=1.0, offset;// , distance, radius, ainc;
	int			inward;

	if (gobj->fAttr.hole.style != FACE_HOLE_STYLE_ROUND_WITH_IN_DIMPLE && gobj->fAttr.hole.style != FACE_HOLE_STYLE_ROUND_WITH_OUT_DIMPLE)
		return 0;

	// determine correct radius and centroid
	radius = face->draw.radius;
	if (gobj->fAttr.scale.enable)
		radius *= gobj->fAttr.scale.factor;
	if (gobj->fAttr.hole.enable)
		radius *= gobj->fAttr.hole.radius;

	v = face->draw.centroid;
	if (gobj->fAttr.offset.enable)
	{
		v.x += face->draw.normal.i * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
		v.y += face->draw.normal.j * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
		v.z += face->draw.normal.k * ctx->eAttr.maxLength * gobj->fAttr.offset.factor;
	}
	c = v;

	// special offset for sphere
	fraction = gobj->fAttr.hole.shallowness; // range (0.3 - 1.25)
	Radius = radius * (1 + fraction * fraction) / (2.0 * fraction);
	Radius *= 1.045; // enlarge radius a little bit so no edge hole
	offset = Radius - fraction * radius;

	if (gobj->fAttr.hole.style == FACE_HOLE_STYLE_ROUND_WITH_IN_DIMPLE)
	{
		inward = 1;
		v.x += face->draw.normal.i * offset;
		v.y += face->draw.normal.j * offset;
		v.z += face->draw.normal.k * offset;

		// draw a bubble at face hole
		// set up clip plane
		// scale the sphere render 
		// special 
		// set up a clip plane for each face vertex based on its position and normal

		// set up plane equation
		// reverse sign of normal
		n = face->draw.rnormal;
		gut_plane_from_point_normal(&c, &face->draw.rnormal, &pl);
		d[0] = pl.A;
		d[1] = pl.B;
		d[2] = pl.C;
		d[3] = pl.D;
		// enable PLANE_N
		glClipPlane(GL_CLIP_PLANE1, (const double*)&d);
		glEnable(GL_CLIP_PLANE1);
	}
	else if (gobj->fAttr.hole.style == FACE_HOLE_STYLE_ROUND_WITH_OUT_DIMPLE)
	{
		inward = 0;
		v.x -= face->draw.normal.i * offset;
		v.y -= face->draw.normal.j * offset;
		v.z -= face->draw.normal.k * offset;

		// draw a bubble at face hole
		// set up clip plane
		// scale the sphere render 
		// special 
		// set up a clip plane for each face vertex based on its position and normal

		// set up plane equation
		// reverse sign of normal
		n = face->draw.normal;
		gut_plane_from_point_normal(&c, &face->draw.normal, &pl);
		d[0] = pl.A;
		d[1] = pl.B;
		d[2] = pl.C;
		d[3] = pl.D;
		// enable PLANE_N
		glClipPlane(GL_CLIP_PLANE1, (const double*)&d);
		glEnable(GL_CLIP_PLANE1);
	}
	// push matrix
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
	glTranslatef((float)c.x, (float)c.y, (float)c.z); // scale matrix
	glPopMatrix(); // return to prior state

	// render sphere - force use of hi res
	ds_gl_render_vertex(ctx, gobj, &v, ctx->renderVertex.vtxObjHiRes.vtx, (GUT_POINT*)ctx->renderVertex.vtxObjHiRes.v_out, ctx->renderVertex.vtxObjHiRes.tri, ctx->renderVertex.vtxObjHiRes.nVtx, ctx->renderVertex.vtxObjHiRes.nTri, Radius, clr, inward);

	// disable the clip planes
	glDisable(GL_CLIP_PLANE1);
	
	return 0;
}
