#include "stdafx.h"
#include "GLFragmentProgram.h"

#include "Emu/system_config.h"
#include "GLCommonDecompiler.h"
#include "../Program/GLSLCommon.h"
#include "../RSXThread.h"

std::string GLFragmentDecompilerThread::getFloatTypeName(usz elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string GLFragmentDecompilerThread::getHalfTypeName(usz elementCount)
{
	return glsl::getHalfTypeNameImpl(elementCount);
}

std::string GLFragmentDecompilerThread::getFunction(FUNCTION f)
{
	return glsl::getFunctionImpl(f);
}

std::string GLFragmentDecompilerThread::compareFunction(COMPARE f, const std::string &Op0, const std::string &Op1)
{
	return glsl::compareFunctionImpl(f, Op0, Op1);
}

void GLFragmentDecompilerThread::insertHeader(std::stringstream & OS)
{
	int gl_version = 430;
	std::vector<std::string> required_extensions;

	if (device_props.has_native_half_support)
	{
		const auto& driver_caps = gl::get_driver_caps();
		if (driver_caps.NV_gpu_shader5_supported)
		{
			required_extensions.push_back("GL_NV_gpu_shader5");
		}
		else if (driver_caps.AMD_gpu_shader_half_float_supported)
		{
			required_extensions.push_back("GL_AMD_gpu_shader_half_float");
		}
	}

	if (properties.multisampled_sampler_mask)
	{
		// Requires this extension or GLSL 450
		const auto& driver_caps = gl::get_driver_caps();
		if (driver_caps.glsl_version.version >= 450)
		{
			gl_version = 450;
		}
		else
		{
			ensure(driver_caps.ARB_shader_texture_image_samples_supported, "MSAA support on OpenGL requires a driver running OpenGL 4.5 or supporting GL_ARB_shader_texture_image_samples.");
			required_extensions.push_back("GL_ARB_shader_texture_image_samples");
		}
	}

	if (m_prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION)
	{
		gl_version = std::max(gl_version, 450);
		required_extensions.push_back("GL_NV_fragment_shader_barycentric");
	}

	OS << "#version " << gl_version << "\n";
	for (const auto& ext : required_extensions)
	{
		OS << "#extension " << ext << ": require\n";
	}

	glsl::insert_subheader_block(OS);
}

void GLFragmentDecompilerThread::insertInputs(std::stringstream & OS)
{
	glsl::insert_fragment_shader_inputs_block(
		OS,
		glsl::extension_flavour::NV,
		m_prog,
		m_parr.params[PF_PARAM_IN],
		{
			.two_sided_color = !!(properties.in_register_mask & in_diff_color),
			.two_sided_specular = !!(properties.in_register_mask & in_spec_color)
		},
		gl::get_varying_register_location
	);
}

void GLFragmentDecompilerThread::insertOutputs(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r0" : "h0" },
		{ "ocol1", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r2" : "h4" },
		{ "ocol2", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r3" : "h6" },
		{ "ocol3", m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS ? "r4" : "h8" },
	};

	const bool float_type = (m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS) || !device_props.has_native_half_support;
	const auto reg_type = float_type ? "vec4" : getHalfTypeName(4);
	for (uint i = 0; i < std::size(table); ++i)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, reg_type, table[i].second))
			OS << "layout(location=" << i << ") out vec4 " << table[i].first << ";\n";
	}
}

void GLFragmentDecompilerThread::insertConstants(std::stringstream & OS)
{
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
			int index = atoi(&PI.name[3]);

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

			if (properties.redirected_sampler_mask & mask)
			{
				// Provide a stencil view of the main resource for the S channel
				OS << "uniform u" << samplerType << " " << PI.name << "_stencil;\n";
			}

			OS << "uniform " << samplerType << " " << PI.name << ";\n";
		}
	}

	OS << "\n";

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
		OS << "layout(std140, binding = " << GL_FRAGMENT_CONSTANT_BUFFERS_BIND_SLOT << ") uniform FragmentConstantsBuffer\n";
		OS << "{\n";
		OS << constants_block;
		OS << "};\n\n";
	}

	OS << "layout(std140, binding = " << GL_FRAGMENT_STATE_BIND_SLOT << ") uniform FragmentStateBuffer\n";
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

	OS << "layout(std140, binding = " << GL_FRAGMENT_TEXTURE_PARAMS_BIND_SLOT << ") uniform TextureParametersBuffer\n";
	OS << "{\n";
	OS << "	sampler_info texture_parameters[16];\n";
	OS << "};\n\n";

	OS << "layout(std140, binding = " << GL_RASTERIZER_STATE_BIND_SLOT << ") uniform RasterizerHeap\n";
	OS << "{\n";
	OS << "	uvec4 stipple_pattern[8];\n";
	OS << "};\n\n";
}

void GLFragmentDecompilerThread::insertGlobalFunctions(std::stringstream &OS)
{
	m_shader_props.domain = glsl::glsl_fragment_program;
	m_shader_props.require_lit_emulation = properties.has_lit_op;
	m_shader_props.fp32_outputs = !!(m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS);
	m_shader_props.require_depth_conversion = properties.redirected_sampler_mask != 0;
	m_shader_props.require_wpos = !!(properties.in_register_mask & in_wpos);
	m_shader_props.require_texture_ops = properties.has_tex_op;
	m_shader_props.require_tex_shadow_ops = properties.shadow_sampler_mask != 0;
	m_shader_props.require_msaa_ops = properties.multisampled_sampler_mask != 0;
	m_shader_props.require_texture_expand = properties.has_exp_tex_op;
	m_shader_props.require_srgb_to_linear = properties.has_upg;
	m_shader_props.require_linear_to_srgb = properties.has_pkg;
	m_shader_props.require_fog_read = properties.in_register_mask & in_fogc;
	m_shader_props.emulate_coverage_tests = !rsx::get_renderer_backend_config().supports_hw_a2c_1spp;
	m_shader_props.emulate_shadow_compare = device_props.emulate_depth_compare;
	m_shader_props.low_precision_tests = ::gl::get_driver_caps().vendor_NVIDIA && !(m_prog.ctrl & RSX_SHADER_CONTROL_ATTRIBUTE_INTERPOLATION);
	m_shader_props.disable_early_discard = !::gl::get_driver_caps().vendor_NVIDIA;
	m_shader_props.supports_native_fp16 = device_props.has_native_half_support;
	m_shader_props.ROP_output_rounding = g_cfg.video.shader_precision != gpu_preset_level::low;
	m_shader_props.require_tex1D_ops = properties.has_tex1D;
	m_shader_props.require_tex2D_ops = properties.has_tex2D;
	m_shader_props.require_tex3D_ops = properties.has_tex3D;
	m_shader_props.require_shadowProj_ops = properties.shadow_sampler_mask != 0 && properties.has_texShadowProj;

	glsl::insert_glsl_legacy_function(OS, m_shader_props);
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
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
		for (const auto& PI : PT.items)
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

void GLFragmentDecompilerThread::insertMainEnd(std::stringstream & OS)
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

void GLFragmentDecompilerThread::Task()
{
	m_shader = Decompile();
}

GLFragmentProgram::GLFragmentProgram() = default;

GLFragmentProgram::~GLFragmentProgram()
{
	Delete();
}

void GLFragmentProgram::Decompile(const RSXFragmentProgram& prog)
{
	u32 size;
	std::string source;
	GLFragmentDecompilerThread decompiler(source, parr, prog, size);

	if (g_cfg.video.shader_precision == gpu_preset_level::low)
	{
		const auto& driver_caps = gl::get_driver_caps();
		decompiler.device_props.has_native_half_support = driver_caps.NV_gpu_shader5_supported || driver_caps.AMD_gpu_shader_half_float_supported;
		decompiler.device_props.has_low_precision_rounding = driver_caps.vendor_NVIDIA;
	}

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

			usz offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}

	shader.create(::glsl::program_domain::glsl_fragment_program, source);
	id = shader.id();
}

void GLFragmentProgram::Delete()
{
	shader.remove();
	id = 0;
}
