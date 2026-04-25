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
#ifndef QINDIEGL_D3D_EXTENSION_H
#define QINDIEGL_D3D_EXTENSION_H

extern void D3DExtension_BuildExtensionsString();

OPENGL_API void glActiveTexture(GLenum texture);
OPENGL_API void glClientActiveTexture(GLenum texture);
OPENGL_API void glMultiTexCoord1s( GLenum target, GLshort s );
OPENGL_API void glMultiTexCoord1i( GLenum target, GLint s );
OPENGL_API void glMultiTexCoord1f( GLenum target, GLfloat s );
OPENGL_API void glMultiTexCoord1d( GLenum target, GLdouble s );
OPENGL_API void glMultiTexCoord2s( GLenum target, GLshort s, GLshort t );
OPENGL_API void glMultiTexCoord2i( GLenum target, GLint s, GLint t );
OPENGL_API void glMultiTexCoord2f( GLenum target, GLfloat s, GLfloat t );
OPENGL_API void glMultiTexCoord2d( GLenum target, GLdouble s, GLdouble t );
OPENGL_API void glMultiTexCoord3s( GLenum target, GLshort s, GLshort t, GLshort r );
OPENGL_API void glMultiTexCoord3i( GLenum target, GLint s, GLint t, GLint r );
OPENGL_API void glMultiTexCoord3f( GLenum target, GLfloat s, GLfloat t, GLfloat r );
OPENGL_API void glMultiTexCoord3d( GLenum target, GLdouble s, GLdouble t, GLdouble r );
OPENGL_API void glMultiTexCoord4s( GLenum target, GLshort s, GLshort t, GLshort r, GLshort q );
OPENGL_API void glMultiTexCoord4i( GLenum target, GLint s, GLint t, GLint r, GLint q );
OPENGL_API void glMultiTexCoord4f( GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q );
OPENGL_API void glMultiTexCoord4d( GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q );
OPENGL_API void glMultiTexCoord1sv( GLenum target, const GLshort *v );
OPENGL_API void glMultiTexCoord1iv( GLenum target, const GLint *v );
OPENGL_API void glMultiTexCoord1fv( GLenum target, const GLfloat *v );
OPENGL_API void glMultiTexCoord1dv( GLenum target, const GLdouble *v );
OPENGL_API void glMultiTexCoord2sv( GLenum target, const GLshort *v );
OPENGL_API void glMultiTexCoord2iv( GLenum target, const GLint *v );
OPENGL_API void glMultiTexCoord2fv( GLenum target, const GLfloat *v );
OPENGL_API void glMultiTexCoord2dv( GLenum target, const GLdouble *v );
OPENGL_API void glMultiTexCoord3sv( GLenum target, const GLshort *v );
OPENGL_API void glMultiTexCoord3iv( GLenum target, const GLint *v );
OPENGL_API void glMultiTexCoord3fv( GLenum target, const GLfloat *v );
OPENGL_API void glMultiTexCoord3dv( GLenum target, const GLdouble *v );
OPENGL_API void glMultiTexCoord4sv( GLenum target, const GLshort *v );
OPENGL_API void glMultiTexCoord4iv( GLenum target, const GLint *v );
OPENGL_API void glMultiTexCoord4fv( GLenum target, const GLfloat *v );
OPENGL_API void glMultiTexCoord4dv( GLenum target, const GLdouble *v );
OPENGL_API void glActiveStencilFace(GLenum face);
OPENGL_API void glDeleteTextures(GLsizei n, const GLuint *textures);
OPENGL_API void glGenTextures(GLsizei n, GLuint *textures);
OPENGL_API GLboolean glIsTexture(GLuint texture);
OPENGL_API void glBindTexture(GLenum target, GLuint texture);
OPENGL_API GLboolean glAreTexturesResident(GLsizei n,  const GLuint *textures,  GLboolean *residences);
OPENGL_API void glPrioritizeTextures(GLsizei n,  const GLuint *textures,  const GLclampf *priorities);
OPENGL_API void glTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
OPENGL_API void glTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
OPENGL_API void glCopyTexImage3D(GLenum target,  GLint level,  GLenum internalFormat,  GLint x,  GLint y,  GLint z,  GLsizei width,  GLsizei height,  GLsizei depth,  GLint border);
OPENGL_API void glCopyTexSubImage3D(GLenum target,  GLint level,  GLint xoffset,  GLint yoffset,  GLint zoffset,  GLint x,  GLint y,  GLint z,  GLsizei width,  GLsizei height,  GLsizei depth);
OPENGL_API BOOL wglSwapInterval(int interval);
OPENGL_API int wglGetSwapInterval();
OPENGL_API void glSelectTexture(GLenum texture);
OPENGL_API void glMTexCoord2f( GLenum target, GLfloat s, GLfloat t );
OPENGL_API void glMTexCoord2fv( GLenum target, const GLfloat *v );
OPENGL_API void glLoadTransposeMatrixf(const GLfloat *m);
OPENGL_API void glLoadTransposeMatrixd(const GLdouble *m);
OPENGL_API void glMultTransposeMatrixf(const GLfloat *m);
OPENGL_API void glMultTransposeMatrixd(const GLdouble *m);
OPENGL_API void glFogCoordd( GLdouble coord );
OPENGL_API void glFogCoordf( GLfloat coord );
OPENGL_API void glFogCoorddv( GLdouble *coord );
OPENGL_API void glFogCoordfv( GLfloat *coord );
OPENGL_API void glFogCoordPointer(GLenum type,  GLsizei stride,  const GLvoid *pointer);
OPENGL_API void glSecondaryColor3b(GLbyte red,  GLbyte green,  GLbyte blue);
OPENGL_API void glSecondaryColor3bv(const GLbyte *v);
OPENGL_API void glSecondaryColor3d(GLdouble red,  GLdouble green,  GLdouble blue);
OPENGL_API void glSecondaryColor3dv(const GLdouble *v);
OPENGL_API void glSecondaryColor3f(GLfloat red,  GLfloat green,  GLfloat blue);
OPENGL_API void glSecondaryColor3fv(const GLfloat *v);
OPENGL_API void glSecondaryColor3i(GLint red,  GLint green,  GLint blue);
OPENGL_API void glSecondaryColor3iv(const GLint *v);
OPENGL_API void glSecondaryColor3s(GLshort red,  GLshort green,  GLshort blue);
OPENGL_API void glSecondaryColor3sv(const GLshort *v);
OPENGL_API void glSecondaryColor3ub(GLubyte red,  GLubyte green,  GLubyte blue);
OPENGL_API void glSecondaryColor3ubv(const GLubyte *v);
OPENGL_API void glSecondaryColor3ui(GLuint red,  GLuint green,  GLuint blue);
OPENGL_API void glSecondaryColor3uiv(const GLuint *v);
OPENGL_API void glSecondaryColor3us(GLushort red,  GLushort green,  GLushort blue);
OPENGL_API void glSecondaryColor3usv(const GLushort *v);
OPENGL_API void glSecondaryColorPointer(GLint size,  GLenum type,  GLsizei stride,  const GLvoid *pointer);
OPENGL_API void glDrawRangeElements( GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices );
OPENGL_API void glMultiDrawArrays( GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount );
OPENGL_API void glMultiDrawElements( GLenum mode, GLsizei *count, GLenum type, const GLvoid **indices, GLsizei primcount );
OPENGL_API void glLockArrays( GLint first, GLsizei count );
OPENGL_API void glUnlockArrays();
OPENGL_API void glBlendColor( GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha );
OPENGL_API void glBlendEquation( GLenum mode );
OPENGL_API void glCompressedTexImage1D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glCompressedTexImage2D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glCompressedTexImage3D(GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glCompressedTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glCompressedTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glCompressedTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *pixels);
OPENGL_API void glGetCompressedTexImage(GLenum target, GLint level, GLvoid *img);

#endif //QINDIEGL_D3D_EXTENSION_H