﻿#pragma once

#include "util/types.hpp"
#include <string>

namespace utils
{
	bool has_ssse3();

	bool has_sse41();

	bool has_avx();

	bool has_avx2();

	bool has_rtm();

	bool has_tsx_force_abort();

	bool has_rtm_always_abort();

	bool has_mpx();

	bool has_avx512();

	bool has_avx512_icl();

	bool has_avx512_vnni();

	bool has_avx10();

	bool has_avx10_512();

	u32 avx10_isa_version();

	bool has_avx512_256();

	bool has_avx512_icl_256();

	bool has_xop();

	bool has_clwb();

	bool has_invariant_tsc();

	bool has_fma3();

	bool has_fma4();

	bool has_fast_vperm2b();

	bool has_erms();

	bool has_fsrm();

	bool has_waitx();

	bool has_waitpkg();

	bool has_appropriate_um_wait();

	bool has_um_wait();

	std::string get_cpu_brand();

	std::string get_system_info();

	std::string get_firmware_version();

	std::string get_OS_version();

	int get_maxfiles();

	bool get_low_power_mode();

	ullong get_tsc_freq();

	u64 get_total_memory();

	u32 get_thread_count();

	u32 get_cpu_family();

	u32 get_cpu_model();

	// A threshold of 0xFFFFFFFF means that the rep movsb is expected to be slow on this platform
	u32 get_rep_movsb_threshold();

	u64 _get_main_tid();

	inline const u64 main_tid = _get_main_tid();
}
