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

#ifndef ENABLE_GLES
PFNGLACTIVETEXTUREPROC                 gl_ActiveTexture               = NULL;
PFNGLBLENDCOLORPROC                    gl_BlendColor                  = NULL;
PFNGLATTACHSHADERPROC                  gl_AttachShader                = NULL;
PFNGLBINDBUFFERPROC                    gl_BindBuffer                  = NULL;
PFNGLBINDBUFFERBASEPROC                gl_BindBufferBase              = NULL;
PFNGLBINDFRAMEBUFFERPROC               gl_BindFramebuffer             = NULL;
PFNGLBINDSAMPLERPROC                   gl_BindSampler                 = NULL;
PFNGLBINDVERTEXARRAYPROC               gl_BindVertexArray             = NULL;
PFNGLBLENDEQUATIONSEPARATEIARBPROC     gl_BlendEquationSeparateiARB   = NULL;
PFNGLBLENDFUNCSEPARATEIARBPROC         gl_BlendFuncSeparateiARB       = NULL;
PFNGLBLITFRAMEBUFFERPROC               gl_BlitFramebuffer             = NULL;
PFNGLBUFFERDATAPROC                    gl_BufferData                  = NULL;
PFNGLCHECKFRAMEBUFFERSTATUSPROC        gl_CheckFramebufferStatus      = NULL;
PFNGLCLEARBUFFERFVPROC                 gl_ClearBufferfv               = NULL;
PFNGLCLEARBUFFERIVPROC                 gl_ClearBufferiv               = NULL;
PFNGLCLEARBUFFERUIVPROC                gl_ClearBufferuiv              = NULL;
PFNGLCOLORMASKIPROC                    gl_ColorMaski                  = NULL;
PFNGLCOMPILESHADERPROC                 gl_CompileShader               = NULL;
PFNGLCREATEPROGRAMPROC                 gl_CreateProgram               = NULL;
PFNGLCREATESHADERPROC                  gl_CreateShader                = NULL;
PFNGLCREATESHADERPROGRAMVPROC          gl_CreateShaderProgramv        = NULL;
PFNGLDELETEBUFFERSPROC                 gl_DeleteBuffers               = NULL;
PFNGLDELETEFRAMEBUFFERSPROC            gl_DeleteFramebuffers          = NULL;
PFNGLDELETEPROGRAMPROC                 gl_DeleteProgram               = NULL;
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
PFNGLGENSAMPLERSPROC                   gl_GenSamplers                 = NULL;
PFNGLGENVERTEXARRAYSPROC               gl_GenVertexArrays             = NULL;
PFNGLGETBUFFERPARAMETERIVPROC          gl_GetBufferParameteriv        = NULL;
PFNGLGETDEBUGMESSAGELOGARBPROC         gl_GetDebugMessageLogARB       = NULL;
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
PFNGLBUFFERSUBDATAPROC                 gl_BufferSubData               = NULL;
PFNGLFENCESYNCPROC                     gl_FenceSync                   = NULL;
PFNGLDELETESYNCPROC                    gl_DeleteSync                  = NULL;
PFNGLCLIENTWAITSYNCPROC                gl_ClientWaitSync              = NULL;
PFNGLFLUSHMAPPEDBUFFERRANGEPROC        gl_FlushMappedBufferRange      = NULL;
// GL4.0
PFNGLUNIFORMSUBROUTINESUIVPROC         gl_UniformSubroutinesuiv       = NULL;
// GL4.1
PFNGLBINDPROGRAMPIPELINEPROC           gl_BindProgramPipeline         = NULL;
PFNGLGENPROGRAMPIPELINESPROC           gl_GenProgramPipelines         = NULL;
PFNGLDELETEPROGRAMPIPELINESPROC        gl_DeleteProgramPipelines      = NULL;
PFNGLGETPROGRAMPIPELINEIVPROC          gl_GetProgramPipelineiv        = NULL;
PFNGLVALIDATEPROGRAMPIPELINEPROC       gl_ValidateProgramPipeline     = NULL;
PFNGLGETPROGRAMPIPELINEINFOLOGPROC     gl_GetProgramPipelineInfoLog   = NULL;
// NO GL4.1
PFNGLUSEPROGRAMPROC                    gl_UseProgram                  = NULL;
PFNGLGETSHADERINFOLOGPROC              gl_GetShaderInfoLog            = NULL;
PFNGLPROGRAMUNIFORM1IPROC              gl_ProgramUniform1i            = NULL;
// NO GL4.2
PFNGLGETUNIFORMBLOCKINDEXPROC          gl_GetUniformBlockIndex        = NULL;
PFNGLUNIFORMBLOCKBINDINGPROC           gl_UniformBlockBinding         = NULL;
PFNGLGETUNIFORMLOCATIONPROC            gl_GetUniformLocation          = NULL;
// GL4.3
PFNGLCOPYIMAGESUBDATAPROC              gl_CopyImageSubData            = NULL;
// GL4.2
PFNGLBINDIMAGETEXTUREPROC              gl_BindImageTexture            = NULL;
PFNGLMEMORYBARRIERPROC                 gl_MemoryBarrier               = NULL;
PFNGLTEXSTORAGE2DPROC                  gl_TexStorage2D                = NULL;
// GL4.4
PFNGLCLEARTEXIMAGEPROC                 gl_ClearTexImage               = NULL;
PFNGLBINDTEXTURESPROC                  gl_BindTextures                = NULL;
PFNGLBUFFERSTORAGEPROC                 gl_BufferStorage               = NULL;
// GL_ARB_bindless_texture (GL5?)
PFNGLGETTEXTURESAMPLERHANDLEARBPROC    gl_GetTextureSamplerHandleARB  = NULL;
PFNGLMAKETEXTUREHANDLERESIDENTARBPROC  gl_MakeTextureHandleResidentARB = NULL;
PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC gl_MakeTextureHandleNonResidentARB = NULL;
PFNGLUNIFORMHANDLEUI64VARBPROC         gl_UniformHandleui64vARB        = NULL;
PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC  gl_ProgramUniformHandleui64vARB = NULL;
#endif

namespace GLLoader {

	bool fglrx_buggy_driver = false;
	bool nvidia_buggy_driver = false;
	bool in_replayer = false;

	// Optional
	bool found_GL_ARB_separate_shader_objects = false;
	bool found_geometry_shader = true;
	bool found_only_gl30	= false; // Drop it when mesa support GLSL330
	bool found_GL_ARB_clear_texture = false; // Don't know if GL3 GPU can support it
	bool found_GL_ARB_buffer_storage = false;
	bool found_GL_ARB_explicit_uniform_location = false; // need by subroutine
	// GL4 hardware
	bool found_GL_ARB_copy_image = false; // Not sure actually maybe GL3 GPU can do it
	bool found_GL_ARB_gpu_shader5 = false;
	bool found_GL_ARB_shader_image_load_store = false;
	bool found_GL_ARB_shader_subroutine = false;
	bool found_GL_ARB_bindless_texture = false; // GL5 GPU?

	// Mandatory for FULL GL (but optional for GLES)
	bool found_GL_ARB_multi_bind = false; // Not yet. Wait Mesa & AMD drivers. Note might be deprecated by bindless_texture
	bool found_GL_ARB_shading_language_420pack = false; // Not yet. Wait Mesa & AMD drivers

	// Mandatory
	bool found_GL_ARB_texture_storage = false;

	static bool status_and_override(bool& found, const std::string& name, bool mandatory = false)
	{
		if (!found) {
			fprintf(stderr, "INFO: %s is not supported\n", name.c_str());
			if(mandatory) return false;
		}

		std::string opt("override_");
		opt += name;

		if (theApp.GetConfig(opt.c_str(), -1) != -1) {
			found = !!theApp.GetConfig(opt.c_str(), -1);
			fprintf(stderr, "Override %s detection (%s)\n", name.c_str(), found ? "Enabled" : "Disabled");
		}

		return true;
	}

    bool check_gl_version(uint32 major, uint32 minor) {

		const GLubyte* s = glGetString(GL_VERSION);
		if (s == NULL) {
			fprintf(stderr, "Error: GLLoader failed to get GL version\n");
			return false;
		}

		const char* vendor = (const char*)glGetString(GL_VENDOR);
		fprintf(stderr, "Supported Opengl version: %s on GPU: %s. Vendor: %s\n", s, glGetString(GL_RENDERER), vendor);

		// Name change but driver is still bad!
		if (strstr(vendor, "ATI") || strstr(vendor, "Advanced Micro Devices"))
			fglrx_buggy_driver = true;
		if (strstr(vendor, "NVIDIA Corporation"))
			nvidia_buggy_driver = true;

		GLuint dot = 0;
		while (s[dot] != '\0' && s[dot] != '.') dot++;
		if (dot == 0) return false;

		GLuint major_gl = s[dot-1]-'0';
		GLuint minor_gl = s[dot+1]-'0';

#ifndef ENABLE_GLES
		if ( (major_gl < 3) || ( major_gl == 3 && minor_gl < 2 ) ) {
			fprintf(stderr, "Geometry shaders are not supported. Required openGL 3.2\n");
			found_geometry_shader = false;
		}
		if (nvidia_buggy_driver) {
			fprintf(stderr, "Buggy driver detected. Geometry shaders will be disabled\n");
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
#else
		found_geometry_shader = false;
#endif
		if ( (major_gl < major) || ( major_gl == major && minor_gl < minor ) ) {
			fprintf(stderr, "OPENGL %d.%d is not supported\n", major, minor);
			return false;
		}

        return true;
    }

	bool check_gl_supported_extension() {
		int max_ext = 0;
		glGetIntegerv(GL_NUM_EXTENSIONS, &max_ext);

		if (gl_GetStringi && max_ext) {
			for (GLint i = 0; i < max_ext; i++) {
				string ext((const char*)gl_GetStringi(GL_EXTENSIONS, i));
				if (ext.compare("GL_ARB_separate_shader_objects") == 0) {
					if (!fglrx_buggy_driver) found_GL_ARB_separate_shader_objects = true;
					else fprintf(stderr, "Buggy driver detected, GL_ARB_separate_shader_objects will be disabled\n");
				}
				if (ext.compare("GL_ARB_shading_language_420pack") == 0) found_GL_ARB_shading_language_420pack = true;
				if (ext.compare("GL_ARB_texture_storage") == 0) found_GL_ARB_texture_storage = true;
				if (ext.compare("GL_ARB_copy_image") == 0) found_GL_ARB_copy_image = true;
				if (ext.compare("GL_ARB_gpu_shader5") == 0) found_GL_ARB_gpu_shader5 = true;
				if (ext.compare("GL_ARB_shader_image_load_store") == 0) found_GL_ARB_shader_image_load_store = true;
#if 0
				// Erratum: on nvidia implementation, gain is very nice : 42.5 fps => 46.5 fps
				//
				// Strangely it doesn't provide the speed boost as expected.
				// Note: only atst/colclip was replaced with subroutine for the moment. It replace 2000 program switch on
				// colin mcrae 3 by 2100 uniform, but code is slower!
				//
				// Current hypothesis: the validation of useprogram is done in the "driver thread" whereas the extra function calls
				// are done on the overloaded main threads.
				// Apitrace profiling shows faster GPU draw times

				if (ext.compare("GL_ARB_shader_subroutine") == 0) found_GL_ARB_shader_subroutine = true;
#endif
				if (ext.compare("GL_ARB_explicit_uniform_location") == 0) found_GL_ARB_explicit_uniform_location = true;
#ifdef GL44 // Need to debug the code first
				// Need to check the clean (in particular of depth/stencil texture)
				if (ext.compare("GL_ARB_clear_texture") == 0) found_GL_ARB_clear_texture = true;
				// FIXME unattach context case + perf
				if (ext.compare("GL_ARB_buffer_storage") == 0) found_GL_ARB_buffer_storage = true;
				// OK but no apitrace support
				if (ext.compare("GL_ARB_multi_bind") == 0) found_GL_ARB_multi_bind = true;
#endif
#ifdef GLBINDLESS // Need to debug the code first
				if (ext.compare("GL_ARB_bindless_texture") == 0) found_GL_ARB_bindless_texture = true;
#endif

#ifdef ENABLE_GLES
				fprintf(stderr, "DEBUG ext: %s\n", ext.c_str());
#endif
			}
		}

		bool status = true;
#ifndef ENABLE_GLES
		fprintf(stderr, "\n");

		status &= status_and_override(found_GL_ARB_separate_shader_objects,"GL_ARB_separate_shader_objects");
		status &= status_and_override(found_GL_ARB_gpu_shader5,"GL_ARB_gpu_shader5");
		status &= status_and_override(found_GL_ARB_shader_image_load_store,"GL_ARB_shader_image_load_store");
		status &= status_and_override(found_GL_ARB_clear_texture,"GL_ARB_clear_texture");
		status &= status_and_override(found_GL_ARB_buffer_storage,"GL_ARB_buffer_storage");
		status &= status_and_override(found_GL_ARB_shader_subroutine,"GL_ARB_shader_subroutine");
		status &= status_and_override(found_GL_ARB_explicit_uniform_location,"GL_ARB_explicit_uniform_location");

		status &= status_and_override(found_GL_ARB_texture_storage, "GL_ARB_texture_storage", true);
		status &= status_and_override(found_GL_ARB_shading_language_420pack,"GL_ARB_shading_language_420pack");
		status &= status_and_override(found_GL_ARB_multi_bind,"GL_ARB_multi_bind");
		status &= status_and_override(found_GL_ARB_bindless_texture,"GL_ARB_bindless_texture");

		fprintf(stderr, "\n");
#endif

		return status;
	}
}
