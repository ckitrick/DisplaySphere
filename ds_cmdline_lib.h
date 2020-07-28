
#define CLAMP_ZERO_TO_ONE(value) ( value < 0.0 ? 0.0 : ( value > 1.0 ? 1.0 : value ) )

#define ARG_ERROR_ARG_NOT_FOUND			1 
#define ARG_ERROR_ARG_DATA_NOT_FOUND	2 

#define ATYPE_SUB_ARGUMENT			0 
#define ATYPE_STRING				1
#define ATYPE_INTEGER				2 
#define	ATYPE_FLOAT					5 
#define	ATYPE_DOUBLE				6 
#define	ATYPE_FLOAT_CLAMP			7 
#define	ATYPE_DOUBLE_CLAMP			8 
#define	ATYPE_SET_EXPLICIT			9
#define	ATYPE_USER_FUNCTION			10 
#define ATYPE_ARRAY					0x100 

typedef int ATYPE;
typedef int ADATA;
typedef void *ADDR;

typedef struct {
	char	*text;			// invoking text
	char	*alt;			// alternate text
	int		len;			// compare length (if 0 then whole string)
	int		nArgs;			// number of args		(or'd value of number with array flag
	int		*type;			// array of arg types = 0-subMatch, 1-char*, 2-int, 3-intFlag, 4-intReverseFlag, 5-SetExplicit, 6-float, 7-floatClamp	//2-char* malloc, 
	int		*data;			// explicit data
	void	**addr;			// addresses of where to save arg data or pointer to a sub-Argument
	char	*description;	// description of the command line option
} ARGUMENT;

typedef struct {
	int			nArguments;
	ARGUMENT	*argument;
} ARGUMENT_SET;

typedef struct {
//	int		id;
	char	*shortForm;
	char	*longForm;
}ARGUMENT_SUBSTITUTE;

typedef struct {
	int					nArguments;
	ARGUMENT_SUBSTITUTE	*argument;
} ARGUMENT_SUBSTITUTE_SET;

typedef int(*ARG_USER_FUNCTION)(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex);
ARGUMENT_SUBSTITUTE *arg_find_substitute(ARGUMENT_SUBSTITUTE_SET *set, char *arg);
ARGUMENT *arg_find(ARGUMENT_SET *set, char *arg);
int arg_process(ARGUMENT *arg, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex);
int arg_decode(ARGUMENT_SET *set, int *currentArgIndex, int maxNArgs, char **av, int *error, int *argIndex);

