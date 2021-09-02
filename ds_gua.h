#pragma once
/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

typedef struct {
	void		*vdb;		// unique vertices (coordinates)
	void		*eldb;		// unique edge lengths 
	void		*edb;		// unique edges (2 unique vertex indices) with unique edge
	void		*tedb;		// unique triangles by unique edge lengths
	void		*tdb;		// unique triangles by unique vertex indices along with unique tri by edge
	void		*ndb;		// unique normals (coordinates)
	char		name[128];	// user supplied name
//	GUA_OFF		off;		// OFF data
//	int			*vIndex;	// array of vertex indices
} GUA_UNIQUE;

typedef int(*GUA_USER_FUNC)(void *user_ctx, void *data);
int gua_db_traverse(void *guap, void *dbp, GUA_USER_FUNC ufunc, void *uctx);
//int gua_off_read(void *guap, char *spc_filename, float *defaultColor);
int gua_off_read(DS_CTX *ctx, void *gua, FILE *fp, float *clr);
int gua_spc_read(DS_CTX *ctx, void **gua, FILE *fp, float *defaultColor);
int gua_convert_to_object(DS_CTX *ctx, void *gua, DS_GEO_OBJECT *geo);
void gua_dump(DS_CTX *ctx, void *guap, char *inputFilename, char *outputFilename);

int ngua_spc_read(DS_CTX *ctx, void **ngua, FILE *fp, float *defaultColor);
int ngua_convert_to_object(DS_CTX *ctx, void *nguap, DS_GEO_OBJECT *geo);
