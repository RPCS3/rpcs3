#include "stdafx.h"
#include "CgBinaryProgram.h"

#include "Emu/System.h"
#include "Emu/RSX/RSXVertexProgram.h"

void CgBinaryDisasm::AddScaCodeDisasm(const std::string& code)
{
	verify(HERE), (m_sca_opcode < 21);
	m_arb_shader += rsx_vp_sca_op_names[m_sca_opcode] + code + " ";
}

void CgBinaryDisasm::AddVecCodeDisasm(const std::string& code)
{
	verify(HERE), (m_vec_opcode < 26);
	m_arb_shader += rsx_vp_vec_op_names[m_vec_opcode] + code + " ";
}

std::string CgBinaryDisasm::GetMaskDisasm(bool is_sca)
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

std::string CgBinaryDisasm::GetVecMaskDisasm()
{
	return GetMaskDisasm(false);
}

std::string CgBinaryDisasm::GetScaMaskDisasm()
{
	return GetMaskDisasm(true);
}

std::string CgBinaryDisasm::GetDSTDisasm(bool is_sca)
{
	std::string ret;
	std::string mask = GetMaskDisasm(is_sca);

	switch ((is_sca && d3.sca_dst_tmp != 0x3f) ? 0x1f : d3.dst)
	{
	case 0x1f:
		ret += (is_sca ? fmt::format("R%d", d3.sca_dst_tmp) : fmt::format("R%d", d0.dst_tmp)) + mask;
		break;

	default:
		if (d3.dst > 15)
			rsx_log.error("dst index out of range: %u", d3.dst);

		ret += fmt::format("o[%d]", d3.dst) + mask;
		// Vertex Program supports double destinations, notably in MOV
		if (d0.dst_tmp != 0x3f)
			ret += fmt::format(" R%d", d0.dst_tmp) + mask;
		break;
	}

	return ret;
}

std::string CgBinaryDisasm::GetSRCDisasm(const u32 n)
{
	std::string ret;

	switch (src[n].reg_type)
	{
	case 1: //temp
		ret += "R" + std::to_string(src[n].tmp_src);
		break;
	case 2: //input
		if (d1.input_src < 16)
		{
			ret += fmt::format("v[%d]", d1.input_src);
		}
		else
		{
			rsx_log.error("Bad input src num: %d", u32{ d1.input_src });
			ret += fmt::format("v[%d] # bad src", d1.input_src);
		}
		break;
	case 3: //const
		ret += std::string("c[" + (d3.index_const ? AddAddrRegDisasm() + " + " : "") + std::to_string(d1.const_src) + "]");
		break;

	default:
		rsx_log.error("Bad src%u reg type: %d", n, u32{ src[n].reg_type });
		Emu.Pause();
		break;
	}

	static const std::string f = "xyzw";

	std::string swizzle;

	swizzle += f[src[n].swz_x];
	swizzle += f[src[n].swz_y];
	swizzle += f[src[n].swz_z];
	swizzle += f[src[n].swz_w];

	if (swizzle == "xxxx") swizzle = "x";
	if (swizzle == "yyyy") swizzle = "y";
	if (swizzle == "zzzz") swizzle = "z";
	if (swizzle == "wwww") swizzle = "w";

	if (swizzle != f) ret += '.' + swizzle;

	bool abs = false;

	switch (n)
	{
	case 0: abs = d0.src0_abs; break;
	case 1: abs = d0.src1_abs; break;
	case 2: abs = d0.src2_abs; break;
	}

	if (abs) ret = "|" + ret + "|";
	if (src[n].neg) ret = "-" + ret;

	return ret;
}

void CgBinaryDisasm::SetDSTDisasm(bool is_sca, const std::string& value)
{
	is_sca ? AddScaCodeDisasm() : AddVecCodeDisasm();

	if (d0.cond == 0) return;

	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4
	};

	if (d0.staturate)
	{
		m_arb_shader.pop_back();
		m_arb_shader += "_sat ";
	}

	std::string dest;
	if (d0.cond_update_enable_0 && d0.cond_update_enable_1)
	{
		m_arb_shader.pop_back();
		m_arb_shader += "C ";
		dest = fmt::format("RC%s", GetMaskDisasm(is_sca).c_str());
	}
	else if (d3.dst != 0x1f || (is_sca ? d3.sca_dst_tmp != 0x3f : d0.dst_tmp != 0x3f))
	{
		dest = GetDSTDisasm(is_sca);
	}

	AddCodeCondDisasm(FormatDisasm(dest), value);
}

std::string CgBinaryDisasm::GetTexDisasm()
{
	return fmt::format("TEX%d", 0);
}

std::string CgBinaryDisasm::FormatDisasm(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$",  []() -> std::string { return "$"; } },
		{ "$0",  [this]{ return GetSRCDisasm(0); } },
		{ "$1",  [this]{ return GetSRCDisasm(1); } },
		{ "$2",  [this]{ return GetSRCDisasm(2); } },
		{ "$s",  [this]{ return GetSRCDisasm(2); } },
		{ "$am", [this]{ return AddAddrMaskDisasm(); } },
		{ "$a",  [this]{ return AddAddrRegDisasm(); } },
		{ "$t",  [this]{ return GetTexDisasm(); } },
		{ "$fa", [this]{ return std::to_string(GetAddrDisasm()); } },

		{ "$ifcond ", [this]
		{
			std::string cond = GetCondDisasm();
			if (cond == "true") cond.clear();
			return cond;
		}
		},

		{ "$cond", [this]{ return GetCondDisasm(); } },
	};

	return fmt::replace_all(code, repl_list);
}

std::string CgBinaryDisasm::GetCondDisasm()
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4
	};

	if (d0.cond == 0) return "false";
	if (d0.cond == (lt | gt | eq)) return "true";

	static const char* cond_string_table[(lt | gt | eq) + 1] =
	{
		"ERROR",
		"LT", "EQ", "LE",
		"GT", "NE", "GE",
		"ERROR"
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	if (swizzle == "xxxx") swizzle = "x";
	if (swizzle == "yyyy") swizzle = "y";
	if (swizzle == "zzzz") swizzle = "z";
	if (swizzle == "wwww") swizzle = "w";

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	return fmt::format("(%s%s)", cond_string_table[d0.cond], swizzle.c_str());
}

void CgBinaryDisasm::AddCodeCondDisasm(const std::string& dst, const std::string& src)
{
	enum
	{
		lt = 0x1,
		eq = 0x2,
		gt = 0x4
	};

	if (!d0.cond_test_enable || d0.cond == (lt | gt | eq))
	{
		AddCodeDisasm(dst + ", " + src + ";");
		return;
	}

	if (d0.cond == 0)
	{
		AddCodeDisasm("# " + dst + ", " + src + ";");
		return;
	}

	static const char* cond_string_table[(lt | gt | eq) + 1] =
	{
		"ERROR",
		"LT", "EQ", "LE",
		"GT", "NE", "GE",
		"ERROR"
	};

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle;
	swizzle += f[d0.mask_x];
	swizzle += f[d0.mask_y];
	swizzle += f[d0.mask_z];
	swizzle += f[d0.mask_w];

	if (swizzle == "xxxx") swizzle = "x";
	if (swizzle == "yyyy") swizzle = "y";
	if (swizzle == "zzzz") swizzle = "z";
	if (swizzle == "wwww") swizzle = "w";

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	std::string cond = fmt::format("%s%s", cond_string_table[d0.cond], swizzle.c_str());
	AddCodeDisasm(dst + "(" + cond + ") " + ", " + src + ";");
}


std::string CgBinaryDisasm::AddAddrMaskDisasm()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return std::string(".") + f[d0.addr_swz];
}

std::string CgBinaryDisasm::AddAddrRegDisasm()
{
	static const char f[] = { 'x', 'y', 'z', 'w' };
	return fmt::format("A%d", d0.addr_reg_sel_1) + AddAddrMaskDisasm();
}

u32 CgBinaryDisasm::GetAddrDisasm()
{
	return (d2.iaddrh << 3) | d3.iaddrl;
}

void CgBinaryDisasm::AddCodeDisasm(const std::string& code)
{
	m_arb_shader += FormatDisasm(code) + "\n";
}

void CgBinaryDisasm::SetDSTVecDisasm(const std::string& code)
{
	SetDSTDisasm(false, code);
}

void CgBinaryDisasm::SetDSTScaDisasm(const std::string& code)
{
	SetDSTDisasm(true, code);
}

void CgBinaryDisasm::TaskVP()
{
	m_instr_count = 0;
	bool is_has_BRA = false;

	for (u32 i = 1; m_instr_count < m_max_instr_count; m_instr_count++)
	{
		if (is_has_BRA)
		{
			d3.HEX = m_data[i];
			i += 4;
		}
		else
		{
			d1.HEX = m_data[i++];


			m_sca_opcode = d1.sca_opcode;
			switch (d1.sca_opcode)
			{
			case 0x08: //BRA
				is_has_BRA = true;
				d3.HEX = m_data[++i];
				i += 4;
				AddScaCodeDisasm("# WARNING");
				break;

			case 0x09: //BRI
				d2.HEX = m_data[i++];
				d3.HEX = m_data[i];
				i += 2;
				AddScaCodeDisasm("$ifcond # WARNING");
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
				rsx_log.error("Program end before buffer end.");
			}

			break;
		}
	}

	for (u32 i = 0; i < m_instr_count; ++i)
	{
		d0.HEX = m_data[i * 4 + 0];
		d1.HEX = m_data[i * 4 + 1];
		d2.HEX = m_data[i * 4 + 2];
		d3.HEX = m_data[i * 4 + 3];

		src[0].src0l = d2.src0l;
		src[0].src0h = d1.src0h;
		src[1].src1 = d2.src1;
		src[2].src2l = d3.src2l;
		src[2].src2h = d2.src2h;

		m_sca_opcode = d1.sca_opcode;
		switch (d1.sca_opcode)
		{
		case RSX_SCA_OPCODE_NOP: break;
		case RSX_SCA_OPCODE_MOV: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_RCP: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_RCC: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_RSQ: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_EXP: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_LOG: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_LIT: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_BRA: AddScaCodeDisasm("BRA # WARNING"); break;
		case RSX_SCA_OPCODE_BRI: AddCodeDisasm("$ifcond # WARNING"); break;
		case RSX_SCA_OPCODE_CAL: AddCodeDisasm("$ifcond $f# WARNING"); break;
		case RSX_SCA_OPCODE_CLI: AddCodeDisasm("$ifcond $f # WARNING"); break;
		case RSX_SCA_OPCODE_RET: AddCodeDisasm("$ifcond # WARNING"); break;
		case RSX_SCA_OPCODE_LG2: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_EX2: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_SIN: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_COS: SetDSTScaDisasm("$s"); break;
		case RSX_SCA_OPCODE_BRB: SetDSTScaDisasm("# WARNING Boolean constant"); break;
		case RSX_SCA_OPCODE_CLB: SetDSTScaDisasm("# WARNING Boolean constant"); break;
		case RSX_SCA_OPCODE_PSH: SetDSTScaDisasm(""); break;
		case RSX_SCA_OPCODE_POP: SetDSTScaDisasm(""); break;

		default:
			rsx_log.error("Unknown vp sca_opcode 0x%x", u32{ d1.sca_opcode });
			break;
		}

		m_vec_opcode = d1.vec_opcode;
		switch (d1.vec_opcode)
		{
		case RSX_VEC_OPCODE_NOP: break;
		case RSX_VEC_OPCODE_MOV: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_MUL: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_ADD: SetDSTVecDisasm("$0, $2"); break;
		case RSX_VEC_OPCODE_MAD: SetDSTVecDisasm("$0, $1, $2"); break;
		case RSX_VEC_OPCODE_DP3: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_DPH: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_DP4: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_DST: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_MIN: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_MAX: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_SLT: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_SGE: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_ARL: AddCodeDisasm("ARL, $a, $0"); break;
		case RSX_VEC_OPCODE_FRC: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_FLR: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_SEQ: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_SFL: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_SGT: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_SLE: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_SNE: SetDSTVecDisasm("$0, $1"); break;
		case RSX_VEC_OPCODE_STR: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_SSG: SetDSTVecDisasm("$0"); break;
		case RSX_VEC_OPCODE_TXL: SetDSTVecDisasm("$t, $0"); break;

		default:
			rsx_log.error("Unknown vp opcode 0x%x", u32{ d1.vec_opcode });
			break;
		}
	}

	m_arb_shader += "END\n";
}
