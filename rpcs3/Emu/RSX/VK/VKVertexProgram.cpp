#include "stdafx.h"

#include "VKVertexProgram.h"
#include "VKCommonDecompiler.h"
#include "VKHelpers.h"
#include "vkutils/device.h"
#include "../Program/GLSLCommon.h"

std::string VKVertexDecompilerThread::getFloatTypeName(usz elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string VKVertexDecompilerThread::getIntTypeName(usz /*elementCount*/)
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

void VKVertexDecompilerThread::prepareBindingTable()
{
	u32 location = 0;
	vk_prog->binding_table.vertex_buffers_location = location;
	location += 3; // Persistent verts, volatile and layout data

	vk_prog->binding_table.context_buffer_location = location++;
	if (m_device_props.emulate_conditional_rendering)
	{
		vk_prog->binding_table.cr_pred_buffer_location = location++;
	}

	std::memset(vk_prog->binding_table.vtex_location, 0xff, sizeof(vk_prog->binding_table.vtex_location));

	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		const bool is_texture_type = PT.type.starts_with("sampler");

		for (const ParamItem& PI : PT.items)
		{
			if (is_texture_type)
			{
				const int id = vk::get_texture_index(PI.name);
				vk_prog->binding_table.vtex_location[id] = location++;
				continue;
			}

			if (PI.name.starts_with("vc["))
			{
				if (!(m_prog.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS))
				{
					vk_prog->binding_table.cbuf_location = location++;
					continue;
				}

				vk_prog->binding_table.instanced_lut_buffer_location = location++;
				vk_prog->binding_table.instanced_cbuf_location = location++;
				continue;
			}
		}
	}
}

void VKVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	prepareBindingTable();

	OS <<
		"#version 450\n\n"
		"#extension GL_ARB_separate_shader_objects : enable\n\n";

	glsl::insert_subheader_block(OS);

	OS <<
		// Variable redirection
		"#define get_draw_params() draw_parameters[draw_parameters_offset]\n"
		"#define vs_context_offset get_draw_params().vs_context_offset\n\n"
		// Helpers
		"#define get_vertex_context() vertex_contexts[vs_context_offset]\n"
		"#define get_user_clip_config() get_vertex_context().user_clip_configuration_bits\n\n";

	OS <<
		"layout(std430, set=0, binding=" << vk_prog->binding_table.context_buffer_location << ") readonly restrict buffer VertexContextBuffer\n"
		"{\n"
		"	vertex_context_t vertex_contexts[];\n"
		"};\n\n";

	const vk::glsl::program_input context_input
	{
		.domain = glsl::glsl_vertex_program,
		.type = vk::glsl::input_type_storage_buffer,
		.set = vk::glsl::binding_set_index_vertex,
		.location = vk_prog->binding_table.context_buffer_location,
		.name = "VertexContextBuffer"
	};
	inputs.push_back(context_input);

	if (m_device_props.emulate_conditional_rendering)
	{
		OS <<
			"layout(std430, set=0, binding=" << vk_prog->binding_table.cr_pred_buffer_location << ") readonly restrict buffer EXT_Conditional_Rendering\n"
			"{\n"
			"	uint cr_predicate_value;\n"
			"};\n\n";

		const vk::glsl::program_input predicate_input
		{
			.domain = glsl::glsl_vertex_program,
			.type = vk::glsl::input_type_storage_buffer,
			.set = vk::glsl::binding_set_index_vertex,
			.location = vk_prog->binding_table.cr_pred_buffer_location,
			.name = "EXT_Conditional_Rendering"
		};
		inputs.push_back(predicate_input);
	}

	OS <<
		"layout(std430, set=0, binding=" << vk_prog->binding_table.vertex_buffers_location + 2 << ") readonly restrict buffer DrawParametersBuffer\n"
		"{\n"
		"	draw_parameters_t draw_parameters[];\n"
		"};\n\n";

	const vk::glsl::program_input layouts_input
	{
		.domain = glsl::glsl_vertex_program,
		.type = vk::glsl::input_type_storage_buffer,
		.set = vk::glsl::binding_set_index_vertex,
		.location = vk_prog->binding_table.vertex_buffers_location + 2,
		.name = "DrawParametersBuffer"
	};
	inputs.push_back(layouts_input);

	OS <<
		"layout(push_constant) uniform push_constants_block\n"
		"{\n"
		"	uint draw_parameters_offset;\n"
		"};\n\n";

	const vk::glsl::program_input push_constants
	{
		.domain = glsl::glsl_vertex_program,
		.type = vk::glsl::input_type_push_constant,
		.bound_data = vk::glsl::push_constant_ref{ .offset = 0, .size = 4 },
		.set = vk::glsl::binding_set_index_vertex,
		.location = umax,
		.name = "push_constants_block"
	};
	inputs.push_back(push_constants);
}

void VKVertexDecompilerThread::insertInputs(std::stringstream& OS, const std::vector<ParamType>& /*inputs*/)
{
	static const char* input_streams[] =
	{
		"persistent_input_stream",    // Data stream with persistent vertex data (cacheable)
		"volatile_input_stream"      // Data stream with per-draw data (registers and immediate draw data)
	};

	u32 location = vk_prog->binding_table.vertex_buffers_location;
	for (const auto& stream : input_streams)
	{
		OS << "layout(set=0, binding=" << location << ") uniform usamplerBuffer " << stream << ";\n";

		const vk::glsl::program_input input
		{
			.domain = glsl::glsl_vertex_program,
			.type = vk::glsl::input_type_texel_buffer,
			.set = vk::glsl::binding_set_index_vertex,
			.location = location++,
			.name = stream
		};
		this->inputs.push_back(input);
	}
}

void VKVertexDecompilerThread::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	vk::glsl::program_input in
	{
		.domain = ::glsl::glsl_vertex_program,
		.set = vk::glsl::binding_set_index_vertex
	};

	for (const ParamType &PT : constants)
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name.starts_with("vc["))
			{
				if (!(m_prog.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS))
				{
					OS << "layout(std430, set=0, binding=" << vk_prog->binding_table.cbuf_location << ") readonly restrict buffer VertexConstantsBuffer\n";
					OS << "{\n";
					OS << "	vec4 vc[];\n";
					OS << "};\n\n";

					in.location = vk_prog->binding_table.cbuf_location;
					in.name = "VertexConstantsBuffer";
					in.type = vk::glsl::input_type_storage_buffer;

					inputs.push_back(in);
					continue;
				}
				else
				{
					// 1. Bind indirection lookup buffer
					OS << "layout(std430, set=0, binding=" << vk_prog->binding_table.instanced_lut_buffer_location << ") readonly restrict buffer InstancingData\n";
					OS << "{\n";
					OS << "	int constants_addressing_lookup[];\n";
					OS << "};\n\n";

					in.location = vk_prog->binding_table.instanced_lut_buffer_location;
					in.name = "InstancingData";
					in.type = vk::glsl::input_type_storage_buffer;
					inputs.push_back(in);

					// 2. Bind actual constants buffer
					OS << "layout(std430, set=0, binding=" << vk_prog->binding_table.instanced_cbuf_location << ") readonly restrict buffer VertexConstantsBuffer\n";
					OS << "{\n";
					OS << "	vec4 instanced_constants_array[];\n";
					OS << "};\n\n";

					OS << "#define CONSTANTS_ARRAY_LENGTH " << (properties.has_indexed_constants ? 468 : ::size32(m_constant_ids)) << "\n\n";

					in.location = vk_prog->binding_table.instanced_cbuf_location;
					in.name = "VertexConstantsBuffer";
					in.type = vk::glsl::input_type_storage_buffer;
					inputs.push_back(in);
					continue;
				}
			}

			if (PT.type.starts_with("sampler"))
			{
				const int id = vk::get_texture_index(PI.name);
				in.location = vk_prog->binding_table.vtex_location[id];
				in.name = PI.name;
				in.type = vk::glsl::input_type_texture;

				inputs.push_back(in);

				auto samplerType = PT.type;

				if (m_prog.texture_state.multisampled_textures) [[ unlikely ]]
				{
					ensure(PI.name.length() > 3);
					int index = atoi(&PI.name[3]);

					if (m_prog.texture_state.multisampled_textures & (1 << index))
					{
						if (samplerType != "sampler1D" && samplerType != "sampler2D")
						{
							rsx_log.error("Unexpected multisampled sampler type '%s'", samplerType);
						}

						samplerType = "sampler2DMS";
					}
				}

				OS << "layout(set=0, binding=" << in.location << ") uniform " << samplerType << " " << PI.name << ";\n";
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
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y * user_clip_factor(0)", false, "is_user_clip_enabled(0)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC0 },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z * user_clip_factor(1)", false, "is_user_clip_enabled(1)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC1 },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w * user_clip_factor(2)", false, "is_user_clip_enabled(2)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC2 },
	{ "gl_PointSize", false, "dst_reg6", ".x", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_POINTSIZE },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y * user_clip_factor(3)", false, "is_user_clip_enabled(3)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC3 },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z * user_clip_factor(4)", false, "is_user_clip_enabled(4)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC4 },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w * user_clip_factor(5)", false, "is_user_clip_enabled(5)", "0.5", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_UC5 },
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

void VKVertexDecompilerThread::insertOutputs(std::stringstream& OS, const std::vector<ParamType>& /*outputs*/)
{
	for (auto &i : reg_table)
	{
		if (i.need_declare)
		{
			// All outputs must be declared always to allow setting default values
			OS << "layout(location=" << vk::get_varying_register_location(i.name) << ") out vec4 " << i.name << ";\n";
		}
	}

	if (!(m_prog.ctrl & RSX_SHADER_CONTROL_INTERPRETER_MODEL))
	{
		OS << "layout(location=" << vk::get_varying_register_location("usr") << ") out flat uvec4 draw_params_payload;\n";
	}
}

void VKVertexDecompilerThread::insertFSExport(std::stringstream& OS)
{
	OS <<
		"void write_fs_payload()\n"
		"{\n"
		"	draw_params_payload.x = get_draw_params().fs_constants_offset;\n"
		"	draw_params_payload.y = get_draw_params().fs_context_offset;\n"
		"	draw_params_payload.z = get_draw_params().fs_texture_base_index;\n"
		"	draw_params_payload.w = get_draw_params().fs_stipple_pattern_offset;\n"
		"}\n\n";
}

void VKVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	glsl::shader_properties properties2{};
	properties2.domain = glsl::glsl_vertex_program;
	properties2.require_lit_emulation = properties.has_lit_op;
	properties2.require_clip_plane_functions = true;
	properties2.emulate_zclip_transform = true;
	properties2.emulate_depth_clip_only = vk::g_render_device->get_shader_types_support().allow_float64;
	properties2.low_precision_tests = vk::is_NVIDIA(vk::get_driver_vendor());
	properties2.require_explicit_invariance = (vk::is_NVIDIA(vk::get_driver_vendor()) && g_cfg.video.shader_precision != gpu_preset_level::low);
	properties2.require_instanced_render = !!(m_prog.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS);

	glsl::insert_glsl_legacy_function(OS, properties2);
	glsl::insert_vertex_input_fetch(OS, glsl::glsl_rules_vulkan);

	insertFSExport(OS);

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

	// Expand indexed uniform structs here. We don't need to commit registers - these are very rarely consumed anyway.
	OS <<
		"#define scale_offset_mat get_vertex_context().scale_offset_mat\n"
		"#define transform_branch_bits get_vertex_context().transform_branch_bits\n"
		"#define point_size get_vertex_context().point_size\n"
		"#define z_near get_vertex_context().z_near\n"
		"#define z_far get_vertex_context().z_far\n\n";

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

	OS << "\nuint xform_constants_offset = get_draw_params().xform_constants_offset;\n\n";
}

void VKVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main ()\n";
	OS << "{\n\n";

	if (m_device_props.emulate_conditional_rendering)
	{
		OS << "	if (cr_predicate_value == 0)\n";
		OS << "	{\n";
		OS << "		gl_Position = vec4(0., 0., 0., -1.);\n";
		OS << "		return;\n";
		OS << "}\n\n";
	}

	OS << "	vs_main();\n\n";

	// FS payload
	OS << "write_fs_payload();\n\n";

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

	has_indexed_constants = decompiler.properties.has_indexed_constants;
	constant_ids = std::vector<u16>(decompiler.m_constant_ids.begin(), decompiler.m_constant_ids.end());

	shader.create(::glsl::program_domain::glsl_vertex_program, source);
}

void VKVertexProgram::Compile()
{
	if (g_cfg.video.log_programs)
		fs::write_file(fs::get_cache_dir() + "shaderlog/VertexProgram" + std::to_string(id) + ".spirv", fs::rewrite, shader.get_source());
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
