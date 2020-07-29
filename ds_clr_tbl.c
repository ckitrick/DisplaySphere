#pragma once
/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions are designed to handle color tables. 
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

typedef struct {
	unsigned char	r, g, b;
} HCOLOR;

static HCOLOR hclr008[8] = {
0xff, 0xb3, 0x62,
0x00, 0x39, 0xb1,
0x01, 0xc0, 0x80,
0xef, 0x00, 0x20,
0x52, 0x7f, 0xff,
0x9c, 0x00, 0x5e,
0xcb, 0xad, 0xff,
0x00, 0x2d, 0x61,
};

static HCOLOR hclr016[16] = {
0xf6, 0xa5, 0x00,
0x5e, 0x00, 0xb8,
0x4a, 0xe0, 0x82,
0xff, 0x4f, 0xe8,
0x67, 0x9d, 0x00,
0x5f, 0x8d, 0xff,
0x00, 0x74, 0x34,
0xed, 0x00, 0x59,
0x01, 0xa6, 0xd0,
0x51, 0x4f, 0x00,
0x01, 0x33, 0x85,
0xfc, 0xb4, 0xaf,
0x45, 0x09, 0x4a,
0xd6, 0xbd, 0xe6,
0x71, 0x47, 0x55,
0xff, 0xa9, 0xd3,
};

static HCOLOR hclr032[32] = {
0xff, 0xb7, 0x75,
0x2d, 0x00, 0x96,
0xec, 0xc2, 0x2b,
0xb8, 0x2d, 0xe3,
0x00, 0x77, 0x12,
0xc5, 0x71, 0xff,
0x8f, 0x88, 0x00,
0x01, 0x41, 0xc7,
0xff, 0x21, 0x15,
0x27, 0x88, 0xff,
0xb8, 0x57, 0x00,
0x00, 0xca, 0xf5,
0xfd, 0x00, 0x60,
0x01, 0xc2, 0xbf,
0xff, 0x11, 0x89,
0x7c, 0xda, 0x97,
0x42, 0x04, 0x56,
0x63, 0x56, 0x00,
0xff, 0x77, 0xde,
0x2c, 0x3d, 0x00,
0xd9, 0x9d, 0xff,
0x3d, 0x1f, 0x07,
0xa4, 0xc5, 0xff,
0xb5, 0x00, 0x24,
0x00, 0x7e, 0x6e,
0xff, 0x65, 0x63,
0x01, 0x7d, 0xb2,
0x50, 0x06, 0x1c,
0xe3, 0xb6, 0xf6,
0x00, 0x47, 0x86,
0xcd, 0xa9, 0x90,
0xff, 0xac, 0xc9,
};

static HCOLOR hclr064[64] = {
0xc7, 0xce, 0x42,
0x4a, 0x1a, 0xc6,
0x34, 0xdb, 0x25,
0x91, 0x18, 0xd1,
0x9d, 0xd8, 0x1d,
0x59, 0x65, 0xff,
0x01, 0xb2, 0x35,
0xab, 0x00, 0x9b,
0x98, 0xb4, 0x00,
0x00, 0x34, 0xaf,
0xff, 0xaa, 0x17,
0x35, 0x00, 0x73,
0x8b, 0xd9, 0x73,
0x01, 0x73, 0xe5,
0xf3, 0x00, 0x06,
0x20, 0xe0, 0xa1,
0xdd, 0x00, 0x5f,
0x00, 0x98, 0x4b,
0xb3, 0x8c, 0xff,
0x2e, 0x85, 0x00,
0xdb, 0x9a, 0xff,
0x85, 0x8f, 0x00,
0xff, 0xa1, 0xfa,
0x5d, 0x79, 0x00,
0xff, 0x86, 0xd2,
0x28, 0x4e, 0x00,
0x67, 0x00, 0x5a,
0xe4, 0xc4, 0x4e,
0x1d, 0x1d, 0x5c,
0xc1, 0x97, 0x00,
0x00, 0x34, 0x6d,
0xff, 0x73, 0x2a,
0x22, 0xc2, 0xff,
0xb6, 0x00, 0x1b,
0x00, 0x9d, 0x79,
0xa5, 0x00, 0x56,
0xb4, 0xd1, 0x78,
0x3f, 0x13, 0x42,
0xdb, 0xc6, 0x71,
0x00, 0x32, 0x57,
0xff, 0x7b, 0x3f,
0x93, 0xb7, 0xff,
0xb5, 0x40, 0x00,
0x8d, 0xcf, 0xea,
0x71, 0x29, 0x00,
0x00, 0x97, 0x92,
0xff, 0x74, 0x8d,
0x00, 0x36, 0x0d,
0xff, 0x8e, 0x5a,
0x00, 0x54, 0x2d,
0xd2, 0xbd, 0xef,
0x9b, 0x59, 0x00,
0x4f, 0x05, 0x2e,
0xfe, 0xb8, 0x6f,
0x67, 0x00, 0x1d,
0xd3, 0xc6, 0x96,
0x43, 0x1a, 0x05,
0xf3, 0xb5, 0xcb,
0x64, 0x4e, 0x00,
0xff, 0xa2, 0x85,
0x53, 0x5d, 0x3c,
0x8a, 0x67, 0x00,
0x92, 0x9e, 0x7a,
0x93, 0x74, 0x5b,
};

static DS_COLOR c008[8];
static DS_COLOR c016[16];
static DS_COLOR c032[32];
static DS_COLOR c064[64];

//--------------------------------------------------------------------------------
void ds_clr_default_color_tables(DS_CTX *ctx) 
//--------------------------------------------------------------------------------
{
	// Initialize the color table set with the built in set (8,16,32,64)
	DS_COLOR_TABLE		ct;
	DS_COLOR			color[128];
	int					i;

	ct.color = color;
	for (i = 0; i < 8; ++i)
	{
		ct.color[i].r = (float)(hclr008[i].r / 255.0);
		ct.color[i].g = (float)(hclr008[i].g / 255.0);
		ct.color[i].b = (float)(hclr008[i].b / 255.0);
	}
	ct.nColors = 8;
	ds_ctbl_add_color_table(&ctx->cts, &ct);
	for (i = 0; i < 16; ++i)
	{
		ct.color[i].r = (float)(hclr016[i].r / 255.0);
		ct.color[i].g = (float)(hclr016[i].g / 255.0);
		ct.color[i].b = (float)(hclr016[i].b / 255.0);
	}
	ct.nColors = 16;
	ds_ctbl_add_color_table(&ctx->cts, &ct);
	for (i = 0; i < 32; ++i)
	{
		ct.color[i].r = (float)(hclr032[i].r / 255.0);
		ct.color[i].g = (float)(hclr032[i].g / 255.0);
		ct.color[i].b = (float)(hclr032[i].b / 255.0);
	}
	ct.nColors = 32;
	ds_ctbl_add_color_table(&ctx->cts, &ct);
	for (i = 0; i < 64; ++i)
	{
		ct.color[i].r = (float)(hclr064[i].r / 255.0);
		ct.color[i].g = (float)(hclr064[i].g / 255.0);
		ct.color[i].b = (float)(hclr064[i].b / 255.0);
	}
	ct.nColors = 64;
	ds_ctbl_add_color_table(&ctx->cts, &ct);
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
	DS_COLOR_TABLE	*p;
	int			i;

	p = (DS_COLOR_TABLE*)cts->head;
	while (p)
	{
		if (p->nColors == ct->nColors) // replace 
		{	// copy data
			for (i = 0; i < ct->nColors; ++i)
				p->color[i] = ct->color[i]; // copy color
			return 0;
		}
		p = (DS_COLOR_TABLE*)p->next;
	}

	++cts->nTables;
	p = (DS_COLOR_TABLE*)malloc(sizeof(DS_COLOR_TABLE));
	p->color = (DS_COLOR*)malloc(sizeof(DS_COLOR) * ct->nColors);
	p->nColors = ct->nColors;
	p->next = 0; 

	for (i = 0; i < ct->nColors; ++i)
		p->color[i] = ct->color[i];

	if (!cts->head)
		cts->head = (void*)p;

	if (cts->tail)
		((DS_COLOR_TABLE*)cts->tail)->next = (void*)p;

	cts->tail = (void*)p;
	return 0;
}

//-------------------------------------------------------------------------
DS_COLOR_TABLE *ds_ctbl_get_color_table(DS_COLOR_TABLE_SET *cts, int nColors)
//-------------------------------------------------------------------------
{
	// Find the best matching color table based on the number of colors needed
	// Returns a pointer to color table
	//
	DS_COLOR_TABLE	*p, *lp=0, *hp=0;
	int				delta, hi, lo;

	p = (DS_COLOR_TABLE*)cts->head;
	hi = 100000;
	lo = 100000;

	while (p) // check every table
	{
		delta = p->nColors - nColors;
		if (!delta) // match
			return p;
		else if (delta < 0) // not enough colors
		{
			delta *= -1; // change sign
			if (delta < lo)
			{
				lp = p;
				lo = delta;
			}
		}
		else // too many colors
		{
			if (delta < hi)
			{
				hp = p; // save pointer
				hi = delta;
			}
		}
		p = (DS_COLOR_TABLE*)p->next;
	}

	if (hp)
		return hp; // best fit that is available with more colors
	else if (lp)
		return lp; // best fit with less colors 
	else
		return 0; // no color table available   
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

		if (n == 1)
		{
			if (*nc == max)
				return 1; // color table exceeded

						  // get color from hex
			i = ds_hex_to_color(word[0].buffer, &num, &ur, &ug, &ub);
			c[*nc].r = (float)(ur / 255.0);
			c[*nc].g = (float)(ug / 255.0);
			c[*nc].b = (float)(ub / 255.0);
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
		return 1; // failed to open file

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
