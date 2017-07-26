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

	inline bool has_ssse3()
	{
		return get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x200;
	}

	inline bool has_avx()
	{
		return get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x10000000;
	}

	inline bool has_rtm()
	{
		// Check RTM and MPX extensions in order to filter out TSX on Haswell CPUs
		return get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x4800) == 0x4800;
	}

	inline bool transaction_enter()
	{
		while (true)
		{
			const auto status = _xbegin();

			if (status == _XBEGIN_STARTED)
			{
				return true;
			}

			if (!(status & _XABORT_RETRY))
			{
				return false;
			}
		}
	}

	std::string get_system_info();
}
