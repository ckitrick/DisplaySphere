/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle the processing of geometric data files.
	Performs analysis to determine unique attributes of geometric data.
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
#include <avl_new.h>
#include <time.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"

//#define MEM_DEBUG

#ifdef MEM_DEBUG
	#define MALLOC	dbg_malloc
	#define FREE	dbg_free
typedef struct {
	int		freeFlag[10000];
	void	*address[10000];
	int		bytes[10000];
	int		count;
	unsigned int	allocated, freed;
} MY_MEM_DEBUG;

MY_MEM_DEBUG m_debug;

static void *dbg_malloc(size_t nBytes)
{
	void	*ptr = malloc(nBytes);
	m_debug.address[m_debug.count] = ptr;
	m_debug.bytes[m_debug.count] = nBytes;
	m_debug.allocated += nBytes;
	if (m_debug.count == 26)
	{
		int	i;
		i = 12;
	}
	++m_debug.count;

	return ptr;
}
static void dbg_free(void *ptr)
{
	int		i;
	for (i = 0; i < m_debug.count; ++i)
	{
		if (m_debug.address[i] == ptr)
		{
			if (m_debug.freeFlag[i])
			{
				int		i;
				i = 12;
				// error;
			}
			++m_debug.freeFlag[i];
			m_debug.freed += m_debug.bytes[i];
			free(ptr);
		}
	}
}
#else
	#define MALLOC	malloc
	#define FREE	free
#endif 

/*	
	GUA Library functions - Determining geometry uniqueness from geometric file data 
	
	NEW VERSION - Uses AVL trees to sort during data entry - much faster
	
	gua_tdata_init	  ( mnc ) // max number of components per block
	gua_tedata_init	  ( mnc ) 
	gua_eldata_init	  ( mnc )
	gua_edata_init	  ( mnc )
	gua_vdata_init	  ( mnc )
	gua_blk_init	  ( mnc, data_init ) 
	gua_db_init		  ( init, insert, mnc, zero ) // mnc - max number of components per block
	gua_vtx_dist	  ( a, b ) 
	gua_edata_insert  ( db, vid, elid )
	gua_tdata_insert  (GUA_DB *db, int *vid, int nv, int teid, float *rgb)
	gua_tedata_insert ( db, eid, ne )
	gua_eldata_insert ( db, va, vb, len )
	gua_vdb_dump      ( db ) 
	gua_vdata_insert  ( db, vtx ) 
	gua_parse         ( buffer, n, word )
	gua_db_traverse   ( db, ufunc, uctx ) 
	gua_spc_read      ( gua, spc_filename )
*/

#define GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK	200 

// GUA data specific templates
typedef struct {
	char	buffer[64];
} GUA_WORD;

typedef struct {
	GUT_POINT		p;		
	int				id;		// global unique ID - need to save since place in AVL tree will not be in order
} GUA_VDATA;  // vertex data

typedef struct {
	GUT_POINT		p;
	int				id;		// global unique ID - need to save since place in AVL tree will not be in order
} GUA_NDATA;  // normal data

typedef struct {
	double				len;	//
	int					id;		// global unique ID 
} GUA_ELDATA; // EL Edge Length data

typedef struct {
	int					vid[2];	// 2 vertex ids
	int					elid;	// unique edge length id
	int					id;		// global ID 
} GUA_EDATA; 	// E Edge data

typedef struct {
	int					*elid;		// array of edge length IDs - in order
	int					*relid;		// array of edge length IDs - in reverse order
	int					id;			// global ID 
	int					ne;			// number of edges
} GUA_TEDATA; 	// TE Triangle Edge data

typedef struct {
	int					*vid;		// array of uniqiue vertex IDs (sorted)
	int					*rvid;		// array of reverse ordered uniqiue vertex IDs (sorted)
	int					*ovid;		// array of unique vertex IDs in original order
	int					*onid;		// array of unique vertex IDs in original order
	int					teid;		// unique triangle ID 
	float				rgba[4];	// explict color 
	int					id;			// global ID 
	int					nv;			// number of vertices
	int					normalFlag;	// normals
} GUA_TDATA;	// T Triangle data

// generic block of data 
// each block hold some predetermined size of data specific information 
typedef struct {
	void			*next;	// next BLK
	int				nc; 	// number of components used in this block
	void			*data;	// block specific array data[mnc]
} GUA_BLK;			// Generic block  

// generic block of integer data 
typedef struct {
	void			*next;	// next BLK
	int				nc; 	// number of components used in this block
	int				*data;	// block specific array data[mnc]
} GUA_INTEGER_BLK;			// Generic block  

typedef void *(*GUA_DATA_INIT)(int);

// generic database definition used for each geometric component (vertices, edges, faces, etc.)
typedef struct {
	void			*p;			// pointer to duplicate node during AVL insert
	void			*head;		// first BLK in database
	void			*ihead;		// first BLK in integer blocks
	void			*avl;		// avl tree for data
	double			zero;		// tolerance (may or may not be used)
	int				mnc;		// max number of components per block
	int				nc;			// total number of components in all blocks
	int				nci;		// total number of integer components in all blocks
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

typedef int(*GUA_EDATA_INS)(void *dbp, int *vid, int elid);
typedef int(*GUA_TDATA_INS)(void *dbp, int *vid, int nv, int teid, float *rgb);

typedef int(*GUA_TEDATA_INS)(void *dbp, int *eid, int ne);
typedef int(*GUA_ELDATA_INS)(void *dbp, GUA_VDATA *va, GUA_VDATA *vb, double *len);
typedef int(*GUA_VDATA_INS)(void *dbp, GUA_VDATA *vtx);
typedef int(*GUA_COMPARE)(GUA_DB *db, void *a, void *b);

static void *gua_tdata_init  (int mnc);
static void *gua_tedata_init (int mnc);
static void *gua_eldata_init (int mnc);
static void *gua_edata_init  (int mnc);
static void *gua_vdata_init  (int mnc);
static void *gua_blk_init    (GUA_DB *db); 
static void *gua_db_init     (GUA_DATA_INIT init, void *insert, int mnc, GUA_COMPARE); // mnc - max number of components per block
static double gua_vtx_dist(GUT_POINT *a, GUT_POINT *b);
static int gua_edata_insert(GUA_DB *db, int *vid, int elid);

static int gua_tdata_insert(void *dbp, int *vid, int nv, int tid, float *rgb);
static int gua_tedata_insert(void *dbp, int *eid, int ne );
static int gua_eldata_insert(GUA_DB *db, GUA_VDATA *va, GUA_VDATA *vb, double *len);
static int gua_vdb_dump(void *dbp);
static int gua_parse(char * buffer, int *n, GUA_WORD *word);
static int gua_vdata_insert(GUA_DB *db, GUT_POINT *vtx);

#include "ds_gua.h"
static void gua_destroy_db(GUA_DB *db);
static void gua_destroy(GUA_UNIQUE *gua);
static int gua_integer_blk_init(GUA_DB *db);
static int *gua_integer_array(GUA_DB *db, int length);

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
	fprintf(pt->fp, "e  %6d %6d %6d %6d\n", e->id, e->vid[0], e->vid[1], e->elid ); 
	return 0;
}

//--------------------------------------------------------------------------------
static int te_out(void *passThru, GUA_TEDATA *te)
//--------------------------------------------------------------------------------
{
	int				i;
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "te %6d ", te->id );
	for (i = 0; i < te->ne; ++i)
	{
		fprintf(pt->fp, "%6d ", te->elid[i] );
	}
	fprintf(pt->fp, "\n"); // , te->id, te->elid[0], te->elid[1], te->elid[2]);
	return 0;
}

//--------------------------------------------------------------------------------
static int t_out(void *passThru, GUA_TDATA *t)
//--------------------------------------------------------------------------------
{
	int				i;
	GUA_PASS_THRU	*pt = (GUA_PASS_THRU*)passThru;
	fprintf(pt->fp, "t  %6d %6d ", t->id, t->teid );
	for (i = 0; i < t->nv; ++i)
	{
		fprintf(pt->fp, "%6d ", t->vid[i] );
	}
	fprintf(pt->fp, "\n" );
	return 0;
}

//--------------------------------------------------------------------------------
static int xyz_compare(GUA_DB *db, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	GUA_VDATA *a = (GUA_VDATA*)av, *b = (GUA_VDATA*)bv;
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
static int el_compare(GUA_DB *db, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	GUA_ELDATA *a = (GUA_ELDATA*)av;
	GUA_ELDATA *b = (GUA_ELDATA*)bv;
	double	diff;
	db->nCmp++;
	diff = a->len - b->len;
	if (fabs(diff) > db->zero) // unique
		return diff > 0.0 ? 1 : -1; // done - point will inserted into tree

	db->p = (void*)a; // save the matching node
	return 0;
}

//--------------------------------------------------------------------------------
static int e_compare(GUA_DB *db, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	GUA_EDATA *a = (GUA_EDATA*)av;
	GUA_EDATA *b = (GUA_EDATA*)bv;
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
static int te_compare(GUA_DB *db, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	GUA_TEDATA *a = (GUA_TEDATA*)av;
	GUA_TEDATA *b = (GUA_TEDATA*)bv;
	int			i;
	int			diff, rdiff;

	db->nCmp++;

	if (a->ne > b->ne)
		return 1;
	else if (a->ne < b->ne)
		return -1;

	for (i = 0; i < a->ne; ++i)
	{
		diff = a->elid[i] - b->elid[i];
		if (diff)
			break;
	}

	for (i = 0; i < a->ne; ++i)
	{
		rdiff = a->elid[i] - b->relid[i];
		if (rdiff)
			break; 
	}

	if (!diff || !rdiff) // check if either is a match
	{
		db->p = (void*)a; // save the matching node
		return 0;
	}
	return diff > 0 ? 1 : -1; // done - point will inserted into tree based on diff

	return 0;
}

//--------------------------------------------------------------------------------
static int t_compare(GUA_DB *db, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	GUA_TDATA *a = (GUA_TDATA*)av;
	GUA_TDATA *b = (GUA_TDATA*)bv;
	int			i;
	int			diff, rdiff, ndiff=0;

	db->nCmp++;

	if (a->nv > b->nv)
		return 1;
	else if (a->nv < b->nv)
		return -1;

	for (i = 0; i < a->nv; ++i)
	{
		diff = a->vid[i] - b->vid[i];
		if (diff)
			break;
	}

	for (i = 0; i < a->nv; ++i)
	{
		rdiff = a->vid[i] - b->rvid[i];
		if (rdiff)
			break; 
	}

	if (a->normalFlag)
	{
		for (i = 0; i < a->nv; ++i)
		{
			ndiff = a->onid[i] - b->onid[i];
			if (ndiff)
				break;
		}
	}

	if (( !diff || !rdiff ) && !ndiff) // check if either is a match
	{
		db->p = (void*)a; // save the matching node
		return 0;
	}
	if ( ndiff )
		return ndiff > 0 ? 1 : -1; // done - point will inserted into tree based on diff
	else
		return diff > 0 ? 1 : -1; // done - point will inserted into tree based on diff
}

//--------------------------------------------------------------------------------
static int gua_integer_blk_init(GUA_DB *db)
//--------------------------------------------------------------------------------
{
	//	CREATE a BLK and initialize with new array of integers
	GUA_INTEGER_BLK	*blk;
	blk = (GUA_INTEGER_BLK*)MALLOC(sizeof(GUA_INTEGER_BLK));
	if (!blk)
	{
		int		i;
		i = 12;
	}
	blk->nc = 0; // initial
	blk->next = db->ihead; // linked list pointer
	blk->data = (int*)MALLOC(sizeof(int)*db->mnc); // data specific init 
	db->ihead = (void*)blk;
//	++db->niblks;
	return blk->data ? 0 : 1; // 0 = success
}

//--------------------------------------------------------------------------------
static int *gua_integer_array(GUA_DB *db, int length)
//--------------------------------------------------------------------------------
{
	// return an integer array of length long 
	// need to get storage for vtx 
	GUA_INTEGER_BLK	*blk;
	int				*iptr=0;

	blk = (GUA_INTEGER_BLK*)db->ihead;

	if ((blk->nc + length) > db->mnc ) // check if out if enough room
	{
		if (!gua_integer_blk_init(db))
			return gua_integer_array(db, length);
		else
			return 0; // failed
	}

	// set pointer to new integer array
	
	iptr = &blk->data[blk->nc];
	blk->nc += length; // move the count forward by length
	db->nci += length; // global counter for entire database

	return iptr; // return available array
}

//--------------------------------------------------------------------------------
static int gua_vdata_insert(GUA_DB *db, GUT_POINT *vtx)
//--------------------------------------------------------------------------------
{
//	double		ydiff, zdiff;
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
static int gua_eldata_insert(GUA_DB *db, GUA_VDATA *va, GUA_VDATA *vb, double *len)
//--------------------------------------------------------------------------------
{
	GUA_BLK		*blk;
	GUA_ELDATA	*e;

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	e = ((GUA_ELDATA*)blk->data) + blk->nc; // db specific data

//	*len = gua_vtx_dist(va, vb); // length of edge  
	*len = gua_vtx_dist(&va->p, &vb->p); // length of edge  
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
static void gua_integer_array_order(int *array, int n)  // put lowest id# first
//--------------------------------------------------------------------------------
{
	static int	copy[32];
	int			index = 0;
	int			low = 1000000;
	int			i, j;

	for (i = 0; i < n; ++i) // find low ID in array
	{
		if (array[i] < low)
		{
			index = i;
			low = array[i];
		}
	}

	for (i = index+1; i < n; ++i) // find last low in multiple sequence of same value
	{
		if (array[i] == low)
			index = i;
		else
			break;
	}

	if (index) // only reorder if index is not zero
	{

		// copy low section 
		for (i = 0; i < index; ++i)
			copy[i] = array[i];

		// move top section to bottom
		for (i = index, j = 0; i < n; ++i, ++j)
			array[j] = array[i];

		// move copy of low section to top
		for (i = 0, j = n - index; i < index; ++i, ++j)
			array[j] = copy[i];
	}
}

//--------------------------------------------------------------------------------
static void gua_integer_array_order_reverse(int *array, int *rev_array, int n)
//--------------------------------------------------------------------------------
{
	// copy ordered array into reverse array
	// reverse rest of array beyond the first entry
	int		i, s, e, tmp;

	for (i = 0; i < n; ++i) // copy original array
		rev_array[i] = array[i];

	s = 1;
	e = n - 1;
	while (s < e)
	{
		tmp = rev_array[s];
		rev_array[s] = rev_array[e];
		rev_array[e] = tmp;
		++s;
		--e;
	}
}

//--------------------------------------------------------------------------------
static int gua_tedata_insert(GUA_DB *db, int *eid, int ne)
//--------------------------------------------------------------------------------
{
	GUA_BLK				*blk;
	GUA_TEDATA			*te;
//	long long			seq;
	int					i; // , tmp;

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	te = ((GUA_TEDATA*)blk->data) + blk->nc; // db specific data

	te->elid = gua_integer_array(db, ne); // get an array to hold info
	te->relid = gua_integer_array(db, ne); // get an array to hold info

	te->ne = ne;
	for (i = 0; i < ne; ++i) // save original vertex id's
		te->elid[i] = eid[i];

	gua_integer_array_order(te->elid, ne); // put lowest id# first
	gua_integer_array_order_reverse(te->elid, te->relid, ne); // put lowest id# first and revers

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
static int gua_tdata_insert(GUA_DB *db, int *vid, int *nid, int nv, int normalFlag, int teid, float *rgba)
//--------------------------------------------------------------------------------
{
	GUA_BLK				*blk;
	GUA_TDATA			*t;
	int					i; 

	// need to get storage for edge 
	blk = (GUA_BLK*)db->head;

	if (blk->nc == db->mnc) // check if out of room
		blk = (GUA_BLK*)gua_blk_init(db);

	t = ((GUA_TDATA*)blk->data) + blk->nc; // db specific data

	t->vid = gua_integer_array(db, nv);  // get an array to hold vertex index info (sorted)
	t->rvid = gua_integer_array(db, nv); // get an array to hold vertex index info (sorted - reverse)
	t->ovid = gua_integer_array(db, nv); // get an array to hold vertex index info (original order)
	t->onid = gua_integer_array(db, nv); // get an array to hold normal index info (original order)

	t->nv = nv;
	t->normalFlag = normalFlag;
	for (i = 0; i < nv; ++i) // save original vertex id's
	{
		t->ovid[i] = t->vid[i] = vid[i];
		if ( normalFlag )
		t->onid[i] = nid[i]; 
	}
	gua_integer_array_order(t->vid, nv); // put lowest id# first
	gua_integer_array_order_reverse(t->vid, t->rvid, nv); // put lowest id# first and revers
										 
	t->teid = teid; // save the unique triangle ID by edge set
	t->id = db->nc; // assign an unique ID 
	t->rgba[0] = rgba[0];
	t->rgba[1] = rgba[1];
	t->rgba[2] = rgba[2];
	t->rgba[3] = rgba[3];

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
	p = (void*)MALLOC(sizeof(GUA_TDATA) * mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_tedata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of triangles 
	void *p;
	p = (void*)MALLOC(sizeof(GUA_TEDATA) * mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_eldata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of edge lengths 
	void *p;
	p = (void*)MALLOC(sizeof(GUA_ELDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_edata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of edges 
	void *p;
	p = (void*)MALLOC(sizeof(GUA_EDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_vdata_init(int mnc)
//--------------------------------------------------------------------------------
{
	//	CREATE data array of vertices 
	void *p;
	p = (void*)MALLOC(sizeof(GUA_VDATA)*mnc);
	return p;
}

//--------------------------------------------------------------------------------
static void *gua_blk_init(GUA_DB *db) //int mnc, GUA_DATA_INIT data_init)
//--------------------------------------------------------------------------------
{
	//	CREATE a BLK and initialize with data specific init function
	GUA_BLK	*blk;
	blk			= (GUA_BLK*)MALLOC(sizeof(GUA_BLK));
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
	db = (GUA_DB*)MALLOC(sizeof(GUA_DB));
	db->p			= 0;						// pointer used during AVL insert
	db->nc			= 0;						// total # of unique components in all blocks
	db->nci			= 0;						// total # of integer components in all blocks
	db->mnc			= mnc; 						// max number of components per block
//	db->zero		= zero;		 				// generic tolerance 
	db->insert		= insert;					// insert function
	db->init		= init;						// block init function 
	db->nDup		= 0;
	db->nCmp		= 0;
	db->nProcessed	= 0;
	db->avl			= avl_create(compare, (void*)db );
	db->head		= 0;
	db->ihead		= 0;
	gua_blk_init(db);							// db specific init 
	gua_integer_blk_init(db);
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
	int		i = 0, j=0, in=0;
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
	GUA_BLK			*blk, *nextBlk;
	GUA_INTEGER_BLK	*iblk, *iNextBlk;
	GUA_VDATA		*data;

	blk = db->head;
	while (blk)
	{
		data = (GUA_VDATA*)blk->data;
		nextBlk = blk->next;	// save pointer to next block
		FREE(blk->data);		// FREE all data in the block
		FREE(blk);				// FREE the block
		blk = nextBlk;			// copy pointer back to continue
	}

	// remove the integer arrays
	iblk = db->ihead;
	while (iblk)
	{
		iNextBlk = iblk->next;	// save pointer to next block
		FREE(iblk->data);		// FREE all data in the block
		FREE(iblk);				// FREE the block
		iblk = iNextBlk;			// copy pointer back to continue
	}

	avl_destroy(db->avl,0);
	FREE(db);
}

//--------------------------------------------------------------------------------
static void gua_destroy_db(GUA_DB *db)
//--------------------------------------------------------------------------------
{
	GUA_BLK *blk, *nextBlk;
	GUA_INTEGER_BLK	*iblk, *iNextBlk;

	blk = db->head;
	while (blk)
	{
		nextBlk = blk->next;	// save pointer to next block
		FREE(blk->data);		// FREE all data in the block
		FREE(blk);				// FREE the block
		blk = nextBlk;			// copy pointer back to continue
	}

	// remove the integer arrays
	iblk = db->ihead;
	while (iblk)
	{
		iNextBlk = iblk->next;	// save pointer to next block
		FREE(iblk->data);		// FREE all data in the block
		FREE(iblk);				// FREE the block
		iblk = iNextBlk;			// copy pointer back to continue
	}

	avl_destroy(db->avl,0);
	FREE(db);
}

//--------------------------------------------------------------------------------
static void gua_destroy(GUA_UNIQUE *gua)
//--------------------------------------------------------------------------------
{
	//	FREE up all memory associated with object

	gua_destroy_db((GUA_DB*)gua->edb); // needs special avl_destroy
	gua_destroy_db((GUA_DB*)gua->eldb);
	gua_destroy_db((GUA_DB*)gua->tedb);
	gua_destroy_db((GUA_DB*)gua->tdb);
	gua_destroy_vdb((GUA_DB*)gua->vdb);

	FREE(gua); // final FREE
}

//--------------------------------------------------------------------------------
static int center_and_scale(DS_CTX *ctx, GUT_POINT *vertex, int nv)
//--------------------------------------------------------------------------------
{
	GUT_POINT	*p;
	int			i;
	struct {
		double	x_min, x_max, y_min, y_max, z_min, z_max, x_scale, y_scale, z_scale, scale, ox, oy, oz;
	} dim = { 10000000,-10000000, 10000000,-10000000,10000000,-10000000,1,1,1,100000000,0,0,0 };

	// find extent of coordinates
	for (i = 0, p = vertex + 0; i<nv; ++i, ++p)
	{
		// add to vertex database for unique ID
		if (p->x < dim.x_min) dim.x_min = p->x;
		if (p->x > dim.x_max) dim.x_max = p->x;
		if (p->y < dim.y_min) dim.y_min = p->y;
		if (p->y > dim.y_max) dim.y_max = p->y;
		if (p->z < dim.z_min) dim.z_min = p->z;
		if (p->z > dim.z_max) dim.z_max = p->z;
	}

	dim.ox = (dim.x_max + dim.x_min) / 2.0;
	dim.oy = (dim.y_max + dim.y_min) / 2.0;
	dim.oz = (dim.z_max + dim.z_min) / 2.0;
	dim.x_scale = 2.0 / (dim.x_max - dim.x_min);
	dim.y_scale = 2.0 / (dim.y_max - dim.y_min);
	dim.z_scale = 2.0 / (dim.z_max - dim.z_min);
	if (dim.x_scale < dim.scale) dim.scale = dim.x_scale;
	if (dim.y_scale < dim.scale) dim.scale = dim.y_scale;
	if (dim.z_scale < dim.scale) dim.scale = dim.z_scale;

	// update vertices 
	for (i = 0, p = vertex + 0; i < nv; ++i, ++p)
	{
		p->x -= dim.ox;
		p->y -= dim.oy;
		p->z -= dim.oz;
		p->x *= dim.scale;
		p->y *= dim.scale;
		p->z *= dim.scale;
	}

	ctx->eAttr.maxLength *= dim.scale;
	ctx->eAttr.minLength *= dim.scale;

	return 0;
}

//--------------------------------------------------------------------------------
int gua_convert_to_object(DS_CTX *ctx, void *guap, DS_GEO_OBJECT *geo)
//--------------------------------------------------------------------------------
{
	//	When finished FREE up all the memory associated with the GUA 
	//
	GUA_UNIQUE	*gua = (GUA_UNIQUE*)guap;
	GUA_DB		*vdb;  // vertex DB      - unqiue coordinates 
	GUA_DB		*ndb;  // normal DB      - unqiue coordinates 
	GUA_DB		*edb;  // edge DB        - unique vertex pairs 
	GUA_DB		*eldb; // edge length DB - unique edge lengths  
	GUA_DB		*tedb; // triangle DB    - unique edge length IDs 
	GUA_DB		*tdb;  // triangle DB    - unqiue vertex ID sets 
	GUA_BLK		*blk;
	int			i, j, n, m;
	int			nBlk;
//	GUA_VDATA	*vdata;
	int			*iptr; // used for vertices
	int			*inptr; // used for normals

	// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	ndb  = (GUA_DB*)gua->ndb;
	eldb = (GUA_DB*)gua->eldb;
	edb  = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb  = (GUA_DB*)gua->tdb;

	geo->nmlFlag = ndb ? 1 : 0;

	// convert unique geo data to geo object
	if (strlen(gua->name)) // copy name
		strcpy(geo->name, gua->name);

	// vertex array 
	// allocate array
	geo->vtx  = (GUT_POINT*)malloc(sizeof(GUT_POINT)*vdb->nc);
	if (ndb)
		geo->nml = (GUT_POINT*)malloc(sizeof(GUT_POINT)*ndb->nc);
	else
		geo->nml = 0;
	geo->nVtx = vdb->nc;

	// transfer vertex coordinate data 
	blk = (GUA_BLK*)vdb->head;
	n = 0;
	while (blk)
	{
		GUA_VDATA *data = (GUA_VDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			geo->vtx[data[i].id] = *(GUT_POINT*)&data[i].p;
			++n;
		}

		blk = (GUA_BLK*)blk->next;
	}

	if (ndb)
	{
		// transfer normal coordinate data 
		blk = (GUA_BLK*)ndb->head;
		n = 0;
		while (blk)
		{
			GUA_VDATA *data = (GUA_VDATA*)blk->data;
			for (i = 0; i < blk->nc; ++i) // go thru all components of this block
			{
				geo->nml[data[i].id] = *(GUT_POINT*)&data[i].p;
				++n;
			}

			blk = (GUA_BLK*)blk->next;
		}
	}

	// tri array 
	// allocate array
	geo->tri = (DS_FACE*)malloc(sizeof(DS_FACE)*tdb->nc);
	geo->nTri = tdb->nc;
	geo->vIndex = iptr = (int*)malloc(sizeof(int)*(tdb->nci / 3)); // array to hold indices
	geo->nIndex = inptr = (int*)malloc(sizeof(int)*(tdb->nci / 3)); // array to hold indices

	// transfer data for all triangles
	blk = (GUA_BLK*)tdb->head;
	n = 0;
	m = 0;
	nBlk = 0;
	while (blk)
	{
		++nBlk;
		GUA_TDATA *data = (GUA_TDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			geo->tri[n].color = *(DS_COLOR*)&data[i].rgba;	// explict color - which can be the default color
			geo->tri[n].id = data[i].teid;			// unique ID 
			geo->tri[n].vtx = iptr;					// attach array memory
			geo->tri[n].nml = inptr;				// attach array memory
			geo->tri[n].nVtx = data[i].nv;
			geo->tri[n].draw.nSegments = 0;
			m += data[i].nv;
			for (j = 0; j < data[i].nv; ++j)
			{
				geo->tri[n].vtx[j] = data[i].ovid[j];	// unique vertex IDs
				geo->tri[n].nml[j] = data[i].onid[j];	// unique normal IDs
			}
			//
			// NEED TO GENERATE NORMALS
			iptr += data[i].nv; // move pointer forward
			inptr += data[i].nv; // move pointer forward
			++n;
		}

		blk = (GUA_BLK*)blk->next;
	}

	// EDGES ARE SAME FOR SPC & OFF 
	// edge array 
	// allocate array
	geo->edge  = (DS_EDGE*)malloc(sizeof(DS_EDGE)*edb->nc);
	geo->nEdge = edb->nc;
	geo->edge[0].draw.n = 0;

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
//	ctx->eAttr.maxLength = 0;
//	ctx->eAttr.minLength = 100000000;
	geo->eAttr.maxLength = ctx->eAttr.maxLength;
	geo->eAttr.minLength = ctx->eAttr.minLength;
	while (blk)
	{
//		GUA_ELDATA *data = (GUA_EDATA*)blk->data;
		GUA_ELDATA *data = (GUA_ELDATA*)blk->data;
		for (i = 0; i < blk->nc; ++i) // go thru all components of this block
		{
			if (data[i].len > ctx->eAttr.maxLength) ctx->eAttr.maxLength = data[i].len;
			if (data[i].len < ctx->eAttr.minLength) ctx->eAttr.minLength = data[i].len;
			if (data[i].len > geo->eAttr.maxLength) geo->eAttr.maxLength = data[i].len;
			if (data[i].len < geo->eAttr.minLength) geo->eAttr.minLength = data[i].len;
			++n;
		}

		blk = (GUA_BLK*)blk->next;
	}


	geo->nUTri = tedb->nc; // number of geometrically different triangles (same set of edge lengths)
	geo->nUEdge = eldb->nc; // number of geometrically different edges (by length)

	geo->ctE =ds_ctbl_get_color_table(&ctx->cts, eldb->nc);
	geo->ctT =ds_ctbl_get_color_table(&ctx->cts, tedb->nc);
	geo->ctB =ds_ctbl_get_color_table(&ctx->cts, tedb->nc + edb->nc);

	gua_destroy(gua); // total memory deallocation

	if (ctx->inputTrans.centerAndScaleFlag)
		center_and_scale(ctx, geo->vtx, geo->nVtx);

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
int gua_spc_read(DS_CTX *ctx, void **guap, FILE *fp, float *defaultColor)
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
	GUA_DB			*ndb;			// vertex DB      - unqiue coordinates 
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
	float			rgb[4];			// color of triangle
	int				explicitColor;	// color flag 
	int				maxVID  = 0;
	int				nDigits = 0;
	int				decimal = 0;
	GUT_POINT		vpt[3];
	GUT_POINT		vpt_dup[3];
	GUT_POINT		vtmp[3];		// vertex coordinate data for current triangle 
	int				colorFormat = 0;

	if (!fp) return 1; // error 

	// create a GUA structure
	gua = (GUA_UNIQUE*)MALLOC(sizeof(GUA_UNIQUE)); // guap;
	if (gua == NULL) return 1; // error
	*guap = (void*)gua; // pass pointer back 

	// create all the attribute databases
	gua->vdb  = gua_db_init ( gua_vdata_init,  gua_vdata_insert,  mnc, xyz_compare);	// vertex DB
	gua->ndb  = 0;																		// normal DB
	gua->eldb = gua_db_init ( gua_eldata_init, gua_eldata_insert, mnc,  el_compare);	// edge by length DB
	gua->edb  = gua_db_init ( gua_edata_init,  gua_edata_insert,  mnc,   e_compare);	// edge by unique vertex pairs DB
	gua->tedb = gua_db_init ( gua_tedata_init, gua_tedata_insert, mnc,  te_compare);	// triangle by unique edges DB
	gua->tdb  = gua_db_init ( gua_tdata_init,  gua_tdata_insert,  mnc,   t_compare);		// triangle by unique vertices DB
	gua->name[0] = 0;

	// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	ndb  = (GUA_DB*)gua->ndb;
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

	if (ctx->inputTrans.transformFlag) // build transformation matrix
	{
		ds_geo_build_transform_matrix(ctx);
	}

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
			if (!colorFormat)
			{
				if (strchr(word[9].buffer, '.') || strchr(word[10].buffer, '.') || strchr(word[11].buffer, '.'))
					colorFormat = COLOR_FORMAT_ZERO_TO_ONE;
				else
					colorFormat = COLOR_FORMAT_255;
			}
			rgb[3] = defaultColor[3];
			switch (colorFormat) {
			case COLOR_FORMAT_ZERO_TO_ONE:
				rgb[0] = (float)atof(word[9].buffer);
				rgb[1] = (float)atof(word[10].buffer);
				rgb[2] = (float)atof(word[11].buffer);
				rgb[3] = (float)((DS_COLOR*)defaultColor)->a;
				if (n>=13)
					rgb[3] = (float)atof(word[12].buffer);
				break;
			case COLOR_FORMAT_255:
				rgb[0] = (float)(atof(word[ 9].buffer) / 255.0);
				rgb[1] = (float)(atof(word[10].buffer) / 255.0);
				rgb[2] = (float)(atof(word[11].buffer) / 255.0);
				rgb[3] = (float)((DS_COLOR*)defaultColor)->a;
				if (n >= 13)
					rgb[3] = (float)(atof(word[12].buffer) / 255.0);
				break;
			}
		}
		else
		{
			rgb[0] = defaultColor[0];
			rgb[1] = defaultColor[1];
			rgb[2] = defaultColor[2];
			rgb[3] = defaultColor[3];
		}

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			// NEW SECTION
			// IF replication and transformation are defined in the command line then:
			// we will first transform each vertex then replicate in Z 120 degree rotation.
			// This will be all done prior to insertion. At insertion duplicates that
			// may result will be culled out
			mtx_vector_multiply(3, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[0]);
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
				tid = teins((void*)tedb, elid,3); // use array of edge length IDs 
				++tdb->nProcessed;
				ttid = tins((void*)tdb, vid, 0, 3, 0, tid, rgb); // use array of unique vertex IDs 

				if (ctx->inputTrans.replicateFlag && j < 2)
				{	// perform rotation transformation on coordinate data
					mtx_vector_multiply(3, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[1]); // 120 degree rotation of z axis
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
void gua_dump(DS_CTX *ctx, void *guap, char *inputFilename, char *outputFilename )
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
	{
		char buffer[128];
		sprintf(buffer, "Unique geometry dump file <%s> failed to open.", outputFilename);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
		return; // failed to open
	}
	passThru.m = vdb->nDigits;
	passThru.n = vdb->nDigits + 4;

	fprintf(fp, "%s - Unique Geometry Processing\n", ctx->version.text);
	fprintf(fp, "Version - %d.%d\n", ctx->version.major, ctx->version.minor );

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
	fprintf(fp, "Unique Edges by Vertex Pairs - ordered ( e ID v1 v2 elID )\n");
	avl_traverse_rtl(edb->avl,  (void*)&passThru,  e_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Triangles by Unique Edge Lengths - ordered ( te ID el1 el2 el3 ... )\n");
	avl_traverse_rtl(tedb->avl, (void*)&passThru, te_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned
	fprintf(fp, "Unique Triangles by Unique Vertices - ordered ( t ID teID v1 v2 v3 ... )\n");
	avl_traverse_rtl(tdb->avl,  (void*)&passThru,  t_out); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned

	fclose(fp);
}

typedef struct {
	int			nVtx;
	int			vtx[3];
	GUT_POINT	vcood[3];
	DS_COLOR	color;
}NGUA_FACE;

typedef struct {
	DS_VTX_CLR		*block;
	void			*next;
	int				nc;
	void			*data;
}NGUA_BLK;

typedef struct {
	void		*head;
	void		*ihead;
	void		*tail;
	int			mnc;
	int			nc;
	int			nci;
	double		max_length;
	double		min_length;
}NGUA_DB;

//--------------------------------------------------------------------------------
int ngua_spc_read(DS_CTX *ctx, void **nguap, FILE *fp, float *defaultColor)
//--------------------------------------------------------------------------------
{
	NGUA_BLK	*blk;
	NGUA_DB		*db;
//	NGUA_FACE	*face;
	GUA_WORD	word[32];
	int			n, m;
	int			i, j, k; 
	char		buffer[256];
	DS_VTX_CLR		*vc;
	DS_VTX_CLR		vct;
	GUT_POINT		vpt[3], vpt_dup[3];
	GUT_POINT		vtmp[3];
	double		length;
	int			colorFormat = 0;

	// initialize database 
	db				= (NGUA_DB*)MALLOC(sizeof(NGUA_DB));
	db->nc			= 0;
	db->mnc			= GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK;
	db->head		= db->tail = 0;
	blk				= (NGUA_BLK*)MALLOC(sizeof(NGUA_BLK));
	blk->nc			= 0;
	blk->next		= 0;
	blk->data = (DS_VTX_CLR*)MALLOC(sizeof(DS_VTX_CLR)*db->mnc);
	db->head		= (void*)blk;
	db->tail		= (void*)blk;
	db->max_length	= 0;
	db->min_length	= 10000000;
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

		// determine edge lengths
		gut_distance_from_point_to_point(&vpt[0], &vpt[1], &length);
		if (length < db->min_length) db->min_length = length;
		if (length > db->max_length) db->max_length = length;
		gut_distance_from_point_to_point(&vpt[1], &vpt[2], &length);
		if (length < db->min_length) db->min_length = length;
		if (length > db->max_length) db->max_length = length;
		gut_distance_from_point_to_point(&vpt[2], &vpt[0], &length);
		if (length < db->min_length) db->min_length = length;
		if (length > db->max_length) db->max_length = length;

		if (n >= 12) // color included
		{
			if (!colorFormat)
			{
				if (strchr(word[9].buffer, '.') || strchr(word[10].buffer, '.') || strchr(word[11].buffer, '.'))
					colorFormat = COLOR_FORMAT_ZERO_TO_ONE;
				else
					colorFormat = COLOR_FORMAT_255;
			}
			switch (colorFormat) {
			case COLOR_FORMAT_ZERO_TO_ONE:
				vct.color.r = (float)atof(word[9].buffer);
				vct.color.g = (float)atof(word[10].buffer);
				vct.color.b = (float)atof(word[11].buffer);
				vct.color.a = (float)((DS_COLOR*)defaultColor)->a;
				if ( n >= 13 )
					vct.color.a = (float)atof(word[12].buffer);
				break;
			case COLOR_FORMAT_255:
				vct.color.r = (float)(atof(word[ 9].buffer) / 255.0);
				vct.color.g = (float)(atof(word[10].buffer) / 255.0);
				vct.color.b = (float)(atof(word[11].buffer) / 255.0);
				vct.color.a = (float)((DS_COLOR*)defaultColor)->a;
				if (n >= 13)
					vct.color.a = (float)(atof(word[12].buffer) / 255.0);
			}
		}
		else
		{
			vct.color.r = defaultColor[0]; // default
			vct.color.g = defaultColor[1]; // default
			vct.color.b = defaultColor[2]; // default
			vct.color.a = defaultColor[3]; // default
		}

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			mtx_vector_multiply(3, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[0]);
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
					blk = (NGUA_BLK*)MALLOC(sizeof(NGUA_BLK));
					blk->nc = 0;
					blk->next = 0;
					blk->data = (DS_VTX_CLR*)MALLOC(sizeof(DS_VTX_CLR)*db->mnc);
					((NGUA_BLK*)db->tail)->next = blk;
					db->tail = (void*)blk;
				}
				vc = (DS_VTX_CLR*)blk->data + blk->nc++;
				++db->nc;

				vc->color = vct.color;

				if (ctx->inputTrans.replicateFlag && j < 2)
				{	// perform rotation transformation on coordinate data
					mtx_vector_multiply(3, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[1]); // 120 degree rotation of z axis
					vpt[0] = vtmp[0];
					vpt[1] = vtmp[1];
					vpt[2] = vtmp[2];
				}

				for (i = 0; i < 3; ++i)
				{
					//					face->vtx[i] = 
					vc->vertex[i] = vpt[i]; // copy coordinate data into node structure
				}
			}
		}
	}

	return 0;
}

//--------------------------------------------------------------------------------
int ngua_convert_to_object(DS_CTX *ctx, void *nguap, DS_GEO_OBJECT *geo)
//--------------------------------------------------------------------------------
{
	//	When finished FREE up all the memory associated with the NGUA 
	//
	NGUA_BLK	*blk, *nblk;
	NGUA_DB		*db;
	int			n;
	int			i, j;
	int			count = 0;
	DS_VTX_CLR		*vc;
	int			*iptr;
	db = (NGUA_DB*)nguap;

	// convert non-unique geo data to geo object

	// vertex array 
	// allocate array
	geo->vtx = (GUT_POINT*)malloc(sizeof(GUT_POINT)*db->nc*3);
	geo->nVtx = db->nc* 3;
	geo->vIndex = iptr = (int*)malloc(sizeof(int)*db->nc * 3);

	// transfer vertex data 
	blk = (NGUA_BLK*)db->head;
	n = 0;
	while (blk)
	{
		vc = (DS_VTX_CLR*)blk->data;
		for (i = 0; i < blk->nc; ++i, ++vc) // go thru all components of this block
		{
			geo->vtx[n++] = *(GUT_POINT*)&vc->vertex[0];
			geo->vtx[n++] = *(GUT_POINT*)&vc->vertex[1];
			geo->vtx[n++] = *(GUT_POINT*)&vc->vertex[2];
		}

		blk = (NGUA_BLK*)blk->next;
	}

	// tri array 
	// allocate array
	geo->tri	= (DS_FACE*)malloc(sizeof(DS_FACE)*db->nc);
	geo->nTri	= db->nc;
	// transfer data 
	blk			= (NGUA_BLK*)db->head;
	n			 = 0;
	j			 = 0;
	while (blk)
	{
		vc = (DS_VTX_CLR*)blk->data;
		for (i = 0; i < blk->nc; ++i, ++vc) // go thru all components of this block
		{
			geo->tri[n].color = *(DS_COLOR*)&vc->color;					// explict color
			geo->tri[n].id = n;											// unique ID 
			geo->tri[n].nVtx = 3;										// triangle
			geo->tri[i].vtx = iptr;										// next available space for indices
			geo->tri[n].vtx[0] = j++;	// v[0];			// unique vertex IDs
			geo->tri[n].vtx[1] = j++;	// v[1];
			geo->tri[n].vtx[2] = j++;	// v[2];
			geo->tri[n].draw.nSegments = 0;
			iptr += 3;
			++n;
		}

		blk = (NGUA_BLK*)blk->next;
	}

	// edge array 
	// allocate array
	geo->edge = 0; // (EDGE*)MALLOC(sizeof(EDGE)*edb->nc);
	geo->nEdge = 0; // edb->nc;

	geo->nUTri = db->nc; // number of geometrically different triangles (same set of edge lengths)
	geo->nUEdge = 0; // number of geometrically different edges (by length)
	// loop thru all edges and find min/max size 
	geo->eAttr.maxLength = ctx->eAttr.maxLength;
	geo->eAttr.minLength = ctx->eAttr.minLength;

	if (db->max_length > ctx->eAttr.maxLength)
		ctx->eAttr.maxLength = db->max_length;
	if (db->min_length < ctx->eAttr.maxLength)
		ctx->eAttr.minLength = db->min_length;
	if (db->max_length > geo->eAttr.maxLength)
		geo->eAttr.maxLength = db->max_length;
	if (db->min_length < geo->eAttr.maxLength)
		geo->eAttr.minLength = db->min_length;

	geo->ctE = 0; //ds_ctbl_get_color_table(&ctx->cts, eldb->nc);
	geo->ctT =ds_ctbl_get_color_table(&ctx->cts, db->nc);
	geo->ctB =ds_ctbl_get_color_table(&ctx->cts, db->nc);

	// total memory deallocation
	blk = (NGUA_BLK*)db->head;
	while (blk)
	{
		nblk = (NGUA_BLK*)blk->next;
		FREE(blk->data);
		FREE(blk);
		blk = nblk;
	}
	FREE(db);

	if (ctx->inputTrans.centerAndScaleFlag)
		center_and_scale(ctx, geo->vtx, geo->nVtx);

	return 0;
}

//--------------------------------------------------------------------------------
int gua_off_read(DS_CTX *ctx, void **guap, FILE *fp, float *defaultColor)
//--------------------------------------------------------------------------------
{
	// Consume an OFF file and determine uniqueness for all components
	//
	// Automatically determines zero tolerance for Vertex & Edges from number of 
	// significant digits in first vertex string
	//
	int				mnc = GUA_MAX_NUMBER_COMPONENTS_PER_BLOCK;
	GUA_UNIQUE		*gua;			// = (GUA_UNIQUE*)guap;
	char			*bp;
	GUA_DB			*vdb;			// vertex DB      - unqiue coordinates 
	GUA_DB			*ndb;			// normal DB      - unqiue normals 
	GUA_DB			*edb;			// edge DB        - unique vertex pairs 
	GUA_DB			*eldb;			// edge length DB - unique edge lengths  
	GUA_DB			*tedb;			// triangle DB    - unique edge length IDs 
	GUA_DB			*tdb;			// triangle DB    - unqiue vertex ID sets 
	int				i, j, k, l;		// counters
	int				vid[32]; 		// unique vertex IDs for current triangle
	int				nid[32]; 		// unique vertex IDs for current triangle
	int				elid[32];		// unique edge length IDs for current triangle 
	int				eid[32];		// unigue edge by unique vertex ID's for current triangle
	int				tid;			// unique triangle by unique edge lengths
	int				ttid;			// unique triangle by unique vertex IDs
	char			buffer[256];
	GUA_WORD		word[32];
	int				n, m; 			// number of words parsed
	GUA_VDATA		v[32];			// vertex coordinate data for current triangle 
	GUA_VDATA		nml[32];		// normal coordinate data for current triangle 
	double			elen[32];		// length of three edges of current triangle
	int				maxVID = 0;
	int				nDigits = 0;
	int				decimal = 0;
	GUT_POINT		vpt[32],     npt[32];
	GUT_POINT		vpt_dup[32], npt_dup[32];
	GUT_POINT		vtmp[32],    ntmp[32];		// vertex coordinate data for current triangle 
	int				nv;
	int				nf;
	int				ne;
	GUT_POINT		*ov;	// original vertices
	GUT_POINT		*on;	// original normals
	DS_POLYGON		*of;
	int				*ocf; // off color flag
	int				modulo;
	int				nVIndx = 0; // sum of all vertex indices for all faces
	int				colorFormat = 0;
	int				fileType = 0;
	char			objectName[64];

	if (!fp) return 1; // error 

	bp = fgets(buffer, 256, fp); // read first line
	gua_parse(buffer, &n, word);
	// First line options 
	// OFF [name]
	// NOFF [name]
	if (!n) fileType = 0; 
	else if (!strcmp(word[0].buffer, "OFF")) fileType = 1;
	else if (!strcmp(word[0].buffer, "NOFF")) fileType = 2;
	else fileType = 0;
	if(!fileType)
	{
		rewind(fp); // can't process
		return 1;	// error
	}
	objectName[0] = 0;
	if (n > 1) // non-standard option to assign a name to the object after the OFF signature
	{
		strcpy(objectName, word[1].buffer);
	}
	
	// create a GUA structure
	gua = (GUA_UNIQUE*)MALLOC(sizeof(GUA_UNIQUE)); // guap;
	if (gua == NULL) return 1; // error
	*guap = (void*)gua; // pass pointer back 
	strcpy(gua->name, objectName); // save

	// create all the attribute databases
	gua->vdb  = gua_db_init(gua_vdata_init,  gua_vdata_insert,  mnc, xyz_compare);	// vertex DB
	if (fileType == 2) // normals are required to present
		gua->ndb = gua_db_init(gua_vdata_init, gua_vdata_insert, mnc, xyz_compare);	// normal DB
	else
		gua->ndb = 0;
	gua->eldb = gua_db_init(gua_eldata_init, gua_eldata_insert, mnc, el_compare);	// edge by length DB
	gua->edb  = gua_db_init(gua_edata_init,  gua_edata_insert,  mnc, e_compare);	// edge by unique vertex pairs DB
	gua->tedb = gua_db_init(gua_tedata_init, gua_tedata_insert, mnc, te_compare);	// triangle by unique edges DB
	gua->tdb  = gua_db_init(gua_tdata_init,  gua_tdata_insert,  mnc, t_compare);	// triangle by unique vertices DB

	// local copies re-casted
	vdb  = (GUA_DB*)gua->vdb;
	if (gua->ndb)
		ndb = (GUA_DB*)gua->ndb;
	else
		ndb = 0;
	eldb = (GUA_DB*)gua->eldb;
	edb  = (GUA_DB*)gua->edb;
	tedb = (GUA_DB*)gua->tedb;
	tdb  = (GUA_DB*)gua->tdb;

	// data specific insert functions from each database with correct cast
	GUA_VDATA_INS  vins = (GUA_VDATA_INS)vdb->insert;
	GUA_VDATA_INS  nins = 0;// = (GUA_VDATA_INS)ndb->insert; // same data type
	if (ndb)
		nins = (GUA_VDATA_INS)ndb->insert; // same data type
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

	ov = (GUT_POINT*)MALLOC(sizeof(GUT_POINT)*nv);		// array storage for all vertices 
	if (fileType == 2)
		on = (GUT_POINT*)MALLOC(sizeof(GUT_POINT)*nv);		// array storage for all normals 
	else
		on = 0;
	of = (DS_POLYGON*)MALLOC(sizeof(DS_POLYGON)*nf);	// array storage for polygon faces 
	ocf = (int*)MALLOC(sizeof(int)*nf);					// array of color flags for all polygon faces

	// vertex section 
	for (i = 0; i < nv; ) 
	{
		bp = fgets(buffer, 256, fp);
		gua_parse(buffer, &n, word);
		if (n && word[0].buffer[0] == '#')
		{
			continue;
		}
		else if (n < 3) // error
		{
			continue;
		}
		else if (fileType == 2 && n < 6) // error
		{
			continue; 
		}
		else if (n >= 3)
		{
			if (!decimal) // need to determine the accuracy level based on the supplied text - JUST DO ONCE
			{
				int	nDigits;
				vdb->zero = eldb->zero = set_tolerance(word[0].buffer, &decimal, &nDigits);
				vdb->nDigits = eldb->nDigits = nDigits;
			}
			// save the vertex xyz data
			ov[i].x = atof(word[0].buffer);
			ov[i].y = atof(word[1].buffer);
			ov[i].z = atof(word[2].buffer);
			ov[i].w = 1;

			if (n > 3 && on) // add normal to array 
			{
				on[i].x = atof(word[3].buffer);
				on[i].y = atof(word[4].buffer);
				on[i].z = atof(word[5].buffer);
				on[i].w = 1;
			}
			++i; // increment count when valid data is processed
		}
	}

	// face section 
	for (i = 0; i < nf; ) 
	{
		bp = fgets(buffer, 256, fp);
		gua_parse(buffer, &n, word);
		if (!n) // error
		{
			return 1;
		}
		else if (word[0].buffer[0] == '#')
		{
			continue;
		}
		of[i].nVtx = atoi(word[0].buffer);
		if (n < of[i].nVtx + 1)
			return 1; // errror
		else if (of[i].nVtx > 32 || of[i].nVtx < 0)
			return 1; // max 

		of[i].vtx = gua_integer_array(tdb, of[i].nVtx); // array for vertex indices
		if (on)
			of[i].nml = gua_integer_array(tdb, of[i].nVtx); // array for normal indices
		else
			of[i].nml = 0;

		for (j = 1, k=0; j < of[i].nVtx +1; ++j, ++k)
		{
			of[i].vtx[k] = atoi(word[j].buffer); // save original indices
			if (on)
				of[i].nml[k] = of[i].vtx[k];
		}
		of[i].id = i;
		ocf[i] = 0;

		if (n >= of[i].nVtx + 1 + 3) // color 
		{
			j = of[i].nVtx + 1;
			if (!colorFormat)
			{
				if (strchr(word[j+0].buffer, '.') || strchr(word[j+1].buffer, '.') || strchr(word[j+2].buffer, '.'))
					colorFormat = COLOR_FORMAT_ZERO_TO_ONE;
				else
					colorFormat = COLOR_FORMAT_255;
			}
			ocf[i] = 1; // flag indicating that color was explicitly defined
			switch (colorFormat) {
			case COLOR_FORMAT_ZERO_TO_ONE:
				of[i].color.r = (float)atof(word[j+0].buffer);
				of[i].color.g = (float)atof(word[j+1].buffer);
				of[i].color.b = (float)atof(word[j+2].buffer);
				of[i].color.a = (float)((DS_COLOR*)defaultColor)->a;
				if (n >= of[i].nVtx + 1 + 4) // color 
					of[i].color.a = (float)atof(word[j + 3].buffer);
				break;
			case COLOR_FORMAT_255:
				of[i].color.r = (float)(atoi(word[j+0].buffer) / 255.0);
				of[i].color.g = (float)(atoi(word[j+1].buffer) / 255.0);
				of[i].color.b = (float)(atoi(word[j+2].buffer) / 255.0);
				of[i].color.a = (float)((DS_COLOR*)defaultColor)->a;
				if (n >= of[i].nVtx + 1 + 4) // color 
					of[i].color.a = (float)(atoi(word[j + 3].buffer) / 255.0);
				break;
			}
		}
		++i; // increment count when valid data is processed
	}

	if ( ctx->inputTrans.transformFlag ) // build transformation matrix
	{
		ds_geo_build_transform_matrix(ctx);
	}

	// process all faces 
	for (i = 0; i < nf; ++i)
	{
		modulo = of[i].nVtx;

		// make a copy of all the original vertices based on index reference
		for (j = 0; j < of[i].nVtx; ++j)
		{
			vpt[j] = ov[of[i].vtx[j]];
			if(on)
				npt[j] = on[of[i].nml[j]]; 
		}

		if (!ocf[i])
			of[i].color = *(DS_COLOR*)defaultColor; // use the default color since it wasn't explicitly defined

		// transform vertices
		if (ctx->inputTrans.transformFlag)
		{
			mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[0]);
			for (j = 0; j < of[i].nVtx; ++j)
				vpt[j] = vtmp[j];
			if (on) // transform normals
			{
				mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&npt[0], (MTX_VECTOR*)&ntmp[0], &ctx->inputTrans.matrix[0]);
				for (j = 0; j < of[i].nVtx; ++j)
					npt[j] = ntmp[j];
			}
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
				if (on) // normals
				{
					for (j = 0; j < of[i].nVtx; ++j)
						npt_dup[j] = npt[j];
				}
			}
			else
			{	// mirror the x coord
				for (j = 0; j < of[i].nVtx; ++j)
				{
					vpt[j] = vpt_dup[j]; vpt[j].x *= -1;
				}
				if (on) // normals
				{
					for (j = 0; j < of[i].nVtx; ++j)
					{
						npt[j] = npt_dup[j]; npt[j].x *= -1;
					}
				}
			}

			for (j = 0; j < n; ++j) // z rotations
			{
				for (l = 0; l < of[i].nVtx; ++l)
				{
					v[l].p = vpt[l]; // copy coordinate data into node structure
					of[i].vtx[l] = vid[l] = vins((void*)vdb, &v[l]); // insert into tree
					++vdb->nProcessed;
					if (on)
					{
						nml[l].p = npt[l];
						of[i].nml[l] = nid[l] = vins((void*)ndb, &nml[l]); // insert into tree
						++ndb->nProcessed;
					}
				}
				if (of[i].nVtx == 1)
				{
					elid[0] = vid[0]; // no edges to inherit vertex ID

					++tedb->nProcessed;
					tid = teins((void*)tedb, elid, of[i].nVtx); // use array of edge length IDs 
					++tdb->nProcessed;
					ttid = tins((void*)tdb, vid, nid, of[i].nVtx, on ? 1 : 0, tid, (float*)&of[i].color); // use array of unique vertex IDs 

					if (ctx->inputTrans.replicateFlag && j < 2)
					{	// perform rotation transformation on coordinate data
						mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[0]);
						for (j = 0; j < of[i].nVtx; ++j)
							vpt[j] = vtmp[j];
						if (on)
						{
							mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&npt[0], (MTX_VECTOR*)&ntmp[0], &ctx->inputTrans.matrix[0]);
							for (j = 0; j < of[i].nVtx; ++j)
								npt[j] = ntmp[j];
						}
					}
				}
				else
				{
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

					++tedb->nProcessed;
					tid = teins((void*)tedb, elid, of[i].nVtx); // use array of edge length IDs 
					++tdb->nProcessed;
					ttid = tins((void*)tdb, vid, nid, of[i].nVtx, on ? 1 : 0, tid, (float*)&of[i].color); // use array of unique vertex IDs 

					if (ctx->inputTrans.replicateFlag && j < 2)
					{	// perform rotation transformation on coordinate data
						mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&vpt, (MTX_VECTOR*)&vtmp, &ctx->inputTrans.matrix[0]);
						for (j = 0; j < of[i].nVtx; ++j)
							vpt[j] = vtmp[j];
						if (on)
						{
							mtx_vector_multiply(of[i].nVtx, (MTX_VECTOR*)&npt, (MTX_VECTOR*)&ntmp, &ctx->inputTrans.matrix[0]);
							for (j = 0; j < of[i].nVtx; ++j)
								npt[j] = ntmp[j];
						}
					}
				}
			}
		}
	}

	if (!nf) // no defined faces so just save vertices
	{
		// process all vertices 
		for (i = 0; i < nv; ++i)
		{
			vpt[0] = ov[i];


			modulo = of[i].nVtx;
													 // transform vertices
			if (ctx->inputTrans.transformFlag)
			{
				mtx_vector_multiply(1, (MTX_VECTOR*)&vpt, (MTX_VECTOR*)&vtmp, &ctx->inputTrans.matrix[0]);
				vpt[0] = vtmp[0];
				if (on)
				{
					mtx_vector_multiply(1, (MTX_VECTOR*)&npt, (MTX_VECTOR*)&ntmp, &ctx->inputTrans.matrix[0]);
					npt[0] = ntmp[0];
				}
			}

			// replicate as required
			m = ctx->inputTrans.mirrorFlag ? 3 : 1;
			n = ctx->inputTrans.replicateFlag ? 3 : 1;
			for (k = 0; k < m; ++k) // mirror loop
			{
				if (!k)
				{
					vpt_dup[0] = vpt[0];
					if (on)
						npt_dup[0] = npt[0];
				}
				else
				{	// mirror the x coord
					vpt[0] = vpt_dup[0]; vpt[0].x *= -1;
					if (on)
						npt[0] = npt_dup[0]; npt[0].x *= -1;
				}

				for (j = 0; j < n; ++j) // z rotations
				{
					v[0].p = vpt[0]; // copy coordinate data into node structure
					vid[0] = vins((void*)vdb, &v[0]); // insert into tree
					++vdb->nProcessed;

					if (on)
					{
						nml[0].p = npt[0]; // copy coordinate data into node structure
						nid[0] = vins((void*)ndb, &nml[0]); // insert into tree
						++ndb->nProcessed;
					}
					if (ctx->inputTrans.replicateFlag && j < 2)
					{	// perform rotation transformation on coordinate data
						mtx_vector_multiply(1, (MTX_VECTOR*)&vpt[0], (MTX_VECTOR*)&vtmp[0], &ctx->inputTrans.matrix[0]);
						vpt[0] = vtmp[0];
						if (on)
						{
							mtx_vector_multiply(1, (MTX_VECTOR*)&npt[0], (MTX_VECTOR*)&ntmp[0], &ctx->inputTrans.matrix[0]);
							npt[0] = ntmp[0];
						}
					}
				}
			}
		}
	}

	// remove temp memory
	FREE(ov); 
	FREE(ocf);
	FREE(of);
	if(on)
		FREE(on);

	return 0; // success
}
