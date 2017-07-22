#include "stdafx.h"
#include "Emu/System.h"

#include "VertexProgramDecompiler.h"

#include <algorithm>

std::string VertexProgramDecompiler::GetMask(bool is_sca)
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

std::string VertexProgramDecompiler::GetVecMask()
{
	return GetMask(false);
}

std::string VertexProgramDecompiler::GetScaMask()
{
	return GetMask(true);
}

std::string VertexProgramDecompiler::GetDST(bool isSca)
{
	std::string ret;

	std::string mask = GetMask(isSca);

	switch (isSca ? 0x1f : d3.dst)
	{
	case 0x1f:
		ret += m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), std::string("tmp") + std::to_string(isSca ? d3.sca_dst_tmp : d0.dst_tmp)) + mask;
		break;

	default:
		if (d3.dst > 15)
			LOG_ERROR(RSX, "dst index out of range: %u", d3.dst);
		ret += m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), std::string("dst_reg") + std::to_string(d3.dst), d3.dst == 0 ? getFloatTypeName(4) + "(0.0f, 0.0f, 0.0f, 1.0f)" : getFloatTypeName(4) + "(0.0, 0.0, 0.0, 0.0)") + mask;
		// Handle double destination register as 'dst_reg = tmp'
		if (d0.dst_tmp != 0x3f)
			ret += " = " + m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), std::string("tmp") + std::to_string(d0.dst_tmp)) + mask;
		break;
	}

	return ret;
}

std::string VertexProgramDecompiler::GetSRC(const u32 n)
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
	case RSX_VP_REGISTER_TYPE_TEMP:
		ret += m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "tmp" + std::to_string(src[n].tmp_src));
		break;
	case RSX_VP_REGISTER_TYPE_INPUT:
		if (d1.input_src < (sizeof(reg_table) / sizeof(reg_table[0])))
		{
			ret += m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), reg_table[d1.input_src], d1.input_src);
		}
		else
		{
			LOG_ERROR(RSX, "Bad input src num: %d", u32{ d1.input_src });
			ret += m_parr.AddParam(PF_PARAM_IN, getFloatTypeName(4), "in_unk", d1.input_src);
		}
		break;
	case RSX_VP_REGISTER_TYPE_CONSTANT:
		m_parr.AddParam(PF_PARAM_UNIFORM, getFloatTypeName(4), std::string("vc[468]"));
		ret += std::string("vc[") + std::to_string(d1.const_src) + (d3.index_const ? " + " + AddAddrReg() : "") + "]";
		break;

	default:
		LOG_ERROR(RSX, "Bad src%u reg type: %d", n, u32{ src[n].reg_type });
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

	bool abs = false;

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

void VertexProgramDecompiler::SetDST(bool is_sca, std::string value)
{
	if (d0.cond == 0) return;

	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	std::string mask = GetMask(is_sca);

	if (is_sca)
	{
		value = getFloatTypeName(4) + "(" + value + ")";
	}

	value += mask;

	if (d0.staturate)
	{
		value = "clamp(" + value + ", 0.0, 1.0)";
	}

	std::string dest;

	if (d0.cond_update_enable_0 || d0.cond_update_enable_1)
	{
		dest = m_parr.AddParam(PF_PARAM_NONE, getFloatTypeName(4), "cc" + std::to_string(d0.cond_reg_sel_1), getFloatTypeName(4) + "(0., 0., 0., 0.)") + mask;
	}
	else if (d3.dst != 0x1f || (is_sca ? d3.sca_dst_tmp != 0x3f : d0.dst_tmp != 0x3f))
	{
		dest = GetDST(is_sca);
	}

	//std::string code;
	//if (d0.cond_test_enable)
	//	code += "$ifcond ";
	//code += dest + value;
	//AddCode(code + ";");

	AddCodeCond(Format(dest), value);
}

std::string VertexProgramDecompiler::GetFunc()
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

std::string VertexProgramDecompiler::GetTex()
{
	return m_parr.AddParam(PF_PARAM_UNIFORM, "sampler2D", std::string("vtex") + std::to_string(d2.tex_num));
}

std::string VertexProgramDecompiler::Format(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 0) },
		{ "$1", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 1) },
		{ "$2", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 2) },
		{ "$s", std::bind(std::mem_fn(&VertexProgramDecompiler::GetSRC), this, 2) },
		{ "$awm", std::bind(std::mem_fn(&VertexProgramDecompiler::AddAddrRegWithoutMask), this) },
		{ "$am", std::bind(std::mem_fn(&VertexProgramDecompiler::AddAddrMask), this) },
		{ "$a", std::bind(std::mem_fn(&VertexProgramDecompiler::AddAddrReg), this) },

		{ "$t", std::bind(std::mem_fn(&VertexProgramDecompiler::GetTex), this) },

		{ "$fa", [this]()->std::string { return std::to_string(GetAddr()); } },
		{ "$f()", std::bind(std::mem_fn(&VertexProgramDecompiler::GetFunc), this) },
		{ "$ifcond ", [this]() -> std::string
	{
		const std::string& cond = GetCond();
		if (cond == "true") return "";
		return "if(" + cond + ") ";
	}
		},
		{ "$cond", std::bind(std::mem_fn(&VertexProgramDecompiler::GetCond), this) },
		{ "$ifbcond", std::bind(std::mem_fn(&VertexProgramDecompiler::GetOptionalBranchCond), this) }
	};

	return fmt::replace_all(code, repl_list);
}

std::string VertexProgramDecompiler::GetCond()
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4,
	};

	if (d0.cond == 0) return "false";
	if (d0.cond == (lt | gt | eq)) return "true";

	static const COMPARE cond_string_table[(lt | gt | eq) + 1] =
	{
		COMPARE::FUNCTION_SLT, // "error"
		COMPARE::FUNCTION_SLT,
		COMPARE::FUNCTION_SEQ,
		COMPARE::FUNCTION_SLE,
		COMPARE::FUNCTION_SGT,
		COMPARE::FUNCTION_SNE,
		COMPARE::FUNCTION_SGE,
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;
	return "any(" + compareFunction(cond_string_table[d0.cond], "cc" + std::to_string(d0.cond_reg_sel_1) + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)" + swizzle) + ")";
}

std::string VertexProgramDecompiler::GetOptionalBranchCond()
{
	std::string cond_operator = d3.brb_cond_true ? " != " : " == ";
	std::string cond = "(transform_branch_bits & (1 << " + std::to_string(d3.branch_index) + "))" + cond_operator + "0";
	
	return "if (" + cond + ")";
}

void VertexProgramDecompiler::AddCodeCond(const std::string& dst, const std::string& src)
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

	static const COMPARE cond_string_table[(lt | gt | eq) + 1] =
	{
		COMPARE::FUNCTION_SLT, // "error"
		COMPARE::FUNCTION_SLT,
		COMPARE::FUNCTION_SEQ,
		COMPARE::FUNCTION_SLE,
		COMPARE::FUNCTION_SGT,
		COMPARE::FUNCTION_SNE,
		COMPARE::FUNCTION_SGE,
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	std::string cond = compareFunction(cond_string_table[d0.cond], "cc" + std::to_string(d0.cond_reg_sel_1) + swizzle, getFloatTypeName(4) + "(0., 0., 0., 0.)");

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


std::string VertexProgramDecompiler::AddAddrMask()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return std::string(".") + f[d0.addr_swz];
}

std::string VertexProgramDecompiler::AddAddrReg()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return m_parr.AddParam(PF_PARAM_NONE, getIntTypeName(4), "a" + std::to_string(d0.addr_reg_sel_1), getIntTypeName(4) + "(0, 0, 0, 0)") + AddAddrMask();
}

std::string VertexProgramDecompiler::AddAddrRegWithoutMask()
{
	return m_parr.AddParam(PF_PARAM_NONE, getIntTypeName(4), "a" + std::to_string(d0.addr_reg_sel_1), getIntTypeName(4) + "(0, 0, 0, 0)");
}

u32 VertexProgramDecompiler::GetAddr()
{
	return (d2.iaddrh << 3) | d3.iaddrl;
}

void VertexProgramDecompiler::AddCode(const std::string& code)
{
	m_body.push_back(Format(code) + ";");
	m_cur_instr->body.push_back(Format(code));
}

void VertexProgramDecompiler::SetDSTVec(const std::string& code)
{
	SetDST(false, code);
}

void VertexProgramDecompiler::SetDSTSca(const std::string& code)
{
	SetDST(true, code);
}

std::string VertexProgramDecompiler::NotZeroPositive(const std::string code)
{
	return "max(" + code + ", 1.E-10)";
}

std::string VertexProgramDecompiler::BuildFuncBody(const FuncInfo& func)
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

std::string VertexProgramDecompiler::BuildCode()
{
	std::string main_body;
	for (uint i = 0, lvl = 1; i < m_instr_count; i++)
	{
		lvl -= m_instructions[i].close_scopes;
		if (lvl < 1) lvl = 1;
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

VertexProgramDecompiler::VertexProgramDecompiler(const RSXVertexProgram& prog) :
	m_data(prog.data)
{
	m_funcs.emplace_back();
	m_funcs[0].offset = 0;
	m_funcs[0].name = "main";
	m_funcs.emplace_back();
	m_funcs[1].offset = 0;
	m_funcs[1].name = "func0";
	//m_cur_func->body = "\tgl_Position = vec4(0.0f, 0.0f, 0.0f, 1.0f);\n";
}

std::string VertexProgramDecompiler::Decompile()
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
			case RSX_SCA_OPCODE_BRA:
				is_has_BRA = true;
				m_jump_lvls.clear();
				d3.HEX = m_data[++i];
				i += 4;
				break;

			case RSX_SCA_OPCODE_BRB:
			case RSX_SCA_OPCODE_BRI:
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

		AddCode(fmt::format("if (jump_position <= %u)", jump_position++));
		AddCode("{");
		m_cur_instr->open_scopes++;
	}

	auto find_jump_lvl = [this](u32 address)
	{
		u32 jump = 1;

		for (auto pos : m_jump_lvls)
		{
			if (address == pos)
				break;

			++jump;
		}

		return jump;
	};

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

			AddCode(fmt::format("if (jump_position <= %u)", jump_position++));
			AddCode("{");
			m_cur_instr->open_scopes++;
		}

		if (!d1.sca_opcode && !d1.vec_opcode)
		{
			AddCode("//nop");
		}

		switch (d1.vec_opcode)
		{
		case RSX_VEC_OPCODE_NOP: break;
		case RSX_VEC_OPCODE_MOV: SetDSTVec("$0"); break;
		case RSX_VEC_OPCODE_MUL: SetDSTVec("($0 * $1)"); break;
		case RSX_VEC_OPCODE_ADD: SetDSTVec("($0 + $2)"); break;
		case RSX_VEC_OPCODE_MAD: SetDSTVec("($0 * $1 + $2)"); break;
		case RSX_VEC_OPCODE_DP3: SetDSTVec(getFunction(FUNCTION::FUNCTION_DP3)); break;
		case RSX_VEC_OPCODE_DPH: SetDSTVec(getFunction(FUNCTION::FUNCTION_DPH)); break;
		case RSX_VEC_OPCODE_DP4: SetDSTVec(getFunction(FUNCTION::FUNCTION_DP4)); break;
		case RSX_VEC_OPCODE_DST: SetDSTVec("vec4(distance($0, $1))"); break;
		case RSX_VEC_OPCODE_MIN: SetDSTVec("min($0, $1)"); break;
		case RSX_VEC_OPCODE_MAX: SetDSTVec("max($0, $1)"); break;
		case RSX_VEC_OPCODE_SLT: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLT, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SGE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGE, "$0", "$1") + ")"); break;
			// Note: It looks like ARL opcode ignore input/output swizzle mask (SH3)
		case RSX_VEC_OPCODE_ARL: AddCode("$ifcond $awm = " + getIntTypeName(4) + "($0);");  break;
		case RSX_VEC_OPCODE_FRC: SetDSTVec(getFunction(FUNCTION::FUNCTION_FRACT)); break;
		case RSX_VEC_OPCODE_FLR: SetDSTVec("floor($0)"); break;
		case RSX_VEC_OPCODE_SEQ: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SEQ, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SFL: SetDSTVec(getFunction(FUNCTION::FUNCTION_SFL)); break;
		case RSX_VEC_OPCODE_SGT: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SGT, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SLE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SLE, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_SNE: SetDSTVec(getFloatTypeName(4) + "(" + compareFunction(COMPARE::FUNCTION_SNE, "$0", "$1") + ")"); break;
		case RSX_VEC_OPCODE_STR: SetDSTVec(getFunction(FUNCTION::FUNCTION_STR)); break;
		case RSX_VEC_OPCODE_SSG: SetDSTVec("sign($0)"); break;
		case RSX_VEC_OPCODE_TXL: SetDSTVec(getFunction(FUNCTION::FUNCTION_VERTEX_TEXTURE_FETCH2D)); break;

		default:
			AddCode(fmt::format("//Unknown vp opcode 0x%x", u32{ d1.vec_opcode }));
			LOG_ERROR(RSX, "Unknown vp opcode 0x%x", u32{ d1.vec_opcode });
			Emu.Pause();
			break;
		}

		//NOTE: Branch instructions have to be decoded last in case there was a dual-issued instruction!
		switch (d1.sca_opcode)
		{
		case RSX_SCA_OPCODE_NOP: break;
		case RSX_SCA_OPCODE_MOV: SetDSTSca("$s"); break;
		case RSX_SCA_OPCODE_RCP: SetDSTSca("(1.0 / $s)"); break;
		case RSX_SCA_OPCODE_RCC: SetDSTSca("clamp(1.0 / $s, 5.42101e-20, 1.884467e19)"); break;
		case RSX_SCA_OPCODE_RSQ: SetDSTSca("1. / sqrt(" + NotZeroPositive("$s.x") +").xxxx"); break;
		case RSX_SCA_OPCODE_EXP: SetDSTSca("exp($s)"); break;
		case RSX_SCA_OPCODE_LOG: SetDSTSca("log($s)"); break;
		case RSX_SCA_OPCODE_LIT: SetDSTSca("lit_legacy($s)"); break;
		case RSX_SCA_OPCODE_BRA:
		{
			AddCode("$if ($cond) //BRA");
			AddCode("{");
			m_cur_instr->open_scopes++;
			AddCode("jump_position = $a$am;");
			AddCode("continue;");
			m_cur_instr->close_scopes++;
			AddCode("}");
		}
		break;
		case RSX_SCA_OPCODE_BRI: // works differently (BRI o[1].x(TR) L0;)
		{
			u32 jump_position = find_jump_lvl(GetAddr());

			AddCode("$ifcond //BRI");
			AddCode("{");
			m_cur_instr->open_scopes++;
			AddCode(fmt::format("jump_position = %u;", jump_position));
			AddCode("continue;");
			m_cur_instr->close_scopes++;
			AddCode("}");
		}
		break;
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
		case RSX_SCA_OPCODE_LG2: SetDSTSca("log2(" + NotZeroPositive("$s") + ")"); break;
		case RSX_SCA_OPCODE_EX2: SetDSTSca("exp2($s)"); break;
		case RSX_SCA_OPCODE_SIN: SetDSTSca("sin($s)"); break;
		case RSX_SCA_OPCODE_COS: SetDSTSca("cos($s)"); break;
		case RSX_SCA_OPCODE_BRB:
			// works differently (BRB o[1].x !b0, L0;)
		{
			LOG_WARNING(RSX, "sca_opcode BRB, d0=0x%X, d1=0x%X, d2=0x%X, d3=0x%X", d0.HEX, d1.HEX, d2.HEX, d3.HEX);
			AddCode(fmt::format("//BRB opcode, d0=0x%X, d1=0x%X, d2=0x%X, d3=0x%X", d0.HEX, d1.HEX, d2.HEX, d3.HEX));
			
			u32 jump_position = find_jump_lvl(GetAddr());
			
			AddCode("$ifbcond //BRB");
			AddCode("{");
			m_cur_instr->open_scopes++;
			AddCode(fmt::format("jump_position = %u;", jump_position));
			AddCode("continue;");
			m_cur_instr->close_scopes++;
			AddCode("}");
			AddCode("");

			break;
		}
		case RSX_SCA_OPCODE_CLB: break;
			// works same as BRB
			LOG_WARNING(RSX, "sca_opcode CLB, d0=0x%X, d1=0x%X, d2=0x%X, d3=0x%X", d0.HEX, d1.HEX, d2.HEX, d3.HEX);
			AddCode("//CLB");
			
			AddCode("$ifbcond $f(); //CLB");
			AddCode("");

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
			AddCode(fmt::format("//Unknown vp sca_opcode 0x%x", u32{ d1.sca_opcode }));
			LOG_ERROR(RSX, "Unknown vp sca_opcode 0x%x", u32{ d1.sca_opcode });
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
