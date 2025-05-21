#include "stdafx.h"
#include "CgBinaryProgram.h"

#include "RSXFragmentProgram.h"

#include <algorithm>

void CgBinaryDisasm::AddCodeAsm(const std::string& code)
{
	ensure((m_opcode < 70));
	std::string op_name;

	if (dst.dest_reg == 63)
	{
		m_dst_reg_name = fmt::format("RC%s, ", GetMask());
		op_name = rsx_fp_op_names[m_opcode] + "XC";
	}
	else
	{
		m_dst_reg_name = fmt::format("%s%d%s, ", dst.fp16 ? "H" : "R", dst.dest_reg, GetMask());
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
	case RSX_FP_OPCODE_RET:
		m_dst_reg_name.clear();
		op_name = rsx_fp_op_names[m_opcode] + std::string(dst.fp16 ? "H" : "R");
		break;

	default: break;
	}

	m_arb_shader += (op_name + " " + m_dst_reg_name + FormatDisAsm(code) + ";" + "\n");
}

std::string CgBinaryDisasm::GetMask() const
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

std::string CgBinaryDisasm::AddRegDisAsm(u32 index, int fp16) const
{
	return (fp16 ? 'H' : 'R') + std::to_string(index);
}

std::string CgBinaryDisasm::AddConstDisAsm()
{
	u32* data = reinterpret_cast<u32*>(&m_buffer[m_offset + m_size + 4 * sizeof(u32)]);

	m_step = 2 * 4 * sizeof(u32);
	const u32 x = GetData(data[0]);
	const u32 y = GetData(data[1]);
	const u32 z = GetData(data[2]);
	const u32 w = GetData(data[3]);

	return fmt::format("{0x%08x(%g), 0x%08x(%g), 0x%08x(%g), 0x%08x(%g)}", x, std::bit_cast<f32>(x), y, std::bit_cast<f32>(y), z, std::bit_cast<f32>(z), w, std::bit_cast<f32>(w));
}

std::string CgBinaryDisasm::AddTexDisAsm() const
{
	return (std::string("TEX") + std::to_string(dst.tex_num));
}

std::string CgBinaryDisasm::GetCondDisAsm() const
{
	static constexpr std::string_view f = "xyzw";

	std::string swizzle, cond;
	swizzle.reserve(5);
	swizzle += '.';
	swizzle += f[src0.cond_swizzle_x];
	swizzle += f[src0.cond_swizzle_y];
	swizzle += f[src0.cond_swizzle_z];
	swizzle += f[src0.cond_swizzle_w];

	if (swizzle == ".xxxx"sv) swizzle = ".x";
	else if (swizzle == ".yyyy"sv) swizzle = ".y";
	else if (swizzle == ".zzzz"sv) swizzle = ".z";
	else if (swizzle == ".wwww"sv) swizzle = ".w";

	if (swizzle == ".xyzw"sv)
	{
		swizzle.clear();
	}

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
	const std::pair<std::string_view, std::function<std::string()>> repl_list[] =
	{
		{ "$$",    []() -> std::string { return "$"; } },
		{ "$0",    [this]{ return GetSrcDisAsm<SRC0>(src0); } },
		{ "$1",    [this]{ return GetSrcDisAsm<SRC1>(src1); } },
		{ "$2",    [this]{ return GetSrcDisAsm<SRC2>(src2); } },
		{ "$t",    [this]{ return AddTexDisAsm(); } },
		{ "$m",    [this]{ return GetMask(); } },
		{ "$cond", [this]{ return GetCondDisAsm(); } },
		{ "$c",    [this]{ return AddConstDisAsm(); } },
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

		switch (dst.src_attr_reg_num)
		{
		case 0x00: ret += reg_table[0]; break;
		default:
			if (dst.src_attr_reg_num < std::size(reg_table))
			{
				const std::string perspective_correction = src2.perspective_corr ? "g" : "f";
				const std::string input_attr_reg         = reg_table[dst.src_attr_reg_num];
				fmt::append(ret, "%s[%s]", perspective_correction, input_attr_reg);
			}
			else
			{
				rsx_log.error("Bad src reg num: %d", u32{ dst.src_attr_reg_num });
			}
			break;
		}
		break;
	}

	case 2: //const
		ret += AddConstDisAsm();
		break;

	default:
		rsx_log.error("Bad src type %d", u32{ src.reg_type });
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

	if (swizzle == ".xxxx"sv) swizzle = ".x";
	else if (swizzle == ".yyyy"sv) swizzle = ".y";
	else if (swizzle == ".zzzz"sv) swizzle = ".z";
	else if (swizzle == ".wwww"sv) swizzle = ".w";

	if (swizzle != ".xyzw"sv)
	{
		ret += swizzle;
	}

	if (src.neg) ret = "-" + ret;
	if (src.abs) ret = "|" + ret + "|";

	return ret;
}

void CgBinaryDisasm::TaskFP()
{
	m_size = 0;
	u32* data = reinterpret_cast<u32*>(&m_buffer[m_offset]);
	ensure((m_buffer.size() - m_offset) % sizeof(u32) == 0);

	enum
	{
		FORCE_NONE,
		FORCE_SCT,
		FORCE_SCB
	};

	int forced_unit = FORCE_NONE;

	while (true)
	{
		for (auto found = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
			found != m_end_offsets.end();
			found = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(found);
			m_arb_shader += "ENDIF;\n";
		}

		for (auto found = std::find(m_loop_end_offsets.begin(), m_loop_end_offsets.end(), m_size);
			found != m_loop_end_offsets.end();
			found = std::find(m_loop_end_offsets.begin(), m_loop_end_offsets.end(), m_size))
		{
			m_loop_end_offsets.erase(found);
			m_arb_shader += "ENDLOOP;\n";
		}

		for (auto found = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
			found != m_else_offsets.end();
			found = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size))
		{
			m_else_offsets.erase(found);
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
				break;
			}
			case RSX_FP_OPCODE_LOOP:
			{
				if (!src0.exec_if_eq && !src0.exec_if_gr && !src0.exec_if_lt)
				{
					AddCodeAsm(fmt::format("{ %u, %u, %u }", src1.end_counter, src1.init_counter, src1.increment));
				}
				else
				{
					m_loop_end_offsets.push_back(src2.end_offset << 2);
					AddCodeAsm(fmt::format("{ %u, %u, %u }", src1.end_counter, src1.init_counter, src1.increment));
				}
				break;
			}
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
				break;
			}
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

			rsx_log.error("Unknown/illegal instruction: 0x%x (forced unit %d)", m_opcode, forced_unit);
			break;
		}

		m_size += m_step;

		if (dst.end)
		{
			m_arb_shader.pop_back();
			m_arb_shader += " # last instruction\nEND\n";
			break;
		}

		ensure(m_step % sizeof(u32) == 0);
		data += m_step / sizeof(u32);
	}
}
