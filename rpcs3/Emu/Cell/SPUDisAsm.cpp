#include "stdafx.h"
#include "SPUDisAsm.h"
#include "SPUAnalyser.h"
#include "SPUThread.h"

constexpr spu_decoder<SPUDisAsm> s_spu_disasm;
constexpr spu_decoder<spu_itype> s_spu_itype;
constexpr spu_decoder<spu_iflag> s_spu_iflag;

u32 SPUDisAsm::disasm(u32 pc)
{
	const u32 op = *reinterpret_cast<const be_t<u32>*>(offset + pc);
	(this->*(s_spu_disasm.decode(op)))({ op });
	return 4;
}

std::pair<bool, v128> SPUDisAsm::try_get_const_value(u32 reg, u32 pc) const
{
	if (m_mode != CPUDisAsm_InterpreterMode)
	{
		return {};
	}

	if (pc == umax)
	{
		pc = dump_pc;
	}

	// Scan LS backwards from this instruction (until PC=0)
	// Search for the first register modification or branch instruction

	for (s32 i = pc - 4; i >= 0; i -= 4)
	{
		const u32 opcode = *reinterpret_cast<const be_t<u32>*>(offset + i);
		const spu_opcode_t op0{ opcode };

		const auto type = s_spu_itype.decode(opcode);

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

		const auto flag = s_spu_iflag.decode(opcode);

		// TODO: It detects spurious register modifications
		if (u32 dst = type & spu_itype::_quadrop ? +op0.rt4 : +op0.rt; dst == reg)
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
					default: ASSUME(0);
					}

					const u32 index = (~op0.i7 & 0xf) / size;
					auto res = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);

					switch (size)
					{
					case 1: res._u8[index]  = 0x03; break;
					case 2: res._u16[index] = 0x0203; break;
					case 4: res._u32[index] = 0x00010203; break;
					case 8: res._u64[index] = 0x0001020304050607ull; break;
					default: ASSUME(0);
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
				// Avoid multi-recursion for now
				if (dump_pc != pc)
				{
					return {};
				}

				if (i >= 4)
				{
					// Search for ILHU+IOHL pattern (common pattern for 32-bit constants formation)
					// But don't limit to it
					const auto [is_const, value] = try_get_const_value(reg, i);

					if (is_const)
					{
						return { true, value | v128::from32p(op0.i16) };
					}
				}

				return {};
			}
			case spu_itype::STQA:
			case spu_itype::STQD:
			case spu_itype::STQR:
			case spu_itype::STQX:
			case spu_itype::WRCH:
			{
				// Do not modify RT
				break;
			}
			default: return {};
			}
		}
	}

	return {};
}

typename SPUDisAsm::insert_mask_info SPUDisAsm::try_get_insert_mask_info(v128 mask)
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
	const auto [is_const, value] = try_get_const_value(op.rt);

	if (is_const)
	{
		switch (op.ra)
		{
		case MFC_Cmd:
		{
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], MFC(value._u8[12])).c_str());
			return;
		}
		case MFC_WrListStallAck:
		case MFC_WrTagMask:
		{
			const u32 v = value._u32[3];
			if (v && !(v & (v - 1)))
				DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s (tag=%u)", spu_reg_name[op.rt], SignedHex(v), std::countr_zero(v)).c_str()); // Single-tag mask
			else
				DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], SignedHex(v)).c_str()); // Multi-tag mask (or zero)
			return;
		}
		case MFC_EAH:
		{
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], SignedHex(value._u32[3])).c_str());
			return;
		}
		case MFC_Size:
		{
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], SignedHex(value._u16[6])).c_str());
			return;
		}
		case MFC_TagID:
		{
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%u", spu_reg_name[op.rt], value._u8[12]).c_str());
			return;
		}
		case MFC_WrTagUpdate:
		{
			const auto upd = fmt::format("%s", mfc_tag_update(value._u32[3]));
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], upd == "empty" ? "IMMEDIATE" : upd).c_str());
			return;
		}
		default:
		{
			DisAsm("wrch", spu_ch_name[op.ra], fmt::format("%s #%s", spu_reg_name[op.rt], SignedHex(value._u32[3])).c_str());
			return;
		}
		}
	}

	DisAsm("wrch", spu_ch_name[op.ra], spu_reg_name[op.rt]);
}

void SPUDisAsm::IOHL(spu_opcode_t op)
{
	const auto [is_const, value] = try_get_const_value(op.rt);

	if (is_const)
	{
		// Only print constant for a 4 equal 32-bit constants array
		if (value == v128::from32p(value._u32[0]))
		{
			DisAsm("iohl", spu_reg_name[op.rt], fmt::format("%s #%s", SignedHex(+op.i16), (value._u32[0] | op.i16)).c_str());
			return;
		}
	}

	DisAsm("iohl", spu_reg_name[op.rt], op.i16);
}
