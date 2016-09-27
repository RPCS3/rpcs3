#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "VKFragmentProgram.h"

#include "VKCommonDecompiler.h"
#include "VKHelpers.h"
#include "../GCM.h"

std::string VKFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return vk::getFloatTypeNameImpl(elementCount);
}

std::string VKFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return vk::getFunctionImpl(f);
}

std::string VKFragmentDecompilerThread::saturate(const std::string & code)
{
	return "clamp(" + code + ", 0., 1.)";
}

std::string VKFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return vk::compareFunctionImpl(f, Op0, Op1);
}

void VKFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	OS << "#version 420" << std::endl;
	OS << "#extension GL_ARB_separate_shader_objects: enable" << std::endl << std::endl;

	OS << "layout(std140, set=0, binding = 0) uniform ScaleOffsetBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	mat4 scaleOffsetMat;" << std::endl;
	OS << "	float fog_param0;" << std::endl;
	OS << "	float fog_param1;" << std::endl;
	OS << "	uint alpha_test;" << std::endl;
	OS << "	float alpha_ref;" << std::endl;
	OS << "};" << std::endl << std::endl;

	vk::glsl::program_input in;
	in.location = 0;
	in.domain = vk::glsl::glsl_fragment_program;
	in.name = "ScaleOffsetBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);
}

void VKFragmentDecompilerThread::insertIntputs(std::stringstream & OS)
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

			OS << "layout(location=" << reg.reg_location << ") in " << PT.type << " " << var_name << ";" << std::endl;
		}
	}

	if (two_sided_enabled)
	{
		//Only include the front counterparts if the default output is for back only and exists.
		if (m_prog.front_color_diffuse_output && m_prog.back_color_diffuse_output)
		{
			const vk::varying_register_t &reg = vk::get_varying_register("front_diff_color");
			OS << "layout(location=" << reg.reg_location << ") in vec4 front_diff_color;" << std::endl;
		}

		if (m_prog.front_color_specular_output && m_prog.back_color_specular_output)
		{
			const vk::varying_register_t &reg = vk::get_varying_register("front_spec_color");
			OS << "layout(location=" << reg.reg_location << ") in vec4 front_spec_color;" << std::endl;
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

	//We always bind the first usable image to index 0, even if surface type is surface_type::b
	//If only surface 1 is being written to, redirect to output 0

	if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[1].second) && !m_parr.HasParam(PF_PARAM_NONE, "vec4", table[0].second))
		OS << "layout(location=0) out vec4 " << table[1].first << ";" << std::endl;
	else
	{
		for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
		{
			if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
				OS << "layout(location=" << i << ") " << "out vec4 " << table[i].first << ";" << std::endl;
		}
	}
}

void VKFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	int location = 0;

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

			if (m_prog.unnormalized_coords & (1 << index))
				samplerType = "sampler2DRect";

			vk::glsl::program_input in;
			in.location = location;
			in.domain = vk::glsl::glsl_fragment_program;
			in.name = PI.name;
			in.type = vk::glsl::input_type_texture;

			inputs.push_back(in);

			OS << "layout(set=0, binding=" << 19 + location++ << ") uniform " << samplerType << " " << PI.name << ";" << std::endl;
		}
	}

	OS << "layout(std140, set = 0, binding = 2) uniform FragmentConstantsBuffer" << std::endl;
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

	vk::glsl::program_input in;
	in.location = 1;
	in.domain = vk::glsl::glsl_fragment_program;
	in.name = "FragmentConstantsBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);
}

namespace vk
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

	std::string insert_texture_fetch(const RSXFragmentProgram& prog, int index)
	{
		std::string tex_name = "tex" + std::to_string(index);
		std::string coord_name = "tc" + std::to_string(index);

		switch (prog.get_texture_dimension(index))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d: return "texture(" + tex_name + ", " + coord_name + ".x)";
		case rsx::texture_dimension_extended::texture_dimension_2d: return "texture(" + tex_name + ", " + coord_name + ".xy)";
		case rsx::texture_dimension_extended::texture_dimension_3d:
		case rsx::texture_dimension_extended::texture_dimension_cubemap: return "texture(" + tex_name + ", " + coord_name + ".xyz)";
		}
	}
}

void VKFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	vk::insert_glsl_legacy_function(OS);

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

	OS << "void fs_main(" << parameters << ")" << std::endl;
	OS << "{" << std::endl;

	for (const ParamType& PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (output_values.find(PI.name) != output_values.end())
				continue;

			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;

			OS << ";" << std::endl;
		}
	}

	OS << "	vec4 ssa = gl_FrontFacing ? vec4(1.) : vec4(-1.);\n";
	OS << "	vec4 wpos = gl_FragCoord;\n";

	//Flip wpos in Y
	//We could optionally export wpos from the VS, but this is so much easier
	if (m_prog.origin_mode == rsx::window_origin::bottom)
		OS << "	wpos.y = " << std::to_string(m_prog.height) << " - wpos.y;\n";

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
				vk::insert_fog_declaration(OS, m_prog.fog_equation);
				continue;
			}
		}
	}
}

void VKFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
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
				std::string fetch_texture = vk::insert_texture_fetch(m_prog, index) + ".a";
				OS << make_comparison_test((rsx::comparison_function)m_prog.textures_zfunc[index], "", "0", fetch_texture);
			}
		}

		OS << make_comparison_test(m_prog.alpha_func, "bool(alpha_test) && ", first_output_name + ".a", "alpha_ref");
	}

	OS << "}" << std::endl << std::endl;

	OS << "void main()" << std::endl;
	OS << "{" << std::endl;

	std::string parameters = "";
	for (auto &reg_name : output_values)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", reg_name))
		{
			if (parameters.length())
				parameters += ", ";

			parameters += reg_name;
			OS << "	vec4 " << reg_name << " = vec4(0.);" << std::endl;
		}
	}

	OS << std::endl << "	fs_main(" + parameters + ");" << std::endl << std::endl;

	//Append the color output assignments
	OS << color_output_block;

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

	OS << "}" << std::endl;
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
	VKFragmentDecompilerThread decompiler(shader, parr, prog, size, *this);
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

void VKFragmentProgram::Compile()
{
	fs::create_path(fs::get_config_dir() + "/shaderlog");
	fs::file(fs::get_config_dir() + "shaderlog/FragmentProgram.spirv", fs::rewrite).write(shader);

	std::vector<u32> spir_v;
	if (!vk::compile_glsl_to_spv(shader, vk::glsl::glsl_fragment_program, spir_v))
		fmt::throw_exception("Failed to compile fragment shader" HERE);

	//Create the object and compile
	VkShaderModuleCreateInfo fs_info;
	fs_info.codeSize = spir_v.size() * sizeof(u32);
	fs_info.pNext = nullptr;
	fs_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	fs_info.pCode = (uint32_t*)spir_v.data();
	fs_info.flags = 0;

	VkDevice dev = (VkDevice)*vk::get_current_renderer();
	vkCreateShaderModule(dev, &fs_info, nullptr, &handle);

	id = (u32)((u64)handle);
}

void VKFragmentProgram::Delete()
{
	shader.clear();

	if (handle)
	{
		VkDevice dev = (VkDevice)*vk::get_current_renderer();
		vkDestroyShaderModule(dev, handle, NULL);
		handle = nullptr;
	}
}

void VKFragmentProgram::SetInputs(std::vector<vk::glsl::program_input>& inputs)
{
	for (auto &it : inputs)
	{
		uniforms.push_back(it);
	}
}
