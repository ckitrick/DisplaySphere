
/*
	This group of functions are designed to provide general argument handling of the command line options.
*/

#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
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
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"
#include "ds_cmdline_lib.h"

//-----------------------------------------------------------------------------
ARGUMENT *arg_find(ARGUMENT_SET *set, char *arg)
//-----------------------------------------------------------------------------
{
	int			lo = 0,
		hi = set->nArguments - 1,
		md,
		diff;

	while (hi >= lo)
	{
		md = (lo + hi) / 2;

		if (!set->argument[md].len) // decide which compare function to use
			diff = strcmp(arg, set->argument[md].text);
		else
			diff = strncmp(arg, set->argument[md].text, set->argument[md].len);

		if (!diff)
		{
			return set->argument + md;
		}
		else if (diff < 0)
		{
			hi = md - 1;
		}
		else
		{
			lo = md + 1;
		}
	}

	return 0; // UNDEFINED
}

//-----------------------------------------------------------------------------
ARGUMENT_SUBSTITUTE *arg_find_substitute(ARGUMENT_SUBSTITUTE_SET *set, char *arg)
//-----------------------------------------------------------------------------
{
	int			lo = 0,
		hi = set->nArguments - 1,
		md,
		diff;

	while (hi >= lo)
	{
		md = (lo + hi) / 2;

		diff = strcmp(arg, set->argument[md].shortForm);

		if (!diff)
		{
			return set->argument + md;
		}
		else if (diff < 0)
		{
			hi = md - 1;
		}
		else
		{
			lo = md + 1;
		}
	}

	return 0; // UNDEFINED
}

//-----------------------------------------------------------------------------
int arg_process(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex)
//-----------------------------------------------------------------------------
{
	if (!(arg->nArgs & 0xff)) // zero sub-args
	{
		if (!arg->addr) { ++*error; return 1; }
		++*currentArgIndex; // consume the arg
		switch ((int)arg->type) {
		case ATYPE_SET_EXPLICIT:	arg->addr ? (*(int*)arg->addr = (int)arg->data) : 0; break;
		case ATYPE_USER_FUNCTION:	return ((ARG_USER_FUNCTION)arg->addr)(arg, currentArgIndex, maxNArgs, av, error, argIndex); break;
		default:
			++*error;
			return 1;
		}
		return 0;
	}
	else if ((arg->nArgs & 0xff) == 1)
	{
		++*currentArgIndex; // consume the originating arg
		float	f;
		double	d;

		if (*currentArgIndex == maxNArgs) { ++*error; return 1; }//error
		if (!arg->addr) { ++*error; return 1; }

		switch ((int)arg->type) {
		case ATYPE_SUB_ARGUMENT: // recurse down
		{
			ARGUMENT_SET	*set;
			if (!(set = (ARGUMENT_SET*)arg->addr)) { ++*error; return 1; }
			if (arg_decode(set, currentArgIndex, maxNArgs, av, error, argIndex)) { ++*error; return 1; }
		}
		break;
		case ATYPE_STRING:  arg->addr ? strcpy((char*)arg->addr, av[*currentArgIndex]) : 0;		break;
		case ATYPE_INTEGER: arg->addr ? *(int*)arg->addr = atoi(av[*currentArgIndex]) : 0;		break;
		case ATYPE_FLOAT:   arg->addr ? *(float*)arg->addr = (float)atof(av[*currentArgIndex]) : 0;	break;
		case ATYPE_DOUBLE:  arg->addr ? *(double*)arg->addr = atof(av[*currentArgIndex]) : 0;	break;
		case ATYPE_FLOAT_CLAMP:
			if (arg->addr)
			{
				f = (float)atof(av[*currentArgIndex]);
				*(float*)arg->addr = (float)(CLAMP_ZERO_TO_ONE(f));
			}
			break;
		case ATYPE_DOUBLE_CLAMP:
			if (arg->addr)
			{
				d = atof(av[*currentArgIndex]);
				*(double*)arg->addr = CLAMP_ZERO_TO_ONE(d);
			}
			break;
		}
		if ((int)arg->type != ATYPE_SUB_ARGUMENT)
			++*currentArgIndex;
	}
	else
	{
		++*currentArgIndex; // consume the originating arg
		int		i;
		float	f;
		double	d;

		if (arg->nArgs & ATYPE_ARRAY) // array of same type
		{
			int		nArgs = arg->nArgs & 0xff;
			int		type;
			int		*j;
			char	*s;
			float	*f, ff;
			double	*d, dd;
			void	*addr;

			type = (int)arg->type; // array so type is constant
			addr = (void*)arg->addr; // array so single addr base 

			for (i = 0; i < nArgs; ++i)
			{
				if (*currentArgIndex == maxNArgs) { ++*error; return 1; }
				if (!addr) { ++*error; return 1; }

				switch (type) {
				case ATYPE_SUB_ARGUMENT: // recurse down
				{
					ARGUMENT_SET	*set;
					if (!(set = (ARGUMENT_SET*)addr)) return 1;
					if (arg_decode(set, currentArgIndex, maxNArgs, av, error, argIndex)) { ++*error; return 1; }
					addr = (void*)(set + 1); // (((ARGUMENT_SET*)addr)[1]); // next
				}
				break;
				case ATYPE_STRING:  s = (char*)addr; strcpy(s, av[*currentArgIndex]);		addr = (void*)(s + 1); break;
				case ATYPE_INTEGER: j = (int*)addr; *j = atoi(av[*currentArgIndex]);		addr = (void*)(j + 1); break;
				case ATYPE_FLOAT:   f = (float*)addr; *f = (float)atof(av[*currentArgIndex]);		addr = (void*)(f + 1); break;
				case ATYPE_DOUBLE:  d = (double*)addr; *d = atof(av[*currentArgIndex]);		addr = (void*)(d + 1); break;
				case ATYPE_FLOAT_CLAMP:
					ff = (float)atof(av[*currentArgIndex]);
					f = (float*)addr;
					*f = (float)(CLAMP_ZERO_TO_ONE(ff));
					addr = (void*)(f + 1);
					break;
				case ATYPE_DOUBLE_CLAMP:
					dd = atof(av[*currentArgIndex]);
					d = (double*)addr;
					*d = CLAMP_ZERO_TO_ONE(dd);
					addr = (void*)(d + 1);
					break;
				}
				if ((int)arg->type != ATYPE_SUB_ARGUMENT)
					++*currentArgIndex;
			}
		}
		else // normal - every arg can be a different type
		{
			for (i = 0; i < arg->nArgs; ++i)
			{
				if (*currentArgIndex == maxNArgs) { ++*error; return 1; }
				if (!arg->addr) { ++*error; return 1; }

				switch (arg->type[i]) {
				case ATYPE_SUB_ARGUMENT: // recurse down
				{
					ARGUMENT_SET	*set;
					if (!arg->addr[i]) { ++*error; return 1; }
					if (!(set = (ARGUMENT_SET*)arg->addr[i])) { ++*error; return 1; }
					if (arg_decode(set, currentArgIndex, maxNArgs, av, error, argIndex)) { ++*error; return 1; }
				}
				break;
				case ATYPE_STRING:  arg->addr[i] ? strcpy((char*)arg->addr[i], av[*currentArgIndex]) : 0;	break;
				case ATYPE_INTEGER: arg->addr[i] ? *(int*)arg->addr[i] = atoi(av[*currentArgIndex]) : 0;	break;
				case ATYPE_FLOAT:   arg->addr[i] ? *(float*)arg->addr[i] = (float)atof(av[*currentArgIndex]) : 0;	break;
				case ATYPE_DOUBLE:  arg->addr[i] ? *(double*)arg->addr[i] = atof(av[*currentArgIndex]) : 0;	break;
				case ATYPE_FLOAT_CLAMP:
					if (arg->addr[i])
					{
						f = (float)atof(av[*currentArgIndex]);
						*(float*)arg->addr[i] = (float)(CLAMP_ZERO_TO_ONE(f));
					}
					break;
				case ATYPE_DOUBLE_CLAMP:
					if (arg->addr[i])
					{
						d = atof(av[*currentArgIndex]);
						*(double*)arg->addr[i] = CLAMP_ZERO_TO_ONE(d);
					}
					break;
				}
				if ((int)arg->type != ATYPE_SUB_ARGUMENT)
					++*currentArgIndex;
			}
		}
	}
	return 0;
}

//-----------------------------------------------------------------------------
int arg_decode(ARGUMENT_SET *set, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex )
//-----------------------------------------------------------------------------
{
	ARGUMENT	*arg;

	if (!*error && *currentArgIndex < maxNArgs)
	{
		arg = arg_find(set, av[*currentArgIndex]);
		if (!arg)
		{
			*error = ARG_ERROR_ARG_NOT_FOUND; // failure
			*argIndex = *currentArgIndex;
			return 1; // stop
		}
		else
		{
			// process single arg
			if (arg_process(arg, currentArgIndex, maxNArgs, av, error, argIndex))
			{
//				++*error;
				*error = ARG_ERROR_ARG_DATA_NOT_FOUND; // failure
				*argIndex = *currentArgIndex;
				return 1;// break; // error
			}
		}
		return 0;
	}
	else
	{
		return 1;
	}
}
