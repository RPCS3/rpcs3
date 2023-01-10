#pragma once

#include "util/types.hpp"

#ifdef _MSC_VER
extern "C" void _mm_lfence();
#elif _M_X64
#include <immintrin.h>
#endif

namespace utils
{
	inline void lfence()
	{
#ifdef _M_X64
		_mm_lfence();
#elif defined(ARCH_X64)
		__builtin_ia32_lfence();
#elif defined(ARCH_ARM64)
		// TODO
		__asm__ volatile("isb");
#else
#error "Missing lfence() implementation"
#endif
	}
}
