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

#include "stdafx.h"
#include "GLLoader.h"

// Those are provided on gl.h on linux...
#ifdef _WINDOWS
PFNGLACTIVETEXTUREPROC                 glActiveTexture                 =   NULL;
PFNGLBLENDCOLORPROC                    glBlendColor                    =   NULL;
#endif

PFNGLATTACHSHADERPROC                  glAttachShader                  =   NULL;
PFNGLBINDBUFFERPROC                    glBindBuffer                    =   NULL;
PFNGLBINDBUFFERBASEPROC                glBindBufferBase                =   NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC   glBindFragDataLocationIndexed   =   NULL;
PFNGLBINDFRAMEBUFFERPROC               glBindFramebuffer               =   NULL;
PFNGLBINDPROGRAMPIPELINEPROC           glBindProgramPipeline           =   NULL;
PFNGLBINDSAMPLERPROC                   glBindSampler                   =   NULL;
PFNGLBINDVERTEXARRAYPROC               glBindVertexArray               =   NULL;
PFNGLBLENDEQUATIONSEPARATEPROC         glBlendEquationSeparate         =   NULL;
PFNGLBLENDFUNCSEPARATEPROC             glBlendFuncSeparate             =   NULL;
PFNGLBLITFRAMEBUFFERPROC               glBlitFramebuffer               =   NULL;
PFNGLBUFFERDATAPROC                    glBufferData                    =   NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC        glCheckFramebufferStatus        =   NULL;
PFNGLCLEARBUFFERFVPROC                 glClearBufferfv                 =   NULL;
PFNGLCLEARBUFFERIVPROC                 glClearBufferiv                 =   NULL;
PFNGLCOMPILESHADERPROC                 glCompileShader                 =   NULL;
PFNGLCOPYIMAGESUBDATANVPROC            glCopyImageSubDataNV            =   NULL;
PFNGLCREATEPROGRAMPROC                 glCreateProgram                 =   NULL;
PFNGLCREATESHADERPROC                  glCreateShader                  =   NULL;
PFNGLCREATESHADERPROGRAMVPROC          glCreateShaderProgramv          =   NULL;
PFNGLDELETEBUFFERSPROC                 glDeleteBuffers                 =   NULL;
PFNGLDELETEFRAMEBUFFERSPROC            glDeleteFramebuffers            =   NULL;
PFNGLDELETEPROGRAMPROC                 glDeleteProgram                 =   NULL;
PFNGLDELETEPROGRAMPIPELINESPROC        glDeleteProgramPipelines        =   NULL;
PFNGLDELETESAMPLERSPROC                glDeleteSamplers                =   NULL;
PFNGLDELETESHADERPROC                  glDeleteShader                  =   NULL;
PFNGLDELETEVERTEXARRAYSPROC            glDeleteVertexArrays            =   NULL;
PFNGLDETACHSHADERPROC                  glDetachShader                  =   NULL;
PFNGLDRAWBUFFERSPROC                   glDrawBuffers                   =   NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC        glDrawElementsBaseVertex        =   NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC       glEnableVertexAttribArray       =   NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC       glFramebufferRenderbuffer       =   NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC          glFramebufferTexture2D          =   NULL;
PFNGLGENBUFFERSPROC                    glGenBuffers                    =   NULL;
PFNGLGENFRAMEBUFFERSPROC               glGenFramebuffers               =   NULL;
PFNGLGENPROGRAMPIPELINESPROC           glGenProgramPipelines           =   NULL;
PFNGLGENSAMPLERSPROC                   glGenSamplers                   =   NULL;
PFNGLGENVERTEXARRAYSPROC               glGenVertexArrays               =   NULL;
PFNGLGETBUFFERPARAMETERIVPROC          glGetBufferParameteriv          =   NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC         glGetDebugMessageLogARB         =   NULL;
PFNGLGETFRAGDATAINDEXPROC              glGetFragDataIndex              =   NULL;
PFNGLGETFRAGDATALOCATIONPROC           glGetFragDataLocation           =   NULL;
PFNGLGETPROGRAMINFOLOGPROC             glGetProgramInfoLog             =   NULL;
PFNGLGETPROGRAMIVPROC                  glGetProgramiv                  =   NULL;
PFNGLGETSHADERIVPROC                   glGetShaderiv                   =   NULL;
PFNGLGETSTRINGIPROC                    glGetStringi                    =   NULL;
PFNGLISFRAMEBUFFERPROC                 glIsFramebuffer                 =   NULL;
PFNGLLINKPROGRAMPROC                   glLinkProgram                   =   NULL;
PFNGLMAPBUFFERPROC                     glMapBuffer                     =   NULL;
PFNGLMAPBUFFERRANGEPROC                glMapBufferRange                =   NULL;
PFNGLPROGRAMPARAMETERIPROC             glProgramParameteri             =   NULL;
PFNGLSAMPLERPARAMETERFPROC             glSamplerParameterf             =   NULL;
PFNGLSAMPLERPARAMETERIPROC             glSamplerParameteri             =   NULL;
PFNGLSHADERSOURCEPROC                  glShaderSource                  =   NULL;
PFNGLUNIFORM1IPROC                     glUniform1i                     =   NULL;
PFNGLUNMAPBUFFERPROC                   glUnmapBuffer                   =   NULL;
PFNGLUSEPROGRAMSTAGESPROC              glUseProgramStages              =   NULL;
PFNGLVERTEXATTRIBIPOINTERPROC          glVertexAttribIPointer          =   NULL;
PFNGLVERTEXATTRIBPOINTERPROC           glVertexAttribPointer           =   NULL;

namespace GLLoader {

    bool check_gl_version(uint32 major, uint32 minor) {
		const GLubyte* s;
		s = glGetString(GL_VERSION);
		if (s == NULL) return false;
		fprintf(stderr, "Supported Opengl version: %s on GPU: %s. Vendor: %s\n", s, glGetString(GL_RENDERER), glGetString(GL_VENDOR));
		// Could be useful to detect the GPU vendor:
		// if ( strcmp((const char*)glGetString(GL_VENDOR), "ATI Technologies Inc.") == 0 )

		GLuint dot = 0;
		while (s[dot] != '\0' && s[dot] != '.') dot++;
		if (dot == 0) return false;

		GLuint major_gl = s[dot-1]-'0';
		GLuint minor_gl = s[dot+1]-'0';

		if ( (major_gl < major) || ( major_gl == major && minor_gl < minor ) ) {
			fprintf(stderr, "OPENGL 3.3 is not supported\n");
			return false;
		}

        return true;
    }

    void init_gl_function() {
		// Those are provided on gl.h on linux...
#ifdef _WINDOWS
		GL_LOADFN(glActiveTexture);
		GL_LOADFN(glBlendColor);
#endif

		GL_LOADFN(glAttachShader);
		GL_LOADFN(glBindBuffer);
		GL_LOADFN(glBindBufferBase);
		GL_LOADFN(glBindFragDataLocationIndexed);
		GL_LOADFN(glBindFramebuffer);
		GL_LOADFN(glBindProgramPipeline);
		GL_LOADFN(glBindSampler);
		GL_LOADFN(glBindVertexArray);
		GL_LOADFN(glBlendEquationSeparate);
		GL_LOADFN(glBlendFuncSeparate);
		GL_LOADFN(glBlitFramebuffer);
		GL_LOADFN(glBufferData);
		GL_LOADFN(glCheckFramebufferStatus);
		GL_LOADFN(glClearBufferfv);
		GL_LOADFN(glClearBufferiv);
		GL_LOADFN(glCompileShader);
		GL_LOADFN(glCopyImageSubDataNV);
		GL_LOADFN(glCreateProgram);
		GL_LOADFN(glCreateShader);
		GL_LOADFN(glCreateShaderProgramv);
		GL_LOADFN(glDeleteBuffers);
		GL_LOADFN(glDeleteFramebuffers);
		GL_LOADFN(glDeleteProgram);
		GL_LOADFN(glDeleteProgramPipelines);
		GL_LOADFN(glDeleteSamplers);
		GL_LOADFN(glDeleteShader);
		GL_LOADFN(glDeleteVertexArrays);
		GL_LOADFN(glDetachShader);
		GL_LOADFN(glDrawBuffers);
		GL_LOADFN(glDrawElementsBaseVertex);
		GL_LOADFN(glEnableVertexAttribArray);
		GL_LOADFN(glFramebufferRenderbuffer);
		GL_LOADFN(glFramebufferTexture2D);
		GL_LOADFN(glGenBuffers);
		GL_LOADFN(glGenFramebuffers);
		GL_LOADFN(glGenProgramPipelines);
		GL_LOADFN(glGenSamplers);
		GL_LOADFN(glGenVertexArrays);
		GL_LOADFN(glGetBufferParameteriv);
		GL_LOADFN(glGetDebugMessageLogARB);
		GL_LOADFN(glGetFragDataIndex);
		GL_LOADFN(glGetFragDataLocation);
		GL_LOADFN(glGetProgramInfoLog);
		GL_LOADFN(glGetProgramiv);
		GL_LOADFN(glGetShaderiv);
		GL_LOADFN(glGetStringi);
		GL_LOADFN(glIsFramebuffer);
		GL_LOADFN(glLinkProgram);
		GL_LOADFN(glMapBuffer);
		GL_LOADFN(glMapBufferRange);
		GL_LOADFN(glProgramParameteri);
		GL_LOADFN(glSamplerParameterf);
		GL_LOADFN(glSamplerParameteri);
		GL_LOADFN(glShaderSource);
		GL_LOADFN(glUniform1i);
		GL_LOADFN(glUnmapBuffer);
		GL_LOADFN(glUseProgramStages);
		GL_LOADFN(glVertexAttribIPointer);
		GL_LOADFN(glVertexAttribPointer);
    }

	bool check_gl_supported_extension() {
		int max_ext = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max_ext);

		bool found_GL_ARB_separate_shader_objects = false;
		bool found_GL_ARB_shading_language_420pack = false;
		fprintf(stderr, "DEBUG: check_gl_supported_extension\n");

		if (glGetStringi && max_ext) {
			for (GLint i = 0; i < max_ext; i++) {
				string ext((const char*)glGetStringi(GL_EXTENSIONS, i));
				if (ext.compare("GL_ARB_separate_shader_objects") == 0) {
					found_GL_ARB_separate_shader_objects = true;
				}
				if (ext.compare("GL_ARB_shading_language_420pack") == 0) {
					found_GL_ARB_shading_language_420pack = true;
				}
				fprintf(stderr, "EXT: %s\n", ext.c_str());
			}
		}

		if ( !found_GL_ARB_separate_shader_objects) {
			fprintf(stderr, "GL_ARB_separate_shader_objects is not supported\n");
			return false;
		}
		if ( !found_GL_ARB_shading_language_420pack) {
			fprintf(stderr, "GL_ARB_shading_language_420pack is not supported\n");
			//return false;
		}

		return true;
	}
}
