#pragma once
// full set of attributes of geometric data
//typedef struct {
////	GUT_POINT	*vertex;
//	int			nPoly;
//	POLYGON		*polygon;
//} GUA_OFF;

typedef struct {
	void		*vdb;		// unique vertices (coordinates)
	void		*eldb;		// unique edge lengths 
	void		*edb;		// unique edges (2 unique vertex indices) with unique edge
	void		*tedb;		// unique triangles by unique edge lengths
	void		*tdb;		// unique triangles by unique vertex indices along with unique tri by edge
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
