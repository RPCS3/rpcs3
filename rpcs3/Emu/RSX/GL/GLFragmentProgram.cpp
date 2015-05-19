#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLFragmentProgram.h"

std::string GLFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	switch (elementCount)
	{
	default:
		abort();
	case 1:
		return "float";
	case 2:
		return "vec2";
	case 3:
		return "vec3";
	case 4:
		return "vec4";
	}
}

std::string GLFragmentDecompilerThread::getFunction(FUNCTION f)
{
	switch (f)
	{
	default:
		abort();
	case FUNCTION::FUNCTION_DP2:
		return "vec4(dot($0.xy, $1.xy))";
	case FUNCTION::FUNCTION_DP2A:
		return "";
	case FUNCTION::FUNCTION_DP3:
		return "vec4(dot($0.xyz, $1.xyz))";
	case FUNCTION::FUNCTION_DP4:
		return "vec4(dot($0, $1))";
	case FUNCTION::FUNCTION_SFL:
		return "vec4(0., 0., 0., 0.)";
	case FUNCTION::FUNCTION_STR:
		return "vec4(1., 1., 1., 1.)";
	case FUNCTION::FUNCTION_FRACT:
		return "fract($0)";
	case FUNCTION::FUNCTION_TEXTURE_SAMPLE:
		return "texture($t, $0.xy)";
	case FUNCTION::FUNCTION_DFDX:
		return "dFdx($0)";
	case FUNCTION::FUNCTION_DFDY:
		return "dFdy($0)";
	}
}

std::string GLFragmentDecompilerThread::saturate(const std::string & code)
{
	return "clamp(" + code + ", 0., 1.)";
}

std::string GLFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	switch (f)
	{
	case COMPARE::FUNCTION_SEQ:
		return "equal(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SGE:
		return "greaterThanEqual(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SGT:
		return "greaterThan(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SLE:
		return "lessThanEqual(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SLT:
		return "lessThan(" + Op0 + ", " + Op1 + ")";
	case COMPARE::FUNCTION_SNE:
		return "notEqual(" + Op0 + ", " + Op1 + ")";
	}
}


void GLFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	OS << "#version 420" << std::endl;
}

void GLFragmentDecompilerThread::insertIntputs(std::stringstream & OS)
{
	for (ParamType PT : m_parr.params[PF_PARAM_IN])
	{
		for (ParamItem PI : PT.items)
			OS << "in " << PT.type << " " << PI.name << ";" << std::endl;
	}
}

void GLFragmentDecompilerThread::insertOutputs(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & 0x40 ? "r0" : "h0" },
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h4" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h6" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
			OS << "out vec4 " << table[i].first << ";" << std::endl;
	}
}

void GLFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (ParamItem PI : PT.items)
			OS << "uniform " << PT.type << " " << PI.name << ";" << std::endl;

	}

	for (ParamType PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D")
			continue;
		for (ParamItem PI : PT.items)
			OS << "uniform " << PT.type << " " << PI.name << ";" << std::endl;
	}
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	OS << "void main ()" << std::endl;
	OS << "{" << std::endl;

	for (ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (ParamItem PI : PT.items)
		{
			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;
			OS << ";" << std::endl;
		}
	}
}

void GLFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & 0x40 ? "r0" : "h0" },
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h4" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h6" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
			OS << "	" << table[i].first << " = " << table[i].second << ";" << std::endl;
	}

	OS << "};" << std::endl;
}

void GLFragmentDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLFragmentProgram::GLFragmentProgram()
	: m_decompiler_thread(nullptr)
	, id(0)
{
}

GLFragmentProgram::~GLFragmentProgram()
{
	if (m_decompiler_thread)
	{
		Wait();
		if (m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	Delete();
}

void GLFragmentProgram::Wait()
{
	if (m_decompiler_thread && m_decompiler_thread->IsAlive())
	{
		m_decompiler_thread->Join();
	}
}

void GLFragmentProgram::Decompile(RSXFragmentProgram& prog)
{
	GLFragmentDecompilerThread decompiler(shader, parr, prog.addr, prog.size, prog.ctrl);
	decompiler.Task();
	for (const ParamType& PT : decompiler.m_parr.params[PF_PARAM_UNIFORM])
	{
		for (const ParamItem PI : PT.items)
		{
			size_t offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}
}

void GLFragmentProgram::DecompileAsync(RSXFragmentProgram& prog)
{
	if (m_decompiler_thread)
	{
		Wait();
		if (m_decompiler_thread->IsAlive())
		{
			m_decompiler_thread->Stop();
		}

		delete m_decompiler_thread;
		m_decompiler_thread = nullptr;
	}

	m_decompiler_thread = new GLFragmentDecompilerThread(shader, parr, prog.addr, prog.size, prog.ctrl);
	m_decompiler_thread->Start();
}

void GLFragmentProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* str = shader.c_str();
	const int strlen = shader.length();

	glShaderSource(id, 1, &str, &strlen);
	glCompileShader(id);

	GLint compileStatus = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus); // Determine the result of the glCompileShader call
	if (compileStatus != GL_TRUE) // If the shader failed to compile...
	{
		GLint infoLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLength); // Retrieve the length in bytes (including trailing NULL) of the shader info log

		if (infoLength > 0)
		{
			GLsizei len;
			char* buf = new char[infoLength]; // Buffer to store infoLog

			glGetShaderInfoLog(id, infoLength, &len, buf); // Retrieve the shader info log into our buffer
			LOG_ERROR(RSX, "Failed to compile shader: %s", buf); // Write log to the console

			delete[] buf;
		}

		LOG_NOTICE(RSX, shader.c_str()); // Log the text of the shader that failed to compile
		Emu.Pause(); // Pause the emulator, we can't really continue from here
	}
}

void GLFragmentProgram::Delete()
{
	shader.clear();

	if (id)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "GLFragmentProgram::Delete(): glDeleteShader(%d) avoided", id);
		}
		else
		{
			glDeleteShader(id);
		}
		id = 0;
	}
}
