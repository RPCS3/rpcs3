/*
 *	Copyright (C) 2007-2009 Gabest
 *	http://www.gabest.org
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
#include "GSWnd.h"

void GSWndGL::PopulateGlFunction()
{
	*(void**)&(gl_ActiveTexture) = GetProcAddress("glActiveTexture");
	*(void**)&(gl_BlendColor) = GetProcAddress("glBlendColor");
	*(void**)&(gl_AttachShader) = GetProcAddress("glAttachShader");
	*(void**)&(gl_BindBuffer) = GetProcAddress("glBindBuffer");
	*(void**)&(gl_BindBufferBase) = GetProcAddress("glBindBufferBase");
	*(void**)&(gl_BindFragDataLocationIndexed) = GetProcAddress("glBindFragDataLocationIndexed");
	*(void**)&(gl_BindFramebuffer) = GetProcAddress("glBindFramebuffer");
	*(void**)&(gl_BindSampler) = GetProcAddress("glBindSampler");
	*(void**)&(gl_BindVertexArray) = GetProcAddress("glBindVertexArray");
	*(void**)&(gl_BlendEquationSeparate) = GetProcAddress("glBlendEquationSeparate");
	*(void**)&(gl_BlendFuncSeparate) = GetProcAddress("glBlendFuncSeparate");
	*(void**)&(gl_BlitFramebuffer) = GetProcAddress("glBlitFramebuffer");
	*(void**)&(gl_BufferData) = GetProcAddress("glBufferData");
	*(void**)&(gl_CheckFramebufferStatus) = GetProcAddress("glCheckFramebufferStatus");
	*(void**)&(gl_ClearBufferfv) = GetProcAddress("glClearBufferfv");
	*(void**)&(gl_ClearBufferiv) = GetProcAddress("glClearBufferiv");
	*(void**)&(gl_CompileShader) = GetProcAddress("glCompileShader");
	*(void**)&(gl_CopyImageSubDataNV) = GetProcAddress("glCopyImageSubDataNV");
	*(void**)&(gl_CreateProgram) = GetProcAddress("glCreateProgram");
	*(void**)&(gl_CreateShader) = GetProcAddress("glCreateShader");
	*(void**)&(gl_CreateShaderProgramv) = GetProcAddress("glCreateShaderProgramv");
	*(void**)&(gl_DeleteBuffers) = GetProcAddress("glDeleteBuffers");
	*(void**)&(gl_DeleteFramebuffers) = GetProcAddress("glDeleteFramebuffers");
	*(void**)&(gl_DeleteProgram) = GetProcAddress("glDeleteProgram");
	*(void**)&(gl_DeleteSamplers) = GetProcAddress("glDeleteSamplers");
	*(void**)&(gl_DeleteShader) = GetProcAddress("glDeleteShader");
	*(void**)&(gl_DeleteVertexArrays) = GetProcAddress("glDeleteVertexArrays");
	*(void**)&(gl_DetachShader) = GetProcAddress("glDetachShader");
	*(void**)&(gl_DrawBuffers) = GetProcAddress("glDrawBuffers");
	*(void**)&(gl_DrawElementsBaseVertex) = GetProcAddress("glDrawElementsBaseVertex");
	*(void**)&(gl_EnableVertexAttribArray) = GetProcAddress("glEnableVertexAttribArray");
	*(void**)&(gl_FramebufferRenderbuffer) = GetProcAddress("glFramebufferRenderbuffer");
	*(void**)&(gl_FramebufferTexture2D) = GetProcAddress("glFramebufferTexture2D");
	*(void**)&(gl_GenBuffers) = GetProcAddress("glGenBuffers");
	*(void**)&(gl_GenFramebuffers) = GetProcAddress("glGenFramebuffers");
	*(void**)&(gl_GenSamplers) = GetProcAddress("glGenSamplers");
	*(void**)&(gl_GenVertexArrays) = GetProcAddress("glGenVertexArrays");
	*(void**)&(gl_GetBufferParameteriv) = GetProcAddress("glGetBufferParameteriv");
	*(void**)&(gl_GetDebugMessageLogARB) = GetProcAddress("glGetDebugMessageLogARB");
	*(void**)&(gl_GetFragDataIndex) = GetProcAddress("glGetFragDataIndex");
	*(void**)&(gl_GetFragDataLocation) = GetProcAddress("glGetFragDataLocation");
	*(void**)&(gl_GetProgramInfoLog) = GetProcAddress("glGetProgramInfoLog");
	*(void**)&(gl_GetProgramiv) = GetProcAddress("glGetProgramiv");
	*(void**)&(gl_GetShaderiv) = GetProcAddress("glGetShaderiv");
	*(void**)&(gl_GetStringi) = GetProcAddress("glGetStringi");
	*(void**)&(gl_IsFramebuffer) = GetProcAddress("glIsFramebuffer");
	*(void**)&(gl_LinkProgram) = GetProcAddress("glLinkProgram");
	*(void**)&(gl_MapBuffer) = GetProcAddress("glMapBuffer");
	*(void**)&(gl_MapBufferRange) = GetProcAddress("glMapBufferRange");
	*(void**)&(gl_ProgramParameteri) = GetProcAddress("glProgramParameteri");
	*(void**)&(gl_SamplerParameterf) = GetProcAddress("glSamplerParameterf");
	*(void**)&(gl_SamplerParameteri) = GetProcAddress("glSamplerParameteri");
	*(void**)&(gl_ShaderSource) = GetProcAddress("glShaderSource");
	*(void**)&(gl_Uniform1i) = GetProcAddress("glUniform1i");
	*(void**)&(gl_UnmapBuffer) = GetProcAddress("glUnmapBuffer");
	*(void**)&(gl_UseProgramStages) = GetProcAddress("glUseProgramStages");
	*(void**)&(gl_VertexAttribIPointer) = GetProcAddress("glVertexAttribIPointer");
	*(void**)&(gl_VertexAttribPointer) = GetProcAddress("glVertexAttribPointer");
	*(void**)&(gl_TexStorage2D) = GetProcAddress("glTexStorage2D");
	*(void**)&(gl_BufferSubData) = GetProcAddress("glBufferSubData");
	// GL4.1
	*(void**)&(gl_BindProgramPipeline) = GetProcAddress("glBindProgramPipeline");
	*(void**)&(gl_DeleteProgramPipelines) = GetProcAddress("glDeleteProgramPipelines");
	*(void**)&(gl_GenProgramPipelines) = GetProcAddress("glGenProgramPipelines");
	*(void**)&(gl_GetProgramPipelineiv) = GetProcAddress("glGetProgramPipelineiv");
	*(void**)&(gl_GetProgramPipelineInfoLog) = GetProcAddress("glGetProgramPipelineInfoLog");
	*(void**)&(gl_ValidateProgramPipeline) = GetProcAddress("glValidateProgramPipeline");
	// NO GL4.1
	*(void**)&(gl_UseProgram) = GetProcAddress("glUseProgram");
	*(void**)&(gl_GetShaderInfoLog) = GetProcAddress("glGetShaderInfoLog");
	*(void**)&(gl_ProgramUniform1i) = GetProcAddress("glProgramUniform1i");
	// NO GL4.2
	*(void**)&(gl_GetUniformBlockIndex) = GetProcAddress("glGetUniformBlockIndex");
	*(void**)&(gl_UniformBlockBinding) = GetProcAddress("glUniformBlockBinding");
	*(void**)&(gl_GetUniformLocation) = GetProcAddress("glGetUniformLocation");
}
