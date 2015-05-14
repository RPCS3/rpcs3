#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "FragmentProgramDecompiler.h"

#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

static std::string typeName[] =
{
	"float",
	"float2",
	"float3",
	"float4"
};

enum FUNCTION {
	FUNCTION_SATURATE,
	FUNCTION_DP2,
	FUNCTION_FRACT,
};

static std::string functionName[] =
{
	"saturate",
	"float4(dot($0.xy, $1.xy), dot($0.xy, $1.xy), dot($0.xy, $1.xy), dot($0.xy, $1.xy))",
	"frac($0)",
};

FragmentDecompiler::FragmentDecompiler(u32 addr, u32& size, u32 ctrl) :
	m_addr(addr),
	m_size(size),
	m_const_index(0),
	m_location(0),
	m_ctrl(ctrl)
{
	m_size = 0;
}


void FragmentDecompiler::SetDst(std::string code, bool append_mask)
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
		LOG_ERROR(RSX, "Bad scale: %d", fmt::by_value(src1.scale));
		Emu.Pause();
		break;
	}

	if (dst.saturate)
	{
		code = functionName[FUNCTION_SATURATE] + "(" + code + ")";
	}

	code += (append_mask ? "$m" : "");

	if (dst.no_dest)
	{
		if (dst.set_cond)
		{
			AddCode("$ifcond " + m_parr.AddParam(PARAM_NONE, typeName[3], "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + code + ";");
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
		AddCode(m_parr.AddParam(PARAM_NONE, typeName[3], "cc" + std::to_string(src0.cond_mod_reg_index)) + "$m = " + dest + ";");
	}
}

void FragmentDecompiler::AddCode(const std::string& code)
{
	main.append(m_code_level, '\t') += Format(code) + "\n";
}

std::string FragmentDecompiler::GetMask()
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

std::string FragmentDecompiler::AddReg(u32 index, int fp16)
{
	return m_parr.AddParam(PARAM_NONE, typeName[3], std::string(fp16 ? "h" : "r") + std::to_string(index), typeName[3] + "(0.0)");
}

bool FragmentDecompiler::HasReg(u32 index, int fp16)
{
	return m_parr.HasParam(PARAM_NONE, typeName[3],
		std::string(fp16 ? "h" : "r") + std::to_string(index));
}

std::string FragmentDecompiler::AddCond()
{
	return m_parr.AddParam(PARAM_NONE, typeName[3], "cc" + std::to_string(src0.cond_reg_index));
}

std::string FragmentDecompiler::AddConst()
{
	std::string name = std::string("fc") + std::to_string(m_size + 4 * 4);
	if (m_parr.HasParam(PARAM_UNIFORM, typeName[3], name))
	{
		return name;
	}

	auto data = vm::ptr<u32>::make(m_addr + m_size + 4 * sizeof(u32));

	m_offset = 2 * 4 * sizeof(u32);
	u32 x = GetData(data[0]);
	u32 y = GetData(data[1]);
	u32 z = GetData(data[2]);
	u32 w = GetData(data[3]);
	return m_parr.AddParam(PARAM_UNIFORM, typeName[3], name,
		std::string(typeName[3] + "(") + std::to_string((float&)x) + ", " + std::to_string((float&)y)
		+ ", " + std::to_string((float&)z) + ", " + std::to_string((float&)w) + ")");
}

std::string FragmentDecompiler::AddTex()
{
	return m_parr.AddParam(PARAM_UNIFORM, "sampler2D", std::string("tex") + std::to_string(dst.tex_num));
}

std::string FragmentDecompiler::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", [this]() -> std::string {return GetSRC<SRC0>(src0);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC0>), *this, src0) },
		{ "$1", [this]() -> std::string {return GetSRC<SRC1>(src1);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC1>), this, src1) },
		{ "$2", [this]() -> std::string {return GetSRC<SRC2>(src2);} },//std::bind(std::mem_fn(&GLFragmentDecompilerThread::GetSRC<SRC2>), this, src2) },
		{ "$t", std::bind(std::mem_fn(&FragmentDecompiler::AddTex), this) },
		{ "$m", std::bind(std::mem_fn(&FragmentDecompiler::GetMask), this) },
		{ "$ifcond ", [this]() -> std::string
	{
		const std::string& cond = GetCond();
		if (cond == "true") return "";
		return "if(" + cond + ") ";
	}
		},
		{ "$cond", std::bind(std::mem_fn(&FragmentDecompiler::GetCond), this) },
		{ "$c", std::bind(std::mem_fn(&FragmentDecompiler::AddConst), this) }
	};

	return fmt::replace_all(code, repl_list);
}

std::string FragmentDecompiler::GetCond()
{
	if (src0.exec_if_gr && src0.exec_if_lt && src0.exec_if_eq)
	{
		return "true";
	}
	else if (!src0.exec_if_gr && !src0.exec_if_lt && !src0.exec_if_eq)
	{
		return "false";
	}

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle, cond;
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];
	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	if (src0.exec_if_gr && src0.exec_if_eq)
	{
		cond = "greaterThanEqual";
	}
	else if (src0.exec_if_lt && src0.exec_if_eq)
	{
		cond = "lessThanEqual";
	}
	else if (src0.exec_if_gr && src0.exec_if_lt)
	{
		cond = "notEqual";
	}
	else if (src0.exec_if_gr)
	{
		cond = "greaterThan";
	}
	else if (src0.exec_if_lt)
	{
		cond = "lessThan";
	}
	else //if(src0.exec_if_eq)
	{
		cond = "equal";
	}

	return "any(" + cond + "(" + AddCond() + swizzle + ", " + typeName[3] +"(0.0)))";
}

void FragmentDecompiler::AddCodeCond(const std::string& dst, const std::string& src)
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

	std::string swizzle, cond;
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];
	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	if (src0.exec_if_gr && src0.exec_if_eq)
	{
		cond = ">=";
	}
	else if (src0.exec_if_lt && src0.exec_if_eq)
	{
		cond = "<=";
	}
	else if (src0.exec_if_gr && src0.exec_if_lt)
	{
		cond = "!=";
	}
	else if (src0.exec_if_gr)
	{
		cond = ">";
	}
	else if (src0.exec_if_lt)
	{
		cond = "<";
	}
	else //if(src0.exec_if_eq)
	{
		cond = "==";
	}

	cond = "(" + AddCond() + swizzle + " " + cond + " " + typeName[3] + "(0., 0., 0., 0.))";

	ShaderVar dst_var(dst);
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

template<typename T> std::string FragmentDecompiler::GetSRC(T src)
{
	std::string ret;

	switch (src.reg_type)
	{
	case 0: //tmp
		ret += AddReg(src.tmp_reg_index, src.fp16);
		break;

	case 1: //input
	{
		static const std::string reg_table[] =
		{
			"gl_Position",
			"diff_color", "spec_color",
			"fogc",
			"tc0", "tc1", "tc2", "tc3", "tc4", "tc5", "tc6", "tc7", "tc8", "tc9",
			"ssa"
		};

		switch (dst.src_attr_reg_num)
		{
		case 0x00: ret += reg_table[0]; break;
		default:
			if (dst.src_attr_reg_num < sizeof(reg_table) / sizeof(reg_table[0]))
			{
				ret += m_parr.AddParam(PARAM_IN, typeName[3], reg_table[dst.src_attr_reg_num]);
			}
			else
			{
				LOG_ERROR(RSX, "Bad src reg num: %d", fmt::by_value(dst.src_attr_reg_num));
				ret += m_parr.AddParam(PARAM_IN, typeName[3], "unk");
				Emu.Pause();
			}
			break;
		}
	}
	break;

	case 2: //const
		ret += AddConst();
		break;

	default:
		LOG_ERROR(RSX, "Bad src type %d", fmt::by_value(src.reg_type));
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

	if (src.abs) ret = "abs(" + ret + ")";
	if (src.neg) ret = "-" + ret;

	return ret;
}

std::string FragmentDecompiler::BuildCode()
{
	//main += fmt::Format("\tgl_FragColor = %c0;\n", m_ctrl & 0x40 ? 'r' : 'h');

	if (m_ctrl & 0xe) main += m_ctrl & 0x40 ? "\tgl_FragDepth = r1.z;\n" : "\tgl_FragDepth = h2.z;\n";

	std::stringstream OS;
	insertHeader(OS);
	OS << std::endl;
	insertConstants(OS);
	OS << std::endl;
	insertIntputs(OS);
	OS << std::endl;
	insertOutputs(OS);
	OS << std::endl;
	insertMainStart(OS);
	OS << main << std::endl;
	insertMainEnd(OS);

	return OS.str();
}

void FragmentDecompiler::insertHeader(std::stringstream & OS)
{
	OS << "// Header" << std::endl;
}

void FragmentDecompiler::insertIntputs(std::stringstream & OS)
{
	OS << "struct PixelInput" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 Position : SV_POSITION;" << std::endl;
	OS << "	float4 diff_color : COLOR0;" << std::endl;
	OS << "	float4 spec_color : COLOR1;" << std::endl;
	OS << "	float4 dst_reg3 : COLOR2;" << std::endl;
	OS << "	float4 dst_reg4 : COLOR3;" << std::endl;
	OS << "	float fogc : FOG;" << std::endl;
	OS << "	float4 dummy : COLOR4;" << std::endl;
	OS << "	float4 tc0 : TEXCOORD0;" << std::endl;
	OS << "	float4 tc1 : TEXCOORD1;" << std::endl;
	OS << "	float4 tc2 : TEXCOORD2;" << std::endl;
	OS << "	float4 tc3 : TEXCOORD3;" << std::endl;
	OS << "	float4 tc4 : TEXCOORD4;" << std::endl;
	OS << "	float4 tc5 : TEXCOORD5;" << std::endl;
	OS << "	float4 tc6 : TEXCOORD6;" << std::endl;
	OS << "	float4 tc7 : TEXCOORD7;" << std::endl;
	OS << "	float4 tc8 : TEXCOORD8;" << std::endl;
	OS << "};" << std::endl;
}

void FragmentDecompiler::insertOutputs(std::stringstream & OS)
{
	OS << "struct PixelOutput" << std::endl;
	OS << "{" << std::endl;
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & 0x40 ? "r0" : "h0" },
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h4" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h6" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h8" },
	};

	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PARAM_NONE, typeName[3], table[i].second))
			OS << "	" << typeName[3] << " " << table[i].first << " : SV_TARGET" << i << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void FragmentDecompiler::insertConstants(std::stringstream & OS)
{
	OS << "cbuffer CONSTANT : register(b2)" << std::endl;
	OS << "{" << std::endl;
	for (ParamType PT : m_parr.params[PARAM_UNIFORM])
	{
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void FragmentDecompiler::insertMainStart(std::stringstream & OS)
{
	OS << "PixelOutput main(PixelInput In)" << std::endl;
	OS << "{" << std::endl;
	for (ParamType PT : m_parr.params[PARAM_IN])
	{
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";" << std::endl;
	}
	// Declare output
	for (ParamType PT : m_parr.params[PARAM_NONE])
	{
		for (ParamItem PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = float4(0., 0., 0., 0.);" << std::endl;
	}
}

void FragmentDecompiler::insertMainEnd(std::stringstream & OS)
{
	const std::pair<std::string, std::string> table[] =
	{
		{ "ocol0", m_ctrl & 0x40 ? "r0" : "h0" },
		{ "ocol1", m_ctrl & 0x40 ? "r2" : "h4" },
		{ "ocol2", m_ctrl & 0x40 ? "r3" : "h6" },
		{ "ocol3", m_ctrl & 0x40 ? "r4" : "h8" },
	};

	OS << "	PixelOutput Out;" << std::endl;
	for (int i = 0; i < sizeof(table) / sizeof(*table); ++i)
	{
		if (m_parr.HasParam(PARAM_NONE, typeName[3], table[i].second))
			OS << "	Out." << table[i].first << " = " << table[i].second << ";" << std::endl;
	}
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}

std::string FragmentDecompiler::Decompile()
{
	auto data = vm::ptr<u32>::make(m_addr);
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
		for (auto finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
		finded != m_end_offsets.end();
			finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(finded);
			m_code_level--;
			AddCode("}");
			m_loop_count--;
		}

		for (auto finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
		finded != m_else_offsets.end();
			finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size))
		{
			m_else_offsets.erase(finded);
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

		auto SCT = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); break;
			case RSX_FP_OPCODE_DIV: SetDst("($0 / $1)"); break;
			case RSX_FP_OPCODE_DIVSQ: SetDst("($0 / sqrt($1))"); break;
			case RSX_FP_OPCODE_DP2: SetDst(functionName[FUNCTION_DP2]); break;
			case RSX_FP_OPCODE_DP3: SetDst("vec4(dot($0.xyz, $1.xyz))"); break;
			case RSX_FP_OPCODE_DP4: SetDst("vec4(dot($0, $1))"); break;
			case RSX_FP_OPCODE_DP2A: SetDst("vec4($0.x * $1.x + $0.y * $1.y + $2.x)"); break;
			case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); break;
			case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)"); break;
			case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)"); break;
			case RSX_FP_OPCODE_MOV: SetDst("$0"); break;
			case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); break;
			case RSX_FP_OPCODE_RCP: SetDst("1 / $0"); break;
			case RSX_FP_OPCODE_RSQ: SetDst("inversesqrt(abs($0))"); break;
			case RSX_FP_OPCODE_SEQ: SetDst("vec4(equal($0, $1))"); break;
			case RSX_FP_OPCODE_SFL: SetDst("vec4(0.0)"); break;
			case RSX_FP_OPCODE_SGE: SetDst("vec4(greaterThanEqual($0, $1))"); break;
			case RSX_FP_OPCODE_SGT: SetDst("vec4(greaterThan($0, $1))"); break;
			case RSX_FP_OPCODE_SLE: SetDst("vec4(lessThanEqual($0, $1))"); break;
			case RSX_FP_OPCODE_SLT: SetDst("vec4(lessThan($0, $1))"); break;
			case RSX_FP_OPCODE_SNE: SetDst("vec4(notEqual($0, $1))"); break;
			case RSX_FP_OPCODE_STR: SetDst("vec4(1.0)"); break;

			default:
				return false;
			}

			return true;
		};

		auto SCB = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_ADD: SetDst("($0 + $1)"); break;
			case RSX_FP_OPCODE_COS: SetDst("cos($0)"); break;
			case RSX_FP_OPCODE_DP2: SetDst("vec4(dot($0.xy, $1.xy))"); break;
			case RSX_FP_OPCODE_DP3: SetDst("vec4(dot($0.xyz, $1.xyz))"); break;
			case RSX_FP_OPCODE_DP4: SetDst("vec4(dot($0, $1))"); break;
			case RSX_FP_OPCODE_DP2A: SetDst("vec4($0.x * $1.x + $0.y * $1.y + $2.x)"); break;
			case RSX_FP_OPCODE_DST: SetDst("vec4(distance($0, $1))"); break;
			case RSX_FP_OPCODE_REFL: LOG_ERROR(RSX, "Unimplemented SCB instruction: REFL"); break; // TODO: Is this in the right category?
			case RSX_FP_OPCODE_EX2: SetDst("exp2($0)"); break;
			case RSX_FP_OPCODE_FLR: SetDst("floor($0)"); break;
			case RSX_FP_OPCODE_FRC: SetDst(functionName[FUNCTION_FRACT]); break;
			case RSX_FP_OPCODE_LIT: SetDst("vec4(1.0, $0.x, ($0.x > 0.0 ? exp($0.w * log2($0.y)) : 0.0), 1.0)"); break;
			case RSX_FP_OPCODE_LIF: SetDst("vec4(1.0, $0.y, ($0.y > 0 ? pow(2.0, $0.w) : 0.0), 1.0)"); break;
			case RSX_FP_OPCODE_LRP: LOG_ERROR(RSX, "Unimplemented SCB instruction: LRP"); break; // TODO: Is this in the right category?
			case RSX_FP_OPCODE_LG2: SetDst("log2($0)"); break;
			case RSX_FP_OPCODE_MAD: SetDst("($0 * $1 + $2)"); break;
			case RSX_FP_OPCODE_MAX: SetDst("max($0, $1)"); break;
			case RSX_FP_OPCODE_MIN: SetDst("min($0, $1)"); break;
			case RSX_FP_OPCODE_MOV: SetDst("$0"); break;
			case RSX_FP_OPCODE_MUL: SetDst("($0 * $1)"); break;
			case RSX_FP_OPCODE_PK2: SetDst("packSnorm2x16($0)"); break; // TODO: More testing (Sonic The Hedgehog (NPUB-30442/NPEB-00478))
			case RSX_FP_OPCODE_PK4: SetDst("packSnorm4x8($0)"); break; // TODO: More testing (Sonic The Hedgehog (NPUB-30442/NPEB-00478))
			case RSX_FP_OPCODE_PK16: LOG_ERROR(RSX, "Unimplemented SCB instruction: PK16"); break;
			case RSX_FP_OPCODE_PKB: LOG_ERROR(RSX, "Unimplemented SCB instruction: PKB"); break;
			case RSX_FP_OPCODE_PKG: LOG_ERROR(RSX, "Unimplemented SCB instruction: PKG"); break;
			case RSX_FP_OPCODE_SEQ: SetDst("vec4(equal($0, $1))"); break;
			case RSX_FP_OPCODE_SFL: SetDst("vec4(0.0)"); break;
			case RSX_FP_OPCODE_SGE: SetDst("vec4(greaterThanEqual($0, $1))"); break;
			case RSX_FP_OPCODE_SGT: SetDst("vec4(greaterThan($0, $1))"); break;
			case RSX_FP_OPCODE_SIN: SetDst("sin($0)"); break;
			case RSX_FP_OPCODE_SLE: SetDst("vec4(lessThanEqual($0, $1))"); break;
			case RSX_FP_OPCODE_SLT: SetDst("vec4(lessThan($0, $1))"); break;
			case RSX_FP_OPCODE_SNE: SetDst("vec4(notEqual($0, $1))"); break;
			case RSX_FP_OPCODE_STR: SetDst("vec4(1.0)"); break;

			default:
				return false;
			}

			return true;
		};

		auto TEX_SRB = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_DDX: SetDst("dFdx($0)"); break;
			case RSX_FP_OPCODE_DDY: SetDst("dFdy($0)"); break;
			case RSX_FP_OPCODE_NRM: SetDst("normalize($0)"); break;
			case RSX_FP_OPCODE_BEM: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: BEM"); break;
			case RSX_FP_OPCODE_TEX: SetDst("texture($t, $0.xy)");  break;
			case RSX_FP_OPCODE_TEXBEM: SetDst("texture($t, $0.xy, $1.x)"); break;
			case RSX_FP_OPCODE_TXP: SetDst("textureProj($t, $0.xyz, $1.x)"); break; //TODO: More testing (Sonic The Hedgehog (NPUB-30442/NPEB-00478) and The Simpsons Arcade Game (NPUB30563))
			case RSX_FP_OPCODE_TXPBEM: SetDst("textureProj($t, $0.xyz, $1.x)"); break;
			case RSX_FP_OPCODE_TXD: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: TXD"); break;
			case RSX_FP_OPCODE_TXB: SetDst("texture($t, $0.xy, $1.x)"); break;
			case RSX_FP_OPCODE_TXL: SetDst("textureLod($t, $0.xy, $1.x)"); break;
			case RSX_FP_OPCODE_UP2: SetDst("unpackSnorm2x16($0)"); break; // TODO: More testing (Sonic The Hedgehog (NPUB-30442/NPEB-00478))
			case RSX_FP_OPCODE_UP4: SetDst("unpackSnorm4x8($0)"); break; // TODO: More testing (Sonic The Hedgehog (NPUB-30442/NPEB-00478))
			case RSX_FP_OPCODE_UP16: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: UP16"); break;
			case RSX_FP_OPCODE_UPB: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: UPB"); break;
			case RSX_FP_OPCODE_UPG: LOG_ERROR(RSX, "Unimplemented TEX_SRB instruction: UPG"); break;

			default:
				return false;
			}

			return true;
		};

		auto SIP = [&]()
		{
			switch (opcode)
			{
			case RSX_FP_OPCODE_BRK: SetDst("break"); break;
			case RSX_FP_OPCODE_CAL: LOG_ERROR(RSX, "Unimplemented SIP instruction: CAL"); break;
			case RSX_FP_OPCODE_FENCT: forced_unit = FORCE_SCT; break;
			case RSX_FP_OPCODE_FENCB: forced_unit = FORCE_SCB; break;
			case RSX_FP_OPCODE_IFE:
				AddCode("if($cond)");
				m_else_offsets.push_back(src1.else_offset << 2);
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCode("{");
				m_code_level++;
				break;
			case RSX_FP_OPCODE_LOOP:
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //LOOP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
				}
				else
				{
					AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) //LOOP",
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
					AddCode(fmt::Format("$ifcond for(int i%u = %u; i%u < %u; i%u += %u) {} //-> %u //REP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment, src2.end_offset));
				}
				else
				{
					AddCode(fmt::Format("if($cond) for(int i%u = %u; i%u < %u; i%u += %u) //REP",
						m_loop_count, src1.init_counter, m_loop_count, src1.end_counter, m_loop_count, src1.increment));
					m_loop_count++;
					m_end_offsets.push_back(src2.end_offset << 2);
					AddCode("{");
					m_code_level++;
				}
				break;
			case RSX_FP_OPCODE_RET: SetDst("return"); break;

			default:
				return false;
			}

			return true;
		};

		switch (opcode)
		{
		case RSX_FP_OPCODE_NOP: break;
		case RSX_FP_OPCODE_KIL: SetDst("discard", false); break;

		default:
			if (forced_unit == FORCE_NONE)
			{
				if (SIP()) break;
				if (SCT()) break;
				if (TEX_SRB()) break;
				if (SCB()) break;
			}
			else if (forced_unit == FORCE_SCT)
			{
				forced_unit = FORCE_NONE;
				if (SCT()) break;
			}
			else if (forced_unit == FORCE_SCB)
			{
				forced_unit = FORCE_NONE;
				if (SCB()) break;
			}

			LOG_ERROR(RSX, "Unknown/illegal instruction: 0x%x (forced unit %d)", opcode, forced_unit);
			break;
		}

		m_size += m_offset;

		if (dst.end) break;

		assert(m_offset % sizeof(u32) == 0);
		data += m_offset / sizeof(u32);
	}

	// flush m_code_level
	m_code_level = 1;
	std::string m_shader = BuildCode();
	main.clear();
//	m_parr.params.clear();
	return m_shader;
}

#endif