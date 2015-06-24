#include "stdafx.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Emu/SysCalls/Modules.h"
#include "Emu/Cell/PPUDecoder.h"
#include "PPUInstrTable.h"
#include "PPUInterpreter.h"
#include "PPUInterpreter2.h"
#include "Emu/CPU/CPUThreadManager.h"

class ppu_scale_table_t
{
	std::array<__m128, 32 + 31> m_data;

public:
	ppu_scale_table_t()
	{
		for (s32 i = -31; i < 32; i++)
		{
			m_data[i + 31] = _mm_set1_ps(static_cast<float>(exp2(i)));
		}
	}

	force_inline __m128 operator [] (s32 scale) const
	{
		return m_data[scale + 31];
	}
}
const g_ppu_scale_table;


void ppu_interpreter::NULL_OP(PPUThread& CPU, ppu_opcode_t op)
{
	PPUInterpreter inter(CPU); (*PPU_instr::main_list)(&inter, op.opcode);
}

void ppu_interpreter::NOP(PPUThread& CPU, ppu_opcode_t op)
{
}


void ppu_interpreter::TDI(PPUThread& CPU, ppu_opcode_t op)
{
	const s64 a = CPU.GPR[op.ra], b = op.simm16;
	const u64 a_ = a, b_ = b; // unsigned

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		throw __FUNCTION__;
	}
}

void ppu_interpreter::TWI(PPUThread& CPU, ppu_opcode_t op)
{
	const s32 a = (s32)CPU.GPR[op.ra], b = op.simm16;
	const u32 a_ = a, b_ = b; // unsigned

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		throw __FUNCTION__;
	}
}


void ppu_interpreter::MFVSCR(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::MTVSCR(PPUThread& CPU, ppu_opcode_t op)
{
	// ignored (MFVSCR disabled)
}

void ppu_interpreter::VADDCUW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	CPU.VPR[op.vd].vi = _mm_srli_epi32(_mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))), 31);
}

void ppu_interpreter::VADDFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::addfs(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VADDSBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_adds_epi8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_adds_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VADDSWS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va];
	const auto b = CPU.VPR[op.vb];
	const auto s = u128::add32(a, b); // a + b
	const auto m = (a ^ s) & (b ^ s); // overflow bit
	const auto x = _mm_srai_epi32(m.vi, 31); // saturation mask
	const auto y = _mm_srai_epi32(_mm_and_si128(s.vi, m.vi), 31); // positive saturation mask
	CPU.VPR[op.vd].vi = _mm_xor_si128(_mm_xor_si128(_mm_srli_epi32(x, 1), y), _mm_or_si128(s.vi, x));
}

void ppu_interpreter::VADDUBM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::add8(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VADDUBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_adds_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VADDUHM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::add16(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VADDUHS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_adds_epu16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VADDUWM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::add32(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VADDUWS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_add_epi32(a, b), _mm_cmpgt_epi32(_mm_xor_si128(b, _mm_set1_epi32(0x80000000)), _mm_xor_si128(a, _mm_set1_epi32(0x7fffffff))));
}

void ppu_interpreter::VAND(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = CPU.VPR[op.va] & CPU.VPR[op.vb];
}

void ppu_interpreter::VANDC(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = CPU.VPR[op.va] & ~CPU.VPR[op.vb];
}

void ppu_interpreter::VAVGSB(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va];
	const auto b = u128::add8(CPU.VPR[op.vb], u128::from8p(1)); // add 1
	const auto summ = u128::add8(a, b) & u128::from8p(0xfe);
	const auto sign = u128::from8p(0x80);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ u128::eq8(b, sign)) & sign; // calculate msb
	CPU.VPR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi64(summ.vi, 1));
}

void ppu_interpreter::VAVGSH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va];
	const auto b = u128::add16(CPU.VPR[op.vb], u128::from16p(1)); // add 1
	const auto summ = u128::add16(a, b);
	const auto sign = u128::from16p(0x8000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ u128::eq16(b, sign)) & sign; // calculate msb
	CPU.VPR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi16(summ.vi, 1));
}

void ppu_interpreter::VAVGSW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va];
	const auto b = u128::add32(CPU.VPR[op.vb], u128::from32p(1)); // add 1
	const auto summ = u128::add32(a, b);
	const auto sign = u128::from32p(0x80000000);
	const auto overflow = (((a ^ summ) & (b ^ summ)) ^ summ ^ u128::eq32(b, sign)) & sign; // calculate msb
	CPU.VPR[op.vd].vi = _mm_or_si128(overflow.vi, _mm_srli_epi32(summ.vi, 1));
}

void ppu_interpreter::VAVGUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_avg_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VAVGUH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_avg_epu16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VAVGUW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va];
	const auto b = CPU.VPR[op.vb];
	const auto summ = u128::add32(u128::add32(a, b), u128::from32p(1));
	const auto carry = _mm_xor_si128(_mm_slli_epi32(sse_cmpgt_epu32(summ.vi, a.vi), 31), _mm_set1_epi32(0x80000000));
	CPU.VPR[op.vd].vi = _mm_or_si128(carry, _mm_srli_epi32(summ.vi, 1));
}

void ppu_interpreter::VCFSX(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_mul_ps(_mm_cvtepi32_ps(CPU.VPR[op.vb].vi), g_ppu_scale_table[0 - op.vuimm]);
}

void ppu_interpreter::VCFUX(PPUThread& CPU, ppu_opcode_t op)
{
	const auto b = CPU.VPR[op.vb].vi;
	const auto fix = _mm_and_ps(_mm_castsi128_ps(_mm_srai_epi32(b, 31)), _mm_set1_ps(0x80000000));
	CPU.VPR[op.vd].vf = _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(b, _mm_set1_epi32(0x7fffffff))), fix), g_ppu_scale_table[0 - op.vuimm]);
}

void ppu_interpreter::VCMPBFP(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vf;
	const auto b = CPU.VPR[op.vb].vf;
	const auto sign = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	const auto bneg = _mm_xor_ps(b, sign);
	CPU.VPR[op.vd].vf = _mm_or_ps(_mm_and_ps(_mm_cmpnle_ps(a, b), sign), _mm_and_ps(_mm_cmpnge_ps(a, bneg), _mm_castsi128_ps(_mm_set1_epi32(0x40000000))));
}

void ppu_interpreter::VCMPBFP_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPBFP(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? 0 : 2; // set 2 if all in bounds
}

void ppu_interpreter::VCMPEQFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_cmpeq_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VCMPEQFP_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPEQFP(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2; // set 2 if none equal, 8 if all equal
}

void ppu_interpreter::VCMPEQUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::eq8(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VCMPEQUB_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPEQUB(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2; // set 2 if none equal, 8 if all equal
}

void ppu_interpreter::VCMPEQUH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::eq16(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VCMPEQUH_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPEQUH(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2; // set 2 if none equal, 8 if all equal
}

void ppu_interpreter::VCMPEQUW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::eq32(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VCMPEQUW_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPEQUW(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2; // set 2 if none equal, 8 if all equal
}

void ppu_interpreter::VCMPGEFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_cmpge_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VCMPGEFP_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGEFP(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_cmpgt_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VCMPGTFP_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTFP(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTSB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_cmpgt_epi8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTSB_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTSB(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTSH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_cmpgt_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTSH_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTSH(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTSW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_cmpgt_epi32(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTSW_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTSW(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = sse_cmpgt_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTUB_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTUB(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTUH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = sse_cmpgt_epu16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTUH_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTUH(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCMPGTUW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = sse_cmpgt_epu32(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VCMPGTUW_(PPUThread& CPU, ppu_opcode_t op)
{
	VCMPGTUW(CPU, op);

	CPU.CR.cr6 = CPU.VPR[op.vd].is_any_1() ? (CPU.VPR[op.vd].is_any_0() ? 0 : 8) : 2;
}

void ppu_interpreter::VCTSXS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto scaled = _mm_mul_ps(CPU.VPR[op.vb].vf, g_ppu_scale_table[op.vuimm]);
	CPU.VPR[op.vd].vi = _mm_xor_si128(_mm_cvttps_epi32(scaled), _mm_castps_si128(_mm_cmpge_ps(scaled, _mm_set1_ps(0x80000000))));
}

void ppu_interpreter::VCTUXS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto scaled1 = _mm_max_ps(_mm_mul_ps(CPU.VPR[op.vb].vf, g_ppu_scale_table[op.vuimm]), _mm_set1_ps(0.0f));
	const auto scaled2 = _mm_and_ps(_mm_sub_ps(scaled1, _mm_set1_ps(0x80000000)), _mm_cmpge_ps(scaled1, _mm_set1_ps(0x80000000)));
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_or_si128(_mm_cvttps_epi32(scaled1), _mm_cvttps_epi32(scaled2)), _mm_castps_si128(_mm_cmpge_ps(scaled1, _mm_set1_ps(0x100000000))));
}

void ppu_interpreter::VEXPTEFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = sse_exp2_ps(CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VLOGEFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = sse_log2_ps(CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VMADDFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_add_ps(_mm_mul_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vc].vf), CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VMAXFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_max_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VMAXSB(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
}

void ppu_interpreter::VMAXSH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_max_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VMAXSW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
}

void ppu_interpreter::VMAXUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_max_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VMAXUH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	CPU.VPR[op.vd].vi = _mm_xor_si128(_mm_max_epi16(_mm_xor_si128(CPU.VPR[op.va].vi, mask), _mm_xor_si128(CPU.VPR[op.vb].vi, mask)), mask);
}

void ppu_interpreter::VMAXUW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_and_si128(m, a), _mm_andnot_si128(m, b));
}

void ppu_interpreter::VMHADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto c = CPU.VPR[op.vc].vi;
	const auto m = _mm_or_si128(_mm_srli_epi16(_mm_mullo_epi16(a, b), 15), _mm_slli_epi16(_mm_mulhi_epi16(a, b), 1));
	const auto s = _mm_cmpeq_epi16(m, _mm_set1_epi16(-0x8000)); // detect special case (positive 0x8000)
	CPU.VPR[op.vd].vi = _mm_adds_epi16(_mm_adds_epi16(_mm_xor_si128(m, s), c), _mm_srli_epi16(s, 15));
}

void ppu_interpreter::VMHRADDSHS(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto c = CPU.VPR[op.vc].vi;
	const auto m = _mm_mulhrs_epi16(a, b);
	const auto s = _mm_cmpeq_epi16(m, _mm_set1_epi16(-0x8000)); // detect special case (positive 0x8000)
	CPU.VPR[op.vd].vi = _mm_adds_epi16(_mm_adds_epi16(_mm_xor_si128(m, s), c), _mm_srli_epi16(s, 15));
}

void ppu_interpreter::VMINFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_min_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VMINSB(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = _mm_cmpgt_epi8(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
}

void ppu_interpreter::VMINSH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_min_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VMINSW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = _mm_cmpgt_epi32(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
}

void ppu_interpreter::VMINUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_min_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VMINUH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x80008000);
	CPU.VPR[op.vd].vi = _mm_xor_si128(_mm_min_epi16(_mm_xor_si128(CPU.VPR[op.va].vi, mask), _mm_xor_si128(CPU.VPR[op.vb].vi, mask)), mask);
}

void ppu_interpreter::VMINUW(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto m = sse_cmpgt_epu32(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_andnot_si128(m, a), _mm_and_si128(m, b));
}

void ppu_interpreter::VMLADDUHM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_add_epi16(_mm_mullo_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi), CPU.VPR[op.vc].vi);
}

void ppu_interpreter::VMRGHB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpackhi_epi8(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMRGHH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpackhi_epi16(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMRGHW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpackhi_epi32(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMRGLB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpacklo_epi8(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMRGLH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpacklo_epi16(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMRGLW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_unpacklo_epi32(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VMSUMMBM(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi; // signed bytes
	const auto b = CPU.VPR[op.vb].vi; // unsigned bytes
	const auto c = CPU.VPR[op.vc].vi;
	const auto ah = _mm_srai_epi16(a, 8);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto al = _mm_srai_epi16(_mm_slli_epi16(a, 8), 8);
	const auto bl = _mm_and_si128(b, _mm_set1_epi16(0x00ff));
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	CPU.VPR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
}

void ppu_interpreter::VMSUMSHM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_add_epi32(_mm_madd_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi), CPU.VPR[op.vc].vi);
}

void ppu_interpreter::VMSUMSHS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		s64 result = 0;
		s32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += CPU.VPR[op.va]._s16[w * 2 + h] * CPU.VPR[op.vb]._s16[w * 2 + h];
		}

		result += CPU.VPR[op.vc]._s32[w];

		if (result > 0x7fffffff)
		{
			saturated = 0x7fffffff;
		}
		else if (result < (s64)(s32)0x80000000)
		{
			saturated = 0x80000000;
		}
		else
			saturated = (s32)result;

		CPU.VPR[op.vd]._s32[w] = saturated;
	}
}

void ppu_interpreter::VMSUMUBM(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto c = CPU.VPR[op.vc].vi;
	const auto mask = _mm_set1_epi16(0x00ff);
	const auto ah = _mm_srli_epi16(a, 8);
	const auto al = _mm_and_si128(a, mask);
	const auto bh = _mm_srli_epi16(b, 8);
	const auto bl = _mm_and_si128(b, mask);
	const auto sh = _mm_madd_epi16(ah, bh);
	const auto sl = _mm_madd_epi16(al, bl);
	CPU.VPR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, sh), sl);
}

void ppu_interpreter::VMSUMUHM(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto c = CPU.VPR[op.vc].vi;
	const auto ml = _mm_mullo_epi16(a, b); // low results
	const auto mh = _mm_mulhi_epu16(a, b); // high results
	const auto ls = _mm_add_epi32(_mm_srli_epi32(ml, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
	const auto hs = _mm_add_epi32(_mm_slli_epi32(mh, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
	CPU.VPR[op.vd].vi = _mm_add_epi32(_mm_add_epi32(c, ls), hs);
}

void ppu_interpreter::VMSUMUHS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		u64 result = 0;
		u32 saturated = 0;

		for (uint h = 0; h < 2; h++)
		{
			result += (u64)CPU.VPR[op.va]._u16[w * 2 + h] * (u64)CPU.VPR[op.vb]._u16[w * 2 + h];
		}

		result += CPU.VPR[op.vc]._u32[w];

		if (result > 0xffffffffu)
		{
			saturated = 0xffffffff;
		}
		else
			saturated = (u32)result;

		CPU.VPR[op.vd]._u32[w] = saturated;
	}
}

void ppu_interpreter::VMULESB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(CPU.VPR[op.va].vi, 8), _mm_srai_epi16(CPU.VPR[op.vb].vi, 8));
}

void ppu_interpreter::VMULESH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_madd_epi16(_mm_srli_epi32(CPU.VPR[op.va].vi, 16), _mm_srli_epi32(CPU.VPR[op.vb].vi, 16));
}

void ppu_interpreter::VMULEUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_mullo_epi16(_mm_srli_epi16(CPU.VPR[op.va].vi, 8), _mm_srli_epi16(CPU.VPR[op.vb].vi, 8));
}

void ppu_interpreter::VMULEUH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_srli_epi32(ml, 16), _mm_and_si128(mh, _mm_set1_epi32(0xffff0000)));
}

void ppu_interpreter::VMULOSB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_mullo_epi16(_mm_srai_epi16(_mm_slli_epi16(CPU.VPR[op.va].vi, 8), 8), _mm_srai_epi16(_mm_slli_epi16(CPU.VPR[op.vb].vi, 8), 8));
}

void ppu_interpreter::VMULOSH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0x0000ffff);
	CPU.VPR[op.vd].vi = _mm_madd_epi16(_mm_and_si128(CPU.VPR[op.va].vi, mask), _mm_and_si128(CPU.VPR[op.vb].vi, mask));
}

void ppu_interpreter::VMULOUB(PPUThread& CPU, ppu_opcode_t op)
{
	const auto mask = _mm_set1_epi16(0x00ff);
	CPU.VPR[op.vd].vi = _mm_mullo_epi16(_mm_and_si128(CPU.VPR[op.va].vi, mask), _mm_and_si128(CPU.VPR[op.vb].vi, mask));
}

void ppu_interpreter::VMULOUH(PPUThread& CPU, ppu_opcode_t op)
{
	const auto a = CPU.VPR[op.va].vi;
	const auto b = CPU.VPR[op.vb].vi;
	const auto ml = _mm_mullo_epi16(a, b);
	const auto mh = _mm_mulhi_epu16(a, b);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_slli_epi32(mh, 16), _mm_and_si128(ml, _mm_set1_epi32(0x0000ffff)));
}

void ppu_interpreter::VNMSUBFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_sub_ps(CPU.VPR[op.vb].vf, _mm_mul_ps(CPU.VPR[op.va].vf, CPU.VPR[op.vc].vf));
}

void ppu_interpreter::VNOR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = ~(CPU.VPR[op.va] | CPU.VPR[op.vb]);
}

void ppu_interpreter::VOR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = CPU.VPR[op.va] | CPU.VPR[op.vb];
}

void ppu_interpreter::VPERM(PPUThread& CPU, ppu_opcode_t op)
{
	const auto index = _mm_andnot_si128(CPU.VPR[op.vc].vi, _mm_set1_epi8(0x1f));
	const auto mask = _mm_cmpgt_epi8(index, _mm_set1_epi8(0xf));
	const auto sa = _mm_shuffle_epi8(CPU.VPR[op.va].vi, index);
	const auto sb = _mm_shuffle_epi8(CPU.VPR[op.vb].vi, index);
	CPU.VPR[op.vd].vi = _mm_or_si128(_mm_and_si128(mask, sa), _mm_andnot_si128(mask, sb));
}

void ppu_interpreter::VPKPX(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		u16 bb7 = VB._u8[15 - (h * 4 + 0)] & 0x1;
		u16 bb8 = VB._u8[15 - (h * 4 + 1)] >> 3;
		u16 bb16 = VB._u8[15 - (h * 4 + 2)] >> 3;
		u16 bb24 = VB._u8[15 - (h * 4 + 3)] >> 3;
		u16 ab7 = VA._u8[15 - (h * 4 + 0)] & 0x1;
		u16 ab8 = VA._u8[15 - (h * 4 + 1)] >> 3;
		u16 ab16 = VA._u8[15 - (h * 4 + 2)] >> 3;
		u16 ab24 = VA._u8[15 - (h * 4 + 3)] >> 3;

		CPU.VPR[op.vd]._u16[3 - h] = (bb7 << 15) | (bb8 << 10) | (bb16 << 5) | bb24;
		CPU.VPR[op.vd]._u16[4 + (3 - h)] = (ab7 << 15) | (ab8 << 10) | (ab16 << 5) | ab24;
	}
}

void ppu_interpreter::VPKSHSS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_packs_epi16(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VPKSHUS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_packus_epi16(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VPKSWSS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_packs_epi32(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);
}

void ppu_interpreter::VPKSWUS(PPUThread& CPU, ppu_opcode_t op)
{
	//CPU.VPR[op.vd].vi = _mm_packus_epi32(CPU.VPR[op.vb].vi, CPU.VPR[op.va].vi);

	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		s32 result = VA._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}
		else if (result < 0)
		{
			result = 0;
		}

		CPU.VPR[op.vd]._u16[h + 4] = result;

		result = VB._s32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}
		else if (result < 0)
		{
			result = 0;
		}

		CPU.VPR[op.vd]._u16[h] = result;
	}
}

void ppu_interpreter::VPKUHUM(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		CPU.VPR[op.vd]._u8[b + 8] = VA._u8[b * 2];
		CPU.VPR[op.vd]._u8[b] = VB._u8[b * 2];
	}
}

void ppu_interpreter::VPKUHUS(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint b = 0; b < 8; b++)
	{
		u16 result = VA._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		CPU.VPR[op.vd]._u8[b + 8] = (u8)result;

		result = VB._u16[b];

		if (result > UINT8_MAX)
		{
			result = UINT8_MAX;
		}

		CPU.VPR[op.vd]._u8[b] = (u8)result;
	}
}

void ppu_interpreter::VPKUWUM(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		CPU.VPR[op.vd]._u16[h + 4] = VA._u16[h * 2];
		CPU.VPR[op.vd]._u16[h] = VB._u16[h * 2];
	}
}

void ppu_interpreter::VPKUWUS(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 4; h++)
	{
		u32 result = VA._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}

		CPU.VPR[op.vd]._u16[h + 4] = result;

		result = VB._u32[h];

		if (result > UINT16_MAX)
		{
			result = UINT16_MAX;
		}

		CPU.VPR[op.vd]._u16[h] = result;
	}
}

void ppu_interpreter::VREFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_rcp_ps(CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VRFIM(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		const float b = CPU.VPR[op.vb]._f[w];
		CPU.VPR[op.vd]._f[w] = floorf(CPU.VPR[op.vb]._f[w]);
	}
}

void ppu_interpreter::VRFIN(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		const float b = CPU.VPR[op.vb]._f[w];
		CPU.VPR[op.vd]._f[w] = nearbyintf(CPU.VPR[op.vb]._f[w]);
	}
}

void ppu_interpreter::VRFIP(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		const float b = CPU.VPR[op.vb]._f[w];
		CPU.VPR[op.vd]._f[w] = ceilf(CPU.VPR[op.vb]._f[w]);
	}
}

void ppu_interpreter::VRFIZ(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		const float b = CPU.VPR[op.vb]._f[w];
		CPU.VPR[op.vd]._f[w] = truncf(CPU.VPR[op.vb]._f[w]);
	}
}

void ppu_interpreter::VRLB(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint b = 0; b < 16; b++)
	{
		int nRot = CPU.VPR[op.vb]._u8[b] & 0x7;

		CPU.VPR[op.vd]._u8[b] = (CPU.VPR[op.va]._u8[b] << nRot) | (CPU.VPR[op.va]._u8[b] >> (8 - nRot));
	}
}

void ppu_interpreter::VRLH(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._u16[h] = rotl16(CPU.VPR[op.va]._u16[h], CPU.VPR[op.vb]._u8[h * 2] & 0xf);
	}
}

void ppu_interpreter::VRLW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = (u32)rotl32(CPU.VPR[op.va]._u32[w], CPU.VPR[op.vb]._u8[w * 4] & 0x1f);
	}
}

void ppu_interpreter::VRSQRTEFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vf = _mm_rsqrt_ps(CPU.VPR[op.vb].vf);
}

void ppu_interpreter::VSEL(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = (CPU.VPR[op.vb] & CPU.VPR[op.vc]) | (CPU.VPR[op.va] & ~CPU.VPR[op.vc]);
}

void ppu_interpreter::VSL(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u8 sh = CPU.VPR[op.vb]._u8[0] & 0x7;

	CPU.VPR[op.vd]._u8[0] = VA._u8[0] << sh;
	for (uint b = 1; b < 16; b++)
	{
		CPU.VPR[op.vd]._u8[b] = (VA._u8[b] << sh) | (VA._u8[b - 1] >> (8 - sh));
	}
}

void ppu_interpreter::VSLB(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint b = 0; b < 16; b++)
	{
		CPU.VPR[op.vd]._u8[b] = CPU.VPR[op.va]._u8[b] << (CPU.VPR[op.vb]._u8[b] & 0x7);
	}
}

void ppu_interpreter::VSLDOI(PPUThread& CPU, ppu_opcode_t op)
{
	u8 tmpSRC[32];
	memcpy(tmpSRC, CPU.VPR[op.vb]._u8, 16);
	memcpy(tmpSRC + 16, CPU.VPR[op.va]._u8, 16);

	for (uint b = 0; b<16; b++)
	{
		CPU.VPR[op.vd]._u8[15 - b] = tmpSRC[31 - (b + op.vsh)];
	}
}

void ppu_interpreter::VSLH(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._u16[h] = CPU.VPR[op.va]._u16[h] << (CPU.VPR[op.vb]._u16[h] & 0xf);
	}
}

void ppu_interpreter::VSLO(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u8 nShift = (CPU.VPR[op.vb]._u8[0] >> 3) & 0xf;

	CPU.VPR[op.vd].clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		CPU.VPR[op.vd]._u8[15 - b] = VA._u8[15 - (b + nShift)];
	}
}

void ppu_interpreter::VSLW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = CPU.VPR[op.va]._u32[w] << (CPU.VPR[op.vb]._u32[w] & 0x1f);
	}
}

void ppu_interpreter::VSPLTB(PPUThread& CPU, ppu_opcode_t op)
{
	u8 byte = CPU.VPR[op.vb]._u8[15 - op.vuimm];

	for (uint b = 0; b < 16; b++)
	{
		CPU.VPR[op.vd]._u8[b] = byte;
	}
}

void ppu_interpreter::VSPLTH(PPUThread& CPU, ppu_opcode_t op)
{
	assert(op.vuimm < 8);

	u16 hword = CPU.VPR[op.vb]._u16[7 - op.vuimm];

	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._u16[h] = hword;
	}
}

void ppu_interpreter::VSPLTISB(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint b = 0; b < 16; b++)
	{
		CPU.VPR[op.vd]._u8[b] = op.vsimm;
	}
}

void ppu_interpreter::VSPLTISH(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._u16[h] = (s16)op.vsimm;
	}
}

void ppu_interpreter::VSPLTISW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = (s32)op.vsimm;
	}
}

void ppu_interpreter::VSPLTW(PPUThread& CPU, ppu_opcode_t op)
{
	assert(op.vuimm < 4);

	u32 word = CPU.VPR[op.vb]._u32[3 - op.vuimm];

	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = word;
	}
}

void ppu_interpreter::VSR(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u8 sh = CPU.VPR[op.vb]._u8[0] & 0x7;

	CPU.VPR[op.vd]._u8[15] = VA._u8[15] >> sh;
	for (uint b = 14; ~b; b--)
	{
		CPU.VPR[op.vd]._u8[b] = (VA._u8[b] >> sh) | (VA._u8[b + 1] << (8 - sh));
	}
}

void ppu_interpreter::VSRAB(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint b = 0; b < 16; b++)
	{
		CPU.VPR[op.vd]._s8[b] = CPU.VPR[op.va]._s8[b] >> (CPU.VPR[op.vb]._u8[b] & 0x7);
	}
}

void ppu_interpreter::VSRAH(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._s16[h] = CPU.VPR[op.va]._s16[h] >> (CPU.VPR[op.vb]._u16[h] & 0xf);
	}
}

void ppu_interpreter::VSRAW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._s32[w] = CPU.VPR[op.va]._s32[w] >> (CPU.VPR[op.vb]._u32[w] & 0x1f);
	}
}

void ppu_interpreter::VSRB(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint b = 0; b < 16; b++)
	{
		CPU.VPR[op.vd]._u8[b] = CPU.VPR[op.va]._u8[b] >> (CPU.VPR[op.vb]._u8[b] & 0x7);
	}
}

void ppu_interpreter::VSRH(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._u16[h] = CPU.VPR[op.va]._u16[h] >> (CPU.VPR[op.vb]._u16[h] & 0xf);
	}
}

void ppu_interpreter::VSRO(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VA = CPU.VPR[op.va];
	u8 nShift = (CPU.VPR[op.vb]._u8[0] >> 3) & 0xf;

	CPU.VPR[op.vd].clear();

	for (u8 b = 0; b < 16 - nShift; b++)
	{
		CPU.VPR[op.vd]._u8[b] = VA._u8[b + nShift];
	}
}

void ppu_interpreter::VSRW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = CPU.VPR[op.va]._u32[w] >> (CPU.VPR[op.vb]._u32[w] & 0x1f);
	}
}

void ppu_interpreter::VSUBCUW(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._u32[w] = CPU.VPR[op.va]._u32[w] < CPU.VPR[op.vb]._u32[w] ? 0 : 1;
	}
}

void ppu_interpreter::VSUBFP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::subfs(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VSUBSBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_subs_epi8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VSUBSHS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_subs_epi16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VSUBSWS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		s64 result = (s64)CPU.VPR[op.va]._s32[w] - (s64)CPU.VPR[op.vb]._s32[w];

		if (result < INT32_MIN)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MIN;
		}
		else if (result > INT32_MAX)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MAX;
		}
		else
			CPU.VPR[op.vd]._s32[w] = (s32)result;
	}
}

void ppu_interpreter::VSUBUBM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::sub8(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VSUBUBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_subs_epu8(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VSUBUHM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::sub16(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VSUBUHS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd].vi = _mm_subs_epu16(CPU.VPR[op.va].vi, CPU.VPR[op.vb].vi);
}

void ppu_interpreter::VSUBUWM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = u128::sub32(CPU.VPR[op.va], CPU.VPR[op.vb]);
}

void ppu_interpreter::VSUBUWS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		s64 result = (s64)CPU.VPR[op.va]._u32[w] - (s64)CPU.VPR[op.vb]._u32[w];

		if (result < 0)
		{
			CPU.VPR[op.vd]._u32[w] = 0;
		}
		else
			CPU.VPR[op.vd]._u32[w] = (u32)result;
	}
}

void ppu_interpreter::VSUMSWS(PPUThread& CPU, ppu_opcode_t op)
{
	s64 sum = CPU.VPR[op.vb]._s32[0];

	for (uint w = 0; w < 4; w++)
	{
		sum += CPU.VPR[op.va]._s32[w];
	}

	CPU.VPR[op.vd].clear();
	if (sum > INT32_MAX)
	{
		CPU.VPR[op.vd]._s32[0] = (s32)INT32_MAX;
	}
	else if (sum < INT32_MIN)
	{
		CPU.VPR[op.vd]._s32[0] = (s32)INT32_MIN;
	}
	else
		CPU.VPR[op.vd]._s32[0] = (s32)sum;
}

void ppu_interpreter::VSUM2SWS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint n = 0; n < 2; n++)
	{
		s64 sum = (s64)CPU.VPR[op.va]._s32[n * 2] + CPU.VPR[op.va]._s32[n * 2 + 1] + CPU.VPR[op.vb]._s32[n * 2];

		if (sum > INT32_MAX)
		{
			CPU.VPR[op.vd]._s32[n * 2] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			CPU.VPR[op.vd]._s32[n * 2] = (s32)INT32_MIN;
		}
		else
			CPU.VPR[op.vd]._s32[n * 2] = (s32)sum;
	}
	CPU.VPR[op.vd]._s32[1] = 0;
	CPU.VPR[op.vd]._s32[3] = 0;
}

void ppu_interpreter::VSUM4SBS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		s64 sum = CPU.VPR[op.vb]._s32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += CPU.VPR[op.va]._s8[w * 4 + b];
		}

		if (sum > INT32_MAX)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MIN;
		}
		else
			CPU.VPR[op.vd]._s32[w] = (s32)sum;
	}
}

void ppu_interpreter::VSUM4SHS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		s64 sum = CPU.VPR[op.vb]._s32[w];

		for (uint h = 0; h < 2; h++)
		{
			sum += CPU.VPR[op.va]._s16[w * 2 + h];
		}

		if (sum > INT32_MAX)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MAX;
		}
		else if (sum < INT32_MIN)
		{
			CPU.VPR[op.vd]._s32[w] = (s32)INT32_MIN;
		}
		else
			CPU.VPR[op.vd]._s32[w] = (s32)sum;
	}
}

void ppu_interpreter::VSUM4UBS(PPUThread& CPU, ppu_opcode_t op)
{
	for (uint w = 0; w < 4; w++)
	{
		u64 sum = CPU.VPR[op.vb]._u32[w];

		for (uint b = 0; b < 4; b++)
		{
			sum += CPU.VPR[op.va]._u8[w * 4 + b];
		}

		if (sum > UINT32_MAX)
		{
			CPU.VPR[op.vd]._u32[w] = (u32)UINT32_MAX;
		}
		else
			CPU.VPR[op.vd]._u32[w] = (u32)sum;
	}
}

void ppu_interpreter::VUPKHPX(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._s8[w * 4 + 3] = VB._s8[8 + w * 2 + 1] >> 7;  // signed shift sign extends
		CPU.VPR[op.vd]._u8[w * 4 + 2] = (VB._u8[8 + w * 2 + 1] >> 2) & 0x1f;
		CPU.VPR[op.vd]._u8[w * 4 + 1] = ((VB._u8[8 + w * 2 + 1] & 0x3) << 3) | ((VB._u8[8 + w * 2 + 0] >> 5) & 0x7);
		CPU.VPR[op.vd]._u8[w * 4 + 0] = VB._u8[8 + w * 2 + 0] & 0x1f;
	}
}

void ppu_interpreter::VUPKHSB(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._s16[h] = VB._s8[8 + h];
	}
}

void ppu_interpreter::VUPKHSH(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._s32[w] = VB._s16[4 + w];
	}
}

void ppu_interpreter::VUPKLPX(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._s8[w * 4 + 3] = VB._s8[w * 2 + 1] >> 7;  // signed shift sign extends
		CPU.VPR[op.vd]._u8[w * 4 + 2] = (VB._u8[w * 2 + 1] >> 2) & 0x1f;
		CPU.VPR[op.vd]._u8[w * 4 + 1] = ((VB._u8[w * 2 + 1] & 0x3) << 3) | ((VB._u8[w * 2 + 0] >> 5) & 0x7);
		CPU.VPR[op.vd]._u8[w * 4 + 0] = VB._u8[w * 2 + 0] & 0x1f;
	}
}

void ppu_interpreter::VUPKLSB(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint h = 0; h < 8; h++)
	{
		CPU.VPR[op.vd]._s16[h] = VB._s8[h];
	}
}

void ppu_interpreter::VUPKLSH(PPUThread& CPU, ppu_opcode_t op)
{
	u128 VB = CPU.VPR[op.vb];
	for (uint w = 0; w < 4; w++)
	{
		CPU.VPR[op.vd]._s32[w] = VB._s16[w];
	}
}

void ppu_interpreter::VXOR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.VPR[op.vd] = CPU.VPR[op.va] ^ CPU.VPR[op.vb];
}

void ppu_interpreter::MULLI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = (s64)CPU.GPR[op.ra] * op.simm16;
}

void ppu_interpreter::SUBFIC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 IMM = (s64)op.simm16;
	CPU.GPR[op.rd] = ~RA + IMM + 1;

	CPU.XER.CA = CPU.IsCarry(~RA, IMM, 1);
}

void ppu_interpreter::CMPLI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.UpdateCRnU(op.l10, op.crfd, CPU.GPR[op.ra], op.uimm16);
}

void ppu_interpreter::CMPI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.UpdateCRnS(op.l10, op.crfd, CPU.GPR[op.ra], op.simm16);
}

void ppu_interpreter::ADDIC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + op.simm16;
	CPU.XER.CA = CPU.IsCarry(RA, op.simm16);
}

void ppu_interpreter::ADDIC_(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + op.simm16;
	CPU.XER.CA = CPU.IsCarry(RA, op.simm16);
	CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = op.ra ? ((s64)CPU.GPR[op.ra] + op.simm16) : op.simm16;
}

void ppu_interpreter::ADDIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = op.ra ? ((s64)CPU.GPR[op.ra] + (op.simm16 << 16)) : (op.simm16 << 16);
}

void ppu_interpreter::BC(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 bo0 = (op.bo & 0x10) ? 1 : 0;
	const u8 bo1 = (op.bo & 0x08) ? 1 : 0;
	const u8 bo2 = (op.bo & 0x04) ? 1 : 0;
	const u8 bo3 = (op.bo & 0x02) ? 1 : 0;

	if (!bo2) --CPU.CTR;

	const u8 ctr_ok = bo2 | ((CPU.CTR != 0) ^ bo3);
	const u8 cond_ok = bo0 | (CPU.IsCR(op.bi) ^ (~bo1 & 0x1));

	if (ctr_ok && cond_ok)
	{
		const u32 nextLR = CPU.PC + 4;
		CPU.SetBranch(PPUOpcodes::branchTarget((op.aa ? 0 : CPU.PC), op.simm16), op.lk);
		if (op.lk) CPU.LR = nextLR;
	}
}

void ppu_interpreter::HACK(PPUThread& CPU, ppu_opcode_t op)
{
	execute_ppu_func_by_index(CPU, op.opcode & 0x3ffffff);
}

void ppu_interpreter::SC(PPUThread& CPU, ppu_opcode_t op)
{
	switch (op.lev)
	{
	case 0x0: SysCalls::DoSyscall(CPU, CPU.GPR[11]); break;
	case 0x3: CPU.FastStop(); break;
	default: throw __FUNCTION__;
	}
}

void ppu_interpreter::B(PPUThread& CPU, ppu_opcode_t op)
{
	const u32 nextLR = CPU.PC + 4;
	CPU.SetBranch(PPUOpcodes::branchTarget(op.aa ? 0 : CPU.PC, op.ll), op.lk);
	if (op.lk) CPU.LR = nextLR;
}

void ppu_interpreter::MCRF(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.SetCR(op.crfd, CPU.GetCR(op.crfs));
}

void ppu_interpreter::BCLR(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 bo0 = (op.bo & 0x10) ? 1 : 0;
	const u8 bo1 = (op.bo & 0x08) ? 1 : 0;
	const u8 bo2 = (op.bo & 0x04) ? 1 : 0;
	const u8 bo3 = (op.bo & 0x02) ? 1 : 0;

	if (!bo2) --CPU.CTR;

	const u8 ctr_ok = bo2 | ((CPU.CTR != 0) ^ bo3);
	const u8 cond_ok = bo0 | (CPU.IsCR(op.bi) ^ (~bo1 & 0x1));

	if (ctr_ok && cond_ok)
	{
		const u32 nextLR = CPU.PC + 4;
		CPU.SetBranch(PPUOpcodes::branchTarget(0, (u32)CPU.LR), true);
		if (op.lk) CPU.LR = nextLR;
	}
}

void ppu_interpreter::CRNOR(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = 1 ^ (CPU.IsCR(op.crba) | CPU.IsCR(op.crbb));
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CRANDC(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = CPU.IsCR(op.crba) & (1 ^ CPU.IsCR(op.crbb));
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::ISYNC(PPUThread& CPU, ppu_opcode_t op)
{
	_mm_mfence();
}

void ppu_interpreter::CRXOR(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = CPU.IsCR(op.crba) ^ CPU.IsCR(op.crbb);
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CRNAND(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = 1 ^ (CPU.IsCR(op.crba) & CPU.IsCR(op.crbb));
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CRAND(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = CPU.IsCR(op.crba) & CPU.IsCR(op.crbb);
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CREQV(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = 1 ^ (CPU.IsCR(op.crba) ^ CPU.IsCR(op.crbb));
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CRORC(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = CPU.IsCR(op.crba) | (1 ^ CPU.IsCR(op.crbb));
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::CROR(PPUThread& CPU, ppu_opcode_t op)
{
	const u8 v = CPU.IsCR(op.crba) | CPU.IsCR(op.crbb);
	CPU.SetCRBit2(op.crbd, v & 0x1);
}

void ppu_interpreter::BCCTR(PPUThread& CPU, ppu_opcode_t op)
{
	if (op.bo & 0x10 || CPU.IsCR(op.bi) == ((op.bo & 0x8) != 0))
	{
		const u32 nextLR = CPU.PC + 4;
		CPU.SetBranch(PPUOpcodes::branchTarget(0, (u32)CPU.CTR), true);
		if (op.lk) CPU.LR = nextLR;
	}
}

void ppu_interpreter::RLWIMI(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 mask = rotate_mask[32 + op.mb][32 + op.me];
	CPU.GPR[op.ra] = (CPU.GPR[op.ra] & ~mask) | (rotl32(CPU.GPR[op.rs], op.sh) & mask);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLWINM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = rotl32(CPU.GPR[op.rs], op.sh) & rotate_mask[32 + op.mb][32 + op.me];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLWNM(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = rotl32(CPU.GPR[op.rs], CPU.GPR[op.rb] & 0x1f) & rotate_mask[32 + op.mb][32 + op.me];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ORI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | op.uimm16;
}

void ppu_interpreter::ORIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | ((u64)op.uimm16 << 16);
}

void ppu_interpreter::XORI(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] ^ op.uimm16;
}

void ppu_interpreter::XORIS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] ^ ((u64)op.uimm16 << 16);
}

void ppu_interpreter::ANDI_(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & op.uimm16;
	CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ANDIS_(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & ((u64)op.uimm16 << 16);
	CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDICL(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (op.shh << 5) | op.shl;
	auto mb = (op.mbmeh << 5) | op.mbmel;

	CPU.GPR[op.ra] = rotl64(CPU.GPR[op.rs], sh) & rotate_mask[mb][63];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDICR(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (op.shh << 5) | op.shl;
	auto me = (op.mbmeh << 5) | op.mbmel;
	
	CPU.GPR[op.ra] = rotl64(CPU.GPR[op.rs], sh) & rotate_mask[0][me];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDIC(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (op.shh << 5) | op.shl;
	auto mb = (op.mbmeh << 5) | op.mbmel;

	CPU.GPR[op.ra] = rotl64(CPU.GPR[op.rs], sh) & rotate_mask[mb][63 - sh];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDIMI(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (op.shh << 5) | op.shl;
	auto mb = (op.mbmeh << 5) | op.mbmel;

	const u64 mask = rotate_mask[mb][63 - sh];
	CPU.GPR[op.ra] = (CPU.GPR[op.ra] & ~mask) | (rotl64(CPU.GPR[op.rs], sh) & mask);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::RLDC_LR(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (u32)(CPU.GPR[op.rb] & 0x3F);
	auto mbme = (op.mbmeh << 5) | op.mbmel;

	if (op.aa) // rldcr
	{
		CPU.GPR[op.ra] = rotl64(CPU.GPR[op.rs], sh) & rotate_mask[0][mbme];
	}
	else // rldcl
	{
		CPU.GPR[op.ra] = rotl64(CPU.GPR[op.rs], sh) & rotate_mask[mbme][63];
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::CMP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.UpdateCRnS(op.l10, op.crfd, CPU.GPR[op.ra], CPU.GPR[op.rb]);
}

void ppu_interpreter::TW(PPUThread& CPU, ppu_opcode_t op)
{
	s32 a = (s32)CPU.GPR[op.ra];
	s32 b = (s32)CPU.GPR[op.rb];

	if ((a < b && (op.bo & 0x10)) ||
		(a > b && (op.bo & 0x8)) ||
		(a == b && (op.bo & 0x4)) ||
		((u32)a < (u32)b && (op.bo & 0x2)) ||
		((u32)a >(u32)b && (op.bo & 0x1)))
	{
		throw __FUNCTION__;
	}
}

void ppu_interpreter::LVSL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	static const u64 lvsl_values[0x10][2] =
	{
		{ 0x08090A0B0C0D0E0F, 0x0001020304050607 },
		{ 0x090A0B0C0D0E0F10, 0x0102030405060708 },
		{ 0x0A0B0C0D0E0F1011, 0x0203040506070809 },
		{ 0x0B0C0D0E0F101112, 0x030405060708090A },
		{ 0x0C0D0E0F10111213, 0x0405060708090A0B },
		{ 0x0D0E0F1011121314, 0x05060708090A0B0C },
		{ 0x0E0F101112131415, 0x060708090A0B0C0D },
		{ 0x0F10111213141516, 0x0708090A0B0C0D0E },
		{ 0x1011121314151617, 0x08090A0B0C0D0E0F },
		{ 0x1112131415161718, 0x090A0B0C0D0E0F10 },
		{ 0x1213141516171819, 0x0A0B0C0D0E0F1011 },
		{ 0x131415161718191A, 0x0B0C0D0E0F101112 },
		{ 0x1415161718191A1B, 0x0C0D0E0F10111213 },
		{ 0x15161718191A1B1C, 0x0D0E0F1011121314 },
		{ 0x161718191A1B1C1D, 0x0E0F101112131415 },
		{ 0x1718191A1B1C1D1E, 0x0F10111213141516 },
	};

	CPU.VPR[op.vd]._u64[0] = lvsl_values[addr & 0xf][0];
	CPU.VPR[op.vd]._u64[1] = lvsl_values[addr & 0xf][1];
}

void ppu_interpreter::LVEBX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.VPR[op.vd]._u8[15 - (addr & 0xf)] = vm::read8(vm::cast(addr));
}

void ppu_interpreter::SUBFC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = ~RA + RB + 1;
	CPU.XER.CA = CPU.IsCarry(~RA, RB, 1);
	if (op.oe) CPU.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MULHDU(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = __umulh(CPU.GPR[op.ra], CPU.GPR[op.rb]);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDC(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = RA + RB;
	CPU.XER.CA = CPU.IsCarry(RA, RB);
	if (op.oe) CPU.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MULHWU(PPUThread& CPU, ppu_opcode_t op)
{
	u32 a = (u32)CPU.GPR[op.ra];
	u32 b = (u32)CPU.GPR[op.rb];
	CPU.GPR[op.rd] = ((u64)a * (u64)b) >> 32;
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MFOCRF(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = CPU.CR.CR;
}

void ppu_interpreter::LWARX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	be_t<u32> value;
	vm::reservation_acquire(&value, vm::cast(addr), sizeof(value));

	CPU.GPR[op.rd] = value;
}

void ppu_interpreter::LDX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read64(vm::cast(addr));
}

void ppu_interpreter::LWZX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read32(vm::cast(addr));
}

void ppu_interpreter::SLW(PPUThread& CPU, ppu_opcode_t op)
{
	u32 n = CPU.GPR[op.rb] & 0x1f;
	u32 r = (u32)rotl32((u32)CPU.GPR[op.rs], n);
	u32 m = ((u32)CPU.GPR[op.rb] & 0x20) ? 0 : (u32)rotate_mask[32][63 - n];

	CPU.GPR[op.ra] = r & m;

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::CNTLZW(PPUThread& CPU, ppu_opcode_t op)
{
	u32 i;
	for (i = 0; i < 32; i++)
	{
		if (CPU.GPR[op.rs] & (1ULL << (31 - i))) break;
	}

	CPU.GPR[op.ra] = i;
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::SLD(PPUThread& CPU, ppu_opcode_t op)
{
	u32 n = CPU.GPR[op.rb] & 0x3f;
	u64 r = rotl64(CPU.GPR[op.rs], n);
	u64 m = (CPU.GPR[op.rb] & 0x40) ? 0 : rotate_mask[0][63 - n];

	CPU.GPR[op.ra] = r & m;

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::AND(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & CPU.GPR[op.rb];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::CMPL(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.UpdateCRnU(op.l10, op.crfd, CPU.GPR[op.ra], CPU.GPR[op.rb]);
}

void ppu_interpreter::LVSR(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	static const u64 lvsr_values[0x10][2] =
	{
		{ 0x18191A1B1C1D1E1F, 0x1011121314151617 },
		{ 0x1718191A1B1C1D1E, 0x0F10111213141516 },
		{ 0x161718191A1B1C1D, 0x0E0F101112131415 },
		{ 0x15161718191A1B1C, 0x0D0E0F1011121314 },
		{ 0x1415161718191A1B, 0x0C0D0E0F10111213 },
		{ 0x131415161718191A, 0x0B0C0D0E0F101112 },
		{ 0x1213141516171819, 0x0A0B0C0D0E0F1011 },
		{ 0x1112131415161718, 0x090A0B0C0D0E0F10 },
		{ 0x1011121314151617, 0x08090A0B0C0D0E0F },
		{ 0x0F10111213141516, 0x0708090A0B0C0D0E },
		{ 0x0E0F101112131415, 0x060708090A0B0C0D },
		{ 0x0D0E0F1011121314, 0x05060708090A0B0C },
		{ 0x0C0D0E0F10111213, 0x0405060708090A0B },
		{ 0x0B0C0D0E0F101112, 0x030405060708090A },
		{ 0x0A0B0C0D0E0F1011, 0x0203040506070809 },
		{ 0x090A0B0C0D0E0F10, 0x0102030405060708 },
	};

	CPU.VPR[op.vd]._u64[0] = lvsr_values[addr & 0xf][0];
	CPU.VPR[op.vd]._u64[1] = lvsr_values[addr & 0xf][1];
}

void ppu_interpreter::LVEHX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~1ULL;
	CPU.VPR[op.vd]._u16[7 - ((addr >> 1) & 0x7)] = vm::read16(vm::cast(addr));
}

void ppu_interpreter::SUBF(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = RB - RA;
	if (op.oe) CPU.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::LDUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read64(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::DCBST(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::LWZUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read32(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::CNTLZD(PPUThread& CPU, ppu_opcode_t op)
{
	u32 i;
	for (i = 0; i < 64; i++)
	{
		if (CPU.GPR[op.rs] & (1ULL << (63 - i))) break;
	}

	CPU.GPR[op.ra] = i;
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ANDC(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] & ~CPU.GPR[op.rb];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::TD(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::LVEWX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~3ULL;
	CPU.VPR[op.vd]._u32[3 - ((addr >> 2) & 0x3)] = vm::read32(vm::cast(addr));
}

void ppu_interpreter::MULHD(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = __mulh(CPU.GPR[op.ra], CPU.GPR[op.rb]);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MULHW(PPUThread& CPU, ppu_opcode_t op)
{
	s32 a = (s32)CPU.GPR[op.ra];
	s32 b = (s32)CPU.GPR[op.rb];
	CPU.GPR[op.rd] = ((s64)a * (s64)b) >> 32;
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::LDARX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	be_t<u64> value;
	vm::reservation_acquire(&value, vm::cast(addr), sizeof(value));

	CPU.GPR[op.rd] = value;
}

void ppu_interpreter::DCBF(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::LBZX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read8(vm::cast(addr));
}

void ppu_interpreter::LVX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~0xfull;
	CPU.VPR[op.vd] = vm::read128(vm::cast(addr));
}

void ppu_interpreter::NEG(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = 0 - RA;
	if (op.oe) CPU.SetOV((~RA >> 63 == 0) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::LBZUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read8(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::NOR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = ~(CPU.GPR[op.rs] | CPU.GPR[op.rb]);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::STVEBX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u8 eb = addr & 0xf;
	vm::write8(vm::cast(addr), CPU.VPR[op.vs]._u8[15 - eb]);
}

void ppu_interpreter::SUBFE(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = ~RA + RB + CPU.XER.CA;
	CPU.XER.CA = CPU.IsCarry(~RA, RB, CPU.XER.CA);
	if (op.oe) CPU.SetOV((~RA >> 63 == RB >> 63) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDE(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	if (CPU.XER.CA)
	{
		if (RA == ~0ULL) //-1
		{
			CPU.GPR[op.rd] = RB;
			CPU.XER.CA = 1;
		}
		else
		{
			CPU.GPR[op.rd] = RA + 1 + RB;
			CPU.XER.CA = CPU.IsCarry(RA + 1, RB);
		}
	}
	else
	{
		CPU.GPR[op.rd] = RA + RB;
		CPU.XER.CA = CPU.IsCarry(RA, RB);
	}
	if (op.oe) CPU.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MTOCRF(PPUThread& CPU, ppu_opcode_t op)
{
	if (op.l11)
	{
		u32 n = 0, count = 0;
		for (u32 i = 0; i<8; ++i)
		{
			if (op.crm & (1 << i))
			{
				n = i;
				count++;
			}
		}

		if (count == 1)
		{
			//CR[4*n : 4*n+3] = RS[32+4*n : 32+4*n+3];
			CPU.SetCR(7 - n, (CPU.GPR[op.rs] >> (4 * n)) & 0xf);
		}
		else
			CPU.CR.CR = 0;
	}
	else
	{
		for (u32 i = 0; i<8; ++i)
		{
			if (op.crm & (1 << i))
			{
				CPU.SetCR(7 - i, (CPU.GPR[op.rs] >> (i * 4)) & 0xf);
			}
		}
	}
}

void ppu_interpreter::STDX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::write64(vm::cast(addr), CPU.GPR[op.rs]);
}

void ppu_interpreter::STWCX_(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	const be_t<u32> value = (u32)CPU.GPR[op.rs];
	CPU.SetCR_EQ(0, vm::reservation_update(vm::cast(addr), &value, sizeof(value)));
}

void ppu_interpreter::STWX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::write32(vm::cast(addr), (u32)CPU.GPR[op.rs]);
}

void ppu_interpreter::STVEHX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~1ULL;
	const u8 eb = (addr & 0xf) >> 1;
	vm::write16(vm::cast(addr), CPU.VPR[op.vs]._u16[7 - eb]);
}

void ppu_interpreter::STDUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::write64(vm::cast(addr), CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STWUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::write32(vm::cast(addr), (u32)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STVEWX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~3ULL;
	const u8 eb = (addr & 0xf) >> 2;
	vm::write32(vm::cast(addr), CPU.VPR[op.vs]._u32[3 - eb]);
}

void ppu_interpreter::SUBFZE(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = ~RA + CPU.XER.CA;
	CPU.XER.CA = CPU.IsCarry(~RA, CPU.XER.CA);
	if (op.oe) CPU.SetOV((~RA >> 63 == 0) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDZE(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + CPU.XER.CA;
	CPU.XER.CA = CPU.IsCarry(RA, CPU.XER.CA);
	if (op.oe) CPU.SetOV((RA >> 63 == 0) && (RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::STDCX_(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	const be_t<u64> value = CPU.GPR[op.rs];
	CPU.SetCR_EQ(0, vm::reservation_update(vm::cast(addr), &value, sizeof(value)));
}

void ppu_interpreter::STBX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::write8(vm::cast(addr), (u8)CPU.GPR[op.rs]);
}

void ppu_interpreter::STVX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~0xfull;
	vm::write128(vm::cast(addr), CPU.VPR[op.vs]);
}

void ppu_interpreter::MULLD(PPUThread& CPU, ppu_opcode_t op)
{
	const s64 RA = CPU.GPR[op.ra];
	const s64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = (s64)(RA * RB);
	if (op.oe)
	{
		const s64 high = __mulh(RA, RB);
		CPU.SetOV(high != s64(CPU.GPR[op.rd]) >> 63);
	}
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::SUBFME(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = ~RA + CPU.XER.CA + ~0ULL;
	CPU.XER.CA = CPU.IsCarry(~RA, CPU.XER.CA, ~0ULL);
	if (op.oe) CPU.SetOV((~RA >> 63 == 1) && (~RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::ADDME(PPUThread& CPU, ppu_opcode_t op)
{
	const s64 RA = CPU.GPR[op.ra];
	CPU.GPR[op.rd] = RA + CPU.XER.CA - 1;
	CPU.XER.CA |= RA != 0;

	if (op.oe) CPU.SetOV((u64(RA) >> 63 == 1) && (u64(RA) >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MULLW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.rd] = (s64)((s64)(s32)CPU.GPR[op.ra] * (s64)(s32)CPU.GPR[op.rb]);
	if (op.oe) CPU.SetOV(s64(CPU.GPR[op.rd]) < s64(-1) << 31 || s64(CPU.GPR[op.rd]) >= s64(1) << 31);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::DCBTST(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::STBUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::write8(vm::cast(addr), (u8)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::ADD(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];
	CPU.GPR[op.rd] = RA + RB;
	if (op.oe) CPU.SetOV((RA >> 63 == RB >> 63) && (RA >> 63 != CPU.GPR[op.rd] >> 63));
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::DCBT(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::LHZX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read16(vm::cast(addr));
}

void ppu_interpreter::EQV(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = ~(CPU.GPR[op.rs] ^ CPU.GPR[op.rb]);
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ECIWX(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::LHZUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::read16(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::XOR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] ^ CPU.GPR[op.rb];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::MFSPR(PPUThread& CPU, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001: CPU.GPR[op.rd] = CPU.XER.XER; return;
	case 0x008: CPU.GPR[op.rd] = CPU.LR; return;
	case 0x009: CPU.GPR[op.rd] = CPU.CTR; return;
	case 0x100: CPU.GPR[op.rd] = CPU.VRSAVE; return;
	case 0x103: CPU.GPR[op.rd] = CPU.SPRG[3]; return;

	case 0x10C: CPU.TB = get_time(); CPU.GPR[op.rd] = CPU.TB; return;
	case 0x10D: CPU.TB = get_time(); CPU.GPR[op.rd] = CPU.TB >> 32; return;

	case 0x110:
	case 0x111:
	case 0x112:
	case 0x113:
	case 0x114:
	case 0x115:
	case 0x116:
	case 0x117: CPU.GPR[op.rd] = CPU.SPRG[n - 0x110]; return;
	}

	throw __FUNCTION__;
}

void ppu_interpreter::LWAX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr));
}

void ppu_interpreter::DST(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::LHAX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr));
}

void ppu_interpreter::LVXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~0xfull;
	CPU.VPR[op.vd] = vm::read128(vm::cast(addr));
}

void ppu_interpreter::MFTB(PPUThread& CPU, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	CPU.TB = get_time();
	switch (n)
	{
	case 0x10C: CPU.GPR[op.rd] = CPU.TB; break;
	case 0x10D: CPU.GPR[op.rd] = CPU.TB >> 32; break;
	default: throw __FUNCTION__;
	}
}

void ppu_interpreter::LWAUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::DSTST(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::LHAUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STHX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::write16(vm::cast(addr), (u16)CPU.GPR[op.rs]);
}

void ppu_interpreter::ORC(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | ~CPU.GPR[op.rb];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ECOWX(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}

void ppu_interpreter::STHUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::write16(vm::cast(addr), (u16)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::OR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = CPU.GPR[op.rs] | CPU.GPR[op.rb];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::DIVDU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 RA = CPU.GPR[op.ra];
	const u64 RB = CPU.GPR[op.rb];

	if (RB == 0)
	{
		if (op.oe) CPU.SetOV(true);
		CPU.GPR[op.rd] = 0;
	}
	else
	{
		if (op.oe) CPU.SetOV(false);
		CPU.GPR[op.rd] = RA / RB;
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::DIVWU(PPUThread& CPU, ppu_opcode_t op)
{
	const u32 RA = (u32)CPU.GPR[op.ra];
	const u32 RB = (u32)CPU.GPR[op.rb];

	if (RB == 0)
	{
		if (op.oe) CPU.SetOV(true);
		CPU.GPR[op.rd] = 0;
	}
	else
	{
		if (op.oe) CPU.SetOV(false);
		CPU.GPR[op.rd] = RA / RB;
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::MTSPR(PPUThread& CPU, ppu_opcode_t op)
{
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001: CPU.XER.XER = CPU.GPR[op.rs]; return;
	case 0x008: CPU.LR = CPU.GPR[op.rs]; return;
	case 0x009: CPU.CTR = CPU.GPR[op.rs]; return;
	case 0x100: CPU.VRSAVE = (u32)CPU.GPR[op.rs]; return;

	case 0x110:
	case 0x111:
	case 0x112:
	case 0x113:
	case 0x114:
	case 0x115:
	case 0x116:
	case 0x117: CPU.SPRG[n - 0x110] = CPU.GPR[op.rs]; return;
	}

	throw __FUNCTION__;
}

void ppu_interpreter::DCBI(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::NAND(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = ~(CPU.GPR[op.rs] & CPU.GPR[op.rb]);

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::STVXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb]) & ~0xfull;
	vm::write128(vm::cast(addr), CPU.VPR[op.vs]);
}

void ppu_interpreter::DIVD(PPUThread& CPU, ppu_opcode_t op)
{
	const s64 RA = CPU.GPR[op.ra];
	const s64 RB = CPU.GPR[op.rb];

	if (RB == 0 || ((u64)RA == (1ULL << 63) && RB == -1))
	{
		if (op.oe) CPU.SetOV(true);
		CPU.GPR[op.rd] = /*(((u64)RA & (1ULL << 63)) && RB == 0) ? -1 :*/ 0;
	}
	else
	{
		if (op.oe) CPU.SetOV(false);
		CPU.GPR[op.rd] = RA / RB;
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::DIVW(PPUThread& CPU, ppu_opcode_t op)
{
	const s32 RA = (s32)CPU.GPR[op.ra];
	const s32 RB = (s32)CPU.GPR[op.rb];

	if (RB == 0 || ((u32)RA == (1 << 31) && RB == -1))
	{
		if (op.oe) CPU.SetOV(true);
		CPU.GPR[op.rd] = /*(((u32)RA & (1 << 31)) && RB == 0) ? -1 :*/ 0;
	}
	else
	{
		if (op.oe) CPU.SetOV(false);
		CPU.GPR[op.rd] = (u32)(RA / RB);
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.rd]);
}

void ppu_interpreter::LVLX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u32 eb = addr & 0xf;

	CPU.VPR[op.vd].clear();
	for (u32 i = 0; i < 16u - eb; ++i) CPU.VPR[op.vd]._u8[15 - i] = vm::read8(vm::cast(addr + i));
}

void ppu_interpreter::LDBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::get_ref<u64>(vm::cast(addr));
}

void ppu_interpreter::LSWX(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	u32 count = CPU.XER.XER & 0x7F;
	for (; count >= 4; count -= 4, addr += 4, op.rd = (op.rd + 1) & 31)
	{
		CPU.GPR[op.rd] = vm::get_ref<be_t<u32>>(vm::cast(addr));
	}
	if (count)
	{
		u32 value = 0;
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = vm::get_ref<u8>(vm::cast(addr + byte));
			value |= byte_value << ((3 ^ byte) * 8);
		}
		CPU.GPR[op.rd] = value;
	}
}

void ppu_interpreter::LWBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::get_ref<u32>(vm::cast(addr));
}

void ppu_interpreter::LFSX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
}

void ppu_interpreter::SRW(PPUThread& CPU, ppu_opcode_t op)
{
	u32 n = CPU.GPR[op.rb] & 0x1f;
	u32 r = (u32)rotl32((u32)CPU.GPR[op.rs], 64 - n);
	u32 m = ((u32)CPU.GPR[op.rb] & 0x20) ? 0 : (u32)rotate_mask[32 + n][63];
	CPU.GPR[op.ra] = r & m;

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::SRD(PPUThread& CPU, ppu_opcode_t op)
{
	u32 n = CPU.GPR[op.rb] & 0x3f;
	u64 r = rotl64(CPU.GPR[op.rs], 64 - n);
	u64 m = (CPU.GPR[op.rb] & 0x40) ? 0 : rotate_mask[n][63];
	CPU.GPR[op.ra] = r & m;

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::LVRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u8 eb = addr & 0xf;

	CPU.VPR[op.vd].clear();
	for (u32 i = 16 - eb; i < 16; ++i) CPU.VPR[op.vd]._u8[15 - i] = vm::read8(vm::cast(addr + i - 16));
}

void ppu_interpreter::LSWI(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			CPU.GPR[reg] = vm::read32(vm::cast(addr));
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = 0;
			u32 i = 3;
			while (N > 0)
			{
				N = N - 1;
				buf |= vm::read8(vm::cast(addr)) << (i * 8);
				addr++;
				i--;
			}
			CPU.GPR[reg] = buf;
		}
		reg = (reg + 1) % 32;
	}
}

void ppu_interpreter::LFSUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::SYNC(PPUThread& CPU, ppu_opcode_t op)
{
	_mm_mfence();
}

void ppu_interpreter::LFDX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
}

void ppu_interpreter::LFDUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STVLX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u32 eb = addr & 0xf;

	for (u32 i = 0; i < 16u - eb; ++i) vm::write8(vm::cast(addr + i), CPU.VPR[op.vs]._u8[15 - i]);
}

void ppu_interpreter::STDBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::get_ref<u64>(vm::cast(addr)) = CPU.GPR[op.rs];
}

void ppu_interpreter::STSWX(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	u32 count = CPU.XER.XER & 0x7F;
	for (; count >= 4; count -= 4, addr += 4, op.rs = (op.rs + 1) & 31)
	{
		vm::write32(vm::cast(addr), (u32)CPU.GPR[op.rs]);
	}
	if (count)
	{
		u32 value = (u32)CPU.GPR[op.rs];
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = (u8)(value >> ((3 ^ byte) * 8));
			vm::write8(vm::cast(addr + byte), byte_value);
		}
	}
}

void ppu_interpreter::STWBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::get_ref<u32>(vm::cast(addr)) = (u32)CPU.GPR[op.rs];
}

void ppu_interpreter::STFSX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::get_ref<be_t<float>>(vm::cast(addr)) = static_cast<float>(CPU.FPR[op.frs]);
}

void ppu_interpreter::STVRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u8 eb = addr & 0xf;

	for (u32 i = 16 - eb; i < 16; ++i) vm::write8(vm::cast(addr + i - 16), CPU.VPR[op.vs]._u8[15 - i]);
}

void ppu_interpreter::STFSUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::get_ref<be_t<float>>(vm::cast(addr)) = static_cast<float>(CPU.FPR[op.frs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STSWI(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			vm::write32(vm::cast(addr), (u32)CPU.GPR[reg]);
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = (u32)CPU.GPR[reg];
			while (N > 0)
			{
				N = N - 1;
				vm::write8(vm::cast(addr), (0xFF000000 & buf) >> 24);
				buf <<= 8;
				addr++;
			}
		}
		reg = (reg + 1) % 32;
	}
}

void ppu_interpreter::STFDX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[op.frs];
}

void ppu_interpreter::STFDUX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + CPU.GPR[op.rb];
	vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[op.frs];
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LVLXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u32 eb = addr & 0xf;

	CPU.VPR[op.vd].clear();
	for (u32 i = 0; i < 16u - eb; ++i) CPU.VPR[op.vd]._u8[15 - i] = vm::read8(vm::cast(addr + i));
}

void ppu_interpreter::LHBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	CPU.GPR[op.rd] = vm::get_ref<u16>(vm::cast(addr));
}

void ppu_interpreter::SRAW(PPUThread& CPU, ppu_opcode_t op)
{
	s32 RS = (s32)CPU.GPR[op.rs];
	u8 shift = CPU.GPR[op.rb] & 63;
	if (shift > 31)
	{
		CPU.GPR[op.ra] = 0 - (RS < 0);
		CPU.XER.CA = (RS < 0);
	}
	else
	{
		CPU.GPR[op.ra] = RS >> shift;
		CPU.XER.CA = (RS < 0) & ((CPU.GPR[op.ra] << shift) != RS);
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::SRAD(PPUThread& CPU, ppu_opcode_t op)
{
	s64 RS = CPU.GPR[op.rs];
	u8 shift = CPU.GPR[op.rb] & 127;
	if (shift > 63)
	{
		CPU.GPR[op.ra] = 0 - (RS < 0);
		CPU.XER.CA = (RS < 0);
	}
	else
	{
		CPU.GPR[op.ra] = RS >> shift;
		CPU.XER.CA = (RS < 0) & ((CPU.GPR[op.ra] << shift) != RS);
	}

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::LVRXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u8 eb = addr & 0xf;

	CPU.VPR[op.vd].clear();
	for (u32 i = 16 - eb; i < 16; ++i) CPU.VPR[op.vd]._u8[15 - i] = vm::read8(vm::cast(addr + i - 16));
}

void ppu_interpreter::DSS(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::SRAWI(PPUThread& CPU, ppu_opcode_t op)
{
	s32 RS = (u32)CPU.GPR[op.rs];
	CPU.GPR[op.ra] = RS >> op.sh;
	CPU.XER.CA = (RS < 0) & ((u32)(CPU.GPR[op.ra] << op.sh) != RS);

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::SRADI(PPUThread& CPU, ppu_opcode_t op)
{
	auto sh = (op.shh << 5) | op.shl;
	s64 RS = CPU.GPR[op.rs];
	CPU.GPR[op.ra] = RS >> sh;
	CPU.XER.CA = (RS < 0) & ((CPU.GPR[op.ra] << sh) != RS);

	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::EIEIO(PPUThread& CPU, ppu_opcode_t op)
{
	_mm_mfence();
}

void ppu_interpreter::STVLXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u32 eb = addr & 0xf;

	for (u32 i = 0; i < 16u - eb; ++i) vm::write8(vm::cast(addr + i), CPU.VPR[op.vs]._u8[15 - i]);
}

void ppu_interpreter::STHBRX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::get_ref<u16>(vm::cast(addr)) = (u16)CPU.GPR[op.rs];
}

void ppu_interpreter::EXTSH(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = (s64)(s16)CPU.GPR[op.rs];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::STVRXL(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	const u8 eb = addr & 0xf;

	for (u32 i = 16 - eb; i < 16; ++i) vm::write8(vm::cast(addr + i - 16), CPU.VPR[op.vs]._u8[15 - i]);
}

void ppu_interpreter::EXTSB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = (s64)(s8)CPU.GPR[op.rs];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::STFIWX(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];
	vm::write32(vm::cast(addr), (u32&)CPU.FPR[op.frs]);
}

void ppu_interpreter::EXTSW(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.GPR[op.ra] = (s64)(s32)CPU.GPR[op.rs];
	if (op.rc) CPU.UpdateCR0<s64>(CPU.GPR[op.ra]);
}

void ppu_interpreter::ICBI(PPUThread& CPU, ppu_opcode_t op)
{
}

void ppu_interpreter::DCBZ(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + CPU.GPR[op.rb] : CPU.GPR[op.rb];

	memset(vm::get_ptr<u8>(vm::cast(addr) & ~127), 0, 128);
}

void ppu_interpreter::LWZ(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.GPR[op.rd] = vm::read32(vm::cast(addr));
}

void ppu_interpreter::LWZU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.GPR[op.rd] = vm::read32(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LBZ(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.GPR[op.rd] = vm::read8(vm::cast(addr));
}

void ppu_interpreter::LBZU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.GPR[op.rd] = vm::read8(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STW(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write32(vm::cast(addr), (u32)CPU.GPR[op.rs]);
}

void ppu_interpreter::STWU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	vm::write32(vm::cast(addr), (u32)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STB(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write8(vm::cast(addr), (u8)CPU.GPR[op.rs]);
}

void ppu_interpreter::STBU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	vm::write8(vm::cast(addr), (u8)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LHZ(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.GPR[op.rd] = vm::read16(vm::cast(addr));
}

void ppu_interpreter::LHZU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.GPR[op.rd] = vm::read16(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LHA(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr));
}

void ppu_interpreter::LHAU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.GPR[op.rd] = (s64)(s16)vm::read16(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STH(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	vm::write16(vm::cast(addr), (u16)CPU.GPR[op.rs]);
}

void ppu_interpreter::STHU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	vm::write16(vm::cast(addr), (u16)CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LMW(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rd; i<32; ++i, addr += 4)
	{
		CPU.GPR[i] = vm::read32(vm::cast(addr));
	}
}

void ppu_interpreter::STMW(PPUThread& CPU, ppu_opcode_t op)
{
	u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rs; i<32; ++i, addr += 4)
	{
		vm::write32(vm::cast(addr), (u32)CPU.GPR[i]);
	}
}

void ppu_interpreter::LFS(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
}

void ppu_interpreter::LFSU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<float>>(vm::cast(addr)).value();
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LFD(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
}

void ppu_interpreter::LFDU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	CPU.FPR[op.frd]._double = vm::get_ref<be_t<double>>(vm::cast(addr)).value();
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STFS(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	vm::get_ref<be_t<float>>(vm::cast(addr)) = static_cast<float>(CPU.FPR[op.frs]);
}

void ppu_interpreter::STFSU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	vm::get_ref<be_t<float>>(vm::cast(addr)) = static_cast<float>(CPU.FPR[op.frs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::STFD(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = op.ra ? CPU.GPR[op.ra] + op.simm16 : op.simm16;
	vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[op.frs];
}

void ppu_interpreter::STFDU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + op.simm16;
	vm::get_ref<be_t<double>>(vm::cast(addr)) = CPU.FPR[op.frs];
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LD(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? CPU.GPR[op.ra] : 0);
	CPU.GPR[op.rd] = vm::read64(vm::cast(addr));
}

void ppu_interpreter::LDU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + (op.simm16 & ~3);
	CPU.GPR[op.rd] = vm::read64(vm::cast(addr));
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::LWA(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? CPU.GPR[op.ra] : 0);
	CPU.GPR[op.rd] = (s64)(s32)vm::read32(vm::cast(addr));
}

void ppu_interpreter::FDIVS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] / CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FADDS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FSQRTS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = sqrt(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FRES(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = 1.0 / CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMULS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMADDS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc] + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc] - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FNMSUBS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -(CPU.FPR[op.fra] * CPU.FPR[op.frc]) + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FNMADDS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -(CPU.FPR[op.fra] * CPU.FPR[op.frc]) - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::STD(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = (op.simm16 & ~3) + (op.ra ? CPU.GPR[op.ra] : 0);
	vm::write64(vm::cast(addr), CPU.GPR[op.rs]);
}

void ppu_interpreter::STDU(PPUThread& CPU, ppu_opcode_t op)
{
	const u64 addr = CPU.GPR[op.ra] + (op.simm16 & ~3);
	vm::write64(vm::cast(addr), CPU.GPR[op.rs]);
	CPU.GPR[op.ra] = addr;
}

void ppu_interpreter::MTFSB1(PPUThread& CPU, ppu_opcode_t op)
{
	u32 mask = 1 << (31 - op.crbd);
	if ((op.crbd >= 3 && op.crbd <= 6) && !(CPU.FPSCR.FPSCR & mask)) mask |= 1ULL << 31;  //FPSCR.FX
	if ((op.crbd == 29) && !CPU.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode enabled");
	CPU.SetFPSCR(CPU.FPSCR.FPSCR | mask);

	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::MCRFS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.SetCR(op.crfd, (CPU.FPSCR.FPSCR >> ((7 - op.crfs) * 4)) & 0xf);
	const u32 exceptions_mask = 0x9FF80700;
	CPU.SetFPSCR(CPU.FPSCR.FPSCR & ~(exceptions_mask & 0xf << ((7 - op.crfs) * 4)));
}

void ppu_interpreter::MTFSB0(PPUThread& CPU, ppu_opcode_t op)
{
	u32 mask = 1 << (31 - op.crbd);
	if ((op.crbd == 29) && !CPU.FPSCR.NI) LOG_WARNING(PPU, "Non-IEEE mode disabled");
	CPU.SetFPSCR(CPU.FPSCR.FPSCR & ~mask);

	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::MTFSFI(PPUThread& CPU, ppu_opcode_t op)
{
	u32 mask = 0xF0000000 >> (op.crfd * 4);
	u32 val = (op.i & 0xF) << ((7 - op.crfd) * 4);

	const u32 oldNI = CPU.FPSCR.NI;
	CPU.SetFPSCR((CPU.FPSCR.FPSCR & ~mask) | val);
	if (CPU.FPSCR.NI != oldNI)
	{
		if (oldNI)
			LOG_WARNING(PPU, "Non-IEEE mode disabled");
		else
			LOG_WARNING(PPU, "Non-IEEE mode enabled");
	}

	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::MFFS(PPUThread& CPU, ppu_opcode_t op)
{
	(u64&)CPU.FPR[op.frd]._double = CPU.FPSCR.FPSCR;
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::MTFSF(PPUThread& CPU, ppu_opcode_t op)
{
	u32 mask = 0;
	for (u32 i = 0; i<8; ++i)
	{
		if (op.flm & (1 << i)) mask |= 0xf << (i * 4);
	}
	mask &= ~0x60000000;

	const u32 oldNI = CPU.FPSCR.NI;
	CPU.SetFPSCR((CPU.FPSCR.FPSCR & ~mask) | ((u32&)CPU.FPR[op.frb] & mask));
	if (CPU.FPSCR.NI != oldNI)
	{
		if (oldNI)
			LOG_WARNING(PPU, "Non-IEEE mode disabled");
		else
			LOG_WARNING(PPU, "Non-IEEE mode enabled");
	}
	if (op.rc) CPU.UpdateCR1();
}


void ppu_interpreter::FCMPU(PPUThread& CPU, ppu_opcode_t op)
{
	s32 cmp_res = FPRdouble::Cmp(CPU.FPR[op.fra], CPU.FPR[op.frb]);
	//CPU.FPSCR.FPRF = cmp_res;
	CPU.SetCR(op.crfd, cmp_res);
}

void ppu_interpreter::FRSP(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = static_cast<float>(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCTIW(PPUThread& CPU, ppu_opcode_t op)
{
	(s32&)CPU.FPR[op.frd]._double = lrint(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCTIWZ(PPUThread& CPU, ppu_opcode_t op)
{
	(s32&)CPU.FPR[op.frd]._double = static_cast<s32>(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FDIV(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] / CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FSUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FADD(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FSQRT(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = sqrt(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FSEL(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] >= 0.0 ? CPU.FPR[op.frc] : CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMUL(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FRSQRTE(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = 1.0 / sqrt(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMSUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc] - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMADD(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.fra] * CPU.FPR[op.frc] + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FNMSUB(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -(CPU.FPR[op.fra] * CPU.FPR[op.frc]) + CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FNMADD(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -(CPU.FPR[op.fra] * CPU.FPR[op.frc]) - CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCMPO(PPUThread& CPU, ppu_opcode_t op)
{
	s32 cmp_res = FPRdouble::Cmp(CPU.FPR[op.fra], CPU.FPR[op.frb]);
	//CPU.FPSCR.FPRF = cmp_res;
	CPU.SetCR(op.crfd, cmp_res);
}

void ppu_interpreter::FNEG(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FMR(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = CPU.FPR[op.frb];
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FNABS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = -fabs(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FABS(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = fabs(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCTID(PPUThread& CPU, ppu_opcode_t op)
{
	(s64&)CPU.FPR[op.frd]._double = llrint(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCTIDZ(PPUThread& CPU, ppu_opcode_t op)
{
	(s64&)CPU.FPR[op.frd]._double = static_cast<s64>(CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}

void ppu_interpreter::FCFID(PPUThread& CPU, ppu_opcode_t op)
{
	CPU.FPR[op.frd]._double = static_cast<double>((s64&)CPU.FPR[op.frb]);
	if (op.rc) CPU.UpdateCR1();
}


void ppu_interpreter::UNK(PPUThread& CPU, ppu_opcode_t op)
{
	throw __FUNCTION__;
}
