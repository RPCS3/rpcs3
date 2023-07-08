#pragma once

#include "util/types.hpp"

#ifdef _M_X64
#ifdef _MSC_VER
extern "C" u64 __rdtsc();
#else
#include <immintrin.h>
#endif
#endif

namespace utils
{
	inline u64 get_tsc()
	{
#if defined(ARCH_ARM64)
		u64 r = 0;
		__asm__ volatile("mrs %0, cntvct_el0" : "=r" (r));
		return r;
#elif defined(_M_X64)
		return __rdtsc();
#elif defined(ARCH_X64)
		return __builtin_ia32_rdtsc();
#else
#error "Missing utils::get_tsc() implementation"
#endif
	}
}
