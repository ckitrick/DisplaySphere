#pragma once
/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions are designed to handle color tables. 
	Alpha is always set to 1.0
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
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"

typedef unsigned int IHCOLOR;

static IHCOLOR ihclr004[4] = { 0xdec964, 0xdbb1cf, 0x90e168, 0x99d8c8};
static IHCOLOR ihclr008[8] = { 
0xa2c8e3,
0xceda4b,
0xe7a5dd,
0x7ae263,
0xe6bba7,
0x70e1c3,
0xe8b558,
0xbbd89c,
};
static IHCOLOR ihclr012[12] = { 0x7bdea1,
0xdea6e4,
0xa6e43d,
0x81c7eb,
0xe2ca39,
0xe1b8c7,
0x63e368,
0xeeab7a,
0x83ded0,
0xb8dc6c,
0xc1d1cc,
0xd2d18e,
};
static IHCOLOR ihclr016[16] = { 0xaed6c2,
0xc7e235,
0xeb9de3,
0x70e45a,
0xc0b6eb,
0xe7c135,
0x75c3f2,
0xb5dc68,
0xe5b3c6,
0x68e1a1,
0xf1a66d,
0x5fdfd9,
0xdccc6e,
0xa4cee2,
0xbcd899,
0xe1c3a5,
};
static IHCOLOR ihclr024[24] = { 0x5eead9,
0xf993d2,
0x6ae459,
0xdcaae8,
0xacdf42,
0x9ab5f1,
0xdeda31,
0x77cdef,
0xe9b346,
0x59d5de,
0xf49f89,
0x56e5a1,
0xe7bac2,
0xa3e078,
0xbebfdd,
0xd4d363,
0xbde6e8,
0xe7be88,
0x89dbac,
0xcad58d,
0x93c7c1,
0x99da90,
0xb4c0a0,
0xd8e7c5,
};
static IHCOLOR ihclr032[32] = { 0xe3bfd0,
0x64e74e,
0xe29cf1,
0xc3eb3f,
0x83acf3,
0xdcd733,
0xeda2d8,
0x90db51,
0xc4b6eb,
0xecba38,
0x5bcfeb,
0xd4d357,
0x92c5f0,
0xa9c554,
0xf59ba9,
0x61e385,
0xacafcb,
0xcfeb89,
0xcfd3ee,
0x4fe7ae,
0xf0ab70,
0x59e1d7,
0xdecd76,
0xaedade,
0x9ada89,
0xe5b6a5,
0x87d9aa,
0xdad8bf,
0xadebd6,
0xb9bf8b,
0x99beac,
0xdce7b0,

};
static IHCOLOR ihclr064[64] = { 0xc7ce42,
0x4a1ac6,
0x34db25,
0x9118d1,
0x9dd81d,
0x5965ff,
0x01b235,
0xab009b,
0x98b400,
0x0034af,
0xffaa17,
0x350073,
0x8bd973,
0x0173e5,
0xf30006,
0x20e0a1,
0xdd005f,
0x00984b,
0xb38cff,
0x2e8500,
0xdb9aff,
0x858f00,
0xffa1fa,
0x5d7900,
0xff86d2,
0x284e00,
0x67005a,
0xe4c44e,
0x1d1d5c,
0xc19700,
0x00346d,
0xff732a,
0x22c2ff,
0xb6001b,
0x009d79,
0xa50056,
0xb4d178,
0x3f1342,
0xdbc671,
0x003257,
0xff7b3f,
0x93b7ff,
0xb54000,
0x8dcfea,
0x712900,
0x009792,
0xff748d,
0x00360d,
0xff8e5a,
0x00542d,
0xd2bdef,
0x9b5900,
0x4f052e,
0xfeb86f,
0x67001d,
0xd3c696,
0x431a05,
0xf3b5cb,
0x644e00,
0xffa285,
0x535d3c,
0x8a6700,
0x929e7a,
0x93745b,
};

//--------------------------------------------------------------------------------
static int compare(void *passThru, void *av, void *bv)
//--------------------------------------------------------------------------------
{
	int		diff = ((DS_COLOR_TABLE*)av)->nColors - ((DS_COLOR_TABLE*)bv)->nColors;

	if (!diff)
		((DS_COLOR_TABLE_SET*)passThru)->match = av; // save existing match

	return diff;
}

//--------------------------------------------------------------------------------
static int compare2(void *passThru, void *userdata)
//--------------------------------------------------------------------------------
{
	if (((DS_COLOR_TABLE*)userdata)->nColors >= ((DS_COLOR_TABLE_SET*)passThru)->nColors)
	{
		((DS_COLOR_TABLE_SET*)passThru)->match = (DS_COLOR_TABLE*)userdata;
		return 1; // stop traversal
	}
	else
	{
		((DS_COLOR_TABLE_SET*)passThru)->match = (DS_COLOR_TABLE*)userdata; // will end up with largest table
		return 0;
	}
}

//--------------------------------------------------------------------------------
static int output_table (void *passThru, void *userdata)
//--------------------------------------------------------------------------------
{
	DS_COLOR_TABLE_SET	*cts = (DS_COLOR_TABLE_SET*)passThru;
	DS_COLOR_TABLE		*ct = (DS_COLOR_TABLE*)userdata;
	int					i;

	fprintf(cts->fp, "\n// %d entry\n", ct->nColors);
	for (i = 0; i < ct->nColors; ++i)
	{
		// write color
		fprintf(cts->fp, "#%02x%02x%02x\n", (int)(ct->color[i].r * 255), (int)(ct->color[i].g * 255), (int)(ct->color[i].b * 255));
	}
	return 0;
}

//--------------------------------------------------------------------------------
static int process(DS_COLOR_TABLE_SET *cts, DS_COLOR_TABLE *ct, IHCOLOR *array, int length)
//--------------------------------------------------------------------------------
{
	int		i;
	for (i = 0; i < length; ++i)
	{
		ct->color[i].r = (float)(((array[i] >> 16) & 0xff) / 255.0);
		ct->color[i].g = (float)(((array[i] >>  8) & 0xff) / 255.0);
		ct->color[i].b = (float)(((array[i] >>  0) & 0xff) / 255.0);
		ct->color[i].a = (float)1.0;
	}
	ct->nColors = length;
	ds_ctbl_add_color_table(cts, ct);
	return 0;
}

//--------------------------------------------------------------------------------
void ds_clr_default_color_tables(DS_CTX *ctx) 
//--------------------------------------------------------------------------------
{
	// initialize the AVL tree
	ctx->cts.avl = avl_create(compare, (void*)&ctx->cts );
	ctx->cts.match = 0;

	// Initialize the color table set with the built in set (4,8,12,16,24,32,64)
	DS_COLOR_TABLE		ct;
	DS_COLOR			color[128];

	ct.color = color;
	process(&ctx->cts, &ct, ihclr004,  4);
	process(&ctx->cts, &ct, ihclr008,  8);
	process(&ctx->cts, &ct, ihclr012, 12);
	process(&ctx->cts, &ct, ihclr016, 16);
	process(&ctx->cts, &ct, ihclr024, 24);
	process(&ctx->cts, &ct, ihclr032, 32);
	process(&ctx->cts, &ct, ihclr064, 64);
}

//-------------------------------------------------------------------------
static int ds_hex_to_color(char *str, int *num, unsigned char *r, unsigned char *g, unsigned char *b)
//-------------------------------------------------------------------------
{
	// Convert string that can be in two formats 0xff or #ff (CSS)
	// Extract 8bit color information.
	char	c;
	int  	i, j, k, len;

	len = strlen(str);
	*num = 0;

	if (str[0] == '#') k = 1;
	else if (!strncmp(str, "0x", 2)) k = 2;
	else return 1; // failure 

	for (i = len - 1, j = 1; i >= k; --i, j *= 16)
	{
		c = str[i];
		if (c >= '0' && c <= '9')		*num += (c - '0') * j;
		else if (c >= 'a' && c <= 'f')	*num += (c - 'a' + 10) * j;
		else if (c >= 'A' && c <= 'F')	*num += (c - 'A' + 10) * j;
	}

	*b = *num & 0xff;
	*g = (*num >> 8) & 0xff;
	*r = (*num >> 16) & 0xff;
	return 0;
}

//-------------------------------------------------------------------------
static int ds_text_to_color(int *colorFormat, char *string, float *color)
//-------------------------------------------------------------------------
{
	// Convert text to appropriate color value
	// If colorFormat is not set then look for presence of period '.' indicating 
	// a real number, then set the colorFormat
	// 
	if (!*colorFormat)
	{
		if (strchr(string, '.'))	*colorFormat = COLOR_FORMAT_ZERO_TO_ONE;
		else						*colorFormat = COLOR_FORMAT_255;
	}
	switch (*colorFormat) {
	case COLOR_FORMAT_ZERO_TO_ONE:
		*color = (float)atof(string);
		if (*color < 0)				*color = 0;
		else if (*color > 1.0)		*color = 1.0;
		break;
	case COLOR_FORMAT_255:
		*color = (float)(atof(string) / 255.0);
		if (*color < 0.0)			*color = 0.0;
		else if (*color > 255.0)	*color = 255.0;
		break;
	}
	return 0;
}

//-------------------------------------------------------------------------
int ds_ctbl_add_color_table(DS_COLOR_TABLE_SET *cts, DS_COLOR_TABLE *ct)
//-------------------------------------------------------------------------
{
	// Add a color table to the color table set 
	// Will search all existing tables to determine if table
	// already exists - if true will replace existing table
	DS_COLOR_TABLE	*p = 0;
	int				i;

	avl_find(cts->avl, (void*)ct, (void*)&p);

//	p = cts->match;

	if ( p )
	{	// copy data to replace current table of same size
		for (i = 0; i < ct->nColors; ++i)
			p->color[i] = ct->color[i]; // copy color
		return 0;
	}
	else
	{
		p = (DS_COLOR_TABLE*)malloc(sizeof(DS_COLOR_TABLE));
		p->color = (DS_COLOR*)malloc(sizeof(DS_COLOR) * ct->nColors);
		p->nColors = ct->nColors;
		p->next = 0;

		for (i = 0; i < ct->nColors; ++i)
			p->color[i] = ct->color[i];

		avl_insert(cts->avl, (void*)p);
	}
	return 0;
}

//-------------------------------------------------------------------------
DS_COLOR_TABLE *ds_ctbl_get_color_table(DS_COLOR_TABLE_SET *cts, int nColors)
//-------------------------------------------------------------------------
{
	cts->match = 0;
	cts->nColors = nColors;

	// Find the best matching color table based on the number of colors needed
	avl_traverse_rtl(cts->avl, (void*)cts, compare2); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned

	return cts->match;
}

//--------------------------------------------------------------------------------
static void ds_clr_parse(char *buffer, int *n, DS_CWORD *word)
//--------------------------------------------------------------------------------
{
	// 	This is a very simple word parser 
	//	Copies each word into word[] buffer and returns the number of words found
	//
	int		i=0, j=0, in=0;
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
}

//-------------------------------------------------------------------------
int ds_ctbl_read_color_table(FILE *fp, DS_COLOR_TABLE *ct, int max, int *nc, int *more)
//-------------------------------------------------------------------------
{
	// Attempt to read a color table from a file 
	// Continue to end of sequence (end of file or invalid data)
	// Empty lines will end a sequence
	//	fp - File pointer
	//	ct - temp color table
	//	max - max number of colors allowed in temp color table 
	//	nc - number of colors found
	//  more - flag that indicates file is finished 
	//	return - 0 if success, 1 - if something wrong

	char			*bp;
	char			buffer[256];
	DS_CWORD		word[32];
	int				n;				// number of words parsed
	unsigned char	ur, ug, ub;
	int				i, num;
	DS_COLOR		*c;
	int				colorFormat = 0;

	if (!*more || !fp)
		return 1; // failure 	

	*nc = 0;
	c = ct->color;

	while (bp = fgets(buffer, 256, fp)) // read a line of color data 
	{
		ds_clr_parse(buffer, &n, word);

		if (!n)
			return 0; // empty lines end sequence 

		if (!strncmp(word[0].buffer, "//", 2))
			continue;

		if (n == 1)
		{
			if (*nc == max)
				return 1; // color table exceeded

						  // get color from hex
			i = ds_hex_to_color(word[0].buffer, &num, &ur, &ug, &ub);
			c[*nc].r = (float)(ur / 255.0);
			c[*nc].g = (float)(ug / 255.0);
			c[*nc].b = (float)(ub / 255.0);
			c[*nc].a = (float)1.0;
			ct->nColors = ++*nc;
		}
		else if (n == 3)
		{
			if (*nc == max)
				return 1; // color table space exceeded

			// get color from strings
			ds_text_to_color(&colorFormat, word[0].buffer, &c[*nc].r);
			ds_text_to_color(&colorFormat, word[1].buffer, &c[*nc].g);
			ds_text_to_color(&colorFormat, word[2].buffer, &c[*nc].b);
			c[*nc].a = (float)1.0; 
			ct->nColors = ++*nc;
		}
		else
			return 0; // succes :: end of sequence 
	}
	*more = 0; // file finished

	return 0; // success
}

//-------------------------------------------------------------------------
int ds_ctbl_get_color(DS_COLOR_TABLE *ct, int index, DS_COLOR **color)
//-------------------------------------------------------------------------
{
	if (!ct || !color)
		return 1; // failure
	*color = &ct->color[index%ct->nColors];
	return 0;
}

//-------------------------------------------------------------------------
int ds_ctbl_get_color_new(DS_COLOR_TABLE *ct, int index, int reverse, DS_COLOR **color)
//-------------------------------------------------------------------------
{
	if (!ct || !color)
		return 1; // failure
	index = index % ct->nColors;
	if (reverse)
		index = ( ct->nColors - 1 ) - index;
	*color = &ct->color[index];
	return 0;
}

//-------------------------------------------------------------------------
int ds_ctbl_process_color_table_file(DS_COLOR_TABLE_SET *cts, char *filename)
//-------------------------------------------------------------------------
{
	FILE			*fp;
	DS_COLOR_TABLE	ct; // temp
	DS_COLOR		clr[256]; // temp
	int				more = 1;
	int				nc = 0;

	if (!strlen(filename))
		return 1; // empty file

	fopen_s(&fp, filename, "r");
	if (!fp)
	{
		char buffer[128];
		sprintf(buffer, "Color table file <%s> failed to open.", filename);
		MessageBox(0, buffer, "File Open Failure", MB_OK);
		return 1; // failed to open file
	}
	else
	{
		char	*bp, buffer[256];
		char	*word[5];
		int		n;
		if (bp = fgets(buffer, 256, fp)) // read first line looking for keyword "DS_COLOR" 
		{
			n = ds_parse_lexeme(buffer, word, 5);
			if (n && strcmp(word[0], "DS_COLOR"))
				rewind(fp);
		}
		else
			return 1; // error
	}

	ct.nColors = 0;
	ct.color = clr;

	while (more)
	{
		if (ds_ctbl_read_color_table(fp, &ct, 256, &nc, &more))
			break;
		else if (nc)
		{
			// SAVE A NEW COLOR TABLE
			ds_ctbl_add_color_table(cts, &ct);
		}
	}
	fclose(fp);

	return 0; // success
}

//-------------------------------------------------------------------------
int ds_ctbl_output_color_table_file(DS_COLOR_TABLE_SET *cts, char *filename)
//-------------------------------------------------------------------------
{
	int				nTables, nLevels;

	if (!strlen(filename))
		return 1; // empty file

	avl_info(cts->avl, &nTables, &nLevels);
		
	if (!nTables)
		return 0; 

	fopen_s(&cts->fp, filename, "w"); // open for write

	if (!cts->fp)
	{
		char buffer[128];
		sprintf(buffer, "Color table output file <%s> failed to open.", filename);
		MessageBox(0, buffer, "File Open Failure", MB_OK);
		return 1; // failed to open file
	}

	if (nTables)
		fprintf(cts->fp, "DS_COLOR\n// AUTO DUMP WITH DSS CREATION\n");

	// Find the best matching color table based on the number of colors needed
	avl_traverse_rtl(cts->avl, (void*)cts, output_table); // Traverse the tree from right to left - if user function returns non-zero traversal is stopped and value returned

	fclose(cts->fp);
	cts->fp = 0;
	return 0;
}
