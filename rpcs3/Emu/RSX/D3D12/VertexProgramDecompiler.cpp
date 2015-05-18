#include "stdafx.h"
#if defined(DX12_SUPPORT)
#include "VertexProgramDecompiler.h"

#include "Utilities/Log.h"
#include "Emu/System.h"

static std::string typeName[] =
{
	"float",
	"float2",
	"float3",
	"float4"
};

std::string VertexDecompiler::GetMask(bool is_sca)
{
	std::string ret;

	if (is_sca)
	{
		if (d3.sca_writemask_x) ret += "x";
		if (d3.sca_writemask_y) ret += "y";
		if (d3.sca_writemask_z) ret += "z";
		if (d3.sca_writemask_w) ret += "w";
	}
	else
	{
		if (d3.vec_writemask_x) ret += "x";
		if (d3.vec_writemask_y) ret += "y";
		if (d3.vec_writemask_z) ret += "z";
		if (d3.vec_writemask_w) ret += "w";
	}

	return ret.empty() || ret == "xyzw" ? "" : ("." + ret);
}

std::string VertexDecompiler::GetVecMask()
{
	return GetMask(false);
}

std::string VertexDecompiler::GetScaMask()
{
	return GetMask(true);
}

std::string VertexDecompiler::GetDST(bool isSca)
{
	std::string ret;

	switch (isSca ? 0x1f : d3.dst)
	{
	case 0x1f:
		ret += m_parr.AddParam(PF_PARAM_NONE, typeName[3], std::string("tmp") + std::to_string(isSca ? d3.sca_dst_tmp : d0.dst_tmp));
		break;

	default:
		if (d3.dst > 15)
			LOG_ERROR(RSX, fmt::Format("dst index out of range: %u", d3.dst));
		ret += m_parr.AddParam(PF_PARAM_NONE, typeName[3], std::string("dst_reg") + std::to_string(d3.dst), d3.dst == 0 ? typeName[3] + "(0.0f, 0.0f, 0.0f, 1.0f)" : typeName[3] + "(0.0, 0.0, 0.0, 0.0)");
		break;
	}

	return ret;
}

std::string VertexDecompiler::GetSRC(const u32 n)
{
	static const std::string reg_table[] =
	{
		"in_pos", "in_weight", "in_normal",
		"in_diff_color", "in_spec_color",
		"in_fog",
		"in_point_size", "in_7",
		"in_tc0", "in_tc1", "in_tc2", "in_tc3",
		"in_tc4", "in_tc5", "in_tc6", "in_tc7"
	};

	std::string ret;

	switch (src[n].reg_type)
	{
	case 1: //temp
		ret += m_parr.AddParam(PF_PARAM_NONE, typeName[3], "tmp" + std::to_string(src[n].tmp_src));
		break;
	case 2: //input
		if (d1.input_src < (sizeof(reg_table) / sizeof(reg_table[0])))
		{
			ret += m_parr.AddParam(PF_PARAM_IN, typeName[3], reg_table[d1.input_src], d1.input_src);
		}
		else
		{
			LOG_ERROR(RSX, "Bad input src num: %d", fmt::by_value(d1.input_src));
			ret += m_parr.AddParam(PF_PARAM_IN, typeName[3], "in_unk", d1.input_src);
		}
		break;
	case 3: //const
		m_parr.AddParam(PF_PARAM_UNIFORM, typeName[3], std::string("vc[468]"));
		ret += std::string("vc[") + std::to_string(d1.const_src) + (d3.index_const ? " + " + AddAddrReg() : "") + "]";
		break;

	default:
		LOG_ERROR(RSX, fmt::Format("Bad src%u reg type: %d", n, fmt::by_value(src[n].reg_type)));
		Emu.Pause();
		break;
	}

	static const std::string f = "xyzw";

	std::string swizzle;

	swizzle += f[src[n].swz_x];
	swizzle += f[src[n].swz_y];
	swizzle += f[src[n].swz_z];
	swizzle += f[src[n].swz_w];

	if (swizzle != f) ret += '.' + swizzle;

	bool abs;

	switch (n)
	{
	case 0: abs = d0.src0_abs; break;
	case 1: abs = d0.src1_abs; break;
	case 2: abs = d0.src2_abs; break;
	}

	if (abs) ret = "abs(" + ret + ")";
	if (src[n].neg) ret = "-" + ret;

	return ret;
}

void VertexDecompiler::SetDST(bool is_sca, std::string value)
{
	if (d0.cond == 0) return;

	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	std::string mask = GetMask(is_sca);

	value += mask;

	if (is_sca && d0.vec_result)
	{
		//value = "vec4(" + value + ")";
	}

	if (d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	std::string dest;

	if (d0.cond_update_enable_0 && d0.cond_update_enable_1)
	{
		dest = m_parr.AddParam(PF_PARAM_NONE, typeName[3], "cc" + std::to_string(d0.cond_reg_sel_1), typeName[3] + "(0.0)") + mask;
	}
	else if (d3.dst != 0x1f || (is_sca ? d3.sca_dst_tmp != 0x3f : d0.dst_tmp != 0x3f))
	{
		dest = GetDST(is_sca) + mask;
	}

	//std::string code;
	//if (d0.cond_test_enable)
	//	code += "$ifcond ";
	//code += dest + value;
	//AddCode(code + ";");

	AddCodeCond(Format(dest), value);
}

std::string VertexDecompiler::GetFunc()
{
	std::string name = "func$a";

	for (const auto& func : m_funcs) {
		if (func.name.compare(name) == 0) {
			return name + "()";
		}
	}

	m_funcs.emplace_back();
	FuncInfo &idx = m_funcs.back();
	idx.offset = GetAddr();
	idx.name = name;

	return name + "()";
}

std::string VertexDecompiler::GetTex()
{
	return m_parr.AddParam(PF_PARAM_UNIFORM, "sampler2D", std::string("vtex") + std::to_string(/*?.tex_num*/0));
}

std::string VertexDecompiler::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&VertexDecompiler::GetSRC), this, 0) },
		{ "$1", std::bind(std::mem_fn(&VertexDecompiler::GetSRC), this, 1) },
		{ "$2", std::bind(std::mem_fn(&VertexDecompiler::GetSRC), this, 2) },
		{ "$s", std::bind(std::mem_fn(&VertexDecompiler::GetSRC), this, 2) },
		{ "$am", std::bind(std::mem_fn(&VertexDecompiler::AddAddrMask), this) },
		{ "$a", std::bind(std::mem_fn(&VertexDecompiler::AddAddrReg), this) },

		{ "$t", std::bind(std::mem_fn(&VertexDecompiler::GetTex), this) },

		{ "$fa", [this]()->std::string { return std::to_string(GetAddr()); } },
		{ "$f()", std::bind(std::mem_fn(&VertexDecompiler::GetFunc), this) },
		{ "$ifcond ", [this]() -> std::string
	{
		const std::string& cond = GetCond();
		if (cond == "true") return "";
		return "if(" + cond + ") ";
	}
		},
		{ "$cond", std::bind(std::mem_fn(&VertexDecompiler::GetCond), this) }
	};

	return fmt::replace_all(code, repl_list);
}

std::string VertexDecompiler::GetCond()
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	if (d0.cond == 0) return "false";
	if (d0.cond == (lt | gt | eq)) return "true";

	static const char* cond_string_table[(lt | gt | eq) + 1] =
	{
		"error",
		"lessThan",
		"equal",
		"lessThanEqual",
		"greaterThan",
		"notEqual",
		"greaterThanEqual",
		"error"
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	return fmt::Format("any(%s(cc%d%s, vec4(0.0)%s))", cond_string_table[d0.cond], d0.cond_reg_sel_1, swizzle.c_str(), swizzle.c_str());
}

void VertexDecompiler::AddCodeCond(const std::string& dst, const std::string& src)
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};


	if (!d0.cond_test_enable || d0.cond == (lt | gt | eq))
	{
		AddCode(dst + " = " + src + ";");
		return;
	}

	if (d0.cond == 0)
	{
		AddCode("//" + dst + " = " + src + ";");
		return;
	}

	static const char* cond_string_table[(lt | gt | eq) + 1] =
	{
		"error",
		"<",
		"==",
		"<=",
		">",
		"!=",
		">=",
		"error"
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	std::string cond = fmt::Format("(cc%d%s %s float4(0., 0., 0., 0.))", d0.cond_reg_sel_1, swizzle.c_str(), cond_string_table[d0.cond]);

	ShaderVariable dst_var(dst);
	dst_var.symplify();

	//const char *c_mask = f;

	if (dst_var.swizzles[0].length() == 1)
	{
		AddCode("if (" + cond + ".x) " + dst + " = " + typeName[3] + "(" + src + ".xxxx).x;");
	}
	else
	{
		for (int i = 0; i < dst_var.swizzles[0].length(); ++i)
		{
			AddCode("if (" + cond + "." + f[i] + ") " + dst + "." + f[i] + " = " + src + "." + f[i] + ";");
		}
	}
}


std::string VertexDecompiler::AddAddrMask()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return std::string(".") + f[d0.addr_swz];
}

std::string VertexDecompiler::AddAddrReg()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return m_parr.AddParam(PF_PARAM_NONE, "ivec4", "a" + std::to_string(d0.addr_reg_sel_1), "ivec4(0)") + AddAddrMask();
}

u32 VertexDecompiler::GetAddr()
{
	return (d2.iaddrh << 3) | d3.iaddrl;
}

void VertexDecompiler::AddCode(const std::string& code)
{
	m_body.push_back(Format(code) + ";");
	m_cur_instr->body.push_back(Format(code));
}

void VertexDecompiler::SetDSTVec(const std::string& code)
{
	SetDST(false, code);
}

void VertexDecompiler::SetDSTSca(const std::string& code)
{
	SetDST(true, code);
}

std::string VertexDecompiler::BuildFuncBody(const FuncInfo& func)
{
	std::string result;

	for (uint i = func.offset; i<m_body.size(); ++i)
	{
		if (i != func.offset)
		{
			uint call_func = -1;
			for (uint j = 0; j<m_funcs.size(); ++j)
			{
				if (m_funcs[j].offset == i)
				{
					call_func = j;
					break;
				}
			}

			if (call_func != -1)
			{
				result += '\t' + m_funcs[call_func].name + "();\n";
				break;
			}
		}

		result += '\t' + m_body[i] + '\n';
	}

	return result;
}

std::string VertexDecompiler::BuildCode()
{
	std::string main_body;
	for (uint i = 0, lvl = 1; i < m_instr_count; i++)
	{
		lvl -= m_instructions[i].close_scopes;
		if (lvl < 1) lvl = 1;
		//assert(lvl >= 1);
		for (int j = 0; j < m_instructions[i].put_close_scopes; ++j)
		{
			--lvl;
			if (lvl < 1) lvl = 1;
			main_body.append(lvl, '\t') += "}\n";
		}

		for (int j = 0; j < m_instructions[i].do_count; ++j)
		{
			main_body.append(lvl, '\t') += "do\n";
			main_body.append(lvl, '\t') += "{\n";
			lvl++;
		}

		for (uint j = 0; j < m_instructions[i].body.size(); ++j)
		{
			main_body.append(lvl, '\t') += m_instructions[i].body[j] + "\n";
		}

		lvl += m_instructions[i].open_scopes;
	}

	std::stringstream OS;
	insertHeader(OS);

	insertInputs(OS, m_parr.params[PF_PARAM_IN]);
	OS << std::endl;
	insertOutputs(OS, m_parr.params[PF_PARAM_NONE]);
	OS << std::endl;
	insertConstants(OS, m_parr.params[PF_PARAM_UNIFORM]);
	OS << std::endl;

	insertMainStart(OS);
	OS << main_body.c_str() << std::endl;
	insertMainEnd(OS);

	return OS.str();
}

void VertexDecompiler::insertHeader(std::stringstream &OS)
{
	OS << "cbuffer SCALE_OFFSET : register(b0)" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4x4 scaleOffsetMat;" << std::endl;
	OS << "};" << std::endl;
}

void VertexDecompiler::insertInputs(std::stringstream & OS, const std::vector<ParamType>& inputs)
{
	OS << "struct VertexInput" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : inputs)
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ": TEXCOORD" << PI.location << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void VertexDecompiler::insertConstants(std::stringstream & OS, const std::vector<ParamType> & constants)
{
	OS << "cbuffer CONSTANT_BUFFER : register(b1)" << std::endl;
	OS << "{" << std::endl;
	for (const ParamType PT : constants)
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << ";" << std::endl;
	}
	OS << "};" << std::endl;
}

void VertexDecompiler::insertOutputs(std::stringstream & OS, const std::vector<ParamType> & outputs)
{
	OS << "struct PixelInput" << std::endl;
	OS << "{" << std::endl;
	OS << "	float4 dst_reg0 : SV_POSITION;" << std::endl;
	OS << "	float4 dst_reg1 : COLOR0;" << std::endl;
	OS << "	float4 dst_reg2 : COLOR1;" << std::endl;
	OS << "	float4 dst_reg3 : COLOR2;" << std::endl;
	OS << "	float4 dst_reg4 : COLOR3;" << std::endl;
	OS << "	float dst_reg5 : FOG;" << std::endl;
	OS << "	float4 dst_reg6 : COLOR4;" << std::endl;
	OS << "	float4 dst_reg7 : TEXCOORD0;" << std::endl;
	OS << "	float4 dst_reg8 : TEXCOORD1;" << std::endl;
	OS << "	float4 dst_reg9 : TEXCOORD2;" << std::endl;
	OS << "	float4 dst_reg10 : TEXCOORD3;" << std::endl;
	OS << "	float4 dst_reg11 : TEXCOORD4;" << std::endl;
	OS << "	float4 dst_reg12 : TEXCOORD5;" << std::endl;
	OS << "	float4 dst_reg13 : TEXCOORD6;" << std::endl;
	OS << "	float4 dst_reg14 : TEXCOORD7;" << std::endl;
	OS << "	float4 dst_reg15 : TEXCOORD8;" << std::endl;
	OS << "};" << std::endl;
}

struct reg_info
{
	std::string name;
	bool need_declare;
	std::string src_reg;
	std::string src_reg_mask;
	bool need_cast;
};

static const reg_info reg_table[] =
{
	{ "gl_Position", false, "dst_reg0", "", false },
	{ "diff_color", true, "dst_reg1", "", false },
	{ "spec_color", true, "dst_reg2", "", false },
	{ "front_diff_color", true, "dst_reg3", "", false },
	{ "front_spec_color", true, "dst_reg4", "", false },
	{ "fogc", true, "dst_reg5", ".x", true },
	{ "gl_ClipDistance[0]", false, "dst_reg5", ".y", false },
	{ "gl_ClipDistance[1]", false, "dst_reg5", ".z", false },
	{ "gl_ClipDistance[2]", false, "dst_reg5", ".w", false },
	{ "gl_PointSize", false, "dst_reg6", ".x", false },
	{ "gl_ClipDistance[3]", false, "dst_reg6", ".y", false },
	{ "gl_ClipDistance[4]", false, "dst_reg6", ".z", false },
	{ "gl_ClipDistance[5]", false, "dst_reg6", ".w", false },
	{ "tc0", true, "dst_reg7", "", false },
	{ "tc1", true, "dst_reg8", "", false },
	{ "tc2", true, "dst_reg9", "", false },
	{ "tc3", true, "dst_reg10", "", false },
	{ "tc4", true, "dst_reg11", "", false },
	{ "tc5", true, "dst_reg12", "", false },
	{ "tc6", true, "dst_reg13", "", false },
	{ "tc7", true, "dst_reg14", "", false },
	{ "tc8", true, "dst_reg15", "", false },
	{ "tc9", true, "dst_reg6", "", false }  // In this line, dst_reg6 is correct since dst_reg goes from 0 to 15.
};

void VertexDecompiler::insertMainStart(std::stringstream & OS)
{
	OS << "PixelInput main(VertexInput In)" << std::endl;
	OS << "{" << std::endl;

	// Declare inside main function
	for (const ParamType PT : m_parr.params[PF_PARAM_NONE])
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = float4(0., 0., 0., 0.);" << std::endl;
	}

	for (const ParamType PT : m_parr.params[PF_PARAM_IN])
	{
		for (const ParamItem &PI : PT.items)
			OS << "	" << PT.type << " " << PI.name << " = In." << PI.name << ";" << std::endl;
	}


}


void VertexDecompiler::insertMainEnd(std::stringstream & OS)
{
	OS << "	PixelInput Out;" << std::endl;
	// Declare inside main function
	for (auto &i : reg_table)
	{
		if (m_parr.HasParam(PF_PARAM_NONE, typeName[3], i.src_reg))
			OS << "	Out." << i.src_reg << " = " << i.src_reg << ";" << std::endl;
	}
	// TODO: Find why I need to do this
//	OS << "	Out.dst_reg0.z *= -1.;" << std::endl;
	OS << "	Out.dst_reg0 = mul(Out.dst_reg0, scaleOffsetMat);" << std::endl;
	OS << "	return Out;" << std::endl;
	OS << "}" << std::endl;
}

VertexDecompiler::VertexDecompiler(std::vector<u32>& data) :
 m_data(data)
{
	m_funcs.emplace_back();
	m_funcs[0].offset = 0;
	m_funcs[0].name = "main";
	m_funcs.emplace_back();
	m_funcs[1].offset = 0;
	m_funcs[1].name = "func0";
	//m_cur_func->body = "\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
}

std::string VertexDecompiler::Decompile()
{
	for (unsigned i = 0; i < PF_PARAM_COUNT; i++)
		m_parr.params[i].clear();
	m_instr_count = 0;

	for (int i = 0; i < m_max_instr_count; ++i)
	{
		m_instructions[i].reset();
	}

	bool is_has_BRA = false;

	for (u32 i = 1; m_instr_count < m_max_instr_count; m_instr_count++)
	{
		m_cur_instr = &m_instructions[m_instr_count];

		if (is_has_BRA)
		{
			d3.HEX = m_data[i];
			i += 4;
		}
		else
		{
			d1.HEX = m_data[i++];

			switch (d1.sca_opcode)
			{
			case 0x08: //BRA
				LOG_ERROR(RSX, "BRA found. Please report to RPCS3 team.");
				is_has_BRA = true;
				m_jump_lvls.clear();
				d3.HEX = m_data[++i];
				i += 4;
				break;

			case 0x09: //BRI
				d2.HEX = m_data[i++];
				d3.HEX = m_data[i];
				i += 2;
				m_jump_lvls.emplace(GetAddr());
				break;

			default:
				d3.HEX = m_data[++i];
				i += 2;
				break;
			}
		}

		if (d3.end)
		{
			m_instr_count++;

			if (i < m_data.size())
			{
				LOG_ERROR(RSX, "Program end before buffer end.");
			}

			break;
		}
	}

	uint jump_position = 0;

	if (is_has_BRA || !m_jump_lvls.empty())
	{
		m_cur_instr = &m_instructions[0];
		AddCode("int jump_position = 0;");
		AddCode("while (true)");
		AddCode("{");
		m_cur_instr->open_scopes++;

		AddCode(fmt::Format("if (jump_position <= %u)", jump_position++));
		AddCode("{");
		m_cur_instr->open_scopes++;
	}

	for (u32 i = 0; i < m_instr_count; ++i)
	{
		m_cur_instr = &m_instructions[i];

		d0.HEX = m_data[i * 4 + 0];
		d1.HEX = m_data[i * 4 + 1];
		d2.HEX = m_data[i * 4 + 2];
		d3.HEX = m_data[i * 4 + 3];

		src[0].src0l = d2.src0l;
		src[0].src0h = d1.src0h;
		src[1].src1 = d2.src1;
		src[2].src2l = d3.src2l;
		src[2].src2h = d2.src2h;

		if (i && (is_has_BRA || std::find(m_jump_lvls.begin(), m_jump_lvls.end(), i) != m_jump_lvls.end()))
		{
			m_cur_instr->close_scopes++;
			AddCode("}");
			AddCode("");

			AddCode(fmt::Format("if (jump_position <= %u)", jump_position++));
			AddCode("{");
			m_cur_instr->open_scopes++;
		}

		if (!d1.sca_opcode && !d1.vec_opcode)
		{
			AddCode("//nop");
		}

		switch (d1.sca_opcode)
		{
		case RSX_SCA_OPCODE_NOP: break;
		case RSX_SCA_OPCODE_MOV: SetDSTSca("$s"); break;
		case RSX_SCA_OPCODE_RCP: SetDSTSca("(1.0 / $s)"); break;
		case RSX_SCA_OPCODE_RCC: SetDSTSca("clamp(1.0 / $s, 5.42101e-20, 1.884467e19)"); break;
		case RSX_SCA_OPCODE_RSQ: SetDSTSca("1.F / sqrt($s)"); break;
		case RSX_SCA_OPCODE_EXP: SetDSTSca("exp($s)"); break;
		case RSX_SCA_OPCODE_LOG: SetDSTSca("log($s)"); break;
		case RSX_SCA_OPCODE_LIT: SetDSTSca(typeName[3] + "(1.0, $s.x, ($s.x > 0.0 ? exp($s.w * log2($s.y)) : 0.0), 1.0)"); break;
		case RSX_SCA_OPCODE_BRA:
		{
			AddCode("$if ($cond)");
			AddCode("{");
			m_cur_instr->open_scopes++;
			AddCode("jump_position = $a$am;");
			AddCode("continue;");
			m_cur_instr->close_scopes++;
			AddCode("}");
		}
		break;
		/* This triggers opengl driver lost connection error code 7
		case RSX_SCA_OPCODE_BRI: // works differently (BRI o[1].x(TR) L0;)
		{
		uint jump_position;

		if (is_has_BRA)
		{
		jump_position = GetAddr();
		}
		else
		{
		int addr = GetAddr();

		jump_position = 0;
		for (auto pos : m_jump_lvls)
		{
		if (addr == pos)
		break;

		++jump_position;
		}
		}

		AddCode("$ifcond ");
		AddCode("{");
		m_cur_instr->open_scopes++;
		AddCode(fmt::Format("jump_position = %u;", jump_position));
		AddCode("continue;");
		m_cur_instr->close_scopes++;
		AddCode("}");
		}
		break;
		*/
		case RSX_SCA_OPCODE_CAL:
			// works same as BRI
			AddCode("$ifcond $f(); //CAL");
			break;
		case RSX_SCA_OPCODE_CLI:
			// works same as BRI
			AddCode("$ifcond $f(); //CLI");
			break;
		case RSX_SCA_OPCODE_RET:
			// works like BRI but shorter (RET o[1].x(TR);)
			AddCode("$ifcond return;");
			break;
		case RSX_SCA_OPCODE_LG2: SetDSTSca("log2($s)"); break;
		case RSX_SCA_OPCODE_EX2: SetDSTSca("exp2($s)"); break;
		case RSX_SCA_OPCODE_SIN: SetDSTSca("sin($s)"); break;
		case RSX_SCA_OPCODE_COS: SetDSTSca("cos($s)"); break;
		case RSX_SCA_OPCODE_BRB:
			// works differently (BRB o[1].x !b0, L0;)
			LOG_ERROR(RSX, "Unimplemented sca_opcode BRB");
			break;
		case RSX_SCA_OPCODE_CLB: break;
			// works same as BRB
			LOG_ERROR(RSX, "Unimplemented sca_opcode CLB");
			break;
		case RSX_SCA_OPCODE_PSH: break;
			// works differently (PSH o[1].x A0;)
			LOG_ERROR(RSX, "Unimplemented sca_opcode PSH");
			break;
		case RSX_SCA_OPCODE_POP: break;
			// works differently (POP o[1].x;)
			LOG_ERROR(RSX, "Unimplemented sca_opcode POP");
			break;

		default:
			AddCode(fmt::Format("//Unknown vp sca_opcode 0x%x", fmt::by_value(d1.sca_opcode)));
			LOG_ERROR(RSX, "Unknown vp sca_opcode 0x%x", fmt::by_value(d1.sca_opcode));
			Emu.Pause();
			break;
		}

		switch (d1.vec_opcode)
		{
		case RSX_VEC_OPCODE_NOP: break;
		case RSX_VEC_OPCODE_MOV: SetDSTVec("$0"); break;
		case RSX_VEC_OPCODE_MUL: SetDSTVec("($0 * $1)"); break;
		case RSX_VEC_OPCODE_ADD: SetDSTVec("($0 + $2)"); break;
		case RSX_VEC_OPCODE_MAD: SetDSTVec("($0 * $1 + $2)"); break;
		case RSX_VEC_OPCODE_DP3: SetDSTVec("dot($0.xyz, $1.xyz).xxxx"); break;
		case RSX_VEC_OPCODE_DPH: SetDSTVec("dot(float4($0.xyz, 1.0), $1).xxxx"); break;
		case RSX_VEC_OPCODE_DP4: SetDSTVec("dot($0, $1).xxxx"); break;
		case RSX_VEC_OPCODE_DST: SetDSTVec("vec4(distance($0, $1))"); break;
		case RSX_VEC_OPCODE_MIN: SetDSTVec("min($0, $1)"); break;
		case RSX_VEC_OPCODE_MAX: SetDSTVec("max($0, $1)"); break;
		case RSX_VEC_OPCODE_SLT: SetDSTVec("vec4(lessThan($0, $1))"); break;
		case RSX_VEC_OPCODE_SGE: SetDSTVec("vec4(greaterThanEqual($0, $1))"); break;
		case RSX_VEC_OPCODE_ARL: AddCode("$ifcond $a = ivec4($0)$am;");  break;
		case RSX_VEC_OPCODE_FRC: SetDSTVec("frac($0)"); break;
		case RSX_VEC_OPCODE_FLR: SetDSTVec("floor($0)"); break;
		case RSX_VEC_OPCODE_SEQ: SetDSTVec("vec4(equal($0, $1))"); break;
		case RSX_VEC_OPCODE_SFL: SetDSTVec("vec4(equal($0, vec4(0.0)))"); break;
		case RSX_VEC_OPCODE_SGT: SetDSTVec("vec4(greaterThan($0, $1))"); break;
		case RSX_VEC_OPCODE_SLE: SetDSTVec("vec4(lessThanEqual($0, $1))"); break;
		case RSX_VEC_OPCODE_SNE: SetDSTVec("vec4(notEqual($0, $1))"); break;
		case RSX_VEC_OPCODE_STR: SetDSTVec("vec4(equal($0, vec4(1.0)))"); break;
		case RSX_VEC_OPCODE_SSG: SetDSTVec("sign($0)"); break;
		case RSX_VEC_OPCODE_TXL: SetDSTVec("texture($t, $0.xy)"); break;

		default:
			AddCode(fmt::Format("//Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode)));
			LOG_ERROR(RSX, "Unknown vp opcode 0x%x", fmt::by_value(d1.vec_opcode));
			Emu.Pause();
			break;
		}
	}

	if (is_has_BRA || !m_jump_lvls.empty())
	{
		m_cur_instr = &m_instructions[m_instr_count - 1];
		m_cur_instr->close_scopes++;
		AddCode("}");
		AddCode("break;");
		m_cur_instr->close_scopes++;
		AddCode("}");
	}

	std::string result = BuildCode();

	m_jump_lvls.clear();
	m_body.clear();
	if (m_funcs.size() > 2)
	{
		m_funcs.erase(m_funcs.begin() + 2, m_funcs.end());
	}
	return result;
}

#endif