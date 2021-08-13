#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/date_time.h"
#include "Emu/Memory/vm.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Memory/vm_reservation.h"

#include "Loader/ELF.h"
#include "Emu/VFS.h"
#include "Emu/IdManager.h"
#include "Emu/perf_meter.hpp"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_interrupt.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/timers.hpp"

#include <cmath>
#include <cfenv>
#include <thread>
#include <shared_mutex>
#include "util/vm.hpp"
#include "util/asm.hpp"
#include "util/v128.hpp"
#include "util/v128sse.hpp"
#include "util/sysinfo.hpp"

using spu_rdata_t = decltype(spu_thread::rdata);

template <>
void fmt_class_string<mfc_atomic_status>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mfc_atomic_status arg)
	{
		switch (arg)
		{
		case MFC_PUTLLC_SUCCESS: return "PUTLLC";
		case MFC_PUTLLC_FAILURE: return "PUTLLC-FAIL";
		case MFC_PUTLLUC_SUCCESS: return "PUTLLUC";
		case MFC_GETLLAR_SUCCESS: return "GETLLAR";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<mfc_tag_update>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mfc_tag_update arg)
	{
		switch (arg)
		{
		case MFC_TAG_UPDATE_IMMEDIATE: return "empty";
		case MFC_TAG_UPDATE_ANY: return "ANY";
		case MFC_TAG_UPDATE_ALL: return "ALL";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<spu_type>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](spu_type arg)
	{
		switch (arg)
		{
		case spu_type::threaded: return "Threaded";
		case spu_type::raw: return "Raw";
		case spu_type::isolated: return "Isolated";
		}

		return unknown;
	});
}

// Verify AVX availability for TSX transactions
static const bool s_tsx_avx = utils::has_avx();

// For special case
static const bool s_tsx_haswell = utils::has_rtm() && !utils::has_mpx();

static FORCE_INLINE bool cmp_rdata_avx(const __m256i* lhs, const __m256i* rhs)
{
#if defined(_MSC_VER) || defined(__AVX__)
	const __m256 x0 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 0)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 0)));
	const __m256 x1 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 1)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 1)));
	const __m256 x2 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 2)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 2)));
	const __m256 x3 = _mm256_xor_ps(_mm256_castsi256_ps(_mm256_loadu_si256(lhs + 3)), _mm256_castsi256_ps(_mm256_loadu_si256(rhs + 3)));
	const __m256 c0 = _mm256_or_ps(x0, x1);
	const __m256 c1 = _mm256_or_ps(x2, x3);
	const __m256 c2 = _mm256_or_ps(c0, c1);
	return _mm256_testz_si256(_mm256_castps_si256(c2), _mm256_castps_si256(c2)) != 0;
#else
	bool result = 0;
	__asm__(
		"vmovups 0*32(%[lhs]), %%ymm0;" // load
		"vmovups 1*32(%[lhs]), %%ymm1;"
		"vmovups 2*32(%[lhs]), %%ymm2;"
		"vmovups 3*32(%[lhs]), %%ymm3;"
		"vxorps 0*32(%[rhs]), %%ymm0, %%ymm0;" // compare
		"vxorps 1*32(%[rhs]), %%ymm1, %%ymm1;"
		"vxorps 2*32(%[rhs]), %%ymm2, %%ymm2;"
		"vxorps 3*32(%[rhs]), %%ymm3, %%ymm3;"
		"vorps %%ymm0, %%ymm1, %%ymm0;" // merge
		"vorps %%ymm2, %%ymm3, %%ymm2;"
		"vorps %%ymm0, %%ymm2, %%ymm0;"
		"vptest %%ymm0, %%ymm0;" // test
		"vzeroupper"
		: "=@ccz" (result)
		: [lhs] "r" (lhs)
		, [rhs] "r" (rhs)
		: "cc" // Clobber flags
		, "xmm0" // Clobber registers ymm0-ymm3 (see mov_rdata_avx)
		, "xmm1"
		, "xmm2"
		, "xmm3"
	);
	return result;
#endif
}

#ifdef _MSC_VER
__forceinline
#endif
extern bool cmp_rdata(const spu_rdata_t& _lhs, const spu_rdata_t& _rhs)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		return cmp_rdata_avx(reinterpret_cast<const __m256i*>(_lhs), reinterpret_cast<const __m256i*>(_rhs));
	}

	const auto lhs = reinterpret_cast<const v128*>(_lhs);
	const auto rhs = reinterpret_cast<const v128*>(_rhs);
	const v128 a = (lhs[0] ^ rhs[0]) | (lhs[1] ^ rhs[1]);
	const v128 b = (lhs[2] ^ rhs[2]) | (lhs[3] ^ rhs[3]);
	const v128 c = (lhs[4] ^ rhs[4]) | (lhs[5] ^ rhs[5]);
	const v128 d = (lhs[6] ^ rhs[6]) | (lhs[7] ^ rhs[7]);
	const v128 r = (a | b) | (c | d);
	return r == v128{};
}

static FORCE_INLINE void mov_rdata_avx(__m256i* dst, const __m256i* src)
{
#ifdef _MSC_VER
	_mm256_storeu_si256(dst + 0, _mm256_loadu_si256(src + 0));
	_mm256_storeu_si256(dst + 1, _mm256_loadu_si256(src + 1));
	_mm256_storeu_si256(dst + 2, _mm256_loadu_si256(src + 2));
	_mm256_storeu_si256(dst + 3, _mm256_loadu_si256(src + 3));
#else
	__asm__(
		"vmovdqu 0*32(%[src]), %%ymm0;" // load
		"vmovdqu %%ymm0, 0*32(%[dst]);" // store
		"vmovdqu 1*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 1*32(%[dst]);"
		"vmovdqu 2*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 2*32(%[dst]);"
		"vmovdqu 3*32(%[src]), %%ymm0;"
		"vmovdqu %%ymm0, 3*32(%[dst]);"
#ifndef __AVX__
		"vzeroupper" // Don't need in AVX mode (should be emitted automatically)
#endif
		:
		: [src] "r" (src)
		, [dst] "r" (dst)
#ifdef __AVX__
		: "ymm0" // Clobber ymm0 register (acknowledge its modification)
#else
		: "xmm0" // ymm0 is "unknown" if not compiled in AVX mode, so clobber xmm0 only
#endif
	);
#endif
}

#ifdef _MSC_VER
__forceinline
#endif
extern void mov_rdata(spu_rdata_t& _dst, const spu_rdata_t& _src)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_avx(reinterpret_cast<__m256i*>(_dst), reinterpret_cast<const __m256i*>(_src));
		return;
	}

	{
		const __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 0));
		const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 16));
		const __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 32));
		const __m128i v3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 48));
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 0), v0);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 16), v1);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 32), v2);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 48), v3);
	}

	const __m128i v0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 64));
	const __m128i v1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 80));
	const __m128i v2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 96));
	const __m128i v3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(_src + 112));
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 64), v0);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 80), v1);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 96), v2);
	_mm_storeu_si128(reinterpret_cast<__m128i*>(_dst + 112), v3);
}

static FORCE_INLINE void mov_rdata_nt_avx(__m256i* dst, const __m256i* src)
{
#ifdef _MSC_VER
	_mm256_stream_si256(dst + 0, _mm256_load_si256(src + 0));
	_mm256_stream_si256(dst + 1, _mm256_load_si256(src + 1));
	_mm256_stream_si256(dst + 2, _mm256_load_si256(src + 2));
	_mm256_stream_si256(dst + 3, _mm256_load_si256(src + 3));
#else
	__asm__(
		"vmovdqa 0*32(%[src]), %%ymm0;" // load
		"vmovntdq %%ymm0, 0*32(%[dst]);" // store
		"vmovdqa 1*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 1*32(%[dst]);"
		"vmovdqa 2*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 2*32(%[dst]);"
		"vmovdqa 3*32(%[src]), %%ymm0;"
		"vmovntdq %%ymm0, 3*32(%[dst]);"
#ifndef __AVX__
		"vzeroupper" // Don't need in AVX mode (should be emitted automatically)
#endif
		:
		: [src] "r" (src)
		, [dst] "r" (dst)
#ifdef __AVX__
		: "ymm0" // Clobber ymm0 register (acknowledge its modification)
#else
		: "xmm0" // ymm0 is "unknown" if not compiled in AVX mode, so clobber xmm0 only
#endif
	);
#endif
}

extern void mov_rdata_nt(spu_rdata_t& _dst, const spu_rdata_t& _src)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_nt_avx(reinterpret_cast<__m256i*>(_dst), reinterpret_cast<const __m256i*>(_src));
		return;
	}

	{
		const __m128i v0 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 0));
		const __m128i v1 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 16));
		const __m128i v2 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 32));
		const __m128i v3 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 48));
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 0), v0);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 16), v1);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 32), v2);
		_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 48), v3);
	}

	const __m128i v0 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 64));
	const __m128i v1 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 80));
	const __m128i v2 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 96));
	const __m128i v3 = _mm_load_si128(reinterpret_cast<const __m128i*>(_src + 112));
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 64), v0);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 80), v1);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 96), v2);
	_mm_stream_si128(reinterpret_cast<__m128i*>(_dst + 112), v3);
}

void do_cell_atomic_128_store(u32 addr, const void* to_write);

extern thread_local u64 g_tls_fault_spu;

const spu_decoder<spu_itype> s_spu_itype;

namespace spu
{
	namespace scheduler
	{
		std::array<atomic_t<u8>, 65536> atomic_instruction_table = {};
		constexpr u32 native_jiffy_duration_us = 1500; //About 1ms resolution with a half offset

		void acquire_pc_address(spu_thread& spu, u32 pc, u32 timeout_ms, u32 max_concurrent_instructions)
		{
			const u32 pc_offset = pc >> 2;

			if (atomic_instruction_table[pc_offset].observe() >= max_concurrent_instructions)
			{
				spu.state += cpu_flag::wait + cpu_flag::temp;

				if (timeout_ms > 0)
				{
					const u64 timeout = timeout_ms * 1000u; //convert to microseconds
					const u64 start = get_system_time();
					auto remaining = timeout;

					while (atomic_instruction_table[pc_offset].observe() >= max_concurrent_instructions)
					{
						if (remaining >= native_jiffy_duration_us)
							std::this_thread::sleep_for(1ms);
						else
							std::this_thread::yield();

						const auto now = get_system_time();
						const auto elapsed = now - start;

						if (elapsed > timeout) break;
						remaining = timeout - elapsed;
					}
				}
				else
				{
					//Slight pause if function is overburdened
					const auto count = atomic_instruction_table[pc_offset].observe() * 100ull;
					busy_wait(count);
				}

				ensure(!spu.check_state());
			}

			atomic_instruction_table[pc_offset]++;
		}

		void release_pc_address(u32 pc)
		{
			const u32 pc_offset = pc >> 2;

			atomic_instruction_table[pc_offset]--;
		}

		struct concurrent_execution_watchdog
		{
			u32 pc = 0;
			bool active = false;

			concurrent_execution_watchdog(spu_thread& spu)
				:pc(spu.pc)
			{
				if (const u32 max_concurrent_instructions = g_cfg.core.preferred_spu_threads)
				{
					acquire_pc_address(spu, pc, g_cfg.core.spu_delay_penalty, max_concurrent_instructions);
					active = true;
				}
			}

			~concurrent_execution_watchdog()
			{
				if (active)
					release_pc_address(pc);
			}
		};
	}
}

std::array<u32, 2> op_branch_targets(u32 pc, spu_opcode_t op)
{
	std::array<u32, 2> res{spu_branch_target(pc + 4), umax};

	switch (const auto type = s_spu_itype.decode(op.opcode))
	{
	case spu_itype::BR:
	case spu_itype::BRA:
	case spu_itype::BRNZ:
	case spu_itype::BRZ:
	case spu_itype::BRHNZ:
	case spu_itype::BRHZ:
	case spu_itype::BRSL:
	case spu_itype::BRASL:
	{
		const int index = (type == spu_itype::BR || type == spu_itype::BRA || type == spu_itype::BRSL || type == spu_itype::BRASL ? 0 : 1);
		res[index] = (spu_branch_target(type == spu_itype::BRASL || type == spu_itype::BRA ? 0 : pc, op.i16));
		break;
	}
	case spu_itype::IRET:
	case spu_itype::BI:
	case spu_itype::BISLED:
	case spu_itype::BISL:
	case spu_itype::BIZ:
	case spu_itype::BINZ:
	case spu_itype::BIHZ:
	case spu_itype::BIHNZ: // TODO (detect constant address branches, such as for interrupts enable/disable pattern)

	case spu_itype::UNK:
	{
		res[0] = umax;
		break;
	}
	default: break;
	}

	return res;
}

const auto spu_putllc_tx = build_function_asm<u64(*)(u32 raddr, u64 rtime, void* _old, const void* _new)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();
	Label _ret = c.newLabel();
	Label skip = c.newLabel();
	Label next = c.newLabel();
	Label load = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.sub(x86::rsp, 168);
#ifdef _WIN32
	if (s_tsx_avx)
	{
		c.vmovups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.vmovups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
	else
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
		c.movups(x86::oword_ptr(x86::rsp, 32), x86::xmm8);
		c.movups(x86::oword_ptr(x86::rsp, 48), x86::xmm9);
		c.movups(x86::oword_ptr(x86::rsp, 64), x86::xmm10);
		c.movups(x86::oword_ptr(x86::rsp, 80), x86::xmm11);
		c.movups(x86::oword_ptr(x86::rsp, 96), x86::xmm12);
		c.movups(x86::oword_ptr(x86::rsp, 112), x86::xmm13);
		c.movups(x86::oword_ptr(x86::rsp, 128), x86::xmm14);
		c.movups(x86::oword_ptr(x86::rsp, 144), x86::xmm15);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r12);
	c.mov(x86::rbp, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_sudo_addr)));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(reinterpret_cast<u64>(+vm::g_reservations), args[0]));
	c.prefetchw(x86::byte_ptr(x86::rbx));
	c.mov(x86::r13, args[1]);

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::yword_ptr(args[2], 0));
		c.vmovups(x86::ymm1, x86::yword_ptr(args[2], 32));
		c.vmovups(x86::ymm2, x86::yword_ptr(args[2], 64));
		c.vmovups(x86::ymm3, x86::yword_ptr(args[2], 96));
		c.vmovups(x86::ymm4, x86::yword_ptr(args[3], 0));
		c.vmovups(x86::ymm5, x86::yword_ptr(args[3], 32));
		c.vmovups(x86::ymm6, x86::yword_ptr(args[3], 64));
		c.vmovups(x86::ymm7, x86::yword_ptr(args[3], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[2], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[2], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[2], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[2], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[2], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[2], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[2], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[2], 112));
		c.movaps(x86::xmm8, x86::oword_ptr(args[3], 0));
		c.movaps(x86::xmm9, x86::oword_ptr(args[3], 16));
		c.movaps(x86::xmm10, x86::oword_ptr(args[3], 32));
		c.movaps(x86::xmm11, x86::oword_ptr(args[3], 48));
		c.movaps(x86::xmm12, x86::oword_ptr(args[3], 64));
		c.movaps(x86::xmm13, x86::oword_ptr(args[3], 80));
		c.movaps(x86::xmm14, x86::oword_ptr(args[3], 96));
		c.movaps(x86::xmm15, x86::oword_ptr(args[3], 112));
	}

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	const auto stamp1 = args[1];
	build_get_tsc(c, stamp0);

	// Begin transaction
	Label tx0 = build_transaction_enter(c, fall, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx) - ::offset32(&spu_thread::rdata)), 1);
		build_get_tsc(c, stamp1);
		c.sub(stamp1, stamp0);
		c.xor_(x86::eax, x86::eax);
		c.cmp(stamp1, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit1)));
		c.jae(fall);
	});
	c.bt(x86::dword_ptr(args[2], ::offset32(&spu_thread::state) - ::offset32(&spu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.xbegin(tx0);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.test(x86::eax, vm::rsrv_unique_lock);
	c.jnz(skip);
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::yword_ptr(x86::rbp, 96));
		c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
		c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
		c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
		c.vptest(x86::ymm0, x86::ymm0);
	}
	else
	{
		c.xorps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.xorps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.xorps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.xorps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.xorps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.xorps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.xorps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.xorps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
		c.orps(x86::xmm0, x86::xmm1);
		c.orps(x86::xmm2, x86::xmm3);
		c.orps(x86::xmm4, x86::xmm5);
		c.orps(x86::xmm6, x86::xmm7);
		c.orps(x86::xmm0, x86::xmm2);
		c.orps(x86::xmm4, x86::xmm6);
		c.orps(x86::xmm0, x86::xmm4);
		c.ptest(x86::xmm0, x86::xmm0);
	}

	c.jnz(fail);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(x86::rbp, 0), x86::ymm4);
		c.vmovaps(x86::yword_ptr(x86::rbp, 32), x86::ymm5);
		c.vmovaps(x86::yword_ptr(x86::rbp, 64), x86::ymm6);
		c.vmovaps(x86::yword_ptr(x86::rbp, 96), x86::ymm7);
	}
	else
	{
		c.movaps(x86::oword_ptr(x86::rbp, 0), x86::xmm8);
		c.movaps(x86::oword_ptr(x86::rbp, 16), x86::xmm9);
		c.movaps(x86::oword_ptr(x86::rbp, 32), x86::xmm10);
		c.movaps(x86::oword_ptr(x86::rbp, 48), x86::xmm11);
		c.movaps(x86::oword_ptr(x86::rbp, 64), x86::xmm12);
		c.movaps(x86::oword_ptr(x86::rbp, 80), x86::xmm13);
		c.movaps(x86::oword_ptr(x86::rbp, 96), x86::xmm14);
		c.movaps(x86::oword_ptr(x86::rbp, 112), x86::xmm15);
	}

	c.sub(x86::qword_ptr(x86::rbx), -128);
	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so finish with xend instead
	c.bind(fail);

	// Load old data to store back in rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vmovaps(x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vmovaps(x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vmovaps(x86::ymm3, x86::yword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	c.jmp(load);

	c.bind(skip);
	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	build_get_tsc(c, stamp1);
	//c.jmp(fall);

	c.bind(fall);

	Label fall2 = c.newLabel();
	Label fail2 = c.newLabel();
	Label fail3 = c.newLabel();

	// Lightened transaction: only compare and swap data
	c.bind(next);

	// Try to "lock" reservation
	c.mov(x86::eax, 1);
	c.lock().xadd(x86::qword_ptr(x86::rbx), x86::rax);
	c.test(x86::eax, vm::rsrv_unique_lock);
	c.jnz(fail2);

	// Check if already updated
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail2);

	// Exclude some time spent on touching memory: stamp1 contains last success or failure
	c.mov(x86::rax, stamp1);
	c.sub(x86::rax, stamp0);
	build_get_tsc(c, stamp1);
	c.sub(stamp1, x86::rax);
	c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
	c.jae(fall2);

	Label tx1 = build_transaction_enter(c, fall2, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx) - ::offset32(&spu_thread::rdata)), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp1);
		c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
		c.jae(fall2);
		c.test(x86::qword_ptr(x86::rbx), 127 - 1);
		c.jnz(fall2);
	});
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&spu_thread::state) - ::offset32(&spu_thread::rdata)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall2);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail2);
	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vxorps(x86::ymm0, x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vxorps(x86::ymm1, x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vxorps(x86::ymm2, x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vxorps(x86::ymm3, x86::ymm3, x86::yword_ptr(x86::rbp, 96));
		c.vorps(x86::ymm0, x86::ymm0, x86::ymm1);
		c.vorps(x86::ymm1, x86::ymm2, x86::ymm3);
		c.vorps(x86::ymm0, x86::ymm1, x86::ymm0);
		c.vptest(x86::ymm0, x86::ymm0);
	}
	else
	{
		c.xorps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.xorps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.xorps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.xorps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.xorps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.xorps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.xorps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.xorps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
		c.orps(x86::xmm0, x86::xmm1);
		c.orps(x86::xmm2, x86::xmm3);
		c.orps(x86::xmm4, x86::xmm5);
		c.orps(x86::xmm6, x86::xmm7);
		c.orps(x86::xmm0, x86::xmm2);
		c.orps(x86::xmm4, x86::xmm6);
		c.orps(x86::xmm0, x86::xmm4);
		c.ptest(x86::xmm0, x86::xmm0);
	}

	c.jnz(fail3);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(x86::rbp, 0), x86::ymm4);
		c.vmovaps(x86::yword_ptr(x86::rbp, 32), x86::ymm5);
		c.vmovaps(x86::yword_ptr(x86::rbp, 64), x86::ymm6);
		c.vmovaps(x86::yword_ptr(x86::rbp, 96), x86::ymm7);
	}
	else
	{
		c.movaps(x86::oword_ptr(x86::rbp, 0), x86::xmm8);
		c.movaps(x86::oword_ptr(x86::rbp, 16), x86::xmm9);
		c.movaps(x86::oword_ptr(x86::rbp, 32), x86::xmm10);
		c.movaps(x86::oword_ptr(x86::rbp, 48), x86::xmm11);
		c.movaps(x86::oword_ptr(x86::rbp, 64), x86::xmm12);
		c.movaps(x86::oword_ptr(x86::rbp, 80), x86::xmm13);
		c.movaps(x86::oword_ptr(x86::rbp, 96), x86::xmm14);
		c.movaps(x86::oword_ptr(x86::rbp, 112), x86::xmm15);
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	c.lock().add(x86::qword_ptr(x86::rbx), 127);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	// XABORT is expensive so try to finish with xend instead
	c.bind(fail3);

	// Load previous data to store back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vmovaps(x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vmovaps(x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vmovaps(x86::ymm3, x86::yword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx) - ::offset32(&spu_thread::rdata)), 1);
	c.jmp(fail2);

	c.bind(fall2);
	c.mov(x86::rax, -1);
	c.jmp(_ret);

	c.bind(fail2);
	c.lock().sub(x86::qword_ptr(x86::rbx), 1);
	c.bind(load);

	// Store previous data back to rdata
	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(args[2], 0), x86::ymm0);
		c.vmovaps(x86::yword_ptr(args[2], 32), x86::ymm1);
		c.vmovaps(x86::yword_ptr(args[2], 64), x86::ymm2);
		c.vmovaps(x86::yword_ptr(args[2], 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[2], 0), x86::xmm0);
		c.movaps(x86::oword_ptr(args[2], 16), x86::xmm1);
		c.movaps(x86::oword_ptr(args[2], 32), x86::xmm2);
		c.movaps(x86::oword_ptr(args[2], 48), x86::xmm3);
		c.movaps(x86::oword_ptr(args[2], 64), x86::xmm4);
		c.movaps(x86::oword_ptr(args[2], 80), x86::xmm5);
		c.movaps(x86::oword_ptr(args[2], 96), x86::xmm6);
		c.movaps(x86::oword_ptr(args[2], 112), x86::xmm7);
	}

	c.mov(x86::rax, -1);
	c.mov(x86::qword_ptr(args[2], ::offset32(&spu_thread::last_ftime) - ::offset32(&spu_thread::rdata)), x86::rax);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (s_tsx_avx)
	{
		c.vmovups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.vmovups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
	else
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
		c.movups(x86::xmm8, x86::oword_ptr(x86::rsp, 32));
		c.movups(x86::xmm9, x86::oword_ptr(x86::rsp, 48));
		c.movups(x86::xmm10, x86::oword_ptr(x86::rsp, 64));
		c.movups(x86::xmm11, x86::oword_ptr(x86::rsp, 80));
		c.movups(x86::xmm12, x86::oword_ptr(x86::rsp, 96));
		c.movups(x86::xmm13, x86::oword_ptr(x86::rsp, 112));
		c.movups(x86::xmm14, x86::oword_ptr(x86::rsp, 128));
		c.movups(x86::xmm15, x86::oword_ptr(x86::rsp, 144));
	}
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 168);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::rbp);
	c.ret();
});

const auto spu_putlluc_tx = build_function_asm<u64(*)(u32 raddr, const void* rdata, cpu_thread* _spu)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label _ret = c.newLabel();
	Label skip = c.newLabel();
	Label next = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r12);
	c.mov(x86::rbp, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_sudo_addr)));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(reinterpret_cast<u64>(+vm::g_reservations), args[0]));
	c.prefetchw(x86::byte_ptr(x86::rbx));
	c.mov(x86::r13, args[1]);

	// Prepare data
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::yword_ptr(args[1], 0));
		c.vmovups(x86::ymm1, x86::yword_ptr(args[1], 32));
		c.vmovups(x86::ymm2, x86::yword_ptr(args[1], 64));
		c.vmovups(x86::ymm3, x86::yword_ptr(args[1], 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(args[1], 0));
		c.movaps(x86::xmm1, x86::oword_ptr(args[1], 16));
		c.movaps(x86::xmm2, x86::oword_ptr(args[1], 32));
		c.movaps(x86::xmm3, x86::oword_ptr(args[1], 48));
		c.movaps(x86::xmm4, x86::oword_ptr(args[1], 64));
		c.movaps(x86::xmm5, x86::oword_ptr(args[1], 80));
		c.movaps(x86::xmm6, x86::oword_ptr(args[1], 96));
		c.movaps(x86::xmm7, x86::oword_ptr(args[1], 112));
	}

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	const auto stamp1 = args[1];
	build_get_tsc(c, stamp0);

	// Begin transaction
	Label tx0 = build_transaction_enter(c, fall, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx)), 1);
		build_get_tsc(c, stamp1);
		c.sub(stamp1, stamp0);
		c.xor_(x86::eax, x86::eax);
		c.cmp(stamp1, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit1)));
		c.jae(fall);
	});
	c.xbegin(tx0);
	c.test(x86::qword_ptr(x86::rbx), vm::rsrv_unique_lock);
	c.jnz(skip);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(x86::rbp, 0), x86::ymm0);
		c.vmovaps(x86::yword_ptr(x86::rbp, 32), x86::ymm1);
		c.vmovaps(x86::yword_ptr(x86::rbp, 64), x86::ymm2);
		c.vmovaps(x86::yword_ptr(x86::rbp, 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(x86::rbp, 0), x86::xmm0);
		c.movaps(x86::oword_ptr(x86::rbp, 16), x86::xmm1);
		c.movaps(x86::oword_ptr(x86::rbp, 32), x86::xmm2);
		c.movaps(x86::oword_ptr(x86::rbp, 48), x86::xmm3);
		c.movaps(x86::oword_ptr(x86::rbp, 64), x86::xmm4);
		c.movaps(x86::oword_ptr(x86::rbp, 80), x86::xmm5);
		c.movaps(x86::oword_ptr(x86::rbp, 96), x86::xmm6);
		c.movaps(x86::oword_ptr(x86::rbp, 112), x86::xmm7);
	}

	c.sub(x86::qword_ptr(x86::rbx), -128);
	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx)), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	c.bind(skip);
	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx)), 1);
	build_get_tsc(c, stamp1);
	//c.jmp(fall);

	c.bind(fall);

	Label fall2 = c.newLabel();

	// Lightened transaction
	c.bind(next);

	// Lock reservation
	c.mov(x86::eax, 1);
	c.lock().xadd(x86::qword_ptr(x86::rbx), x86::rax);
	c.test(x86::eax, 127 - 1);
	c.jnz(fall2);

	// Exclude some time spent on touching memory: stamp1 contains last success or failure
	c.mov(x86::rax, stamp1);
	c.sub(x86::rax, stamp0);
	c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
	c.jae(fall2);
	build_get_tsc(c, stamp1);
	c.sub(stamp1, x86::rax);

	Label tx1 = build_transaction_enter(c, fall2, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx)), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp1);
		c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit2)));
		c.jae(fall2);
	});

	c.prefetchw(x86::byte_ptr(x86::rbp, 0));
	c.prefetchw(x86::byte_ptr(x86::rbp, 64));

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&cpu_thread::state)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall2);
	// Check contention
	c.test(x86::qword_ptr(x86::rbx), 127 - 1);
	c.jc(fall2);
	c.xbegin(tx1);

	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(x86::rbp, 0), x86::ymm0);
		c.vmovaps(x86::yword_ptr(x86::rbp, 32), x86::ymm1);
		c.vmovaps(x86::yword_ptr(x86::rbp, 64), x86::ymm2);
		c.vmovaps(x86::yword_ptr(x86::rbp, 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(x86::rbp, 0), x86::xmm0);
		c.movaps(x86::oword_ptr(x86::rbp, 16), x86::xmm1);
		c.movaps(x86::oword_ptr(x86::rbp, 32), x86::xmm2);
		c.movaps(x86::oword_ptr(x86::rbp, 48), x86::xmm3);
		c.movaps(x86::oword_ptr(x86::rbp, 64), x86::xmm4);
		c.movaps(x86::oword_ptr(x86::rbp, 80), x86::xmm5);
		c.movaps(x86::oword_ptr(x86::rbp, 96), x86::xmm6);
		c.movaps(x86::oword_ptr(x86::rbp, 112), x86::xmm7);
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx)), 1);
	c.lock().add(x86::qword_ptr(x86::rbx), 127);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);
	c.jmp(_ret);

	c.bind(fall2);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 40);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::rbp);
	c.ret();
});

const extern auto spu_getllar_tx = build_function_asm<u64(*)(u32 raddr, void* rdata, cpu_thread* _cpu, u64 rtime)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label _ret = c.newLabel();

	//if (utils::has_avx() && !s_tsx_avx)
	//{
	//	c.vzeroupper();
	//}

	// Create stack frame if necessary (Windows ABI has only 6 volatile vector registers)
	c.push(x86::rbp);
	c.push(x86::r13);
	c.push(x86::r12);
	c.push(x86::rbx);
	c.sub(x86::rsp, 40);
#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::oword_ptr(x86::rsp, 0), x86::xmm6);
		c.movups(x86::oword_ptr(x86::rsp, 16), x86::xmm7);
	}
#endif

	// Prepare registers
	build_swap_rdx_with(c, args, x86::r12);
	c.mov(x86::rbp, x86::qword_ptr(reinterpret_cast<u64>(&vm::g_sudo_addr)));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(reinterpret_cast<u64>(+vm::g_reservations), args[0]));
	c.mov(x86::r13, args[1]);

	// Alloc args[0] to stamp0
	const auto stamp0 = args[0];
	build_get_tsc(c, stamp0);

	// Begin transaction
	Label tx0 = build_transaction_enter(c, fall, [&]()
	{
		c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::ftx)), 1);
		build_get_tsc(c);
		c.sub(x86::rax, stamp0);
		c.cmp(x86::rax, x86::qword_ptr(reinterpret_cast<u64>(&g_rtm_tx_limit1)));
		c.jae(fall);
	});

	// Check pause flag
	c.bt(x86::dword_ptr(args[2], ::offset32(&cpu_thread::state)), static_cast<u32>(cpu_flag::pause));
	c.jc(fall);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.and_(x86::rax, ~vm::rsrv_shared_mask);
	c.cmp(x86::rax, args[3]);
	c.jne(fall);
	c.xbegin(tx0);

	// Just read data to registers
	if (s_tsx_avx)
	{
		c.vmovups(x86::ymm0, x86::yword_ptr(x86::rbp, 0));
		c.vmovups(x86::ymm1, x86::yword_ptr(x86::rbp, 32));
		c.vmovups(x86::ymm2, x86::yword_ptr(x86::rbp, 64));
		c.vmovups(x86::ymm3, x86::yword_ptr(x86::rbp, 96));
	}
	else
	{
		c.movaps(x86::xmm0, x86::oword_ptr(x86::rbp, 0));
		c.movaps(x86::xmm1, x86::oword_ptr(x86::rbp, 16));
		c.movaps(x86::xmm2, x86::oword_ptr(x86::rbp, 32));
		c.movaps(x86::xmm3, x86::oword_ptr(x86::rbp, 48));
		c.movaps(x86::xmm4, x86::oword_ptr(x86::rbp, 64));
		c.movaps(x86::xmm5, x86::oword_ptr(x86::rbp, 80));
		c.movaps(x86::xmm6, x86::oword_ptr(x86::rbp, 96));
		c.movaps(x86::xmm7, x86::oword_ptr(x86::rbp, 112));
	}

	c.xend();
	c.add(x86::qword_ptr(args[2], ::offset32(&spu_thread::stx)), 1);
	build_get_tsc(c);
	c.sub(x86::rax, stamp0);

	// Store data
	if (s_tsx_avx)
	{
		c.vmovaps(x86::yword_ptr(args[1], 0), x86::ymm0);
		c.vmovaps(x86::yword_ptr(args[1], 32), x86::ymm1);
		c.vmovaps(x86::yword_ptr(args[1], 64), x86::ymm2);
		c.vmovaps(x86::yword_ptr(args[1], 96), x86::ymm3);
	}
	else
	{
		c.movaps(x86::oword_ptr(args[1], 0), x86::xmm0);
		c.movaps(x86::oword_ptr(args[1], 16), x86::xmm1);
		c.movaps(x86::oword_ptr(args[1], 32), x86::xmm2);
		c.movaps(x86::oword_ptr(args[1], 48), x86::xmm3);
		c.movaps(x86::oword_ptr(args[1], 64), x86::xmm4);
		c.movaps(x86::oword_ptr(args[1], 80), x86::xmm5);
		c.movaps(x86::oword_ptr(args[1], 96), x86::xmm6);
		c.movaps(x86::oword_ptr(args[1], 112), x86::xmm7);
	}

	c.jmp(_ret);
	c.bind(fall);
	c.xor_(x86::eax, x86::eax);
	//c.jmp(_ret);

	c.bind(_ret);

#ifdef _WIN32
	if (!s_tsx_avx)
	{
		c.movups(x86::xmm6, x86::oword_ptr(x86::rsp, 0));
		c.movups(x86::xmm7, x86::oword_ptr(x86::rsp, 16));
	}
#endif

	if (s_tsx_avx)
	{
		c.vzeroupper();
	}

	c.add(x86::rsp, 40);
	c.pop(x86::rbx);
	c.pop(x86::r12);
	c.pop(x86::r13);
	c.pop(x86::rbp);
	c.ret();
});

void spu_int_ctrl_t::set(u64 ints)
{
	// leave only enabled interrupts
	ints &= mask;

	// notify if at least 1 bit was set
	if (ints && ~stat.fetch_or(ints) & ints)
	{
		std::shared_lock rlock(id_manager::g_mutex);

		if (lv2_obj::check(tag))
		{
			if (auto handler = tag->handler; lv2_obj::check(handler))
			{
				rlock.unlock();
				handler->exec();
			}
		}
	}
}

const spu_imm_table_t g_spu_imm;

spu_imm_table_t::scale_table_t::scale_table_t()
{
	for (s32 i = -155; i < 174; i++)
	{
		m_data[i + 155].vf = _mm_set1_ps(static_cast<float>(std::exp2(i)));
	}
}

spu_imm_table_t::spu_imm_table_t()
{
	for (u32 i = 0; i < std::size(sldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			sldq_pshufb[i]._u8[j] = static_cast<u8>(j - i);
		}
	}

	for (u32 i = 0; i < std::size(srdq_pshufb); i++)
	{
		const u32 im = (0u - i) & 0x1f;

		for (u32 j = 0; j < 16; j++)
		{
			srdq_pshufb[i]._u8[j] = (j + im > 15) ? 0xff : static_cast<u8>(j + im);
		}
	}

	for (u32 i = 0; i < std::size(rldq_pshufb); i++)
	{
		for (u32 j = 0; j < 16; j++)
		{
			rldq_pshufb[i]._u8[j] = static_cast<u8>((j - i) & 0xf);
		}
	}
}

std::string spu_thread::dump_regs() const
{
	std::string ret;

	const bool floats_only = debugger_float_mode.load();

	for (u32 i = 0; i < 128; i++, ret += '\n')
	{
		fmt::append(ret, "%s: ", spu_reg_name[i]);

		const auto r = gpr[i];

		if (auto [size, dst, src] = SPUDisAsm::try_get_insert_mask_info(r); size)
		{
			// Special: insertion masks

			const std::string_view type =
				size == 1 ? "byte" :
				size == 2 ? "half" :
				size == 4 ? "word" :
				size == 8 ? "dword" : "error";

			if ((size >= 4u && !src) || (size == 2u && src == 1u) || (size == 1u && src == 3u))
			{
				fmt::append(ret, "insert -> %s[%u]", type, dst);
				continue;
			}
		}

		auto to_f64 = [](u32 bits)
		{
			const u32 abs = bits & 0x7fff'ffff;
			constexpr u32 scale = (1 << 23);
			return std::copysign(abs < scale ? 0 : std::ldexp((scale + (abs % scale)) / f64{scale}, static_cast<int>(abs >> 23) - 127), bits >> 31 ? -1 : 1);
		};

		const double array[]{to_f64(r.u32r[0]), to_f64(r.u32r[1]), to_f64(r.u32r[2]), to_f64(r.u32r[3])};

		const u32 i3 = r._u32[3];
		const bool is_packed = v128::from32p(i3) == r;

		if (floats_only)
		{
			fmt::append(ret, "%g, %g, %g, %g", array[0], array[1], array[2], array[3]);
			continue;
		}

		if (is_packed)
		{
			// Shortand formatting
			fmt::append(ret, "%08x", i3);
		}
		else
		{
			fmt::append(ret, "%08x %08x %08x %08x", r.u32r[0], r.u32r[1], r.u32r[2], r.u32r[3]);
		}

		if (i3 >= 0x80 && is_exec_code(i3))
		{
			SPUDisAsm dis_asm(cpu_disasm_mode::normal, ls);
			dis_asm.disasm(i3);
			fmt::append(ret, " -> %s", dis_asm.last_opcode);
		}

		if (std::any_of(std::begin(array), std::end(array), [](f64 v){ return v != 0; }))
		{
			if (is_packed)
			{
				fmt::append(ret, " (%g)", array[0]);
			}
			else
			{
				fmt::append(ret, " (%g, %g, %g, %g)", array[0], array[1], array[2], array[3]);
			}
		}
	}

	const auto events = ch_events.load();

	fmt::append(ret, "\nEvent Stat: 0x%x\n", events.events);
	fmt::append(ret, "Event Mask: 0x%x\n", events.mask);
	fmt::append(ret, "Event Count: %u\n", events.count);
	fmt::append(ret, "SRR0: 0x%05x\n", srr0);
	fmt::append(ret, "Stall Stat: %s\n", ch_stall_stat);
	fmt::append(ret, "Stall Mask: 0x%x\n", ch_stall_mask);
	fmt::append(ret, "Tag Stat: %s\n", ch_tag_stat);
	fmt::append(ret, "Tag Update: %s\n", mfc_tag_update{ch_tag_upd});
	fmt::append(ret, "Atomic Stat: %s\n", ch_atomic_stat); // TODO: use mfc_atomic_status formatting
	fmt::append(ret, "Interrupts: %s\n", interrupts_enabled ? "Enabled" : "Disabled");
	fmt::append(ret, "Inbound Mailbox: %s\n", ch_in_mbox);
	fmt::append(ret, "Out Mailbox: %s\n", ch_out_mbox);
	fmt::append(ret, "Out Interrupts Mailbox: %s\n", ch_out_intr_mbox);
	fmt::append(ret, "SNR config: 0x%llx\n", snr_config);
	fmt::append(ret, "SNR1: %s\n", ch_snr1);
	fmt::append(ret, "SNR2: %s\n", ch_snr2);

	const u32 addr = raddr;

	fmt::append(ret, "Reservation Addr: %s\n", addr ? fmt::format("0x%x", addr) : "N/A");
	fmt::append(ret, "Reservation Data:\n");

	be_t<u32> data[32]{};
	std::memcpy(data, rdata, sizeof(rdata)); // Show the data even if the reservation was lost inside the atomic loop

	for (usz i = 0; i < std::size(data); i += 4)
	{
		fmt::append(ret, "[0x%02x] %08x %08x %08x %08x\n", i * sizeof(data[0])
			, data[i + 0], data[i + 1], data[i + 2], data[i + 3]);
	}

	return ret;
}

std::string spu_thread::dump_callstack() const
{
	std::string ret;

	fmt::append(ret, "Call stack:\n=========\n0x%08x (0x0) called\n", pc);

	for (const auto& sp : dump_callstack_list())
	{
		// TODO: function addresses too
		fmt::append(ret, "> from 0x%08x (sp=0x%08x)\n", sp.first, sp.second);
	}

	return ret;
}

std::vector<std::pair<u32, u32>> spu_thread::dump_callstack_list() const
{
	std::vector<std::pair<u32, u32>> call_stack_list;

	bool first = true;

	// Declare first 128-bytes as invalid for stack (common values such as 0 do not make sense here)
	for (u32 sp = gpr[1]._u32[3]; (sp & 0xF) == 0u && sp >= 0x80u && sp <= 0x3FFE0u; sp = _ref<u32>(sp), first = false)
	{
		v128 lr = _ref<v128>(sp + 16);

		auto is_invalid = [this](v128 v)
		{
			const u32 addr = v._u32[3] & 0x3FFFC;

			if (v != v128::from32r(addr))
			{
				// 1) Non-zero lower words are invalid (because BRSL-like instructions generate only zeroes)
				// 2) Bits normally masked out by indirect braches are considered invalid
				return true;
			}

			return !addr || !is_exec_code(addr);
		};

		if (is_invalid(lr))
		{
			if (first)
			{
				// Function hasn't saved LR, could be because it's a leaf function
				// Use LR directly instead
				lr = gpr[0];

				if (is_invalid(lr))
				{
					// Skip it, workaround
					continue;
				}
			}
			else
			{
				break;
			}
		}

		// TODO: function addresses too
		call_stack_list.emplace_back(lr._u32[3], sp);
	}

	return call_stack_list;
}

std::string spu_thread::dump_misc() const
{
	std::string ret;

	fmt::append(ret, "Block Weight: %u (Retreats: %u)", block_counter, block_failure);

	if (g_cfg.core.spu_prof)
	{
		// Get short function hash
		const u64 name = atomic_storage<u64>::load(block_hash);

		fmt::append(ret, "\nCurrent block: %s", fmt::base57(be_t<u64>{name}));

		// Print only 7 hash characters out of 11 (which covers roughly 48 bits)
		ret.resize(ret.size() - 4);

		// Print chunk address from lowest 16 bits
		fmt::append(ret, "...chunk-0x%05x", (name & 0xffff) * 4);
	}

	const u32 offset = group ? SPU_FAKE_BASE_ADDR + (id & 0xffffff) * SPU_LS_SIZE : RAW_SPU_BASE_ADDR + index * RAW_SPU_OFFSET;

	fmt::append(ret, "\n[%s]", ch_mfc_cmd);
	fmt::append(ret, "\nLocal Storage: 0x%08x..0x%08x", offset, offset + 0x3ffff);

	if (const u64 _time = start_time)
	{
		if (const auto func = current_func)
		{
			ret += "\nIn function: ";
			ret += func;
		}
		else
		{
			ret += '\n';
		}


		fmt::append(ret, "\nWaiting: %fs", (get_system_time() - _time) / 1000000.);
	}
	else
	{
		ret += "\n\n";
	}

	fmt::append(ret, "\nTag Mask: 0x%08x", ch_tag_mask);
	fmt::append(ret, "\nMFC Queue Size: %u", mfc_size);

	for (u32 i = 0; i < 16; i++)
	{
		if (i < mfc_size)
		{
			fmt::append(ret, "\n%s", mfc_queue[i]);
		}
		else
		{
			break;
		}
	}

	return ret;
}

void spu_thread::cpu_init()
{
	std::memset(gpr.data(), 0, gpr.size() * sizeof(gpr[0]));
	fpscr.Reset();

	ch_mfc_cmd = {};

	srr0 = 0;
	mfc_size = 0;
	mfc_barrier = 0;
	mfc_fence = 0;
	ch_tag_upd = 0;
	ch_tag_mask = 0;
	ch_tag_stat.data.raw() = {};
	ch_stall_mask = 0;
	ch_stall_stat.data.raw() = {};
	ch_atomic_stat.data.raw() = {};

	ch_out_mbox.data.raw() = {};
	ch_out_intr_mbox.data.raw() = {};

	ch_events.raw() = {};
	interrupts_enabled = false;
	raddr = 0;

	ch_dec_start_timestamp = get_timebased_time();
	ch_dec_value = option & SYS_SPU_THREAD_OPTION_DEC_SYNC_TB_ENABLE ? ~static_cast<u32>(ch_dec_start_timestamp) : 0;

	if (get_type() >= spu_type::raw)
	{
		ch_in_mbox.clear();
		ch_snr1.data.raw() = {};
		ch_snr2.data.raw() = {};

		snr_config = 0;

		mfc_prxy_mask.raw() = 0;
		mfc_prxy_write_state = {};
	}

	status_npc.raw() = {get_type() == spu_type::isolated ? SPU_STATUS_IS_ISOLATED : 0, 0};
	run_ctrl.raw() = 0;

	int_ctrl[0].clear();
	int_ctrl[1].clear();
	int_ctrl[2].clear();

	gpr[1]._u32[3] = 0x3FFF0; // initial stack frame pointer
}

void spu_thread::cpu_return()
{
	if (get_type() >= spu_type::raw)
	{
		if (status_npc.fetch_op([this](status_npc_sync_var& state)
		{
			if (state.status & SPU_STATUS_RUNNING)
			{
				// Save next PC and current SPU Interrupt Status
				// Used only by RunCtrl stop requests
				state.status &= ~SPU_STATUS_RUNNING;
				state.npc = pc | +interrupts_enabled;
				return true;
			}

			return false;
		}).second)
		{
			status_npc.notify_one();
		}
	}
	else if (is_stopped())
	{
		ch_in_mbox.clear();

		if (ensure(group->running)-- == 1)
		{
			{
				std::lock_guard lock(group->mutex);
				group->run_state = SPU_THREAD_GROUP_STATUS_INITIALIZED;

				if (!group->join_state)
				{
					group->join_state = SYS_SPU_THREAD_GROUP_JOIN_ALL_THREADS_EXIT;
				}

				for (const auto& thread : group->threads)
				{
					if (thread && thread.get() != this && thread->status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
					{
						// Wait for all threads to have error codes if exited by sys_spu_thread_exit
						for (u32 status; !thread->exit_status.try_read(status)
							|| status != thread->last_exit_status;)
						{
							utils::pause();
						}
					}
				}

				if (status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
				{
					// Set exit status now, in conjunction with group state changes
					exit_status.set_value(last_exit_status);
				}

				group->stop_count++;

				if (const auto ppu = std::exchange(group->waiter, nullptr))
				{
					// Send exit status directly to the joining thread
					ppu->gpr[4] = group->join_state;
					ppu->gpr[5] = group->exit_status;
					group->join_state.release(0);
					lv2_obj::awake(ppu);
				}
			}

			// Notify on last thread stopped
			group->stop_count.notify_all();
		}
		else if (status_npc.load().status >> 16 == SYS_SPU_THREAD_STOP_THREAD_EXIT)
		{
			exit_status.set_value(last_exit_status);
		}
	}
}

extern thread_local std::string(*g_tls_log_prefix)();

void spu_thread::cpu_task()
{
	// Get next PC and SPU Interrupt status
	pc = status_npc.load().npc;

	// Note: works both on RawSPU and threaded SPU!
	set_interrupt_status((pc & 1) != 0);

	pc &= 0x3fffc;

	std::fesetround(FE_TOWARDZERO);

	g_tls_log_prefix = []
	{
		const auto cpu = static_cast<spu_thread*>(get_current_cpu_thread());

		static thread_local shared_ptr<std::string> name_cache;

		if (!cpu->spu_tname.is_equal(name_cache)) [[unlikely]]
		{
			cpu->spu_tname.peek_op([&](const shared_ptr<std::string>& ptr)
			{
				if (ptr != name_cache)
				{
					name_cache = ptr;
				}
			});
		}

		const auto type = cpu->get_type();
		return fmt::format("%sSPU[0x%07x] Thread (%s) [0x%05x]", type >= spu_type::raw ? type == spu_type::isolated ? "Iso" : "Raw" : "", cpu->lv2_id, *name_cache.get(), cpu->pc);
	};

	if (jit)
	{
		while (true)
		{
			if (state) [[unlikely]]
			{
				if (check_state())
					break;
			}

			if (_ref<u32>(pc) == 0x0u)
			{
				if (spu_thread::stop_and_signal(0x0))
					pc += 4;
				continue;
			}

			spu_runtime::g_gateway(*this, _ptr<u8>(0), nullptr);
		}

		// Print some stats
		spu_log.notice("Stats: Block Weight: %u (Retreats: %u);", block_counter, block_failure);
	}
	else
	{
		ensure(spu_runtime::g_interpreter);

		while (true)
		{
			if (state) [[unlikely]]
			{
				if (check_state())
					break;
			}

			spu_runtime::g_interpreter(*this, _ptr<u8>(0), nullptr);
		}
	}
}

struct raw_spu_cleanup
{
	raw_spu_cleanup() = default;

	raw_spu_cleanup(const raw_spu_cleanup&) = delete;

	raw_spu_cleanup& operator =(const raw_spu_cleanup&) = delete;

	~raw_spu_cleanup()
	{
		std::memset(spu_thread::g_raw_spu_id, 0, sizeof(spu_thread::g_raw_spu_id));
		spu_thread::g_raw_spu_ctr = 0;
		g_fxo->get<raw_spu_cleanup>(); // Register destructor
	}
};

void spu_thread::cleanup()
{
	// Deallocate local storage
	ensure(vm::dealloc(vm_offset(), vm::spu, &shm));

	// Deallocate RawSPU ID
	if (get_type() >= spu_type::raw)
	{
		g_raw_spu_id[index] = 0;
		g_raw_spu_ctr--;
	}

	// Free range lock (and signals cleanup was called to the destructor)
	vm::free_range_lock(range_lock);

	// Signal the debugger about the termination
	state += cpu_flag::exit;
}

spu_thread::~spu_thread()
{
	// Unmap LS and its mirrors
	shm->unmap(ls + SPU_LS_SIZE);
	shm->unmap(ls);
	shm->unmap(ls - SPU_LS_SIZE);

	perf_log.notice("Perf stats for transactions: success %u, failure %u", stx, ftx);
	perf_log.notice("Perf stats for PUTLLC reload: successs %u, failure %u", last_succ, last_fail);
}

spu_thread::spu_thread(lv2_spu_group* group, u32 index, std::string_view name, u32 lv2_id, bool is_isolated, u32 option)
	: cpu_thread(idm::last_id())
	, group(group)
	, index(index)
	, shm(std::make_shared<utils::shm>(SPU_LS_SIZE))
	, ls([&]()
	{
		if (!group)
		{
			ensure(vm::get(vm::spu)->falloc(vm_offset(), SPU_LS_SIZE, &shm, 0x200));
		}
		else
		{
			// 0x1000 indicates falloc to allocate page with no access rights in base memory
			ensure(vm::get(vm::spu)->falloc(vm_offset(), SPU_LS_SIZE, &shm, 0x1200));
		}

		// Try to guess free area
		const auto start = vm::g_free_addr + SPU_LS_SIZE * (cpu_thread::id & 0xffffff) * 12;

		u32 total = 0;

		// Map LS and its mirrors
		for (u64 addr = reinterpret_cast<u64>(start); addr < 0x8000'0000'0000;)
		{
			if (auto ptr = shm->try_map(reinterpret_cast<u8*>(addr)))
			{
				if (++total == 3)
				{
					// Use the middle mirror
					return ptr - SPU_LS_SIZE;
				}

				addr += SPU_LS_SIZE;
			}
			else
			{
				// Reset, cleanup and start again
				for (u32 i = 1; i <= total; i++)
				{
					shm->unmap(reinterpret_cast<u8*>(addr - i * SPU_LS_SIZE));
				}

				total = 0;

				addr += 0x10000;
			}
		}

		fmt::throw_exception("Failed to map SPU LS memory");
	}())
	, thread_type(group ? spu_type::threaded : is_isolated ? spu_type::isolated : spu_type::raw)
	, option(option)
	, lv2_id(lv2_id)
	, spu_tname(make_single<std::string>(name))
{
	if (g_cfg.core.spu_decoder == spu_decoder_type::asmjit)
	{
		jit = spu_recompiler_base::make_asmjit_recompiler();
	}

	if (g_cfg.core.spu_decoder == spu_decoder_type::llvm)
	{
		jit = spu_recompiler_base::make_fast_llvm_recompiler();
	}

	if (g_cfg.core.spu_decoder != spu_decoder_type::fast && g_cfg.core.spu_decoder != spu_decoder_type::precise)
	{
		if (g_cfg.core.spu_block_size != spu_block_size_type::safe)
		{
			// Initialize stack mirror
			std::memset(stack_mirror.data(), 0xff, sizeof(stack_mirror));
		}
	}

	if (get_type() >= spu_type::raw)
	{
		cpu_init();
	}

	if (g_cfg.core.mfc_debug)
	{
		mfc_history.resize(max_mfc_dump_idx);
	}

	range_lock = vm::alloc_range_lock();
}

void spu_thread::push_snr(u32 number, u32 value)
{
	// Get channel
	const auto channel = number & 1 ? &ch_snr2 : &ch_snr1;

	// Prepare some data
	const u32 event_bit = SPU_EVENT_S1 >> (number & 1);
	const u32 bitor_bit = (snr_config >> number) & 1;

	// Redundant, g_use_rtm is checked inside tx_start now.
	if (g_use_rtm)
	{
		bool channel_notify = false;
		bool thread_notify = false;

		const bool ok = utils::tx_start([&]
		{
			channel_notify = (channel->data.raw() & spu_channel::bit_wait) != 0;
			thread_notify = (channel->data.raw() & spu_channel::bit_count) == 0;

			if (bitor_bit)
			{
				channel->data.raw() &= ~spu_channel::bit_wait;
				channel->data.raw() |= spu_channel::bit_count | value;
			}
			else
			{
				channel->data.raw() = spu_channel::bit_count | value;
			}

			if (thread_notify)
			{
				ch_events.raw().events |= event_bit;

				if (ch_events.raw().mask & event_bit)
				{
					ch_events.raw().count = 1;
					thread_notify = ch_events.raw().waiting != 0;
				}
				else
				{
					thread_notify = false;
				}
			}
		});

		if (ok)
		{
			if (channel_notify)
				channel->data.notify_one();
			if (thread_notify)
				this->notify();

			return;
		}
	}

	// Lock event channel in case it needs event notification
	ch_events.atomic_op([](ch_events_t& ev)
	{
		ev.locks++;
	});

	// Check corresponding SNR register settings
	if (bitor_bit)
	{
		if (channel->push_or(value))
			set_events(event_bit);
	}
	else
	{
		if (channel->push(value))
			set_events(event_bit);
	}

	ch_events.atomic_op([](ch_events_t& ev)
	{
		ev.locks--;
	});
}

void spu_thread::do_dma_transfer(spu_thread* _this, const spu_mfc_cmd& args, u8* ls)
{
	perf_meter<"DMA"_u32> perf_;

	const bool is_get = (args.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK | MFC_START_MASK)) == MFC_GET_CMD;

	u32 eal = args.eal;
	u32 lsa = args.lsa & 0x3ffff;

	// Keep src point to const
	u8* dst = nullptr;
	const u8* src = nullptr;

	std::tie(dst, src) = [&]() -> std::pair<u8*, const u8*>
	{
		u8* dst = vm::_ptr<u8>(eal);
		u8* src = ls + lsa;

		if (is_get)
		{
			std::swap(src, dst);
		}

		return {dst, src};
	}();

	// SPU Thread Group MMIO (LS and SNR) and RawSPU MMIO
	if (_this && eal >= RAW_SPU_BASE_ADDR)
	{
		if (g_cfg.core.mfc_debug && _this)
		{
			// TODO
		}

		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		if (eal < SYS_SPU_THREAD_BASE_LOW)
		{
			// RawSPU MMIO
			auto thread = idm::get<named_thread<spu_thread>>(find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

			if (!thread)
			{
				// Access Violation
			}
			else if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + args.size - 1 < SPU_LS_SIZE) // LS access
			{
			}
			else if (u32 value; args.size == 4 && is_get && thread->read_reg(eal, value))
			{
				_this->_ref<u32>(lsa) = value;
				return;
			}
			else if (args.size == 4 && !is_get && thread->write_reg(eal, args.cmd != MFC_SDCRZ_CMD ? + _this->_ref<u32>(lsa) : 0))
			{
				return;
			}
			else
			{
				fmt::throw_exception("Invalid RawSPU MMIO offset (cmd=[%s])", args);
			}
		}
		else if (_this->get_type() >= spu_type::raw)
		{
			// Access Violation
		}
		else if (_this->group && _this->group->threads_map[index] != -1)
		{
			auto& spu = static_cast<spu_thread&>(*_this->group->threads[_this->group->threads_map[index]]);

			if (offset + args.size <= SPU_LS_SIZE) // LS access
			{
				// redirect access
				if (auto ptr = spu.ls + offset; is_get)
					src = ptr;
				else
					dst = ptr;
			}
			else if (!is_get && args.size == 4 && (offset == SYS_SPU_THREAD_SNR1 || offset == SYS_SPU_THREAD_SNR2))
			{
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, args.cmd != MFC_SDCRZ_CMD ? +_this->_ref<u32>(lsa) : 0);
				return;
			}
			else
			{
				fmt::throw_exception("Invalid MMIO offset (cmd=[%s])", args);
			}
		}
		else
		{
			// Access Violation
		}
	}

	// Cleanup: if PUT or GET happens after PUTLLC failure, it's too complicated and it's easier to just give up
	if (_this)
	{
		_this->last_faddr = 0;
	}

	// It is so rare that optimizations are not implemented (TODO)
	alignas(64) static constexpr u8 zero_buf[0x10000]{};

	if (args.cmd == MFC_SDCRZ_CMD)
	{
		src = zero_buf;
	}

	if ((!g_use_rtm && !is_get) || g_cfg.core.spu_accurate_dma)  [[unlikely]]
	{
		perf_meter<"ADMA_GET"_u64> perf_get = perf_;
		perf_meter<"ADMA_PUT"_u64> perf_put = perf_;

		cpu_thread* _cpu = _this ? _this : get_current_cpu_thread();

		atomic_t<u64, 64>* range_lock = nullptr;

		if (!_this) [[unlikely]]
		{
			if (_cpu->id_type() == 2)
			{
				// Use range_lock of current SPU thread for range locks
				range_lock = static_cast<spu_thread*>(_cpu)->range_lock;
			}
			else
			{
				goto plain_access;
			}
		}
		else
		{
			range_lock = _this->range_lock;
		}

		utils::prefetch_write(range_lock);

		for (u32 size = args.size, size0; is_get; size -= size0, dst += size0, src += size0, eal += size0)
		{
			size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

			for (u64 i = 0;; [&]()
			{
				if (_cpu->state)
				{
					_cpu->check_state();
				}
				else if (++i < 25) [[likely]]
				{
					busy_wait(300);
				}
				else
				{
					_cpu->state += cpu_flag::wait + cpu_flag::temp;
					std::this_thread::yield();
					_cpu->check_state();
				}
			}())
			{
				const u64 time0 = vm::reservation_acquire(eal);

				if (time0 & 127)
				{
					continue;
				}

				const auto cpu = get_current_cpu_thread<spu_thread>();

				alignas(64) u8 temp[128];
				u8* dst0 = cpu && (eal & -128) == cpu->raddr ? temp : dst;

				if (dst0 == +temp && time0 != cpu->rtime)
				{
					// Validate rtime for read data
					cpu->set_events(SPU_EVENT_LR);
					cpu->raddr = 0;
				}

				switch (size0)
				{
				case 1:
				{
					*reinterpret_cast<u8*>(dst0) = *reinterpret_cast<const u8*>(src);
					break;
				}
				case 2:
				{
					*reinterpret_cast<u16*>(dst0) = *reinterpret_cast<const u16*>(src);
					break;
				}
				case 4:
				{
					*reinterpret_cast<u32*>(dst0) = *reinterpret_cast<const u32*>(src);
					break;
				}
				case 8:
				{
					*reinterpret_cast<u64*>(dst0) = *reinterpret_cast<const u64*>(src);
					break;
				}
				case 128:
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst0), *reinterpret_cast<const spu_rdata_t*>(src));
					break;
				}
				default:
				{
					auto dst1 = dst0;
					auto src1 = src;
					auto size1 = size0;

					while (size1)
					{
						*reinterpret_cast<v128*>(dst1) = *reinterpret_cast<const v128*>(src1);

						dst1 += 16;
						src1 += 16;
						size1 -= 16;
					}

					break;
				}
				}

				if (time0 != vm::reservation_acquire(eal) || (size0 == 128 && !cmp_rdata(*reinterpret_cast<spu_rdata_t*>(dst0), *reinterpret_cast<const spu_rdata_t*>(src))))
				{
					continue;
				}

				if (dst0 == +temp)
				{
					// Write to LS
					std::memcpy(dst, dst0, size0);

					// Validate data
					if (std::memcmp(dst0, &cpu->rdata[eal & 127], size0) != 0)
					{
						cpu->set_events(SPU_EVENT_LR);
						cpu->raddr = 0;
					}
				}

				break;
			}

			if (size == size0)
			{
				if (g_cfg.core.mfc_debug && _this)
				{
					auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
					dump.cmd = args;
					dump.cmd.eah = _this->pc;
					std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
				}

				return;
			}
		}

		if (g_cfg.core.spu_accurate_dma) [[unlikely]]
		{
			for (u32 size0, size = args.size;; size -= size0, dst += size0, src += size0, eal += size0)
			{
				size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

				if (size0 == 128u && g_cfg.core.accurate_cache_line_stores)
				{
					// As atomic as PUTLLUC
					do_cell_atomic_128_store(eal, src);

					if (size == size0)
					{
						break;
					}

					continue;
				}

				// Lock each cache line
				auto& res = vm::reservation_acquire(eal);

				// Lock each bit corresponding to a byte being written, using some free space in reservation memory
				auto* bits = utils::bless<atomic_t<u128>>(vm::g_reservations + ((eal & 0xff80) / 2 + 16));

				// Get writing mask
				const u128 wmask = (~u128{} << (eal & 127)) & (~u128{} >> (127 - ((eal + size0 - 1) & 127)));
				//const u64 start = (eal & 127) / 2;
				//const u64 _end_ = ((eal + size0 - 1) & 127) / 2;
				//const u64 wmask = (UINT64_MAX << start) & (UINT64_MAX >> (63 - _end_));

				u128 old = 0;

				for (u64 i = 0; i != umax; [&]()
				{
					if (_cpu->state & cpu_flag::pause)
					{
						const bool ok = cpu_thread::if_suspended<0>(_cpu, {dst, dst + 64, &res}, [&]
						{
							std::memcpy(dst, src, size0);
							res += 128;
						});

						if (ok)
						{
							// Exit loop and function
							i = -1;
							bits = nullptr;
							return;
						}
					}

					if (++i < 10)
					{
						busy_wait(500);
					}
					else
					{
						// Wait
						_cpu->state += cpu_flag::wait + cpu_flag::temp;
						bits->wait(old, wmask);
						_cpu->check_state();
					}
				}())
				{
					// Completed in suspend_all()
					if (!bits)
					{
						break;
					}

					bool ok = false;

					std::tie(old, ok) = bits->fetch_op([&](auto& v)
					{
						if (v & wmask)
						{
							return false;
						}

						v |= wmask;
						return true;
					});

					if (ok) [[likely]]
					{
						break;
					}
				}

				if (!bits)
				{
					if (size == size0)
					{
						break;
					}

					continue;
				}

				// Lock reservation (shared)
				auto [_oldd, _ok] = res.fetch_op([&](u64& r)
				{
					if (r & vm::rsrv_unique_lock)
					{
						return false;
					}

					r += 1;
					return true;
				});

				if (!_ok)
				{
					vm::reservation_shared_lock_internal(res);
				}

				// Obtain range lock as normal store
				vm::range_lock(range_lock, eal, size0);

				switch (size0)
				{
				case 1:
				{
					*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
					break;
				}
				case 2:
				{
					*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
					break;
				}
				case 4:
				{
					*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
					break;
				}
				case 8:
				{
					*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
					break;
				}
				case 128:
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));
					break;
				}
				default:
				{
					auto _dst = dst;
					auto _src = src;
					auto _size = size0;

					while (_size)
					{
						*reinterpret_cast<v128*>(_dst) = *reinterpret_cast<const v128*>(_src);

						_dst += 16;
						_src += 16;
						_size -= 16;
					}

					break;
				}
				}

				range_lock->release(0);

				res += 127;

				// Release bits and notify
				bits->atomic_op([&](auto& v)
				{
					v &= ~wmask;
				});

				bits->notify_all(wmask);

				if (size == size0)
				{
					break;
				}
			}

			//atomic_fence_seq_cst();

			if (g_cfg.core.mfc_debug && _this)
			{
				auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
				dump.cmd = args;
				dump.cmd.eah = _this->pc;
				std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
			}

			return;
		}
		else
		{
			perf_put.reset();
			perf_get.reset();
		}

		perf_meter<"DMA_PUT"_u64> perf2 = perf_;

		switch (u32 size = args.size)
		{
		case 1:
		{
			vm::range_lock<1>(range_lock, eal, 1);
			*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
			range_lock->release(0);
			break;
		}
		case 2:
		{
			vm::range_lock<2>(range_lock, eal, 2);
			*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
			range_lock->release(0);
			break;
		}
		case 4:
		{
			vm::range_lock<4>(range_lock, eal, 4);
			*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
			range_lock->release(0);
			break;
		}
		case 8:
		{
			vm::range_lock<8>(range_lock, eal, 8);
			*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
			range_lock->release(0);
			break;
		}
		default:
		{
			if (((eal & 127) + size) <= 128)
			{
				vm::range_lock(range_lock, eal, size);

				while (size)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}

				range_lock->release(0);
				break;
			}

			u32 range_addr = eal & -128;
			u32 range_end = utils::align(eal + size, 128);

			// Handle the case of crossing 64K page borders (TODO: maybe split in 4K fragments?)
			if (range_addr >> 16 != (range_end - 1) >> 16)
			{
				u32 nexta = range_end & -65536;
				u32 size0 = nexta - eal;
				size -= size0;

				// Split locking + transfer in two parts (before 64K border, and after it)
				vm::range_lock(range_lock, range_addr, size0);

				// Avoid unaligned stores in mov_rdata_avx
				if (reinterpret_cast<u64>(dst) & 0x10)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size0 -= 16;
				}

				while (size0 >= 128)
				{
					mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

					dst += 128;
					src += 128;
					size0 -= 128;
				}

				while (size0)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size0 -= 16;
				}

				range_lock->release(0);
				range_addr = nexta;
			}

			vm::range_lock(range_lock, range_addr, range_end - range_addr);

			// Avoid unaligned stores in mov_rdata_avx
			if (reinterpret_cast<u64>(dst) & 0x10)
			{
				*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

				dst += 16;
				src += 16;
				size -= 16;
			}

			while (size >= 128)
			{
				mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

				dst += 128;
				src += 128;
				size -= 128;
			}

			while (size)
			{
				*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

				dst += 16;
				src += 16;
				size -= 16;
			}

			range_lock->release(0);
			break;
		}
		}

		if (g_cfg.core.mfc_debug && _this)
		{
			auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
			dump.cmd = args;
			dump.cmd.eah = _this->pc;
			std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
		}

		return;
	}

plain_access:

	switch (u32 size = args.size)
	{
	case 1:
	{
		*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
		break;
	}
	case 2:
	{
		*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
		break;
	}
	case 4:
	{
		*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
		break;
	}
	case 8:
	{
		*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
		break;
	}
	default:
	{
		// Avoid unaligned stores in mov_rdata_avx
		if (reinterpret_cast<u64>(dst) & 0x10)
		{
			*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

			dst += 16;
			src += 16;
			size -= 16;
		}

		while (size >= 128)
		{
			mov_rdata(*reinterpret_cast<spu_rdata_t*>(dst), *reinterpret_cast<const spu_rdata_t*>(src));

			dst += 128;
			src += 128;
			size -= 128;
		}

		while (size)
		{
			*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

			dst += 16;
			src += 16;
			size -= 16;
		}

		break;
	}
	}

	if (g_cfg.core.mfc_debug && _this)
	{
		auto& dump = _this->mfc_history[_this->mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
		dump.cmd = args;
		dump.cmd.eah = _this->pc;
		std::memcpy(dump.data, is_get ? dst : src, std::min<u32>(args.size, 128));
	}
}

bool spu_thread::do_dma_check(const spu_mfc_cmd& args)
{
	const u32 mask = utils::rol32(1, args.tag);

	if (mfc_barrier & mask || (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && mfc_fence & mask)) [[unlikely]]
	{
		// Check for special value combination (normally impossible)
		if (false)
		{
			// Update barrier/fence masks if necessary
			mfc_barrier = 0;
			mfc_fence = 0;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if ((mfc_queue[i].cmd & ~0xc) == MFC_BARRIER_CMD)
				{
					mfc_barrier |= -1;
					mfc_fence |= utils::rol32(1, mfc_queue[i].tag);
					continue;
				}

				if (true)
				{
					const u32 _mask = utils::rol32(1u, mfc_queue[i].tag);

					// A command with barrier hard blocks that tag until it's been dealt with
					if (mfc_queue[i].cmd & MFC_BARRIER_MASK)
					{
						mfc_barrier |= _mask;
					}

					// A new command that has a fence can't be executed until the stalled list has been dealt with
					mfc_fence |= _mask;
				}
			}

			if (mfc_barrier & mask || (args.cmd & MFC_FENCE_MASK && mfc_fence & mask))
			{
				return false;
			}

			return true;
		}

		return false;
	}

	return true;
}

bool spu_thread::do_list_transfer(spu_mfc_cmd& args)
{
	// Amount of elements to fetch in one go
	constexpr u32 fetch_size = 6;

	struct alignas(8) list_element
	{
		be_t<u16> sb; // Stall-and-Notify bit (0x8000)
		be_t<u16> ts; // List Transfer Size
		be_t<u32> ea; // External Address Low
	};

	union
	{
		list_element items[fetch_size];
		alignas(v128) char bufitems[sizeof(items)];
	};

	spu_mfc_cmd transfer;
	transfer.eah  = 0;
	transfer.tag  = args.tag;
	transfer.cmd  = MFC(args.cmd & ~MFC_LIST_MASK);

	args.lsa &= 0x3fff0;
	args.eal &= 0x3fff8;

	u32 index = fetch_size;

	// Assume called with size greater than 0
	while (true)
	{
		// Check if fetching is needed
		if (index == fetch_size)
		{
			// Reset to elements array head
			index = 0;

			const auto src = _ptr<const void>(args.eal);
			const v128 data0 = v128::loadu(src, 0);
			const v128 data1 = v128::loadu(src, 1);
			const v128 data2 = v128::loadu(src, 2);

			reinterpret_cast<v128*>(bufitems)[0] = data0;
			reinterpret_cast<v128*>(bufitems)[1] = data1;
			reinterpret_cast<v128*>(bufitems)[2] = data2;
		}

		const u32 size = items[index].ts & 0x7fff;
		const u32 addr = items[index].ea;

		spu_log.trace("LIST: item=0x%016x, lsa=0x%05x", std::bit_cast<be_t<u64>>(items[index]), args.lsa | (addr & 0xf));

		if (size)
		{
			transfer.eal  = addr;
			transfer.lsa  = args.lsa | (addr & 0xf);
			transfer.size = size;

			do_dma_transfer(this, transfer, ls);
			const u32 add_size = std::max<u32>(size, 16);
			args.lsa += add_size;
		}

		args.size -= 8;

		if (!args.size)
		{
			// No more elements
			break;
		}

		args.eal += 8;

		if (items[index].sb & 0x8000) [[unlikely]]
		{
			ch_stall_mask |= utils::rol32(1, args.tag);

			if (!ch_stall_stat.get_count())
			{
				set_events(SPU_EVENT_SN);
			}

			ch_stall_stat.set_value(utils::rol32(1, args.tag) | ch_stall_stat.get_value());

			args.tag |= 0x80; // Set stalled status
			return false;
		}

		index++;
	}

	return true;
}

bool spu_thread::do_putllc(const spu_mfc_cmd& args)
{
	perf_meter<"PUTLLC-"_u64> perf0;
	perf_meter<"PUTLLC+"_u64> perf1 = perf0;

	// Store conditionally
	const u32 addr = args.eal & -128;

	if ([&]()
	{
		perf_meter<"PUTLLC."_u64> perf2 = perf0;

		if (raddr != addr)
		{
			return false;
		}

		const auto& to_write = _ref<spu_rdata_t>(args.lsa & 0x3ff80);
		auto& res = vm::reservation_acquire(addr);

		// TODO: Limit scope!!
		rsx::reservation_lock rsx_lock(addr, 128);

		if (!g_use_rtm && rtime != res)
		{
			return false;
		}

		if (!g_use_rtm && cmp_rdata(to_write, rdata))
		{
			// Writeback of unchanged data. Only check memory change
			return cmp_rdata(rdata, vm::_ref<spu_rdata_t>(addr)) && res.compare_and_swap_test(rtime, rtime + 128);
		}

		if (g_use_rtm) [[likely]]
		{
			switch (u64 count = spu_putllc_tx(addr, rtime, rdata, to_write))
			{
			case umax:
			{
				auto& data = *vm::get_super_ptr<spu_rdata_t>(addr);

				const bool ok = cpu_thread::suspend_all<+3>(this, {data, data + 64, &res}, [&]()
				{
					if ((res & -128) == rtime)
					{
						if (cmp_rdata(rdata, data))
						{
							mov_rdata(data, to_write);
							res += 127;
							return true;
						}
					}

					// Save previous data
					mov_rdata_nt(rdata, data);
					res -= 1;
					return false;
				});

				const u64 count2 = __rdtsc() - perf2.get();

				if (count2 > 20000 && g_cfg.core.perf_report) [[unlikely]]
				{
					perf_log.warning(u8"PUTLLC: took too long: %.3fµs (%u c) (addr=0x%x) (S)", count2 / (utils::get_tsc_freq() / 1000'000.), count2, addr);
				}

				if (ok)
				{
					break;
				}

				last_ftime = -1;
				[[fallthrough]];
			}
			case 0:
			{
				if (addr == last_faddr)
				{
					last_fail++;
				}

				if (last_ftime != umax)
				{
					last_faddr = 0;
					return false;
				}

				utils::prefetch_read(rdata);
				utils::prefetch_read(rdata + 64);
				last_faddr = addr;
				last_ftime = res.load() & -128;
				last_ftsc = __rdtsc();
				return false;
			}
			default:
			{
				if (count > 20000 && g_cfg.core.perf_report) [[unlikely]]
				{
					perf_log.warning(u8"PUTLLC: took too long: %.3fµs (%u c) (addr = 0x%x)", count / (utils::get_tsc_freq() / 1000'000.), count, addr);
				}

				break;
			}
			}

			if (addr == last_faddr)
			{
				last_succ++;
			}

			last_faddr = 0;
			return true;
		}

		auto [_oldd, _ok] = res.fetch_op([&](u64& r)
		{
			if ((r & -128) != rtime || (r & 127))
			{
				return false;
			}

			r += vm::rsrv_unique_lock;
			return true;
		});

		if (!_ok)
		{
			// Already locked or updated: give up
			return false;
		}

		vm::_ref<atomic_t<u32>>(addr) += 0;

		auto& super_data = *vm::get_super_ptr<spu_rdata_t>(addr);
		const bool success = [&]()
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr);

			if (cmp_rdata(rdata, super_data))
			{
				mov_rdata(super_data, to_write);
				res += 64;
				return true;
			}

			res -= 64;
			return false;
		}();

		return success;
	}())
	{
		vm::reservation_notifier(addr).notify_all(-128);
		raddr = 0;
		perf0.reset();
		return true;
	}
	else
	{
		if (raddr)
		{
			// Last check for event before we clear the reservation
			if (raddr == addr)
			{
				set_events(SPU_EVENT_LR);
			}
			else
			{
				get_events(SPU_EVENT_LR);
			}
		}

		if (!vm::check_addr(addr, vm::page_writable))
		{
			vm::_ref<atomic_t<u8>>(addr) += 0; // Access violate
		}

		raddr = 0;
		perf1.reset();
		return false;
	}
}

void do_cell_atomic_128_store(u32 addr, const void* to_write)
{
	perf_meter<"STORE128"_u64> perf0;

	const auto cpu = get_current_cpu_thread();
	rsx::reservation_lock rsx_lock(addr, 128);

	if (g_use_rtm) [[likely]]
	{
		u64 result = spu_putlluc_tx(addr, to_write, cpu);

		if (result == 0)
		{
			auto& sdata = *vm::get_super_ptr<spu_rdata_t>(addr);
			auto& res = vm::reservation_acquire(addr);

			cpu_thread::suspend_all<+2>(cpu, {&res}, [&]
			{
				mov_rdata_nt(sdata, *static_cast<const spu_rdata_t*>(to_write));
				res += 127;
			});
		}

		if (!result)
		{
			result = __rdtsc() - perf0.get();
		}

		if (result > 20000 && g_cfg.core.perf_report) [[unlikely]]
		{
			perf_log.warning(u8"STORE128: took too long: %.3fµs (%u c) (addr=0x%x)", result / (utils::get_tsc_freq() / 1000'000.), result, addr);
		}

		static_cast<void>(cpu->test_stopped());
	}
	else
	{
		auto& data = vm::_ref<spu_rdata_t>(addr);
		auto [res, time0] = vm::reservation_lock(addr);

		*reinterpret_cast<atomic_t<u32>*>(&data) += 0;

		auto& super_data = *vm::get_super_ptr<spu_rdata_t>(addr);
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr);
			mov_rdata(super_data, *static_cast<const spu_rdata_t*>(to_write));
			res += 64;
		}
	}
}

void spu_thread::do_putlluc(const spu_mfc_cmd& args)
{
	perf_meter<"PUTLLUC"_u64> perf0;

	const u32 addr = args.eal & -128;

	if (raddr && addr == raddr)
	{
		// Try to process PUTLLUC using PUTLLC when a reservation is active:
		// If it fails the reservation is cleared, LR event is set and we fallback to the main implementation
		// All of this is done atomically in PUTLLC
		if (do_putllc(args))
		{
			// Success, return as our job was done here
			return;
		}

		// Failure, fallback to the main implementation
	}

	do_cell_atomic_128_store(addr, _ptr<spu_rdata_t>(args.lsa & 0x3ff80));
	vm::reservation_notifier(addr).notify_all(-128);
}

void spu_thread::do_mfc(bool /*wait*/)
{
	u32 removed = 0;
	u32 barrier = 0;
	u32 fence = 0;

	// Process enqueued commands
	static_cast<void>(std::remove_if(mfc_queue + 0, mfc_queue + mfc_size, [&](spu_mfc_cmd& args)
	{
		// Select tag bit in the tag mask or the stall mask
		const u32 mask = utils::rol32(1, args.tag);

		if ((args.cmd & ~0xc) == MFC_BARRIER_CMD)
		{
			if (&args - mfc_queue <= removed)
			{
				// Remove barrier-class command if it's the first in the queue
				atomic_fence_seq_cst();
				removed++;
				return true;
			}

			// Block all tags
			barrier |= -1;
			fence |= mask;
			return false;
		}

		if (barrier & mask)
		{
			fence |= mask;
			return false;
		}

		if (args.cmd & (MFC_BARRIER_MASK | MFC_FENCE_MASK) && fence & mask)
		{
			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			return false;
		}

		if (args.cmd & MFC_LIST_MASK)
		{
			if (!(args.tag & 0x80))
			{
				if (do_list_transfer(args))
				{
					removed++;
					return true;
				}
			}

			if (args.cmd & MFC_BARRIER_MASK)
			{
				barrier |= mask;
			}

			fence |= mask;
			return false;
		}

		if (args.cmd == MFC_PUTQLLUC_CMD)
		{
			if (fence & mask)
			{
				return false;
			}

			do_putlluc(args);
		}
		else if (args.size)
		{
			do_dma_transfer(this, args, ls);
		}

		removed++;
		return true;
	}));

	mfc_size -= removed;
	mfc_barrier = barrier;
	mfc_fence = fence;

	if (removed && ch_tag_upd)
	{
		const u32 completed = get_mfc_completed();

		if (completed && ch_tag_upd == MFC_TAG_UPDATE_ANY)
		{
			ch_tag_stat.set_value(completed);
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
		}
		else if (completed == ch_tag_mask && ch_tag_upd == MFC_TAG_UPDATE_ALL)
		{
			ch_tag_stat.set_value(completed);
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
		}
	}

	if (check_mfc_interrupts(pc + 4))
	{
		spu_runtime::g_escape(this);
	}
}

bool spu_thread::check_mfc_interrupts(u32 next_pc)
{
	if (ch_events.load().count && std::exchange(interrupts_enabled, false))
	{
		srr0 = next_pc;

		// Test for BR/BRA instructions (they are equivalent at zero pc)
		const u32 br = _ref<u32>(0);
		pc = (br & 0xfd80007f) == 0x30000000 ? (br >> 5) & 0x3fffc : 0;
		return true;
	}

	return false;
}

bool spu_thread::is_exec_code(u32 addr) const
{
	if (addr & ~0x3FFFC)
	{
		return false;
	}

	for (u32 i = 0; i < 30; i++)
	{
		const u32 addr0 = addr + (i * 4);
		const u32 op = _ref<u32>(addr0);
		const auto type = s_spu_itype.decode(op);

		if (type == spu_itype::UNK || !op)
		{
			return false;
		}

		if (type & spu_itype::branch)
		{
			// TODO
			break;
		}
	}

	return true;
}

u32 spu_thread::get_mfc_completed() const
{
	return ch_tag_mask & ~mfc_fence;
}

bool spu_thread::process_mfc_cmd()
{
	// Stall infinitely if MFC queue is full
	while (mfc_size >= 16) [[unlikely]]
	{
		auto old = state.add_fetch(cpu_flag::wait);

		if (is_stopped(old))
		{
			return false;
		}

		thread_ctrl::wait_on(state, old);
	}

	spu::scheduler::concurrent_execution_watchdog watchdog(*this);
	spu_log.trace("DMAC: (%s)", ch_mfc_cmd);

	switch (ch_mfc_cmd.cmd)
	{
	case MFC_SDCRT_CMD:
	case MFC_SDCRTST_CMD:
		return true;
	case MFC_GETLLAR_CMD:
	{
		perf_meter<"GETLLAR"_u64> perf0;

		const u32 addr = ch_mfc_cmd.eal & -128;
		const auto& data = vm::_ref<spu_rdata_t>(addr);

		if (addr == last_faddr)
		{
			// TODO: make this configurable and possible to disable
			spu_log.trace(u8"GETLLAR after fail: addr=0x%x, time=%u c", last_faddr, (perf0.get() - last_ftsc));
		}

		if (addr == last_faddr && perf0.get() - last_ftsc < 1000 && (vm::reservation_acquire(addr) & -128) == last_ftime)
		{
			rtime = last_ftime;
			raddr = last_faddr;
			last_ftime = 0;
			mov_rdata(_ref<spu_rdata_t>(ch_mfc_cmd.lsa & 0x3ff80), rdata);

			ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
			return true;
		}
		else
		{
			// Silent failure
			last_faddr = 0;
		}

		if (addr == raddr && !g_use_rtm && g_cfg.core.spu_getllar_polling_detection && rtime == vm::reservation_acquire(addr) && cmp_rdata(rdata, data))
		{
			// Spinning, might as well yield cpu resources
			std::this_thread::yield();

			// Reset perf
			perf0.restart();
		}

		alignas(64) spu_rdata_t temp;
		u64 ntime;
		rsx::reservation_lock rsx_lock(addr, 128);

		if (raddr)
		{
			// Save rdata from previous reservation
			mov_rdata(temp, rdata);
		}

		for (u64 i = 0; i != umax; [&]()
		{
			if (state & cpu_flag::pause)
			{
				auto& sdata = *vm::get_super_ptr<spu_rdata_t>(addr);

				const bool ok = cpu_thread::if_suspended<0>(this, {&ntime}, [&]
				{
					// Guaranteed success
					ntime = vm::reservation_acquire(addr);
					mov_rdata_nt(rdata, sdata);
				});

				// Exit loop
				if (ok && (ntime & 127) == 0)
				{
					atomic_fence_seq_cst();
					i = -1;
					return;
				}
			}

			if (++i < 25) [[likely]]
			{
				busy_wait(300);
			}
			else
			{
				state += cpu_flag::wait + cpu_flag::temp;
				std::this_thread::yield();
				!check_state();
			}
		}())
		{
			ntime = vm::reservation_acquire(addr);

			if (ntime & vm::rsrv_unique_lock)
			{
				// There's an on-going reservation store, wait
				continue;
			}

			u64 test_mask = -1;

			if (ntime & 127)
			{
				// Try to use TSX to obtain data atomically
				if (!g_use_rtm || !spu_getllar_tx(addr, rdata, this, ntime & -128))
				{
					// See previous ntime check.
					continue;
				}
				else
				{
					// If succeeded, only need to check unique lock bit
					test_mask = ~vm::rsrv_shared_mask;
				}
			}
			else
			{
				mov_rdata(rdata, data);
			}

			if (u64 time0 = vm::reservation_acquire(addr); (ntime & test_mask) != (time0 & test_mask))
			{
				// Reservation data has been modified recently
				if (time0 & vm::rsrv_unique_lock) i += 12;
				continue;
			}

			if (g_cfg.core.spu_accurate_getllar && !cmp_rdata(rdata, data))
			{
				i += 2;
				continue;
			}

			if (i >= 15 && g_cfg.core.perf_report) [[unlikely]]
			{
				perf_log.warning("GETLLAR: took too long: %u", i);
			}

			break;
		}

		if (raddr && raddr != addr)
		{
			// Last check for event before we replace the reservation with a new one
			if (reservation_check(raddr, temp))
			{
				set_events(SPU_EVENT_LR);
			}
		}
		else if (raddr == addr)
		{
			// Lost previous reservation on polling
			if (ntime != rtime || !cmp_rdata(rdata, temp))
			{
				set_events(SPU_EVENT_LR);
			}
		}

		raddr = addr;
		rtime = ntime;
		mov_rdata(_ref<spu_rdata_t>(ch_mfc_cmd.lsa & 0x3ff80), rdata);

		ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);

		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % spu_thread::max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			std::memcpy(dump.data, rdata, 128);
		}

		return true;
	}

	case MFC_PUTLLC_CMD:
	{
		// Avoid logging useless commands if there is no reservation
		const bool dump = g_cfg.core.mfc_debug && raddr;

		if (do_putllc(ch_mfc_cmd))
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_SUCCESS);
		}
		else
		{
			ch_atomic_stat.set_value(MFC_PUTLLC_FAILURE);
		}

		if (dump)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			dump.cmd.tag = static_cast<u32>(ch_atomic_stat.get_value()); // Use tag as atomic status
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		return !test_stopped();
	}
	case MFC_PUTLLUC_CMD:
	{
		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		do_putlluc(ch_mfc_cmd);
		ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		return !test_stopped();
	}
	case MFC_PUTQLLUC_CMD:
	{
		if (g_cfg.core.mfc_debug)
		{
			auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			dump.cmd = ch_mfc_cmd;
			dump.cmd.eah = pc;
			std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.lsa & 0x3ff80), 128);
		}

		const u32 mask = utils::rol32(1, ch_mfc_cmd.tag);

		if ((mfc_barrier | mfc_fence) & mask) [[unlikely]]
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= mask;
		}
		else
		{
			do_putlluc(ch_mfc_cmd);
		}

		return true;
	}
	case MFC_SNDSIG_CMD:
	case MFC_SNDSIGB_CMD:
	case MFC_SNDSIGF_CMD:
	{
		if (ch_mfc_cmd.size != 4)
		{
			break;
		}

		[[fallthrough]];
	}
	case MFC_PUT_CMD:
	case MFC_PUTB_CMD:
	case MFC_PUTF_CMD:
	case MFC_PUTR_CMD:
	case MFC_PUTRB_CMD:
	case MFC_PUTRF_CMD:
	case MFC_GET_CMD:
	case MFC_GETB_CMD:
	case MFC_GETF_CMD:
	case MFC_SDCRZ_CMD:
	{
		if (ch_mfc_cmd.size <= 0x4000) [[likely]]
		{
			if (do_dma_check(ch_mfc_cmd)) [[likely]]
			{
				if (ch_mfc_cmd.size)
				{
					do_dma_transfer(this, ch_mfc_cmd, ls);
				}

				return true;
			}

			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_fence |= utils::rol32(1, ch_mfc_cmd.tag);

			if (ch_mfc_cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, ch_mfc_cmd.tag);
			}

			return true;
		}

		break;
	}
	case MFC_PUTL_CMD:
	case MFC_PUTLB_CMD:
	case MFC_PUTLF_CMD:
	case MFC_PUTRL_CMD:
	case MFC_PUTRLB_CMD:
	case MFC_PUTRLF_CMD:
	case MFC_GETL_CMD:
	case MFC_GETLB_CMD:
	case MFC_GETLF_CMD:
	{
		if (ch_mfc_cmd.size <= 0x4000) [[likely]]
		{
			auto& cmd = mfc_queue[mfc_size];
			cmd = ch_mfc_cmd;

			//if (g_cfg.core.mfc_debug)
			//{
			//  TODO: This needs a disambiguator with list elements dumping
			//	auto& dump = mfc_history[mfc_dump_idx++ % max_mfc_dump_idx];
			//	dump.cmd = ch_mfc_cmd;
			//	dump.cmd.eah = pc;
			//	std::memcpy(dump.data, _ptr<u8>(ch_mfc_cmd.eah & 0x3fff0), std::min<u32>(ch_mfc_cmd.size, 128));
			//}

			if (do_dma_check(cmd)) [[likely]]
			{
				if (!cmd.size || do_list_transfer(cmd)) [[likely]]
				{
					return true;
				}
			}

			mfc_size++;
			mfc_fence |= utils::rol32(1, cmd.tag);

			if (cmd.cmd & MFC_BARRIER_MASK)
			{
				mfc_barrier |= utils::rol32(1, cmd.tag);
			}

			if (check_mfc_interrupts(pc + 4))
			{
				spu_runtime::g_escape(this);
			}

			return true;
		}

		break;
	}
	case MFC_BARRIER_CMD:
	case MFC_EIEIO_CMD:
	case MFC_SYNC_CMD:
	{
		if (mfc_size == 0)
		{
			atomic_fence_seq_cst();
		}
		else
		{
			mfc_queue[mfc_size++] = ch_mfc_cmd;
			mfc_barrier |= -1;
			mfc_fence |= utils::rol32(1, ch_mfc_cmd.tag);
		}

		return true;
	}
	default:
	{
		break;
	}
	}

	fmt::throw_exception("Unknown command (cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)",
		ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);
}

bool spu_thread::reservation_check(u32 addr, const decltype(rdata)& data) const
{
	if (!addr)
	{
		// No reservation to be lost in the first place
		return false;
	}

	if ((vm::reservation_acquire(addr) & -128) != rtime)
	{
		return true;
	}

	// Ensure data is allocated (HACK: would raise LR event if not)
	// Set range_lock first optimistically
	range_lock->store(u64{128} << 32 | addr);

	u64 lock_val = vm::g_range_lock;
	u64 old_lock = 0;

	while (lock_val != old_lock)
	{
		// Since we want to read data, let's check readability first
		if (!(lock_val & vm::range_readable))
		{
			// Only one abnormal operation is "unreadable"
			if ((lock_val >> vm::range_pos) == (vm::range_locked >> vm::range_pos))
			{
				// All page flags are untouched and can be read safely
				if (!vm::check_addr(addr))
				{
					// Assume our memory is being (de)allocated
					range_lock->release(0);
					break;
				}

				// g_shmem values are unchanged too
				const u64 is_shmem = vm::g_shmem[addr >> 16];

				const u64 test_addr = is_shmem ? (is_shmem | static_cast<u16>(addr)) / 128 : u64{addr} / 128;
				const u64 lock_addr = lock_val / 128;

				if (test_addr == lock_addr)
				{
					// Our reservation is locked
					range_lock->release(0);
					break;
				}

				break;
			}
		}

		// Fallback to normal range check
		const u64 lock_addr = static_cast<u32>(lock_val);
		const u32 lock_size = static_cast<u32>(lock_val << 3 >> 35);

		if (lock_addr + lock_size <= addr || lock_addr >= addr + 128)
		{
			// We are outside locked range, so page flags are unaffected
			if (!vm::check_addr(addr))
			{
				range_lock->release(0);
				break;
			}
		}
		else if (!(lock_val & vm::range_readable))
		{
			range_lock->release(0);
			break;
		}

		old_lock = std::exchange(lock_val, vm::g_range_lock);
	}

	if (!range_lock->load()) [[unlikely]]
	{
		return true;
	}

	const bool res = cmp_rdata(data, vm::_ref<decltype(rdata)>(addr));

	range_lock->release(0);
	return !res;
}

spu_thread::ch_events_t spu_thread::get_events(u32 mask_hint, bool waiting, bool reading)
{
	if (auto mask1 = ch_events.load().mask; mask1 & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)", mask1);
	}

retry:
	u32 collect = 0;

	// Check reservation status and set SPU_EVENT_LR if lost
	if (mask_hint & SPU_EVENT_LR)
	{
		if (reservation_check(raddr, rdata))
		{
			collect |= SPU_EVENT_LR;
			raddr = 0;
		}
	}

	// SPU Decrementer Event on underflow (use the upper 32-bits to determine it)
	if (mask_hint & SPU_EVENT_TM)
	{
		if (const u64 res = (ch_dec_value - (get_timebased_time() - ch_dec_start_timestamp)) >> 32)
		{
			// Set next event to the next time the decrementer underflows
			ch_dec_start_timestamp -= res << 32;
			collect |= SPU_EVENT_TM;
		}
	}

	if (collect)
	{
		set_events(collect);
	}

	auto [res, ok] = ch_events.fetch_op([&](ch_events_t& events)
	{
		if (!reading)
			return false;
		if (waiting)
			events.waiting = !events.count;

		events.count = false;
		return true;
	});

	if (reading && res.locks && mask_hint & (SPU_EVENT_S1 | SPU_EVENT_S2))
	{
		busy_wait(100);
		goto retry;
	}

	return res;
}

void spu_thread::set_events(u32 bits)
{
	if (ch_events.atomic_op([&](ch_events_t& events)
	{
		events.events |= bits;

		// If one masked event was fired, set the channel count (even if the event bit was already 1)
		if (events.mask & bits)
		{
			events.count = true;
			return !!events.waiting;
		}

		return false;
	}))
	{
		notify();
	}
}

void spu_thread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// Detect enabling interrupts with events masked
		if (auto mask = ch_events.load().mask; mask & ~SPU_EVENT_INTR_IMPLEMENTED)
		{
			fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x)", mask);
		}
	}

	interrupts_enabled = enable;
}

u32 spu_thread::get_ch_count(u32 ch)
{
	if (ch < 128) spu_log.trace("get_ch_count(ch=%s)", spu_ch_name[ch]);

	switch (ch)
	{
	case SPU_WrOutMbox:       return ch_out_mbox.get_count() ^ 1;
	case SPU_WrOutIntrMbox:   return ch_out_intr_mbox.get_count() ^ 1;
	case SPU_RdInMbox:        return ch_in_mbox.get_count();
	case MFC_RdTagStat:       return ch_tag_stat.get_count();
	case MFC_RdListStallStat: return ch_stall_stat.get_count();
	case MFC_WrTagUpdate:     return 1;
	case SPU_RdSigNotify1:    return ch_snr1.get_count();
	case SPU_RdSigNotify2:    return ch_snr2.get_count();
	case MFC_RdAtomicStat:    return ch_atomic_stat.get_count();
	case SPU_RdEventStat:     return get_events().count;
	case MFC_Cmd:             return 16 - mfc_size;

	// Channels with a constant count of 1:
	case SPU_WrEventMask:
	case SPU_WrEventAck:
	case SPU_WrDec:
	case SPU_RdDec:
	case SPU_RdEventMask:
	case SPU_RdMachStat:
	case SPU_WrSRR0:
	case SPU_RdSRR0:
	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	case MFC_RdTagMask:
	case MFC_LSA:
	case MFC_EAH:
	case MFC_EAL:
	case MFC_Size:
	case MFC_TagID:
	case MFC_WrTagMask:
	case MFC_WrListStallAck:
		return 1;
	default: break;
	}

	ensure(ch < 128u);
	spu_log.error("Unknown/illegal channel in RCHCNT (ch=%s)", spu_ch_name[ch]);
	return 0; // Default count
}

s64 spu_thread::get_ch_value(u32 ch)
{
	if (ch < 128) spu_log.trace("get_ch_value(ch=%s)", spu_ch_name[ch]);

	auto read_channel = [&](spu_channel& channel) -> s64
	{
		if (channel.get_count() == 0)
		{
			state += cpu_flag::wait + cpu_flag::temp;
		}

		for (int i = 0; i < 10 && channel.get_count() == 0; i++)
		{
			busy_wait();
		}

		const s64 out = channel.pop_wait(*this);
		static_cast<void>(test_stopped());
		return out;
	};

	switch (ch)
	{
	case SPU_RdSRR0:
	{
		return srr0;
	}
	case SPU_RdInMbox:
	{
		if (ch_in_mbox.get_count() == 0)
		{
			state += cpu_flag::wait;
		}

		while (true)
		{
			for (int i = 0; i < 10 && ch_in_mbox.get_count() == 0; i++)
			{
				busy_wait();
			}

			u32 out = 0;

			if (const uint old_count = ch_in_mbox.try_pop(out))
			{
				if (old_count == 4 /* SPU_IN_MBOX_THRESHOLD */) // TODO: check this
				{
					int_ctrl[2].set(SPU_INT2_STAT_SPU_MAILBOX_THRESHOLD_INT);
				}

				check_state();
				return out;
			}

			auto old = +state;

			if (is_stopped(old))
			{
				return -1;
			}

			thread_ctrl::wait_on(state, old);
		}
	}

	case MFC_RdTagStat:
	{
		if (u32 out; ch_tag_stat.try_read(out))
		{
			ch_tag_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_tag_stat);
	}

	case MFC_RdTagMask:
	{
		return ch_tag_mask;
	}

	case SPU_RdSigNotify1:
	{
		return read_channel(ch_snr1);
	}

	case SPU_RdSigNotify2:
	{
		return read_channel(ch_snr2);
	}

	case MFC_RdAtomicStat:
	{
		if (u32 out; ch_atomic_stat.try_read(out))
		{
			ch_atomic_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_atomic_stat);
	}

	case MFC_RdListStallStat:
	{
		if (u32 out; ch_stall_stat.try_read(out))
		{
			ch_stall_stat.set_value(0, false);
			return out;
		}

		// Will stall infinitely
		return read_channel(ch_stall_stat);
	}

	case SPU_RdDec:
	{
		u32 out = ch_dec_value - static_cast<u32>(get_timebased_time() - ch_dec_start_timestamp);

		//Polling: We might as well hint to the scheduler to slot in another thread since this one is counting down
		if (g_cfg.core.spu_loop_detection && out > spu::scheduler::native_jiffy_duration_us)
		{
			state += cpu_flag::wait;
			std::this_thread::yield();
		}

		return out;
	}

	case SPU_RdEventMask:
	{
		return ch_events.load().mask;
	}

	case SPU_RdEventStat:
	{
		const u32 mask1 = ch_events.load().mask;
		auto events = get_events(mask1, false, true);

		if (events.count)
		{
			return events.events & mask1;
		}

		spu_function_logger logger(*this, "MFC Events read");

		if (mask1 & SPU_EVENT_LR && raddr)
		{
			if (mask1 != SPU_EVENT_LR && mask1 != SPU_EVENT_LR + SPU_EVENT_TM)
			{
				// Combining LR with other flags needs another solution
				fmt::throw_exception("Not supported: event mask 0x%x", mask1);
			}

			for (; !events.count; events = get_events(mask1, false, true))
			{
				const auto old = state.add_fetch(cpu_flag::wait);

				if (is_stopped(old))
				{
					return -1;
				}

				if (is_paused(old))
				{
					// Ensure reservation data won't change while paused for debugging purposes
					check_state();
					continue;
				}

				vm::reservation_notifier(raddr).wait(rtime, -128, atomic_wait_timeout{100'000});
			}

			check_state();
			return events.events & mask1;
		}

		for (; !events.count; events = get_events(mask1, true, true))
		{
			const auto old = state.add_fetch(cpu_flag::wait);

			if (is_stopped(old))
			{
				return -1;
			}

			if (is_paused(old))
			{
				check_state();
				continue;
			}

			thread_ctrl::wait_on(state, old, 100);
		}

		check_state();
		return events.events & mask1;
	}

	case SPU_RdMachStat:
	{
		// Return SPU Interrupt status in LSB
		return u32{interrupts_enabled} | (u32{get_type() == spu_type::isolated} << 1);
	}
	}

	fmt::throw_exception("Unknown/illegal channel in RDCH (ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool spu_thread::set_ch_value(u32 ch, u32 value)
{
	if (ch < 128) spu_log.trace("set_ch_value(ch=%s, value=0x%x)", spu_ch_name[ch], value);

	switch (ch)
	{
	case SPU_WrSRR0:
	{
		srr0 = value & 0x3fffc;
		return true;
	}

	case SPU_WrOutIntrMbox:
	{
		if (get_type() >= spu_type::raw)
		{
			if (ch_out_intr_mbox.get_count())
			{
				state += cpu_flag::wait;
			}

			if (!ch_out_intr_mbox.push_wait(*this, value))
			{
				return false;
			}

			int_ctrl[2].set(SPU_INT2_STAT_MAILBOX_INT);
			check_state();
			return true;
		}

		state += cpu_flag::wait;

		const u32 code = value >> 24;
		{
			if (code < 64)
			{
				/* ===== sys_spu_thread_send_event (used by spu_printf) ===== */

				u32 spup = code & 63;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				spu_log.trace("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				std::lock_guard lock(group->mutex);

				const auto queue = this->spup[spup].get();

				const auto res = ch_in_mbox.get_count() ? CELL_EBUSY :
					!queue ? CELL_ENOTCONN :
					queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, lv2_id, (u64{spup} << 32) | (value & 0x00ffffff), data);

				if (ch_in_mbox.get_count())
				{
					spu_log.warning("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): In_MBox is not empty (%d)", spup, (value & 0x00ffffff), data, ch_in_mbox.get_count());
				}
				else if (res == CELL_ENOTCONN)
				{
					spu_log.warning("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x): error (%s)", spup, (value & 0x00ffffff), data, res);
				}

				ch_in_mbox.set_values(1, res);
				return true;
			}
			else if (code < 128)
			{
				/* ===== sys_spu_thread_throw_event ===== */

				u32 spup = code & 63;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty", value, spup);
				}

				spu_log.trace("sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = (std::lock_guard{group->mutex}, this->spup[spup]);

				// TODO: check passing spup value
				if (auto res = queue ? queue->send(SYS_SPU_THREAD_EVENT_USER_KEY, lv2_id, (u64{spup} << 32) | (value & 0x00ffffff), data) : CELL_ENOTCONN)
				{
					spu_log.warning("sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x) failed (error=%s)", spup, (value & 0x00ffffff), data, res);
				}

				return true;
			}
			else if (code == 128)
			{
				/* ===== sys_event_flag_set_bit ===== */

				u32 flag = value & 0xffffff;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);

				std::lock_guard lock(group->mutex);

				// Use the syscall to set flag
				const auto res = ch_in_mbox.get_count() ? CELL_EBUSY : 0u + sys_event_flag_set(*this, data, 1ull << flag);

				if (res == CELL_EBUSY)
				{
					spu_log.warning("sys_event_flag_set_bit(value=0x%x (flag=%d)): In_MBox is not empty (%d)", value, flag, ch_in_mbox.get_count());
				}

				ch_in_mbox.set_values(1, res);
				return true;
			}
			else if (code == 192)
			{
				/* ===== sys_event_flag_set_bit_impatient ===== */

				u32 flag = value & 0xffffff;
				u32 data = 0;

				if (!ch_out_mbox.try_pop(data))
				{
					fmt::throw_exception("sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty", value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);

				// Use the syscall to set flag
				sys_event_flag_set(*this, data, 1ull << flag);
				return true;
			}
			else
			{
				fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x, Out_MBox=%s)", value, ch_out_mbox);
			}
		}
	}

	case SPU_WrOutMbox:
	{
		if (ch_out_mbox.get_count())
		{
			state += cpu_flag::wait;
		}

		if (!ch_out_mbox.push_wait(*this, value))
		{
			return false;
		}

		check_state();
		return true;
	}

	case MFC_WrTagMask:
	{
		ch_tag_mask = value;

		if (ch_tag_upd)
		{
			const u32 completed = get_mfc_completed();

			if (completed && ch_tag_upd == MFC_TAG_UPDATE_ANY)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
			else if (completed == value && ch_tag_upd == MFC_TAG_UPDATE_ALL)
			{
				ch_tag_stat.set_value(completed);
				ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			}
		}

		return true;
	}

	case MFC_WrTagUpdate:
	{
		if (value > MFC_TAG_UPDATE_ALL)
		{
			break;
		}

		const u32 completed = get_mfc_completed();

		if (!value)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else if (completed && value == MFC_TAG_UPDATE_ANY)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else if (completed == ch_tag_mask && value == MFC_TAG_UPDATE_ALL)
		{
			ch_tag_upd = MFC_TAG_UPDATE_IMMEDIATE;
			ch_tag_stat.set_value(completed);
		}
		else
		{
			ch_tag_upd = value;
		}

		return true;
	}

	case MFC_LSA:
	{
		ch_mfc_cmd.lsa = value;
		return true;
	}

	case MFC_EAH:
	{
		ch_mfc_cmd.eah = value;
		return true;
	}

	case MFC_EAL:
	{
		ch_mfc_cmd.eal = value;
		return true;
	}

	case MFC_Size:
	{
		ch_mfc_cmd.size = value & 0x7fff;
		return true;
	}

	case MFC_TagID:
	{
		ch_mfc_cmd.tag = value & 0x1f;
		return true;
	}

	case MFC_Cmd:
	{
		ch_mfc_cmd.cmd = MFC(value & 0xff);
		return process_mfc_cmd();
	}

	case MFC_WrListStallAck:
	{
		value &= 0x1f;

		// Reset stall status for specified tag
		const u32 tag_mask = utils::rol32(1, value);

		if (ch_stall_mask & tag_mask)
		{
			ch_stall_mask &= ~tag_mask;

			for (u32 i = 0; i < mfc_size; i++)
			{
				if (mfc_queue[i].tag == (value | 0x80))
				{
					// Unset stall bit
					mfc_queue[i].tag &= 0x7f;
				}
			}

			do_mfc(true);
		}

		return true;
	}

	case SPU_WrDec:
	{
		get_events(SPU_EVENT_TM); // Don't discard possibly occured old event
		ch_dec_start_timestamp = get_timebased_time();
		ch_dec_value = value;
		return true;
	}

	case SPU_WrEventMask:
	{
		get_events(value);

		if (ch_events.atomic_op([&](ch_events_t& events)
		{
			events.mask = value;

			if (events.events & events.mask)
			{
				events.count = true;
				return true;
			}

			return false;
		}))
		{
			// Check interrupts in case count is 1
			if (check_mfc_interrupts(pc + 4))
			{
				spu_runtime::g_escape(this);
			}
		}

		return true;
	}

	case SPU_WrEventAck:
	{
		// "Collect" events before final acknowledgment
		get_events(value);

		if (ch_events.atomic_op([&](ch_events_t& events)
		{
			events.events &= ~value;

			if (events.events & events.mask)
			{
				events.count = true;
				return true;
			}

			return false;
		}))
		{
			// Check interrupts in case count is 1
			if (check_mfc_interrupts(pc + 4))
			{
				spu_runtime::g_escape(this);
			}
		}

		return true;
	}

	case SPU_Set_Bkmk_Tag:
	case SPU_PM_Start_Ev:
	case SPU_PM_Stop_Ev:
	{
		return true;
	}
	}

	fmt::throw_exception("Unknown/illegal channel in WRCH (ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);
}

extern void resume_spu_thread_group_from_waiting(spu_thread& spu)
{
	const auto group = spu.group;

	std::lock_guard lock(group->mutex);

	if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
	}
	else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
	{
		group->run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		spu.state += cpu_flag::signal;
		spu.state.notify_one(cpu_flag::signal);
		return;
	}

	for (auto& thread : group->threads)
	{
		if (thread)
		{
			if (thread.get() == &spu)
			{
				constexpr auto flags = cpu_flag::suspend + cpu_flag::signal;
				ensure(((thread->state ^= flags) & flags) == cpu_flag::signal);
			}
			else
			{
				thread->state -= cpu_flag::suspend;
			}

			thread->state.notify_one(cpu_flag::suspend + cpu_flag::signal);
		}
	}
}

bool spu_thread::stop_and_signal(u32 code)
{
	spu_log.trace("stop_and_signal(code=0x%x)", code);

	auto set_status_npc = [&]()
	{
		status_npc.atomic_op([&](status_npc_sync_var& state)
		{
			state.status = (state.status & 0xffff) | (code << 16);
			state.status |= SPU_STATUS_STOPPED_BY_STOP;
			state.status &= ~SPU_STATUS_RUNNING;
			state.npc = (pc + 4) | +interrupts_enabled;
		});
	};

	if (get_type() >= spu_type::raw)
	{
		// Save next PC and current SPU Interrupt Status
		state += cpu_flag::stop + cpu_flag::wait + cpu_flag::ret;
		set_status_npc();

		status_npc.notify_one();

		int_ctrl[2].set(SPU_INT2_STAT_SPU_STOP_AND_SIGNAL_INT);
		check_state();
		return true;
	}

	auto get_queue = [this](u32 spuq) -> const std::shared_ptr<lv2_event_queue>&
	{
		for (auto& v : this->spuq)
		{
			if (spuq == v.first)
			{
				if (lv2_obj::check(v.second))
				{
					return v.second;
				}
			}
		}

		static const std::shared_ptr<lv2_event_queue> empty;
		return empty;
	};

	switch (code)
	{
	case 0x001:
	{
		state += cpu_flag::wait;
		std::this_thread::sleep_for(1ms); // hack
		check_state();
		return true;
	}

	case 0x002:
	{
		state += cpu_flag::ret;
		return true;
	}

	case SYS_SPU_THREAD_STOP_RECEIVE_EVENT:
	{
		/* ===== sys_spu_thread_receive_event ===== */

		u32 spuq = 0;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_receive_event(): Out_MBox is empty");
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			spu_log.error("sys_spu_thread_receive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		spu_log.trace("sys_spu_thread_receive_event(spuq=0x%x)", spuq);

		if (!group->has_scheduler_context /*|| group->type & 0xf00*/)
		{
			spu_log.error("sys_spu_thread_receive_event(): Incompatible group type = 0x%x", group->type);
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		state += cpu_flag::wait;

		spu_function_logger logger(*this, "sys_spu_thread_receive_event");

		std::shared_ptr<lv2_event_queue> queue;
	
		while (true)
		{
			// Check group status, wait if necessary
			for (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
				_state = group->run_state)
			{
				const auto old = state.load();

				if (is_stopped(old))
				{
					return false;
				}

				thread_ctrl::wait_on(state, old);;
			}

			reader_lock{group->mutex}, queue = get_queue(spuq);

			if (!queue)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			// Lock queue's mutex first, then group's mutex
			std::scoped_lock lock(queue->mutex, group->mutex);

			if (is_stopped())
			{
				return false;
			}

			if (group->run_state >= SPU_THREAD_GROUP_STATUS_WAITING && group->run_state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// Try again
				continue;
			}

			if (queue != get_queue(spuq))
			{
				// Try again
				continue;
			}

			if (!queue->exists)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			if (queue->events.empty())
			{
				queue->sq.emplace_back(this);
				group->run_state = SPU_THREAD_GROUP_STATUS_WAITING;

				for (auto& thread : group->threads)
				{
					if (thread)
					{
						thread->state += cpu_flag::suspend;
					}
				}

				// Wait
				break;
			}
			else
			{
				// Return the event immediately
				const auto event = queue->events.front();
				const auto data1 = static_cast<u32>(std::get<1>(event));
				const auto data2 = static_cast<u32>(std::get<2>(event));
				const auto data3 = static_cast<u32>(std::get<3>(event));
				ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
				queue->events.pop_front();
				return true;
			}
		}

		while (auto old = state.fetch_sub(cpu_flag::signal))
		{
			if (is_stopped(old))
			{
				return false;
			}

			if (old & cpu_flag::signal)
			{
				break;
			}

			thread_ctrl::wait_on(state, old);
		}

		return true;
	}

	case SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT:
	{
		/* ===== sys_spu_thread_tryreceive_event ===== */

		u32 spuq = 0;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_tryreceive_event(): Out_MBox is empty");
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			spu_log.error("sys_spu_thread_tryreceive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		spu_log.trace("sys_spu_thread_tryreceive_event(spuq=0x%x)", spuq);

		std::shared_ptr<lv2_event_queue> queue;

		reader_lock{group->mutex}, queue = get_queue(spuq);

		std::unique_lock<shared_mutex> qlock, group_lock;

		while (true)
		{
			if (!queue)
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			// Lock queue's mutex first, then group's mutex
			qlock = std::unique_lock{queue->mutex};
			group_lock = std::unique_lock{group->mutex};

			if (const auto& queue0 = get_queue(spuq); queue != queue0)
			{
				// Keep atleast one reference of the pointer so mutex unlock can work
				const auto old_ref = std::exchange(queue, queue0);
				group_lock.unlock();
				qlock.unlock();
			}
			else
			{
				break;
			}
		}

		if (!queue->exists)
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		if (queue->events.empty())
		{
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		const auto event = queue->events.front();
		const auto data1 = static_cast<u32>(std::get<1>(event));
		const auto data2 = static_cast<u32>(std::get<2>(event));
		const auto data3 = static_cast<u32>(std::get<3>(event));
		ch_in_mbox.set_values(4, CELL_OK, data1, data2, data3);
		queue->events.pop_front();
		return true;
	}

	case SYS_SPU_THREAD_STOP_YIELD:
	{
		// SPU thread group yield (TODO)
		if (ch_out_mbox.get_count())
		{
			fmt::throw_exception("STOP code 0x100: Out_MBox is not empty");
		}

		atomic_fence_seq_cst();
		return true;
	}

	case SYS_SPU_THREAD_STOP_GROUP_EXIT:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		state += cpu_flag::wait;

		u32 value = 0;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_group_exit(): Out_MBox is empty");
		}

		spu_log.trace("sys_spu_thread_group_exit(status=0x%x)", value);

		while (true)
		{
			for (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
				_state = group->run_state)
			{
				const auto old = +state;

				if (is_stopped(old))
				{
					return false;
				}

				thread_ctrl::wait_on(state, old);;
			}

			std::lock_guard lock(group->mutex);

			if (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// We can't exit while we are waiting on an SPU event
				continue;
			}

			if (std::exchange(group->set_terminate, true))
			{
				// Whoever terminated first decides the error status + cause
				return true;
			}

			for (auto& thread : group->threads)
			{
				if (thread)
				{
					thread->state.fetch_op([](bs_t<cpu_flag>& flags)
					{
						if (flags & cpu_flag::stop)
						{
							// In case the thread raised the ret flag itself at some point do not raise it again
							return false;
						}

						flags += cpu_flag::stop + cpu_flag::ret;
						return true;
					});

					if (thread.get() != this)
						thread_ctrl::notify(*thread);
				}
			}

			group->exit_status = value;
			group->join_state = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;
			set_status_npc();
			break;
		}

		check_state();
		return true;
	}

	case SYS_SPU_THREAD_STOP_THREAD_EXIT:
	{
		/* ===== sys_spu_thread_exit ===== */

		state += cpu_flag::wait;

		u32 value;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_exit(): Out_MBox is empty");
		}

		spu_log.trace("sys_spu_thread_exit(status=0x%x)", value);
		last_exit_status.release(value);
		set_status_npc();
		state += cpu_flag::stop + cpu_flag::ret;
		check_state();
		return true;
	}
	}

	fmt::throw_exception("Unknown STOP code: 0x%x (op=0x%x, Out_MBox=%s)", code, _ref<u32>(pc), ch_out_mbox);
}

void spu_thread::halt()
{
	spu_log.trace("halt()");

	if (get_type() >= spu_type::raw)
	{
		state += cpu_flag::stop + cpu_flag::wait;

		status_npc.atomic_op([this](status_npc_sync_var& state)
		{
			state.status |= SPU_STATUS_STOPPED_BY_HALT;
			state.status &= ~SPU_STATUS_RUNNING;
			state.npc = pc | +interrupts_enabled;
		});

		status_npc.notify_one();

		int_ctrl[2].set(SPU_INT2_STAT_SPU_HALT_OR_STEP_INT);

		spu_runtime::g_escape(this);
	}

	spu_log.fatal("Halt");
	spu_runtime::g_escape(this);
}

void spu_thread::fast_call(u32 ls_addr)
{
	// LS:0x0: this is originally the entry point of the interrupt handler, but interrupts are not implemented
	_ref<u32>(0) = 0x00000002; // STOP 2

	auto old_pc = pc;
	auto old_lr = gpr[0]._u32[3];
	auto old_stack = gpr[1]._u32[3]; // only saved and restored (may be wrong)

	pc = ls_addr;
	gpr[0]._u32[3] = 0x0;

	cpu_task();

	state -= cpu_flag::ret;

	pc = old_pc;
	gpr[0]._u32[3] = old_lr;
	gpr[1]._u32[3] = old_stack;
}

bool spu_thread::capture_local_storage() const
{
	spu_exec_object spu_exec;

	// Save data as an executable segment, even the SPU stack
	// In the past, an optimization was made here to save only non-zero chunks of data
	// But Ghidra didn't like accessing memory out of chunks (pretty common)
	// So it has been reverted
	auto& prog = spu_exec.progs.emplace_back(SYS_SPU_SEGMENT_TYPE_COPY, 0x7, 0, SPU_LS_SIZE, 8, std::vector<uchar>(ls, ls + SPU_LS_SIZE));

	prog.p_paddr = prog.p_vaddr;
	spu_log.success("Segment: p_type=0x%x, p_vaddr=0x%x, p_filesz=0x%x, p_memsz=0x%x", prog.p_type, prog.p_vaddr, prog.p_filesz, prog.p_memsz);

	std::string name;

	if (get_type() == spu_type::threaded)
	{
		name = *spu_tname.load();

		if (name.empty())
		{
			// TODO: Maybe add thread group name here
			fmt::append(name, "SPU.0x%07x", lv2_id);
		}
	}
	else
	{
		fmt::append(name, "RawSPU.%u", lv2_id);
	}

	u32 pc0 = pc;

	for (; pc0; pc0 -= 4)
	{
		be_t<u32> op;
		std::memcpy(&op, prog.bin.data() + pc0 - 4, 4);

		// Try to find function entry (if they are placed sequentially search for BI $LR of previous function)
		if (!op || op == 0x35000000u || s_spu_itype.decode(op) == spu_itype::UNK)
		{
			break;
		}
	}

	spu_exec.header.e_entry = pc0;

	name = vfs::escape(name, true);
	std::replace(name.begin(), name.end(), ' ', '_');

	auto get_filename = [&]() -> std::string
	{
		return fs::get_cache_dir() + "spu_progs/" + Emu.GetTitleID() + "_" + vfs::escape(name, true) + '_' + date_time::current_time_narrow() + "_capture.elf";
	};

	auto elf_path = get_filename();

	if (fs::exists(elf_path))
	{
		// Wait 1 second so current_time_narrow() will return a different string
		std::this_thread::sleep_for(1s);

		if (elf_path = get_filename(); fs::exists(elf_path))
		{
			spu_log.error("Failed to create '%s' (error=%s)", elf_path, fs::g_tls_error);
			return false;
		}
	}

	fs::pending_file temp(elf_path);

	if (!temp.file)
	{
		spu_log.error("Failed to create temporary file for '%s' (error=%s)", elf_path, fs::g_tls_error);
		return false;
	}

	temp.file.write(spu_exec.save());

	if (!temp.commit(false))
	{
		spu_log.error("Failed to create rename temporary file to '%s' (error=%s)", elf_path, fs::g_tls_error);
		return false;
	}

	spu_log.success("SPU Local Storage image saved to '%s'", elf_path);
	return true;
}

spu_function_logger::spu_function_logger(spu_thread& spu, const char* func)
	: spu(spu)
{
	spu.current_func = func;
	spu.start_time = get_system_time();
}

spu_thread::thread_name_t::operator std::string() const
{
	std::string full_name = fmt::format("%s[0x%07x]", [](spu_type type) -> std::string_view
	{
		switch (type)
		{
		case spu_type::threaded: return "SPU"sv;
		case spu_type::raw: return "RawSPU"sv;
		case spu_type::isolated: return "Iso"sv;
		default: fmt::throw_exception("Unreachable");
		}
	}(_this->get_type()), _this->lv2_id);

	if (const std::string name = *_this->spu_tname.load(); !name.empty())
	{
		fmt::append(full_name, " %s", name);
	}

	return full_name;
}

spu_thread::priority_t::operator s32() const
{
	if (_this->get_type() != spu_type::threaded || !_this->group->has_scheduler_context)
	{
		return s32{smax};
	}

	return _this->group->prio;
}

template <>
void fmt_class_string<spu_channel>::format(std::string& out, u64 arg)
{
	const auto& ch = get_object(arg);

	u32 data = 0;

	if (ch.try_read(data))
	{
		fmt::append(out, "0x%08x", data);
	}
	else
	{
		out += "empty";
	}
}

template <>
void fmt_class_string<spu_channel_4_t>::format(std::string& out, u64 arg)
{
	const auto& ch = get_object(arg);

	// TODO (use try_read)
	fmt::append(out, "count = %d", ch.get_count());
}

DECLARE(spu_thread::g_raw_spu_ctr){};
DECLARE(spu_thread::g_raw_spu_id){};
