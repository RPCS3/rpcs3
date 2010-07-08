#ifndef _GLPROCS_H_
#define _GLPROCS_H_

/*
** GLprocs utility for getting function addresses for OpenGL(R) 1.2,
** OpenGL 1.3, OpenGL 1.4, OpenGL 1.5 and OpenGL extension functions.
**
** SGI FREE SOFTWARE LICENSE B (Version 2.0, Sept. 18, 2008)
** Copyright (C) 1991-2000 Silicon Graphics, Inc. All Rights Reserved.
**
** Permission is hereby granted, free of charge, to any person obtaining a
** copy of this software and associated documentation files (the "Software"),
** to deal in the Software without restriction, including without limitation
** the rights to use, copy, modify, merge, publish, distribute, sublicense,
** and/or sell copies of the Software, and to permit persons to whom the
** Software is furnished to do so, subject to the following conditions:
**
** The above copyright notice including the dates of first publication and
** either this permission notice or a reference to
** http://oss.sgi.com/projects/FreeB/
** shall be included in all copies or substantial portions of the Software.
**
** THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
** OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
** FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
** SILICON GRAPHICS, INC. BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
** WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
** OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
** SOFTWARE.
**
** Except as contained in this notice, the name of Silicon Graphics, Inc.
** shall not be used in advertising or otherwise to promote the sale, use or
** other dealings in this Software without prior written authorization from
** Silicon Graphics, Inc.
**
** Additional Notice Provisions: This software was created using the
** OpenGL(R) version 1.2.1 Sample Implementation published by SGI, but has
** not been independently verified as being compliant with the OpenGL(R)
** version 1.2.1 Specification.
**
** Initial version of glprocs.{c,h} contributed by Intel(R) Corporation.
*/

#ifdef _WIN32
  #include <GL/glext.h>
  #include <GL/wglext.h>
#else
  #include <GL/glext.h>
#endif

#ifndef _WIN32 /* non-Windows environment */
  #ifndef APIENTRY
    #define APIENTRY
  #endif
  #ifdef __GNUC__
    #define _inline __inline__
  #else
    #define _inline
  #endif
#endif

#ifdef __cplusplus
extern "C" {
#endif

/* Structure of all OpenGL {1.2, 1.3, 1.4, 1.5}, GL extension procs.*/

typedef struct {
  void (APIENTRY *BlendColor) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
  void (APIENTRY *BlendEquation) (GLenum mode);
  void (APIENTRY *DrawRangeElements) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
  void (APIENTRY *ColorTable) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
  void (APIENTRY *ColorTableParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *ColorTableParameteriv) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *CopyColorTable) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
  void (APIENTRY *GetColorTable) (GLenum target, GLenum format, GLenum type, GLvoid *table);
  void (APIENTRY *GetColorTableParameterfv) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetColorTableParameteriv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *ColorSubTable) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
  void (APIENTRY *CopyColorSubTable) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
  void (APIENTRY *ConvolutionFilter1D) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
  void (APIENTRY *ConvolutionFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
  void (APIENTRY *ConvolutionParameterf) (GLenum target, GLenum pname, GLfloat params);
  void (APIENTRY *ConvolutionParameterfv) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *ConvolutionParameteri) (GLenum target, GLenum pname, GLint params);
  void (APIENTRY *ConvolutionParameteriv) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *CopyConvolutionFilter1D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
  void (APIENTRY *CopyConvolutionFilter2D) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
  void (APIENTRY *GetConvolutionFilter) (GLenum target, GLenum format, GLenum type, GLvoid *image);
  void (APIENTRY *GetConvolutionParameterfv) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetConvolutionParameteriv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetSeparableFilter) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
  void (APIENTRY *SeparableFilter2D) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
  void (APIENTRY *GetHistogram) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
  void (APIENTRY *GetHistogramParameterfv) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetHistogramParameteriv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetMinmax) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
  void (APIENTRY *GetMinmaxParameterfv) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetMinmaxParameteriv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *Histogram) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
  void (APIENTRY *Minmax) (GLenum target, GLenum internalformat, GLboolean sink);
  void (APIENTRY *ResetHistogram) (GLenum target);
  void (APIENTRY *ResetMinmax) (GLenum target);
  void (APIENTRY *TexImage3D) (GLenum target, GLint level, GLint internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *TexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *CopyTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
  void (APIENTRY *ActiveTexture) (GLenum texture);
  void (APIENTRY *ClientActiveTexture) (GLenum texture);
  void (APIENTRY *MultiTexCoord1d) (GLenum target, GLdouble s);
  void (APIENTRY *MultiTexCoord1dv) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord1f) (GLenum target, GLfloat s);
  void (APIENTRY *MultiTexCoord1fv) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord1i) (GLenum target, GLint s);
  void (APIENTRY *MultiTexCoord1iv) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord1s) (GLenum target, GLshort s);
  void (APIENTRY *MultiTexCoord1sv) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord2d) (GLenum target, GLdouble s, GLdouble t);
  void (APIENTRY *MultiTexCoord2dv) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord2f) (GLenum target, GLfloat s, GLfloat t);
  void (APIENTRY *MultiTexCoord2fv) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord2i) (GLenum target, GLint s, GLint t);
  void (APIENTRY *MultiTexCoord2iv) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord2s) (GLenum target, GLshort s, GLshort t);
  void (APIENTRY *MultiTexCoord2sv) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord3d) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
  void (APIENTRY *MultiTexCoord3dv) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord3f) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
  void (APIENTRY *MultiTexCoord3fv) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord3i) (GLenum target, GLint s, GLint t, GLint r);
  void (APIENTRY *MultiTexCoord3iv) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord3s) (GLenum target, GLshort s, GLshort t, GLshort r);
  void (APIENTRY *MultiTexCoord3sv) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord4d) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
  void (APIENTRY *MultiTexCoord4dv) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord4f) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
  void (APIENTRY *MultiTexCoord4fv) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord4i) (GLenum target, GLint s, GLint t, GLint r, GLint q);
  void (APIENTRY *MultiTexCoord4iv) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord4s) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
  void (APIENTRY *MultiTexCoord4sv) (GLenum target, const GLshort *v);
  void (APIENTRY *LoadTransposeMatrixf) (const GLfloat *m);
  void (APIENTRY *LoadTransposeMatrixd) (const GLdouble *m);
  void (APIENTRY *MultTransposeMatrixf) (const GLfloat *m);
  void (APIENTRY *MultTransposeMatrixd) (const GLdouble *m);
  void (APIENTRY *SampleCoverage) (GLclampf value, GLboolean invert);
  void (APIENTRY *CompressedTexImage3D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexImage2D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexImage1D) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage3D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage2D) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage1D) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *GetCompressedTexImage) (GLenum target, GLint level, GLvoid *img);
  void (APIENTRY *BlendFuncSeparate) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
  void (APIENTRY *FogCoordf) (GLfloat coord);
  void (APIENTRY *FogCoordfv) (const GLfloat *coord);
  void (APIENTRY *FogCoordd) (GLdouble coord);
  void (APIENTRY *FogCoorddv) (const GLdouble *coord);
  void (APIENTRY *FogCoordPointer) (GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *MultiDrawArrays) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
  void (APIENTRY *MultiDrawElements) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
  void (APIENTRY *PointParameterf) (GLenum pname, GLfloat param);
  void (APIENTRY *PointParameterfv) (GLenum pname, const GLfloat *params);
  void (APIENTRY *PointParameteri) (GLenum pname, GLint param);
  void (APIENTRY *PointParameteriv) (GLenum pname, const GLint *params);
  void (APIENTRY *SecondaryColor3b) (GLbyte red, GLbyte green, GLbyte blue);
  void (APIENTRY *SecondaryColor3bv) (const GLbyte *v);
  void (APIENTRY *SecondaryColor3d) (GLdouble red, GLdouble green, GLdouble blue);
  void (APIENTRY *SecondaryColor3dv) (const GLdouble *v);
  void (APIENTRY *SecondaryColor3f) (GLfloat red, GLfloat green, GLfloat blue);
  void (APIENTRY *SecondaryColor3fv) (const GLfloat *v);
  void (APIENTRY *SecondaryColor3i) (GLint red, GLint green, GLint blue);
  void (APIENTRY *SecondaryColor3iv) (const GLint *v);
  void (APIENTRY *SecondaryColor3s) (GLshort red, GLshort green, GLshort blue);
  void (APIENTRY *SecondaryColor3sv) (const GLshort *v);
  void (APIENTRY *SecondaryColor3ub) (GLubyte red, GLubyte green, GLubyte blue);
  void (APIENTRY *SecondaryColor3ubv) (const GLubyte *v);
  void (APIENTRY *SecondaryColor3ui) (GLuint red, GLuint green, GLuint blue);
  void (APIENTRY *SecondaryColor3uiv) (const GLuint *v);
  void (APIENTRY *SecondaryColor3us) (GLushort red, GLushort green, GLushort blue);
  void (APIENTRY *SecondaryColor3usv) (const GLushort *v);
  void (APIENTRY *SecondaryColorPointer) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *WindowPos2d) (GLdouble x, GLdouble y);
  void (APIENTRY *WindowPos2dv) (const GLdouble *v);
  void (APIENTRY *WindowPos2f) (GLfloat x, GLfloat y);
  void (APIENTRY *WindowPos2fv) (const GLfloat *v);
  void (APIENTRY *WindowPos2i) (GLint x, GLint y);
  void (APIENTRY *WindowPos2iv) (const GLint *v);
  void (APIENTRY *WindowPos2s) (GLshort x, GLshort y);
  void (APIENTRY *WindowPos2sv) (const GLshort *v);
  void (APIENTRY *WindowPos3d) (GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *WindowPos3dv) (const GLdouble *v);
  void (APIENTRY *WindowPos3f) (GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *WindowPos3fv) (const GLfloat *v);
  void (APIENTRY *WindowPos3i) (GLint x, GLint y, GLint z);
  void (APIENTRY *WindowPos3iv) (const GLint *v);
  void (APIENTRY *WindowPos3s) (GLshort x, GLshort y, GLshort z);
  void (APIENTRY *WindowPos3sv) (const GLshort *v);
  void (APIENTRY *GenQueries) (GLsizei n, GLuint *ids);
  void (APIENTRY *DeleteQueries) (GLsizei n, const GLuint *ids);
  GLboolean (APIENTRY *IsQuery) (GLuint id);
  void (APIENTRY *BeginQuery) (GLenum target, GLuint id);
  void (APIENTRY *EndQuery) (GLenum target);
  void (APIENTRY *GetQueryiv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetQueryObjectiv) (GLuint id, GLenum pname, GLint *params);
  void (APIENTRY *GetQueryObjectuiv) (GLuint id, GLenum pname, GLuint *params);
  void (APIENTRY *BindBuffer) (GLenum target, GLuint buffer);
  void (APIENTRY *DeleteBuffers) (GLsizei n, const GLuint *buffers);
  void (APIENTRY *GenBuffers) (GLsizei n, GLuint *buffers);
  GLboolean (APIENTRY *IsBuffer) (GLuint buffer);
  void (APIENTRY *BufferData) (GLenum target, GLsizeiptr size, const GLvoid *data, GLenum usage);
  void (APIENTRY *BufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
  void (APIENTRY *GetBufferSubData) (GLenum target, GLintptr offset, GLsizeiptr size, GLvoid *data);
  GLvoid* (APIENTRY *MapBuffer) (GLenum target, GLenum access);
  GLboolean (APIENTRY *UnmapBuffer) (GLenum target);
  void (APIENTRY *GetBufferParameteriv) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetBufferPointerv) (GLenum target, GLenum pname, GLvoid* *params);
  void (APIENTRY *ActiveTextureARB) (GLenum texture);
  void (APIENTRY *ClientActiveTextureARB) (GLenum texture);
  void (APIENTRY *MultiTexCoord1dARB) (GLenum target, GLdouble s);
  void (APIENTRY *MultiTexCoord1dvARB) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord1fARB) (GLenum target, GLfloat s);
  void (APIENTRY *MultiTexCoord1fvARB) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord1iARB) (GLenum target, GLint s);
  void (APIENTRY *MultiTexCoord1ivARB) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord1sARB) (GLenum target, GLshort s);
  void (APIENTRY *MultiTexCoord1svARB) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord2dARB) (GLenum target, GLdouble s, GLdouble t);
  void (APIENTRY *MultiTexCoord2dvARB) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord2fARB) (GLenum target, GLfloat s, GLfloat t);
  void (APIENTRY *MultiTexCoord2fvARB) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord2iARB) (GLenum target, GLint s, GLint t);
  void (APIENTRY *MultiTexCoord2ivARB) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord2sARB) (GLenum target, GLshort s, GLshort t);
  void (APIENTRY *MultiTexCoord2svARB) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord3dARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r);
  void (APIENTRY *MultiTexCoord3dvARB) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord3fARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r);
  void (APIENTRY *MultiTexCoord3fvARB) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord3iARB) (GLenum target, GLint s, GLint t, GLint r);
  void (APIENTRY *MultiTexCoord3ivARB) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord3sARB) (GLenum target, GLshort s, GLshort t, GLshort r);
  void (APIENTRY *MultiTexCoord3svARB) (GLenum target, const GLshort *v);
  void (APIENTRY *MultiTexCoord4dARB) (GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q);
  void (APIENTRY *MultiTexCoord4dvARB) (GLenum target, const GLdouble *v);
  void (APIENTRY *MultiTexCoord4fARB) (GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q);
  void (APIENTRY *MultiTexCoord4fvARB) (GLenum target, const GLfloat *v);
  void (APIENTRY *MultiTexCoord4iARB) (GLenum target, GLint s, GLint t, GLint r, GLint q);
  void (APIENTRY *MultiTexCoord4ivARB) (GLenum target, const GLint *v);
  void (APIENTRY *MultiTexCoord4sARB) (GLenum target, GLshort s, GLshort t, GLshort r, GLshort q);
  void (APIENTRY *MultiTexCoord4svARB) (GLenum target, const GLshort *v);
  void (APIENTRY *LoadTransposeMatrixfARB) (const GLfloat *m);
  void (APIENTRY *LoadTransposeMatrixdARB) (const GLdouble *m);
  void (APIENTRY *MultTransposeMatrixfARB) (const GLfloat *m);
  void (APIENTRY *MultTransposeMatrixdARB) (const GLdouble *m);
  void (APIENTRY *SampleCoverageARB) (GLclampf value, GLboolean invert);
  void (APIENTRY *CompressedTexImage3DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexImage2DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexImage1DARB) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLint border, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage3DARB) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage2DARB) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *CompressedTexSubImage1DARB) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLsizei imageSize, const GLvoid *data);
  void (APIENTRY *GetCompressedTexImageARB) (GLenum target, GLint level, GLvoid *img);
  void (APIENTRY *PointParameterfARB) (GLenum pname, GLfloat param);
  void (APIENTRY *PointParameterfvARB) (GLenum pname, const GLfloat *params);
  void (APIENTRY *WeightbvARB) (GLint size, const GLbyte *weights);
  void (APIENTRY *WeightsvARB) (GLint size, const GLshort *weights);
  void (APIENTRY *WeightivARB) (GLint size, const GLint *weights);
  void (APIENTRY *WeightfvARB) (GLint size, const GLfloat *weights);
  void (APIENTRY *WeightdvARB) (GLint size, const GLdouble *weights);
  void (APIENTRY *WeightubvARB) (GLint size, const GLubyte *weights);
  void (APIENTRY *WeightusvARB) (GLint size, const GLushort *weights);
  void (APIENTRY *WeightuivARB) (GLint size, const GLuint *weights);
  void (APIENTRY *WeightPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *VertexBlendARB) (GLint count);
  void (APIENTRY *CurrentPaletteMatrixARB) (GLint index);
  void (APIENTRY *MatrixIndexubvARB) (GLint size, const GLubyte *indices);
  void (APIENTRY *MatrixIndexusvARB) (GLint size, const GLushort *indices);
  void (APIENTRY *MatrixIndexuivARB) (GLint size, const GLuint *indices);
  void (APIENTRY *MatrixIndexPointerARB) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *WindowPos2dARB) (GLdouble x, GLdouble y);
  void (APIENTRY *WindowPos2dvARB) (const GLdouble *v);
  void (APIENTRY *WindowPos2fARB) (GLfloat x, GLfloat y);
  void (APIENTRY *WindowPos2fvARB) (const GLfloat *v);
  void (APIENTRY *WindowPos2iARB) (GLint x, GLint y);
  void (APIENTRY *WindowPos2ivARB) (const GLint *v);
  void (APIENTRY *WindowPos2sARB) (GLshort x, GLshort y);
  void (APIENTRY *WindowPos2svARB) (const GLshort *v);
  void (APIENTRY *WindowPos3dARB) (GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *WindowPos3dvARB) (const GLdouble *v);
  void (APIENTRY *WindowPos3fARB) (GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *WindowPos3fvARB) (const GLfloat *v);
  void (APIENTRY *WindowPos3iARB) (GLint x, GLint y, GLint z);
  void (APIENTRY *WindowPos3ivARB) (const GLint *v);
  void (APIENTRY *WindowPos3sARB) (GLshort x, GLshort y, GLshort z);
  void (APIENTRY *WindowPos3svARB) (const GLshort *v);
  void (APIENTRY *VertexAttrib1dARB) (GLuint index, GLdouble x);
  void (APIENTRY *VertexAttrib1dvARB) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib1fARB) (GLuint index, GLfloat x);
  void (APIENTRY *VertexAttrib1fvARB) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib1sARB) (GLuint index, GLshort x);
  void (APIENTRY *VertexAttrib1svARB) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib2dARB) (GLuint index, GLdouble x, GLdouble y);
  void (APIENTRY *VertexAttrib2dvARB) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib2fARB) (GLuint index, GLfloat x, GLfloat y);
  void (APIENTRY *VertexAttrib2fvARB) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib2sARB) (GLuint index, GLshort x, GLshort y);
  void (APIENTRY *VertexAttrib2svARB) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib3dARB) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *VertexAttrib3dvARB) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib3fARB) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *VertexAttrib3fvARB) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib3sARB) (GLuint index, GLshort x, GLshort y, GLshort z);
  void (APIENTRY *VertexAttrib3svARB) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib4NbvARB) (GLuint index, const GLbyte *v);
  void (APIENTRY *VertexAttrib4NivARB) (GLuint index, const GLint *v);
  void (APIENTRY *VertexAttrib4NsvARB) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib4NubARB) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
  void (APIENTRY *VertexAttrib4NubvARB) (GLuint index, const GLubyte *v);
  void (APIENTRY *VertexAttrib4NuivARB) (GLuint index, const GLuint *v);
  void (APIENTRY *VertexAttrib4NusvARB) (GLuint index, const GLushort *v);
  void (APIENTRY *VertexAttrib4bvARB) (GLuint index, const GLbyte *v);
  void (APIENTRY *VertexAttrib4dARB) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *VertexAttrib4dvARB) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib4fARB) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *VertexAttrib4fvARB) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib4ivARB) (GLuint index, const GLint *v);
  void (APIENTRY *VertexAttrib4sARB) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
  void (APIENTRY *VertexAttrib4svARB) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib4ubvARB) (GLuint index, const GLubyte *v);
  void (APIENTRY *VertexAttrib4uivARB) (GLuint index, const GLuint *v);
  void (APIENTRY *VertexAttrib4usvARB) (GLuint index, const GLushort *v);
  void (APIENTRY *VertexAttribPointerARB) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *EnableVertexAttribArrayARB) (GLuint index);
  void (APIENTRY *DisableVertexAttribArrayARB) (GLuint index);
  void (APIENTRY *ProgramStringARB) (GLenum target, GLenum format, GLsizei len, const GLvoid *string);
  void (APIENTRY *BindProgramARB) (GLenum target, GLuint program);
  void (APIENTRY *DeleteProgramsARB) (GLsizei n, const GLuint *programs);
  void (APIENTRY *GenProgramsARB) (GLsizei n, GLuint *programs);
  void (APIENTRY *ProgramEnvParameter4dARB) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *ProgramEnvParameter4dvARB) (GLenum target, GLuint index, const GLdouble *params);
  void (APIENTRY *ProgramEnvParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *ProgramEnvParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
  void (APIENTRY *ProgramLocalParameter4dARB) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *ProgramLocalParameter4dvARB) (GLenum target, GLuint index, const GLdouble *params);
  void (APIENTRY *ProgramLocalParameter4fARB) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *ProgramLocalParameter4fvARB) (GLenum target, GLuint index, const GLfloat *params);
  void (APIENTRY *GetProgramEnvParameterdvARB) (GLenum target, GLuint index, GLdouble *params);
  void (APIENTRY *GetProgramEnvParameterfvARB) (GLenum target, GLuint index, GLfloat *params);
  void (APIENTRY *GetProgramLocalParameterdvARB) (GLenum target, GLuint index, GLdouble *params);
  void (APIENTRY *GetProgramLocalParameterfvARB) (GLenum target, GLuint index, GLfloat *params);
  void (APIENTRY *GetProgramivARB) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetProgramStringARB) (GLenum target, GLenum pname, GLvoid *string);
  void (APIENTRY *GetVertexAttribdvARB) (GLuint index, GLenum pname, GLdouble *params);
  void (APIENTRY *GetVertexAttribfvARB) (GLuint index, GLenum pname, GLfloat *params);
  void (APIENTRY *GetVertexAttribivARB) (GLuint index, GLenum pname, GLint *params);
  void (APIENTRY *GetVertexAttribPointervARB) (GLuint index, GLenum pname, GLvoid* *pointer);
  GLboolean (APIENTRY *IsProgramARB) (GLuint program);
  void (APIENTRY *BindBufferARB) (GLenum target, GLuint buffer);
  void (APIENTRY *DeleteBuffersARB) (GLsizei n, const GLuint *buffers);
  void (APIENTRY *GenBuffersARB) (GLsizei n, GLuint *buffers);
  GLboolean (APIENTRY *IsBufferARB) (GLuint buffer);
  void (APIENTRY *BufferDataARB) (GLenum target, GLsizeiptrARB size, const GLvoid *data, GLenum usage);
  void (APIENTRY *BufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, const GLvoid *data);
  void (APIENTRY *GetBufferSubDataARB) (GLenum target, GLintptrARB offset, GLsizeiptrARB size, GLvoid *data);
  GLvoid* (APIENTRY *MapBufferARB) (GLenum target, GLenum access);
  GLboolean (APIENTRY *UnmapBufferARB) (GLenum target);
  void (APIENTRY *GetBufferParameterivARB) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetBufferPointervARB) (GLenum target, GLenum pname, GLvoid* *params);
  void (APIENTRY *GenQueriesARB) (GLsizei n, GLuint *ids);
  void (APIENTRY *DeleteQueriesARB) (GLsizei n, const GLuint *ids);
  GLboolean (APIENTRY *IsQueryARB) (GLuint id);
  void (APIENTRY *BeginQueryARB) (GLenum target, GLuint id);
  void (APIENTRY *EndQueryARB) (GLenum target);
  void (APIENTRY *GetQueryivARB) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetQueryObjectivARB) (GLuint id, GLenum pname, GLint *params);
  void (APIENTRY *GetQueryObjectuivARB) (GLuint id, GLenum pname, GLuint *params);
  void (APIENTRY *DeleteObjectARB) (GLhandleARB obj);
  GLhandleARB (APIENTRY *GetHandleARB) (GLenum pname);
  void (APIENTRY *DetachObjectARB) (GLhandleARB containerObj, GLhandleARB attachedObj);
  GLhandleARB (APIENTRY *CreateShaderObjectARB) (GLenum shaderType);
  void (APIENTRY *ShaderSourceARB) (GLhandleARB shaderObj, GLsizei count, const GLcharARB* *string, const GLint *length);
  void (APIENTRY *CompileShaderARB) (GLhandleARB shaderObj);
  GLhandleARB (APIENTRY *CreateProgramObjectARB) (void);
  void (APIENTRY *AttachObjectARB) (GLhandleARB containerObj, GLhandleARB obj);
  void (APIENTRY *LinkProgramARB) (GLhandleARB programObj);
  void (APIENTRY *UseProgramObjectARB) (GLhandleARB programObj);
  void (APIENTRY *ValidateProgramARB) (GLhandleARB programObj);
  void (APIENTRY *Uniform1fARB) (GLint location, GLfloat v0);
  void (APIENTRY *Uniform2fARB) (GLint location, GLfloat v0, GLfloat v1);
  void (APIENTRY *Uniform3fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2);
  void (APIENTRY *Uniform4fARB) (GLint location, GLfloat v0, GLfloat v1, GLfloat v2, GLfloat v3);
  void (APIENTRY *Uniform1iARB) (GLint location, GLint v0);
  void (APIENTRY *Uniform2iARB) (GLint location, GLint v0, GLint v1);
  void (APIENTRY *Uniform3iARB) (GLint location, GLint v0, GLint v1, GLint v2);
  void (APIENTRY *Uniform4iARB) (GLint location, GLint v0, GLint v1, GLint v2, GLint v3);
  void (APIENTRY *Uniform1fvARB) (GLint location, GLsizei count, const GLfloat *value);
  void (APIENTRY *Uniform2fvARB) (GLint location, GLsizei count, const GLfloat *value);
  void (APIENTRY *Uniform3fvARB) (GLint location, GLsizei count, const GLfloat *value);
  void (APIENTRY *Uniform4fvARB) (GLint location, GLsizei count, const GLfloat *value);
  void (APIENTRY *Uniform1ivARB) (GLint location, GLsizei count, const GLint *value);
  void (APIENTRY *Uniform2ivARB) (GLint location, GLsizei count, const GLint *value);
  void (APIENTRY *Uniform3ivARB) (GLint location, GLsizei count, const GLint *value);
  void (APIENTRY *Uniform4ivARB) (GLint location, GLsizei count, const GLint *value);
  void (APIENTRY *UniformMatrix2fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
  void (APIENTRY *UniformMatrix3fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
  void (APIENTRY *UniformMatrix4fvARB) (GLint location, GLsizei count, GLboolean transpose, const GLfloat *value);
  void (APIENTRY *GetObjectParameterfvARB) (GLhandleARB obj, GLenum pname, GLfloat *params);
  void (APIENTRY *GetObjectParameterivARB) (GLhandleARB obj, GLenum pname, GLint *params);
  void (APIENTRY *GetInfoLogARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *infoLog);
  void (APIENTRY *GetAttachedObjectsARB) (GLhandleARB containerObj, GLsizei maxCount, GLsizei *count, GLhandleARB *obj);
  GLint (APIENTRY *GetUniformLocationARB) (GLhandleARB programObj, const GLcharARB *name);
  void (APIENTRY *GetActiveUniformARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
  void (APIENTRY *GetUniformfvARB) (GLhandleARB programObj, GLint location, GLfloat *params);
  void (APIENTRY *GetUniformivARB) (GLhandleARB programObj, GLint location, GLint *params);
  void (APIENTRY *GetShaderSourceARB) (GLhandleARB obj, GLsizei maxLength, GLsizei *length, GLcharARB *source);
  void (APIENTRY *BindAttribLocationARB) (GLhandleARB programObj, GLuint index, const GLcharARB *name);
  void (APIENTRY *GetActiveAttribARB) (GLhandleARB programObj, GLuint index, GLsizei maxLength, GLsizei *length, GLint *size, GLenum *type, GLcharARB *name);
  GLint (APIENTRY *GetAttribLocationARB) (GLhandleARB programObj, const GLcharARB *name);
  void (APIENTRY *BlendColorEXT) (GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
  void (APIENTRY *PolygonOffsetEXT) (GLfloat factor, GLfloat bias);
  void (APIENTRY *TexImage3DEXT) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *TexSubImage3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *GetTexFilterFuncSGIS) (GLenum target, GLenum filter, GLfloat *weights);
  void (APIENTRY *TexFilterFuncSGIS) (GLenum target, GLenum filter, GLsizei n, const GLfloat *weights);
  void (APIENTRY *TexSubImage1DEXT) (GLenum target, GLint level, GLint xoffset, GLsizei width, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *TexSubImage2DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *CopyTexImage1DEXT) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border);
  void (APIENTRY *CopyTexImage2DEXT) (GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border);
  void (APIENTRY *CopyTexSubImage1DEXT) (GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width);
  void (APIENTRY *CopyTexSubImage2DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height);
  void (APIENTRY *CopyTexSubImage3DEXT) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height);
  void (APIENTRY *GetHistogramEXT) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
  void (APIENTRY *GetHistogramParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetHistogramParameterivEXT) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetMinmaxEXT) (GLenum target, GLboolean reset, GLenum format, GLenum type, GLvoid *values);
  void (APIENTRY *GetMinmaxParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetMinmaxParameterivEXT) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *HistogramEXT) (GLenum target, GLsizei width, GLenum internalformat, GLboolean sink);
  void (APIENTRY *MinmaxEXT) (GLenum target, GLenum internalformat, GLboolean sink);
  void (APIENTRY *ResetHistogramEXT) (GLenum target);
  void (APIENTRY *ResetMinmaxEXT) (GLenum target);
  void (APIENTRY *ConvolutionFilter1DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *image);
  void (APIENTRY *ConvolutionFilter2DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *image);
  void (APIENTRY *ConvolutionParameterfEXT) (GLenum target, GLenum pname, GLfloat params);
  void (APIENTRY *ConvolutionParameterfvEXT) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *ConvolutionParameteriEXT) (GLenum target, GLenum pname, GLint params);
  void (APIENTRY *ConvolutionParameterivEXT) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *CopyConvolutionFilter1DEXT) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
  void (APIENTRY *CopyConvolutionFilter2DEXT) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height);
  void (APIENTRY *GetConvolutionFilterEXT) (GLenum target, GLenum format, GLenum type, GLvoid *image);
  void (APIENTRY *GetConvolutionParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetConvolutionParameterivEXT) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetSeparableFilterEXT) (GLenum target, GLenum format, GLenum type, GLvoid *row, GLvoid *column, GLvoid *span);
  void (APIENTRY *SeparableFilter2DEXT) (GLenum target, GLenum internalformat, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid *row, const GLvoid *column);
  void (APIENTRY *ColorTableSGI) (GLenum target, GLenum internalformat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
  void (APIENTRY *ColorTableParameterfvSGI) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *ColorTableParameterivSGI) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *CopyColorTableSGI) (GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width);
  void (APIENTRY *GetColorTableSGI) (GLenum target, GLenum format, GLenum type, GLvoid *table);
  void (APIENTRY *GetColorTableParameterfvSGI) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetColorTableParameterivSGI) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *PixelTexGenSGIX) (GLenum mode);
  void (APIENTRY *PixelTexGenParameteriSGIS) (GLenum pname, GLint param);
  void (APIENTRY *PixelTexGenParameterivSGIS) (GLenum pname, const GLint *params);
  void (APIENTRY *PixelTexGenParameterfSGIS) (GLenum pname, GLfloat param);
  void (APIENTRY *PixelTexGenParameterfvSGIS) (GLenum pname, const GLfloat *params);
  void (APIENTRY *GetPixelTexGenParameterivSGIS) (GLenum pname, GLint *params);
  void (APIENTRY *GetPixelTexGenParameterfvSGIS) (GLenum pname, GLfloat *params);
  void (APIENTRY *TexImage4DSGIS) (GLenum target, GLint level, GLenum internalformat, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLint border, GLenum format, GLenum type, const GLvoid *pixels);
  void (APIENTRY *TexSubImage4DSGIS) (GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint woffset, GLsizei width, GLsizei height, GLsizei depth, GLsizei size4d, GLenum format, GLenum type, const GLvoid *pixels);
  GLboolean (APIENTRY *AreTexturesResidentEXT) (GLsizei n, const GLuint *textures, GLboolean *residences);
  void (APIENTRY *BindTextureEXT) (GLenum target, GLuint texture);
  void (APIENTRY *DeleteTexturesEXT) (GLsizei n, const GLuint *textures);
  void (APIENTRY *GenTexturesEXT) (GLsizei n, GLuint *textures);
  GLboolean (APIENTRY *IsTextureEXT) (GLuint texture);
  void (APIENTRY *PrioritizeTexturesEXT) (GLsizei n, const GLuint *textures, const GLclampf *priorities);
  void (APIENTRY *DetailTexFuncSGIS) (GLenum target, GLsizei n, const GLfloat *points);
  void (APIENTRY *GetDetailTexFuncSGIS) (GLenum target, GLfloat *points);
  void (APIENTRY *SharpenTexFuncSGIS) (GLenum target, GLsizei n, const GLfloat *points);
  void (APIENTRY *GetSharpenTexFuncSGIS) (GLenum target, GLfloat *points);
  void (APIENTRY *SampleMaskSGIS) (GLclampf value, GLboolean invert);
  void (APIENTRY *SamplePatternSGIS) (GLenum pattern);
  void (APIENTRY *ArrayElementEXT) (GLint i);
  void (APIENTRY *ColorPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
  void (APIENTRY *DrawArraysEXT) (GLenum mode, GLint first, GLsizei count);
  void (APIENTRY *EdgeFlagPointerEXT) (GLsizei stride, GLsizei count, const GLboolean *pointer);
  void (APIENTRY *GetPointervEXT) (GLenum pname, GLvoid* *params);
  void (APIENTRY *IndexPointerEXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
  void (APIENTRY *NormalPointerEXT) (GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
  void (APIENTRY *TexCoordPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
  void (APIENTRY *VertexPointerEXT) (GLint size, GLenum type, GLsizei stride, GLsizei count, const GLvoid *pointer);
  void (APIENTRY *BlendEquationEXT) (GLenum mode);
  void (APIENTRY *SpriteParameterfSGIX) (GLenum pname, GLfloat param);
  void (APIENTRY *SpriteParameterfvSGIX) (GLenum pname, const GLfloat *params);
  void (APIENTRY *SpriteParameteriSGIX) (GLenum pname, GLint param);
  void (APIENTRY *SpriteParameterivSGIX) (GLenum pname, const GLint *params);
  void (APIENTRY *PointParameterfEXT) (GLenum pname, GLfloat param);
  void (APIENTRY *PointParameterfvEXT) (GLenum pname, const GLfloat *params);
  void (APIENTRY *PointParameterfSGIS) (GLenum pname, GLfloat param);
  void (APIENTRY *PointParameterfvSGIS) (GLenum pname, const GLfloat *params);
  GLint (APIENTRY *GetInstrumentsSGIX) (void);
  void (APIENTRY *InstrumentsBufferSGIX) (GLsizei size, GLint *buffer);
  GLint (APIENTRY *PollInstrumentsSGIX) (GLint *marker_p);
  void (APIENTRY *ReadInstrumentsSGIX) (GLint marker);
  void (APIENTRY *StartInstrumentsSGIX) (void);
  void (APIENTRY *StopInstrumentsSGIX) (GLint marker);
  void (APIENTRY *FrameZoomSGIX) (GLint factor);
  void (APIENTRY *TagSampleBufferSGIX) (void);
  void (APIENTRY *DeformationMap3dSGIX) (GLenum target, GLdouble u1, GLdouble u2, GLint ustride, GLint uorder, GLdouble v1, GLdouble v2, GLint vstride, GLint vorder, GLdouble w1, GLdouble w2, GLint wstride, GLint worder, const GLdouble *points);
  void (APIENTRY *DeformationMap3fSGIX) (GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder, GLfloat v1, GLfloat v2, GLint vstride, GLint vorder, GLfloat w1, GLfloat w2, GLint wstride, GLint worder, const GLfloat *points);
  void (APIENTRY *DeformSGIX) (GLbitfield mask);
  void (APIENTRY *LoadIdentityDeformationMapSGIX) (GLbitfield mask);
  void (APIENTRY *ReferencePlaneSGIX) (const GLdouble *equation);
  void (APIENTRY *FlushRasterSGIX) (void);
  void (APIENTRY *FogFuncSGIS) (GLsizei n, const GLfloat *points);
  void (APIENTRY *GetFogFuncSGIS) (GLfloat *points);
  void (APIENTRY *ImageTransformParameteriHP) (GLenum target, GLenum pname, GLint param);
  void (APIENTRY *ImageTransformParameterfHP) (GLenum target, GLenum pname, GLfloat param);
  void (APIENTRY *ImageTransformParameterivHP) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *ImageTransformParameterfvHP) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *GetImageTransformParameterivHP) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetImageTransformParameterfvHP) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *ColorSubTableEXT) (GLenum target, GLsizei start, GLsizei count, GLenum format, GLenum type, const GLvoid *data);
  void (APIENTRY *CopyColorSubTableEXT) (GLenum target, GLsizei start, GLint x, GLint y, GLsizei width);
  void (APIENTRY *HintPGI) (GLenum target, GLint mode);
  void (APIENTRY *ColorTableEXT) (GLenum target, GLenum internalFormat, GLsizei width, GLenum format, GLenum type, const GLvoid *table);
  void (APIENTRY *GetColorTableEXT) (GLenum target, GLenum format, GLenum type, GLvoid *data);
  void (APIENTRY *GetColorTableParameterivEXT) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetColorTableParameterfvEXT) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetListParameterfvSGIX) (GLuint list, GLenum pname, GLfloat *params);
  void (APIENTRY *GetListParameterivSGIX) (GLuint list, GLenum pname, GLint *params);
  void (APIENTRY *ListParameterfSGIX) (GLuint list, GLenum pname, GLfloat param);
  void (APIENTRY *ListParameterfvSGIX) (GLuint list, GLenum pname, const GLfloat *params);
  void (APIENTRY *ListParameteriSGIX) (GLuint list, GLenum pname, GLint param);
  void (APIENTRY *ListParameterivSGIX) (GLuint list, GLenum pname, const GLint *params);
  void (APIENTRY *IndexMaterialEXT) (GLenum face, GLenum mode);
  void (APIENTRY *IndexFuncEXT) (GLenum func, GLclampf ref);
  void (APIENTRY *LockArraysEXT) (GLint first, GLsizei count);
  void (APIENTRY *UnlockArraysEXT) (void);
  void (APIENTRY *CullParameterdvEXT) (GLenum pname, GLdouble *params);
  void (APIENTRY *CullParameterfvEXT) (GLenum pname, GLfloat *params);
  void (APIENTRY *FragmentColorMaterialSGIX) (GLenum face, GLenum mode);
  void (APIENTRY *FragmentLightfSGIX) (GLenum light, GLenum pname, GLfloat param);
  void (APIENTRY *FragmentLightfvSGIX) (GLenum light, GLenum pname, const GLfloat *params);
  void (APIENTRY *FragmentLightiSGIX) (GLenum light, GLenum pname, GLint param);
  void (APIENTRY *FragmentLightivSGIX) (GLenum light, GLenum pname, const GLint *params);
  void (APIENTRY *FragmentLightModelfSGIX) (GLenum pname, GLfloat param);
  void (APIENTRY *FragmentLightModelfvSGIX) (GLenum pname, const GLfloat *params);
  void (APIENTRY *FragmentLightModeliSGIX) (GLenum pname, GLint param);
  void (APIENTRY *FragmentLightModelivSGIX) (GLenum pname, const GLint *params);
  void (APIENTRY *FragmentMaterialfSGIX) (GLenum face, GLenum pname, GLfloat param);
  void (APIENTRY *FragmentMaterialfvSGIX) (GLenum face, GLenum pname, const GLfloat *params);
  void (APIENTRY *FragmentMaterialiSGIX) (GLenum face, GLenum pname, GLint param);
  void (APIENTRY *FragmentMaterialivSGIX) (GLenum face, GLenum pname, const GLint *params);
  void (APIENTRY *GetFragmentLightfvSGIX) (GLenum light, GLenum pname, GLfloat *params);
  void (APIENTRY *GetFragmentLightivSGIX) (GLenum light, GLenum pname, GLint *params);
  void (APIENTRY *GetFragmentMaterialfvSGIX) (GLenum face, GLenum pname, GLfloat *params);
  void (APIENTRY *GetFragmentMaterialivSGIX) (GLenum face, GLenum pname, GLint *params);
  void (APIENTRY *LightEnviSGIX) (GLenum pname, GLint param);
  void (APIENTRY *DrawRangeElementsEXT) (GLenum mode, GLuint start, GLuint end, GLsizei count, GLenum type, const GLvoid *indices);
  void (APIENTRY *ApplyTextureEXT) (GLenum mode);
  void (APIENTRY *TextureLightEXT) (GLenum pname);
  void (APIENTRY *TextureMaterialEXT) (GLenum face, GLenum mode);
  void (APIENTRY *AsyncMarkerSGIX) (GLuint marker);
  GLint (APIENTRY *FinishAsyncSGIX) (GLuint *markerp);
  GLint (APIENTRY *PollAsyncSGIX) (GLuint *markerp);
  GLuint (APIENTRY *GenAsyncMarkersSGIX) (GLsizei range);
  void (APIENTRY *DeleteAsyncMarkersSGIX) (GLuint marker, GLsizei range);
  GLboolean (APIENTRY *IsAsyncMarkerSGIX) (GLuint marker);
  void (APIENTRY *VertexPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
  void (APIENTRY *NormalPointervINTEL) (GLenum type, const GLvoid* *pointer);
  void (APIENTRY *ColorPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
  void (APIENTRY *TexCoordPointervINTEL) (GLint size, GLenum type, const GLvoid* *pointer);
  void (APIENTRY *PixelTransformParameteriEXT) (GLenum target, GLenum pname, GLint param);
  void (APIENTRY *PixelTransformParameterfEXT) (GLenum target, GLenum pname, GLfloat param);
  void (APIENTRY *PixelTransformParameterivEXT) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *PixelTransformParameterfvEXT) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *SecondaryColor3bEXT) (GLbyte red, GLbyte green, GLbyte blue);
  void (APIENTRY *SecondaryColor3bvEXT) (const GLbyte *v);
  void (APIENTRY *SecondaryColor3dEXT) (GLdouble red, GLdouble green, GLdouble blue);
  void (APIENTRY *SecondaryColor3dvEXT) (const GLdouble *v);
  void (APIENTRY *SecondaryColor3fEXT) (GLfloat red, GLfloat green, GLfloat blue);
  void (APIENTRY *SecondaryColor3fvEXT) (const GLfloat *v);
  void (APIENTRY *SecondaryColor3iEXT) (GLint red, GLint green, GLint blue);
  void (APIENTRY *SecondaryColor3ivEXT) (const GLint *v);
  void (APIENTRY *SecondaryColor3sEXT) (GLshort red, GLshort green, GLshort blue);
  void (APIENTRY *SecondaryColor3svEXT) (const GLshort *v);
  void (APIENTRY *SecondaryColor3ubEXT) (GLubyte red, GLubyte green, GLubyte blue);
  void (APIENTRY *SecondaryColor3ubvEXT) (const GLubyte *v);
  void (APIENTRY *SecondaryColor3uiEXT) (GLuint red, GLuint green, GLuint blue);
  void (APIENTRY *SecondaryColor3uivEXT) (const GLuint *v);
  void (APIENTRY *SecondaryColor3usEXT) (GLushort red, GLushort green, GLushort blue);
  void (APIENTRY *SecondaryColor3usvEXT) (const GLushort *v);
  void (APIENTRY *SecondaryColorPointerEXT) (GLint size, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *TextureNormalEXT) (GLenum mode);
  void (APIENTRY *MultiDrawArraysEXT) (GLenum mode, GLint *first, GLsizei *count, GLsizei primcount);
  void (APIENTRY *MultiDrawElementsEXT) (GLenum mode, const GLsizei *count, GLenum type, const GLvoid* *indices, GLsizei primcount);
  void (APIENTRY *FogCoordfEXT) (GLfloat coord);
  void (APIENTRY *FogCoordfvEXT) (const GLfloat *coord);
  void (APIENTRY *FogCoorddEXT) (GLdouble coord);
  void (APIENTRY *FogCoorddvEXT) (const GLdouble *coord);
  void (APIENTRY *FogCoordPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *Tangent3bEXT) (GLbyte tx, GLbyte ty, GLbyte tz);
  void (APIENTRY *Tangent3bvEXT) (const GLbyte *v);
  void (APIENTRY *Tangent3dEXT) (GLdouble tx, GLdouble ty, GLdouble tz);
  void (APIENTRY *Tangent3dvEXT) (const GLdouble *v);
  void (APIENTRY *Tangent3fEXT) (GLfloat tx, GLfloat ty, GLfloat tz);
  void (APIENTRY *Tangent3fvEXT) (const GLfloat *v);
  void (APIENTRY *Tangent3iEXT) (GLint tx, GLint ty, GLint tz);
  void (APIENTRY *Tangent3ivEXT) (const GLint *v);
  void (APIENTRY *Tangent3sEXT) (GLshort tx, GLshort ty, GLshort tz);
  void (APIENTRY *Tangent3svEXT) (const GLshort *v);
  void (APIENTRY *Binormal3bEXT) (GLbyte bx, GLbyte by, GLbyte bz);
  void (APIENTRY *Binormal3bvEXT) (const GLbyte *v);
  void (APIENTRY *Binormal3dEXT) (GLdouble bx, GLdouble by, GLdouble bz);
  void (APIENTRY *Binormal3dvEXT) (const GLdouble *v);
  void (APIENTRY *Binormal3fEXT) (GLfloat bx, GLfloat by, GLfloat bz);
  void (APIENTRY *Binormal3fvEXT) (const GLfloat *v);
  void (APIENTRY *Binormal3iEXT) (GLint bx, GLint by, GLint bz);
  void (APIENTRY *Binormal3ivEXT) (const GLint *v);
  void (APIENTRY *Binormal3sEXT) (GLshort bx, GLshort by, GLshort bz);
  void (APIENTRY *Binormal3svEXT) (const GLshort *v);
  void (APIENTRY *TangentPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *BinormalPointerEXT) (GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *FinishTextureSUNX) (void);
  void (APIENTRY *GlobalAlphaFactorbSUN) (GLbyte factor);
  void (APIENTRY *GlobalAlphaFactorsSUN) (GLshort factor);
  void (APIENTRY *GlobalAlphaFactoriSUN) (GLint factor);
  void (APIENTRY *GlobalAlphaFactorfSUN) (GLfloat factor);
  void (APIENTRY *GlobalAlphaFactordSUN) (GLdouble factor);
  void (APIENTRY *GlobalAlphaFactorubSUN) (GLubyte factor);
  void (APIENTRY *GlobalAlphaFactorusSUN) (GLushort factor);
  void (APIENTRY *GlobalAlphaFactoruiSUN) (GLuint factor);
  void (APIENTRY *ReplacementCodeuiSUN) (GLuint code);
  void (APIENTRY *ReplacementCodeusSUN) (GLushort code);
  void (APIENTRY *ReplacementCodeubSUN) (GLubyte code);
  void (APIENTRY *ReplacementCodeuivSUN) (const GLuint *code);
  void (APIENTRY *ReplacementCodeusvSUN) (const GLushort *code);
  void (APIENTRY *ReplacementCodeubvSUN) (const GLubyte *code);
  void (APIENTRY *ReplacementCodePointerSUN) (GLenum type, GLsizei stride, const GLvoid* *pointer);
  void (APIENTRY *Color4ubVertex2fSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y);
  void (APIENTRY *Color4ubVertex2fvSUN) (const GLubyte *c, const GLfloat *v);
  void (APIENTRY *Color4ubVertex3fSUN) (GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *Color4ubVertex3fvSUN) (const GLubyte *c, const GLfloat *v);
  void (APIENTRY *Color3fVertex3fSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *Color3fVertex3fvSUN) (const GLfloat *c, const GLfloat *v);
  void (APIENTRY *Normal3fVertex3fSUN) (GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *Normal3fVertex3fvSUN) (const GLfloat *n, const GLfloat *v);
  void (APIENTRY *Color4fNormal3fVertex3fSUN) (GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *Color4fNormal3fVertex3fvSUN) (const GLfloat *c, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *TexCoord2fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *TexCoord2fVertex3fvSUN) (const GLfloat *tc, const GLfloat *v);
  void (APIENTRY *TexCoord4fVertex4fSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *TexCoord4fVertex4fvSUN) (const GLfloat *tc, const GLfloat *v);
  void (APIENTRY *TexCoord2fColor4ubVertex3fSUN) (GLfloat s, GLfloat t, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *TexCoord2fColor4ubVertex3fvSUN) (const GLfloat *tc, const GLubyte *c, const GLfloat *v);
  void (APIENTRY *TexCoord2fColor3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *TexCoord2fColor3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *v);
  void (APIENTRY *TexCoord2fNormal3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *TexCoord2fNormal3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *TexCoord2fColor4fNormal3fVertex3fSUN) (GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *TexCoord2fColor4fNormal3fVertex3fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *TexCoord4fColor4fNormal3fVertex4fSUN) (GLfloat s, GLfloat t, GLfloat p, GLfloat q, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *TexCoord4fColor4fNormal3fVertex4fvSUN) (const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiVertex3fSUN) (GLuint rc, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiVertex3fvSUN) (const GLuint *rc, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiColor4ubVertex3fSUN) (GLuint rc, GLubyte r, GLubyte g, GLubyte b, GLubyte a, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiColor4ubVertex3fvSUN) (const GLuint *rc, const GLubyte *c, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiColor3fVertex3fSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiColor3fVertex3fvSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiNormal3fVertex3fSUN) (GLuint rc, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiColor4fNormal3fVertex3fSUN) (GLuint rc, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiColor4fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiTexCoord2fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiTexCoord2fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN) (GLuint rc, GLfloat s, GLfloat t, GLfloat r, GLfloat g, GLfloat b, GLfloat a, GLfloat nx, GLfloat ny, GLfloat nz, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN) (const GLuint *rc, const GLfloat *tc, const GLfloat *c, const GLfloat *n, const GLfloat *v);
  void (APIENTRY *BlendFuncSeparateEXT) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
  void (APIENTRY *BlendFuncSeparateINGR) (GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha);
  void (APIENTRY *VertexWeightfEXT) (GLfloat weight);
  void (APIENTRY *VertexWeightfvEXT) (const GLfloat *weight);
  void (APIENTRY *VertexWeightPointerEXT) (GLsizei size, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *FlushVertexArrayRangeNV) (void);
  void (APIENTRY *VertexArrayRangeNV) (GLsizei length, const GLvoid *pointer);
  void (APIENTRY *CombinerParameterfvNV) (GLenum pname, const GLfloat *params);
  void (APIENTRY *CombinerParameterfNV) (GLenum pname, GLfloat param);
  void (APIENTRY *CombinerParameterivNV) (GLenum pname, const GLint *params);
  void (APIENTRY *CombinerParameteriNV) (GLenum pname, GLint param);
  void (APIENTRY *CombinerInputNV) (GLenum stage, GLenum portion, GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
  void (APIENTRY *CombinerOutputNV) (GLenum stage, GLenum portion, GLenum abOutput, GLenum cdOutput, GLenum sumOutput, GLenum scale, GLenum bias, GLboolean abDotProduct, GLboolean cdDotProduct, GLboolean muxSum);
  void (APIENTRY *FinalCombinerInputNV) (GLenum variable, GLenum input, GLenum mapping, GLenum componentUsage);
  void (APIENTRY *GetCombinerInputParameterfvNV) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLfloat *params);
  void (APIENTRY *GetCombinerInputParameterivNV) (GLenum stage, GLenum portion, GLenum variable, GLenum pname, GLint *params);
  void (APIENTRY *GetCombinerOutputParameterfvNV) (GLenum stage, GLenum portion, GLenum pname, GLfloat *params);
  void (APIENTRY *GetCombinerOutputParameterivNV) (GLenum stage, GLenum portion, GLenum pname, GLint *params);
  void (APIENTRY *GetFinalCombinerInputParameterfvNV) (GLenum variable, GLenum pname, GLfloat *params);
  void (APIENTRY *GetFinalCombinerInputParameterivNV) (GLenum variable, GLenum pname, GLint *params);
  void (APIENTRY *ResizeBuffersMESA) (void);
  void (APIENTRY *WindowPos2dMESA) (GLdouble x, GLdouble y);
  void (APIENTRY *WindowPos2dvMESA) (const GLdouble *v);
  void (APIENTRY *WindowPos2fMESA) (GLfloat x, GLfloat y);
  void (APIENTRY *WindowPos2fvMESA) (const GLfloat *v);
  void (APIENTRY *WindowPos2iMESA) (GLint x, GLint y);
  void (APIENTRY *WindowPos2ivMESA) (const GLint *v);
  void (APIENTRY *WindowPos2sMESA) (GLshort x, GLshort y);
  void (APIENTRY *WindowPos2svMESA) (const GLshort *v);
  void (APIENTRY *WindowPos3dMESA) (GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *WindowPos3dvMESA) (const GLdouble *v);
  void (APIENTRY *WindowPos3fMESA) (GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *WindowPos3fvMESA) (const GLfloat *v);
  void (APIENTRY *WindowPos3iMESA) (GLint x, GLint y, GLint z);
  void (APIENTRY *WindowPos3ivMESA) (const GLint *v);
  void (APIENTRY *WindowPos3sMESA) (GLshort x, GLshort y, GLshort z);
  void (APIENTRY *WindowPos3svMESA) (const GLshort *v);
  void (APIENTRY *WindowPos4dMESA) (GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *WindowPos4dvMESA) (const GLdouble *v);
  void (APIENTRY *WindowPos4fMESA) (GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *WindowPos4fvMESA) (const GLfloat *v);
  void (APIENTRY *WindowPos4iMESA) (GLint x, GLint y, GLint z, GLint w);
  void (APIENTRY *WindowPos4ivMESA) (const GLint *v);
  void (APIENTRY *WindowPos4sMESA) (GLshort x, GLshort y, GLshort z, GLshort w);
  void (APIENTRY *WindowPos4svMESA) (const GLshort *v);
  void (APIENTRY *MultiModeDrawArraysIBM) (const GLenum *mode, const GLint *first, const GLsizei *count, GLsizei primcount, GLint modestride);
  void (APIENTRY *MultiModeDrawElementsIBM) (const GLenum *mode, const GLsizei *count, GLenum type, const GLvoid* const *indices, GLsizei primcount, GLint modestride);
  void (APIENTRY *ColorPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *SecondaryColorPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *EdgeFlagPointerListIBM) (GLint stride, const GLboolean* *pointer, GLint ptrstride);
  void (APIENTRY *FogCoordPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *IndexPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *NormalPointerListIBM) (GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *TexCoordPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *VertexPointerListIBM) (GLint size, GLenum type, GLint stride, const GLvoid* *pointer, GLint ptrstride);
  void (APIENTRY *TbufferMask3DFX) (GLuint mask);
  void (APIENTRY *SampleMaskEXT) (GLclampf value, GLboolean invert);
  void (APIENTRY *SamplePatternEXT) (GLenum pattern);
  void (APIENTRY *TextureColorMaskSGIS) (GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha);
  void (APIENTRY *IglooInterfaceSGIX) (GLenum pname, const GLvoid *params);
  void (APIENTRY *DeleteFencesNV) (GLsizei n, const GLuint *fences);
  void (APIENTRY *GenFencesNV) (GLsizei n, GLuint *fences);
  GLboolean (APIENTRY *IsFenceNV) (GLuint fence);
  GLboolean (APIENTRY *TestFenceNV) (GLuint fence);
  void (APIENTRY *GetFenceivNV) (GLuint fence, GLenum pname, GLint *params);
  void (APIENTRY *FinishFenceNV) (GLuint fence);
  void (APIENTRY *SetFenceNV) (GLuint fence, GLenum condition);
  void (APIENTRY *MapControlPointsNV) (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLint uorder, GLint vorder, GLboolean packed, const GLvoid *points);
  void (APIENTRY *MapParameterivNV) (GLenum target, GLenum pname, const GLint *params);
  void (APIENTRY *MapParameterfvNV) (GLenum target, GLenum pname, const GLfloat *params);
  void (APIENTRY *GetMapControlPointsNV) (GLenum target, GLuint index, GLenum type, GLsizei ustride, GLsizei vstride, GLboolean packed, GLvoid *points);
  void (APIENTRY *GetMapParameterivNV) (GLenum target, GLenum pname, GLint *params);
  void (APIENTRY *GetMapParameterfvNV) (GLenum target, GLenum pname, GLfloat *params);
  void (APIENTRY *GetMapAttribParameterivNV) (GLenum target, GLuint index, GLenum pname, GLint *params);
  void (APIENTRY *GetMapAttribParameterfvNV) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
  void (APIENTRY *EvalMapsNV) (GLenum target, GLenum mode);
  void (APIENTRY *CombinerStageParameterfvNV) (GLenum stage, GLenum pname, const GLfloat *params);
  void (APIENTRY *GetCombinerStageParameterfvNV) (GLenum stage, GLenum pname, GLfloat *params);
  GLboolean (APIENTRY *AreProgramsResidentNV) (GLsizei n, const GLuint *programs, GLboolean *residences);
  void (APIENTRY *BindProgramNV) (GLenum target, GLuint id);
  void (APIENTRY *DeleteProgramsNV) (GLsizei n, const GLuint *programs);
  void (APIENTRY *ExecuteProgramNV) (GLenum target, GLuint id, const GLfloat *params);
  void (APIENTRY *GenProgramsNV) (GLsizei n, GLuint *programs);
  void (APIENTRY *GetProgramParameterdvNV) (GLenum target, GLuint index, GLenum pname, GLdouble *params);
  void (APIENTRY *GetProgramParameterfvNV) (GLenum target, GLuint index, GLenum pname, GLfloat *params);
  void (APIENTRY *GetProgramivNV) (GLuint id, GLenum pname, GLint *params);
  void (APIENTRY *GetProgramStringNV) (GLuint id, GLenum pname, GLubyte *program);
  void (APIENTRY *GetTrackMatrixivNV) (GLenum target, GLuint address, GLenum pname, GLint *params);
  void (APIENTRY *GetVertexAttribdvNV) (GLuint index, GLenum pname, GLdouble *params);
  void (APIENTRY *GetVertexAttribfvNV) (GLuint index, GLenum pname, GLfloat *params);
  void (APIENTRY *GetVertexAttribivNV) (GLuint index, GLenum pname, GLint *params);
  void (APIENTRY *GetVertexAttribPointervNV) (GLuint index, GLenum pname, GLvoid* *pointer);
  GLboolean (APIENTRY *IsProgramNV) (GLuint id);
  void (APIENTRY *LoadProgramNV) (GLenum target, GLuint id, GLsizei len, const GLubyte *program);
  void (APIENTRY *ProgramParameter4dNV) (GLenum target, GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *ProgramParameter4dvNV) (GLenum target, GLuint index, const GLdouble *v);
  void (APIENTRY *ProgramParameter4fNV) (GLenum target, GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *ProgramParameter4fvNV) (GLenum target, GLuint index, const GLfloat *v);
  void (APIENTRY *ProgramParameters4dvNV) (GLenum target, GLuint index, GLuint count, const GLdouble *v);
  void (APIENTRY *ProgramParameters4fvNV) (GLenum target, GLuint index, GLuint count, const GLfloat *v);
  void (APIENTRY *RequestResidentProgramsNV) (GLsizei n, const GLuint *programs);
  void (APIENTRY *TrackMatrixNV) (GLenum target, GLuint address, GLenum matrix, GLenum transform);
  void (APIENTRY *VertexAttribPointerNV) (GLuint index, GLint fsize, GLenum type, GLsizei stride, const GLvoid *pointer);
  void (APIENTRY *VertexAttrib1dNV) (GLuint index, GLdouble x);
  void (APIENTRY *VertexAttrib1dvNV) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib1fNV) (GLuint index, GLfloat x);
  void (APIENTRY *VertexAttrib1fvNV) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib1sNV) (GLuint index, GLshort x);
  void (APIENTRY *VertexAttrib1svNV) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib2dNV) (GLuint index, GLdouble x, GLdouble y);
  void (APIENTRY *VertexAttrib2dvNV) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib2fNV) (GLuint index, GLfloat x, GLfloat y);
  void (APIENTRY *VertexAttrib2fvNV) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib2sNV) (GLuint index, GLshort x, GLshort y);
  void (APIENTRY *VertexAttrib2svNV) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib3dNV) (GLuint index, GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *VertexAttrib3dvNV) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib3fNV) (GLuint index, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *VertexAttrib3fvNV) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib3sNV) (GLuint index, GLshort x, GLshort y, GLshort z);
  void (APIENTRY *VertexAttrib3svNV) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib4dNV) (GLuint index, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *VertexAttrib4dvNV) (GLuint index, const GLdouble *v);
  void (APIENTRY *VertexAttrib4fNV) (GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *VertexAttrib4fvNV) (GLuint index, const GLfloat *v);
  void (APIENTRY *VertexAttrib4sNV) (GLuint index, GLshort x, GLshort y, GLshort z, GLshort w);
  void (APIENTRY *VertexAttrib4svNV) (GLuint index, const GLshort *v);
  void (APIENTRY *VertexAttrib4ubNV) (GLuint index, GLubyte x, GLubyte y, GLubyte z, GLubyte w);
  void (APIENTRY *VertexAttrib4ubvNV) (GLuint index, const GLubyte *v);
  void (APIENTRY *VertexAttribs1dvNV) (GLuint index, GLsizei count, const GLdouble *v);
  void (APIENTRY *VertexAttribs1fvNV) (GLuint index, GLsizei count, const GLfloat *v);
  void (APIENTRY *VertexAttribs1svNV) (GLuint index, GLsizei count, const GLshort *v);
  void (APIENTRY *VertexAttribs2dvNV) (GLuint index, GLsizei count, const GLdouble *v);
  void (APIENTRY *VertexAttribs2fvNV) (GLuint index, GLsizei count, const GLfloat *v);
  void (APIENTRY *VertexAttribs2svNV) (GLuint index, GLsizei count, const GLshort *v);
  void (APIENTRY *VertexAttribs3dvNV) (GLuint index, GLsizei count, const GLdouble *v);
  void (APIENTRY *VertexAttribs3fvNV) (GLuint index, GLsizei count, const GLfloat *v);
  void (APIENTRY *VertexAttribs3svNV) (GLuint index, GLsizei count, const GLshort *v);
  void (APIENTRY *VertexAttribs4dvNV) (GLuint index, GLsizei count, const GLdouble *v);
  void (APIENTRY *VertexAttribs4fvNV) (GLuint index, GLsizei count, const GLfloat *v);
  void (APIENTRY *VertexAttribs4svNV) (GLuint index, GLsizei count, const GLshort *v);
  void (APIENTRY *VertexAttribs4ubvNV) (GLuint index, GLsizei count, const GLubyte *v);
  void (APIENTRY *TexBumpParameterivATI) (GLenum pname, const GLint *param);
  void (APIENTRY *TexBumpParameterfvATI) (GLenum pname, const GLfloat *param);
  void (APIENTRY *GetTexBumpParameterivATI) (GLenum pname, GLint *param);
  void (APIENTRY *GetTexBumpParameterfvATI) (GLenum pname, GLfloat *param);
  GLuint (APIENTRY *GenFragmentShadersATI) (GLuint range);
  void (APIENTRY *BindFragmentShaderATI) (GLuint id);
  void (APIENTRY *DeleteFragmentShaderATI) (GLuint id);
  void (APIENTRY *BeginFragmentShaderATI) (void);
  void (APIENTRY *EndFragmentShaderATI) (void);
  void (APIENTRY *PassTexCoordATI) (GLuint dst, GLuint coord, GLenum swizzle);
  void (APIENTRY *SampleMapATI) (GLuint dst, GLuint interp, GLenum swizzle);
  void (APIENTRY *ColorFragmentOp1ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
  void (APIENTRY *ColorFragmentOp2ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
  void (APIENTRY *ColorFragmentOp3ATI) (GLenum op, GLuint dst, GLuint dstMask, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
  void (APIENTRY *AlphaFragmentOp1ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod);
  void (APIENTRY *AlphaFragmentOp2ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod);
  void (APIENTRY *AlphaFragmentOp3ATI) (GLenum op, GLuint dst, GLuint dstMod, GLuint arg1, GLuint arg1Rep, GLuint arg1Mod, GLuint arg2, GLuint arg2Rep, GLuint arg2Mod, GLuint arg3, GLuint arg3Rep, GLuint arg3Mod);
  void (APIENTRY *SetFragmentShaderConstantATI) (GLuint dst, const GLfloat *value);
  void (APIENTRY *PNTrianglesiATI) (GLenum pname, GLint param);
  void (APIENTRY *PNTrianglesfATI) (GLenum pname, GLfloat param);
  GLuint (APIENTRY *NewObjectBufferATI) (GLsizei size, const GLvoid *pointer, GLenum usage);
  GLboolean (APIENTRY *IsObjectBufferATI) (GLuint buffer);
  void (APIENTRY *UpdateObjectBufferATI) (GLuint buffer, GLuint offset, GLsizei size, const GLvoid *pointer, GLenum preserve);
  void (APIENTRY *GetObjectBufferfvATI) (GLuint buffer, GLenum pname, GLfloat *params);
  void (APIENTRY *GetObjectBufferivATI) (GLuint buffer, GLenum pname, GLint *params);
  void (APIENTRY *FreeObjectBufferATI) (GLuint buffer);
  void (APIENTRY *ArrayObjectATI) (GLenum array, GLint size, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
  void (APIENTRY *GetArrayObjectfvATI) (GLenum array, GLenum pname, GLfloat *params);
  void (APIENTRY *GetArrayObjectivATI) (GLenum array, GLenum pname, GLint *params);
  void (APIENTRY *VariantArrayObjectATI) (GLuint id, GLenum type, GLsizei stride, GLuint buffer, GLuint offset);
  void (APIENTRY *GetVariantArrayObjectfvATI) (GLuint id, GLenum pname, GLfloat *params);
  void (APIENTRY *GetVariantArrayObjectivATI) (GLuint id, GLenum pname, GLint *params);
  void (APIENTRY *BeginVertexShaderEXT) (void);
  void (APIENTRY *EndVertexShaderEXT) (void);
  void (APIENTRY *BindVertexShaderEXT) (GLuint id);
  GLuint (APIENTRY *GenVertexShadersEXT) (GLuint range);
  void (APIENTRY *DeleteVertexShaderEXT) (GLuint id);
  void (APIENTRY *ShaderOp1EXT) (GLenum op, GLuint res, GLuint arg1);
  void (APIENTRY *ShaderOp2EXT) (GLenum op, GLuint res, GLuint arg1, GLuint arg2);
  void (APIENTRY *ShaderOp3EXT) (GLenum op, GLuint res, GLuint arg1, GLuint arg2, GLuint arg3);
  void (APIENTRY *SwizzleEXT) (GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
  void (APIENTRY *WriteMaskEXT) (GLuint res, GLuint in, GLenum outX, GLenum outY, GLenum outZ, GLenum outW);
  void (APIENTRY *InsertComponentEXT) (GLuint res, GLuint src, GLuint num);
  void (APIENTRY *ExtractComponentEXT) (GLuint res, GLuint src, GLuint num);
  GLuint (APIENTRY *GenSymbolsEXT) (GLenum datatype, GLenum storagetype, GLenum range, GLuint components);
  void (APIENTRY *SetInvariantEXT) (GLuint id, GLenum type, const GLvoid *addr);
  void (APIENTRY *SetLocalConstantEXT) (GLuint id, GLenum type, const GLvoid *addr);
  void (APIENTRY *VariantbvEXT) (GLuint id, const GLbyte *addr);
  void (APIENTRY *VariantsvEXT) (GLuint id, const GLshort *addr);
  void (APIENTRY *VariantivEXT) (GLuint id, const GLint *addr);
  void (APIENTRY *VariantfvEXT) (GLuint id, const GLfloat *addr);
  void (APIENTRY *VariantdvEXT) (GLuint id, const GLdouble *addr);
  void (APIENTRY *VariantubvEXT) (GLuint id, const GLubyte *addr);
  void (APIENTRY *VariantusvEXT) (GLuint id, const GLushort *addr);
  void (APIENTRY *VariantuivEXT) (GLuint id, const GLuint *addr);
  void (APIENTRY *VariantPointerEXT) (GLuint id, GLenum type, GLuint stride, const GLvoid *addr);
  void (APIENTRY *EnableVariantClientStateEXT) (GLuint id);
  void (APIENTRY *DisableVariantClientStateEXT) (GLuint id);
  GLuint (APIENTRY *BindLightParameterEXT) (GLenum light, GLenum value);
  GLuint (APIENTRY *BindMaterialParameterEXT) (GLenum face, GLenum value);
  GLuint (APIENTRY *BindTexGenParameterEXT) (GLenum unit, GLenum coord, GLenum value);
  GLuint (APIENTRY *BindTextureUnitParameterEXT) (GLenum unit, GLenum value);
  GLuint (APIENTRY *BindParameterEXT) (GLenum value);
  GLboolean (APIENTRY *IsVariantEnabledEXT) (GLuint id, GLenum cap);
  void (APIENTRY *GetVariantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
  void (APIENTRY *GetVariantIntegervEXT) (GLuint id, GLenum value, GLint *data);
  void (APIENTRY *GetVariantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
  void (APIENTRY *GetVariantPointervEXT) (GLuint id, GLenum value, GLvoid* *data);
  void (APIENTRY *GetInvariantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
  void (APIENTRY *GetInvariantIntegervEXT) (GLuint id, GLenum value, GLint *data);
  void (APIENTRY *GetInvariantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
  void (APIENTRY *GetLocalConstantBooleanvEXT) (GLuint id, GLenum value, GLboolean *data);
  void (APIENTRY *GetLocalConstantIntegervEXT) (GLuint id, GLenum value, GLint *data);
  void (APIENTRY *GetLocalConstantFloatvEXT) (GLuint id, GLenum value, GLfloat *data);
  void (APIENTRY *VertexStream1sATI) (GLenum stream, GLshort x);
  void (APIENTRY *VertexStream1svATI) (GLenum stream, const GLshort *coords);
  void (APIENTRY *VertexStream1iATI) (GLenum stream, GLint x);
  void (APIENTRY *VertexStream1ivATI) (GLenum stream, const GLint *coords);
  void (APIENTRY *VertexStream1fATI) (GLenum stream, GLfloat x);
  void (APIENTRY *VertexStream1fvATI) (GLenum stream, const GLfloat *coords);
  void (APIENTRY *VertexStream1dATI) (GLenum stream, GLdouble x);
  void (APIENTRY *VertexStream1dvATI) (GLenum stream, const GLdouble *coords);
  void (APIENTRY *VertexStream2sATI) (GLenum stream, GLshort x, GLshort y);
  void (APIENTRY *VertexStream2svATI) (GLenum stream, const GLshort *coords);
  void (APIENTRY *VertexStream2iATI) (GLenum stream, GLint x, GLint y);
  void (APIENTRY *VertexStream2ivATI) (GLenum stream, const GLint *coords);
  void (APIENTRY *VertexStream2fATI) (GLenum stream, GLfloat x, GLfloat y);
  void (APIENTRY *VertexStream2fvATI) (GLenum stream, const GLfloat *coords);
  void (APIENTRY *VertexStream2dATI) (GLenum stream, GLdouble x, GLdouble y);
  void (APIENTRY *VertexStream2dvATI) (GLenum stream, const GLdouble *coords);
  void (APIENTRY *VertexStream3sATI) (GLenum stream, GLshort x, GLshort y, GLshort z);
  void (APIENTRY *VertexStream3svATI) (GLenum stream, const GLshort *coords);
  void (APIENTRY *VertexStream3iATI) (GLenum stream, GLint x, GLint y, GLint z);
  void (APIENTRY *VertexStream3ivATI) (GLenum stream, const GLint *coords);
  void (APIENTRY *VertexStream3fATI) (GLenum stream, GLfloat x, GLfloat y, GLfloat z);
  void (APIENTRY *VertexStream3fvATI) (GLenum stream, const GLfloat *coords);
  void (APIENTRY *VertexStream3dATI) (GLenum stream, GLdouble x, GLdouble y, GLdouble z);
  void (APIENTRY *VertexStream3dvATI) (GLenum stream, const GLdouble *coords);
  void (APIENTRY *VertexStream4sATI) (GLenum stream, GLshort x, GLshort y, GLshort z, GLshort w);
  void (APIENTRY *VertexStream4svATI) (GLenum stream, const GLshort *coords);
  void (APIENTRY *VertexStream4iATI) (GLenum stream, GLint x, GLint y, GLint z, GLint w);
  void (APIENTRY *VertexStream4ivATI) (GLenum stream, const GLint *coords);
  void (APIENTRY *VertexStream4fATI) (GLenum stream, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *VertexStream4fvATI) (GLenum stream, const GLfloat *coords);
  void (APIENTRY *VertexStream4dATI) (GLenum stream, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *VertexStream4dvATI) (GLenum stream, const GLdouble *coords);
  void (APIENTRY *NormalStream3bATI) (GLenum stream, GLbyte nx, GLbyte ny, GLbyte nz);
  void (APIENTRY *NormalStream3bvATI) (GLenum stream, const GLbyte *coords);
  void (APIENTRY *NormalStream3sATI) (GLenum stream, GLshort nx, GLshort ny, GLshort nz);
  void (APIENTRY *NormalStream3svATI) (GLenum stream, const GLshort *coords);
  void (APIENTRY *NormalStream3iATI) (GLenum stream, GLint nx, GLint ny, GLint nz);
  void (APIENTRY *NormalStream3ivATI) (GLenum stream, const GLint *coords);
  void (APIENTRY *NormalStream3fATI) (GLenum stream, GLfloat nx, GLfloat ny, GLfloat nz);
  void (APIENTRY *NormalStream3fvATI) (GLenum stream, const GLfloat *coords);
  void (APIENTRY *NormalStream3dATI) (GLenum stream, GLdouble nx, GLdouble ny, GLdouble nz);
  void (APIENTRY *NormalStream3dvATI) (GLenum stream, const GLdouble *coords);
  void (APIENTRY *ClientActiveVertexStreamATI) (GLenum stream);
  void (APIENTRY *VertexBlendEnviATI) (GLenum pname, GLint param);
  void (APIENTRY *VertexBlendEnvfATI) (GLenum pname, GLfloat param);
  void (APIENTRY *ElementPointerATI) (GLenum type, const GLvoid *pointer);
  void (APIENTRY *DrawElementArrayATI) (GLenum mode, GLsizei count);
  void (APIENTRY *DrawRangeElementArrayATI) (GLenum mode, GLuint start, GLuint end, GLsizei count);
  void (APIENTRY *DrawMeshArraysSUN) (GLenum mode, GLint first, GLsizei count, GLsizei width);
  void (APIENTRY *GenOcclusionQueriesNV) (GLsizei n, GLuint *ids);
  void (APIENTRY *DeleteOcclusionQueriesNV) (GLsizei n, const GLuint *ids);
  GLboolean (APIENTRY *IsOcclusionQueryNV) (GLuint id);
  void (APIENTRY *BeginOcclusionQueryNV) (GLuint id);
  void (APIENTRY *EndOcclusionQueryNV) (void);
  void (APIENTRY *GetOcclusionQueryivNV) (GLuint id, GLenum pname, GLint *params);
  void (APIENTRY *GetOcclusionQueryuivNV) (GLuint id, GLenum pname, GLuint *params);
  void (APIENTRY *PointParameteriNV) (GLenum pname, GLint param);
  void (APIENTRY *PointParameterivNV) (GLenum pname, const GLint *params);
  void (APIENTRY *ActiveStencilFaceEXT) (GLenum face);
  void (APIENTRY *ElementPointerAPPLE) (GLenum type, const GLvoid *pointer);
  void (APIENTRY *DrawElementArrayAPPLE) (GLenum mode, GLint first, GLsizei count);
  void (APIENTRY *DrawRangeElementArrayAPPLE) (GLenum mode, GLuint start, GLuint end, GLint first, GLsizei count);
  void (APIENTRY *MultiDrawElementArrayAPPLE) (GLenum mode, const GLint *first, const GLsizei *count, GLsizei primcount);
  void (APIENTRY *MultiDrawRangeElementArrayAPPLE) (GLenum mode, GLuint start, GLuint end, const GLint *first, const GLsizei *count, GLsizei primcount);
  void (APIENTRY *GenFencesAPPLE) (GLsizei n, GLuint *fences);
  void (APIENTRY *DeleteFencesAPPLE) (GLsizei n, const GLuint *fences);
  void (APIENTRY *SetFenceAPPLE) (GLuint fence);
  GLboolean (APIENTRY *IsFenceAPPLE) (GLuint fence);
  GLboolean (APIENTRY *TestFenceAPPLE) (GLuint fence);
  void (APIENTRY *FinishFenceAPPLE) (GLuint fence);
  GLboolean (APIENTRY *TestObjectAPPLE) (GLenum object, GLuint name);
  void (APIENTRY *FinishObjectAPPLE) (GLenum object, GLint name);
  void (APIENTRY *BindVertexArrayAPPLE) (GLuint array);
  void (APIENTRY *DeleteVertexArraysAPPLE) (GLsizei n, const GLuint *arrays);
  void (APIENTRY *GenVertexArraysAPPLE) (GLsizei n, const GLuint *arrays);
  GLboolean (APIENTRY *IsVertexArrayAPPLE) (GLuint array);
  void (APIENTRY *VertexArrayRangeAPPLE) (GLsizei length, GLvoid *pointer);
  void (APIENTRY *FlushVertexArrayRangeAPPLE) (GLsizei length, GLvoid *pointer);
  void (APIENTRY *VertexArrayParameteriAPPLE) (GLenum pname, GLint param);
  void (APIENTRY *DrawBuffersATI) (GLsizei n, const GLenum *bufs);
  void (APIENTRY *ProgramNamedParameter4fNV) (GLuint id, GLsizei len, const GLubyte *name, GLfloat x, GLfloat y, GLfloat z, GLfloat w);
  void (APIENTRY *ProgramNamedParameter4dNV) (GLuint id, GLsizei len, const GLubyte *name, GLdouble x, GLdouble y, GLdouble z, GLdouble w);
  void (APIENTRY *ProgramNamedParameter4fvNV) (GLuint id, GLsizei len, const GLubyte *name, const GLfloat *v);
  void (APIENTRY *ProgramNamedParameter4dvNV) (GLuint id, GLsizei len, const GLubyte *name, const GLdouble *v);
  void (APIENTRY *GetProgramNamedParameterfvNV) (GLuint id, GLsizei len, const GLubyte *name, GLfloat *params);
  void (APIENTRY *GetProgramNamedParameterdvNV) (GLuint id, GLsizei len, const GLubyte *name, GLdouble *params);
  void (APIENTRY *Vertex2hNV) (GLhalfNV x, GLhalfNV y);
  void (APIENTRY *Vertex2hvNV) (const GLhalfNV *v);
  void (APIENTRY *Vertex3hNV) (GLhalfNV x, GLhalfNV y, GLhalfNV z);
  void (APIENTRY *Vertex3hvNV) (const GLhalfNV *v);
  void (APIENTRY *Vertex4hNV) (GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
  void (APIENTRY *Vertex4hvNV) (const GLhalfNV *v);
  void (APIENTRY *Normal3hNV) (GLhalfNV nx, GLhalfNV ny, GLhalfNV nz);
  void (APIENTRY *Normal3hvNV) (const GLhalfNV *v);
  void (APIENTRY *Color3hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue);
  void (APIENTRY *Color3hvNV) (const GLhalfNV *v);
  void (APIENTRY *Color4hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue, GLhalfNV alpha);
  void (APIENTRY *Color4hvNV) (const GLhalfNV *v);
  void (APIENTRY *TexCoord1hNV) (GLhalfNV s);
  void (APIENTRY *TexCoord1hvNV) (const GLhalfNV *v);
  void (APIENTRY *TexCoord2hNV) (GLhalfNV s, GLhalfNV t);
  void (APIENTRY *TexCoord2hvNV) (const GLhalfNV *v);
  void (APIENTRY *TexCoord3hNV) (GLhalfNV s, GLhalfNV t, GLhalfNV r);
  void (APIENTRY *TexCoord3hvNV) (const GLhalfNV *v);
  void (APIENTRY *TexCoord4hNV) (GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
  void (APIENTRY *TexCoord4hvNV) (const GLhalfNV *v);
  void (APIENTRY *MultiTexCoord1hNV) (GLenum target, GLhalfNV s);
  void (APIENTRY *MultiTexCoord1hvNV) (GLenum target, const GLhalfNV *v);
  void (APIENTRY *MultiTexCoord2hNV) (GLenum target, GLhalfNV s, GLhalfNV t);
  void (APIENTRY *MultiTexCoord2hvNV) (GLenum target, const GLhalfNV *v);
  void (APIENTRY *MultiTexCoord3hNV) (GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r);
  void (APIENTRY *MultiTexCoord3hvNV) (GLenum target, const GLhalfNV *v);
  void (APIENTRY *MultiTexCoord4hNV) (GLenum target, GLhalfNV s, GLhalfNV t, GLhalfNV r, GLhalfNV q);
  void (APIENTRY *MultiTexCoord4hvNV) (GLenum target, const GLhalfNV *v);
  void (APIENTRY *FogCoordhNV) (GLhalfNV fog);
  void (APIENTRY *FogCoordhvNV) (const GLhalfNV *fog);
  void (APIENTRY *SecondaryColor3hNV) (GLhalfNV red, GLhalfNV green, GLhalfNV blue);
  void (APIENTRY *SecondaryColor3hvNV) (const GLhalfNV *v);
  void (APIENTRY *VertexWeighthNV) (GLhalfNV weight);
  void (APIENTRY *VertexWeighthvNV) (const GLhalfNV *weight);
  void (APIENTRY *VertexAttrib1hNV) (GLuint index, GLhalfNV x);
  void (APIENTRY *VertexAttrib1hvNV) (GLuint index, const GLhalfNV *v);
  void (APIENTRY *VertexAttrib2hNV) (GLuint index, GLhalfNV x, GLhalfNV y);
  void (APIENTRY *VertexAttrib2hvNV) (GLuint index, const GLhalfNV *v);
  void (APIENTRY *VertexAttrib3hNV) (GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z);
  void (APIENTRY *VertexAttrib3hvNV) (GLuint index, const GLhalfNV *v);
  void (APIENTRY *VertexAttrib4hNV) (GLuint index, GLhalfNV x, GLhalfNV y, GLhalfNV z, GLhalfNV w);
  void (APIENTRY *VertexAttrib4hvNV) (GLuint index, const GLhalfNV *v);
  void (APIENTRY *VertexAttribs1hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
  void (APIENTRY *VertexAttribs2hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
  void (APIENTRY *VertexAttribs3hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
  void (APIENTRY *VertexAttribs4hvNV) (GLuint index, GLsizei n, const GLhalfNV *v);
  void (APIENTRY *PixelDataRangeNV) (GLenum target, GLsizei length, GLvoid *pointer);
  void (APIENTRY *FlushPixelDataRangeNV) (GLenum target);
  void (APIENTRY *PrimitiveRestartNV) (void);
  void (APIENTRY *PrimitiveRestartIndexNV) (GLuint index);
  GLvoid* (APIENTRY *MapObjectBufferATI) (GLuint buffer);
  void (APIENTRY *UnmapObjectBufferATI) (GLuint buffer);
  void (APIENTRY *StencilOpSeparateATI) (GLenum face, GLenum sfail, GLenum dpfail, GLenum dppass);
  void (APIENTRY *StencilFuncSeparateATI) (GLenum frontfunc, GLenum backfunc, GLint ref, GLuint mask);
  void (APIENTRY *VertexAttribArrayObjectATI) (GLuint index, GLint size, GLenum type, GLboolean normalized, GLsizei stride, GLuint buffer, GLuint offset);
  void (APIENTRY *GetVertexAttribArrayObjectfvATI) (GLuint index, GLenum pname, GLfloat *params);
  void (APIENTRY *GetVertexAttribArrayObjectivATI) (GLuint index, GLenum pname, GLint *params);
  void (APIENTRY *DepthBoundsEXT) (GLclampd zmin, GLclampd zmax);
  void (APIENTRY *BlendEquationSeparateEXT) (GLenum modeRGB, GLenum modeAlpha);
  void (APIENTRY *AddSwapHintRectWIN) (GLint x, GLint y, GLsizei width, GLsizei height);
#ifdef _WIN32
  HANDLE (WINAPI *CreateBufferRegionARB) (HDC hDC, int iLayerPlane, UINT uType);
  VOID (WINAPI *DeleteBufferRegionARB) (HANDLE hRegion);
  BOOL (WINAPI *SaveBufferRegionARB) (HANDLE hRegion, int x, int y, int width, int height);
  BOOL (WINAPI *RestoreBufferRegionARB) (HANDLE hRegion, int x, int y, int width, int height, int xSrc, int ySrc);
  const int (WINAPI *GetExtensionsStringARB) (HDC hdc);
  BOOL (WINAPI *GetPixelFormatAttribivARB) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, int *piValues);
  BOOL (WINAPI *GetPixelFormatAttribfvARB) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, const int *piAttributes, FLOAT *pfValues);
  BOOL (WINAPI *ChoosePixelFormatARB) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
  BOOL (WINAPI *MakeContextCurrentARB) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
  HDC (WINAPI *GetCurrentReadDCARB) (void);
  HPBUFFERARB (WINAPI *CreatePbufferARB) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
  HDC (WINAPI *GetPbufferDCARB) (HPBUFFERARB hPbuffer);
  int (WINAPI *ReleasePbufferDCARB) (HPBUFFERARB hPbuffer, HDC hDC);
  BOOL (WINAPI *DestroyPbufferARB) (HPBUFFERARB hPbuffer);
  BOOL (WINAPI *QueryPbufferARB) (HPBUFFERARB hPbuffer, int iAttribute, int *piValue);
  BOOL (WINAPI *BindTexImageARB) (HPBUFFERARB hPbuffer, int iBuffer);
  BOOL (WINAPI *ReleaseTexImageARB) (HPBUFFERARB hPbuffer, int iBuffer);
  BOOL (WINAPI *SetPbufferAttribARB) (HPBUFFERARB hPbuffer, const int *piAttribList);
  GLboolean (WINAPI *CreateDisplayColorTableEXT) (GLushort id);
  GLboolean (WINAPI *LoadDisplayColorTableEXT) (const GLushort *table, GLuint length);
  GLboolean (WINAPI *BindDisplayColorTableEXT) (GLushort id);
  VOID (WINAPI *DestroyDisplayColorTableEXT) (GLushort id);
  const int (WINAPI *GetExtensionsStringEXT) (void);
  BOOL (WINAPI *MakeContextCurrentEXT) (HDC hDrawDC, HDC hReadDC, HGLRC hglrc);
  HDC (WINAPI *GetCurrentReadDCEXT) (void);
  HPBUFFEREXT (WINAPI *CreatePbufferEXT) (HDC hDC, int iPixelFormat, int iWidth, int iHeight, const int *piAttribList);
  HDC (WINAPI *GetPbufferDCEXT) (HPBUFFEREXT hPbuffer);
  int (WINAPI *ReleasePbufferDCEXT) (HPBUFFEREXT hPbuffer, HDC hDC);
  BOOL (WINAPI *DestroyPbufferEXT) (HPBUFFEREXT hPbuffer);
  BOOL (WINAPI *QueryPbufferEXT) (HPBUFFEREXT hPbuffer, int iAttribute, int *piValue);
  BOOL (WINAPI *GetPixelFormatAttribivEXT) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, int *piValues);
  BOOL (WINAPI *GetPixelFormatAttribfvEXT) (HDC hdc, int iPixelFormat, int iLayerPlane, UINT nAttributes, int *piAttributes, FLOAT *pfValues);
  BOOL (WINAPI *ChoosePixelFormatEXT) (HDC hdc, const int *piAttribIList, const FLOAT *pfAttribFList, UINT nMaxFormats, int *piFormats, UINT *nNumFormats);
  BOOL (WINAPI *SwapIntervalEXT) (int interval);
  int (WINAPI *GetSwapIntervalEXT) (void);
  void* (WINAPI *AllocateMemoryNV) (GLsizei size, GLfloat readfreq, GLfloat writefreq, GLfloat priority);
  void (WINAPI *FreeMemoryNV) (void);
  BOOL (WINAPI *GetSyncValuesOML) (HDC hdc, INT64 *ust, INT64 *msc, INT64 *sbc);
  BOOL (WINAPI *GetMscRateOML) (HDC hdc, INT32 *numerator, INT32 *denominator);
  INT64 (WINAPI *SwapBuffersMscOML) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder);
  INT64 (WINAPI *SwapLayerBuffersMscOML) (HDC hdc, int fuPlanes, INT64 target_msc, INT64 divisor, INT64 remainder);
  BOOL (WINAPI *WaitForMscOML) (HDC hdc, INT64 target_msc, INT64 divisor, INT64 remainder, INT64 *ust, INT64 *msc, INT64 *sbc);
  BOOL (WINAPI *WaitForSbcOML) (HDC hdc, INT64 target_sbc, INT64 *ust, INT64 *msc, INT64 *sbc);
  BOOL (WINAPI *GetDigitalVideoParametersI3D) (HDC hDC, int iAttribute, int *piValue);
  BOOL (WINAPI *SetDigitalVideoParametersI3D) (HDC hDC, int iAttribute, const int *piValue);
  BOOL (WINAPI *GetGammaTableParametersI3D) (HDC hDC, int iAttribute, int *piValue);
  BOOL (WINAPI *SetGammaTableParametersI3D) (HDC hDC, int iAttribute, const int *piValue);
  BOOL (WINAPI *GetGammaTableI3D) (HDC hDC, int iEntries, USHORT *puRed, USHORT *puGreen, USHORT *puBlue);
  BOOL (WINAPI *SetGammaTableI3D) (HDC hDC, int iEntries, const USHORT *puRed, const USHORT *puGreen, const USHORT *puBlue);
  BOOL (WINAPI *EnableGenlockI3D) (HDC hDC);
  BOOL (WINAPI *DisableGenlockI3D) (HDC hDC);
  BOOL (WINAPI *IsEnabledGenlockI3D) (HDC hDC, BOOL *pFlag);
  BOOL (WINAPI *GenlockSourceI3D) (HDC hDC, UINT uSource);
  BOOL (WINAPI *GetGenlockSourceI3D) (HDC hDC, UINT *uSource);
  BOOL (WINAPI *GenlockSourceEdgeI3D) (HDC hDC, UINT uEdge);
  BOOL (WINAPI *GetGenlockSourceEdgeI3D) (HDC hDC, UINT *uEdge);
  BOOL (WINAPI *GenlockSampleRateI3D) (HDC hDC, UINT uRate);
  BOOL (WINAPI *GetGenlockSampleRateI3D) (HDC hDC, UINT *uRate);
  BOOL (WINAPI *GenlockSourceDelayI3D) (HDC hDC, UINT uDelay);
  BOOL (WINAPI *GetGenlockSourceDelayI3D) (HDC hDC, UINT *uDelay);
  BOOL (WINAPI *QueryGenlockMaxSourceDelayI3D) (HDC hDC, UINT *uMaxLineDelay, UINT *uMaxPixelDelay);
  LPVOID (WINAPI *CreateImageBufferI3D) (HDC hDC, DWORD dwSize, UINT uFlags);
  BOOL (WINAPI *DestroyImageBufferI3D) (HDC hDC, LPVOID pAddress);
  BOOL (WINAPI *AssociateImageBufferEventsI3D) (HDC hDC, const HANDLE *pEvent, const LPVOID *pAddress, const DWORD *pSize, UINT count);
  BOOL (WINAPI *ReleaseImageBufferEventsI3D) (HDC hDC, const LPVOID *pAddress, UINT count);
  BOOL (WINAPI *EnableFrameLockI3D) (void);
  BOOL (WINAPI *DisableFrameLockI3D) (void);
  BOOL (WINAPI *IsEnabledFrameLockI3D) (BOOL *pFlag);
  BOOL (WINAPI *QueryFrameLockMasterI3D) (BOOL *pFlag);
  BOOL (WINAPI *GetFrameUsageI3D) (float *pUsage);
  BOOL (WINAPI *BeginFrameTrackingI3D) (void);
  BOOL (WINAPI *EndFrameTrackingI3D) (void);
  BOOL (WINAPI *QueryFrameTrackingI3D) (DWORD *pFrameCount, DWORD *pMissedFrames, float *pLastMissedUsage);
#endif /* _WIN32 */
} _GLextensionProcs;

#define glBlendColor                     (_GET_TLS_PROCTABLE()->BlendColor)
#define glBlendEquation                  (_GET_TLS_PROCTABLE()->BlendEquation)
#define glDrawRangeElements              (_GET_TLS_PROCTABLE()->DrawRangeElements)
#define glColorTable                     (_GET_TLS_PROCTABLE()->ColorTable)
#define glColorTableParameterfv          (_GET_TLS_PROCTABLE()->ColorTableParameterfv)
#define glColorTableParameteriv          (_GET_TLS_PROCTABLE()->ColorTableParameteriv)
#define glCopyColorTable                 (_GET_TLS_PROCTABLE()->CopyColorTable)
#define glGetColorTable                  (_GET_TLS_PROCTABLE()->GetColorTable)
#define glGetColorTableParameterfv       (_GET_TLS_PROCTABLE()->GetColorTableParameterfv)
#define glGetColorTableParameteriv       (_GET_TLS_PROCTABLE()->GetColorTableParameteriv)
#define glColorSubTable                  (_GET_TLS_PROCTABLE()->ColorSubTable)
#define glCopyColorSubTable              (_GET_TLS_PROCTABLE()->CopyColorSubTable)
#define glConvolutionFilter1D            (_GET_TLS_PROCTABLE()->ConvolutionFilter1D)
#define glConvolutionFilter2D            (_GET_TLS_PROCTABLE()->ConvolutionFilter2D)
#define glConvolutionParameterf          (_GET_TLS_PROCTABLE()->ConvolutionParameterf)
#define glConvolutionParameterfv         (_GET_TLS_PROCTABLE()->ConvolutionParameterfv)
#define glConvolutionParameteri          (_GET_TLS_PROCTABLE()->ConvolutionParameteri)
#define glConvolutionParameteriv         (_GET_TLS_PROCTABLE()->ConvolutionParameteriv)
#define glCopyConvolutionFilter1D        (_GET_TLS_PROCTABLE()->CopyConvolutionFilter1D)
#define glCopyConvolutionFilter2D        (_GET_TLS_PROCTABLE()->CopyConvolutionFilter2D)
#define glGetConvolutionFilter           (_GET_TLS_PROCTABLE()->GetConvolutionFilter)
#define glGetConvolutionParameterfv      (_GET_TLS_PROCTABLE()->GetConvolutionParameterfv)
#define glGetConvolutionParameteriv      (_GET_TLS_PROCTABLE()->GetConvolutionParameteriv)
#define glGetSeparableFilter             (_GET_TLS_PROCTABLE()->GetSeparableFilter)
#define glSeparableFilter2D              (_GET_TLS_PROCTABLE()->SeparableFilter2D)
#define glGetHistogram                   (_GET_TLS_PROCTABLE()->GetHistogram)
#define glGetHistogramParameterfv        (_GET_TLS_PROCTABLE()->GetHistogramParameterfv)
#define glGetHistogramParameteriv        (_GET_TLS_PROCTABLE()->GetHistogramParameteriv)
#define glGetMinmax                      (_GET_TLS_PROCTABLE()->GetMinmax)
#define glGetMinmaxParameterfv           (_GET_TLS_PROCTABLE()->GetMinmaxParameterfv)
#define glGetMinmaxParameteriv           (_GET_TLS_PROCTABLE()->GetMinmaxParameteriv)
#define glHistogram                      (_GET_TLS_PROCTABLE()->Histogram)
#define glMinmax                         (_GET_TLS_PROCTABLE()->Minmax)
#define glResetHistogram                 (_GET_TLS_PROCTABLE()->ResetHistogram)
#define glResetMinmax                    (_GET_TLS_PROCTABLE()->ResetMinmax)
#define glTexImage3D                     (_GET_TLS_PROCTABLE()->TexImage3D)
#define glTexSubImage3D                  (_GET_TLS_PROCTABLE()->TexSubImage3D)
#define glCopyTexSubImage3D              (_GET_TLS_PROCTABLE()->CopyTexSubImage3D)
#define glActiveTexture                  (_GET_TLS_PROCTABLE()->ActiveTexture)
#define glClientActiveTexture            (_GET_TLS_PROCTABLE()->ClientActiveTexture)
#define glMultiTexCoord1d                (_GET_TLS_PROCTABLE()->MultiTexCoord1d)
#define glMultiTexCoord1dv               (_GET_TLS_PROCTABLE()->MultiTexCoord1dv)
#define glMultiTexCoord1f                (_GET_TLS_PROCTABLE()->MultiTexCoord1f)
#define glMultiTexCoord1fv               (_GET_TLS_PROCTABLE()->MultiTexCoord1fv)
#define glMultiTexCoord1i                (_GET_TLS_PROCTABLE()->MultiTexCoord1i)
#define glMultiTexCoord1iv               (_GET_TLS_PROCTABLE()->MultiTexCoord1iv)
#define glMultiTexCoord1s                (_GET_TLS_PROCTABLE()->MultiTexCoord1s)
#define glMultiTexCoord1sv               (_GET_TLS_PROCTABLE()->MultiTexCoord1sv)
#define glMultiTexCoord2d                (_GET_TLS_PROCTABLE()->MultiTexCoord2d)
#define glMultiTexCoord2dv               (_GET_TLS_PROCTABLE()->MultiTexCoord2dv)
#define glMultiTexCoord2f                (_GET_TLS_PROCTABLE()->MultiTexCoord2f)
#define glMultiTexCoord2fv               (_GET_TLS_PROCTABLE()->MultiTexCoord2fv)
#define glMultiTexCoord2i                (_GET_TLS_PROCTABLE()->MultiTexCoord2i)
#define glMultiTexCoord2iv               (_GET_TLS_PROCTABLE()->MultiTexCoord2iv)
#define glMultiTexCoord2s                (_GET_TLS_PROCTABLE()->MultiTexCoord2s)
#define glMultiTexCoord2sv               (_GET_TLS_PROCTABLE()->MultiTexCoord2sv)
#define glMultiTexCoord3d                (_GET_TLS_PROCTABLE()->MultiTexCoord3d)
#define glMultiTexCoord3dv               (_GET_TLS_PROCTABLE()->MultiTexCoord3dv)
#define glMultiTexCoord3f                (_GET_TLS_PROCTABLE()->MultiTexCoord3f)
#define glMultiTexCoord3fv               (_GET_TLS_PROCTABLE()->MultiTexCoord3fv)
#define glMultiTexCoord3i                (_GET_TLS_PROCTABLE()->MultiTexCoord3i)
#define glMultiTexCoord3iv               (_GET_TLS_PROCTABLE()->MultiTexCoord3iv)
#define glMultiTexCoord3s                (_GET_TLS_PROCTABLE()->MultiTexCoord3s)
#define glMultiTexCoord3sv               (_GET_TLS_PROCTABLE()->MultiTexCoord3sv)
#define glMultiTexCoord4d                (_GET_TLS_PROCTABLE()->MultiTexCoord4d)
#define glMultiTexCoord4dv               (_GET_TLS_PROCTABLE()->MultiTexCoord4dv)
#define glMultiTexCoord4f                (_GET_TLS_PROCTABLE()->MultiTexCoord4f)
#define glMultiTexCoord4fv               (_GET_TLS_PROCTABLE()->MultiTexCoord4fv)
#define glMultiTexCoord4i                (_GET_TLS_PROCTABLE()->MultiTexCoord4i)
#define glMultiTexCoord4iv               (_GET_TLS_PROCTABLE()->MultiTexCoord4iv)
#define glMultiTexCoord4s                (_GET_TLS_PROCTABLE()->MultiTexCoord4s)
#define glMultiTexCoord4sv               (_GET_TLS_PROCTABLE()->MultiTexCoord4sv)
#define glLoadTransposeMatrixf           (_GET_TLS_PROCTABLE()->LoadTransposeMatrixf)
#define glLoadTransposeMatrixd           (_GET_TLS_PROCTABLE()->LoadTransposeMatrixd)
#define glMultTransposeMatrixf           (_GET_TLS_PROCTABLE()->MultTransposeMatrixf)
#define glMultTransposeMatrixd           (_GET_TLS_PROCTABLE()->MultTransposeMatrixd)
#define glSampleCoverage                 (_GET_TLS_PROCTABLE()->SampleCoverage)
#define glCompressedTexImage3D           (_GET_TLS_PROCTABLE()->CompressedTexImage3D)
#define glCompressedTexImage2D           (_GET_TLS_PROCTABLE()->CompressedTexImage2D)
#define glCompressedTexImage1D           (_GET_TLS_PROCTABLE()->CompressedTexImage1D)
#define glCompressedTexSubImage3D        (_GET_TLS_PROCTABLE()->CompressedTexSubImage3D)
#define glCompressedTexSubImage2D        (_GET_TLS_PROCTABLE()->CompressedTexSubImage2D)
#define glCompressedTexSubImage1D        (_GET_TLS_PROCTABLE()->CompressedTexSubImage1D)
#define glGetCompressedTexImage          (_GET_TLS_PROCTABLE()->GetCompressedTexImage)
#define glBlendFuncSeparate              (_GET_TLS_PROCTABLE()->BlendFuncSeparate)
#define glFogCoordf                      (_GET_TLS_PROCTABLE()->FogCoordf)
#define glFogCoordfv                     (_GET_TLS_PROCTABLE()->FogCoordfv)
#define glFogCoordd                      (_GET_TLS_PROCTABLE()->FogCoordd)
#define glFogCoorddv                     (_GET_TLS_PROCTABLE()->FogCoorddv)
#define glFogCoordPointer                (_GET_TLS_PROCTABLE()->FogCoordPointer)
#define glMultiDrawArrays                (_GET_TLS_PROCTABLE()->MultiDrawArrays)
#define glMultiDrawElements              (_GET_TLS_PROCTABLE()->MultiDrawElements)
#define glPointParameterf                (_GET_TLS_PROCTABLE()->PointParameterf)
#define glPointParameterfv               (_GET_TLS_PROCTABLE()->PointParameterfv)
#define glPointParameteri                (_GET_TLS_PROCTABLE()->PointParameteri)
#define glPointParameteriv               (_GET_TLS_PROCTABLE()->PointParameteriv)
#define glSecondaryColor3b               (_GET_TLS_PROCTABLE()->SecondaryColor3b)
#define glSecondaryColor3bv              (_GET_TLS_PROCTABLE()->SecondaryColor3bv)
#define glSecondaryColor3d               (_GET_TLS_PROCTABLE()->SecondaryColor3d)
#define glSecondaryColor3dv              (_GET_TLS_PROCTABLE()->SecondaryColor3dv)
#define glSecondaryColor3f               (_GET_TLS_PROCTABLE()->SecondaryColor3f)
#define glSecondaryColor3fv              (_GET_TLS_PROCTABLE()->SecondaryColor3fv)
#define glSecondaryColor3i               (_GET_TLS_PROCTABLE()->SecondaryColor3i)
#define glSecondaryColor3iv              (_GET_TLS_PROCTABLE()->SecondaryColor3iv)
#define glSecondaryColor3s               (_GET_TLS_PROCTABLE()->SecondaryColor3s)
#define glSecondaryColor3sv              (_GET_TLS_PROCTABLE()->SecondaryColor3sv)
#define glSecondaryColor3ub              (_GET_TLS_PROCTABLE()->SecondaryColor3ub)
#define glSecondaryColor3ubv             (_GET_TLS_PROCTABLE()->SecondaryColor3ubv)
#define glSecondaryColor3ui              (_GET_TLS_PROCTABLE()->SecondaryColor3ui)
#define glSecondaryColor3uiv             (_GET_TLS_PROCTABLE()->SecondaryColor3uiv)
#define glSecondaryColor3us              (_GET_TLS_PROCTABLE()->SecondaryColor3us)
#define glSecondaryColor3usv             (_GET_TLS_PROCTABLE()->SecondaryColor3usv)
#define glSecondaryColorPointer          (_GET_TLS_PROCTABLE()->SecondaryColorPointer)
#define glWindowPos2d                    (_GET_TLS_PROCTABLE()->WindowPos2d)
#define glWindowPos2dv                   (_GET_TLS_PROCTABLE()->WindowPos2dv)
#define glWindowPos2f                    (_GET_TLS_PROCTABLE()->WindowPos2f)
#define glWindowPos2fv                   (_GET_TLS_PROCTABLE()->WindowPos2fv)
#define glWindowPos2i                    (_GET_TLS_PROCTABLE()->WindowPos2i)
#define glWindowPos2iv                   (_GET_TLS_PROCTABLE()->WindowPos2iv)
#define glWindowPos2s                    (_GET_TLS_PROCTABLE()->WindowPos2s)
#define glWindowPos2sv                   (_GET_TLS_PROCTABLE()->WindowPos2sv)
#define glWindowPos3d                    (_GET_TLS_PROCTABLE()->WindowPos3d)
#define glWindowPos3dv                   (_GET_TLS_PROCTABLE()->WindowPos3dv)
#define glWindowPos3f                    (_GET_TLS_PROCTABLE()->WindowPos3f)
#define glWindowPos3fv                   (_GET_TLS_PROCTABLE()->WindowPos3fv)
#define glWindowPos3i                    (_GET_TLS_PROCTABLE()->WindowPos3i)
#define glWindowPos3iv                   (_GET_TLS_PROCTABLE()->WindowPos3iv)
#define glWindowPos3s                    (_GET_TLS_PROCTABLE()->WindowPos3s)
#define glWindowPos3sv                   (_GET_TLS_PROCTABLE()->WindowPos3sv)
#define glGenQueries                     (_GET_TLS_PROCTABLE()->GenQueries)
#define glDeleteQueries                  (_GET_TLS_PROCTABLE()->DeleteQueries)
#define glIsQuery                        (_GET_TLS_PROCTABLE()->IsQuery)
#define glBeginQuery                     (_GET_TLS_PROCTABLE()->BeginQuery)
#define glEndQuery                       (_GET_TLS_PROCTABLE()->EndQuery)
#define glGetQueryiv                     (_GET_TLS_PROCTABLE()->GetQueryiv)
#define glGetQueryObjectiv               (_GET_TLS_PROCTABLE()->GetQueryObjectiv)
#define glGetQueryObjectuiv              (_GET_TLS_PROCTABLE()->GetQueryObjectuiv)
#define glBindBuffer                     (_GET_TLS_PROCTABLE()->BindBuffer)
#define glDeleteBuffers                  (_GET_TLS_PROCTABLE()->DeleteBuffers)
#define glGenBuffers                     (_GET_TLS_PROCTABLE()->GenBuffers)
#define glIsBuffer                       (_GET_TLS_PROCTABLE()->IsBuffer)
#define glBufferData                     (_GET_TLS_PROCTABLE()->BufferData)
#define glBufferSubData                  (_GET_TLS_PROCTABLE()->BufferSubData)
#define glGetBufferSubData               (_GET_TLS_PROCTABLE()->GetBufferSubData)
#define glMapBuffer                      (_GET_TLS_PROCTABLE()->MapBuffer)
#define glUnmapBuffer                    (_GET_TLS_PROCTABLE()->UnmapBuffer)
#define glGetBufferParameteriv           (_GET_TLS_PROCTABLE()->GetBufferParameteriv)
#define glGetBufferPointerv              (_GET_TLS_PROCTABLE()->GetBufferPointerv)
#define glActiveTextureARB               (_GET_TLS_PROCTABLE()->ActiveTextureARB)
#define glClientActiveTextureARB         (_GET_TLS_PROCTABLE()->ClientActiveTextureARB)
#define glMultiTexCoord1dARB             (_GET_TLS_PROCTABLE()->MultiTexCoord1dARB)
#define glMultiTexCoord1dvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord1dvARB)
#define glMultiTexCoord1fARB             (_GET_TLS_PROCTABLE()->MultiTexCoord1fARB)
#define glMultiTexCoord1fvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord1fvARB)
#define glMultiTexCoord1iARB             (_GET_TLS_PROCTABLE()->MultiTexCoord1iARB)
#define glMultiTexCoord1ivARB            (_GET_TLS_PROCTABLE()->MultiTexCoord1ivARB)
#define glMultiTexCoord1sARB             (_GET_TLS_PROCTABLE()->MultiTexCoord1sARB)
#define glMultiTexCoord1svARB            (_GET_TLS_PROCTABLE()->MultiTexCoord1svARB)
#define glMultiTexCoord2dARB             (_GET_TLS_PROCTABLE()->MultiTexCoord2dARB)
#define glMultiTexCoord2dvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord2dvARB)
#define glMultiTexCoord2fARB             (_GET_TLS_PROCTABLE()->MultiTexCoord2fARB)
#define glMultiTexCoord2fvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord2fvARB)
#define glMultiTexCoord2iARB             (_GET_TLS_PROCTABLE()->MultiTexCoord2iARB)
#define glMultiTexCoord2ivARB            (_GET_TLS_PROCTABLE()->MultiTexCoord2ivARB)
#define glMultiTexCoord2sARB             (_GET_TLS_PROCTABLE()->MultiTexCoord2sARB)
#define glMultiTexCoord2svARB            (_GET_TLS_PROCTABLE()->MultiTexCoord2svARB)
#define glMultiTexCoord3dARB             (_GET_TLS_PROCTABLE()->MultiTexCoord3dARB)
#define glMultiTexCoord3dvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord3dvARB)
#define glMultiTexCoord3fARB             (_GET_TLS_PROCTABLE()->MultiTexCoord3fARB)
#define glMultiTexCoord3fvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord3fvARB)
#define glMultiTexCoord3iARB             (_GET_TLS_PROCTABLE()->MultiTexCoord3iARB)
#define glMultiTexCoord3ivARB            (_GET_TLS_PROCTABLE()->MultiTexCoord3ivARB)
#define glMultiTexCoord3sARB             (_GET_TLS_PROCTABLE()->MultiTexCoord3sARB)
#define glMultiTexCoord3svARB            (_GET_TLS_PROCTABLE()->MultiTexCoord3svARB)
#define glMultiTexCoord4dARB             (_GET_TLS_PROCTABLE()->MultiTexCoord4dARB)
#define glMultiTexCoord4dvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord4dvARB)
#define glMultiTexCoord4fARB             (_GET_TLS_PROCTABLE()->MultiTexCoord4fARB)
#define glMultiTexCoord4fvARB            (_GET_TLS_PROCTABLE()->MultiTexCoord4fvARB)
#define glMultiTexCoord4iARB             (_GET_TLS_PROCTABLE()->MultiTexCoord4iARB)
#define glMultiTexCoord4ivARB            (_GET_TLS_PROCTABLE()->MultiTexCoord4ivARB)
#define glMultiTexCoord4sARB             (_GET_TLS_PROCTABLE()->MultiTexCoord4sARB)
#define glMultiTexCoord4svARB            (_GET_TLS_PROCTABLE()->MultiTexCoord4svARB)
#define glLoadTransposeMatrixfARB        (_GET_TLS_PROCTABLE()->LoadTransposeMatrixfARB)
#define glLoadTransposeMatrixdARB        (_GET_TLS_PROCTABLE()->LoadTransposeMatrixdARB)
#define glMultTransposeMatrixfARB        (_GET_TLS_PROCTABLE()->MultTransposeMatrixfARB)
#define glMultTransposeMatrixdARB        (_GET_TLS_PROCTABLE()->MultTransposeMatrixdARB)
#define glSampleCoverageARB              (_GET_TLS_PROCTABLE()->SampleCoverageARB)
#define glCompressedTexImage3DARB        (_GET_TLS_PROCTABLE()->CompressedTexImage3DARB)
#define glCompressedTexImage2DARB        (_GET_TLS_PROCTABLE()->CompressedTexImage2DARB)
#define glCompressedTexImage1DARB        (_GET_TLS_PROCTABLE()->CompressedTexImage1DARB)
#define glCompressedTexSubImage3DARB     (_GET_TLS_PROCTABLE()->CompressedTexSubImage3DARB)
#define glCompressedTexSubImage2DARB     (_GET_TLS_PROCTABLE()->CompressedTexSubImage2DARB)
#define glCompressedTexSubImage1DARB     (_GET_TLS_PROCTABLE()->CompressedTexSubImage1DARB)
#define glGetCompressedTexImageARB       (_GET_TLS_PROCTABLE()->GetCompressedTexImageARB)
#define glPointParameterfARB             (_GET_TLS_PROCTABLE()->PointParameterfARB)
#define glPointParameterfvARB            (_GET_TLS_PROCTABLE()->PointParameterfvARB)
#define glWeightbvARB                    (_GET_TLS_PROCTABLE()->WeightbvARB)
#define glWeightsvARB                    (_GET_TLS_PROCTABLE()->WeightsvARB)
#define glWeightivARB                    (_GET_TLS_PROCTABLE()->WeightivARB)
#define glWeightfvARB                    (_GET_TLS_PROCTABLE()->WeightfvARB)
#define glWeightdvARB                    (_GET_TLS_PROCTABLE()->WeightdvARB)
#define glWeightubvARB                   (_GET_TLS_PROCTABLE()->WeightubvARB)
#define glWeightusvARB                   (_GET_TLS_PROCTABLE()->WeightusvARB)
#define glWeightuivARB                   (_GET_TLS_PROCTABLE()->WeightuivARB)
#define glWeightPointerARB               (_GET_TLS_PROCTABLE()->WeightPointerARB)
#define glVertexBlendARB                 (_GET_TLS_PROCTABLE()->VertexBlendARB)
#define glCurrentPaletteMatrixARB        (_GET_TLS_PROCTABLE()->CurrentPaletteMatrixARB)
#define glMatrixIndexubvARB              (_GET_TLS_PROCTABLE()->MatrixIndexubvARB)
#define glMatrixIndexusvARB              (_GET_TLS_PROCTABLE()->MatrixIndexusvARB)
#define glMatrixIndexuivARB              (_GET_TLS_PROCTABLE()->MatrixIndexuivARB)
#define glMatrixIndexPointerARB          (_GET_TLS_PROCTABLE()->MatrixIndexPointerARB)
#define glWindowPos2dARB                 (_GET_TLS_PROCTABLE()->WindowPos2dARB)
#define glWindowPos2dvARB                (_GET_TLS_PROCTABLE()->WindowPos2dvARB)
#define glWindowPos2fARB                 (_GET_TLS_PROCTABLE()->WindowPos2fARB)
#define glWindowPos2fvARB                (_GET_TLS_PROCTABLE()->WindowPos2fvARB)
#define glWindowPos2iARB                 (_GET_TLS_PROCTABLE()->WindowPos2iARB)
#define glWindowPos2ivARB                (_GET_TLS_PROCTABLE()->WindowPos2ivARB)
#define glWindowPos2sARB                 (_GET_TLS_PROCTABLE()->WindowPos2sARB)
#define glWindowPos2svARB                (_GET_TLS_PROCTABLE()->WindowPos2svARB)
#define glWindowPos3dARB                 (_GET_TLS_PROCTABLE()->WindowPos3dARB)
#define glWindowPos3dvARB                (_GET_TLS_PROCTABLE()->WindowPos3dvARB)
#define glWindowPos3fARB                 (_GET_TLS_PROCTABLE()->WindowPos3fARB)
#define glWindowPos3fvARB                (_GET_TLS_PROCTABLE()->WindowPos3fvARB)
#define glWindowPos3iARB                 (_GET_TLS_PROCTABLE()->WindowPos3iARB)
#define glWindowPos3ivARB                (_GET_TLS_PROCTABLE()->WindowPos3ivARB)
#define glWindowPos3sARB                 (_GET_TLS_PROCTABLE()->WindowPos3sARB)
#define glWindowPos3svARB                (_GET_TLS_PROCTABLE()->WindowPos3svARB)
#define glVertexAttrib1dARB              (_GET_TLS_PROCTABLE()->VertexAttrib1dARB)
#define glVertexAttrib1dvARB             (_GET_TLS_PROCTABLE()->VertexAttrib1dvARB)
#define glVertexAttrib1fARB              (_GET_TLS_PROCTABLE()->VertexAttrib1fARB)
#define glVertexAttrib1fvARB             (_GET_TLS_PROCTABLE()->VertexAttrib1fvARB)
#define glVertexAttrib1sARB              (_GET_TLS_PROCTABLE()->VertexAttrib1sARB)
#define glVertexAttrib1svARB             (_GET_TLS_PROCTABLE()->VertexAttrib1svARB)
#define glVertexAttrib2dARB              (_GET_TLS_PROCTABLE()->VertexAttrib2dARB)
#define glVertexAttrib2dvARB             (_GET_TLS_PROCTABLE()->VertexAttrib2dvARB)
#define glVertexAttrib2fARB              (_GET_TLS_PROCTABLE()->VertexAttrib2fARB)
#define glVertexAttrib2fvARB             (_GET_TLS_PROCTABLE()->VertexAttrib2fvARB)
#define glVertexAttrib2sARB              (_GET_TLS_PROCTABLE()->VertexAttrib2sARB)
#define glVertexAttrib2svARB             (_GET_TLS_PROCTABLE()->VertexAttrib2svARB)
#define glVertexAttrib3dARB              (_GET_TLS_PROCTABLE()->VertexAttrib3dARB)
#define glVertexAttrib3dvARB             (_GET_TLS_PROCTABLE()->VertexAttrib3dvARB)
#define glVertexAttrib3fARB              (_GET_TLS_PROCTABLE()->VertexAttrib3fARB)
#define glVertexAttrib3fvARB             (_GET_TLS_PROCTABLE()->VertexAttrib3fvARB)
#define glVertexAttrib3sARB              (_GET_TLS_PROCTABLE()->VertexAttrib3sARB)
#define glVertexAttrib3svARB             (_GET_TLS_PROCTABLE()->VertexAttrib3svARB)
#define glVertexAttrib4NbvARB            (_GET_TLS_PROCTABLE()->VertexAttrib4NbvARB)
#define glVertexAttrib4NivARB            (_GET_TLS_PROCTABLE()->VertexAttrib4NivARB)
#define glVertexAttrib4NsvARB            (_GET_TLS_PROCTABLE()->VertexAttrib4NsvARB)
#define glVertexAttrib4NubARB            (_GET_TLS_PROCTABLE()->VertexAttrib4NubARB)
#define glVertexAttrib4NubvARB           (_GET_TLS_PROCTABLE()->VertexAttrib4NubvARB)
#define glVertexAttrib4NuivARB           (_GET_TLS_PROCTABLE()->VertexAttrib4NuivARB)
#define glVertexAttrib4NusvARB           (_GET_TLS_PROCTABLE()->VertexAttrib4NusvARB)
#define glVertexAttrib4bvARB             (_GET_TLS_PROCTABLE()->VertexAttrib4bvARB)
#define glVertexAttrib4dARB              (_GET_TLS_PROCTABLE()->VertexAttrib4dARB)
#define glVertexAttrib4dvARB             (_GET_TLS_PROCTABLE()->VertexAttrib4dvARB)
#define glVertexAttrib4fARB              (_GET_TLS_PROCTABLE()->VertexAttrib4fARB)
#define glVertexAttrib4fvARB             (_GET_TLS_PROCTABLE()->VertexAttrib4fvARB)
#define glVertexAttrib4ivARB             (_GET_TLS_PROCTABLE()->VertexAttrib4ivARB)
#define glVertexAttrib4sARB              (_GET_TLS_PROCTABLE()->VertexAttrib4sARB)
#define glVertexAttrib4svARB             (_GET_TLS_PROCTABLE()->VertexAttrib4svARB)
#define glVertexAttrib4ubvARB            (_GET_TLS_PROCTABLE()->VertexAttrib4ubvARB)
#define glVertexAttrib4uivARB            (_GET_TLS_PROCTABLE()->VertexAttrib4uivARB)
#define glVertexAttrib4usvARB            (_GET_TLS_PROCTABLE()->VertexAttrib4usvARB)
#define glVertexAttribPointerARB         (_GET_TLS_PROCTABLE()->VertexAttribPointerARB)
#define glEnableVertexAttribArrayARB     (_GET_TLS_PROCTABLE()->EnableVertexAttribArrayARB)
#define glDisableVertexAttribArrayARB    (_GET_TLS_PROCTABLE()->DisableVertexAttribArrayARB)
#define glProgramStringARB               (_GET_TLS_PROCTABLE()->ProgramStringARB)
#define glBindProgramARB                 (_GET_TLS_PROCTABLE()->BindProgramARB)
#define glDeleteProgramsARB              (_GET_TLS_PROCTABLE()->DeleteProgramsARB)
#define glGenProgramsARB                 (_GET_TLS_PROCTABLE()->GenProgramsARB)
#define glProgramEnvParameter4dARB       (_GET_TLS_PROCTABLE()->ProgramEnvParameter4dARB)
#define glProgramEnvParameter4dvARB      (_GET_TLS_PROCTABLE()->ProgramEnvParameter4dvARB)
#define glProgramEnvParameter4fARB       (_GET_TLS_PROCTABLE()->ProgramEnvParameter4fARB)
#define glProgramEnvParameter4fvARB      (_GET_TLS_PROCTABLE()->ProgramEnvParameter4fvARB)
#define glProgramLocalParameter4dARB     (_GET_TLS_PROCTABLE()->ProgramLocalParameter4dARB)
#define glProgramLocalParameter4dvARB    (_GET_TLS_PROCTABLE()->ProgramLocalParameter4dvARB)
#define glProgramLocalParameter4fARB     (_GET_TLS_PROCTABLE()->ProgramLocalParameter4fARB)
#define glProgramLocalParameter4fvARB    (_GET_TLS_PROCTABLE()->ProgramLocalParameter4fvARB)
#define glGetProgramEnvParameterdvARB    (_GET_TLS_PROCTABLE()->GetProgramEnvParameterdvARB)
#define glGetProgramEnvParameterfvARB    (_GET_TLS_PROCTABLE()->GetProgramEnvParameterfvARB)
#define glGetProgramLocalParameterdvARB  (_GET_TLS_PROCTABLE()->GetProgramLocalParameterdvARB)
#define glGetProgramLocalParameterfvARB  (_GET_TLS_PROCTABLE()->GetProgramLocalParameterfvARB)
#define glGetProgramivARB                (_GET_TLS_PROCTABLE()->GetProgramivARB)
#define glGetProgramStringARB            (_GET_TLS_PROCTABLE()->GetProgramStringARB)
#define glGetVertexAttribdvARB           (_GET_TLS_PROCTABLE()->GetVertexAttribdvARB)
#define glGetVertexAttribfvARB           (_GET_TLS_PROCTABLE()->GetVertexAttribfvARB)
#define glGetVertexAttribivARB           (_GET_TLS_PROCTABLE()->GetVertexAttribivARB)
#define glGetVertexAttribPointervARB     (_GET_TLS_PROCTABLE()->GetVertexAttribPointervARB)
#define glIsProgramARB                   (_GET_TLS_PROCTABLE()->IsProgramARB)
#define glBindBufferARB                  (_GET_TLS_PROCTABLE()->BindBufferARB)
#define glDeleteBuffersARB               (_GET_TLS_PROCTABLE()->DeleteBuffersARB)
#define glGenBuffersARB                  (_GET_TLS_PROCTABLE()->GenBuffersARB)
#define glIsBufferARB                    (_GET_TLS_PROCTABLE()->IsBufferARB)
#define glBufferDataARB                  (_GET_TLS_PROCTABLE()->BufferDataARB)
#define glBufferSubDataARB               (_GET_TLS_PROCTABLE()->BufferSubDataARB)
#define glGetBufferSubDataARB            (_GET_TLS_PROCTABLE()->GetBufferSubDataARB)
#define glMapBufferARB                   (_GET_TLS_PROCTABLE()->MapBufferARB)
#define glUnmapBufferARB                 (_GET_TLS_PROCTABLE()->UnmapBufferARB)
#define glGetBufferParameterivARB        (_GET_TLS_PROCTABLE()->GetBufferParameterivARB)
#define glGetBufferPointervARB           (_GET_TLS_PROCTABLE()->GetBufferPointervARB)
#define glGenQueriesARB                  (_GET_TLS_PROCTABLE()->GenQueriesARB)
#define glDeleteQueriesARB               (_GET_TLS_PROCTABLE()->DeleteQueriesARB)
#define glIsQueryARB                     (_GET_TLS_PROCTABLE()->IsQueryARB)
#define glBeginQueryARB                  (_GET_TLS_PROCTABLE()->BeginQueryARB)
#define glEndQueryARB                    (_GET_TLS_PROCTABLE()->EndQueryARB)
#define glGetQueryivARB                  (_GET_TLS_PROCTABLE()->GetQueryivARB)
#define glGetQueryObjectivARB            (_GET_TLS_PROCTABLE()->GetQueryObjectivARB)
#define glGetQueryObjectuivARB           (_GET_TLS_PROCTABLE()->GetQueryObjectuivARB)
#define glDeleteObjectARB                (_GET_TLS_PROCTABLE()->DeleteObjectARB)
#define glGetHandleARB                   (_GET_TLS_PROCTABLE()->GetHandleARB)
#define glDetachObjectARB                (_GET_TLS_PROCTABLE()->DetachObjectARB)
#define glCreateShaderObjectARB          (_GET_TLS_PROCTABLE()->CreateShaderObjectARB)
#define glShaderSourceARB                (_GET_TLS_PROCTABLE()->ShaderSourceARB)
#define glCompileShaderARB               (_GET_TLS_PROCTABLE()->CompileShaderARB)
#define glCreateProgramObjectARB         (_GET_TLS_PROCTABLE()->CreateProgramObjectARB)
#define glAttachObjectARB                (_GET_TLS_PROCTABLE()->AttachObjectARB)
#define glLinkProgramARB                 (_GET_TLS_PROCTABLE()->LinkProgramARB)
#define glUseProgramObjectARB            (_GET_TLS_PROCTABLE()->UseProgramObjectARB)
#define glValidateProgramARB             (_GET_TLS_PROCTABLE()->ValidateProgramARB)
#define glUniform1fARB                   (_GET_TLS_PROCTABLE()->Uniform1fARB)
#define glUniform2fARB                   (_GET_TLS_PROCTABLE()->Uniform2fARB)
#define glUniform3fARB                   (_GET_TLS_PROCTABLE()->Uniform3fARB)
#define glUniform4fARB                   (_GET_TLS_PROCTABLE()->Uniform4fARB)
#define glUniform1iARB                   (_GET_TLS_PROCTABLE()->Uniform1iARB)
#define glUniform2iARB                   (_GET_TLS_PROCTABLE()->Uniform2iARB)
#define glUniform3iARB                   (_GET_TLS_PROCTABLE()->Uniform3iARB)
#define glUniform4iARB                   (_GET_TLS_PROCTABLE()->Uniform4iARB)
#define glUniform1fvARB                  (_GET_TLS_PROCTABLE()->Uniform1fvARB)
#define glUniform2fvARB                  (_GET_TLS_PROCTABLE()->Uniform2fvARB)
#define glUniform3fvARB                  (_GET_TLS_PROCTABLE()->Uniform3fvARB)
#define glUniform4fvARB                  (_GET_TLS_PROCTABLE()->Uniform4fvARB)
#define glUniform1ivARB                  (_GET_TLS_PROCTABLE()->Uniform1ivARB)
#define glUniform2ivARB                  (_GET_TLS_PROCTABLE()->Uniform2ivARB)
#define glUniform3ivARB                  (_GET_TLS_PROCTABLE()->Uniform3ivARB)
#define glUniform4ivARB                  (_GET_TLS_PROCTABLE()->Uniform4ivARB)
#define glUniformMatrix2fvARB            (_GET_TLS_PROCTABLE()->UniformMatrix2fvARB)
#define glUniformMatrix3fvARB            (_GET_TLS_PROCTABLE()->UniformMatrix3fvARB)
#define glUniformMatrix4fvARB            (_GET_TLS_PROCTABLE()->UniformMatrix4fvARB)
#define glGetObjectParameterfvARB        (_GET_TLS_PROCTABLE()->GetObjectParameterfvARB)
#define glGetObjectParameterivARB        (_GET_TLS_PROCTABLE()->GetObjectParameterivARB)
#define glGetInfoLogARB                  (_GET_TLS_PROCTABLE()->GetInfoLogARB)
#define glGetAttachedObjectsARB          (_GET_TLS_PROCTABLE()->GetAttachedObjectsARB)
#define glGetUniformLocationARB          (_GET_TLS_PROCTABLE()->GetUniformLocationARB)
#define glGetActiveUniformARB            (_GET_TLS_PROCTABLE()->GetActiveUniformARB)
#define glGetUniformfvARB                (_GET_TLS_PROCTABLE()->GetUniformfvARB)
#define glGetUniformivARB                (_GET_TLS_PROCTABLE()->GetUniformivARB)
#define glGetShaderSourceARB             (_GET_TLS_PROCTABLE()->GetShaderSourceARB)
#define glBindAttribLocationARB          (_GET_TLS_PROCTABLE()->BindAttribLocationARB)
#define glGetActiveAttribARB             (_GET_TLS_PROCTABLE()->GetActiveAttribARB)
#define glGetAttribLocationARB           (_GET_TLS_PROCTABLE()->GetAttribLocationARB)
#define glBlendColorEXT                  (_GET_TLS_PROCTABLE()->BlendColorEXT)
#define glPolygonOffsetEXT               (_GET_TLS_PROCTABLE()->PolygonOffsetEXT)
#define glTexImage3DEXT                  (_GET_TLS_PROCTABLE()->TexImage3DEXT)
#define glTexSubImage3DEXT               (_GET_TLS_PROCTABLE()->TexSubImage3DEXT)
#define glGetTexFilterFuncSGIS           (_GET_TLS_PROCTABLE()->GetTexFilterFuncSGIS)
#define glTexFilterFuncSGIS              (_GET_TLS_PROCTABLE()->TexFilterFuncSGIS)
#define glTexSubImage1DEXT               (_GET_TLS_PROCTABLE()->TexSubImage1DEXT)
#define glTexSubImage2DEXT               (_GET_TLS_PROCTABLE()->TexSubImage2DEXT)
#define glCopyTexImage1DEXT              (_GET_TLS_PROCTABLE()->CopyTexImage1DEXT)
#define glCopyTexImage2DEXT              (_GET_TLS_PROCTABLE()->CopyTexImage2DEXT)
#define glCopyTexSubImage1DEXT           (_GET_TLS_PROCTABLE()->CopyTexSubImage1DEXT)
#define glCopyTexSubImage2DEXT           (_GET_TLS_PROCTABLE()->CopyTexSubImage2DEXT)
#define glCopyTexSubImage3DEXT           (_GET_TLS_PROCTABLE()->CopyTexSubImage3DEXT)
#define glGetHistogramEXT                (_GET_TLS_PROCTABLE()->GetHistogramEXT)
#define glGetHistogramParameterfvEXT     (_GET_TLS_PROCTABLE()->GetHistogramParameterfvEXT)
#define glGetHistogramParameterivEXT     (_GET_TLS_PROCTABLE()->GetHistogramParameterivEXT)
#define glGetMinmaxEXT                   (_GET_TLS_PROCTABLE()->GetMinmaxEXT)
#define glGetMinmaxParameterfvEXT        (_GET_TLS_PROCTABLE()->GetMinmaxParameterfvEXT)
#define glGetMinmaxParameterivEXT        (_GET_TLS_PROCTABLE()->GetMinmaxParameterivEXT)
#define glHistogramEXT                   (_GET_TLS_PROCTABLE()->HistogramEXT)
#define glMinmaxEXT                      (_GET_TLS_PROCTABLE()->MinmaxEXT)
#define glResetHistogramEXT              (_GET_TLS_PROCTABLE()->ResetHistogramEXT)
#define glResetMinmaxEXT                 (_GET_TLS_PROCTABLE()->ResetMinmaxEXT)
#define glConvolutionFilter1DEXT         (_GET_TLS_PROCTABLE()->ConvolutionFilter1DEXT)
#define glConvolutionFilter2DEXT         (_GET_TLS_PROCTABLE()->ConvolutionFilter2DEXT)
#define glConvolutionParameterfEXT       (_GET_TLS_PROCTABLE()->ConvolutionParameterfEXT)
#define glConvolutionParameterfvEXT      (_GET_TLS_PROCTABLE()->ConvolutionParameterfvEXT)
#define glConvolutionParameteriEXT       (_GET_TLS_PROCTABLE()->ConvolutionParameteriEXT)
#define glConvolutionParameterivEXT      (_GET_TLS_PROCTABLE()->ConvolutionParameterivEXT)
#define glCopyConvolutionFilter1DEXT     (_GET_TLS_PROCTABLE()->CopyConvolutionFilter1DEXT)
#define glCopyConvolutionFilter2DEXT     (_GET_TLS_PROCTABLE()->CopyConvolutionFilter2DEXT)
#define glGetConvolutionFilterEXT        (_GET_TLS_PROCTABLE()->GetConvolutionFilterEXT)
#define glGetConvolutionParameterfvEXT   (_GET_TLS_PROCTABLE()->GetConvolutionParameterfvEXT)
#define glGetConvolutionParameterivEXT   (_GET_TLS_PROCTABLE()->GetConvolutionParameterivEXT)
#define glGetSeparableFilterEXT          (_GET_TLS_PROCTABLE()->GetSeparableFilterEXT)
#define glSeparableFilter2DEXT           (_GET_TLS_PROCTABLE()->SeparableFilter2DEXT)
#define glColorTableSGI                  (_GET_TLS_PROCTABLE()->ColorTableSGI)
#define glColorTableParameterfvSGI       (_GET_TLS_PROCTABLE()->ColorTableParameterfvSGI)
#define glColorTableParameterivSGI       (_GET_TLS_PROCTABLE()->ColorTableParameterivSGI)
#define glCopyColorTableSGI              (_GET_TLS_PROCTABLE()->CopyColorTableSGI)
#define glGetColorTableSGI               (_GET_TLS_PROCTABLE()->GetColorTableSGI)
#define glGetColorTableParameterfvSGI    (_GET_TLS_PROCTABLE()->GetColorTableParameterfvSGI)
#define glGetColorTableParameterivSGI    (_GET_TLS_PROCTABLE()->GetColorTableParameterivSGI)
#define glPixelTexGenSGIX                (_GET_TLS_PROCTABLE()->PixelTexGenSGIX)
#define glPixelTexGenParameteriSGIS      (_GET_TLS_PROCTABLE()->PixelTexGenParameteriSGIS)
#define glPixelTexGenParameterivSGIS     (_GET_TLS_PROCTABLE()->PixelTexGenParameterivSGIS)
#define glPixelTexGenParameterfSGIS      (_GET_TLS_PROCTABLE()->PixelTexGenParameterfSGIS)
#define glPixelTexGenParameterfvSGIS     (_GET_TLS_PROCTABLE()->PixelTexGenParameterfvSGIS)
#define glGetPixelTexGenParameterivSGIS  (_GET_TLS_PROCTABLE()->GetPixelTexGenParameterivSGIS)
#define glGetPixelTexGenParameterfvSGIS  (_GET_TLS_PROCTABLE()->GetPixelTexGenParameterfvSGIS)
#define glTexImage4DSGIS                 (_GET_TLS_PROCTABLE()->TexImage4DSGIS)
#define glTexSubImage4DSGIS              (_GET_TLS_PROCTABLE()->TexSubImage4DSGIS)
#define glAreTexturesResidentEXT         (_GET_TLS_PROCTABLE()->AreTexturesResidentEXT)
#define glBindTextureEXT                 (_GET_TLS_PROCTABLE()->BindTextureEXT)
#define glDeleteTexturesEXT              (_GET_TLS_PROCTABLE()->DeleteTexturesEXT)
#define glGenTexturesEXT                 (_GET_TLS_PROCTABLE()->GenTexturesEXT)
#define glIsTextureEXT                   (_GET_TLS_PROCTABLE()->IsTextureEXT)
#define glPrioritizeTexturesEXT          (_GET_TLS_PROCTABLE()->PrioritizeTexturesEXT)
#define glDetailTexFuncSGIS              (_GET_TLS_PROCTABLE()->DetailTexFuncSGIS)
#define glGetDetailTexFuncSGIS           (_GET_TLS_PROCTABLE()->GetDetailTexFuncSGIS)
#define glSharpenTexFuncSGIS             (_GET_TLS_PROCTABLE()->SharpenTexFuncSGIS)
#define glGetSharpenTexFuncSGIS          (_GET_TLS_PROCTABLE()->GetSharpenTexFuncSGIS)
#define glSampleMaskSGIS                 (_GET_TLS_PROCTABLE()->SampleMaskSGIS)
#define glSamplePatternSGIS              (_GET_TLS_PROCTABLE()->SamplePatternSGIS)
#define glArrayElementEXT                (_GET_TLS_PROCTABLE()->ArrayElementEXT)
#define glColorPointerEXT                (_GET_TLS_PROCTABLE()->ColorPointerEXT)
#define glDrawArraysEXT                  (_GET_TLS_PROCTABLE()->DrawArraysEXT)
#define glEdgeFlagPointerEXT             (_GET_TLS_PROCTABLE()->EdgeFlagPointerEXT)
#define glGetPointervEXT                 (_GET_TLS_PROCTABLE()->GetPointervEXT)
#define glIndexPointerEXT                (_GET_TLS_PROCTABLE()->IndexPointerEXT)
#define glNormalPointerEXT               (_GET_TLS_PROCTABLE()->NormalPointerEXT)
#define glTexCoordPointerEXT             (_GET_TLS_PROCTABLE()->TexCoordPointerEXT)
#define glVertexPointerEXT               (_GET_TLS_PROCTABLE()->VertexPointerEXT)
#define glBlendEquationEXT               (_GET_TLS_PROCTABLE()->BlendEquationEXT)
#define glSpriteParameterfSGIX           (_GET_TLS_PROCTABLE()->SpriteParameterfSGIX)
#define glSpriteParameterfvSGIX          (_GET_TLS_PROCTABLE()->SpriteParameterfvSGIX)
#define glSpriteParameteriSGIX           (_GET_TLS_PROCTABLE()->SpriteParameteriSGIX)
#define glSpriteParameterivSGIX          (_GET_TLS_PROCTABLE()->SpriteParameterivSGIX)
#define glPointParameterfEXT             (_GET_TLS_PROCTABLE()->PointParameterfEXT)
#define glPointParameterfvEXT            (_GET_TLS_PROCTABLE()->PointParameterfvEXT)
#define glPointParameterfSGIS            (_GET_TLS_PROCTABLE()->PointParameterfSGIS)
#define glPointParameterfvSGIS           (_GET_TLS_PROCTABLE()->PointParameterfvSGIS)
#define glGetInstrumentsSGIX             (_GET_TLS_PROCTABLE()->GetInstrumentsSGIX)
#define glInstrumentsBufferSGIX          (_GET_TLS_PROCTABLE()->InstrumentsBufferSGIX)
#define glPollInstrumentsSGIX            (_GET_TLS_PROCTABLE()->PollInstrumentsSGIX)
#define glReadInstrumentsSGIX            (_GET_TLS_PROCTABLE()->ReadInstrumentsSGIX)
#define glStartInstrumentsSGIX           (_GET_TLS_PROCTABLE()->StartInstrumentsSGIX)
#define glStopInstrumentsSGIX            (_GET_TLS_PROCTABLE()->StopInstrumentsSGIX)
#define glFrameZoomSGIX                  (_GET_TLS_PROCTABLE()->FrameZoomSGIX)
#define glTagSampleBufferSGIX            (_GET_TLS_PROCTABLE()->TagSampleBufferSGIX)
#define glDeformationMap3dSGIX           (_GET_TLS_PROCTABLE()->DeformationMap3dSGIX)
#define glDeformationMap3fSGIX           (_GET_TLS_PROCTABLE()->DeformationMap3fSGIX)
#define glDeformSGIX                     (_GET_TLS_PROCTABLE()->DeformSGIX)
#define glLoadIdentityDeformationMapSGIX (_GET_TLS_PROCTABLE()->LoadIdentityDeformationMapSGIX)
#define glReferencePlaneSGIX             (_GET_TLS_PROCTABLE()->ReferencePlaneSGIX)
#define glFlushRasterSGIX                (_GET_TLS_PROCTABLE()->FlushRasterSGIX)
#define glFogFuncSGIS                    (_GET_TLS_PROCTABLE()->FogFuncSGIS)
#define glGetFogFuncSGIS                 (_GET_TLS_PROCTABLE()->GetFogFuncSGIS)
#define glImageTransformParameteriHP     (_GET_TLS_PROCTABLE()->ImageTransformParameteriHP)
#define glImageTransformParameterfHP     (_GET_TLS_PROCTABLE()->ImageTransformParameterfHP)
#define glImageTransformParameterivHP    (_GET_TLS_PROCTABLE()->ImageTransformParameterivHP)
#define glImageTransformParameterfvHP    (_GET_TLS_PROCTABLE()->ImageTransformParameterfvHP)
#define glGetImageTransformParameterivHP (_GET_TLS_PROCTABLE()->GetImageTransformParameterivHP)
#define glGetImageTransformParameterfvHP (_GET_TLS_PROCTABLE()->GetImageTransformParameterfvHP)
#define glColorSubTableEXT               (_GET_TLS_PROCTABLE()->ColorSubTableEXT)
#define glCopyColorSubTableEXT           (_GET_TLS_PROCTABLE()->CopyColorSubTableEXT)
#define glHintPGI                        (_GET_TLS_PROCTABLE()->HintPGI)
#define glColorTableEXT                  (_GET_TLS_PROCTABLE()->ColorTableEXT)
#define glGetColorTableEXT               (_GET_TLS_PROCTABLE()->GetColorTableEXT)
#define glGetColorTableParameterivEXT    (_GET_TLS_PROCTABLE()->GetColorTableParameterivEXT)
#define glGetColorTableParameterfvEXT    (_GET_TLS_PROCTABLE()->GetColorTableParameterfvEXT)
#define glGetListParameterfvSGIX         (_GET_TLS_PROCTABLE()->GetListParameterfvSGIX)
#define glGetListParameterivSGIX         (_GET_TLS_PROCTABLE()->GetListParameterivSGIX)
#define glListParameterfSGIX             (_GET_TLS_PROCTABLE()->ListParameterfSGIX)
#define glListParameterfvSGIX            (_GET_TLS_PROCTABLE()->ListParameterfvSGIX)
#define glListParameteriSGIX             (_GET_TLS_PROCTABLE()->ListParameteriSGIX)
#define glListParameterivSGIX            (_GET_TLS_PROCTABLE()->ListParameterivSGIX)
#define glIndexMaterialEXT               (_GET_TLS_PROCTABLE()->IndexMaterialEXT)
#define glIndexFuncEXT                   (_GET_TLS_PROCTABLE()->IndexFuncEXT)
#define glLockArraysEXT                  (_GET_TLS_PROCTABLE()->LockArraysEXT)
#define glUnlockArraysEXT                (_GET_TLS_PROCTABLE()->UnlockArraysEXT)
#define glCullParameterdvEXT             (_GET_TLS_PROCTABLE()->CullParameterdvEXT)
#define glCullParameterfvEXT             (_GET_TLS_PROCTABLE()->CullParameterfvEXT)
#define glFragmentColorMaterialSGIX      (_GET_TLS_PROCTABLE()->FragmentColorMaterialSGIX)
#define glFragmentLightfSGIX             (_GET_TLS_PROCTABLE()->FragmentLightfSGIX)
#define glFragmentLightfvSGIX            (_GET_TLS_PROCTABLE()->FragmentLightfvSGIX)
#define glFragmentLightiSGIX             (_GET_TLS_PROCTABLE()->FragmentLightiSGIX)
#define glFragmentLightivSGIX            (_GET_TLS_PROCTABLE()->FragmentLightivSGIX)
#define glFragmentLightModelfSGIX        (_GET_TLS_PROCTABLE()->FragmentLightModelfSGIX)
#define glFragmentLightModelfvSGIX       (_GET_TLS_PROCTABLE()->FragmentLightModelfvSGIX)
#define glFragmentLightModeliSGIX        (_GET_TLS_PROCTABLE()->FragmentLightModeliSGIX)
#define glFragmentLightModelivSGIX       (_GET_TLS_PROCTABLE()->FragmentLightModelivSGIX)
#define glFragmentMaterialfSGIX          (_GET_TLS_PROCTABLE()->FragmentMaterialfSGIX)
#define glFragmentMaterialfvSGIX         (_GET_TLS_PROCTABLE()->FragmentMaterialfvSGIX)
#define glFragmentMaterialiSGIX          (_GET_TLS_PROCTABLE()->FragmentMaterialiSGIX)
#define glFragmentMaterialivSGIX         (_GET_TLS_PROCTABLE()->FragmentMaterialivSGIX)
#define glGetFragmentLightfvSGIX         (_GET_TLS_PROCTABLE()->GetFragmentLightfvSGIX)
#define glGetFragmentLightivSGIX         (_GET_TLS_PROCTABLE()->GetFragmentLightivSGIX)
#define glGetFragmentMaterialfvSGIX      (_GET_TLS_PROCTABLE()->GetFragmentMaterialfvSGIX)
#define glGetFragmentMaterialivSGIX      (_GET_TLS_PROCTABLE()->GetFragmentMaterialivSGIX)
#define glLightEnviSGIX                  (_GET_TLS_PROCTABLE()->LightEnviSGIX)
#define glDrawRangeElementsEXT           (_GET_TLS_PROCTABLE()->DrawRangeElementsEXT)
#define glApplyTextureEXT                (_GET_TLS_PROCTABLE()->ApplyTextureEXT)
#define glTextureLightEXT                (_GET_TLS_PROCTABLE()->TextureLightEXT)
#define glTextureMaterialEXT             (_GET_TLS_PROCTABLE()->TextureMaterialEXT)
#define glAsyncMarkerSGIX                (_GET_TLS_PROCTABLE()->AsyncMarkerSGIX)
#define glFinishAsyncSGIX                (_GET_TLS_PROCTABLE()->FinishAsyncSGIX)
#define glPollAsyncSGIX                  (_GET_TLS_PROCTABLE()->PollAsyncSGIX)
#define glGenAsyncMarkersSGIX            (_GET_TLS_PROCTABLE()->GenAsyncMarkersSGIX)
#define glDeleteAsyncMarkersSGIX         (_GET_TLS_PROCTABLE()->DeleteAsyncMarkersSGIX)
#define glIsAsyncMarkerSGIX              (_GET_TLS_PROCTABLE()->IsAsyncMarkerSGIX)
#define glVertexPointervINTEL            (_GET_TLS_PROCTABLE()->VertexPointervINTEL)
#define glNormalPointervINTEL            (_GET_TLS_PROCTABLE()->NormalPointervINTEL)
#define glColorPointervINTEL             (_GET_TLS_PROCTABLE()->ColorPointervINTEL)
#define glTexCoordPointervINTEL          (_GET_TLS_PROCTABLE()->TexCoordPointervINTEL)
#define glPixelTransformParameteriEXT    (_GET_TLS_PROCTABLE()->PixelTransformParameteriEXT)
#define glPixelTransformParameterfEXT    (_GET_TLS_PROCTABLE()->PixelTransformParameterfEXT)
#define glPixelTransformParameterivEXT   (_GET_TLS_PROCTABLE()->PixelTransformParameterivEXT)
#define glPixelTransformParameterfvEXT   (_GET_TLS_PROCTABLE()->PixelTransformParameterfvEXT)
#define glSecondaryColor3bEXT            (_GET_TLS_PROCTABLE()->SecondaryColor3bEXT)
#define glSecondaryColor3bvEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3bvEXT)
#define glSecondaryColor3dEXT            (_GET_TLS_PROCTABLE()->SecondaryColor3dEXT)
#define glSecondaryColor3dvEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3dvEXT)
#define glSecondaryColor3fEXT            (_GET_TLS_PROCTABLE()->SecondaryColor3fEXT)
#define glSecondaryColor3fvEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3fvEXT)
#define glSecondaryColor3iEXT            (_GET_TLS_PROCTABLE()->SecondaryColor3iEXT)
#define glSecondaryColor3ivEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3ivEXT)
#define glSecondaryColor3sEXT            (_GET_TLS_PROCTABLE()->SecondaryColor3sEXT)
#define glSecondaryColor3svEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3svEXT)
#define glSecondaryColor3ubEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3ubEXT)
#define glSecondaryColor3ubvEXT          (_GET_TLS_PROCTABLE()->SecondaryColor3ubvEXT)
#define glSecondaryColor3uiEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3uiEXT)
#define glSecondaryColor3uivEXT          (_GET_TLS_PROCTABLE()->SecondaryColor3uivEXT)
#define glSecondaryColor3usEXT           (_GET_TLS_PROCTABLE()->SecondaryColor3usEXT)
#define glSecondaryColor3usvEXT          (_GET_TLS_PROCTABLE()->SecondaryColor3usvEXT)
#define glSecondaryColorPointerEXT       (_GET_TLS_PROCTABLE()->SecondaryColorPointerEXT)
#define glTextureNormalEXT               (_GET_TLS_PROCTABLE()->TextureNormalEXT)
#define glMultiDrawArraysEXT             (_GET_TLS_PROCTABLE()->MultiDrawArraysEXT)
#define glMultiDrawElementsEXT           (_GET_TLS_PROCTABLE()->MultiDrawElementsEXT)
#define glFogCoordfEXT                   (_GET_TLS_PROCTABLE()->FogCoordfEXT)
#define glFogCoordfvEXT                  (_GET_TLS_PROCTABLE()->FogCoordfvEXT)
#define glFogCoorddEXT                   (_GET_TLS_PROCTABLE()->FogCoorddEXT)
#define glFogCoorddvEXT                  (_GET_TLS_PROCTABLE()->FogCoorddvEXT)
#define glFogCoordPointerEXT             (_GET_TLS_PROCTABLE()->FogCoordPointerEXT)
#define glTangent3bEXT                   (_GET_TLS_PROCTABLE()->Tangent3bEXT)
#define glTangent3bvEXT                  (_GET_TLS_PROCTABLE()->Tangent3bvEXT)
#define glTangent3dEXT                   (_GET_TLS_PROCTABLE()->Tangent3dEXT)
#define glTangent3dvEXT                  (_GET_TLS_PROCTABLE()->Tangent3dvEXT)
#define glTangent3fEXT                   (_GET_TLS_PROCTABLE()->Tangent3fEXT)
#define glTangent3fvEXT                  (_GET_TLS_PROCTABLE()->Tangent3fvEXT)
#define glTangent3iEXT                   (_GET_TLS_PROCTABLE()->Tangent3iEXT)
#define glTangent3ivEXT                  (_GET_TLS_PROCTABLE()->Tangent3ivEXT)
#define glTangent3sEXT                   (_GET_TLS_PROCTABLE()->Tangent3sEXT)
#define glTangent3svEXT                  (_GET_TLS_PROCTABLE()->Tangent3svEXT)
#define glBinormal3bEXT                  (_GET_TLS_PROCTABLE()->Binormal3bEXT)
#define glBinormal3bvEXT                 (_GET_TLS_PROCTABLE()->Binormal3bvEXT)
#define glBinormal3dEXT                  (_GET_TLS_PROCTABLE()->Binormal3dEXT)
#define glBinormal3dvEXT                 (_GET_TLS_PROCTABLE()->Binormal3dvEXT)
#define glBinormal3fEXT                  (_GET_TLS_PROCTABLE()->Binormal3fEXT)
#define glBinormal3fvEXT                 (_GET_TLS_PROCTABLE()->Binormal3fvEXT)
#define glBinormal3iEXT                  (_GET_TLS_PROCTABLE()->Binormal3iEXT)
#define glBinormal3ivEXT                 (_GET_TLS_PROCTABLE()->Binormal3ivEXT)
#define glBinormal3sEXT                  (_GET_TLS_PROCTABLE()->Binormal3sEXT)
#define glBinormal3svEXT                 (_GET_TLS_PROCTABLE()->Binormal3svEXT)
#define glTangentPointerEXT              (_GET_TLS_PROCTABLE()->TangentPointerEXT)
#define glBinormalPointerEXT             (_GET_TLS_PROCTABLE()->BinormalPointerEXT)
#define glFinishTextureSUNX              (_GET_TLS_PROCTABLE()->FinishTextureSUNX)
#define glGlobalAlphaFactorbSUN          (_GET_TLS_PROCTABLE()->GlobalAlphaFactorbSUN)
#define glGlobalAlphaFactorsSUN          (_GET_TLS_PROCTABLE()->GlobalAlphaFactorsSUN)
#define glGlobalAlphaFactoriSUN          (_GET_TLS_PROCTABLE()->GlobalAlphaFactoriSUN)
#define glGlobalAlphaFactorfSUN          (_GET_TLS_PROCTABLE()->GlobalAlphaFactorfSUN)
#define glGlobalAlphaFactordSUN          (_GET_TLS_PROCTABLE()->GlobalAlphaFactordSUN)
#define glGlobalAlphaFactorubSUN         (_GET_TLS_PROCTABLE()->GlobalAlphaFactorubSUN)
#define glGlobalAlphaFactorusSUN         (_GET_TLS_PROCTABLE()->GlobalAlphaFactorusSUN)
#define glGlobalAlphaFactoruiSUN         (_GET_TLS_PROCTABLE()->GlobalAlphaFactoruiSUN)
#define glReplacementCodeuiSUN           (_GET_TLS_PROCTABLE()->ReplacementCodeuiSUN)
#define glReplacementCodeusSUN           (_GET_TLS_PROCTABLE()->ReplacementCodeusSUN)
#define glReplacementCodeubSUN           (_GET_TLS_PROCTABLE()->ReplacementCodeubSUN)
#define glReplacementCodeuivSUN          (_GET_TLS_PROCTABLE()->ReplacementCodeuivSUN)
#define glReplacementCodeusvSUN          (_GET_TLS_PROCTABLE()->ReplacementCodeusvSUN)
#define glReplacementCodeubvSUN          (_GET_TLS_PROCTABLE()->ReplacementCodeubvSUN)
#define glReplacementCodePointerSUN      (_GET_TLS_PROCTABLE()->ReplacementCodePointerSUN)
#define glColor4ubVertex2fSUN            (_GET_TLS_PROCTABLE()->Color4ubVertex2fSUN)
#define glColor4ubVertex2fvSUN           (_GET_TLS_PROCTABLE()->Color4ubVertex2fvSUN)
#define glColor4ubVertex3fSUN            (_GET_TLS_PROCTABLE()->Color4ubVertex3fSUN)
#define glColor4ubVertex3fvSUN           (_GET_TLS_PROCTABLE()->Color4ubVertex3fvSUN)
#define glColor3fVertex3fSUN             (_GET_TLS_PROCTABLE()->Color3fVertex3fSUN)
#define glColor3fVertex3fvSUN            (_GET_TLS_PROCTABLE()->Color3fVertex3fvSUN)
#define glNormal3fVertex3fSUN            (_GET_TLS_PROCTABLE()->Normal3fVertex3fSUN)
#define glNormal3fVertex3fvSUN           (_GET_TLS_PROCTABLE()->Normal3fVertex3fvSUN)
#define glColor4fNormal3fVertex3fSUN     (_GET_TLS_PROCTABLE()->Color4fNormal3fVertex3fSUN)
#define glColor4fNormal3fVertex3fvSUN    (_GET_TLS_PROCTABLE()->Color4fNormal3fVertex3fvSUN)
#define glTexCoord2fVertex3fSUN          (_GET_TLS_PROCTABLE()->TexCoord2fVertex3fSUN)
#define glTexCoord2fVertex3fvSUN         (_GET_TLS_PROCTABLE()->TexCoord2fVertex3fvSUN)
#define glTexCoord4fVertex4fSUN          (_GET_TLS_PROCTABLE()->TexCoord4fVertex4fSUN)
#define glTexCoord4fVertex4fvSUN         (_GET_TLS_PROCTABLE()->TexCoord4fVertex4fvSUN)
#define glTexCoord2fColor4ubVertex3fSUN  (_GET_TLS_PROCTABLE()->TexCoord2fColor4ubVertex3fSUN)
#define glTexCoord2fColor4ubVertex3fvSUN (_GET_TLS_PROCTABLE()->TexCoord2fColor4ubVertex3fvSUN)
#define glTexCoord2fColor3fVertex3fSUN   (_GET_TLS_PROCTABLE()->TexCoord2fColor3fVertex3fSUN)
#define glTexCoord2fColor3fVertex3fvSUN  (_GET_TLS_PROCTABLE()->TexCoord2fColor3fVertex3fvSUN)
#define glTexCoord2fNormal3fVertex3fSUN  (_GET_TLS_PROCTABLE()->TexCoord2fNormal3fVertex3fSUN)
#define glTexCoord2fNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->TexCoord2fNormal3fVertex3fvSUN)
#define glTexCoord2fColor4fNormal3fVertex3fSUN (_GET_TLS_PROCTABLE()->TexCoord2fColor4fNormal3fVertex3fSUN)
#define glTexCoord2fColor4fNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->TexCoord2fColor4fNormal3fVertex3fvSUN)
#define glTexCoord4fColor4fNormal3fVertex4fSUN (_GET_TLS_PROCTABLE()->TexCoord4fColor4fNormal3fVertex4fSUN)
#define glTexCoord4fColor4fNormal3fVertex4fvSUN (_GET_TLS_PROCTABLE()->TexCoord4fColor4fNormal3fVertex4fvSUN)
#define glReplacementCodeuiVertex3fSUN   (_GET_TLS_PROCTABLE()->ReplacementCodeuiVertex3fSUN)
#define glReplacementCodeuiVertex3fvSUN  (_GET_TLS_PROCTABLE()->ReplacementCodeuiVertex3fvSUN)
#define glReplacementCodeuiColor4ubVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor4ubVertex3fSUN)
#define glReplacementCodeuiColor4ubVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor4ubVertex3fvSUN)
#define glReplacementCodeuiColor3fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor3fVertex3fSUN)
#define glReplacementCodeuiColor3fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor3fVertex3fvSUN)
#define glReplacementCodeuiNormal3fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiNormal3fVertex3fSUN)
#define glReplacementCodeuiNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiNormal3fVertex3fvSUN)
#define glReplacementCodeuiColor4fNormal3fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor4fNormal3fVertex3fSUN)
#define glReplacementCodeuiColor4fNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiColor4fNormal3fVertex3fvSUN)
#define glReplacementCodeuiTexCoord2fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fVertex3fSUN)
#define glReplacementCodeuiTexCoord2fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fVertex3fvSUN)
#define glReplacementCodeuiTexCoord2fNormal3fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fNormal3fVertex3fSUN)
#define glReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fNormal3fVertex3fvSUN)
#define glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fSUN)
#define glReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN (_GET_TLS_PROCTABLE()->ReplacementCodeuiTexCoord2fColor4fNormal3fVertex3fvSUN)
#define glBlendFuncSeparateEXT           (_GET_TLS_PROCTABLE()->BlendFuncSeparateEXT)
#define glBlendFuncSeparateINGR          (_GET_TLS_PROCTABLE()->BlendFuncSeparateINGR)
#define glVertexWeightfEXT               (_GET_TLS_PROCTABLE()->VertexWeightfEXT)
#define glVertexWeightfvEXT              (_GET_TLS_PROCTABLE()->VertexWeightfvEXT)
#define glVertexWeightPointerEXT         (_GET_TLS_PROCTABLE()->VertexWeightPointerEXT)
#define glFlushVertexArrayRangeNV        (_GET_TLS_PROCTABLE()->FlushVertexArrayRangeNV)
#define glVertexArrayRangeNV             (_GET_TLS_PROCTABLE()->VertexArrayRangeNV)
#define glCombinerParameterfvNV          (_GET_TLS_PROCTABLE()->CombinerParameterfvNV)
#define glCombinerParameterfNV           (_GET_TLS_PROCTABLE()->CombinerParameterfNV)
#define glCombinerParameterivNV          (_GET_TLS_PROCTABLE()->CombinerParameterivNV)
#define glCombinerParameteriNV           (_GET_TLS_PROCTABLE()->CombinerParameteriNV)
#define glCombinerInputNV                (_GET_TLS_PROCTABLE()->CombinerInputNV)
#define glCombinerOutputNV               (_GET_TLS_PROCTABLE()->CombinerOutputNV)
#define glFinalCombinerInputNV           (_GET_TLS_PROCTABLE()->FinalCombinerInputNV)
#define glGetCombinerInputParameterfvNV  (_GET_TLS_PROCTABLE()->GetCombinerInputParameterfvNV)
#define glGetCombinerInputParameterivNV  (_GET_TLS_PROCTABLE()->GetCombinerInputParameterivNV)
#define glGetCombinerOutputParameterfvNV (_GET_TLS_PROCTABLE()->GetCombinerOutputParameterfvNV)
#define glGetCombinerOutputParameterivNV (_GET_TLS_PROCTABLE()->GetCombinerOutputParameterivNV)
#define glGetFinalCombinerInputParameterfvNV (_GET_TLS_PROCTABLE()->GetFinalCombinerInputParameterfvNV)
#define glGetFinalCombinerInputParameterivNV (_GET_TLS_PROCTABLE()->GetFinalCombinerInputParameterivNV)
#define glResizeBuffersMESA              (_GET_TLS_PROCTABLE()->ResizeBuffersMESA)
#define glWindowPos2dMESA                (_GET_TLS_PROCTABLE()->WindowPos2dMESA)
#define glWindowPos2dvMESA               (_GET_TLS_PROCTABLE()->WindowPos2dvMESA)
#define glWindowPos2fMESA                (_GET_TLS_PROCTABLE()->WindowPos2fMESA)
#define glWindowPos2fvMESA               (_GET_TLS_PROCTABLE()->WindowPos2fvMESA)
#define glWindowPos2iMESA                (_GET_TLS_PROCTABLE()->WindowPos2iMESA)
#define glWindowPos2ivMESA               (_GET_TLS_PROCTABLE()->WindowPos2ivMESA)
#define glWindowPos2sMESA                (_GET_TLS_PROCTABLE()->WindowPos2sMESA)
#define glWindowPos2svMESA               (_GET_TLS_PROCTABLE()->WindowPos2svMESA)
#define glWindowPos3dMESA                (_GET_TLS_PROCTABLE()->WindowPos3dMESA)
#define glWindowPos3dvMESA               (_GET_TLS_PROCTABLE()->WindowPos3dvMESA)
#define glWindowPos3fMESA                (_GET_TLS_PROCTABLE()->WindowPos3fMESA)
#define glWindowPos3fvMESA               (_GET_TLS_PROCTABLE()->WindowPos3fvMESA)
#define glWindowPos3iMESA                (_GET_TLS_PROCTABLE()->WindowPos3iMESA)
#define glWindowPos3ivMESA               (_GET_TLS_PROCTABLE()->WindowPos3ivMESA)
#define glWindowPos3sMESA                (_GET_TLS_PROCTABLE()->WindowPos3sMESA)
#define glWindowPos3svMESA               (_GET_TLS_PROCTABLE()->WindowPos3svMESA)
#define glWindowPos4dMESA                (_GET_TLS_PROCTABLE()->WindowPos4dMESA)
#define glWindowPos4dvMESA               (_GET_TLS_PROCTABLE()->WindowPos4dvMESA)
#define glWindowPos4fMESA                (_GET_TLS_PROCTABLE()->WindowPos4fMESA)
#define glWindowPos4fvMESA               (_GET_TLS_PROCTABLE()->WindowPos4fvMESA)
#define glWindowPos4iMESA                (_GET_TLS_PROCTABLE()->WindowPos4iMESA)
#define glWindowPos4ivMESA               (_GET_TLS_PROCTABLE()->WindowPos4ivMESA)
#define glWindowPos4sMESA                (_GET_TLS_PROCTABLE()->WindowPos4sMESA)
#define glWindowPos4svMESA               (_GET_TLS_PROCTABLE()->WindowPos4svMESA)
#define glMultiModeDrawArraysIBM         (_GET_TLS_PROCTABLE()->MultiModeDrawArraysIBM)
#define glMultiModeDrawElementsIBM       (_GET_TLS_PROCTABLE()->MultiModeDrawElementsIBM)
#define glColorPointerListIBM            (_GET_TLS_PROCTABLE()->ColorPointerListIBM)
#define glSecondaryColorPointerListIBM   (_GET_TLS_PROCTABLE()->SecondaryColorPointerListIBM)
#define glEdgeFlagPointerListIBM         (_GET_TLS_PROCTABLE()->EdgeFlagPointerListIBM)
#define glFogCoordPointerListIBM         (_GET_TLS_PROCTABLE()->FogCoordPointerListIBM)
#define glIndexPointerListIBM            (_GET_TLS_PROCTABLE()->IndexPointerListIBM)
#define glNormalPointerListIBM           (_GET_TLS_PROCTABLE()->NormalPointerListIBM)
#define glTexCoordPointerListIBM         (_GET_TLS_PROCTABLE()->TexCoordPointerListIBM)
#define glVertexPointerListIBM           (_GET_TLS_PROCTABLE()->VertexPointerListIBM)
#define glTbufferMask3DFX                (_GET_TLS_PROCTABLE()->TbufferMask3DFX)
#define glSampleMaskEXT                  (_GET_TLS_PROCTABLE()->SampleMaskEXT)
#define glSamplePatternEXT               (_GET_TLS_PROCTABLE()->SamplePatternEXT)
#define glTextureColorMaskSGIS           (_GET_TLS_PROCTABLE()->TextureColorMaskSGIS)
#define glIglooInterfaceSGIX             (_GET_TLS_PROCTABLE()->IglooInterfaceSGIX)
#define glDeleteFencesNV                 (_GET_TLS_PROCTABLE()->DeleteFencesNV)
#define glGenFencesNV                    (_GET_TLS_PROCTABLE()->GenFencesNV)
#define glIsFenceNV                      (_GET_TLS_PROCTABLE()->IsFenceNV)
#define glTestFenceNV                    (_GET_TLS_PROCTABLE()->TestFenceNV)
#define glGetFenceivNV                   (_GET_TLS_PROCTABLE()->GetFenceivNV)
#define glFinishFenceNV                  (_GET_TLS_PROCTABLE()->FinishFenceNV)
#define glSetFenceNV                     (_GET_TLS_PROCTABLE()->SetFenceNV)
#define glMapControlPointsNV             (_GET_TLS_PROCTABLE()->MapControlPointsNV)
#define glMapParameterivNV               (_GET_TLS_PROCTABLE()->MapParameterivNV)
#define glMapParameterfvNV               (_GET_TLS_PROCTABLE()->MapParameterfvNV)
#define glGetMapControlPointsNV          (_GET_TLS_PROCTABLE()->GetMapControlPointsNV)
#define glGetMapParameterivNV            (_GET_TLS_PROCTABLE()->GetMapParameterivNV)
#define glGetMapParameterfvNV            (_GET_TLS_PROCTABLE()->GetMapParameterfvNV)
#define glGetMapAttribParameterivNV      (_GET_TLS_PROCTABLE()->GetMapAttribParameterivNV)
#define glGetMapAttribParameterfvNV      (_GET_TLS_PROCTABLE()->GetMapAttribParameterfvNV)
#define glEvalMapsNV                     (_GET_TLS_PROCTABLE()->EvalMapsNV)
#define glCombinerStageParameterfvNV     (_GET_TLS_PROCTABLE()->CombinerStageParameterfvNV)
#define glGetCombinerStageParameterfvNV  (_GET_TLS_PROCTABLE()->GetCombinerStageParameterfvNV)
#define glAreProgramsResidentNV          (_GET_TLS_PROCTABLE()->AreProgramsResidentNV)
#define glBindProgramNV                  (_GET_TLS_PROCTABLE()->BindProgramNV)
#define glDeleteProgramsNV               (_GET_TLS_PROCTABLE()->DeleteProgramsNV)
#define glExecuteProgramNV               (_GET_TLS_PROCTABLE()->ExecuteProgramNV)
#define glGenProgramsNV                  (_GET_TLS_PROCTABLE()->GenProgramsNV)
#define glGetProgramParameterdvNV        (_GET_TLS_PROCTABLE()->GetProgramParameterdvNV)
#define glGetProgramParameterfvNV        (_GET_TLS_PROCTABLE()->GetProgramParameterfvNV)
#define glGetProgramivNV                 (_GET_TLS_PROCTABLE()->GetProgramivNV)
#define glGetProgramStringNV             (_GET_TLS_PROCTABLE()->GetProgramStringNV)
#define glGetTrackMatrixivNV             (_GET_TLS_PROCTABLE()->GetTrackMatrixivNV)
#define glGetVertexAttribdvNV            (_GET_TLS_PROCTABLE()->GetVertexAttribdvNV)
#define glGetVertexAttribfvNV            (_GET_TLS_PROCTABLE()->GetVertexAttribfvNV)
#define glGetVertexAttribivNV            (_GET_TLS_PROCTABLE()->GetVertexAttribivNV)
#define glGetVertexAttribPointervNV      (_GET_TLS_PROCTABLE()->GetVertexAttribPointervNV)
#define glIsProgramNV                    (_GET_TLS_PROCTABLE()->IsProgramNV)
#define glLoadProgramNV                  (_GET_TLS_PROCTABLE()->LoadProgramNV)
#define glProgramParameter4dNV           (_GET_TLS_PROCTABLE()->ProgramParameter4dNV)
#define glProgramParameter4dvNV          (_GET_TLS_PROCTABLE()->ProgramParameter4dvNV)
#define glProgramParameter4fNV           (_GET_TLS_PROCTABLE()->ProgramParameter4fNV)
#define glProgramParameter4fvNV          (_GET_TLS_PROCTABLE()->ProgramParameter4fvNV)
#define glProgramParameters4dvNV         (_GET_TLS_PROCTABLE()->ProgramParameters4dvNV)
#define glProgramParameters4fvNV         (_GET_TLS_PROCTABLE()->ProgramParameters4fvNV)
#define glRequestResidentProgramsNV      (_GET_TLS_PROCTABLE()->RequestResidentProgramsNV)
#define glTrackMatrixNV                  (_GET_TLS_PROCTABLE()->TrackMatrixNV)
#define glVertexAttribPointerNV          (_GET_TLS_PROCTABLE()->VertexAttribPointerNV)
#define glVertexAttrib1dNV               (_GET_TLS_PROCTABLE()->VertexAttrib1dNV)
#define glVertexAttrib1dvNV              (_GET_TLS_PROCTABLE()->VertexAttrib1dvNV)
#define glVertexAttrib1fNV               (_GET_TLS_PROCTABLE()->VertexAttrib1fNV)
#define glVertexAttrib1fvNV              (_GET_TLS_PROCTABLE()->VertexAttrib1fvNV)
#define glVertexAttrib1sNV               (_GET_TLS_PROCTABLE()->VertexAttrib1sNV)
#define glVertexAttrib1svNV              (_GET_TLS_PROCTABLE()->VertexAttrib1svNV)
#define glVertexAttrib2dNV               (_GET_TLS_PROCTABLE()->VertexAttrib2dNV)
#define glVertexAttrib2dvNV              (_GET_TLS_PROCTABLE()->VertexAttrib2dvNV)
#define glVertexAttrib2fNV               (_GET_TLS_PROCTABLE()->VertexAttrib2fNV)
#define glVertexAttrib2fvNV              (_GET_TLS_PROCTABLE()->VertexAttrib2fvNV)
#define glVertexAttrib2sNV               (_GET_TLS_PROCTABLE()->VertexAttrib2sNV)
#define glVertexAttrib2svNV              (_GET_TLS_PROCTABLE()->VertexAttrib2svNV)
#define glVertexAttrib3dNV               (_GET_TLS_PROCTABLE()->VertexAttrib3dNV)
#define glVertexAttrib3dvNV              (_GET_TLS_PROCTABLE()->VertexAttrib3dvNV)
#define glVertexAttrib3fNV               (_GET_TLS_PROCTABLE()->VertexAttrib3fNV)
#define glVertexAttrib3fvNV              (_GET_TLS_PROCTABLE()->VertexAttrib3fvNV)
#define glVertexAttrib3sNV               (_GET_TLS_PROCTABLE()->VertexAttrib3sNV)
#define glVertexAttrib3svNV              (_GET_TLS_PROCTABLE()->VertexAttrib3svNV)
#define glVertexAttrib4dNV               (_GET_TLS_PROCTABLE()->VertexAttrib4dNV)
#define glVertexAttrib4dvNV              (_GET_TLS_PROCTABLE()->VertexAttrib4dvNV)
#define glVertexAttrib4fNV               (_GET_TLS_PROCTABLE()->VertexAttrib4fNV)
#define glVertexAttrib4fvNV              (_GET_TLS_PROCTABLE()->VertexAttrib4fvNV)
#define glVertexAttrib4sNV               (_GET_TLS_PROCTABLE()->VertexAttrib4sNV)
#define glVertexAttrib4svNV              (_GET_TLS_PROCTABLE()->VertexAttrib4svNV)
#define glVertexAttrib4ubNV              (_GET_TLS_PROCTABLE()->VertexAttrib4ubNV)
#define glVertexAttrib4ubvNV             (_GET_TLS_PROCTABLE()->VertexAttrib4ubvNV)
#define glVertexAttribs1dvNV             (_GET_TLS_PROCTABLE()->VertexAttribs1dvNV)
#define glVertexAttribs1fvNV             (_GET_TLS_PROCTABLE()->VertexAttribs1fvNV)
#define glVertexAttribs1svNV             (_GET_TLS_PROCTABLE()->VertexAttribs1svNV)
#define glVertexAttribs2dvNV             (_GET_TLS_PROCTABLE()->VertexAttribs2dvNV)
#define glVertexAttribs2fvNV             (_GET_TLS_PROCTABLE()->VertexAttribs2fvNV)
#define glVertexAttribs2svNV             (_GET_TLS_PROCTABLE()->VertexAttribs2svNV)
#define glVertexAttribs3dvNV             (_GET_TLS_PROCTABLE()->VertexAttribs3dvNV)
#define glVertexAttribs3fvNV             (_GET_TLS_PROCTABLE()->VertexAttribs3fvNV)
#define glVertexAttribs3svNV             (_GET_TLS_PROCTABLE()->VertexAttribs3svNV)
#define glVertexAttribs4dvNV             (_GET_TLS_PROCTABLE()->VertexAttribs4dvNV)
#define glVertexAttribs4fvNV             (_GET_TLS_PROCTABLE()->VertexAttribs4fvNV)
#define glVertexAttribs4svNV             (_GET_TLS_PROCTABLE()->VertexAttribs4svNV)
#define glVertexAttribs4ubvNV            (_GET_TLS_PROCTABLE()->VertexAttribs4ubvNV)
#define glTexBumpParameterivATI          (_GET_TLS_PROCTABLE()->TexBumpParameterivATI)
#define glTexBumpParameterfvATI          (_GET_TLS_PROCTABLE()->TexBumpParameterfvATI)
#define glGetTexBumpParameterivATI       (_GET_TLS_PROCTABLE()->GetTexBumpParameterivATI)
#define glGetTexBumpParameterfvATI       (_GET_TLS_PROCTABLE()->GetTexBumpParameterfvATI)
#define glGenFragmentShadersATI          (_GET_TLS_PROCTABLE()->GenFragmentShadersATI)
#define glBindFragmentShaderATI          (_GET_TLS_PROCTABLE()->BindFragmentShaderATI)
#define glDeleteFragmentShaderATI        (_GET_TLS_PROCTABLE()->DeleteFragmentShaderATI)
#define glBeginFragmentShaderATI         (_GET_TLS_PROCTABLE()->BeginFragmentShaderATI)
#define glEndFragmentShaderATI           (_GET_TLS_PROCTABLE()->EndFragmentShaderATI)
#define glPassTexCoordATI                (_GET_TLS_PROCTABLE()->PassTexCoordATI)
#define glSampleMapATI                   (_GET_TLS_PROCTABLE()->SampleMapATI)
#define glColorFragmentOp1ATI            (_GET_TLS_PROCTABLE()->ColorFragmentOp1ATI)
#define glColorFragmentOp2ATI            (_GET_TLS_PROCTABLE()->ColorFragmentOp2ATI)
#define glColorFragmentOp3ATI            (_GET_TLS_PROCTABLE()->ColorFragmentOp3ATI)
#define glAlphaFragmentOp1ATI            (_GET_TLS_PROCTABLE()->AlphaFragmentOp1ATI)
#define glAlphaFragmentOp2ATI            (_GET_TLS_PROCTABLE()->AlphaFragmentOp2ATI)
#define glAlphaFragmentOp3ATI            (_GET_TLS_PROCTABLE()->AlphaFragmentOp3ATI)
#define glSetFragmentShaderConstantATI   (_GET_TLS_PROCTABLE()->SetFragmentShaderConstantATI)
#define glPNTrianglesiATI                (_GET_TLS_PROCTABLE()->PNTrianglesiATI)
#define glPNTrianglesfATI                (_GET_TLS_PROCTABLE()->PNTrianglesfATI)
#define glNewObjectBufferATI             (_GET_TLS_PROCTABLE()->NewObjectBufferATI)
#define glIsObjectBufferATI              (_GET_TLS_PROCTABLE()->IsObjectBufferATI)
#define glUpdateObjectBufferATI          (_GET_TLS_PROCTABLE()->UpdateObjectBufferATI)
#define glGetObjectBufferfvATI           (_GET_TLS_PROCTABLE()->GetObjectBufferfvATI)
#define glGetObjectBufferivATI           (_GET_TLS_PROCTABLE()->GetObjectBufferivATI)
#define glFreeObjectBufferATI            (_GET_TLS_PROCTABLE()->FreeObjectBufferATI)
#define glArrayObjectATI                 (_GET_TLS_PROCTABLE()->ArrayObjectATI)
#define glGetArrayObjectfvATI            (_GET_TLS_PROCTABLE()->GetArrayObjectfvATI)
#define glGetArrayObjectivATI            (_GET_TLS_PROCTABLE()->GetArrayObjectivATI)
#define glVariantArrayObjectATI          (_GET_TLS_PROCTABLE()->VariantArrayObjectATI)
#define glGetVariantArrayObjectfvATI     (_GET_TLS_PROCTABLE()->GetVariantArrayObjectfvATI)
#define glGetVariantArrayObjectivATI     (_GET_TLS_PROCTABLE()->GetVariantArrayObjectivATI)
#define glBeginVertexShaderEXT           (_GET_TLS_PROCTABLE()->BeginVertexShaderEXT)
#define glEndVertexShaderEXT             (_GET_TLS_PROCTABLE()->EndVertexShaderEXT)
#define glBindVertexShaderEXT            (_GET_TLS_PROCTABLE()->BindVertexShaderEXT)
#define glGenVertexShadersEXT            (_GET_TLS_PROCTABLE()->GenVertexShadersEXT)
#define glDeleteVertexShaderEXT          (_GET_TLS_PROCTABLE()->DeleteVertexShaderEXT)
#define glShaderOp1EXT                   (_GET_TLS_PROCTABLE()->ShaderOp1EXT)
#define glShaderOp2EXT                   (_GET_TLS_PROCTABLE()->ShaderOp2EXT)
#define glShaderOp3EXT                   (_GET_TLS_PROCTABLE()->ShaderOp3EXT)
#define glSwizzleEXT                     (_GET_TLS_PROCTABLE()->SwizzleEXT)
#define glWriteMaskEXT                   (_GET_TLS_PROCTABLE()->WriteMaskEXT)
#define glInsertComponentEXT             (_GET_TLS_PROCTABLE()->InsertComponentEXT)
#define glExtractComponentEXT            (_GET_TLS_PROCTABLE()->ExtractComponentEXT)
#define glGenSymbolsEXT                  (_GET_TLS_PROCTABLE()->GenSymbolsEXT)
#define glSetInvariantEXT                (_GET_TLS_PROCTABLE()->SetInvariantEXT)
#define glSetLocalConstantEXT            (_GET_TLS_PROCTABLE()->SetLocalConstantEXT)
#define glVariantbvEXT                   (_GET_TLS_PROCTABLE()->VariantbvEXT)
#define glVariantsvEXT                   (_GET_TLS_PROCTABLE()->VariantsvEXT)
#define glVariantivEXT                   (_GET_TLS_PROCTABLE()->VariantivEXT)
#define glVariantfvEXT                   (_GET_TLS_PROCTABLE()->VariantfvEXT)
#define glVariantdvEXT                   (_GET_TLS_PROCTABLE()->VariantdvEXT)
#define glVariantubvEXT                  (_GET_TLS_PROCTABLE()->VariantubvEXT)
#define glVariantusvEXT                  (_GET_TLS_PROCTABLE()->VariantusvEXT)
#define glVariantuivEXT                  (_GET_TLS_PROCTABLE()->VariantuivEXT)
#define glVariantPointerEXT              (_GET_TLS_PROCTABLE()->VariantPointerEXT)
#define glEnableVariantClientStateEXT    (_GET_TLS_PROCTABLE()->EnableVariantClientStateEXT)
#define glDisableVariantClientStateEXT   (_GET_TLS_PROCTABLE()->DisableVariantClientStateEXT)
#define glBindLightParameterEXT          (_GET_TLS_PROCTABLE()->BindLightParameterEXT)
#define glBindMaterialParameterEXT       (_GET_TLS_PROCTABLE()->BindMaterialParameterEXT)
#define glBindTexGenParameterEXT         (_GET_TLS_PROCTABLE()->BindTexGenParameterEXT)
#define glBindTextureUnitParameterEXT    (_GET_TLS_PROCTABLE()->BindTextureUnitParameterEXT)
#define glBindParameterEXT               (_GET_TLS_PROCTABLE()->BindParameterEXT)
#define glIsVariantEnabledEXT            (_GET_TLS_PROCTABLE()->IsVariantEnabledEXT)
#define glGetVariantBooleanvEXT          (_GET_TLS_PROCTABLE()->GetVariantBooleanvEXT)
#define glGetVariantIntegervEXT          (_GET_TLS_PROCTABLE()->GetVariantIntegervEXT)
#define glGetVariantFloatvEXT            (_GET_TLS_PROCTABLE()->GetVariantFloatvEXT)
#define glGetVariantPointervEXT          (_GET_TLS_PROCTABLE()->GetVariantPointervEXT)
#define glGetInvariantBooleanvEXT        (_GET_TLS_PROCTABLE()->GetInvariantBooleanvEXT)
#define glGetInvariantIntegervEXT        (_GET_TLS_PROCTABLE()->GetInvariantIntegervEXT)
#define glGetInvariantFloatvEXT          (_GET_TLS_PROCTABLE()->GetInvariantFloatvEXT)
#define glGetLocalConstantBooleanvEXT    (_GET_TLS_PROCTABLE()->GetLocalConstantBooleanvEXT)
#define glGetLocalConstantIntegervEXT    (_GET_TLS_PROCTABLE()->GetLocalConstantIntegervEXT)
#define glGetLocalConstantFloatvEXT      (_GET_TLS_PROCTABLE()->GetLocalConstantFloatvEXT)
#define glVertexStream1sATI              (_GET_TLS_PROCTABLE()->VertexStream1sATI)
#define glVertexStream1svATI             (_GET_TLS_PROCTABLE()->VertexStream1svATI)
#define glVertexStream1iATI              (_GET_TLS_PROCTABLE()->VertexStream1iATI)
#define glVertexStream1ivATI             (_GET_TLS_PROCTABLE()->VertexStream1ivATI)
#define glVertexStream1fATI              (_GET_TLS_PROCTABLE()->VertexStream1fATI)
#define glVertexStream1fvATI             (_GET_TLS_PROCTABLE()->VertexStream1fvATI)
#define glVertexStream1dATI              (_GET_TLS_PROCTABLE()->VertexStream1dATI)
#define glVertexStream1dvATI             (_GET_TLS_PROCTABLE()->VertexStream1dvATI)
#define glVertexStream2sATI              (_GET_TLS_PROCTABLE()->VertexStream2sATI)
#define glVertexStream2svATI             (_GET_TLS_PROCTABLE()->VertexStream2svATI)
#define glVertexStream2iATI              (_GET_TLS_PROCTABLE()->VertexStream2iATI)
#define glVertexStream2ivATI             (_GET_TLS_PROCTABLE()->VertexStream2ivATI)
#define glVertexStream2fATI              (_GET_TLS_PROCTABLE()->VertexStream2fATI)
#define glVertexStream2fvATI             (_GET_TLS_PROCTABLE()->VertexStream2fvATI)
#define glVertexStream2dATI              (_GET_TLS_PROCTABLE()->VertexStream2dATI)
#define glVertexStream2dvATI             (_GET_TLS_PROCTABLE()->VertexStream2dvATI)
#define glVertexStream3sATI              (_GET_TLS_PROCTABLE()->VertexStream3sATI)
#define glVertexStream3svATI             (_GET_TLS_PROCTABLE()->VertexStream3svATI)
#define glVertexStream3iATI              (_GET_TLS_PROCTABLE()->VertexStream3iATI)
#define glVertexStream3ivATI             (_GET_TLS_PROCTABLE()->VertexStream3ivATI)
#define glVertexStream3fATI              (_GET_TLS_PROCTABLE()->VertexStream3fATI)
#define glVertexStream3fvATI             (_GET_TLS_PROCTABLE()->VertexStream3fvATI)
#define glVertexStream3dATI              (_GET_TLS_PROCTABLE()->VertexStream3dATI)
#define glVertexStream3dvATI             (_GET_TLS_PROCTABLE()->VertexStream3dvATI)
#define glVertexStream4sATI              (_GET_TLS_PROCTABLE()->VertexStream4sATI)
#define glVertexStream4svATI             (_GET_TLS_PROCTABLE()->VertexStream4svATI)
#define glVertexStream4iATI              (_GET_TLS_PROCTABLE()->VertexStream4iATI)
#define glVertexStream4ivATI             (_GET_TLS_PROCTABLE()->VertexStream4ivATI)
#define glVertexStream4fATI              (_GET_TLS_PROCTABLE()->VertexStream4fATI)
#define glVertexStream4fvATI             (_GET_TLS_PROCTABLE()->VertexStream4fvATI)
#define glVertexStream4dATI              (_GET_TLS_PROCTABLE()->VertexStream4dATI)
#define glVertexStream4dvATI             (_GET_TLS_PROCTABLE()->VertexStream4dvATI)
#define glNormalStream3bATI              (_GET_TLS_PROCTABLE()->NormalStream3bATI)
#define glNormalStream3bvATI             (_GET_TLS_PROCTABLE()->NormalStream3bvATI)
#define glNormalStream3sATI              (_GET_TLS_PROCTABLE()->NormalStream3sATI)
#define glNormalStream3svATI             (_GET_TLS_PROCTABLE()->NormalStream3svATI)
#define glNormalStream3iATI              (_GET_TLS_PROCTABLE()->NormalStream3iATI)
#define glNormalStream3ivATI             (_GET_TLS_PROCTABLE()->NormalStream3ivATI)
#define glNormalStream3fATI              (_GET_TLS_PROCTABLE()->NormalStream3fATI)
#define glNormalStream3fvATI             (_GET_TLS_PROCTABLE()->NormalStream3fvATI)
#define glNormalStream3dATI              (_GET_TLS_PROCTABLE()->NormalStream3dATI)
#define glNormalStream3dvATI             (_GET_TLS_PROCTABLE()->NormalStream3dvATI)
#define glClientActiveVertexStreamATI    (_GET_TLS_PROCTABLE()->ClientActiveVertexStreamATI)
#define glVertexBlendEnviATI             (_GET_TLS_PROCTABLE()->VertexBlendEnviATI)
#define glVertexBlendEnvfATI             (_GET_TLS_PROCTABLE()->VertexBlendEnvfATI)
#define glElementPointerATI              (_GET_TLS_PROCTABLE()->ElementPointerATI)
#define glDrawElementArrayATI            (_GET_TLS_PROCTABLE()->DrawElementArrayATI)
#define glDrawRangeElementArrayATI       (_GET_TLS_PROCTABLE()->DrawRangeElementArrayATI)
#define glDrawMeshArraysSUN              (_GET_TLS_PROCTABLE()->DrawMeshArraysSUN)
#define glGenOcclusionQueriesNV          (_GET_TLS_PROCTABLE()->GenOcclusionQueriesNV)
#define glDeleteOcclusionQueriesNV       (_GET_TLS_PROCTABLE()->DeleteOcclusionQueriesNV)
#define glIsOcclusionQueryNV             (_GET_TLS_PROCTABLE()->IsOcclusionQueryNV)
#define glBeginOcclusionQueryNV          (_GET_TLS_PROCTABLE()->BeginOcclusionQueryNV)
#define glEndOcclusionQueryNV            (_GET_TLS_PROCTABLE()->EndOcclusionQueryNV)
#define glGetOcclusionQueryivNV          (_GET_TLS_PROCTABLE()->GetOcclusionQueryivNV)
#define glGetOcclusionQueryuivNV         (_GET_TLS_PROCTABLE()->GetOcclusionQueryuivNV)
#define glPointParameteriNV              (_GET_TLS_PROCTABLE()->PointParameteriNV)
#define glPointParameterivNV             (_GET_TLS_PROCTABLE()->PointParameterivNV)
#define glActiveStencilFaceEXT           (_GET_TLS_PROCTABLE()->ActiveStencilFaceEXT)
#define glElementPointerAPPLE            (_GET_TLS_PROCTABLE()->ElementPointerAPPLE)
#define glDrawElementArrayAPPLE          (_GET_TLS_PROCTABLE()->DrawElementArrayAPPLE)
#define glDrawRangeElementArrayAPPLE     (_GET_TLS_PROCTABLE()->DrawRangeElementArrayAPPLE)
#define glMultiDrawElementArrayAPPLE     (_GET_TLS_PROCTABLE()->MultiDrawElementArrayAPPLE)
#define glMultiDrawRangeElementArrayAPPLE (_GET_TLS_PROCTABLE()->MultiDrawRangeElementArrayAPPLE)
#define glGenFencesAPPLE                 (_GET_TLS_PROCTABLE()->GenFencesAPPLE)
#define glDeleteFencesAPPLE              (_GET_TLS_PROCTABLE()->DeleteFencesAPPLE)
#define glSetFenceAPPLE                  (_GET_TLS_PROCTABLE()->SetFenceAPPLE)
#define glIsFenceAPPLE                   (_GET_TLS_PROCTABLE()->IsFenceAPPLE)
#define glTestFenceAPPLE                 (_GET_TLS_PROCTABLE()->TestFenceAPPLE)
#define glFinishFenceAPPLE               (_GET_TLS_PROCTABLE()->FinishFenceAPPLE)
#define glTestObjectAPPLE                (_GET_TLS_PROCTABLE()->TestObjectAPPLE)
#define glFinishObjectAPPLE              (_GET_TLS_PROCTABLE()->FinishObjectAPPLE)
#define glBindVertexArrayAPPLE           (_GET_TLS_PROCTABLE()->BindVertexArrayAPPLE)
#define glDeleteVertexArraysAPPLE        (_GET_TLS_PROCTABLE()->DeleteVertexArraysAPPLE)
#define glGenVertexArraysAPPLE           (_GET_TLS_PROCTABLE()->GenVertexArraysAPPLE)
#define glIsVertexArrayAPPLE             (_GET_TLS_PROCTABLE()->IsVertexArrayAPPLE)
#define glVertexArrayRangeAPPLE          (_GET_TLS_PROCTABLE()->VertexArrayRangeAPPLE)
#define glFlushVertexArrayRangeAPPLE     (_GET_TLS_PROCTABLE()->FlushVertexArrayRangeAPPLE)
#define glVertexArrayParameteriAPPLE     (_GET_TLS_PROCTABLE()->VertexArrayParameteriAPPLE)
#define glDrawBuffersATI                 (_GET_TLS_PROCTABLE()->DrawBuffersATI)
#define glProgramNamedParameter4fNV      (_GET_TLS_PROCTABLE()->ProgramNamedParameter4fNV)
#define glProgramNamedParameter4dNV      (_GET_TLS_PROCTABLE()->ProgramNamedParameter4dNV)
#define glProgramNamedParameter4fvNV     (_GET_TLS_PROCTABLE()->ProgramNamedParameter4fvNV)
#define glProgramNamedParameter4dvNV     (_GET_TLS_PROCTABLE()->ProgramNamedParameter4dvNV)
#define glGetProgramNamedParameterfvNV   (_GET_TLS_PROCTABLE()->GetProgramNamedParameterfvNV)
#define glGetProgramNamedParameterdvNV   (_GET_TLS_PROCTABLE()->GetProgramNamedParameterdvNV)
#define glVertex2hNV                     (_GET_TLS_PROCTABLE()->Vertex2hNV)
#define glVertex2hvNV                    (_GET_TLS_PROCTABLE()->Vertex2hvNV)
#define glVertex3hNV                     (_GET_TLS_PROCTABLE()->Vertex3hNV)
#define glVertex3hvNV                    (_GET_TLS_PROCTABLE()->Vertex3hvNV)
#define glVertex4hNV                     (_GET_TLS_PROCTABLE()->Vertex4hNV)
#define glVertex4hvNV                    (_GET_TLS_PROCTABLE()->Vertex4hvNV)
#define glNormal3hNV                     (_GET_TLS_PROCTABLE()->Normal3hNV)
#define glNormal3hvNV                    (_GET_TLS_PROCTABLE()->Normal3hvNV)
#define glColor3hNV                      (_GET_TLS_PROCTABLE()->Color3hNV)
#define glColor3hvNV                     (_GET_TLS_PROCTABLE()->Color3hvNV)
#define glColor4hNV                      (_GET_TLS_PROCTABLE()->Color4hNV)
#define glColor4hvNV                     (_GET_TLS_PROCTABLE()->Color4hvNV)
#define glTexCoord1hNV                   (_GET_TLS_PROCTABLE()->TexCoord1hNV)
#define glTexCoord1hvNV                  (_GET_TLS_PROCTABLE()->TexCoord1hvNV)
#define glTexCoord2hNV                   (_GET_TLS_PROCTABLE()->TexCoord2hNV)
#define glTexCoord2hvNV                  (_GET_TLS_PROCTABLE()->TexCoord2hvNV)
#define glTexCoord3hNV                   (_GET_TLS_PROCTABLE()->TexCoord3hNV)
#define glTexCoord3hvNV                  (_GET_TLS_PROCTABLE()->TexCoord3hvNV)
#define glTexCoord4hNV                   (_GET_TLS_PROCTABLE()->TexCoord4hNV)
#define glTexCoord4hvNV                  (_GET_TLS_PROCTABLE()->TexCoord4hvNV)
#define glMultiTexCoord1hNV              (_GET_TLS_PROCTABLE()->MultiTexCoord1hNV)
#define glMultiTexCoord1hvNV             (_GET_TLS_PROCTABLE()->MultiTexCoord1hvNV)
#define glMultiTexCoord2hNV              (_GET_TLS_PROCTABLE()->MultiTexCoord2hNV)
#define glMultiTexCoord2hvNV             (_GET_TLS_PROCTABLE()->MultiTexCoord2hvNV)
#define glMultiTexCoord3hNV              (_GET_TLS_PROCTABLE()->MultiTexCoord3hNV)
#define glMultiTexCoord3hvNV             (_GET_TLS_PROCTABLE()->MultiTexCoord3hvNV)
#define glMultiTexCoord4hNV              (_GET_TLS_PROCTABLE()->MultiTexCoord4hNV)
#define glMultiTexCoord4hvNV             (_GET_TLS_PROCTABLE()->MultiTexCoord4hvNV)
#define glFogCoordhNV                    (_GET_TLS_PROCTABLE()->FogCoordhNV)
#define glFogCoordhvNV                   (_GET_TLS_PROCTABLE()->FogCoordhvNV)
#define glSecondaryColor3hNV             (_GET_TLS_PROCTABLE()->SecondaryColor3hNV)
#define glSecondaryColor3hvNV            (_GET_TLS_PROCTABLE()->SecondaryColor3hvNV)
#define glVertexWeighthNV                (_GET_TLS_PROCTABLE()->VertexWeighthNV)
#define glVertexWeighthvNV               (_GET_TLS_PROCTABLE()->VertexWeighthvNV)
#define glVertexAttrib1hNV               (_GET_TLS_PROCTABLE()->VertexAttrib1hNV)
#define glVertexAttrib1hvNV              (_GET_TLS_PROCTABLE()->VertexAttrib1hvNV)
#define glVertexAttrib2hNV               (_GET_TLS_PROCTABLE()->VertexAttrib2hNV)
#define glVertexAttrib2hvNV              (_GET_TLS_PROCTABLE()->VertexAttrib2hvNV)
#define glVertexAttrib3hNV               (_GET_TLS_PROCTABLE()->VertexAttrib3hNV)
#define glVertexAttrib3hvNV              (_GET_TLS_PROCTABLE()->VertexAttrib3hvNV)
#define glVertexAttrib4hNV               (_GET_TLS_PROCTABLE()->VertexAttrib4hNV)
#define glVertexAttrib4hvNV              (_GET_TLS_PROCTABLE()->VertexAttrib4hvNV)
#define glVertexAttribs1hvNV             (_GET_TLS_PROCTABLE()->VertexAttribs1hvNV)
#define glVertexAttribs2hvNV             (_GET_TLS_PROCTABLE()->VertexAttribs2hvNV)
#define glVertexAttribs3hvNV             (_GET_TLS_PROCTABLE()->VertexAttribs3hvNV)
#define glVertexAttribs4hvNV             (_GET_TLS_PROCTABLE()->VertexAttribs4hvNV)
#define glPixelDataRangeNV               (_GET_TLS_PROCTABLE()->PixelDataRangeNV)
#define glFlushPixelDataRangeNV          (_GET_TLS_PROCTABLE()->FlushPixelDataRangeNV)
#define glPrimitiveRestartNV             (_GET_TLS_PROCTABLE()->PrimitiveRestartNV)
#define glPrimitiveRestartIndexNV        (_GET_TLS_PROCTABLE()->PrimitiveRestartIndexNV)
#define glMapObjectBufferATI             (_GET_TLS_PROCTABLE()->MapObjectBufferATI)
#define glUnmapObjectBufferATI           (_GET_TLS_PROCTABLE()->UnmapObjectBufferATI)
#define glStencilOpSeparateATI           (_GET_TLS_PROCTABLE()->StencilOpSeparateATI)
#define glStencilFuncSeparateATI         (_GET_TLS_PROCTABLE()->StencilFuncSeparateATI)
#define glVertexAttribArrayObjectATI     (_GET_TLS_PROCTABLE()->VertexAttribArrayObjectATI)
#define glGetVertexAttribArrayObjectfvATI (_GET_TLS_PROCTABLE()->GetVertexAttribArrayObjectfvATI)
#define glGetVertexAttribArrayObjectivATI (_GET_TLS_PROCTABLE()->GetVertexAttribArrayObjectivATI)
#define glDepthBoundsEXT                 (_GET_TLS_PROCTABLE()->DepthBoundsEXT)
#define glBlendEquationSeparateEXT       (_GET_TLS_PROCTABLE()->BlendEquationSeparateEXT)
#define glAddSwapHintRectWIN             (_GET_TLS_PROCTABLE()->AddSwapHintRectWIN)
#ifdef _WIN32
#define wglCreateBufferRegionARB          (_GET_TLS_PROCTABLE()->CreateBufferRegionARB)
#define wglDeleteBufferRegionARB          (_GET_TLS_PROCTABLE()->DeleteBufferRegionARB)
#define wglSaveBufferRegionARB            (_GET_TLS_PROCTABLE()->SaveBufferRegionARB)
#define wglRestoreBufferRegionARB         (_GET_TLS_PROCTABLE()->RestoreBufferRegionARB)
#define wglGetExtensionsStringARB         (_GET_TLS_PROCTABLE()->GetExtensionsStringARB)
#define wglGetPixelFormatAttribivARB      (_GET_TLS_PROCTABLE()->GetPixelFormatAttribivARB)
#define wglGetPixelFormatAttribfvARB      (_GET_TLS_PROCTABLE()->GetPixelFormatAttribfvARB)
#define wglChoosePixelFormatARB           (_GET_TLS_PROCTABLE()->ChoosePixelFormatARB)
#define wglMakeContextCurrentARB          (_GET_TLS_PROCTABLE()->MakeContextCurrentARB)
#define wglGetCurrentReadDCARB            (_GET_TLS_PROCTABLE()->GetCurrentReadDCARB)
#define wglCreatePbufferARB               (_GET_TLS_PROCTABLE()->CreatePbufferARB)
#define wglGetPbufferDCARB                (_GET_TLS_PROCTABLE()->GetPbufferDCARB)
#define wglReleasePbufferDCARB            (_GET_TLS_PROCTABLE()->ReleasePbufferDCARB)
#define wglDestroyPbufferARB              (_GET_TLS_PROCTABLE()->DestroyPbufferARB)
#define wglQueryPbufferARB                (_GET_TLS_PROCTABLE()->QueryPbufferARB)
#define wglBindTexImageARB                (_GET_TLS_PROCTABLE()->BindTexImageARB)
#define wglReleaseTexImageARB             (_GET_TLS_PROCTABLE()->ReleaseTexImageARB)
#define wglSetPbufferAttribARB            (_GET_TLS_PROCTABLE()->SetPbufferAttribARB)
#define wglCreateDisplayColorTableEXT     (_GET_TLS_PROCTABLE()->CreateDisplayColorTableEXT)
#define wglLoadDisplayColorTableEXT       (_GET_TLS_PROCTABLE()->LoadDisplayColorTableEXT)
#define wglBindDisplayColorTableEXT       (_GET_TLS_PROCTABLE()->BindDisplayColorTableEXT)
#define wglDestroyDisplayColorTableEXT    (_GET_TLS_PROCTABLE()->DestroyDisplayColorTableEXT)
#define wglGetExtensionsStringEXT         (_GET_TLS_PROCTABLE()->GetExtensionsStringEXT)
#define wglMakeContextCurrentEXT          (_GET_TLS_PROCTABLE()->MakeContextCurrentEXT)
#define wglGetCurrentReadDCEXT            (_GET_TLS_PROCTABLE()->GetCurrentReadDCEXT)
#define wglCreatePbufferEXT               (_GET_TLS_PROCTABLE()->CreatePbufferEXT)
#define wglGetPbufferDCEXT                (_GET_TLS_PROCTABLE()->GetPbufferDCEXT)
#define wglReleasePbufferDCEXT            (_GET_TLS_PROCTABLE()->ReleasePbufferDCEXT)
#define wglDestroyPbufferEXT              (_GET_TLS_PROCTABLE()->DestroyPbufferEXT)
#define wglQueryPbufferEXT                (_GET_TLS_PROCTABLE()->QueryPbufferEXT)
#define wglGetPixelFormatAttribivEXT      (_GET_TLS_PROCTABLE()->GetPixelFormatAttribivEXT)
#define wglGetPixelFormatAttribfvEXT      (_GET_TLS_PROCTABLE()->GetPixelFormatAttribfvEXT)
#define wglChoosePixelFormatEXT           (_GET_TLS_PROCTABLE()->ChoosePixelFormatEXT)
#define wglSwapIntervalEXT                (_GET_TLS_PROCTABLE()->SwapIntervalEXT)
#define wglGetSwapIntervalEXT             (_GET_TLS_PROCTABLE()->GetSwapIntervalEXT)
#define wglAllocateMemoryNV               (_GET_TLS_PROCTABLE()->AllocateMemoryNV)
#define wglFreeMemoryNV                   (_GET_TLS_PROCTABLE()->FreeMemoryNV)
#define wglGetSyncValuesOML               (_GET_TLS_PROCTABLE()->GetSyncValuesOML)
#define wglGetMscRateOML                  (_GET_TLS_PROCTABLE()->GetMscRateOML)
#define wglSwapBuffersMscOML              (_GET_TLS_PROCTABLE()->SwapBuffersMscOML)
#define wglSwapLayerBuffersMscOML         (_GET_TLS_PROCTABLE()->SwapLayerBuffersMscOML)
#define wglWaitForMscOML                  (_GET_TLS_PROCTABLE()->WaitForMscOML)
#define wglWaitForSbcOML                  (_GET_TLS_PROCTABLE()->WaitForSbcOML)
#define wglGetDigitalVideoParametersI3D   (_GET_TLS_PROCTABLE()->GetDigitalVideoParametersI3D)
#define wglSetDigitalVideoParametersI3D   (_GET_TLS_PROCTABLE()->SetDigitalVideoParametersI3D)
#define wglGetGammaTableParametersI3D     (_GET_TLS_PROCTABLE()->GetGammaTableParametersI3D)
#define wglSetGammaTableParametersI3D     (_GET_TLS_PROCTABLE()->SetGammaTableParametersI3D)
#define wglGetGammaTableI3D               (_GET_TLS_PROCTABLE()->GetGammaTableI3D)
#define wglSetGammaTableI3D               (_GET_TLS_PROCTABLE()->SetGammaTableI3D)
#define wglEnableGenlockI3D               (_GET_TLS_PROCTABLE()->EnableGenlockI3D)
#define wglDisableGenlockI3D              (_GET_TLS_PROCTABLE()->DisableGenlockI3D)
#define wglIsEnabledGenlockI3D            (_GET_TLS_PROCTABLE()->IsEnabledGenlockI3D)
#define wglGenlockSourceI3D               (_GET_TLS_PROCTABLE()->GenlockSourceI3D)
#define wglGetGenlockSourceI3D            (_GET_TLS_PROCTABLE()->GetGenlockSourceI3D)
#define wglGenlockSourceEdgeI3D           (_GET_TLS_PROCTABLE()->GenlockSourceEdgeI3D)
#define wglGetGenlockSourceEdgeI3D        (_GET_TLS_PROCTABLE()->GetGenlockSourceEdgeI3D)
#define wglGenlockSampleRateI3D           (_GET_TLS_PROCTABLE()->GenlockSampleRateI3D)
#define wglGetGenlockSampleRateI3D        (_GET_TLS_PROCTABLE()->GetGenlockSampleRateI3D)
#define wglGenlockSourceDelayI3D          (_GET_TLS_PROCTABLE()->GenlockSourceDelayI3D)
#define wglGetGenlockSourceDelayI3D       (_GET_TLS_PROCTABLE()->GetGenlockSourceDelayI3D)
#define wglQueryGenlockMaxSourceDelayI3D  (_GET_TLS_PROCTABLE()->QueryGenlockMaxSourceDelayI3D)
#define wglCreateImageBufferI3D           (_GET_TLS_PROCTABLE()->CreateImageBufferI3D)
#define wglDestroyImageBufferI3D          (_GET_TLS_PROCTABLE()->DestroyImageBufferI3D)
#define wglAssociateImageBufferEventsI3D  (_GET_TLS_PROCTABLE()->AssociateImageBufferEventsI3D)
#define wglReleaseImageBufferEventsI3D    (_GET_TLS_PROCTABLE()->ReleaseImageBufferEventsI3D)
#define wglEnableFrameLockI3D             (_GET_TLS_PROCTABLE()->EnableFrameLockI3D)
#define wglDisableFrameLockI3D            (_GET_TLS_PROCTABLE()->DisableFrameLockI3D)
#define wglIsEnabledFrameLockI3D          (_GET_TLS_PROCTABLE()->IsEnabledFrameLockI3D)
#define wglQueryFrameLockMasterI3D        (_GET_TLS_PROCTABLE()->QueryFrameLockMasterI3D)
#define wglGetFrameUsageI3D               (_GET_TLS_PROCTABLE()->GetFrameUsageI3D)
#define wglBeginFrameTrackingI3D          (_GET_TLS_PROCTABLE()->BeginFrameTrackingI3D)
#define wglEndFrameTrackingI3D            (_GET_TLS_PROCTABLE()->EndFrameTrackingI3D)
#define wglQueryFrameTrackingI3D          (_GET_TLS_PROCTABLE()->QueryFrameTrackingI3D)
#endif /* _WIN32 */

#ifndef _APP_PROCTABLE

/*
 * Applications can replace the following function with its own function
 * for accessing thread local proc/context dependent proc table.
 * The following default function works for most applications which
 * are using the same device for all their contexts - even if
 * the contexts are on different threads.
 */

static _inline _GLextensionProcs *_GET_TLS_PROCTABLE(void)

{
	extern _GLextensionProcs _extensionProcs;

	return (&_extensionProcs);
}

#else

/*
 * Application should replace this compiled function with
 * an inlined function for maximum performance.
 */

extern _GLextensionProcs *_GET_TLS_PROCTABLE(void);

#endif

/*
 * Provide an initialization function for the application
 * to initialize its own proc tables in case the application
 * needs to use multiple proc tables.
 */

static _inline void _InitExtensionProcs(_GLextensionProcs *appProcs)
{
	extern _GLextensionProcs _extensionProcs;

	*appProcs = _extensionProcs;
}

#ifdef __cplusplus
}
#endif


#endif /* _GLPROCS_H_ */
