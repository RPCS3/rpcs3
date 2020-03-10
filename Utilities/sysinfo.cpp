#include "sysinfo.h"
#include "StrFmt.h"
#include "File.h"
#include "Emu/system_config.h"

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
		fmt::append(result, " | TSC: %.02fGHz", tsc_freq / 1000000000.);
	}
	else
	{
		fmt::append(result, " | TSC: Bad");
	}

	if (has_avx())
	{
		result += " | AVX";

		if (has_avx2())
		{
			result += '+';
		}

		if (has_avx512())
		{
			result += '+';
		}

		if (has_xop())
		{
			result += 'x';
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
		const size_t start = version.find_first_of(':') + 1;
		const size_t end = version.find_first_of(':', start);
		version = version.substr(start, end - start);

		// Trim version
		const size_t trim_start = version.find_first_not_of('0');
		const size_t trim_end = version.find_last_not_of('0');
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

ullong utils::get_tsc_freq()
{
#ifdef _WIN32
	LARGE_INTEGER freq;
	if (!QueryPerformanceFrequency(&freq) || freq.QuadPart > 9'999'999)
		return 0;
	return freq.QuadPart * 1024;
#else
	// TODO
	return 0;
#endif
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
