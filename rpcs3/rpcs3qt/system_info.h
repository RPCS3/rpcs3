#pragma once

#include <regex>

#ifdef _WIN32
#include "windows.h"
#include <bitset>
typedef unsigned __int32  uint32_t;
#else
#include <stdint.h>
#include <unistd.h>
#include <cmath>
#endif

class System_Info
{
	class CPUID {
		uint32_t regs[4];

	public:
		explicit CPUID(uint32_t func, uint32_t subfunc) {
#ifdef _WIN32 
			__cpuidex((int *)regs, func, subfunc);
#else 
			asm volatile
				("cpuid" : "=a" (regs[0]), "=b" (regs[1]), "=c" (regs[2]), "=d" (regs[3])
					: "a" (func), "c" (subfunc));
			// ECX is set to zero for CPUID function 4 
#endif 
		}

		const uint32_t &EAX() const { return regs[0]; }
		const uint32_t &EBX() const { return regs[1]; }
		const uint32_t &ECX() const { return regs[2]; }
		const uint32_t &EDX() const { return regs[3]; }
		const uint32_t *data() const { return &regs[0]; }
	};

public:

	/**
		Retrieves various information about the system's hardware
		@return a pair consisting of the compiled information as string value and a bool for SSSE3 compatibility
	*/
	static const std::pair<std::string, bool> getCPU()
	{
		int nIds_ = 0;
		int nExIds_ = 0;
		char brand[0x40];
		std::bitset<32> cpu_capabilities = 0;

		nIds_ = CPUID(0, 0).EAX();
		// load bitset with flags for function 0x00000001 
		if (nIds_ >= 1)
		{
			cpu_capabilities = CPUID(1, 0).ECX();
		}

		nExIds_ = CPUID(0x80000000, 0).EAX();
		memset(brand, 0, sizeof(brand));
		if (nExIds_ >= 0x80000004)
		{
			memcpy(brand, CPUID(0x80000002, 0).data(), 16);
			memcpy(brand + 16, CPUID(0x80000003, 0).data(), 16);
			memcpy(brand + 32, CPUID(0x80000004, 0).data(), 16);
		}

		bool supports_ssse3 = cpu_capabilities[9];

		std::string s_sysInfo = fmt::format("%s | SSSE3 %s", std::regex_replace(brand, std::regex("^ +"), ""), supports_ssse3 ? "Supported" : "Not Supported");

#ifdef _WIN32
		SYSTEM_INFO sysInfo;
		GetNativeSystemInfo(&sysInfo);
		MEMORYSTATUSEX memInfo;
		memInfo.dwLength = sizeof(memInfo);
		GlobalMemoryStatusEx(&memInfo);
		s_sysInfo += fmt::format(" | %d Threads | %.2f GB RAM", sysInfo.dwNumberOfProcessors, (float)memInfo.ullTotalPhys / std::pow(1024.0f, 3));
#else
		long mem_total = sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE);
		s_sysInfo += fmt::format(" | %d Threads | %.2f GB RAM", sysconf(_SC_NPROCESSORS_ONLN), (float)mem_total / std::pow(1024.0f, 3));
#endif

		return std::pair<std::string, bool>(s_sysInfo, supports_ssse3);
	};
};
