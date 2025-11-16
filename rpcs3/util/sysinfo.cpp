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

#include <thread>
#include <fstream>

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

#ifdef _WIN32
#if !defined(ARCH_X64)
namespace utils
{
	// Some helpers for sanity
	const auto read_reg_dword = [](HKEY hKey, std::string_view value_name) -> std::pair<bool, DWORD>
	{
		DWORD val = 0;
		DWORD len = sizeof(val);
		if (ERROR_SUCCESS != RegQueryValueExA(hKey, value_name.data(), nullptr, nullptr, reinterpret_cast<LPBYTE>(&val), &len))
		{
			return { false, 0 };
		}
		return { true, val };
	};

	const auto read_reg_sz = [](HKEY hKey, std::string_view value_name) -> std::pair<bool, std::string>
	{
		constexpr usz MAX_SZ_LEN = 255;
		char sz[MAX_SZ_LEN + 1] {};
		DWORD sz_len = MAX_SZ_LEN;

		// Safety; null terminate
		sz[0] = 0;
		sz[MAX_SZ_LEN] = 0;

		// Read string
		if (ERROR_SUCCESS != RegQueryValueExA(hKey, value_name.data(), nullptr, nullptr, reinterpret_cast<LPBYTE>(sz), &sz_len))
		{
			return { false, "" };
		}

		// Safety, force null terminator
		if (sz_len < MAX_SZ_LEN)
		{
			sz[sz_len] = 0;
		}
		return { true, sz };
	};

	// Alternative way to read OS version using the registry.
	static std::string get_fallback_windows_version()
	{
		HKEY hKey;
		if (ERROR_SUCCESS != RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey))
		{
			return "Unknown Windows";
		}

		// ProductName (SZ) - Actual windows install name e.g Windows 10 Pro)
		// CurrentMajorVersionNumber (DWORD) - e.g 10 for windows 10, 11 for windows 11
		// CurrentMinorVersionNumber (DWORD) - usually 0 for newer windows, pairs with major version
		// CurrentBuildNumber (SZ) - Windows build number, e.g 19045, used to identify different releases like 23H2, 24H2, etc
		// CurrentVersion (SZ) - NT kernel version, e.g 6.3 for Windows 10
		const auto [product_valid, product_name] = read_reg_sz(hKey, "ProductName");
		if (!product_valid)
		{
			RegCloseKey(hKey);
			return "Unknown Windows";
		}

		const auto [check_major, version_major] = read_reg_dword(hKey, "CurrentMajorVersionNumber");
		const auto [check_minor, version_minor] = read_reg_dword(hKey, "CurrentMinorVersionNumber");
		const auto [check_build_no, build_no] = read_reg_sz(hKey, "CurrentBuildNumber");
		const auto [check_nt_ver, nt_ver] = read_reg_sz(hKey, "CurrentVersion");

		// Close the registry key
		RegCloseKey(hKey);

		std::string version_id = "Unknown";
		if (check_major && check_minor && check_build_no)
		{
			version_id = fmt::format("%u.%u.%s", version_major, version_minor, build_no);
			if (check_nt_ver)
			{
				version_id += " NT" + nt_ver;
			}
		}

		return fmt::format("Operating system: %s, Version %s", product_name, version_id);
	}
}
#endif
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
		fmt::append(result, " | TSC: Disabled");
	}

	if (has_avx())
	{
		result += " | AVX";

		if (has_avx10())
		{
			const u32 avx10_version = avx10_isa_version();
			fmt::append(result, "10.%d", avx10_version);
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

utils::OS_version utils::get_OS_version()
{
	OS_version res {};

#if _WIN32
	res.type = "windows";
#elif __linux__
	res.type = "linux";
#elif __APPLE__
	res.type = "macos";
#elif __FreeBSD__
	res.type = "freebsd";
#else
	res.type = "unknown";
#endif

#if defined(ARCH_X64)
	res.arch = "x64";
#elif defined(ARCH_ARM64)
	res.arch = "arm64";
#else
	res.arch = "unknown";
#endif

#ifdef _WIN32
	// GetVersionEx is deprecated, RtlGetVersion is kernel-mode only and AnalyticsInfo is UWP only.
	// So we're forced to read PEB instead to get Windows version info. It's ugly but works.
#if defined(ARCH_X64)
	constexpr DWORD peb_offset = 0x60;
	const INT_PTR peb = __readgsqword(peb_offset);
	res.version_major = *reinterpret_cast<const DWORD*>(peb + 0x118);
	res.version_minor = *reinterpret_cast<const DWORD*>(peb + 0x11c);
	res.version_patch = *reinterpret_cast<const WORD*>(peb + 0x120);
#else
	HKEY hKey;
	if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion", 0, KEY_READ, &hKey) == ERROR_SUCCESS)
	{
		const auto [check_major, version_major] = read_reg_dword(hKey, "CurrentMajorVersionNumber");
		const auto [check_minor, version_minor] = read_reg_dword(hKey, "CurrentMinorVersionNumber");
		const auto [check_build, version_patch] = read_reg_sz(hKey, "CurrentBuildNumber");

		if (check_major) res.version_major = version_major;
		if (check_minor) res.version_minor = version_minor;
		if (check_build) res.version_patch = stoi(version_patch);

		RegCloseKey(hKey);
	}
#endif
#elif defined (__APPLE__)
	res.version_major = Darwin_Version::getNSmajorVersion();
	res.version_minor = Darwin_Version::getNSminorVersion();
	res.version_patch = Darwin_Version::getNSpatchVersion();
#else
	if (struct utsname details = {}; !uname(&details))
	{
		const std::vector<std::string> version_list = fmt::split(details.release, { "." });
		const auto get_version_part = [&version_list](usz i) -> usz
		{
			if (version_list.size() <= i) return 0;
			if (const auto [success, version_part] = string_to_number(version_list[i]); success)
			{
				return version_part;
			}
			return 0;
		};
		res.version_major = get_version_part(0);
		res.version_minor = get_version_part(1);
		res.version_patch = get_version_part(2);
	}
#endif

	return res;
}

std::string utils::get_OS_version_string()
{
	std::string output;
#ifdef _WIN32
	// GetVersionEx is deprecated, RtlGetVersion is kernel-mode only and AnalyticsInfo is UWP only.
	// So we're forced to read PEB instead to get Windows version info. It's ugly but works.
#if defined(ARCH_X64)
	constexpr DWORD peb_offset = 0x60;
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
#else
	// PEB cannot be easily accessed on ARM64, fall back to registry
	static const auto s_windows_version = utils::get_fallback_windows_version();
	return s_windows_version;
#endif
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

static constexpr ullong round_tsc(ullong val, ullong known_error)
{
	if (known_error >= 500'000)
	{
		// Do not accept large errors
		return 0;
	}

	ullong by = 1000;
	known_error /= 1000;

	while (known_error && by < 100'000)
	{
		by *= 10;
		known_error /= 10;
	}

	return utils::rounded_div(val, by) * by;
}

namespace utils
{
	u64 s_tsc_freq = 0;
}

static const bool s_tsc_freq_evaluated = []() -> bool
{
	static const ullong cal_tsc = []() -> ullong
	{
#ifdef ARCH_ARM64
		u64 r = 0;
		__asm__ volatile("mrs %0, cntfrq_el0" : "=r" (r));
		return r;
#endif

		if (!utils::has_invariant_tsc())
		{
			return 0;
		}

#ifdef _WIN32
		LARGE_INTEGER freq;
		if (!QueryPerformanceFrequency(&freq))
		{
			return 0;
		}

		if (freq.QuadPart <= 9'999'999)
		{
			return 0;
		}

		const ullong timer_freq = freq.QuadPart;
#else

#ifdef __linux__
		// Check if system clocksource is TSC. If the kernel trusts the TSC, we should too.
		// Some Ryzen laptops have broken firmware when running linux (requires a kernel patch). This is also a problem on some older intel CPUs.
		const char* clocksource_file = "/sys/devices/system/clocksource/clocksource0/available_clocksource";
		if (!fs::is_file(clocksource_file))
		{
			// OS doesn't support sysfs?
			printf("[TSC calibration] Could not determine available clock sources. Disabling TSC.\n");
			return 0;
		}

		std::string clock_sources;
		std::ifstream file(clocksource_file);
		std::getline(file, clock_sources);

		if (file.fail())
		{
			printf("[TSC calibration] Could not read the available clock sources on this system. Disabling TSC.\n");
			return 0;
		}

		printf("[TSC calibration] Available clock sources: '%s'\n", clock_sources.c_str());

		// Check if the Kernel has blacklisted the TSC
		const auto available_clocks = fmt::split(clock_sources, { " " });
		const bool tsc_reliable = std::find(available_clocks.begin(), available_clocks.end(), "tsc") != available_clocks.end();

		if (!tsc_reliable)
		{
			printf("[TSC calibration] TSC is not a supported clock source on this system.\n");
			return 0;
		}

		printf("[TSC calibration] Kernel reports the TSC is reliable.\n");
#else
		if (utils::get_cpu_brand().find("Ryzen") != umax)
		{
			// MacOS is arm-native these days and I don't know much about BSD to fix this if it's an issue. (kd-11)
			// Having this check only for Ryzen is broken behavior - other CPUs can also have this problem.
			return 0;
		}
#endif

		constexpr ullong timer_freq = 1'000'000'000;
#endif

		constexpr u64 retry_count = 1024;

		// First is entry is for the onset measurements, last is for the end measurements
		constexpr usz sample_count = 2;
		std::array<u64, sample_count> rdtsc_data{};
		std::array<u64, sample_count> rdtsc_diff{};
		std::array<u64, sample_count> timer_data{};

#ifdef _WIN32
		LARGE_INTEGER ctr0;
		QueryPerformanceCounter(&ctr0);
		const ullong time_base = ctr0.QuadPart;
#else
		struct timespec ts0;
		clock_gettime(CLOCK_MONOTONIC, &ts0);
		const ullong sec_base = ts0.tv_sec;
#endif

		constexpr usz sleep_time_ms = 40;

		for (usz sample = 0; sample < sample_count; sample++)
		{
			for (usz i = 0; i < retry_count; i++)
			{
				const u64 rdtsc_read = (utils::lfence(), utils::get_tsc());
#ifdef _WIN32
				LARGE_INTEGER ctr;
				QueryPerformanceCounter(&ctr);
#else
				struct timespec ts;
				clock_gettime(CLOCK_MONOTONIC, &ts);
#endif
				const u64 rdtsc_read2 = (utils::lfence(), utils::get_tsc());

#ifdef _WIN32
				const u64 timer_read = ctr.QuadPart - time_base;
#else
				const u64 timer_read = ts.tv_nsec + (ts.tv_sec - sec_base) * 1'000'000'000;
#endif

				if (i == 0 || (rdtsc_read2 >= rdtsc_read && rdtsc_read2 - rdtsc_read < rdtsc_diff[sample]))
				{
					rdtsc_data[sample] = rdtsc_read; // Note: rdtsc_read2 can also be written here because of the assumption of accuracy
					timer_data[sample] = timer_read;
					rdtsc_diff[sample] = rdtsc_read2 >= rdtsc_read ? rdtsc_read2 - rdtsc_read : u64{umax};
				}

				// 80 results in an error range of 4000 hertz (0.00025% of 4GHz CPU, quite acceptable)
				// Error of 2.5 seconds per month
				if (rdtsc_read2 - rdtsc_read < 80 && rdtsc_read2 >= rdtsc_read)
				{
					break;
				}

				// 8 yields seems to reduce significantly thread contention, improving accuracy
				// Even 3 seem to do the job though, but just in case
				if (i % 128 == 64)
				{
					std::this_thread::yield();
				}

				// Take 50% more yields with the last sample because it helps accuracy additionally the more time that passes
				if (sample == sample_count - 1 && i % 256 == 128)
				{
					std::this_thread::yield();
				}
			}

			if (sample < sample_count - 1)
			{
				// Sleep between first and last sample
#ifdef _WIN32
				Sleep(sleep_time_ms);
#else
				usleep(sleep_time_ms * 1000);
#endif
			}
		}

		if (timer_data[1] == timer_data[0])
		{
			// Division by zero
			return 0;
		}

		const u128 data = u128_from_mul(rdtsc_data[1] - rdtsc_data[0], timer_freq);

		const u64 res = utils::udiv128(static_cast<u64>(data >> 64), static_cast<u64>(data), (timer_data[1] - timer_data[0]));

		// Rounding
		return round_tsc(res, utils::mul_saturate<u64>(utils::add_saturate<u64>(rdtsc_diff[0], rdtsc_diff[1]), utils::aligned_div(timer_freq, timer_data[1] - timer_data[0])));
	}();

	atomic_storage<u64>::store(utils::s_tsc_freq, cal_tsc);
	return true;
}();

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

u64 utils::_get_main_tid()
{
	return thread_ctrl::get_tid();
}

std::pair<bool, usz> utils::string_to_number(std::string_view str)
{
	std::add_pointer_t<char> eval;
	const usz number = std::strtol(str.data(), &eval, 10);

	if (str.data() + str.size() == eval)
	{
		return { true, number };
	}

	return { false, 0 };
}
