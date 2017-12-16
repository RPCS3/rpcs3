#include "stdafx.h"
#include <set>
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "GLFragmentProgram.h"
#include "../Common/ProgramStateCache.h"
#include "GLCommonDecompiler.h"
#include "../GCM.h"


std::string GLFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string GLFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string GLFragmentDecompilerThread::saturate(const std::string & code)
{
	return "clamp(" + code + ", 0., 1.)";
}

std::string GLFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void GLFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	OS << "#version 430\n";
}

void GLFragmentDecompilerThread::insertIntputs(std::stringstream & OS)
{
	bool two_sided_enabled = m_prog.front_back_color_enabled && (m_prog.back_color_diffuse_output || m_prog.back_color_specular_output);
	std::vector<std::string> inputs_to_declare;

	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			//ssa is defined in the program body and is not a varying type
			if (PI.name == "ssa") continue;

			std::string var_name = PI.name;

			if (two_sided_enabled)
			{
				if (m_prog.back_color_diffuse_output && var_name == "diff_color")
					var_name = "back_diff_color";

				if (m_prog.back_color_specular_output && var_name == "spec_color")
					var_name = "back_spec_color";
			}

			if (var_name == "fogc")
				var_name = "fog_c";

			inputs_to_declare.push_back(var_name);
		}
	}

	if (two_sided_enabled)
	{
		if (m_prog.front_color_diffuse_output && m_prog.back_color_diffuse_output)
		{
			inputs_to_declare.push_back("front_diff_color");
		}

		if (m_prog.front_color_specular_output && m_prog.back_color_specular_output)
		{
			inputs_to_declare.push_back("front_spec_color");
		}
	}

	for (auto &name: inputs_to_declare)
	{
		OS << "layout(location=" << gl::get_varying_register_location(name) << ") in vec4 " << name << ";\n";
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
			OS << "layout(location=" << i << ") out vec4 " << table[i].first << ";\n";
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

			const auto mask = (1 << index);

			if (m_prog.shadow_textures & mask)
			{
				if (m_shadow_sampled_textures & mask)
				{
					if (m_2d_sampled_textures & mask)
						LOG_ERROR(RSX, "Texture unit %d is sampled as both a shadow texture and a depth texture", index);
					else
						samplerType = "sampler2DShadow";
				}
			}

			OS << "uniform " << samplerType << " " << PI.name << ";\n";
		}
	}

	OS << "\n";
	OS << "layout(std140, binding = 2) uniform FragmentConstantsBuffer\n";
	OS << "{\n";

	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler1D" ||
			PT.type == "sampler2D" ||
			PT.type == "sampler3D" ||
			PT.type == "samplerCube")
			continue;

		for (const ParamItem& PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";\n";
	}

	// Fragment state parameters
	OS << "	float fog_param0;\n";
	OS << "	float fog_param1;\n";
	OS << "	uint alpha_test;\n";
	OS << "	float alpha_ref;\n";
	OS << "	uint alpha_func;\n";
	OS << "	uint fog_mode;\n";
	OS << "	float wpos_scale;\n";
	OS << "	float wpos_bias;\n";
	OS << "	vec4 texture_parameters[16];\n";	//sampling: x,y scaling and (unused) offsets data
	OS << "};\n";
}

namespace
{
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

		fmt::throw_exception("Invalid texture dimension %d" HERE, (u32)prog.get_texture_dimension(index));
	}
}

void GLFragmentDecompilerThread::insertGlobalFunctions(std::stringstream &OS)
{
	glsl::insert_glsl_legacy_function(OS, glsl::glsl_fragment_program);
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	//TODO: Generate input mask during parse stage to avoid this
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (PI.name == "fogc")
			{
				glsl::insert_fog_declaration(OS);
				break;
			}
		}
	}

	const std::set<std::string> output_values =
	{
		"r0", "r1", "r2", "r3", "r4",
		"h0", "h2", "h4", "h6", "h8"
	};

	std::string parameters = "";
	for (auto &reg_name : output_values)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", reg_name))
		{
			if (parameters.length())
				parameters += ", ";

			parameters += "inout vec4 " + reg_name;
		}
	}

	OS << "void fs_main(" << parameters << ")\n";
	OS << "{\n";

	for (const ParamType& PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (output_values.find(PI.name) != output_values.end())
				continue;

			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;

			OS << ";\n";
		}
	}

	OS << "	vec4 ssa = gl_FrontFacing ? vec4(1.) : vec4(-1.);\n";
	OS << "	vec4 wpos = get_wpos();\n";

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

	bool two_sided_enabled = m_prog.front_back_color_enabled && (m_prog.back_color_diffuse_output || m_prog.back_color_specular_output);

	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (two_sided_enabled)
			{
				if (PI.name == "spec_color")
				{
					if (m_prog.back_color_specular_output)
					{
						if (m_prog.back_color_specular_output && m_prog.front_color_specular_output)
						{
							OS << "	vec4 spec_color = gl_FrontFacing ? front_spec_color : back_spec_color;\n";
						}
						else
						{
							OS << "	vec4 spec_color = back_spec_color;\n";
						}
					}

					continue;
				}

				else if (PI.name == "diff_color")
				{
					if (m_prog.back_color_diffuse_output)
					{
						if (m_prog.back_color_diffuse_output && m_prog.front_color_diffuse_output)
						{
							OS << "	vec4 diff_color = gl_FrontFacing ? front_diff_color : back_diff_color;\n";
						}
						else
						{
							OS << "	vec4 diff_color = back_diff_color;\n";
						}
					}

					continue;
				}
			}

			if (PI.name == "fogc")
			{
				OS << "	vec4 fogc = fetch_fog_value(fog_mode);\n";
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

	const std::set<std::string> output_values =
	{
		"r0", "r1", "r2", "r3", "r4",
		"h0", "h2", "h4", "h6", "h8"
	};

	std::string first_output_name = "";
	std::string color_output_block = "";

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
		{
			color_output_block += "	" + table[i].first + " = " + table[i].second + ";\n";
			if (first_output_name.empty()) first_output_name = table[i].second;
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
				const std::string texture_name = "tex" + std::to_string(index);
				if (m_parr.HasParamTypeless(PF_PARAM_UNIFORM, texture_name))
				{
					std::string fetch_texture = insert_texture_fetch(m_prog, index) + ".a";
					OS << make_comparison_test((rsx::comparison_function)m_prog.textures_zfunc[index], "", "0", fetch_texture);
				}
			}
		}

		OS << "	if (alpha_test != 0 && !comparison_passes(" << first_output_name << ".a, alpha_ref, alpha_func)) discard;\n";
	}

	OS << "}\n\n";

	OS << "void main()\n";
	OS << "{\n";

	std::string parameters = "";
	for (auto &reg_name : output_values)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", reg_name))
		{
			if (parameters.length())
				parameters += ", ";

			parameters += reg_name;
			OS << "	vec4 " << reg_name << " = vec4(0.);\n";
		}
	}

	OS << "\n" << "	fs_main(" + parameters + ");\n\n";

	//Append the color output assignments
	OS << color_output_block;

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "r1"))
		{
			/** Note: Naruto Shippuden : Ultimate Ninja Storm 2 sets CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS in a shader
			* but it writes depth in r1.z and not h2.z.
			* Maybe there's a different flag for depth ?
			*/
			//OS << ((m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) ? "\tgl_FragDepth = r1.z;\n" : "\tgl_FragDepth = h0.z;\n") << "\n";
			OS << "	gl_FragDepth = r1.z;\n";
		}
		else
		{
			//Input not declared. Leave commented to assist in debugging the shader
			OS << "	//gl_FragDepth = r1.z;\n";
		}
	}

	OS << "}\n";
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
			if (PT.type == "sampler1D" ||
				PT.type == "sampler2D" ||
				PT.type == "sampler3D" ||
				PT.type == "samplerCube")
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
