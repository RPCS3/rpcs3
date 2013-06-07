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
#include "GSdx.h"

PFNGLACTIVETEXTUREPROC                 gl_ActiveTexture               = NULL;
PFNGLBLENDCOLORPROC                    gl_BlendColor                  = NULL;
PFNGLATTACHSHADERPROC                  gl_AttachShader                = NULL;
PFNGLBINDBUFFERPROC                    gl_BindBuffer                  = NULL;
PFNGLBINDBUFFERBASEPROC                gl_BindBufferBase              = NULL;
PFNGLBINDFRAGDATALOCATIONINDEXEDPROC   gl_BindFragDataLocationIndexed = NULL;
PFNGLBINDFRAMEBUFFERPROC               gl_BindFramebuffer             = NULL;
PFNGLBINDPROGRAMPIPELINEPROC           gl_BindProgramPipeline         = NULL;
PFNGLBINDSAMPLERPROC                   gl_BindSampler                 = NULL;
PFNGLBINDVERTEXARRAYPROC               gl_BindVertexArray             = NULL;
PFNGLBLENDEQUATIONSEPARATEPROC         gl_BlendEquationSeparate       = NULL;
PFNGLBLENDFUNCSEPARATEPROC             gl_BlendFuncSeparate           = NULL;
PFNGLBLITFRAMEBUFFERPROC               gl_BlitFramebuffer             = NULL;
PFNGLBUFFERDATAPROC                    gl_BufferData                  = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC        gl_CheckFramebufferStatus      = NULL;
PFNGLCLEARBUFFERFVPROC                 gl_ClearBufferfv               = NULL;
PFNGLCLEARBUFFERIVPROC                 gl_ClearBufferiv               = NULL;
PFNGLCOMPILESHADERPROC                 gl_CompileShader               = NULL;
PFNGLCOPYIMAGESUBDATANVPROC            gl_CopyImageSubDataNV          = NULL;
PFNGLCREATEPROGRAMPROC                 gl_CreateProgram               = NULL;
PFNGLCREATESHADERPROC                  gl_CreateShader                = NULL;
PFNGLCREATESHADERPROGRAMVPROC          gl_CreateShaderProgramv        = NULL;
PFNGLDELETEBUFFERSPROC                 gl_DeleteBuffers               = NULL;
PFNGLDELETEFRAMEBUFFERSPROC            gl_DeleteFramebuffers          = NULL;
PFNGLDELETEPROGRAMPROC                 gl_DeleteProgram               = NULL;
PFNGLDELETEPROGRAMPIPELINESPROC        gl_DeleteProgramPipelines      = NULL;
PFNGLDELETESAMPLERSPROC                gl_DeleteSamplers              = NULL;
PFNGLDELETESHADERPROC                  gl_DeleteShader                = NULL;
PFNGLDELETEVERTEXARRAYSPROC            gl_DeleteVertexArrays          = NULL;
PFNGLDETACHSHADERPROC                  gl_DetachShader                = NULL;
PFNGLDRAWBUFFERSPROC                   gl_DrawBuffers                 = NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC        gl_DrawElementsBaseVertex      = NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC       gl_EnableVertexAttribArray     = NULL;
PFNGLFRAMEBUFFERRENDERBUFFERPROC       gl_FramebufferRenderbuffer     = NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC          gl_FramebufferTexture2D        = NULL;
PFNGLGENBUFFERSPROC                    gl_GenBuffers                  = NULL;
PFNGLGENFRAMEBUFFERSPROC               gl_GenFramebuffers             = NULL;
PFNGLGENPROGRAMPIPELINESPROC           gl_GenProgramPipelines         = NULL;
PFNGLGENSAMPLERSPROC                   gl_GenSamplers                 = NULL;
PFNGLGENVERTEXARRAYSPROC               gl_GenVertexArrays             = NULL;
PFNGLGETBUFFERPARAMETERIVPROC          gl_GetBufferParameteriv        = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC         gl_GetDebugMessageLogARB       = NULL;
PFNGLGETFRAGDATAINDEXPROC              gl_GetFragDataIndex            = NULL;
PFNGLGETFRAGDATALOCATIONPROC           gl_GetFragDataLocation         = NULL;
PFNGLGETPROGRAMINFOLOGPROC             gl_GetProgramInfoLog           = NULL;
PFNGLGETPROGRAMIVPROC                  gl_GetProgramiv                = NULL;
PFNGLGETSHADERIVPROC                   gl_GetShaderiv                 = NULL;
PFNGLGETSTRINGIPROC                    gl_GetStringi                  = NULL;
PFNGLISFRAMEBUFFERPROC                 gl_IsFramebuffer               = NULL;
PFNGLLINKPROGRAMPROC                   gl_LinkProgram                 = NULL;
PFNGLMAPBUFFERPROC                     gl_MapBuffer                   = NULL;
PFNGLMAPBUFFERRANGEPROC                gl_MapBufferRange              = NULL;
PFNGLPROGRAMPARAMETERIPROC             gl_ProgramParameteri           = NULL;
PFNGLSAMPLERPARAMETERFPROC             gl_SamplerParameterf           = NULL;
PFNGLSAMPLERPARAMETERIPROC             gl_SamplerParameteri           = NULL;
PFNGLSHADERSOURCEPROC                  gl_ShaderSource                = NULL;
PFNGLUNIFORM1IPROC                     gl_Uniform1i                   = NULL;
PFNGLUNMAPBUFFERPROC                   gl_UnmapBuffer                 = NULL;
PFNGLUSEPROGRAMSTAGESPROC              gl_UseProgramStages            = NULL;
PFNGLVERTEXATTRIBIPOINTERPROC          gl_VertexAttribIPointer        = NULL;
PFNGLVERTEXATTRIBPOINTERPROC           gl_VertexAttribPointer         = NULL;
PFNGLTEXSTORAGE2DPROC                  gl_TexStorage2D                = NULL;
PFNGLBUFFERSUBDATAPROC                 gl_BufferSubData               = NULL;
// NO GL4.1
PFNGLUSEPROGRAMPROC                    gl_UseProgram                  = NULL;
PFNGLGETSHADERINFOLOGPROC              gl_GetShaderInfoLog            = NULL;
PFNGLPROGRAMUNIFORM1IPROC              gl_ProgramUniform1i            = NULL;
// NO GL4.2
PFNGLGETUNIFORMBLOCKINDEXPROC          gl_GetUniformBlockIndex        = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC           gl_UniformBlockBinding         = NULL;
PFNGLGETUNIFORMLOCATIONPROC            gl_GetUniformLocation          = NULL;

namespace GLLoader {

	bool fglrx_buggy_driver = false;
	bool found_GL_ARB_separate_shader_objects = false;
	bool found_GL_ARB_shading_language_420pack = false;
	bool found_GL_ARB_texture_storage = false;
	bool found_geometry_shader = true;
	bool found_GL_NV_copy_image = false;
	bool found_GL_ARB_copy_image = false;
	bool found_only_gl30	= false;

    bool check_gl_version(uint32 major, uint32 minor) {

		const GLubyte* s = glGetString(GL_VERSION);
		if (s == NULL) return false;
		fprintf(stderr, "Supported Opengl version: %s on GPU: %s. Vendor: %s\n", s, glGetString(GL_RENDERER), glGetString(GL_VENDOR));

		// Could be useful to detect the GPU vendor:
		if (strcmp((const char*)glGetString(GL_VENDOR), "ATI Technologies Inc.") == 0)
			fglrx_buggy_driver = true;

		GLuint dot = 0;
		while (s[dot] != '\0' && s[dot] != '.') dot++;
		if (dot == 0) return false;

		GLuint major_gl = s[dot-1]-'0';
		GLuint minor_gl = s[dot+1]-'0';

		if ( (major_gl < 3) || ( major_gl == 3 && minor_gl < 2 ) ) {
			fprintf(stderr, "Geometry shaders are not supported. Required openGL3.2\n");
			found_geometry_shader = false;
		}
		if (theApp.GetConfig("override_geometry_shader", -1) != -1) {
			found_geometry_shader = !!theApp.GetConfig("override_geometry_shader", -1);
			fprintf(stderr, "Override geometry shaders detection\n");
		}
		if ( (major_gl == 3) && minor_gl < 3) {
			// Opensource driver spotted
			found_only_gl30 = true;
		}

		if ( (major_gl < major) || ( major_gl == major && minor_gl < minor ) ) {
			fprintf(stderr, "OPENGL %d.%d is not supported\n", major, minor);
			return false;
		}


        return true;
    }

    void init_gl_function() {
		GL_LOADFN(gl_ActiveTexture, glActiveTexture);
		GL_LOADFN(gl_BlendColor, glBlendColor);
		GL_LOADFN(gl_AttachShader, glAttachShader);
		GL_LOADFN(gl_BindBuffer, glBindBuffer);
		GL_LOADFN(gl_BindBufferBase, glBindBufferBase);
		GL_LOADFN(gl_BindFragDataLocationIndexed, glBindFragDataLocationIndexed);
		GL_LOADFN(gl_BindFramebuffer, glBindFramebuffer);
		GL_LOADFN(gl_BindProgramPipeline, glBindProgramPipeline);
		GL_LOADFN(gl_BindSampler, glBindSampler);
		GL_LOADFN(gl_BindVertexArray, glBindVertexArray);
		GL_LOADFN(gl_BlendEquationSeparate, glBlendEquationSeparate);
		GL_LOADFN(gl_BlendFuncSeparate, glBlendFuncSeparate);
		GL_LOADFN(gl_BlitFramebuffer, glBlitFramebuffer);
		GL_LOADFN(gl_BufferData, glBufferData);
		GL_LOADFN(gl_CheckFramebufferStatus, glCheckFramebufferStatus);
		GL_LOADFN(gl_ClearBufferfv, glClearBufferfv);
		GL_LOADFN(gl_ClearBufferiv, glClearBufferiv);
		GL_LOADFN(gl_CompileShader, glCompileShader);
		GL_LOADFN(gl_CopyImageSubDataNV, glCopyImageSubDataNV);
		GL_LOADFN(gl_CreateProgram, glCreateProgram);
		GL_LOADFN(gl_CreateShader, glCreateShader);
		GL_LOADFN(gl_CreateShaderProgramv, glCreateShaderProgramv);
		GL_LOADFN(gl_DeleteBuffers, glDeleteBuffers);
		GL_LOADFN(gl_DeleteFramebuffers, glDeleteFramebuffers);
		GL_LOADFN(gl_DeleteProgram, glDeleteProgram);
		GL_LOADFN(gl_DeleteProgramPipelines, glDeleteProgramPipelines);
		GL_LOADFN(gl_DeleteSamplers, glDeleteSamplers);
		GL_LOADFN(gl_DeleteShader, glDeleteShader);
		GL_LOADFN(gl_DeleteVertexArrays, glDeleteVertexArrays);
		GL_LOADFN(gl_DetachShader, glDetachShader);
		GL_LOADFN(gl_DrawBuffers, glDrawBuffers);
		GL_LOADFN(gl_DrawElementsBaseVertex, glDrawElementsBaseVertex);
		GL_LOADFN(gl_EnableVertexAttribArray, glEnableVertexAttribArray);
		GL_LOADFN(gl_FramebufferRenderbuffer, glFramebufferRenderbuffer);
		GL_LOADFN(gl_FramebufferTexture2D, glFramebufferTexture2D);
		GL_LOADFN(gl_GenBuffers, glGenBuffers);
		GL_LOADFN(gl_GenFramebuffers, glGenFramebuffers);
		GL_LOADFN(gl_GenProgramPipelines, glGenProgramPipelines);
		GL_LOADFN(gl_GenSamplers, glGenSamplers);
		GL_LOADFN(gl_GenVertexArrays, glGenVertexArrays);
		GL_LOADFN(gl_GetBufferParameteriv, glGetBufferParameteriv);
		GL_LOADFN(gl_GetDebugMessageLogARB, glGetDebugMessageLogARB);
		GL_LOADFN(gl_GetFragDataIndex, glGetFragDataIndex);
		GL_LOADFN(gl_GetFragDataLocation, glGetFragDataLocation);
		GL_LOADFN(gl_GetProgramInfoLog, glGetProgramInfoLog);
		GL_LOADFN(gl_GetProgramiv, glGetProgramiv);
		GL_LOADFN(gl_GetShaderiv, glGetShaderiv);
		GL_LOADFN(gl_GetStringi, glGetStringi);
		GL_LOADFN(gl_IsFramebuffer, glIsFramebuffer);
		GL_LOADFN(gl_LinkProgram, glLinkProgram);
		GL_LOADFN(gl_MapBuffer, glMapBuffer);
		GL_LOADFN(gl_MapBufferRange, glMapBufferRange);
		GL_LOADFN(gl_ProgramParameteri, glProgramParameteri);
		GL_LOADFN(gl_SamplerParameterf, glSamplerParameterf);
		GL_LOADFN(gl_SamplerParameteri, glSamplerParameteri);
		GL_LOADFN(gl_ShaderSource, glShaderSource);
		GL_LOADFN(gl_Uniform1i, glUniform1i);
		GL_LOADFN(gl_UnmapBuffer, glUnmapBuffer);
		GL_LOADFN(gl_UseProgramStages, glUseProgramStages);
		GL_LOADFN(gl_VertexAttribIPointer, glVertexAttribIPointer);
		GL_LOADFN(gl_VertexAttribPointer, glVertexAttribPointer);
		GL_LOADFN(gl_TexStorage2D, glTexStorage2D);
		GL_LOADFN(gl_BufferSubData, glBufferSubData);
		// NO GL4.1
		GL_LOADFN(gl_UseProgram, glUseProgram);
		GL_LOADFN(gl_GetShaderInfoLog, glGetShaderInfoLog);
		GL_LOADFN(gl_ProgramUniform1i, glProgramUniform1i);
		// NO GL4.2
		GL_LOADFN(gl_GetUniformBlockIndex, glGetUniformBlockIndex);
		GL_LOADFN(gl_UniformBlockBinding, glUniformBlockBinding);
		GL_LOADFN(gl_GetUniformLocation, glGetUniformLocation);
    }

	bool check_gl_supported_extension() {
		int max_ext = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max_ext);

		fprintf(stderr, "DEBUG: check_gl_supported_extension\n");

		if (gl_GetStringi && max_ext) {
			for (GLint i = 0; i < max_ext; i++) {
				string ext((const char*)gl_GetStringi(GL_EXTENSIONS, i));
				if (ext.compare("GL_ARB_separate_shader_objects") == 0) {
					if (!fglrx_buggy_driver) found_GL_ARB_separate_shader_objects = true;
					else fprintf(stderr, "Buggy driver detected, GL_ARB_separate_shader_objects will be disabled\n");
				}
				if (ext.compare("GL_ARB_shading_language_420pack") == 0) found_GL_ARB_shading_language_420pack = true;
				if (ext.compare("GL_ARB_texture_storage") == 0) found_GL_ARB_texture_storage = true;
				if (ext.compare("GL_NV_copy_image") == 0) found_GL_NV_copy_image = true;
				// Replace previous extensions (when driver will be updated)
				if (ext.compare("GL_ARB_copy_image") == 0) found_GL_ARB_copy_image = true;
			}
		}

		if (!found_GL_ARB_separate_shader_objects) {
			fprintf(stderr, "GL_ARB_separate_shader_objects is not supported\n");
		}
		if (!found_GL_ARB_shading_language_420pack) {
			fprintf(stderr, "GL_ARB_shading_language_420pack is not supported\n");
		}
		if (!found_GL_ARB_texture_storage) {
			fprintf(stderr, "GL_ARB_texture_storage is not supported\n");
			return false;
		}


		if (theApp.GetConfig("override_GL_ARB_shading_language_420pack", -1) != -1) {
			found_GL_ARB_shading_language_420pack = !!theApp.GetConfig("override_GL_ARB_shading_language_420pack", -1);
			fprintf(stderr, "Override GL_ARB_shading_language_420pack detection\n");
		}
		if (theApp.GetConfig("override_GL_ARB_separate_shader_objects", -1) != -1) {
			found_GL_ARB_separate_shader_objects = !!theApp.GetConfig("override_GL_ARB_separate_shader_objects", -1);
			fprintf(stderr, "Override GL_ARB_separate_shader_objects detection\n");
		}
		if (theApp.GetConfig("ovveride_GL_ARB_copy_image", -1) != -1) {
			// Same extension so override both
			found_GL_ARB_copy_image = !!theApp.GetConfig("override_GL_ARB_copy_image", -1);
			found_GL_NV_copy_image = !!theApp.GetConfig("override_GL_ARB_copy_image", -1);
			fprintf(stderr, "Override GL_ARB_copy_image detection\n");
			fprintf(stderr, "Override GL_NV_copy_image detection\n");
		}

		return true;
	}
}
