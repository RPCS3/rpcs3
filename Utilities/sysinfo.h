#pragma once

#include "types.h"
#include <string>

namespace utils
{
	inline std::array<u32, 4> get_cpuid(u32 func, u32 subfunc)
	{
		int regs[4];
#ifdef _MSC_VER
		__cpuidex(regs, func, subfunc);
#else
		__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func), "c" (subfunc));
#endif
		return {0u+regs[0], 0u+regs[1], 0u+regs[2], 0u+regs[3]};
	}

	inline u64 get_xgetbv(u32 xcr)
	{
#ifdef _MSC_VER
		return _xgetbv(xcr);
#else
		u32 eax, edx;
		__asm__ volatile( "xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
		return eax | (u64(edx) << 32);
#endif
	}

	bool has_ssse3();

	bool has_sse41();

	bool has_avx();

	bool has_avx2();

	bool has_rtm();

	bool has_tsx_force_abort();

	bool has_mpx();

	bool has_avx512();

	bool has_xop();

	bool has_clwb();

	std::string get_cpu_brand();

	std::string get_system_info();

	std::string get_firmware_version();

	std::string get_OS_version();

	ullong get_tsc_freq();

	u64 get_total_memory();

	u32 get_thread_count();
}
