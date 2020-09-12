#include "stdafx.h"
#include "Utilities/JIT.h"
#include "Utilities/asm.h"
#include "Utilities/sysinfo.h"
#include "Emu/Memory/vm.h"
#include "Emu/Memory/vm_ptr.h"
#include "Emu/Memory/vm_reservation.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/lv2/sys_event_flag.h"
#include "Emu/Cell/lv2/sys_event.h"
#include "Emu/Cell/lv2/sys_interrupt.h"

#include "Emu/Cell/SPUDisAsm.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/SPUInterpreter.h"
#include "Emu/Cell/SPURecompiler.h"
#include "Emu/Cell/RawSPUThread.h"

#include <cmath>
#include <cfenv>
#include <atomic>
#include <thread>

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

static FORCE_INLINE bool cmp_rdata(const decltype(spu_thread::rdata)& lhs, const decltype(spu_thread::rdata)& rhs)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		return cmp_rdata_avx(reinterpret_cast<const __m256i*>(&lhs), reinterpret_cast<const __m256i*>(&rhs));
	}

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

static FORCE_INLINE void mov_rdata(decltype(spu_thread::rdata)& dst, const decltype(spu_thread::rdata)& src)
{
#ifndef __AVX__
	if (s_tsx_avx) [[likely]]
#endif
	{
		mov_rdata_avx(reinterpret_cast<__m256i*>(&dst), reinterpret_cast<const __m256i*>(&src));
		return;
	}

	{
		const v128 data0 = src[0];
		const v128 data1 = src[1];
		const v128 data2 = src[2];
		dst[0] = data0;
		dst[1] = data1;
		dst[2] = data2;
	}

	{
		const v128 data0 = src[3];
		const v128 data1 = src[4];
		const v128 data2 = src[5];
		dst[3] = data0;
		dst[4] = data1;
		dst[5] = data2;
	}

	{
		const v128 data0 = src[6];
		const v128 data1 = src[7];
		dst[6] = data0;
		dst[7] = data1;
	}
}

// Returns nullptr if rsx does not need pausing on reservations op, rsx ptr otherwise
static FORCE_INLINE rsx::thread* get_rsx_if_needs_res_pause(u32 addr)
{
	if (!g_cfg.core.rsx_accurate_res_access) [[likely]]
	{
		return {};
	}

	const auto render = rsx::get_current_renderer();

	ASSUME(render);

	if (render->iomap_table.io[addr >> 20].load() == umax) [[likely]]
	{
		return {};
	}

	return render;
}

extern u64 get_timebased_time();
extern u64 get_system_time();

extern thread_local u64 g_tls_fault_spu;

namespace spu
{
	namespace scheduler
	{
		std::array<std::atomic<u8>, 65536> atomic_instruction_table = {};
		constexpr u32 native_jiffy_duration_us = 1500; //About 1ms resolution with a half offset

		void acquire_pc_address(spu_thread& spu, u32 pc, u32 timeout_ms, u32 max_concurrent_instructions)
		{
			const u32 pc_offset = pc >> 2;

			if (atomic_instruction_table[pc_offset].load(std::memory_order_consume) >= max_concurrent_instructions)
			{
				spu.state += cpu_flag::wait;

				if (timeout_ms > 0)
				{
					const u64 timeout = timeout_ms * 1000u; //convert to microseconds
					const u64 start = get_system_time();
					auto remaining = timeout;

					while (atomic_instruction_table[pc_offset].load(std::memory_order_consume) >= max_concurrent_instructions)
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
					const auto count = atomic_instruction_table[pc_offset].load(std::memory_order_consume) * 100ull;
					busy_wait(count);
				}

				if (spu.test_stopped())
				{
					spu_runtime::g_escape(&spu);
				}
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

const auto spu_putllc_tx = build_function_asm<u32(*)(u32 raddr, u64 rtime, const void* _old, const void* _new)>([](asmjit::X86Assembler& c, auto& args)
{
	using namespace asmjit;

	Label fall = c.newLabel();
	Label fail = c.newLabel();
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
	c.mov(x86::rbx, imm_ptr(+vm::g_reservations));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::rbp, x86::qword_ptr(x86::rax));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(x86::rbx, args[0]));
	c.xor_(x86::r12d, x86::r12d);
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

	// Begin transaction
	build_transaction_enter(c, fall, x86::r12, 4);
	c.mov(x86::rax, x86::qword_ptr(x86::rbx));
	c.and_(x86::rax, -128);
	c.cmp(x86::rax, x86::r13);
	c.jne(fail);
	c.test(x86::qword_ptr(x86::rbx), 127);
	c.jnz(skip);

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
	c.mov(x86::eax, 1);
	c.jmp(_ret);

	c.bind(skip);
	c.xor_(x86::eax, x86::eax);
	c.xor_(x86::r12d, x86::r12d);
	build_transaction_abort(c, 0);
	//c.jmp(fall);

	c.bind(fall);
	c.sar(x86::eax, 24);
	c.js(fail);
	c.lock().bts(x86::dword_ptr(args[2], ::offset32(&spu_thread::state) - ::offset32(&spu_thread::rdata)), static_cast<u32>(cpu_flag::wait));

	// Touch memory if transaction failed without RETRY flag on the first attempt
	c.cmp(x86::r12, 1);
	c.jne(next);
	c.xor_(x86::rbp, 0xf80);
	c.lock().add(x86::dword_ptr(x86::rbp), 0);
	c.xor_(x86::rbp, 0xf80);

	Label fall2 = c.newLabel();
	Label fail2 = c.newLabel();

	// Lightened transaction: only compare and swap data
	c.bind(next);

	// Try to "lock" reservation
	c.mov(x86::rax, x86::r13);
	c.add(x86::r13, 1);
	c.lock().cmpxchg(x86::qword_ptr(x86::rbx), x86::r13);
	c.jne(fail);

	build_transaction_enter(c, fall2, x86::r12, 666);

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

	c.jnz(fail2);

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
	c.lock().add(x86::qword_ptr(x86::rbx), 127);
	c.mov(x86::eax, 1);
	c.jmp(_ret);

	c.bind(fall2);
	c.sar(x86::eax, 24);
	c.js(fail2);
	c.mov(x86::eax, 2);
	c.jmp(_ret);

	c.bind(fail);
	build_transaction_abort(c, 0xff);
	c.xor_(x86::eax, x86::eax);
	c.jmp(_ret);

	c.bind(fail2);
	build_transaction_abort(c, 0xff);
	c.lock().sub(x86::qword_ptr(x86::rbx), 1);
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

const auto spu_putlluc_tx = build_function_asm<u32(*)(u32 raddr, const void* rdata, spu_thread* _spu)>([](asmjit::X86Assembler& c, auto& args)
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
	c.mov(x86::rbx, imm_ptr(+vm::g_reservations));
	c.mov(x86::rax, imm_ptr(&vm::g_base_addr));
	c.mov(x86::rbp, x86::qword_ptr(x86::rax));
	c.lea(x86::rbp, x86::qword_ptr(x86::rbp, args[0]));
	c.and_(args[0].r32(), 0xff80);
	c.shr(args[0].r32(), 1);
	c.lea(x86::rbx, x86::qword_ptr(x86::rbx, args[0]));
	c.xor_(x86::r12d, x86::r12d);
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

	// Begin transaction
	build_transaction_enter(c, fall, x86::r12, 8);
	c.test(x86::dword_ptr(x86::rbx), 127);
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
	c.mov(x86::eax, 1);
	c.jmp(_ret);

	c.bind(skip);
	c.xor_(x86::eax, x86::eax);
	c.xor_(x86::r12d, x86::r12d);
	build_transaction_abort(c, 0);
	//c.jmp(fall);

	c.bind(fall);
	c.lock().bts(x86::dword_ptr(args[2], ::offset32(&spu_thread::state)), static_cast<u32>(cpu_flag::wait));

	// Touch memory if transaction failed without RETRY flag on the first attempt
	c.cmp(x86::r12, 1);
	c.jne(next);
	c.xor_(x86::rbp, 0xf80);
	c.lock().add(x86::dword_ptr(x86::rbp), 0);
	c.xor_(x86::rbp, 0xf80);

	Label fall2 = c.newLabel();
	Label fail2 = c.newLabel();

	// Lightened transaction
	c.bind(next);

	// Try to acquire "PUTLLUC lock"
	c.lock().bts(x86::qword_ptr(x86::rbx), std::countr_zero<u32>(vm::putlluc_lockb));
	c.jc(fail2);

	build_transaction_enter(c, fall2, x86::r12, 666);

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
	c.lock().add(x86::qword_ptr(x86::rbx), 64);
	c.mov(x86::eax, 1);
	c.jmp(_ret);

	c.bind(fail2);
	c.xor_(x86::eax, x86::eax);
	c.jmp(_ret);

	c.bind(fall2);
	c.mov(x86::eax, 2);
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

		if (const auto tag_ptr = tag.lock())
		{
			if (auto handler = tag_ptr->handler.lock())
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

std::string spu_thread::dump_all() const
{
	std::string ret = cpu_thread::dump_misc();
	ret += '\n';
	ret += dump_misc();
	ret += '\n';
	ret += dump_regs();
	ret += '\n';

	return ret;
}

std::string spu_thread::dump_regs() const
{
	std::string ret;

	for (u32 i = 0; i < 128; i++)
	{
		fmt::append(ret, "r%d: %s\n", i, gpr[i]);
	}

	fmt::append(ret, "\nEvent Stat: 0x%x\n", +ch_event_stat);
	fmt::append(ret, "Event Mask: 0x%x\n", +ch_event_mask);
	fmt::append(ret, "SRR0: 0x%05x\n", srr0);
	fmt::append(ret, "Stall Stat: %s\n", ch_stall_stat);
	fmt::append(ret, "Stall Mask: 0x%x\n", ch_stall_mask);
	fmt::append(ret, "Tag Stat: %s\n", ch_tag_stat);
	fmt::append(ret, "Tag Update: %s\n", mfc_tag_update{ch_tag_upd});

	if (const u32 addr = raddr)
		fmt::append(ret, "Reservation Addr: 0x%x\n", addr);
	else
		fmt::append(ret, "Reservation Addr: none\n");

	fmt::append(ret, "Atomic Stat: %s\n", ch_atomic_stat); // TODO: use mfc_atomic_status formatting
	fmt::append(ret, "Interrupts Enabled: %s\n", interrupts_enabled.load());
	fmt::append(ret, "Inbound Mailbox: %s\n", ch_in_mbox);
	fmt::append(ret, "Out Mailbox: %s\n", ch_out_mbox);
	fmt::append(ret, "Out Interrupts Mailbox: %s\n", ch_out_intr_mbox);
	fmt::append(ret, "SNR config: 0x%llx\n", snr_config);
	fmt::append(ret, "SNR1: %s\n", ch_snr1);
	fmt::append(ret, "SNR2: %s", ch_snr2);

	return ret;
}

std::string spu_thread::dump_callstack() const
{
	return {};
}

std::vector<std::pair<u32, u32>> spu_thread::dump_callstack_list() const
{
	return {};
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

	ch_event_mask.raw() = 0;
	ch_event_stat.raw() = 0;
	interrupts_enabled.raw() = false;
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

		if (verify(HERE, group->running--) == 1)
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
							_mm_pause();
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

		static thread_local stx::shared_cptr<std::string> name_cache;

		if (!cpu->spu_tname.is_equal(name_cache)) [[unlikely]]
		{
			name_cache = cpu->spu_tname.load();
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
		ASSERT(spu_runtime::g_interpreter);

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

void spu_thread::cpu_mem()
{
	//vm::passive_lock(*this);
}

void spu_thread::cpu_unmem()
{
	//state.test_and_set(cpu_flag::memory);
}

spu_thread::~spu_thread()
{
	{
		const auto [_, shm] = vm::get(vm::any, offset)->get(offset);

		for (s32 i = -1; i < 2; i++)
		{
			// Unmap LS mirrors
			shm->unmap_critical(ls + (i * SPU_LS_SIZE));
		}

		// Deallocate Local Storage
		vm::dealloc_verbose_nothrow(offset);
	}

	// Release LS mirrors area
	utils::memory_release(ls - (SPU_LS_SIZE * 2), SPU_LS_SIZE * 5);

	// Deallocate RawSPU ID
	if (get_type() >= spu_type::raw)
	{
		g_raw_spu_id[index] = 0;
		g_raw_spu_ctr--;
	}
}

spu_thread::spu_thread(vm::addr_t _ls, lv2_spu_group* group, u32 index, std::string_view name, u32 lv2_id, bool is_isolated, u32 option)
	: cpu_thread(idm::last_id())
	, index(index)
	, ls([&]()
	{
		const auto [_, shm] = vm::get(vm::any, _ls)->get(_ls);
		const auto addr = static_cast<u8*>(utils::memory_reserve(SPU_LS_SIZE * 5));

		for (u32 i = 1; i < 4; i++)
		{
			// Map LS mirrors
			const auto ptr = addr + (i * SPU_LS_SIZE);
			verify(HERE), shm->map_critical(ptr) == ptr;
		}

		// Use the middle mirror
		return addr + (SPU_LS_SIZE * 2);
	}())
	, thread_type(group ? spu_type::threaded : is_isolated ? spu_type::isolated : spu_type::raw)
	, offset(_ls)
	, group(group)
	, option(option)
	, lv2_id(lv2_id)
	, spu_tname(stx::shared_cptr<std::string>::make(name))
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
}

void spu_thread::push_snr(u32 number, u32 value)
{
	// Get channel
	const auto channel = number & 1 ? &ch_snr2 : &ch_snr1;

	// Check corresponding SNR register settings
	if ((snr_config >> number) & 1)
	{
		channel->push_or(value);
	}
	else
	{
		channel->push(value);
	}
}

void spu_thread::do_dma_transfer(const spu_mfc_cmd& args)
{
	const bool is_get = (args.cmd & ~(MFC_BARRIER_MASK | MFC_FENCE_MASK | MFC_START_MASK)) == MFC_GET_CMD;

	u32 eal = args.eal;
	u32 lsa = args.lsa & 0x3ffff;

	// SPU Thread Group MMIO (LS and SNR) and RawSPU MMIO
	if (eal >= RAW_SPU_BASE_ADDR)
	{
		const u32 index = (eal - SYS_SPU_THREAD_BASE_LOW) / SYS_SPU_THREAD_OFFSET; // thread number in group
		const u32 offset = (eal - SYS_SPU_THREAD_BASE_LOW) % SYS_SPU_THREAD_OFFSET; // LS offset or MMIO register

		if (eal < SYS_SPU_THREAD_BASE_LOW)
		{
			// RawSPU MMIO
			auto thread = idm::get<named_thread<spu_thread>>(find_raw_spu((eal - RAW_SPU_BASE_ADDR) / RAW_SPU_OFFSET));

			if (!thread)
			{
				fmt::throw_exception("RawSPU not found (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}

			u32 value;
			if ((eal - RAW_SPU_BASE_ADDR) % RAW_SPU_OFFSET + args.size - 1 < SPU_LS_SIZE) // LS access
			{
			}
			else if (args.size == 4 && is_get && thread->read_reg(eal, value))
			{
				_ref<u32>(lsa) = value;
				return;
			}
			else if (args.size == 4 && !is_get && thread->write_reg(eal, args.cmd != MFC_SDCRZ_CMD ? +_ref<u32>(lsa) : 0))
			{
				return;
			}
			else
			{
				fmt::throw_exception("Invalid RawSPU MMIO offset (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}
		}
		else if (get_type() >= spu_type::raw)
		{
			fmt::throw_exception("SPU MMIO used for RawSPU (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
		}
		else if (group && group->threads_map[index] != -1)
		{
			auto& spu = static_cast<spu_thread&>(*group->threads[group->threads_map[index]]);

			if (offset + args.size - 1 < SPU_LS_SIZE) // LS access
			{
				eal = spu.offset + offset; // redirect access
			}
			else if (!is_get && args.size == 4 && (offset == SYS_SPU_THREAD_SNR1 || offset == SYS_SPU_THREAD_SNR2))
			{
				spu.push_snr(SYS_SPU_THREAD_SNR2 == offset, args.cmd != MFC_SDCRZ_CMD ? +_ref<u32>(lsa) : 0);
				return;
			}
			else
			{
				fmt::throw_exception("Invalid MMIO offset (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
			}
		}
		else
		{
			fmt::throw_exception("Invalid thread type (cmd=0x%x, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE, args.cmd, args.lsa, args.eal, args.tag, args.size);
		}
	}

	// Keep src point to const
	auto [dst, src] = [&]() -> std::pair<u8*, const u8*>
	{
		u8* dst = vm::_ptr<u8>(eal);
		u8* src = _ptr<u8>(lsa);

		if (is_get)
		{
			std::swap(src, dst);
		}

		return {dst, src};
	}();

	// It is so rare that optimizations are not implemented (TODO)
	alignas(64) static constexpr u8 zero_buf[0x10000]{};

	if (args.cmd == MFC_SDCRZ_CMD)
	{
		src = zero_buf;
	}

	if ((!g_use_rtm && !is_get) || g_cfg.core.spu_accurate_dma)  [[unlikely]]
	{
		for (u32 size = args.size, size0; is_get;
			size -= size0, dst += size0, src += size0, eal += size0)
		{
			size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

			for (u64 i = 0;; [&]()
			{
				if (state)
				{
					check_state();
				}
				else if (++i < 25) [[likely]]
				{
					busy_wait(300);
				}
				else
				{
					std::this_thread::yield();
				}
			}())
			{
				const u64 time0 = vm::reservation_acquire(eal, size0);

				if (time0 & 127)
				{
					continue;
				}

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
					mov_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src));
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

				if (time0 != vm::reservation_acquire(eal, size0) || (size0 == 128 && !cmp_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src))))
				{
					continue;
				}

				break;
			}

			if (size == size0)
			{
				return;
			}
		}

		switch (u32 size = args.size)
		{
		case 1:
		{
			auto [res, time0] = vm::reservation_lock(eal, 1, vm::dma_lockb);
			*reinterpret_cast<u8*>(dst) = *reinterpret_cast<const u8*>(src);
			res.release(time0 + 128);
			break;
		}
		case 2:
		{
			auto [res, time0] = vm::reservation_lock(eal, 2, vm::dma_lockb);
			*reinterpret_cast<u16*>(dst) = *reinterpret_cast<const u16*>(src);
			res.release(time0 + 128);
			break;
		}
		case 4:
		{
			auto [res, time0] = vm::reservation_lock(eal, 4, vm::dma_lockb);
			*reinterpret_cast<u32*>(dst) = *reinterpret_cast<const u32*>(src);
			res.release(time0 + 128);
			break;
		}
		case 8:
		{
			auto [res, time0] = vm::reservation_lock(eal, 8, vm::dma_lockb);
			*reinterpret_cast<u64*>(dst) = *reinterpret_cast<const u64*>(src);
			res.release(time0 + 128);
			break;
		}
		default:
		{
			if (g_cfg.core.spu_accurate_dma)
			{
				for (u32 size0;; size -= size0, dst += size0, src += size0, eal += size0)
				{
					size0 = std::min<u32>(128 - (eal & 127), std::min<u32>(size, 128));

					// Lock each cache line execlusively
					auto [res, time0] = vm::reservation_lock(eal, size0, vm::dma_lockb);

					switch (size0)
					{
					case 128:
					{
						mov_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src));
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

					res.release(time0 + 128);

					if (size == size0)
					{
						break;
					}
				}

				break;
			}

			if (((eal & 127) + size) <= 128)
			{
				// Lock one cache line
				auto [res, time0] = vm::reservation_lock(eal, 128);

				while (size)
				{
					*reinterpret_cast<v128*>(dst) = *reinterpret_cast<const v128*>(src);

					dst += 16;
					src += 16;
					size -= 16;
				}

				res.release(time0);
				break;
			}

			u32 range_addr = eal & -128;
			u32 range_end = ::align(eal + size, 128);

			// Handle the case of crossing 64K page borders
			if (range_addr >> 16 != (range_end - 1) >> 16)
			{
				u32 nexta = range_end & -65536;
				u32 size0 = nexta - eal;
				size -= size0;

				// Split locking + transfer in two parts (before 64K border, and after it)
				const auto lock = vm::range_lock(range_addr, nexta);

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
					mov_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src));

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

				lock->release(0);
				range_addr = nexta;
			}

			const auto lock = vm::range_lock(range_addr, range_end);

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
				mov_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src));

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

			lock->release(0);
			break;
		}
		}

		if (g_cfg.core.spu_accurate_dma)
		{
			std::atomic_thread_fence(std::memory_order_seq_cst);
		}

		return;
	}

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
			mov_rdata(*reinterpret_cast<decltype(spu_thread::rdata)*>(dst), *reinterpret_cast<const decltype(spu_thread::rdata)*>(src));

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

		spu_log.trace("LIST: addr=0x%x, size=0x%x, lsa=0x%05x, sb=0x%x", addr, size, args.lsa | (addr & 0xf), items[index].sb);

		if (size)
		{
			transfer.eal  = addr;
			transfer.lsa  = args.lsa | (addr & 0xf);
			transfer.size = size;

			do_dma_transfer(transfer);
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
				ch_event_stat |= SPU_EVENT_SN;
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
	// Store conditionally
	const u32 addr = args.eal & -128;

	if ([&]()
	{
		if (raddr != addr)
		{
			return false;
		}

		const auto& to_write = _ref<decltype(rdata)>(args.lsa & 0x3ff80);
		auto& res = vm::reservation_acquire(addr, 128);

		if (!g_use_rtm && rtime != res)
		{
			return false;
		}

		if (!g_use_rtm && cmp_rdata(to_write, rdata))
		{
			// Writeback of unchanged data. Only check memory change
			return cmp_rdata(rdata, vm::_ref<decltype(rdata)>(addr)) && res.compare_and_swap_test(rtime, rtime + 128);
		}

		if (g_use_rtm) [[likely]]
		{
			switch (spu_putllc_tx(addr, rtime, rdata.data(), to_write.data()))
			{
			case 2:
			{
				const auto render = get_rsx_if_needs_res_pause(addr);

				if (render) render->pause();

				cpu_thread::suspend_all cpu_lock(this);

				// Give up if PUTLLUC happened
				if (res == (rtime | 1))
				{
					auto& data = vm::_ref<decltype(rdata)>(addr);

					if (cmp_rdata(rdata, data))
					{
						mov_rdata(data, to_write);
						res += 127;
						if (render) render->unpause();
						return true;
					}
				}

				res -= 1;
				if (render) render->unpause();
				return false;
			}
			case 1: return true;
			case 0: return false;
			default: ASSUME(0);
			}
		}

		if (!vm::reservation_trylock(res, rtime))
		{
			return false;
		}

		vm::_ref<atomic_t<u32>>(addr) += 0;

		const auto render = get_rsx_if_needs_res_pause(addr);

		if (render) render->pause();

		auto& super_data = *vm::get_super_ptr<decltype(rdata)>(addr);
		const bool success = [&]()
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr);

			if (cmp_rdata(rdata, super_data))
			{
				mov_rdata(super_data, to_write);
				res.release(rtime + 128);
				return true;
			}

			res.release(rtime);
			return false;
		}();

		if (render) render->unpause();
		return success;
	}())
	{
		vm::reservation_notifier(addr, 128).notify_all();
		raddr = 0;
		return true;
	}
	else
	{
		if (raddr)
		{
			// Last check for event before we clear the reservation
			if (raddr == addr || rtime != (vm::reservation_acquire(raddr, 128) & (-128 | vm::dma_lockb)) || !cmp_rdata(rdata, vm::_ref<decltype(rdata)>(raddr)))
			{
				ch_event_stat |= SPU_EVENT_LR;
			}
		}

		raddr = 0;
		return false;
	}
}

void spu_thread::do_putlluc(const spu_mfc_cmd& args)
{
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

	const auto& to_write = _ref<decltype(rdata)>(args.lsa & 0x3ff80);

	// Store unconditionally
	if (g_use_rtm) [[likely]]
	{
		const u32 result = spu_putlluc_tx(addr, to_write.data(), this);

		const auto render = result != 1 ? get_rsx_if_needs_res_pause(addr) : nullptr;

		if (render) render->pause();

		if (result == 2)
		{
			cpu_thread::suspend_all cpu_lock(this);

			if (vm::reservation_acquire(addr, 128) & 64)
			{
				// Wait for PUTLLC to complete
				while (vm::reservation_acquire(addr, 128) & 63)
				{
					busy_wait(100);
				}

				mov_rdata(vm::_ref<decltype(rdata)>(addr), to_write);
				vm::reservation_acquire(addr, 128) += 64;
			}
		}
		else if (result == 0)
		{
			cpu_thread::suspend_all cpu_lock(this);

			while (vm::reservation_acquire(addr, 128).bts(std::countr_zero<u32>(vm::putlluc_lockb)))
			{
				busy_wait(100);
			}

			while (vm::reservation_acquire(addr, 128) & 63)
			{
				busy_wait(100);
			}

			mov_rdata(vm::_ref<decltype(rdata)>(addr), to_write);
			vm::reservation_acquire(addr, 128) += 64;
		}

		if (render) render->unpause();
		static_cast<void>(test_stopped());
	}
	else
	{
		auto& data = vm::_ref<decltype(rdata)>(addr);
		auto [res, time0] = vm::reservation_lock(addr, 128);

		*reinterpret_cast<atomic_t<u32>*>(&data) += 0;

		const auto render = get_rsx_if_needs_res_pause(addr);

		if (render) render->pause();

		auto& super_data = *vm::get_super_ptr<decltype(rdata)>(addr);
		{
			// Full lock (heavyweight)
			// TODO: vm::check_addr
			vm::writer_lock lock(addr);
			mov_rdata(super_data, to_write);
			res.release(time0 + 128);
		}

		if (render) render->unpause();
	}

	vm::reservation_notifier(addr, 128).notify_all();
}

void spu_thread::do_mfc(bool wait)
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
				std::atomic_thread_fence(std::memory_order_seq_cst);
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
			do_dma_transfer(args);
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
	if (interrupts_enabled && (ch_event_mask & ch_event_stat & SPU_EVENT_INTR_IMPLEMENTED) > 0)
	{
		interrupts_enabled.release(false);
		srr0 = next_pc;

		// Test for BR/BRA instructions (they are equivalent at zero pc)
		const u32 br = _ref<u32>(0);
		pc = (br & 0xfd80007f) == 0x30000000 ? (br >> 5) & 0x3fffc : 0;
		return true;
	}

	return false;
}

u32 spu_thread::get_mfc_completed()
{
	return ch_tag_mask & ~mfc_fence;
}

bool spu_thread::process_mfc_cmd()
{
	// Stall infinitely if MFC queue is full
	while (mfc_size >= 16) [[unlikely]]
	{
		state += cpu_flag::wait;

		if (is_stopped())
		{
			return false;
		}

		thread_ctrl::wait();
	}

	spu::scheduler::concurrent_execution_watchdog watchdog(*this);
	spu_log.trace("DMAC: cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x", ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);

	switch (ch_mfc_cmd.cmd)
	{
	case MFC_SDCRT_CMD:
	case MFC_SDCRTST_CMD:
		return true;
	case MFC_GETLLAR_CMD:
	{
		const u32 addr = ch_mfc_cmd.eal & -128;
		const auto& data = vm::_ref<decltype(rdata)>(addr);

		if (addr == raddr && !g_use_rtm && g_cfg.core.spu_getllar_polling_detection && rtime == vm::reservation_acquire(addr, 128) && cmp_rdata(rdata, data))
		{
			// Spinning, might as well yield cpu resources
			std::this_thread::yield();
		}

		auto& dst = _ref<decltype(rdata)>(ch_mfc_cmd.lsa & 0x3ff80);
		u64 ntime;

		for (u64 i = 0;; [&]()
		{
			if (state & cpu_flag::pause)
			{
				check_state();
			}

			if (++i < 25) [[likely]]
			{
				busy_wait(300);
			}
			else
			{
				if (g_use_rtm)
				{
					state += cpu_flag::wait;
				}

				std::this_thread::yield();

				if (test_stopped())
				{
				}
			}
		}())
		{
			ntime = vm::reservation_acquire(addr, 128);

			if (ntime & 127)
			{
				// There's an on-going reservation store, wait
				continue;
			}

			mov_rdata(dst, data);

			if (u64 time0 = vm::reservation_acquire(addr, 128);
				ntime != time0)
			{
				// Reservation data has been modified recently
				if (time0 & 127) i += 12;
				continue;
			}

			if (g_cfg.core.spu_accurate_getllar && !cmp_rdata(dst, data))
			{
				i += 2;
				continue;
			}

			break;
		}

		if (raddr && raddr != addr)
		{
			// Last check for event before we replace the reservation with a new one
			if ((vm::reservation_acquire(raddr, 128) & (-128 | vm::dma_lockb)) != rtime || !cmp_rdata(rdata, vm::_ref<decltype(rdata)>(raddr)))
			{
				ch_event_stat |= SPU_EVENT_LR;
			}
		}
		else if (raddr == addr)
		{
			// Lost previous reservation on polling
			if (ntime != rtime || !cmp_rdata(rdata, dst))
			{
				ch_event_stat |= SPU_EVENT_LR;
			}
		}

		raddr = addr;
		rtime = ntime;
		mov_rdata(rdata, dst);

		ch_atomic_stat.set_value(MFC_GETLLAR_SUCCESS);
		return true;
	}

	case MFC_PUTLLC_CMD:
	{
		ch_atomic_stat.set_value(do_putllc(ch_mfc_cmd) ? MFC_PUTLLC_SUCCESS : MFC_PUTLLC_FAILURE);
		return true;
	}
	case MFC_PUTLLUC_CMD:
	{
		do_putlluc(ch_mfc_cmd);
		ch_atomic_stat.set_value(MFC_PUTLLUC_SUCCESS);
		return true;
	}
	case MFC_PUTQLLUC_CMD:
	{
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
					do_dma_transfer(ch_mfc_cmd);
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
			std::atomic_thread_fence(std::memory_order_seq_cst);
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

	fmt::throw_exception("Unknown command (cmd=%s, lsa=0x%x, ea=0x%llx, tag=0x%x, size=0x%x)" HERE,
		ch_mfc_cmd.cmd, ch_mfc_cmd.lsa, ch_mfc_cmd.eal, ch_mfc_cmd.tag, ch_mfc_cmd.size);
}

u32 spu_thread::get_events(u32 mask_hint, bool waiting)
{
	const u32 mask1 = ch_event_mask;

	if (mask1 & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)" HERE, mask1);
	}

	// Check reservation status and set SPU_EVENT_LR if lost
	if (mask_hint & SPU_EVENT_LR && raddr && ((vm::reservation_acquire(raddr, sizeof(rdata)) & -128) != rtime || !cmp_rdata(rdata, vm::_ref<decltype(rdata)>(raddr))))
	{
		ch_event_stat |= SPU_EVENT_LR;
		raddr = 0;
	}

	// SPU Decrementer Event on underflow (use the upper 32-bits to determine it)
	if (mask_hint & SPU_EVENT_TM)
	{
		if (const u64 res = (ch_dec_value - (get_timebased_time() - ch_dec_start_timestamp)) >> 32)
		{
			// Set next event to the next time the decrementer underflows
			ch_dec_start_timestamp -= res << 32;

			if ((ch_event_stat & SPU_EVENT_TM) == 0)
			{
				ch_event_stat |= SPU_EVENT_TM;
			}
		}
	}

	// Simple polling or polling with atomically set/removed SPU_EVENT_WAITING flag
	return !waiting ? ch_event_stat & mask1 : ch_event_stat.atomic_op([&](u32& stat) -> u32
	{
		if (u32 res = stat & mask1)
		{
			stat &= ~SPU_EVENT_WAITING;
			return res;
		}

		stat |= SPU_EVENT_WAITING;
		return 0;
	});
}

void spu_thread::set_events(u32 mask)
{
	if (mask & ~SPU_EVENT_IMPLEMENTED)
	{
		fmt::throw_exception("SPU Events not implemented (mask=0x%x)" HERE, mask);
	}

	// Set new events, get old event mask
	const u32 old_stat = ch_event_stat.fetch_or(mask);

	// Notify if some events were set
	if (~old_stat & mask && old_stat & SPU_EVENT_WAITING && ch_event_stat & SPU_EVENT_WAITING)
	{
		notify();
	}
}

void spu_thread::set_interrupt_status(bool enable)
{
	if (enable)
	{
		// Detect enabling interrupts with events masked
		if (ch_event_mask & ~SPU_EVENT_INTR_IMPLEMENTED)
		{
			fmt::throw_exception("SPU Interrupts not implemented (mask=0x%x)" HERE, +ch_event_mask);
		}

		interrupts_enabled = true;
	}
	else
	{
		interrupts_enabled = false;
	}
}

u32 spu_thread::get_ch_count(u32 ch)
{
	spu_log.trace("get_ch_count(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

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
	case SPU_RdEventStat:     return get_events() != 0;
	case MFC_Cmd:             return 16 - mfc_size;
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

s64 spu_thread::get_ch_value(u32 ch)
{
	spu_log.trace("get_ch_value(ch=%d [%s])", ch, ch < 128 ? spu_ch_name[ch] : "???");

	auto read_channel = [&](spu_channel& channel) -> s64
	{
		if (channel.get_count() == 0)
		{
			state += cpu_flag::wait;
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

			if (is_stopped())
			{
				return -1;
			}

			thread_ctrl::wait();
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
		return ch_event_mask;
	}

	case SPU_RdEventStat:
	{
		const u32 mask1 = ch_event_mask;

		u32 res = get_events(mask1);

		if (res)
		{
			return res;
		}

		spu_function_logger logger(*this, "MFC Events read");

		if (mask1 & SPU_EVENT_LR && raddr)
		{
			if (mask1 != SPU_EVENT_LR && mask1 != SPU_EVENT_LR + SPU_EVENT_TM)
			{
				// Combining LR with other flags needs another solution
				fmt::throw_exception("Not supported: event mask 0x%x" HERE, mask1);
			}

			while (res = get_events(mask1), !res)
			{
				state += cpu_flag::wait;

				if (is_stopped())
				{
					return -1;
				}

				vm::reservation_notifier(raddr, 128).wait<UINT64_MAX & -128>(rtime, atomic_wait_timeout{100'000});
			}

			check_state();
			return res;
		}

		while (res = get_events(mask1, true), !res)
		{
			state += cpu_flag::wait;

			if (is_stopped())
			{
				return -1;
			}

			thread_ctrl::wait_for(100);
		}

		check_state();
		return res;
	}

	case SPU_RdMachStat:
	{
		// Return SPU Interrupt status in LSB
		return u32{interrupts_enabled} | (u32{get_type() == spu_type::isolated} << 1);
	}
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s])" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???");
}

bool spu_thread::set_ch_value(u32 ch, u32 value)
{
	spu_log.trace("set_ch_value(ch=%d [%s], value=0x%x)", ch, ch < 128 ? spu_ch_name[ch] : "???", value);

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
					fmt::throw_exception("sys_spu_thread_send_event(value=0x%x, spup=%d): Out_MBox is empty" HERE, value, spup);
				}

				spu_log.trace("sys_spu_thread_send_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				std::lock_guard lock(group->mutex);

				const auto queue = this->spup[spup].lock();

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
					fmt::throw_exception("sys_spu_thread_throw_event(value=0x%x, spup=%d): Out_MBox is empty" HERE, value, spup);
				}

				spu_log.trace("sys_spu_thread_throw_event(spup=%d, data0=0x%x, data1=0x%x)", spup, value & 0x00ffffff, data);

				const auto queue = (std::lock_guard{group->mutex}, this->spup[spup].lock());

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
					fmt::throw_exception("sys_event_flag_set_bit(value=0x%x (flag=%d)): Out_MBox is empty" HERE, value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit(id=%d, value=0x%x (flag=%d))", data, value, flag);

				std::lock_guard lock(group->mutex);

				// Use the syscall to set flag
				const auto res = ch_in_mbox.get_count() ? CELL_EBUSY : 0u + sys_event_flag_set(data, 1ull << flag);

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
					fmt::throw_exception("sys_event_flag_set_bit_impatient(value=0x%x (flag=%d)): Out_MBox is empty" HERE, value, flag);
				}

				spu_log.trace("sys_event_flag_set_bit_impatient(id=%d, value=0x%x (flag=%d))", data, value, flag);

				// Use the syscall to set flag
				sys_event_flag_set(data, 1ull << flag);
				return true;
			}
			else
			{
				if (ch_out_mbox.get_count())
				{
					fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x); Out_MBox = 0x%x" HERE, value, ch_out_mbox.get_value());
				}
				else
				{
					fmt::throw_exception("SPU_WrOutIntrMbox: unknown data (value=0x%x)" HERE, value);
				}
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
		ch_event_mask = value;
		return true;
	}

	case SPU_WrEventAck:
	{
		// "Collect" events before final acknowledgment
		get_events(value);
		ch_event_stat &= ~value;
		return true;
	}

	case 69:
	{
		return true;
	}
	}

	fmt::throw_exception("Unknown/illegal channel (ch=%d [%s], value=0x%x)" HERE, ch, ch < 128 ? spu_ch_name[ch] : "???", value);
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

	switch (code)
	{
	case 0x001:
	{
		state += cpu_flag::wait;
		thread_ctrl::wait_for(1000); // hack
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
			fmt::throw_exception("sys_spu_thread_receive_event(): Out_MBox is empty" HERE);
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

		std::shared_ptr<lv2_event_queue> queue;

		state += cpu_flag::wait;

		spu_function_logger logger(*this, "sys_spu_thread_receive_event");

		while (true)
		{
			queue.reset();

			// Check group status, wait if necessary
			for (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
				_state = group->run_state)
			{
				if (is_stopped())
				{
					return false;
				}

				thread_ctrl::wait();
			}

			reader_lock rlock(id_manager::g_mutex);

			std::lock_guard lock(group->mutex);

			if (is_stopped())
			{
				return false;
			}

			if (group->run_state >= SPU_THREAD_GROUP_STATUS_WAITING && group->run_state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
			{
				// Try again
				continue;
			}

			for (auto& v : this->spuq)
			{
				if (spuq == v.first)
				{
					queue = v.second.lock();

					if (lv2_event_queue::check(queue))
					{
						break;
					}
				}
			}

			if (!lv2_event_queue::check(queue))
			{
				return ch_in_mbox.set_values(1, CELL_EINVAL), true;
			}

			std::lock_guard qlock(queue->mutex);

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

		while (true)
		{
			if (is_stopped())
			{
				// The thread group cannot be stopped while waiting for an event
				verify(HERE), !(state & cpu_flag::stop);
				return false;
			}

			if (!state.test_and_reset(cpu_flag::signal))
			{
				thread_ctrl::wait();
			}
			else
			{
				break;
			}
		}

		std::lock_guard lock(group->mutex);

		if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING)
		{
			group->run_state = SPU_THREAD_GROUP_STATUS_RUNNING;
		}
		else if (group->run_state == SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED)
		{
			group->run_state = SPU_THREAD_GROUP_STATUS_SUSPENDED;
		}

		for (auto& thread : group->threads)
		{
			if (thread)
			{
				thread->state -= cpu_flag::suspend;

				if (thread.get() != this)
				{
					thread_ctrl::raw_notify(*thread);
				}
			}
		}

		return true;
	}

	case SYS_SPU_THREAD_STOP_TRY_RECEIVE_EVENT:
	{
		/* ===== sys_spu_thread_tryreceive_event ===== */

		u32 spuq = 0;

		if (!ch_out_mbox.try_pop(spuq))
		{
			fmt::throw_exception("sys_spu_thread_tryreceive_event(): Out_MBox is empty" HERE);
		}

		if (u32 count = ch_in_mbox.get_count())
		{
			spu_log.error("sys_spu_thread_tryreceive_event(): In_MBox is not empty (%d)", count);
			return ch_in_mbox.set_values(1, CELL_EBUSY), true;
		}

		spu_log.trace("sys_spu_thread_tryreceive_event(spuq=0x%x)", spuq);

		std::lock_guard lock(group->mutex);

		std::shared_ptr<lv2_event_queue> queue;

		for (auto& v : this->spuq)
		{
			if (spuq == v.first)
			{
				if (queue = v.second.lock(); lv2_event_queue::check(queue))
				{
					break;
				}
			}
		}

		if (!lv2_event_queue::check(queue))
		{
			return ch_in_mbox.set_values(1, CELL_EINVAL), true;
		}

		std::lock_guard qlock(queue->mutex);

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
			fmt::throw_exception("STOP code 0x100: Out_MBox is not empty" HERE);
		}

		std::atomic_thread_fence(std::memory_order_seq_cst);
		return true;
	}

	case SYS_SPU_THREAD_STOP_GROUP_EXIT:
	{
		/* ===== sys_spu_thread_group_exit ===== */

		state += cpu_flag::wait;

		u32 value = 0;

		if (!ch_out_mbox.try_pop(value))
		{
			fmt::throw_exception("sys_spu_thread_group_exit(): Out_MBox is empty" HERE);
		}

		spu_log.trace("sys_spu_thread_group_exit(status=0x%x)", value);

		while (true)
		{
			for (auto _state = +group->run_state;
				_state >= SPU_THREAD_GROUP_STATUS_WAITING && _state <= SPU_THREAD_GROUP_STATUS_WAITING_AND_SUSPENDED;
				_state = group->run_state)
			{
				if (is_stopped())
				{
					return false;
				}

				thread_ctrl::wait();
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
				if (thread && thread.get() != this)
				{
					thread->state += cpu_flag::stop + cpu_flag::ret;
					thread_ctrl::raw_notify(*thread);
				}
			}

			group->exit_status = value;
			group->join_state = SYS_SPU_THREAD_GROUP_JOIN_GROUP_EXIT;
			set_status_npc();
			break;
		}

		state += cpu_flag::stop + cpu_flag::ret;
		check_state();
		return true;
	}

	case SYS_SPU_THREAD_STOP_THREAD_EXIT:
	{
		/* ===== sys_spu_thread_exit ===== */

		state += cpu_flag::wait;

		if (!ch_out_mbox.get_count())
		{
			fmt::throw_exception("sys_spu_thread_exit(): Out_MBox is empty" HERE);
		}

		const u32 value = ch_out_mbox.get_value();
		spu_log.trace("sys_spu_thread_exit(status=0x%x)", value);
		last_exit_status.release(value);
		set_status_npc();
		state += cpu_flag::stop + cpu_flag::ret;
		check_state();
		return true;
	}
	}

	fmt::throw_exception("Unknown STOP code: 0x%x (Out_MBox=%s)" HERE, code, ch_out_mbox);
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

	fmt::throw_exception("Halt" HERE);
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

spu_function_logger::spu_function_logger(spu_thread& spu, const char* func)
	: spu(spu)
{
	spu.current_func = func;
	spu.start_time = get_system_time();
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
