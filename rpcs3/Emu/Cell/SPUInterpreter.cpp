#include "stdafx.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "SPUThread.h"
#include "SPUInterpreter.h"

#include <cmath>
#include <cfenv>

// Compare 16 packed unsigned bytes (greater than)
inline __m128i sse_cmpgt_epu8(__m128i A, __m128i B)
{
	// (A xor 0x80) > (B xor 0x80)
	const auto sign = _mm_set1_epi32(0x80808080);
	return _mm_cmpgt_epi8(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu16(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80008000);
	return _mm_cmpgt_epi16(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

inline __m128i sse_cmpgt_epu32(__m128i A, __m128i B)
{
	const auto sign = _mm_set1_epi32(0x80000000);
	return _mm_cmpgt_epi32(_mm_xor_si128(A, sign), _mm_xor_si128(B, sign));
}

void spu_interpreter::UNK(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unknown/Illegal instruction (0x%08x)" HERE, op.opcode);
}


void spu_interpreter::set_interrupt_status(SPUThread& spu, spu_opcode_t op)
{
	if (op.e)
	{
		if (op.d)
		{
			fmt::throw_exception("Undefined behaviour" HERE);
		}

		spu.set_interrupt_status(true);
	}
	else if (op.d)
	{
		spu.set_interrupt_status(false);
	}

	if ((spu.ch_event_stat & SPU_EVENT_INTR_TEST & spu.ch_event_mask) > SPU_EVENT_INTR_ENABLED)
	{
		spu.ch_event_stat &= ~SPU_EVENT_INTR_ENABLED;
		spu.srr0 = std::exchange(spu.pc, -4) + 4;
	}
}


void spu_interpreter::STOP(SPUThread& spu, spu_opcode_t op)
{
	if (!spu.stop_and_signal(op.opcode & 0x3fff))
	{
		spu.pc -= 4;
	}
}

void spu_interpreter::LNOP(SPUThread& spu, spu_opcode_t op)
{
}

// This instruction must be used following a store instruction that modifies the instruction stream.
void spu_interpreter::SYNC(SPUThread& spu, spu_opcode_t op)
{
	_mm_mfence(); 
}

// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
void spu_interpreter::DSYNC(SPUThread& spu, spu_opcode_t op)
{
	_mm_mfence();
}

void spu_interpreter::MFSPR(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].clear(); // All SPRs read as zero. TODO: check it.
}

void spu_interpreter::RDCH(SPUThread& spu, spu_opcode_t op)
{
	u32 result;

	if (!spu.get_ch_value(op.ra, result))
	{
		spu.pc -= 4;
	}
	else
	{
		spu.gpr[op.rt] = v128::from32r(result);
	}
}

void spu_interpreter::RCHCNT(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(spu.get_ch_count(op.ra));
}

void spu_interpreter::SF(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::sub32(spu.gpr[op.rb], spu.gpr[op.ra]);
}

void spu_interpreter::OR(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] | spu.gpr[op.rb];
}

void spu_interpreter::BG(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_add_epi32(sse_cmpgt_epu32(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi), _mm_set1_epi32(1));
}

void spu_interpreter::SFH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::sub16(spu.gpr[op.rb], spu.gpr[op.ra]);
}

void spu_interpreter::NOR(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] | spu.gpr[op.rb]);
}

void spu_interpreter::ABSDB(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	spu.gpr[op.rt] = v128::sub8(v128::maxu8(a, b), v128::minu8(a, b));
}

void spu_interpreter::ROT(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		spu.gpr[op.rt]._u32[i] = rol32(a._u32[i], b._u32[i]);
	}
}

void spu_interpreter::ROTM(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		spu.gpr[op.rt]._u32[i] = static_cast<u32>(value >> (0 - b._u32[i]));
	}
}

void spu_interpreter::ROTMA(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const s64 value = a._s32[i];
		spu.gpr[op.rt]._s32[i] = static_cast<s32>(value >> (0 - b._u32[i]));
	}
}

void spu_interpreter::SHL(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		spu.gpr[op.rt]._u32[i] = static_cast<u32>(value << b._u32[i]);
	}
}

void spu_interpreter::ROTH(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		spu.gpr[op.rt]._u16[i] = rol16(a._u16[i], b._s16[i]);
	}
}

void spu_interpreter::ROTHM(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		spu.gpr[op.rt]._u16[i] = static_cast<u16>(value >> (0 - b._u16[i]));
	}
}

void spu_interpreter::ROTMAH(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const s32 value = a._s16[i];
		spu.gpr[op.rt]._s16[i] = static_cast<s16>(value >> (0 - b._u16[i]));
	}
}

void spu_interpreter::SHLH(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		spu.gpr[op.rt]._u16[i] = static_cast<u16>(value << b._u16[i]);
	}
}

void spu_interpreter::ROTI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = op.i7 & 0x1f;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi32(a, n), _mm_srli_epi32(a, 32 - n));
}

void spu_interpreter::ROTMI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srli_epi32(spu.gpr[op.ra].vi, 0-op.i7 & 0x3f);
}

void spu_interpreter::ROTMAI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srai_epi32(spu.gpr[op.ra].vi, 0-op.i7 & 0x3f);
}

void spu_interpreter::SHLI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_slli_epi32(spu.gpr[op.ra].vi, op.i7 & 0x3f);
}

void spu_interpreter::ROTHI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = op.i7 & 0xf;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi16(a, n), _mm_srli_epi16(a, 16 - n));
}

void spu_interpreter::ROTHMI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srli_epi16(spu.gpr[op.ra].vi, 0-op.i7 & 0x1f);
}

void spu_interpreter::ROTMAHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srai_epi16(spu.gpr[op.ra].vi, 0-op.i7 & 0x1f);
}

void spu_interpreter::SHLHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_slli_epi16(spu.gpr[op.ra].vi, op.i7 & 0x1f);
}

void spu_interpreter::A(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::add32(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter::AND(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] & spu.gpr[op.rb];
}

void spu_interpreter::CG(SPUThread& spu, spu_opcode_t op)
{
	const auto a = _mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(0x7fffffff));
	const auto b = _mm_xor_si128(spu.gpr[op.rb].vi, _mm_set1_epi32(0x80000000));
	spu.gpr[op.rt].vi = _mm_srli_epi32(_mm_cmpgt_epi32(b, a), 31);
}

void spu_interpreter::AH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::add16(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter::NAND(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] & spu.gpr[op.rb]);
}

void spu_interpreter::AVGB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_avg_epu8(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::MTSPR(SPUThread& spu, spu_opcode_t op)
{
	// SPR writes are ignored. TODO: check it.
}

void spu_interpreter::WRCH(SPUThread& spu, spu_opcode_t op)
{
	if (!spu.set_ch_value(op.ra, spu.gpr[op.rt]._u32[3]))
	{
		spu.pc -= 4;
	}
}

void spu_interpreter::BIZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] == 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]) - 4;
		set_interrupt_status(spu, op);
	}
}

void spu_interpreter::BINZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] != 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]) - 4;
		set_interrupt_status(spu, op);
	}
}

void spu_interpreter::BIHZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] == 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]) - 4;
		set_interrupt_status(spu, op);
	}
}

void spu_interpreter::BIHNZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] != 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]) - 4;
		set_interrupt_status(spu, op);
	}
}

void spu_interpreter::STOPD(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unimplemented instruction" HERE);
}

void spu_interpreter::STQX(SPUThread& spu, spu_opcode_t op)
{
	spu._ref<v128>((spu.gpr[op.ra]._u32[3] + spu.gpr[op.rb]._u32[3]) & 0x3fff0) = spu.gpr[op.rt];
}

void spu_interpreter::BI(SPUThread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]) - 4;
	set_interrupt_status(spu, op);
}

void spu_interpreter::BISL(SPUThread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(spu.gpr[op.ra]._u32[3]);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target - 4;
	set_interrupt_status(spu, op);
}

void spu_interpreter::IRET(SPUThread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(spu.srr0) - 4;
	set_interrupt_status(spu, op);
}

void spu_interpreter::BISLED(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unimplemented instruction" HERE);
}

void spu_interpreter::HBR(SPUThread& spu, spu_opcode_t op)
{
}

void spu_interpreter::GB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_epi8(_mm_slli_epi64(_mm_shuffle_epi8(spu.gpr[op.ra].vi, _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 8, 4, 0)), 7)));
}

void spu_interpreter::GBH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_epi8(_mm_slli_epi64(_mm_shuffle_epi8(spu.gpr[op.ra].vi, _mm_set_epi8(-1, -1, -1, -1, -1, -1, -1, -1, 14, 12, 10, 8, 6, 4, 2, 0)), 7)));
}

void spu_interpreter::GBB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_epi8(_mm_slli_epi64(spu.gpr[op.ra].vi, 7)));
}

void spu_interpreter::FSM(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = g_spu_imm.fsm[spu.gpr[op.ra]._u32[3] & 0xf];
}

void spu_interpreter::FSMH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = g_spu_imm.fsmh[spu.gpr[op.ra]._u32[3] & 0xff];
}

void spu_interpreter::FSMB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = g_spu_imm.fsmb[spu.gpr[op.ra]._u32[3] & 0xffff];
}

void spu_interpreter_fast::FREST(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vf = _mm_rcp_ps(spu.gpr[op.ra].vf);
}

void spu_interpreter_fast::FRSQEST(SPUThread& spu, spu_opcode_t op)
{
	const auto mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	spu.gpr[op.rt].vf = _mm_rsqrt_ps(_mm_and_ps(spu.gpr[op.ra].vf, mask));
}

void spu_interpreter::LQX(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>((spu.gpr[op.ra]._u32[3] + spu.gpr[op.rb]._u32[3]) & 0x3fff0);
}

void spu_interpreter::ROTQBYBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.rldq_pshufb[spu.gpr[op.rb]._u32[3] >> 3 & 0xf].vi);
}

void spu_interpreter::ROTQMBYBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.srdq_pshufb[-(spu.gpr[op.rb]._s32[3] >> 3) & 0x1f].vi);
}

void spu_interpreter::SHLQBYBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.sldq_pshufb[spu.gpr[op.rb]._u32[3] >> 3 & 0x1f].vi);
}

void spu_interpreter::CBX(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = ~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xf;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u8[t] = 0x03;
}

void spu_interpreter::CHX(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xe) >> 1;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u16[t] = 0x0203;
}

void spu_interpreter::CWX(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xc) >> 2;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u32[t] = 0x00010203;
}

void spu_interpreter::CDX(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0x8) >> 3;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u64[t] = 0x0001020304050607ull;
}

void spu_interpreter::ROTQBI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = spu.gpr[op.rb]._s32[3] & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_alignr_epi8(a, a, 8), 64 - n));
}

void spu_interpreter::ROTQMBI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = -spu.gpr[op.rb]._s32[3] & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_srli_epi64(a, n), _mm_slli_epi64(_mm_srli_si128(a, 8), 64 - n));
}

void spu_interpreter::SHLQBI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = spu.gpr[op.rb]._u32[3] & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_slli_si128(a, 8), 64 - n));
}

void spu_interpreter::ROTQBY(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.rldq_pshufb[spu.gpr[op.rb]._u32[3] & 0xf].vi);
}

void spu_interpreter::ROTQMBY(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.srdq_pshufb[-spu.gpr[op.rb]._s32[3] & 0x1f].vi);
}

void spu_interpreter::SHLQBY(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.sldq_pshufb[spu.gpr[op.rb]._u32[3] & 0x1f].vi);
}

void spu_interpreter::ORX(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(spu.gpr[op.ra]._u32[0] | spu.gpr[op.ra]._u32[1] | spu.gpr[op.ra]._u32[2] | spu.gpr[op.ra]._u32[3]);
}

void spu_interpreter::CBD(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = ~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xf;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u8[t] = 0x03;
}

void spu_interpreter::CHD(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xe) >> 1;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u16[t] = 0x0203;
}

void spu_interpreter::CWD(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xc) >> 2;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u32[t] = 0x00010203;
}

void spu_interpreter::CDD(SPUThread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x" HERE, spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0x8) >> 3;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u64[t] = 0x0001020304050607ull;
}

void spu_interpreter::ROTQBII(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = op.i7 & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_alignr_epi8(a, a, 8), 64 - n));
}

void spu_interpreter::ROTQMBII(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = 0-op.i7 & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_srli_epi64(a, n), _mm_slli_epi64(_mm_srli_si128(a, 8), 64 - n));
}

void spu_interpreter::SHLQBII(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const s32 n = op.i7 & 0x7;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_slli_si128(a, 8), 64 - n));
}

void spu_interpreter::ROTQBYI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.rldq_pshufb[op.i7 & 0xf].vi);
}

void spu_interpreter::ROTQMBYI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.srdq_pshufb[0-op.i7 & 0x1f].vi);
}

void spu_interpreter::SHLQBYI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(spu.gpr[op.ra].vi, g_spu_imm.sldq_pshufb[op.i7 & 0x1f].vi);
}

void spu_interpreter::NOP(SPUThread& spu, spu_opcode_t op)
{
}

void spu_interpreter::CGT(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi32(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::XOR(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] ^ spu.gpr[op.rb];
}

void spu_interpreter::CGTH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi16(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::EQV(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] ^ spu.gpr[op.rb]);
}

void spu_interpreter::CGTB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi8(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::SUMB(SPUThread& spu, spu_opcode_t op)
{
	const auto ones = _mm_set1_epi8(1);
	const auto a = _mm_maddubs_epi16(spu.gpr[op.ra].vi, ones);
	const auto b = _mm_maddubs_epi16(spu.gpr[op.rb].vi, ones);
	spu.gpr[op.rt].vi = _mm_shuffle_epi8(_mm_hadd_epi16(a, b), _mm_set_epi8(15, 14, 7, 6, 13, 12, 5, 4, 11, 10, 3, 2, 9, 8, 1, 0));
}

void spu_interpreter::HGT(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] > spu.gpr[op.rb]._s32[3])
	{
		spu.halt();
	}
}

void spu_interpreter::CLZ(SPUThread& spu, spu_opcode_t op)
{
	for (u32 i = 0; i < 4; i++)
	{
		spu.gpr[op.rt]._u32[i] = cntlz32(spu.gpr[op.ra]._u32[i]);
	}
}

void spu_interpreter::XSWD(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt]._s64[0] = spu.gpr[op.ra]._s32[0];
	spu.gpr[op.rt]._s64[1] = spu.gpr[op.ra]._s32[2];
}

void spu_interpreter::XSHW(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srai_epi32(_mm_slli_epi32(spu.gpr[op.ra].vi, 16), 16);
}

void spu_interpreter::CNTB(SPUThread& spu, spu_opcode_t op)
{
	const auto counts = _mm_set_epi8(4, 3, 3, 2, 3, 2, 2, 1, 3, 2, 2, 1, 2, 1, 1, 0);
	const auto mask = _mm_set1_epi8(0xf);
	const auto a = spu.gpr[op.ra].vi;
	spu.gpr[op.rt].vi = _mm_add_epi8(_mm_shuffle_epi8(counts, _mm_and_si128(a, mask)), _mm_shuffle_epi8(counts, _mm_and_si128(_mm_srli_epi64(a, 4), mask)));
}

void spu_interpreter::XSBH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srai_epi16(_mm_slli_epi16(spu.gpr[op.ra].vi, 8), 8);
}

void spu_interpreter::CLGT(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = sse_cmpgt_epu32(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::ANDC(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::andnot(spu.gpr[op.rb], spu.gpr[op.ra]);
}

void spu_interpreter_fast::FCGT(SPUThread& spu, spu_opcode_t op)
{
	// IMPL NOTES:
	// if (v is inf) v = (inf - 1) i.e nearest normal value to inf with mantissa bits left intact
	// if (v is denormalized) v = 0 flush denormals
	// return v1 > v2
	// branching simulated using bitwise ops and_not+or

	const auto zero = _mm_set1_ps(0.f);
	const auto nan_check_a = _mm_cmpunord_ps(spu.gpr[op.ra].vf, zero);    //mask true where a is extended
	const auto nan_check_b = _mm_cmpunord_ps(spu.gpr[op.rb].vf, zero);    //mask true where b is extended

	//calculate lowered a and b. The mantissa bits are left untouched for now unless its proven they should be flushed
	const auto last_exp_bit = _mm_castsi128_ps(_mm_set1_epi32(0x00800000));
	const auto lowered_a =_mm_andnot_ps(last_exp_bit, spu.gpr[op.ra].vf);      //a is lowered to largest unextended value with sign
	const auto lowered_b = _mm_andnot_ps(last_exp_bit, spu.gpr[op.rb].vf);		//b is lowered to largest unextended value with sign

	//check if a and b are denormalized
	const auto all_exp_bits = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
	const auto denorm_check_a = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.ra].vf));
	const auto denorm_check_b = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.rb].vf));

	//set a and b to their lowered values if they are extended
	const auto a_values_lowered = _mm_and_ps(nan_check_a, lowered_a);
	const auto original_a_masked = _mm_andnot_ps(nan_check_a, spu.gpr[op.ra].vf);
	const auto a_final1 = _mm_or_ps(a_values_lowered, original_a_masked);

	const auto b_values_lowered = _mm_and_ps(nan_check_b, lowered_b);
	const auto original_b_masked = _mm_andnot_ps(nan_check_b, spu.gpr[op.rb].vf);
	const auto b_final1 = _mm_or_ps(b_values_lowered, original_b_masked);

	//Flush denormals to zero
	const auto final_a = _mm_andnot_ps(denorm_check_a, a_final1);
	const auto final_b = _mm_andnot_ps(denorm_check_b, b_final1);

	spu.gpr[op.rt].vf = _mm_cmplt_ps(final_b, final_a);
}

void spu_interpreter::DFCGT(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_interpreter_fast::FA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::addfs(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter_fast::FS(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::subfs(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter_fast::FM(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vf = _mm_mul_ps(spu.gpr[op.ra].vf, spu.gpr[op.rb].vf);
}

void spu_interpreter::CLGTH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = sse_cmpgt_epu16(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::ORC(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] | ~spu.gpr[op.rb];
}

void spu_interpreter_fast::FCMGT(SPUThread& spu, spu_opcode_t op)
{
	//IMPL NOTES: See FCGT

	const auto zero = _mm_set1_ps(0.f);
	const auto nan_check_a = _mm_cmpunord_ps(spu.gpr[op.ra].vf, zero);    //mask true where a is extended
	const auto nan_check_b = _mm_cmpunord_ps(spu.gpr[op.rb].vf, zero);    //mask true where b is extended

	//check if a and b are denormalized
	const auto all_exp_bits = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
	const auto denorm_check_a = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.ra].vf));
	const auto denorm_check_b = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.rb].vf));

	//Flush denormals to zero
	const auto final_a = _mm_andnot_ps(denorm_check_a, spu.gpr[op.ra].vf);
	const auto final_b = _mm_andnot_ps(denorm_check_b, spu.gpr[op.rb].vf);

	//Mask to make a > b if a is extended but b is not (is this necessary on x86?)
	const auto nan_mask = _mm_andnot_ps(nan_check_b, _mm_xor_ps(nan_check_a, nan_check_b));

	const auto sign_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	const auto comparison = _mm_cmplt_ps(_mm_and_ps(final_b, sign_mask), _mm_and_ps(final_a, sign_mask));

	spu.gpr[op.rt].vf = _mm_or_ps(comparison, nan_mask);
}

void spu_interpreter::DFCMGT(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_interpreter_fast::DFA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::addfd(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter_fast::DFS(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::subfd(spu.gpr[op.ra], spu.gpr[op.rb]);
}

void spu_interpreter_fast::DFM(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vd = _mm_mul_pd(spu.gpr[op.ra].vd, spu.gpr[op.rb].vd);
}

void spu_interpreter::CLGTB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = sse_cmpgt_epu8(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::HLGT(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._u32[3] > spu.gpr[op.rb]._u32[3])
	{
		spu.halt();
	}
}

void spu_interpreter_fast::DFMA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vd = _mm_add_pd(_mm_mul_pd(spu.gpr[op.ra].vd, spu.gpr[op.rb].vd), spu.gpr[op.rt].vd);
}

void spu_interpreter_fast::DFMS(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vd = _mm_sub_pd(_mm_mul_pd(spu.gpr[op.ra].vd, spu.gpr[op.rb].vd), spu.gpr[op.rt].vd);
}

void spu_interpreter_fast::DFNMS(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vd = _mm_sub_pd(spu.gpr[op.rt].vd, _mm_mul_pd(spu.gpr[op.ra].vd, spu.gpr[op.rb].vd));
}

void spu_interpreter_fast::DFNMA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vd = _mm_sub_pd(_mm_set1_pd(0.0), _mm_add_pd(_mm_mul_pd(spu.gpr[op.ra].vd, spu.gpr[op.rb].vd), spu.gpr[op.rt].vd));
}

void spu_interpreter::CEQ(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi32(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter::MPYHHU(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const auto b = spu.gpr[op.rb].vi;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_srli_epi32(_mm_mullo_epi16(a, b), 16), _mm_and_si128(_mm_mulhi_epu16(a, b), _mm_set1_epi32(0xffff0000)));
}

void spu_interpreter::ADDX(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::add32(v128::add32(spu.gpr[op.ra], spu.gpr[op.rb]), spu.gpr[op.rt] & v128::from32p(1));
}

void spu_interpreter::SFX(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::sub32(v128::sub32(spu.gpr[op.rb], spu.gpr[op.ra]), v128::andnot(spu.gpr[op.rt], v128::from32p(1)));
}

void spu_interpreter::CGX(SPUThread& spu, spu_opcode_t op)
{
	for (s32 i = 0; i < 4; i++)
	{
		const u64 carry = spu.gpr[op.rt]._u32[i] & 1;
		spu.gpr[op.rt]._u32[i] = (carry + spu.gpr[op.ra]._u32[i] + spu.gpr[op.rb]._u32[i]) >> 32;
	}
}

void spu_interpreter::BGX(SPUThread& spu, spu_opcode_t op)
{
	for (s32 i = 0; i < 4; i++)
	{
		const s64 result = (u64)spu.gpr[op.rb]._u32[i] - (u64)spu.gpr[op.ra]._u32[i] - (u64)(1 - (spu.gpr[op.rt]._u32[i] & 1));
		spu.gpr[op.rt]._u32[i] = result >= 0;
	}
}

void spu_interpreter::MPYHHA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_add_epi32(spu.gpr[op.rt].vi, _mm_madd_epi16(_mm_srli_epi32(spu.gpr[op.ra].vi, 16), _mm_srli_epi32(spu.gpr[op.rb].vi, 16)));
}

void spu_interpreter::MPYHHAU(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const auto b = spu.gpr[op.rb].vi;
	spu.gpr[op.rt].vi = _mm_add_epi32(spu.gpr[op.rt].vi, _mm_or_si128(_mm_srli_epi32(_mm_mullo_epi16(a, b), 16), _mm_and_si128(_mm_mulhi_epu16(a, b), _mm_set1_epi32(0xffff0000))));
}

void spu_interpreter_fast::FSCRRD(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].clear();
}

void spu_interpreter_fast::FESD(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vf;
	spu.gpr[op.rt].vd = _mm_cvtps_pd(_mm_shuffle_ps(a, a, 0x8d));
}

void spu_interpreter_fast::FRDS(SPUThread& spu, spu_opcode_t op)
{
	const auto t = _mm_cvtpd_ps(spu.gpr[op.ra].vd);
	spu.gpr[op.rt].vf = _mm_shuffle_ps(t, t, 0x72);
}

void spu_interpreter_fast::FSCRWR(SPUThread& spu, spu_opcode_t op)
{
}

void spu_interpreter::DFTSV(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_interpreter_fast::FCEQ(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vf = _mm_cmpeq_ps(spu.gpr[op.rb].vf, spu.gpr[op.ra].vf);
}

void spu_interpreter::DFCEQ(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_interpreter::MPY(SPUThread& spu, spu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0xffff);
	spu.gpr[op.rt].vi = _mm_madd_epi16(_mm_and_si128(spu.gpr[op.ra].vi, mask), _mm_and_si128(spu.gpr[op.rb].vi, mask));
}

void spu_interpreter::MPYH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_slli_epi32(_mm_mullo_epi16(_mm_srli_epi32(spu.gpr[op.ra].vi, 16), spu.gpr[op.rb].vi), 16);
}

void spu_interpreter::MPYHH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_madd_epi16(_mm_srli_epi32(spu.gpr[op.ra].vi, 16), _mm_srli_epi32(spu.gpr[op.rb].vi, 16));
}

void spu_interpreter::MPYS(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_srai_epi32(_mm_slli_epi32(_mm_mulhi_epi16(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi), 16), 16);
}

void spu_interpreter::CEQH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi16(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter_fast::FCMEQ(SPUThread& spu, spu_opcode_t op)
{
	const auto mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	spu.gpr[op.rt].vf = _mm_cmpeq_ps(_mm_and_ps(spu.gpr[op.rb].vf, mask), _mm_and_ps(spu.gpr[op.ra].vf, mask));
}

void spu_interpreter::DFCMEQ(SPUThread& spu, spu_opcode_t op)
{
	fmt::throw_exception("Unexpected instruction" HERE);
}

void spu_interpreter::MPYU(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const auto b = spu.gpr[op.rb].vi;
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi32(_mm_mulhi_epu16(a, b), 16), _mm_and_si128(_mm_mullo_epi16(a, b), _mm_set1_epi32(0xffff)));
}

void spu_interpreter::CEQB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi8(spu.gpr[op.ra].vi, spu.gpr[op.rb].vi);
}

void spu_interpreter_fast::FI(SPUThread& spu, spu_opcode_t op)
{
	// TODO
	const auto mask_se = _mm_castsi128_ps(_mm_set1_epi32(0xff800000)); // sign and exponent mask
	const auto mask_bf = _mm_castsi128_ps(_mm_set1_epi32(0x007ffc00)); // base fraction mask
	const auto mask_sf = _mm_set1_epi32(0x000003ff); // step fraction mask
	const auto mask_yf = _mm_set1_epi32(0x0007ffff); // Y fraction mask (bits 13..31)
	const auto base = _mm_or_ps(_mm_and_ps(spu.gpr[op.rb].vf, mask_bf), _mm_castsi128_ps(_mm_set1_epi32(0x3f800000)));
	const auto step = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(spu.gpr[op.rb].vi, mask_sf)), _mm_set1_ps(std::exp2(-13.f)));
	const auto y = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(spu.gpr[op.ra].vi, mask_yf)), _mm_set1_ps(std::exp2(-19.f)));
	spu.gpr[op.rt].vf = _mm_or_ps(_mm_and_ps(mask_se, spu.gpr[op.rb].vf), _mm_andnot_ps(mask_se, _mm_sub_ps(base, _mm_mul_ps(step, y))));
}

void spu_interpreter::HEQ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] == spu.gpr[op.rb]._s32[3])
	{
		spu.halt();
	}
}


void spu_interpreter_fast::CFLTS(SPUThread& spu, spu_opcode_t op)
{
	const auto scaled = _mm_mul_ps(spu.gpr[op.ra].vf, g_spu_imm.scale[173 - op.i8]);
	spu.gpr[op.rt].vi = _mm_xor_si128(_mm_cvttps_epi32(scaled), _mm_castps_si128(_mm_cmpge_ps(scaled, _mm_set1_ps(0x80000000))));
}

void spu_interpreter_fast::CFLTU(SPUThread& spu, spu_opcode_t op)
{
	const auto scaled1 = _mm_max_ps(_mm_mul_ps(spu.gpr[op.ra].vf, g_spu_imm.scale[173 - op.i8]), _mm_set1_ps(0.0f));
	const auto scaled2 = _mm_and_ps(_mm_sub_ps(scaled1, _mm_set1_ps(0x80000000)), _mm_cmpge_ps(scaled1, _mm_set1_ps(0x80000000)));
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_or_si128(_mm_cvttps_epi32(scaled1), _mm_cvttps_epi32(scaled2)), _mm_castps_si128(_mm_cmpge_ps(scaled1, _mm_set1_ps(0x100000000))));
}

void spu_interpreter_fast::CSFLT(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vf = _mm_mul_ps(_mm_cvtepi32_ps(spu.gpr[op.ra].vi), g_spu_imm.scale[op.i8 - 155]);
}

void spu_interpreter_fast::CUFLT(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const auto fix = _mm_and_ps(_mm_castsi128_ps(_mm_srai_epi32(a, 31)), _mm_set1_ps(0x80000000));
	spu.gpr[op.rt].vf = _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(a, _mm_set1_epi32(0x7fffffff))), fix), g_spu_imm.scale[op.i8 - 155]);
}


void spu_interpreter::BRZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] == 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16) - 4;
	}
}

void spu_interpreter::STQA(SPUThread& spu, spu_opcode_t op)
{
	spu._ref<v128>(spu_ls_target(0, op.i16)) = spu.gpr[op.rt];
}

void spu_interpreter::BRNZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] != 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16) - 4;
	}
}

void spu_interpreter::BRHZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] == 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16) - 4;
	}
}

void spu_interpreter::BRHNZ(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] != 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16) - 4;
	}
}

void spu_interpreter::STQR(SPUThread& spu, spu_opcode_t op)
{
	spu._ref<v128>(spu_ls_target(spu.pc, op.i16)) = spu.gpr[op.rt];
}

void spu_interpreter::BRA(SPUThread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(0, op.i16) - 4;
}

void spu_interpreter::LQA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>(spu_ls_target(0, op.i16));
}

void spu_interpreter::BRASL(SPUThread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target - 4;
}

void spu_interpreter::BR(SPUThread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(spu.pc, op.i16) - 4;
}

void spu_interpreter::FSMBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = g_spu_imm.fsmb[op.i16];
}

void spu_interpreter::BRSL(SPUThread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(spu.pc, op.i16);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target - 4;
}

void spu_interpreter::LQR(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>(spu_ls_target(spu.pc, op.i16));
}

void spu_interpreter::IL(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_set1_epi32(op.si16);
}

void spu_interpreter::ILHU(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_set1_epi32(op.i16 << 16);
}

void spu_interpreter::ILH(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_set1_epi16(op.i16);
}

void spu_interpreter::IOHL(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_or_si128(spu.gpr[op.rt].vi, _mm_set1_epi32(op.i16));
}


void spu_interpreter::ORI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_or_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10));
}

void spu_interpreter::ORHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_or_si128(spu.gpr[op.ra].vi, _mm_set1_epi16(op.si10));
}

void spu_interpreter::ORBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_or_si128(spu.gpr[op.ra].vi, _mm_set1_epi8(op.i8));
}

void spu_interpreter::SFI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_sub_epi32(_mm_set1_epi32(op.si10), spu.gpr[op.ra].vi);
}

void spu_interpreter::SFHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_sub_epi16(_mm_set1_epi16(op.si10), spu.gpr[op.ra].vi);
}

void spu_interpreter::ANDI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_and_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10));
}

void spu_interpreter::ANDHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_and_si128(spu.gpr[op.ra].vi, _mm_set1_epi16(op.si10));
}

void spu_interpreter::ANDBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_and_si128(spu.gpr[op.ra].vi, _mm_set1_epi8(op.i8));
}

void spu_interpreter::AI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_add_epi32(_mm_set1_epi32(op.si10), spu.gpr[op.ra].vi);
}

void spu_interpreter::AHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_add_epi16(_mm_set1_epi16(op.si10), spu.gpr[op.ra].vi);
}

void spu_interpreter::STQD(SPUThread& spu, spu_opcode_t op)
{
	spu._ref<v128>((spu.gpr[op.ra]._s32[3] + (op.si10 << 4)) & 0x3fff0) = spu.gpr[op.rt];
}

void spu_interpreter::LQD(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>((spu.gpr[op.ra]._s32[3] + (op.si10 << 4)) & 0x3fff0);
}

void spu_interpreter::XORI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10));
}

void spu_interpreter::XORHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi16(op.si10));
}

void spu_interpreter::XORBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi8(op.i8));
}

void spu_interpreter::CGTI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi32(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10));
}

void spu_interpreter::CGTHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi16(spu.gpr[op.ra].vi, _mm_set1_epi16(op.si10));
}

void spu_interpreter::CGTBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi8(spu.gpr[op.ra].vi, _mm_set1_epi8(op.i8));
}

void spu_interpreter::HGTI(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] > op.si10)
	{
		spu.halt();
	}
}

void spu_interpreter::CLGTI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi32(_mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(0x80000000)), _mm_set1_epi32(op.si10 ^ 0x80000000));
}

void spu_interpreter::CLGTHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi16(_mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(0x80008000)), _mm_set1_epi16(op.si10 ^ 0x8000));
}

void spu_interpreter::CLGTBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpgt_epi8(_mm_xor_si128(spu.gpr[op.ra].vi, _mm_set1_epi32(0x80808080)), _mm_set1_epi8(op.i8 ^ 0x80));
}

void spu_interpreter::HLGTI(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._u32[3] > static_cast<u32>(op.si10))
	{
		spu.halt();
	}
}

void spu_interpreter::MPYI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_madd_epi16(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10 & 0xffff));
}

void spu_interpreter::MPYUI(SPUThread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra].vi;
	const auto i = _mm_set1_epi32(op.si10 & 0xffff);
	spu.gpr[op.rt].vi = _mm_or_si128(_mm_slli_epi32(_mm_mulhi_epu16(a, i), 16), _mm_mullo_epi16(a, i));
}

void spu_interpreter::CEQI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi32(spu.gpr[op.ra].vi, _mm_set1_epi32(op.si10));
}

void spu_interpreter::CEQHI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi16(spu.gpr[op.ra].vi, _mm_set1_epi16(op.si10));
}

void spu_interpreter::CEQBI(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_cmpeq_epi8(spu.gpr[op.ra].vi, _mm_set1_epi8(op.i8));
}

void spu_interpreter::HEQI(SPUThread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] == op.si10)
	{
		spu.halt();
	}
}


void spu_interpreter::HBRA(SPUThread& spu, spu_opcode_t op)
{
}

void spu_interpreter::HBRR(SPUThread& spu, spu_opcode_t op)
{
}

void spu_interpreter::ILA(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].vi = _mm_set1_epi32(op.i18);
}


void spu_interpreter::SELB(SPUThread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt4] = (spu.gpr[op.rc] & spu.gpr[op.rb]) | v128::andnot(spu.gpr[op.rc], spu.gpr[op.ra]);
}

void spu_interpreter::SHUFB(SPUThread& spu, spu_opcode_t op)
{
	const auto index = _mm_xor_si128(spu.gpr[op.rc].vi, _mm_set1_epi32(0x0f0f0f0f));
	const auto res1 = _mm_shuffle_epi8(spu.gpr[op.ra].vi, index);
	const auto bit4 = _mm_set1_epi32(0x10101010);
	const auto k1 = _mm_cmpeq_epi8(_mm_and_si128(index, bit4), bit4);
	const auto res2 = _mm_or_si128(_mm_and_si128(k1, _mm_shuffle_epi8(spu.gpr[op.rb].vi, index)), _mm_andnot_si128(k1, res1));
	const auto bit67 = _mm_set1_epi32(0xc0c0c0c0);
	const auto k2 = _mm_cmpeq_epi8(_mm_and_si128(index, bit67), bit67);
	const auto res3 = _mm_or_si128(res2, k2);
	const auto bit567 = _mm_set1_epi32(0xe0e0e0e0);
	const auto k3 = _mm_cmpeq_epi8(_mm_and_si128(index, bit567), bit567);
	spu.gpr[op.rt4].vi = _mm_sub_epi8(res3, _mm_and_si128(k3, _mm_set1_epi32(0x7f7f7f7f)));
}

void spu_interpreter::MPYA(SPUThread& spu, spu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0xffff);
	spu.gpr[op.rt4].vi = _mm_add_epi32(spu.gpr[op.rc].vi, _mm_madd_epi16(_mm_and_si128(spu.gpr[op.ra].vi, mask), _mm_and_si128(spu.gpr[op.rb].vi, mask)));
}

void spu_interpreter_fast::FNMS(SPUThread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps((f32&)test_bits);

	auto test_a = _mm_and_ps(spu.gpr[op.ra].vf, mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb].vf, mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra].vf, mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb].vf, mask_b);

	spu.gpr[op.rt4].vf = _mm_sub_ps(spu.gpr[op.rc].vf, _mm_mul_ps(a, b));
}

void spu_interpreter_fast::FMA(SPUThread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps((f32&)test_bits);

	auto test_a = _mm_and_ps(spu.gpr[op.ra].vf, mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb].vf, mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra].vf, mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb].vf, mask_b);

	spu.gpr[op.rt4].vf = _mm_add_ps(_mm_mul_ps(a, b), spu.gpr[op.rc].vf);
}

void spu_interpreter_fast::FMS(SPUThread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps((f32&)test_bits);

	auto test_a = _mm_and_ps(spu.gpr[op.ra].vf, mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb].vf, mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra].vf, mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb].vf, mask_b);

	spu.gpr[op.rt4].vf = _mm_sub_ps(_mm_mul_ps(a, b), spu.gpr[op.rc].vf);
}

static void SetHostRoundingMode(u32 rn)
{
	switch (rn)
	{
	case FPSCR_RN_NEAR:
		fesetround(FE_TONEAREST);
		break;
	case FPSCR_RN_ZERO:
		fesetround(FE_TOWARDZERO);
		break;
	case FPSCR_RN_PINF:
		fesetround(FE_UPWARD);
		break;
	case FPSCR_RN_MINF:
		fesetround(FE_DOWNWARD);
		break;
	}
}

// Floating-point utility constants and functions
static const u32 FLOAT_MAX_NORMAL_I = 0x7F7FFFFF;
static const float& FLOAT_MAX_NORMAL = (float&)FLOAT_MAX_NORMAL_I;
static const u32 FLOAT_NAN_I = 0x7FC00000;
static const float& FLOAT_NAN = (float&)FLOAT_NAN_I;
static const u64 DOUBLE_NAN_I = 0x7FF8000000000000ULL;
static const double DOUBLE_NAN = (double&)DOUBLE_NAN_I;

inline bool issnan(double x)
{
	return std::isnan(x) && ((s64&)x) << 12 > 0;
}

inline bool issnan(float x)
{
	return std::isnan(x) && ((s32&)x) << 9 > 0;
}

inline bool isextended(float x)
{
	return fexpf(x) == 255;
}

inline float extended(bool sign, u32 mantissa) // returns -1^sign * 2^127 * (1.mantissa)
{
	u32 bits = sign << 31 | 0x7F800000 | mantissa;
	return (float&)bits;
}

inline float ldexpf_extended(float x, int exp)  // ldexpf() for extended values, assumes result is in range
{
	u32 bits = (u32&)x;
	if (bits << 1 != 0) bits += exp * 0x00800000;
	return (float&)bits;
}

inline bool isdenormal(float x)
{
	return std::fpclassify(x) == FP_SUBNORMAL;
}

inline bool isdenormal(double x)
{
	return std::fpclassify(x) == FP_SUBNORMAL;
}

void spu_interpreter_precise::FREST(SPUThread& spu, spu_opcode_t op)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	for (int i = 0; i < 4; i++)
	{
		const float a = spu.gpr[op.ra]._f[i];
		float result;
		if (fexpf(a) == 0)
		{
			spu.fpscr.setDivideByZeroFlag(i);
			result = extended(std::signbit(a), 0x7FFFFF);
		}
		else if (isextended(a))
			result = 0.0f;
		else
			result = 1 / a;
		spu.gpr[op.rt]._f[i] = result;
	}
}

void spu_interpreter_precise::FRSQEST(SPUThread& spu, spu_opcode_t op)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	for (int i = 0; i < 4; i++)
	{
		const float a = spu.gpr[op.ra]._f[i];
		float result;
		if (fexpf(a) == 0)
		{
			spu.fpscr.setDivideByZeroFlag(i);
			result = extended(0, 0x7FFFFF);
		}
		else if (isextended(a))
			result = 0.5f / std::sqrt(std::fabs(ldexpf_extended(a, -2)));
		else
			result = 1 / std::sqrt(std::fabs(a));
		spu.gpr[op.rt]._f[i] = result;
	}
}

void spu_interpreter_precise::FCGT(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		const u32 b = spu.gpr[op.rb]._u32[i];
		const u32 abs_a = a & 0x7FFFFFFF;
		const u32 abs_b = b & 0x7FFFFFFF;
		const bool a_zero = (abs_a < 0x00800000);
		const bool b_zero = (abs_b < 0x00800000);
		bool pass;
		if (a_zero)
			pass = b >= 0x80800000;
		else if (b_zero)
			pass = (s32)a >= 0x00800000;
		else if (a >= 0x80000000)
			pass = (b >= 0x80000000 && a < b);
		else
			pass = (b >= 0x80000000 || a > b);
		spu.gpr[op.rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
	}
}

static void FA_FS(SPUThread& spu, spu_opcode_t op, bool sub)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	for (int w = 0; w < 4; w++)
	{
		const float a = spu.gpr[op.ra]._f[w];
		const float b = sub ? -spu.gpr[op.rb]._f[w] : spu.gpr[op.rb]._f[w];
		float result;
		if (isdenormal(a))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			if (b == 0.0f || isdenormal(b))
				result = +0.0f;
			else
				result = b;
		}
		else if (isdenormal(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			if (a == 0.0f)
				result = +0.0f;
			else
				result = a;
		}
		else if (isextended(a) || isextended(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			if (isextended(a) && fexpf(b) < 255 - 32)
			{
				if (std::signbit(a) != std::signbit(b))
				{
					const u32 bits = (u32&)a - 1;
					result = (float&)bits;
				}
				else

					result = a;
			}
			else if (isextended(b) && fexpf(a) < 255 - 32)
			{
				if (std::signbit(a) != std::signbit(b))
				{
					const u32 bits = (u32&)b - 1;
					result = (float&)bits;
				}
				else
					result = b;
			}
			else
			{
				feclearexcept(FE_ALL_EXCEPT);
				result = ldexpf_extended(a, -1) + ldexpf_extended(b, -1);
				result = ldexpf_extended(result, 1);
				if (fetestexcept(FE_OVERFLOW))
				{
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(std::signbit(result), 0x7FFFFF);
				}
			}
		}
		else
		{
			result = a + b;
			if (result == std::copysign(FLOAT_MAX_NORMAL, result))
			{
				result = ldexpf_extended(std::ldexp(a, -1) + std::ldexp(b, -1), 1);
				if (isextended(result))
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			}
			else if (isdenormal(result))
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				result = +0.0f;
			}
			else if (result == 0.0f)
			{
				if (std::fabs(a) != std::fabs(b))
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				result = +0.0f;
			}
		}
		spu.gpr[op.rt]._f[w] = result;
	}
}

void spu_interpreter_precise::FA(SPUThread& spu, spu_opcode_t op) { FA_FS(spu, op, false); }

void spu_interpreter_precise::FS(SPUThread& spu, spu_opcode_t op) { FA_FS(spu, op, true); }

void spu_interpreter_precise::FM(SPUThread& spu, spu_opcode_t op)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	for (int w = 0; w < 4; w++)
	{
		const float a = spu.gpr[op.ra]._f[w];
		const float b = spu.gpr[op.rb]._f[w];
		float result;
		if (isdenormal(a) || isdenormal(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			result = +0.0f;
		}
		else if (isextended(a) || isextended(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			const bool sign = std::signbit(a) ^ std::signbit(b);
			if (a == 0.0f || b == 0.0f)
			{
				result = +0.0f;
			}
			else if ((fexpf(a) - 127) + (fexpf(b) - 127) >= 129)
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
				result = extended(sign, 0x7FFFFF);
			}
			else
			{
				if (isextended(a))
					result = ldexpf_extended(a, -1) * b;
				else
					result = a * ldexpf_extended(b, -1);
				if (result == std::copysign(FLOAT_MAX_NORMAL, result))
				{
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
					result = ldexpf_extended(result, 1);
			}
		}
		else
		{
			result = a * b;
			if (result == std::copysign(FLOAT_MAX_NORMAL, result))
			{
				feclearexcept(FE_ALL_EXCEPT);
				if (fexpf(a) > fexpf(b))
					result = std::ldexp(a, -1) * b;
				else
					result = a * std::ldexp(b, -1);
				result = ldexpf_extended(result, 1);
				if (isextended(result))
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (fetestexcept(FE_OVERFLOW))
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
			}
			else if (isdenormal(result))
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				result = +0.0f;
			}
			else if (result == 0.0f)
			{
				if (a != 0.0f && b != 0.0f)
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
				result = +0.0f;
			}
		}
		spu.gpr[op.rt]._f[w] = result;
	}
}

void spu_interpreter_precise::FCMGT(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		const u32 b = spu.gpr[op.rb]._u32[i];
		const u32 abs_a = a & 0x7FFFFFFF;
		const u32 abs_b = b & 0x7FFFFFFF;
		const bool a_zero = (abs_a < 0x00800000);
		const bool b_zero = (abs_b < 0x00800000);
		bool pass;
		if (a_zero)
			pass = false;
		else if (b_zero)
			pass = !a_zero;
		else
			pass = abs_a > abs_b;
		spu.gpr[op.rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
	}
}

enum DoubleOp
{
	DFASM_A,
	DFASM_S,
	DFASM_M,
};

static void DFASM(SPUThread& spu, spu_opcode_t op, DoubleOp operation)
{
	for (int i = 0; i < 2; i++)
	{
		double a = spu.gpr[op.ra]._d[i];
		double b = spu.gpr[op.rb]._d[i];
		if (isdenormal(a))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			a = std::copysign(0.0, a);
		}
		if (isdenormal(b))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			b = std::copysign(0.0, b);
		}
		double result;
		if (std::isnan(a) || std::isnan(b))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
			if (issnan(a) || issnan(b))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
			result = DOUBLE_NAN;
		}
		else
		{
			SetHostRoundingMode(spu.fpscr.checkSliceRounding(i));
			feclearexcept(FE_ALL_EXCEPT);
			switch (operation)
			{
			case DFASM_A: result = a + b; break;
			case DFASM_S: result = a - b; break;
			case DFASM_M: result = a * b; break;
			}
			if (fetestexcept(FE_INVALID))
			{
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				if (fetestexcept(FE_OVERFLOW))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
				if (fetestexcept(FE_UNDERFLOW))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
				if (fetestexcept(FE_INEXACT))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
			}
		}
		spu.gpr[op.rt]._d[i] = result;
	}
}

void spu_interpreter_precise::DFA(SPUThread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_A); }

void spu_interpreter_precise::DFS(SPUThread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_S); }

void spu_interpreter_precise::DFM(SPUThread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_M); }

static void DFMA(SPUThread& spu, spu_opcode_t op, bool neg, bool sub)
{
	for (int i = 0; i < 2; i++)
	{
		double a = spu.gpr[op.ra]._d[i];
		double b = spu.gpr[op.rb]._d[i];
		double c = spu.gpr[op.rt]._d[i];
		if (isdenormal(a))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			a = std::copysign(0.0, a);
		}
		if (isdenormal(b))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			b = std::copysign(0.0, b);
		}
		if (isdenormal(c))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			c = std::copysign(0.0, c);
		}
		double result;
		if (std::isnan(a) || std::isnan(b) || std::isnan(c))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
			if (issnan(a) || issnan(b) || issnan(c) || (std::isinf(a) && b == 0.0f) || (a == 0.0f && std::isinf(b)))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
			result = DOUBLE_NAN;
		}
		else
		{
			SetHostRoundingMode(spu.fpscr.checkSliceRounding(i));
			feclearexcept(FE_ALL_EXCEPT);
			result = fma(a, b, sub ? -c : c);
			if (fetestexcept(FE_INVALID))
			{
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				if (fetestexcept(FE_OVERFLOW))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
				if (fetestexcept(FE_UNDERFLOW))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
				if (fetestexcept(FE_INEXACT))
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
				if (neg) result = -result;
			}
		}
		spu.gpr[op.rt]._d[i] = result;
	}
}

void spu_interpreter_precise::DFMA(SPUThread& spu, spu_opcode_t op) { ::DFMA(spu, op, false, false); }

void spu_interpreter_precise::DFMS(SPUThread& spu, spu_opcode_t op) { ::DFMA(spu, op, false, true); }

void spu_interpreter_precise::DFNMS(SPUThread& spu, spu_opcode_t op) { ::DFMA(spu, op, true, true); }

void spu_interpreter_precise::DFNMA(SPUThread& spu, spu_opcode_t op) { ::DFMA(spu, op, true, false); }

void spu_interpreter_precise::FSCRRD(SPUThread& spu, spu_opcode_t op)
{
	spu.fpscr.Read(spu.gpr[op.rt]);
}

void spu_interpreter_precise::FESD(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 2; i++)
	{
		const float a = spu.gpr[op.ra]._f[i * 2 + 1];
		if (std::isnan(a))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
			if (issnan(a))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
			spu.gpr[op.rt]._d[i] = DOUBLE_NAN;
		}
		else if (isdenormal(a))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DDENORM);
			spu.gpr[op.rt]._d[i] = 0.0;
		}
		else
		{
			spu.gpr[op.rt]._d[i] = (double)a;
		}
	}
}

void spu_interpreter_precise::FRDS(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 2; i++)
	{
		SetHostRoundingMode(spu.fpscr.checkSliceRounding(i));
		const double a = spu.gpr[op.ra]._d[i];
		if (std::isnan(a))
		{
			spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DNAN);
			if (issnan(a))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
			spu.gpr[op.rt]._f[i * 2 + 1] = FLOAT_NAN;
		}
		else
		{
			feclearexcept(FE_ALL_EXCEPT);
			spu.gpr[op.rt]._f[i * 2 + 1] = (float)a;
			if (fetestexcept(FE_OVERFLOW))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
			if (fetestexcept(FE_UNDERFLOW))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
			if (fetestexcept(FE_INEXACT))
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
		}
		spu.gpr[op.rt]._u32[i * 2] = 0;
	}
}

void spu_interpreter_precise::FSCRWR(SPUThread& spu, spu_opcode_t op)
{
	spu.fpscr.Write(spu.gpr[op.ra]);
}

void spu_interpreter_precise::FCEQ(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		const u32 b = spu.gpr[op.rb]._u32[i];
		const u32 abs_a = a & 0x7FFFFFFF;
		const u32 abs_b = b & 0x7FFFFFFF;
		const bool a_zero = (abs_a < 0x00800000);
		const bool b_zero = (abs_b < 0x00800000);
		const bool pass = a == b || (a_zero && b_zero);
		spu.gpr[op.rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
	}
}

void spu_interpreter_precise::FCMEQ(SPUThread& spu, spu_opcode_t op)
{
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		const u32 b = spu.gpr[op.rb]._u32[i];
		const u32 abs_a = a & 0x7FFFFFFF;
		const u32 abs_b = b & 0x7FFFFFFF;
		const bool a_zero = (abs_a < 0x00800000);
		const bool b_zero = (abs_b < 0x00800000);
		const bool pass = abs_a == abs_b || (a_zero && b_zero);
		spu.gpr[op.rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
	}
}

void spu_interpreter_precise::FI(SPUThread& spu, spu_opcode_t op)
{
	// TODO
	spu.gpr[op.rt] = spu.gpr[op.rb];
}

void spu_interpreter_precise::CFLTS(SPUThread& spu, spu_opcode_t op)
{
	const int scale = 173 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const float a = spu.gpr[op.ra]._f[i];
		float scaled;
		if ((fexpf(a) - 127) + scale >= 32)
			scaled = std::copysign(4294967296.0f, a);
		else
			scaled = std::ldexp(a, scale);
		s32 result;
		if (scaled >= 2147483648.0f)
			result = 0x7FFFFFFF;
		else if (scaled < -2147483648.0f)
			result = 0x80000000;
		else
			result = (s32)scaled;
		spu.gpr[op.rt]._s32[i] = result;
	}
}

void spu_interpreter_precise::CFLTU(SPUThread& spu, spu_opcode_t op)
{
	const int scale = 173 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const float a = spu.gpr[op.ra]._f[i];
		float scaled;
		if ((fexpf(a) - 127) + scale >= 32)
			scaled = std::copysign(4294967296.0f, a);
		else
			scaled = std::ldexp(a, scale);
		u32 result;
		if (scaled >= 4294967296.0f)
			result = 0xFFFFFFFF;
		else if (scaled < 0.0f)
			result = 0;
		else
			result = (u32)scaled;
		spu.gpr[op.rt]._u32[i] = result;
	}
}

void spu_interpreter_precise::CSFLT(SPUThread& spu, spu_opcode_t op)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	const int scale = 155 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const s32 a = spu.gpr[op.ra]._s32[i];
		spu.gpr[op.rt]._f[i] = (float)a;

		u32 exp = ((spu.gpr[op.rt]._u32[i] >> 23) & 0xff) - scale;

		if (exp > 255) //< 0
			exp = 0;

		spu.gpr[op.rt]._u32[i] = (spu.gpr[op.rt]._u32[i] & 0x807fffff) | (exp << 23);
		if (isdenormal(spu.gpr[op.rt]._f[i]) || (spu.gpr[op.rt]._f[i] == 0.0f && a != 0))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(i, FPSCR_SUNF | FPSCR_SDIFF);
			spu.gpr[op.rt]._f[i] = 0.0f;
		}
	}
}

void spu_interpreter_precise::CUFLT(SPUThread& spu, spu_opcode_t op)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	const int scale = 155 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		spu.gpr[op.rt]._f[i] = (float)a;

		u32 exp = ((spu.gpr[op.rt]._u32[i] >> 23) & 0xff) - scale;

		if (exp > 255) //< 0
			exp = 0;

		spu.gpr[op.rt]._u32[i] = (spu.gpr[op.rt]._u32[i] & 0x807fffff) | (exp << 23);
		if (isdenormal(spu.gpr[op.rt]._f[i]) || (spu.gpr[op.rt]._f[i] == 0.0f && a != 0))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(i, FPSCR_SUNF | FPSCR_SDIFF);
			spu.gpr[op.rt]._f[i] = 0.0f;
		}
	}
}

static void FMA(SPUThread& spu, spu_opcode_t op, bool neg, bool sub)
{
	SetHostRoundingMode(FPSCR_RN_ZERO);
	for (int w = 0; w < 4; w++)
	{
		float a = spu.gpr[op.ra]._f[w];
		float b = neg ? -spu.gpr[op.rb]._f[w] : spu.gpr[op.rb]._f[w];
		float c = (neg != sub) ? -spu.gpr[op.rc]._f[w] : spu.gpr[op.rc]._f[w];
		if (isdenormal(a))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			a = 0.0f;
		}
		if (isdenormal(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			b = 0.0f;
		}
		if (isdenormal(c))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			c = 0.0f;
		}

		const bool sign = std::signbit(a) ^ std::signbit(b);
		float result;

		if (isextended(a) || isextended(b))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			if (a == 0.0f || b == 0.0f)
			{
				result = c;
			}
			else if ((fexpf(a) - 127) + (fexpf(b) - 127) >= 130)
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
				result = extended(sign, 0x7FFFFF);
			}
			else
			{
				float new_a, new_b;
				if (isextended(a))
				{
					new_a = ldexpf_extended(a, -2);
					new_b = b;
				}
				else
				{
					new_a = a;
					new_b = ldexpf_extended(b, -2);
				}
				if (fexpf(c) < 3)
				{
					result = new_a * new_b;
					if (c != 0.0f && std::signbit(c) != sign)
					{
						u32 bits = (u32&)result - 1;
						result = (float&)bits;
					}
				}
				else
				{
					result = std::fma(new_a, new_b, ldexpf_extended(c, -2));
				}
				if (std::fabs(result) >= std::ldexp(1.0f, 127))
				{
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
				{
					result = ldexpf_extended(result, 2);
				}
			}
		}
		else if (isextended(c))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
			if (a == 0.0f || b == 0.0f)
			{
				result = c;
			}
			else if ((fexpf(a) - 127) + (fexpf(b) - 127) < 96)
			{
				result = c;
				if (sign != std::signbit(c))
				{
					u32 bits = (u32&)result - 1;
					result = (float&)bits;
				}
			}
			else
			{
				result = std::fma(std::ldexp(a, -1), std::ldexp(b, -1), ldexpf_extended(c, -2));
				if (std::fabs(result) >= std::ldexp(1.0f, 127))
				{
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
				{
					result = ldexpf_extended(result, 2);
				}
			}
		}
		else
		{
			feclearexcept(FE_ALL_EXCEPT);
			result = std::fma(a, b, c);
			if (fetestexcept(FE_OVERFLOW))
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SDIFF);
				if (fexpf(a) > fexpf(b))
					result = std::fma(std::ldexp(a, -2), b, std::ldexp(c, -2));
				else
					result = std::fma(a, std::ldexp(b, -2), std::ldexp(c, -2));
				if (fabsf(result) >= std::ldexp(1.0f, 127))
				{
					spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SOVF);
					result = extended(sign, 0x7FFFFF);
				}
				else
				{
					result = ldexpf_extended(result, 2);
				}
			}
			else if (fetestexcept(FE_UNDERFLOW))
			{
				spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
			}
		}
		if (isdenormal(result))
		{
			spu.fpscr.setSinglePrecisionExceptionFlags(w, FPSCR_SUNF | FPSCR_SDIFF);
			result = 0.0f;
		}
		else if (result == 0.0f)
		{
			result = +0.0f;
		}
		spu.gpr[op.rt4]._f[w] = result;
	}
}

void spu_interpreter_precise::FNMS(SPUThread& spu, spu_opcode_t op) { ::FMA(spu, op, true, true); }

void spu_interpreter_precise::FMA(SPUThread& spu, spu_opcode_t op) { ::FMA(spu, op, false, false); }

void spu_interpreter_precise::FMS(SPUThread& spu, spu_opcode_t op) { ::FMA(spu, op, false, true); }
