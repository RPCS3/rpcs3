#include "stdafx.h"
#include "PPUInterpreter.h"

#include "Emu/Memory/vm_reservation.h"
#include "Emu/system_config.h"
#include "PPUThread.h"
#include "Emu/Cell/Common.h"
#include "Emu/Cell/PPUFunction.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/timers.hpp"
#include "Emu/IdManager.h"

#include <bit>
#include <cmath>
#include <climits>

#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/simd.hpp"
#include "util/sysinfo.hpp"
#include "Utilities/JIT.h"

#if !defined(_MSC_VER)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wold-style-cast"
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
void ppubreak(ppu_thread& ppu)
{
	if (!g_breakpoint_handler.IsBreakOnBPM())
		return;

	if (!ppu.state.test_and_set(cpu_flag::dbg_pause))
	{
		ppu.check_state();
	}
}

#define PPU_WRITE(type, addr, value) vm::write<type>(addr, value, &ppu);
#define PPU_WRITE_8(addr, value) vm::write8(addr, value, &ppu);
#define PPU_WRITE_16(addr, value) vm::write16(addr, value, &ppu);
#define PPU_WRITE_32(addr, value) vm::write32(addr, value, &ppu);
#define PPU_WRITE_64(addr, value) vm::write64(addr, value, &ppu);
#else
#define PPU_WRITE(type, addr, value) vm::write<type>(addr, value);
#define PPU_WRITE_8(addr, value) vm::write8(addr, value);
#define PPU_WRITE_16(addr, value) vm::write16(addr, value);
#define PPU_WRITE_32(addr, value) vm::write32(addr, value);
#define PPU_WRITE_64(addr, value) vm::write64(addr, value);
#endif

extern bool is_debugger_present();

extern const ppu_decoder<ppu_itype> g_ppu_itype;
extern const ppu_decoder<ppu_iname> g_ppu_iname;

enum class ppu_exec_bit : u64
{
	has_oe,
	has_rc,
	set_sat,
	use_nj,
	fix_nj,
	set_vnan,
	fix_vnan,
	set_fpcc,
	use_dfma,
	set_cr_stats,
	set_call_history,
	use_feed_data,

	__bitset_enum_max
};

using enum ppu_exec_bit;

// Helper for combining only used subset of exec flags at compile time
template <ppu_exec_bit... Flags0>
struct ppu_exec_select
{
	template <ppu_exec_bit Flag, ppu_exec_bit... Flags, typename F>
	static ppu_intrp_func_t select(bs_t<ppu_exec_bit> selected, F func)
	{
		// Make sure there is no flag duplication, otherwise skip flag
		if constexpr (((Flags0 != Flag) && ...))
		{
			// Test only relevant flags at runtime initialization (compile both variants)
			if (selected & Flag)
			{
				// In this branch, selected flag is added to Flags0
				return ppu_exec_select<Flags0..., Flag>::template select<Flags...>(selected, func);
			}
		}

		return ppu_exec_select<Flags0...>::template select<Flags...>(selected, func);
	}

	template <typename F>
	static ppu_intrp_func_t select(bs_t<ppu_exec_bit>, F func)
	{
		// Instantiate interpreter function with required set of flags
		return func.template operator()<Flags0...>();
	}

	template <ppu_exec_bit... Flags1>
	static auto select()
	{
#ifndef __INTELLISENSE__
		return [](bs_t<ppu_exec_bit> selected, auto func)
		{
			return ppu_exec_select::select<Flags1...>(selected, func);
		};
#endif
	}
};

// Switch between inlined interpreter invocation (exec) and builder function
#if defined(ARCH_X64)
#define RETURN(...) \
	if constexpr (Build == 0) { \
		static_cast<void>(exec); \
		static const ppu_intrp_func_t f = build_function_asm<ppu_intrp_func_t, asmjit::ppu_builder>("ppu_"s + __func__, [&](asmjit::ppu_builder& c, native_args&) { \
			static ppu_opcode_t op{}; \
			static ppu_abstract_t ppu; \
			exec(__VA_ARGS__); \
			c.ppu_ret(); \
			return !c.fail_flag; \
		}); \
		if (f) return f; \
		ppu_log.error("Can't build instruction %s", __func__); \
		RETURN_(__VA_ARGS__); \
	}
#else
#define RETURN RETURN_
#endif

#define RETURN_(...) \
	if constexpr (Build == 0) { \
		static_cast<void>(exec); \
		if (is_debugger_present() || g_cfg.core.ppu_debug) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn) { \
			exec(__VA_ARGS__); \
			const auto next_op = this_op + 1; \
			const auto fn = atomic_storage<ppu_intrp_func_t>::load(next_fn->fn); \
			ppu.cia = vm::get_addr(next_op); \
			return fn(ppu, {*next_op}, next_op, next_fn + 1); \
		}; \
		return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn) { \
			exec(__VA_ARGS__); \
			const auto next_op = this_op + 1; \
			const auto fn = atomic_storage<ppu_intrp_func_t>::observe(next_fn->fn); \
			return fn(ppu, {*next_op}, next_op, next_fn + 1); \
		}; \
	}

#ifdef ARCH_X64
static constexpr ppu_opcode_t s_op{};
#endif

namespace asmjit
{
#if defined(ARCH_X64)
	struct ppu_builder : vec_builder
	{
		using base = vec_builder;

#ifdef _WIN32
		static constexpr x86::Gp arg_ppu = x86::rcx;
		static constexpr x86::Gp arg_op = x86::edx;
		static constexpr x86::Gp arg_this_op = x86::r8;
		static constexpr x86::Gp arg_next_fn = x86::r9;
#else
		static constexpr x86::Gp arg_ppu = x86::rdi;
		static constexpr x86::Gp arg_op = x86::esi;
		static constexpr x86::Gp arg_this_op = x86::rdx;
		static constexpr x86::Gp arg_next_fn = x86::rcx;
#endif

		u32 xmm_count = 0;
		u32 ppu_base = 0;
		x86::Xmm tmp;

		ppu_builder(CodeHolder* ch)
			: base(ch)
		{
		}

		// Indexed offset to ppu.member
		template <auto MPtr, u32 Size = sizeof((std::declval<ppu_thread&>().*MPtr)[0]), uint I, uint N>
		x86::Mem ppu_mem(const bf_t<u32, I, N>&, bool last = false)
		{
			// Required index shift for array indexing
			constexpr u32 Shift = std::countr_zero(sizeof((std::declval<ppu_thread&>().*MPtr)[0]));

			const u32 offset = ::offset32(MPtr);

			auto tmp_r32 = x86::eax;
			auto reg_ppu = arg_ppu;

			if (last)
			{
				tmp_r32 = arg_op.r32();
			}
			else
			{
				base::mov(tmp_r32, arg_op);

				if (offset % 16 == 0 && ppu_base != offset)
				{
					// Optimistically precompute offset to avoid [ppu + tmp*x + offset] addressing
					base::lea(x86::r10, x86::qword_ptr(arg_ppu, static_cast<s32>(offset)));
					ppu_base = offset;
				}
			}

			if (ppu_base == offset)
			{
				reg_ppu = x86::r10;
			}

			// Use max possible index shift
			constexpr u32 X86Shift = Shift > 3 ? 3 : Shift;
			constexpr u32 AddShift = Shift - X86Shift;
			constexpr u32 AndMask = (1u << N) - 1;

			if constexpr (I >= AddShift)
			{
				if constexpr (I != AddShift)
					base::shr(tmp_r32, I - AddShift);
				base::and_(tmp_r32, AndMask << AddShift);
			}
			else
			{
				base::and_(tmp_r32, AndMask << I);
				base::shl(tmp_r32, I + AddShift);
			}

			return x86::ptr(reg_ppu, tmp_r32.r64(), X86Shift, static_cast<s32>(offset - ppu_base), Size);
		}

		// Generic offset to ppu.member
		template <auto MPtr, u32 Size = sizeof(std::declval<ppu_thread&>().*MPtr)>
		x86::Mem ppu_mem()
		{
			return x86::ptr(arg_ppu, static_cast<s32>(::offset32(MPtr)), Size);
		}

		template <u32 Size = 16, uint I, uint N>
		x86::Mem ppu_vr(const bf_t<u32, I, N>& bf, bool last = false)
		{
			return ppu_mem<&ppu_thread::vr, Size>(bf, last);
		}

		x86::Mem ppu_sat()
		{
			return ppu_mem<&ppu_thread::sat>();
		}

		void ppu_ret(bool last = true)
		{
			// Initialize pointer to next function
			base::mov(x86::rax, x86::qword_ptr(arg_next_fn));
			base::add(arg_this_op, 4);
			if (is_debugger_present())
				base::mov(ppu_mem<&ppu_thread::cia>(), arg_this_op.r32());
			base::mov(arg_op, x86::dword_ptr(arg_this_op));
			base::bswap(arg_op);
			base::add(arg_next_fn, 8);
			base::jmp(x86::rax);

			// Embed constants (TODO: after last return)
			if (last)
				base::emit_consts();
		}
	};
#elif defined(ARCH_ARM64)
	struct ppu_builder : a64::Assembler
	{
	};
#else
	struct ppu_builder
	{
	};
#endif
}

struct ppu_abstract_t
{
	struct abstract_vr
	{
		template <uint I, uint N>
		struct lazy_vr : asmjit::mem_lazy
		{
			const asmjit::Operand& eval(bool is_lv)
			{
				if (is_lv && !this->isReg())
				{
					Operand::operator=(g_vc->vec_alloc());
				#if defined(ARCH_X64)
					g_vc->emit(asmjit::x86::Inst::kIdMovaps, *this, static_cast<asmjit::ppu_builder*>(g_vc)->ppu_vr(bf_t<u32, I, N>{}, false));
				#endif
				}

				if (!is_lv)
				{
					if (this->isReg())
					{
						g_vc->vec_dealloc(asmjit::vec_type{this->id()});
					}
					else
					{
					#if defined(ARCH_X64)
						Operand::operator=(static_cast<asmjit::ppu_builder*>(g_vc)->ppu_vr(bf_t<u32, I, N>{}, false));
					#endif
					}
				}

				return *this;
			}

			template <typename T>
			void operator=([[maybe_unused]] T&& _val) const
			{
				FOR_X64(store_op, kIdMovaps, kIdVmovaps, static_cast<asmjit::ppu_builder*>(g_vc)->ppu_vr(bf_t<u32, I, N>{}, true), std::forward<T>(_val));
			}
		};

		template <uint I, uint N>
		lazy_vr<I, N> operator[](const bf_t<u32, I, N>&) const
		{
			return {};
		}
	} vr;

	struct abstract_sat : asmjit::mem_type
	{
		abstract_sat()
		#if defined(ARCH_X64)
			: asmjit::mem_type(static_cast<asmjit::ppu_builder*>(g_vc)->ppu_sat())
		#endif
		{
		}

		template <typename T>
		void operator=([[maybe_unused]] T&& _val) const
		{
			FOR_X64(store_op, kIdMovaps, kIdVmovaps, *this, std::forward<T>(_val));
		}
	} sat{};
};

extern void do_cell_atomic_128_store(u32 addr, const void* to_write);

inline u64 dup32(u32 x) { return x | static_cast<u64>(x) << 32; }

// Write values to CR field
inline void ppu_cr_set(ppu_thread& ppu, u32 field, bool le, bool gt, bool eq, bool so)
{
	ppu.cr[field * 4 + 0] = le;
	ppu.cr[field * 4 + 1] = gt;
	ppu.cr[field * 4 + 2] = eq;
	ppu.cr[field * 4 + 3] = so;

	if (g_cfg.core.ppu_debug) [[unlikely]]
	{
		*reinterpret_cast<u32*>(vm::g_stat_addr + ppu.cia) |= *reinterpret_cast<u32*>(ppu.cr.bits + field * 4);
	}
}

// Write comparison results to CR field
template<typename T>
inline void ppu_cr_set(ppu_thread& ppu, u32 field, const T& a, const T& b)
{
	ppu_cr_set(ppu, field, a < b, a > b, a == b, ppu.xer.so);
}

// TODO
template <ppu_exec_bit... Flags>
void ppu_set_cr(ppu_thread& ppu, u32 field, bool le, bool gt, bool eq, bool so)
{
	ppu.cr[field * 4 + 0] = le;
	ppu.cr[field * 4 + 1] = gt;
	ppu.cr[field * 4 + 2] = eq;
	ppu.cr[field * 4 + 3] = so;

	if constexpr (((Flags == set_cr_stats) || ...))
	{
		*reinterpret_cast<u32*>(vm::g_stat_addr + ppu.cia) |= *reinterpret_cast<u32*>(ppu.cr.bits + field * 4);
	}
}

// Set XER.OV bit (overflow)
inline void ppu_ov_set(ppu_thread& ppu, bool bit)
{
	ppu.xer.ov = bit;
	ppu.xer.so |= bit;
}

// Write comparison results to FPCC field with optional CR field update
template <ppu_exec_bit... Flags>
void ppu_set_fpcc(ppu_thread& ppu, f64 a, f64 b, u64 cr_field = 1)
{
	if constexpr (((Flags == set_fpcc || Flags == has_rc) || ...))
	{
		static_assert(std::endian::native == std::endian::little, "Not implemented");

		bool fpcc[4];
#if defined(ARCH_X64) && !defined(_M_X64)
		__asm__("comisd %[b], %[a]\n"
			: "=@ccb" (fpcc[0])
			, "=@cca" (fpcc[1])
			, "=@ccz" (fpcc[2])
			, "=@ccp" (fpcc[3])
			: [a] "x" (a)
			, [b] "x" (b)
			: "cc");
		if (fpcc[3]) [[unlikely]]
		{
			fpcc[0] = fpcc[1] = fpcc[2] = false;
		}
#else
		const auto cmp = a <=> b;
		fpcc[0] = cmp == std::partial_ordering::less;
		fpcc[1] = cmp == std::partial_ordering::greater;
		fpcc[2] = cmp == std::partial_ordering::equivalent;
		fpcc[3] = cmp == std::partial_ordering::unordered;
#endif

		const u32 data = std::bit_cast<u32>(fpcc);

		// Write FPCC
		ppu.fpscr.fields[4] = data;

		if constexpr (((Flags == has_rc) || ...))
		{
			// Previous behaviour was throwing an exception; TODO
			ppu.cr.fields[cr_field] = data;

			if (g_cfg.core.ppu_debug) [[unlikely]]
			{
				*reinterpret_cast<u32*>(vm::g_stat_addr + ppu.cia) |= data;
			}
		}
	}
}

// Validate read data in case does not match reservation
template <typename T, ppu_exec_bit... Flags>
auto ppu_feed_data(ppu_thread& ppu, u64 addr)
{
	static_assert(sizeof(T) <= 128, "Incompatible type-size, break down into smaller loads");

	auto value = vm::_ref<T>(vm::cast(addr));

#ifdef RPCS3_HAS_MEMORY_BREAKPOINTS
	if (g_breakpoint_handler.HasBreakpoint(addr, breakpoint_types::bp_read))
	{
		debugbp_log.success("BPMR: breakpoint reading 0x%x at 0x%x", value, addr);
		ppubreak(ppu);
	}
#endif

	if constexpr (!((Flags == use_feed_data) || ...))
	{
		return value;
	}

	if (!ppu.use_full_rdata)
	{
		return value;
	}

	const u32 raddr = ppu.raddr;

	if (addr / 128 > raddr / 128 || (addr + sizeof(T) - 1) / 128 < raddr / 128)
	{
		// Out of range or reservation does not exist
		return value;
	}

	if (sizeof(T) == 1 || addr / 128 == (addr + sizeof(T) - 1) / 128)
	{
		// Optimized comparison
		if (std::memcmp(&value, &ppu.rdata[addr & 127], sizeof(T)))
		{
			// Reservation was lost
			ppu.raddr = 0;
			ppu.res_cached = 0;
		}
	}
	else
	{
		alignas(16) std::byte buffer[sizeof(T)];
		std::memcpy(buffer, &value, sizeof(value)); // Put in memory explicitly (ensure the compiler won't do it beforehand)

		const std::byte* src;
		u32 size;
		u32 offs = 0;

		if (raddr / 128 == addr / 128)
			src = &ppu.rdata[addr & 127], size = std::min<u32>(128 - (addr % 128), sizeof(T));
		else
			src = &ppu.rdata[0], size = (addr + u32{sizeof(T)}) % 127, offs = sizeof(T) - size;

		if (std::memcmp(buffer + offs, src, size))
		{
			ppu.raddr = 0;
			ppu.res_cached = 0;
		}
	}

	return value;
}

// Push called address to custom call history for debugging
template <ppu_exec_bit... Flags>
u32 ppu_record_call(ppu_thread& ppu, u32 new_cia, ppu_opcode_t op, bool indirect = false)
{
	if constexpr (!((Flags == set_call_history) || ...))
	{
		return new_cia;
	}

	if (auto& history = ppu.call_history; !history.data.empty())
	{
		if (!op.lk)
		{
			if (indirect)
			{
				// Register LLE exported function trampolines
				// Trampolines do not change the stack pointer, and ones to exported functions change RTOC
				if (ppu.gpr[1] == history.last_r1 && ppu.gpr[2] != history.last_r2)
				{
					// Cancel condition
					history.last_r1 = umax;
					history.last_r2 = ppu.gpr[2];

					// Register trampolie with TOC
					history.data[history.index++ % ppu.call_history_max_size] = new_cia;
				}
			}

			return new_cia;
		}

		history.data[history.index++ % ppu.call_history_max_size] = new_cia;
		history.last_r1 = ppu.gpr[1];
		history.last_r2 = ppu.gpr[2];
	}

	return new_cia;
}

template<typename T>
struct add_flags_result_t
{
	T result;
	bool carry;

	add_flags_result_t() = default;

	// Straighforward ADD with flags
	add_flags_result_t(T a, T b)
		: result(a + b)
		, carry(result < a)
	{
	}

	// Straighforward ADC with flags
	add_flags_result_t(T a, T b, bool c)
		: add_flags_result_t(a, b)
	{
		add_flags_result_t r(result, c);
		result = r.result;
		carry |= r.carry;
	}
};

static add_flags_result_t<u64> add64_flags(u64 a, u64 b)
{
	return{ a, b };
}

static add_flags_result_t<u64> add64_flags(u64 a, u64 b, bool c)
{
	return{ a, b, c };
}

extern void ppu_execute_syscall(ppu_thread& ppu, u64 code);

extern u32 ppu_lwarx(ppu_thread& ppu, u32 addr);
extern u64 ppu_ldarx(ppu_thread& ppu, u32 addr);
extern bool ppu_stwcx(ppu_thread& ppu, u32 addr, u32 reg_value);
extern bool ppu_stdcx(ppu_thread& ppu, u32 addr, u64 reg_value);
extern void ppu_trap(ppu_thread& ppu, u64 addr);

// NaNs production precedence: NaN from Va, Vb, Vc
// and lastly the result of the operation in case none of the operands is a NaN
// Signaling NaNs are 'quieted' (MSB of fraction is set) with other bits of data remain the same
inline v128 ppu_select_vnan(v128 a)
{
	return a;
}

inline v128 ppu_select_vnan(v128 a, v128 b)
{
	return gv_selectfs(gv_eqfs(a, a), b, a | gv_bcst32(0x7fc00000u));
}

inline v128 ppu_select_vnan(v128 a, v128 b, Vector128 auto... args)
{
	return ppu_select_vnan(a, ppu_select_vnan(b, args...));
}

// Flush denormals to zero if NJ is 1
template <bool Result = false, ppu_exec_bit... Flags>
inline v128 ppu_flush_denormal(const v128& mask, const v128& a)
{
	if constexpr (((Flags == use_nj) || ...) || (Result && ((Flags == fix_nj) || ...)))
	{
		return gv_andn(gv_shr32(gv_eq32(mask & a, gv_bcst32(0)), 1), a);
	}
	else
	{
		return a;
	}
}

inline v128 ppu_fix_vnan(v128 r)
{
	return gv_selectfs(gv_eqfs(r, r), r, gv_bcst32(0x7fc00000u));
}

template <ppu_exec_bit... Flags>
inline v128 ppu_set_vnan(v128 r, Vector128 auto... args)
{
	if constexpr (((Flags == set_vnan) || ...) && sizeof...(args) > 0)
	{
		// Full propagation
		return ppu_select_vnan(args..., ppu_fix_vnan(r));
	}
	else if constexpr (((Flags == fix_vnan) || ...))
	{
		// Only fix the result
		return ppu_fix_vnan(r);
	}
	else
	{
		// Return as is
		return r;
	}
}

template <u32 Build, ppu_exec_bit... Flags>
auto MFVSCR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& sat, auto&& nj)
	{
		u32 sat_bit = 0;
		if constexpr (((Flags == set_sat) || ...))
			sat_bit = !gv_testz(sat); //!!sat._u;
		d._u64[0] = 0;
		d._u64[1] = u64(sat_bit | (u32{nj} << 16)) << 32;
	};

	RETURN_(ppu.vr[op.vd], ppu.sat, ppu.nj);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTVSCR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat, use_nj, fix_nj>();

	static const auto exec = [](auto&& sat, auto&& nj, auto&& jm_mask, auto&& b)
	{
		const u32 vscr = b._u32[3];
		if constexpr (((Flags == set_sat) || ...))
			sat._u = vscr & 1;
		if constexpr (((Flags == use_nj || Flags == fix_nj) || ...))
			jm_mask = (vscr & 0x10000) ? 0x7f80'0000 : 0x7fff'ffff;
		nj = (vscr & 0x10000) != 0;
	};

	RETURN_(ppu.sat, ppu.nj, ppu.jm_mask, ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDCUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		// ~a is how much can be added to a without carry
		d = gv_sub32(gv_geu32(gv_not32(std::move(a)), std::move(b)), gv_bcst32(-1));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a_, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto a = ppu_flush_denormal<false, Flags...>(m, std::move(a_));
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_addfs(a, b), a, b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDSBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_adds_s8(a, b);
			sat = gv_or32(gv_xor32(gv_add8(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_adds_s8(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDSHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_adds_s16(a, b);
			sat = gv_or32(gv_xor32(gv_add16(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_adds_s16(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDSWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_adds_s32(a, b);
			sat = gv_or32(gv_xor32(gv_add32(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_adds_s32(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUBM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_add8(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_addus_u8(a, b);
			sat = gv_or32(gv_xor32(gv_add8(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_addus_u8(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUHM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_add16(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_addus_u16(a, b);
			sat = gv_or32(gv_xor32(gv_add16(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_addus_u16(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUWM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_add32(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VADDUWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_addus_u32(a, b);
			sat = gv_or32(gv_xor32(gv_add32(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_addus_u32(std::move(a), std::move(b));
		}
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAND()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_andfs(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VANDC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_andnfs(std::move(b), std::move(a));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgs8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgs16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGSW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgs32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgu8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgu16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VAVGUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_avgu32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCFSX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b, u32 i)
	{
		d = gv_subus_u16(gv_cvts32_tofs(std::move(b)), gv_bcst32(i));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], op.vuimm << 23);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCFUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b, u32 i)
	{
		d = gv_subus_u16(gv_cvtu32_tofs(std::move(b)), gv_bcst32(i));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], op.vuimm << 23);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPBFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto sign = gv_bcstfs(-0.);
		auto cmp1 = gv_nlefs(a, b);
		auto cmp2 = gv_ngefs(a, b ^ sign);
		auto r = (std::move(cmp1) & sign) | gv_shr32(std::move(cmp2) & sign, 1);
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, false, false, gv_testz(r), false);
		d = std::move(r);
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPEQFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_eqfs(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPEQUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_eq8(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPEQUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_eq16(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPEQUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_eq32(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGEFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gefs(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gtfs(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gts8(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gts16(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTSW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gts32(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gtu8(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gtu16(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCMPGTUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& a, auto&& b)
	{
		auto r = gv_gtu32(std::move(a), std::move(b));
		if constexpr (((Flags == has_oe) || ...))
			ppu_cr_set(ppu, 6, gv_testall1(r), false, gv_testall0(r), false);
		d = r;
	};

	RETURN_(ppu, ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCTSXS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_vnan, set_sat>();

	static const auto exec = [](auto&& d, auto&& b, auto&& sat, u32 i)
	{
		auto r = gv_mulfs(b, gv_bcst32(i));
		auto l = gv_ltfs(r, gv_bcstfs(-2147483648.));
		auto h = gv_gefs(r, gv_bcstfs(2147483648.));
#if !defined(ARCH_X64) && !defined(ARCH_ARM64)
		r = gv_selectfs(l, gv_bcstfs(-2147483648.), std::move(r));
#endif
		r = gv_cvtfs_tos32(std::move(r));
#if !defined(ARCH_ARM64)
		r = gv_select32(h, gv_bcst32(0x7fffffff), std::move(r));
#endif
		if constexpr (((Flags == fix_vnan) || ...))
			r = gv_and32(std::move(r), gv_eqfs(b, b));
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_or32(std::move(l), std::move(h)), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.sat, (op.vuimm + 127) << 23);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VCTUXS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_vnan, set_sat>();

	static const auto exec = [](auto&& d, auto&& b, auto&& sat, u32 i)
	{
		auto r = gv_mulfs(b, gv_bcst32(i));
		auto l = gv_ltfs(r, gv_bcstfs(0.));
		auto h = gv_gefs(r, gv_bcstfs(4294967296.));
		r = gv_cvtfs_tou32(std::move(r));
#if !defined(ARCH_ARM64)
		r = gv_andn32(l, std::move(r)); // saturate to zero
#endif
#if !defined(__AVX512VL__) && !defined(ARCH_ARM64)
		r = gv_or32(std::move(r), h); // saturate to 0xffffffff
#endif
		if constexpr (((Flags == fix_vnan) || ...))
			r = gv_and32(std::move(r), gv_eqfs(b, b));
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_or32(std::move(l), std::move(h)), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.sat, (op.vuimm + 127) << 23);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VEXPTEFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		// for (u32 i = 0; i < 4; i++) d._f[i] = std::exp2f(b._f[i]);
		d = ppu_set_vnan<Flags...>(gv_exp2_approxfs(std::move(b)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VLOGEFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		// for (u32 i = 0; i < 4; i++) d._f[i] = std::log2f(b._f[i]);
		d = ppu_set_vnan<Flags...>(gv_log2_approxfs(std::move(b)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMADDFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a_, auto&& b_, auto&& c_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto a = ppu_flush_denormal<false, Flags...>(m, std::move(a_));
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		auto c = ppu_flush_denormal<false, Flags...>(m, std::move(c_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_fmafs(a, c, b)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& jm_mask)
	{
		d = ppu_flush_denormal<true, Flags...>(gv_bcst32(jm_mask, &ppu_thread::jm_mask), ppu_set_vnan<Flags...>(gv_maxfs(a, b), a, b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxs8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxs16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXSW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxs32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxu8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxu16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMAXUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_maxu32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMHADDSHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c, auto&& sat)
	{
		auto m = gv_muls_hds16(a, b);
		auto f = gv_gts16(gv_bcst16(0), c);
		auto x = gv_eq16(gv_maxs16(std::move(a), std::move(b)), gv_bcst16(0x8000));
		auto r = gv_sub16(gv_adds_s16(m, c), gv_and32(x, f));
		auto s = gv_add16(std::move(m), std::move(c));
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_or32(gv_andn32(std::move(f), x), gv_andn32(x, gv_xor32(std::move(s), r))), sat);
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMHRADDSHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c, auto&& sat)
	{
		if constexpr (((Flags != set_sat) && ...))
		{
			d = gv_rmuladds_hds16(std::move(a), std::move(b), std::move(c));
		}
		else
		{
			auto m = gv_rmuls_hds16(a, b);
			auto f = gv_gts16(gv_bcst16(0), c);
			auto x = gv_eq16(gv_maxs16(std::move(a), std::move(b)), gv_bcst16(0x8000));
			auto r = gv_sub16(gv_adds_s16(m, c), gv_and32(x, f));
			auto s = gv_add16(std::move(m), std::move(c));
			if constexpr (((Flags == set_sat) || ...))
				sat = gv_or32(gv_or32(gv_andn32(std::move(f), x), gv_andn32(x, gv_xor32(std::move(s), r))), sat);
			d = std::move(r);
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& jm_mask)
	{
		d = ppu_flush_denormal<true, Flags...>(gv_bcst32(jm_mask, &ppu_thread::jm_mask), ppu_set_vnan<Flags...>(gv_minfs(a, b), a, b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mins8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mins16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINSW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mins32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_minu8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_minu16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMINUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_minu32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMLADDUHM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		d = gv_muladd16(std::move(a), std::move(b), std::move(c));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGHB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpackhi8(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGHH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpackhi16(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGHW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpackhi32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGLB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpacklo8(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGLH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpacklo16(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMRGLW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_unpacklo32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMMBM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		d = gv_dotu8s8x4(std::move(b), std::move(a), std::move(c));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMSHM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		d = gv_dots16x2(std::move(a), std::move(b), std::move(c));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMSHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c, auto&& sat)
	{
		auto r = gv_dots_s16x2(a, b, c);
		auto s = gv_dots16x2(std::move(a), std::move(b), std::move(c));
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_xor32(std::move(s), r), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMUBM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		d = gv_dotu8x4(std::move(a), std::move(b), std::move(c));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMUHM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		d = gv_add32(std::move(c), gv_dotu16x2(std::move(a), std::move(b)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMSUMUHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c, auto&& sat)
	{
		auto m1 = gv_mul_even_u16(a, b);
		auto m2 = gv_mul_odds_u16(std::move(a), std::move(b));
		auto s1 = gv_add32(m1, m2);
		auto x1 = gv_gtu32(m1, s1);
		auto s2 = gv_or32(gv_add32(s1, std::move(c)), x1);
		auto x2 = gv_gtu32(std::move(s1), s2);
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_or32(std::move(x1), x2), std::move(sat));
		d = gv_or32(std::move(s2), std::move(x2));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULESB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul16(gv_sar16(std::move(a), 8), gv_sar16(std::move(b), 8));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULESH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul_odds_s16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULEUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul16(gv_shr16(std::move(a), 8), gv_shr16(std::move(b), 8));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULEUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul_odds_u16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULOSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul16(gv_sar16(gv_shl16(std::move(a), 8), 8), gv_sar16(gv_shl16(std::move(b), 8), 8));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULOSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul_even_s16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULOUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		auto mask = gv_bcst16(0x00ff);
		d = gv_mul16(gv_and32(std::move(a), mask), gv_and32(std::move(b), mask));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VMULOUH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_mul_even_u16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VNMSUBFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a_, auto&& b_, auto&& c_, auto&& jm_mask)
	{
		// An odd case with (FLT_MIN, FLT_MIN, FLT_MIN) produces FLT_MIN instead of 0
		auto s = gv_bcstfs(-0.0f);
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto a = ppu_flush_denormal<false, Flags...>(m, std::move(a_));
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		auto c = ppu_flush_denormal<false, Flags...>(m, std::move(c_));
		auto r = gv_xorfs(std::move(s), gv_fmafs(std::move(a), std::move(c), gv_xorfs(std::move(b), s)));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(std::move(r)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VNOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_notfs(gv_orfs(std::move(a), std::move(b)));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_orfs(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPERM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

#if defined (ARCH_X64)
	if constexpr (Build == 0)
	{
		static const ppu_intrp_func_t f = build_function_asm<ppu_intrp_func_t, asmjit::ppu_builder>("ppu_VPERM", [&](asmjit::ppu_builder& c, native_args&)
		{
			const auto [v0, v1, v2, v3] = c.vec_alloc<4>();
			c.movdqa(v0, c.ppu_vr(s_op.vc));
			c.pandn(v0, c.get_const(v128::from8p(0x1f)));
			c.movdqa(v1, v0);
			c.pcmpgtb(v1, c.get_const(v128::from8p(0xf)));
			c.movdqa(v2, c.ppu_vr(s_op.va));
			c.movdqa(v3, c.ppu_vr(s_op.vb));
			c.pshufb(v2, v0);
			c.pshufb(v3, v0);
			c.pand(v2, v1);
			c.pandn(v1, v3);
			c.por(v1, v2);
			c.movdqa(c.ppu_vr(s_op.vd), v1);
			c.ppu_ret();
		});

		if (utils::has_ssse3())
		{
			return f;
		}
	}
#endif

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
	#if defined(ARCH_ARM64)
		uint8x16x2_t ab;
		ab.val[0] = b;
		ab.val[1] = a;
		d = vqtbl2q_u8(ab, vbicq_u8(vdupq_n_u8(0x1f), c));
	#else
		u8 ab[32];
		std::memcpy(ab + 0, &b, 16);
		std::memcpy(ab + 16, &a, 16);

		for (u32 i = 0; i < 16; i++)
		{
			d._u8[i] = ab[~c._u8[i] & 0x1f];
		}
	#endif
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKPX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		auto a1 = gv_sar32(gv_shl32(a, 7), 7 + 9);
		auto b1 = gv_sar32(gv_shl32(b, 7), 7 + 9);
		auto a2 = gv_sar32(gv_shl32(a, 16), 16 + 3);
		auto b2 = gv_sar32(gv_shl32(b, 16), 16 + 3);
		auto p1 = gv_packss_s32(b1, a1);
		auto p2 = gv_packss_s32(b2, a2);
		d = gv_or32(gv_or32(gv_and32(p1, gv_bcst16(0xfc00)), gv_shl16(gv_and32(p1, gv_bcst16(0x7c)), 3)), gv_and32(p2, gv_bcst16(0x1f)));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKSHSS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr16(gv_add16(a, gv_bcst16(0x80)) | gv_add16(b, gv_bcst16(0x80)), 8), std::move(sat));
		d = gv_packss_s16(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKSHUS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr16(a | b, 8), std::move(sat));
		d = gv_packus_s16(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKSWSS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr32(gv_add32(a, gv_bcst32(0x8000)) | gv_add32(b, gv_bcst32(0x8000)), 16), std::move(sat));
		d = gv_packss_s32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKSWUS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr32(a | b, 16), std::move(sat));
		d = gv_packus_s32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKUHUM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_packtu16(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKUHUS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr16(a | b, 8), std::move(sat));
		d = gv_packus_u16(std::move(b), std::move(a));
	};
	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKUWUM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_packtu32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VPKUWUS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr32(a | b, 16), std::move(sat));
		d = gv_packus_u32(std::move(b), std::move(a));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VREFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_divfs(gv_bcstfs(1.0f), b), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRFIM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_roundfs_floor(b), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRFIN()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_roundfs_even(b), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRFIP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_roundfs_ceil(b), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRFIZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_roundfs_trunc(b), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRLB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_rol8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRLH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_rol16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRLW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_rol32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VRSQRTEFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_divfs(gv_bcstfs(1.0f), gv_sqrtfs(b)), b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSEL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& c)
	{
		auto x = gv_andfs(std::move(b), c);
		d = gv_orfs(std::move(x), gv_andnfs(std::move(c), std::move(a)));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.vr[op.vc]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_fshl8(std::move(a), gv_shuffle_left<1>(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSLB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shl8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Count>
struct VSLDOI
{
	template <ppu_exec_bit... Flags>
	static auto select(bs_t<ppu_exec_bit> selected, auto func)
	{
		return ppu_exec_select<>::select<Flags...>(selected, func);
	}

	template <u32 Build, ppu_exec_bit... Flags>
	static auto impl()
	{
		static const auto exec = [](auto&& d, auto&& a, auto&& b)
		{
			d = gv_or32(gv_shuffle_left<Count>(std::move(a)), gv_shuffle_right<16 - Count>(std::move(b)));
		};

		RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
	}
};


template <u32 Build, ppu_exec_bit... Flags>
auto VSLH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shl16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSLO()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d._u = a._u << (b._u8[0] & 0x78);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSLW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shl32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b, auto&& imm)
	{
		d = gv_bcst8(b.u8r[imm & 15]);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], op.vuimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b, auto&& imm)
	{
		d = gv_bcst16(b.u16r[imm & 7]);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], op.vuimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTISB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& imm)
	{
		d = gv_bcst8(imm);
	};

	RETURN_(ppu.vr[op.vd], op.vsimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTISH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& imm)
	{
		d = gv_bcst16(imm);
	};

	RETURN_(ppu.vr[op.vd], op.vsimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTISW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& imm)
	{
		d = gv_bcst32(imm);
	};

	RETURN_(ppu.vr[op.vd], op.vsimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSPLTW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b, auto&& imm)
	{
		d = gv_bcst32(b.u32r[imm & 3]);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.vb], op.vuimm);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_fshr8(gv_shuffle_right<1>(a), std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRAB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sar8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRAH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sar16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRAW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sar32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shr8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shr16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRO()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d._u = a._u >> (b._u8[0] & 0x78);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSRW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shr32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBCUW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_shr32(gv_geu32(std::move(a), std::move(b)), 31);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBFP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_nj, fix_nj, set_vnan, fix_vnan>();

	static const auto exec = [](auto&& d, auto&& a_, auto&& b_, auto&& jm_mask)
	{
		auto m = gv_bcst32(jm_mask, &ppu_thread::jm_mask);
		auto a = ppu_flush_denormal<false, Flags...>(m, std::move(a_));
		auto b = ppu_flush_denormal<false, Flags...>(m, std::move(b_));
		d = ppu_flush_denormal<true, Flags...>(std::move(m), ppu_set_vnan<Flags...>(gv_subfs(a, b), a, b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.jm_mask);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBSBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subs_s8(a, b);
			sat = gv_or32(gv_xor32(gv_sub8(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subs_s8(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBSHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subs_s16(a, b);
			sat = gv_or32(gv_xor32(gv_sub16(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subs_s16(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBSWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subs_s32(a, b);
			sat = gv_or32(gv_xor32(gv_sub32(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subs_s32(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUBM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sub8(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subus_u8(a, b);
			sat = gv_or32(gv_xor32(gv_sub8(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subus_u8(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUHM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sub16(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subus_u16(a, b);
			sat = gv_or32(gv_xor32(gv_sub16(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subus_u16(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUWM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_sub32(std::move(a), std::move(b));
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUBUWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		if constexpr (((Flags == set_sat) || ...))
		{
			auto r = gv_subus_u32(a, b);
			sat = gv_or32(gv_xor32(gv_sub32(std::move(a), std::move(b)), r), std::move(sat));
			d = std::move(r);
		}
		else
		{
			d = gv_subus_u32(std::move(a), std::move(b));
		}
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUMSWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		s64 sum = s64{b._s32[0]} + a._s32[0] + a._s32[1] + a._s32[2] + a._s32[3];
		if (sum > INT32_MAX)
		{
			sum = u32(INT32_MAX);
			if constexpr (((Flags == set_sat) || ...))
				sat._bytes[0] = 1;
		}
		else if (sum < INT32_MIN)
		{
			sum = u32(INT32_MIN);
			if constexpr (((Flags == set_sat) || ...))
				sat._bytes[0] = 1;
		}
		else
		{
			sum = static_cast<u32>(sum);
		}

		d._u = sum;
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUM2SWS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
#if defined(__AVX512VL__)
		const auto x = gv_add64(gv_sar64(gv_shl64(a, 32), 32), gv_sar64(a, 32));
		const auto y = gv_add64(x, gv_sar64(gv_shl64(b, 32), 32));
		const auto r = _mm_unpacklo_epi32(_mm_cvtsepi64_epi32(y), _mm_setzero_si128());
#elif defined(ARCH_ARM64)
		const auto x = vaddl_s32(vget_low_s32(vuzp1q_s32(a, a)), vget_low_s32(vuzp2q_s32(a, a)));
		const auto y = vaddw_s32(x, vget_low_s32(vuzp1q_s32(b, b)));
		const auto r = vmovl_u32(uint32x2_t(vqmovn_s64(y)));
#else
		v128 y{};
		y._s64[0] = s64{a._s32[0]} + a._s32[1] + b._s32[0];
		y._s64[1] = s64{a._s32[2]} + a._s32[3] + b._s32[2];
		v128 r{};
		r._u64[0] = y._s64[0] > INT32_MAX ? INT32_MAX : y._s64[0] < INT32_MIN ? u32(INT32_MIN) : static_cast<u32>(y._s64[0]);
		r._u64[1] = y._s64[1] > INT32_MAX ? INT32_MAX : y._s64[1] < INT32_MIN ? u32(INT32_MIN) : static_cast<u32>(y._s64[1]);
#endif
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_shr64(gv_add64(y, gv_bcst64(0x80000000u)), 32), std::move(sat));
		d = r;
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUM4SBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		auto r = gv_dots_u8s8x4(gv_bcst8(1), a, b);
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_xor32(gv_hadds8x4(std::move(a), std::move(b)), r), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUM4SHS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		auto r = gv_dots_s16x2(a, gv_bcst16(1), b);
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_xor32(gv_hadds16x2(std::move(a), std::move(b)), r), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VSUM4UBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_sat>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b, auto&& sat)
	{
		auto x = gv_haddu8x4(a);
		auto r = gv_addus_u32(x, b);
		if constexpr (((Flags == set_sat) || ...))
			sat = gv_or32(gv_xor32(gv_add32(std::move(x), std::move(b)), r), std::move(sat));
		d = std::move(r);
	};

	RETURN_(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb], ppu.sat);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKHPX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		auto x = gv_extend_hi_s16(std::move(b));
		auto y = gv_or32(gv_and32(gv_shl32(x, 6), gv_bcst32(0x1f0000)), gv_and32(gv_shl32(x, 3), gv_bcst32(0x1f00)));
		d = gv_or32(std::move(y), gv_and32(std::move(x), gv_bcst32(0xff00001f)));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKHSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		d = gv_extend_hi_s8(std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKHSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		d = gv_extend_hi_s16(std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKLPX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		auto x = gv_extend_lo_s16(std::move(b));
		auto y = gv_or32(gv_and32(gv_shl32(x, 6), gv_bcst32(0x1f0000)), gv_and32(gv_shl32(x, 3), gv_bcst32(0x1f00)));
		d = gv_or32(std::move(y), gv_and32(std::move(x), gv_bcst32(0xff00001f)));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKLSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		d = gv_extend_lo_s8(std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VUPKLSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& b)
	{
		d = gv_extend_lo_s16(std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto VXOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& d, auto&& a, auto&& b)
	{
		d = gv_xorfs(std::move(a), std::move(b));
	};

	RETURN(ppu.vr[op.vd], ppu.vr[op.va], ppu.vr[op.vb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto TDI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const s64 a = ppu.gpr[op.ra], b = op.simm16;
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		[[unlikely]]
		ppu_trap(ppu, vm::get_addr(this_op));
		return;
	}
	return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto TWI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const s32 a = static_cast<u32>(ppu.gpr[op.ra]), b = op.simm16;
	const u32 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		[[unlikely]]
		ppu_trap(ppu, vm::get_addr(this_op));
		return;
	}
	return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULLI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = static_cast<s64>(ppu.gpr[op.ra]) * op.simm16;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBFIC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 a = ppu.gpr[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(~a, i, 1);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CMPLI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if (op.l10)
	{
		ppu_cr_set<u64>(ppu, op.crfd, ppu.gpr[op.ra], op.uimm16);
	}
	else
	{
		ppu_cr_set<u32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), op.uimm16);
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CMPI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if (op.l10)
	{
		ppu_cr_set<s64>(ppu, op.crfd, ppu.gpr[op.ra], op.simm16);
	}
	else
	{
		ppu_cr_set<s32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), op.simm16);
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDIC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const s64 a = ppu.gpr[op.ra];
	const s64 i = op.simm16;
	const auto r = add64_flags(a, i);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if (op.main & 1) [[unlikely]] ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDIS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = op.ra ? ppu.gpr[op.ra] + (op.simm16 * 65536) : (op.simm16 * 65536);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto BC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_call_history>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.ctr -= (bo2 ^ true);
	const u32 link = vm::get_addr(this_op) + 4;
	if (op.lk) ppu.lr = link;

	const bool ctr_ok = bo2 | ((ppu.ctr != 0) ^ bo3);
	const bool cond_ok = bo0 | (!!(ppu.cr[op.bi]) ^ (bo1 ^ true));

	const u32 old_cia = ppu.cia;

	if (ctr_ok && cond_ok)
	{
		ppu.cia = vm::get_addr(this_op);
		// Provide additional information by using the origin of the call
		// Because this is a fixed target branch there's no abiguity about it
		ppu_record_call<Flags...>(ppu, ppu.cia, op);

		ppu.cia = (op.aa ? 0 : ppu.cia) + op.bt14;
	}
	else if (!ppu.state) [[likely]]
	{
		return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	}
	else
	{
		ppu.cia = link;
	}

	ppu.exec_bytes += link - old_cia;
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto SC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0)
	{
		return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func*)
		{
			const u32 old_cia = ppu.cia;
			ppu.cia = vm::get_addr(this_op);
			ppu.exec_bytes += ppu.cia - old_cia;
			if (op.opcode != ppu_instructions::SC(0))
			{
				fmt::throw_exception("Unknown/Illegal SC: 0x%08x", op.opcode);
			}

			ppu_execute_syscall(ppu, ppu.gpr[11]);
		};
	}
}

template <u32 Build, ppu_exec_bit... Flags>
auto B()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_call_history>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func*)
	{
	const u32 old_cia = ppu.cia;
	const u32 link = (ppu.cia = vm::get_addr(this_op)) + 4;
	// Provide additional information by using the origin of the call
	// Because this is a fixed target branch there's no abiguity about it
	ppu_record_call<Flags...>(ppu, ppu.cia, op);

	ppu.cia = (op.aa ? 0 : ppu.cia) + op.bt24;
	if (op.lk) ppu.lr = link;
	ppu.exec_bytes += link - old_cia;
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto MCRF()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	CHECK_SIZE(ppu_thread::cr, 32);
	ppu.cr.fields[op.crfd] = ppu.cr.fields[op.crfs];
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto BCLR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_call_history>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const bool bo0 = (op.bo & 0x10) != 0;
	const bool bo1 = (op.bo & 0x08) != 0;
	const bool bo2 = (op.bo & 0x04) != 0;
	const bool bo3 = (op.bo & 0x02) != 0;

	ppu.ctr -= (bo2 ^ true);

	const bool ctr_ok = bo2 | ((ppu.ctr != 0) ^ bo3);
	const bool cond_ok = bo0 | (!!(ppu.cr[op.bi]) ^ (bo1 ^ true));

	const u32 target = static_cast<u32>(ppu.lr) & ~3;
	const u32 link = vm::get_addr(this_op) + 4;
	if (op.lk) ppu.lr = link;

	const u32 old_cia = ppu.cia;

	if (ctr_ok && cond_ok)
	{
		ppu_record_call<Flags...>(ppu, target, op, true);
		ppu.cia = target;
	}
	else if (!ppu.state) [[likely]]
	{
		return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	}
	else
	{
		ppu.cia = link;
	}

	ppu.exec_bytes += link - old_cia;
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRNOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = (ppu.cr[op.crba] | ppu.cr[op.crbb]) ^ true;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRANDC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = ppu.cr[op.crba] & (ppu.cr[op.crbb] ^ true);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ISYNC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	atomic_fence_acquire();
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRXOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = ppu.cr[op.crba] ^ ppu.cr[op.crbb];
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRNAND()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = (ppu.cr[op.crba] & ppu.cr[op.crbb]) ^ true;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRAND()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = ppu.cr[op.crba] & ppu.cr[op.crbb];
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CREQV()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = (ppu.cr[op.crba] ^ ppu.cr[op.crbb]) ^ true;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CRORC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = ppu.cr[op.crba] | (ppu.cr[op.crbb] ^ true);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CROR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.cr[op.crbd] = ppu.cr[op.crba] | ppu.cr[op.crbb];
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto BCCTR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_call_history>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const u32 link = vm::get_addr(this_op) + 4;
	if (op.lk) ppu.lr = link;
	const u32 old_cia = ppu.cia;

	if (op.bo & 0x10 || ppu.cr[op.bi] == ((op.bo & 0x8) != 0))
	{
		const u32 target = static_cast<u32>(ppu.ctr) & ~3;
		ppu_record_call<Flags...>(ppu, target, op, true);
		ppu.cia = target;
	}
	else if (!ppu.state) [[likely]]
	{
		return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	}
	else
	{
		ppu.cia = link;
	}

	ppu.exec_bytes += link - old_cia;
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLWIMI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 mask = ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	ppu.gpr[op.ra] = (ppu.gpr[op.ra] & ~mask) | (dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), op.sh32)) & mask);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLWINM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), op.sh32)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLWNM()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = dup32(utils::rol32(static_cast<u32>(ppu.gpr[op.rs]), ppu.gpr[op.rb] & 0x1f)) & ppu_rotate_mask(32 + op.mb32, 32 + op.me32);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ORI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | op.uimm16;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ORIS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | (u64{op.uimm16} << 16);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto XORI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ op.uimm16;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto XORIS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ (u64{op.uimm16} << 16);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ANDI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & op.uimm16;
	ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ANDIS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & (u64{op.uimm16} << 16);
	ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDICL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & (~0ull >> op.mbe64);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDICR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & (~0ull << (op.mbe64 ^ 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDIC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], op.sh64) & ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDIMI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 mask = ppu_rotate_mask(op.mbe64, op.sh64 ^ 63);
	ppu.gpr[op.ra] = (ppu.gpr[op.ra] & ~mask) | (utils::rol64(ppu.gpr[op.rs], op.sh64) & mask);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDCL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], ppu.gpr[op.rb] & 0x3f) & (~0ull >> op.mbe64);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto RLDCR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = utils::rol64(ppu.gpr[op.rs], ppu.gpr[op.rb] & 0x3f) & (~0ull << (op.mbe64 ^ 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CMP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if (op.l10)
	{
		ppu_cr_set<s64>(ppu, op.crfd, ppu.gpr[op.ra], ppu.gpr[op.rb]);
	}
	else
	{
		ppu_cr_set<s32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), static_cast<u32>(ppu.gpr[op.rb]));
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto TW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	s32 a = static_cast<s32>(ppu.gpr[op.ra]);
	s32 b = static_cast<s32>(ppu.gpr[op.rb]);

	if ((a < b && (op.bo & 0x10)) ||
		(a > b && (op.bo & 0x8)) ||
		(a == b && (op.bo & 0x4)) ||
		(static_cast<u32>(a) < static_cast<u32>(b) && (op.bo & 0x2)) ||
		(static_cast<u32>(a) > static_cast<u32>(b) && (op.bo & 0x1)))
	{
		[[unlikely]]
		ppu_trap(ppu, vm::get_addr(this_op));
		return;
	}
	return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	};
}

const v128 s_lvsl_base = v128::from64r(0x0001020304050607, 0x08090a0b0c0d0e0f);

const v128 s_lvsl_consts[16] =
{
	gv_add8(s_lvsl_base, gv_bcst8(0)),
	gv_add8(s_lvsl_base, gv_bcst8(1)),
	gv_add8(s_lvsl_base, gv_bcst8(2)),
	gv_add8(s_lvsl_base, gv_bcst8(3)),
	gv_add8(s_lvsl_base, gv_bcst8(4)),
	gv_add8(s_lvsl_base, gv_bcst8(5)),
	gv_add8(s_lvsl_base, gv_bcst8(6)),
	gv_add8(s_lvsl_base, gv_bcst8(7)),
	gv_add8(s_lvsl_base, gv_bcst8(8)),
	gv_add8(s_lvsl_base, gv_bcst8(9)),
	gv_add8(s_lvsl_base, gv_bcst8(10)),
	gv_add8(s_lvsl_base, gv_bcst8(11)),
	gv_add8(s_lvsl_base, gv_bcst8(12)),
	gv_add8(s_lvsl_base, gv_bcst8(13)),
	gv_add8(s_lvsl_base, gv_bcst8(14)),
	gv_add8(s_lvsl_base, gv_bcst8(15)),
};

template <u32 Build, ppu_exec_bit... Flags>
auto LVSL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
		ppu.vr[op.vd] = s_lvsl_consts[addr % 16];
	};

	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVEBX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = ppu_feed_data<v128, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBFC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(~RA, RB, 1);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULHDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = utils::umulh64(ppu.gpr[op.ra], ppu.gpr[op.rb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(RA, RB);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULHWU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u32 a = static_cast<u32>(ppu.gpr[op.ra]);
	u32 b = static_cast<u32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = (u64{a} * b) >> 32;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 N>
struct MFOCRF
{
	template <ppu_exec_bit... Flags>
	static auto select(bs_t<ppu_exec_bit> selected, auto func)
	{
		return ppu_exec_select<>::select<Flags...>(selected, func);
	}

	template <u32 Build, ppu_exec_bit... Flags>
	static auto impl()
	{
		static const auto exec = [](ppu_thread& ppu, auto&& d)
		{
			const u32 p = N * 4;
			const u32 v = ppu.cr[p + 0] << 3 | ppu.cr[p + 1] << 2 | ppu.cr[p + 2] << 1 | ppu.cr[p + 3] << 0;

			d = v << (p ^ 0x1c);
		};

		RETURN_(ppu, ppu.gpr[op.rd]);
	}
};

template <u32 Build, ppu_exec_bit... Flags>
auto MFCR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, auto&& d)
	{
#if defined(ARCH_X64)
		be_t<v128> lane0, lane1;
		std::memcpy(&lane0, ppu.cr.bits, sizeof(v128));
		std::memcpy(&lane1, ppu.cr.bits + 16, sizeof(v128));
		const u32 mh = _mm_movemask_epi8(_mm_slli_epi64(lane0.value(), 7));
		const u32 ml = _mm_movemask_epi8(_mm_slli_epi64(lane1.value(), 7));

		d = (mh << 16) | ml;
#else
		d = ppu.cr.pack();
#endif
	};

	RETURN_(ppu, ppu.gpr[op.rd]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWARX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_lwarx(ppu, vm::cast(addr));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LDX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u64, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWZX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SLW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = static_cast<u32>(ppu.gpr[op.rs] << (ppu.gpr[op.rb] & 0x3f));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CNTLZW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = std::countl_zero(static_cast<u32>(ppu.gpr[op.rs]));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SLD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 n = ppu.gpr[op.rb] & 0x7f;
	ppu.gpr[op.ra] = n & 0x40 ? 0 : ppu.gpr[op.rs] << n;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto AND()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & ppu.gpr[op.rb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CMPL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if (op.l10)
	{
		ppu_cr_set<u64>(ppu, op.crfd, ppu.gpr[op.ra], ppu.gpr[op.rb]);
	}
	else
	{
		ppu_cr_set<u32>(ppu, op.crfd, static_cast<u32>(ppu.gpr[op.ra]), static_cast<u32>(ppu.gpr[op.rb]));
	}
	};
	RETURN_(ppu, op);
}

const v128 s_lvsr_consts[16] =
{
	gv_add8(s_lvsl_base, gv_bcst8(16)),
	gv_add8(s_lvsl_base, gv_bcst8(15)),
	gv_add8(s_lvsl_base, gv_bcst8(14)),
	gv_add8(s_lvsl_base, gv_bcst8(13)),
	gv_add8(s_lvsl_base, gv_bcst8(12)),
	gv_add8(s_lvsl_base, gv_bcst8(11)),
	gv_add8(s_lvsl_base, gv_bcst8(10)),
	gv_add8(s_lvsl_base, gv_bcst8(9)),
	gv_add8(s_lvsl_base, gv_bcst8(8)),
	gv_add8(s_lvsl_base, gv_bcst8(7)),
	gv_add8(s_lvsl_base, gv_bcst8(6)),
	gv_add8(s_lvsl_base, gv_bcst8(5)),
	gv_add8(s_lvsl_base, gv_bcst8(4)),
	gv_add8(s_lvsl_base, gv_bcst8(3)),
	gv_add8(s_lvsl_base, gv_bcst8(2)),
	gv_add8(s_lvsl_base, gv_bcst8(1)),
};

template <u32 Build, ppu_exec_bit... Flags>
auto LVSR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
		ppu.vr[op.vd] = s_lvsr_consts[addr % 16];
	};

	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVEHX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = ppu_feed_data<v128, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBF()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RB - RA;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LDUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u64, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBST()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWZUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u32, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto CNTLZD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = std::countl_zero(ppu.gpr[op.rs]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ANDC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] & ~ppu.gpr[op.rb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto TD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0) return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func* next_fn)
	{
	const s64 a = ppu.gpr[op.ra], b = ppu.gpr[op.rb];
	const u64 a_ = a, b_ = b;

	if (((op.bo & 0x10) && a < b) ||
		((op.bo & 0x8) && a > b) ||
		((op.bo & 0x4) && a == b) ||
		((op.bo & 0x2) && a_ < b_) ||
		((op.bo & 0x1) && a_ > b_))
	{
		[[unlikely]]
		ppu_trap(ppu, vm::get_addr(this_op));
		return;
	}
	return next_fn->fn(ppu, {this_op[1]}, this_op + 1, next_fn + 1);
	};
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVEWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = ppu_feed_data<v128, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULHD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = utils::mulh64(ppu.gpr[op.ra], ppu.gpr[op.rb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULHW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	s32 a = static_cast<s32>(ppu.gpr[op.ra]);
	s32 b = static_cast<s32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = (s64{a} * b) >> 32;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LDARX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_ldarx(ppu, vm::cast(addr));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBF()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LBZX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u8, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = ppu_feed_data<v128, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto NEG()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	ppu.gpr[op.rd] = 0 - RA;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, RA == (1ull << 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LBZUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u8, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto NOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] | ppu.gpr[op.rb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVEBX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	const u8 eb = addr & 0xf;
	PPU_WRITE_8(vm::cast(addr), ppu.vr[op.vs]._u8[15 - eb]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBFE()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(~RA, RB, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (~RA >> 63 == RB >> 63) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDE()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	const auto r = add64_flags(RA, RB, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTOCRF()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	alignas(4) static const u8 s_table[16][4]
	{
		{0, 0, 0, 0},
		{0, 0, 0, 1},
		{0, 0, 1, 0},
		{0, 0, 1, 1},
		{0, 1, 0, 0},
		{0, 1, 0, 1},
		{0, 1, 1, 0},
		{0, 1, 1, 1},
		{1, 0, 0, 0},
		{1, 0, 0, 1},
		{1, 0, 1, 0},
		{1, 0, 1, 1},
		{1, 1, 0, 0},
		{1, 1, 0, 1},
		{1, 1, 1, 0},
		{1, 1, 1, 1},
	};

	const u64 s = ppu.gpr[op.rs];

	if (op.l11)
	{
		// MTOCRF

		const u32 n = std::countl_zero<u32>(op.crm) & 7;
		const u64 v = (s >> ((n * 4) ^ 0x1c)) & 0xf;
		ppu.cr.fields[n] = *reinterpret_cast<const u32*>(s_table + v);
	}
	else
	{
		// MTCRF

		for (u32 i = 0; i < 8; i++)
		{
			if (op.crm & (128 >> i))
			{
				const u64 v = (s >> ((i * 4) ^ 0x1c)) & 0xf;
				ppu.cr.fields[i] = *reinterpret_cast<const u32*>(s_table + v);
			}
		}
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STDX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE_64(vm::cast(addr), ppu.gpr[op.rs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STWCX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu_cr_set(ppu, 0, false, false, ppu_stwcx(ppu, vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs])), ppu.xer.so);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVEHX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~1ULL;
	const u8 eb = (addr & 0xf) >> 1;
	PPU_WRITE_16(vm::cast(addr), ppu.vr[op.vs]._u16[7 - eb]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STDUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE_64(vm::cast(addr), ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STWUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVEWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~3ULL;
	const u8 eb = (addr & 0xf) >> 2;
	PPU_WRITE_32(vm::cast(addr), ppu.vr[op.vs]._u32[3 - eb]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBFZE()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(~RA, 0, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (~RA >> 63 == 0) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDZE()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(RA, 0, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (RA >> 63 == 0) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STDCX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu_cr_set(ppu, 0, false, false, ppu_stdcx(ppu, vm::cast(addr), ppu.gpr[op.rs]), ppu.xer.so);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STBX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE_8(vm::cast(addr), static_cast<u8>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	PPU_WRITE(v128, vm::cast(addr), ppu.vr[op.vs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULLD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const s64 RA = ppu.gpr[op.ra];
	const s64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RA * RB;
	if (op.oe) [[unlikely]]
	{
		const s64 high = utils::mulh64(RA, RB);
		ppu_ov_set(ppu, high != s64(ppu.gpr[op.rd]) >> 63);
	}
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SUBFME()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(~RA, ~0ull, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (~RA >> 63 == 1) && (~RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADDME()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const s64 RA = ppu.gpr[op.ra];
	const auto r = add64_flags(RA, ~0ull, ppu.xer.ca);
	ppu.gpr[op.rd] = r.result;
	ppu.xer.ca = r.carry;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (u64(RA) >> 63 == 1) && (u64(RA) >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, r.result, 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MULLW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.rd] = s64{static_cast<s32>(ppu.gpr[op.ra])} * static_cast<s32>(ppu.gpr[op.rb]);
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, s64(ppu.gpr[op.rd]) < INT32_MIN || s64(ppu.gpr[op.rd]) > INT32_MAX);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBTST()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STBUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE_8(vm::cast(addr), static_cast<u8>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ADD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RA + RB;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, (RA >> 63 == RB >> 63) && (RA >> 63 != ppu.gpr[op.rd] >> 63));
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBT()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHZX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u16, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto EQV()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] ^ ppu.gpr[op.rb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ECIWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	fmt::throw_exception("ECIWX");
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHZUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<u16, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto XOR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] ^ ppu.gpr[op.rb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MFSPR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001: ppu.gpr[op.rd] = u32{ppu.xer.so} << 31 | ppu.xer.ov << 30 | ppu.xer.ca << 29 | ppu.xer.cnt; break;
	case 0x008: ppu.gpr[op.rd] = ppu.lr; break;
	case 0x009: ppu.gpr[op.rd] = ppu.ctr; break;
	case 0x100: ppu.gpr[op.rd] = ppu.vrsave; break;

	case 0x10C: ppu.gpr[op.rd] = get_timebased_time(); break;
	case 0x10D: ppu.gpr[op.rd] = get_timebased_time() >> 32; break;
	default: fmt::throw_exception("MFSPR 0x%x", n);
	}

	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWAX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<s32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DST()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHAX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<s16, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVXL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	ppu.vr[op.vd] = ppu_feed_data<v128, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MFTB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x10C: ppu.gpr[op.rd] = get_timebased_time(); break;
	case 0x10D: ppu.gpr[op.rd] = get_timebased_time() >> 32; break;
	default: fmt::throw_exception("MFTB 0x%x", n);
	}

	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWAUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<s32, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DSTST()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHAUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<s16, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STHX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE_16(vm::cast(addr), static_cast<u16>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ORC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | ~ppu.gpr[op.rb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ECOWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	fmt::throw_exception("ECOWX");
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STHUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE_16(vm::cast(addr), static_cast<u16>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto OR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ppu.gpr[op.rs] | ppu.gpr[op.rb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DIVDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 RA = ppu.gpr[op.ra];
	const u64 RB = ppu.gpr[op.rb];
	ppu.gpr[op.rd] = RB == 0 ? 0 : RA / RB;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, RB == 0);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DIVWU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 RA = static_cast<u32>(ppu.gpr[op.ra]);
	const u32 RB = static_cast<u32>(ppu.gpr[op.rb]);
	ppu.gpr[op.rd] = RB == 0 ? 0 : RA / RB;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, RB == 0);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTSPR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 n = (op.spr >> 5) | ((op.spr & 0x1f) << 5);

	switch (n)
	{
	case 0x001:
	{
		const u64 value = ppu.gpr[op.rs];
		ppu.xer.so = (value & 0x80000000) != 0;
		ppu.xer.ov = (value & 0x40000000) != 0;
		ppu.xer.ca = (value & 0x20000000) != 0;
		ppu.xer.cnt = value & 0x7f;
		break;
	}
	case 0x008: ppu.lr = ppu.gpr[op.rs]; break;
	case 0x009: ppu.ctr = ppu.gpr[op.rs]; break;
	case 0x100: ppu.vrsave = static_cast<u32>(ppu.gpr[op.rs]); break;
	default: fmt::throw_exception("MTSPR 0x%x", n);
	}

	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto NAND()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = ~(ppu.gpr[op.rs] & ppu.gpr[op.rb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVXL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb]) & ~0xfull;
	PPU_WRITE(v128, vm::cast(addr), ppu.vr[op.vs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DIVD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const s64 RA = ppu.gpr[op.ra];
	const s64 RB = ppu.gpr[op.rb];
	const bool o = RB == 0 || (RA == INT64_MIN && RB == -1);
	ppu.gpr[op.rd] = o ? 0 : RA / RB;
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, o);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DIVW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const s32 RA = static_cast<s32>(ppu.gpr[op.ra]);
	const s32 RB = static_cast<s32>(ppu.gpr[op.rb]);
	const bool o = RB == 0 || (RA == INT32_MIN && RB == -1);
	ppu.gpr[op.rd] = o ? 0 : static_cast<u32>(RA / RB);
	if constexpr (((Flags == has_oe) || ...))
		ppu_ov_set(ppu, o);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.rd], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVLX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
		const u128 data = ppu_feed_data<u128, Flags...>(ppu, addr & -16);
		ppu.vr[op.vd] = data << ((addr & 15) * 8);
	};

	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LDBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<le_t<u64>, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LSWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	u32 count = ppu.xer.cnt & 0x7f;
	for (; count >= 4; count -= 4, addr += 4, op.rd = (op.rd + 1) & 31)
	{
		ppu.gpr[op.rd] = ppu_feed_data<u32, Flags...>(ppu, addr);
	}
	if (count)
	{
		u32 value = 0;
		for (u32 byte = 0; byte < count; byte++)
		{
			u32 byte_value = ppu_feed_data<u8, Flags...>(ppu, addr + byte);
			value |= byte_value << ((3 ^ byte) * 8);
		}
		ppu.gpr[op.rd] = value;
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<le_t<u32>, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFSX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.fpr[op.frd] = ppu_feed_data<f32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = (ppu.gpr[op.rs] & 0xffffffff) >> (ppu.gpr[op.rb] & 0x3f);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 n = ppu.gpr[op.rb] & 0x7f;
	ppu.gpr[op.ra] = n & 0x40 ? 0 : ppu.gpr[op.rs] >> n;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];

		// Read from instruction address if offset is 0, this prevents accessing potentially bad memory from addr (because no actual memory is dereferenced)
		const u128 data = ppu_feed_data<u128, Flags...>(ppu, ((addr & 15) == 0 ? ppu.cia : addr) & -16);
		ppu.vr[op.vd] = data >> ((~addr & 15) * 8) >> 8;
	};

	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LSWI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			ppu.gpr[reg] = ppu_feed_data<u32, Flags...>(ppu, addr);
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
				buf |= ppu_feed_data<u8, Flags...>(ppu, addr) << (i * 8);
				addr++;
				i--;
			}
			ppu.gpr[reg] = buf;
		}
		reg = (reg + 1) % 32;
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFSUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.fpr[op.frd] = ppu_feed_data<f32, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SYNC()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	atomic_fence_seq_cst();
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFDX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.fpr[op.frd] = ppu_feed_data<f64, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFDUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	ppu.fpr[op.frd] = ppu_feed_data<f64, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVLX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& s, ppu_opcode_t op, auto&& a, auto&& b)
	{
		const u64 addr = op.ra ? a + b : b;
		const u32 tail = u32(addr & 15);
		u8* ptr = vm::_ptr<u8>(addr);
		for (u32 j = 0; j < 16 - tail; j++)
			ptr[j] = s.u8r[j];
	};

	RETURN_(ppu.vr[op.vs], op, ppu.gpr[op.ra], ppu.gpr[op.rb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STDBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE(le_t<u64>, vm::cast(addr), ppu.gpr[op.rs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STSWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	u32 count = ppu.xer.cnt & 0x7F;
	for (; count >= 4; count -= 4, addr += 4, op.rs = (op.rs + 1) & 31)
	{
		PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs]));
	}
	if (count)
	{
		u32 value = static_cast<u32>(ppu.gpr[op.rs]);
		for (u32 byte = 0; byte < count; byte++)
		{
			u8 byte_value = static_cast<u8>(value >> ((3 ^ byte) * 8));
			PPU_WRITE_8(vm::cast(addr + byte), byte_value);
		}
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STWBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE(le_t<u32>, vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFSX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE(f32, vm::cast(addr), static_cast<float>(ppu.fpr[op.frs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& s, ppu_opcode_t op, auto&& a, auto&& b)
	{
		const u64 addr = op.ra ? a + b : b;
		const u32 tail = u32(addr & 15);
		u8* ptr = vm::_ptr<u8>(addr - 16);
		for (u32 i = 15; i > 15 - tail; i--)
			ptr[i] = s.u8r[i];
	};

	RETURN_(ppu.vr[op.vs], op, ppu.gpr[op.ra], ppu.gpr[op.rb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFSUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE(f32, vm::cast(addr), static_cast<float>(ppu.fpr[op.frs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STSWI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] : 0;
	u64 N = op.rb ? op.rb : 32;
	u8 reg = op.rd;

	while (N > 0)
	{
		if (N > 3)
		{
			PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[reg]));
			addr += 4;
			N -= 4;
		}
		else
		{
			u32 buf = static_cast<u32>(ppu.gpr[reg]);
			while (N > 0)
			{
				N = N - 1;
				PPU_WRITE_8(vm::cast(addr), (0xFF000000 & buf) >> 24);
				buf <<= 8;
				addr++;
			}
		}
		reg = (reg + 1) % 32;
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFDX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE(f64, vm::cast(addr), ppu.fpr[op.frs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFDUX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + ppu.gpr[op.rb];
	PPU_WRITE(f64, vm::cast(addr), ppu.fpr[op.frs]);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVLXL()
{
	return LVLX<Build, Flags...>();
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	ppu.gpr[op.rd] = ppu_feed_data<le_t<u16>, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRAW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	s32 RS = static_cast<s32>(ppu.gpr[op.rs]);
	u8 shift = ppu.gpr[op.rb] & 63;
	if (shift > 31)
	{
		ppu.gpr[op.ra] = 0 - (RS < 0);
		ppu.xer.ca = (RS < 0);
	}
	else
	{
		ppu.gpr[op.ra] = RS >> shift;
		ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << shift) != static_cast<u64>(RS));
	}

	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRAD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	s64 RS = ppu.gpr[op.rs];
	u8 shift = ppu.gpr[op.rb] & 127;
	if (shift > 63)
	{
		ppu.gpr[op.ra] = 0 - (RS < 0);
		ppu.xer.ca = (RS < 0);
	}
	else
	{
		ppu.gpr[op.ra] = RS >> shift;
		ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << shift) != static_cast<u64>(RS));
	}

	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LVRXL()
{
	return LVRX<Build, Flags...>();
}

template <u32 Build, ppu_exec_bit... Flags>
auto DSS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRAWI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	s32 RS = static_cast<u32>(ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = RS >> op.sh32;
	ppu.xer.ca = (RS < 0) && (static_cast<u32>(ppu.gpr[op.ra] << op.sh32) != static_cast<u32>(RS));

	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto SRADI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	auto sh = op.sh64;
	s64 RS = ppu.gpr[op.rs];
	ppu.gpr[op.ra] = RS >> sh;
	ppu.xer.ca = (RS < 0) && ((ppu.gpr[op.ra] << sh) != static_cast<u64>(RS));

	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto EIEIO()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	atomic_fence_seq_cst();
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVLXL()
{
	return STVLX<Build, Flags...>();
}

template <u32 Build, ppu_exec_bit... Flags>
auto STHBRX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE(le_t<u16>, vm::cast(addr), static_cast<u16>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto EXTSH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = static_cast<s16>(ppu.gpr[op.rs]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STVRXL()
{
	return STVRX<Build, Flags...>();
}

template <u32 Build, ppu_exec_bit... Flags>
auto EXTSB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = static_cast<s8>(ppu.gpr[op.rs]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFIWX()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	PPU_WRITE_32(vm::cast(addr), static_cast<u32>(std::bit_cast<u64>(ppu.fpr[op.frs])));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto EXTSW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.gpr[op.ra] = static_cast<s32>(ppu.gpr[op.rs]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set<s64>(ppu, 0, ppu.gpr[op.ra], 0);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto ICBI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&&, auto) {
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto DCBZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra ? ppu.gpr[op.ra] + ppu.gpr[op.rb] : ppu.gpr[op.rb];
	const u32 addr0 = vm::cast(addr) & ~127;

	if (g_cfg.core.accurate_cache_line_stores)
	{
		alignas(64) static constexpr u8 zero_buf[128]{};
		do_cell_atomic_128_store(addr0, zero_buf);
		return;
	}

	std::memset(vm::base(addr0), 0, 128);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWZU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u32, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LBZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u8, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LBZU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u8, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	const u32 value = static_cast<u32>(ppu.gpr[op.rs]);
	PPU_WRITE_32(vm::cast(addr), value);

	//Insomniac engine v3 & v4 (newer R&C, Fuse, Resitance 3)
	if (value == 0xAAAAAAAA) [[unlikely]]
	{
		vm::reservation_update(vm::cast(addr));
	}

	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STWU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	PPU_WRITE_8(vm::cast(addr), static_cast<u8>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STBU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	PPU_WRITE_8(vm::cast(addr), static_cast<u8>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u16, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHZU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<u16, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHA()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<s16, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LHAU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.gpr[op.rd] = ppu_feed_data<s16, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STH()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	PPU_WRITE_16(vm::cast(addr), static_cast<u16>(ppu.gpr[op.rs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STHU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	PPU_WRITE_16(vm::cast(addr), static_cast<u16>(ppu.gpr[op.rs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LMW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rd; i<32; ++i, addr += 4)
	{
		ppu.gpr[i] = ppu_feed_data<u32, Flags...>(ppu, addr);
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STMW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	u64 addr = op.ra ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	for (u32 i = op.rs; i<32; ++i, addr += 4)
	{
		PPU_WRITE_32(vm::cast(addr), static_cast<u32>(ppu.gpr[i]));
	}
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.fpr[op.frd] = ppu_feed_data<f32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFSU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.fpr[op.frd] = ppu_feed_data<f32, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	ppu.fpr[op.frd] = ppu_feed_data<f64, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LFDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	ppu.fpr[op.frd] = ppu_feed_data<f64, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	PPU_WRITE(f32, vm::cast(addr), static_cast<float>(ppu.fpr[op.frs]));
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFSU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	PPU_WRITE(f32, vm::cast(addr), static_cast<float>(ppu.fpr[op.frs]));
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = op.ra || 1 ? ppu.gpr[op.ra] + op.simm16 : op.simm16;
	PPU_WRITE(f64, vm::cast(addr), ppu.fpr[op.frs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STFDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + op.simm16;
	PPU_WRITE(f64, vm::cast(addr), ppu.fpr[op.frs]);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	ppu.gpr[op.rd] = ppu_feed_data<u64, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + (op.simm16 & ~3);
	ppu.gpr[op.rd] = ppu_feed_data<u64, Flags...>(ppu, addr);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto LWA()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_feed_data>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	ppu.gpr[op.rd] = ppu_feed_data<s32, Flags...>(ppu, addr);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = (op.simm16 & ~3) + (op.ra ? ppu.gpr[op.ra] : 0);
	PPU_WRITE_64(vm::cast(addr), ppu.gpr[op.rs]);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto STDU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u64 addr = ppu.gpr[op.ra] + (op.simm16 & ~3);
	PPU_WRITE_64(vm::cast(addr), ppu.gpr[op.rs]);
	ppu.gpr[op.ra] = addr;
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FDIVS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] / ppu.fpr[op.frb]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FSUBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] - ppu.fpr[op.frb]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FADDS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] + ppu.fpr[op.frb]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FSQRTS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(std::sqrt(ppu.fpr[op.frb]));
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FRES()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const f64 a = ppu.fpr[op.frb];
		const u64 b = std::bit_cast<u64>(a);
		const u64 e = (b >> 52) & 0x7ff; // double exp
		const u64 i = (b >> 45) & 0x7f; // mantissa LUT index
		const u64 r = e >= (0x3ff + 0x80) ? 0 : (0x7ff - 2 - e) << 52 | u64{ppu_fres_mantissas[i]} << (32 - 3);

		ppu.fpr[op.frd] = f32(std::bit_cast<f64>(a == a ? (b & 0x8000'0000'0000'0000) | r : (0x8'0000'0000'0000 | b)));
		ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMULS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMADDS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = f32(std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]));
	else
		ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]);

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMSUBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = f32(std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]));
	else
		ppu.fpr[op.frd] = f32(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]);

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNMSUBS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = f32(-std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]));
	else
		ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]));

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNMADDS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = f32(-std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]));
	else
		ppu.fpr[op.frd] = f32(-(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]));

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTFSB1()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 bit = op.crbd;
	if (bit < 16 || bit > 19) ppu_log.warning("MTFSB1(%d)", bit);
	ppu.fpscr.bits[bit] = 1;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MCRFS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if (op.crfs != 4) ppu_log.warning("MCRFS(%d)", op.crfs);
	ppu.cr.fields[op.crfd] = ppu.fpscr.fields[op.crfs];
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTFSB0()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 bit = op.crbd;
	if (bit < 16 || bit > 19) ppu_log.warning("MTFSB0(%d)", bit);
	ppu.fpscr.bits[bit] = 0;
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTFSFI()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const u32 bf = op.crfd;

	if (bf != 4)
	{
		// Do nothing on non-FPCC field (TODO)
		ppu_log.warning("MTFSFI(%d)", op.crfd);
	}
	else
	{
		static constexpr std::array<u32, 16> all_values = []() -> std::array<u32, 16>
		{
			std::array<u32, 16> values{};

			for (u32 i = 0; i < values.size(); i++)
			{
				u32 value = 0, im = i;
				value |= (im & 1) << (8 * 3); im >>= 1;
				value |= (im & 1) << (8 * 2); im >>= 1;
				value |= (im & 1) << (8 * 1); im >>= 1;
				value |= (im & 1) << (8 * 0);
				values[i] = value;
			}

			return values;
		}();

		ppu.fpscr.fields[bf] = all_values[op.i];
	}

	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MFFS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu_log.warning("MFFS");
	ppu.fpr[op.frd] = std::bit_cast<f64>(u64{ppu.fpscr.fl} << 15 | u64{ppu.fpscr.fg} << 14 | u64{ppu.fpscr.fe} << 13 | u64{ppu.fpscr.fu} << 12);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto MTFSF()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](auto&& ppu, auto) {
	ppu_log.warning("MTFSF");
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCMPU()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	const f64 a = ppu.fpr[op.fra];
	const f64 b = ppu.fpr[op.frb];
	ppu_set_fpcc<set_fpcc, has_rc, Flags...>(ppu, a, b, op.crfd);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCTIW()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& b)
	{
	#if defined(ARCH_X64)
		const auto val = _mm_set_sd(b);
		const auto res = _mm_xor_si128(_mm_cvtpd_epi32(val), _mm_castpd_si128(_mm_cmpge_pd(val, _mm_set1_pd(0x80000000))));
		d = std::bit_cast<f64, s64>(_mm_cvtsi128_si32(res));
	#elif defined(ARCH_ARM64)
		d = std::bit_cast<f64, s64>(!(b == b) ? INT32_MIN : vqmovnd_s64(std::bit_cast<f64>(vrndi_f64(std::bit_cast<float64x1_t>(b)))));
	#endif
		ppu_set_fpcc<Flags...>(ppu, 0., 0.); // undefined (TODO)
	};

	RETURN_(ppu, ppu.fpr[op.frd], ppu.fpr[op.frb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCTIWZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& b)
	{
	#if defined(ARCH_X64)
		const auto val = _mm_set_sd(b);
		const auto res = _mm_xor_si128(_mm_cvttpd_epi32(val), _mm_castpd_si128(_mm_cmpge_pd(val, _mm_set1_pd(0x80000000))));
		d = std::bit_cast<f64, s64>(_mm_cvtsi128_si32(res));
	#elif defined(ARCH_ARM64)
		d = std::bit_cast<f64, s64>(!(b == b) ? INT32_MIN : vqmovnd_s64(std::bit_cast<s64>(vcvt_s64_f64(std::bit_cast<float64x1_t>(b)))));
	#endif
		ppu_set_fpcc<Flags...>(ppu, 0., 0.); // undefined (TODO)
	};

	RETURN_(ppu, ppu.fpr[op.frd], ppu.fpr[op.frb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FRSP()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = f32(ppu.fpr[op.frb]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FDIV()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.fra] / ppu.fpr[op.frb];
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FSUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.fra] - ppu.fpr[op.frb];
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FADD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.fra] + ppu.fpr[op.frb];
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FSQRT()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = std::sqrt(ppu.fpr[op.frb]);
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FSEL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.fra] >= 0.0 ? ppu.fpr[op.frc] : ppu.fpr[op.frb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMUL()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc];
	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FRSQRTE()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op)
	{
		const u64 b = std::bit_cast<u64>(ppu.fpr[op.frb]);
		ppu.fpr[op.frd] = std::bit_cast<f64>(u64{ppu_frqrte_lut.data[b >> 49]} << 32);
		ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};

	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMSUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]);
	else
		ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb];

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMADD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]);
	else
		ppu.fpr[op.frd] = ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb];

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNMSUB()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = -std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], -ppu.fpr[op.frb]);
	else
		ppu.fpr[op.frd] = -(ppu.fpr[op.fra] * ppu.fpr[op.frc] - ppu.fpr[op.frb]);

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNMADD()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc, use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	if constexpr (((Flags == use_dfma) || ...))
		ppu.fpr[op.frd] = -std::fma(ppu.fpr[op.fra], ppu.fpr[op.frc], ppu.fpr[op.frb]);
	else
		ppu.fpr[op.frd] = -(ppu.fpr[op.fra] * ppu.fpr[op.frc] + ppu.fpr[op.frb]);

	ppu_set_fpcc<Flags...>(ppu, ppu.fpr[op.frd], 0.);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCMPO()
{
	return FCMPU<Build, Flags...>();
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNEG()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<use_dfma>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = -ppu.fpr[op.frb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FMR()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = ppu.fpr[op.frb];
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FNABS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = -std::fabs(ppu.fpr[op.frb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FABS()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	static const auto exec = [](ppu_thread& ppu, ppu_opcode_t op) {
	ppu.fpr[op.frd] = std::fabs(ppu.fpr[op.frb]);
	if constexpr (((Flags == has_rc) || ...))
		ppu_cr_set(ppu, 1, ppu.fpscr.fg, ppu.fpscr.fl, ppu.fpscr.fe, ppu.fpscr.fu);
	};
	RETURN_(ppu, op);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCTID()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& b)
	{
	#if defined(ARCH_X64)
		const auto val = _mm_set_sd(b);
		const auto res = _mm_xor_si128(_mm_set1_epi64x(_mm_cvtsd_si64(val)), _mm_castpd_si128(_mm_cmpge_pd(val, _mm_set1_pd(f64(1ull << 63)))));
		d = std::bit_cast<f64>(_mm_cvtsi128_si64(res));
	#elif defined(ARCH_ARM64)
		d = std::bit_cast<f64, s64>(!(b == b) ? f64{INT64_MIN} : std::bit_cast<f64>(vrndi_f64(std::bit_cast<float64x1_t>(b))));
	#endif
		ppu_set_fpcc<Flags...>(ppu, 0., 0.); // undefined (TODO)
	};

	RETURN_(ppu, ppu.fpr[op.frd], ppu.fpr[op.frb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCTIDZ()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& b)
	{
	#if defined(ARCH_X64)
		const auto val = _mm_set_sd(b);
		const auto res = _mm_xor_si128(_mm_set1_epi64x(_mm_cvttsd_si64(val)), _mm_castpd_si128(_mm_cmpge_pd(val, _mm_set1_pd(f64(1ull << 63)))));
		d = std::bit_cast<f64>(_mm_cvtsi128_si64(res));
	#elif defined(ARCH_ARM64)
		d = std::bit_cast<f64>(!(b == b) ? int64x1_t{INT64_MIN} : vcvt_s64_f64(std::bit_cast<float64x1_t>(b)));
	#endif
		ppu_set_fpcc<Flags...>(ppu, 0., 0.); // undefined (TODO)
	};

	RETURN_(ppu, ppu.fpr[op.frd], ppu.fpr[op.frb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto FCFID()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<set_fpcc>();

	static const auto exec = [](ppu_thread& ppu, auto&& d, auto&& b)
	{
		f64 r = static_cast<f64>(std::bit_cast<s64>(b));
		d = r;
		ppu_set_fpcc<Flags...>(ppu, r, 0.);
	};

	RETURN_(ppu, ppu.fpr[op.frd], ppu.fpr[op.frb]);
}

template <u32 Build, ppu_exec_bit... Flags>
auto UNK()
{
	if constexpr (Build == 0xf1a6)
		return ppu_exec_select<Flags...>::template select<>();

	if constexpr (Build == 0)
	{
		return +[](ppu_thread& ppu, ppu_opcode_t op, be_t<u32>* this_op, ppu_intrp_func*)
		{
			const u32 old_cia = ppu.cia;
			ppu.cia = vm::get_addr(this_op);
			ppu.exec_bytes += ppu.cia - old_cia;

			// HLE function index
			const u32 index = (ppu.cia - g_fxo->get<ppu_function_manager>().addr) / 8;

			const auto& hle_funcs = ppu_function_manager::get();

			if (ppu.cia % 8 == 4 && index < hle_funcs.size())
			{
				return hle_funcs[index](ppu, op, this_op, nullptr);
			}

			fmt::throw_exception("Unknown/Illegal opcode: 0x%08x at 0x%x", op.opcode, ppu.cia);
		};
	}
}

template <typename IT>
struct ppu_interpreter_t
{
	IT MFVSCR;
	IT MTVSCR;
	IT VADDCUW;
	IT VADDFP;
	IT VADDSBS;
	IT VADDSHS;
	IT VADDSWS;
	IT VADDUBM;
	IT VADDUBS;
	IT VADDUHM;
	IT VADDUHS;
	IT VADDUWM;
	IT VADDUWS;
	IT VAND;
	IT VANDC;
	IT VAVGSB;
	IT VAVGSH;
	IT VAVGSW;
	IT VAVGUB;
	IT VAVGUH;
	IT VAVGUW;
	IT VCFSX;
	IT VCFUX;
	IT VCMPBFP;
	IT VCMPBFP_;
	IT VCMPEQFP;
	IT VCMPEQFP_;
	IT VCMPEQUB;
	IT VCMPEQUB_;
	IT VCMPEQUH;
	IT VCMPEQUH_;
	IT VCMPEQUW;
	IT VCMPEQUW_;
	IT VCMPGEFP;
	IT VCMPGEFP_;
	IT VCMPGTFP;
	IT VCMPGTFP_;
	IT VCMPGTSB;
	IT VCMPGTSB_;
	IT VCMPGTSH;
	IT VCMPGTSH_;
	IT VCMPGTSW;
	IT VCMPGTSW_;
	IT VCMPGTUB;
	IT VCMPGTUB_;
	IT VCMPGTUH;
	IT VCMPGTUH_;
	IT VCMPGTUW;
	IT VCMPGTUW_;
	IT VCTSXS;
	IT VCTUXS;
	IT VEXPTEFP;
	IT VLOGEFP;
	IT VMADDFP;
	IT VMAXFP;
	IT VMAXSB;
	IT VMAXSH;
	IT VMAXSW;
	IT VMAXUB;
	IT VMAXUH;
	IT VMAXUW;
	IT VMHADDSHS;
	IT VMHRADDSHS;
	IT VMINFP;
	IT VMINSB;
	IT VMINSH;
	IT VMINSW;
	IT VMINUB;
	IT VMINUH;
	IT VMINUW;
	IT VMLADDUHM;
	IT VMRGHB;
	IT VMRGHH;
	IT VMRGHW;
	IT VMRGLB;
	IT VMRGLH;
	IT VMRGLW;
	IT VMSUMMBM;
	IT VMSUMSHM;
	IT VMSUMSHS;
	IT VMSUMUBM;
	IT VMSUMUHM;
	IT VMSUMUHS;
	IT VMULESB;
	IT VMULESH;
	IT VMULEUB;
	IT VMULEUH;
	IT VMULOSB;
	IT VMULOSH;
	IT VMULOUB;
	IT VMULOUH;
	IT VNMSUBFP;
	IT VNOR;
	IT VOR;
	IT VPERM;
	IT VPKPX;
	IT VPKSHSS;
	IT VPKSHUS;
	IT VPKSWSS;
	IT VPKSWUS;
	IT VPKUHUM;
	IT VPKUHUS;
	IT VPKUWUM;
	IT VPKUWUS;
	IT VREFP;
	IT VRFIM;
	IT VRFIN;
	IT VRFIP;
	IT VRFIZ;
	IT VRLB;
	IT VRLH;
	IT VRLW;
	IT VRSQRTEFP;
	IT VSEL;
	IT VSL;
	IT VSLB;
	IT VSLDOI{};
	IT VSLDOI_[16];
	IT VSLH;
	IT VSLO;
	IT VSLW;
	IT VSPLTB;
	IT VSPLTH;
	IT VSPLTISB;
	IT VSPLTISH;
	IT VSPLTISW;
	IT VSPLTW;
	IT VSR;
	IT VSRAB;
	IT VSRAH;
	IT VSRAW;
	IT VSRB;
	IT VSRH;
	IT VSRO;
	IT VSRW;
	IT VSUBCUW;
	IT VSUBFP;
	IT VSUBSBS;
	IT VSUBSHS;
	IT VSUBSWS;
	IT VSUBUBM;
	IT VSUBUBS;
	IT VSUBUHM;
	IT VSUBUHS;
	IT VSUBUWM;
	IT VSUBUWS;
	IT VSUMSWS;
	IT VSUM2SWS;
	IT VSUM4SBS;
	IT VSUM4SHS;
	IT VSUM4UBS;
	IT VUPKHPX;
	IT VUPKHSB;
	IT VUPKHSH;
	IT VUPKLPX;
	IT VUPKLSB;
	IT VUPKLSH;
	IT VXOR;
	IT TDI;
	IT TWI;
	IT MULLI;
	IT SUBFIC;
	IT CMPLI;
	IT CMPI;
	IT ADDIC;
	IT ADDI;
	IT ADDIS;
	IT BC;
	IT SC;
	IT B;
	IT MCRF;
	IT BCLR;
	IT CRNOR;
	IT CRANDC;
	IT ISYNC;
	IT CRXOR;
	IT CRNAND;
	IT CRAND;
	IT CREQV;
	IT CRORC;
	IT CROR;
	IT BCCTR;
	IT RLWIMI;
	IT RLWINM;
	IT RLWNM;
	IT ORI;
	IT ORIS;
	IT XORI;
	IT XORIS;
	IT ANDI;
	IT ANDIS;
	IT RLDICL;
	IT RLDICR;
	IT RLDIC;
	IT RLDIMI;
	IT RLDCL;
	IT RLDCR;
	IT CMP;
	IT TW;
	IT LVSL;
	IT LVEBX;
	IT SUBFC;
	IT ADDC;
	IT MULHDU;
	IT MULHWU;
	IT MFOCRF{};
	IT MFOCRF_[8];
	IT MFCR; //+
	IT LWARX;
	IT LDX;
	IT LWZX;
	IT SLW;
	IT CNTLZW;
	IT SLD;
	IT AND;
	IT CMPL;
	IT LVSR;
	IT LVEHX;
	IT SUBF;
	IT LDUX;
	IT DCBST;
	IT LWZUX;
	IT CNTLZD;
	IT ANDC;
	IT TD;
	IT LVEWX;
	IT MULHD;
	IT MULHW;
	IT LDARX;
	IT DCBF;
	IT LBZX;
	IT LVX;
	IT NEG;
	IT LBZUX;
	IT NOR;
	IT STVEBX;
	IT SUBFE;
	IT ADDE;
	IT MTOCRF;
	IT STDX;
	IT STWCX;
	IT STWX;
	IT STVEHX;
	IT STDUX;
	IT STWUX;
	IT STVEWX;
	IT SUBFZE;
	IT ADDZE;
	IT STDCX;
	IT STBX;
	IT STVX;
	IT SUBFME;
	IT MULLD;
	IT ADDME;
	IT MULLW;
	IT DCBTST;
	IT STBUX;
	IT ADD;
	IT DCBT;
	IT LHZX;
	IT EQV;
	IT ECIWX;
	IT LHZUX;
	IT XOR;
	IT MFSPR;
	IT LWAX;
	IT DST;
	IT LHAX;
	IT LVXL;
	IT MFTB;
	IT LWAUX;
	IT DSTST;
	IT LHAUX;
	IT STHX;
	IT ORC;
	IT ECOWX;
	IT STHUX;
	IT OR;
	IT DIVDU;
	IT DIVWU;
	IT MTSPR;
	IT DCBI;
	IT NAND;
	IT STVXL;
	IT DIVD;
	IT DIVW;
	IT LVLX;
	IT LDBRX;
	IT LSWX;
	IT LWBRX;
	IT LFSX;
	IT SRW;
	IT SRD;
	IT LVRX;
	IT LSWI;
	IT LFSUX;
	IT SYNC;
	IT LFDX;
	IT LFDUX;
	IT STVLX;
	IT STDBRX;
	IT STSWX;
	IT STWBRX;
	IT STFSX;
	IT STVRX;
	IT STFSUX;
	IT STSWI;
	IT STFDX;
	IT STFDUX;
	IT LVLXL;
	IT LHBRX;
	IT SRAW;
	IT SRAD;
	IT LVRXL;
	IT DSS;
	IT SRAWI;
	IT SRADI;
	IT EIEIO;
	IT STVLXL;
	IT STHBRX;
	IT EXTSH;
	IT STVRXL;
	IT EXTSB;
	IT STFIWX;
	IT EXTSW;
	IT ICBI;
	IT DCBZ;
	IT LWZ;
	IT LWZU;
	IT LBZ;
	IT LBZU;
	IT STW;
	IT STWU;
	IT STB;
	IT STBU;
	IT LHZ;
	IT LHZU;
	IT LHA;
	IT LHAU;
	IT STH;
	IT STHU;
	IT LMW;
	IT STMW;
	IT LFS;
	IT LFSU;
	IT LFD;
	IT LFDU;
	IT STFS;
	IT STFSU;
	IT STFD;
	IT STFDU;
	IT LD;
	IT LDU;
	IT LWA;
	IT STD;
	IT STDU;
	IT FDIVS;
	IT FSUBS;
	IT FADDS;
	IT FSQRTS;
	IT FRES;
	IT FMULS;
	IT FMADDS;
	IT FMSUBS;
	IT FNMSUBS;
	IT FNMADDS;
	IT MTFSB1;
	IT MCRFS;
	IT MTFSB0;
	IT MTFSFI;
	IT MFFS;
	IT MTFSF;
	IT FCMPU;
	IT FRSP;
	IT FCTIW;
	IT FCTIWZ;
	IT FDIV;
	IT FSUB;
	IT FADD;
	IT FSQRT;
	IT FSEL;
	IT FMUL;
	IT FRSQRTE;
	IT FMSUB;
	IT FMADD;
	IT FNMSUB;
	IT FNMADD;
	IT FCMPO;
	IT FNEG;
	IT FMR;
	IT FNABS;
	IT FABS;
	IT FCTID;
	IT FCTIDZ;
	IT FCFID;
	IT UNK;

	IT SUBFCO;
	IT ADDCO;
	IT SUBFO;
	IT NEGO;
	IT SUBFEO;
	IT ADDEO;
	IT SUBFZEO;
	IT ADDZEO;
	IT SUBFMEO;
	IT MULLDO;
	IT ADDMEO;
	IT MULLWO;
	IT ADDO;
	IT DIVDUO;
	IT DIVWUO;
	IT DIVDO;
	IT DIVWO;

	IT SUBFCO_;
	IT ADDCO_;
	IT SUBFO_;
	IT NEGO_;
	IT SUBFEO_;
	IT ADDEO_;
	IT SUBFZEO_;
	IT ADDZEO_;
	IT SUBFMEO_;
	IT MULLDO_;
	IT ADDMEO_;
	IT MULLWO_;
	IT ADDO_;
	IT DIVDUO_;
	IT DIVWUO_;
	IT DIVDO_;
	IT DIVWO_;

	IT RLWIMI_;
	IT RLWINM_;
	IT RLWNM_;
	IT RLDICL_;
	IT RLDICR_;
	IT RLDIC_;
	IT RLDIMI_;
	IT RLDCL_;
	IT RLDCR_;
	IT SUBFC_;
	IT MULHDU_;
	IT ADDC_;
	IT MULHWU_;
	IT SLW_;
	IT CNTLZW_;
	IT SLD_;
	IT AND_;
	IT SUBF_;
	IT CNTLZD_;
	IT ANDC_;
	IT MULHD_;
	IT MULHW_;
	IT NEG_;
	IT NOR_;
	IT SUBFE_;
	IT ADDE_;
	IT SUBFZE_;
	IT ADDZE_;
	IT MULLD_;
	IT SUBFME_;
	IT ADDME_;
	IT MULLW_;
	IT ADD_;
	IT EQV_;
	IT XOR_;
	IT ORC_;
	IT OR_;
	IT DIVDU_;
	IT DIVWU_;
	IT NAND_;
	IT DIVD_;
	IT DIVW_;
	IT SRW_;
	IT SRD_;
	IT SRAW_;
	IT SRAD_;
	IT SRAWI_;
	IT SRADI_;
	IT EXTSH_;
	IT EXTSB_;
	IT EXTSW_;
	IT FDIVS_;
	IT FSUBS_;
	IT FADDS_;
	IT FSQRTS_;
	IT FRES_;
	IT FMULS_;
	IT FMADDS_;
	IT FMSUBS_;
	IT FNMSUBS_;
	IT FNMADDS_;
	IT MTFSB1_;
	IT MTFSB0_;
	IT MTFSFI_;
	IT MFFS_;
	IT MTFSF_;
	IT FRSP_;
	IT FCTIW_;
	IT FCTIWZ_;
	IT FDIV_;
	IT FSUB_;
	IT FADD_;
	IT FSQRT_;
	IT FSEL_;
	IT FMUL_;
	IT FRSQRTE_;
	IT FMSUB_;
	IT FMADD_;
	IT FNMSUB_;
	IT FNMADD_;
	IT FNEG_;
	IT FMR_;
	IT FNABS_;
	IT FABS_;
	IT FCTID_;
	IT FCTIDZ_;
	IT FCFID_;

	/* Optimized variants */
};

ppu_interpreter_rt_base::ppu_interpreter_rt_base() noexcept
{
	// Obtain required set of flags from settings
	bs_t<ppu_exec_bit> selected{};
	if (g_cfg.core.ppu_set_sat_bit)
		selected += set_sat;
	if (g_cfg.core.ppu_use_nj_bit)
		selected += use_nj + fix_nj;
	if (g_cfg.core.ppu_llvm_nj_fixup)
		selected += fix_nj;
	if (g_cfg.core.ppu_set_vnan)
		selected += set_vnan + fix_vnan;
	if (g_cfg.core.ppu_fix_vnan)
		selected += fix_vnan;
	if (g_cfg.core.ppu_set_fpcc)
		selected += set_fpcc;
	if (g_cfg.core.use_accurate_dfma)
		selected += use_dfma;
	if (g_cfg.core.ppu_debug)
		selected += set_cr_stats; // TODO
	if (g_cfg.core.ppu_call_history)
		selected += set_call_history;
	if (g_cfg.core.ppu_128_reservations_loop_max_length != 0)
		selected += use_feed_data;

	if (selected & use_nj)
		ppu_log.success("Enabled: Accurate Non-Java Mode");
	else if (selected & fix_nj)
		ppu_log.success("Enabled: Non-Java Mode Fixup");
	if (selected & set_vnan)
		ppu_log.success("Enabled: Accurate VNAN");
	else if (selected & fix_vnan)
		ppu_log.success("Enabled: VNAN Fixup");
	if (selected & set_sat)
		ppu_log.success("Enabled: Accurate SAT");
	if (selected & set_fpcc)
		ppu_log.success("Enabled: Accurate FPCC");

	ptrs = std::make_unique<decltype(ptrs)::element_type>();

#ifndef __INTELLISENSE__

#define INIT_VCMP(name) \
	ptrs->name = ::name<0>(); \
	ptrs->name##_ = ::name<0, has_oe>(); \

#define INIT_OV(name) \
	ptrs->name = ::name<0>(); \
	ptrs->name##O = ::name<0, has_oe>(); \

#define INIT_RC(name) \
	ptrs->name = ::name<0xf1a6>()(selected, []<ppu_exec_bit... Flags>() { \
		return ::name<0, Flags...>(); \
	}); \
	ptrs->name##_ = ::name<0xf1a6, set_fpcc>()(selected, []<ppu_exec_bit... Flags>() { \
		/* Minor optimization: has_rc implies set_fpcc so don't compile has_rc alone */ \
		return ::name<0, has_rc, Flags...>(); \
	}); \

#define INIT_RC_OV(name) \
	ptrs->name = ::name<0>(); \
	ptrs->name##O = ::name<0, has_oe>(); \
	ptrs->name##_ = ::name<0, has_rc>(); \
	ptrs->name##O_ = ::name<0, has_oe, has_rc>(); \

	// Initialize instructions with their own sets of supported flags (except INIT_VCMP, INIT_OV, INIT_RC_OV)
#define INIT(name) \
	ptrs->name = ::name<0xf1a6>()(selected, []<ppu_exec_bit... Flags>() { \
		return ::name<0, Flags...>(); \
	}); \

#define INIT_ONE(name, bits) \
	ptrs->name##_[0b##bits] = ::name<0b##bits>::select(selected, []<ppu_exec_bit... Flags>() { \
		return ::name<0b##bits>::impl<0, Flags...>(); \
	}); \

#define INIT_PACK2(name, bits) \
	INIT_ONE(name, bits##0) \
	INIT_ONE(name, bits##1) \

#define INIT_PACK4(name, bits) \
	INIT_PACK2(name, bits##0) \
	INIT_PACK2(name, bits##1) \

#define INIT_PACK8(name, bits) \
	INIT_PACK4(name, bits##0) \
	INIT_PACK4(name, bits##1) \

#define INIT_PACK16(name, bits) \
	INIT_PACK8(name, bits##0) \
	INIT_PACK8(name, bits##1) \

	INIT(MFVSCR);
	INIT(MTVSCR);
	INIT(VADDCUW);
	INIT(VADDFP);
	INIT(VADDSBS);
	INIT(VADDSHS);
	INIT(VADDSWS);
	INIT(VADDUBM);
	INIT(VADDUBS);
	INIT(VADDUHM);
	INIT(VADDUHS);
	INIT(VADDUWM);
	INIT(VADDUWS);
	INIT(VAND);
	INIT(VANDC);
	INIT(VAVGSB);
	INIT(VAVGSH);
	INIT(VAVGSW);
	INIT(VAVGUB);
	INIT(VAVGUH);
	INIT(VAVGUW);
	INIT(VCFSX);
	INIT(VCFUX);
	INIT_VCMP(VCMPBFP);
	INIT_VCMP(VCMPEQFP);
	INIT_VCMP(VCMPEQUB);
	INIT_VCMP(VCMPEQUH);
	INIT_VCMP(VCMPEQUW);
	INIT_VCMP(VCMPGEFP);
	INIT_VCMP(VCMPGTFP);
	INIT_VCMP(VCMPGTSB);
	INIT_VCMP(VCMPGTSH);
	INIT_VCMP(VCMPGTSW);
	INIT_VCMP(VCMPGTUB);
	INIT_VCMP(VCMPGTUH);
	INIT_VCMP(VCMPGTUW);
	INIT(VCTSXS);
	INIT(VCTUXS);
	INIT(VEXPTEFP);
	INIT(VLOGEFP);
	INIT(VMADDFP);
	INIT(VMAXFP);
	INIT(VMAXSB);
	INIT(VMAXSH);
	INIT(VMAXSW);
	INIT(VMAXUB);
	INIT(VMAXUH);
	INIT(VMAXUW);
	INIT(VMHADDSHS);
	INIT(VMHRADDSHS);
	INIT(VMINFP);
	INIT(VMINSB);
	INIT(VMINSH);
	INIT(VMINSW);
	INIT(VMINUB);
	INIT(VMINUH);
	INIT(VMINUW);
	INIT(VMLADDUHM);
	INIT(VMRGHB);
	INIT(VMRGHH);
	INIT(VMRGHW);
	INIT(VMRGLB);
	INIT(VMRGLH);
	INIT(VMRGLW);
	INIT(VMSUMMBM);
	INIT(VMSUMSHM);
	INIT(VMSUMSHS);
	INIT(VMSUMUBM);
	INIT(VMSUMUHM);
	INIT(VMSUMUHS);
	INIT(VMULESB);
	INIT(VMULESH);
	INIT(VMULEUB);
	INIT(VMULEUH);
	INIT(VMULOSB);
	INIT(VMULOSH);
	INIT(VMULOUB);
	INIT(VMULOUH);
	INIT(VNMSUBFP);
	INIT(VNOR);
	INIT(VOR);
	INIT(VPERM);
	INIT(VPKPX);
	INIT(VPKSHSS);
	INIT(VPKSHUS);
	INIT(VPKSWSS);
	INIT(VPKSWUS);
	INIT(VPKUHUM);
	INIT(VPKUHUS);
	INIT(VPKUWUM);
	INIT(VPKUWUS);
	INIT(VREFP);
	INIT(VRFIM);
	INIT(VRFIN);
	INIT(VRFIP);
	INIT(VRFIZ);
	INIT(VRLB);
	INIT(VRLH);
	INIT(VRLW);
	INIT(VRSQRTEFP);
	INIT(VSEL);
	INIT(VSL);
	INIT(VSLB);
	INIT_PACK16(VSLDOI,);
	INIT(VSLH);
	INIT(VSLO);
	INIT(VSLW);
	INIT(VSPLTB);
	INIT(VSPLTH);
	INIT(VSPLTISB);
	INIT(VSPLTISH);
	INIT(VSPLTISW);
	INIT(VSPLTW);
	INIT(VSR);
	INIT(VSRAB);
	INIT(VSRAH);
	INIT(VSRAW);
	INIT(VSRB);
	INIT(VSRH);
	INIT(VSRO);
	INIT(VSRW);
	INIT(VSUBCUW);
	INIT(VSUBFP);
	INIT(VSUBSBS);
	INIT(VSUBSHS);
	INIT(VSUBSWS);
	INIT(VSUBUBM);
	INIT(VSUBUBS);
	INIT(VSUBUHM);
	INIT(VSUBUHS);
	INIT(VSUBUWM);
	INIT(VSUBUWS);
	INIT(VSUMSWS);
	INIT(VSUM2SWS);
	INIT(VSUM4SBS);
	INIT(VSUM4SHS);
	INIT(VSUM4UBS);
	INIT(VUPKHPX);
	INIT(VUPKHSB);
	INIT(VUPKHSH);
	INIT(VUPKLPX);
	INIT(VUPKLSB);
	INIT(VUPKLSH);
	INIT(VXOR);
	INIT(TDI);
	INIT(TWI);
	INIT(MULLI);
	INIT(SUBFIC);
	INIT(CMPLI);
	INIT(CMPI);
	INIT(ADDIC);
	INIT(ADDI);
	INIT(ADDIS);
	INIT(BC);
	INIT(SC);
	INIT(B);
	INIT(MCRF);
	INIT(BCLR);
	INIT(CRNOR);
	INIT(CRANDC);
	INIT(ISYNC);
	INIT(CRXOR);
	INIT(CRNAND);
	INIT(CRAND);
	INIT(CREQV);
	INIT(CRORC);
	INIT(CROR);
	INIT(BCCTR);
	INIT_RC(RLWIMI);
	INIT_RC(RLWINM);
	INIT_RC(RLWNM);
	INIT(ORI);
	INIT(ORIS);
	INIT(XORI);
	INIT(XORIS);
	INIT(ANDI);
	INIT(ANDIS);
	INIT_RC(RLDICL);
	INIT_RC(RLDICR);
	INIT_RC(RLDIC);
	INIT_RC(RLDIMI);
	INIT_RC(RLDCL);
	INIT_RC(RLDCR);
	INIT(CMP);
	INIT(TW);
	INIT(LVSL);
	INIT(LVEBX);
	INIT_RC_OV(SUBFC);
	INIT_RC_OV(ADDC);
	INIT_RC(MULHDU);
	INIT_RC(MULHWU);
	INIT_PACK8(MFOCRF,);
	INIT(MFCR); //+
	INIT(LWARX);
	INIT(LDX);
	INIT(LWZX);
	INIT_RC(SLW);
	INIT_RC(CNTLZW);
	INIT_RC(SLD);
	INIT_RC(AND);
	INIT(CMPL);
	INIT(LVSR);
	INIT(LVEHX);
	INIT_RC_OV(SUBF);
	INIT(LDUX);
	INIT(DCBST);
	INIT(LWZUX);
	INIT_RC(CNTLZD);
	INIT_RC(ANDC);
	INIT(TD);
	INIT(LVEWX);
	INIT_RC(MULHD);
	INIT_RC(MULHW);
	INIT(LDARX);
	INIT(DCBF);
	INIT(LBZX);
	INIT(LVX);
	INIT_RC_OV(NEG);
	INIT(LBZUX);
	INIT_RC(NOR);
	INIT(STVEBX);
	INIT_OV(SUBFE);
	INIT_OV(ADDE);
	INIT(MTOCRF);
	INIT(STDX);
	INIT(STWCX);
	INIT(STWX);
	INIT(STVEHX);
	INIT(STDUX);
	INIT(STWUX);
	INIT(STVEWX);
	INIT_RC_OV(SUBFZE);
	INIT_RC_OV(ADDZE);
	INIT(STDCX);
	INIT(STBX);
	INIT(STVX);
	INIT_RC_OV(SUBFME);
	INIT_RC_OV(MULLD);
	INIT_RC_OV(ADDME);
	INIT_RC_OV(MULLW);
	INIT(DCBTST);
	INIT(STBUX);
	INIT_RC_OV(ADD);
	INIT(DCBT);
	INIT(LHZX);
	INIT_RC(EQV);
	INIT(ECIWX);
	INIT(LHZUX);
	INIT_RC(XOR);
	INIT(MFSPR);
	INIT(LWAX);
	INIT(DST);
	INIT(LHAX);
	INIT(LVXL);
	INIT(MFTB);
	INIT(LWAUX);
	INIT(DSTST);
	INIT(LHAUX);
	INIT(STHX);
	INIT_RC(ORC);
	INIT(ECOWX);
	INIT(STHUX);
	INIT_RC(OR);
	INIT_RC_OV(DIVDU);
	INIT_RC_OV(DIVWU);
	INIT(MTSPR);
	INIT(DCBI);
	INIT_RC(NAND);
	INIT(STVXL);
	INIT_RC_OV(DIVD);
	INIT_RC_OV(DIVW);
	INIT(LVLX);
	INIT(LDBRX);
	INIT(LSWX);
	INIT(LWBRX);
	INIT(LFSX);
	INIT_RC(SRW);
	INIT_RC(SRD);
	INIT(LVRX);
	INIT(LSWI);
	INIT(LFSUX);
	INIT(SYNC);
	INIT(LFDX);
	INIT(LFDUX);
	INIT(STVLX);
	INIT(STDBRX);
	INIT(STSWX);
	INIT(STWBRX);
	INIT(STFSX);
	INIT(STVRX);
	INIT(STFSUX);
	INIT(STSWI);
	INIT(STFDX);
	INIT(STFDUX);
	INIT(LVLXL);
	INIT(LHBRX);
	INIT_RC(SRAW);
	INIT_RC(SRAD);
	INIT(LVRXL);
	INIT(DSS);
	INIT_RC(SRAWI);
	INIT_RC(SRADI);
	INIT(EIEIO);
	INIT(STVLXL);
	INIT(STHBRX);
	INIT_RC(EXTSH);
	INIT(STVRXL);
	INIT_RC(EXTSB);
	INIT(STFIWX);
	INIT_RC(EXTSW);
	INIT(ICBI);
	INIT(DCBZ);
	INIT(LWZ);
	INIT(LWZU);
	INIT(LBZ);
	INIT(LBZU);
	INIT(STW);
	INIT(STWU);
	INIT(STB);
	INIT(STBU);
	INIT(LHZ);
	INIT(LHZU);
	INIT(LHA);
	INIT(LHAU);
	INIT(STH);
	INIT(STHU);
	INIT(LMW);
	INIT(STMW);
	INIT(LFS);
	INIT(LFSU);
	INIT(LFD);
	INIT(LFDU);
	INIT(STFS);
	INIT(STFSU);
	INIT(STFD);
	INIT(STFDU);
	INIT(LD);
	INIT(LDU);
	INIT(LWA);
	INIT(STD);
	INIT(STDU);
	INIT_RC(FDIVS);
	INIT_RC(FSUBS);
	INIT_RC(FADDS);
	INIT_RC(FSQRTS);
	INIT_RC(FRES);
	INIT_RC(FMULS);
	INIT_RC(FMADDS);
	INIT_RC(FMSUBS);
	INIT_RC(FNMSUBS);
	INIT_RC(FNMADDS);
	INIT_RC(MTFSB1);
	INIT(MCRFS);
	INIT_RC(MTFSB0);
	INIT_RC(MTFSFI);
	INIT_RC(MFFS);
	INIT_RC(MTFSF);
	INIT(FCMPU);
	INIT_RC(FRSP);
	INIT_RC(FCTIW);
	INIT_RC(FCTIWZ);
	INIT_RC(FDIV);
	INIT_RC(FSUB);
	INIT_RC(FADD);
	INIT_RC(FSQRT);
	INIT_RC(FSEL);
	INIT_RC(FMUL);
	INIT_RC(FRSQRTE);
	INIT_RC(FMSUB);
	INIT_RC(FMADD);
	INIT_RC(FNMSUB);
	INIT_RC(FNMADD);
	INIT(FCMPO);
	INIT_RC(FNEG);
	INIT_RC(FMR);
	INIT_RC(FNABS);
	INIT_RC(FABS);
	INIT_RC(FCTID);
	INIT_RC(FCTIDZ);
	INIT_RC(FCFID);
	INIT(UNK);
#endif
}

ppu_interpreter_rt_base::~ppu_interpreter_rt_base()
{
}

ppu_interpreter_rt::ppu_interpreter_rt() noexcept
	: ppu_interpreter_rt_base()
	, table(*ptrs)
{
}

ppu_intrp_func_t ppu_interpreter_rt::decode(u32 opv) const noexcept
{
	const auto op = ppu_opcode_t{opv};

	switch (g_ppu_itype.decode(opv))
	{
	case ppu_itype::LWZ:
	case ppu_itype::LBZ:
	case ppu_itype::STW:
	case ppu_itype::STB:
	case ppu_itype::LHZ:
	case ppu_itype::LHA:
	case ppu_itype::STH:
	case ppu_itype::LFS:
	case ppu_itype::LFD:
	case ppu_itype::STFS:
	case ppu_itype::STFD:
	{
		// Minor optimization: 16-bit absolute addressing never points to a valid memory
		if (!op.ra)
		{
			return [](ppu_thread&, ppu_opcode_t op, be_t<u32>*, ppu_intrp_func*)
			{
				fmt::throw_exception("Invalid instruction: %s r%d,0x%016x(r0)", g_ppu_iname.decode(op.opcode), op.rd, op.simm16);
			};
		}

		break;
	}
	case ppu_itype::VSLDOI: return ptrs->VSLDOI_[op.vsh];
	case ppu_itype::MFOCRF:
	{
		if (op.l11)
		{
			const u32 n = std::countl_zero<u32>(op.crm) & 7;

			if (0x80u >> n != op.crm)
			{
				return [](ppu_thread&, ppu_opcode_t op, be_t<u32>*, ppu_intrp_func*)
				{
					fmt::throw_exception("Invalid instruction: MFOCRF with bits 0x%x", op.crm);
				};
			}

			return ptrs->MFOCRF_[n];
		}

		return ptrs->MFCR;
	}
	default: break;
	}

	return table.decode(opv);
}
