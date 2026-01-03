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
	bool has_textures = false;
	for (const ParamType& PT : m_parr.params[PF_PARAM_UNIFORM])
	{
		if (PT.type.starts_with("sampler"))
		{
			has_textures = true;
			break;
		}
	}

	unsigned location = 0; // All bindings must be set from this var
	vk_prog->binding_table.context_buffer_location = location++;
	if (!properties.constant_offsets.empty())
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
		{ "ocol0", m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	// NOTE: We do not skip outputs, the only possible combinations are a(0), b(0), ab(0,1), abc(0,1,2), abcd(0,1,2,3)
	u8 output_index = 0;
	const bool float_type = (m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) || !device_props.has_native_half_support;
	const auto reg_type = float_type ? "vec4" : getHalfTypeName(4);
	for (uint i = 0; i < std::size(table); ++i)
	{
		if (!m_parr.HasParam(PF_PARAM_NONE, reg_type, table[i].second))
		{
			continue;
		}

		if (i >= m_prog.mrt_buffers_count)
		{
			// Dead writes. Declare as temp variables for DCE to clean up.
			OS << "vec4 " << table[i].first << "; // Unused\n";
			vk_prog->output_color_masks[i] = 0;
			continue;
		}

		OS << "layout(location=" << std::to_string(output_index++) << ") " << "out vec4 " << table[i].first << ";\n";
		vk_prog->output_color_masks[i] = -1;
	}
}

void VKFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
	// Fixed inputs from shader decompilation process
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

	// Draw params are always provided by vertex program. Instead of pointer chasing, they're provided as varyings.
	if (!(m_prog.ctrl & RSX_SHADER_CONTROL_INTERPRETER_MODEL))
	{
		OS <<
			"layout(location=" << vk::get_varying_register_location("usr") << ") in flat uvec4 draw_params_payload;\n\n";
	}

	OS <<
		"#define _fs_constants_offset draw_params_payload.x\n"
		"#define _fs_context_offset draw_params_payload.y\n"
		"#define _fs_texture_base_index draw_params_payload.z\n"
		"#define _fs_stipple_pattern_array_offset draw_params_payload.w\n\n";

	if (!properties.constant_offsets.empty())
	{
		OS << "layout(std430, set=1, binding=" << vk_prog->binding_table.cbuf_location << ") readonly buffer FragmentConstantsBuffer\n";
		OS << "{\n";
		OS << "	vec4 fc[];\n";
		OS << "};\n";
		OS << "#define _fetch_constant(x) fc[x + _fs_constants_offset]\n\n";
	}

	OS <<
		"layout(std430, set=1, binding=" << vk_prog->binding_table.context_buffer_location << ") readonly buffer FragmentStateBuffer\n"
		"{\n"
		"	fragment_context_t fs_contexts[];\n"
		"};\n\n";

	OS << "layout(std430, set=1, binding=" << vk_prog->binding_table.tex_param_location << ") readonly buffer TextureParametersBuffer\n";
	OS << "{\n";
	OS << "	sampler_info texture_parameters[];\n";
	OS << "};\n\n";

	OS << "layout(std430, set=1, binding=" << vk_prog->binding_table.polygon_stipple_params_location << ") readonly buffer RasterizerHeap\n";
	OS << "{\n";
	OS << "	uvec4 stipple_pattern[];\n";
	OS << "};\n\n";

	vk::glsl::program_input in
	{
		.domain = glsl::glsl_fragment_program,
		.set = vk::glsl::binding_set_index_fragment
	};

	if (!properties.constant_offsets.empty())
	{
		in.location = vk_prog->binding_table.cbuf_location;
		in.name = "FragmentConstantsBuffer";
		in.type = vk::glsl::input_type_storage_buffer,
		inputs.push_back(in);
	}

	in.location = vk_prog->binding_table.context_buffer_location;
	in.name = "FragmentStateBuffer";
	in.type = vk::glsl::input_type_storage_buffer;
	inputs.push_back(in);

	in.location = vk_prog->binding_table.tex_param_location;
	in.name = "TextureParametersBuffer";
	in.type = vk::glsl::input_type_storage_buffer;
	inputs.push_back(in);

	in.location = vk_prog->binding_table.polygon_stipple_params_location;
	in.type = vk::glsl::input_type_storage_buffer;
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
	m_shader_props.emulate_shadow_compare = device_props.emulate_depth_compare;

	m_shader_props.low_precision_tests = device_props.has_low_precision_rounding && !(m_prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION);
	m_shader_props.disable_early_discard = !vk::is_NVIDIA(vk::get_driver_vendor());
	m_shader_props.supports_native_fp16 = device_props.has_native_half_support;

	m_shader_props.ROP_output_rounding = (g_cfg.video.shader_precision != gpu_preset_level::low) && !!(m_prog.ctrl & RSX_SHADER_CONTROL_8BIT_FRAMEBUFFER);
	m_shader_props.ROP_sRGB_packing = !!(m_prog.ctrl & RSX_SHADER_CONTROL_SRGB_FRAMEBUFFER);
	m_shader_props.ROP_alpha_test = !!(m_prog.ctrl & RSX_SHADER_CONTROL_ALPHA_TEST);
	m_shader_props.ROP_alpha_to_coverage_test = !!(m_prog.ctrl & RSX_SHADER_CONTROL_ALPHA_TO_COVERAGE);
	m_shader_props.ROP_polygon_stipple_test = !!(m_prog.ctrl & RSX_SHADER_CONTROL_POLYGON_STIPPLE);
	m_shader_props.ROP_discard = !!(m_prog.ctrl & RSX_SHADER_CONTROL_USES_KIL);

	m_shader_props.require_tex1D_ops = properties.has_tex1D;
	m_shader_props.require_tex2D_ops = properties.has_tex2D;
	m_shader_props.require_tex3D_ops = properties.has_tex3D;
	m_shader_props.require_shadowProj_ops = properties.shadow_sampler_mask != 0 && properties.has_texShadowProj;
	m_shader_props.require_alpha_kill = !!(m_prog.ctrl & RSX_SHADER_CONTROL_TEXTURE_ALPHA_KILL);

	// Declare global constants
	if (m_shader_props.require_fog_read)
	{
		OS <<
			"#define fog_param0 fs_contexts[_fs_context_offset].fog_param0\n"
			"#define fog_param1 fs_contexts[_fs_context_offset].fog_param1\n"
			"#define fog_mode fs_contexts[_fs_context_offset].fog_mode\n\n";
	}

	if (m_shader_props.require_wpos)
	{
		OS <<
			"#define wpos_scale fs_contexts[_fs_context_offset].wpos_scale\n"
			"#define wpos_bias fs_contexts[_fs_context_offset].wpos_bias\n\n";
	}

	OS <<
		"#define texture_base_index _fs_texture_base_index\n"
		"#define TEX_PARAM(index) texture_parameters_##index\n\n";

	glsl::insert_glsl_legacy_function(OS, m_shader_props);
}

void VKFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	std::set<std::string> output_registers;
	if (m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS)
	{
		output_registers = { "r0", "r2", "r3", "r4" };
	}
	else
	{
		output_registers = { "h0", "h4", "h6", "h8" };
	}

	if (m_prog.ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
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

	for (u16 i = 0, mask = (properties.common_access_sampler_mask | properties.shadow_sampler_mask); mask != 0; ++i, mask >>= 1)
	{
		if (!(mask & 1))
		{
			continue;
		}

		OS << "	const sampler_info texture_parameters_" << i << " = texture_parameters[texture_base_index + " << i << "];\n";
	}
}

void VKFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
{
	OS << "}\n\n";

	OS << "void main()\n";
	OS << "{\n";

	if (m_prog.ctrl & RSX_SHADER_CONTROL_ALPHA_TEST)
	{
		OS <<
			"	const uint rop_control = fs_contexts[_fs_context_offset].rop_control;\n"
			"	const float alpha_ref = fs_contexts[_fs_context_offset].alpha_ref;\n\n";
	}

	::glsl::insert_rop_init(OS);

	OS << "\n" << "	fs_main();\n\n";

	if (m_prog.ctrl & RSX_SHADER_CONTROL_DISABLE_EARLY_Z)
	{
		// This is effectively unreachable code, but good enough to trick the GPU to skip early Z
		// For vulkan, depth export has stronger semantics than discard.
		OS <<
			"	// Insert pseudo-barrier sequence to disable early-Z\n"
			"	gl_FragDepth = gl_FragCoord.z;\n\n";
	}

	glsl::insert_rop(OS, m_shader_props);

	if (m_prog.ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
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

	constant_offsets = std::move(decompiler.properties.constant_offsets);
	shader.create(::glsl::program_domain::glsl_fragment_program, source);
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
