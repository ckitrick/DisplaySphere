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
void ds_geo_edge_to_triangles(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *out, int normalize, GUT_POINT *origin)
//-----------------------------------------------------------------------------------
{
	// create new points that represent triangles around edge
	GUT_POINT			*o = origin;
	GUT_PLANE	p; 
	GUT_VECTOR	v; 
	double		d;

//	gut_distance_from_point_to_point(a, b, &d);
//	d = eattr->maxLength;
	d = ctx->eAttr.maxLength;
	gut_plane_from_points(origin, b, a, &p);
	d *= eattr->width / 2; // scale the edge length
	v.i = p.A * d; // scale the normal vector
	v.j = p.B * d;
	v.k = p.C * d;
	gut_point_plus_vector(a, &v, &out[0]);
	gut_point_plus_vector(b, &v, &out[1]);
	v.i *= -1; // reverse the direction
	v.j *= -1;
	v.k *= -1;
	gut_point_plus_vector(b, &v, &out[2]);
	gut_point_plus_vector(a, &v, &out[3]);
	d = (1 - eattr->height);
	gut_scale_pt(&out[0], &out[4], d);
	gut_scale_pt(&out[1], &out[5], d);
	gut_scale_pt(&out[2], &out[6], d);
	gut_scale_pt(&out[3], &out[7], d);
	// triangle out(0,1,2) - top
	// triangle out(2,3,0) - top
	// triangle out(0,4,5) - side
	// triangle out(5,1,0) - side 
	// triangle out(2,6,7) - side 
	// triangle out(7,3,2) - side 
	if (eattr->offset != 1.0)
	{	// move the points radially (in or out)
		d = eattr->offset;
		gut_scale_pt(&out[0], &out[0], d);
		gut_scale_pt(&out[1], &out[1], d);
		gut_scale_pt(&out[2], &out[2], d);
		gut_scale_pt(&out[3], &out[3], d);
		gut_scale_pt(&out[4], &out[4], d);
		gut_scale_pt(&out[5], &out[5], d);
		gut_scale_pt(&out[6], &out[6], d);
		gut_scale_pt(&out[7], &out[7], d);
	}
}

//-----------------------------------------------------------------------------------
void ds_geo_edge_to_triangles_hex_axii(DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *p, GUT_VECTOR *x, GUT_VECTOR *y, GUT_VECTOR *z, GUT_VECTOR *n, int normalize, GUT_POINT *origin)
//-----------------------------------------------------------------------------------
{
	// create normals
	GUT_VECTOR	h, w, tmp; // r, tmp;
//	GUT_POINT	m;
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
void ds_geo_edge_to_triangles_hex(DS_CTX *ctx, DS_EDGE_ATTRIBUTES *eattr, GUT_POINT *a, GUT_POINT *b, GUT_POINT *pa, GUT_POINT *pb, GUT_VECTOR *n, int normalize, GUT_POINT *origin, int nSeg)
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
	GUT_POINT	m;
	double		rad, d, t;
	int			i;
	double		angle, aInc;

	gut_vector((GUT_POINT*)a, (GUT_POINT*)b, &y);
	gut_normalize_vector(&y);

	gut_point_on_line_closest_to_origin((GUT_POINT*)a, (GUT_POINT*)b, &m, &t);
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

	// create points 
	d = ctx->eAttr.maxLength;
	rad = d * eattr->width / 2;

	for (i = 0, angle = 0, aInc = 360.0 / nSeg; i < nSeg; ++i, angle += aInc)
	{
		gut_scale_vector(&z, rad * sin(DTR(angle)), &h);
		gut_scale_vector(&x, rad * cos(DTR(angle)), &w);
		gut_add_vector(&h, &w, &tmp);
		gut_point_plus_vector((GUT_POINT*)a, &tmp, &pa[i]);
		gut_point_plus_vector((GUT_POINT*)b, &tmp, &pb[i]);
		n[i] = tmp;
		gut_normalize_vector(&n[i]);
	}
}
