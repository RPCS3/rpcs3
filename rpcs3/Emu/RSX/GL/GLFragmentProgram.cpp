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

	OS << "layout(std140, binding = 0) uniform ScaleOffsetBuffer\n";
	OS << "{\n";
	OS << "	mat4 scaleOffsetMat;\n";
	OS << "	float fog_param0;\n";
	OS << "	float fog_param1;\n";
	OS << "	uint alpha_test;\n";
	OS << "	float alpha_ref;\n";
	OS << "};\n";
}

void GLFragmentDecompilerThread::insertIntputs(std::stringstream & OS)
{
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			//Rename fogc to fog_c to differentiate the input register from the variable
			if (PI.name == "fogc")
				OS << "in vec4 fog_c;" << std::endl;
			else
				OS << "in " << PT.type << " " << PI.name << ";" << std::endl;
		}
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
		if (PT.type != "sampler1D" &&
			PT.type != "sampler2D" &&
			PT.type != "sampler3D" &&
			PT.type != "samplerCube")
			continue;

		for (const ParamItem& PI : PT.items)
		{
			std::string samplerType = PT.type;
			int index = atoi(&PI.name.data()[3]);

			OS << "uniform " << samplerType << " " << PI.name << ";" << std::endl;
		}
	}

	OS << "layout(std140, binding = 2) uniform FragmentConstantsBuffer" << std::endl;
	OS << "{" << std::endl;

	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler1D" ||
			PT.type == "sampler2D" ||
			PT.type == "sampler3D" ||
			PT.type == "samplerCube")
			continue;

		for (const ParamItem& PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}

	// A dummy value otherwise it's invalid to create an empty uniform buffer
	OS << "	vec4 void_value;" << std::endl;
	OS << "};" << std::endl;
}


namespace
{
	// Note: It's not clear whether fog is computed per pixel or per vertex.
	// But it makes more sense to compute exp of interpoled value than to interpolate exp values.
	void insert_fog_declaration(std::stringstream & OS, rsx::fog_mode mode)
	{
		switch (mode)
		{
		case rsx::fog_mode::linear:
			OS << "	vec4 fogc = vec4(fog_param1 * fog_c.x + (fog_param0 - 1.), fog_param1 * fog_c.x + (fog_param0 - 1.), 0., 0.);\n";
			return;
		case rsx::fog_mode::exponential:
			OS << "	vec4 fogc = vec4(11.084 * (fog_param1 * fog_c.x + fog_param0 - 1.5), exp(11.084 * (fog_param1 * fog_c.x + fog_param0 - 1.5)), 0., 0.);\n";
			return;
		case rsx::fog_mode::exponential2:
			OS << "	vec4 fogc = vec4(4.709 * (fog_param1 * fog_c.x + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * fog_c.x + fog_param0 - 1.5)), 2.), 0., 0.);\n";
			return;
		case rsx::fog_mode::linear_abs:
			OS << "	vec4 fogc = vec4(fog_param1 * abs(fog_c.x) + (fog_param0 - 1.), fog_param1 * abs(fog_c.x) + (fog_param0 - 1.), 0., 0.);\n";
			return;
		case rsx::fog_mode::exponential_abs:
			OS << "	vec4 fogc = vec4(11.084 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5), exp(11.084 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5)), 0., 0.);\n";
			return;
		case rsx::fog_mode::exponential2_abs:
			OS << "	vec4 fogc = vec4(4.709 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5)), 2.), 0., 0.);\n";
			return;
		}
	}
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	insert_glsl_legacy_function(OS);

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

	OS << "	vec4 ssa = gl_FrontFacing ? vec4(1.) : vec4(-1.);\n";

	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type != "sampler2D")
			continue;

		for (const ParamItem& PI : PT.items)
		{
			std::string samplerType = PT.type;
			int index = atoi(&PI.name.data()[3]);

			if (m_prog.unnormalized_coords & (1 << index))
			{
				OS << "vec2 tex" << index << "_coord_scale = 1. / textureSize(" << PI.name << ", 0);\n";
			}
			else
			{
				OS << "vec2 tex" << index << "_coord_scale = vec2(1.);\n";
			}
		}
	}

	// search if there is fogc in inputs
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (PI.name == "fogc")
			{
				insert_fog_declaration(OS, m_prog.fog_equation);
				return;
			}
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

	std::string first_output_name;
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
		{
			OS << "	" << table[i].first << " = " << table[i].second << ";" << std::endl;
			if (first_output_name.empty()) first_output_name = table[i].first;
		}
	}

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		{
			/** Note: Naruto Shippuden : Ultimate Ninja Storm 2 sets CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS in a shader
			* but it writes depth in r1.z and not h2.z.
			* Maybe there's a different flag for depth ?
			*/
			//OS << ((m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? "\tgl_FragDepth = r1.z;\n" : "\tgl_FragDepth = h0.z;\n") << std::endl;
			OS << "	gl_FragDepth = r1.z;\n";
		}
	}

	if (!first_output_name.empty())
	{
		switch (m_prog.alpha_func)
		{
		case rsx::comparaison_function::equal:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a != alpha_ref) discard;\n";
			break;
		case rsx::comparaison_function::not_equal:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a == alpha_ref) discard;\n";
			break;
		case rsx::comparaison_function::less_or_equal:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a > alpha_ref) discard;\n";
			break;
		case rsx::comparaison_function::less:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a >= alpha_ref) discard;\n";
			break;
		case rsx::comparaison_function::greater:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a <= alpha_ref) discard;\n";
			break;
		case rsx::comparaison_function::greater_or_equal:
			OS << "	if (bool(alpha_test) && " << first_output_name << ".a < alpha_ref) discard;\n";
			break;
		}
	}

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
	const int strlen = ::narrow<int>(shader.length());

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
