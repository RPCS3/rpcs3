#include "sysinfo.h"
#include "StrFmt.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <unistd.h>
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

bool utils::has_mpx()
{
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0x4000) == 0x4000;
	return g_value;
}

bool utils::has_512()
{
	// Check AVX512F, AVX512CD, AVX512DQ, AVX512BW, AVX512VL extensions (Skylake-X level support)
	static const bool g_value = get_cpuid(0, 0)[0] >= 0x7 && (get_cpuid(7, 0)[1] & 0xd0030000) == 0xd0030000 && (get_cpuid(1,0)[2] & 0x0C000000) == 0x0C000000 && (get_xgetbv(0) & 0xe6) == 0xe6;
	return g_value;
}

bool utils::has_xop()
{
	static const bool g_value = has_avx() && get_cpuid(0x80000001, 0)[2] & 0x800;
	return g_value;
}

std::string utils::get_system_info()
{
	std::string result;
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

	brand.erase(0, brand.find_first_not_of(' '));
	brand.erase(brand.find_last_not_of(' ') + 1);

	while (auto found = brand.find("  ") + 1)
	{
		brand.erase(brand.begin() + found);
	}

#ifdef _WIN32
	::SYSTEM_INFO sysInfo;
	::GetNativeSystemInfo(&sysInfo);
	::MEMORYSTATUSEX memInfo;
	memInfo.dwLength = sizeof(memInfo);
	::GlobalMemoryStatusEx(&memInfo);
	const u32 num_proc = sysInfo.dwNumberOfProcessors;
	const u64 mem_total = memInfo.ullTotalPhys;
#else
	const u32 num_proc = ::sysconf(_SC_NPROCESSORS_ONLN);
	const u64 mem_total = ::sysconf(_SC_PHYS_PAGES) * ::sysconf(_SC_PAGE_SIZE);
#endif

	fmt::append(result, "%s | %d Threads | %.2f GiB RAM", brand, num_proc, mem_total / (1024.0f * 1024 * 1024));

	if (has_avx())
	{
		result += " | AVX";

		if (has_avx2())
		{
			result += '+';
		}

		if (has_512())
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
		if (!has_mpx())
		{
			result += " disabled by default";
		}
	}

	return result;
}
