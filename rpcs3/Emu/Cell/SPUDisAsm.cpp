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
			return {};
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
