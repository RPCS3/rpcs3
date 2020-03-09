#include "stdafx.h"
#include "GLFragmentProgram.h"

#include "Emu/System.h"
#include "GLHelpers.h"
#include "GLFragmentProgram.h"
#include "GLCommonDecompiler.h"
#include "../GCM.h"
#include "../Common/GLSLCommon.h"

std::string GLFragmentDecompilerThread::getFloatTypeName(size_t elementCount)
{
	return glsl::getFloatTypeNameImpl(elementCount);
}

std::string GLFragmentDecompilerThread::getHalfTypeName(size_t elementCount)
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
	OS << "#version 430\n";

	if (device_props.has_native_half_support)
	{
		const auto driver_caps = gl::get_driver_caps();
		if (driver_caps.NV_gpu_shader5_supported)
		{
			OS << "#extension GL_NV_gpu_shader5: require\n";
		}
		else if (driver_caps.AMD_gpu_shader_half_float_supported)
		{
			OS << "#extension GL_AMD_gpu_shader_half_float: require\n";
		}
	}

	glsl::insert_subheader_block(OS);
}

void GLFragmentDecompilerThread::insertInputs(std::stringstream & OS)
{
	for (const ParamType& PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem& PI : PT.items)
		{
			//ssa is defined in the program body and is not a varying type
			if (PI.name == "ssa") continue;

			const auto reg_location = gl::get_varying_register_location(PI.name);
			std::string var_name = PI.name;

			if (var_name == "fogc")
			{
				var_name = "fog_c";
			}
			else if (m_prog.two_sided_lighting)
			{
				if (var_name == "diff_color")
				{
					var_name = "diff_color0";
				}
				else if (var_name == "spec_color")
				{
					var_name = "spec_color0";
				}
			}

			OS << "layout(location=" << reg_location << ") in vec4 " << var_name << ";\n";
		}
	}

	if (m_prog.two_sided_lighting)
	{
		if (properties.in_register_mask & in_diff_color)
		{
			OS << "layout(location=" << gl::get_varying_register_location("diff_color1") << ") in vec4 diff_color1;\n";
		}

		if (properties.in_register_mask & in_spec_color)
		{
			OS << "layout(location=" << gl::get_varying_register_location("spec_color1") << ") in vec4 spec_color1;\n";
		}
	}
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

			if (m_prog.redirected_textures & mask)
			{
				// Provide a stencil view of the main resource for the S channel
				OS << "uniform u" << samplerType << " " << PI.name << "_stencil;\n";
			}
			else if (m_prog.shadow_textures & mask)
			{
				if (m_shadow_sampled_textures & mask)
				{
					if (m_2d_sampled_textures & mask)
						rsx_log.error("Texture unit %d is sampled as both a shadow texture and a depth texture", index);
					else
						samplerType = "sampler2DShadow";
				}
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
		OS << "layout(std140, binding = 3) uniform FragmentConstantsBuffer\n";
		OS << "{\n";
		OS << constants_block;
		OS << "};\n\n";
	}

	OS << "layout(std140, binding = 4) uniform FragmentStateBuffer\n";
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

	OS << "layout(std140, binding = 5) uniform TextureParametersBuffer\n";
	OS << "{\n";
	OS << "	sampler_info texture_parameters[16];\n";
	OS << "};\n\n";
}

void GLFragmentDecompilerThread::insertGlobalFunctions(std::stringstream &OS)
{
	m_shader_props.domain = glsl::glsl_fragment_program;
	m_shader_props.require_lit_emulation = properties.has_lit_op;
	m_shader_props.fp32_outputs = !!(m_prog.ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS);
	m_shader_props.require_depth_conversion = m_prog.redirected_textures != 0;
	m_shader_props.require_wpos = !!(properties.in_register_mask & in_wpos);
	m_shader_props.require_texture_ops = properties.has_tex_op;
	m_shader_props.require_shadow_ops = m_prog.shadow_textures != 0;
	m_shader_props.emulate_coverage_tests = true; // g_cfg.video.antialiasing_level == msaa_level::none;
	m_shader_props.emulate_shadow_compare = device_props.emulate_depth_compare;
	m_shader_props.low_precision_tests = ::gl::get_driver_caps().vendor_NVIDIA;
	m_shader_props.disable_early_discard = !::gl::get_driver_caps().vendor_NVIDIA;
	m_shader_props.supports_native_fp16 = device_props.has_native_half_support;

	glsl::insert_glsl_legacy_function(OS, m_shader_props);
}

void GLFragmentDecompilerThread::insertMainStart(std::stringstream & OS)
{
	if (properties.in_register_mask & in_fogc)
		glsl::insert_fog_declaration(OS);

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
	GLFragmentDecompilerThread decompiler(shader, parr, prog, size);

	if (!g_cfg.video.disable_native_float16)
	{
		const auto driver_caps = gl::get_driver_caps();
		decompiler.device_props.has_native_half_support = driver_caps.NV_gpu_shader5_supported || driver_caps.AMD_gpu_shader_half_float_supported;
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

			size_t offset = atoi(PI.name.c_str() + 2);
			FragmentConstantOffsetCache.push_back(offset);
		}
	}
}

void GLFragmentProgram::Compile()
{
	if (id)
	{
		glDeleteShader(id);
	}

	id = glCreateShader(GL_FRAGMENT_SHADER);

	const char* str = shader.c_str();
	const int strlen = ::narrow<int>(shader.length());

	fs::file(fs::get_cache_dir() + "shaderlog/FragmentProgram" + std::to_string(id) + ".glsl", fs::rewrite).write(str);

	glShaderSource(id, 1, &str, &strlen);
	glCompileShader(id);

	GLint compileStatus = GL_FALSE;
	glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus); // Determine the result of the glCompileShader call
	if (compileStatus != GL_TRUE) // If the shader failed to compile...
	{
		GLint infoLength;
		glGetShaderiv(id, GL_INFO_LOG_LENGTH, &infoLength); // Retrieve the length in bytes (including trailing NULL) of the shader info log

		if (infoLength > 0)
		{
			GLsizei len;
			char* buf = new char[infoLength]; // Buffer to store infoLog

			glGetShaderInfoLog(id, infoLength, &len, buf); // Retrieve the shader info log into our buffer
			rsx_log.error("Failed to compile shader: %s", buf); // Write log to the console

			delete[] buf;
		}

		rsx_log.notice("%s", shader); // Log the text of the shader that failed to compile
		Emu.Pause(); // Pause the emulator, we can't really continue from here
	}
}

void GLFragmentProgram::Delete()
{
	shader.clear();

	if (id)
	{
		glDeleteShader(id);
		id = 0;
	}
}
