/*
 *	Copyright (C) 2011-2013 Gregory hainaut
 *	Copyright (C) 2007-2009 Gabest
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#pragma once

#ifdef _LINUX
#include <GL/glx.h>
#include <EGL/egl.h>
#include <EGL/eglext.h>
#endif

#ifdef _WINDOWS
#define GL_LOADFN(name) { \
		if( (*(void**)&name = (void*)wglGetProcAddress(#name)) == NULL ) { \
			fprintf(stderr,"Failed to find %s\n", #name); \
		} \
}
#else
#ifdef EGL_API
#define GL_LOADFN(name) { \
	if( (*(void**)&name = (void*)eglGetProcAddress(#name)) == NULL ) { \
		fprintf(stderr,"Failed to find %s\n", #name); \
	} \
}
#else
#define GL_LOADFN(name) { \
	if( (*(void**)&name = (void*)glXGetProcAddress((const GLubyte*)#name)) == NULL ) { \
		fprintf(stderr,"Failed to find %s\n", #name); \
	} \
}
#endif
#endif

// Those are provided on gl.h on linux...
#ifdef _WINDOWS
extern   PFNGLACTIVETEXTUREPROC                 glActiveTexture;
extern   PFNGLBLENDCOLORPROC                    glBlendColor;
#endif
extern   PFNGLATTACHSHADERPROC                  glAttachShader;
extern   PFNGLBINDBUFFERPROC                    glBindBuffer;
extern   PFNGLBINDBUFFERBASEPROC                glBindBufferBase;
extern   PFNGLBINDFRAGDATALOCATIONINDEXEDPROC   glBindFragDataLocationIndexed;
extern   PFNGLBINDFRAMEBUFFERPROC               glBindFramebuffer;
extern   PFNGLBINDPROGRAMPIPELINEPROC           glBindProgramPipeline;
extern   PFNGLBINDSAMPLERPROC                   glBindSampler;
extern   PFNGLBINDVERTEXARRAYPROC               glBindVertexArray;
extern   PFNGLBLENDEQUATIONSEPARATEPROC         glBlendEquationSeparate;
extern   PFNGLBLENDFUNCSEPARATEPROC             glBlendFuncSeparate;
extern   PFNGLBLITFRAMEBUFFERPROC               glBlitFramebuffer;
extern   PFNGLBUFFERDATAPROC                    glBufferData;
extern   PFNGLCHECKFRAMEBUFFERSTATUSPROC        glCheckFramebufferStatus;
extern   PFNGLCLEARBUFFERFVPROC                 glClearBufferfv;
extern   PFNGLCLEARBUFFERIVPROC                 glClearBufferiv;
extern   PFNGLCOMPILESHADERPROC                 glCompileShader;
extern   PFNGLCOPYIMAGESUBDATANVPROC            glCopyImageSubDataNV;
extern   PFNGLCREATEPROGRAMPROC                 glCreateProgram;
extern   PFNGLCREATESHADERPROC                  glCreateShader;
extern   PFNGLCREATESHADERPROGRAMVPROC          glCreateShaderProgramv;
extern   PFNGLDELETEBUFFERSPROC                 glDeleteBuffers;
extern   PFNGLDELETEFRAMEBUFFERSPROC            glDeleteFramebuffers;
extern   PFNGLDELETEPROGRAMPROC                 glDeleteProgram;
extern   PFNGLDELETEPROGRAMPIPELINESPROC        glDeleteProgramPipelines;
extern   PFNGLDELETESAMPLERSPROC                glDeleteSamplers;
extern   PFNGLDELETESHADERPROC                  glDeleteShader;
extern   PFNGLDELETEVERTEXARRAYSPROC            glDeleteVertexArrays;
extern   PFNGLDETACHSHADERPROC                  glDetachShader;
extern   PFNGLDRAWBUFFERSPROC                   glDrawBuffers;
extern   PFNGLDRAWELEMENTSBASEVERTEXPROC        glDrawElementsBaseVertex;
extern   PFNGLENABLEVERTEXATTRIBARRAYPROC       glEnableVertexAttribArray;
extern   PFNGLFRAMEBUFFERRENDERBUFFERPROC       glFramebufferRenderbuffer;
extern   PFNGLFRAMEBUFFERTEXTURE2DPROC          glFramebufferTexture2D;
extern   PFNGLGENBUFFERSPROC                    glGenBuffers;
extern   PFNGLGENFRAMEBUFFERSPROC               glGenFramebuffers;
extern   PFNGLGENPROGRAMPIPELINESPROC           glGenProgramPipelines;
extern   PFNGLGENSAMPLERSPROC                   glGenSamplers;
extern   PFNGLGENVERTEXARRAYSPROC               glGenVertexArrays;
extern   PFNGLGETBUFFERPARAMETERIVPROC          glGetBufferParameteriv;
extern   PFNGLGETDEBUGMESSAGELOGARBPROC         glGetDebugMessageLogARB;
extern   PFNGLGETFRAGDATAINDEXPROC              glGetFragDataIndex;
extern   PFNGLGETFRAGDATALOCATIONPROC           glGetFragDataLocation;
extern   PFNGLGETPROGRAMINFOLOGPROC             glGetProgramInfoLog;
extern   PFNGLGETPROGRAMIVPROC                  glGetProgramiv;
extern   PFNGLGETSHADERIVPROC                   glGetShaderiv;
extern   PFNGLGETSTRINGIPROC                    glGetStringi;
extern   PFNGLISFRAMEBUFFERPROC                 glIsFramebuffer;
extern   PFNGLLINKPROGRAMPROC                   glLinkProgram;
extern   PFNGLMAPBUFFERPROC                     glMapBuffer;
extern   PFNGLMAPBUFFERRANGEPROC                glMapBufferRange;
extern   PFNGLPROGRAMPARAMETERIPROC             glProgramParameteri;
extern   PFNGLSAMPLERPARAMETERFPROC             glSamplerParameterf;
extern   PFNGLSAMPLERPARAMETERIPROC             glSamplerParameteri;
extern   PFNGLSHADERSOURCEPROC                  glShaderSource;
extern   PFNGLUNIFORM1IPROC                     glUniform1i;
extern   PFNGLUNMAPBUFFERPROC                   glUnmapBuffer;
extern   PFNGLUSEPROGRAMSTAGESPROC              glUseProgramStages;
extern   PFNGLVERTEXATTRIBIPOINTERPROC          glVertexAttribIPointer;
extern   PFNGLVERTEXATTRIBPOINTERPROC           glVertexAttribPointer;

namespace GLLoader {
	bool check_gl_version(uint32 major, uint32 minor);
	void init_gl_function();
	bool check_gl_supported_extension();
}
