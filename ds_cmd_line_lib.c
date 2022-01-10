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
#include <ctype.h>
#include <link.h>
#include <avl_new.h>
#include <geoutil.h>
#include <matrix.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_cmd_line.h"

//--------------------------------------------------------------------------------------------------------------
_DS_CMD_LINE_ARGUMENT *_ds_add_option_number_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataType, int count, void *dstAddr, int dataSizePerComponent, int optionFlag, int clampFlag, void *min, void *max)
//--------------------------------------------------------------------------------------------------------------
{
	// initialize a typical numerical argument(s)
	_DS_CMD_LINE_ARGUMENT	*arg;

	if (!option) // check pointer
		return 0;

	// allocate and initialize the argument
	arg = (_DS_CMD_LINE_ARGUMENT*)malloc(sizeof(_DS_CMD_LINE_ARGUMENT));

	arg->type					= _DS_ARGUMENT_TYPE_DATA;	// argument type determines what kind of data to expect ARRAY, CHOICE
	arg->text					= text;						// text
	arg->optionFlag				= optionFlag;				// argument is optional (use carefully)
	arg->clampFlag				= clampFlag;				// clamping required flag
	arg->dataType				= dataType;					// destination data type
	arg->dataCount				= count;					// number of components 
	arg->dataSizePerComponent	= dataSizePerComponent;
	arg->dstAddr				= dstAddr;					// where to transfer input data
	arg->clamp.minValueAddr		= min;
	arg->clamp.maxValueAddr		= max;
	arg->choiceq				= 0;						// not used
	LL_AddTail(option->argq, (void*)arg);					// add the argument(s) in order to the option

	return arg; // return the pointer as success
}

//--------------------------------------------------------------------------------------------------------------
_DS_CMD_LINE_ARGUMENT *_ds_add_option_string_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataType, void *dstAddr, int dstBufferSize, int optionFlag)
//--------------------------------------------------------------------------------------------------------------
{
	_DS_CMD_LINE_ARGUMENT	*arg;

	if (!option)
		return 0;

	arg = (_DS_CMD_LINE_ARGUMENT*)malloc(sizeof(_DS_CMD_LINE_ARGUMENT));

	arg->type				= _DS_ARGUMENT_TYPE_DATA;	// argument type determines what kind of data to expect ARRAY, CHOICE
	arg->text				= text;						// text
	arg->optionFlag			= optionFlag;				// argument is optional (use carefully)
	arg->clampFlag			= 0;						// clamping required flag
	arg->dataType			= dataType;					// destination data type
	arg->dataCount			= 1;						// number of components is fixed
	arg->dstBufferSize		= dstBufferSize;
	arg->dstAddr			= dstAddr;					// where to transfer input data
	arg->choiceq			= 0;						// not used
	LL_AddTail(option->argq, (void*)arg);
	return arg;
}

//--------------------------------------------------------------------------------------------------------------
_DS_CMD_LINE_ARGUMENT *_ds_add_option_choice_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataSize, void *dstAddr, int optionFlag, ...)// pairs of ("text", void *srcData)
//--------------------------------------------------------------------------------------------------------------
{
	// initialize a choice argument
	// any number of choice pairs can follow the dstAddr in the call
	// a ZERO in the call is required to stop the function arg processing
	_DS_CMD_LINE_ARGUMENT	*arg;
	_DS_ARGUMENT_CHOICE		*choice;
	char					*matchText;
	void					*matchSrcAddr;
	int						error = 0;

	if (!option) // check pointer value
		return 0;

	// allocated a new argument structure
	arg = (_DS_CMD_LINE_ARGUMENT*)malloc(sizeof(_DS_CMD_LINE_ARGUMENT));

	// initialize
	arg->type					= _DS_ARGUMENT_TYPE_CHOICE;	// argument type determines what kind of data to expect ARRAY, CHOICE
	arg->text					= text;						// text
//	arg->dataType				= dataType;					// destination data type
	arg->dataSize				= dataSize;					// destination data type
	arg->dstAddr				= dstAddr;					// where to transfer input data
	arg->optionFlag				= optionFlag;				// argument is optional (use carefully)
	arg->clampFlag				= 0;						// clamping required flag
	arg->dataCount				= 1;						// number of components 
	arg->dataSizePerComponent	= 0;
	arg->clamp.minValueAddr		= 0;
	arg->clamp.maxValueAddr		= 0;
	arg->choiceq				= LL_Create();				// queue to place the choice pairs

//	if (optionFlag)
//	{
//		arg->clamp.minValueAddr = srcAddr;
//	}

	// process all the available choice pairs on the call stack
	va_list ap;
	va_start(ap, optionFlag); //Requires the last fixed parameter (to get the address)

	while (1)
	{
		// get and validate the matchText for the pair
		matchText = va_arg(ap, char*);
		if (!matchText)
		{
			// indicates the end of the pairs so return as successful
			break;
		}

		// get and validate the matchSrcAddr for the pair
		matchSrcAddr = va_arg(ap, void*);
		if (!matchSrcAddr)
		{
			error = 1; // this is flagged as an error
			break;
		}
		// There is a valid pair
		// allocate and initialize a choice then add to the argument queue
		choice					= (_DS_ARGUMENT_CHOICE*)malloc(sizeof(_DS_ARGUMENT_CHOICE));
		choice->matchText		= (char*)malloc(strlen(matchText) + 1);
		strcpy(choice->matchText, matchText);
		choice->matchSrcAddr	= matchSrcAddr;
		LL_AddTail(arg->choiceq, (void*)choice);
	}
	va_end(ap);

	// add the arg to the queue
	if(!error)
		LL_AddTail(option->argq, (void*)arg);

	return arg; // return pointer as proof of success
}

//--------------------------------------------------------------------------------------------------------------
_DS_CMD_LINE_OPTION *_ds_add_cmd_line_option( _DS_CMD_LINE_CONTEXT *ctx, char *text, int dataSize,void *srcAddr, void *dstAddr, void *preFunction, void *postFunction )
//--------------------------------------------------------------------------------------------------------------
{
	// Attempt to add a new option
	_DS_CMD_LINE_OPTION	option, *opt=0, *dup; // , check;
	
	option.text = text;

	avl_find(ctx->optionAVL, &option, &opt);

	if (opt) 
	{
		// duplicates are not allowed
		return 0; // fail
	}

	// allocated a new option structure and initialize it
	opt = (_DS_CMD_LINE_OPTION*)malloc(sizeof(_DS_CMD_LINE_OPTION));

	opt->text = text; // (char*)malloc(strlen(text) + 1);
//	strcpy(opt->text, text);
//	opt->dataType		= dataType;		// type of src/dst data
	opt->dataSize		= dataSize;		// customization
	opt->srcAddr		= srcAddr;		// address of src data
	opt->dstAddr		= dstAddr;		// address to copy user supplied src data
	opt->preFunction	= preFunction;	// function to call before option processed  int function ( void )
	opt->postFunction	= postFunction;	// function to call after option processed int function ( int status )
	opt->argq			= LL_Create();

	// insert new option into the tree
	avl_insert(ctx->optionAVL, (void*)opt);
	return opt; // success - this pointer will be used to add any arguments
}

//--------------------------------------------------------------------------------------------------------------
_DS_CMD_LINE_OPTION *_ds_add_cmd_line_option_copy(_DS_CMD_LINE_CONTEXT *ctx, char *existingText, char *copyText )
//--------------------------------------------------------------------------------------------------------------
{
	// make a copy of an option with a new text definition
	_DS_CMD_LINE_OPTION	option, *opt=0, *copy; 

	// intialize temp option to search with
	option.text = existingText;

	// prform the lookup
	avl_find(ctx->optionAVL, &option, &copy);

	if (copy)
	{
		// allocated a new option with all the same information 
		// BUT replace the matching option string
		opt = (_DS_CMD_LINE_OPTION*)malloc(sizeof(_DS_CMD_LINE_OPTION));
		*opt = *copy;
		opt->text = (char*)malloc(strlen(existingText) + 1);
		strcpy(opt->text, copyText);
		return opt; // success
	}
	else
		return 0; // failed to make a copy
}

//--------------------------------------------------------------------------------------------------------------
int isnumber(char *string)
//--------------------------------------------------------------------------------------------------------------
{
	// validate a number to be an integer or real number
	// simple state machine [+-][0-9][.][0-9][eE][+-][0-9] (7 states)

	int		state = 0, count = 0, len = 0;
	char	*src, *dst;
	char	buffer[256];

	src = string;
	dst = buffer;

	while (*src)
	{
		switch (state) {
		case 0: //sign 
			if (*src == '+' || *src == '-') 
				*dst++ = *src++; 
			++state;
			break;
		case 1: // mantissa
			if (*src < '0' || *src > '9')
				++state;
			else
				*dst++ = *src++;
			break;
		case 2: // decimal
			if (*src == '.')
				*dst++ = *src++;
			++state;
			break;
		case 3: //fraction
			if (*src < '0' || *src > '9')
				++state;
			else
				*dst++ = *src++;
			break;
		case 4: // eE
			if (*src == 'e' || *src == 'E') 
				*dst++ = *src++;
			++state;
			break;
		case 5: // esign
			if (*src == '+' || *src == '-') 
				*dst++ = *src++;
			++state;
			break;
		case 6: // exponent
			if (*src < '0' || *src > '9')
				++state;
			else
				*dst++ = *src++;
		default:
			return 0;
		}
	}
	*dst++ = 0;
	return 1;
}

//--------------------------------------------------------------------------------------------------------------
int _ds_cmd_line_post_function(_DS_CMD_LINE_CONTEXT *ctx, _DS_CMD_LINE_OPTION *opt, void *userData) //, _DS_CMD_LINE_ARGUMENT *arg, int ac, int *index, char **av)
//--------------------------------------------------------------------------------------------------------------
{

}
//
//--------------------------------------------------------------------------------------------------------------
int _ds_cmd_line_process_argument(_DS_CMD_LINE_CONTEXT *ctx, _DS_CMD_LINE_OPTION *opt, _DS_CMD_LINE_ARGUMENT *arg, int ac, int *index, char **av)
//--------------------------------------------------------------------------------------------------------------
{
	int			i;
	int			idx = *index;  // make a copy
	void		*dstAddr = arg->dstAddr; // make a copy

	// check if the argument's required data is available
	switch (arg->type) {
	case _DS_ARGUMENT_TYPE_DATA:
		// check the number required versus how many available
		if ((idx + arg->dataCount) > ac)
		{
			// fail
			if (arg->optionFlag)
				return 0; // SUCCESS
			else
				return 1; // FAIL 
		}
		// attempt to get each argument value
		for (i = 0; i < arg->dataCount; ++i)
		{
			switch (arg->dataType) {
			case _DS_DATA_TYPE_INT: 
				if (isnumber(av[idx]))
				{
					*(int*)dstAddr = atoi(av[idx++]); // copy data
					// perform any required clamping of the numerical data
					if (arg->clampFlag && arg->clamp.minValueAddr && arg->clamp.maxValueAddr)
					{
						if (*(int*)dstAddr < *(int*)arg->clamp.minValueAddr)
							*(int*)dstAddr = *(int*)arg->clamp.minValueAddr; // copy data
						else if (*(int*)dstAddr > *(int*)arg->clamp.maxValueAddr)
							*(int*)dstAddr = *(int*)arg->clamp.maxValueAddr; // copy data
					}
					(char*)dstAddr += sizeof(int); // move forward
				}
				else
				{
					if (!i && arg->optionFlag)
					{
						*index = idx;
						return 0; // SUCCESS
					}
					else
						return 1; // FAIL
				}
				break;
			case _DS_DATA_TYPE_FLOAT:
				if (isnumber(av[idx]))
				{
					*(float*)dstAddr = atof(av[idx++]); // copy data
					// perform any required clamping of the numerical data
					if (arg->clampFlag && arg->clamp.minValueAddr && arg->clamp.maxValueAddr)
					{
						if (*(float*)dstAddr < *(float*)arg->clamp.minValueAddr)
							*(float*)dstAddr = *(float*)arg->clamp.minValueAddr; // copy data
						else if (*(float*)dstAddr > *(float*)arg->clamp.maxValueAddr)
							*(float*)dstAddr = *(float*)arg->clamp.maxValueAddr; // copy data
					}
					(char*)dstAddr += sizeof(float); // move forward
				}
				else
				{
					if (!i && arg->optionFlag)
					{
						*index = idx;
						return 0; // SUCCESS
					}
					else
						return 1; // FAIL
				}
				break;
			case _DS_DATA_TYPE_DOUBLE:
				if (isnumber(av[idx]))
				{
					*(double*)dstAddr = atof(av[idx++]); // copy data
					// perform any required clamping of the numerical data
					if (arg->clampFlag && arg->clamp.minValueAddr && arg->clamp.maxValueAddr)
					{
						if (*(double*)dstAddr < *(double*)arg->clamp.minValueAddr)
							*(double*)dstAddr = *(double*)arg->clamp.minValueAddr; // copy data
						else if (*(double*)dstAddr > *(double*)arg->clamp.maxValueAddr)
							*(double*)dstAddr = *(double*)arg->clamp.maxValueAddr; // copy data
					}
					(char*)dstAddr += sizeof(double); // move forward
				}
				else
				{
					if (!i && arg->optionFlag)
					{
						*index = idx;
						return 0; // SUCCESS
					}
					else
						return 1; // FAIL
				}
				break;
			case _DS_DATA_TYPE_STRING_POINTER:
				// malloc a pointer and save argument text in destination
				*(char**)arg->dstAddr = (char*)malloc(strlen(av[idx])+1);
				strcpy(*(char**)arg->dstAddr, av[idx++]);
				break;
			case _DS_DATA_TYPE_STRING_COPY:
				// copy text into destination buffer
				strncpy((char*)arg->dstAddr, av[idx++], arg->dstBufferSize);
				break;
			}
		}
		break;
	case _DS_ARGUMENT_TYPE_CHOICE:
		_DS_ARGUMENT_CHOICE	*choice;
		// check the number required versus how many available
		// check the data type versus what is provided
		// if fails but was optional then return success
		// check the number required versus how many available
		if ((idx + arg->dataCount) > ac)
		{
			// fail
			if (arg->optionFlag)
			{
				*index = idx;
				return 0; // SUCCESS
			}
			else
			{
				*index = idx;
				return 1; // FAIL 
			}
		}
		LL_SetHead(arg->choiceq);
		while (choice = (_DS_ARGUMENT_CHOICE*)LL_GetNext(arg->choiceq))
		{
			if (!stricmp(choice->matchText, av[idx])) // case insensitive by default
			{
				++idx; // consume
				memcpy(arg->dstAddr, choice->matchSrcAddr, arg->dataSize);
				*index = idx;
				return 0;
			}
		}
		// choice text not found so check for optional
		if (arg->optionFlag)
		{
			*index = idx;
			return 0; // SUCCESS
		}
		else
		{
			*index = idx;
			return 1; // FAIL 
		}
		break;
	default:
		*index = idx;
		return 1;
	}

	*index += arg->dataCount; // move the index over the consumed data
	return 0;
}

//--------------------------------------------------------------------------------------------------------------
int _ds_cmd_line_process(_DS_CMD_LINE_CONTEXT *ctx, int ac, char **av, int *consumedCount, char **error)
//--------------------------------------------------------------------------------------------------------------
{
	// Process a command line made up of 'n' number of text components
	// The 'consumed' variable is updated with the number of components used
	int						index = 0;
	_DS_CMD_LINE_OPTION		*opt = 0, temp;
	_DS_CMD_LINE_ARGUMENT	*arg;
	static char				optionName[64];

	// intialize
	*consumedCount = 0;

	// loop until all the arguments are used up
	while (index < ac)
	{
		// initialize a dummy option to seach for the real one if it exists
		temp.text = av[index];

		// find option and process it
		if (avl_find(ctx->optionAVL, &temp, &opt))
		{
			++index; // move to next item - since this item is already used

			// INDEPENDENT OF SUCCESS - so update the associated variable if needed
			if (opt->dstAddr && opt->srcAddr && opt->dataSize) // check to make sure there is a destination address
			{
				memcpy(opt->dstAddr, opt->srcAddr, opt->dataSize);
			}

			// process arguments
			LL_SetHead(opt->argq); // initialize queue traversal
			while (arg = (_DS_CMD_LINE_ARGUMENT*)LL_GetNext(opt->argq))
			{
				// process any defined arguments to this option 
				if (_ds_cmd_line_process_argument(ctx, opt, arg, ac, &index, av))
				{
					// there was a problem with the arguments
					*consumedCount = index;
					strcpy(optionName, temp.text);
					*error = optionName;
					return 2; // FAIL
					break; // error
				}
			}
		}
		else // option not found
		{

			*consumedCount = index;
			strcpy(optionName, temp.text); // copy into static buffer
			*error = optionName;
			return 1; // FAIL
			break; // error
		}
	}
	*consumedCount = index;
	if (opt && opt->postFunction)
	{
		// return the results of the post_function call
		return opt->postFunction(ctx, opt, ctx->userData);
	}
	else
		return 0;
}

//--------------------------------------------------------------------------------------------------------------
AVL_COMPARE _ds_cmd_line_option_compare(void *passThru, void *a, void *b)
//--------------------------------------------------------------------------------------------------------------
{
	// compare two options by their text - CASE SENSITIVE
	// this function is called by the AVL library when searching/inserting 
	_DS_CMD_LINE_OPTION	*ao=(_DS_CMD_LINE_OPTION*)a, *bo=(_DS_CMD_LINE_OPTION*)b;
	return strcmp(bo->text, ao->text);
}

//--------------------------------------------------------------------------------------------------------------
void *_ds_create_cmd_line_context(void *userData)
//--------------------------------------------------------------------------------------------------------------
{
	// create the context to hold all information on options and their arguments
	_DS_CMD_LINE_CONTEXT	*ctx = (_DS_CMD_LINE_CONTEXT*)malloc(sizeof(_DS_CMD_LINE_CONTEXT));

	// create self-balancing AVL tree of options
	ctx->optionAVL = avl_create(_ds_cmd_line_option_compare, ctx); // Create an AVL tree
	ctx->userData = userData;

	// return the context to be sent on subsequent function calls
	return ctx;
}
