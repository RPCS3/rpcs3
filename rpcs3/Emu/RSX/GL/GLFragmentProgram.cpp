#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLFragmentProgram.h"

#include "GLCommonDecompiler.h"
#include "../GCM.h"

std::string GLFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return getFloatTypeNameImpl(elementCount);
}

std::string GLFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return getFunctionImpl(f);
}

std::string GLFragmentDecompilerThread::saturate(const std::string & code)
{
	return "clamp(" + code + ", 0., 1.)";
}

std::string GLFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return compareFunctionImpl(f, Op0, Op1);
}

void GLFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	OS << "#version 420" << std::endl;
}

void GLFragmentDecompilerThread::insertIntputs(std::stringstream & OS)
{
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
			OS << "in " << PT.type << " " << PI.name << ";" << std::endl;
	}
}

void GLFragmentDecompilerThread::insertOutputs(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
			OS << "out vec4 " << table[i].first << ";" << std::endl;
	}
}

void GLFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;
		for (const ParamItem& PI : PT.items)
			OS << "uniform " << PT.type << " " << PI.name << ";" << std::endl;

	}

	OS << "layout(std140, binding = 2) uniform FragmentConstantsBuffer" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler2D")
			continue;
		for (const ParamItem& PI : PT.items)
			OS << "	 " << PT.type << " " << PI.name << ";" << std::endl;
	}
	// A dummy value otherwise it's invalid to create an empty uniform buffer
	OS << "	vec4 void_value;" << std::endl;
	OS << "};" << std::endl;
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	// "lib" function
	// 0.00001 is used as "some non zero very little number"
	OS << "vec4 divsq_legacy(vec4 num, vec4 denum)\n";
	OS << "{\n";
	OS << "	return num / sqrt(max(denum.xxxx, 0.00001));\n";
	OS << "}\n";

	OS << "vec4 rcp_legacy(vec4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / denum;\n";
	OS << "}\n";

	OS << "vec4 rsq_legacy(vec4 denum)\n";
	OS << "{\n";
	OS << "	return 1. / sqrt(max(denum, 0.00001));\n";
	OS << "}\n";

	OS << "void main ()" << std::endl;
	OS << "{" << std::endl;

	for (const ParamType& PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem& PI : PT.items)
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
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
			OS << "	" << table[i].first << " = " << table[i].second << ";" << std::endl;
	}

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
		OS << ((m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? "\tgl_FragDepth = r1.z;\n" : "\tgl_FragDepth = h0.z;\n") << std::endl;

	OS << "}" << std::endl;
}

void GLFragmentDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLFragmentProgram::GLFragmentProgram()
{
}

GLFragmentProgram::~GLFragmentProgram()
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

//void GLFragmentProgram::Wait()
//{
//	if (m_decompiler_thread && m_decompiler_thread->IsAlive())
//	{
//		m_decompiler_thread->Join();
//	}
//}

void GLFragmentProgram::Decompile(const RSXFragmentProgram& prog)
{
	u32 size;
	GLFragmentDecompilerThread decompiler(shader, parr, prog, size);
	decompiler.Task();
	for (const ParamType& PT : decompiler.m_parr.params[PF_PARAM_UNIFORM])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (PT.type == "sampler2D")
				continue;
			size_t offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}
}

//void GLFragmentProgram::DecompileAsync(RSXFragmentProgram& prog)
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
//	m_decompiler_thread = new GLFragmentDecompilerThread(shader, parr, prog.addr, prog.size, prog.ctrl);
//	m_decompiler_thread->Start();
//}

void GLFragmentProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* str = shader.c_str();
	const int strlen = gsl::narrow<int>(shader.length());

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
