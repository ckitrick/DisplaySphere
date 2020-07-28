
#ifndef FILE_HEADER
#define FILE_HEADER

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <direct.h>
#include <commdlg.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ds_sph.h"
#include "ds_par.h"

// Functions
int		ds_file_initialization(HWND hWnd, char *filename);
int		ds_open_file_dialog		(HWND hOwnerWnd, DS_CTX *ctx, int type);
int		ds_write_file_dialog	(HWND hOwnerWnd, DS_CTX *ctx, int type);
int		ds_read_file_from_buffer( DS_CTX *ctx );
int		ds_file_drag_and_drop	( HWND hWnd, HDROP hdrop );
int		ds_process_restore_file(DS_CTX *ctx, char *filename);

#endif