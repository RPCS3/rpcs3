#include "stdafx.h"
#include "GLVertexProgram.h"

#include "Emu/system_config.h"

#include "GLCommonDecompiler.h"
#include "../Program/GLSLCommon.h"

std::string GLVertexDecompilerThread::getFloatTypeName(usz elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string GLVertexDecompilerThread::getIntTypeName(usz /*elementCount*/)
{
	return "ivec4";
}

std::string GLVertexDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string GLVertexDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1, bool scalar)
{
	return glsl::compareFunctionImpl(f, Op0, Op1, scalar);
}

void GLVertexDecompilerThread::insertHeader(std::stringstream &OS)
{
	OS <<
		"#version 430\n"
		"layout(std140, binding = " << GL_VERTEX_PARAMS_BIND_SLOT << ") uniform VertexContextBuffer\n"
		"{\n"
		"	mat4 scale_offset_mat;\n"
		"	ivec4 user_clip_enabled[2];\n"
		"	vec4 user_clip_factor[2];\n"
		"	uint transform_branch_bits;\n"
		"	float point_size;\n"
		"	float z_near;\n"
		"	float z_far;\n"
		"};\n\n"

		"layout(std140, binding = " << GL_VERTEX_LAYOUT_BIND_SLOT << ") uniform VertexLayoutBuffer\n"
		"{\n"
		"	uint  vertex_base_index;\n"
		"	uint  vertex_index_offset;\n"
		"	uvec4 input_attributes_blob[16 / 2];\n"
		"};\n\n";
}

void GLVertexDecompilerThread::insertInputs(std::stringstream& OS, const std::vector<ParamType>& /*inputs*/)
{
	OS << "layout(location=0) uniform usamplerBuffer persistent_input_stream;\n";    // Data stream with persistent vertex data (cacheable)
	OS << "layout(location=1) uniform usamplerBuffer volatile_input_stream;\n";      // Data stream with per-draw data (registers and immediate draw data)
}

void GLVertexDecompilerThread::insertConstants(std::stringstream& OS, const std::vector<ParamType>& constants)
{
	for (const ParamType &PT: constants)
	{
		for (const ParamItem &PI : PT.items)
		{
			if (PI.name.starts_with("vc["))
			{
				if (!(m_prog.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS))
				{
					OS <<
						"layout(std140, binding = " << GL_VERTEX_CONSTANT_BUFFERS_BIND_SLOT << ") uniform VertexConstantsBuffer\n"
						"{\n"
						"	vec4 " << PI.name << ";\n"
						"};\n\n";
				}
				else
				{
					OS <<
						"layout(std430, binding = " << GL_INSTANCING_LUT_BIND_SLOT << ") readonly buffer InstancingIndirectionLUT\n"
						"{\n"
						"	int constants_addressing_lookup[];\n"
						"};\n\n"

						"layout(std430, binding = " << GL_INSTANCING_XFORM_CONSTANTS_SLOT << ") readonly buffer InstancingVertexConstantsBlock\n"
						"{\n"
						"	vec4 instanced_constants_array[];\n"
						"};\n\n"

						"#define CONSTANTS_ARRAY_LENGTH " << (properties.has_indexed_constants ? 468 : ::size32(m_constant_ids)) << "\n\n";
				}

				continue;
			}

			auto type = PT.type;

			if (PT.type == "sampler2D" ||
				PT.type == "samplerCube" ||
				PT.type == "sampler1D" ||
				PT.type == "sampler3D")
			{
				if (m_prog.texture_state.multisampled_textures) [[ unlikely ]]
				{
					ensure(PI.name.length() > 3);
					int index = atoi(&PI.name[3]);

					if (m_prog.texture_state.multisampled_textures & (1 << index))
					{
						if (type != "sampler1D" && type != "sampler2D")
						{
							rsx_log.error("Unexpected multisampled sampler type '%s'", type);
						}

						type = "sampler2DMS";
					}
				}
			}

			OS << "uniform " << type << " " << PI.name << ";\n";
		}
	}
}

static const vertex_reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE },
	{ "spec_color", true, "dst_reg2", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR },
	// These are only present when back variants are specified, otherwise the default diff/spec color vars are for both front and back
	{ "diff_color1", true, "dst_reg3", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTDIFFUSE | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKDIFFUSE },
	{ "spec_color1", true, "dst_reg4", "", false, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FRONTSPECULAR | CELL_GCM_ATTRIB_OUTPUT_MASK_BACKSPECULAR },
	// Fog output shares a data source register with clip planes 0-2 so only declare when specified
	{ "fog_c", true, "dst_reg5", ".xxxx", true, "", "", "", true, CELL_GCM_ATTRIB_OUTPUT_MASK_FOG },
	// Warning: Always define all 3 clip plane groups together to avoid flickering with openGL
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

void GLVertexDecompilerThread::insertOutputs(std::stringstream& OS, const std::vector<ParamType>& /*outputs*/)
{
	for (auto &i : reg_table)
	{
		if (i.need_declare)
		{
			// All outputs must be declared always to allow setting default values
			OS << "layout(location=" << gl::get_varying_register_location(i.name) << ") out vec4 " << i.name << ";\n";
		}
	}
}

void GLVertexDecompilerThread::insertMainStart(std::stringstream & OS)
{
	const auto& dev_caps = gl::get_driver_caps();

	glsl::shader_properties properties2{};
	properties2.domain = glsl::glsl_vertex_program;
	properties2.require_lit_emulation = properties.has_lit_op;
	properties2.emulate_zclip_transform = true;
	properties2.emulate_depth_clip_only = dev_caps.NV_depth_buffer_float_supported;
	properties2.low_precision_tests = dev_caps.vendor_NVIDIA;
	properties2.require_explicit_invariance = dev_caps.vendor_MESA || (dev_caps.vendor_NVIDIA && g_cfg.video.shader_precision != gpu_preset_level::low);
	properties2.require_instanced_render = !!(m_prog.ctrl & RSX_SHADER_CONTROL_INSTANCED_CONSTANTS);

	insert_glsl_legacy_function(OS, properties2);
	glsl::insert_vertex_input_fetch(OS, glsl::glsl_rules_opengl4, dev_caps.vendor_INTEL == false);

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

	// Declare temporary registers, ignoring those mapped to outputs
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

void GLVertexDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main ()\n";
	OS << "{\n";

	OS << "\n" << "	vs_main();\n\n";

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
					// Insert if-else condition
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

	// Since our clip_space is symmetrical [-1, 1] we map it to linear space using the eqn:
	// ln = (clip * 2) - 1 to fully utilize the 0-1 range of the depth buffer
	// RSX matrices passed already map to the [0, 1] range but mapping to classic OGL requires that we undo this step
	// This can be made unnecessary using the call glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE).
	// However, ClipControl only made it to opengl core in ver 4.5 though, so this is a workaround.
	   
	// NOTE: It is completely valid for games to use very large w values, causing the post-multiplied z to be in the hundreds
	// It is therefore critical that this step is done post-transform and the result re-scaled by w
	// SEE Naruto: UNS
	   
	// NOTE: On GPUs, poor fp32 precision means dividing z by w, then multiplying by w again gives slightly incorrect results
	// This equation is simplified algebraically to an addition and subtraction which gives more accurate results (Fixes flickering skybox in Dark Souls 2)
	// OS << "	float ndc_z = gl_Position.z / gl_Position.w;\n";
	// OS << "	ndc_z = (ndc_z * 2.) - 1.;\n";
	// OS << "	gl_Position.z = ndc_z * gl_Position.w;\n";
	OS << "	gl_Position.z = (gl_Position.z + gl_Position.z) - gl_Position.w;\n";
	OS << "}\n";
}

void GLVertexDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLVertexProgram::GLVertexProgram() = default;

GLVertexProgram::~GLVertexProgram()
{
	Delete();
}

void GLVertexProgram::Decompile(const RSXVertexProgram& prog)
{
	std::string source;
	GLVertexDecompilerThread decompiler(prog, source, parr);
	decompiler.Task();

	has_indexed_constants = decompiler.properties.has_indexed_constants;
	constant_ids = std::vector<u16>(decompiler.m_constant_ids.begin(), decompiler.m_constant_ids.end());

	shader.create(::glsl::program_domain::glsl_vertex_program, source);
	id = shader.id();
}

void GLVertexProgram::Delete()
{
	shader.remove();
	id = 0;
}
