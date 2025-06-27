#include "stdafx.h"
#include "VKFragmentProgram.h"
#include "VKCommonDecompiler.h"
#include "VKHelpers.h"
#include "vkutils/device.h"
#include "Emu/system_config.h"
#include "../Program/GLSLCommon.h"

std::string VKFragmentDecompilerThread::getFloatTypeName(usz elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string VKFragmentDecompilerThread::getHalfTypeName(usz elementCount)
{
	return glsl::getHalfTypeNameImpl(elementCount);
}

std::string VKFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string VKFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void VKFragmentDecompilerThread::prepareBindingTable()
{
	// First check if we have constants and textures as those need extra work
	bool has_constants = false, has_textures = false;
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (has_constants && has_textures)
		{
			break;
		}

		if (PT.type.starts_with("sampler"))
		{
			has_textures = true;
			continue;
		}

		ensure(PT.type.starts_with("vec"));
		has_constants = true;
	}

	unsigned location = 0; // All bindings must be set from this var
	vk_prog->binding_table.context_buffer_location = location++;
	if (has_constants)
	{
		vk_prog->binding_table.cbuf_location = location++;
	}

	vk_prog->binding_table.tex_param_location = location++;
	vk_prog->binding_table.polygon_stipple_params_location = location++;

	std::memset(vk_prog->binding_table.ftex_location, 0xff, sizeof(vk_prog->binding_table.ftex_location));
	std::memset(vk_prog->binding_table.ftex_stencil_location, 0xff, sizeof(vk_prog->binding_table.ftex_stencil_location));

	if (has_textures) [[ likely ]]
	{
		for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
		{
			if (!PT.type.starts_with("sampler"))
			{
				continue;
			}

			for (const ParamItem& PI : PT.items)
			{
				const auto texture_id = vk::get_texture_index(PI.name);
				const auto mask = 1u << texture_id;

				// Allocate real binding
				vk_prog->binding_table.ftex_location[texture_id] = location++;

				// Tag the stencil mirror if required
				if (properties.redirected_sampler_mask & mask) [[ unlikely ]]
				{
					vk_prog->binding_table.ftex_stencil_location[texture_id] = 0;
				}
			}

			// Normalize stencil offsets
			if (properties.redirected_sampler_mask != 0) [[ unlikely ]]
			{
				for (auto& stencil_location : vk_prog->binding_table.ftex_stencil_location)
				{
					if (stencil_location != 0)
					{
						continue;
					}

					stencil_location = location++;
				}
			}
		}
	}
}

void VKFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	prepareBindingTable();

	std::vector<const char*> required_extensions;

	if (device_props.has_native_half_support)
	{
		required_extensions.emplace_back("GL_EXT_shader_explicit_arithmetic_types_float16");
	}

	if (properties.multisampled_sampler_mask)
	{
		required_extensions.emplace_back("GL_ARB_shader_texture_image_samples");
	}

	if (m_prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION)
	{
		required_extensions.emplace_back("GL_EXT_fragment_shader_barycentric");
	}

	OS << "#version 450\n";
	for (const auto ext : required_extensions)
	{
		OS << "#extension " << ext << ": require\n";
	}

	OS << "#extension GL_ARB_separate_shader_objects: enable\n\n";

	glsl::insert_subheader_block(OS);
}

void VKFragmentDecompilerThread::insertInputs(std::stringstream & OS)
{
	glsl::insert_fragment_shader_inputs_block(
		OS,
		glsl::extension_flavour::EXT,
		m_prog,
		m_parr.params[PF_PARAM_IN],
		{
			.two_sided_color = !!(properties.in_register_mask & in_diff_color),
			.two_sided_specular = !!(properties.in_register_mask & in_spec_color)
		},
		vk::get_varying_register_location
	);
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
	const bool float_type = (m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) || !device_props.has_native_half_support;
	const auto reg_type = float_type ? "vec4" : getHalfTypeName(4);
	for (uint i = 0; i < std::size(table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, reg_type, table[i].second))
		{
			OS << "layout(location=" << std::to_string(output_index++) << ") " << "out vec4 " << table[i].first << ";\n";
			vk_prog->output_color_masks[i] = -1;
		}
	}
}

void VKFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (!PT.type.starts_with("sampler"))
		{
			continue;
		}

		for (const ParamItem& PI : PT.items)
		{
			std::string samplerType = PT.type;

			const int index = vk::get_texture_index(PI.name);
			const auto mask = (1 << index);

			if (properties.multisampled_sampler_mask & mask)
			{
				if (samplerType != "sampler1D" && samplerType != "sampler2D")
				{
					rsx_log.error("Unexpected multisampled image type '%s'", samplerType);
				}

				samplerType = "sampler2DMS";
			}
			else if (properties.shadow_sampler_mask & mask)
			{
				if (properties.common_access_sampler_mask & mask)
				{
					rsx_log.error("Texture unit %d is sampled as both a shadow texture and a depth texture", index);
				}
				else
				{
					samplerType += "Shadow";
				}
			}

			const int id = vk::get_texture_index(PI.name);
			auto in = vk::glsl::program_input::make(
				glsl::glsl_fragment_program,
				PI.name,
				vk::glsl::input_type_texture,
				vk::glsl::binding_set_index_fragment,
				vk_prog->binding_table.ftex_location[id]
			);
			inputs.push_back(in);

			OS << "layout(set=1, binding=" << in.location << ") uniform " << samplerType << " " << PI.name << ";\n";

			if (properties.redirected_sampler_mask & mask)
			{
				// Insert stencil mirror declaration
				in.name += "_stencil";
				in.location = vk_prog->binding_table.ftex_stencil_location[id];
				inputs.push_back(in);

				OS << "layout(set=1, binding=" << in.location << ") uniform u" << samplerType << " " << in.name << ";\n";
			}
		}
	}

	std::string constants_block;
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type.starts_with("sampler"))
		{
			continue;
		}

		for (const ParamItem& PI : PT.items)
		{
			constants_block += "	" + PT.type + " " + PI.name + ";\n";
		}
	}

	if (!constants_block.empty())
	{
		OS << "layout(std140, set=1, binding=" << vk_prog->binding_table.cbuf_location << ") uniform FragmentConstantsBuffer\n";
		OS << "{\n";
		OS << constants_block;
		OS << "};\n\n";
	}

	OS << "layout(std140, set=1, binding=" << vk_prog->binding_table.context_buffer_location << ") uniform FragmentStateBuffer\n";
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

	OS << "layout(std140, set=1, binding=" << vk_prog->binding_table.tex_param_location << ") uniform TextureParametersBuffer\n";
	OS << "{\n";
	OS << "	sampler_info texture_parameters[16];\n";
	OS << "};\n\n";

	OS << "layout(std140, set=1, binding=" << vk_prog->binding_table.polygon_stipple_params_location << ") uniform RasterizerHeap\n";
	OS << "{\n";
	OS << "	uvec4 stipple_pattern[8];\n";
	OS << "};\n\n";

	vk::glsl::program_input in
	{
		.domain = glsl::glsl_fragment_program,
		.type = vk::glsl::input_type_uniform_buffer,
		.set = vk::glsl::binding_set_index_fragment
	};

	if (!constants_block.empty())
	{
		in.location = vk_prog->binding_table.cbuf_location;
		in.name = "FragmentConstantsBuffer";
		inputs.push_back(in);
	}

	in.location = vk_prog->binding_table.context_buffer_location;
	in.name = "FragmentStateBuffer";
	inputs.push_back(in);

	in.location = vk_prog->binding_table.tex_param_location;
	in.name = "TextureParametersBuffer";
	inputs.push_back(in);

	in.location = vk_prog->binding_table.polygon_stipple_params_location;
	in.name = "RasterizerHeap";
	inputs.push_back(in);
}

void VKFragmentDecompilerThread::insertGlobalFunctions(std::stringstream &OS)
{
	m_shader_props.domain = glsl::glsl_fragment_program;
	m_shader_props.require_lit_emulation = properties.has_lit_op;
	m_shader_props.fp32_outputs = !!(m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS);
	m_shader_props.require_depth_conversion = properties.redirected_sampler_mask != 0;
	m_shader_props.require_wpos = !!(properties.in_register_mask & in_wpos);
	m_shader_props.require_texture_ops = properties.has_tex_op;
	m_shader_props.require_tex_shadow_ops = properties.shadow_sampler_mask != 0;
	m_shader_props.require_msaa_ops = m_prog.texture_state.multisampled_textures != 0;
	m_shader_props.require_texture_expand = properties.has_exp_tex_op;
	m_shader_props.require_srgb_to_linear = properties.has_upg;
	m_shader_props.require_linear_to_srgb = properties.has_pkg;
	m_shader_props.require_fog_read = properties.in_register_mask & in_fogc;
	m_shader_props.emulate_coverage_tests = g_cfg.video.antialiasing_level == msaa_level::none;
	m_shader_props.emulate_shadow_compare = device_props.emulate_depth_compare;
	m_shader_props.low_precision_tests = device_props.has_low_precision_rounding && !(m_prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION);
	m_shader_props.disable_early_discard = !vk::is_NVIDIA(vk::get_driver_vendor());
	m_shader_props.supports_native_fp16 = device_props.has_native_half_support;
	m_shader_props.ROP_output_rounding = g_cfg.video.shader_precision != gpu_preset_level::low;
	m_shader_props.require_tex1D_ops = properties.has_tex1D;
	m_shader_props.require_tex2D_ops = properties.has_tex2D;
	m_shader_props.require_tex3D_ops = properties.has_tex3D;
	m_shader_props.require_shadowProj_ops = properties.shadow_sampler_mask != 0 && properties.has_texShadowProj;

	glsl::insert_glsl_legacy_function(OS, m_shader_props);
}

void VKFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	std::set<std::string> output_registers;
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS)
	{
		output_registers = { "r0", "r2", "r3", "r4" };
	}
	else
	{
		output_registers = { "h0", "h4", "h6", "h8" };
	}

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		output_registers.insert("r1");
	}

	std::string registers;
	std::string reg_type;
	const auto half4 = getHalfTypeName(4);
	for (auto &reg_name : output_registers)
	{
		const auto type = (reg_name[0] == 'r' || !device_props.has_native_half_support)? "vec4" : half4;
		if (reg_type == type) [[likely]]
		{
			registers += ", " + reg_name + " = " + type + "(0.)";
		}
		else
		{
			if (!registers.empty())
				registers += ";\n";

			registers += type + " " + reg_name + " = " + type + "(0.)";
		}

		reg_type = type;
	}

	if (!registers.empty())
	{
		OS << registers << ";\n";
	}

	OS << "void fs_main()\n";
	OS << "{\n";

	for (const ParamType& PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (output_registers.find(PI.name) != output_registers.end())
				continue;

			OS << "	" << PT.type << " " << PI.name;
			if (!PI.value.empty())
				OS << " = " << PI.value;

			OS << ";\n";
		}
	}

	if (properties.has_w_access)
		OS << "	float in_w = (1. / gl_FragCoord.w);\n";

	if (properties.in_register_mask & in_ssa)
		OS << "	vec4 ssa = gl_FrontFacing ? vec4(1.) : vec4(-1.);\n";

	if (properties.in_register_mask & in_wpos)
		OS << "	vec4 wpos = get_wpos();\n";

	if (properties.in_register_mask & in_fogc)
		OS << "	vec4 fogc = fetch_fog_value(fog_mode);\n";

	if (m_prog.two_sided_lighting)
	{
		if (properties.in_register_mask & in_diff_color)
			OS << "	vec4 diff_color = gl_FrontFacing ? diff_color1 : diff_color0;\n";

		if (properties.in_register_mask & in_spec_color)
			OS << "	vec4 spec_color = gl_FrontFacing ? spec_color1 : spec_color0;\n";
	}
}

void VKFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main()\n";
	OS << "{\n";

	::glsl::insert_rop_init(OS);

	OS << "\n" << "	fs_main();\n\n";

	glsl::insert_rop(OS, m_shader_props);

	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, "vec4", "r1"))
		{
			// NOTE: Depth writes are always from a fp32 register. See issues section on nvidia's NV_fragment_program spec
			// https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt

			// NOTE: Depth writes in OpenGL (and by extension RSX) are clamped to 0,1 range.
			// Indeed, hardware tests on realhw prove that even in depth float mode, values outside this range are clamped.
			OS << "	gl_FragDepth = _saturate(r1.z);\n";
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

VKFragmentProgram::VKFragmentProgram() = default;

VKFragmentProgram::~VKFragmentProgram()
{
	Delete();
}

void VKFragmentProgram::Decompile(const RSXFragmentProgram& prog)
{
	u32 size;
	std::string source;
	VKFragmentDecompilerThread decompiler(source, parr, prog, size, *this);

	const auto pdev = vk::get_current_renderer();
	if (g_cfg.video.shader_precision == gpu_preset_level::low)
	{
		decompiler.device_props.has_native_half_support = pdev->get_shader_types_support().allow_float16;
	}

	decompiler.device_props.emulate_depth_compare = !pdev->get_formats_support().d24_unorm_s8;
	decompiler.device_props.has_low_precision_rounding = vk::is_NVIDIA(vk::get_driver_vendor());
	decompiler.Task();

	shader.create(::glsl::program_domain::glsl_fragment_program, source);

	for (const ParamType& PT : decompiler.m_parr.params[PF_PARAM_UNIFORM])
	{
		for (const ParamItem& PI : PT.items)
		{
			if (PT.type.starts_with("sampler"))
				continue;

			usz offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}
}

void VKFragmentProgram::Compile()
{
	if (g_cfg.video.log_programs)
		fs::write_file(fs::get_cache_dir() + "shaderlog/FragmentProgram" + std::to_string(id) + ".spirv", fs::rewrite, shader.get_source());
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
