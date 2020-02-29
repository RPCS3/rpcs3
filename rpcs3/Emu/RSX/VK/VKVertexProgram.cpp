#include "stdafx.h"

#include "VKVertexProgram.h"
#include "VKCommonDecompiler.h"
#include "VKHelpers.h"
#include "../Common/GLSLCommon.h"


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

std::string VKVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1, bool scalar)
{
	return glsl::compareFunctionImpl(f, Op0, Op1, scalar);
}

void VKVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS << "#version 450\n\n";
	OS << "#extension GL_ARB_separate_shader_objects : enable\n\n";

	OS << "layout(std140, set = 0, binding = 0) uniform VertexContextBuffer\n";
	OS << "{\n";
	OS << "	mat4 scale_offset_mat;\n";
	OS << "	ivec4 user_clip_enabled[2];\n";
	OS << "	vec4 user_clip_factor[2];\n";
	OS << "	uint transform_branch_bits;\n";
	OS << "	float point_size;\n";
	OS << "	float z_near;\n";
	OS << "	float z_far;\n";
	OS << "};\n\n";

	if (m_device_props.emulate_conditional_rendering)
	{
		OS << "layout(std430, set = 0, binding = 8) readonly buffer EXT_Conditional_Rendering\n";
		OS << "{\n";
		OS << "	uint conditional_rendering_predicate;\n";
		OS << "};\n\n";
	}

	OS << "layout(push_constant) uniform VertexLayoutBuffer\n";
	OS << "{\n";
	OS << "	uint vertex_base_index;\n";
	OS << "	uint vertex_index_offset;\n";
	OS << "	uint draw_id;\n";
	OS << "	uint layout_ptr_offset;\n";

	if (m_device_props.emulate_conditional_rendering)
	{
		OS << "	uint conditional_rendering_enabled;\n";
	}

	OS << "};\n\n";

	vk::glsl::program_input in;
	in.location = m_binding_table.vertex_params_bind_slot;;
	in.domain = glsl::glsl_vertex_program;
	in.name = "VertexContextBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;
	inputs.push_back(in);
}

void VKVertexDecompilerThread::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "layout(set=0, binding=5) uniform usamplerBuffer persistent_input_stream;\n";    // Data stream with persistent vertex data (cacheable)
	OS << "layout(set=0, binding=6) uniform usamplerBuffer volatile_input_stream;\n";      // Data stream with per-draw data (registers and immediate draw data)
	OS << "layout(set=0, binding=7) uniform usamplerBuffer vertex_layout_stream;\n";       // Data stream defining vertex data layout

	vk::glsl::program_input in;
	in.location = m_binding_table.vertex_buffers_first_bind_slot;
	in.domain = glsl::glsl_vertex_program;
	in.name = "persistent_input_stream";
	in.type = vk::glsl::input_type_texel_buffer;
	this->inputs.push_back(in);

	in.location = m_binding_table.vertex_buffers_first_bind_slot + 1;
	in.domain = glsl::glsl_vertex_program;
	in.name = "volatile_input_stream";
	in.type = vk::glsl::input_type_texel_buffer;
	this->inputs.push_back(in);

	in.location = m_binding_table.vertex_buffers_first_bind_slot + 2;
	in.domain = glsl::glsl_vertex_program;
	in.name = "vertex_layout_stream";
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
	in.location = m_binding_table.vertex_constant_buffers_bind_slot;
	in.domain = glsl::glsl_vertex_program;
	in.name = "VertexConstantsBuffer";
	in.type = vk::glsl::input_type_uniform_buffer;

	inputs.push_back(in);


	u32 location = m_binding_table.vertex_textures_first_bind_slot;
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
	{ "diff_color", true, "dst_reg1", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE },
	{ "spec_color", true, "dst_reg2", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR },
	{ "diff_color1", true, "dst_reg3", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE },
	{ "spec_color1", true, "dst_reg4", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR },
	{ "fog_c", true, "dst_reg5", ".xxxx", true, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FOG },
	//Warning: With spir-v if you declare clip distance var, you must assign a value even when its disabled! Runtime does not assign a default value
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y * user_clip_factor[0].x", false, "user_clip_enabled[0].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z * user_clip_factor[0].y", false, "user_clip_enabled[0].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w * user_clip_factor[0].z", false, "user_clip_enabled[0].z > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_PointSize", false, "dst_reg6", ".x", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_POINTSIZE },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y * user_clip_factor[0].w", false, "user_clip_enabled[0].w > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z * user_clip_factor[1].x", false, "user_clip_enabled[1].x > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w * user_clip_factor[1].y", false, "user_clip_enabled[1].y > 0", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
	{ "tc0", true, "dst_reg7", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX0 },
	{ "tc1", true, "dst_reg8", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX1 },
	{ "tc2", true, "dst_reg9", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX2 },
	{ "tc3", true, "dst_reg10", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX3 },
	{ "tc4", true, "dst_reg11", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX4 },
	{ "tc5", true, "dst_reg12", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX5 },
	{ "tc6", true, "dst_reg13", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX6 },
	{ "tc7", true, "dst_reg14", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX7 },
	{ "tc8", true, "dst_reg15", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX8 },
	{ "tc9", true, "dst_reg6", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_TEX9 }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void VKVertexDecompilerThread::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	for (auto &i : reg_table)
	{
		if (i.need_declare)
		{
			// All outputs must be declared always to allow setting default values
			OS << "layout(location=" << vk::get_varying_register_location(i.name) << ") out vec4 " << i.name << ";\n";
		}
	}
}

void VKVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	glsl::shader_properties properties2{};
	properties2.domain = glsl::glsl_vertex_program;
	properties2.require_lit_emulation = properties.has_lit_op;

	glsl::insert_glsl_legacy_function(OS, properties2);
	glsl::insert_vertex_input_fetch(OS, glsl::glsl_rules_spirv);

	// Declare global registers with optional initialization
	std::string registers;
	if (ParamType *vec4Types = m_parr.SearchParam(PF_PARAM_OUT, "vec4"))
	{
		for (auto &PI : vec4Types->items)
		{
			if (registers.length())
				registers += ", ";
			else
				registers = "vec4 ";

			registers += PI.name;

			if (!PI.value.empty())
			{
				// Simplify default initialization
				if (PI.value == "vec4(0.0, 0.0, 0.0, 0.0)")
					registers += " = vec4(0.)";
				else
					registers += " = " + PI.value;
			}
		}
	}

	if (!registers.empty())
	{
		OS << registers << ";\n";
	}

	OS << "void vs_main()\n";
	OS << "{\n";

	//Declare temporary registers, ignoring those mapped to outputs
	for (const ParamType &PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name.starts_with("dst_reg"))
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
	OS << "{\n\n";

	if (m_device_props.emulate_conditional_rendering)
	{
		OS << "	if (conditional_rendering_enabled != 0 && conditional_rendering_predicate == 0)\n";
		OS << "	{\n";
		OS << "		gl_Position = vec4(0., 0., 0., -1.);\n";
		OS << "		return;\n";
		OS << "}\n\n";
	}

	OS << "	vs_main();\n\n";

	for (auto &i : reg_table)
	{
		if (!i.check_mask || i.test(rsx_vertex_program.output_mask))
		{
			if (m_parr.HasParam(PF_PARAM_OUT, "vec4", i.src_reg))
			{
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

				// Register was marked for output and a properly initialized source register exists
				// Nothing more to do
				continue;
			}
		}

		if (i.need_declare)
		{
			OS << "	" << i.name << " = vec4(0., 0., 0., 1.);\n";
		}
		else if (i.check_mask_value == CELL_GCM_ATTRIB_OUTPUT_MASK_POINTSIZE)
		{
			// Default point size if none was generated by the program
			OS << "	gl_PointSize = point_size;\n";
		}
	}

	OS << "	gl_Position = gl_Position * scale_offset_mat;\n";
	OS << "	gl_Position = apply_zclip_xform(gl_Position, z_near, z_far);\n";
	OS << "}\n";
}


void VKVertexDecompilerThread::Task()
{
	m_device_props.emulate_conditional_rendering = vk::emulate_conditional_rendering();
	m_binding_table = vk::get_current_renderer()->get_pipeline_binding_table();

	m_shader = Decompile();
	vk_prog->SetInputs(inputs);
}

VKVertexProgram::VKVertexProgram() = default;

VKVertexProgram::~VKVertexProgram()
{
	Delete();
}

void VKVertexProgram::Decompile(const RSXVertexProgram& prog)
{
	std::string source;
	VKVertexDecompilerThread decompiler(prog, source, parr, *this);
	decompiler.Task();

	shader.create(::glsl::program_domain::glsl_vertex_program, source);
}

void VKVertexProgram::Compile()
{
	if (g_cfg.video.log_programs)
		fs::file(fs::get_cache_dir() + "shaderlog/VertexProgram" + std::to_string(id) + ".spirv", fs::rewrite).write(shader.get_source());
	handle = shader.compile();
}

void VKVertexProgram::Delete()
{
	shader.destroy();
}

void VKVertexProgram::SetInputs(std::vector<vk::glsl::program_input>& inputs)
{
	for (auto &it : inputs)
	{
		uniforms.push_back(it);
	}
}
