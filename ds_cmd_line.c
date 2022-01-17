#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glext.h>		/* OpenGL header file */
#include <GL/wglext.h>
#include <GL/glut.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <link.h>
#include <avl_new.h>
#include <geoutil.h>
#include <matrix.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_cmd_line.h"

static char *font_name(void *id);

//----------------------------------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_current_directory_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//----------------------------------------------------------------------------------------------------
{
	char				*p; // c, *p;
	DS_CTX				*ctx = (DS_CTX*)userData;
	int					status = 0;
																//	// directory change - prior to reading any external files
	p = ctx->tempBuffer;

	if (strlen(p))
	{
		ds_build_dsf(&ctx->curDir, p, 0);
		strcpy(ctx->currentDir, ctx->curDir.fullName);

		strcpy(ctx->curWorkingDir, p);
		if (!ctx->dssStateFlag) // don't change current directory during restore
		{
			status = SetCurrentDirectory(ctx->currentDir) ? 0 : 1;
			GetCurrentDirectory(512, ctx->currentDir);
		}
		return status;
	}
	else
		return 1;
}

//-----------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_capture_directory_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	char				*p; // c, *p;
	DS_CTX				*ctx = (DS_CTX*)userData;

	// directory change - prior to reading any external files
	p = ctx->tempBuffer;

	if (strlen(p))
	{
		char	buf1[1024], buf2[1024];
		// Need to convert relative path to absolute path 
		// CurDirectory should be the location of the .dss file 
		// save CurDir
		// set cur directory from provided relative path
		// get cur directory as absolute and save
		// reset curDir back to original
		GetCurrentDirectory(1024, buf1);
		if (SetCurrentDirectory(p))
		{
			GetCurrentDirectory(1024, buf2);
			ds_build_dsf(&ctx->capDir, buf2, 0);
			strcpy(ctx->captureDir, ctx->capDir.fullName);
			SetCurrentDirectory(buf1);
		}
		else
		{
			sprintf(buf2, "Capture Directory <%s> not valid.", p);
			MessageBox(ctx->mainWindow, buf2, "Capture Directory Error", MB_OK);
		}
		return 0;
	}
	else
		return 1;
}

//-----------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_command_line_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	char				*p; 
	DS_CTX				*ctx = (DS_CTX*)userData;

	FILE				*fp;
//	FILE				*fp2;
	int					currentIndex = 0;
	char				buffer[1024];
	char				*array[64];
	int					argCount;
	char				*error;
	int					i, consumedCount;
	int					status;

	// open file
	// read each line
	// discard comments
	// for each real line call parse components and call arg_decode
	//

	p = ctx->tempBuffer; // filename

	fopen_s(&fp, p, "r"); // attempt to open file
	if (!fp)
	{
		char buffer[128];
		sprintf(buffer, "File <%s> failed to open.", p);
		MessageBox(ctx->mainWindow, buffer, "File Open Failure", MB_OK);
		return 1;
	}

//	switch (ctx->stateRecorder.mode) {
//	case DS_STATE_NULL:
//		break;
//	case DS_STATE_PLAYBACK:
//		// move the position of the file read to last position
//		fseek(fp, ctx->stateRecorder.offset, SEEK_SET);
//		break;
//	case DS_STATE_RECORD:
//		break;
//	}

//	fp2 = fopen("c:/TEMP/junk.txt", "w");
	while (fgets(buffer, 1024, fp)) // read each line
	{
		argCount = ds_parse_lexeme(buffer, array, 64); // split line into words

		if (!strncmp(array[0], "#", 1) || !strncmp(array[0], "//", 2)) // simple check for comment
			continue; // comment line

		if (!strcmp(array[0], "DS_STATE")) // File type
		{
			ctx->dssStateFlag = 1; // trap this case so we delay change cur directory to after objects are processed
			continue; // header
		}
		
		if (!strcmp(array[0],"-state_marker"))
		{
			if (argCount > 1)
			{
				strcpy(ctx->stateWrite.description, array[1]);
			}
			// save position of file read
			ctx->stateWrite.offset = ftell (fp);
			// close the file for now
			fclose(fp);
			// retain playback mode
			ctx->stateWrite.mode = DS_STATE_OPEN;
			return 0;
		}
		
		if (!strcmp(array[0], "-command_line") || !strcmp(array[0], "-cl")) // simple check to avoid recursive condition
			continue;

		currentIndex = 0;
		// process one argument
		if(status = _ds_cmd_line_process(ctx->cmdLineCtx, argCount, array, &consumedCount, &error))
		{
			if (status && error)
			{
				char	buffer[128];
				int		i;
				buffer[0] = 0;
				sprintf(buffer, "An error in the command line was encountered at \"%s\"", error);
				if (status == 2)
					strcat(buffer, " :: argument problem");

				MessageBox(NULL, buffer, "Command Line Error", MB_OK);
			}
			fclose(fp);
			return 1;
		}
	}
	fclose(fp);
	
	// return mode back to neutral
	ctx->stateWrite.mode = DS_STATE_NULL;

	return 0;
}

//-----------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_image_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	DS_CTX				*ctx;
	ctx->png.enabled = 1;
	return 0;
}

//-----------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_help_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	DS_CTX			*ctx = (DS_CTX*)userData;
	int				i, len;
	DWORD			nBytes;
	char			buffer[1024];

	AllocConsole();
	ctx->handle_out = GetStdHandle(STD_OUTPUT_HANDLE);
	ctx->handle_in = GetStdHandle(STD_INPUT_HANDLE);

	_ds_cmd_line_output(cmdLineCtx);

	len = sprintf_s(buffer, sizeof(buffer), "\nHit return to exit or c to continue: ");
	WriteFile(ctx->handle_out, buffer, len, NULL, NULL);
	{

		char buffer[256];
		ReadFile(ctx->handle_in, buffer, 1, &nBytes, 0);
		if (buffer[0] != 'c')
			exit(0);
		FreeConsole();
	}

	return 0;
}

//-----------------------------------------------------------------------------
_DS_CMD_LINE_FUNCTION ds_object_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	DS_CTX				*ctx = (DS_CTX*)userData;

	if (LL_GetLength(ctx->inputObjq))
	{
		char	*filename;
		// objects already defined so copy attribute data accumulated to the last one created
		DS_GEO_INPUT_OBJECT	*gio = (DS_GEO_INPUT_OBJECT*)LL_GetTail(ctx->inputObjq);

		// save filename pointer since it will be overwritten
		filename = gio->filename;

		// copy attributes from temporary space
		*(DS_GEO_INPUT_OBJECT*)gio = *(DS_GEO_INPUT_OBJECT*)&ctx->curInputObj;

		// restore filename
		gio->filename = filename;
	}
	else
	{
		//	copy curInputObj data to appropriate place if first object 
		*(DS_GEO_INPUT_OBJECT*)&ctx->defInputObj = *(DS_GEO_INPUT_OBJECT*)&ctx->curInputObj;
	}

	// create new input object
	{
		DS_GEO_INPUT_OBJECT	*gio = (DS_GEO_INPUT_OBJECT*)malloc(sizeof(DS_GEO_INPUT_OBJECT)); // allocate a new object to hold attributes

		// add to queue of input objects	
		LL_AddTail(ctx->inputObjq, gio); 

		// copy current default attributes 
		*(DS_GEO_INPUT_OBJECT*)gio = *(DS_GEO_INPUT_OBJECT*)&ctx->defInputObj;

		// reset temp space to match default settings
		*(DS_GEO_INPUT_OBJECT*)&ctx->curInputObj = *(DS_GEO_INPUT_OBJECT*)&ctx->defInputObj;

		// save the defined name
		gio->filename = (char*)malloc(strlen(ctx->tempBuffer) + 1);
		strcpy(gio->filename, ctx->tempBuffer);
	}
	return 0;
}

//-----------------------------------------------------------------------------
int ds_object_final(void *userData)
//-----------------------------------------------------------------------------
{
	DS_CTX				*ctx = (DS_CTX*)userData;

	if (LL_GetLength(ctx->inputObjq))
	{
		char	*filename;
		// objects already defined so copy attribute data accumulated to the last one created
		DS_GEO_INPUT_OBJECT	*gio = (DS_GEO_INPUT_OBJECT*)LL_GetTail(ctx->inputObjq);

		// save filename pointer since it will be overwritten
		filename = gio->filename;

		// copy attributes from temporary space
		*(DS_GEO_INPUT_OBJECT*)gio = *(DS_GEO_INPUT_OBJECT*)&ctx->curInputObj;

		// restore filename
		gio->filename = filename;
	}
	else
	{
		//	copy curInputObj data to appropriate place if first object 
		*(DS_GEO_INPUT_OBJECT*)&ctx->defInputObj = *(DS_GEO_INPUT_OBJECT*)&ctx->curInputObj;
	}
	return 0;
}

//-----------------------------------------------------------------------------
static _DS_CMD_LINE_FUNCTION ds_rotation_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData)
//-----------------------------------------------------------------------------
{
	DS_CTX		*ctx = (DS_CTX*)userData;

	if (ctx->curRotationAngle != 0)
	{
		MTX_MATRIX	mr, mm;

		ctx->matrixFlag = 1;
		mtx_create_rotation_matrix(&mr, ctx->curRotationAxis, DTR(ctx->curRotationAngle));
		mtx_multiply_matrix(&ctx->matrix, &mr, &mm);
		ctx->matrix = mm;
	}

	return 0;
}
/*
//-----------------------------------------------------------------------------
int _ds_cmd_line_dump_option(void *userData, void *option )
//-----------------------------------------------------------------------------
{
	_DS_CMD_LINE_OPTION		*opt = (_DS_CMD_LINE_OPTION*)option;
	_DS_CMD_LINE_ARGUMENT	*arg;
	_DS_ARGUMENT_CHOICE		*choice;
	int						i, count;
	char					buffer[512], *p;
	FILE					*fp = (FILE*)userData;

	p = buffer;

	sprintf(p, "%s ", opt->text);
	p += strlen(p);

	sprintf(p, ":: %s ", opt->description);
	p += strlen(p);
	fprintf(fp, "%s\n", buffer);

	p = buffer;
	sprintf(p, "%s ", opt->text);
	p += strlen(p);

	LL_SetHead(opt->argq);
	while (arg = (_DS_CMD_LINE_ARGUMENT*)LL_GetNext(opt->argq))
	{
		switch (arg->type) {
		case _DS_ARGUMENT_TYPE_DATA:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " ");
				p += strlen(p);
			}
			break;
		case _DS_ARGUMENT_TYPE_CHOICE:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "{");
			p += strlen(p);
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			sprintf(p, "}");
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			break;
		}
	}
	i = 12;
	fprintf(fp, "\t%s\n", buffer);
	LL_SetHead(opt->argq);
	while (arg = (_DS_CMD_LINE_ARGUMENT*)LL_GetNext(opt->argq))
	{
		p = buffer;
		switch (arg->type) {
		case _DS_ARGUMENT_TYPE_DATA:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] = ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " = ");
				p += strlen(p);
			}

			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			switch (arg->dataType) {
			case _DS_DATA_TYPE_NA:
				break;
			case _DS_DATA_TYPE_INT :
				sprintf(p, "%s ", arg->text);
				if (arg->dataCount > 1)
					sprintf(p, "(int)*%d ", arg->dataCount);
				else
					sprintf(p, "(int) ");
				p += strlen(p);
				break;
			case _DS_DATA_TYPE_FLOAT:
			case _DS_DATA_TYPE_DOUBLE:
				if (arg->dataCount > 1)
					sprintf(p, "(real)*%d ", arg->dataCount);
				else
					sprintf(p, "(real) ");
				p += strlen(p);
				break;
			case _DS_DATA_TYPE_STRING_POINTER:
			case _DS_DATA_TYPE_STRING_COPY:
				sprintf(p, "(string) ");
				p += strlen(p);
				break;
			}
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " ");
				p += strlen(p);
			}
			if (arg->clampFlag)
			{
				switch (arg->dataType) {
				case _DS_DATA_TYPE_INT:
					sprintf(p, "|| clamped between ( %d, %d )", *(int*)arg->clamp.minValueAddr, *(int*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				case _DS_DATA_TYPE_FLOAT:
					sprintf(p, "|| clamped between ( %.3f, %.3f )", *(float*)arg->clamp.minValueAddr, *(float*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				case _DS_DATA_TYPE_DOUBLE:
					sprintf(p, "|| clamped between ( %.3f, %.3f )", *(double*)arg->clamp.minValueAddr, *(double*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				}
			}
			break;
		case _DS_ARGUMENT_TYPE_CHOICE:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] = ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " = ");
				p += strlen(p);
			}

			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "{");
			p += strlen(p);
			count = 0;
			LL_SetHead(arg->choiceq);
			while (choice = (_DS_ARGUMENT_CHOICE*)LL_GetNext(arg->choiceq))
			{
				if(!count)
					sprintf(p, "%s", choice->matchText);
				else 
					sprintf(p, ",%s", choice->matchText);
				p += strlen(p);
				++count;
			}
			sprintf(p, "}");
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			break;
		}
		fprintf(fp, "\t\t%s\n", buffer);
	}
	fprintf(fp, "\n");
	return 0;
}
*/

//-----------------------------------------------------------------------------
int _ds_cmd_line_output_option(void *userData, void *option)
//-----------------------------------------------------------------------------
{
	_DS_CMD_LINE_OPTION		*opt = (_DS_CMD_LINE_OPTION*)option;
	_DS_CMD_LINE_ARGUMENT	*arg;
	_DS_ARGUMENT_CHOICE		*choice;
	int						i, count;
	char					buffer[512], outputBuffer[512], *p;
	FILE					*fp = (FILE*)userData;
	DS_CTX					*ctx = (DS_CTX*)userData;

	p = buffer;

	sprintf(p, "%s ", opt->text);
	p += strlen(p);

	sprintf(p, ":: %s ", opt->description);
	p += strlen(p);
//	fprintf(fp, "%s\n", buffer);
	sprintf(outputBuffer, "%s\n", buffer);
	WriteFile(ctx->handle_out, outputBuffer, strlen(outputBuffer), NULL, NULL);

	p = buffer;
	sprintf(p, "%s ", opt->text);
	p += strlen(p);

	LL_SetHead(opt->argq);
	while (arg = (_DS_CMD_LINE_ARGUMENT*)LL_GetNext(opt->argq))
	{
		switch (arg->type) {
		case _DS_ARGUMENT_TYPE_DATA:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " ");
				p += strlen(p);
			}
			break;
		case _DS_ARGUMENT_TYPE_CHOICE:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "{");
			p += strlen(p);
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			sprintf(p, "}");
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			break;
		}
	}
//	fprintf(fp, "\t%s\n", buffer);
	sprintf(outputBuffer, "\t%s\n", buffer);
	WriteFile(ctx->handle_out, outputBuffer, strlen(outputBuffer), NULL, NULL);

	LL_SetHead(opt->argq);
	while (arg = (_DS_CMD_LINE_ARGUMENT*)LL_GetNext(opt->argq))
	{
		p = buffer;
		switch (arg->type) {
		case _DS_ARGUMENT_TYPE_DATA:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] = ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " = ");
				p += strlen(p);
			}

			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			switch (arg->dataType) {
			case _DS_DATA_TYPE_NA:
				break;
			case _DS_DATA_TYPE_INT:
				sprintf(p, "%s ", arg->text);
				if (arg->dataCount > 1)
					sprintf(p, "(int)*%d ", arg->dataCount);
				else
					sprintf(p, "(int) ");
				p += strlen(p);
				break;
			case _DS_DATA_TYPE_FLOAT:
			case _DS_DATA_TYPE_DOUBLE:
				if (arg->dataCount > 1)
					sprintf(p, "(real)*%d ", arg->dataCount);
				else
					sprintf(p, "(real) ");
				p += strlen(p);
				break;
			case _DS_DATA_TYPE_STRING_POINTER:
			case _DS_DATA_TYPE_STRING_COPY:
				sprintf(p, "(string) ");
				p += strlen(p);
				break;
			}
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " ");
				p += strlen(p);
			}
			if (arg->clampFlag)
			{
				switch (arg->dataType) {
				case _DS_DATA_TYPE_INT:
					sprintf(p, "|| clamped between ( %d, %d )", *(int*)arg->clamp.minValueAddr, *(int*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				case _DS_DATA_TYPE_FLOAT:
					sprintf(p, "|| clamped between ( %.3f, %.3f )", *(float*)arg->clamp.minValueAddr, *(float*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				case _DS_DATA_TYPE_DOUBLE:
					sprintf(p, "|| clamped between ( %.3f, %.3f )", *(double*)arg->clamp.minValueAddr, *(double*)arg->clamp.maxValueAddr);
					p += strlen(p);
					break;
				}
			}
			break;
		case _DS_ARGUMENT_TYPE_CHOICE:
			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "%s", arg->text);
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] = ");
				p += strlen(p);
			}
			else
			{
				sprintf(p, " = ");
				p += strlen(p);
			}

			if (arg->optionFlag)
			{
				sprintf(p, "[");
				p += strlen(p);
			}
			sprintf(p, "{");
			p += strlen(p);
			count = 0;
			LL_SetHead(arg->choiceq);
			while (choice = (_DS_ARGUMENT_CHOICE*)LL_GetNext(arg->choiceq))
			{
				if (!count)
					sprintf(p, "%s", choice->matchText);
				else
					sprintf(p, ",%s", choice->matchText);
				p += strlen(p);
				++count;
			}
			sprintf(p, "}");
			p += strlen(p);
			if (arg->optionFlag)
			{
				sprintf(p, "] ");
				p += strlen(p);
			}
			break;
		}
//		fprintf(fp, "\t\t%s\n", buffer);
		sprintf(outputBuffer, "\t\t%s\n", buffer);
		WriteFile(ctx->handle_out, outputBuffer, strlen(outputBuffer), NULL, NULL);
	}
//	fprintf(fp, "\n");
	sprintf(outputBuffer, "\n");
	WriteFile(ctx->handle_out, outputBuffer, strlen(outputBuffer), NULL, NULL);
	return 0;
}

//-----------------------------------------------------------------------------
int _ds_cmd_line_output(_DS_CMD_LINE_CONTEXT *ctx)
//-----------------------------------------------------------------------------
{
	DS_CTX	*ds = (DS_CTX*)ctx->userData;
	int		len;
	char	buffer[512];

	len = sprintf(buffer, "DisplaySphere - Version %d.%d\n", ds->version.major, ds->version.minor);					WriteFile(ds->handle_out, buffer, len, NULL, NULL);

	len = sprintf(buffer, "Command Line Options\n\n", ds->version.major, ds->version.minor);						WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "OPTION DISPLAY FORMAT\n");																WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "'-optionName' :: (category) description\n");												WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "\toptionName' 'argument names'\n");														WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "\t\t'argument name' = associated data type\n\n");										WriteFile(ds->handle_out, buffer, len, NULL, NULL);

	len = sprintf(buffer, "ARGUMENT NOTATION:\n");																	WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "[ optional argument ]   :: Not required\n");												WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "( data type )           :: integer, real, or string\n");									WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "( data type )*count     :: multiple values\n");											WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "{choiceA,choiceB,...}   :: one of the choice values must match (case insensitive)\n");	WriteFile(ds->handle_out, buffer, len, NULL, NULL);
	len = sprintf(buffer, "\n======================== Command Line Options =======================\n\n");			WriteFile(ds->handle_out, buffer, len, NULL, NULL);

	avl_traverse_ltr(ctx->optionAVL, ds, _ds_cmd_line_output_option);
}

//----------------------------------------------------------------------------------------------------
int ds_build_command_line(DS_CTX *ctx)
//----------------------------------------------------------------------------------------------------
{
	void					*cmdLineCtx = 0;
	static char				*cn[] = { "on","1","yes","enable","off","0","no","disable" };
	static int				cv[2] = { 0,1 };
	static int				font[] = { GLUT_BITMAP_HELVETICA_10, GLUT_BITMAP_HELVETICA_12, GLUT_BITMAP_HELVETICA_18, GLUT_BITMAP_TIMES_ROMAN_10, GLUT_BITMAP_TIMES_ROMAN_24 };
	static char				*fname[] = { "H10","H12","H18","T10","T24" };
	static int				enable = 1, disable = 0;
	static char				*ename[] = { "cylindrical", "round", "square", "box" };
	static int				etype[] = { GEOMETRY_EDGE_ROUND, GEOMETRY_EDGE_SQUARE };
	static int				geotype[] = { GEOMETRY_ICOSAHEDRON, GEOMETRY_OCTAHEDRON, GEOMETRY_TETRAHEDRON, GEOMETRY_CUBEHEDRON, GEOMETRY_DODECAHEDRON};
	static int				gOrient[] = { GEOMETRY_ORIENTATION_FACE, GEOMETRY_ORIENTATION_EDGE, GEOMETRY_ORIENTATION_VERTEX };
	static int				fexd[] = { 0, 1, 2, 3 };
	static int				gl[] = { GEOMETRY_POLYMODE_FILL, GEOMETRY_POLYMODE_LINE, GEOMETRY_POLYMODE_POINT };
	static int				projection[2] = { GEOMETRY_PROJECTION_PERSPECTIVE , GEOMETRY_PROJECTION_ORTHOGRAPHIC };
	static int				sppminmax[2] = { 1, 32 };
	static int				gcValue[] = { ORTHODROME_STYLE_RIM, ORTHODROME_STYLE_DISC_TWO_SIDED, ORTHODROME_STYLE_DISC_ONE_SIDED, ORTHODROME_STYLE_SPHERICAL_SECTION };
	static char				*gcName[] = { "rim","disc_2_sided","disc_1_sided","spherical_sections" };
	_DS_CMD_LINE_OPTION		*opt;
	_DS_CMD_LINE_ARGUMENT	*arg;
	static float			minmax[2] = { 0.0,1.0 };
	static double			dminmax[2] = { 0.0,1.0 };
	static double			minmaxhole[2] = { 0.1,0.95 };
	static double			minmaxdimple[2] = { 0.3,1.5 };
	static double			matShine[2] = { 0, 128.0 };
	static int				use[3] = { DS_COLOR_STATE_EXPLICIT, DS_COLOR_STATE_AUTOMATIC, DS_COLOR_STATE_OVERRIDE };
	static MTX_MATRIX		matrix;
	static int				rotAxis[] = { MTX_ROTATE_X_AXIS, MTX_ROTATE_Y_AXIS, MTX_ROTATE_Z_AXIS };
//	static char				cdBuffer[512];

	mtx_set_unity(&matrix);

	// start by creating context
	ctx->cmdLineCtx = cmdLineCtx = _ds_create_cmd_line_context((void*)ctx);

	// add options and associated arguments 

	// axis 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-axis", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.axiiFlag, 0, 0); // requires choice
	opt->description = "(global) Control visibility of the reference xyz axii";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.axiiFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// axis label																															// axis 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-axis_label", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.axiiLabelFlag, 0, 0); // requires choice
	opt->description = "(global) Control visibility of reference axii labels";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.axiiLabelFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);
	_DS_CMD_LINE_ARGUMENT *_ds_add_option_choice_argument(_DS_CMD_LINE_OPTION *option, int dataSize, void *dstAddr, int optionFlag, ...); // pairs of ("text", void *srcData);

	// background color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-background_color", 0, 0, 0, 0, 0);
	opt->description = "(global) Modify background color of the rendering window";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->clrCtl.bkgClear.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp
	arg = _ds_add_option_number_argument(opt, "a", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->clrCtl.bkgClear.a, sizeof(float), 1, 1, (void*)&minmax[0], (void*)&minmax[1]); // optional

	// capture directory
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-capture_directory", 0, 0, 0, 0, ds_capture_directory_option);
	opt->description = "(capture) Modify the film/image capture destination directory";
	arg = _ds_add_option_string_argument(opt, "path/", _DS_DATA_TYPE_STRING_COPY, (void*)ctx->tempBuffer, sizeof(ctx->tempBuffer), 0);

	// current working directory NEEDS POST FUNCTION
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-cd", 0, 0, 0, 0, ds_current_directory_option);
	opt->description = "(global) Immediately change the current working directory. Affects all object files that don't have a full path.";
	arg = _ds_add_option_string_argument(opt, "path/", _DS_DATA_TYPE_STRING_COPY, (void*)ctx->tempBuffer, sizeof(ctx->tempBuffer), 0);

	// circle mode 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-circle", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.circleFlag, 0, 0); // requires choice
	opt->description = "(global) Set the state of the circle drawing mode.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.circleFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// clip mode 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-clip", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.clipFlag, 0, 0); // requires choice
	opt->description = "(global) Set the state of the clipping plane.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.clipFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// clip z position and increment 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-clip_z_increment", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.clipZValue, 0, 0); // requires choice
	opt->description = "(global) Set the z position of the clip plan and increment.";
	arg = _ds_add_option_number_argument(opt, "z_position increment", _DS_DATA_TYPE_DOUBLE, 2, (void*)&ctx->drawAdj.clipZValue, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// color table
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-color_table", 0, 0, 0, 0, 0);
	opt->description = "(global) Load a color table file (.dsc)";
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)ctx->clrCtl.user_color_table, sizeof(ctx->clrCtl.user_color_table), 0);

	// command line NEEDS POST FUNCTION
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-command_line", 0, 0, 0, 0, 0);
	opt->description = "(global) Load a command line or state file (.dss)";
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)ctx->tempBuffer, sizeof(ctx->tempBuffer), 0);

	// edge color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_override_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the override edge color.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.cAttr.edge.color.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// edge color to use
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_color_use", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set which color to use when drawing the edge";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->curInputObj.cAttr.edge.state, 0,
		"automatic", (void*)&use[1], "override", (void*)&use[2], 0);// pairs of ("text", void *srcData);

	// edge draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_draw", sizeof(int), (void*)&enable, (void*)&ctx->curInputObj.eAttr.draw, 0, 0); // requires choice
	opt->description = "(object) Set the draw state for edges (defaults to enable)";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.eAttr.draw, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// edge label draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_label", sizeof(int), (void*)&enable, (void*)&ctx->curInputObj.eAttr.label.enable, 0, 0); // requires choice
	opt->description = "(object) Set the draw state for edge labels (defaults to enable if not specified)";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.eAttr.label.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// edge lable color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_label_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the color of the edge label text";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.eAttr.label.color.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// edge label font 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_label_font", sizeof(int), (void*)&font[0], (void*)&ctx->curInputObj.eAttr.label.font, 0, 0); // requires choice
	opt->description = "(object) Specify the font used for an object's edge label";
	arg = _ds_add_option_choice_argument(opt, "name", sizeof(int), (void*)&ctx->curInputObj.eAttr.label.font, 1,
		fname[0], &font[0], fname[1], &font[1], fname[2], &font[2], fname[3], &font[3], fname[4], &font[4], 0); // pairs of ("text", void *srcData);

//	// edge offset value
//	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_offset", 0, 0, 0, 0, 0);
//	opt->description = "(object) Set the amount of offset to be applied to box/square edges";
//	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.eAttr.offset, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// edge offset enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_offset", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.eAttr.offset.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face's offset state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.eAttr.offset.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// edge offset size factor
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_offset_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the edge's offset value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.eAttr.offset.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// edge scale enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_scale", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.eAttr.scale.enable, 0, 0); // requires choice
	opt->description = "(object) Set the edge's scale state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.eAttr.scale.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// edge scale size factor
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_scale_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the edge's scale value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.eAttr.scale.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// edge type 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_type", sizeof(int), &etype[0], (void*)&ctx->curInputObj.eAttr.type, 0, 0); // requires choice
	opt->description = "Specify the type of edge to draw.";
	arg = _ds_add_option_choice_argument(opt, "style", sizeof(int), (void*)&ctx->curInputObj.eAttr.type, 1,
		ename[0], &etype[0], ename[1], &etype[0], ename[2], &etype[1], ename[3], &etype[1], 0); // pairs of ("text", void *srcData);

	// edge arc enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_arc", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.eAttr.arcEnable, 0, 0); // requires choice
	opt->description = "(object) Set the edge's arc state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.eAttr.arcEnable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// edge width & height values
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-edge_width_height", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the width and height of the edge. Only width is used for cylindrical edges.";
	arg = _ds_add_option_number_argument(opt, "w h", _DS_DATA_TYPE_DOUBLE, 2, (void*)&ctx->curInputObj.eAttr.width, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face default color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_default_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face color to be used when object file does not specify. Can include alpha value.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.faceDefault.a, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp
	arg = _ds_add_option_number_argument(opt, "a", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->curInputObj.faceDefault.a, sizeof(float), 1, 1, (void*)&minmax[0], (void*)&minmax[1]); // optional

	// face override color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_override_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face color to be used when object file does not specify. Can include alpha value.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.cAttr.face.color, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp
	arg = _ds_add_option_number_argument(opt, "a", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->curInputObj.cAttr.face.color.a, sizeof(float), 1, 1, (void*)&minmax[0], (void*)&minmax[1]); // optional

	// face color to use
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_color_use", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set which face color to use when drawing.";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->curInputObj.cAttr.face.state, 0,
		"explicit", (void*)&use[0], "automatic", (void*)&use[1], "override", (void*)&use[2], 0);// pairs of ("text", void *srcData);

	// face draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_draw", sizeof(int), (void*)&enable, (void*)&ctx->curInputObj.fAttr.draw, 0, 0); // requires choice
	opt->description = "(object) Set face's draw state (defaults to visible).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.draw, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable,  cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face extrude 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_extrude", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.extrusion.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face extrusion state (defaults to disabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.extrusion.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face extrude both sides
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_extrude_2sides", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.extrusion.bothSides, 0, 0); // requires choice
	opt->description = "(object) Set the face extrusion both sides option (defaults to disabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.extrusion.bothSides, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face extrusion direction
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_extrude_direction", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the face extrusion direction.";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->curInputObj.fAttr.extrusion.direction, 0,
		"normal", (void*)&fexd[0], "radial", (void*)&fexd[1], 0);// pairs of ("text", void *srcData);

	// face extrude height value
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_extrude_height", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face extrusion height factor.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.extrusion.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face extrude hole only
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_extrude_hole_only", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.extrusion.holeOnly, 0, 0); // requires choice
	opt->description = "(object) Set the option to only extrude the face hole (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.extrusion.holeOnly, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face hole radius value
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_hole_radius", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face hole radius.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.hole.radius, sizeof(double), 0, 1, (void*)&minmaxhole[0], (void*)&minmaxhole[1]); // NEEED NEW  clamp values

	// face hole dimple height value
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_hole_dimple_height", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face hole dimple height.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.hole.shallowness, sizeof(double), 0, 1, (void*)&minmaxdimple[0], (void*)&minmaxdimple[1]); // NEEED NEW  clamp values

	// face hole enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_hole", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.hole.enable, 0, 0); // requires choice
	opt->description = "(object) Set the state of the face hole (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.hole.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face hole type
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_hole_type", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the face hole type.";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->curInputObj.fAttr.hole.style, 0, 
		"round", (void*)&fexd[0], "polygonal", (void*)&fexd[1], "round_in_dimple", (void*)&fexd[2], "round_out_dimple", (void*)&fexd[3], 0);// pairs of ("text", void *srcData);

	// face label draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_label", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.label.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face's label visbility state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.label.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face label color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_label_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's label color.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.fAttr.label.color.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// face label font 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_label_font", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the face's label font.";
	arg = _ds_add_option_choice_argument(opt, "name", sizeof(int), (void*)&ctx->curInputObj.fAttr.label.font, 1,
		fname[0], &font[0], fname[1], &font[1], fname[2], &font[2], fname[3], &font[3], fname[4], &font[4], 0); // pairs of ("text", void *srcData);

	// face offset enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_offset", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.offset.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face's offset state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.offset.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face offset size factor
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_offset_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's offset value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.offset.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face scale enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_scale", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.fAttr.scale.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face's scale state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.scale.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face scale size factor
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_scale_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's scale value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.scale.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face transparency enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_transparency", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.tAttr.onFlag, 0, 0); // requires choice
	opt->description = "(object) Set the face's transparency state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.tAttr.onFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face tranparency override alpha
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_transparency_alpha", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's transparency alpha value to use when override is enabled.";
	arg = _ds_add_option_number_argument(opt, "a", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->curInputObj.tAttr.alpha, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// face transparency override enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_transparency_override", sizeof(int), (void*)&use[2], (void*)&ctx->curInputObj.tAttr.state, 0, 0); // requires choice
	opt->description = "(object) Set the face's transparency override state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.tAttr.state, 1,
		cn[0], &use[2], cn[1], &use[2], cn[2], &use[2], cn[3], &use[2], cn[4], &use[0], cn[5], &use[0], cn[6], &use[0], cn[7], &use[0], 0); // pairs of ("text", void *srcData);

	// face great circles & spherical sections enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circles", sizeof(int), (void*)&use[2], (void*)&ctx->curInputObj.fAttr.orthodrome.enable, 0, 0); // requires choice
	opt->description = "(object) Set the face's great circle state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.orthodrome.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face great circles & spherical sections type
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_type", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the face's great circle type.";
	arg = _ds_add_option_choice_argument(opt, "name", sizeof(int), (void*)&ctx->curInputObj.fAttr.orthodrome.style, 0,
		gcName[0], &gcValue[0],
		gcName[1], &gcValue[1],
		gcName[2], &gcValue[2],
		gcName[3], &gcValue[3], 0); // pairs of ("text", void *srcData);

	// face great circles & spherical sections rim dash enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_dashed", sizeof(int), (void*)&use[2], (void*)&ctx->curInputObj.fAttr.orthodrome.dashEnable, 0, 0); // requires choice
	opt->description = "(object) Set the face's great circle dash state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.orthodrome.dashEnable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face great circles & spherical sections rim depth 1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_rim_depth1", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's great circle rim depth1 value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.orthodrome.depth1, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face great circles & spherical sections rim depth 2
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_rim_depth2", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's great circle rim depth2 value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.orthodrome.depth2, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face great circles & spherical sections rim  height
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_rim_height", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's great circle rim height value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.fAttr.orthodrome.height, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// face great circles & spherical sections seperate cut color enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_cut_color", sizeof(int), (void*)&use[2], (void*)&ctx->curInputObj.fAttr.orthodrome.cutColorEnable, 0, 0); // requires choice
	opt->description = "(object) Set the face's great circle cut color state (defaults to enabled).";
//	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.orthodrome.cutColorEnable, 1,
//		cn[0], &use[2], cn[1], &use[2], cn[2], &use[2], cn[3], &use[2], cn[4], &use[0], cn[5], &use[0], cn[6], &use[0], cn[7], &use[0], 0); // pairs of ("text", void *srcData);
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.fAttr.orthodrome.cutColorEnable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// face great circles & spherical sections seperate cut color 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-face_great_circle_cut_color_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the face's great circle cut color.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.fAttr.orthodrome.cutColor.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// global lighting enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-lighting", sizeof(int), (void*)&cv[1], (void*)&ctx->lighting.useLightingFlag, 0, 0); // requires choice
	opt->description = "(global) Set the global lighting state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->lighting.useLightingFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

//	-light_ambient 0/1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_ambient", sizeof(int), (void*)&cv[1], (void*)&ctx->lighting.ambientEnabled, 0, 0); // requires choice
	opt->description = "(global) Set the global ambient lighting state (default is enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->lighting.ambientEnabled, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);
																																				//	-light_ambient_value 0-1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_ambient_value", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the global ambient lighting value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->lighting.ambientPercent, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

//	-light_diffuse 0/1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_diffuse", sizeof(int), (void*)&cv[1], (void*)&ctx->lighting.diffuseEnabled, 0, 0); // requires choice
	opt->description = "(global) Set the global diffuse lighting state (default is enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->lighting.diffuseEnabled, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);
																																				//	-light_diffuse_value 0-1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_diffuse_value", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the global diffuse lighting value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->lighting.diffusePercent, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp
																																										 //	-light_diffuse_value 0-1
//	-light_specular 0/1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_specular", sizeof(int), (void*)&cv[1], (void*)&ctx->lighting.specularEnabled, 0, 0); // requires choice
	opt->description = "(global) Set the global specular lighting state (default is enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->lighting.specularEnabled, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);
//	-light_specular_value 0-1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_specular_value", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the global specular lighting value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->lighting.specularPercent, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

//	-light_material_specular 0-1
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_material_specular", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the global material specular value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->lighting.matSpecular, sizeof(float), 0, 1, (void*)&dminmax[0], (void*)&dminmax[1]); // clamp
//	-light_material_shininess 0-128
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_material_shininess", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the global material shininess value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->lighting.matShininess, sizeof(float), 0, 1, (void*)&matShine[0], (void*)&matShine[1]); // clamp

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-film", 0, 0, 0, 0, ds_image_option); 
	opt->description = "(capture) Set the film base filename and number of frames to capture.";
//	arg = _ds_add_option_number_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, 1, (void*)&ctx->png.basename, sizeof(ctx->png.basename), 0, 0, 0, 0);
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)&ctx->png.basename, sizeof(ctx->png.basename), 0);
	arg = _ds_add_option_number_argument(opt, "#frames", _DS_DATA_TYPE_INT, 1, (void*)&ctx->png.nFrames, sizeof(int), 0, 0, 0, 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-image_capture", sizeof(int), (void*)&cv[1], (void*)&ctx->png.nFrames, 0, ds_image_option);
	opt->description = "(capture) Set the image capture filename.";
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)&ctx->png.basename, sizeof(ctx->png.basename), 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-image_basename", 0, 0, 0, 0, 0);
	opt->description = "(capture) Set the image capture base filename.";
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)&ctx->png.basename, sizeof(ctx->png.basename), 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-geometry", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set which geometry to apply to the current object.";
	arg = _ds_add_option_choice_argument(opt, "polyhedron", sizeof(int), (void*)&ctx->curInputObj.geo_type, 0,
		"Icosahedron", (void*)&geotype[0], "Octahedron", (void*)&geotype[1], "Tetrahedron", (void*)&geotype[2], "Cube", (void*)&geotype[3], "Dodecahedron", (void*)&geotype[4], 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-gl_back", sizeof(int), (void*)&gl[0], (void*)&ctx->geomAdj.polymode[1], 0, 0); // requires choice
	opt->description = "(global) Set the OpenGL back face draw state.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->geomAdj.polymode[1], 0,
		"fill", (void*)&gl[0], "line", (void*)&gl[1], "point", (void*)&gl[2], 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-gl_front", sizeof(int), (void*)&gl[0], (void*)&ctx->geomAdj.polymode[0], 0, 0); // requires choice
	opt->description = "(global) Set the OpenGL front face draw state.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->geomAdj.polymode[0], 0,
		"fill", (void*)&gl[0], "line", (void*)&gl[1], "point", (void*)&gl[2], 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-gl_back_cull", sizeof(int), (void*)&cv[1], (void*)&ctx->geomAdj.cull[1], 0, 0); // requires choice
	opt->description = "(global) Set the OpenGL back face cull state.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->geomAdj.cull[1], 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-gl_front_cull", sizeof(int), (void*)&cv[1], (void*)&ctx->geomAdj.cull[0], 0, 0); // requires choice
	opt->description = "(global) Set the OpenGL front face cull state.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->geomAdj.cull[0], 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-help", 0, 0, 0, 0, ds_help_option); // requires choice
	opt->description = "Display all command line options and associated argument details.";

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-high_resolution", sizeof(int), (void*)&enable, (void*)&ctx->drawAdj.hiResFlag, 0, 0); // requires choice
	opt->description = "Set the high resolution state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.hiResFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-input_center_scale", sizeof(int), (void*)&cv[1], (void*)&ctx->inputTrans.centerAndScaleFlag, 0, 0); // requires choice
	opt->description = "(global) Set the center and scale option for input object geometry data (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.centerAndScaleFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-input_transform", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(global) Set the transform option for input object geometry data (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.transformFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-input_transform_axii", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(global) Set the input transform axii values.";
	arg = _ds_add_option_number_argument(opt, "zaxis(x,y,z) yaxis(x,y,z)", _DS_DATA_TYPE_DOUBLE, 6, (void*)&ctx->inputTrans.zAxis, sizeof(double), 0, 0, 0, 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-input_x_mirror", sizeof(int), (void*)&cv[1], (void*)&ctx->inputTrans.mirrorFlag, 0, 0); // requires choice
	opt->description = "(global) Set the x mirroring option state for incoming object geometry (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.mirrorFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-input_z_rotate", sizeof(int), (void*)&cv[1], (void*)&ctx->inputTrans.replicateFlag, 0, 0); // requires choice
	opt->description = "(global) Set the z rotation option state for incoming object geometry (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.replicateFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-light_position", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(global) Set the global light position coordinates.";
	arg = _ds_add_option_number_argument(opt, "x y z", _DS_DATA_TYPE_DOUBLE, 3, (void*)&ctx->lighting.position, sizeof(double), 0, 0, 0, 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-fog", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.fogFlag, 0, 0); // requires choice
	opt->description = "(global) Set the global fog state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.fogFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-image_state_save", sizeof(int), (void*)&cv[1], (void*)&ctx->png.stateSaveFlag, 0, 0); // requires choice
	opt->description = "(capture) Set the image state save option (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->png.stateSaveFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-unique", sizeof(int), (void*)&cv[1], (void*)&ctx->inputTrans.guaFlag, 0, 0); // requires choice
	opt->description = "(global) Set the unique analysis state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.guaFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-normalize", sizeof(int), (void*)&enable, (void*)&ctx->drawAdj.normalizeFlag, 0, 0); // size, src, dst, pre, post
	opt->description = "(global) Set the global normalize state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.normalizeFlag, 1,
		cn[0], (void*)&enable, cn[1], (void*)&enable, cn[2], &enable, cn[3], (void*)&enable, cn[4], (void*)&disable, cn[5], (void*)&disable,  cn[6], (void*)&disable, cn[7], (void*)&disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-object", 0, 0, 0, 0, ds_object_option); // requires choice
	opt->description = "(object) Read the specified object geometry file.";
	arg = _ds_add_option_string_argument(opt, "filename", _DS_DATA_TYPE_STRING_COPY, (void*)ctx->tempBuffer, sizeof(ctx->tempBuffer), 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-orientation", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the geometry orientation.";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->curInputObj.geo_orientation, 1,
		"face", &gOrient[0], "edge", &gOrient[1], "vertex", &gOrient[2], 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-projection", 0, 0, 0, 0, 0); // requires choice
	opt->description = "(global) Set the projection state.";
	arg = _ds_add_option_choice_argument(opt, "which", sizeof(int), (void*)&ctx->drawAdj.projection, 1,
		"perspective", &projection[0], "orthographic", &projection[1], 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-replicate_one_face", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.rAttr.oneFaceFlag, 0, 0); // requires choice
	opt->description = "(object) Set the one face replication state for the current object (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.rAttr.oneFaceFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-replicate_z_rotate", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.rAttr.zRotationFlag, 0, 0); // requires choice
	opt->description = "(object) Set the face z axis rotation replication state for the current object (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.rAttr.zRotationFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-replicate_x_mirror", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.rAttr.xMirrorFlag, 0, 0); // requires choice
	opt->description = "(object) Set the face x axis mirroring replication state for the current object (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.rAttr.xMirrorFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-relative_object_path", sizeof(int), (void*)&cv[1], (void*)&ctx->relativeObjPathFlag, 0, 0); // requires choice
	opt->description = "(global) Set the relative object path state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->relativeObjPathFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// rotation matrix
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-rotate_x_axis", sizeof(int), (void*)&rotAxis[0], (void*)&ctx->curRotationAxis, 0, ds_rotation_option); // need to set matrix to unity first
	opt->description = "(global) Define an initial x axis rotation amount to be applied.";
	arg = _ds_add_option_number_argument(opt, "angle", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curRotationAngle, sizeof(float), 0, 0, 0, 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-rotate_y_axis", sizeof(int), (void*)&rotAxis[1], (void*)&ctx->curRotationAxis, 0, ds_rotation_option); // need to set matrix to unity first
	opt->description = "(global) Define an initial y axis rotation amount to be applied.";
	arg = _ds_add_option_number_argument(opt, "angle", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curRotationAngle, sizeof(float), 0, 0, 0, 0);

	opt = _ds_add_cmd_line_option(cmdLineCtx, "-rotate_z_axis", sizeof(int), (void*)&rotAxis[2], (void*)&ctx->curRotationAxis, 0, ds_rotation_option); // need to set matrix to unity first
	opt->description = "(global) Define an initial z axis rotation amount to be applied.";
	arg = _ds_add_option_number_argument(opt, "angle", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curRotationAngle, sizeof(float), 0, 0, 0, 0);

	// rotation matrix
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-rotation_matrix", sizeof(matrix), (void*)&matrix, (void*)&ctx->matrix.data.row_column[0][0], 0, 0); // need to set matrix to unity first
	opt->description = "(global) Define an initial rotation matrix to be applied.";
	arg = _ds_add_option_number_argument(opt, "row[0][0,1,2]", _DS_DATA_TYPE_DOUBLE, 3, (void*)&ctx->matrix.data.row_column[0][0], sizeof(double), 0, 0, 0, 0);
	arg = _ds_add_option_number_argument(opt, "row[1][0,1,2]", _DS_DATA_TYPE_DOUBLE, 3, (void*)&ctx->matrix.data.row_column[1][0], sizeof(double), 0, 0, 0, 0); 
	arg = _ds_add_option_number_argument(opt, "row[2][0,1,2]", _DS_DATA_TYPE_DOUBLE, 3, (void*)&ctx->matrix.data.row_column[2][0], sizeof(double), 0, 0, 0, 0); 

	// spin
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-spin", sizeof(int), (void*)&enable, (void*)&ctx->drawAdj.spin.spinState, 0, 0);
	opt->description = "(global) Set the spin state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.spin.spinState, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// spin_values
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-spin_values", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the spin parameter values.";
	arg = _ds_add_option_number_argument(opt, "dx", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->drawAdj.spin.dx, sizeof(float), 0, 0, 0, 0); // clamp 1-32
	arg = _ds_add_option_number_argument(opt, "dy", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->drawAdj.spin.dy, sizeof(float), 0, 0, 0, 0); // clamp 1-32
	arg = _ds_add_option_number_argument(opt, "dz", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->drawAdj.spin.dz, sizeof(float), 0, 0, 0, 0); // clamp 1-32
	arg = _ds_add_option_number_argument(opt, "mSec", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->drawAdj.spin.timerMSec, sizeof(float), 0, 0, 0, 0); // clamp 1-32

	// pixel samples
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-samples_per_pixel", 0, 0, 0, 0, 0);
	opt->description = "(global) Specify the number of requested samples per pixel to render (not guaranteed0.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_INT, 1, (void*)&ctx->opengl.samplesPerPixel, sizeof(int), 0, 0, (void*)&sppminmax[0], (void*)&sppminmax[1]); // clamp 1-32

	// stereo enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-stereo", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.stereoFlag, 0, 0); // requires choice
	opt->description = "(global) Set the stereo view state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.stereoFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// stereo no cross
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-stereo_cross_eye", sizeof(int), (void*)&cv[1], (void*)&ctx->drawAdj.stereoCrossEyeFlag, 0, 0); // requires choice
	opt->description = "(global) Set the stereo cross-eye state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->drawAdj.stereoCrossEyeFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// stereo eye separation angle
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-stereo_angle", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the stereo angle between eyes.";
	arg = _ds_add_option_number_argument(opt, "angle", _DS_DATA_TYPE_FLOAT, 1, (void*)&ctx->drawAdj.eyeSeparation, sizeof(float), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// tool window visible
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-tool_windows_visible", sizeof(int), (void*)&cv[1], (void*)&ctx->window.toolsVisible, 0, 0); // requires choice
	opt->description = "(global) Set the tool windows state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->window.toolsVisible, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// Initial translation xyz values
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-translate_xyz", 0, 0, 0, 0, 0);
	opt->description = "(global) Set the intial translation values.";
	arg = _ds_add_option_number_argument(opt, "x y z", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->trans[0], sizeof(float), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// udump
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-unique_dump", sizeof(int), (void*)&cv[1], (void*)&ctx->inputTrans.guaResultsFlag, 0, 0); // requires choice
	opt->description = "(global) Set the udump state (.dsu)(defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->inputTrans.guaResultsFlag, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// object visible
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-visible", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.active, 0, 0); // requires choice
	opt->description = "(object) Set the visibility state of the current object (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.active, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// vertex color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_color", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the vertex color for the current object.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.cAttr.vertex.color.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// vertex draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_draw", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.vAttr.draw, 0, 0); // requires choice
	opt->description = "(object) Set the vertex draw state for the current object (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.vAttr.draw, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// vertex label draw 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_label", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.vAttr.label.enable, 0, 0); // requires choice
	opt->description = "(object) Set the vertex label draw state for the current object.";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.vAttr.label.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// vertex lable color
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_label_color", _DS_DATA_TYPE_NA, 0, 0, 0, 0, 0);
	opt->description = "(object) Set the vertex label color for the current object.";
	arg = _ds_add_option_number_argument(opt, "r g b", _DS_DATA_TYPE_FLOAT, 3, (void*)&ctx->curInputObj.vAttr.label.color.r, sizeof(float), 0, 1, (void*)&minmax[0], (void*)&minmax[1]); // clamp

	// vertex label font 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_label_font", _DS_DATA_TYPE_NA, 0, 0, 0, 0, 0); // requires choice
	opt->description = "(object) Set the vertex label font for the current object.";
	arg = _ds_add_option_choice_argument(opt, "name", sizeof(int), (void*)&ctx->curInputObj.vAttr.label.font, 0,
		fname[0], &font[0], fname[1], &font[1], fname[2], &font[2], fname[3], &font[3], fname[4], &font[4], 0); // pairs of ("text", void *srcData);

	// vertex scale 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_scale", _DS_DATA_TYPE_NA, 0, 0, 0, 0, 0);
	opt->description = "(object) Set the vertex label font for the current object.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.vAttr.scale, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// vertex offset enable
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_offset", sizeof(int), (void*)&cv[1], (void*)&ctx->curInputObj.vAttr.offset.enable, 0, 0); // requires choice
	opt->description = "(object) Set the vertice's offset state (defaults to enabled).";
	arg = _ds_add_option_choice_argument(opt, "state", sizeof(int), (void*)&ctx->curInputObj.vAttr.offset.enable, 1,
		cn[0], &enable, cn[1], &enable, cn[2], &enable, cn[3], &enable, cn[4], &disable, cn[5], &disable, cn[6], &disable, cn[7], &disable, 0); // pairs of ("text", void *srcData);

	// vertex offset size factor
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-vertex_offset_value", 0, 0, 0, 0, 0);
	opt->description = "(object) Set the vertex's offset value.";
	arg = _ds_add_option_number_argument(opt, "value", _DS_DATA_TYPE_DOUBLE, 1, (void*)&ctx->curInputObj.vAttr.offset.factor, sizeof(double), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values
																																														   
	// window width & height 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-window_width_height", _DS_DATA_TYPE_NA, 0, 0, 0, 0, 0);
	opt->description = "(global) Set the initial width and height of the render window.";
	arg = _ds_add_option_number_argument(opt, "width height", _DS_DATA_TYPE_INT, 2, (void*)&ctx->window.width, sizeof(int), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	// window x & y position 
	opt = _ds_add_cmd_line_option(cmdLineCtx, "-window_left_top", _DS_DATA_TYPE_NA, 0, 0, 0, 0, 0);
	opt->description = "(global) Set the initial top left corner of the render window.";
	arg = _ds_add_option_number_argument(opt, "x y", _DS_DATA_TYPE_INT, 2, (void*)&ctx->window.start_x, sizeof(int), 0, 0, (void*)&minmax[0], (void*)&minmax[1]); // NEEED NEW  clamp values

	return 0;
}


//-----------------------------------------------------------------------------
int ds_restore_state(DS_CTX *ctx, char *filename, DS_ERROR *errInfo)
//-----------------------------------------------------------------------------
{
	int					currentArgIndex = 1;
	int					error = 0;
	int					status = 0;
	int					argIndex;

	char	*av[] = { "-command_line", filename };
	if (ctx->stateWrite.mode == DS_STATE_NULL)
	{
		ds_pre_init2(ctx); // reset everything to the default and delete any existing objects
		strcpy(ctx->tempBuffer, filename);
		ds_command_line_option(ctx->cmdLineCtx, 0, (void*)ctx);
		ds_object_final((void*)ctx); // update object geometry based on attributes
		ds_post_init2(ctx); // post command line updates
	}
	else if (ctx->stateWrite.mode == DS_STATE_OPEN)
	{
		ds_pre_init2(ctx); // reset everything to the default and delete any existing objects
		strcpy(ctx->tempBuffer, filename);
		ds_command_line_option(ctx->cmdLineCtx, 0, (void*)ctx);
		ds_object_final((void*)ctx); // update object geometry based on attributes
		ds_post_init2(ctx); // post command line updates
	}
	return error;
}

//-----------------------------------------------------------------------------
int ds_save_object_state(DS_CTX *ctx, DS_FILE *base, FILE *fp, DS_GEO_OBJECT *gobj)
//-----------------------------------------------------------------------------
{
	char	*p;

	if (gobj->filename)
	{
		char	relativeFilename[1024];
		ds_cd_relative_filename(ctx, base, gobj->dsf, relativeFilename);

		fprintf(fp, "# Object settings for <%s>\n", gobj->dsf->nameOnly);
		fprintf(fp, "-object \"%s\"\n", relativeFilename);
	}
	else
	{
		fprintf(fp, "# Default object settings\n");
	}
	if (gobj->filename)
	{
		fprintf(fp, "-visible %s\n", gobj->active ? "enable" : "disable");
	}
	else 
		fprintf(fp, "-visible enable\n");

	// Geometry
	switch (gobj->geo_type) {
	case GEOMETRY_CUBEHEDRON:	p = "cube"; break;
	case GEOMETRY_ICOSAHEDRON:	p = "icosahedron"; break;
	case GEOMETRY_OCTAHEDRON:	p = "octahedron"; break;
	case GEOMETRY_TETRAHEDRON:	p = "tetrahedron"; break;
	case GEOMETRY_DODECAHEDRON:	p = "dodecahedron"; break;
	default:					p = "icosahedron";
	}
	fprintf(fp, "-geometry %s\n", p);

	//- orientation
	switch (gobj->geo_orientation) {
	case GEOMETRY_ORIENTATION_EDGE:		p = "edge"; break;
	case GEOMETRY_ORIENTATION_FACE:		p = "face"; break;
	case GEOMETRY_ORIENTATION_VERTEX:	p = "vertex"; break;
	default:							p = "face";
	}
	fprintf(fp, "-orientation %s\n", p);
	fprintf(fp, "-replicate_one_face %s\n", gobj->rAttr.oneFaceFlag ? "enable" : "disable");	
	fprintf(fp, "-replicate_z_rotate %s\n", gobj->rAttr.zRotationFlag ? "enable" : "disable");	
	fprintf(fp, "-replicate_x_mirror %s\n", gobj->rAttr.xMirrorFlag ? "enable" : "disable");	

	// Faces
	fprintf(fp, "-face_draw %s\n", gobj->fAttr.draw ? "enable" : "disable");

	switch (gobj->cAttr.face.state) {
	case DS_COLOR_STATE_AUTOMATIC: fprintf(fp, "-face_color_use automatic\n"); break;
	case DS_COLOR_STATE_EXPLICIT:  fprintf(fp, "-face_color_use explicit\n"); break;
	case DS_COLOR_STATE_OVERRIDE:  fprintf(fp, "-face_color_use override\n"); break;
	}
	fprintf(fp, "-face_override_color %f %f %f %f\n", gobj->cAttr.face.color.r, gobj->cAttr.face.color.g, gobj->cAttr.face.color.b, gobj->tAttr.alpha);
	
	// extrusion
	fprintf(fp, "-face_extrude %s\n", gobj->fAttr.extrusion.enable ? "enable" : "disable");
	fprintf(fp, "-face_extrude_2sides %s\n", gobj->fAttr.extrusion.bothSides ? "enable" : "disable");
	fprintf(fp, "-face_extrude_direction %s\n", gobj->fAttr.extrusion.direction ? "radial" : "normal");
	fprintf(fp, "-face_extrude_height %f\n", gobj->fAttr.extrusion.factor );
	fprintf(fp, "-face_extrude_hole_only %s\n", gobj->fAttr.extrusion.holeOnly ? "enable" : "disable");
	// hole
	fprintf(fp, "-face_hole %s\n", gobj->fAttr.hole.enable ? "enable" : "disable");
	fprintf(fp, "-face_hole_type ", gobj->fAttr.hole.style ? "polygonal" : "round");
	switch (gobj->fAttr.hole.style) {
	case FACE_HOLE_STYLE_ROUND:	fprintf(fp, "round\n"); break;
	case FACE_HOLE_STYLE_POLYGONAL:	fprintf(fp, "polygonal\n"); break;
	case FACE_HOLE_STYLE_ROUND_WITH_IN_DIMPLE:	fprintf(fp, "round_in_dimple\n"); break;
	case FACE_HOLE_STYLE_ROUND_WITH_OUT_DIMPLE:	fprintf(fp, "round_out_dimple\n"); break;
	}
	fprintf(fp, "-face_hole_radius %.2f\n", gobj->fAttr.hole.radius);
	fprintf(fp, "-face_hole_dimple_height %.2f\n", gobj->fAttr.hole.shallowness);
	// scale
	fprintf(fp, "-face_scale %s\n", gobj->fAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-face_scale_value %f\n", gobj->fAttr.scale.factor);
	// offset
	fprintf(fp, "-face_offset %s\n", gobj->fAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-face_offset_value %f\n", gobj->fAttr.offset.factor);
	// transparency
	fprintf(fp, "-face_transparency %s\n", gobj->tAttr.onFlag ? "enable" : "disable");
	fprintf(fp, "-face_transparency_override %s\n", gobj->tAttr.state == DS_COLOR_STATE_OVERRIDE ? "enable" : "disable");
	fprintf(fp, "-face_transparency_alpha %f\n", gobj->tAttr.alpha);

	// great circles 
	fprintf(fp, "-face_great_circles %s\n", gobj->fAttr.orthodrome.enable ? "enable" : "disable");

	// face great circles & spherical sections type
	switch (gobj->fAttr.orthodrome.style) {
	case ORTHODROME_STYLE_RIM:					p = "rim"; break;
	case ORTHODROME_STYLE_DISC_TWO_SIDED:		p = "disc_2_sided"; break;
	case ORTHODROME_STYLE_DISC_ONE_SIDED:		p = "disc_1_sided"; break;
	case ORTHODROME_STYLE_SPHERICAL_SECTION:	p = "spherical_sections"; break;
	default:									p = "rim";
	}
	fprintf(fp, "-face_great_circle_type %s\n", p );

	// face great circles & spherical sections rim dash enable
	fprintf(fp, "-face_great_circle_dashed %s\n", gobj->fAttr.orthodrome.dashEnable ? "enable" : "disable");

	// face great circles & spherical sections rim depth 1
	fprintf(fp, "-face_great_circle_rim_depth1 %.2f\n", gobj->fAttr.orthodrome.depth1 ); // NEEED NEW  clamp values

	// face great circles & spherical sections rim depth 2
	fprintf(fp, "-face_great_circle_rim_depth2 %.2f\n", gobj->fAttr.orthodrome.depth2);

	// face great circles & spherical sections rim  height
	fprintf(fp, "-face_great_circle_rim_height %.2f\n", gobj->fAttr.orthodrome.height);

	// face great circles & spherical sections seperate cut color enable
	fprintf(fp, "-face_great_circle_cut_color %s\n", gobj->fAttr.orthodrome.cutColorEnable ? "enable" : "disable");

	// face great circles & spherical sections seperate cut color 
	fprintf(fp, "-face_great_circle_cut_color_value %f %f %f\n", gobj->fAttr.orthodrome.cutColor.r, ctx->curInputObj.fAttr.orthodrome.cutColor.g, ctx->curInputObj.fAttr.orthodrome.cutColor.b);

	// label
	fprintf(fp, "-face_label %s\n", gobj->fAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-face_label_color %f %f %f\n", gobj->fAttr.label.color.r, gobj->fAttr.label.color.g, gobj->fAttr.label.color.b );
	fprintf(fp, "-face_label_font %s\n", font_name(gobj->fAttr.label.font));

	// Edges
	fprintf(fp, "-edge_draw %s\n", gobj->eAttr.draw ? "enable" : "disable");
	switch (gobj->cAttr.edge.state) {
	case DS_COLOR_STATE_AUTOMATIC: p = "automatic"; break;
	case DS_COLOR_STATE_EXPLICIT:  p = "explicit"; break;
	case DS_COLOR_STATE_OVERRIDE:  p = "override"; break;
	}
	fprintf(fp, "-edge_color_use %s\n", p);
	fprintf(fp,"-edge_override_color %f %f %f\n", gobj->cAttr.edge.color.r, gobj->cAttr.edge.color.g, gobj->cAttr.edge.color.b);

	// scale
	fprintf(fp, "-edge_scale %s\n", gobj->eAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-edge_scale_value %f\n", gobj->eAttr.scale.factor);

	// offset
	fprintf(fp, "-edge_offset %s\n", gobj->eAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-edge_offset_value %f\n", gobj->eAttr.offset.factor);

	// type
	fprintf(fp, "-edge_type %s\n", gobj->eAttr.type == GEOMETRY_EDGE_ROUND ? "cylindrical" : "square");
	fprintf(fp, "-edge_arc %d\n", gobj->eAttr.arcEnable);
//	fprintf(fp, "-edge_scale %s\n"gobj->eAttr.scale.enable ? "enable" : "disable");
	fprintf(fp, "-edge_width_height %f %f\n", gobj->eAttr.width, gobj->eAttr.height);

	// label
	fprintf(fp, "-edge_label %s\n", gobj->eAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-edge_label_color %f %f %f\n", gobj->eAttr.label.color.r, gobj->eAttr.label.color.g, gobj->eAttr.label.color.b);
	fprintf(fp, "-edge_label_font %s\n", font_name(gobj->eAttr.label.font));

	// Vertex
	fprintf(fp, "-vertex_draw %s\n", gobj->vAttr.draw ? "enable" : "disable");
	fprintf(fp, "-vertex_color %f %f %f\n", gobj->cAttr.vertex.color.r, gobj->cAttr.vertex.color.g, gobj->cAttr.vertex.color.b);
	// offset
	fprintf(fp, "-vertex_offset %s\n", gobj->vAttr.offset.enable ? "enable" : "disable");
	fprintf(fp, "-vertex_offset_value %f\n", gobj->vAttr.offset.factor);
	fprintf(fp, "-vertex_scale %f\n", gobj->vAttr.scale);//.width, gobj->eAttr.height, gobj->eAttr.offset);

	// label
	fprintf(fp, "-vertex_label %s\n", gobj->vAttr.label.enable ? "enable" : "disable");
	fprintf(fp, "-vertex_label_color %f %f %f\n", gobj->vAttr.label.color.r, gobj->vAttr.label.color.g, gobj->vAttr.label.color.b);
	fprintf(fp, "-vertex_label_font %s\n", font_name(gobj->vAttr.label.font));

	return 0;
}

//-----------------------------------------------------------------------------
static char *font_name(void *id)
//-----------------------------------------------------------------------------
{
	switch ((int)id) {
	case 0x0004: return "T10"; break;
	case 0x0005: return "T24"; break;
	case 0x0006: return "H10"; break;
	case 0x0007: return "H12"; break;
	case 0x0008: return "H18"; break;
	default: return "T24";
	}
}

//-----------------------------------------------------------------------------
int cmd_line_init(DS_CTX *ctx)
//-----------------------------------------------------------------------------
{
	return ds_build_command_line(ctx);
}

//-----------------------------------------------------------------------------
int ds_command_line(DS_CTX *ctx, int ac, char **av, int *error, int *argIndex, DS_ERROR *errInfo)
//-----------------------------------------------------------------------------
{
//	int					currentIndex = 0;
//	ARGUMENT_SUBSTITUTE *arg_sub;
//	int					status = 0;
//
//	cmd_line_init(ctx); // initialize any pointers
//
//	while (currentIndex < ac)
//	{
//		// look for alternate short form - and replace if found
//		if (arg_sub = arg_find_substitute(set_main_substitute, av[currentIndex]))
//		{
//			av[currentIndex] = arg_sub->longForm;
//		}
//		if (arg_decode(set_main, &currentIndex, ac, av, error, argIndex, errInfo))
//			break;
//	}
	return 0;
}
