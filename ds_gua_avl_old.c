/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
// Geometry Uniqueness Attributes 
//
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
#include <avl_new.h>
#include <time.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"

//	
//	GUA Library functions - Determining geometry uniqueness from SPC file data 
//	
//	NEW VERSION - Uses AVL trees to sort during data entry - much faster
//
//	gua_tdata_init ( mnc ) // max number of components per block
//	gua_tedata_init ( mnc )
//	gua_eldata_init ( mnc )
//	gua_edata_init ( mnc )
//	gua_vdata_init ( mnc )
//	gua_blk_init ( mnc, data_init ) 
//	gua_db_init ( init, insert, mnc, zero ) // mnc - max number of components per block
//	gua_vtx_dist ( a, b ) 
//	gua_edata_insert ( db, vid, elid )
//	gua_tdata_insert ( db, vid, tid )
//	gua_tedata_insert ( db, eid )
//	gua_eldata_insert ( db, va, vb, len )
//	gua_vdb_dump ( db ) 
//	gua_vdata_insert ( db, vtx ) 
//	gua_parse ( buffer, n, word )
//	gua_db_traverse ( db, ufunc, uctx ) 
//	gua_spc_read ( gua, spc_filename )

#define GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK	1000

// GUA data specific templates
typedef struct {
	char	buffer[64];
} GUA_WORD;

typedef struct {
	GUT_POINT		p;		
	int				id;		// global unique ID - need to save since place in AVL tree will not be in order
} GUA_VDATA;  // vertex data

typedef struct {
	double			len;
	int				id;		// global unique ID 
} GUA_ELDATA; // edge lengths

typedef struct {
	int					vid[2];	// 2 vertex ids
	int					elid;	// unique edge length id
	int					id;		// global ID 
} GUA_EDATA; 	// edges

typedef struct {
	int					elid[3];	// 3 edge length id composite (20 bit each)
	int					id;			// global ID 
} GUA_TEDATA; 	// triangle data

typedef struct {
	int					vid[3];		// comparison id set
	int					ovid[3];	// original order of ids
	int					teid;		// unique triangle ID 
	float				rgb[3];		// explict color 
	int					id;			// global ID 
} GUA_TDATA; 			// triangle data

// generic block of data 
typedef struct {
	void			*next;	// next BLK
	int				nc; 	// number of components used in this block
	void			*data;	// block specific array data[mnc]
} GUA_BLK;			// Generic block  

typedef void *(*GUA_DATA_INIT)(int);

// generic database set
typedef struct {
	void			*p;			// pointer to duplicate node during AVL insert
	void			*head;		// first BLK in database
	void			*avl;		// avl tree for data
	double			zero;		// tolerance (may or may not be used)
	int				mnc;		// max number of components per block
	int				nc;			// total number of components in all blocks
	void			*insert;	// insert function - data specific
	GUA_DATA_INIT	init;		// block init function
	int				nCmp;		// number of compares performed
	int				nDup;		// number of duplicates 
	int				nProcessed; // total number of components processed
	int				nDigits;	// number of precision digits
} GUA_DB;						// General linear database 	 

typedef struct {
	FILE	*fp;
	int		n, m; // format %n.m
} GUA_PASS_THRU;

typedef int(*GUA_EDATA_INS)(void *dbp, int *vid, int *elid);
typedef int(*GUA_TDATA_INS)(void *dbp, int *vid, int *tid, float *rgb);
typedef int(*GUA_TEDATA_INS)(void *dbp, int *eid);
typedef int(*GUA_ELDATA_INS)(void *dbp, GUA_VDATA *va, GUA_VDATA *vb, double *len);
typedef int(*GUA_VDATA_INS)(void *dbp, GUA_VDATA *vtx);
//typedef int (*GUA_USER_FUNC)(void *user_ctx, void *data);
typedef double(*GUA_COMPARE)(GUA_DB *db, void *a, void *b);

static void *gua_tdata_init(int mnc);
static void *gua_tedata_init(int mnc);
static void *gua_eldata_init(int mnc);
static void *gua_edata_init(int mnc);
static void *gua_vdata_init(int mnc);
static void *gua_blk_init(GUA_DB *db); 
static void *gua_db_init(GUA_DATA_INIT init, void *insert, int mnc, GUA_COMPARE); // mnc - max number of components per block
static double gua_vtx_dist(GUA_VDATA *a, GUA_VDATA *b);
static int gua_edata_insert(void *dbp, int *vid, int *elid);
static int gua_tdata_insert(void *dbp, int *vid, int *tid);
static int gua_tedata_insert(void *dbp, int *eid);
static int gua_eldata_insert(void *dbp, GUA_VDATA *va, GUA_VDATA *vb, double *len);
static int gua_vdb_dump(void *dbp);
static int gua_vdata_insert(void *dbp, GUA_VDATA *vtx);
static int gua_parse(char * buffer, int *n, GUA_WORD *word);

#include "ds_gua.h"
static void gua_destroy_db(GUA_DB *db);
static void gua_destroy(GUA_UNIQUE *gua);

//--------------------------------------------------------------------------------
static int v_out(void *passThru, GUA_VDATA *a)
//--------------------------------------------------------------------------------
{
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "v  %6d %*.*f %*.*f %*.*f\n", a->id, pt->n, pt->m, a->p.x, pt->n, pt->m, a->p.y, pt->n, pt->m, a->p.z);
	return 0;
}

//--------------------------------------------------------------------------------
static int el_out(void *passThru, GUA_ELDATA *e)
//--------------------------------------------------------------------------------
{
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "el %6d %*.*f\n", e->id, pt->n, pt->m, e->len);
	return 0;
}

//--------------------------------------------------------------------------------
static int e_out(void *passThru, GUA_EDATA *e)
//--------------------------------------------------------------------------------
{
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "e  %6d %6d %6d\n", e->id, e->vid[0], e->vid[1]); 
	return 0;
}

//--------------------------------------------------------------------------------
static int te_out(void *passThru, GUA_TEDATA *te)
//--------------------------------------------------------------------------------
{
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "te %6d %6d %6d %6d\n", te->id, te->elid[0], te->elid[1], te->elid[2]); 
	return 0;
}

//--------------------------------------------------------------------------------
static int t_out(void *passThru, GUA_TDATA *t)
//--------------------------------------------------------------------------------
{
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "t  %6d %6d %6d %6d %6d\n", t->id, t->vid[0], t->vid[1], t->vid[2], t->teid);
	return 0;
}

//--------------------------------------------------------------------------------
static int xyz_compare(GUA_DB *db, GUA_VDATA *a, GUA_VDATA *b)
//--------------------------------------------------------------------------------
{
	double	diff;
	db->nCmp++;
	diff = a->p.x - b->p.x;
	if (fabs(diff) > db->zero) // unique
		return diff > 0.0 ? 1 : -1; // done - point will inserted into xyz tree

	diff = a->p.y - b->p.y;
	if (fabs(diff) > db->zero) // unique
		return diff > 0.0 ? 1 : -1; // done - point will inserted into xyz tree

	diff = a->p.z - b->p.z;
	if (fabs(diff) > db->zero) // unique
		return diff > 0.0 ? 1 : -1; // done - point will inserted into xyz tree

	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int x_compare(GUA_DB *db, GUA_VDATA *a, GUA_VDATA *b)
//--------------------------------------------------------------------------------
{
	double	xdiff;
	db->nCmp++;
	xdiff = a->p.x - b->p.x;
	if (fabs(xdiff) > db->zero) // unique
		return xdiff > 0.0 ? 1 : -1; // done - point will inserted into x tree
	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int y_compare(GUA_DB *db, GUA_VDATA *a, GUA_VDATA *b)
//--------------------------------------------------------------------------------
{
	double	ydiff;
	db->nCmp++;
	ydiff = a->p.y - b->p.y;
	if (fabs(ydiff) > db->zero) // unique
		return ydiff > 0.0 ? 1 : -1; // done - point will inserted into x tree
	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int z_compare(GUA_DB *db, GUA_VDATA *a, GUA_VDATA *b)
//--------------------------------------------------------------------------------
{
	double	zdiff;
	db->nCmp++;
	zdiff = a->p.z - b->p.z;
	if (fabs(zdiff) > db->zero) // unique
		return zdiff > 0.0 ? 1 : -1; // done - point will inserted into x tree
	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int el_compare(GUA_DB *db, GUA_ELDATA *a, GUA_ELDATA *b)
//--------------------------------------------------------------------------------
{
	double	diff;
	db->nCmp++;
	diff = a->len - b->len;
	if (fabs(diff) > db->zero) // unique
		return diff > 0.0 ? 1 : -1; // done - point will inserted into tree

	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int e_compare(GUA_DB *db, GUA_EDATA *a, GUA_EDATA *b)
//--------------------------------------------------------------------------------
{
	int	diff;
	db->nCmp++;
	diff = a->vid[0] - b->vid[0];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into tree

	diff = a->vid[1] - b->vid[1];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into tree

	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int te_compare(GUA_DB *db, GUA_TEDATA *a, GUA_TEDATA *b)
//--------------------------------------------------------------------------------
{
	int diff;
	db->nCmp++;
	diff = a->elid[0] - b->elid[0];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into tree
	diff = a->elid[1] - b->elid[1];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into tree
	diff = a->elid[2] - b->elid[2];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into tree
	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int t_compare(GUA_DB *db, GUA_TDATA *a, GUA_TDATA *b)
//--------------------------------------------------------------------------------
{
	int	diff;
	db->nCmp++;
	diff = a->vid[0] - b->vid[0];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into x tree
	diff = a->vid[1] - b->vid[1];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into x tree
	diff = a->vid[2] - b->vid[2];
	if (diff) // unique
		return diff > 0 ? 1 : -1; // done - point will inserted into x tree
	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int gua_vdata_insert(GUA_DB *db, GUT_POINT *vtx)
//--------------------------------------------------------------------------------
{
	double		ydiff, zdiff;
	GUA_BLK		*blk;
	GUA_VDATA	*v;

	// need to get storage for vtx 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	v = ((GUA_VDATA*)blk->data) + blk->nc; // ++; // db specific data

	v->p = *vtx; // copy coord data
	v->id = db->nc; // assign an unique ID 

	// first attempt on tree insert 
	db->p = 0;
	if (avl_insert(db->avl, v)) // if fails then insert into a subtree 
	{
		++blk->nc; // incremented number of inserted nodes/vertices
		++db->nc; // incremented number of inserted nodes/vertices
		return v->id; // return unique ID of vertex
	}

	++db->nDup;
	if (!db->p)
		return 0; // error
	return ((GUA_VDATA*)db->p)->id; // duplicate v.id
}

//--------------------------------------------------------------------------------
static int gua_eldata_insert(GUA_DB *db, GUA_ELDATA *va, GUA_ELDATA *vb, double *len)
//--------------------------------------------------------------------------------
{
	GUA_BLK		*blk;
	GUA_ELDATA	*e;

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	e = ((GUA_ELDATA*)blk->data) + blk->nc; // db specific data

	*len = gua_vtx_dist(va, vb); // length of edge  
	e->len = *len;
	e->id = db->nc; // assign an unique ID 

	db->p = 0;
	// first attempt tree insert 
	if (avl_insert(db->avl, e)) // if fails then insert into a subtree 
	{
		++blk->nc; // incremented number of inserted nodes/vertices
		++db->nc; // incremented number of inserted nodes/vertices
		return e->id; //db.p.id // return unique ID of vertex
	}

	++db->nDup;
	if (!db->p)
		return 0; // error
	return ((GUA_ELDATA*)db->p)->id; // duplicate e.id
}

//--------------------------------------------------------------------------------
static int gua_edata_insert(GUA_DB *db, int *vid, int elid)
//--------------------------------------------------------------------------------
{
	GUA_BLK				*blk;
	GUA_EDATA			*e;
	long long			seq;

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	e = ((GUA_EDATA*)blk->data) + blk->nc; // db specific data

	if (vid[0] < vid[1])
	{
		e->vid[0] = vid[0];
		e->vid[1] = vid[1];
	}
	else
	{
		e->vid[0] = vid[1];
		e->vid[1] = vid[0];
	}

	e->elid = elid;
	e->id = db->nc; // assign an unique ID 

	db->p = 0;
	// first attempt at tree insert 
	if (avl_insert(db->avl, (void*)e)) // if fails then insert into a subtree 
	{
		++blk->nc; // incremented number of inserted nodes/vertices
		++db->nc; // incremented number of inserted nodes/vertices
		return e->id; //db.p.id // return unique ID of vertex
	}

	++db->nDup;
	if (!db->p)
		return 0; //error
	return ((GUA_EDATA*)db->p)->id; // duplicate e.id
}

//--------------------------------------------------------------------------------
static int gua_tedata_insert(GUA_DB *db, int *eid)
//--------------------------------------------------------------------------------
{
	GUA_BLK				*blk;
	GUA_TEDATA			*te;
	long long			seq;
	int					tmp;

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	te = ((GUA_TEDATA*)blk->data) + blk->nc; // db specific data

	// sort the edge ID array 
	if (eid[1] < eid[0])
	{
		tmp = eid[0];
		eid[0] = eid[1];
		eid[1] = tmp;
	}
	if (eid[2] < eid[1])
	{
		tmp = eid[1];
		eid[1] = eid[2];
		eid[2] = tmp;
		if (eid[1] < eid[0])
		{
			tmp = eid[0];
			eid[0] = eid[1];
			eid[1] = tmp;
		}
	}

	te->elid[0] = eid[0];
	te->elid[1] = eid[1];
	te->elid[2] = eid[2];

	te->id = db->nc; // assign an unique ID 

	db->p = 0;
	// first attempt at tree insert 
	if (avl_insert(db->avl, (void*)te)) // if fails then insert into a subtree 
	{
		++blk->nc; // incremented number of inserted nodes/vertices
		++db->nc; // incremented number of inserted nodes/vertices
		return te->id; //db.p.id // return unique ID of vertex
	}

	++db->nDup;
	if (!db->p)
		return 0; // error
	return ((GUA_TEDATA*)db->p)->id; // duplicate e.id
}

//--------------------------------------------------------------------------------
static int gua_tdata_insert(GUA_DB *db, int *vid, int teid, float *rgb)
//--------------------------------------------------------------------------------
{
	GUA_BLK				*blk;
	GUA_TDATA			*t;
	long long			seq;
	long long			oseq;
	int					tmp;
	int					ovid[3];

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	t = ((GUA_TDATA*)blk->data) + blk->nc; // db specific data

	// save original vertex id order
	t->ovid[0] = vid[0];
	t->ovid[1] = vid[1];
	t->ovid[2] = vid[2];

	// sort the unique vertex ID array 
	// sort the edge ID array 
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

	// save sorted vertex id order
	t->vid[0] = vid[0];
	t->vid[1] = vid[1];
	t->vid[2] = vid[2];

	t->teid = teid; // save the unique triangle ID by edge set
	t->id = db->nc; // assign an unique ID 
	t->rgb[0] = rgb[0];
	t->rgb[1] = rgb[1];
	t->rgb[2] = rgb[2];

	db->p = 0;
	// first attempt at tree insert 
	if (avl_insert(db->avl, (void*)t)) // if fails then insert into a subtree 
	{
		++blk->nc; // incremented number of inserted nodes/vertices
		++db->nc; // incremented number of inserted nodes/vertices
		return t->id; //db.p.id // return unique ID of vertex
	}

	++db->nDup;
	if (!db->p)
		return 0; // error
	return ((GUA_TDATA*)db->p)->id; // duplicate e.id
}

//--------------------------------------------------------------------------------
static void *gua_tdata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of triangles 
	void *p;
	p = (void*)malloc(sizeof(GUA_TDATA) * mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_tedata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of triangles 
	void *p;
	p = (void*)malloc(sizeof(GUA_TEDATA) * mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_eldata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of edge lengths 
	void *p;
	p = (void*)malloc(sizeof(GUA_ELDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_edata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of edges 
	void *p;
	p = (void*)malloc(sizeof(GUA_EDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_vdata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of vertices 
	void *p;
	p = (void*)malloc(sizeof(GUA_VDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_blk_init(GUA_DB *db) //int mnc, GUA_DATA_INIT data_init)
//--------------------------------------------------------------------------------
{
	//	CREATE a BLK and initialize with data specific init function
	GUA_BLK	*blk;
	blk			= (GUA_BLK*)malloc(sizeof(GUA_BLK));
	blk->nc		= 0; // initial
	blk->next	= db->head; // linked list pointer
	blk->data	= db->init(db->mnc); // data specific init 
	db->head	= (void*)blk;
	return (void*)blk;
}

//--------------------------------------------------------------------------------
static void *gua_db_init(GUA_DATA_INIT init, void *insert, int mnc, GUA_COMPARE compare) // mnc - max number of components per block
//--------------------------------------------------------------------------------
{
	//	Create a general database with an initial BLK of any type
	GUA_DB *db;
	db = (GUA_DB*)malloc(sizeof(GUA_DB));
	db->p			= 0;						// pointer used during AVL insert
	db->nc			= 0;						// total # of unique components in all blocks
	db->mnc			= mnc; 						// max number of components per block
//	db->zero		= zero;		 				// generic tolerance 
	db->insert		= insert;					// insert function
	db->init		= init;						// block init function 
//	db->nc			= 0;
	db->nDup		= 0;
	db->nCmp		= 0;
	db->nProcessed	= 0;
	db->avl			= avl_create(compare, (void*)db );
	db->head		= 0;
	gua_blk_init(db);						// db specific init 
	return (void*)db;
}

//--------------------------------------------------------------------------------
static double gua_vtx_dist(GUT_POINT *a, GUT_POINT *b)
//--------------------------------------------------------------------------------
{
	//	Measure distance between two vertices
	double	dx, dy, dz;
	dx = b->x - a->x;
	dy = b->y - a->y;
	dz = b->z - a->z;
	return sqrt(dx * dx + dy * dy + dz * dz);
}

//--------------------------------------------------------------------------------
static int gua_parse(char *buffer, int *n, GUA_WORD *word)
//--------------------------------------------------------------------------------
{
	// 	This is a very simple word parser 
	//	Copies each word into word[] buffer and returns the number of words found
	//
	int		i = 0, j=0, m, in=0;
	char	c;
	*n = 0;
	while (c = buffer[i++])
	{
		if (c == 10)
			break;
		else if (c == ' ' || c == '\t')
		{
			if (in)
			{
				in = 0;
				++*n;
				j = 0;
			}
		}
		else
		{
			in = 1;
			word[*n].buffer[j++] = c;
			word[*n].buffer[j] = 0;
		}
	}
	if (in)
		++*n;

	return 0;
}

//--------------------------------------------------------------------------------
static void gua_destroy_vdb(GUA_DB *db)
//--------------------------------------------------------------------------------
{
	GUA_BLK		*blk, *nextBlk;
	GUA_VDATA	*data;
	int			i;

	blk = db->head;
	while (blk)
	{
		data = (GUA_VDATA*)blk->data;
		nextBlk = blk->next;	// save pointer to next block
		free(blk->data);		// free all data in the block
		free(blk);				// free the block
		blk = nextBlk;			// copy pointer back to continue
	}
	avl_destroy(db->avl,0);
	free(db);
}

//--------------------------------------------------------------------------------
static void gua_destroy_db(GUA_DB *db)
//--------------------------------------------------------------------------------
{
	GUA_BLK *blk, *nextBlk;

	blk = db->head;
	while (blk)
	{
		nextBlk = blk->next;	// save pointer to next block
		free(blk->data);		// free all data in the block
		free(blk);				// free the block
		blk = nextBlk;			// copy pointer back to continue
	}
	avl_destroy(db->avl,0);
	free(db);
}

//--------------------------------------------------------------------------------
static void gua_destroy(GUA_UNIQUE *gua)
//--------------------------------------------------------------------------------
{
	//	Free up all memory associated with object

	gua_destroy_db((GUA_DB*)gua->edb); // needs special avl_destroy
	gua_destroy_db((GUA_DB*)gua->eldb);
	gua_destroy_db((GUA_DB*)gua->tedb);
	gua_destroy_db((GUA_DB*)gua->tdb);
	gua_destroy_vdb((GUA_DB*)gua->vdb);

	free(gua); // final free
}

//--------------------------------------------------------------------------------
int gua_convert_to_object(CTX *ctx, void *guap, GEO_OBJECT *geo)
//--------------------------------------------------------------------------------
{
	//	When finished free up all the memory associated with the GUA 
	//
	GUA_UNIQUE	*gua = (GUA_UNIQUE*)guap;
	GUA_DB		*vdb;  // vertex DB      - unqiue coordinates 
	GUA_DB		*edb;  // edge DB        - unique vertex pairs 
	GUA_DB		*eldb; // edge length DB - unique edge lengths  
	GUA_DB		*tedb; // triangle DB    - unique edge length IDs 
	GUA_DB		*tdb;  // triangle DB    - unqiue vertex ID sets 
	GUA_BLK		*blk;
	int			i, n;
	int			nBlk;
	GUA_VDATA	*vdata;

		// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	eldb = (GUA_DB*)gua->eldb;
	edb  = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb  = (GUA_DB*)gua->tdb;

	// convert unique geo data to geo object

	// vertex array 
	// allocate array
	geo->vtx  = (GUT_POINT*)malloc(sizeof(GUT_POINT)*vdb->nc);
	geo->nVtx = vdb->nc;

	// transfer data 
	blk = (GUA_BLK*)vdb->head;
	n = 0;
	while (blk)
	{
		GUA_VDATA *data = (GUA_VDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			geo->vtx[n++] = *(VERTEX*)&data[i].p;
		}

		blk = (GUA_BLK*)blk->next;
	}

	if (!gua->off.polygon) // this is normal SPC data
	{
		// tri array 
		// allocate array
		geo->tri = (TRIANGLE*)malloc(sizeof(TRIANGLE)*tdb->nc);
		geo->nTri = tdb->nc;
//		geo->
		// transfer data 
		blk = (GUA_BLK*)tdb->head;
		n = 0;
		nBlk = 0;
		while (blk)
		{
			++nBlk;
			GUA_TDATA *data = (GUA_TDATA*)blk->data;
			for (i = 0; i < blk->nc; ++i) // go thru all components of this block
			{
				geo->tri[n].color = *(COLOR*)&data[i].rgb;	// explict color
				geo->tri[n].id = data[i].teid;			// unique ID 
				geo->tri[n].vtx[0] = data[i].ovid[0];		// unique vertex IDs
				geo->tri[n].vtx[1] = data[i].ovid[1];
				geo->tri[n].vtx[2] = data[i].ovid[2];
				geo->tri[n].nVtx = 3;
				++n;
			}

			blk = (GUA_BLK*)blk->next;
		}
	}
	else // OFF data
	{
		geo->nTri = gua->off.nPoly;
		geo->poly = gua->off.polygon;
	}

	// EDGES ARE SAME FOR SPC & OFF 
	// edge array 
	// allocate array
	geo->edge  = (EDGE*)malloc(sizeof(EDGE)*edb->nc);
	geo->nEdge = edb->nc;
	// transfer data 
	blk = (GUA_BLK*)edb->head;
	n = 0;
	while (blk)
	{
		GUA_EDATA *data = (GUA_EDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			geo->edge[n].id     = data[i].elid;		// unique ID 
			geo->edge[n].vtx[0] = data[i].vid[0];	// unique vertex IDs
			geo->edge[n].vtx[1] = data[i].vid[1];	
			++n;
		}

		blk = (GUA_BLK*)blk->next;
	}

	// transfer data - capture min and max lengths
	blk = (GUA_BLK*)eldb->head;
	n = 0;
	while (blk)
	{
		GUA_ELDATA *data = (GUA_EDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			if (!i)
			{
				ctx->eAttr.minLength = data[i].len;
				ctx->eAttr.maxLength = data[i].len;
			}
			else
				ctx->eAttr.maxLength = data[i].len;
			++n;
		}

		blk = (GUA_BLK*)blk->next;
	}

	if (!gua->off.nPoly)
		geo->nUTri = tedb->nc; // number of geometrically different triangles (same set of edge lengths)
	else
		geo->nUTri = gua->off.nPoly; // tedb->nc; // number of geometrically different triangles (same set of edge lengths)

	geo->nUEdge = eldb->nc; // number of geometrically different edges (by length)

	geo->ctE = ctbl_get_color_table(&ctx->cts, eldb->nc);
	geo->ctT = ctbl_get_color_table(&ctx->cts, tedb->nc);
	geo->ctB = ctbl_get_color_table(&ctx->cts, tedb->nc + edb->nc);

	geo->vIndex = gua->vIndex; // pointer to array of all vertex indices - need for free() when object is destroyed

	gua_destroy(gua); // total memory deallocation
	return 0;
}

//--------------------------------------------------------------------------------
static double set_tolerance(char *string, int *flag, int *nDigits )
//--------------------------------------------------------------------------------
{
	char		c, buf[32];
	int			i = 0, n = 0;

	strcpy(buf, "0.0");

	// count the number of places after the decimal
	while (c = string[i++])
	{
		if (c == '.' && !*flag ) // only come here once
		{
			*flag = 1; // start counting 
			buf[n++] = c; // save the decimal
			buf[n] = 0;
		}
		else if (*flag && n < 31)
		{
			buf[n++] = '0'; // add a zero
			buf[n] = 0;
		}
	}

	// n will be number of decimal place plus one 
	// thus: ".12345" n = 6
	if (*flag)
	{
		buf[n - 1] = '7';
		buf[n - 0] = 0;
	}
	if (n)
		*nDigits = n - 1;
	else
		*nDigits = 1;
	return atof(buf);
}

//--------------------------------------------------------------------------------
int gua_spc_read(CTX *ctx, void **guap, FILE *fp, float *defaultColor)
//--------------------------------------------------------------------------------
{
	// Consume a SPC file and determine uniqueness for all vertices/edges/triangles
	//
	// Automatically determines zero tolerance for Vertex & Edges from number of 
	// significant digits in first vertex string
	//
	int				mnc = GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK;
	GUA_UNIQUE		*gua;			// = (GUA_UNIQUE*)guap;
	char			*bp;
	GUA_DB			*vdb;			// vertex DB      - unqiue coordinates 
	GUA_DB			*edb;			// edge DB        - unique vertex pairs 
	GUA_DB			*eldb;			// edge length DB - unique edge lengths  
	GUA_DB			*tedb;			// triangle DB    - unique edge length IDs 
	GUA_DB			*tdb;			// triangle DB    - unqiue vertex ID sets 
	int				i, j, k;
	int				vid[3]; 		// unique vertex IDs for current triangle
	int				elid[3];		// unique edge length IDs for current triangle 
	int				eid[3];			// unigue edge by unique vertex ID's for current triangle
	int				tid;			// unique triangle by unique edge lengths
	int				ttid;			// unique triangle by unique vertex IDs
	char			buffer[256];
	GUA_WORD		word[32];
	int				n, m; 				// number of words parsed
	GUA_VDATA		v[3];			// vertex coordinate data for current triangle 
	double			elen[3];		// length of three edges of current triangle
	float			rgb[3];			// color of triangle
	int				explicitColor;	// color flag 
	int				maxVID  = 0;
	int				nDigits = 0;
	int				decimal = 0;
	GUT_POINT		vpt[3];
	GUT_POINT		vpt_dup[3];
	GUT_POINT		vtmp[3];		// vertex coordinate data for current triangle 

	if (!fp) return 1; // error 

	// create a GUA structure
	gua = (GUA_UNIQUE*)malloc(sizeof(GUA_UNIQUE)); // guap;
	if (gua == NULL) return 1; // error
	*guap = (void*)gua; // pass pointer back 

	// create all the attribute databases
	gua->vdb  = gua_db_init(gua_vdata_init, gua_vdata_insert, mnc, xyz_compare);	// vertex DB
	gua->eldb = gua_db_init(gua_eldata_init, gua_eldata_insert, mnc, el_compare);	// edge by length DB
	gua->edb  = gua_db_init(gua_edata_init, gua_edata_insert, mnc, e_compare);		// edge by unique vertex pairs DB
	gua->tedb = gua_db_init(gua_tedata_init, gua_tedata_insert, mnc, te_compare);	// triangle by unique edges DB
	gua->tdb  = gua_db_init(gua_tdata_init, gua_tdata_insert, mnc, t_compare);		// triangle by unique vertices DB

	// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	eldb = (GUA_DB*)gua->eldb;
	edb  = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb  = (GUA_DB*)gua->tdb;

	// data specific insert functions from each database with correct cast
	GUA_VDATA_INS  vins = (GUA_VDATA_INS)vdb->insert;
	GUA_EDATA_INS  eins = (GUA_EDATA_INS)edb->insert;
	GUA_ELDATA_INS elins = (GUA_ELDATA_INS)eldb->insert;
	GUA_TEDATA_INS teins = (GUA_TEDATA_INS)tedb->insert;
	GUA_TDATA_INS  tins = (GUA_TDATA_INS)tdb->insert;

	while (bp = fgets(buffer, 256, fp)) // read a line of 3 vertex coordinates
	{
		gua_parse(buffer, &n, word);
		if (!n || n < 9)
		{
			continue; // empty or invalid line
		}

		if (!decimal)
		{
			int	nDigits;
			vdb->zero = eldb->zero = set_tolerance(word[0].buffer, &decimal, &nDigits);
			vdb->nDigits = eldb->nDigits = nDigits;
		}
		// process 3 vertices  
		for (i = 0, j = 0; i < 3; ++i, j += 3)
		{
			vpt[i].x = atof(word[j + 0].buffer);
			vpt[i].y = atof(word[j + 1].buffer);
			vpt[i].z = atof(word[j + 2].buffer);
			vpt[i].w = 1.0;
		}
		// determine color to assign 
		explicitColor = 0;
		if (n >= 12) // extract explicit color info
		{
			explicitColor = 1;
			rgb[0] = atof(word[9].buffer);
			rgb[1] = atof(word[10].buffer);
			rgb[2] = atof(word[11].buffer);
		}
		else
		{
			rgb[0] = defaultColor[0];
			rgb[1] = defaultColor[1];
			rgb[2] = defaultColor[2];
		}

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			// NEW SECTION
			// IF replication and transformation are defined in the command line then:
			// we will first transform each vertex then replicate in Z 120 degree rotation.
			// This will be all done prior to insertion. At insertion duplicates that
			// may result will be culled out
			mtx_vector_multiply(3, &vpt, &vtmp, &ctx->inputTrans.matrix[0]);
			vpt[0] = vtmp[0];
			vpt[1] = vtmp[1];
			vpt[2] = vtmp[2];
		}

		// replicate as required
		m = ctx->inputTrans.mirrorFlag ? 3 : 1;
		n = ctx->inputTrans.replicateFlag ? 3 : 1;
		for (k = 0; k < m; ++k)
		{
			if (!k)
			{
				vpt_dup[0] = vpt[0];
				vpt_dup[1] = vpt[2];
				vpt_dup[2] = vpt[1];
			}
			else
			{	// mirror the x coord
				vpt[0] = vpt_dup[0]; vpt[0].x *= -1;
				vpt[1] = vpt_dup[1]; vpt[1].x *= -1;
				vpt[2] = vpt_dup[2]; vpt[2].x *= -1;
			}

			for (j = 0; j < n; ++j)
			{
				for (i = 0; i < 3; ++i)
				{
					v[i].p = vpt[i]; // copy coordinate data into node structure
					vid[i] = vins((void*)vdb, &v[i]); // insert into tree
					++vdb->nProcessed;
				}

				// for each vertex pair determine the unique edge length id  
				for (i = 0; i < 3; ++i)
				{
					elid[i] = elins((void*)eldb, &v[i + 0], &v[(i + 1) % 3], &elen[i]);
					++eldb->nProcessed;
				}

				// for each vertex pair (3 of them) determine the unique edge id 
				// add edges by vertex ID's
				{
					int	vuid[2], elnid;
					for (i = 0; i < 3; ++i)
					{
						vuid[0] = vid[i];
						vuid[1] = vid[(i + 1) % 3];
						elnid = elid[i];
						++edb->nProcessed;
						eid[i] = eins((void*)edb, vuid, elnid); // save the vertex pair along with the edge length id 
					}
				}

				++tedb->nProcessed;
				tid = teins((void*)tedb, elid); // use array of edge length IDs 
				++tdb->nProcessed;
				ttid = tins((void*)tdb, vid, tid, rgb); // use array of unique vertex IDs 

				if (ctx->inputTrans.replicateFlag && j < 2)
				{	// perform rotation transformation on coordinate data
					mtx_vector_multiply(3, &vpt[0], &vtmp[0], &ctx->inputTrans.matrix[1]); // 120 degree rotation of z axis
					vpt[0] = vtmp[0];
					vpt[1] = vtmp[1];
					vpt[2] = vtmp[2];
				}
			}
		}
	}

	return 0; // success
}

//--------------------------------------------------------------------------------
void gua_dump(CTX *ctx, void *guap, char *inputFilename, char *outputFilename )
//--------------------------------------------------------------------------------
{
	if (!ctx || !guap || !inputFilename || !outputFilename)
		return;

	GUA_UNIQUE		*gua = (GUA_UNIQUE*)guap;
	GUA_DB			*vdb;  // vertex DB      - unqiue coordinates 
	GUA_DB			*edb;  // edge DB        - unique vertex pairs 
	GUA_DB			*eldb; // edge length DB - unique edge lengths  
	GUA_DB			*tedb; // triangle DB    - unique edge length IDs 
	GUA_DB			*tdb;  // triangle DB    - unqiue vertex ID sets 
	GUA_PASS_THRU	passThru;
	FILE			*fp;

	// local copies re-casted
	vdb = (GUA_DB*)gua->vdb;
	eldb = (GUA_DB*)gua->eldb;
	edb = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb = (GUA_DB*)gua->tdb;
	
	passThru.fp = fp = fopen(outputFilename,"w");
	if (!passThru.fp)
		return; // failed to open
	passThru.m = vdb->nDigits;
	passThru.n = vdb->nDigits + 4;

	fprintf(fp, "DisplaySphere - Unique Geometry Processing\n" );
	
	// Get current date/time, format is YYYY-MM-DD.HH:mm:ss	
	time_t     now = time(0);
	struct tm  tstruct;
	char       buf[80];
	tstruct = *localtime(&now);
	strftime(buf, sizeof(buf), "%Y-%m-%d.%X", &tstruct);

	fprintf(fp, "Time:                            %s\n", buf);

	fprintf(fp, "FILE PROCESSED:                  %s\n", inputFilename);
	fprintf(fp, "VERTICES                         #unique %7d #processed %7d  #duplicates %7d  #compares %7d\n", vdb->nc, vdb->nProcessed, vdb->nDup, vdb->nCmp);
	fprintf(fp, "EDGE LENGTHS                     #unique %7d #processed %7d  #duplicates %7d  #compares %7d\n", eldb->nc, eldb->nProcessed, eldb->nDup, eldb->nCmp);
	fprintf(fp, "EDGES BY UNIQUE VERTEX PAIRS     #unique %7d #processed %7d  #duplicates %7d  #compares %7d\n", edb->nc, edb->nProcessed, edb->nDup, edb->nCmp);
	fprintf(fp, "TRIANGLES BY UNIQUE EDGE LENGTHS #unique %7d #processed %7d  #duplicates %7d  #compares %7d\n", tedb->nc, tedb->nProcessed, tedb->nDup, tedb->nCmp);
	fprintf(fp, "TRIANGLES BY UNIQUE VERTEX ID    #unique %7d #processed %7d  #duplicates %7d  #compares %7d\n", tdb->nc, tdb->nProcessed, tdb->nDup, tdb->nCmp);
	fprintf(fp, "PRECISION ZERO:                  %*.*f (based on input data)\n", passThru.n, passThru.m, vdb->zero);
	
	fprintf(fp, "Unique Vertices - ordered ( v ID X Y Z )\n");
	avl_traverse_rtl(vdb->avl,  (void*)&passThru,  v_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Edge Lengths - ordered ( el ID length )\n");
	avl_traverse_rtl(eldb->avl, (void*)&passThru, el_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Edges by Vertex Pairs - ordered ( e ID v1 v2  )\n");
	avl_traverse_rtl(edb->avl,  (void*)&passThru,  e_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Triangles by Unique Edge Lengths - ordered ( te ID el1 el2 el3 )\n");
	avl_traverse_rtl(tedb->avl, (void*)&passThru, te_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Triangles by Unique Vertices - ordered ( t ID v1 v2 v3  teID )\n");
	avl_traverse_rtl(tdb->avl,  (void*)&passThru,  t_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned

	fclose(fp);
}

typedef struct {
	VTX_CLR		*block;
	void		*next;
	int			nc;
	void		*data;
}NGUA_BLK;

typedef struct {
	void		*head;
	void		*tail;
	int			mnc;
	int			nc;
}NGUA_DB;

//--------------------------------------------------------------------------------
int ngua_spc_read(CTX *ctx, void **nguap, FILE *fp, float *defaultColor)
//--------------------------------------------------------------------------------
{
	NGUA_BLK	*blk;
	NGUA_DB		*db;
	GUA_WORD	word[32];
	int			n, m;
	int			i, j, k; 
	char		buffer[256];
	VTX_CLR		*vc;
	VTX_CLR		vct;
	VERTEX		vpt[3], vpt_dup[3];
	VERTEX		vtmp[3];

	// initialize database 
	db			= (NGUA_DB*)malloc(sizeof(NGUA_DB));
	db->nc		= 0;
	db->mnc		= GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK;
	db->head	= db->tail = 0;
	blk			= (NGUA_BLK*)malloc(sizeof(NGUA_BLK));
	blk->nc		= 0;
	blk->next	= 0;
	blk->data	= (VTX_CLR*)malloc(sizeof(VTX_CLR)*db->mnc);
	db->head	= (void*)blk;
	db->tail	= (void*)blk;
	*nguap		= (void*)db; // send back to caller

	while (fgets(buffer, 256, fp))
	{
		// parse out the number of triangles
		gua_parse(buffer, &n, word);

		if (n < 9)
			continue; // not enough data

		// read the data
		for (i = 0, j = 0; i < 3; ++i, j += 3)
		{
			vpt[i].x = atof(word[j + 0].buffer);
			vpt[i].y = atof(word[j + 1].buffer);
			vpt[i].z = atof(word[j + 2].buffer);
			vpt[i].w = 1;
		}

		if (n >= 12) // color included
		{
			vct.color.r = atof(word[9].buffer);
			vct.color.g = atof(word[10].buffer);
			vct.color.b = atof(word[11].buffer);
		}
		else
		{
			vct.color.r = defaultColor[0]; // default
			vct.color.g = defaultColor[1]; // default
			vct.color.b = defaultColor[2]; // default
		}

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			mtx_vector_multiply(3, &vpt, &vtmp, &ctx->inputTrans.matrix[0]);
			vpt[0] = vtmp[0];
			vpt[1] = vtmp[1];
			vpt[2] = vtmp[2];
		}

		// replicate as required
		m = ctx->inputTrans.mirrorFlag ? 2 : 1;
		n = ctx->inputTrans.replicateFlag ? 3 : 1;
		for (k = 0; k < m; ++k) // mirrorFlag
		{
			if (!k) // make a copy of the original data and switch order
			{
				vpt_dup[0] = vpt[0];
				vpt_dup[1] = vpt[2];
				vpt_dup[2] = vpt[1];
			}
			else
			{	// mirror the x coord
				vpt[0] = vpt_dup[0]; vpt[0].x *= -1;
				vpt[1] = vpt_dup[1]; vpt[1].x *= -1;
				vpt[2] = vpt_dup[2]; vpt[2].x *= -1;
			}

			for (j = 0; j < n; ++j)
			{
				if (blk->nc == db->mnc)
				{
					blk = (NGUA_BLK*)malloc(sizeof(NGUA_BLK));
					blk->nc = 0;
					blk->next = 0;
					blk->data = (VTX_CLR*)malloc(sizeof(VTX_CLR)*db->mnc);
					((NGUA_BLK*)db->tail)->next = blk;
					db->tail = (void*)blk;
				}
				vc = (VTX_CLR*)blk->data + blk->nc++;
				++db->nc;

				vc->color = vct.color;

				for (i = 0; i < 3; ++i)
				{
					vc->vertex[i] = vpt[i]; // copy coordinate data into node structure
				}

				if (ctx->inputTrans.replicateFlag && j < 2)
				{	// perform rotation transformation on coordinate data
					mtx_vector_multiply(3, &vpt[0], &vtmp[0], &ctx->inputTrans.matrix[1]); // 120 degree rotation of z axis
					vpt[0] = vtmp[0];
					vpt[1] = vtmp[1];
					vpt[2] = vtmp[2];
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------------
int ngua_convert_to_object(CTX *ctx, void *nguap, GEO_OBJECT *geo)
//--------------------------------------------------------------------------------
{
	//	When finished free up all the memory associated with the NGUA 
	//
	NGUA_BLK	*blk, *nblk;
	NGUA_DB		*db;
	GUA_WORD	word[32];
	int			n;
	int			i, j;
	char		buffer[256];
	int			count = 0;
	VTX_CLR		*vc;
	db = (NGUA_DB*)nguap;

	// convert non-unique geo data to geo object

	// vertex array 
	// allocate array
	geo->vtx = (VERTEX*)malloc(sizeof(VERTEX)*db->nc*3);
	geo->nVtx = db->nc* 3;

	// transfer vertex data 
	blk = (NGUA_BLK*)db->head;
	n = 0;
	while (blk)
	{
		vc = (VTX_CLR*)blk->data;
		for (i = 0; i < blk->nc; ++i, ++vc) // go thru all components of this block
		{
			geo->vtx[n++] = *(VERTEX*)&vc->vertex[0];
			geo->vtx[n++] = *(VERTEX*)&vc->vertex[1];
			geo->vtx[n++] = *(VERTEX*)&vc->vertex[2];
		}

		blk = (NGUA_BLK*)blk->next;
	}

	// tri array 
	// allocate array
	geo->tri	= (TRIANGLE*)malloc(sizeof(TRIANGLE)*db->nc);
	geo->nTri	= db->nc;
	// transfer data 
	blk			= (NGUA_BLK*)db->head;
	n			 = 0;
	j			 = 0;
	while (blk)
	{
		vc = (VTX_CLR*)blk->data;
		for (i = 0; i < blk->nc; ++i, ++vc) // go thru all components of this block
		{
			geo->tri[n].color = *(COLOR*)&vc->color;					// explict color
			geo->tri[n].id = n;											// unique ID 
			geo->tri[n].vtx[0] = j++;	// v[0];			// unique vertex IDs
			geo->tri[n].vtx[1] = j++;	// v[1];
			geo->tri[n].vtx[2] = j++;	// v[2];
			++n;
		}

		blk = (NGUA_BLK*)blk->next;
	}

	// edge array 
	// allocate array
	geo->edge = 0; // (EDGE*)malloc(sizeof(EDGE)*edb->nc);
	geo->nEdge = 0; // edb->nc;

	geo->nUTri = db->nc; // number of geometrically different triangles (same set of edge lengths)
	geo->nUEdge = 0; // number of geometrically different edges (by length)

	geo->ctE = 0; // ctbl_get_color_table(&ctx->cts, eldb->nc);
	geo->ctT = ctbl_get_color_table(&ctx->cts, db->nc);
	geo->ctB = ctbl_get_color_table(&ctx->cts, db->nc);

	// total memory deallocation
	blk = (NGUA_BLK*)db->head;
	while (blk)
	{
		nblk = (NGUA_BLK*)blk->next;
		free(blk->data);
		free(blk);
		blk = nblk;
	}
	free(db);
	return 0;
}

//--------------------------------------------------------------------------------
int gua_off_read(CTX *ctx, void **guap, FILE *fp, float *defaultColor)
//--------------------------------------------------------------------------------
{
	// Consume an OFF file and determine uniqueness for all edges
	//
	// Automatically determines zero tolerance for Vertex & Edges from number of 
	// significant digits in first vertex string
	//
	int				mnc = GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK;
	GUA_UNIQUE		*gua;			// = (GUA_UNIQUE*)guap;
	char			*bp;
	GUA_DB			*vdb;			// vertex DB      - unqiue coordinates 
	GUA_DB			*edb;			// edge DB        - unique vertex pairs 
	GUA_DB			*eldb;			// edge length DB - unique edge lengths  
	GUA_DB			*tedb;			// triangle DB    - unique edge length IDs 
	GUA_DB			*tdb;			// triangle DB    - unqiue vertex ID sets 
	int				i, j, k, l;		// counters
	int				vid[32]; 		// unique vertex IDs for current triangle
	int				elid[32];		// unique edge length IDs for current triangle 
	int				eid[32];		// unigue edge by unique vertex ID's for current triangle
	int				tid;			// unique triangle by unique edge lengths
	int				ttid;			// unique triangle by unique vertex IDs
	char			buffer[256];
	GUA_WORD		word[32];
	int				n, m; 			// number of words parsed
	GUA_VDATA		v[32];			// vertex coordinate data for current triangle 
	double			elen[32];		// length of three edges of current triangle
	float			rgb[3];			// color of triangle
	int				explicitColor;	// color flag 
	int				maxVID = 0;
	int				nDigits = 0;
	int				decimal = 0;
	GUT_POINT		vpt[32];
	GUT_POINT		vpt_dup[32];
	GUT_POINT		vtmp[32];		// vertex coordinate data for current triangle 
	int				nv;
	int				nf;
	int				ne;
	GUT_POINT		*ov;
	POLYGON			*of;
	int				*ocf; // off color flag
	int				*ofvi; // array for all vertex indices across faces 
	int				modulo;
	COLOR			*clr;
	int				nVIndx = 0; // sum of all vertex indices for all faces
	int				*vindx;

	if (!fp) return 1; // error 

	bp = fgets(buffer, 256, fp); // read first line
	gua_parse(buffer, &n, word);
	if (!n || strcmp(word[0].buffer, "OFF"))
	{
		rewind(fp); // can't process
		return 1; // error
	}
	
	// create a GUA structure
	gua = (GUA_UNIQUE*)malloc(sizeof(GUA_UNIQUE)); // guap;
	if (gua == NULL) return 1; // error
	*guap = (void*)gua; // pass pointer back 

	// create all the attribute databases
	gua->vdb = gua_db_init(gua_vdata_init, gua_vdata_insert, mnc, xyz_compare);		// vertex DB
	gua->eldb = gua_db_init(gua_eldata_init, gua_eldata_insert, mnc, el_compare);	// edge by length DB
	gua->edb = gua_db_init(gua_edata_init, gua_edata_insert, mnc, e_compare);		// edge by unique vertex pairs DB
	gua->tedb = gua_db_init(gua_tedata_init, gua_tedata_insert, mnc, te_compare);	// triangle by unique edges DB
	gua->tdb = gua_db_init(gua_tdata_init, gua_tdata_insert, mnc, t_compare);		// triangle by unique vertices DB
	gua->off.polygon   = 0;
	gua->vIndex		   = 0; 
	gua->off.nPoly	   = 0;
	// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	eldb = (GUA_DB*)gua->eldb;
	edb  = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb  = (GUA_DB*)gua->tdb;

	// data specific insert functions from each database with correct cast
	GUA_VDATA_INS  vins = (GUA_VDATA_INS)vdb->insert;
	GUA_EDATA_INS  eins = (GUA_EDATA_INS)edb->insert;
	GUA_ELDATA_INS elins = (GUA_ELDATA_INS)eldb->insert;
	GUA_TEDATA_INS teins = (GUA_TEDATA_INS)tedb->insert;
	GUA_TDATA_INS  tins = (GUA_TDATA_INS)tdb->insert;

	while (bp = fgets(buffer, 256, fp)) // read a line of 3 vertex coordinates
	{
		gua_parse(buffer, &n, word);
		if (!n)
			continue; // empty or invalid line
		else if (word[0].buffer[0] == '#')
			continue;
		else if (n == 3)
			break;
		else
		{
			rewind(fp);
			return 1;
		}
	}
	
	nv = atoi(word[0].buffer); // number of vertices
	nf = atoi(word[1].buffer); // number of faces
	ne = atoi(word[2].buffer); // number of edges

	ov = (GUT_POINT*)malloc(sizeof(GUT_POINT)*nv); // array storage for all vertices 
	gua->off.polygon = of = (POLYGON*)malloc(sizeof(POLYGON)*nf);		// array storage for polygon faces 
	gua->off.nPoly = nf;
	ocf = (int*)malloc(sizeof(int)*nf);				// array of color flags for all polygon faces

	// vertex section 
	for (i = 0; i < nv; ++i)
	{
		bp = fgets(buffer, 256, fp);
		gua_parse(buffer, &n, word);
		if (n < 3) // error
		{
			continue;
		}
		else if ( n >= 3 )
		{
			if (!decimal)
			{
				int	nDigits;
				vdb->zero = eldb->zero = set_tolerance(word[0].buffer, &decimal, &nDigits);
				vdb->nDigits = eldb->nDigits = nDigits;
			}
			ov[i].x = atof(word[0].buffer);
			ov[i].y = atof(word[1].buffer);
			ov[i].z = atof(word[2].buffer);
			ov[i].w = 1;
			// add to vertex database for unique ID
		}
	}

	// face section 
	for (i = 0; i < nf; ++i)
	{
		bp = fgets(buffer, 256, fp);
		gua_parse(buffer, &n, word);
		if (!n ) // error
			continue;
		of[i].nVtx = atoi(word[0].buffer);
		if (n < of[i].nVtx + 1)
			return 1; // errror
		of[i].vtx = (int*)malloc(sizeof(int)*of[i].nVtx);
		nVIndx += of[i].nVtx;

		for (j = 1, k=0; j < of[i].nVtx +1; ++j, ++k)
		{
			of[i].vtx[k] = atoi(word[j].buffer);
		}
		of[i].id = i;
		ocf[i] = 0;
		if (n >= of[i].nVtx + 1 + 3) // color 
		{
			ocf[i] = 1;
			of[i].color.r = atoi(word[5].buffer) / 255.0;
			of[i].color.g = atoi(word[6].buffer) / 255.0;
			of[i].color.b = atoi(word[7].buffer) / 255.0;
		}
	}

	gua->vIndex = ofvi = (int*)malloc ( sizeof(int)*nVIndx); // array for all vertex indices across faces 

	// process all faces 
	for (i = 0, vindx = ofvi; i < nf; ++i)
	{
		modulo = of[i].nVtx;

		// copy temp index storage and free up
		memcpy(vindx, of[i].vtx, sizeof(int)*of[i].nVtx); // copy indices into portion of single array
		free(of[i].vtx); // free up temp memory
		of[i].vtx = vindx; // replace pointer in the face
		vindx += of[i].nVtx; // move pointer

		// make a copy of all the original vertices
		for (j = 0; j < of[i].nVtx; ++j)
			vpt[j] = ov[of[i].vtx[j]];

		if (!ocf[i])
			of[i].color = *(COLOR*)&defaultColor; // use the default color since it wasn't explicitly defined

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			mtx_vector_multiply(of[i].nVtx, &vpt, &vtmp, &ctx->inputTrans.matrix[0]);
			for (j = 0; j < of[i].nVtx; ++j)
				vpt[j] = vtmp[j];
		}

		// replicate as required
		m = ctx->inputTrans.mirrorFlag ? 3 : 1;
		n = ctx->inputTrans.replicateFlag ? 3 : 1;
		for (k = 0; k < m; ++k) // mirror loop
		{
			if (!k)
			{
				for (j = 0; j < of[i].nVtx; ++j)
					vpt_dup[j] = vpt[j];
			}
			else
			{	// mirror the x coord
				for (j = 0; j < of[i].nVtx; ++j)
				{
					vpt[j] = vpt_dup[j]; vpt[j].x *= -1;
				}
			}

			for (j = 0; j < n; ++j) // z rotations
			{
				for (l = 0; l < of[i].nVtx; ++l)
				{
					v[l].p = vpt[l]; // copy coordinate data into node structure
					of[i].vtx[l] = vid[l] = vins((void*)vdb, &v[l]); // insert into tree
					++vdb->nProcessed;
				}

				// for each vertex pair determine the unique edge length id  
				for (l = 0; l < of[i].nVtx; ++l)
				{
					elid[l] = elins((void*)eldb, &v[l + 0], &v[(l + 1) % modulo], &elen[l]);
					++eldb->nProcessed;
				}

				// for each vertex pair (3 of them) determine the unique edge id 
				// add edges by vertex ID's
				{
					int	vuid[2], elnid;
					for (l = 0; l < of[i].nVtx; ++l)
					{
						vuid[0] = vid[l];
						vuid[1] = vid[(l + 1) % modulo];
						elnid = elid[l];
						++edb->nProcessed;
						eid[l] = eins((void*)edb, vuid, elnid); // save the vertex pair along with the edge length id 
					}
				}

//				++tedb->nProcessed;
//				tid = teins((void*)tedb, elid); // use array of edge length IDs 
//				++tdb->nProcessed;
//				ttid = tins((void*)tdb, vid, tid, rgb); // use array of unique vertex IDs 

				if (ctx->inputTrans.replicateFlag && j < 2)
				{	// perform rotation transformation on coordinate data
					mtx_vector_multiply(of[i].nVtx, &vpt, &vtmp, &ctx->inputTrans.matrix[0]);
					for (j = 0; j < of[i].nVtx; ++j)
						vpt[j] = vtmp[j];

				}
			}
		}
	}

	// remove temp memory
	free(ov); 
	free(ocf);
//	free(of);
//	free(ofvi);

	return 0; // success
}