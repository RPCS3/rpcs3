/*  ZZ Open GL graphics plugin
 *  Copyright (c)2009-2010 zeydlitz@gmail.com, arcum42@gmail.com
 *  Based on Zerofrog's ZeroGS KOSMOS (c)2005-2008
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#ifndef ZZGL_H_INCLUDED
#define ZZGL_H_INCLUDED

#include "PS2Etypes.h"
#include "PS2Edefs.h"

// Need this before gl.h
#ifdef _WIN32

#include <windows.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include "glprocs.h"

#else

// adding glew support instead of glXGetProcAddress (thanks to scaught)
#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glext.h>
#include <GL/glx.h>

inline void* wglGetProcAddress(const char* x)
{
	return (void*)glXGetProcAddress((const GLubyte*)x);
}

#endif

extern u32 s_stencilfunc, s_stencilref, s_stencilmask;
// Defines

#ifndef GL_DEPTH24_STENCIL8_EXT // allows FBOs to support stencils
#	define GL_DEPTH_STENCIL_EXT 0x84F9
#	define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#	define GL_DEPTH24_STENCIL8_EXT 0x88F0
#	define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1
#endif

#define GL_STENCILFUNC(func, ref, mask) { \
	s_stencilfunc  = func; \
	s_stencilref = ref; \
	s_stencilmask = mask; \
	glStencilFunc(func, ref, mask); \
}

#define GL_STENCILFUNC_SET() glStencilFunc(s_stencilfunc, s_stencilref, s_stencilmask)


// sets the data stream
#define SET_STREAM() { \
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)8); \
	glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12); \
	glTexCoordPointer(3, GL_FLOAT, sizeof(VertexGPU), (void*)16); \
	glVertexPointer(4, GL_SHORT, sizeof(VertexGPU), (void*)0); \
}


// global alpha blending settings
extern GLenum g_internalRGBAFloat16Fmt;

extern const GLenum primtype[8];

#define SAFE_RELEASE_TEX(x) { if( (x) != 0 ) { glDeleteTextures(1, &(x)); x = 0; } }

// inline for an extremely often used sequence
// This is turning off all gl functions. Safe to do updates.
inline void DisableAllgl()
{
	glDisable(GL_SCISSOR_TEST);
	glDisable(GL_BLEND);
	glDisable(GL_ALPHA_TEST);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(0);
	glDisable(GL_STENCIL_TEST);
	glColorMask(1, 1, 1, 1);
}

//--------------------- Dummies

#ifdef _WIN32
extern void (__stdcall *zgsBlendEquationSeparateEXT)(GLenum, GLenum);
extern void (__stdcall *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum);
#else
extern void (APIENTRY *zgsBlendEquationSeparateEXT)(GLenum, GLenum);
extern void (APIENTRY *zgsBlendFuncSeparateEXT)(GLenum, GLenum, GLenum, GLenum);
#endif


// ------------------------ Types -------------------------

/////////////////////
// graphics resources
extern GLenum s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha; // set by zgsBlendFuncSeparateEXT

// GL prototypes
extern PFNGLISRENDERBUFFEREXTPROC glIsRenderbufferEXT;
extern PFNGLBINDRENDERBUFFEREXTPROC glBindRenderbufferEXT;
extern PFNGLDELETERENDERBUFFERSEXTPROC glDeleteRenderbuffersEXT;
extern PFNGLGENRENDERBUFFERSEXTPROC glGenRenderbuffersEXT;
extern PFNGLRENDERBUFFERSTORAGEEXTPROC glRenderbufferStorageEXT;
extern PFNGLGETRENDERBUFFERPARAMETERIVEXTPROC glGetRenderbufferParameterivEXT;
extern PFNGLISFRAMEBUFFEREXTPROC glIsFramebufferEXT;
extern PFNGLBINDFRAMEBUFFEREXTPROC glBindFramebufferEXT;
extern PFNGLDELETEFRAMEBUFFERSEXTPROC glDeleteFramebuffersEXT;
extern PFNGLGENFRAMEBUFFERSEXTPROC glGenFramebuffersEXT;
extern PFNGLCHECKFRAMEBUFFERSTATUSEXTPROC glCheckFramebufferStatusEXT;
extern PFNGLFRAMEBUFFERTEXTURE1DEXTPROC glFramebufferTexture1DEXT;
extern PFNGLFRAMEBUFFERTEXTURE2DEXTPROC glFramebufferTexture2DEXT;
extern PFNGLFRAMEBUFFERTEXTURE3DEXTPROC glFramebufferTexture3DEXT;
extern PFNGLFRAMEBUFFERRENDERBUFFEREXTPROC glFramebufferRenderbufferEXT;
extern PFNGLGETFRAMEBUFFERATTACHMENTPARAMETERIVEXTPROC glGetFramebufferAttachmentParameterivEXT;
extern PFNGLGENERATEMIPMAPEXTPROC glGenerateMipmapEXT;
extern PFNGLDRAWBUFFERSPROC glDrawBuffers;

#endif // ZZGL_H_INCLUDED
