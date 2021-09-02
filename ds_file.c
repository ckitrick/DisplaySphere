/*
	Copyright (c) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
	This group of functions is designed to handle most of the file access and control.
	Also includes some interaction between window titles and files.
*/

#include <stdlib.h>
#include <stdio.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <direct.h>
#include <commdlg.h>
#include "resource.h"
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <matrix.h>
#include <geoutil.h>
#include <link.h>
#include "ds_sph.h"
#include "ds_par.h"
#include <link.h>
#include <mem.h>
#include "ds_file.h"
#include "ds_gua.h"
#include "ds_color.h"

//-----------------------------------------------------------------------------
int ds_build_url(DS_CTX *ctx, char *url)
//-----------------------------------------------------------------------------
{
	char	env[256], post_env[256], *src, *dst, c;
	sprintf(env, "%%%s%%", PROGRAM_DATA_LOCATION);
	//#define PROGRAM_DOCUMENTATION_URL		"file:///c:/Program%20Files%20(x86)/DisplaySphere/DisplaySphere%20Documentation%20v0-96.pdf"
	sprintf(url, "file:///");
	ds_filename_env_var(env, post_env);
	src = post_env;
	dst = url + strlen(url);
	while (c = *src++)
	{
		if (c == ' ') { *dst++ = '%'; *dst++ = '2'; *dst++ = '0'; }
		else if (c == '\\') *dst++ = '/';
		else *dst++ = c;
	}
	*dst = 0;

	src = PROGRAM_DOCUMENTATION;
	dst = url + strlen(url);
	while (c = *src++)
	{
		if (c == ' ') { *dst++ = '%'; *dst++ = '2'; *dst++ = '0'; }
		else if (c == '\\') *dst++ = '/';
		else *dst++ = c;
	}
	*dst = 0;
	return 0;
}

//-----------------------------------------------------------------------------
int ds_filename_env_var(char *original , char *modified)
//-----------------------------------------------------------------------------
{
	// process a string and expand embedded environment variables: %env_var% 
	char		c;
	while (c = *original++)
	{
		if (c == '%')
		{
			char	buffer[512];
			int		n = 0;
			while (c = *original++)
			{
				if (c == '%')
					break;
				else
					buffer[n++] = c;
			}
			if (n)
			{
				char		env[512], *cp;
				buffer[n++] = 0;
				if (n = GetEnvironmentVariable(buffer, env, 512))
				{
					cp = env;
					while (c = *cp++)
					{
						*modified++ = c;
					}
				}
				else
				{
					return 1; // error
				}
			}
		}
		else
			*modified++ = c;
	}
	*modified = 0;
	return 0;
}

//-----------------------------------------------------------------------------
void ds_filename_split(char *name, int *array, int *count)
//-----------------------------------------------------------------------------
{
	// split a path-filename into parts - this function changes the name string
	int		i, j, flag, len;
	*count = 0;
	flag = 0;
	len = strlen(name);
	array[(*count)++] = 0;
	for (i = 0; i < len; ++i)
	{
		if (name[i] == '\\' || name[i] == '/')
		{
			name[i] = 0;
			flag = 1;
		}
		else if (flag)
		{
			array[(*count)++] = i;
			flag = 0;
		}
	}
/*
	// clean up unneccessary pieces
	for (i = *count - 1; i >= 0; --i)
	{
		if (!strcmp(&name[array[i]], "."))
		{
			if (i == (*count - 1))
				--*count;
			else
			{
				for (j = i + 1; j < (*count); ++j)
				{
					array[j - 1] = array[j];
				}
				--*count;
			}
		}
		else if (!strcmp(&name[array[i]], ".."))
		{
			for (j = i + 1; j < (*count); ++j)
			{
				array[j - 2] = array[j];
			}
			*count -= 2;
		}
	}
*/
}

//-----------------------------------------------------------------------------
void ds_cd_relative_filenameXXX (DS_CTX *ctx, DS_FILE *exec, DS_FILE *object, char *newFilename)
//-----------------------------------------------------------------------------
{
	// create new filename based on its relative location to the current directory path
	// current directory path is from a environment variable (%ProgramData%)
	int		i, j=0, k, l = 0;

	if (ctx->relativeObjPathFlag)
	{
		int		min = exec->count;

		if (min > object->count)
			min = object->count;

		for (i = j = 0; i < min; ++i, ++j)
		{
			if (stricmp(&exec->splitName[exec->word[i]], &object->splitName[object->word[i]]))
				break;
		}

		if (j)
		{
//			strcpy(&newFilename[l], "%ProgramData%");
//			strcpy(&newFilename[l], "%USERPROFILE%");
			strcpy(&newFilename[l], "%PUBLIC%");
			l += strlen(newFilename);

			k = exec->count - j;
			if (!k)
			{
				//			strcpy(&newFilename[l], "./");
				//			l += 2; //strlen(newFilename[l])
							strcpy(&newFilename[l], "/");
							l += 1; //strlen(newFilename[l])
			}
			else
			{
				for (i = 0; i < k; ++i)
				{
//					strcpy(&newFilename[l], "../");
//					l += 3;
					strcpy(&newFilename[l], "/../");
					l += 4;
				}
			}
		}
	}
	for (i = j; i < object->count; ++i)
	{
		strcpy(&newFilename[l], &object->splitName[object->word[i]]);
		l += strlen(&object->splitName[object->word[i]]);
		if (i < object->count - 1)
		{
			strcpy(&newFilename[l], "/");
			l += 1;
		}
	}
}

//-----------------------------------------------------------------------------
void ds_cd_relative_filenameX(DS_CTX *ctx, DS_FILE *base, int basePath, DS_FILE *file, int filePath, char *newFilename)
//-----------------------------------------------------------------------------
{
	// create new filename based on its relative location to the base directory path
	int		i, j = 0, k, l = 0;
	int		baseCount, fileCount;

	// choose how to compare DS_FILE by full name or just by path
	baseCount = basePath ? base->count - 1 : base->count;
	fileCount = filePath ? file->count - 1 : file->count;

	if (ctx->relativeObjPathFlag)
	{
		// determine where the base path and file path deviate
		int		min = baseCount;

		if (min > fileCount)
			min = fileCount;

		for (i = j = 0; i < min; ++i, ++j)
		{
			if (stricmp(&base->splitName[base->word[i]], &file->splitName[file->word[i]]))
				break;
		}
		if (j == min) // exclude a complete match
		{
			if (filePath)
				strcpy(newFilename, ".");
			else
				strcpy(newFilename, &file->splitName[file->word[file->count - 1]]);
			return;
		}
		else if (j) //&& j != ( min - 1 )) // exclude a complete match
		{
			k = baseCount - j;
			if (!k)
			{
			}
			else
			{
				for (i = 0; i < k; ++i)
				{
					//					strcpy(&newFilename[l], "/../");
					strcpy(&newFilename[l], "../");
					l += 3;
				}
			}
		}
	}
	for (i = j; i < fileCount; ++i)
	{
		strcpy(&newFilename[l], &file->splitName[file->word[i]]);
		l += strlen(&file->splitName[file->word[i]]);
		if (i < file->count - 1)
		{
			strcpy(&newFilename[l], "/");
			l += 1;
		}
	}
}

//-----------------------------------------------------------------------------
void ds_cd_relative_filename (DS_CTX *ctx, DS_FILE *base, DS_FILE *file, char *newFilename)
//-----------------------------------------------------------------------------
{
	// create new filename based on its relative location to the base directory path
	//
	int		i, j, k, l = 0;

	if (ctx->relativeObjPathFlag)
	{
		// determine where the base path and file path deviate
		int		min = base->count;

		// set min to the shortest path from either 
		if (min > file->count)
			min = file->count;

		// loop to find where paths deviate
		for (i = j = 0; i < min; ++i, ++j)
		{
			if (stricmp(&base->splitName[base->word[i]], &file->splitName[file->word[i]]))
				break; // stop if paths don't match (j equals index where the paths are no longer the same)
		}

		if ( k = base->count - j ) 
		{
			// add necessary change in path
			for (i = 0; i < k; ++i)
			{
				strcpy(&newFilename[l], "../");
				l += 3;
			}
		}
	}

	// add file and its path from where it deviates from base path
	for (i = j; i < file->count; ++i)
	{
		strcpy(&newFilename[l], &file->splitName[file->word[i]]);
		l += strlen(&file->splitName[file->word[i]]);
		if (i < file->count - 1)
		{
			// always separate path components by standard backslash character
			strcpy(&newFilename[l], "/");
			l += 1;
		}
	}

	// final check
	if (!l) // nothing was built since there is a complete match
	{
		strcpy(newFilename, "."); // should only occur when both base and file are identical
	}
}

//-----------------------------------------------------------------------------
DS_FILE *ds_build_dsf(DS_FILE *dsf, char *buffer, int pathFlag)
//-----------------------------------------------------------------------------
{
	// buffer is the input filename/path (may have imbedded env variable)
	// either use what is provided or create one
	if (!dsf)
	{
		if (!(dsf = (DS_FILE*)malloc(sizeof(DS_FILE))))
			return dsf;
	}
	else
	{
		if (dsf->fullName) { free(dsf->fullName); dsf->fullName = 0; }
		if (dsf->userName) { free(dsf->userName); dsf->userName = 0; }
		if (dsf->splitName) { free(dsf->splitName); dsf->splitName = 0; }
		if (dsf->word) { free(dsf->word); dsf->word = 0; }
		if (dsf->fp) { fclose(dsf->fp); dsf->fp = 0; }
	}

	int		i; // , j;
	int		word[128];
	char	string[512];

	// initialize memory
	dsf->fullName	= 0;
	dsf->userName	= 0;
	dsf->nameOnly	= 0;
	dsf->word		= 0;
	dsf->fp			= 0;
	dsf->splitName	= 0;

	if (!strlen(buffer)) // nothing more to do
		return dsf;

	// process any embedded environment variables 
	ds_filename_env_var(buffer, string);

	dsf->fullName  = (char*)malloc(strlen(string) + 1);
	dsf->splitName = (char*)malloc(strlen(string) + 1);
	strcpy(dsf->fullName, string);
	strcpy(dsf->splitName, string);
	ds_filename_split(dsf->splitName, word, &dsf->count);
	if (pathFlag)
	{
		if (dsf->count > 1)
		{
			dsf->fullName[word[dsf->count - 1] - 1] = 0; // truncate the string
			--dsf->count; // remove the name of the file to just get the path
		}
	}
	if (dsf->count)
	{
		dsf->word = (int*)malloc(sizeof(int)*dsf->count);
		for (i = 0; i < dsf->count; ++i)
			dsf->word[i] = word[i];
	}
//	// make a copy of the fullName's path only
//	for (i = 0, j = 0; i < dsf->count; ++i)
//	{
//		sprintf(&buffer[j], "%s", &dsf->splitName[dsf->word[i]]);
//		j += strlen(&buffer[j]);
//
//		if (i < dsf->count - 1)
//		{
//			buffer[j++] = '/';
//			buffer[j] = 0;
//		}
//	}
	return dsf;
}

//-----------------------------------------------------------------------------
DS_FILE *ds_file_open(DS_CTX *ctx, char *userName, char *mode)
//-----------------------------------------------------------------------------
{
	FILE		*fp;
	// standard interface to open file

	if (!fopen_s(&fp, userName, mode))
	{
		char	fullName[512], *nameOnly;
		DS_FILE	*dsf = (DS_FILE*)malloc(sizeof(DS_FILE));
		int		word[128];
		int		i;

		if (!dsf)
			return 0;

		dsf->userName	= 0;
		dsf->fullName	= 0;
		dsf->nameOnly	= 0;
		dsf->splitName	= 0;
		dsf->fp			= fp;
		dsf->userName = (char*)malloc(strlen(userName) + 1);
		strcpy(dsf->userName, userName);
		GetFullPathName(userName, 512, fullName, &nameOnly);
		dsf->fullName = (char*)malloc(strlen(fullName) + 1);
		dsf->splitName = (char*)malloc(strlen(fullName) + 1);
		strcpy(dsf->fullName, fullName);
		strcpy(dsf->splitName, fullName);
		dsf->nameOnly = dsf->fullName + (nameOnly - fullName);
		ds_filename_split(dsf->splitName, word, &dsf->count);
		if (dsf->count)
		{
			dsf->word = (int*)malloc(sizeof(int)*dsf->count);
			for (i = 0; i < dsf->count; ++i)
				dsf->word[i] = word[i];
		}
		ds_build_dsf(&ctx->curDir, fullName, 1);
		strcpy(ctx->currentDir, ctx->curDir.fullName);
//		SetCurrentDirectory(ctx->curDir.fullName);
		return dsf;
	}
	else
		return 0;
}

//-----------------------------------------------------------------------------
int ds_file_close(DS_CTX *ctx, DS_FILE *dsf)
//-----------------------------------------------------------------------------
{
	if (!dsf)
		return 1;

	if (dsf->fp)		{ fclose(dsf->fp); dsf->fp = 0; }
	if (dsf->userName)	{ free(dsf->userName); dsf->userName = 0; }
	if (dsf->fullName)	{ free(dsf->fullName); dsf->fullName = 0; }
	if (dsf->splitName) { free(dsf->splitName); dsf->splitName; }
	if (dsf->word)		{ free(dsf->word); dsf->word = 0; }
	
	free(dsf);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_set_window_text(HWND hWnd, char *name)
//-----------------------------------------------------------------------------
{
	DS_CTX		*ctx;

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);
	SetWindowText(hWnd, name);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_type_initialization ( DS_CTX *ctx, char *filename )
//-----------------------------------------------------------------------------
{
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_initialization( HWND hWnd, char *filename )
//-----------------------------------------------------------------------------
{
//	char		c, *p;
//	FILE		*fp;
	DS_CTX		*ctx;
	DS_FILE		*dsf;

	ctx = (DS_CTX*)GetWindowLong ( hWnd, GWL_USERDATA );

	dsf = ds_file_open(ctx, filename, "r");

	if (dsf)
	{
		ds_file_set_window_text(hWnd, dsf->nameOnly);
		if ( !ds_parse_file(ctx, dsf, (DS_GEO_INPUT_OBJECT*)&ctx->defInputObj) )
			ds_file_close(ctx, dsf); // free up the dsf
		else
		{
			fclose(dsf->fp); // close the file
			dsf->fp = 0; // track the closure
		}
	}
	else
	{
		char buffer[128];
		sprintf(buffer, "File <%s> failed to open.", filename);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
	}

	return 0;
}

//-----------------------------------------------------------------------------
int ds_process_restore_file (DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	RECT		before;
	int			w, h, stereoFlag = ctx->drawAdj.stereoFlag;
	char		curArg[128], buffer[128], curDir[512];
	static DS_FILE		base;

	GetWindowRect(ctx->mainWindow, &before);
	w = before.right - before.left - WINDOW_SIZE_OFFSET_WIDTH;
	h = before.bottom - before.top - WINDOW_SIZE_OFFSET_HEIGHT;
	curArg[0] = 0;

	ctx->errorInfo.count = 0;
	/*
		set cur dir to filename location
		DS_FILE	base;
		ds_dsf_file (
	*/	
	ds_build_dsf(&base, filename, 1 ); // get path 
	GetCurrentDirectory(512, curDir);
	SetCurrentDirectory(base.fullName);

	if (ds_restore_state(ctx, filename, &ctx->errorInfo))
	{
		int		i;
		buffer[0] = 0;

		strcat(buffer, "State restoration contained errors: <" );
		for (i = 0; i < ctx->errorInfo.count; ++i)
		{
			strcat(buffer, ctx->errorInfo.text[i]);
			strcat(buffer, ",");
		}
		strcat(buffer, ">");
		MessageBox(NULL, buffer, "State Restoration", MB_OK);
		SetCurrentDirectory(curDir);
		return 1;
	}
	else
	{
		if (ctx->window.start_x != before.left || ctx->window.start_y != before.top ||
			ctx->window.width != w || ctx->window.height != h)
		{
			ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height);
			MoveWindow(ctx->mainWindow, ctx->window.start_x, ctx->window.start_y, ctx->window.width + WINDOW_SIZE_OFFSET_WIDTH, ctx->window.height + WINDOW_SIZE_OFFSET_HEIGHT, 1); // fix size & position
		}
		else if (stereoFlag && !ctx->drawAdj.stereoFlag )
			ds_reshape(ctx->mainWindow, ctx->window.width, ctx->window.height);

		if (ctx->window.toolsVisible)
		{
			if (!ctx->attrControl)
			{
				ctx->attrControl = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG5), ctx->mainWindow, ds_dlg_attributes);
				ShowWindow(ctx->attrControl, SW_SHOW);
				ds_position_window(ctx, ctx->attrControl, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
			if (!ctx->objDashboard)
			{
				ctx->objDashboard = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), ctx->mainWindow, ds_dlg_object_dashboard);
				ShowWindow(ctx->objDashboard, SW_SHOW);
				ds_position_window(ctx, ctx->objDashboard, 0, 0); // move windows to appropriate spots - bottom or right of the current window 
			}
		}
		InvalidateRect(ctx->mainWindow, 0, 0);
	}
	SetCurrentDirectory(curDir);
	return 0;
}

//-----------------------------------------------------------------------------
int ds_open_file_dialog (HWND hOwnerWnd, DS_CTX *ctx, int type)
//-----------------------------------------------------------------------------
{
	char			pFilter[] = "Geometry Data (*.off,*.spc)\0*.off;*.spc\0OFF Data (*.off)\0*.off;\0SPC Data (*.spc)\0*.spc;\0All Files (*.*)\0*.*\0\0";
	char			cFilter[] = "Color Table Files (*.dsc,*.txt)\0*.dsc\0All Files (*.*)\0*.*\0\0";
	char			sFilter[] = "State Files (*.dss,*.txt)\0*.dss;*.txt\0All Files (*.*)\0*.*\0\0";
	BOOL			bRes;
	OPENFILENAMEA	ofn;

	ctx->filename[0] = 0; // null terminate

	ofn.lStructSize			= sizeof(OPENFILENAMEA);
	ofn.hwndOwner			= hOwnerWnd;
	ofn.hInstance			= 0;
	ofn.lpstrFilter			= !type ? pFilter : (type == 1 ? cFilter : sFilter);;
	ofn.lpstrCustomFilter	= 0;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= ctx->filename;
	ofn.nMaxFile			= _MAX_PATH;
	ofn.lpstrFileTitle		= 0;
	ofn.nMaxFileTitle		= _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir		= ctx->currentDir;
	ofn.lpstrTitle			= 0;
	ofn.Flags				= OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= "off";
	ofn.lCustData			= 0;
	ofn.lpfnHook			= 0;
	ofn.lpTemplateName		= 0;
//	ofn.lpEditInfo			= 0;
//	ofn.lpstrPrompt			= 0;
	ofn.pvReserved			= 0;
	ofn.dwReserved			= 0;
	ofn.FlagsEx				= 0;

//	ofn.lStructSize       = sizeof( OPENFILENAME );
//	ofn.hwndOwner         = hOwnerWnd;
//	ofn.hInstance         = 0; //hInstance;
//	ofn.lpstrFilter       = !type ? pFilter : ( type==1 ? cFilter : sFilter );
//	ofn.lpstrCustomFilter = 0;
//	ofn.nMaxCustFilter    = 0;
//	ofn.nFilterIndex      = 0;
//	ofn.lpstrFile         = ctx->filename;
//	ofn.nMaxFile          = _MAX_PATH;
//	ofn.lpstrFileTitle    = 0;
//	ofn.nMaxFileTitle     = _MAX_FNAME+_MAX_EXT;
//	ofn.lpstrInitialDir	  = ctx->currentDir; // ctx->curWorkingDir;
//	ofn.lpstrTitle        = 0;
//	ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
//	ofn.nFileOffset       = 0;
//	ofn.nFileExtension    = 0;
//	ofn.lpstrDefExt       = "off";
//	ofn.lCustData         = 0L;
//	ofn.lpfnHook          = 0;
//	ofn.lpTemplateName    = 0;

	bRes = GetOpenFileNameA ( &ofn );
	if (!bRes)
		return 0; // no file selected 

	GetCurrentDirectory(512, ctx->currentDir); // update

	if (!type) // geometry file
	{
		DS_FILE		*dsf;

		if (dsf = ds_file_open(ctx, ctx->filename, "r"))
		{
			ds_file_set_window_text(hOwnerWnd, dsf->nameOnly);
			ds_file_type_initialization(ctx, dsf->nameOnly);
			if (!ds_parse_file(ctx, dsf, (DS_GEO_INPUT_OBJECT*)&ctx->defInputObj))
				ds_file_close(ctx, dsf);
			else
			{
				fclose(dsf->fp);// ds_file_close(ctx, dsf);
				dsf->fp = 0;
			}
		}
		else
		{
			char buffer[128];
			sprintf(buffer, "File <%s> failed to open.", ctx->filename);
			MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
		}
	}
	else if (type == 1) // color tables 
	{
		if (ds_ctbl_process_color_table_file( &ctx->cts, ctx->filename))
			MessageBox(NULL, "Color table file read failed.", 0, MB_OK);
		else
			// save file information 
			ds_build_dsf(&ctx->clrTbl, ctx->filename, 0);

	}
	else if (type == 2) // sat tables 
	{
		return ds_process_restore_file(ctx, ctx->filename);
	}
	return 1;
}

//-----------------------------------------------------------------------------
int ds_write_file_dialog (HWND hOwnerWnd, DS_CTX *ctx, int type)
//-----------------------------------------------------------------------------
{
	char			sFilter[] = "State Files (*.dss)\0*.dss\0All Files (*.*)\0*.*\0\0";
	OPENFILENAME	ofn;
	BOOL			bRes;

	ctx->filename[0]		= 0; // null terminate
	ofn.lStructSize			= sizeof(OPENFILENAME);
	ofn.hwndOwner			= hOwnerWnd;
	ofn.hInstance			= 0; //hInstance;
	ofn.lpstrFilter			= sFilter; // !type ? pFilter : (itype == 1 ? cFilter : sFilter);
	ofn.lpstrCustomFilter	= 0;
	ofn.nMaxCustFilter		= 0;
	ofn.nFilterIndex		= 0;
	ofn.lpstrFile			= ctx->filename;
	ofn.nMaxFile			= _MAX_PATH;
	ofn.lpstrFileTitle		= 0;
	ofn.nMaxFileTitle		= _MAX_FNAME + _MAX_EXT;
	ofn.lpstrInitialDir		= ctx->currentDir;
	ofn.lpstrTitle			= 0;
	ofn.Flags				= OFN_HIDEREADONLY | OFN_CREATEPROMPT;
	ofn.nFileOffset			= 0;
	ofn.nFileExtension		= 0;
	ofn.lpstrDefExt			= "off";
	ofn.lCustData			= 0L;
	ofn.lpfnHook			= 0;
	ofn.lpTemplateName		= 0;

	if( bRes = GetSaveFileName(&ofn))
		GetCurrentDirectory(512,ctx->currentDir); // update
	
	return bRes;
}

//-----------------------------------------------------------------------------
int ds_read_file_from_buffer (DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
//	FILE	*fp;
	DS_FILE	*dsf;

	if (dsf = ds_file_open(ctx, ctx->filename, "r"))
	{
		if (ds_parse_file(ctx, dsf))
		{
			fclose(dsf->fp);
			dsf->fp = 0;
		}
		else 
			ds_file_close(ctx, dsf);
	}
	else
	{
		char buffer[128];
		sprintf(buffer, "File <%s> failed to open.", ctx->filename);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int ds_file_drag_and_drop( HWND hWnd, HDROP hdrop )
//-----------------------------------------------------------------------------
{
	char			buffer[256];
	unsigned int	count, index;
	DS_CTX				*ctx;

	ctx = (DS_CTX*)GetWindowLong(hWnd, GWL_USERDATA);

	// determine if a file is available
	if (count = DragQueryFileA(hdrop, 0xFFFFFFFF, (LPSTR)buffer, (UINT)256))
	{
		for (index = 0; index < count; ++index)
		{
			if (index)
				ctx->gobjAddFlag = 1;

			// get the new filename
			DragQueryFile(hdrop, index, (LPSTR)buffer, (UINT)256);

			// reset the menus
			ds_file_initialization(hWnd, buffer);// , 0);
	
			ctx->gobjAddFlag = 0;
		}
	}

	// release system resources 
	DragFinish ( hdrop );
	return 0;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_geo_object_find(DS_CTX *ctx, char *filename)
//-----------------------------------------------------------------------------
{
	DS_GEO_OBJECT	*gobj;

	LL_SetHead(ctx->gobjectq);
	while (gobj = (DS_GEO_OBJECT*)LL_GetNext(ctx->gobjectq))
	{
		if (!strcmp(gobj->filename, filename))
			return gobj;
	}

	return 0;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_geo_object_create(DS_CTX *ctx, DS_FILE *dsf ) //char *filename)
//-----------------------------------------------------------------------------
{
	// initialize a geometric object
	DS_GEO_OBJECT	*gobj, *lgobj;

	gobj = malloc(sizeof(DS_GEO_OBJECT));
	if (!gobj)
		return 0;

	// memory allocated pointers 
	gobj->filename	= 0; 
	gobj->name[0]	= 0; // null terminated empty string
	gobj->v_out		= 0;
	gobj->n_out		= 0;
	gobj->tri		= 0;
	gobj->edge		= 0;
	// assigned data/pointers
	gobj->nTri		= 0;
	gobj->nEdge		= 0;
	gobj->nVtx		= 0;
	gobj->ctT		= 0;
	gobj->ctE		= 0;
	gobj->ctB		= 0;
	gobj->vIndex    = 0;

	// inherit default settings
	gobj->active			= ctx->defInputObj.active;
	gobj->drawWhat			= ctx->defInputObj.drawWhat;
	gobj->fAttr				= ctx->defInputObj.fAttr;
	gobj->eAttr				= ctx->defInputObj.eAttr;
	gobj->cAttr				= ctx->defInputObj.cAttr;
	gobj->vAttr				= ctx->defInputObj.vAttr;
	gobj->rAttr				= ctx->defInputObj.rAttr; 
	gobj->tAttr				= ctx->defInputObj.tAttr;
	gobj->lFlags			= ctx->defInputObj.lFlags;
	gobj->geo_type			= ctx->defInputObj.geo_type;
	gobj->geo_orientation	= ctx->defInputObj.geo_orientation;
	gobj->faceDefault		= ctx->defInputObj.faceDefault;

	gobj->filename			= malloc(strlen(dsf->nameOnly) + 1);
	strcpy(gobj->filename, dsf->nameOnly);
	gobj->dsf				= dsf;
	gobj->faceMem			= 0;
	gobj->edgeMem			= 0;
	gobj->faceInit			= 0;
	gobj->edgeInit			= 0;
	gobj->attrDialog		= 0;

	if (!ctx->gobjAddFlag) // remove all the previous geometry
	{
		ctx->eAttr.maxLength = 0.0;
		ctx->eAttr.minLength = 1000000000.0;

		while (lgobj = (DS_GEO_OBJECT*)LL_RemoveHead(ctx->gobjectq))
		{	// free up allocated memory
			lgobj->filename ? free(lgobj->filename) : 0;
			lgobj->v_out	? free(lgobj->v_out)	: 0;
			lgobj->n_out	? free(lgobj->n_out)	: 0;
			lgobj->tri		? free(lgobj->tri)		: 0;
			lgobj->edge		? free(lgobj->edge)		: 0;
			lgobj->vIndex   ? free(lgobj->vIndex)   : 0;
			lgobj->faceMem  ? free(lgobj->faceMem)	: 0;
			lgobj->edgeMem  ? free(lgobj->edgeMem)	: 0;
			if (lgobj->attrDialog)
			{
				EndDialog(lgobj->attrDialog, 0);
				DestroyWindow(lgobj->attrDialog);
			}
			lgobj->attrDialog = 0;
			ds_file_close(ctx, lgobj->dsf);
			lgobj			? free ( lgobj )		: 0; // free self
		}
	}

	LL_AddTail(ctx->gobjectq, gobj); // add to queue

	return gobj;
}

//-----------------------------------------------------------------------------
DS_GEO_OBJECT *ds_parse_file(DS_CTX *ctx, DS_FILE *dsf, DS_GEO_INPUT_OBJECT *gio)
//-----------------------------------------------------------------------------
{
	void			*gua;
	DS_GEO_OBJECT	*geo;
	int				resultsFlag = 0;
	int				fileType;
	int				guaFlag = 0;
	enum {
		FILE_OFF,
		FILE_SPC
	};

	// atempt to read OFF format first 
	fileType = gua_off_read(ctx, (void*)&gua, dsf->fp, (float*)&ctx->clrCtl.face.defaultColor);

	switch (fileType) {
	case FILE_OFF:
		guaFlag = 1; // normal GUA data created
		break;
	case FILE_SPC:
		if (ctx->inputTrans.guaFlag)
		{
			guaFlag = 1;// normal GUA data created
			gua_spc_read(ctx, (void*)&gua, dsf->fp, (float*)&ctx->clrCtl.face.defaultColor);
		}
		else
		{	// NGUA data created
			ngua_spc_read(ctx, (void*)&gua, dsf->fp, (float*)&ctx->clrCtl.face.defaultColor);
		}
	}

	if (geo = ds_geo_object_create(ctx, dsf)) // create, initialize, and add to queue
	{
		if (guaFlag && ctx->inputTrans.guaResultsFlag)
		{

			int			i, n = 0;
			char		buffer[512];
			n = strlen(dsf->fullName);
			for (i = n - 1; i >= 0; --i)
				if (dsf->fullName[i] == '.')
				{
					sprintf(buffer, "%.*s.dsu", i, dsf->fullName);
					break;
				}
			if (!i)
				sprintf(buffer, "%s.dsu", dsf->fullName);
			gua_dump(ctx, gua, dsf->nameOnly, buffer);
		}
		if (guaFlag)
			gua_convert_to_object(ctx, gua, geo);
		else
			ngua_convert_to_object(ctx, gua, geo);
	}
	
	// copy attributes
	geo->active				= gio->active;
	geo->drawWhat			= gio->drawWhat;
	geo->fAttr				= gio->fAttr;
	geo->eAttr				= gio->eAttr;
	geo->cAttr				= gio->cAttr;
	geo->vAttr				= gio->vAttr;
	geo->rAttr				= gio->rAttr;
	geo->tAttr				= gio->tAttr;
	geo->lFlags				= gio->lFlags;
	geo->geo_type			= gio->geo_type;
	geo->geo_orientation	= gio->geo_orientation;
	geo->faceDefault		= gio->faceDefault;
	geo->dsf				= dsf;

	// build geometry for drawing
	ds_face_initialize(ctx, geo);
	ds_face_update(ctx, geo);
	ds_edge_initialize(ctx, geo);
	ds_edge_update(ctx, geo);

	// update object dashboard if open
	if (ctx->objDashboard)
	{
		RECT	rect;
		GetWindowRect(ctx->objDashboard, &rect);
		EndDialog(ctx->objDashboard, 0);
		DestroyWindow(ctx->objDashboard);
		ctx->objDashboard = CreateDialog(ctx->hInstance, MAKEINTRESOURCE(IDD_DIALOG8), ctx->mainWindow, ds_dlg_object_dashboard);
		ShowWindow(ctx->objDashboard, SW_SHOW);
		//		MoveWindow(ctx->objDashboard, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top,1);
	}

	return geo;
}
