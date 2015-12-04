#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/System.h"

#include "GLVertexProgram.h"
#include "GLCommonDecompiler.h"

std::string GLVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImpl(elementCount);
}

std::string GLVertexDecompilerThread::getIntTypeName(size_t elementCount)
{
	return "ivec4";
}


std::string GLVertexDecompilerThread::getFunction(FUNCTION f)
{
	return getFunctionImpl(f);
}

std::string GLVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImpl(f, Op0, Op1);
}

void GLVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 420" << std::endl << std::endl;
	OS << "layout(std140, binding = 0) uniform ScaleOffsetBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	mat4 scaleOffsetMat;" << std::endl;
	OS << "};" << std::endl;
}

void GLVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	for (const ParamType PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
			OS << /*"layout(location = " << PI.location << ") "*/  "in " << PT.type << " " << PI.name << ";" << std::endl;
	}
}

void GLVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "layout(std140, binding = 1) uniform VertexConstantsBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	vec4 vc[468];" << std::endl;
	OS << "};" << std::endl;
}

struct reg_info
{
	std::string name;
	bool need_declare;
	std::string src_reg;
	std::string src_reg_mask;
	bool need_cast;
};

static const reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fogc", true, "dst_reg5", ".x", true },
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void GLVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg) && i.need_declare)
		{
			if (i.name == "fogc")
				OS << "out float " << i.name << ";" << std::endl;
			else
				OS << "out vec4 " << i.name << ";" << std::endl;
		}
	}
}


void GLVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	OS << "void main()" << std::endl;
	OS << "{" << std::endl;

	// Declare inside main function
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;
			OS << ";" << std::endl;
		}
	}
}

void GLVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
			OS << "	" << i.name << " = " << i.src_reg << i.src_reg_mask << ";" << std::endl;
	}
	OS << "	gl_Position = gl_Position * scaleOffsetMat;" << std::endl;
	OS << "}" << std::endl;
}


void GLVertexDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLVertexProgram::GLVertexProgram()
{
}

GLVertexProgram::~GLVertexProgram()
{
	//if (m_decompiler_thread)
	//{
	//	Wait();
	//	if (m_decompiler_thread->IsAlive())
	//	{
	//		m_decompiler_thread->Stop();
	//	}

	//	delete m_decompiler_thread;
	//	m_decompiler_thread = nullptr;
	//}

	Delete();
}

//void GLVertexProgram::Wait()
//{
//	if (m_decompiler_thread && m_decompiler_thread->IsAlive())
//	{
//		m_decompiler_thread->Join();
//	}
//}

void GLVertexProgram::Decompile(RSXVertexProgram& prog)
{
	GLVertexDecompilerThread decompiler(prog.data, shader, parr);
	decompiler.Task();
}

//void GLVertexProgram::DecompileAsync(RSXVertexProgram& prog)
//{
//	if (m_decompiler_thread)
//	{
//		Wait();
//		if (m_decompiler_thread->IsAlive())
//		{
//			m_decompiler_thread->Stop();
//		}
//
//		delete m_decompiler_thread;
//		m_decompiler_thread = nullptr;
//	}
//
//	m_decompiler_thread = new GLVertexDecompilerThread(prog.data, shader, parr);
//	m_decompiler_thread->Start();
//}

void GLVertexProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_VERTEX_SHADER);

	const char* str = shader.c_str();
	const int strlen = shader.length();

	glShaderSource(id, 1, &str, &strlen);
	glCompileShader(id);

	GLint r = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &r);
	if (r != GL_TRUE)
	{
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &r);

		if (r)
		{
			char* buf = new char[r + 1]();
			GLsizei len;
			glGetShaderInfoLog(id, r, &len, buf);
			LOG_ERROR(RSX, "Failed to compile vertex shader: %s", buf);
			delete[] buf;
		}

		LOG_NOTICE(RSX, "%s", shader.c_str());
		Emu.Pause();
	}
	//else LOG_WARNING(RSX, "Vertex shader compiled successfully!");

}

void GLVertexProgram::Delete()
{
	shader.clear();

	if (id)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLVertexProgram::Delete(): glDeleteShader(%d) avoided", id);
		}
		else
		{
			glDeleteShader(id);
		}
		id = 0;
	}
}
