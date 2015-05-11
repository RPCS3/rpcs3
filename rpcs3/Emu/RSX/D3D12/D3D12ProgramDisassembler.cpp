#include "stdafx.h"
#if defined (DX12_SUPPORT)
#include "D3D12ProgramDisassembler.h"
#include "Emu/Memory/vm.h"
#include "Utilities/Log.h"

static u32 GetData(const u32 d) { return d << 16 | d >> 16; }

void Decompile(RSXFragmentProgram& prog)
{
	auto data = vm::ptr<u32>::make(prog.addr);
	size_t m_size = 0;
	size_t m_location = 0;
	size_t m_loop_count = 0;
	size_t m_code_level = 1;

	enum
	{
		FORCE_NONE,
		FORCE_SCT,
		FORCE_SCB,
	};

	int forced_unit = FORCE_NONE;

	OPDEST operandDST;

	while (true)
	{
		operandDST.HEX = GetData(data[0]);
/*		for (auto finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size);
		finded != m_end_offsets.end();
			finded = std::find(m_end_offsets.begin(), m_end_offsets.end(), m_size))
		{
			m_end_offsets.erase(finded);
			m_code_level--;
			AddCode("}");
			m_loop_count--;
		}*/

/*		for (auto finded = std::find(m_else_offsets.begin(), m_else_offsets.end(), m_size);
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
			case RSX_FP_OPCODE_DP2: SetDst("vec4(dot($0.xy, $1.xy))"); break;
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
			case RSX_FP_OPCODE_FRC: SetDst("fract($0)"); break;
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

		m_size += m_offset;*/

		if (operandDST.end) break;

//		assert(m_offset % sizeof(u32) == 0);
		data += 4 / sizeof(u32);
	}

	// flush m_code_level
	m_code_level = 1;
/*	m_shader = BuildCode();
	main.clear();
	m_parr.params.clear();*/
}
#endif