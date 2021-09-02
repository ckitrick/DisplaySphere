enum {
	_DS_DATA_TYPE_NA = 0,
	_DS_DATA_TYPE_INT = 1,
	_DS_DATA_TYPE_FLOAT,
	_DS_DATA_TYPE_DOUBLE,
	_DS_DATA_TYPE_STRING_POINTER,
	_DS_DATA_TYPE_STRING_COPY,
	_DS_ARGUMENT_TYPE_DATA = 1,
	_DS_ARGUMENT_TYPE_CHOICE,
};

typedef int(*_DS_CMD_LINE_FUNCTION)(void *cmdLineCtx, void *opt, void *userData);

typedef struct {
	char		*matchText;		// text to match for the choice (if matched then search concludes)
	void		*matchSrcAddr;	// address of data associated with the source 
} _DS_ARGUMENT_CHOICE;

typedef struct {
	void		*minValueAddr; // address of min value (data dependent)
	void		*maxValueAddr; // address of max value (data dependent)
} _DS_ARGUMENT_CLAMP;

typedef struct {
	unsigned int		type		: 8; // argument type determines what kind of data to expect ARRAY, CHOICE
	unsigned int		optionFlag	: 8; // argument is optional (use carefully)
	unsigned int		clampFlag	: 8; // clamping required flag
	unsigned int		dataType	: 8; // destination data type
	int					dataCount	: 8; // number of components 
	int					dataSize;
	int					dstBufferSize;	// for text
	int					dataSizePerComponent;
	void				*dstAddr;		 // where to transfer input data
	char				*text;			// argument value 
	_DS_ARGUMENT_CLAMP	clamp;
	LL					*choiceq;	// que`ue of choices (if needed)
} _DS_CMD_LINE_ARGUMENT;

typedef struct {
	char					*text;			// text to match for option processing
	char					*description;	// 
	int						dataSize;		// customization
	void					*srcAddr;		// address of src data
	void					*dstAddr;		// address to copy user supplied src data
	_DS_CMD_LINE_FUNCTION	preFunction;	// function to call before option processed  int function ( void )
	_DS_CMD_LINE_FUNCTION	postFunction;	// function to call after option processed int function ( int status )
	LL						*argq;			// queue of arguments
} _DS_CMD_LINE_OPTION;

typedef struct {
	void	*optionAVL;
	void	*userData;		// used to store caller data to be returned for pre/post function calls
} _DS_CMD_LINE_CONTEXT;


// functions to add argument definitions
// standard argument for integers and reals
_DS_CMD_LINE_ARGUMENT *_ds_add_option_number_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataType, int count, void *dstAddr, int dataSizePerComponent, int optionFlag, int clampFlag, void *min, void *max);
// standard argument for strings
_DS_CMD_LINE_ARGUMENT *_ds_add_option_string_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataType, void *dstAddr, int dstBufferSize, int optionFlag);
// choice argument
// choice argument
//_DS_CMD_LINE_ARGUMENT *_ds_add_option_choice_argument(_DS_CMD_LINE_OPTION *option, int dataType, void *dstAddr, int optionFlag, void *srcAddr, ...); // pairs of ("text", void *srcData);
_DS_CMD_LINE_ARGUMENT *_ds_add_option_choice_argument(_DS_CMD_LINE_OPTION *option, char *text, int dataSize, void *dstAddr, int optionFlag, ...); // pairs of ("text", void *srcData);
// function to add a command line option
//_DS_CMD_LINE_OPTION *_ds_add_option(void *table, char *text, int dataType, void *srcData, void *dstAddr, _DS_CMD_LINE_FUNCTION pre_function, _DS_CMD_LINE_FUNCTION post_function);
_DS_CMD_LINE_OPTION *_ds_add_option(void *table, char *text, int dataSize, void *srcData, void *dstAddr, _DS_CMD_LINE_FUNCTION pre_function, _DS_CMD_LINE_FUNCTION post_function);
// function to create base level context
void *_ds_create_cmd_line_context();
// process command line and track errors
int _ds_cmd_line_process(void *cmd_line_context, int ac, int av, int *consumedCount, _DS_CMD_LINE_OPTION *errOpt);
//_DS_CMD_LINE_FUNCTION ds_object_option(_DS_CMD_LINE_CONTEXT *cmdLineCtx, _DS_CMD_LINE_OPTION *opt, void *userData);
