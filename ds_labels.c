/*
Copyright (C) 2020 Christopher J Kitrick

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/*
This group of functions is designed to handle rendering operations
*/
#include <stdlib.h>
#include <windows.h>		/* must include this before GL/gl.h */
#include <OBJBASE.H>
#include <direct.h>
#include <ShlObj.h>
#include <GL/gl.h>			/* OpenGL header file */
#include <GL/glut.h>		/* OpenGL header file */
#include <GL/glext.h>		/* OpenGL header file */
#include <GL/wglext.h>
#include <stdio.h>
#include <math.h>
#include <commdlg.h>
#include "resource.h"
#include <geoutil.h>
#include <matrix.h>
#include <mem.h>
#include <link.h>
#include <avl_new.h>
#include "ds_color.h"
#include "ds_sph.h"
#include "ds_file.h"
#include "ds_gua.h"

//----------------------------------------------------------------------------------------------------------
void ds_label_update(DS_LABEL *label)
//----------------------------------------------------------------------------------------------------------
{
	// set the labels font parameters


	// update the offsets based on the specific font - approximate values
	switch ((unsigned int)label->font) {
	case GLUT_BITMAP_8_BY_13:			label->xOffset = (float)-4.0; label->yOffset = (float)-5.0; break;
	case GLUT_BITMAP_9_BY_15:			label->xOffset = (float)-4.5; label->yOffset = (float)-6.0; break;
	case GLUT_BITMAP_TIMES_ROMAN_10:
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.5; // -2.0;
		label->yOffset = (float)-2.0;
		break;
	case GLUT_BITMAP_TIMES_ROMAN_24:
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.25; // -2.0;
		label->yOffset = (float)-4.5;
		break;
	case GLUT_BITMAP_HELVETICA_10:
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.5; // -2.0;
		label->yOffset = (float)-2.0;
		break;
	case GLUT_BITMAP_HELVETICA_12:
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.5; // -2.0;
		label->yOffset = (float)-2.0;
		break;
	case GLUT_BITMAP_HELVETICA_18:
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.25; // -2.0;
		label->yOffset = (float)-3.0;
		break;
	default:
		// default
		label->font = GLUT_BITMAP_TIMES_ROMAN_24;
		label->xOffset = glutBitmapWidth(label->font, '9') / -2.5; // -2.0;
		label->yOffset = (float)-4.5;
	}
}

//----------------------------------------------------------------------------------------------------------
void ds_label_set(DS_LABEL *label, void *font, DS_COLOR *color)
//----------------------------------------------------------------------------------------------------------
{
	// set the labels font parameters


	// update the offsets based on the specific font - approximate values
	switch ((unsigned int)font) {
	case GLUT_BITMAP_8_BY_13:			label->font = font; label->xOffset = (float)-4.0; label->yOffset = (float)-5.0; break;
	case GLUT_BITMAP_9_BY_15:			label->font = font; label->xOffset = (float)-4.5; label->yOffset = (float)-6.0; break;
	case GLUT_BITMAP_TIMES_ROMAN_10:
		label->font = font;
		label->xOffset = glutBitmapWidth(font, '9') / -2.5; // -2.0;
		label->yOffset = (float)-2.0; 
		break;
	case GLUT_BITMAP_TIMES_ROMAN_24:	label->font = font; label->xOffset = (float)-5.0; label->yOffset = (float)-7.0; break;
	case GLUT_BITMAP_HELVETICA_10:		label->font = font; label->xOffset = (float)-3.0; label->yOffset = (float)-4.0; break;
	case GLUT_BITMAP_HELVETICA_12:		label->font = font; label->xOffset = (float)-6.0; label->yOffset = (float)-5.0; break;
	case GLUT_BITMAP_HELVETICA_18:		label->font = font; label->xOffset = (float)-6.0; label->yOffset = (float)-7.0; break;
	default:
		// default
		label->font = GLUT_BITMAP_TIMES_ROMAN_24;
		label->color = *color;
		label->xOffset = -5.0;
		label->yOffset = -7.0;
	}
}

//----------------------------------------------------------------------------------------------------------
void ds_label_draw(DS_LABEL *label, float x, float y, float z, float nz, char *string)
//----------------------------------------------------------------------------------------------------------
{
	// Draw a label on the screen
	int				len;
	int				i;
	static char		buf[12];
	static GLubyte	bitmap[10];

	if (nz < 0.6) return; // don't draw if normal representing face is too oblique to view 

	glColor3f(label->color.r, label->color.g, label->color.b);	// set label specific color
	glRasterPos3f(x, y, z);										// initialize raster position
	len = (int)strlen(string);									// determine length of string

	// move the raster position so that string is centered around the position
	glBitmap((GLsizei)0, (GLsizei)0, (GLfloat)0, (GLfloat)0, (GLfloat)(len*label->xOffset), (GLfloat)(len*label->yOffset), &bitmap[0]);
	for (i = 0; i < len; i++) // draw each character of the string in the appropriate font
		glutBitmapCharacter(label->font, string[i]);
}

//----------------------------------------------------------------------------------------------------------
void ds_label_draw_id(DS_LABEL *label, float x, float y, float z, float nz, int id)
//----------------------------------------------------------------------------------------------------------
{
	// Draw a label on the screen
	int				len;
	int				i;
	static char		buf[12];
	static GLubyte	bitmap[10];

	if (nz < 0.6) return; // don't draw if normal representing face is too oblique to view 

	glColor3f(label->color.r, label->color.g, label->color.b);	// set label specific color
	glRasterPos3f(x, y, z);										// initialize raster position
	sprintf(buf, "%d", id);
	len = (int)strlen(buf);										// determine length of string
																// move the raster position so that string is centered around the position
	glBitmap((GLsizei)0, (GLsizei)0, (GLfloat)0, (GLfloat)0, (GLfloat)(len*label->xOffset), (GLfloat)(len*label->yOffset), &bitmap[0]);
	for (i = 0; i < len; i++) // draw each character of the string in the appropriate font
		glutBitmapCharacter(label->font, buf[i]);
}
