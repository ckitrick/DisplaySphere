#pragma once
/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#ifndef DISP_COLOR_HEADER
#define DISP_COLOR_HEADER

typedef struct {
	float	r,
			g,
			b,
			a;
} DS_COLOR;

typedef struct {
	char	buffer[32];
} DS_CWORD;

typedef struct {
	int				nColors;
	DS_COLOR		*color;
	void			*next;
} DS_COLOR_TABLE;

typedef struct {
	int				nTables;
	void			*head;
	void			*tail;
} DS_COLOR_TABLE_SET;

int ds_ctbl_add_color_table(DS_COLOR_TABLE_SET *cts, DS_COLOR_TABLE *ct);
DS_COLOR_TABLE *ds_ctbl_get_color_table(DS_COLOR_TABLE_SET *cts, int nColors);
int ds_ctbl_read_color_table(FILE *fp, DS_COLOR_TABLE *ct, int max, int *nc, int *more);
int ds_ctbl_get_color(DS_COLOR_TABLE *ct, int index, DS_COLOR **color);
int ds_ctbl_process_color_table_file(DS_COLOR_TABLE_SET *cts, char *filename);
int ds_ctbl_get_color_new(DS_COLOR_TABLE *ct, int index, int reverseFlag, DS_COLOR **color);

#endif DISP_COLOR_HEADER
