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
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			//ssa is defined in the program body and is not a varying type
			if (PI.name == "ssa") continue;

			const vk::varying_register_t &reg = vk::get_varying_register(PI.name);
			
			std::string var_name = PI.name;
			if (var_name == "fogc")
				var_name = "fog_c";

			OS << "layout(location=" << reg.reg_location << ") in " << PT.type << " " << var_name << ";" << std::endl;
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

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", table[i].second))
			OS << "layout(location=" << i << ") " << "out vec4 " << table[i].first << ";" << std::endl;
	}
}

void VKFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	int location = 2;

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
}

void VKFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	vk::insert_glsl_legacy_function(OS);

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

	// search if there is fogc in inputs
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (PI.name == "fogc")
			{
				vk::insert_fog_declaration(OS, m_prog.fog_equation);
				return;
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
			if (PT.type == "sampler2D")
				continue;
			size_t offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}
}

void VKFragmentProgram::Compile()
{
	fs::file(fs::get_config_dir() + "shaderlog/FragmentProgram.spirv", fs::rewrite).write(shader);

	std::vector<u32> spir_v;
	if (!vk::compile_glsl_to_spv(shader, vk::glsl::glsl_fragment_program, spir_v))
		throw EXCEPTION("Failed to compile fragment shader");

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
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "VKFragmentProgram::Delete(): vkDestroyShaderModule(0x%X) avoided", handle);
		}
		else
		{
			VkDevice dev = (VkDevice)*vk::get_current_renderer();
			vkDestroyShaderModule(dev, handle, NULL);
			handle = nullptr;
		}
	}
}

void VKFragmentProgram::SetInputs(std::vector<vk::glsl::program_input>& inputs)
{
	for (auto &it : inputs)
	{
		uniforms.push_back(it);
	}
}
