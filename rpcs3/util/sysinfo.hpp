#pragma once

#include "util/types.hpp"
#include <string>

namespace utils
{
	std::array<u32, 4> get_cpuid(u32 func, u32 subfunc);

	u64 get_xgetbv(u32 xcr);

	bool has_ssse3();

	bool has_sse41();

	bool has_avx();

	bool has_avx2();

	bool has_rtm();

	bool has_tsx_force_abort();

	bool has_mpx();

	bool has_avx512();

	bool has_avx512_icl();

	bool has_xop();

	bool has_clwb();

	bool has_invariant_tsc();

	bool has_fma3();

	bool has_fma4();

	std::string get_cpu_brand();

	std::string get_system_info();

	std::string get_firmware_version();

	std::string get_OS_version();

	ullong get_tsc_freq();

	u64 get_total_memory();

	u32 get_thread_count();

	u32 get_cpu_family();

	u32 get_cpu_model();

	extern const u64 main_tid;
}
