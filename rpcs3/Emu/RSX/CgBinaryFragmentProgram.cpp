#include "stdafx.h"

#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "CgBinaryProgram.h"
#include "Emu/RSX/RSXFragmentProgram.h"

void CgBinaryDisasm::AddCodeAsm(const std::string& code)
{
	std::string op_name = "";

	if (dst.dest_reg == 63)
	{
		m_dst_reg_name = fmt::format("RC%s, ", GetMask().c_str());
		op_name = rsx_fp_op_names[m_opcode] + "XC";
	}

	else
	{
		m_dst_reg_name = fmt::format("%s%d%s, ", dst.fp16 ? "H" : "R", dst.dest_reg, GetMask().c_str());
		op_name = rsx_fp_op_names[m_opcode] + std::string(dst.fp16 ? "H" : "R");
	}

	switch (m_opcode)
	{
	case RSX_FP_OPCODE_BRK:
	case RSX_FP_OPCODE_CAL:
	case RSX_FP_OPCODE_FENCT:
	case RSX_FP_OPCODE_FENCB:
	case RSX_FP_OPCODE_IFE:
	case RSX_FP_OPCODE_KIL:
	case RSX_FP_OPCODE_LOOP:
	case RSX_FP_OPCODE_NOP:
	case RSX_FP_OPCODE_REP:
	case RSX_FP_OPCODE_RET: m_dst_reg_name = ""; break;

	default: break;
	}

	m_arb_shader += (op_name + " " + m_dst_reg_name + FormatDisAsm(code) + ";" + "\n");
}

std::string CgBinaryDisasm::GetMask()
{
	std::string ret;

	static const char dst_mask[4] =
	{
		'x', 'y', 'z', 'w'
	};

	if (dst.mask_x) ret += dst_mask[0];
	if (dst.mask_y) ret += dst_mask[1];
	if (dst.mask_z) ret += dst_mask[2];
	if (dst.mask_w) ret += dst_mask[3];

	return ret.empty() || strncmp(ret.c_str(), dst_mask, 4) == 0 ? "" : ("." + ret);
}

std::string CgBinaryDisasm::AddRegDisAsm(u32 index, int fp16)
{
	return std::string(fp16 ? "H" : "R") + std::to_string(index);
}

std::string CgBinaryDisasm::AddConstDisAsm()
{
	u32* data = (u32*)&m_buffer[m_offset + m_size + 4 * sizeof(u32)];

	m_step = 2 * 4 * sizeof(u32);
	const u32 x = GetData(data[0]);
	const u32 y = GetData(data[1]);
	const u32 z = GetData(data[2]);
	const u32 w = GetData(data[3]);

	char buf[1024];
	sprintf(buf, "{0x%08x(%g), 0x%08x(%g), 0x%08x(%g), 0x%08x(%g)}", x, (float&)x, y, (float&)y, z, (float&)z, w, (float&)w);

	return fmt::format(buf);
}

std::string CgBinaryDisasm::AddTexDisAsm()
{
	return (std::string("TEX") + std::to_string(dst.tex_num));
}

std::string CgBinaryDisasm::GetCondDisAsm()
{
	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle, cond;
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];

	if (swizzle == "xxxx") swizzle = "x";
	if (swizzle == "yyyy") swizzle = "y";
	if (swizzle == "zzzz") swizzle = "z";
	if (swizzle == "wwww") swizzle = "w";

	swizzle = swizzle == "xyzw" ? "" : "." + swizzle;

	if (src0.exec_if_gr && src0.exec_if_eq)
	{
		cond = "GE";
	}
	else if (src0.exec_if_lt && src0.exec_if_eq)
	{
		cond = "LE";
	}
	else if (src0.exec_if_gr && src0.exec_if_lt)
	{
		cond = "NE";
	}
	else if (src0.exec_if_gr)
	{
		cond = "GT";
	}
	else if (src0.exec_if_lt)
	{
		cond = "LT";
	}
	else if (src0.exec_if_eq)
	{
		cond = "FL";
	}

	else
	{
		cond = "TR";
	}

	return cond + swizzle;
}

std::string CgBinaryDisasm::FormatDisAsm(const std::string& code)
{
	const std::pair<std::string, std::function<std::string()>> repl_list[] =
	{
		{ "$$", []() -> std::string { return "$"; } },
		{ "$0", std::bind(std::mem_fn(&CgBinaryDisasm::GetSrcDisAsm<SRC0>), this, src0) },
		{ "$1", std::bind(std::mem_fn(&CgBinaryDisasm::GetSrcDisAsm<SRC1>), this, src1) },
		{ "$2", std::bind(std::mem_fn(&CgBinaryDisasm::GetSrcDisAsm<SRC2>), this, src2) },
		{ "$t", std::bind(std::mem_fn(&CgBinaryDisasm::AddTexDisAsm), this) },
		{ "$m", std::bind(std::mem_fn(&CgBinaryDisasm::GetMask), this) },
		{ "$cond", std::bind(std::mem_fn(&CgBinaryDisasm::GetCondDisAsm), this) },
		{ "$c", std::bind(std::mem_fn(&CgBinaryDisasm::AddConstDisAsm), this) }
	};

	return fmt::replace_all(code, repl_list);
}

template<typename T> std::string CgBinaryDisasm::GetSrcDisAsm(T src)
{
	std::string ret;

	switch (src.reg_type)
	{
	case 0: //tmp
		ret += AddRegDisAsm(src.tmp_reg_index, src.fp16);
		break;

	case 1: //input
	{
		static const std::string reg_table[] =
		{
			"WPOS", "COL0", "COL1", "FOGC", "TEX0",
			"TEX1", "TEX2", "TEX3", "TEX4", "TEX5",
			"TEX6", "TEX7", "TEX8", "TEX9", "SSA"
		};

		const std::string perspective_correction = src2.perspective_corr ? "g" : "f";
		const std::string input_attr_reg = reg_table[dst.src_attr_reg_num];

		switch (dst.src_attr_reg_num)
		{
		case 0x00: ret += reg_table[0]; break;
		default:
			if (dst.src_attr_reg_num < sizeof(reg_table) / sizeof(reg_table[0]))
			{
				ret += fmt::format("%s[%s]", perspective_correction.c_str(), input_attr_reg.c_str());
			}
			else
			{
				LOG_ERROR(RSX, "Bad src reg num: %d", fmt::by_value(dst.src_attr_reg_num));
			}
			break;
		}
	}
	break;

	case 2: //const
		ret += AddConstDisAsm();
		break;

	default:
		LOG_ERROR(RSX, "Bad src type %d", fmt::by_value(src.reg_type));
		break;
	}

	static const char f[4] = { 'x', 'y', 'z', 'w' };

	std::string swizzle = "";
	swizzle += f[src.swizzle_x];
	swizzle += f[src.swizzle_y];
	swizzle += f[src.swizzle_z];
	swizzle += f[src.swizzle_w];

	if (swizzle == "xxxx") swizzle = "x";
	if (swizzle == "yyyy") swizzle = "y";
	if (swizzle == "zzzz") swizzle = "z";
	if (swizzle == "wwww") swizzle = "w";

	if (strncmp(swizzle.c_str(), f, 4) != 0) ret += "." + swizzle;

	if (src.neg) ret = "-" + ret;

	return ret;
}

void CgBinaryDisasm::TaskFP()
{
	m_size = 0;
	u32* data = (u32*)&m_buffer[m_offset];
	assert((m_buffer_size - m_offset) % sizeof(u32) == 0);
	for (u32 i = 0; i < (m_buffer_size - m_offset) / sizeof(u32); i++)
	{
		data[i] = re32(data[i]);
	}

	enum
	{
		FORCE_NONE,
		FORCE_SCT,
		FORCE_SCB
	};

	int forced_unit = FORCE_NONE;

	while (true)
	{
		for (auto finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
			finded != m_end_offsets.end();
			finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(finded);
			m_arb_shader += "ENDIF;\n";
		}

		for (auto finded = std::find(m_loop_end_offsets.begin(), m_loop_end_offsets.end(), m_size);
			finded != m_loop_end_offsets.end();
			finded = std::find(m_loop_end_offsets.begin(), m_loop_end_offsets.end(), m_size))
		{
			m_loop_end_offsets.erase(finded);
			m_arb_shader += "ENDLOOP;\n";
		}

		for (auto finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
			finded != m_else_offsets.end();
			finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size))
		{
			m_else_offsets.erase(finded);
			m_arb_shader += "ELSE;\n";
		}

		dst.HEX = GetData(data[0]);
		src0.HEX = GetData(data[1]);
		src1.HEX = GetData(data[2]);
		src2.HEX = GetData(data[3]);

		m_step = 4 * sizeof(u32);
		m_opcode = dst.opcode | (src1.opcode_is_branch << 6);

		auto SCT = [&]()
		{
			switch (m_opcode)
			{
			case RSX_FP_OPCODE_ADD: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DIV: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DIVSQ: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP2: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP3: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP4: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP2A: AddCodeAsm("$0, $1, $2"); break;
			case RSX_FP_OPCODE_MAD: AddCodeAsm("$0, $1, $2"); break;
			case RSX_FP_OPCODE_MAX: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_MIN: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_MOV: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_MUL: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_RCP: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_RSQ: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_SEQ: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SFL: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SGE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SGT: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SLE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SLT: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SNE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_STR: AddCodeAsm("$0, $1"); break;

			default:
				return false;
			}

			return true;
		};

		auto SCB = [&]()
		{
			switch (m_opcode)
			{
			case RSX_FP_OPCODE_ADD: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_COS: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_DP2: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP3: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP4: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_DP2A: AddCodeAsm("$0, $1, $2"); break;
			case RSX_FP_OPCODE_DST: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_REFL: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_EX2: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_FLR: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_FRC: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_LIT: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_LIF: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_LRP: AddCodeAsm("# WARNING"); break;
			case RSX_FP_OPCODE_LG2: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_MAD: AddCodeAsm("$0, $1, $2"); break;
			case RSX_FP_OPCODE_MAX: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_MIN: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_MOV: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_MUL: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_PK2: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_PK4: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_PK16: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_PKB: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_PKG: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_SEQ: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SFL: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SGE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SGT: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SIN: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_SLE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SLT: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_SNE: AddCodeAsm("$0, $1"); break;
			case RSX_FP_OPCODE_STR: AddCodeAsm("$0, $1"); break;

			default:
				return false;
			}

			return true;
		};

		auto TEX_SRB = [&]()
		{
			switch (m_opcode)
			{
			case RSX_FP_OPCODE_DDX: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_DDY: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_NRM: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_BEM: AddCodeAsm("# WARNING"); break;
			case RSX_FP_OPCODE_TEX: AddCodeAsm("$0, $t"); break;
			case RSX_FP_OPCODE_TEXBEM: AddCodeAsm("# WARNING"); break;
			case RSX_FP_OPCODE_TXP: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_TXPBEM: AddCodeAsm("# WARNING"); break;
			case RSX_FP_OPCODE_TXD: AddCodeAsm("$0, $1, $t"); break;
			case RSX_FP_OPCODE_TXB: AddCodeAsm("$0, $t"); break;
			case RSX_FP_OPCODE_TXL: AddCodeAsm("$0, $t"); break;
			case RSX_FP_OPCODE_UP2: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_UP4: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_UP16: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_UPB: AddCodeAsm("$0"); break;
			case RSX_FP_OPCODE_UPG: AddCodeAsm("$0"); break;

			default:
				return false;
			}

			return true;
		};

		auto SIP = [&]()
		{
			switch (m_opcode)
			{
			case RSX_FP_OPCODE_BRK: AddCodeAsm("$cond"); break;
			case RSX_FP_OPCODE_CAL: AddCodeAsm("$cond"); break;
			case RSX_FP_OPCODE_FENCT: AddCodeAsm(""); break;
			case RSX_FP_OPCODE_FENCB: AddCodeAsm(""); break;

			case RSX_FP_OPCODE_IFE:
			{
				m_else_offsets.push_back(src1.else_offset << 2);
				m_end_offsets.push_back(src2.end_offset << 2);
				AddCodeAsm("($cond)");
			}
			break;

			case RSX_FP_OPCODE_LOOP:
			{
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					AddCodeAsm(fmt::Format("{ %u, %u, %u }", src1.end_counter, src1.init_counter, src1.increment));
				}
				else
				{
					m_loop_end_offsets.push_back(src2.end_offset << 2);
					AddCodeAsm(fmt::Format("{ %u, %u, %u }", src1.end_counter, src1.init_counter, src1.increment));
				}
			}
			break;

			case RSX_FP_OPCODE_REP:
			{
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					m_arb_shader += "# RSX_FP_OPCODE_REP_1\n";
				}
				else
				{
					m_end_offsets.push_back(src2.end_offset << 2);
					m_arb_shader += "# RSX_FP_OPCODE_REP_2\n";
				}
			}
			break;

			case RSX_FP_OPCODE_RET: AddCodeAsm("$cond"); break;

			default:
				return false;
			}

			return true;
		};

		switch (m_opcode)
		{
		case RSX_FP_OPCODE_NOP: AddCodeAsm(""); break;
		case RSX_FP_OPCODE_KIL: AddCodeAsm("$cond"); break;

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

			LOG_ERROR(RSX, "Unknown/illegal instruction: 0x%x (forced unit %d)", m_opcode, forced_unit);
			break;
		}

		m_size += m_step;

		if (dst.end)
		{
			m_arb_shader.pop_back();
			m_arb_shader += " # last inctruction\nEND\n";
			break;
		}

		assert(m_step % sizeof(u32) == 0);
		data += m_step / sizeof(u32);
	}
}