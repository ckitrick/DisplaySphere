/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle general geometric operations that 
	are not handled in the geometry library.
*/
#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
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
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *out, int normalize, GUT_POINT *origin)
//-----------------------------------------------------------------------------------
{
	// create new points that represent triangles around edge
	GUT_PLANE	pl; 
	GUT_VECTOR	v; 
	GUT_POINT	aa, bb, c, o = { 0,0,0,1 };
	double		d;

	aa = *a;
	bb = *b;
	d = ctx->eAttr.maxLength;
	gut_plane_from_points(origin, b, a, &pl);
	d *= eAttr->width / 2; // scale the edge length
	v.i = pl.A * d; // scale the normal vector
	v.j = pl.B * d;
	v.k = pl.C * d;
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		gut_pt_on_line_closest_to_pt(a, b, &o, &c);
		gut_normalize_point(&c);
		aa.x = aa.x + c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		aa.y = aa.y + c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		aa.z = aa.z + c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.x = bb.x + c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.y = bb.y + c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.z = bb.z + c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
	}
	gut_point_plus_vector(&aa, &v, &out[0]);
	gut_point_plus_vector(&bb, &v, &out[4]);
	v.i *= -1; // reverse the direction
	v.j *= -1;
	v.k *= -1;
	gut_point_plus_vector(&aa, &v, &out[1]);
	gut_point_plus_vector(&bb, &v, &out[5]);
	d = (1 - eAttr->height);
	gut_scale_pt_from_origin(origin, &out[0], &out[3], d);
	gut_scale_pt_from_origin(origin, &out[1], &out[2], d);
	gut_scale_pt_from_origin(origin, &out[4], &out[7], d);
	gut_scale_pt_from_origin(origin, &out[5], &out[6], d);
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_new(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *out, GUT_VECTOR *nml, int normalize, GUT_POINT *origin)
//-----------------------------------------------------------------------------------
{
	// create new points that represent triangles around edge
	GUT_PLANE	pl;
	GUT_VECTOR	v;
	GUT_POINT	aa, bb, c, o = { 0,0,0,1 };
	double		d;

	aa = *a;
	bb = *b;
	d = ctx->eAttr.maxLength;
	gut_plane_from_points(origin, &bb, &aa, &pl);
	d *= eAttr->width / 2; // scale the edge length
	v.i = pl.A * d; // scale the normal vector
	v.j = pl.B * d;
	v.k = pl.C * d;
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		gut_pt_on_line_closest_to_pt(&aa, &bb, &o, &c);
		gut_normalize_point(&c);
		aa.x = aa.x + c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		aa.y = aa.y + c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		aa.z = aa.z + c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.x = bb.x + c.x * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.y = bb.y + c.y * ctx->eAttr.maxLength * eAttr->offset.factor;
		bb.z = bb.z + c.z * ctx->eAttr.maxLength * eAttr->offset.factor;
	}
	if (eAttr->scale.enable && eAttr->scale.factor != 0.0)
	{
		GUT_POINT	ta, tb;
		double		f = 1 - eAttr->scale.factor;
		gut_parametric_point(&aa, &bb, &ta, f / 2.0);
		gut_parametric_point(&aa, &bb, &tb, 1 - f / 2.0);
		aa = ta;
		bb = tb;
	}
	gut_point_plus_vector(&aa, &v, &out[0]);
	gut_point_plus_vector(&bb, &v, &out[4]);
	v.i *= -1; // reverse the direction
	v.j *= -1;
	v.k *= -1;
	gut_point_plus_vector(&aa, &v, &out[1]);
	gut_point_plus_vector(&bb, &v, &out[5]);
	d = (1 - eAttr->height);
	gut_scale_pt_from_origin(origin, &out[0], &out[3], d);
	gut_scale_pt_from_origin(origin, &out[1], &out[2], d);
	gut_scale_pt_from_origin(origin, &out[4], &out[7], d);
	gut_scale_pt_from_origin(origin, &out[5], &out[6], d);

	// generate normals
	gut_cross_product_from_triangle_points(&out[0], &out[4], &out[5], &nml[0]);// 0 4 5
	gut_cross_product_from_triangle_points(&out[1], &out[5], &out[6], &nml[1]);// 1 5 6
	gut_cross_product_from_triangle_points(&out[2], &out[6], &out[7], &nml[2]);// 2 6 7 
	gut_cross_product_from_triangle_points(&out[3], &out[7], &out[4], &nml[3]);// 3 7 4
	gut_normalize_vector(&nml[0]);
	gut_normalize_vector(&nml[1]);
	gut_normalize_vector(&nml[2]);
	gut_normalize_vector(&nml[3]);
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_hex_axii(DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *p, GUT_VECTOR *x, GUT_VECTOR *y, GUT_VECTOR *z, GUT_VECTOR *n, int normalize, GUT_POINT *origin)
//-----------------------------------------------------------------------------------
{
	// create normals
	GUT_VECTOR	h, w, tmp; 
	double		rad, d;
	double		angle, aInc;
	int			i, j, nSeg=6;

	// create points 
	gut_distance_from_point_to_point((GUT_POINT*)a, (GUT_POINT*)b, &d);
	rad = d * eattr->width / 2;

	for (i = 0, j=6, angle = 0, aInc = 360.0 / nSeg; i < nSeg; ++i, angle += aInc, ++j)
	{
		gut_scale_vector(z, rad * sin(DTR(angle)), &h);
		gut_scale_vector(x, rad * cos(DTR(angle)), &w);
		gut_add_vector(&h, &w, &tmp);
		gut_point_plus_vector((GUT_POINT*)a, &tmp, &p[i]);
		gut_point_plus_vector((GUT_POINT*)b, &tmp, &p[j]);
		n[i] = tmp;
		gut_normalize_vector(&n[i]);
	}
}

//-----------------------------------------------------------------------------------
void ds_geo_build_transform_matrix ( DS_CTX *ctx )
//-----------------------------------------------------------------------------------
{
	GUT_POINT	xAxis, yAxis, zAxis;
	// build transformation matrix
	// normalize vectors 
	zAxis = *(GUT_POINT*)&ctx->inputTrans.zAxis;
	yAxis = *(GUT_POINT*)&ctx->inputTrans.yAxis;
	gut_normalize_point(&zAxis);
	gut_normalize_point(&yAxis);

	gut_cross_product((GUT_VECTOR*)&yAxis, (GUT_VECTOR*)&zAxis, (GUT_VECTOR*)&xAxis);
	gut_normalize_point(&xAxis);
	gut_cross_product((GUT_VECTOR*)&zAxis, (GUT_VECTOR*)&xAxis, (GUT_VECTOR*)&yAxis);
	gut_normalize_point(&yAxis);

	// set unit matrix initially
	mtx_set_unity(&ctx->inputTrans.matrix[0]);

	ctx->inputTrans.matrix[0].data.array[0]  = xAxis.x;
	ctx->inputTrans.matrix[0].data.array[4]  = xAxis.y;
	ctx->inputTrans.matrix[0].data.array[8]  = xAxis.z;
										    
	ctx->inputTrans.matrix[0].data.array[1]  = yAxis.x;
	ctx->inputTrans.matrix[0].data.array[5]  = yAxis.y;
	ctx->inputTrans.matrix[0].data.array[9]  = yAxis.z;
										    
	ctx->inputTrans.matrix[0].data.array[2]  = zAxis.x;
	ctx->inputTrans.matrix[0].data.array[6]  = zAxis.y;
	ctx->inputTrans.matrix[0].data.array[10] = zAxis.z;
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_hex(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *pa, GUT_POINT *pb, GUT_VECTOR *n, int normalize, GUT_POINT *origin, int nSeg)
//-----------------------------------------------------------------------------------
{
	// a & b are the endpoints of the line
	// p[] is the array of new endpoints arranged in 2 pairs of 6 vertices surrounding each end point
	// n[] is the normals to apply to each new endpoint when drawing
	// normalize is a flag 
	// origin is the local origin point
	//
	// create normals
	GUT_VECTOR	z, x, y, h, w, tmp; //  r, tmp;
	GUT_POINT	aa, bb, ta, tb, m, c, f, o = { 0,0,0,1 };
	double		rad, d; // , t;
	int			i, offsetFlag=0;
	double		angle, aInc;

	aa = *a;
	bb = *b;

	gut_vector((GUT_POINT*)a, (GUT_POINT*)b, &y);
	gut_normalize_vector(&y);

	gut_pt_on_line_closest_to_pt(a, b, origin, &m);
	gut_distance_from_point_to_point(&m, origin, &d);

	if (d < 0.00001) // line ab passes thru origin
	{
		z.i = 0; z.j = 0.0; z.k = 1.0; z.l = 0.0; // z axis
		gut_dot_product(&z, &y, &d);
		if (fabs(d - 1.0) < 0.000001)
		{
			z.i = 0.0; z.j = 1.0; z.k = 0.0;
			x.i = 1.0; x.j = 0.0; x.k = 0.0;
		}
		else
			gut_cross_product(&y, &z, &x);
	}
	else
	{
		gut_vector(&m, origin, &x); // line from origin to point on line is local x axis
		gut_normalize_vector(&x);
		gut_cross_product(&x, &y, &z);
	}

	// determine offset factor
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		offsetFlag = 1;
		gut_pt_on_line_closest_to_pt(a, b, &o, &c);
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
	}

	// create points 
	d = ctx->eAttr.maxLength;
	rad = d * eAttr->width / 2;

	for (i = 0, angle = 0, aInc = 360.0 / nSeg; i < nSeg; ++i, angle += aInc)
	{
		gut_scale_vector(&z, rad * sin(DTR(angle)), &h);
		gut_scale_vector(&x, rad * cos(DTR(angle)), &w);
		gut_add_vector(&h, &w, &tmp);
		gut_point_plus_vector(&aa, &tmp, &pa[i]);
		gut_point_plus_vector(&bb, &tmp, &pb[i]);
		if (offsetFlag)
		{
			pa[i].x += f.x;
			pa[i].y += f.y;
			pa[i].z += f.z;
			pb[i].x += f.x;
			pb[i].y += f.y;
			pb[i].z += f.z;
		}
		n[i] = tmp;
		gut_normalize_vector(&n[i]);
	}
}


int ds_edge_matrix(GUT_POINT *a, GUT_POINT *b, double *length, GUT_POINT *mid, GUT_VECTOR *offsetVec, double *rotMat)
{
	// compute edge information
	// local rotation matrix, length, midpoint, offset vector
	// for every edge
	GUT_VECTOR	z, x, y; //  r, tmp;
	GUT_POINT	m, origin = { 0,0,0,1 };
	double		d; 

	gut_distance_from_point_to_point((GUT_POINT*)a, (GUT_POINT*)b, length); // line length
	gut_mid_point((GUT_POINT*)a, (GUT_POINT*)b, mid); // mid point on line
	gut_vector((GUT_POINT*)a, (GUT_POINT*)b, &y); // line vector (local y axis)
	gut_normalize_vector(&y);

	gut_pt_on_line_closest_to_pt(a, b, &origin, &m);
	gut_distance_from_point_to_point(&m, &origin, &d);

	if (d < 0.00001) // line ab passes thru origin
	{
		z.i = 0; z.j = 0.0; z.k = 1.0; z.l = 0.0; // z axis
		gut_dot_product(&z, &y, &d);
		if (fabs(d - 1.0) < 0.000001)
		{
			z.i = 0.0; z.j = 1.0; z.k = 0.0;
			x.i = 1.0; x.j = 0.0; x.k = 0.0;
		}
		else
			gut_cross_product(&y, &z, &x);
	}
	else
	{
		gut_vector(&origin, &m, &z); // line from origin to point on line is local z axis
		gut_normalize_vector(&z);
		gut_cross_product(&y, &z, &x); // local x axis
	}

	// offset vector
	*offsetVec = z;

	// initialize rotation matrix
//	rotMat[0] = x.i, rotMat[1] = y.i, rotMat[2] = z.i, rotMat[3] = 0;
//	rotMat[4] = x.j, rotMat[5] = y.j, rotMat[6] = z.j, rotMat[7] = 0;
//	rotMat[8] = x.k, rotMat[9] = y.k, rotMat[10] = z.k, rotMat[11] = 0;
//	rotMat[12] = 0, rotMat[13] = 0, rotMat[14] = 0, rotMat[15] = 1;

	rotMat[0] = x.i, rotMat[1] = x.j, rotMat[2] = x.k, rotMat[3] = 0;
	rotMat[4] = y.i, rotMat[5] = y.j, rotMat[6] = y.k, rotMat[7] = 0;
	rotMat[8] = z.i, rotMat[9] = z.j, rotMat[10] = z.k, rotMat[11] = 0;
	rotMat[12] = 0, rotMat[13] = 0, rotMat[14] = 0, rotMat[15] = 1;

	return 0;
}

int ds_create_unit_edge(int style, GUT_POINT *a, GUT_POINT *b, GUT_VECTOR *n)
{
	// create all edge profiles (cylindrical, box, etc.)

	int		i, j;
	double  ang, s, c;

	switch (style) {
	case 0: // box
		// box style (unit cube centered around origin)
		// create 4 points around y axis on xz plane 
		// start at xy (0.707,0.707) and go around by 90 degrees (set y at -0.5)
		// second 4 points at y = 0.5
		// normals are along the x and z axii
		a[0].x = 0.5; // 0.70710678118654752440084436210485;
		a[0].y = -0.5;
		a[0].z = 0.5; // 0.70710678118654752440084436210485;
		b[0].x = 0.5; // 0.70710678118654752440084436210485;
		b[0].y = 0.5;
		b[0].z = 0.5; // 0.70710678118654752440084436210485;
		n[0] = *(GUT_VECTOR*)&a[0];

		ang = DTR(90.0);
		for (j = 0, i = 1, s = sin(ang), c = cos(ang); i < 4; ++i, ++j)
		{
			a[i].x = a[j].x * c - a[j].z * s;
			a[i].y = a[j].y;
			a[i].z = a[j].x * s + a[j].z * c;
			b[i].x = b[j].x * c - b[j].z * s;
			b[i].y = b[j].y;
			b[i].z = b[j].x * s + b[j].z * c;
		}
		n[0].i = 0.0;	n[0].j = 0;		n[0].k = 1;		n[0].l = 0;
		n[1].i = -1.0;	n[1].j = 0;		n[1].k = 0;		n[1].l = 0;
		n[2].i = 0;		n[2].j = 0;		n[2].k = -1;	n[2].l = 0;
		n[3].i = 1.0;	n[3].j = 0;		n[3].k = 0;		n[3].l = 0;
		break;
	case 1: // low-res cylinder
		// cylindrical styles (unit cylindrical centered around origin)
		// create circle of nSegments (6,12,...) around xz axii 
		// one circle is at -0.5 on y axis 
		// second circle is at +0.5 on y axis
		// normals are normalized per vertex 
		ang=DTR(60.0);
		a[0].x = 1.0;
		a[0].y = -0.5;
		a[0].z = 0;
		b[0].x = 1.0;
		b[0].y = 0.5;
		b[0].z = 0;
		n[0] = *(GUT_VECTOR*)&a[0];

		for (j=0, i = 1, s = sin(ang), c=cos(ang); i < 6; ++i, ++j)
		{
			a[i].x = a[j].x * c - a[j].z * s;
			a[i].y = a[j].y;
			a[i].z = a[j].x * s + a[j].z * c;
			n[i] = *(GUT_VECTOR*)&a[i];
			b[i].x = b[j].x * c - b[j].z * s;
			b[i].y = b[j].y;
			b[i].z = b[j].x * s + b[j].z * c;
		}
		break;
	case 2: // hi-res cylinder
		ang = DTR(30.0);
		a[0].x = 1.0;
		a[0].y = -0.5;
		a[0].z = 0;
		b[0].x = 1.0;
		b[0].y = 0.5;
		b[0].z = 0;
		n[0] = *(GUT_VECTOR*)&a[0];

		for (j = 0, i = 1, s = sin(ang), c = cos(ang); i < 6; ++i, ++j)
		{
			a[i].x = a[j].x * c - a[j].z * s;
			a[i].y = a[j].y;
			a[i].z = a[j].x * s + a[j].z * c;
			n[i] = *(GUT_VECTOR*)&a[i];
			b[i].x = b[j].x * c - b[j].z * s;
			b[i].y = b[j].y;
			b[i].z = b[j].x * s + b[j].z * c;
		}
		break;
	}

	return 0;
}

int ds_render_edge(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eAttr, GUT_POINT *a, GUT_POINT *b, DS_COLOR *clr)
{
	double		length, rotMat[16], offset;
	GUT_POINT	mid;
	GUT_VECTOR	offsetVec;
	GUT_POINT	pa[4], pb[4];
	GUT_VECTOR	n[4];

	// find matrix information 
	ds_edge_matrix(a, b, &length, &mid, &offsetVec, rotMat);
	if (eAttr->scale.enable && eAttr->scale.factor != 1.0)
		length *= eAttr->scale.factor;

	// create geometry
	ds_create_unit_edge(0, pa, pb, n);

		// set up 
	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();
//	mid.x -= offsetVec.i * eAttr->heighta
	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
	{
		offset = ctx->eAttr.maxLength * eAttr->offset.factor;
		glTranslatef((float)(mid.x + offsetVec.i * offset), (float)(mid.y + offsetVec.j * offset), (float)(mid.z + offsetVec.k * offset));
	}
	else
		glTranslatef((float)mid.x, (float)mid.y, (float)mid.z);
//	glTranslatef((float)mid.x, (float)mid.y, (float)mid.z);
	glMultMatrixd((double*)rotMat);
	glScalef((float)(eAttr->width*ctx->eAttr.maxLength), (float)length, (float)(eAttr->height*ctx->eAttr.maxLength));
//	ctx->eAttr.maxLength * gobj->eAttr. * vv.x


//	glMultMatrixd((double*)rotMat);
	//	 x = mid.x + zaxis.i * offset 
	//	 y = mid.y + zaxis.j * offset 
	//	 z = mid.z + zaxis.k * offset 
	// determine offset factor
//	if (eAttr->offset.enable && eAttr->offset.factor != 0.0)
//	{
//		offset = ctx->eAttr.maxLength * eAttr->offset.factor;
//		glTranslatef((float)(mid.x + offsetVec.i * offset), (float)(mid.y + offsetVec.j * offset), (float)(mid.z + offsetVec.k * offset));
//	}
//	else 
//		glTranslatef((float)mid.x, (float)mid.y, (float)mid.z);

	// render
	int		i, j;
	int		nSeg = 4;

	glBegin(GL_TRIANGLES);
	for (i = 0, j = nSeg; i < nSeg; ++i, ++j)
	{
		j = (i + 1) % nSeg;
		glColor3f((float)clr->r, (float)clr->g, (float)clr->b);
		glNormal3f((float)n[i].i, (float)n[i].j, (float)n[i].k );

		glVertex3f((float)pa[i].x, (float)pa[i].y, (float)pa[i].z);
		glVertex3f((float)pb[i].x, (float)pb[i].y, (float)pb[i].z);
		glVertex3f((float)pb[j].x, (float)pb[j].y, (float)pb[j].z);

		glVertex3f((float)pb[j].x, (float)pb[j].y, (float)pb[j].z);
		glVertex3f((float)pa[j].x, (float)pa[j].y, (float)pa[j].z);
		glVertex3f((float)pa[i].x, (float)pa[i].y, (float)pa[i].z);
	}
	glEnd();

	glPopMatrix();
	return 0;
}

int ds_face_matrix(GUT_POINT *a, GUT_POINT *b, GUT_POINT *c, GUT_POINT *mid, double *scale, GUT_VECTOR *offsetVec, double *m1, double *m2)
{
	// compute edge information
	// local rotation matrix, length, midpoint, offset vector
	// for every edge
	GUT_VECTOR	z, x, y; //  r, tmp;
	GUT_POINT	origin = { 0,0,0,1 };
	double		mt[16], mr[16], m[16];

	// compute local axii
	gut_vector(a, b, &x);
	gut_vector(a, c, &y);
	gut_normalize_vector(&x);
	gut_normalize_vector(&y);
	gut_cross_product(&x, &y, &z);
	gut_cross_product(&z, &x, &y);

	// centroid is translation vector

	// build matrix
	mtx_create_translation_matrix((MTX_MATRIX*)mt, mid->x, mid->y, mid->z);
	// translation + rotation
	// initialize rotation matrix
	//	mr[0] = x.i, mr[1] = y.i, mr[2] = z.i, mr[3] = 0;
	//	mr[4] = x.j, mr[5] = y.j, mr[6] = z.j, mr[7] = 0;
	//	mr[8] = x.k, mr[9] = y.k, mr[10] = z.k, mr[11] = 0;
	//	mr[12] = 0, mr[13] = 0, mr[14] = 0, mr[15] = 1;

	mr[0] = x.i, mr[1] = x.j, mr[2] = x.k, mr[3] = 0;
	mr[4] = y.i, mr[5] = y.j, mr[6] = y.k, mr[7] = 0;
	mr[8] = z.i, mr[9] = z.j, mr[10] = z.k, mr[11] = 0;
	mr[12] = 0, mr[13] = 0, mr[14] = 0, mr[15] = 1;

	// multiple matrices
	mtx_multiply_matrix((MTX_MATRIX*)mt, (MTX_MATRIX*)mr, (MTX_MATRIX*)m1);
	mtx_multiply_matrix((MTX_MATRIX*)mr, (MTX_MATRIX*)mt, (MTX_MATRIX*)m2);

	return 0;
}