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

	bool has_ssse3();

	bool has_avx();

	bool has_avx2();

	bool has_rtm();

	bool has_512();

	bool has_xop();

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
