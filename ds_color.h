#pragma once
#ifndef DISP_COLOR_HEADER
#define DISP_COLOR_HEADER

typedef struct {
	float	r,
			g,
			b;
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
