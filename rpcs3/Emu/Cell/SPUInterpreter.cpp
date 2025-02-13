#include "stdafx.h"
#include "SPUInterpreter.h"

#include "Utilities/JIT.h"
#include "SPUThread.h"
#include "Emu/Cell/SPUAnalyser.h"
#include "Emu/system_config.h"

#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"

#include <cmath>

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

#if defined(ARCH_ARM64)
#if !defined(_MSC_VER)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
#undef FORCE_INLINE
#include "Emu/CPU/sse2neon.h"
#endif

const extern spu_decoder<spu_itype> g_spu_itype;
const extern spu_decoder<spu_iname> g_spu_iname;
const extern spu_decoder<spu_iflag> g_spu_iflag;

enum class spu_exec_bit : u64
{
	use_dfma,

	__bitset_enum_max
};

using enum spu_exec_bit;

template <spu_exec_bit... Flags0>
struct spu_exec_select
{
	template <spu_exec_bit Flag, spu_exec_bit... Flags, typename F>
	static spu_intrp_func_t select(bs_t<spu_exec_bit> selected, F func)
	{
		// Make sure there is no flag duplication, otherwise skip flag
		if constexpr (((Flags0 != Flag) && ...))
		{
			// Test only relevant flags at runtime (compiling both variants)
			if (selected & Flag)
			{
				// In this branch, selected flag is added to Flags0
				return spu_exec_select<Flags0..., Flag>::template select<Flags...>(selected, func);
			}
		}

		return spu_exec_select<Flags0...>::template select<Flags...>(selected, func);
	}

	template <typename F>
	static spu_intrp_func_t select(bs_t<spu_exec_bit>, F func)
	{
		// Instantiate interpreter function with required set of flags
		return func.template operator()<Flags0...>();
	}
};

#ifdef ARCH_X64
static constexpr spu_opcode_t s_op{};
#endif

namespace asmjit
{
	template <uint I, uint N>
	static void build_spu_gpr_load(x86::Assembler& c, x86::Xmm x, const bf_t<u32, I, N>&, bool store = false)
	{
		static_assert(N == 7, "Invalid bitfield");

#ifdef _WIN32
		const auto& spu = x86::rcx;
		const auto& op = x86::edx;
#else
		const auto& spu = x86::rdi;
		const auto& op = x86::esi;
#endif

		c.mov(x86::eax, op);

		if constexpr (I >= 4)
		{
			c.shr(x86::eax, I - 4);
			c.and_(x86::eax, 0x7f << 4);
		}
		else
		{
			c.and_(x86::eax, 0x7f);
			c.shl(x86::eax, I + 4);
		}

		const auto ptr = x86::oword_ptr(spu, x86::rax, 0, ::offset32(&spu_thread::gpr));

		if (utils::has_avx())
		{
			if (store)
				c.vmovdqa(ptr, x);
			else
				c.vmovdqa(x, ptr);
		}
		else
		{
			if (store)
				c.movdqa(ptr, x);
			else
				c.movdqa(x, ptr);
		}
	}

	template <uint I, uint N>
	static void build_spu_gpr_store(x86::Assembler& c, x86::Xmm x, const bf_t<u32, I, N>&, bool store = true)
	{
		build_spu_gpr_load(c, x, bf_t<u32, I, N>{}, store);
	}
}

template <spu_exec_bit... Flags>
bool UNK(spu_thread&, spu_opcode_t op)
{
	spu_log.fatal("Unknown/Illegal instruction (0x%08x)", op.opcode);
	return false;
}


void spu_interpreter::set_interrupt_status(spu_thread& spu, spu_opcode_t op)
{
	if (op.e)
	{
		if (op.d)
		{
			fmt::throw_exception("Undefined behaviour");
		}

		spu.set_interrupt_status(true);
	}
	else if (op.d)
	{
		spu.set_interrupt_status(false);
	}

	if (spu.check_mfc_interrupts(spu.pc) && spu.state & cpu_flag::pending)
	{
		spu.do_mfc();
	}
}


template <spu_exec_bit... Flags>
bool STOP(spu_thread& spu, spu_opcode_t op)
{
	const bool allow = std::exchange(spu.allow_interrupts_in_cpu_work, false);

	const bool advance_pc = spu.stop_and_signal(op.opcode & 0x3fff);

	spu.allow_interrupts_in_cpu_work = allow;

	if (!advance_pc)
	{
		return false;
	}

	if (spu.state)
	{
		spu.pc += 4;
		return false;
	}

	return true;
}

template <spu_exec_bit... Flags>
bool LNOP(spu_thread&, spu_opcode_t)
{
	return true;
}

// This instruction must be used following a store instruction that modifies the instruction stream.
template <spu_exec_bit... Flags>
bool SYNC(spu_thread&, spu_opcode_t)
{
	atomic_fence_seq_cst();
	return true;
}

// This instruction forces all earlier load, store, and channel instructions to complete before proceeding.
template <spu_exec_bit... Flags>
bool DSYNC(spu_thread&, spu_opcode_t)
{
	atomic_fence_seq_cst();
	return true;
}

template <spu_exec_bit... Flags>
bool MFSPR(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].clear(); // All SPRs read as zero. TODO: check it.
	return true;
}

template <spu_exec_bit... Flags>
bool RDCH(spu_thread& spu, spu_opcode_t op)
{
	const bool allow = std::exchange(spu.allow_interrupts_in_cpu_work, false);

	const s64 result = spu.get_ch_value(op.ra);

	spu.allow_interrupts_in_cpu_work = allow;

	if (result < 0)
	{
		return false;
	}

	spu.gpr[op.rt] = v128::from32r(static_cast<u32>(result));

	if (spu.state)
	{
		spu.pc += 4;
		return false;
	}

	return true;
}

template <spu_exec_bit... Flags>
bool RCHCNT(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(spu.get_ch_count(op.ra));
	return true;
}

template <spu_exec_bit... Flags>
bool SF(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_sub32(spu.gpr[op.rb], spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool OR(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] | spu.gpr[op.rb];
	return true;
}

template <spu_exec_bit... Flags>
bool BG(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_add_epi32(gv_gtu32(spu.gpr[op.ra], spu.gpr[op.rb]), _mm_set1_epi32(1));
	return true;
}

template <spu_exec_bit... Flags>
bool SFH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_sub16(spu.gpr[op.rb], spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool NOR(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] | spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool ABSDB(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	spu.gpr[op.rt] = gv_sub8(gv_maxu8(a, b), gv_minu8(a, b));
	return true;
}

template <spu_exec_bit... Flags>
bool ROT(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		spu.gpr[op.rt]._u32[i] = utils::rol32(a._u32[i], b._u32[i]);
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTM(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		spu.gpr[op.rt]._u32[i] = static_cast<u32>(value >> ((0 - b._u32[i]) & 0x3f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTMA(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const s64 value = a._s32[i];
		spu.gpr[op.rt]._s32[i] = static_cast<s32>(value >> ((0 - b._u32[i]) & 0x3f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool SHL(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 4; i++)
	{
		const u64 value = a._u32[i];
		spu.gpr[op.rt]._u32[i] = static_cast<u32>(value << (b._u32[i] & 0x3f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTH(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		spu.gpr[op.rt]._u16[i] = utils::rol16(a._u16[i], b._u16[i]);
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTHM(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		spu.gpr[op.rt]._u16[i] = static_cast<u16>(value >> ((0 - b._u16[i]) & 0x1f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTMAH(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const s32 value = a._s16[i];
		spu.gpr[op.rt]._s16[i] = static_cast<s16>(value >> ((0 - b._u16[i]) & 0x1f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool SHLH(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];

	for (u32 i = 0; i < 8; i++)
	{
		const u32 value = a._u16[i];
		spu.gpr[op.rt]._u16[i] = static_cast<u16>(value << (b._u16[i] & 0x1f));
	}
	return true;
}

template <spu_exec_bit... Flags>
bool ROTI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = op.i7 & 0x1f;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi32(a, n), _mm_srli_epi32(a, 32 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTMI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srli_epi32(spu.gpr[op.ra], (0-op.i7) & 0x3f);
	return true;
}

template <spu_exec_bit... Flags>
bool ROTMAI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srai_epi32(spu.gpr[op.ra], (0-op.i7) & 0x3f);
	return true;
}

template <spu_exec_bit... Flags>
bool SHLI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_slli_epi32(spu.gpr[op.ra], op.i7 & 0x3f);
	return true;
}

template <spu_exec_bit... Flags>
bool ROTHI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = op.i7 & 0xf;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi16(a, n), _mm_srli_epi16(a, 16 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTHMI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srli_epi16(spu.gpr[op.ra], (0-op.i7) & 0x1f);
	return true;
}

template <spu_exec_bit... Flags>
bool ROTMAHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srai_epi16(spu.gpr[op.ra], (0-op.i7) & 0x1f);
	return true;
}

template <spu_exec_bit... Flags>
bool SHLHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_slli_epi16(spu.gpr[op.ra], op.i7 & 0x1f);
	return true;
}

template <spu_exec_bit... Flags>
bool A(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_add32(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool AND(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] & spu.gpr[op.rb];
	return true;
}

template <spu_exec_bit... Flags>
bool CG(spu_thread& spu, spu_opcode_t op)
{
	const auto a = _mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi32(0x7fffffff));
	const auto b = _mm_xor_si128(spu.gpr[op.rb], _mm_set1_epi32(0x80000000));
	spu.gpr[op.rt] = _mm_srli_epi32(_mm_cmpgt_epi32(b, a), 31);
	return true;
}

template <spu_exec_bit... Flags>
bool AH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_add16(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool NAND(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] & spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool AVGB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_avg_epu8(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool MTSPR(spu_thread&, spu_opcode_t)
{
	// SPR writes are ignored. TODO: check it.
	return true;
}

template <spu_exec_bit... Flags>
bool WRCH(spu_thread& spu, spu_opcode_t op)
{
	const bool allow = std::exchange(spu.allow_interrupts_in_cpu_work, false);

	const bool advance_pc = spu.set_ch_value(op.ra, spu.gpr[op.rt]._u32[3]);

	spu.allow_interrupts_in_cpu_work = allow;

	if (!advance_pc)
	{
		return false;
	}

	if (spu.state)
	{
		spu.pc += 4;
		return false;
	}

	return true;
}

template <spu_exec_bit... Flags>
bool BIZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] == 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]);
		spu_interpreter::set_interrupt_status(spu, op);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BINZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] != 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]);
		spu_interpreter::set_interrupt_status(spu, op);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BIHZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] == 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]);
		spu_interpreter::set_interrupt_status(spu, op);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BIHNZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] != 0)
	{
		spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]);
		spu_interpreter::set_interrupt_status(spu, op);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool STOPD(spu_thread& spu, spu_opcode_t)
{
	return spu.stop_and_signal(0x3fff);
}

template <spu_exec_bit... Flags>
bool STQX(spu_thread& spu, spu_opcode_t op)
{
	spu._ref<v128>((spu.gpr[op.ra]._u32[3] + spu.gpr[op.rb]._u32[3]) & 0x3fff0) = spu.gpr[op.rt];
	return true;
}

template <spu_exec_bit... Flags>
bool BI(spu_thread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(spu.gpr[op.ra]._u32[3]);
	spu_interpreter::set_interrupt_status(spu, op);
	return false;
}

template <spu_exec_bit... Flags>
bool BISL(spu_thread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(spu.gpr[op.ra]._u32[3]);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target;
	spu_interpreter::set_interrupt_status(spu, op);
	return false;
}

template <spu_exec_bit... Flags>
bool IRET(spu_thread& spu, spu_opcode_t op)
{
	spu.pc = spu.srr0;
	spu_interpreter::set_interrupt_status(spu, op);
	return false;
}

template <spu_exec_bit... Flags>
bool BISLED(spu_thread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(spu.gpr[op.ra]._u32[3]);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));

	if (spu.get_events().count)
	{
		spu.pc = target;
		spu_interpreter::set_interrupt_status(spu, op);
		return false;
	}

	return true;
}

template <spu_exec_bit... Flags>
bool HBR(spu_thread&, spu_opcode_t)
{
	return true;
}

template <spu_exec_bit... Flags>
bool GB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_ps(_mm_castsi128_ps(_mm_slli_epi32(spu.gpr[op.ra], 31))));
	return true;
}

template <spu_exec_bit... Flags>
bool GBH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_epi8(_mm_packs_epi16(_mm_slli_epi16(spu.gpr[op.ra], 15), _mm_setzero_si128())));
	return true;
}

template <spu_exec_bit... Flags>
bool GBB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(_mm_movemask_epi8(_mm_slli_epi64(spu.gpr[op.ra], 7)));
	return true;
}

template <spu_exec_bit... Flags>
bool FSM(spu_thread& spu, spu_opcode_t op)
{
	const auto bits = _mm_shuffle_epi32(spu.gpr[op.ra], 0xff);
	const auto mask = _mm_set_epi32(8, 4, 2, 1);
	spu.gpr[op.rt] = _mm_cmpeq_epi32(_mm_and_si128(bits, mask), mask);
	return true;
}

template <spu_exec_bit... Flags>
bool FSMH(spu_thread& spu, spu_opcode_t op)
{
	const auto vsrc = spu.gpr[op.ra];
	const auto bits = _mm_shuffle_epi32(_mm_unpackhi_epi16(vsrc, vsrc), 0xaa);
	const auto mask = _mm_set_epi16(128, 64, 32, 16, 8, 4, 2, 1);
	spu.gpr[op.rt] = _mm_cmpeq_epi16(_mm_and_si128(bits, mask), mask);
	return true;
}

template <spu_exec_bit... Flags>
bool FSMB(spu_thread& spu, spu_opcode_t op)
{
	const auto vsrc = spu.gpr[op.ra];
	const auto bits = _mm_shuffle_epi32(_mm_shufflehi_epi16(_mm_unpackhi_epi8(vsrc, vsrc), 0x50), 0xfa);
	const auto mask = _mm_set_epi8(-128, 64, 32, 16, 8, 4, 2, 1, -128, 64, 32, 16, 8, 4, 2, 1);
	spu.gpr[op.rt] = _mm_cmpeq_epi8(_mm_and_si128(bits, mask), mask);
	return true;
}

template <spu_exec_bit... Flags>
bool FREST(spu_thread& spu, spu_opcode_t op)
{
	v128 fraction_index = v128(_mm_srli_epi32(spu.gpr[op.ra], 18)) & v128(_mm_set1_epi32(0x1F));
	v128 exponent_index = v128(_mm_srli_epi32(spu.gpr[op.ra], 23)) & v128(_mm_set1_epi32(0xFF));
	v128 sign = spu.gpr[op.ra] & _mm_set1_epi32(0x80000000);

	// AVX2
	// v128 fraction = _mm_i32gather_epi32(spu_frest_fraction_lut, fraction_index, 4);
	// v128 exponent = _mm_i32gather_epi32(spu_frest_exponent_lut, exponent_index, 4);

	v128 result;

	for (u32 index = 0; index < 4; index++)
	{
		u32 r = spu_frest_fraction_lut[fraction_index._u32[index]];
		r |= spu_frest_exponent_lut[exponent_index._u32[index]];
		r |= sign._u32[index];
		result._u32[index] = r;
	}

	spu.gpr[op.rt] = result;
	return true;
}

template <spu_exec_bit... Flags>
bool FRSQEST(spu_thread& spu, spu_opcode_t op)
{
	v128 fraction_index = v128(_mm_srli_epi32(spu.gpr[op.ra], 18)) & v128(_mm_set1_epi32(0x3F));
	v128 exponent_index = v128(_mm_srli_epi32(spu.gpr[op.ra], 23)) & v128(_mm_set1_epi32(0xFF));

	// AVX2
	// v128 fraction = _mm_i32gather_epi32(spu_frsqest_fraction_lut, fraction_index, 4);
	// v128 exponent = _mm_i32gather_epi32(spu_frsqest_exponent_lut, exponent_index, 4);

	v128 result;

	for (u32 index = 0; index < 4; index++)
	{
		u32 r = spu_frsqest_fraction_lut[fraction_index._u32[index]];
		r |= spu_frsqest_exponent_lut[exponent_index._u32[index]];
		result._u32[index] = r;
	}

	spu.gpr[op.rt] = result;
	return true;
}

template <spu_exec_bit... Flags>
bool LQX(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>((spu.gpr[op.ra]._u32[3] + spu.gpr[op.rb]._u32[3]) & 0x3fff0);
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQBYBI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(32) const __m128i buf[2]{a, a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (16 - (spu.gpr[op.rb]._u32[3] >> 3 & 0xf))));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQMBYBI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{a, _mm_setzero_si128(), _mm_setzero_si128()};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + ((0 - (spu.gpr[op.rb]._u32[3] >> 3)) & 0x1f)));
	return true;
}

template <spu_exec_bit... Flags>
bool SHLQBYBI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{_mm_setzero_si128(), _mm_setzero_si128(), a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (32 - (spu.gpr[op.rb]._u32[3] >> 3 & 0x1f))));
	return true;
}

template <spu_exec_bit... Flags>
bool CBX(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = ~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xf;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u8[t] = 0x03;
	return true;
}

template <spu_exec_bit... Flags>
bool CHX(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xe) >> 1;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u16[t] = 0x0203;
	return true;
}

template <spu_exec_bit... Flags>
bool CWX(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0xc) >> 2;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u32[t] = 0x00010203;
	return true;
}

template <spu_exec_bit... Flags>
bool CDX(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(spu.gpr[op.rb]._u32[3] + spu.gpr[op.ra]._u32[3]) & 0x8) >> 3;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u64[t] = 0x0001020304050607ull;
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQBI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = spu.gpr[op.rb]._s32[3] & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_shuffle_epi32(a, 0x4E), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQMBI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = -spu.gpr[op.rb]._s32[3] & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_srli_epi64(a, n), _mm_slli_epi64(_mm_srli_si128(a, 8), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool SHLQBI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = spu.gpr[op.rb]._u32[3] & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_slli_si128(a, 8), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQBY(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(32) const __m128i buf[2]{a, a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (16 - (spu.gpr[op.rb]._u32[3] & 0xf))));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQMBY(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{a, _mm_setzero_si128(), _mm_setzero_si128()};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + ((0 - spu.gpr[op.rb]._u32[3]) & 0x1f)));
	return true;
}

template <spu_exec_bit... Flags>
bool SHLQBY(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{_mm_setzero_si128(), _mm_setzero_si128(), a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (32 - (spu.gpr[op.rb]._u32[3] & 0x1f))));
	return true;
}

template <spu_exec_bit... Flags>
bool ORX(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = v128::from32r(spu.gpr[op.ra]._u32[0] | spu.gpr[op.ra]._u32[1] | spu.gpr[op.ra]._u32[2] | spu.gpr[op.ra]._u32[3]);
	return true;
}

template <spu_exec_bit... Flags>
bool CBD(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = ~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xf;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u8[t] = 0x03;
	return true;
}

template <spu_exec_bit... Flags>
bool CHD(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xe) >> 1;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u16[t] = 0x0203;
	return true;
}

template <spu_exec_bit... Flags>
bool CWD(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0xc) >> 2;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u32[t] = 0x00010203;
	return true;
}

template <spu_exec_bit... Flags>
bool CDD(spu_thread& spu, spu_opcode_t op)
{
	if (op.ra == 1 && (spu.gpr[1]._u32[3] & 0xF))
	{
		fmt::throw_exception("Unexpected SP value: LS:0x%05x", spu.gpr[1]._u32[3]);
	}

	const s32 t = (~(op.i7 + spu.gpr[op.ra]._u32[3]) & 0x8) >> 3;
	spu.gpr[op.rt] = v128::from64(0x18191A1B1C1D1E1Full, 0x1011121314151617ull);
	spu.gpr[op.rt]._u64[t] = 0x0001020304050607ull;
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQBII(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = op.i7 & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_shuffle_epi32(a, 0x4E), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQMBII(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = (0-op.i7) & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_srli_epi64(a, n), _mm_slli_epi64(_mm_srli_si128(a, 8), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool SHLQBII(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const s32 n = op.i7 & 0x7;
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi64(a, n), _mm_srli_epi64(_mm_slli_si128(a, 8), 64 - n));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQBYI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(32) const __m128i buf[2]{a, a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (16 - (op.i7 & 0xf))));
	return true;
}

template <spu_exec_bit... Flags>
bool ROTQMBYI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{a, _mm_setzero_si128(), _mm_setzero_si128()};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + ((0 - op.i7) & 0x1f)));
	return true;
}

template <spu_exec_bit... Flags>
bool SHLQBYI(spu_thread& spu, spu_opcode_t op)
{
	const __m128i a = spu.gpr[op.ra];
	alignas(64) const __m128i buf[3]{_mm_setzero_si128(), _mm_setzero_si128(), a};
	spu.gpr[op.rt] = _mm_loadu_si128(reinterpret_cast<const __m128i*>(reinterpret_cast<const u8*>(buf) + (32 - (op.i7 & 0x1f))));
	return true;
}

template <spu_exec_bit... Flags>
bool NOP(spu_thread&, spu_opcode_t)
{
	return true;
}

template <spu_exec_bit... Flags>
bool CGT(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi32(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool XOR(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] ^ spu.gpr[op.rb];
	return true;
}

template <spu_exec_bit... Flags>
bool CGTH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi16(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool EQV(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = ~(spu.gpr[op.ra] ^ spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool CGTB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi8(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool SUMB(spu_thread& spu, spu_opcode_t op)
{
	const auto m1 = _mm_set1_epi16(0xff);
	const auto m2 = _mm_set1_epi32(0xffff);
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	const auto a1 = _mm_srli_epi16(a, 8);
	const auto a2 = _mm_and_si128(a, m1);
	const auto b1 = _mm_srli_epi16(b, 8);
	const auto b2 = _mm_and_si128(b, m1);
	const auto sa = _mm_add_epi16(a1, a2);
	const auto sb = _mm_add_epi16(b1, b2);
	const auto s2 = _mm_and_si128(sa, m2);
	const auto s1 = _mm_srli_epi32(sa, 16);
	const auto s4 = _mm_andnot_si128(m2, sb);
	const auto s3 = _mm_slli_epi32(sb, 16);
	spu.gpr[op.rt] = _mm_or_si128(_mm_add_epi16(s1, s2), _mm_add_epi16(s3, s4));
	return true;
}

template <spu_exec_bit... Flags>
bool HGT(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] > spu.gpr[op.rb]._s32[3])
	{
		spu.halt();
	}
	return true;
}

template <spu_exec_bit... Flags>
bool CLZ(spu_thread& spu, spu_opcode_t op)
{
	for (u32 i = 0; i < 4; i++)
	{
		spu.gpr[op.rt]._u32[i] = std::countl_zero(spu.gpr[op.ra]._u32[i]);
	}
	return true;
}

template <spu_exec_bit... Flags>
bool XSWD(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt]._s64[0] = spu.gpr[op.ra]._s32[0];
	spu.gpr[op.rt]._s64[1] = spu.gpr[op.ra]._s32[2];
	return true;
}

template <spu_exec_bit... Flags>
bool XSHW(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srai_epi32(_mm_slli_epi32(spu.gpr[op.ra], 16), 16);
	return true;
}

template <spu_exec_bit... Flags>
bool CNTB(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto mask1 = _mm_set1_epi8(0x55);
	const auto sum1 = _mm_add_epi8(_mm_and_si128(_mm_srli_epi64(a, 1), mask1), _mm_and_si128(a, mask1));
	const auto mask2 = _mm_set1_epi8(0x33);
	const auto sum2 = _mm_add_epi8(_mm_and_si128(_mm_srli_epi64(sum1, 2), mask2), _mm_and_si128(sum1, mask2));
	const auto mask3 = _mm_set1_epi8(0x0f);
	const auto sum3 = _mm_add_epi8(_mm_and_si128(_mm_srli_epi64(sum2, 4), mask3), _mm_and_si128(sum2, mask3));
	spu.gpr[op.rt] = sum3;
	return true;
}

template <spu_exec_bit... Flags>
bool XSBH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srai_epi16(_mm_slli_epi16(spu.gpr[op.ra], 8), 8);
	return true;
}

template <spu_exec_bit... Flags>
bool CLGT(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_gtu32(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool ANDC(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_andn(spu.gpr[op.rb], spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool FCGT(spu_thread& spu, spu_opcode_t op)
{
	// IMPL NOTES:
	// if (v is inf) v = (inf - 1) i.e nearest normal value to inf with mantissa bits left intact
	// if (v is denormalized) v = 0 flush denormals
	// return v1 > v2
	// branching simulated using bitwise ops and_not+or

	const auto zero = _mm_set1_ps(0.f);
	const auto nan_check_a = _mm_cmpunord_ps(spu.gpr[op.ra], zero);    //mask true where a is extended
	const auto nan_check_b = _mm_cmpunord_ps(spu.gpr[op.rb], zero);    //mask true where b is extended

	//calculate lowered a and b. The mantissa bits are left untouched for now unless its proven they should be flushed
	const auto last_exp_bit = _mm_castsi128_ps(_mm_set1_epi32(0x00800000));
	const auto lowered_a =_mm_andnot_ps(last_exp_bit, spu.gpr[op.ra]);      //a is lowered to largest unextended value with sign
	const auto lowered_b = _mm_andnot_ps(last_exp_bit, spu.gpr[op.rb]);		//b is lowered to largest unextended value with sign

	//check if a and b are denormalized
	const auto all_exp_bits = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
	const auto denorm_check_a = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.ra]));
	const auto denorm_check_b = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.rb]));

	//set a and b to their lowered values if they are extended
	const auto a_values_lowered = _mm_and_ps(nan_check_a, lowered_a);
	const auto original_a_masked = _mm_andnot_ps(nan_check_a, spu.gpr[op.ra]);
	const auto a_final1 = _mm_or_ps(a_values_lowered, original_a_masked);

	const auto b_values_lowered = _mm_and_ps(nan_check_b, lowered_b);
	const auto original_b_masked = _mm_andnot_ps(nan_check_b, spu.gpr[op.rb]);
	const auto b_final1 = _mm_or_ps(b_values_lowered, original_b_masked);

	//Flush denormals to zero
	const auto final_a = _mm_andnot_ps(denorm_check_a, a_final1);
	const auto final_b = _mm_andnot_ps(denorm_check_b, b_final1);

	spu.gpr[op.rt] = _mm_cmplt_ps(final_b, final_a);
	return true;
}

template <spu_exec_bit... Flags>
bool DFCGT(spu_thread&, spu_opcode_t)
{
	spu_log.fatal("DFCGT");
	return false;
}

template <spu_exec_bit... Flags>
bool FA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_addfs(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool FS(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_subfs(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool FM(spu_thread& spu, spu_opcode_t op)
{
	const auto zero = _mm_set1_ps(0.f);
	const auto sign_bits = _mm_castsi128_ps(_mm_set1_epi32(0x80000000));
	const auto all_exp_bits = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));

	//check denormals
	const auto denorm_check_a = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.ra]));
	const auto denorm_check_b = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.rb]));
	const auto denorm_operand_mask = _mm_or_ps(denorm_check_a, denorm_check_b);

	//compute result with flushed denormal inputs
	const auto primary_result = _mm_mul_ps(spu.gpr[op.ra], spu.gpr[op.rb]);
	const auto denom_result_mask = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, primary_result));
	const auto flushed_result = _mm_andnot_ps(_mm_or_ps(denom_result_mask, denorm_operand_mask), primary_result);

	//check for extended
	const auto nan_check = _mm_cmpeq_ps(_mm_and_ps(primary_result, all_exp_bits), all_exp_bits);
	const auto sign_mask = _mm_xor_ps(_mm_and_ps(sign_bits, spu.gpr[op.ra]), _mm_and_ps(sign_bits, spu.gpr[op.rb]));
	const auto extended_result = _mm_or_ps(sign_mask, _mm_andnot_ps(sign_bits, primary_result));
	const auto final_extended = _mm_andnot_ps(denorm_operand_mask, extended_result);

	//if nan, result = ext, else result = flushed
	const auto set1 = _mm_andnot_ps(nan_check, flushed_result);
	const auto set2 = _mm_and_ps(nan_check, final_extended);

	spu.gpr[op.rt] = _mm_or_ps(set1, set2);
	return true;
}

template <spu_exec_bit... Flags>
bool CLGTH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_gtu16(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool ORC(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu.gpr[op.ra] | ~spu.gpr[op.rb];
	return true;
}

template <spu_exec_bit... Flags>
bool FCMGT(spu_thread& spu, spu_opcode_t op)
{
	//IMPL NOTES: See FCGT

	const auto zero = _mm_set1_ps(0.f);
	const auto nan_check_a = _mm_cmpunord_ps(spu.gpr[op.ra], zero);    //mask true where a is extended
	const auto nan_check_b = _mm_cmpunord_ps(spu.gpr[op.rb], zero);    //mask true where b is extended

	//check if a and b are denormalized
	const auto all_exp_bits = _mm_castsi128_ps(_mm_set1_epi32(0x7f800000));
	const auto denorm_check_a = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.ra]));
	const auto denorm_check_b = _mm_cmpeq_ps(zero, _mm_and_ps(all_exp_bits, spu.gpr[op.rb]));

	//Flush denormals to zero
	const auto final_a = _mm_andnot_ps(denorm_check_a, spu.gpr[op.ra]);
	const auto final_b = _mm_andnot_ps(denorm_check_b, spu.gpr[op.rb]);

	//Mask to make a > b if a is extended but b is not (is this necessary on x86?)
	const auto nan_mask = _mm_andnot_ps(nan_check_b, _mm_xor_ps(nan_check_a, nan_check_b));

	const auto sign_mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	const auto comparison = _mm_cmplt_ps(_mm_and_ps(final_b, sign_mask), _mm_and_ps(final_a, sign_mask));

	spu.gpr[op.rt] = _mm_or_ps(comparison, nan_mask);
	return true;
}

template <spu_exec_bit... Flags>
bool DFCMGT(spu_thread&, spu_opcode_t)
{
	spu_log.fatal("DFCMGT");
	return false;
}

template <spu_exec_bit... Flags>
bool DFA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_addfd(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool DFS(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_subfd(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool DFM(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_mul_pd(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool CLGTB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_gtu8(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool HLGT(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._u32[3] > spu.gpr[op.rb]._u32[3])
	{
		spu.halt();
	}
	return true;
}

template <spu_exec_bit... Flags>
bool DFMA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_add_pd(_mm_mul_pd(spu.gpr[op.ra], spu.gpr[op.rb]), spu.gpr[op.rt]);
	return true;
}

template <spu_exec_bit... Flags>
bool DFMS(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_sub_pd(_mm_mul_pd(spu.gpr[op.ra], spu.gpr[op.rb]), spu.gpr[op.rt]);
	return true;
}

template <spu_exec_bit... Flags>
bool DFNMS(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_sub_pd(spu.gpr[op.rt], _mm_mul_pd(spu.gpr[op.ra], spu.gpr[op.rb]));
	return true;
}

template <spu_exec_bit... Flags>
bool DFNMA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_xor_pd(_mm_add_pd(_mm_mul_pd(spu.gpr[op.ra], spu.gpr[op.rb]), spu.gpr[op.rt]), _mm_set1_pd(-0.0));
	return true;
}

template <spu_exec_bit... Flags>
bool CEQ(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi32(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool MPYHHU(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	spu.gpr[op.rt] = _mm_or_si128(_mm_srli_epi32(_mm_mullo_epi16(a, b), 16), _mm_and_si128(_mm_mulhi_epu16(a, b), _mm_set1_epi32(0xffff0000)));
	return true;
}

template <spu_exec_bit... Flags>
bool ADDX(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_add32(gv_add32(spu.gpr[op.ra], spu.gpr[op.rb]), spu.gpr[op.rt] & v128::from32p(1));
	return true;
}

template <spu_exec_bit... Flags>
bool SFX(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = gv_sub32(gv_sub32(spu.gpr[op.rb], spu.gpr[op.ra]), gv_andn(spu.gpr[op.rt], v128::from32p(1)));
	return true;
}

template <spu_exec_bit... Flags>
bool CGX(spu_thread& spu, spu_opcode_t op)
{
	for (s32 i = 0; i < 4; i++)
	{
		const u64 carry = spu.gpr[op.rt]._u32[i] & 1;
		spu.gpr[op.rt]._u32[i] = (carry + spu.gpr[op.ra]._u32[i] + spu.gpr[op.rb]._u32[i]) >> 32;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BGX(spu_thread& spu, spu_opcode_t op)
{
	for (s32 i = 0; i < 4; i++)
	{
		const s64 result = u64{spu.gpr[op.rb]._u32[i]} - spu.gpr[op.ra]._u32[i] - (1 - (spu.gpr[op.rt]._u32[i] & 1));
		spu.gpr[op.rt]._u32[i] = result >= 0;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool MPYHHA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_add_epi32(spu.gpr[op.rt], _mm_madd_epi16(_mm_srli_epi32(spu.gpr[op.ra], 16), _mm_srli_epi32(spu.gpr[op.rb], 16)));
	return true;
}

template <spu_exec_bit... Flags>
bool MPYHHAU(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	spu.gpr[op.rt] = _mm_add_epi32(spu.gpr[op.rt], _mm_or_si128(_mm_srli_epi32(_mm_mullo_epi16(a, b), 16), _mm_and_si128(_mm_mulhi_epu16(a, b), _mm_set1_epi32(0xffff0000))));
	return true;
}

template <spu_exec_bit... Flags>
bool FSCRRD(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt].clear();
	return true;
}

template <spu_exec_bit... Flags>
bool FESD(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	spu.gpr[op.rt] = _mm_cvtps_pd(_mm_shuffle_ps(a, a, 0x8d));
	return true;
}

template <spu_exec_bit... Flags>
bool FRDS(spu_thread& spu, spu_opcode_t op)
{
	const auto t = _mm_cvtpd_ps(spu.gpr[op.ra]);
	spu.gpr[op.rt] = _mm_shuffle_ps(t, t, 0x72);
	return true;
}

template <spu_exec_bit... Flags>
bool FSCRWR(spu_thread&, spu_opcode_t)
{
	return true;
}

template <spu_exec_bit... Flags>
bool DFTSV(spu_thread&, spu_opcode_t)
{
	spu_log.fatal("DFTSV");
	return false;
}

template <spu_exec_bit... Flags>
bool FCEQ(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_ps(spu.gpr[op.rb], spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool DFCEQ(spu_thread&, spu_opcode_t)
{
	spu_log.fatal("DFCEQ");
	return false;
}

template <spu_exec_bit... Flags>
bool MPY(spu_thread& spu, spu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0xffff);
	spu.gpr[op.rt] = _mm_madd_epi16(_mm_and_si128(spu.gpr[op.ra], mask), _mm_and_si128(spu.gpr[op.rb], mask));
	return true;
}

template <spu_exec_bit... Flags>
bool MPYH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_slli_epi32(_mm_mullo_epi16(_mm_srli_epi32(spu.gpr[op.ra], 16), spu.gpr[op.rb]), 16);
	return true;
}

template <spu_exec_bit... Flags>
bool MPYHH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_madd_epi16(_mm_srli_epi32(spu.gpr[op.ra], 16), _mm_srli_epi32(spu.gpr[op.rb], 16));
	return true;
}

template <spu_exec_bit... Flags>
bool MPYS(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_srai_epi32(_mm_slli_epi32(_mm_mulhi_epi16(spu.gpr[op.ra], spu.gpr[op.rb]), 16), 16);
	return true;
}

template <spu_exec_bit... Flags>
bool CEQH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi16(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool FCMEQ(spu_thread& spu, spu_opcode_t op)
{
	const auto mask = _mm_castsi128_ps(_mm_set1_epi32(0x7fffffff));
	spu.gpr[op.rt] = _mm_cmpeq_ps(_mm_and_ps(spu.gpr[op.rb], mask), _mm_and_ps(spu.gpr[op.ra], mask));
	return true;
}

template <spu_exec_bit... Flags>
bool DFCMEQ(spu_thread&, spu_opcode_t)
{
	spu_log.fatal("DFCMEQ");
	return false;
}

template <spu_exec_bit... Flags>
bool MPYU(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto b = spu.gpr[op.rb];
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi32(_mm_mulhi_epu16(a, b), 16), _mm_and_si128(_mm_mullo_epi16(a, b), _mm_set1_epi32(0xffff)));
	return true;
}

template <spu_exec_bit... Flags>
bool CEQB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi8(spu.gpr[op.ra], spu.gpr[op.rb]);
	return true;
}

template <spu_exec_bit... Flags>
bool FI(spu_thread& spu, spu_opcode_t op)
{
	// TODO
	const auto mask_se = _mm_castsi128_ps(_mm_set1_epi32(0xff800000)); // sign and exponent mask
	const auto mask_bf = _mm_castsi128_ps(_mm_set1_epi32(0x007ffc00)); // base fraction mask
	const auto mask_sf = _mm_set1_epi32(0x000003ff); // step fraction mask
	const auto mask_yf = _mm_set1_epi32(0x0007ffff); // Y fraction mask (bits 13..31)
	const auto base = _mm_or_ps(_mm_and_ps(spu.gpr[op.rb], mask_bf), _mm_castsi128_ps(_mm_set1_epi32(0x3f800000)));
	const auto step = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(spu.gpr[op.rb], mask_sf)), _mm_set1_ps(std::exp2(-13.f)));
	const auto y = _mm_mul_ps(_mm_cvtepi32_ps(_mm_and_si128(spu.gpr[op.ra], mask_yf)), _mm_set1_ps(std::exp2(-19.f)));
	spu.gpr[op.rt] = _mm_or_ps(_mm_and_ps(mask_se, spu.gpr[op.rb]), _mm_andnot_ps(mask_se, _mm_sub_ps(base, _mm_mul_ps(step, y))));
	return true;
}

template <spu_exec_bit... Flags>
bool HEQ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] == spu.gpr[op.rb]._s32[3])
	{
		spu.halt();
	}
	return true;
}


template <spu_exec_bit... Flags>
bool CFLTS(spu_thread& spu, spu_opcode_t op)
{
	const auto scaled = _mm_mul_ps(spu.gpr[op.ra], g_spu_imm.scale[173 - op.i8]);
	spu.gpr[op.rt] = _mm_xor_si128(_mm_cvttps_epi32(scaled), _mm_castps_si128(_mm_cmpge_ps(scaled, _mm_set1_ps(0x80000000))));
	return true;
}

template <spu_exec_bit... Flags>
bool CFLTU(spu_thread& spu, spu_opcode_t op)
{
	const auto scaled1 = _mm_max_ps(_mm_mul_ps(spu.gpr[op.ra], g_spu_imm.scale[173 - op.i8]), _mm_set1_ps(0.0f));
	const auto scaled2 = _mm_and_ps(_mm_sub_ps(scaled1, _mm_set1_ps(0x80000000)), _mm_cmpge_ps(scaled1, _mm_set1_ps(0x80000000)));
	spu.gpr[op.rt] = _mm_or_si128(_mm_or_si128(_mm_cvttps_epi32(scaled1), _mm_cvttps_epi32(scaled2)), _mm_castps_si128(_mm_cmpge_ps(scaled1, _mm_set1_ps(0x100000000))));
	return true;
}

template <spu_exec_bit... Flags>
bool CSFLT(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_mul_ps(_mm_cvtepi32_ps(spu.gpr[op.ra]), g_spu_imm.scale[op.i8 - 155]);
	return true;
}

template <spu_exec_bit... Flags>
bool CUFLT(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto fix = _mm_and_ps(_mm_castsi128_ps(_mm_srai_epi32(a, 31)), _mm_set1_ps(0x80000000));
	spu.gpr[op.rt] = _mm_mul_ps(_mm_add_ps(_mm_cvtepi32_ps(_mm_and_si128(a, _mm_set1_epi32(0x7fffffff))), fix), g_spu_imm.scale[op.i8 - 155]);
	return true;
}


template <spu_exec_bit... Flags>
bool BRZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] == 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool STQA(spu_thread& spu, spu_opcode_t op)
{
	spu._ref<v128>(spu_ls_target(0, op.i16)) = spu.gpr[op.rt];
	return true;
}

template <spu_exec_bit... Flags>
bool BRNZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u32[3] != 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BRHZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] == 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool BRHNZ(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.rt]._u16[6] != 0)
	{
		spu.pc = spu_branch_target(spu.pc, op.i16);
		return false;
	}
	return true;
}

template <spu_exec_bit... Flags>
bool STQR(spu_thread& spu, spu_opcode_t op)
{
	spu._ref<v128>(spu_ls_target(spu.pc, op.i16)) = spu.gpr[op.rt];
	return true;
}

template <spu_exec_bit... Flags>
bool BRA(spu_thread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(0, op.i16);
	return false;
}

template <spu_exec_bit... Flags>
bool LQA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>(spu_ls_target(0, op.i16));
	return true;
}

template <spu_exec_bit... Flags>
bool BRASL(spu_thread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(0, op.i16);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target;
	return false;
}

template <spu_exec_bit... Flags>
bool BR(spu_thread& spu, spu_opcode_t op)
{
	spu.pc = spu_branch_target(spu.pc, op.i16);
	return false;
}

template <spu_exec_bit... Flags>
bool FSMBI(spu_thread& spu, spu_opcode_t op)
{
	const auto vsrc = _mm_set_epi32(0, 0, 0, op.i16);
	const auto bits = _mm_shuffle_epi32(_mm_shufflelo_epi16(_mm_unpacklo_epi8(vsrc, vsrc), 0x50), 0x50);
	const auto mask = _mm_set_epi8(-128, 64, 32, 16, 8, 4, 2, 1, -128, 64, 32, 16, 8, 4, 2, 1);
	spu.gpr[op.rt] = _mm_cmpeq_epi8(_mm_and_si128(bits, mask), mask);
	return true;
}

template <spu_exec_bit... Flags>
bool BRSL(spu_thread& spu, spu_opcode_t op)
{
	const u32 target = spu_branch_target(spu.pc, op.i16);
	spu.gpr[op.rt] = v128::from32r(spu_branch_target(spu.pc + 4));
	spu.pc = target;
	return false;
}

template <spu_exec_bit... Flags>
bool LQR(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>(spu_ls_target(spu.pc, op.i16));
	return true;
}

template <spu_exec_bit... Flags>
bool IL(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_set1_epi32(op.si16);
	return true;
}

template <spu_exec_bit... Flags>
bool ILHU(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_set1_epi32(op.i16 << 16);
	return true;
}

template <spu_exec_bit... Flags>
bool ILH(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_set1_epi16(op.i16);
	return true;
}

template <spu_exec_bit... Flags>
bool IOHL(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_or_si128(spu.gpr[op.rt], _mm_set1_epi32(op.i16));
	return true;
}


template <spu_exec_bit... Flags>
bool ORI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_or_si128(spu.gpr[op.ra], _mm_set1_epi32(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool ORHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_or_si128(spu.gpr[op.ra], _mm_set1_epi16(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool ORBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_or_si128(spu.gpr[op.ra], _mm_set1_epi8(op.i8));
	return true;
}

template <spu_exec_bit... Flags>
bool SFI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_sub_epi32(_mm_set1_epi32(op.si10), spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool SFHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_sub_epi16(_mm_set1_epi16(op.si10), spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool ANDI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_and_si128(spu.gpr[op.ra], _mm_set1_epi32(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool ANDHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_and_si128(spu.gpr[op.ra], _mm_set1_epi16(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool ANDBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_and_si128(spu.gpr[op.ra], _mm_set1_epi8(op.i8));
	return true;
}

template <spu_exec_bit... Flags>
bool AI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_add_epi32(_mm_set1_epi32(op.si10), spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool AHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_add_epi16(_mm_set1_epi16(op.si10), spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool STQD(spu_thread& spu, spu_opcode_t op)
{
	spu._ref<v128>((spu.gpr[op.ra]._s32[3] + (op.si10 * 16)) & 0x3fff0) = spu.gpr[op.rt];
	return true;
}

template <spu_exec_bit... Flags>
bool LQD(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = spu._ref<v128>((spu.gpr[op.ra]._s32[3] + (op.si10 * 16)) & 0x3fff0);
	return true;
}

template <spu_exec_bit... Flags>
bool XORI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi32(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool XORHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi16(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool XORBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi8(op.i8));
	return true;
}

template <spu_exec_bit... Flags>
bool CGTI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi32(spu.gpr[op.ra], _mm_set1_epi32(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool CGTHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi16(spu.gpr[op.ra], _mm_set1_epi16(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool CGTBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi8(spu.gpr[op.ra], _mm_set1_epi8(op.i8));
	return true;
}

template <spu_exec_bit... Flags>
bool HGTI(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] > op.si10)
	{
		spu.halt();
	}
	return true;
}

template <spu_exec_bit... Flags>
bool CLGTI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi32(_mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi32(0x80000000)), _mm_set1_epi32(op.si10 ^ 0x80000000));
	return true;
}

template <spu_exec_bit... Flags>
bool CLGTHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi16(_mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi32(0x80008000)), _mm_set1_epi16(op.si10 ^ 0x8000));
	return true;
}

template <spu_exec_bit... Flags>
bool CLGTBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpgt_epi8(_mm_xor_si128(spu.gpr[op.ra], _mm_set1_epi32(0x80808080)), _mm_set1_epi8(op.i8 ^ 0x80));
	return true;
}

template <spu_exec_bit... Flags>
bool HLGTI(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._u32[3] > static_cast<u32>(op.si10))
	{
		spu.halt();
	}
	return true;
}

template <spu_exec_bit... Flags>
bool MPYI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_madd_epi16(spu.gpr[op.ra], _mm_set1_epi32(op.si10 & 0xffff));
	return true;
}

template <spu_exec_bit... Flags>
bool MPYUI(spu_thread& spu, spu_opcode_t op)
{
	const auto a = spu.gpr[op.ra];
	const auto i = _mm_set1_epi32(op.si10 & 0xffff);
	spu.gpr[op.rt] = _mm_or_si128(_mm_slli_epi32(_mm_mulhi_epu16(a, i), 16), _mm_mullo_epi16(a, i));
	return true;
}

template <spu_exec_bit... Flags>
bool CEQI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi32(spu.gpr[op.ra], _mm_set1_epi32(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool CEQHI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi16(spu.gpr[op.ra], _mm_set1_epi16(op.si10));
	return true;
}

template <spu_exec_bit... Flags>
bool CEQBI(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_cmpeq_epi8(spu.gpr[op.ra], _mm_set1_epi8(op.i8));
	return true;
}

template <spu_exec_bit... Flags>
bool HEQI(spu_thread& spu, spu_opcode_t op)
{
	if (spu.gpr[op.ra]._s32[3] == op.si10)
	{
		spu.halt();
	}
	return true;
}


template <spu_exec_bit... Flags>
bool HBRA(spu_thread&, spu_opcode_t)
{
	return true;
}

template <spu_exec_bit... Flags>
bool HBRR(spu_thread&, spu_opcode_t)
{
	return true;
}

template <spu_exec_bit... Flags>
bool ILA(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt] = _mm_set1_epi32(op.i18);
	return true;
}


template <spu_exec_bit... Flags>
bool SELB(spu_thread& spu, spu_opcode_t op)
{
	spu.gpr[op.rt4] = (spu.gpr[op.rc] & spu.gpr[op.rb]) | gv_andn(spu.gpr[op.rc], spu.gpr[op.ra]);
	return true;
}

template <spu_exec_bit... Flags>
bool SHUFB(spu_thread& spu, spu_opcode_t op)
{
	__m128i ab[2]{__m128i(spu.gpr[op.rb]), __m128i(spu.gpr[op.ra])};
	v128 c = spu.gpr[op.rc];
	v128 x = _mm_andnot_si128(c, _mm_set1_epi8(0x1f));
	v128 res;

	// Select bytes
	for (int i = 0; i < 16; i++)
	{
		res._u8[i] = reinterpret_cast<u8*>(ab)[x._u8[i]];
	}

	// Select special values
	const auto xc0 = _mm_set1_epi8(static_cast<s8>(0xc0));
	const auto xe0 = _mm_set1_epi8(static_cast<s8>(0xe0));
	const auto cmp0 = _mm_cmpgt_epi8(_mm_setzero_si128(), c);
	const auto cmp1 = _mm_cmpeq_epi8(_mm_and_si128(c, xc0), xc0);
	const auto cmp2 = _mm_cmpeq_epi8(_mm_and_si128(c, xe0), xc0);
	spu.gpr[op.rt4] = _mm_or_si128(_mm_andnot_si128(cmp0, res), _mm_avg_epu8(cmp1, cmp2));
	return true;
}

#if defined(ARCH_X64)
const spu_intrp_func_t optimized_shufb = build_function_asm<spu_intrp_func_t>("spu_shufb", [](asmjit::x86::Assembler& c, auto& /*args*/)
{
	using namespace asmjit;

	const auto& va = x86::xmm0;
	const auto& vb = x86::xmm1;
	const auto& vc = x86::xmm2;
	const auto& vt = x86::xmm3;
	const auto& vm = x86::xmm4;
	const auto& v5 = x86::xmm5;

	Label xc0 = c.newLabel();
	Label xe0 = c.newLabel();
	Label x0f = c.newLabel();

	build_spu_gpr_load(c, va, s_op.ra);
	build_spu_gpr_load(c, vb, s_op.rb);
	build_spu_gpr_load(c, vc, s_op.rc);

	if (utils::has_avx())
	{
		c.vpand(v5, vc, x86::oword_ptr(xe0));
		c.vpxor(vc, vc, x86::oword_ptr(x0f));
		c.vpshufb(va, va, vc);
		c.vpslld(vt, vc, 3);
		c.vmovdqa(vm, x86::oword_ptr(xc0));
		c.vpcmpeqb(v5, v5, vm);
		c.vpshufb(vb, vb, vc);
		c.vpand(vc, vc, vm);
		c.vpblendvb(vb, va, vb, vt);
		c.vpcmpeqb(vt, vc, vm);
		c.vpavgb(vt, vt, v5);
		c.vpor(vt, vt, vb);
	}
	else
	{
		c.movdqa(v5, vc);
		c.pand(v5, x86::oword_ptr(xe0));
		c.movdqa(vt, vc);
		c.movdqa(vm, x86::oword_ptr(xc0));
		c.pand(vt, vm);
		c.pxor(vc, x86::oword_ptr(x0f));
		c.pshufb(va, vc);
		c.pshufb(vb, vc);
		c.pslld(vc, 3);
		c.pcmpeqb(v5, vm);
		c.pcmpeqb(vt, vm);
		c.pcmpeqb(vm, vm);
		c.pcmpgtb(vc, vm);
		c.pand(va, vc);
		c.pandn(vc, vb);
		c.por(vc, va);
		c.pavgb(vt, v5);
		c.por(vt, vc);
	}

	build_spu_gpr_store(c, vt, s_op.rt4);
	c.mov(x86::eax, 1);
	c.ret();

	c.align(AlignMode::kData, 16);
	c.bind(xc0);
	c.dq(0xc0c0c0c0c0c0c0c0);
	c.dq(0xc0c0c0c0c0c0c0c0);
	c.bind(xe0);
	c.dq(0xe0e0e0e0e0e0e0e0);
	c.dq(0xe0e0e0e0e0e0e0e0);
	c.bind(x0f);
	c.dq(0x0f0f0f0f0f0f0f0f);
	c.dq(0x0f0f0f0f0f0f0f0f);
});
#endif

template <spu_exec_bit... Flags>
bool MPYA(spu_thread& spu, spu_opcode_t op)
{
	const auto mask = _mm_set1_epi32(0xffff);
	spu.gpr[op.rt4] = _mm_add_epi32(spu.gpr[op.rc], _mm_madd_epi16(_mm_and_si128(spu.gpr[op.ra], mask), _mm_and_si128(spu.gpr[op.rb], mask)));
	return true;
}

template <spu_exec_bit... Flags>
bool FNMS(spu_thread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps(std::bit_cast<f32>(test_bits));

	auto test_a = _mm_and_ps(spu.gpr[op.ra], mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb], mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra], mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb], mask_b);

	spu.gpr[op.rt4] = _mm_sub_ps(spu.gpr[op.rc], _mm_mul_ps(a, b));
	return true;
}

template <spu_exec_bit... Flags>
bool FMA(spu_thread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps(std::bit_cast<f32>(test_bits));

	auto test_a = _mm_and_ps(spu.gpr[op.ra], mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb], mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra], mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb], mask_b);

	spu.gpr[op.rt4] = _mm_add_ps(_mm_mul_ps(a, b), spu.gpr[op.rc]);
	return true;
}

template <spu_exec_bit... Flags>
bool FMS(spu_thread& spu, spu_opcode_t op)
{
	const u32 test_bits = 0x7f800000;
	auto mask = _mm_set1_ps(std::bit_cast<f32>(test_bits));

	auto test_a = _mm_and_ps(spu.gpr[op.ra], mask);
	auto mask_a = _mm_cmpneq_ps(test_a, mask);
	auto test_b = _mm_and_ps(spu.gpr[op.rb], mask);
	auto mask_b = _mm_cmpneq_ps(test_b, mask);

	auto a = _mm_and_ps(spu.gpr[op.ra], mask_a);
	auto b = _mm_and_ps(spu.gpr[op.rb], mask_b);

	spu.gpr[op.rt4] = _mm_sub_ps(_mm_mul_ps(a, b), spu.gpr[op.rc]);
	return true;
}

#if 0

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
static const f32 FLOAT_MAX_NORMAL = std::bit_cast<f32>(FLOAT_MAX_NORMAL_I);
static const u32 FLOAT_NAN_I = 0x7FC00000;
static const f32 FLOAT_NAN = std::bit_cast<f32>(FLOAT_NAN_I);
static const u64 DOUBLE_NAN_I = 0x7FF8000000000000ULL;
static const f64 DOUBLE_NAN = std::bit_cast<f64>(DOUBLE_NAN_I);

inline bool issnan(double x)
{
	return std::isnan(x) && (std::bit_cast<s64>(x)) << 12 > 0;
}

inline bool issnan(float x)
{
	return std::isnan(x) && (std::bit_cast<s32>(x)) << 9 > 0;
}

inline bool isextended(float x)
{
	return fexpf(x) == 255;
}

inline float extended(bool sign, u32 mantissa) // returns -1^sign * 2^127 * (1.mantissa)
{
	u32 bits = sign << 31 | 0x7F800000 | mantissa;
	return std::bit_cast<f32>(bits);
}

inline float ldexpf_extended(float x, int exp)  // ldexpf() for extended values, assumes result is in range
{
	u32 bits = std::bit_cast<u32>(x);
	if (bits << 1 != 0) bits += exp * 0x00800000;
	return std::bit_cast<f32>(bits);
}

inline bool isdenormal(float x)
{
	return std::fpclassify(x) == FP_SUBNORMAL;
}

inline bool isdenormal(double x)
{
	return std::fpclassify(x) == FP_SUBNORMAL;
}

bool spu_interpreter_precise::FREST(spu_thread& spu, spu_opcode_t op)
{
	fesetround(FE_TOWARDZERO);
	const auto ra = spu.gpr[op.ra];
	v128 res = _mm_rcp_ps(ra);
	for (int i = 0; i < 4; i++)
	{
		const auto a = ra._f[i];
		const int exp = fexpf(a);

		if (exp == 0)
		{
			spu.fpscr.setDivideByZeroFlag(i);
			res._f[i] = extended(std::signbit(a), 0x7FFFFF);
		}
		else if (exp >= (0x7e800000 >> 23)) // Special case for values not handled properly in rcpps
		{
			res._f[i] = 0.0f;
		}
	}

	spu.gpr[op.rt] = res;
	return true;
}

bool spu_interpreter_precise::FRSQEST(spu_thread& spu, spu_opcode_t op)
{
	fesetround(FE_TOWARDZERO);
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
	return true;
}

bool spu_interpreter_precise::FCGT(spu_thread& spu, spu_opcode_t op)
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
			pass = static_cast<s32>(a) >= 0x00800000;
		else if (a >= 0x80000000)
			pass = (b >= 0x80000000 && a < b);
		else
			pass = (b >= 0x80000000 || a > b);
		spu.gpr[op.rt]._u32[i] = pass ? 0xFFFFFFFF : 0;
	}
	return true;
}

static void FA_FS(spu_thread& spu, spu_opcode_t op, bool sub)
{
	fesetround(FE_TOWARDZERO);
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
					const u32 bits = std::bit_cast<u32>(a) - 1;
					result = std::bit_cast<f32>(bits);
				}
				else

					result = a;
			}
			else if (isextended(b) && fexpf(a) < 255 - 32)
			{
				if (std::signbit(a) != std::signbit(b))
				{
					const u32 bits = std::bit_cast<u32>(b) - 1;
					result = std::bit_cast<f32>(bits);
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

bool spu_interpreter_precise::FA(spu_thread& spu, spu_opcode_t op) { FA_FS(spu, op, false); return true; }

bool spu_interpreter_precise::FS(spu_thread& spu, spu_opcode_t op) { FA_FS(spu, op, true); return true; }

bool spu_interpreter_precise::FM(spu_thread& spu, spu_opcode_t op)
{
	fesetround(FE_TOWARDZERO);
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
	return true;
}

bool spu_interpreter_precise::FCMGT(spu_thread& spu, spu_opcode_t op)
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
	return true;
}

enum DoubleOp
{
	DFASM_A,
	DFASM_S,
	DFASM_M,
};

static void DFASM(spu_thread& spu, spu_opcode_t op, DoubleOp operation)
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
			const u32 e = _mm_getcsr();
			if (e & _MM_EXCEPT_INVALID)
			{
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				if (e & _MM_EXCEPT_OVERFLOW)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
				if (e & _MM_EXCEPT_UNDERFLOW)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
				if (e & _MM_EXCEPT_INEXACT)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
			}
		}
		spu.gpr[op.rt]._d[i] = result;
	}
}

bool spu_interpreter_precise::DFA(spu_thread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_A); return true; }

bool spu_interpreter_precise::DFS(spu_thread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_S); return true; }

bool spu_interpreter_precise::DFM(spu_thread& spu, spu_opcode_t op) { DFASM(spu, op, DFASM_M); return true; }

static void DFMA(spu_thread& spu, spu_opcode_t op, bool neg, bool sub)
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
			const u32 e = _mm_getcsr();
			if (e & _MM_EXCEPT_INVALID)
			{
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINV);
				result = DOUBLE_NAN;
			}
			else
			{
				if (e & _MM_EXCEPT_OVERFLOW)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
				if (e & _MM_EXCEPT_UNDERFLOW)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
				if (e & _MM_EXCEPT_INEXACT)
					spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
				if (neg) result = -result;
			}
		}
		spu.gpr[op.rt]._d[i] = result;
	}
}

bool spu_interpreter_precise::DFMA(spu_thread& spu, spu_opcode_t op) { ::DFMA(spu, op, false, false); return true; }

bool spu_interpreter_precise::DFMS(spu_thread& spu, spu_opcode_t op) { ::DFMA(spu, op, false, true); return true; }

bool spu_interpreter_precise::DFNMS(spu_thread& spu, spu_opcode_t op) { ::DFMA(spu, op, true, true); return true; }

bool spu_interpreter_precise::DFNMA(spu_thread& spu, spu_opcode_t op) { ::DFMA(spu, op, true, false); return true; }

bool spu_interpreter_precise::FSCRRD(spu_thread& spu, spu_opcode_t op)
{
	spu.fpscr.Read(spu.gpr[op.rt]);
	return true;
}

bool spu_interpreter_precise::FESD(spu_thread& spu, spu_opcode_t op)
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
			spu.gpr[op.rt]._d[i] = a;
		}
	}
	return true;
}

bool spu_interpreter_precise::FRDS(spu_thread& spu, spu_opcode_t op)
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
			spu.gpr[op.rt]._f[i * 2 + 1] = static_cast<float>(a);
			const u32 e = _mm_getcsr();
			if (e & _MM_EXCEPT_OVERFLOW)
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DOVF);
			if (e & _MM_EXCEPT_UNDERFLOW)
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DUNF);
			if (e & _MM_EXCEPT_INEXACT)
				spu.fpscr.setDoublePrecisionExceptionFlags(i, FPSCR_DINX);
		}
		spu.gpr[op.rt]._u32[i * 2] = 0;
	}
	return true;
}

bool spu_interpreter_precise::FSCRWR(spu_thread& spu, spu_opcode_t op)
{
	spu.fpscr.Write(spu.gpr[op.ra]);
	return true;
}

bool spu_interpreter_precise::FCEQ(spu_thread& spu, spu_opcode_t op)
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
	return true;
}

bool spu_interpreter_precise::FCMEQ(spu_thread& spu, spu_opcode_t op)
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
	return true;
}

bool spu_interpreter_precise::FI(spu_thread& spu, spu_opcode_t op)
{
	// TODO
	spu.gpr[op.rt] = spu.gpr[op.rb];
	return true;
}

bool spu_interpreter_precise::CFLTS(spu_thread& spu, spu_opcode_t op)
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
			result = static_cast<s32>(scaled);
		spu.gpr[op.rt]._s32[i] = result;
	}
	return true;
}

bool spu_interpreter_precise::CFLTU(spu_thread& spu, spu_opcode_t op)
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
			result = static_cast<u32>(scaled);
		spu.gpr[op.rt]._u32[i] = result;
	}
	return true;
}

bool spu_interpreter_precise::CSFLT(spu_thread& spu, spu_opcode_t op)
{
	fesetround(FE_TOWARDZERO);
	const int scale = 155 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const s32 a = spu.gpr[op.ra]._s32[i];
		spu.gpr[op.rt]._f[i] = static_cast<float>(a);

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
	return true;
}

bool spu_interpreter_precise::CUFLT(spu_thread& spu, spu_opcode_t op)
{
	fesetround(FE_TOWARDZERO);
	const int scale = 155 - (op.i8 & 0xff); //unsigned immediate
	for (int i = 0; i < 4; i++)
	{
		const u32 a = spu.gpr[op.ra]._u32[i];
		spu.gpr[op.rt]._f[i] = static_cast<float>(a);

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
	return true;
}

static void FMA(spu_thread& spu, spu_opcode_t op, bool neg, bool sub)
{
	fesetround(FE_TOWARDZERO);
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
						u32 bits = std::bit_cast<u32>(result) - 1;
						result = std::bit_cast<f32>(bits);
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
					u32 bits = std::bit_cast<u32>(result) - 1;
					result = std::bit_cast<f32>(bits);
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

bool spu_interpreter_precise::FNMS(spu_thread& spu, spu_opcode_t op) { ::FMA(spu, op, true, true); return true; }

bool spu_interpreter_precise::FMA(spu_thread& spu, spu_opcode_t op) { ::FMA(spu, op, false, false); return true; }

bool spu_interpreter_precise::FMS(spu_thread& spu, spu_opcode_t op) { ::FMA(spu, op, false, true); return true; }

#endif /* __SSE2__ */

template <typename IT>
struct spu_interpreter_t
{
	IT UNK;
	IT HEQ;
	IT HEQI;
	IT HGT;
	IT HGTI;
	IT HLGT;
	IT HLGTI;
	IT HBR;
	IT HBRA;
	IT HBRR;
	IT STOP;
	IT STOPD;
	IT LNOP;
	IT NOP;
	IT SYNC;
	IT DSYNC;
	IT MFSPR;
	IT MTSPR;
	IT RDCH;
	IT RCHCNT;
	IT WRCH;
	IT LQD;
	IT LQX;
	IT LQA;
	IT LQR;
	IT STQD;
	IT STQX;
	IT STQA;
	IT STQR;
	IT CBD;
	IT CBX;
	IT CHD;
	IT CHX;
	IT CWD;
	IT CWX;
	IT CDD;
	IT CDX;
	IT ILH;
	IT ILHU;
	IT IL;
	IT ILA;
	IT IOHL;
	IT FSMBI;
	IT AH;
	IT AHI;
	IT A;
	IT AI;
	IT SFH;
	IT SFHI;
	IT SF;
	IT SFI;
	IT ADDX;
	IT CG;
	IT CGX;
	IT SFX;
	IT BG;
	IT BGX;
	IT MPY;
	IT MPYU;
	IT MPYI;
	IT MPYUI;
	IT MPYH;
	IT MPYS;
	IT MPYHH;
	IT MPYHHA;
	IT MPYHHU;
	IT MPYHHAU;
	IT CLZ;
	IT CNTB;
	IT FSMB;
	IT FSMH;
	IT FSM;
	IT GBB;
	IT GBH;
	IT GB;
	IT AVGB;
	IT ABSDB;
	IT SUMB;
	IT XSBH;
	IT XSHW;
	IT XSWD;
	IT AND;
	IT ANDC;
	IT ANDBI;
	IT ANDHI;
	IT ANDI;
	IT OR;
	IT ORC;
	IT ORBI;
	IT ORHI;
	IT ORI;
	IT ORX;
	IT XOR;
	IT XORBI;
	IT XORHI;
	IT XORI;
	IT NAND;
	IT NOR;
	IT EQV;
	IT MPYA;
	IT SELB;
	IT SHUFB;
	IT SHLH;
	IT SHLHI;
	IT SHL;
	IT SHLI;
	IT SHLQBI;
	IT SHLQBII;
	IT SHLQBY;
	IT SHLQBYI;
	IT SHLQBYBI;
	IT ROTH;
	IT ROTHI;
	IT ROT;
	IT ROTI;
	IT ROTQBY;
	IT ROTQBYI;
	IT ROTQBYBI;
	IT ROTQBI;
	IT ROTQBII;
	IT ROTHM;
	IT ROTHMI;
	IT ROTM;
	IT ROTMI;
	IT ROTQMBY;
	IT ROTQMBYI;
	IT ROTQMBYBI;
	IT ROTQMBI;
	IT ROTQMBII;
	IT ROTMAH;
	IT ROTMAHI;
	IT ROTMA;
	IT ROTMAI;
	IT CEQB;
	IT CEQBI;
	IT CEQH;
	IT CEQHI;
	IT CEQ;
	IT CEQI;
	IT CGTB;
	IT CGTBI;
	IT CGTH;
	IT CGTHI;
	IT CGT;
	IT CGTI;
	IT CLGTB;
	IT CLGTBI;
	IT CLGTH;
	IT CLGTHI;
	IT CLGT;
	IT CLGTI;
	IT BR;
	IT BRA;
	IT BRSL;
	IT BRASL;
	IT BI;
	IT IRET;
	IT BISLED;
	IT BISL;
	IT BRNZ;
	IT BRZ;
	IT BRHNZ;
	IT BRHZ;
	IT BIZ;
	IT BINZ;
	IT BIHZ;
	IT BIHNZ;
	IT FA;
	IT DFA;
	IT FS;
	IT DFS;
	IT FM;
	IT DFM;
	IT DFMA;
	IT DFNMS;
	IT DFMS;
	IT DFNMA;
	IT FREST;
	IT FRSQEST;
	IT FI;
	IT CSFLT;
	IT CFLTS;
	IT CUFLT;
	IT CFLTU;
	IT FRDS;
	IT FESD;
	IT FCEQ;
	IT FCMEQ;
	IT FCGT;
	IT FCMGT;
	IT FSCRWR;
	IT FSCRRD;
	IT DFCEQ;
	IT DFCMEQ;
	IT DFCGT;
	IT DFCMGT;
	IT DFTSV;
	IT FMA;
	IT FNMS;
	IT FMS;
};

spu_interpreter_rt_base::spu_interpreter_rt_base() noexcept
{
	// Obtain required set of flags from settings
	bs_t<spu_exec_bit> selected{};
	if (g_cfg.core.use_accurate_dfma)
		selected += use_dfma;

	ptrs = std::make_unique<decltype(ptrs)::element_type>();

	// Initialize instructions with their own sets of supported flags
#define INIT(name, ...) \
	ptrs->name = spu_exec_select<>::select<__VA_ARGS__>(selected, []<spu_exec_bit... Flags>(){ return &::name<Flags...>; }); \

	using enum spu_exec_bit;

	INIT(UNK);
	INIT(HEQ);
	INIT(HEQI);
	INIT(HGT);
	INIT(HGTI);
	INIT(HLGT);
	INIT(HLGTI);
	INIT(HBR);
	INIT(HBRA);
	INIT(HBRR);
	INIT(STOP);
	INIT(STOPD);
	INIT(LNOP);
	INIT(NOP);
	INIT(SYNC);
	INIT(DSYNC);
	INIT(MFSPR);
	INIT(MTSPR);
	INIT(RDCH);
	INIT(RCHCNT);
	INIT(WRCH);
	INIT(LQD);
	INIT(LQX);
	INIT(LQA);
	INIT(LQR);
	INIT(STQD);
	INIT(STQX);
	INIT(STQA);
	INIT(STQR);
	INIT(CBD);
	INIT(CBX);
	INIT(CHD);
	INIT(CHX);
	INIT(CWD);
	INIT(CWX);
	INIT(CDD);
	INIT(CDX);
	INIT(ILH);
	INIT(ILHU);
	INIT(IL);
	INIT(ILA);
	INIT(IOHL);
	INIT(FSMBI);
	INIT(AH);
	INIT(AHI);
	INIT(A);
	INIT(AI);
	INIT(SFH);
	INIT(SFHI);
	INIT(SF);
	INIT(SFI);
	INIT(ADDX);
	INIT(CG);
	INIT(CGX);
	INIT(SFX);
	INIT(BG);
	INIT(BGX);
	INIT(MPY);
	INIT(MPYU);
	INIT(MPYI);
	INIT(MPYUI);
	INIT(MPYH);
	INIT(MPYS);
	INIT(MPYHH);
	INIT(MPYHHA);
	INIT(MPYHHU);
	INIT(MPYHHAU);
	INIT(CLZ);
	INIT(CNTB);
	INIT(FSMB);
	INIT(FSMH);
	INIT(FSM);
	INIT(GBB);
	INIT(GBH);
	INIT(GB);
	INIT(AVGB);
	INIT(ABSDB);
	INIT(SUMB);
	INIT(XSBH);
	INIT(XSHW);
	INIT(XSWD);
	INIT(AND);
	INIT(ANDC);
	INIT(ANDBI);
	INIT(ANDHI);
	INIT(ANDI);
	INIT(OR);
	INIT(ORC);
	INIT(ORBI);
	INIT(ORHI);
	INIT(ORI);
	INIT(ORX);
	INIT(XOR);
	INIT(XORBI);
	INIT(XORHI);
	INIT(XORI);
	INIT(NAND);
	INIT(NOR);
	INIT(EQV);
	INIT(MPYA);
	INIT(SELB);
	INIT(SHUFB);
	INIT(SHLH);
	INIT(SHLHI);
	INIT(SHL);
	INIT(SHLI);
	INIT(SHLQBI);
	INIT(SHLQBII);
	INIT(SHLQBY);
	INIT(SHLQBYI);
	INIT(SHLQBYBI);
	INIT(ROTH);
	INIT(ROTHI);
	INIT(ROT);
	INIT(ROTI);
	INIT(ROTQBY);
	INIT(ROTQBYI);
	INIT(ROTQBYBI);
	INIT(ROTQBI);
	INIT(ROTQBII);
	INIT(ROTHM);
	INIT(ROTHMI);
	INIT(ROTM);
	INIT(ROTMI);
	INIT(ROTQMBY);
	INIT(ROTQMBYI);
	INIT(ROTQMBYBI);
	INIT(ROTQMBI);
	INIT(ROTQMBII);
	INIT(ROTMAH);
	INIT(ROTMAHI);
	INIT(ROTMA);
	INIT(ROTMAI);
	INIT(CEQB);
	INIT(CEQBI);
	INIT(CEQH);
	INIT(CEQHI);
	INIT(CEQ);
	INIT(CEQI);
	INIT(CGTB);
	INIT(CGTBI);
	INIT(CGTH);
	INIT(CGTHI);
	INIT(CGT);
	INIT(CGTI);
	INIT(CLGTB);
	INIT(CLGTBI);
	INIT(CLGTH);
	INIT(CLGTHI);
	INIT(CLGT);
	INIT(CLGTI);
	INIT(BR);
	INIT(BRA);
	INIT(BRSL);
	INIT(BRASL);
	INIT(BI);
	INIT(IRET);
	INIT(BISLED);
	INIT(BISL);
	INIT(BRNZ);
	INIT(BRZ);
	INIT(BRHNZ);
	INIT(BRHZ);
	INIT(BIZ);
	INIT(BINZ);
	INIT(BIHZ);
	INIT(BIHNZ);
	INIT(FA);
	INIT(DFA);
	INIT(FS);
	INIT(DFS);
	INIT(FM);
	INIT(DFM);
	INIT(DFMA);
	INIT(DFNMS);
	INIT(DFMS);
	INIT(DFNMA);
	INIT(FREST);
	INIT(FRSQEST);
	INIT(FI);
	INIT(CSFLT);
	INIT(CFLTS);
	INIT(CUFLT);
	INIT(CFLTU);
	INIT(FRDS);
	INIT(FESD);
	INIT(FCEQ);
	INIT(FCMEQ);
	INIT(FCGT);
	INIT(FCMGT);
	INIT(FSCRWR);
	INIT(FSCRRD);
	INIT(DFCEQ);
	INIT(DFCMEQ);
	INIT(DFCGT);
	INIT(DFCMGT);
	INIT(DFTSV);
	INIT(FMA);
	INIT(FNMS);
	INIT(FMS);
}

spu_interpreter_rt_base::~spu_interpreter_rt_base()
{
}

spu_interpreter_rt::spu_interpreter_rt() noexcept
	: spu_interpreter_rt_base()
	, table(*ptrs)
{
}
