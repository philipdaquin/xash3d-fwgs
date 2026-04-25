/***************************************************************************
* Copyright (C) 2011-2016, Crystice Softworks.
* 
* This file is part of QindieGL source code.
* Please note that QindieGL is not driver, it's emulator.
* 
* QindieGL source code is free software; you can redistribute it and/or 
* modify it under the terms of the GNU General Public License as 
* published by the Free Software Foundation; either version 2 of 
* the License, or (at your option) any later version.
* 
* QindieGL source code is distributed in the hope that it will be 
* useful, but WITHOUT ANY WARRANTY; without even the implied 
* warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
* See the GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software 
* Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
***************************************************************************/
#include "d3d_wrapper.hpp"
#include "d3d_global.hpp"
#include "d3d_state.hpp"
#include "d3d_utils.hpp"

//==================================================================================
// Select & Feedback modes
//----------------------------------------------------------------------------------
// TODO: emulate them in software
//==================================================================================

OPENGL_API GLint glRenderMode( GLenum mode )
{
	switch (mode) {
	default:
	case GL_RENDER:
		return 0;
	case GL_SELECT:
		logPrintf("WARNING: glRenderMode: GL_SELECT is not supported\n");
		return 0;
	case GL_FEEDBACK:
		logPrintf("WARNING: glRenderMode: GL_FEEDBACK is not supported\n");
		return 0;
	}
}

OPENGL_API void glFeedbackBuffer( GLsizei, GLenum, GLfloat* )
{
	logPrintf("WARNING: unimplemented function - glFeedbackBuffer\n");
}
OPENGL_API void glPassThrough( GLfloat )
{
	logPrintf("WARNING: unimplemented function - glPassThrough\n");
}

OPENGL_API void glSelectBuffer( GLsizei, GLuint* )
{
	logPrintf("WARNING: unimplemented function - glSelectBuffer\n");
}
OPENGL_API void glInitNames()
{
	logPrintf("WARNING: unimplemented function - glInitNames\n");
}
OPENGL_API void glLoadName( GLuint )
{
	logPrintf("WARNING: unimplemented function - glLoadName\n");
}
OPENGL_API void glPushName( GLuint )
{
	logPrintf("WARNING: unimplemented function - glPushName\n");
}
OPENGL_API void glPopName()
{
	logPrintf("WARNING: unimplemented function - glPopName\n");
}

