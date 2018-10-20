#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "VKFragmentProgram.h"

#include "VKCommonDecompiler.h"
#include "VKHelpers.h"
#include "../GCM.h"

std::string VKFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string VKFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string VKFragmentDecompilerThread::saturate(const std::string & code)
{
	return "clamp(" + code + ", 0., 1.)";
}

std::string VKFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void VKFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	OS << "#version 420\n";
	OS << "#extension GL_ARB_separate_shader_objects: enable\n\n";
}

void VKFragmentDecompilerThread::insertInputs(std::stringstream & OS)
{
	//It is possible for the two_sided_enabled flag to be set without actual 2-sided outputs
	bool two_sided_enabled = m_prog.front_back_color_enabled && (m_prog.back_color_diffuse_output || m_prog.back_color_specular_output);

	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			//ssa is defined in the program body and is not a varying type
			if (PI.name == "ssa") continue;

			const vk::varying_register_t &reg = vk::get_varying_register(PI.name);
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

			OS << "layout(location=" << reg.reg_location << ") in " << PT.type << " " << var_name << ";\n";
		}
	}

	if (two_sided_enabled)
	{
		//Only include the front counterparts if the default output is for back only and exists.
		if (m_prog.front_color_diffuse_output && m_prog.back_color_diffuse_output)
		{
			const vk::varying_register_t &reg = vk::get_varying_register("front_diff_color");
			OS << "layout(location=" << reg.reg_location << ") in vec4 front_diff_color;\n";
		}

		if (m_prog.front_color_specular_output && m_prog.back_color_specular_output)
		{
			const vk::varying_register_t &reg = vk::get_varying_register("front_spec_color");
			OS << "layout(location=" << reg.reg_location << ") in vec4 front_spec_color;\n";
		}
	}
}

void VKFragmentDecompilerThread::insertOutputs(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	//NOTE: We do not skip outputs, the only possible combinations are a(0), b(0), ab(0,1), abc(0,1,2), abcd(0,1,2,3)
	u8 output_index = 0;
	for (int i = 0; i < std::size(table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
		{
			OS << "layout(location=" << std::to_string(output_index++) << ") " << "out vec4 " << table[i].first << ";\n";
			vk_prog->output_color_masks[i] = UINT32_MAX;
		}
	}
}

void VKFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	int location = TEXTURES_FIRST_BIND_SLOT;
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

			vk::glsl::program_input in;
			in.location = location;
			in.domain = glsl::glsl_fragment_program;
			in.name = PI.name;
			in.type = vk::glsl::input_type_texture;

			inputs.push_back(in);

			OS << "layout(set=0, binding=" << location++ << ") uniform " << samplerType << " " << PI.name << ";\n";
		}
	}

	std::string constants_block;
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type == "sampler1D" ||
			PT.type == "sampler2D" ||
			PT.type == "sampler3D" ||
			PT.type == "samplerCube")
			continue;

		for (const ParamItem& PI : PT.items)
		{
			constants_block += "	" + PT.type + " " + PI.name + ";\n";
		}
	}

	if (!constants_block.empty())
	{
		OS << "layout(std140, set = 0, binding = 3) uniform FragmentConstantsBuffer\n";
		OS << "{\n";
		OS << constants_block;
		OS << "};\n\n";
	}

	OS << "layout(std140, set = 0, binding = 4) uniform FragmentStateBuffer\n";
	OS << "{\n";
	OS << "	float fog_param0;\n";
	OS << "	float fog_param1;\n";
	OS << "	uint rop_control;\n";
	OS << "	float alpha_ref;\n";
	OS << "	uint reserved;\n";
	OS << "	uint fog_mode;\n";
	OS << "	float wpos_scale;\n";
	OS << "	float wpos_bias;\n";
	OS << "};\n\n";

	OS << "layout(std140, set = 0, binding = 5) uniform TextureParametersBuffer\n";
	OS << "{\n";
	OS << "	vec4 texture_parameters[16];\n";
	OS << "};\n\n";

	vk::glsl::program_input in;
	in.location = FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT;
	in.domain = glsl::glsl_fragment_program;
	in.name = "FragmentConstantsBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;
	inputs.push_back(in);

	in.location = FRAGMENT_STATE_BIND_SLOT;
	in.name = "FragmentStateBuffer";
	inputs.push_back(in);

	in.location = FRAGMENT_TEXTURE_PARAMS_BIND_SLOT;
	in.name = "TextureParametersBuffer";
	inputs.push_back(in);
}

void VKFragmentDecompilerThread::insertGlobalFunctions(std::stringstream &OS)
{
	glsl::insert_glsl_legacy_function(OS, glsl::glsl_fragment_program, properties.has_lit_op, m_prog.redirected_textures != 0, properties.has_wpos_input);
}

void VKFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
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

	if (m_parr.HasParam(PF_PARAM_IN, "vec4", "ssa"))
		OS << "	vec4 ssa = gl_FrontFacing ? vec4(1.) : vec4(-1.);\n";

	if (properties.has_wpos_input)
		OS << "	vec4 wpos = get_wpos();\n";

	bool two_sided_enabled = m_prog.front_back_color_enabled && (m_prog.back_color_diffuse_output || m_prog.back_color_specular_output);

	//Some registers require redirection
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (two_sided_enabled)
			{
				if (PI.name == "spec_color")
				{
					//Only redirect/rename variables if the back_color exists
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
					//Only redirect/rename variables if the back_color exists
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

void VKFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	const std::set<std::string> output_values =
	{
		"r0", "r1", "r2", "r3", "r4",
		"h0", "h2", "h4", "h6", "h8"
	};

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

	glsl::insert_rop(OS, !!(m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS));

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "r1"))
		{
			//Depth writes are always from a fp32 register. See issues section on nvidia's NV_fragment_program spec
			//https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
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

void VKFragmentDecompilerThread::Task()
{
	m_shader = Decompile();
	vk_prog->SetInputs(inputs);
}

VKFragmentProgram::VKFragmentProgram()
{
}

VKFragmentProgram::~VKFragmentProgram()
{
	Delete();
}

void VKFragmentProgram::Decompile(const RSXFragmentProgram& prog)
{
	u32 size;
	std::string source;
	VKFragmentDecompilerThread decompiler(source, parr, prog, size, *this);
	decompiler.Task();

	shader.create(::glsl::program_domain::glsl_fragment_program, source);
	
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

void VKFragmentProgram::Compile()
{
	fs::file(fs::get_config_dir() + "shaderlog/FragmentProgram" + std::to_string(id) + ".spirv", fs::rewrite).write(shader.get_source());
	handle = shader.compile();
}

void VKFragmentProgram::Delete()
{
	shader.destroy();
}

void VKFragmentProgram::SetInputs(std::vector<vk::glsl::program_input>& inputs)
{
	for (auto &it : inputs)
	{
		uniforms.push_back(it);
	}
}
