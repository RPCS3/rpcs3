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

#include "Utilities/RedtapeWindows.h"

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

#include "Mem.h"

extern u32 s_stencilfunc, s_stencilref, s_stencilmask;
extern GLenum s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha; // set by zgsBlendFuncSeparateEXT
extern GLenum s_rgbeq, s_alphaeq;

#ifndef GL_DEPTH24_STENCIL8_EXT // allows FBOs to support stencils
#	define GL_DEPTH_STENCIL_EXT 0x84F9
#	define GL_UNSIGNED_INT_24_8_EXT 0x84FA
#	define GL_DEPTH24_STENCIL8_EXT 0x88F0
#	define GL_TEXTURE_STENCIL_SIZE_EXT 0x88F1
#endif

#ifdef _WIN32
#define GL_LOADFN(name) { \
		if( (*(void**)&name = (void*)wglGetProcAddress(#name)) == NULL ) { \
		ZZLog::Error_Log("Failed to find %s, exiting.", #name); \
	} \
}
#else
// let GLEW take care of it
#define GL_LOADFN(name)
#endif

static __forceinline void GL_STENCILFUNC(GLenum func, GLint ref, GLuint mask)
{
	s_stencilfunc  = func; 
	s_stencilref = ref; 
	s_stencilmask = mask; 
	glStencilFunc(func, ref, mask); 
}

static __forceinline void GL_STENCILFUNC_SET()
{
	glStencilFunc(s_stencilfunc, s_stencilref, s_stencilmask); 
}

#ifdef GLSL4_API
#include "ZZoglShaders.h"
#endif
// sets the data stream
static __forceinline void SET_STREAM()
{
#ifndef GLSL4_API
	glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)8);
	glSecondaryColorPointerEXT(4, GL_UNSIGNED_BYTE, sizeof(VertexGPU), (void*)12);
	glTexCoordPointer(3, GL_FLOAT, sizeof(VertexGPU), (void*)16);
	glVertexPointer(4, GL_SHORT, sizeof(VertexGPU), (void*)0);
#else
	vertex_array->set_internal_format();
#endif
}

// global alpha blending settings
extern GLenum g_internalRGBAFloat16Fmt;

//static __forceinline void SAFE_RELEASE_TEX(u32& x)
//{
//	if (x != 0) 
//	{ 
//		glDeleteTextures(1, &x);
//		x = 0; 
//	}
//}
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

#ifdef GLSL4_API
#include "ZZoglShaders.h"
#endif
static __forceinline void DrawTriangleArray()
{
#ifdef GLSL4_API
	ZZshSetupShader();
#endif
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	GL_REPORT_ERRORD();
}

static __forceinline void DrawBuffers(GLenum *buffer)
{
	if (glDrawBuffers != NULL) 
	{
		glDrawBuffers(1, buffer);
	}

	GL_REPORT_ERRORD();
}


namespace FB
{	
	extern u32 buf;

	static __forceinline void Create()
	{
		glGenFramebuffersEXT(1, &buf);
	}
	
	static __forceinline void Bind()
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, buf);
	}
	
	static __forceinline void Unbind()
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
		
	static __forceinline GLenum State()
	{
		return glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	}

	static __forceinline void Attach2D(int attach, int id = 0)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + attach, GL_TEXTURE_RECTANGLE_NV, id, 0);
		GL_REPORT_ERRORD();
	}

	static __forceinline void Attach(GLenum rend, GLuint id = 0)
	{
		glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, rend, GL_RENDERBUFFER_EXT, id);
	}	
};

static __forceinline void ResetRenderTarget(int index)
{
	FB::Attach2D(index);
}

static __forceinline void TextureImage(GLenum tex_type, GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage2D(tex_type, 0, iFormat, width, height, 0, format, type, pixels);
}

static __forceinline void Texture2D(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	TextureImage(GL_TEXTURE_2D, iFormat, width, height, format, type, pixels);
}

static __forceinline void Texture2D(GLint iFormat, GLenum format, GLenum type, const GLvoid* pixels)
{
	TextureImage(GL_TEXTURE_2D, iFormat, BLOCK_TEXWIDTH, BLOCK_TEXHEIGHT, format, type, pixels);
}
	
static __forceinline void TextureRect(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	TextureImage(GL_TEXTURE_RECTANGLE_NV, iFormat, width, height, format, type, pixels);
}

static __forceinline void TextureRect2(GLint iFormat, GLint width, GLint height, GLenum format, GLenum type, const GLvoid* pixels)
{
	TextureImage(GL_TEXTURE_RECTANGLE, iFormat, width, height, format, type, pixels);
}

static __forceinline void Texture3D(GLint iFormat, GLint width, GLint height, GLint depth, GLenum format, GLenum type, const GLvoid* pixels)
{
	glTexImage3D(GL_TEXTURE_3D, 0, iFormat, width, height, depth, 0, format, type, pixels);
}

static __forceinline void setTex2DFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setTex2DWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, type);
}

static __forceinline void setTex3DFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setTex3DWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, type);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, type);
}

static __forceinline void setRectFilters(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MAG_FILTER, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_MIN_FILTER, type);
}

static __forceinline void setRectWrap(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE_NV, GL_TEXTURE_WRAP_T, type);
}

static __forceinline void setRectWrap2(GLint type)
{
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, type);
	glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, type);
}

static __forceinline void GL_BLEND_SET()
{
	zgsBlendFuncSeparateEXT(s_srcrgb, s_dstrgb, s_srcalpha, s_dstalpha);
}

static __forceinline void GL_BLEND_RGB(GLenum src, GLenum dst)
{
	s_srcrgb = src;
	s_dstrgb = dst;
	GL_BLEND_SET();
}

static __forceinline void GL_BLEND_ALPHA(GLenum src, GLenum dst)
{
	s_srcalpha = src;
	s_dstalpha = dst;
	GL_BLEND_SET();
}

static __forceinline void GL_BLEND_ALL(GLenum srcrgb, GLenum dstrgb, GLenum srcalpha, GLenum dstalpha)
{
	s_srcrgb = srcrgb;
	s_dstrgb = dstrgb;
	s_srcalpha = srcalpha;
	s_dstalpha = dstalpha;
	GL_BLEND_SET();
}

static __forceinline void GL_ZTEST(bool enable)
{
	if (enable) 
		glEnable(GL_DEPTH_TEST);
	else 
		glDisable(GL_DEPTH_TEST);
}

static __forceinline void GL_ALPHATEST(bool enable)
{
	if (enable) 
		glEnable(GL_ALPHA_TEST);
	else 
		glDisable(GL_ALPHA_TEST);
}

static __forceinline void GL_BLENDEQ_RGB(GLenum eq)
{
	s_rgbeq = eq;
	zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq);
}

static __forceinline void GL_BLENDEQ_ALPHA(GLenum eq)
{
	s_alphaeq = eq;
	zgsBlendEquationSeparateEXT(s_rgbeq, s_alphaeq);
}

#endif // ZZGL_H_INCLUDED
