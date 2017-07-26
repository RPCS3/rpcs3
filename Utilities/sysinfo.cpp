#include "sysinfo.h"
#include "StrFmt.h"

#ifdef _WIN32
#include "windows.h"
#else
#include <unistd.h>
#endif

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
	}

	if (has_rtm())
	{
		result += " | TSX";
	}

	return result;
}
