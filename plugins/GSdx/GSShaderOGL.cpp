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
#include "GSShaderOGL.h"
#include "GLState.h"

GSShaderOGL::GSShaderOGL(bool debug) :
	m_debug_shader(debug),
	m_vs_sub_count(0),
	m_ps_sub_count(0)
{

	memset(&m_vs_sub, 0, countof(m_vs_sub)*sizeof(m_vs_sub[0]));
	memset(&m_ps_sub, 0, countof(m_ps_sub)*sizeof(m_ps_sub[0]));

	m_single_prog.clear();
#ifndef ENABLE_GLES
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		gl_GenProgramPipelines(1, &m_pipeline);
		gl_BindProgramPipeline(m_pipeline);
	}
#endif
}

GSShaderOGL::~GSShaderOGL()
{
#ifndef ENABLE_GLES
	if (GLLoader::found_GL_ARB_separate_shader_objects)
		gl_DeleteProgramPipelines(1, &m_pipeline);
#endif

	for (auto it = m_single_prog.begin(); it != m_single_prog.end() ; it++) gl_DeleteProgram(it->second);
	m_single_prog.clear();
}

void GSShaderOGL::VS(GLuint s, GLuint sub_count)
{
	if (GLState::vs != s)
	{
		m_vs_sub_count = sub_count;

		GLState::vs = s;
		GLState::dirty_prog = true;
		GLState::dirty_subroutine_vs = true;
#ifndef ENABLE_GLES
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_UseProgramStages(m_pipeline, GL_VERTEX_SHADER_BIT, s);
#endif
	}
}

void GSShaderOGL::VS_subroutine(GLuint *sub)
{
	if (!(m_vs_sub[0] == sub[0])) {
		m_vs_sub[0] = sub[0];
		GLState::dirty_subroutine_vs = true;
	}
}

void GSShaderOGL::PS_subroutine(GLuint *sub)
{
	// FIXME could be more efficient with GSvector
	if (!(m_ps_sub[0] == sub[0] && m_ps_sub[1] == sub[1] && m_ps_sub[2] == sub[2] && m_ps_sub[3] == sub[3] && m_ps_sub[4] == sub[4])) {
		m_ps_sub[0] = sub[0];
		m_ps_sub[1] = sub[1];
		m_ps_sub[2] = sub[2];
		m_ps_sub[3] = sub[3];
		m_ps_sub[4] = sub[4];
		GLState::dirty_subroutine_ps = true;
	}
}

void GSShaderOGL::PS_ressources(GLuint64 handle[2])
{
	if (handle[0] != GLState::tex_handle[0] || handle[1] != GLState::tex_handle[1]) {
		GLState::tex_handle[0] = handle[0];
		GLState::tex_handle[1] = handle[1];
		GLState::dirty_ressources = true;
	}
}

void GSShaderOGL::PS(GLuint s, GLuint sub_count)
{
	if (GLState::ps != s)
	{
		m_ps_sub_count = sub_count;

		GLState::ps = s;
		GLState::dirty_prog = true;
		GLState::dirty_subroutine_ps = true;
#ifndef ENABLE_GLES
		if (GLLoader::found_GL_ARB_separate_shader_objects) {
			gl_UseProgramStages(m_pipeline, GL_FRAGMENT_SHADER_BIT, s);
		}
#endif
	}
}

void GSShaderOGL::GS(GLuint s)
{
	if (GLState::gs != s)
	{
		GLState::gs = s;
		GLState::dirty_prog = true;
#ifndef ENABLE_GLES
		if (GLLoader::found_GL_ARB_separate_shader_objects)
			gl_UseProgramStages(m_pipeline, GL_GEOMETRY_SHADER_BIT, s);
#endif
	}
}

void GSShaderOGL::SetUniformBinding(GLuint prog, GLchar* name, GLuint binding)
{
	GLuint index;
	index = gl_GetUniformBlockIndex(prog, name);
	if (index != GL_INVALID_INDEX) {
		gl_UniformBlockBinding(prog, index, binding);
	}
}

void GSShaderOGL::SetSamplerBinding(GLuint prog, GLchar* name, GLuint binding)
{
	GLint loc = gl_GetUniformLocation(prog, name);
	if (loc != -1) {
		if (GLLoader::found_GL_ARB_separate_shader_objects) {
#ifndef ENABLE_GLES
			gl_ProgramUniform1i(prog, loc, binding);
#endif
		} else {
			gl_Uniform1i(loc, binding);
		}
	}
}

void GSShaderOGL::SetupRessources()
{
	if (!GLLoader::found_GL_ARB_bindless_texture) return;

	if (GLState::dirty_ressources) {
		GLState::dirty_ressources = false;
		// FIXME count !
		// FIXME location !
		GLuint count = (GLState::tex_handle[1]) ? 2 : 1;
		if (GLLoader::found_GL_ARB_separate_shader_objects) {
			gl_ProgramUniformHandleui64vARB(GLState::ps, 0, count, GLState::tex_handle);
		} else {
			gl_UniformHandleui64vARB(0, count, GLState::tex_handle);
		}
	}
}

void GSShaderOGL::SetupUniform()
{
	if (GLLoader::found_GL_ARB_shading_language_420pack) return;

	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		SetUniformBinding(GLState::vs, "cb20", 20);
		SetUniformBinding(GLState::ps, "cb21", 21);

		SetUniformBinding(GLState::ps, "cb10", 10);
		SetUniformBinding(GLState::ps, "cb11", 11);
		SetUniformBinding(GLState::ps, "cb12", 12);
		SetUniformBinding(GLState::ps, "cb13", 13);

		SetSamplerBinding(GLState::ps, "TextureSampler", 0);
		SetSamplerBinding(GLState::ps, "PaletteSampler", 1);
		//SetSamplerBinding(GLState::ps, "RTCopySampler", 2);
	} else {
		SetUniformBinding(GLState::program, "cb20", 20);
		SetUniformBinding(GLState::program, "cb21", 21);

		SetUniformBinding(GLState::program, "cb10", 10);
		SetUniformBinding(GLState::program, "cb11", 11);
		SetUniformBinding(GLState::program, "cb12", 12);
		SetUniformBinding(GLState::program, "cb13", 13);

		SetSamplerBinding(GLState::program, "TextureSampler", 0);
		SetSamplerBinding(GLState::program, "PaletteSampler", 1);
		//SetSamplerBinding(GLState::program, "RTCopySampler", 2);
	}
}

void GSShaderOGL::SetupSubroutineUniform()
{
	if (!GLLoader::found_GL_ARB_shader_subroutine) return;

	if (GLState::dirty_subroutine_vs && m_vs_sub_count) {
		gl_UniformSubroutinesuiv(GL_VERTEX_SHADER, m_vs_sub_count,  m_vs_sub);
		GLState::dirty_subroutine_vs = false;
	}

	if (GLState::dirty_subroutine_ps && m_ps_sub_count) {
		gl_UniformSubroutinesuiv(GL_FRAGMENT_SHADER, m_ps_sub_count,  m_ps_sub);
		GLState::dirty_subroutine_ps = false;
	}
}

bool GSShaderOGL::ValidateShader(GLuint s)
{
	if (!m_debug_shader) return true;

	GLint status;
	gl_GetShaderiv(s, GL_COMPILE_STATUS, &status);
	if (status) return true;

	GLint log_length = 0;
	gl_GetShaderiv(s, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		char* log = new char[log_length];
		gl_GetShaderInfoLog(s, log_length, NULL, log);
		fprintf(stderr, "%s", log);
		delete[] log;
	}
	fprintf(stderr, "\n");

	return false;
}

bool GSShaderOGL::ValidateProgram(GLuint p)
{
	if (!m_debug_shader) return true;

	GLint status;
	gl_GetProgramiv(p, GL_LINK_STATUS, &status);
	if (status) return true;

	GLint log_length = 0;
	gl_GetProgramiv(p, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		char* log = new char[log_length];
		gl_GetProgramInfoLog(p, log_length, NULL, log);
		fprintf(stderr, "%s", log);
		delete[] log;
	}
	fprintf(stderr, "\n");

	return false;
}

bool GSShaderOGL::ValidatePipeline(GLuint p)
{
#ifndef ENABLE_GLES
	if (!m_debug_shader) return true;

	// FIXME: might be mandatory to validate the pipeline
	gl_ValidateProgramPipeline(p);

	GLint status;
	gl_GetProgramPipelineiv(p, GL_VALIDATE_STATUS, &status);
	if (status) return true;

	GLint log_length = 0;
	gl_GetProgramPipelineiv(p, GL_INFO_LOG_LENGTH, &log_length);
	if (log_length > 0) {
		char* log = new char[log_length];
		gl_GetProgramPipelineInfoLog(p, log_length, NULL, log);
		fprintf(stderr, "%s", log);
		delete[] log;
	}
	fprintf(stderr, "\n");

#endif

	return false;
}

GLuint GSShaderOGL::LinkNewProgram()
{
	GLuint p = gl_CreateProgram();
	if (GLState::vs) gl_AttachShader(p, GLState::vs);
	if (GLState::ps) gl_AttachShader(p, GLState::ps);
	if (GLState::gs) gl_AttachShader(p, GLState::gs);

	gl_LinkProgram(p);

	ValidateProgram(p);

	return p;
}

void GSShaderOGL::UseProgram()
{
	if (GLState::dirty_prog) {
		if (!GLLoader::found_GL_ARB_separate_shader_objects) {
			GLState::dirty_subroutine_vs = true;
			GLState::dirty_subroutine_ps = true;
			GLState::dirty_ressources = true;

			hash_map<uint64, GLuint >::iterator it;
			// Note: shader are integer lookup pointer. They start from 1 and incr
			// every time you create a new shader OR a new program.
			// Note2: vs & gs are precompiled at startup. FGLRX and radeon got value < 128.
			// We migth be able to pack the value in a 32bits int
			// I would need to check the behavior on Nvidia (pause/resume).
			uint64 sel = (uint64)GLState::vs << 40 | (uint64)GLState::gs << 20 | GLState::ps;
			it = m_single_prog.find(sel);
			if (it == m_single_prog.end()) {
				GLState::program = LinkNewProgram();
				m_single_prog[sel] = GLState::program;

				ValidateProgram(GLState::program);

				gl_UseProgram(GLState::program);

				// warning it must be done after the "setup" of the program
				SetupUniform();
			} else {
				GLuint prog = it->second;
				if (prog != GLState::program) {
					GLState::program = prog;
					gl_UseProgram(GLState::program);
				}
			}

		} else {
			ValidatePipeline(m_pipeline);

			SetupUniform();
		}
	}

	SetupRessources();

	SetupSubroutineUniform();

	GLState::dirty_prog = false;
}

std::string GSShaderOGL::GenGlslHeader(const std::string& entry, GLenum type, const std::string& macro)
{
	std::string header;
#ifndef ENABLE_GLES

	if (GLLoader::found_only_gl30) {
		header = "#version 130\n";
	} else {
		header = "#version 330 core\n";
	}
	if (GLLoader::found_GL_ARB_shading_language_420pack) {
		// Need GL version 420
		header += "#extension GL_ARB_shading_language_420pack: require\n";
	} else {
		header += "#define DISABLE_GL42\n";
	}
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		// Need GL version 410
		header += "#extension GL_ARB_separate_shader_objects: require\n";
	} else {
		header += "#define DISABLE_SSO\n";
	}
	if (GLLoader::found_only_gl30) {
		// Need version 330
		header += "#extension GL_ARB_explicit_attrib_location: require\n";
		// Need version 140
		header += "#extension GL_ARB_uniform_buffer_object: require\n";
	}
	if (GLLoader::found_GL_ARB_shader_subroutine && GLLoader::found_GL_ARB_explicit_uniform_location) {
		// Need GL version 400
		header += "#define SUBROUTINE_GL40 1\n";
		header += "#extension GL_ARB_shader_subroutine: require\n";
	}
	if (GLLoader::found_GL_ARB_explicit_uniform_location) {
		// Need GL version 430
		header += "#extension GL_ARB_explicit_uniform_location: require\n";
	}
#ifdef ENABLE_OGL_STENCIL_DEBUG
	header += "#define ENABLE_OGL_STENCIL_DEBUG 1\n";
#endif
	if (GLLoader::found_GL_ARB_shader_image_load_store) {
		// Need GL version 420
		header += "#extension GL_ARB_shader_image_load_store: require\n";
	} else {
		header += "#define DISABLE_GL42_image\n";
	}
	if (GLLoader::found_GL_ARB_bindless_texture && GLLoader::found_GL_ARB_explicit_uniform_location) {
		// Future opengl 5?
		header += "#extension GL_ARB_bindless_texture: require\n";
		header += "#define ENABLE_BINDLESS_TEX\n";
	}


#else
	header = "#version 300 es\n";
	// Disable full GL features
	header += "#define DISABLE_SSO\n";
	header += "#define DISABLE_GL42\n";
	header += "#define DISABLE_GL42_image\n";
#endif

	// Stupid GL implementation (can't use GL_ES)
	// AMD/nvidia define it to 0
	// intel window don't define it
	// intel linux refuse to define it
#ifdef ENABLE_GLES
	header += "#define pGL_ES 1\n";
#else
	header += "#define pGL_ES 0\n";
#endif

	// Allow to puts several shader in 1 files
	switch (type) {
		case GL_VERTEX_SHADER:
			header += "#define VERTEX_SHADER 1\n";
			break;
#ifndef ENABLE_GLES
		case GL_GEOMETRY_SHADER:
			header += "#define GEOMETRY_SHADER 1\n";
			break;
#endif
		case GL_FRAGMENT_SHADER:
			header += "#define FRAGMENT_SHADER 1\n";
			break;
		default: ASSERT(0);
	}

	// Select the entry point ie the main function
	header += format("#define %s main\n", entry.c_str());

	header += macro;

	return header;
}

GLuint GSShaderOGL::Compile(const std::string& glsl_file, const std::string& entry, GLenum type, const char* glsl_h_code, const std::string& macro_sel)
{
	ASSERT(glsl_h_code != NULL);

	GLuint program = 0;

#ifndef ENABLE_GLES
	if (type == GL_GEOMETRY_SHADER && !GLLoader::found_geometry_shader) {
		return program;
	}
#endif

	// Note it is better to separate header and source file to have the good line number
	// in the glsl compiler report
	const char* sources[2];

	std::string header = GenGlslHeader(entry, type, macro_sel);
	int shader_nb = 1;
#if 1
	sources[0] = header.c_str();
	sources[1] = glsl_h_code;
	shader_nb++;
#else
	sources[0] = header.append(glsl_h_code).c_str();
#endif

	if (GLLoader::found_GL_ARB_separate_shader_objects) {
#ifndef ENABLE_GLES
		program = gl_CreateShaderProgramv(type, shader_nb, sources);
#endif
	} else {
		program = gl_CreateShader(type);
		gl_ShaderSource(program, shader_nb, sources, NULL);
		gl_CompileShader(program);
	}

	bool status;
	if (GLLoader::found_GL_ARB_separate_shader_objects)
		status = ValidateProgram(program);
	else
		status = ValidateShader(program);

	if (!status) {
		// print extra info
		fprintf(stderr, "%s (entry %s, prog %d) :", glsl_file.c_str(), entry.c_str(), program);
		fprintf(stderr, "\n%s", macro_sel.c_str());
		fprintf(stderr, "\n");
	}
	return program;
}

void GSShaderOGL::Delete(GLuint s)
{
	if (GLLoader::found_GL_ARB_separate_shader_objects) {
		gl_DeleteProgram(s);
	} else {
		gl_DeleteShader(s);
	}
}
