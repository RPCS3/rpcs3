#include "stdafx.h"
#include "FragmentProgramDecompiler.h"

#include <algorithm>

namespace rsx
{
	namespace fragment_program
	{
		static const std::string reg_table[] =
		{
			"wpos",
			"diff_color", "spec_color",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
			"ssa"
		};
	}
}

using namespace rsx::fragment_program;

// SIMD vector lanes
enum VectorLane : u8
{
	X = 0,
	Y = 1,
	Z = 2,
	W = 3,
};

FragmentProgramDecompiler::FragmentProgramDecompiler(const RSXFragmentProgram &prog, u32& size)
	: m_size(size)
	, m_prog(prog)
	, m_ctrl(prog.ctrl)
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
			rsx_log.error("Bad scale: %d", u32{ src1.scale });
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
		if (dst.fp16 && device_props.has_native_half_support && !(flags & OPFLAGS::skip_type_cast))
		{
			// Cast to native data type
			code = ClampValue(code, RSX_FP_PRECISION_HALF);
		}

		if (dst.saturate)
		{
			code = ClampValue(code, RSX_FP_PRECISION_SATURATE);
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
			case RSX_FP_OPCODE_FRC:
			case RSX_FP_OPCODE_LIT:
			case RSX_FP_OPCODE_LIF:
			case RSX_FP_OPCODE_LG2:
				break;
			case RSX_FP_OPCODE_MOV:
				// NOTE: Sometimes varying inputs from VS are out of range so do not exempt any input types, unless fp16 (Naruto UNS)
				if (dst.fp16 && src0.fp16 && src0.reg_type == RSX_FP_REGISTER_TYPE_TEMP)
					break;
				[[fallthrough]];
			default:
			{
				// fp16 precsion flag on f32 register; ignore
				if (dst.prec == 1 && !dst.fp16)
					break;

				// Native type already has fp16 clamped (input must have been cast)
				if (dst.prec == 1 && dst.fp16 && device_props.has_native_half_support)
					break;

				// clamp value to allowed range
				code = ClampValue(code, dst.prec);
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

	const std::string dest = AddReg(dst.dest_reg, !!dst.fp16) + "$m";
	const std::string decoded_dest = Format(dest);

	AddCodeCond(decoded_dest, code);
	//AddCode("$ifcond " + dest + code + (append_mask ? "$m;" : ";"));

	if (dst.set_cond)
	{
		AddCode(m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + dest + ";");
	}

	const u32 reg_index = dst.fp16 ? (dst.dest_reg >> 1) : dst.dest_reg;
	ensure(reg_index < temp_registers.size());

	if (dst.opcode == RSX_FP_OPCODE_MOV &&
		src0.reg_type == RSX_FP_REGISTER_TYPE_TEMP &&
		src0.tmp_reg_index == reg_index)
	{
		// The register did not acquire any new data
		// Common in code with structures like r0.xy = r0.xy
		// Unsure why such code would exist, maybe placeholders for dynamically generated shader code?
		if (decoded_dest == Format(code))
		{
			return;
		}
	}

	temp_registers[reg_index].tag(dst.dest_reg, !!dst.fp16, dst.mask_x, dst.mask_y, dst.mask_z, dst.mask_w);
}

void FragmentProgramDecompiler::AddFlowOp(const std::string& code)
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

std::string FragmentProgramDecompiler::GetMask() const
{
	std::string ret;
	ret.reserve(5);

	static constexpr std::string_view dst_mask = "xyzw";

	ret += '.';
	if (dst.mask_x) ret += dst_mask[0];
	if (dst.mask_y) ret += dst_mask[1];
	if (dst.mask_z) ret += dst_mask[2];
	if (dst.mask_w) ret += dst_mask[3];

	return ret == "."sv || ret == ".xyzw"sv ? "" : (ret);
}

std::string FragmentProgramDecompiler::AddReg(u32 index, bool fp16)
{
	const std::string type_name = (fp16 && device_props.has_native_half_support)? getHalfTypeName(4) : getFloatTypeName(4);
	const std::string reg_name = std::string(fp16 ? "h" : "r") + std::to_string(index);

	return m_parr.AddParam(PF_PARAM_NONE, type_name, reg_name, type_name + "(0.)");
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

	auto data = reinterpret_cast<be_t<u32>*>(reinterpret_cast<uptr>(m_prog.get_data()) + m_size + 4 * sizeof(u32));
	m_offset = 2 * 4 * sizeof(u32);
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);

	const auto var = fmt::format("%s(%f, %f, %f, %f)", type, std::bit_cast<f32>(x), std::bit_cast<f32>(y), std::bit_cast<f32>(z), std::bit_cast<f32>(w));
	return m_parr.AddParam(PF_PARAM_UNIFORM, type, name, var);
}

std::string FragmentProgramDecompiler::AddTex()
{
	properties.has_tex_op = true;

	std::string sampler;
	switch (m_prog.get_texture_dimension(dst.tex_num))
	{
	case rsx::texture_dimension_extended::texture_dimension_1d:
		properties.has_tex1D = true;
		sampler = "sampler1D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_cubemap:
		properties.has_tex3D = true;
		sampler = "samplerCube";
		break;
	case rsx::texture_dimension_extended::texture_dimension_2d:
		properties.has_tex2D = true;
		sampler = "sampler2D";
		break;
	case rsx::texture_dimension_extended::texture_dimension_3d:
		properties.has_tex3D = true;
		sampler = "sampler3D";
		break;
	}

	opflags |= OPFLAGS::texture_ref;
	return m_parr.AddParam(PF_PARAM_UNIFORM, sampler, std::string("tex") + std::to_string(dst.tex_num));
}

std::string FragmentProgramDecompiler::AddX2d()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "x2d", getFloatTypeName(4) + "(0.)");
}

std::string FragmentProgramDecompiler::ClampValue(const std::string& code, u32 precision)
{
	// FP16 is expected to overflow a lot easier at 0+-65504
	// FP32 can still work up to 0+-3.4E38
	// See http://http.download.nvidia.com/developer/Papers/2005/FP_Specials/FP_Specials.pdf

	if (precision > 1 && precision < 5)
	{
		// Define precision_clamp
		properties.has_clamp = true;
	}

	switch (precision)
	{
	case RSX_FP_PRECISION_REAL:
		// Full 32-bit precision
		break;
	case RSX_FP_PRECISION_HALF:
		return "clamp16(" + code + ")";
	case RSX_FP_PRECISION_FIXED12:
		return "precision_clamp(" + code + ", -2., 2.)";
	case RSX_FP_PRECISION_FIXED9:
		return "precision_clamp(" + code + ", -1., 1.)";
	case RSX_FP_PRECISION_SATURATE:
		return "precision_clamp(" + code + ", 0., 1.)";
	case RSX_FP_PRECISION_UNKNOWN:
		// Doesn't seem to do anything to the input from hw tests, same as 0
		break;
	default:
		rsx_log.error("Unexpected precision modifier (%d)\n", precision);
		break;
	}

	return code;
}

bool FragmentProgramDecompiler::DstExpectsSca() const
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
	const std::pair<std::string_view, std::function<std::string()>> repl_list[] =
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
		{ "$float4", [this]() -> std::string { return getFloatTypeName(4); } },
		{ "$float3", [this]() -> std::string { return getFloatTypeName(3); } },
		{ "$float2", [this]() -> std::string { return getFloatTypeName(2); } },
		{ "$float_t", [this]() -> std::string { return getFloatTypeName(1); } },
		{ "$half4", [this]() -> std::string { return getHalfTypeName(4); } },
		{ "$half3", [this]() -> std::string { return getHalfTypeName(3); } },
		{ "$half2", [this]() -> std::string { return getHalfTypeName(2); } },
		{ "$half_t", [this]() -> std::string { return getHalfTypeName(1); } },
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
			std::pair<std::string_view, std::string> repl[] = { { "$0", "x2d" } };
			std::string result = fmt::replace_all(code, repl);

			return fmt::replace_all(result, repl_list);
		}
		}
	}

	return fmt::replace_all(code, repl_list);
}

std::string FragmentProgramDecompiler::GetRawCond()
{
	static constexpr std::string_view f = "xyzw";
	const auto zero = getFloatTypeName(4) + "(0.)";

	std::string swizzle, cond;
	swizzle.reserve(5);
	swizzle += '.';
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];

	if (swizzle == ".xyzw"sv)
	{
		swizzle.clear();
	}

	if (src0.exec_if_gr && src0.exec_if_eq)
		cond = compareFunction(COMPARE::SGE, AddCond() + swizzle, zero);
	else if (src0.exec_if_lt && src0.exec_if_eq)
		cond = compareFunction(COMPARE::SLE, AddCond() + swizzle, zero);
	else if (src0.exec_if_gr && src0.exec_if_lt)
		cond = compareFunction(COMPARE::SNE, AddCond() + swizzle, zero);
	else if (src0.exec_if_gr)
		cond = compareFunction(COMPARE::SGT, AddCond() + swizzle, zero);
	else if (src0.exec_if_lt)
		cond = compareFunction(COMPARE::SLT, AddCond() + swizzle, zero);
	else //if(src0.exec_if_eq)
		cond = compareFunction(COMPARE::SEQ, AddCond() + swizzle, zero);

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

void FragmentProgramDecompiler::AddCodeCond(const std::string& lhs, const std::string& rhs)
{
	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		AddCode(lhs + " = " + rhs + ";");
		return;
	}

	if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		AddCode("//" + lhs + " = " + rhs + ";");
		return;
	}

	std::string src_prefix;
	if (device_props.has_native_half_support && !this->dst.fp16)
	{
		// Target is not fp16 but src might be
		// Usually vecX a = f16vecX b is fine, but causes operator overload issues when used in a mix/lerp function
		// mix(f32, f16, bvec) causes compiler issues
		// NOTE: If dst is fp16 the src will already have been cast to match so this is not a problem in that case

		bool src_is_fp16 = false;
		if ((opflags & (OPFLAGS::texture_ref | OPFLAGS::src_cast_f32)) == 0 &&
			rhs.find("$0") != umax)
		{
			// Texture sample operations are full-width and are exempt
			src_is_fp16 = (src0.fp16 && src0.reg_type == RSX_FP_REGISTER_TYPE_TEMP);

			if (src_is_fp16 && rhs.find("$1") != umax)
			{
				// References operand 1
				src_is_fp16 = (src1.fp16 && src1.reg_type == RSX_FP_REGISTER_TYPE_TEMP);

				if (src_is_fp16 && rhs.find("$2") != umax)
				{
					// References operand 2
					src_is_fp16 = (src2.fp16 && src2.reg_type == RSX_FP_REGISTER_TYPE_TEMP);
				}
			}
		}

		if (src_is_fp16)
		{
			// LHS argument is of native half type, need to cast to proper type!
			if (rhs[0] != '(')
			{
				// Upcast inputs to processing function instead
				opflags |= OPFLAGS::src_cast_f32;
			}
			else
			{
				// No need to add explicit casts all over the place, just cast the result once
				src_prefix = "$Ty";
			}
		}
	}

	// NOTE: x = _select(x, y, cond) is equivalent to x = cond? y : x;
	const auto dst_var = ShaderVariable(lhs);
	const auto raw_cond = dst_var.add_mask(GetRawCond());
	const auto cond = dst_var.match_size(raw_cond);
	AddCode(lhs + " = _select(" + lhs + ", " + src_prefix + rhs + ", " + cond + ");");
}

template<typename T> std::string FragmentProgramDecompiler::GetSRC(T src)
{
	std::string ret;
	u32 precision_modifier = 0;

	if constexpr (std::is_same_v<T, SRC0>)
	{
		precision_modifier = src1.src0_prec_mod;
	}
	else if constexpr (std::is_same_v<T, SRC1>)
	{
		precision_modifier = src1.src1_prec_mod;
	}
	else if constexpr (std::is_same_v<T, SRC2>)
	{
		precision_modifier = src1.src2_prec_mod;
	}

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
		else if (precision_modifier == RSX_FP_PRECISION_HALF)
		{
			// clamp16() is not a cheap operation when emulated; avoid at all costs
			precision_modifier = RSX_FP_PRECISION_REAL;
		}

		ret += AddReg(src.tmp_reg_index, src.fp16);

		if (opflags & OPFLAGS::src_cast_f32 && src.fp16 && device_props.has_native_half_support)
		{
			// Upconvert if there is a chance for ambiguity
			ret = getFloatTypeName(4) + "(" + ret + ")";
		}

		break;

	case RSX_FP_REGISTER_TYPE_INPUT:
	{
		// NOTE: Hw testing showed the following:
		// 1. Reading from registers 1 and 2 (COL0 and COL1) is clamped to (0, 1)
		// 2. Reading from registers 4-12 (inclusive) is not clamped, but..
		// 3. If the texcoord control mask is enabled, the last 2 values are always 0 and hpos.w!
		// 4. [A0 + N] addressing can be applied to dynamically sample texture coordinates.
		// - This is explained in NV_fragment_program2 specification page, Fragment Attributes section.
		// - There is no instruction that writes to the address register directly, it is supposed to be the loop counter!
		u32 register_id = src2.use_index_reg ? (src2.addr_reg + 4) : dst.src_attr_reg_num;
		const std::string reg_var = (register_id < std::size(reg_table))? reg_table[register_id] : "unk";
		bool insert = true;

		if (reg_var == "unk")
		{
			m_is_valid_ucode = false;
			insert = false;
		}

		if (src2.use_index_reg && m_loop_count)
		{
			// Dynamically load the input
			register_id = 0xFF;
		}

		switch (register_id)
		{
		case 0x00:
		{
			// WPOS
			ret += reg_table[0];
			insert = false;
			break;
		}
		case 0x01:
		case 0x02:
		{
			// COL0, COL1
			ret += "_saturate(" + reg_var + ")";
			precision_modifier = RSX_FP_PRECISION_REAL;
			break;
		}
		case 0x03:
		{
			// FOGC
			ret += reg_var;
			break;
		}
		case 0x4:
		case 0x5:
		case 0x6:
		case 0x7:
		case 0x8:
		case 0x9:
		case 0xA:
		case 0xB:
		case 0xC:
		case 0xD:
		{
			// TEX0 - TEX9
			// Texcoord 2d mask seems to reset the last 2 arguments to 0 and w if set
			const u8 texcoord = u8(register_id) - 4;
			if (m_prog.texcoord_is_point_coord(texcoord))
			{
				// Point sprite coord generation. Stacks with the 2D override mask.
				if (m_prog.texcoord_is_2d(texcoord))
				{
					ret += getFloatTypeName(4) + "(gl_PointCoord, 0., in_w)";
					properties.has_w_access = true;
				}
				else
				{
					ret += getFloatTypeName(4) + "(gl_PointCoord, 1., 0.)";
				}
			}
			else if (src2.perspective_corr)
			{
				// Perspective correct flag multiplies the result by 1/w
				if (m_prog.texcoord_is_2d(texcoord))
				{
					ret += getFloatTypeName(4) + "(" + reg_var + ".xy * gl_FragCoord.w, 0., 1.)";
				}
				else
				{
					ret += "(" + reg_var + " * gl_FragCoord.w)";
				}
			}
			else
			{
				if (m_prog.texcoord_is_2d(texcoord))
				{
					ret += getFloatTypeName(4) + "(" + reg_var + ".xy, 0., in_w)";
					properties.has_w_access = true;
				}
				else
				{
					ret += reg_var;
				}
			}
			break;
		}
		case 0xFF:
		{
			if (m_loop_count > 1)
			{
				// Afaik there is only one address/loop register on NV40
				rsx_log.error("Nested loop with indexed load was detected. Report this to developers!");
			}

			if (m_prog.texcoord_control_mask)
			{
				// This would require more work if it exists. It cannot be determined at compile time and has to be part of _indexed_load() subroutine.
				rsx_log.error("Indexed load with control override mask detected. Report this to developers!");
			}

			const auto load_cmd = fmt::format("_indexed_load(i%u + %u)", m_loop_count - 1, src2.addr_reg);
			properties.has_dynamic_register_load = true;
			insert = false;

			if (src2.perspective_corr)
			{
				ret += "(" + load_cmd + " * gl_FragCoord.w)";
			}
			else
			{
				ret += load_cmd;
			}
			break;
		}
		default:
		{
			// SSA (winding direction register)
			// UNK
			if (reg_var == "unk")
			{
				rsx_log.error("Bad src reg num: %d", u32{ register_id });
			}

			ret += reg_var;
			precision_modifier = RSX_FP_PRECISION_REAL;
			break;
		}
		}

		if (insert)
		{
			m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), reg_var);
		}

		properties.in_register_mask |= (1 << register_id);
	}
	break;

	case RSX_FP_REGISTER_TYPE_CONSTANT:
		ret += AddConst();
		break;

	case RSX_FP_REGISTER_TYPE_UNKNOWN: // ??? Used by a few games, what is it?
		rsx_log.error("Src type 3 used, opcode=0x%X, dst=0x%X s0=0x%X s1=0x%X s2=0x%X",
				dst.opcode, dst.HEX, src0.HEX, src1.HEX, src2.HEX);

		// This is not some special type, it is a bug indicating memory corruption
		// Shaders that are even slightly off do not execute on realhw to any meaningful degree
		m_is_valid_ucode = false;
		ret += "src3";
		precision_modifier = RSX_FP_PRECISION_REAL;
		break;

	default:
		rsx_log.fatal("Bad src type %d", u32{ src.reg_type });
		break;
	}

	static constexpr std::string_view f = "xyzw";

	std::string swizzle;
	swizzle.reserve(5);
	swizzle += '.';
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if (swizzle != ".xyzw"sv)
	{
		ret += swizzle;
	}

	// Warning: Modifier order matters. e.g neg should be applied after precision clamping (tested with Naruto UNS)
	if (src.abs) ret = "abs(" + ret + ")";
	if (precision_modifier) ret = ClampValue(ret, precision_modifier);
	if (src.neg) ret = "-" + ret;

	return ret;
}

std::string FragmentProgramDecompiler::BuildCode()
{
	// Shader validation
	// Shader must at least write to one output for the body to be considered valid

	const bool fp16_out = !(m_ctrl & CELL_GCM_SHADER_CONTROL_32_BITS_EXPORTS);
	const std::string float4_type = (fp16_out && device_props.has_native_half_support)? getHalfTypeName(4) : getFloatTypeName(4);
	const std::string init_value = float4_type + "(0.)";
	std::array<std::string, 4> output_register_names;
	std::array<u32, 4> ouput_register_indices = { 0, 2, 3, 4 };

	// Holder for any "cleanup" before exiting main
	std::stringstream main_epilogue;

	// Check depth export
	if (m_ctrl & CELL_GCM_SHADER_CONTROL_DEPTH_EXPORT)
	{
		// Hw tests show that the depth export register is default-initialized to 0 and not wpos.z!!
		m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "r1", init_value);

		auto& r1 = temp_registers[1];
		if (r1.requires_gather(VectorLane::Z))
		{
			// r1.zw was not written to
			properties.has_gather_op = true;
			main_epilogue << "	r1.z = " << float4_type << r1.gather_r() << ".z;\n";

			// Emit debug warning. Useful to diagnose regressions, but should be removed in future.
			rsx_log.warning("ROP reads from shader depth without writing to it. Final value will be gathered.");
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

	for (u32 n = 0; n < 4; ++n)
	{
		const auto& reg_name = output_register_names[n];
		if (!m_parr.HasParam(PF_PARAM_NONE, float4_type, reg_name))
		{
			m_parr.AddParam(PF_PARAM_NONE, float4_type, reg_name, init_value);
		}

		if (n >= m_prog.mrt_buffers_count)
		{
			// Skip gather
			continue;
		}

		const auto block_index = ouput_register_indices[n];
		auto& r = temp_registers[block_index];

		if (fp16_out)
		{
			// Check if we need a split/extract op
			if (r.requires_split(0))
			{
				main_epilogue << "	" << reg_name << " = " << float4_type << r.split_h0() << ";\n";

				// Emit debug warning. Useful to diagnose regressions, but should be removed in future.
				rsx_log.warning("ROP reads from %s without writing to it. Final value will be extracted from the 32-bit register.", reg_name);
			}

			continue;
		}

		if (!r.requires_gather128())
		{
			// Nothing to do
			continue;
		}

		// We need to gather the data from existing registers
		main_epilogue << "	" << reg_name << " = " << float4_type << r.gather_r() << ";\n";
		properties.has_gather_op = true;

		// Emit debug warning. Useful to diagnose regressions, but should be removed in future.
		rsx_log.warning("ROP reads from %s without writing to it. Final value will be gathered.", reg_name);
	}

	if (properties.has_dynamic_register_load)
	{
		// Since the registers will be loaded dynamically, declare all of them
		for (int i = 0; i < 10; ++i)
		{
			m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), reg_table[i + 4]);
		}
	}

	std::stringstream OS;

	if (!m_is_valid_ucode)
	{
		// If the code is broken, do not compile. Simply NOP main and write empty outputs
		insertHeader(OS);
		OS << "\n";
		OS << "void main()\n";
		OS << "{\n";
		OS << "#if 0\n";
		OS << main << "\n";
		OS << "#endif\n";
		OS << "	discard;\n";
		OS << "}\n";

		// Don't consume any args
		m_parr.Clear();
		return OS.str();
	}

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

	std::string float4 = getFloatTypeName(4);
	const bool glsl = float4 == "vec4";

	if (properties.has_clamp)
	{
		std::string precision_func =
		"$float4 precision_clamp($float4 x, float _min, float _max)\n"
		"{\n"
		"	// Treat NaNs as 0\n"
		"	bvec4 nans = isnan(x);\n"
		"	x = _select(x, $float4(0.), nans);\n"
		"	return clamp(x, _min, _max);\n"
		"}\n\n";

		if (device_props.has_native_half_support)
		{
			precision_func +=
			"$half4 precision_clamp($half4 x, float _min, float _max)\n"
			"{\n"
			"	// Treat NaNs as 0\n"
			"	bvec4 nans = isnan(x);\n"
			"	x = _select(x, $half4(0.), nans);\n"
			"	return clamp(x, $half_t(_min), $half_t(_max));\n"
			"}\n\n";
		}

		OS << Format(precision_func);
	}

	if (!device_props.has_native_half_support)
	{
		// Accurate float to half clamping (preserves IEEE-754 NaN)
		std::string clamp_func;
		if (glsl)
		{
			clamp_func +=
			"vec2 clamp16(vec2 val){ return unpackHalf2x16(packHalf2x16(val)); }\n"
			"vec4 clamp16(vec4 val){ return vec4(clamp16(val.xy), clamp16(val.zw)); }\n\n";
		}
		else
		{
			clamp_func +=
			"$float4 clamp16($float4 x)\n"
			"{\n"
			"	if (!isnan(x.x) && !isinf(x.x)) x.x = clamp(x.x, -65504., +65504.);\n"
			"	if (!isnan(x.x) && !isinf(x.x)) x.x = clamp(x.x, -65504., +65504.);\n"
			"	if (!isnan(x.x) && !isinf(x.x)) x.x = clamp(x.x, -65504., +65504.);\n"
			"	if (!isnan(x.x) && !isinf(x.x)) x.x = clamp(x.x, -65504., +65504.);\n"
			"	return x;\n"
			"}\n\n";
		}

		OS << Format(clamp_func);
	}
	else
	{
		// Define raw casts from f32->f16
		OS <<
		"#define clamp16(x) " << getHalfTypeName(4) << "(x)\n";
	}

	OS <<
	"#define _builtin_lit lit_legacy\n"
	"#define _builtin_log2 log2\n"
	"#define _builtin_normalize(x) (length(x) > 0? normalize(x) : x)\n" // HACK!! Workaround for some games that generate NaNs unless texture filtering exactly matches PS3 (BFBC)
	"#define _builtin_sqrt(x) sqrt(abs(x))\n"
	"#define _builtin_rcp(x) (1. / x)\n"
	"#define _builtin_rsq(x) (1. / _builtin_sqrt(x))\n"
	"#define _builtin_div(x, y) (x / y)\n";

	if (device_props.has_low_precision_rounding)
	{
		// NVIDIA has terrible rounding errors interpolating constant values across vertices with different w
		// PS3 games blindly rely on interpolating a constant to not change the values
		// Calling floor/equality will fail randomly causing a moire pattern
		OS <<
		"#define _builtin_floor(x) floor(x + 0.000001)\n\n";
	}
	else
	{
		OS <<
		"#define _builtin_floor floor\n\n";
	}

	if (properties.has_pkg)
	{
		OS <<
		"vec4 _builtin_pkg(const in vec4 value)\n"
		"{\n"
		"	vec4 convert = linear_to_srgb(value);\n"
		"	return uintBitsToFloat(packUnorm4x8(convert)).xxxx;\n"
		"}\n\n";
	}

	if (properties.has_upg)
	{
		OS <<
		"vec4 _builtin_upg(const in float value)\n"
		"{\n"
		"	vec4 raw = unpackUnorm4x8(floatBitsToUint(value));\n"
		"	return srgb_to_linear(raw);\n"
		"}\n\n";
	}

	if (properties.has_divsq)
	{
		// Define RSX-compliant DIVSQ
		// If the numerator is 0, the result is always 0 even if the denominator is 0
		// NOTE: This operation is component-wise and cannot be accelerated with lerp/mix because these always return NaN if any of the choices is NaN
		std::string divsq_func =
			"$float4 _builtin_divsq($float4 a, float b)\n"
			"{\n"
			"	$float4 tmp = a / _builtin_sqrt(b);\n"
			"	$float4 choice = abs(a);\n";

		if (glsl)
		{
			divsq_func +=
				"	return _select(a, tmp, greaterThan(choice, vec4(0.)));\n";
		}
		else
		{
			divsq_func +=
				"	if (choice.x > 0.) a.x = tmp.x;\n"
				"	if (choice.y > 0.) a.y = tmp.y;\n"
				"	if (choice.z > 0.) a.z = tmp.z;\n"
				"	if (choice.w > 0.) a.w = tmp.w;\n"
				"	return a;\n";
		}

		divsq_func +=
			"}\n\n";

		OS << Format(divsq_func);
	}

	// Declare register gather/merge if needed
	if (properties.has_gather_op)
	{
		std::string float2 = getFloatTypeName(2);

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

	if (properties.has_dynamic_register_load)
	{
		OS <<
		"vec4 _indexed_load(int index)\n"
		"{\n"
		"	switch (index)\n"
		"	{\n"
		"		case 0: return tc0;\n"
		"		case 1: return tc1;\n"
		"		case 2: return tc2;\n"
		"		case 3: return tc3;\n"
		"		case 4: return tc4;\n"
		"		case 5: return tc5;\n"
		"		case 6: return tc6;\n"
		"		case 7: return tc7;\n"
		"		case 8: return tc8;\n"
		"		case 9: return tc9;\n"
		"	}\n"
		"	return vec4(0., 0., 0., 1.);\n"
		"}\n\n";
	}

	insertMainStart(OS);
	OS << main << std::endl;

	if (const auto epilogue = main_epilogue.str(); !epilogue.empty())
	{
		OS << "	// Epilogue\n";
		OS << epilogue << std::endl;
	}
	insertMainEnd(OS);

	return OS.str();
}

bool FragmentProgramDecompiler::handle_sct_scb(u32 opcode)
{
	// Compliance notes based on HW tests:
	// DIV is IEEE compliant as is MUL, LG2, EX2. LG2 with negative input returns NaN as expected.
	// DIVSQ is not compliant. Result is 0 if numerator is 0 regardless of denominator
	// RSQ(0) and RCP(0) return INF as expected
	// RSQ ignores the sign of the inputs (Metro Last Light, GTA4)
	// SAT modifier flushes NaNs to 0
	// Some games that rely on broken DIVSQ behaviour include Dark Souls II and Super Puzzle Fighter II Turbo HD Remix

	switch (opcode)
	{
	case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); return true;
	case RSX_FP_OPCODE_DIV: SetDst("_builtin_div($0, $1.x)"); return true;
	case RSX_FP_OPCODE_DIVSQ:
		SetDst("_builtin_divsq($0, $1.x)");
		properties.has_divsq = true;
		return true;
	case RSX_FP_OPCODE_DP2: SetDst(getFunction(FUNCTION::DP2), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP3: SetDst(getFunction(FUNCTION::DP3), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP4: SetDst(getFunction(FUNCTION::DP4), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_DP2A: SetDst(getFunction(FUNCTION::DP2A), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_MAD: SetDst("fma($0, $1, $2)", OPFLAGS::src_cast_f32); return true;
	case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)", OPFLAGS::src_cast_f32); return true;
	case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)", OPFLAGS::src_cast_f32); return true;
	case RSX_FP_OPCODE_MOV: SetDst("$0"); return true;
	case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); return true;
	case RSX_FP_OPCODE_RCP: SetDst("_builtin_rcp($0.x).xxxx"); return true;
	case RSX_FP_OPCODE_RSQ: SetDst("_builtin_rsq($0.x).xxxx"); return true;
	case RSX_FP_OPCODE_SEQ: SetDst("$Ty(" + compareFunction(COMPARE::SEQ, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SFL: SetDst(getFunction(FUNCTION::SFL), OPFLAGS::skip_type_cast); return true;
	case RSX_FP_OPCODE_SGE: SetDst("$Ty(" + compareFunction(COMPARE::SGE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SGT: SetDst("$Ty(" + compareFunction(COMPARE::SGT, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SLE: SetDst("$Ty(" + compareFunction(COMPARE::SLE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SLT: SetDst("$Ty(" + compareFunction(COMPARE::SLT, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_SNE: SetDst("$Ty(" + compareFunction(COMPARE::SNE, "$0", "$1") + ")", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_STR: SetDst(getFunction(FUNCTION::STR), OPFLAGS::skip_type_cast); return true;

	// SCB-only ops
	case RSX_FP_OPCODE_COS: SetDst("cos($0.xxxx)"); return true;
	case RSX_FP_OPCODE_DST: SetDst("$Ty(1.0, $0.y * $1.y, $0.z, $1.w)", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_REFL: SetDst(getFunction(FUNCTION::REFL), OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_EX2: SetDst("exp2($0.xxxx)"); return true;
	case RSX_FP_OPCODE_FLR: SetDst("_builtin_floor($0)"); return true;
	case RSX_FP_OPCODE_FRC: SetDst(getFunction(FUNCTION::FRACT)); return true;
	case RSX_FP_OPCODE_LIT:
		SetDst("_builtin_lit($0)");
		properties.has_lit_op = true;
		return true;
	case RSX_FP_OPCODE_LIF: SetDst("$Ty(1.0, $0.y, ($0.y > 0 ? exp2($0.w) : 0.0), 1.0)", OPFLAGS::op_extern); return true;
	case RSX_FP_OPCODE_LRP: SetDst("$Ty($2 * (1 - $0) + $1 * $0)", OPFLAGS::skip_type_cast); return true;
	case RSX_FP_OPCODE_LG2: SetDst("_builtin_log2($0.x).xxxx"); return true;
	// Pack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	// PK2 = PK2H (2 16-bit floats)
	// PK16 = PK2US (2 unsigned 16-bit scalars)
	// PK4 = PK4B (4 signed 8-bit scalars)
	// PKB = PK4UB (4 unsigned 8-bit scalars)
	// PK16/UP16 behavior confirmed by Saints Row: Gat out of Hell, ARGB8 -> X16Y16 conversion relies on this to render the wings
	case RSX_FP_OPCODE_PK2: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packHalf2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PK4: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packSnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_PK16: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packUnorm2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PKG:
		// Should be similar to PKB but with gamma correction, see description of PK4UBG in khronos page
		properties.has_pkg = true;
		SetDst("_builtin_pkg($0)");
		return true;
	case RSX_FP_OPCODE_PKB: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packUnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_SIN: SetDst("sin($0.xxxx)"); return true;
	}
	return false;
}

bool FragmentProgramDecompiler::handle_tex_srb(u32 opcode)
{
	auto insert_texture_fetch = [this](FUNCTION base_func)
	{
		const auto type = m_prog.get_texture_dimension(dst.tex_num);
		const auto ref_mask = (1 << dst.tex_num);
		std::string swz_mask = "";

		auto func_id = base_func;

		if (m_prog.texture_state.shadow_textures & ref_mask)
		{
			properties.shadow_sampler_mask |= ref_mask;
			swz_mask = ".xxxx";

			func_id = (base_func == FUNCTION::TEXTURE_SAMPLE_PROJ_BASE) ? FUNCTION::TEXTURE_SAMPLE_SHADOW_PROJ_BASE : FUNCTION::TEXTURE_SAMPLE_SHADOW_BASE;
		}
		else
		{
			properties.common_access_sampler_mask |= ref_mask;
			if (m_prog.texture_state.redirected_textures & ref_mask)
			{
				properties.redirected_sampler_mask |= ref_mask;
				func_id = (base_func == FUNCTION::TEXTURE_SAMPLE_PROJ_BASE) ? FUNCTION::TEXTURE_SAMPLE_DEPTH_RGBA_PROJ_BASE : FUNCTION::TEXTURE_SAMPLE_DEPTH_RGBA_BASE;
			}
		}

		ensure(func_id <= FUNCTION::TEXTURE_SAMPLE_MAX_BASE_ENUM && func_id >= FUNCTION::TEXTURE_SAMPLE_BASE);

		if (!(m_prog.texture_state.multisampled_textures & ref_mask)) [[ likely ]]
		{
			// Clamp type to 3 types (1d, 2d, cube+3d) and offset into sampling redirection table
			const auto type_offset = (std::min(static_cast<int>(type), 2) + 1) * static_cast<int>(FUNCTION::TEXTURE_SAMPLE_BASE_ENUM_COUNT);
			func_id = static_cast<FUNCTION>(static_cast<int>(func_id) + type_offset);
		}
		else
		{
			// Map to multisample op
			ensure(type <= rsx::texture_dimension_extended::texture_dimension_2d);
			properties.multisampled_sampler_mask |= ref_mask;
			func_id = static_cast<FUNCTION>(static_cast<int>(func_id) - static_cast<int>(FUNCTION::TEXTURE_SAMPLE_BASE) + static_cast<int>(FUNCTION::TEXTURE_SAMPLE2DMS));
		}

		if (dst.exp_tex)
		{
			properties.has_exp_tex_op = true;
			AddCode("_enable_texture_expand();");
		}

		// Shadow proj
		switch (func_id)
		{
		case FUNCTION::TEXTURE_SAMPLE1D_SHADOW_PROJ:
		case FUNCTION::TEXTURE_SAMPLE2D_SHADOW_PROJ:
		case FUNCTION::TEXTURE_SAMPLE2DMS_SHADOW_PROJ:
		case FUNCTION::TEXTURE_SAMPLE3D_SHADOW_PROJ:
			properties.has_texShadowProj = true;
			break;
		default:
			break;
		}

		SetDst(getFunction(func_id) + swz_mask);

		if (dst.exp_tex)
		{
			// Cleanup
			AddCode("_disable_texture_expand();");
		}
	};

	switch (opcode)
	{
	case RSX_FP_OPCODE_DDX: SetDst(getFunction(FUNCTION::DFDX)); return true;
	case RSX_FP_OPCODE_DDY: SetDst(getFunction(FUNCTION::DFDY)); return true;
	case RSX_FP_OPCODE_NRM: SetDst("_builtin_normalize($0.xyz).xyzz", OPFLAGS::src_cast_f32); return true;
	case RSX_FP_OPCODE_BEM: SetDst("$0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw"); return true;
	case RSX_FP_OPCODE_TEXBEM:
	{
		//Untested, should be x2d followed by TEX
		AddX2d();
		AddCode(Format("x2d = $0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw;", true));
		[[fallthrough]];
	}
	case RSX_FP_OPCODE_TEX:
	{
		AddTex();
		insert_texture_fetch(FUNCTION::TEXTURE_SAMPLE_BASE);
		return true;
	}
	case RSX_FP_OPCODE_TXPBEM:
	{
		// Untested, should be x2d followed by TXP
		AddX2d();
		AddCode(Format("x2d = $0.xyxy + $1.xxxx * $2.xzxz + $1.yyyy * $2.ywyw;", true));
		[[fallthrough]];
	}
	case RSX_FP_OPCODE_TXP:
	{
		AddTex();
		insert_texture_fetch(FUNCTION::TEXTURE_SAMPLE_PROJ_BASE);
		return true;
	}
	case RSX_FP_OPCODE_TXD:
	{
		AddTex();
		insert_texture_fetch(FUNCTION::TEXTURE_SAMPLE_GRAD_BASE);
		return true;
	}
	case RSX_FP_OPCODE_TXB:
	{
		AddTex();
		insert_texture_fetch(FUNCTION::TEXTURE_SAMPLE_BIAS_BASE);
		return true;
	}
	case RSX_FP_OPCODE_TXL:
	{
		AddTex();
		insert_texture_fetch(FUNCTION::TEXTURE_SAMPLE_LOD_BASE);
		return true;
	}
	// Unpack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	// UP2 = UP2H (2 16-bit floats)
	// UP16 = UP2US (2 unsigned 16-bit scalars)
	// UP4 = UP4B (4 signed 8-bit scalars)
	// UPB = UP4UB (4 unsigned 8-bit scalars)
	// PK16/UP16 behavior confirmed by Saints Row: Gat out of Hell, ARGB8 -> X16Y16 conversion relies on this to render the wings
	case RSX_FP_OPCODE_UP2: SetDst("unpackHalf2x16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UP4: SetDst("unpackSnorm4x8(floatBitsToUint($0.x))"); return true;
	case RSX_FP_OPCODE_UP16: SetDst("unpackUnorm2x16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UPG:
		// Same as UPB with gamma correction
		properties.has_upg = true;
		SetDst("_builtin_upg($0.x)");
		return true;
	case RSX_FP_OPCODE_UPB: SetDst("(unpackUnorm4x8(floatBitsToUint($0.x)))"); return true;
	}
	return false;
}

std::string FragmentProgramDecompiler::Decompile()
{
	auto data = static_cast<be_t<u32>*>(m_prog.get_data());
	m_size = 0;
	m_location = 0;
	m_loop_count = 0;
	m_code_level = 1;
	m_is_valid_ucode = true;

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
				else rsx_log.error("BRK opcode found outside of a loop");
				break;
			case RSX_FP_OPCODE_CAL:
				rsx_log.error("Unimplemented SIP instruction: CAL");
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
		case RSX_FP_OPCODE_NOP:
			break;
		case RSX_FP_OPCODE_KIL:
			properties.has_discard_op = true;
			AddFlowOp("_kill()");
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

			rsx_log.error("Unknown/illegal instruction: 0x%x (forced unit %d)", opcode, prev_force_unit);
			break;
		}

		m_size += m_offset;

		if (dst.end) break;

		ensure(m_offset % sizeof(u32) == 0);
		data += m_offset / sizeof(u32);
	}

	while (m_code_level > 1)
	{
		rsx_log.error("Hanging block found at end of shader. Malformed shader?");

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
