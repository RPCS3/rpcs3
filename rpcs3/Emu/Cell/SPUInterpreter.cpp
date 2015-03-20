#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "SPUThread.h"
#include "SPUInstrTable.h"
#include "SPUInterpreter.h"
#include "SPUInterpreter2.h"

#ifdef _MSC_VER
#include <intrin.h>
#define rotl32 _rotl
#define rotl16 _rotl16
#else
#include <x86intrin.h>
#define rotl16(x,r) (((u16)(x) << (r)) | ((u16)(x) >> (16 - (r))))
#define rotl32(x,r) (((u32)(x) << (r)) | ((u32)(x) >> (32 - (r))))
#endif

void spu_interpreter::DEFAULT(SPUThread& CPU, spu_opcode_t op)
{
	SPUInterpreter inter(CPU); (*SPU_instr::rrr_list)(&inter, op.opcode);
}


void spu_interpreter::STOP(SPUThread& CPU, spu_opcode_t op)
{
	CPU.stop_and_signal(op.opcode & 0x3fff);
}

void spu_interpreter::LNOP(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::SYNC(SPUThread& CPU, spu_opcode_t op)
{
	_mm_mfence();
}

void spu_interpreter::DSYNC(SPUThread& CPU, spu_opcode_t op)
{
	_mm_mfence();
}

void spu_interpreter::MFSPR(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].clear();
}

void spu_interpreter::RDCH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(CPU.get_ch_value(op.ra));
}

void spu_interpreter::RCHCNT(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(CPU.get_ch_count(op.ra));
}

void spu_interpreter::SF(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::sub32(CPU.GPR[op.rb], CPU.GPR[op.ra]);
}

void spu_interpreter::OR(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = CPU.GPR[op.ra] | CPU.GPR[op.rb];
}

void spu_interpreter::BG(SPUThread& CPU, spu_opcode_t op)
{
	for (u32 i = 0; i < 4; i++)
	{
		CPU.GPR[op.rt]._u32[i] = CPU.GPR[op.ra]._u32[i] <= CPU.GPR[op.rb]._u32[i];
	}
}

void spu_interpreter::SFH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::sub16(CPU.GPR[op.rb], CPU.GPR[op.ra]);
}

void spu_interpreter::NOR(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = ~(CPU.GPR[op.ra] | CPU.GPR[op.rb]);
}

void spu_interpreter::ABSDB(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	CPU.GPR[op.rt] = u128::sub8(u128::maxu8(a, b), u128::minu8(a, b));
}

void spu_interpreter::ROT(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		CPU.GPR[op.rt]._u32[i] = rotl32(a._u32[i], b._s32[i]);
	}
}

void spu_interpreter::ROTM(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		CPU.GPR[op.rt]._u32[i] = static_cast<u32>(value >> (0 - b._u32[i]));
	}
}

void spu_interpreter::ROTMA(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const s64 value = a._s32[i];
		CPU.GPR[op.rt]._s32[i] = static_cast<s32>(value >> (0 - b._u32[i]));
	}
}

void spu_interpreter::SHL(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		CPU.GPR[op.rt]._u32[i] = static_cast<u32>(value << b._u32[i]);
	}
}

void spu_interpreter::ROTH(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		CPU.GPR[op.rt]._u16[i] = rotl16(a._u16[i], b._u8[i * 2]);
	}
}

void spu_interpreter::ROTHM(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		CPU.GPR[op.rt]._u16[i] = static_cast<u16>(value >> (0 - b._u16[i]));
	}
}

void spu_interpreter::ROTMAH(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const s32 value = a._s16[i];
		CPU.GPR[op.rt]._s16[i] = static_cast<s16>(value >> (0 - b._u16[i]));
	}
}

void spu_interpreter::SHLH(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra];
	const auto b = CPU.GPR[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		CPU.GPR[op.rt]._u16[i] = static_cast<u16>(value << b._u16[i]);
	}
}

void spu_interpreter::ROTI(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra].vi;
	const s32 n = op.si7 & 0x1f;

	CPU.GPR[op.rt].vi = _mm_or_si128(_mm_slli_epi32(a, n), _mm_srli_epi32(a, 32 - n));
}

void spu_interpreter::ROTMI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_srli_epi32(CPU.GPR[op.ra].vi, -op.si7 & 0x3f);
}

void spu_interpreter::ROTMAI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_srai_epi32(CPU.GPR[op.ra].vi, -op.si7 & 0x3f);
}

void spu_interpreter::SHLI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_slli_epi32(CPU.GPR[op.ra].vi, op.si7 & 0x3f);
}

void spu_interpreter::ROTHI(SPUThread& CPU, spu_opcode_t op)
{
	const auto a = CPU.GPR[op.ra].vi;
	const s32 n = op.si7 & 0xf;

	CPU.GPR[op.rt].vi = _mm_or_si128(_mm_slli_epi16(a, n), _mm_srli_epi16(a, 16 - n));
}

void spu_interpreter::ROTHMI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_srli_epi16(CPU.GPR[op.ra].vi, -op.si7 & 0x1f);
}

void spu_interpreter::ROTMAHI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_srai_epi16(CPU.GPR[op.ra].vi, -op.si7 & 0x1f);
}

void spu_interpreter::SHLHI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_slli_epi16(CPU.GPR[op.ra].vi, op.si7 & 0x1f);
}

void spu_interpreter::A(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::add32(CPU.GPR[op.ra], CPU.GPR[op.rb]);
}

void spu_interpreter::AND(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = CPU.GPR[op.ra] & CPU.GPR[op.rb];
}

void spu_interpreter::CG(SPUThread& CPU, spu_opcode_t op)
{
	for (u32 i = 0; i < 4; i++)
	{
		CPU.GPR[op.rt]._u32[i] = ~CPU.GPR[op.ra]._u32[i] < CPU.GPR[op.rb]._u32[i];
	}
}

void spu_interpreter::AH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::add16(CPU.GPR[op.ra], CPU.GPR[op.rb]);
}

void spu_interpreter::NAND(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = ~(CPU.GPR[op.ra] & CPU.GPR[op.rb]);
}

void spu_interpreter::AVGB(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_avg_epu8(CPU.GPR[op.ra].vi, CPU.GPR[op.rb].vi);
}

void spu_interpreter::MTSPR(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::WRCH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.set_ch_value(op.ra, CPU.GPR[op.rt]._u32[3]);
}

void spu_interpreter::BIZ(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	if (CPU.GPR[op.rt]._u32[3] == 0)
	{
		CPU.SetBranch(SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0));
	}
}

void spu_interpreter::BINZ(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	if (CPU.GPR[op.rt]._u32[3] != 0)
	{
		CPU.SetBranch(SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0));
	}
}

void spu_interpreter::BIHZ(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	if (CPU.GPR[op.rt]._u16[6] == 0)
	{
		CPU.SetBranch(SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0));
	}
}

void spu_interpreter::BIHNZ(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	if (CPU.GPR[op.rt]._u16[6] != 0)
	{
		CPU.SetBranch(SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0));
	}
}

void spu_interpreter::STOPD(SPUThread& CPU, spu_opcode_t op)
{
	throw __FUNCTION__;
}

void spu_interpreter::STQX(SPUThread& CPU, spu_opcode_t op)
{
	CPU.write128((CPU.GPR[op.ra]._u32[3] + CPU.GPR[op.rb]._u32[3]) & 0x3fff0, CPU.GPR[op.rt]);
}

void spu_interpreter::BI(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	CPU.SetBranch(SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0));
}

void spu_interpreter::BISL(SPUThread& CPU, spu_opcode_t op)
{
	if (op.d || op.e)
	{
		throw __FUNCTION__;
	}

	const u32 target = SPUOpcodes::branchTarget(CPU.GPR[op.ra]._u32[3], 0);
	CPU.GPR[op.rt] = u128::from32r(CPU.PC + 4);
	CPU.SetBranch(target);
}

void spu_interpreter::IRET(SPUThread& CPU, spu_opcode_t op)
{
	throw __FUNCTION__;
}

void spu_interpreter::BISLED(SPUThread& CPU, spu_opcode_t op)
{
	throw __FUNCTION__;
}

void spu_interpreter::HBR(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::GB(SPUThread& CPU, spu_opcode_t op)
{
	u32 result = 0;
	for (u32 i = 0; i < 4; i++)
	{
		result |= (CPU.GPR[op.ra]._u32[i] & 1) << i;
	}

	CPU.GPR[op.rt] = u128::from32r(result);
}

void spu_interpreter::GBH(SPUThread& CPU, spu_opcode_t op)
{
	u32 result = 0;
	for (u32 i = 0; i < 8; i++)
	{
		result |= (CPU.GPR[op.ra]._u16[i] & 1) << i;
	}
		
	CPU.GPR[op.rt] = u128::from32r(result);
}

void spu_interpreter::GBB(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(_mm_movemask_epi8(_mm_slli_epi64(CPU.GPR[op.ra].vi, 7)));
}

void spu_interpreter::FSM(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = g_imm_table.fsm_table[CPU.GPR[op.ra]._u32[3] & 0xf];
}

void spu_interpreter::FSMH(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = g_imm_table.fsmh_table[CPU.GPR[op.ra]._u32[3] & 0xff];
}

void spu_interpreter::FSMB(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = g_imm_table.fsmb_table[CPU.GPR[op.ra]._u32[3] & 0xffff];
}

void spu_interpreter::FREST(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vf = _mm_rcp_ps(CPU.GPR[op.ra].vf);
}

void spu_interpreter::FRSQEST(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vf = _mm_rsqrt_ps(_mm_and_ps(CPU.GPR[op.ra].vf, _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff))));
}

void spu_interpreter::LQX(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = CPU.read128((CPU.GPR[op.ra]._u32[3] + CPU.GPR[op.rb]._u32[3]) & 0x3fff0);
}

void spu_interpreter::ROTQBYBI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.rldq_pshufb[CPU.GPR[op.rb]._u32[3] >> 3 & 0xf]);
}

void spu_interpreter::ROTQMBYBI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.srdq_pshufb[-(CPU.GPR[op.rb]._s32[3] >> 3) & 0x1f]);
}

void spu_interpreter::SHLQBYBI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.sldq_pshufb[CPU.GPR[op.rb]._u32[3] >> 3 & 0x1f]);
}

void spu_interpreter::CBX(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = ~(CPU.GPR[op.rb]._u32[3] + CPU.GPR[op.ra]._u32[3]) & 0xf;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u8[t] = 0x03;
}

void spu_interpreter::CHX(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(CPU.GPR[op.rb]._u32[3] + CPU.GPR[op.ra]._u32[3]) & 0xe) >> 1;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u16[t] = 0x0203;
}

void spu_interpreter::CWX(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(CPU.GPR[op.rb]._u32[3] + CPU.GPR[op.ra]._u32[3]) & 0xc) >> 2;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u32[t] = 0x00010203;
}

void spu_interpreter::CDX(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(CPU.GPR[op.rb]._u32[3] + CPU.GPR[op.ra]._u32[3]) & 0x8) >> 3;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u64[t] = 0x0001020304050607ull;
}

void spu_interpreter::ROTQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBY(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.rldq_pshufb[CPU.GPR[op.rb]._u32[3] & 0xf]);
}

void spu_interpreter::ROTQMBY(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.srdq_pshufb[-CPU.GPR[op.rb]._s32[3] & 0x1f]);
}

void spu_interpreter::SHLQBY(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.sldq_pshufb[CPU.GPR[op.rb]._u32[3] & 0x1f]);
}

void spu_interpreter::ORX(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt] = u128::from32r(CPU.GPR[op.ra]._u32[0] | CPU.GPR[op.ra]._u32[1] | CPU.GPR[op.ra]._u32[2] | CPU.GPR[op.ra]._u32[3]);
}

void spu_interpreter::CBD(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = ~(op.i7 + CPU.GPR[op.ra]._u32[3]) & 0xf;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u8[t] = 0x03;
}

void spu_interpreter::CHD(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(op.i7 + CPU.GPR[op.ra]._u32[3]) & 0xe) >> 1;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u16[t] = 0x0203;
}

void spu_interpreter::CWD(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(op.i7 + CPU.GPR[op.ra]._u32[3]) & 0xc) >> 2;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u32[t] = 0x00010203;
}

void spu_interpreter::CDD(SPUThread& CPU, spu_opcode_t op)
{
	const s32 t = (~(op.i7 + CPU.GPR[op.ra]._u32[3]) & 0x8) >> 3;
	CPU.GPR[op.rt] = u128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	CPU.GPR[op.rt]._u64[t] = 0x0001020304050607ull;
}

void spu_interpreter::ROTQBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQMBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHLQBII(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ROTQBYI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.rldq_pshufb[op.i7 & 0xf]);
}

void spu_interpreter::ROTQMBYI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.srdq_pshufb[-op.si7 & 0x1f]);
}

void spu_interpreter::SHLQBYI(SPUThread& CPU, spu_opcode_t op)
{
	CPU.GPR[op.rt].vi = _mm_shuffle_epi8(CPU.GPR[op.ra].vi, g_imm_table.sldq_pshufb[op.i7 & 0x1f]);
}

void spu_interpreter::NOP(SPUThread& CPU, spu_opcode_t op)
{
}

void spu_interpreter::CGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XOR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::EQV(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SUMB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSWD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSHW(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CNTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XSBH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDC(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORC(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCMGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCMGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFM(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HLGT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFNMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFNMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ADDX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BGX(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHHAU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSCRRD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FESD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FRDS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSCRWR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFTSV(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPY(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYHH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FCMEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::DFCMEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HEQ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::CFLTS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CFLTU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CSFLT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CUFLT(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::BRZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRNZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRHZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRHNZ(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRASL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FSMBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::BRSL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::IL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILHU(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILH(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::IOHL(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::ORI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ORBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SFHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ANDBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::AHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::STQD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::LQD(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::XORBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CGTBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CLGTBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HLGTI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYUI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQHI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::CEQBI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HEQI(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::HBRA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::HBRR(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::ILA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::SELB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::SHUFB(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::MPYA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FNMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FMA(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}

void spu_interpreter::FMS(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}


void spu_interpreter::UNK(SPUThread& CPU, spu_opcode_t op)
{
	DEFAULT(CPU, op);
}
