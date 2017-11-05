#include "stdafx.h"
#include "Emu/System.h"

#include "VKVertexProgram.h"
#include "VKCommonDecompiler.h"
#include "VKHelpers.h"

std::string VKVertexDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string VKVertexDecompilerThread::getIntTypeName(size_t elementCount)
{
	return "ivec4";
}

std::string VKVertexDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string VKVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void VKVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 450\n\n";
	OS << "#extension GL_ARB_separate_shader_objects : enable\n";
	OS << "layout(std140, set = 0, binding = 0) uniform VertexContextBuffer\n";
	OS << "{\n";
	OS << "	mat4 scale_offset_mat;\n";
	OS << "	ivec4 user_clip_enabled[2];\n";
	OS << "	vec4 user_clip_factor[2];\n";
	OS << "	uint transform_branch_bits;\n";
	OS << "	uint vertex_base_index;\n";
	OS << "	ivec4 input_attributes[16];\n";
	OS << "};\n";

	vk::glsl::program_input in;
	in.location = SCALE_OFFSET_BIND_SLOT;
	in.domain = glsl::glsl_vertex_program;
	in.name = "VertexContextBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);
}

void VKVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "layout(set=0, binding=3) uniform usamplerBuffer persistent_input_stream;\n";    //Data stream with persistent vertex data (cacheable)
	OS << "layout(set=0, binding=4) uniform usamplerBuffer volatile_input_stream;\n";      //Data stream with per-draw data (registers and immediate draw data)

	vk::glsl::program_input in;
	in.location = VERTEX_BUFFERS_FIRST_BIND_SLOT;
	in.domain = glsl::glsl_vertex_program;
	in.name = "persistent_input_stream";
	in.type = vk::glsl::input_type_texel_buffer;
	this->inputs.push_back(in);

	in.location = VERTEX_BUFFERS_FIRST_BIND_SLOT + 1;
	in.domain = glsl::glsl_vertex_program;
	in.name = "volatile_input_stream";
	in.type = vk::glsl::input_type_texel_buffer;
	this->inputs.push_back(in);
}

void VKVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "layout(std140, set=0, binding = 1) uniform VertexConstantsBuffer\n";
	OS << "{\n";
	OS << "	vec4 vc[468];\n";
	OS << "};\n\n";

	vk::glsl::program_input in;
	in.location = VERTEX_CONSTANT_BUFFERS_BIND_SLOT;
	in.domain = glsl::glsl_vertex_program;
	in.name = "VertexConstantsBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);


	int location = VERTEX_TEXTURES_FIRST_BIND_SLOT;
	for (const ParamType &PT : constants)
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name == "vc[468]")
				continue;

			if (PT.type == "sampler2D" ||
				PT.type == "samplerCube" ||
				PT.type == "sampler1D" ||
				PT.type == "sampler3D")
			{
				in.location = location;
				in.name = PI.name;
				in.type = vk::glsl::input_type_texture;

				inputs.push_back(in);

				OS << "layout(set = 0, binding=" << location++ << ") uniform " << PT.type << " " << PI.name << ";\n";
			}
		}
	}
}

static const vertex_reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	//Technically these two are for both back and front
	{ "back_diff_color", true, "dst_reg1", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE },
	{ "back_spec_color", true, "dst_reg2", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fog_c", true, "dst_reg5", ".xxxx", true, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FOG },
	//Warning: With spir-v if you declare clip distance var, you must assign a value even when its disabled! Runtime does not assign a default value
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y * user_clip_factor[0].x", false, "user_clip_enabled[0].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z * user_clip_factor[0].y", false, "user_clip_enabled[0].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w * user_clip_factor[0].z", false, "user_clip_enabled[0].z > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y * user_clip_factor[0].w", false, "user_clip_enabled[0].w > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z * user_clip_factor[1].x", false, "user_clip_enabled[1].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w * user_clip_factor[1].y", false, "user_clip_enabled[1].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
	{ "tc0", true, "dst_reg7", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX0 },
	{ "tc1", true, "dst_reg8", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX1 },
	{ "tc2", true, "dst_reg9", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX2 },
	{ "tc3", true, "dst_reg10", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX3 },
	{ "tc4", true, "dst_reg11", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX4 },
	{ "tc5", true, "dst_reg12", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX5 },
	{ "tc6", true, "dst_reg13", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX6 },
	{ "tc7", true, "dst_reg14", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX7 },
	{ "tc8", true, "dst_reg15", "", false, "", "", "", false, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX8 },
	{ "tc9", true, "dst_reg6", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX9 }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void VKVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	bool insert_front_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE) != 0;
	bool insert_back_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE) != 0;

	bool insert_front_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR) != 0;
	bool insert_back_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR) != 0;

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg) && i.need_declare)
		{
			if (i.check_mask && (rsx_vertex_program.output_mask & i.check_mask_value) == 0)
				continue;

			if (i.name == "front_diff_color")
				insert_front_diffuse = false;

			if (i.name == "front_spec_color")
				insert_front_specular = false;

			const vk::varying_register_t &reg = vk::get_varying_register(i.name);
			OS << "layout(location=" << reg.reg_location << ") out vec4 " << i.name << ";\n";
		}
		else
		{
			//Force some outputs to be declared even if unused so we can set default values
			//NOTE: Registers that can be skept will not have their check_mask_value set
			if (i.need_declare && (rsx_vertex_program.output_mask & i.check_mask_value) > 0)
			{
				const vk::varying_register_t &reg = vk::get_varying_register(i.name);
				OS << "layout(location=" << reg.reg_location << ") out vec4 " << i.name << ";\n";
			}
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		OS << "layout(location=" << vk::get_varying_register("front_diff_color").reg_location << ") out vec4 front_diff_color;\n";

	if (insert_back_specular && insert_front_specular)
		OS << "layout(location=" << vk::get_varying_register("front_spec_color").reg_location << ") out vec4 front_spec_color;\n";
}

void VKVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	glsl::insert_glsl_legacy_function(OS, glsl::glsl_vertex_program);
	glsl::insert_vertex_input_fetch(OS, glsl::glsl_rules_rpirv);

	std::string parameters = "";
	for (int i = 0; i < 16; ++i)
	{
		std::string reg_name = "dst_reg" + std::to_string(i);
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", reg_name))
		{
			if (parameters.length())
				parameters += ", ";

			parameters += "inout vec4 " + reg_name;
		}
	}

	OS << "void vs_main(" << parameters << ")\n";
	OS << "{\n";

	//Declare temporary registers, ignoring those mapped to outputs
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name.substr(0, 7) == "dst_reg")
				continue;

			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;

			OS << ";\n";
		}
	}

	for (const ParamType &PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
		{
			OS << "	vec4 " << PI.name << "= read_location(" << std::to_string(PI.location) << ");\n";
		}
	}
}

void VKVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main ()\n";
	OS << "{\n";

	std::string parameters = "";

	if (ParamType *vec4Types = m_parr.SearchParam(PF_PARAM_NONE, "vec4"))
	{
		for (int i = 0; i < 16; ++i)
		{
			std::string reg_name = "dst_reg" + std::to_string(i);
			for (auto &PI : vec4Types->items)
			{
				if (reg_name == PI.name)
				{
					if (parameters.length())
						parameters += ", ";

					parameters += reg_name;
					OS << "	vec4 " << reg_name;

					if (!PI.value.empty())
						OS << "= " << PI.value;

					OS << ";\n";
				}
			}
		}
	}

	OS << "\n" << "	vs_main(" << parameters << ");\n\n";

	bool insert_front_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE) != 0;
	bool insert_front_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR) != 0;

	bool insert_back_diffuse = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE) != 0;
	bool insert_back_specular = (rsx_vertex_program.output_mask & CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR) != 0;

	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", i.src_reg))
		{
			if (i.check_mask && (rsx_vertex_program.output_mask & i.check_mask_value) == 0)
				continue;

			if (i.name == "front_diff_color")
				insert_front_diffuse = false;

			if (i.name == "front_spec_color")
				insert_front_specular = false;

			std::string condition = (!i.cond.empty()) ? "(" + i.cond + ") " : "";

			if (condition.empty() || i.default_val.empty())
			{
				if (!condition.empty()) condition = "if " + condition;
				OS << "	" << condition << i.name << " = " << i.src_reg << i.src_reg_mask << ";\n";
			}
			else
			{
				//Insert if-else condition
				OS << "	" << i.name << " = " << condition << "? " << i.src_reg << i.src_reg_mask << ": " << i.default_val << ";\n";
			}
		}
		else if (i.need_declare && (rsx_vertex_program.output_mask & i.check_mask_value) > 0)
		{
			//An output was declared but nothing was written to it
			//Set it to all ones (Atelier Escha)
			OS << "	" << i.name << " = vec4(1.);\n";
		}
	}

	if (insert_back_diffuse && insert_front_diffuse)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg1"))
			OS << "	front_diff_color = dst_reg1;\n";

	if (insert_back_specular && insert_front_specular)
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "dst_reg2"))
			OS << "	front_spec_color = dst_reg2;\n";

	OS << "	gl_Position = gl_Position * scale_offset_mat;\n";
	OS << "}\n";
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
	fs::create_path(fs::get_config_dir() + "/shaderlog");
	fs::file(fs::get_config_dir() + "shaderlog/VertexProgram.spirv", fs::rewrite).write(shader);

	std::vector<u32> spir_v;
	if (!vk::compile_glsl_to_spv(shader, glsl::glsl_vertex_program, spir_v))
		fmt::throw_exception("Failed to compile vertex shader" HERE);

	VkShaderModuleCreateInfo vs_info;
	vs_info.codeSize = spir_v.size() * sizeof(u32);
	vs_info.pNext = nullptr;
	vs_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	vs_info.pCode = (uint32_t*)spir_v.data();
	vs_info.flags = 0;

	VkDevice dev = (VkDevice)*vk::get_current_renderer();
	vkCreateShaderModule(dev, &vs_info, nullptr, &handle);

	id = UINT32_MAX;
}

void VKVertexProgram::Delete()
{
	shader.clear();

	if (handle)
	{
		VkDevice dev = (VkDevice)*vk::get_current_renderer();
		vkDestroyShaderModule(dev, handle, nullptr);

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
