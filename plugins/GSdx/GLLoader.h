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

#ifndef ENABLE_GLES
// FIX compilation issue with Mesa 10
// Note it might be possible to do better with the right include 
// in the rigth order but I don't have time
#ifndef APIENTRY
#define APIENTRY
#endif
#ifndef APIENTRYP
#define APIENTRYP APIENTRY *
#endif

// Allow compilation with older mesa

#ifndef GL_ARB_copy_image
#define GL_ARB_copy_image 1
#ifdef GL_GLEXT_PROTOTYPES
GLAPI void APIENTRY glCopyImageSubData (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
#endif /* GL_GLEXT_PROTOTYPES */
typedef void (APIENTRYP PFNGLCOPYIMAGESUBDATAPROC) (GLuint srcName, GLenum srcTarget, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLuint dstName, GLenum dstTarget, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ, GLsizei srcWidth, GLsizei srcHeight, GLsizei srcDepth);
#endif

#ifndef GL_VERSION_4_4
#define GL_VERSION_4_4 1
#define GL_MAX_VERTEX_ATTRIB_STRIDE       0x82E5
#define GL_MAP_PERSISTENT_BIT             0x0040
#define GL_MAP_COHERENT_BIT               0x0080
#define GL_DYNAMIC_STORAGE_BIT            0x0100
#define GL_CLIENT_STORAGE_BIT             0x0200
#define GL_CLIENT_MAPPED_BUFFER_BARRIER_BIT 0x00004000
#define GL_BUFFER_IMMUTABLE_STORAGE       0x821F
#define GL_BUFFER_STORAGE_FLAGS           0x8220
#define GL_CLEAR_TEXTURE                  0x9365
#define GL_LOCATION_COMPONENT             0x934A
#define GL_TRANSFORM_FEEDBACK_BUFFER_INDEX 0x934B
#define GL_TRANSFORM_FEEDBACK_BUFFER_STRIDE 0x934C
#define GL_QUERY_BUFFER                   0x9192
#define GL_QUERY_BUFFER_BARRIER_BIT       0x00008000
#define GL_QUERY_BUFFER_BINDING           0x9193
#define GL_QUERY_RESULT_NO_WAIT           0x9194
#define GL_MIRROR_CLAMP_TO_EDGE           0x8743
typedef void (APIENTRYP PFNGLBUFFERSTORAGEPROC) (GLenum target, GLsizeiptr size, const void *data, GLbitfield flags);
typedef void (APIENTRYP PFNGLCLEARTEXIMAGEPROC) (GLuint texture, GLint level, GLenum format, GLenum type, const void *data);
typedef void (APIENTRYP PFNGLCLEARTEXSUBIMAGEPROC) (GLuint texture, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const void *data);
typedef void (APIENTRYP PFNGLBINDBUFFERSBASEPROC) (GLenum target, GLuint first, GLsizei count, const GLuint *buffers);
typedef void (APIENTRYP PFNGLBINDBUFFERSRANGEPROC) (GLenum target, GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizeiptr *sizes);
typedef void (APIENTRYP PFNGLBINDTEXTURESPROC) (GLuint first, GLsizei count, const GLuint *textures);
typedef void (APIENTRYP PFNGLBINDSAMPLERSPROC) (GLuint first, GLsizei count, const GLuint *samplers);
typedef void (APIENTRYP PFNGLBINDIMAGETEXTURESPROC) (GLuint first, GLsizei count, const GLuint *textures);
typedef void (APIENTRYP PFNGLBINDVERTEXBUFFERSPROC) (GLuint first, GLsizei count, const GLuint *buffers, const GLintptr *offsets, const GLsizei *strides);
#endif /* GL_VERSION_4_4 */

#ifndef GL_ARB_bindless_texture
#define GL_ARB_bindless_texture 1
typedef uint64_t GLuint64EXT;
#define GL_UNSIGNED_INT64_ARB             0x140F
typedef GLuint64 (APIENTRYP PFNGLGETTEXTUREHANDLEARBPROC) (GLuint texture);
typedef GLuint64 (APIENTRYP PFNGLGETTEXTURESAMPLERHANDLEARBPROC) (GLuint texture, GLuint sampler);
typedef void (APIENTRYP PFNGLMAKETEXTUREHANDLERESIDENTARBPROC) (GLuint64 handle);
typedef void (APIENTRYP PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC) (GLuint64 handle);
typedef GLuint64 (APIENTRYP PFNGLGETIMAGEHANDLEARBPROC) (GLuint texture, GLint level, GLboolean layered, GLint layer, GLenum format);
typedef void (APIENTRYP PFNGLMAKEIMAGEHANDLERESIDENTARBPROC) (GLuint64 handle, GLenum access);
typedef void (APIENTRYP PFNGLMAKEIMAGEHANDLENONRESIDENTARBPROC) (GLuint64 handle);
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64ARBPROC) (GLint location, GLuint64 value);
typedef void (APIENTRYP PFNGLUNIFORMHANDLEUI64VARBPROC) (GLint location, GLsizei count, const GLuint64 *value);
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64ARBPROC) (GLuint program, GLint location, GLuint64 value);
typedef void (APIENTRYP PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC) (GLuint program, GLint location, GLsizei count, const GLuint64 *values);
typedef GLboolean (APIENTRYP PFNGLISTEXTUREHANDLERESIDENTARBPROC) (GLuint64 handle);
typedef GLboolean (APIENTRYP PFNGLISIMAGEHANDLERESIDENTARBPROC) (GLuint64 handle);
typedef void (APIENTRYP PFNGLVERTEXATTRIBL1UI64ARBPROC) (GLuint index, GLuint64EXT x);
typedef void (APIENTRYP PFNGLVERTEXATTRIBL1UI64VARBPROC) (GLuint index, const GLuint64EXT *v);
typedef void (APIENTRYP PFNGLGETVERTEXATTRIBLUI64VARBPROC) (GLuint index, GLenum pname, GLuint64EXT *params);
#endif /* GL_ARB_bindless_texture */


#endif

#ifndef ENABLE_GLES
extern   PFNGLACTIVETEXTUREPROC                 gl_ActiveTexture;
extern   PFNGLBLENDCOLORPROC                    gl_BlendColor;
extern   PFNGLATTACHSHADERPROC                  gl_AttachShader;
extern   PFNGLBINDBUFFERPROC                    gl_BindBuffer;
extern   PFNGLBINDBUFFERBASEPROC                gl_BindBufferBase;
extern   PFNGLBINDFRAMEBUFFERPROC               gl_BindFramebuffer;
extern   PFNGLBINDSAMPLERPROC                   gl_BindSampler;
extern   PFNGLBINDVERTEXARRAYPROC               gl_BindVertexArray;
extern   PFNGLBLENDEQUATIONSEPARATEIARBPROC     gl_BlendEquationSeparateiARB;
extern   PFNGLBLENDFUNCSEPARATEIARBPROC         gl_BlendFuncSeparateiARB;
extern   PFNGLBLITFRAMEBUFFERPROC               gl_BlitFramebuffer;
extern   PFNGLBUFFERDATAPROC                    gl_BufferData;
extern   PFNGLCHECKFRAMEBUFFERSTATUSPROC        gl_CheckFramebufferStatus;
extern   PFNGLCLEARBUFFERFVPROC                 gl_ClearBufferfv;
extern   PFNGLCLEARBUFFERIVPROC                 gl_ClearBufferiv;
extern   PFNGLCLEARBUFFERUIVPROC                gl_ClearBufferuiv;
extern   PFNGLCOMPILESHADERPROC                 gl_CompileShader;
extern   PFNGLCOLORMASKIPROC                    gl_ColorMaski;
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
extern   PFNGLBUFFERSUBDATAPROC                 gl_BufferSubData;
extern   PFNGLFENCESYNCPROC                     gl_FenceSync;
extern   PFNGLDELETESYNCPROC                    gl_DeleteSync;
extern   PFNGLCLIENTWAITSYNCPROC                gl_ClientWaitSync;
extern   PFNGLFLUSHMAPPEDBUFFERRANGEPROC        gl_FlushMappedBufferRange;
// GL4.0
extern   PFNGLUNIFORMSUBROUTINESUIVPROC         gl_UniformSubroutinesuiv;
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
// GL4.2
extern   PFNGLBINDIMAGETEXTUREPROC              gl_BindImageTexture;
extern   PFNGLMEMORYBARRIERPROC                 gl_MemoryBarrier;
extern   PFNGLTEXSTORAGE2DPROC                  gl_TexStorage2D;
// GL4.3
extern   PFNGLCOPYIMAGESUBDATAPROC              gl_CopyImageSubData;
// GL4.4
extern   PFNGLCLEARTEXIMAGEPROC                 gl_ClearTexImage;
extern   PFNGLBINDTEXTURESPROC                  gl_BindTextures;
extern   PFNGLBUFFERSTORAGEPROC                 gl_BufferStorage;
// GL_ARB_bindless_texture (GL5?)
extern PFNGLGETTEXTURESAMPLERHANDLEARBPROC      gl_GetTextureSamplerHandleARB;
extern PFNGLMAKETEXTUREHANDLERESIDENTARBPROC    gl_MakeTextureHandleResidentARB;
extern PFNGLMAKETEXTUREHANDLENONRESIDENTARBPROC gl_MakeTextureHandleNonResidentARB;
extern PFNGLUNIFORMHANDLEUI64VARBPROC           gl_UniformHandleui64vARB;
extern PFNGLPROGRAMUNIFORMHANDLEUI64VARBPROC    gl_ProgramUniformHandleui64vARB;

extern PFNGLDEPTHRANGEDNVPROC                   gl_DepthRangedNV;

#else
#define gl_ActiveTexture glActiveTexture
#define gl_BlendColor glBlendColor
#define gl_AttachShader glAttachShader
#define gl_BindBuffer glBindBuffer
#define gl_BindBufferBase glBindBufferBase
#define gl_BindFramebuffer glBindFramebuffer
#define gl_BindSampler glBindSampler
#define gl_BindVertexArray glBindVertexArray
#define gl_BlendEquationSeparate glBlendEquationSeparate
#define gl_BlendFuncSeparate glBlendFuncSeparate
#define gl_BlitFramebuffer glBlitFramebuffer
#define gl_BufferData glBufferData
#define gl_CheckFramebufferStatus glCheckFramebufferStatus
#define gl_ClearBufferfv glClearBufferfv
#define gl_ClearBufferiv glClearBufferiv
#define gl_ClearBufferuiv glClearBufferuiv
#define gl_CompileShader glCompileShader
#define gl_ColorMask glColorMask
#define gl_CreateProgram glCreateProgram
#define gl_CreateShader glCreateShader
#define gl_CreateShaderProgramv glCreateShaderProgramv
#define gl_DeleteBuffers glDeleteBuffers
#define gl_DeleteFramebuffers glDeleteFramebuffers
#define gl_DeleteProgram glDeleteProgram
#define gl_DeleteSamplers glDeleteSamplers
#define gl_DeleteShader glDeleteShader
#define gl_DeleteVertexArrays glDeleteVertexArrays
#define gl_DetachShader glDetachShader
#define gl_DrawBuffers glDrawBuffers
#define gl_DrawElementsBaseVertex glDrawElementsBaseVertex
#define gl_EnableVertexAttribArray glEnableVertexAttribArray
#define gl_FramebufferRenderbuffer glFramebufferRenderbuffer
#define gl_FramebufferTexture2D glFramebufferTexture2D
#define gl_GenBuffers glGenBuffers
#define gl_GenFramebuffers glGenFramebuffers
#define gl_GenSamplers glGenSamplers
#define gl_GenVertexArrays glGenVertexArrays
#define gl_GetBufferParameteriv glGetBufferParameteriv
#define gl_GetDebugMessageLogARB glGetDebugMessageLogARB
#define gl_GetProgramInfoLog glGetProgramInfoLog
#define gl_GetProgramiv glGetProgramiv
#define gl_GetShaderiv glGetShaderiv
#define gl_GetStringi glGetStringi
#define gl_IsFramebuffer glIsFramebuffer
#define gl_LinkProgram glLinkProgram
#define gl_MapBuffer glMapBuffer
#define gl_MapBufferRange glMapBufferRange
#define gl_ProgramParameteri glProgramParameteri
#define gl_SamplerParameterf glSamplerParameterf
#define gl_SamplerParameteri glSamplerParameteri
#define gl_ShaderSource glShaderSource
#define gl_Uniform1i glUniform1i
#define gl_UnmapBuffer glUnmapBuffer
#define gl_UseProgramStages glUseProgramStages
#define gl_VertexAttribIPointer glVertexAttribIPointer
#define gl_VertexAttribPointer glVertexAttribPointer
#define gl_TexStorage2D glTexStorage2D
#define gl_BufferSubData glBufferSubData

#define gl_BindProgramPipeline glBindProgramPipeline
#define gl_DeleteProgramPipelines glDeleteProgramPipelines
#define gl_GenProgramPipelines glGenProgramPipelines
#define gl_GetProgramPipelineiv glGetProgramPipelineiv
#define gl_ValidateProgramPipeline glValidateProgramPipeline
#define gl_GetProgramPipelineInfoLog glGetProgramPipelineInfoLog

#define gl_UseProgram glUseProgram
#define gl_GetShaderInfoLog glGetShaderInfoLog
#define gl_ProgramUniform1i glProgramUniform1i

#define gl_GetUniformBlockIndex glGetUniformBlockIndex
#define gl_UniformBlockBinding glUniformBlockBinding
#define gl_GetUniformLocation glGetUniformLocation
#endif


namespace GLLoader {
	bool check_gl_version(uint32 major, uint32 minor);
	void init_gl_function();
	bool check_gl_supported_extension();

	extern bool fglrx_buggy_driver;
	extern bool mesa_amd_buggy_driver;
	extern bool nvidia_buggy_driver;
	extern bool intel_buggy_driver;
	extern bool in_replayer;

	extern bool found_GL_ARB_separate_shader_objects;
	extern bool found_GL_ARB_shading_language_420pack;
	extern bool found_GL_ARB_copy_image;
	extern bool found_geometry_shader;
	extern bool found_only_gl30;
	extern bool found_GL_ARB_gpu_shader5;
	extern bool found_GL_ARB_shader_image_load_store;
	extern bool found_GL_ARB_clear_texture;
	extern bool found_GL_ARB_multi_bind;
	extern bool found_GL_ARB_buffer_storage;
	extern bool found_GL_ARB_shader_subroutine;
	extern bool found_GL_ARB_bindless_texture;
	extern bool found_GL_ARB_explicit_uniform_location;
	extern bool found_GL_NV_depth_buffer_float;
}
