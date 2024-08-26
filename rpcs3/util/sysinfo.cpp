#include "util/sysinfo.hpp"
#include "Utilities/StrFmt.h"
#include "Utilities/File.h"
#include "Emu/vfs_config.h"
#include "Utilities/Thread.h"

#if defined(ARCH_ARM64)
#include "Emu/CPU/Backends/AArch64/AArch64Common.h"
#endif

#ifdef _WIN32
#include "windows.h"
#include "sysinfoapi.h"
#include "subauth.h"
#include "stringapiset.h"
#else
#include <unistd.h>
#include <sys/resource.h>
#ifndef __APPLE__
#include <sys/utsname.h>
#include <errno.h>
#endif
#endif

#include "util/asm.hpp"
#include "util/fence.hpp"

#if defined(_M_X64) && defined(_MSC_VER)
extern "C" u64 _xgetbv(u32);
#endif

#if defined(ARCH_X64)
static inline std::array<u32, 4> get_cpuid(u32 func, u32 subfunc)
{
	int regs[4];
#ifdef _MSC_VER
	__cpuidex(regs, func, subfunc);
#else
	__asm__ volatile("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3]) : "a" (func), "c" (subfunc));
#endif
	return {0u+regs[0], 0u+regs[1], 0u+regs[2], 0u+regs[3]};
}

static inline u64 get_xgetbv(u32 xcr)
{
#ifdef _MSC_VER
	return _xgetbv(xcr);
#else
	u32 eax, edx;
	__asm__ volatile("xgetbv" : "=a"(eax), "=d"(edx) : "c"(xcr));
	return eax | (u64(edx) << 32);
#endif
}
#endif

#ifdef __APPLE__
// sysinfo_darwin.mm
namespace Darwin_Version
{
	extern int getNSmajorVersion();
	extern int getNSminorVersion();
	extern int getNSpatchVersion();
}

namespace Darwin_ProcessInfo
{
	extern bool getLowPowerModeEnabled();
}
#endif

bool utils::has_ssse3()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x200;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_sse41()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x80000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x10000000 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx2()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && get_cpuid(7, 0)[1] & 0x20 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0x6) == 0x6;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_rtm()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x800) == 0x800;
	return g_value;
#elif defined(ARCH_ARM64)
	return false;
#endif
}

bool utils::has_tsx_force_abort()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[3] & 0x2000) == 0x2000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_rtm_always_abort()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[3] & 0x800) == 0x800;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_mpx()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x4000) == 0x4000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx512()
{
#if defined(ARCH_X64)
	// Check AVX512F, AVX512CD, AVX512DQ, AVX512BW, AVX512VL extensions (Skylake-X level support)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0xd0030000) == 0xd0030000 && (get_cpuid(1, 0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0xe6) == 0xe6;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx512_icl()
{
#if defined(ARCH_X64)
	// Check AVX512IFMA, AVX512VBMI, AVX512VBMI2, AVX512VPOPCNTDQ, AVX512BITALG, AVX512VNNI, AVX512VPCLMULQDQ, AVX512GFNI, AVX512VAES (Icelake-client level support)
	static const bool g_value = has_avx512() && (get_cpuid(7, 0)[1] & 0x00200000) == 0x00200000 && (get_cpuid(7, 0)[2] & 0x00005f42) == 0x00005f42;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx512_vnni()
{
#if defined(ARCH_X64)
	// Check AVX512VNNI
	static const bool g_value = has_avx512() && get_cpuid(7, 0)[2] & 0x00000800;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx10()
{
#if defined(ARCH_X64)
	// Implies support for most AVX-512 instructions
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && get_cpuid(7, 1)[3] & 0x80000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx10_512()
{
#if defined(ARCH_X64)
	// AVX10 with 512 wide vectors
	static const bool g_value = has_avx10() && get_cpuid(24, 0)[2] & 0x40000;
	return g_value;
#else
	return false;
#endif
}

u32 utils::avx10_isa_version()
{
#if defined(ARCH_X64)
	// 8bit value
	static const u32 g_value = []()
	{
		u32 isa_version = 0;
		if (has_avx10())
		{
			isa_version = get_cpuid(24, 0)[2] & 0x000ff;
		}

		return isa_version;
	}();

	return g_value;
#else
	return 0;
#endif
}

bool utils::has_avx512_256()
{
#if defined(ARCH_X64)
	// Either AVX10 or AVX512 implies support for 256-bit length AVX-512 SKL-X tier instructions
	static const bool g_value = (has_avx512() || has_avx10());
	return g_value;
#else
	return false;
#endif
}

bool utils::has_avx512_icl_256()
{
#if defined(ARCH_X64)
	// Check for AVX512_ICL or check for AVX10, together with GFNI, VAES, and VPCLMULQDQ, implies support for the same instructions that AVX-512_icl does at 256 bit length
	static const bool g_value = (has_avx512_icl() || (has_avx10() && get_cpuid(7, 0)[2] & 0x00000700));
	return g_value;
#else
	return false;
#endif
}

bool utils::has_xop()
{
#if defined(ARCH_X64)
	static const bool g_value = has_avx() && get_cpuid(0x80000001, 0)[2] & 0x800;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_clwb()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x1000000) == 0x1000000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_invariant_tsc()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000007, 0)[3] & 0x100) == 0x100;
	return g_value;
#elif defined(ARCH_ARM64)
	return true;
#endif
}

bool utils::has_fma3()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x1 && get_cpuid(1, 0)[2] & 0x1000;
	return g_value;
#elif defined(ARCH_ARM64)
	return true;
#endif
}

bool utils::has_fma4()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000001, 0)[2] & 0x10000) == 0x10000;
	return g_value;
#else
	return false;
#endif
}

// The Zen4 based CPUs support VPERMI2B/VPERMT2B in a single uop.
// Current Intel cpus (as of 2022) need 3 uops to execute these instructions.
// Check for SSE4A (which intel doesn't doesn't support) as well as VBMI.
bool utils::has_fast_vperm2b()
{
#if defined(ARCH_X64)
	static const bool g_value = has_avx512() && (get_cpuid(7, 0)[2] & 0x2) == 0x2 && get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000001, 0)[2] & 0x40) == 0x40;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_erms()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x200) == 0x200;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_fsrm()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[3] & 0x10) == 0x10;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_waitx()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(0x80000001, 0)[2] & 0x20000000) == 0x20000000;
	return g_value;
#else
	return false;
#endif
}

bool utils::has_waitpkg()
{
#if defined(ARCH_X64)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[2] & 0x20) == 0x20;
	return g_value;
#else
	return false;
#endif
}

// User mode waits may be unfriendly to low thread CPUs
// Filter out systems with less than 8 threads for linux and less than 12 threads for other platforms
bool utils::has_appropriate_um_wait()
{
#ifdef __linux__
	static const bool g_value = (has_waitx() || has_waitpkg()) && (get_thread_count() >= 8) && get_tsc_freq();
	return g_value;
#else
	static const bool g_value = (has_waitx() || has_waitpkg()) && (get_thread_count() >= 12) && get_tsc_freq();
	return g_value;
#endif
}

// Similar to the above function but allow execution if alternatives such as yield are not wanted
bool utils::has_um_wait()
{
	static const bool g_value = (has_waitx() || has_waitpkg()) && get_tsc_freq();
	return g_value;
}

u32 utils::get_rep_movsb_threshold()
{
	static const u32 g_value = []()
	{
		u32 thresh_value = umax;
		if (has_fsrm())
		{
			thresh_value = 2047;
		}
		else if (has_erms())
		{
			thresh_value = 4095;
		}

		return thresh_value;
	}();

	return g_value;
}

std::string utils::get_cpu_brand()
{
#if defined(ARCH_X64)
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
#elif defined(ARCH_ARM64)
	static const auto g_cpu_brand = aarch64::get_cpu_brand();
	return g_cpu_brand;
#else
	return "Unidentified CPU";
#endif
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

		if (has_avx10())
		{
			const u32 avx10_version = avx10_isa_version();
			fmt::append(result, "10.%d", avx10_version);

			if (has_avx10_512())
			{
				result += "-512";
			}
			else
			{
				result += "-256";
			}
		}
		else if (has_avx512())
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

		if (!has_mpx() || has_tsx_force_abort())
		{
			result += " disabled by default";
		}
	}
	else if (has_rtm_always_abort())
	{
		result += " | TSX disabled via microcode";
	}

	return result;
}

std::string utils::get_firmware_version()
{
	const std::string file_path = g_cfg_vfs.get_dev_flash() + "vsh/etc/version.txt";
	if (fs::file version_file{file_path})
	{
		const std::string version_str = version_file.to_string();
		std::string_view version = version_str;

		// Extract version
		const usz start = version.find_first_of(':') + 1;
		const usz end = version.find_first_of(':', start);

		if (!start || end == umax)
		{
			return {};
		}

		version = version.substr(start, end - start);

		// Trim version (e.g. '04.8900' becomes '4.89')
		usz trim_start = version.find_first_not_of('0');

		if (trim_start == umax)
		{
			return {};
		}

		// Keep at least one preceding 0 (e.g. '00.3100' becomes '0.31' instead of '.31')
		if (version[trim_start] == '.')
		{
			if (trim_start == 0)
			{
				// Version starts with '.' for some reason
				return {};
			}

			trim_start--;
		}

		const usz dot_pos = version.find_first_of('.', trim_start);

		if (dot_pos == umax)
		{
			return {};
		}

		// Try to keep the second 0 in the minor version (e.g. '04.9000' becomes '4.90' instead of '4.9')
		const usz trim_end = std::max(version.find_last_not_of('0', dot_pos + 1), std::min(dot_pos + 2, version.size()));

		return std::string(version.substr(trim_start, trim_end));
	}

	return {};
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
		WideCharToMultiByte(CP_UTF8, 0, service_pack.Buffer, service_pack.Length,
			static_cast<LPSTR>(holder.data()), static_cast<int>(holder.size()), nullptr, nullptr);
	}

	fmt::append(output,
		"Operating system: Windows, Major: %lu, Minor: %lu, Build: %u, Service Pack: %s, Compatibility mode: %llu",
		version_major, version_minor, build, has_sp ? holder.data() : "none", compatibility_mode);
#elif defined (__APPLE__)
	const int major_version = Darwin_Version::getNSmajorVersion();
	const int minor_version = Darwin_Version::getNSminorVersion();
	const int patch_version = Darwin_Version::getNSpatchVersion();

	fmt::append(output, "Operating system: macOS, Version: %d.%d.%d",
		major_version, minor_version, patch_version);
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

int utils::get_maxfiles()
{
#ifdef _WIN32
	// Virtually unlimited on Windows
	return INT_MAX;
#else
	struct rlimit limits;
	ensure(getrlimit(RLIMIT_NOFILE, &limits) == 0);

	return limits.rlim_cur;
#endif
}

bool utils::get_low_power_mode()
{
#ifdef __APPLE__
	return Darwin_ProcessInfo::getLowPowerModeEnabled();
#else
	return false;
#endif
}

static constexpr ullong round_tsc(ullong val)
{
	return utils::rounded_div(val, 1'000'000) * 1'000'000;
}

ullong utils::get_tsc_freq()
{
	static const ullong cal_tsc = []() -> ullong
	{
#ifdef ARCH_ARM64
		u64 r = 0;
		__asm__ volatile("mrs %0, cntfrq_el0" : "=r" (r));
		return r;
#endif

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
		[[maybe_unused]] ullong error_data[samples];

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
			error_data[i] = (utils::lfence(), utils::get_tsc());
			LARGE_INTEGER ctr;
			QueryPerformanceCounter(&ctr);
			rdtsc_data[i] = (utils::lfence(), utils::get_tsc());
			timer_data[i] = ctr.QuadPart;
#else
			usleep(200);
			error_data[i] = (utils::lfence(), utils::get_tsc());
			struct timespec ts;
			clock_gettime(CLOCK_MONOTONIC, &ts);
			rdtsc_data[i] = (utils::lfence(), utils::get_tsc());
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
	static const u32 g_count = []()
	{
#ifdef _WIN32
		::SYSTEM_INFO sysInfo;
		::GetNativeSystemInfo(&sysInfo);
		return sysInfo.dwNumberOfProcessors;
#else
		return ::sysconf(_SC_NPROCESSORS_ONLN);
#endif
	}();

	return g_count;
}

u32 utils::get_cpu_family()
{
#if defined(ARCH_X64)
	static const u32 g_value = []()
	{
		const u32 reg_value = get_cpuid(0x00000001, 0)[0]; // Processor feature info
		const u32 base_value = (reg_value >> 8) & 0xF;

		if (base_value == 0xF) [[likely]]
		{
			const u32 extended_value = (reg_value >> 20) & 0xFF;
			return base_value + extended_value;
		}
		else
		{
			return base_value;
		}
	}();

	return g_value;
#elif defined(ARCH_ARM64)
	return 0;
#endif
}

u32 utils::get_cpu_model()
{
#if defined(ARCH_X64)
	static const u32 g_value = []()
	{
		const u32 reg_value = get_cpuid(0x00000001, 0)[0]; // Processor feature info
		const u32 base_value = (reg_value >> 4) & 0xF;

		if (const auto base_family_id = (reg_value >> 8) & 0xF;
			base_family_id == 0x6 || base_family_id == 0xF) [[likely]]
		{
			const u32 extended_value = (reg_value >> 16) & 0xF;
			return base_value + (extended_value << 4);
		}
		else
		{
			return base_value;
		}
	}();

	return g_value;
#elif defined(ARCH_ARM64)
	return 0;
#endif
}

namespace utils
{
	u64 _get_main_tid()
	{
		return thread_ctrl::get_tid();
	}
}
