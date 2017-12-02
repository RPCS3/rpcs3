#include "stdafx.h"
#include "Emu/Memory/Memory.h"
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


void FragmentProgramDecompiler::SetDst(std::string code, bool append_mask)
{
	if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt) return;

	switch (src1.scale)
	{
	case 0: break;
	case 1: code = "(" + code + " * 2.0)"; break;
	case 2: code = "(" + code + " * 4.0)"; break;
	case 3: code = "(" + code + " * 8.0)"; break;
	case 5: code = "(" + code + " / 2.0)"; break;
	case 6: code = "(" + code + " / 4.0)"; break;
	case 7: code = "(" + code + " / 8.0)"; break;

	default:
		LOG_ERROR(RSX, "Bad scale: %d", u32{ src1.scale });
		break;
	}

	if (!dst.no_dest)
	{
		if (dst.exp_tex)
		{
			//If dst.exp_tex really is _bx2 postfix, we need to unpack dynamic range
			AddCode("//exp tex flag is set");
			code = "((" + code + "- 0.5) * 2.)";
		}

		if (dst.saturate)
			code = saturate(code);
		else
			code = ClampValue(code, dst.prec);
	}

	code += (append_mask ? "$m" : "");

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

	std::string dest = AddReg(dst.dest_reg, dst.fp16) + "$m";

	AddCodeCond(Format(dest), code);
	//AddCode("$ifcond " + dest + code + (append_mask ? "$m;" : ";"));

	if (dst.set_cond)
	{
		AddCode(m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + dest + ";");
	}

	u32 reg_index = dst.fp16 ? dst.dest_reg >> 1 : dst.dest_reg;
	temp_registers[reg_index].tag(dst.dest_reg, !!dst.fp16);
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

std::string FragmentProgramDecompiler::AddReg(u32 index, int fp16)
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), std::string(fp16 ? "h" : "r") + std::to_string(index), getFloatTypeName(4) + "(0., 0., 0., 0.)");
}

bool FragmentProgramDecompiler::HasReg(u32 index, int fp16)
{
	return m_parr.HasParam(PF_PARAM_NONE, getFloatTypeName(4),
		std::string(fp16 ? "h" : "r") + std::to_string(index));
}

std::string FragmentProgramDecompiler::AddCond()
{
	return m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(src0.cond_reg_index));
}

std::string FragmentProgramDecompiler::AddConst()
{
	std::string name = std::string("fc") + std::to_string(m_size + 4 * 4);
	if (m_parr.HasParam(PF_PARAM_UNIFORM, getFloatTypeName(4), name))
	{
		return name;
	}

	auto data = (be_t<u32>*) ((char*)m_prog.addr + m_size + 4 * SIZE_32(u32));

	m_offset = 2 * 4 * sizeof(u32);
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);
	return m_parr.AddParam(PF_PARAM_UNIFORM, getFloatTypeName(4), name,
		std::string(getFloatTypeName(4) + "(") + std::to_string((float&)x) + ", " + std::to_string((float&)y)
		+ ", " + std::to_string((float&)z) + ", " + std::to_string((float&)w) + ")");
}

std::string FragmentProgramDecompiler::AddTex()
{
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

//Both of these were tested with a trace SoulCalibur IV title screen
//Failure to catch causes infinite values since theres alot of rcp(0)
std::string FragmentProgramDecompiler::NotZero(const std::string& code)
{
	return "(max(abs(" + code + "), 1.E-10) * sign(" + code + "))";
}

std::string FragmentProgramDecompiler::NotZeroPositive(const std::string& code)
{
	return "max(abs(" + code + "), 1.E-10)";
}

std::string FragmentProgramDecompiler::ClampValue(const std::string& code, u32 precision)
{
	//FP16 is expected to overflow alot easier at 0+-65504
	//FP32 can still work upto 0+-3.4E38
	//See http://http.download.nvidia.com/developer/Papers/2005/FP_Specials/FP_Specials.pdf

	switch (precision)
	{
	case 0:
		break;
	case 1:
		return "clamp(" + code + ", -65504., 65504.)";
	case 2:
		return "clamp(" + code + ", -2., 2.)";
	case 3:
		return "clamp(" + code + ", -1., 1.)";
	case 4:
		return "clamp(" + code + ", 0., 1.)";
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

std::string FragmentProgramDecompiler::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", [this]() -> std::string {return GetSRC<SRC0>(src0);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC0>), *this, src0) },
		{ "$1", [this]() -> std::string {return GetSRC<SRC1>(src1);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC1>), this, src1) },
		{ "$2", [this]() -> std::string {return GetSRC<SRC2>(src2);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC2>), this, src2) },
		{ "$t", std::bind(std::mem_fn(&FragmentProgramDecompiler::AddTex), this) },
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
		{ "$_c", std::bind(std::mem_fn(&FragmentProgramDecompiler::AddConst), this) }
	};

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

	static const char f[4] = { 'x', 'y', 'z', 'w' };
	std::string cond = GetRawCond();

	ShaderVariable dst_var(dst);
	dst_var.symplify();

	//const char *c_mask = f;

	if (dst_var.swizzles[0].length() == 1)
	{
		AddCode("if (" + cond + ".x) " + dst + " = " + src + ";");
	}
	else
	{
		for (int i = 0; i < dst_var.swizzles[0].length(); ++i)
		{
			AddCode("if (" + cond + "." + f[i] + ") " + dst + "." + f[i] + " = " + src + "." + f[i] + ";");
		}
	}
}

template<typename T> std::string FragmentProgramDecompiler::GetSRC(T src)
{
	std::string ret;
	bool apply_precision_modifier = !!src1.input_prec_mod;

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
				//TODO: Implement aliased gather for half floats
				bool xy_read = false;
				bool zw_read = false;

				if (src.swizzle_x < 2 || src.swizzle_y < 2 || src.swizzle_z < 2 || src.swizzle_w < 2)
					xy_read = true;
				if (src.swizzle_x > 1 || src.swizzle_y > 1 || src.swizzle_z > 1 || src.swizzle_w > 1)
					zw_read = true;

				auto &reg = temp_registers[src.tmp_reg_index];
				if (reg.requires_gather(xy_read, zw_read))
					AddCode(reg.gather_r());
			}
		}

		ret += AddReg(src.tmp_reg_index, src.fp16);
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
		case 0x00: ret += reg_table[0]; break;
		default:
			if (dst.src_attr_reg_num < sizeof(reg_table) / sizeof(reg_table[0]))
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

	//Warning: Modifier order matters. e.g neg should be applied after precision clamping (tested with Naruto UNS)
	if (src.abs) ret = "abs(" + ret + ")";
	if (apply_precision_modifier) ret = ClampValue(ret, src1.input_prec_mod);
	if (src.neg) ret = "-" + ret;

	return ret;
}

std::string FragmentProgramDecompiler::BuildCode()
{
	//main += fmt::format("\tgl_FragColor = %c0;\n", m_ctrl & 0x40 ? 'r' : 'h');

	std::stringstream OS;
	insertHeader(OS);
	OS << "\n";
	insertConstants(OS);
	OS << "\n";
	insertIntputs(OS);
	OS << "\n";
	insertOutputs(OS);
	OS << "\n";

	//Insert global function definitions
	insertGlobalFunctions(OS);

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

	insertMainStart(OS);
	OS << main << std::endl;
	insertMainEnd(OS);

	return OS.str();
}

bool FragmentProgramDecompiler::handle_sct(u32 opcode)
{
	switch (opcode)
	{
	case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); return true;
	case RSX_FP_OPCODE_DIV: SetDst("($0 / " + NotZero("$1.x") + ")"); return true;
	// Note: DIVSQ is not IEEE compliant. divsq(0, 0) is 0 (Super Puzzle Fighter II Turbo HD Remix).
	// sqrt(x, 0) might be equal to some big value (in absolute) whose sign is sign(x) but it has to be proven.
	case RSX_FP_OPCODE_DIVSQ: SetDst("($0 / sqrt(" + NotZeroPositive("$1.x") + "))"); return true;
	case RSX_FP_OPCODE_DP2: SetDst(getFunction(FUNCTION::FUNCTION_DP2)); return true;
	case RSX_FP_OPCODE_DP3: SetDst(getFunction(FUNCTION::FUNCTION_DP3)); return true;
	case RSX_FP_OPCODE_DP4: SetDst(getFunction(FUNCTION::FUNCTION_DP4)); return true;
	case RSX_FP_OPCODE_DP2A: SetDst(getFunction(FUNCTION::FUNCTION_DP2A)); return true;
	case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); return true;
	case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)"); return true;
	case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)"); return true;
	case RSX_FP_OPCODE_MOV: SetDst("$0"); return true;
	case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); return true;
	// Note: It's higly likely that RCP is not IEEE compliant but a game that uses rcp(0) has to be found
	case RSX_FP_OPCODE_RCP: SetDst("(1. / " +  NotZero("$0.x") + ").xxxx"); return true;
	// Note: RSQ is not IEEE compliant. rsq(0) is some big number (Silent Hill 3 HD)
	// It is not know what happens if 0 is negative.
	case RSX_FP_OPCODE_RSQ: SetDst("(1. / sqrt(" + NotZeroPositive("$0.x") + ").xxxx)"); return true;
	case RSX_FP_OPCODE_SEQ: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SEQ, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SFL: SetDst(getFunction(FUNCTION::FUNCTION_SFL)); return true;
	case RSX_FP_OPCODE_SGE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SGT: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGT, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SLE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SLT: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLT, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SNE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SNE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_STR: SetDst(getFunction(FUNCTION::FUNCTION_STR)); return true;
	}
	return false;
}

bool FragmentProgramDecompiler::handle_scb(u32 opcode)
{
	switch (opcode)
	{
	case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); return true;
	case RSX_FP_OPCODE_COS: SetDst("cos($0.xxxx)"); return true;
	case RSX_FP_OPCODE_DIV: SetDst("($0 / " + NotZero("$1.x") + ")"); return true;
	// Note: DIVSQ is not IEEE compliant. sqrt(0, 0) is 0 (Super Puzzle Fighter II Turbo HD Remix).
	// sqrt(x, 0) might be equal to some big value (in absolute) whose sign is sign(x) but it has to be proven.
	case RSX_FP_OPCODE_DIVSQ: SetDst("($0 / sqrt(" + NotZeroPositive("$1.x") + "))"); return true;
	case RSX_FP_OPCODE_DP2: SetDst(getFunction(FUNCTION::FUNCTION_DP2)); return true;
	case RSX_FP_OPCODE_DP3: SetDst(getFunction(FUNCTION::FUNCTION_DP3)); return true;
	case RSX_FP_OPCODE_DP4: SetDst(getFunction(FUNCTION::FUNCTION_DP4)); return true;
	case RSX_FP_OPCODE_DP2A: SetDst(getFunction(FUNCTION::FUNCTION_DP2A)); return true;
	case RSX_FP_OPCODE_DST: SetDst("vec4(distance($0, $1))"); return true;
	case RSX_FP_OPCODE_REFL: SetDst(getFunction(FUNCTION::FUNCTION_REFL)); return true;
	case RSX_FP_OPCODE_EX2: SetDst("exp2($0.xxxx)"); return true;
	case RSX_FP_OPCODE_FLR: SetDst("floor($0)"); return true;
	case RSX_FP_OPCODE_FRC: SetDst(getFunction(FUNCTION::FUNCTION_FRACT)); return true;
	case RSX_FP_OPCODE_LIT: SetDst("lit_legacy($0)"); return true;
	case RSX_FP_OPCODE_LIF: SetDst(getFloatTypeName(4) + "(1.0, $0.y, ($0.y > 0 ? pow(2.0, $0.w) : 0.0), 1.0)"); return true;
	case RSX_FP_OPCODE_LRP: SetDst(getFloatTypeName(4) + "($2 * (1 - $0) + $1 * $0)"); return true;
	case RSX_FP_OPCODE_LG2: SetDst("log2($0.xxxx)"); return true;
	case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); return true;
	case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)"); return true;
	case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)"); return true;
	case RSX_FP_OPCODE_MOV: SetDst("$0"); return true;
	case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); return true;
	//Pack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	case RSX_FP_OPCODE_PK2: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packHalf2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PK4: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packSnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_PK16: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packSnorm2x16($0.xy)))"); return true;
	case RSX_FP_OPCODE_PKG:
	//Should be similar to PKB but with gamma correction, see description of PK4UBG in khronos page
	case RSX_FP_OPCODE_PKB: SetDst(getFloatTypeName(4) + "(uintBitsToFloat(packUnorm4x8($0)))"); return true;
	case RSX_FP_OPCODE_SEQ: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SEQ, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SFL: SetDst(getFunction(FUNCTION::FUNCTION_SFL)); return true;
	case RSX_FP_OPCODE_SGE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SGT: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGT, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SIN: SetDst("sin($0.xxxx)"); return true;
	case RSX_FP_OPCODE_SLE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SLT: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLT, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_SNE: SetDst(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SNE, "$0", "$1") + ")"); return true;
	case RSX_FP_OPCODE_STR: SetDst(getFunction(FUNCTION::FUNCTION_STR)); return true;
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
	case RSX_FP_OPCODE_BEM: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: BEM"); return true;
	case RSX_FP_OPCODE_TEXBEM:
		//treat as TEX for now
		LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: TEXBEM");
	case RSX_FP_OPCODE_TEX:
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			if (DstExpectsSca() && (m_prog.shadow_textures & (1 << dst.tex_num)))
			{
				m_shadow_sampled_textures |= (1 << dst.tex_num);
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SHADOW2D) + ".r", false);	//No swizzle mask on shadow lookup
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
		//Treat as TXP for now
		LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: TXPBEM");
	case RSX_FP_OPCODE_TXP:
		switch (m_prog.get_texture_dimension(dst.tex_num))
		{
		case rsx::texture_dimension_extended::texture_dimension_1d:
			SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SAMPLE1D_PROJ));
			return true;
		case rsx::texture_dimension_extended::texture_dimension_2d:
			//Note shadow comparison only returns a true/false result!
			if (DstExpectsSca() && (m_prog.shadow_textures & (1 << dst.tex_num)))
			{
				m_shadow_sampled_textures |= (1 << dst.tex_num);
				SetDst(getFunction(FUNCTION::FUNCTION_TEXTURE_SHADOW2D_PROJ) + ".r", false);	//No swizzle mask on shadow lookup
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
	case RSX_FP_OPCODE_TXL:
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
	//Unpack operations. See https://www.khronos.org/registry/OpenGL/extensions/NV/NV_fragment_program.txt
	case RSX_FP_OPCODE_UP2: SetDst("unpackHalf2x16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UP4: SetDst("unpackSnorm4x8(floatBitsToUint($0.x))"); return true;
	case RSX_FP_OPCODE_UP16: SetDst("unpackSnormx16(floatBitsToUint($0.x)).xyxy"); return true;
	case RSX_FP_OPCODE_UPG:
	//Same as UPB with gamma correction
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

		const u32 opcode = dst.opcode | (src1.opcode_is_branch << 6);

		auto SIP = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_BRK:
				if (m_loop_count) AddFlowOp("break");
				else LOG_ERROR(RSX, "BRK opcode found outside of a loop");
				break;
			case RSX_FP_OPCODE_CAL: LOG_ERROR(RSX, "Unimplemented SIP instruction: CAL"); break;
			case RSX_FP_OPCODE_FENCT: forced_unit = FORCE_SCT; break;
			case RSX_FP_OPCODE_FENCB: forced_unit = FORCE_SCB; break;
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
			case RSX_FP_OPCODE_RET: AddFlowOp("return"); break;

			default:
				return false;
			}

			return true;
		};

		switch (opcode)
		{
		case RSX_FP_OPCODE_NOP: break;
		case RSX_FP_OPCODE_KIL: AddFlowOp("discard"); break;

		default:
			int prev_force_unit = forced_unit;

			//Some instructions do not respect forced unit
			//Tested with Tales of Vesperia
			if (SIP()) break;
			if (handle_tex_srb(opcode)) break;

			//FENCT/FENCB do not actually reject instructions if they dont match the forced unit
			//Tested with Dark Souls II where the repecting FENCX instruction will result in empty luminance averaging shaders
			//TODO: More reasearch is needed to determine what real HW does
			if (handle_sct(opcode)) break;
			if (handle_scb(opcode)) break;
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
