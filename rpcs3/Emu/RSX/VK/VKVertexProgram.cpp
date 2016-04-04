#include "stdafx.h"
#include "Emu/System.h"

#include "VKVertexProgram.h"
#include "VKCommonDecompiler.h"
#include "VKHelpers.h"

std::string VKVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return vk::getFloatTypeNameImpl(elementCount);
}

std::string VKVertexDecompilerThread::getIntTypeName(size_t elementCount)
{
	return "ivec4";
}


std::string VKVertexDecompilerThread::getFunction(FUNCTION f)
{
	return vk::getFunctionImpl(f);
}

std::string VKVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return vk::compareFunctionImpl(f, Op0, Op1);
}

void VKVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 450" << std::endl << std::endl;
	OS << "#extension GL_ARB_separate_shader_objects : enable" << std::endl;
	OS << "layout(std140, set = 0, binding = 0) uniform ScaleOffsetBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	mat4 scaleOffsetMat;" << std::endl;
	OS << "	float fog_param0;\n";
	OS << "	float fog_param1;\n";
	OS << "};" << std::endl;

	vk::glsl::program_input in;
	in.location = 0;
	in.domain = vk::glsl::glsl_vertex_program;
	in.name = "ScaleOffsetBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);
}

void VKVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	std::vector<std::tuple<size_t, std::string>> input_data;
	for (const ParamType &PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
		{
			input_data.push_back(std::make_tuple(PI.location, PI.name));
		}
	}

	/**
	 * Its is important that the locations are in the order that vertex attributes are expected.
	 * If order is not adhered to, channels may be swapped leading to corruption
	*/

	std::sort(input_data.begin(), input_data.end());

	int location = 2;
	for (const std::tuple<size_t, std::string> item : input_data)
	{
		for (const ParamType &PT : inputs)
		{
			for (const ParamItem &PI : PT.items)
			{
				if (PI.name == std::get<1>(item))
				{
					vk::glsl::program_input in;
					in.location = location;
					in.domain = vk::glsl::glsl_vertex_program;
					in.name = PI.name + "_buffer";
					in.type = vk::glsl::input_type_texel_buffer;

					this->inputs.push_back(in);

					OS << "layout(set = 0, binding=" << 3 + location++ << ")" << "	uniform samplerBuffer" << " " << PI.name << "_buffer;" << std::endl;
				}
			}
		}
	}
}

void VKVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "layout(std140, set=0, binding = 1) uniform VertexConstantsBuffer" << std::endl;
	OS << "{" << std::endl;
	OS << "	vec4 vc[468];" << std::endl;
	OS << "};" << std::endl;

	vk::glsl::program_input in;
	in.location = 1;
	in.domain = vk::glsl::glsl_vertex_program;
	in.name = "VertexConstantsBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);
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
	{ "fog_c", true, "dst_reg5", ".xxxx", true },
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

void VKVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg) && i.need_declare)
		{
			const vk::varying_register_t &reg = vk::get_varying_register(i.name);
			
	//		if (i.name == "fogc")
	//			OS << "layout(location=" << reg.reg_location << ") out vec4 fog_c;" << std::endl;
	//		else
				OS << "layout(location=" << reg.reg_location << ") out vec4 " << i.name << ";" << std::endl;
		}
	}
}

namespace vk
{
	void add_default_channel_component(std::stringstream &OS, const std::string &name, size_t size)
	{
		switch (size)
		{
		case 1:
			OS << name << ".yzw = vec3(0., 0., 1.);\n";
			return;
		case 2:
			OS << name << ".zw = vec2(0., 1.);\n";
			return;
		case 3:
			OS << name << ".w = 1.;\n";
			return;
		}
	}

	void add_input(std::stringstream & OS, const ParamItem &PI, const std::vector<rsx_vertex_input> &inputs)
	{
		for (const auto &real_input : inputs)
		{
			if (real_input.location != PI.location)
				continue;

			if (!real_input.is_array)
			{
				OS << "	vec4 " << PI.name << " = texelFetch(" << PI.name << "_buffer, 0);" << std::endl;
				add_default_channel_component(OS, PI.name, real_input.size);
				return;
			}

			if (real_input.frequency > 1)
			{
				if (real_input.is_modulo)
				{
					OS << "	vec4 " << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexIndex %" << real_input.frequency << ");" << std::endl;
					add_default_channel_component(OS, PI.name, real_input.size);
					return;
				}

				OS << "	vec4 " << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexIndex /" << real_input.frequency << ");" << std::endl;
				add_default_channel_component(OS, PI.name, real_input.size);
				return;
			}

			OS << "	vec4 " << PI.name << "= texelFetch(" << PI.name << "_buffer, gl_VertexIndex).rgba;" << std::endl;
			add_default_channel_component(OS, PI.name, real_input.size);
			return;
		}

		OS << "	vec4 " << PI.name << " = vec4(0., 0., 0., 1.);" << std::endl;
	}
}

void VKVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	vk::insert_glsl_legacy_function(OS);

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

	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
			vk::add_input(OS, PI, rsx_vertex_program.rsx_vertex_inputs);
	}
}

void VKVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
			OS << "	" << i.name << " = " << i.src_reg << i.src_reg_mask << ";" << std::endl;
	}

	OS << "	gl_Position = gl_Position * scaleOffsetMat;" << std::endl;
	OS << "}" << std::endl;
}


void VKVertexDecompilerThread::Task()
{
	m_shader = Decompile();
	vk_prog->SetInputs(inputs);
}

VKVertexProgram::VKVertexProgram()
{
}

VKVertexProgram::~VKVertexProgram()
{
	Delete();
}

void VKVertexProgram::Decompile(const RSXVertexProgram& prog)
{
	VKVertexDecompilerThread decompiler(prog, shader, parr, *this);
	decompiler.Task();
}

void VKVertexProgram::Compile()
{
	fs::file(fs::get_config_dir() + "VertexProgram.vert", fom::rewrite).write(shader);

	std::vector<u32> spir_v;
	if (!vk::compile_glsl_to_spv(shader, vk::glsl::glsl_vertex_program, spir_v))
		throw EXCEPTION("Failed to compile vertex shader");

	VkShaderModuleCreateInfo vs_info;
	vs_info.codeSize = spir_v.size() * sizeof(u32);
	vs_info.pNext = nullptr;
	vs_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vs_info.pCode = (uint32_t*)spir_v.data();
	vs_info.flags = 0;

	VkDevice dev = (VkDevice)*vk::get_current_renderer();
	vkCreateShaderModule(dev, &vs_info, nullptr, &handle);

	id = (u32)((u64)handle);
}

void VKVertexProgram::Delete()
{
	shader.clear();

	if (handle)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(RSX, "VKVertexProgram::Delete(): vkDestroyShaderModule(0x%X) avoided", handle);
		}
		else
		{
			VkDevice dev = (VkDevice)*vk::get_current_renderer();
			vkDestroyShaderModule(dev, handle, nullptr);
		}

		handle = nullptr;
	}
}

void VKVertexProgram::SetInputs(std::vector<vk::glsl::program_input>& inputs)
{
	for (auto &it : inputs)
	{
		uniforms.push_back(it);
	}
}
