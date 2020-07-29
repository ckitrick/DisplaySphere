/*
	Copyright (C) 2020 Christopher J Kitrick

	Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
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
