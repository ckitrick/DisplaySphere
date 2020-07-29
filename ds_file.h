/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

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