/*
	Simple text parsing function
*/
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include "ds_par.h"

#define QUOTED_STRING   1
#define STRING          0

//-----------------------------------------------------------------------------
int ds_parse_lexeme ( char *pcString, char **pcArray, int nMaxSize )
//-----------------------------------------------------------------------------
{
	// Function : ds_parse_lexeme ( char *pcString, char **pcArray, int nMaxSize )
	//
	// Purpose  : Break a string into words
	//
	// Arguments: pcString      = string buffer to work on
	//            pcArray       = array of character pointers
	//            nMaxSize      = maximum length of pcArray
	//
	// Return   : number of words in string (the start of each word is in array) 
	//  Delimiters
    //      ',' comma
    //      '=' equal
    //      '"' quotes (not currently supported)
    //
    //  NOTE: This routine modifies the original string

    char    c;
    int     index = 0;
    char    *from;
    char    *start;
    int     type;
    int     inside = 0;
        
    // check for degenerate case
    if ( !pcString || nMaxSize <= 0 )
        return index;
    
    // convert pseudo carriage returns to real ones
    from = pcString;
    
    //loop
    while ( c = *from++ )
    {
        if ( inside )
        {            
            if ( isspace ( c ) )
            {
                switch ( type )
                {
                    case QUOTED_STRING:       
                        // keep going
                        break;
                    
                    default:
                        // new string complete
                        inside = 0;
                        *(from - 1) = 0;
                        if ( index >= nMaxSize )
                            return index;
                        pcArray[ index++ ] = start;
                }
            } 
            else if ( c == '"' )
            {
                switch ( type )
                {
                    case QUOTED_STRING:
                        inside = 0;
                        *(from - 1) = 0;
                        if ( index >= nMaxSize )
                            return index;
                        pcArray[ index++ ] = start + 1;
                        break;
                }
            }                 
        }
        else
        {
            if ( !isspace ( c ) )
            {
                inside = 1;
                start = from - 1;
                if ( c == '"' )
                    type = QUOTED_STRING;
                else
                    type = STRING;
            }
        }
    }
    
    if ( !inside )
        return index;
    
    switch ( type )
    {
        case QUOTED_STRING:
            *(from - 1) = 0;
            if ( index >= nMaxSize )
                return index;
            pcArray[ index++ ] = start + 1;
            break;
        
        default:
            *(from - 1) = 0;
            if ( index >= nMaxSize )
                return index;
            pcArray[ index++ ] = start;
    }
    
    return index;
}
