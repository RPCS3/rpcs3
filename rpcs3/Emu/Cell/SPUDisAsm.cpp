#include "stdafx.h"
#include "SPUDisAsm.h"
#include "SPUAnalyser.h"
#include "SPUThread.h"

const spu_decoder<SPUDisAsm> s_spu_disasm;
const extern spu_decoder<spu_itype> g_spu_itype;
const extern spu_decoder<spu_iname> g_spu_iname;
const extern spu_decoder<spu_iflag> g_spu_iflag;

#include "util/v128.hpp"
#include "util/simd.hpp"

u32 SPUDisAsm::disasm(u32 pc)
{
	last_opcode.clear();

	if (pc < m_start_pc || pc >= SPU_LS_SIZE)
	{
		return 0;
	}

	dump_pc = pc;
	be_t<u32> op;
	std::memcpy(&op, m_offset + pc, 4);
	m_op = op;
	(this->*(s_spu_disasm.decode(m_op)))({ m_op });

	format_by_mode();
	return 4;
}

std::pair<const void*, usz> SPUDisAsm::get_memory_span() const
{
	return {m_offset + m_start_pc, SPU_LS_SIZE - m_start_pc};
}

std::unique_ptr<CPUDisAsm> SPUDisAsm::copy_type_erased() const
{
	return std::make_unique<SPUDisAsm>(*this);
}

std::pair<bool, v128> SPUDisAsm::try_get_const_value(u32 reg, u32 pc, u32 TTL) const
{
	if (!TTL)
	{
		// Recursion limit (Time To Live)
		return {};
	}

	if (pc == umax)
	{
		// Default arg: choose pc of previous instruction

		if (dump_pc == 0)
		{
			// Do not underflow
			return {};
		}

		pc = dump_pc - 4;
	}

	// Scan LS backwards from this instruction (until PC=0)
	// Search for the first register modification or branch instruction

	for (s32 i = static_cast<s32>(pc); i >= static_cast<s32>(m_start_pc); i -= 4)
	{
		const u32 opcode = *reinterpret_cast<const be_t<u32>*>(m_offset + i);
		const spu_opcode_t op0{ opcode };

		const auto type = g_spu_itype.decode(opcode);

		if (type & spu_itype::branch || type == spu_itype::UNK || !opcode)
		{
			if (reg < 80u)
			{
				return {};
			}

			// We do not care about function calls if register is non-volatile
			if ((type != spu_itype::BRSL && type != spu_itype::BRASL && type != spu_itype::BISL) || op0.rt == reg)
			{
				return {};
			}

			continue;
		}

		// Get constant register value
		#define GET_CONST_REG(var, reg) \
		{\
			/* Search for the constant value of the register*/\
			const auto [is_const, value] = try_get_const_value(reg, i - 4, TTL - 1);\
		\
			if (!is_const)\
			{\
				/* Cannot compute constant value if register is not constant*/\
				return {};\
			}\
		\
			var = value;\
		} void() /*<- Require a semicolon*/

		//const auto flag = g_spu_iflag.decode(opcode);

		if (u32 dst = type & spu_itype::_quadrop ? +op0.rt4 : +op0.rt; dst == reg && !(type & spu_itype::zregmod))
		{
			// Note: It's not 100% reliable because it won't detect branch targets within [i, dump_pc] range (e.g. if-else statement for command's value)
			switch (type)
			{
			case spu_itype::IL:
			{
				return { true, v128::from32p(op0.si16) };
			}
			case spu_itype::ILA:
			{
				return { true, v128::from32p(op0.i18) };
			}
			case spu_itype::ILHU:
			{
				return { true, v128::from32p(op0.i16 << 16) };
			}
			case spu_itype::ILH:
			{
				return { true, v128::from16p(op0.i16) };
			}
			case spu_itype::CBD:
			case spu_itype::CHD:
			case spu_itype::CWD:
			case spu_itype::CDD:
			{
				// Aligned stack assumption
				if (op0.ra == 1u)
				{
					u32 size = 0;

					switch (type)
					{
					case spu_itype::CBD: size = 1; break;
					case spu_itype::CHD: size = 2; break;
					case spu_itype::CWD: size = 4; break;
					case spu_itype::CDD: size = 8; break;
					default: fmt::throw_exception("Unreachable");
					}

					const u32 index = (~op0.i7 & 0xf) / size;
					auto res = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);

					switch (size)
					{
					case 1: res._u8[index]  = 0x03; break;
					case 2: res._u16[index] = 0x0203; break;
					case 4: res._u32[index] = 0x00010203; break;
					case 8: res._u64[index] = 0x0001020304050607ull; break;
					default: fmt::throw_exception("Unreachable");
					}

					return {true, res};
				}

				return {};
			}
			case spu_itype::FSMBI:
			{
				v128 res;

				for (s32 i = 0; i < 16; i++)
				{
					res._u8[i] = (op0.i16 & (1 << i)) ? 0xFF : 0x00;
				}

				return { true, res };
			}
			case spu_itype::IOHL:
			{
				v128 reg_val{};

				// Search for ILHU+IOHL pattern (common pattern for 32-bit constants formation)
				// But don't limit to it
				GET_CONST_REG(reg_val, op0.rt);

				return { true, reg_val | v128::from32p(op0.i16) };
			}
			case spu_itype::SHLQBYI:
			{
				if (op0.si7)
				{
					// Unimplemented, doubt needed
					return {};
				}

				// Move register value operation
				v128 reg_val{};
				GET_CONST_REG(reg_val, op0.ra);

				return { true, reg_val };
			}
			case spu_itype::ORI:
			{
				v128 reg_val{};
				GET_CONST_REG(reg_val, op0.ra);

				return { true, reg_val | v128::from32p(op0.si10) };
			}
			default: return {};
			}
		}
	}

	return {};
}

SPUDisAsm::insert_mask_info SPUDisAsm::try_get_insert_mask_info(const v128& mask)
{
	if ((mask & v128::from8p(0xe0)) != v128{})
	{
		return {};
	}

	s32 first = -1, src_first = 0, last = 16;

	auto access = [&](u32 index) -> u8
	{
		return mask._u8[index ^ 0xf];
	};

	for (s32 i = 0; i < 16; i++)
	{
		if (access(i) & 0x10)
		{
			if ((access(i) & 0x0f) != i)
			{
				return {};
			}

			if (first != -1 && last == 16)
			{
				last = i;
			}

			continue;
		}

		if (last != 16)
		{
			return {};
		}

		if (first == -1)
		{
			src_first = access(i);
			first = i;
		}

		if (src_first + (i - first) != access(i))
		{
			return {};
		}
	}

	if (first == -1)
	{
		return {};
	}

	const u32 size = last - first;

	if ((size | src_first | first) & (size - 1))
	{
		return {};
	}

	if (size == 16)
	{
		// 0x0, 0x1, 0x2, .. 0xE, 0xF is not allowed
		return {};
	}

	// [type size, dst index, src index]
	return {size, first / size, src_first / size};
}

void SPUDisAsm::WRCH(spu_opcode_t op)
{
	DisAsm("wrch", spu_ch_name[op.ra], spu_reg_name[op.rt]);

	const auto [is_const, value] = try_get_const_value(op.rt);

	if (is_const)
	{
		switch (op.ra)
		{
		case MFC_Cmd:
		{
			fmt::append(last_opcode, " #%s", MFC(value._u8[12]));
			return;
		}
		case MFC_WrListStallAck:
		case MFC_WrTagMask:
		{
			const u32 v = value._u32[3];
			if (v && !(v & (v - 1)))
				fmt::append(last_opcode, " #%s (tag=%u)", SignedHex(v), std::countr_zero(v)); // Single-tag mask
			else
				fmt::append(last_opcode, " #%s", SignedHex(v)); // Multi-tag mask (or zero)
			return;
		}
		case MFC_EAH:
		{
			fmt::append(last_opcode, " #%s", SignedHex(value._u32[3]));
			return;
		}
		case MFC_Size:
		{
			fmt::append(last_opcode, " #%s", SignedHex(value._u16[6]));
			return;
		}
		case MFC_TagID:
		{
			fmt::append(last_opcode, " #%u", value._u8[12]);
			return;
		}
		case MFC_WrTagUpdate:
		{
			const auto upd = fmt::format("%s", mfc_tag_update(value._u32[3]));
			fmt::append(last_opcode, " #%s", upd == "empty" ? "IMMEDIATE" : upd);
			return;
		}
		case SPU_WrOutIntrMbox:
		{
			const u32 code = value._u32[3] >> 24;

			if (code == 128u)
			{
				last_opcode += " #sys_event_flag_set_bit";
			}
			else if (code == 192u)
			{
				last_opcode += " #sys_event_flag_set_bit_impatient";
			}
			else
			{
				fmt::append(last_opcode, " #%s", SignedHex(value._u32[3]));
			}

			return;
		}
		default:
		{
			fmt::append(last_opcode, " #%s", SignedHex(value._u32[3]));
			return;
		}
		}
	}
}

enum CellError : u32;

void SPUDisAsm::IOHL(spu_opcode_t op)
{
	DisAsm("iohl", spu_reg_name[op.rt], op.i16);

	const auto [is_const, value] = try_get_const_equal_value_array<u32>(op.rt);

	// Only print constant for a 4 equal 32-bit constants array
	if (is_const)
	{
		comment_constant(last_opcode, value | op.i16);
	}
}

void SPUDisAsm::SHUFB(spu_opcode_t op)
{
	DisAsm("shufb", spu_reg_name[op.rt4], spu_reg_name[op.ra], spu_reg_name[op.rb], spu_reg_name[op.rc]);

	const auto [is_const, value] = try_get_const_value(op.rc);

	if (is_const)
	{
		const auto [size, dst, src] = try_get_insert_mask_info(value);

		if (size)
		{
			if ((size >= 4u && !src) || (size == 2u && src == 1u) || (size == 1u && src == 3u))
			{
				// Comment insertion pattern for CWD-alike instruction
				fmt::append(last_opcode, " #i%u[%u]", size * 8, dst);
				return;
			}

			// Comment insertion pattern for unknown instruction formations
			fmt::append(last_opcode, " #i%u[%u] = [%u]", size * 8, dst, src);
			return;
		}
	}
}
