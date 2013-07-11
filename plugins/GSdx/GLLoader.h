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

extern   PFNGLACTIVETEXTUREPROC                 gl_ActiveTexture;
extern   PFNGLBLENDCOLORPROC                    gl_BlendColor;
extern   PFNGLATTACHSHADERPROC                  gl_AttachShader;
extern   PFNGLBINDBUFFERPROC                    gl_BindBuffer;
extern   PFNGLBINDBUFFERBASEPROC                gl_BindBufferBase;
extern   PFNGLBINDFRAGDATALOCATIONINDEXEDPROC   gl_BindFragDataLocationIndexed;
extern   PFNGLBINDFRAMEBUFFERPROC               gl_BindFramebuffer;
extern   PFNGLBINDSAMPLERPROC                   gl_BindSampler;
extern   PFNGLBINDVERTEXARRAYPROC               gl_BindVertexArray;
extern   PFNGLBLENDEQUATIONSEPARATEPROC         gl_BlendEquationSeparate;
extern   PFNGLBLENDFUNCSEPARATEPROC             gl_BlendFuncSeparate;
extern   PFNGLBLITFRAMEBUFFERPROC               gl_BlitFramebuffer;
extern   PFNGLBUFFERDATAPROC                    gl_BufferData;
extern   PFNGLCHECKFRAMEBUFFERSTATUSPROC        gl_CheckFramebufferStatus;
extern   PFNGLCLEARBUFFERFVPROC                 gl_ClearBufferfv;
extern   PFNGLCLEARBUFFERIVPROC                 gl_ClearBufferiv;
extern   PFNGLCOMPILESHADERPROC                 gl_CompileShader;
extern   PFNGLCOPYIMAGESUBDATANVPROC            gl_CopyImageSubDataNV;
extern   PFNGLCREATEPROGRAMPROC                 gl_CreateProgram;
extern   PFNGLCREATESHADERPROC                  gl_CreateShader;
extern   PFNGLCREATESHADERPROGRAMVPROC          gl_CreateShaderProgramv;
extern   PFNGLDELETEBUFFERSPROC                 gl_DeleteBuffers;
extern   PFNGLDELETEFRAMEBUFFERSPROC            gl_DeleteFramebuffers;
extern   PFNGLDELETEPROGRAMPROC                 gl_DeleteProgram;
extern   PFNGLDELETESAMPLERSPROC                gl_DeleteSamplers;
extern   PFNGLDELETESHADERPROC                  gl_DeleteShader;
extern   PFNGLDELETEVERTEXARRAYSPROC            gl_DeleteVertexArrays;
extern   PFNGLDETACHSHADERPROC                  gl_DetachShader;
extern   PFNGLDRAWBUFFERSPROC                   gl_DrawBuffers;
extern   PFNGLDRAWELEMENTSBASEVERTEXPROC        gl_DrawElementsBaseVertex;
extern   PFNGLENABLEVERTEXATTRIBARRAYPROC       gl_EnableVertexAttribArray;
extern   PFNGLFRAMEBUFFERRENDERBUFFERPROC       gl_FramebufferRenderbuffer;
extern   PFNGLFRAMEBUFFERTEXTURE2DPROC          gl_FramebufferTexture2D;
extern   PFNGLGENBUFFERSPROC                    gl_GenBuffers;
extern   PFNGLGENFRAMEBUFFERSPROC               gl_GenFramebuffers;
extern   PFNGLGENSAMPLERSPROC                   gl_GenSamplers;
extern   PFNGLGENVERTEXARRAYSPROC               gl_GenVertexArrays;
extern   PFNGLGETBUFFERPARAMETERIVPROC          gl_GetBufferParameteriv;
extern   PFNGLGETDEBUGMESSAGELOGARBPROC         gl_GetDebugMessageLogARB;
extern   PFNGLGETFRAGDATAINDEXPROC              gl_GetFragDataIndex;
extern   PFNGLGETFRAGDATALOCATIONPROC           gl_GetFragDataLocation;
extern   PFNGLGETPROGRAMINFOLOGPROC             gl_GetProgramInfoLog;
extern   PFNGLGETPROGRAMIVPROC                  gl_GetProgramiv;
extern   PFNGLGETSHADERIVPROC                   gl_GetShaderiv;
extern   PFNGLGETSTRINGIPROC                    gl_GetStringi;
extern   PFNGLISFRAMEBUFFERPROC                 gl_IsFramebuffer;
extern   PFNGLLINKPROGRAMPROC                   gl_LinkProgram;
extern   PFNGLMAPBUFFERPROC                     gl_MapBuffer;
extern   PFNGLMAPBUFFERRANGEPROC                gl_MapBufferRange;
extern   PFNGLPROGRAMPARAMETERIPROC             gl_ProgramParameteri;
extern   PFNGLSAMPLERPARAMETERFPROC             gl_SamplerParameterf;
extern   PFNGLSAMPLERPARAMETERIPROC             gl_SamplerParameteri;
extern   PFNGLSHADERSOURCEPROC                  gl_ShaderSource;
extern   PFNGLUNIFORM1IPROC                     gl_Uniform1i;
extern   PFNGLUNMAPBUFFERPROC                   gl_UnmapBuffer;
extern   PFNGLUSEPROGRAMSTAGESPROC              gl_UseProgramStages;
extern   PFNGLVERTEXATTRIBIPOINTERPROC          gl_VertexAttribIPointer;
extern   PFNGLVERTEXATTRIBPOINTERPROC           gl_VertexAttribPointer;
extern   PFNGLTEXSTORAGE2DPROC                  gl_TexStorage2D;
extern   PFNGLBUFFERSUBDATAPROC                 gl_BufferSubData;
// GL4.1
extern   PFNGLBINDPROGRAMPIPELINEPROC           gl_BindProgramPipeline;
extern   PFNGLDELETEPROGRAMPIPELINESPROC        gl_DeleteProgramPipelines;
extern   PFNGLGENPROGRAMPIPELINESPROC           gl_GenProgramPipelines;
extern   PFNGLGETPROGRAMPIPELINEIVPROC          gl_GetProgramPipelineiv;
extern   PFNGLVALIDATEPROGRAMPIPELINEPROC       gl_ValidateProgramPipeline;
extern   PFNGLGETPROGRAMPIPELINEINFOLOGPROC     gl_GetProgramPipelineInfoLog;
// NO GL4.1
extern   PFNGLUSEPROGRAMPROC                    gl_UseProgram;
extern   PFNGLGETSHADERINFOLOGPROC              gl_GetShaderInfoLog;
extern   PFNGLPROGRAMUNIFORM1IPROC              gl_ProgramUniform1i;
// NO GL4.2
extern   PFNGLGETUNIFORMBLOCKINDEXPROC          gl_GetUniformBlockIndex;
extern   PFNGLUNIFORMBLOCKBINDINGPROC           gl_UniformBlockBinding;
extern   PFNGLGETUNIFORMLOCATIONPROC            gl_GetUniformLocation;


namespace GLLoader {
	bool check_gl_version(uint32 major, uint32 minor);
	void init_gl_function();
	bool check_gl_supported_extension();

	extern bool fglrx_buggy_driver;
	extern bool nvidia_buggy_driver;

	extern bool found_GL_ARB_separate_shader_objects;
	extern bool found_GL_ARB_shading_language_420pack;
	extern bool found_GL_ARB_texture_storage;
	extern bool found_GL_ARB_copy_image;
	extern bool found_GL_NV_copy_image;
	extern bool found_geometry_shader;
	extern bool found_only_gl30;
	extern bool found_GL_ARB_gpu_shader5;
}
