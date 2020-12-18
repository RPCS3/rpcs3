#include "util/sysinfo.hpp"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "Emu/system_config.h"
#include "Utilities/Thread.h"

#ifdef _WIN32
#include "windows.h"
#include "sysinfoapi.h"
#include "subauth.h"
#include "stringapiset.h"
#else
#include <unistd.h>
#include <sys/utsname.h>
#include <errno.h>
#endif

inline std::array<u32, 4> utils::get_cpuid(u32 func, u32 subfunc)
{
	int regs[4];
#ifdef _MSC_VER
	__cpuidex(regs, func, subfunc);
#else
	__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func), "c" (subfunc));
#endif
	return {0u+regs[0], 0u+regs[1], 0u+regs[2], 0u+regs[3]};
}

inline u64 utils::get_xgetbv(u32 xcr)
{
#ifdef _MSC_VER
	return _xgetbv(xcr);
#else
	u32 eax, edx;
	__asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
	return eax | (u64(edx) << 32);
#endif
}

bool utils::has_ssse3()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x200;
	return g_value;
}

bool utils::has_sse41()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x80000;
	return g_value;
}

bool utils::has_avx()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x10000000 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
}

bool utils::has_avx2()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && get_cpuid(7, 0)[1] & 0x20 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
}

bool utils::has_rtm()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x800) == 0x800;
	return g_value;
}

bool utils::has_tsx_force_abort()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[3] & 0x2000) == 0x2000;
	return g_value;
}

bool utils::has_mpx()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x4000) == 0x4000;
	return g_value;
}

bool utils::has_avx512()
{
	// Check AVX512F, AVX512CD, AVX512DQ, AVX512BW, AVX512VL extensions (Skylake-X level support)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0xd0030000) == 0xd0030000 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0xe6) == 0xe6;
	return g_value;
}

bool utils::has_avx512_icl()
{
	// Check AVX512IFMA, AVX512VBMI, AVX512VBMI2, AVX512VPOPCNTDQ, AVX512BITALG, AVX512VNNI, AVX512VPCLMULQDQ, AVX512GFNI, AVX512VAES (Icelake-client level support)
	static const bool g_value = has_avx512() && (get_cpuid(7, 0)[1] & 0x00200000) == 0x00200000 && (get_cpuid(7, 0)[2] & 0x00005f42) == 0x00005f42;
	return g_value;
}

bool utils::has_xop()
{
	static const bool g_value = has_avx() && get_cpuid(0x80000001, 0)[2] & 0x800;
	return g_value;
}

bool utils::has_clwb()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x1000000) == 0x1000000;
	return g_value;
}

bool utils::has_invariant_tsc()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000007, 0)[3] & 0x100) == 0x100;
	return g_value;
}

bool utils::has_fma3()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x1000;
	return g_value;
}

bool utils::has_fma4()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000001, 0)[2] & 0x10000) == 0x10000;
	return g_value;
}

std::string utils::get_cpu_brand()
{
	std::string brand;

	if (get_cpuid(0x80000000, 0)[0] >= 0x80000004)
	{
		for (u32 i = 0; i < 3; i++)
		{
			brand.append(reinterpret_cast<const char*>(get_cpuid(0x80000002 + i, 0).data()), 16);
		}
	}
	else
	{
		brand = "Unknown CPU";
	}

	brand.erase(brand.find_last_not_of('\0') + 1);
	brand.erase(brand.find_last_not_of(' ') + 1);
	brand.erase(0, brand.find_first_not_of(' '));

	while (auto found = brand.find("  ") + 1)
	{
		brand.erase(brand.begin() + found);
	}

	return brand;
}

std::string utils::get_system_info()
{
	std::string result;

	const std::string brand = get_cpu_brand();
	const u64 mem_total = get_total_memory();
	const u32 num_proc = get_thread_count();

	fmt::append(result, "%s | %d Threads | %.2f GiB RAM", brand, num_proc, mem_total / (1024.0f * 1024 * 1024));

	if (const ullong tsc_freq = get_tsc_freq())
	{
		fmt::append(result, " | TSC: %.03fGHz", tsc_freq / 1000000000.);
	}
	else
	{
		fmt::append(result, " | TSC: Bad");
	}

	if (has_avx())
	{
		result += " | AVX";

		if (has_avx512())
		{
			result += "-512";

			if (has_avx512_icl())
			{
				result += '+';
			}
		}
		else if (has_avx2())
		{
			result += '+';
		}

		if (has_xop())
		{
			result += 'x';
		}
	}

	if (has_fma3() || has_fma4())
	{
		result += " | FMA";

		if (has_fma3() && has_fma4())
		{
			result += "3+4";
		}
		else if (has_fma3())
		{
			result += "3";
		}
		else if (has_fma4())
		{
			result += "4";
		}
	}

	if (has_rtm())
	{
		result += " | TSX";

		if (has_tsx_force_abort())
		{
			result += "-FA";
		}

		if (!has_mpx())
		{
			result += " disabled by default";
		}
	}

	return result;
}

std::string utils::get_firmware_version()
{
	const std::string file_path = g_cfg.vfs.get_dev_flash() + "vsh/etc/version.txt";
	if (fs::is_file(file_path))
	{
		const fs::file version_file = fs::file(file_path);
		std::string version = version_file.to_string();

		// Extract version
		const usz start = version.find_first_of(':') + 1;
		const usz end = version.find_first_of(':', start);
		version = version.substr(start, end - start);

		// Trim version
		const usz trim_start = version.find_first_not_of('0');
		const usz trim_end = version.find_last_not_of('0');
		version = version.substr(trim_start, trim_end);

		return version;
	}
	return "";
}

std::string utils::get_OS_version()
{
	std::string output;
#ifdef _WIN32
	// GetVersionEx is deprecated, RtlGetVersion is kernel-mode only and AnalyticsInfo is UWP only.
	// So we're forced to read PEB instead to get Windows version info. It's ugly but works.

	const DWORD peb_offset = 0x60;
	const INT_PTR peb = __readgsqword(peb_offset);

	const DWORD version_major = *reinterpret_cast<const DWORD*>(peb + 0x118);
	const DWORD version_minor = *reinterpret_cast<const DWORD*>(peb + 0x11c);
	const WORD build = *reinterpret_cast<const WORD*>(peb + 0x120);
	const UNICODE_STRING service_pack = *reinterpret_cast<const UNICODE_STRING*>(peb + 0x02E8);
	const u64 compatibility_mode = *reinterpret_cast<const u64*>(peb + 0x02C8); // Two DWORDs, major & minor version

	const bool has_sp = service_pack.Length > 0;
	std::vector<char> holder(service_pack.Length + 1, '\0');
	if (has_sp)
	{
		WideCharToMultiByte(CP_UTF8, NULL, service_pack.Buffer, service_pack.Length,
			(LPSTR) holder.data(), static_cast<int>(holder.size()), NULL, NULL);
	}

	fmt::append(output,
		"Operating system: Windows, Major: %lu, Minor: %lu, Build: %u, Service Pack: %s, Compatibility mode: %llu",
		version_major, version_minor, build, has_sp ? holder.data() : "none", compatibility_mode);
#else
	struct utsname details = {};

	if (!uname(&details))
	{
		fmt::append(output, "Operating system: POSIX, Name: %s, Release: %s, Version: %s",
			details.sysname, details.release, details.version);
	}
	else
	{
		fmt::append(output, "Operating system: POSIX, Unknown version! (Error: %d)", errno);
	}
#endif
	return output;
}

static constexpr ullong round_tsc(ullong val)
{
	return ::rounded_div(val, 1'000'000) * 1'000'000;
}

ullong utils::get_tsc_freq()
{
	static const ullong cal_tsc = []() -> ullong
	{
		if (!has_invariant_tsc())
			return 0;

#ifdef _WIN32
		LARGE_INTEGER freq;
		if (!QueryPerformanceFrequency(&freq))
			return 0;

		if (freq.QuadPart <= 9'999'999)
			return round_tsc(freq.QuadPart * 1024);

		const ullong timer_freq = freq.QuadPart;
#else
		const ullong timer_freq = 1'000'000'000;
#endif

		// Calibrate TSC
		constexpr int samples = 40;
		ullong rdtsc_data[samples];
		ullong timer_data[samples];
		ullong error_data[samples];

		// Narrow thread affinity to a single core
		const u64 old_aff = thread_ctrl::get_thread_affinity_mask();
		thread_ctrl::set_thread_affinity_mask(old_aff & (0 - old_aff));

#ifndef _WIN32
		struct timespec ts0;
		clock_gettime(CLOCK_MONOTONIC, &ts0);
		ullong sec_base = ts0.tv_sec;
#endif

		for (int i = 0; i < samples; i++)
		{
#ifdef _WIN32
			Sleep(1);
			error_data[i] = (_mm_lfence(), __rdtsc());
			LARGE_INTEGER ctr;
			QueryPerformanceCounter(&ctr);
			rdtsc_data[i] = (_mm_lfence(), __rdtsc());
			timer_data[i] = ctr.QuadPart;
#else
			usleep(200);
			error_data[i] = (_mm_lfence(), __rdtsc());
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			rdtsc_data[i] = (_mm_lfence(), __rdtsc());
			timer_data[i] = ts.tv_nsec + (ts.tv_sec - sec_base) * 1'000'000'000;
#endif
		}

		// Restore main thread affinity
		thread_ctrl::set_thread_affinity_mask(old_aff);

		// Compute average TSC
		ullong acc = 0;
		for (int i = 0; i < samples - 1; i++)
		{
			acc += (rdtsc_data[i + 1] - rdtsc_data[i]) * timer_freq / (timer_data[i + 1] - timer_data[i]);
		}

		// Rounding
		return round_tsc(acc / (samples - 1));
	}();

	return cal_tsc;
}

u64 utils::get_total_memory()
{
#ifdef _WIN32
	::MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(memInfo);
	::GlobalMemoryStatusEx(&memInfo);
	return memInfo.ullTotalPhys;
#else
	return ::sysconf(_SC_PHYS_PAGES) * ::sysconf(_SC_PAGE_SIZE);
#endif
}

u32 utils::get_thread_count()
{
#ifdef _WIN32
	::SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo(&sysInfo);
	return sysInfo.dwNumberOfProcessors;
#else
	return ::sysconf(_SC_NPROCESSORS_ONLN);
#endif
}
