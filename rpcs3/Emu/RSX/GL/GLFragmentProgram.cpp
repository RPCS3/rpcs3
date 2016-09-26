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

	OS << std::endl;
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
		std::string fog_func = {};
		switch (mode)
		{
		case rsx::fog_mode::linear:
			fog_func = "fog_param1 * fog_c.x + (fog_param0 - 1.), fog_param1 * fog_c.x + (fog_param0 - 1.)";
			break;
		case rsx::fog_mode::exponential:
			fog_func = "11.084 * (fog_param1 * fog_c.x + fog_param0 - 1.5), exp(11.084 * (fog_param1 * fog_c.x + fog_param0 - 1.5))";
			break;
		case rsx::fog_mode::exponential2:
			fog_func = "4.709 * (fog_param1 * fog_c.x + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * fog_c.x + fog_param0 - 1.5)), 2.)";
			break;
		case rsx::fog_mode::linear_abs:
			fog_func = "fog_param1 * abs(fog_c.x) + (fog_param0 - 1.), fog_param1 * abs(fog_c.x) + (fog_param0 - 1.)";
			break;
		case rsx::fog_mode::exponential_abs:
			fog_func = "11.084 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5), exp(11.084 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5))";
			break;
		case rsx::fog_mode::exponential2_abs:
			fog_func = "4.709 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5), exp(-pow(4.709 * (fog_param1 * abs(fog_c.x) + fog_param0 - 1.5)), 2.)";
			break;

		default: fog_func = "0.0, 0.0";
		}

		OS << "	vec4 fogc = clamp(vec4(" << fog_func << ", 0., 0.), 0., 1.);\n";
	}

	void insert_texture_scale(std::stringstream & OS, const RSXFragmentProgram& prog, int index)
	{
		std::string vec_type = "vec2";

		switch (prog.get_texture_dimension(index))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: vec_type = "float"; break;
		case rsx::texture_dimension_extended::texture_dimension_2d: vec_type = "vec2"; break;
		case rsx::texture_dimension_extended::texture_dimension_3d:
		case rsx::texture_dimension_extended::texture_dimension_cubemap: vec_type = "vec3";
		}

		if (prog.unnormalized_coords & (1 << index))
			OS << "\t" << vec_type << " tex" << index << "_coord_scale = 1. / textureSize(tex" << index << ", 0);\n";
		else
			OS << "\t" << vec_type << " tex" << index << "_coord_scale = " << vec_type << "(1.);\n";
	}

	std::string insert_texture_fetch(const RSXFragmentProgram& prog, int index)
	{
		std::string tex_name = "tex" + std::to_string(index);
		std::string coord_name = "tc" + std::to_string(index);

		switch (prog.get_texture_dimension(index))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return "texture(" + tex_name + ", (" + coord_name + ".x * " + tex_name + "_coord_scale))";
		case rsx::texture_dimension_extended::texture_dimension_2d: return "texture(" + tex_name + ", (" + coord_name + ".xy * " + tex_name + "_coord_scale))";
		case rsx::texture_dimension_extended::texture_dimension_3d:
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return "texture(" + tex_name + ", (" + coord_name + ".xyz * " + tex_name + "_coord_scale))";
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

			insert_texture_scale(OS, m_prog, index);
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
				continue;
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
		auto make_comparison_test = [](rsx::comparison_function compare_func, const std::string &test, const std::string &a, const std::string &b) -> std::string
		{
			std::string compare;
			switch (compare_func)
			{
			case rsx::comparison_function::equal:            compare = " == "; break;
			case rsx::comparison_function::not_equal:        compare = " != "; break;
			case rsx::comparison_function::less_or_equal:    compare = " <= "; break;
			case rsx::comparison_function::less:             compare = " < ";  break;
			case rsx::comparison_function::greater:          compare = " > ";  break;
			case rsx::comparison_function::greater_or_equal: compare = " >= "; break;
			default:
				return "";
			}

			return "	if (" + test + "!(" + a + compare + b + ")) discard;\n";
		};

		for (u8 index = 0; index < 16; ++index)
		{
			if (m_prog.textures_alpha_kill[index])
			{
				std::string fetch_texture = insert_texture_fetch(m_prog, index) + ".a";
				OS << make_comparison_test((rsx::comparison_function)m_prog.textures_zfunc[index], "", "0", fetch_texture);
			}
		}

		OS << make_comparison_test(m_prog.alpha_func, "alpha_test != 0 && ", first_output_name + ".a", "alpha_ref");
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
	Delete();
}

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

void GLFragmentProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* str = shader.c_str();
	const int strlen = ::narrow<int>(shader.length());

	fs::create_path(fs::get_config_dir() + "/shaderlog");
	fs::file(fs::get_config_dir() + "shaderlog/FragmentProgram" + std::to_string(id) + ".glsl", fs::rewrite).write(str);

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

		LOG_NOTICE(RSX, "%s", shader); // Log the text of the shader that failed to compile
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
