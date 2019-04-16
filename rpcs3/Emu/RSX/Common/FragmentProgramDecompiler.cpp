#include "stdafx.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "FragmentProgramDecompiler.h"

#include <algorithm>

FragmentProgramDecompiler::FragmentProgramDecompiler(const RSXFragmentProgram &prog, u32& size) :
	m_prog(prog),
	m_size(size),
	m_const_index(0),
	m_location(0),
	m_ctrl(prog.ctrl)
{
	m_size = 0;
}

void FragmentProgramDecompiler::SetDst(std::string code, u32 flags)
{
	if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt) return;

	if (src1.scale)
	{
		std::string modifier;
		switch (src1.scale)
		{
		case 0: break;
		case 1: code = "(" + code + " * "; modifier = "2."; break;
		case 2: code = "(" + code + " * "; modifier = "4."; break;
		case 3: code = "(" + code + " * "; modifier = "8."; break;
		case 5: code = "(" + code + " / "; modifier = "2."; break;
		case 6: code = "(" + code + " / "; modifier = "4."; break;
		case 7: code = "(" + code + " / "; modifier = "8."; break;

		default:
			LOG_ERROR(RSX, "Bad scale: %d", u32{ src1.scale });
			break;
		}

		if (flags & OPFLAGS::skip_type_cast && dst.fp16 && device_props.has_native_half_support)
		{
			modifier = getHalfTypeName(1) + "(" + modifier + ")";
		}

		if (!modifier.empty())
		{
			code = code + modifier + ")";
		}
	}

	if (!dst.no_dest)
	{
		if (dst.exp_tex)
		{
			//Expand [0,1] to [-1, 1]. Confirmed by Castlevania: LOS
			AddCode("//exp tex flag is set");
			code = "((" + code + "- 0.5) * 2.)";
		}

		if (dst.fp16 && device_props.has_native_half_support && !(flags & OPFLAGS::skip_type_cast))
		{
			// Cast to native data type
			if (dst.opcode == RSX_FP_OPCODE_NRM)
			{
				// Returns a 3-component vector as the result
				code = ClampValue(code + ".xyzz", 1, true);
			}
			else
			{
				code = ClampValue(code, 1, true);
			}
		}

		if (dst.saturate)
		{
			code = ClampValue(code, 4, !!dst.fp16);
		}
		else if (dst.prec)
		{
			switch (dst.opcode)
			{
			case RSX_FP_OPCODE_NRM:
			case RSX_FP_OPCODE_MAX:
			case RSX_FP_OPCODE_MIN:
			case RSX_FP_OPCODE_COS:
			case RSX_FP_OPCODE_SIN:
			case RSX_FP_OPCODE_REFL:
			case RSX_FP_OPCODE_EX2:
			case RSX_FP_OPCODE_FRC:
			case RSX_FP_OPCODE_LIT:
			case RSX_FP_OPCODE_LIF:
			case RSX_FP_OPCODE_LRP:
			case RSX_FP_OPCODE_LG2:
				break;
			case RSX_FP_OPCODE_MOV:
				// NOTE: Sometimes varying inputs from VS are out of range so do not exempt any input types, unless fp16 (Naruto UNS)
				if (dst.fp16 && src0.fp16 && src0.reg_type == RSX_FP_REGISTER_TYPE_TEMP)
					break;
			default:
			{
				// fp16 precsion flag on f32 register; ignore
				if (dst.prec == 1 && !dst.fp16)
					break;

				// Native type already has fp16 clamped (input must have been cast)
				if (dst.prec == 1 && dst.fp16 && device_props.has_native_half_support)
					break;

				// clamp value to allowed range
				code = ClampValue(code, dst.prec, !!dst.fp16);
				break;
			}
			}
		}
	}

	opflags = flags;
	code += (flags & OPFLAGS::no_src_mask) ? "" : "$m";

	if (dst.no_dest)
	{
		if (dst.set_cond)
		{
			AddCode("$ifcond " + m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + code + ";");
		}
		else
		{
			AddCode("$ifcond " + code + ";");
		}

		return;
	}

	std::string dest = AddReg(dst.dest_reg, !!dst.fp16) + "$m";

	AddCodeCond(Format(dest), code);
	//AddCode("$ifcond " + dest + code + (append_mask ? "$m;" : ";"));

	if (dst.set_cond)
	{
		AddCode(m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + dest + ";");
	}

	u32 reg_index = dst.fp16 ? dst.dest_reg >> 1 : dst.dest_reg;

	verify(HERE), reg_index < temp_registers.size();
	temp_registers[reg_index].tag(dst.dest_reg, !!dst.fp16, dst.mask_x, dst.mask_y, dst.mask_z, dst.mask_w);
}

void FragmentProgramDecompiler::AddFlowOp(std::string code)
{
	//Flow operations can only consider conditionals and have no dst

	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		AddCode(code + ";");
		return;
	}
	else if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		AddCode("//" + code + ";");
		return;
	}

	//We have a conditional expression
	std::string cond = GetRawCond();

	AddCode("if (any(" + cond + ")) " + code + ";");
}

void FragmentProgramDecompiler::AddCode(const std::string& code)
{
	main.append(m_code_level, '\t') += Format(code) + "\n";
}

std::string FragmentProgramDecompiler::GetMask()
{
	std::string ret;

	static const char dst_mask[4] =
	{
		'x', 'y', 'z', 'w',
	};

	if (dst.mask_x) ret += dst_mask[0];
	if (dst.mask_y) ret += dst_mask[1];
	if (dst.mask_z) ret += dst_mask[2];
	if (dst.mask_w) ret += dst_mask[3];

	return ret.empty() || strncmp(ret.c_str(), dst_mask, 4) == 0 ? "" : ("." + ret);
}

std::string FragmentProgramDecompiler::AddReg(u32 index, bool fp16)
{
	const std::string type_name = (fp16 && device_props.has_native_half_support)? getHalfTypeName(4) : getFloatTypeName(4);
	const std::string reg_name = std::string(fp16 ? "h" : "r") + std::to_string(index);

	return m_parr.AddParam(PF_PARAM_NONE, type_name, reg_name, type_name + "(0., 0., 0., 0.)");
}

bool FragmentProgramDecompiler::HasReg(u32 index, bool fp16)
{
	const std::string type_name = (fp16 && device_props.has_native_half_support)? getHalfTypeName(4) : getFloatTypeName(4);
	const std::string reg_name = std::string(fp16 ? "h" : "r") + std::to_string(index);

	return m_parr.HasParam(PF_PARAM_NONE, type_name, reg_name);
}

std::string FragmentProgramDecompiler::AddCond()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_reg_index));
}

std::string FragmentProgramDecompiler::AddConst()
{
	const std::string name = std::string("fc") + std::to_string(m_size + 4 * 4);
	const std::string type = getFloatTypeName(4);

	if (m_parr.HasParam(PF_PARAM_UNIFORM, type, name))
	{
		return name;
	}

	auto data = (be_t<u32>*) ((char*)m_prog.addr + m_size + 4 * u32{sizeof(u32)});
	m_offset = 2 * 4 * sizeof(u32);
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);

	const auto var = fmt::format("%s(%f, %f, %f, %f)", type, (f32&)x, (f32&)y, (f32&)z, (f32&)w);
	return m_parr.AddParam(PF_PARAM_UNIFORM, type, name, var);
}

std::string FragmentProgramDecompiler::AddTex()
{
	properties.has_tex_op = true;

	std::string sampler;
	switch (m_prog.get_texture_dimension(dst.tex_num))
	{
	case rsx::texture_dimension_extended::texture_dimension_1d:
		sampler = "sampler1D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_cubemap:
		sampler = "samplerCube";
		break;
	case rsx::texture_dimension_extended::texture_dimension_2d:
		sampler = "sampler2D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_3d:
		sampler = "sampler3D";
		break;
	}

	return m_parr.AddParam(PF_PARAM_UNIFORM, sampler, std::string("tex") + std::to_string(dst.tex_num));
}

std::string FragmentProgramDecompiler::AddType3()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "src3", getFloatTypeName(4) + "(1., 1., 1., 1.)");
}

std::string FragmentProgramDecompiler::AddX2d()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "x2d", getFloatTypeName(4) + "(0., 0., 0., 0.)");
}

std::string FragmentProgramDecompiler::ClampValue(const std::string& code, u32 precision, bool is_half_type)
{
	// FP16 is expected to overflow a lot easier at 0+-65504
	// FP32 can still work up to 0+-3.4E38
	// See http://http.download.nvidia.com/developer/Papers/2005/FP_Specials/FP_Specials.pdf

	if (LIKELY(precision <= 1))
	{
		if (precision == 1)
		{
			return "clamp16(" + code + ")";
		}
		else
		{
			return code;
		}
	}

	if (!is_half_type || !device_props.has_native_half_support)
	{
		switch (precision)
		{
		case 2:
			return "clamp(" + code + ", -2., 2.)";
		case 3:
			return "clamp(" + code + ", -1., 1.)";
		case 4:
			return "clamp(" + code + ", 0., 1.)";
		}
	}
	else
	{
		const auto type = getHalfTypeName(1);
		switch (precision)
		{
		case 2:
			return "clamp(" + code + ", " + type + "(-2.), " + type + "(2.))";
		case 3:
			return "clamp(" + code + ", " + type + "(-1.), " + type + "(1.))";
		case 4:
			return "clamp(" + code + ", " + type + "(0.), " + type + "(1.))";
		}
	}

	return code;
}

bool FragmentProgramDecompiler::DstExpectsSca()
{
	int writes = 0;

	if (dst.mask_x) writes++;
	if (dst.mask_y) writes++;
	if (dst.mask_z) writes++;
	if (dst.mask_w) writes++;

	return (writes == 1);
}

std::string FragmentProgramDecompiler::Format(const std::string& code, bool ignore_redirects)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", [this]() -> std::string {return GetSRC<SRC0>(src0);} },
		{ "$1", [this]() -> std::string {return GetSRC<SRC1>(src1);} },
		{ "$2", [this]() -> std::string {return GetSRC<SRC2>(src2);} },
		{ "$t", [this]() -> std::string { return "tex" + std::to_string(dst.tex_num);} },
		{ "$_i", [this]() -> std::string {return std::to_string(dst.tex_num);} },
		{ "$m", std::bind(std::mem_fn(&FragmentProgramDecompiler::GetMask), this) },
		{ "$ifcond ", [this]() -> std::string
			{
				const std::string& cond = GetCond();
				if (cond == "true") return "";
				return "if(" + cond + ") ";
			}
		},
		{ "$cond", std::bind(std::mem_fn(&FragmentProgramDecompiler::GetCond), this) },
		{ "$_c", std::bind(std::mem_fn(&FragmentProgramDecompiler::AddConst), this) },
		{ "$Ty", [this]() -> std::string { return (!device_props.has_native_half_support || !dst.fp16)? getFloatTypeName(4) : getHalfTypeName(4); } }
	};

	if (!ignore_redirects)
	{
		//Special processing redirects
		switch (dst.opcode)
		{
		case RSX_FP_OPCODE_TEXBEM:
		case RSX_FP_OPCODE_TXPBEM:
		{
			//Redirect parameter 0 to the x2d temp register for TEXBEM
			//TODO: Organize this a little better
			std::pair<std::string, std::string> repl[] = { { "$0", "x2d" } };
			std::string result = fmt::replace_all(code, repl);

			return fmt::replace_all(result, repl_list);
		}
		}
	}

	return fmt::replace_all(code, repl_list);
}

std::string FragmentProgramDecompiler::GetRawCond()
{
	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle, cond;
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];
	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	if (src0.exec_if_gr && src0.exec_if_eq)
		cond = compareFunction(COMPARE::FUNCTION_SGE, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");
	else if (src0.exec_if_lt && src0.exec_if_eq)
		cond = compareFunction(COMPARE::FUNCTION_SLE, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");
	else if (src0.exec_if_gr && src0.exec_if_lt)
		cond = compareFunction(COMPARE::FUNCTION_SNE, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");
	else if (src0.exec_if_gr)
		cond = compareFunction(COMPARE::FUNCTION_SGT, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");
	else if (src0.exec_if_lt)
		cond = compareFunction(COMPARE::FUNCTION_SLT, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");
	else //if(src0.exec_if_eq)
		cond = compareFunction(COMPARE::FUNCTION_SEQ, AddCond() + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");

	return cond;
}

std::string FragmentProgramDecompiler::GetCond()
{
	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		return "true";
	}
	else if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		return "false";
	}

	return "any(" + GetRawCond() + ")";
}

void FragmentProgramDecompiler::AddCodeCond(const std::string& dst, const std::string& src)
{
	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		AddCode(dst + " = " + src + ";");
		return;
	}

	if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		AddCode("//" + dst + " = " + src + ";");
		return;
	}

	// NOTE: dst = _select(dst, src, cond) is equivalent to dst = cond? src : dst;
	const auto cond = ShaderVariable(dst).match_size(GetRawCond());
	AddCode(dst + " = _select(" + dst + ", " + src + ", " + cond + ");");
}

template<typename T> std::string FragmentProgramDecompiler::GetSRC(T src)
{
	std::string ret;
	bool apply_precision_modifier = !!src1.input_prec_mod;
	bool is_half_type = false;

	switch (src.reg_type)
	{
	case RSX_FP_REGISTER_TYPE_TEMP:

		if (!src.fp16)
		{
			if (dst.opcode == RSX_FP_OPCODE_UP16 ||
				dst.opcode == RSX_FP_OPCODE_UP2 ||
				dst.opcode == RSX_FP_OPCODE_UP4 ||
				dst.opcode == RSX_FP_OPCODE_UPB ||
				dst.opcode == RSX_FP_OPCODE_UPG)
			{
				auto &reg = temp_registers[src.tmp_reg_index];
				if (reg.requires_gather(src.swizzle_x))
				{
					properties.has_gather_op = true;
					AddReg(src.tmp_reg_index, src.fp16);
					ret = getFloatTypeName(4) + reg.gather_r();
					break;
				}
			}
		}
		else
		{
			is_half_type = true;
		}

		ret += AddReg(src.tmp_reg_index, is_half_type);

		if (opflags & OPFLAGS::src_cast_f32 && is_half_type && device_props.has_native_half_support)
		{
			// Upconvert if there is a chance for ambiguity
			ret = getFloatTypeName(4) + "(" + ret + ")";
		}

		break;

	case RSX_FP_REGISTER_TYPE_INPUT:
	{
		static const std::string reg_table[] =
		{
			"wpos",
			"diff_color", "spec_color",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
			"ssa"
		};

		//TODO: Investigate effect of input modifier on this type

		switch (dst.src_attr_reg_num)
		{
		case 0x00:
			ret += reg_table[0];
			properties.has_wpos_input = true;
			break;
		default:
			if (dst.src_attr_reg_num < std::size(reg_table))
			{
				ret += m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), reg_table[dst.src_attr_reg_num]);
			}
			else
			{
				LOG_ERROR(RSX, "Bad src reg num: %d", u32{ dst.src_attr_reg_num });
				ret += m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), "unk");
				Emu.Pause();
			}
			break;
		}
	}
	break;

	case RSX_FP_REGISTER_TYPE_CONSTANT:
		ret += AddConst();
		apply_precision_modifier = false;
		break;

	case RSX_FP_REGISTER_TYPE_UNKNOWN: // ??? Used by a few games, what is it?
		LOG_ERROR(RSX, "Src type 3 used, opcode=0x%X, dst=0x%X s0=0x%X s1=0x%X s2=0x%X",
				dst.opcode, dst.HEX, src0.HEX, src1.HEX, src2.HEX);

		ret += AddType3();
		apply_precision_modifier = false;
		break;

	default:
		LOG_ERROR(RSX, "Bad src type %d", u32{ src.reg_type });
		Emu.Pause();
		break;
	}

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle = "";
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if (strncmp(swizzle.c_str(), f, 4) != 0) ret += "." + swizzle;

	// Warning: Modifier order matters. e.g neg should be applied after precision clamping (tested with Naruto UNS)
	if (src.abs) ret = "abs(" + ret + ")";
	if (apply_precision_modifier) ret = ClampValue(ret, src1.input_prec_mod, is_half_type);
	if (src.neg) ret = "-" + ret;

	return ret;
}

std::string FragmentProgramDecompiler::BuildCode()
{
	// Shader validation
	// Shader must at least write to one output for the body to be considered valid

	const bool fp16_out = !(m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS);
	const std::string vec4_type = (fp16_out && device_props.has_native_half_support)? getHalfTypeName(4) : getFloatTypeName(4);
	const std::string init_value = vec4_type + "(0., 0., 0., 0.)";
	std::array<std::string, 4> output_register_names;
	std::array<u32, 4> ouput_register_indices = { 0, 2, 3, 4 };
	bool shader_is_valid = false;

	// Check depth export
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		if (shader_is_valid = !!temp_registers[1].h0_writes; !shader_is_valid)
		{
			LOG_WARNING(RSX, "Fragment shader fails to write the depth value!");
		}
	}

	// Add the color output registers. They are statically written to and have guaranteed initialization (except r1.z which == wpos.z)
	// This can be used instead of an explicit clear pass in some games (Motorstorm)
	if (!fp16_out)
	{
		output_register_names = { "r0", "r2", "r3", "r4" };
	}
	else
	{
		output_register_names = { "h0", "h4", "h6", "h8" };
	}

	for (int n = 0; n < 4; ++n)
	{
		if (!m_parr.HasParam(PF_PARAM_NONE, vec4_type, output_register_names[n]))
		{
			m_parr.AddParam(PF_PARAM_NONE, vec4_type, output_register_names[n], init_value);
			continue;
		}

		const auto block_index = ouput_register_indices[n];
		shader_is_valid |= (!!temp_registers[block_index].h0_writes);
	}

	if (!shader_is_valid)
	{
		properties.has_no_output = true;

		if (!properties.has_discard_op)
		{
			// NOTE: Discard operation overrides output
			LOG_WARNING(RSX, "Shader does not write to any output register and will be NOPed");
			main = "/*" + main + "*/";
		}
	}

	std::stringstream OS;
	insertHeader(OS);
	OS << "\n";
	insertConstants(OS);
	OS << "\n";
	insertInputs(OS);
	OS << "\n";
	insertOutputs(OS);
	OS << "\n";

	// Insert global function definitions
	insertGlobalFunctions(OS);

	if (!device_props.has_native_half_support)
	{
		// Accurate float to half clamping (preserves IEEE-754 NaN)
		OS <<
		"vec4 clamp16(vec4 x)\n"
		"{\n"
		"	bvec4 sel = isnan(x);\n"
		"	vec4 clamped = clamp(x, -65504., +65504.);\n"
		"	if (!any(sel))\n"
		"	{\n"
		"		return clamped;\n"
		"	}\n\n"
		"	return _select(clamped, x, sel);\n"
		"}\n\n"

		"vec3 clamp16(vec3 x){ return clamp16(x.xyzz).xyz; }\n"
		"vec2 clamp16(vec2 x){ return clamp16(x.xyxy).xy; }\n"
		"float clamp16(float x){ return isnan(x)? x : clamp(x, -65504., +65504.); }\n\n";

		OS <<
		"#define _builtin_min min\n"
		"#define _builtin_max max\n"
		"#define _builtin_lit lit_legacy\n"
		"#define _builtin_distance distance\n"
		"#define _builtin_rcp(x) (1. / x)\n"
		"#define _builtin_rsq(x) (1. / sqrt(x))\n"
		"#define _builtin_log2(x) log2(abs(x))\n"
		"#define _builtin_div(x, y) (x / y)\n"
		"#define _builtin_divsq(x, y) (x / sqrt(y))\n\n";
	}
	else
	{
		// Define raw casts from f32->f16
		// Also define upcasting to avoid ambiguous function overloading in case of mixed inputs
		const std::string half4 = getHalfTypeName(4);
		OS <<
		"#define clamp16(x) " << half4 << "(x)\n"
		"#define _builtin_min(x, y) min(vec4(x), vec4(y))\n"
		"#define _builtin_max(x, y) max(vec4(x), vec4(y))\n"
		"#define _builtin_lit lit_legacy\n"
		"#define _builtin_distance(x, y) distance(vec4(x), vec4(y))\n"
		"#define _builtin_rcp(x) (1. / x)\n"
		"#define _builtin_rsq(x) (1. / sqrt(x))\n"
		"#define _builtin_log2(x) log2(abs(x))\n"
		"#define _builtin_div(x, y) (x / y)\n"
		"#define _builtin_divsq(x, y) (x / sqrt(y))\n\n";
	}
	

	// Declare register gather/merge if needed
	if (properties.has_gather_op)
	{
		std::string float2 = getFloatTypeName(2);
		std::string float4 = getFloatTypeName(4);

		OS << float4 << " gather(" << float4 << " _h0, " << float4 << " _h1)\n";
		OS << "{\n";
		OS << "	float x = uintBitsToFloat(packHalf2x16(_h0.xy));\n";
		OS << "	float y = uintBitsToFloat(packHalf2x16(_h0.zw));\n";
		OS << "	float z = uintBitsToFloat(packHalf2x16(_h1.xy));\n";
		OS << "	float w = uintBitsToFloat(packHalf2x16(_h1.zw));\n";
		OS << "	return " << float4 << "(x, y, z, w);\n";
		OS << "}\n\n";

		OS << float2 << " gather(" << float4 << " _h)\n";
		OS << "{\n";
		OS << "	float x = uintBitsToFloat(packHalf2x16(_h.xy));\n";
		OS << "	float y = uintBitsToFloat(packHalf2x16(_h.zw));\n";
		OS << "	return " << float2 << "(x, y);\n";
		OS << "}\n\n";
	}

	insertMainStart(OS);
	OS << main << std::endl;
	insertMainEnd(OS);

	return OS.str();
}

bool FragmentProgramDecompiler::handle_sct_scb(u32 opcode)
{
	switch (opcode)
	{
	case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); return true;
	case RSX_FP_OPCODE_DIV: SetDst("_builtin_div($0, $1.x)"); return true;
	// Note: DIVSQ is not IEEE compliant. divsq(0, 0) is 0 (Super Puzzle Fighter II Turbo HD Remix).
	// sqrt(x, 0) might be equal to some big value (in absolute) whose sign is sign(x) but it has to be proven.
	case RSX_FP_OPCODE_DIVSQ: SetDst("_builtin_divsq($0, $1.x)"); return true;
	case RSX_FP_OPCODE_DP2: SetDst(getFunction(FUNCTION::FUNCTION_DP2), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP3: SetDst(getFunction(FUNCTION::FUNCTION_DP3), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP4: SetDst(getFunction(FUNCTION::FUNCTION_DP4), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP2A: SetDst(getFunction(FUNCTION::FUNCTION_DP2A), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); return true;
	case RSX_FP_OPCODE_MAX: SetDst("_builtin_max($0, $1)"); return true;
	case RSX_FP_OPCODE_MIN: SetDst("_builtin_min($0, $1)"); return true;
	case RSX_FP_OPCODE_MOV: SetDst("$0"); return true;
	case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); return true;
	// Note: It's highly likely that RCP is not IEEE compliant but a game that uses rcp(0) has to be found
	case RSX_FP_OPCODE_RCP: SetDst("_builtin_rcp($0.x).xxxx"); return true;
	// Note: RSQ is not IEEE compliant. rsq(0) is some big number (Silent Hill 3 HD)
	// It is not know what happens if 0 is negative.
	case RSX_FP_OPCODE_RSQ: SetDst("_builtin_rsq($0.x).xxxx"); return true;
	case RSX_FP_OPCODE_SEQ: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SEQ, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SFL: SetDst(getFunction(FUNCTION::FUNCTION_SFL), OPFLAGS::skip_type_cast); return true;
	case RSX_FP_OPCODE_SGE: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SGE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SGT: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SGT, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SLE: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SLE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SLT: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SLT, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SNE: SetDst("$Ty(" + compareFunction(COMPARE::FUNCTION_SNE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_STR: SetDst(getFunction(FUNCTION::FUNCTION_STR), OPFLAGS::skip_type_cast); return true;

	// SCB-only ops
	case RSX_FP_OPCODE_COS: SetDst("cos($0.xxxx)"); return true;
	case RSX_FP_OPCODE_DST: SetDst("_builtin_distance($0, $1).xxxx"); return true;
	case RSX_FP_OPCODE_REFL: SetDst(getFunction(FUNCTION::FUNCTION_REFL), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_EX2: SetDst("exp2($0.xxxx)"); return true;
	case RSX_FP_OPCODE_FLR: SetDst("floor($0)"); return true;
	case RSX_FP_OPCODE_FRC: SetDst(getFunction(FUNCTION::FUNCTION_FRACT)); return true;
	case RSX_FP_OPCODE_LIT:
		SetDst("_builtin_lit($0)");
		properties.has_lit_op = true;
		return true;
	case RSX_FP_OPCODE_LIF: SetDst("$Ty(1.0, $0.y, ($0.y > 0 ? pow(2.0, $0.w) : 0.0), 1.0)", OPFLAGS::skip_type_cast); return true;
	case RSX_FP_OPCODE_LRP: SetDst("$Ty($2 * (1 - $0) + $1 * $0)", OPFLAGS::skip_type_cast); return true;
	case RSX_FP_OPCODE_LG2: SetDst("_builtin_log2($0.x).xxxx"); return true;
		// Pack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	case RSX_FP_OPCODE_PK2: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packHalf2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PK4: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packSnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_PK16: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packSnorm2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PKG:
		// Should be similar to PKB but with gamma correction, see description of PK4UBG in khronos page
	case RSX_FP_OPCODE_PKB: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packUnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_SIN: SetDst("sin($0.xxxx)"); return true;
	}
	return false;
}

bool FragmentProgramDecompiler::handle_tex_srb(u32 opcode)
{
	switch (opcode)
	{
	case RSX_FP_OPCODE_DDX: SetDst(getFunction(FUNCTION::FUNCTION_DFDX)); return true;
	case RSX_FP_OPCODE_DDY: SetDst(getFunction(FUNCTION::FUNCTION_DFDY)); return true;
	case RSX_FP_OPCODE_NRM: SetDst("normalize($0.xyz)"); return true;
	case RSX_FP_OPCODE_BEM: SetDst("$0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw"); return true;
	case RSX_FP_OPCODE_TEXBEM:
		//Untested, should be x2d followed by TEX
		AddX2d();
		AddCode(Format("x2d = $0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw;", true));
	case RSX_FP_OPCODE_TEX:
		AddTex();
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			if (m_prog.shadow_textures & (1 << dst.tex_num))
			{
				m_shadow_sampled_textures |= (1 << dst.tex_num);
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SHADOW2D) + ".xxxx");
				return true;
			}
			if (m_prog.redirected_textures & (1 << dst.tex_num))
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_DEPTH_RGBA));
			else
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D));
			m_2d_sampled_textures |= (1 << dst.tex_num);
			return true;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE3D));
			return true;
		}
		return false;
	case RSX_FP_OPCODE_TXPBEM:
		//Untested, should be x2d followed by TXP
		AddX2d();
		AddCode(Format("x2d = $0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw;", true));
	case RSX_FP_OPCODE_TXP:
		AddTex();
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			//Note shadow comparison only returns a true/false result!
			if (m_prog.shadow_textures & (1 << dst.tex_num))
			{
				m_shadow_sampled_textures |= (1 << dst.tex_num);
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ) + ".xxxx");
			}
			else
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_PROJ));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_PROJ));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_PROJ));
			return true;
		}
		return false;
	case RSX_FP_OPCODE_TXD:
		AddTex();
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_GRAD));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_GRAD));
			m_2d_sampled_textures |= (1 << dst.tex_num);
			return true;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_GRAD));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_GRAD));
			return true;
		}
		return false;
	case RSX_FP_OPCODE_TXB:
		AddTex();
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_BIAS));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_BIAS));
			m_2d_sampled_textures |= (1 << dst.tex_num);
			return true;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_BIAS));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_BIAS));
			return true;
		}
		return false;
	case RSX_FP_OPCODE_TXL:
		AddTex();
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_LOD));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE2D_LOD));
			m_2d_sampled_textures |= (1 << dst.tex_num);
			return true;
		case rsx::texture_dimension_extended::texture_dimension_cubemap:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLECUBE_LOD));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_3d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE3D_LOD));
			return true;
		}
		return false;
	// Unpack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	case RSX_FP_OPCODE_UP2: SetDst("unpackHalf2x16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UP4: SetDst("unpackSnorm4x8(floatBitsToUint($0.x))"); return true;
	case RSX_FP_OPCODE_UP16: SetDst("unpackSnorm2x16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UPG:
	// Same as UPB with gamma correction
	case RSX_FP_OPCODE_UPB: SetDst("(unpackUnorm4x8(floatBitsToUint($0.x)))"); return true;
	}
	return false;
};

std::string FragmentProgramDecompiler::Decompile()
{
	auto data = (be_t<u32>*) m_prog.addr;
	m_size = 0;
	m_location = 0;
	m_loop_count = 0;
	m_code_level = 1;

	enum
	{
		FORCE_NONE,
		FORCE_SCT,
		FORCE_SCB,
	};

	int forced_unit = FORCE_NONE;

	while (true)
	{
		for (auto found = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
			found != m_end_offsets.end();
			found = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(found);
			m_code_level--;
			AddCode("}");
			m_loop_count--;
		}

		for (auto found = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
			found != m_else_offsets.end();
			found = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size))
		{
			m_else_offsets.erase(found);
			m_code_level--;
			AddCode("}");
			AddCode("else");
			AddCode("{");
			m_code_level++;
		}

		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		m_offset = 4 * sizeof(u32);
		opflags = 0;

		const u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

		auto SIP = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_BRK:
				if (m_loop_count) AddFlowOp("break");
				else LOG_ERROR(RSX, "BRK opcode found outside of a loop");
				break;
			case RSX_FP_OPCODE_CAL:
				LOG_ERROR(RSX, "Unimplemented SIP instruction: CAL");
				break;
			case RSX_FP_OPCODE_FENCT:
				AddCode("//FENCT");
				forced_unit = FORCE_SCT;
				break;
			case RSX_FP_OPCODE_FENCB:
				AddCode("//FENCB");
				forced_unit = FORCE_SCB;
				break;
			case RSX_FP_OPCODE_IFE:
				AddCode("if($cond)");
				if (src2.end_offset != src1.else_offset)
					m_else_offsets.push_back(src1.else_offset << 2);
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
				break;
			case RSX_FP_OPCODE_LOOP:
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					AddCode(fmt::format("//$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //LOOP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
				}
				else
				{
					AddCode(fmt::format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) //LOOP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment));
					m_loop_count++;
					m_end_offsets.push_back(src2.end_offset << 2);
					AddCode("{");
					m_code_level++;
				}
				break;
			case RSX_FP_OPCODE_REP:
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					AddCode(fmt::format("//$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //REP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
				}
				else
				{
					AddCode(fmt::format("if($cond) for(int i%u = %u; i%u < %u; i%u += %u) //REP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment));
					m_loop_count++;
					m_end_offsets.push_back(src2.end_offset << 2);
					AddCode("{");
					m_code_level++;
				}
				break;
			case RSX_FP_OPCODE_RET:
				AddFlowOp("return");
				break;

			default:
				return false;
			}

			return true;
		};

		switch (opcode)
		{
		case RSX_FP_OPCODE_NOP: break;
		case RSX_FP_OPCODE_KIL:
			properties.has_discard_op = true;
			AddFlowOp("discard");
			break;

		default:
			int prev_force_unit = forced_unit;

			// Some instructions do not respect forced unit
			// Tested with Tales of Vesperia
			if (SIP()) break;
			if (handle_tex_srb(opcode)) break;

			// FENCT/FENCB do not actually reject instructions if they dont match the forced unit
			// Looks like they are optimization hints and not hard-coded forced paths
			if (handle_sct_scb(opcode)) break;
			forced_unit = FORCE_NONE;

			LOG_ERROR(RSX, "Unknown/illegal instruction: 0x%x (forced unit %d)", opcode, prev_force_unit);
			break;
		}

		m_size += m_offset;

		if (dst.end) break;

		verify(HERE), m_offset % sizeof(u32) == 0;
		data += m_offset / sizeof(u32);
	}

	while (m_code_level > 1)
	{
		LOG_ERROR(RSX, "Hanging block found at end of shader. Malformed shader?");

		m_code_level--;
		AddCode("}");
	}

	// flush m_code_level
	m_code_level = 1;
	std::string m_shader = BuildCode();
	main.clear();
	//	m_parr.params.clear();
	return m_shader;
}
