#include "util/types.hpp"
#include "util/cpu_stats.hpp"
#include "util/sysinfo.hpp"
#include "util/logs.hpp"
#include "Utilities/StrUtil.h"

#include <algorithm>

#ifdef _WIN32
#include "util/asm.hpp"
#include "windows.h"
#include "tlhelp32.h"
#ifdef _MSC_VER
#pragma comment(lib, "pdh.lib")
#endif
#else
#include "fstream"
#include "sstream"
#include "stdlib.h"
#include "sys/times.h"
#endif

#ifdef __APPLE__
# include <mach/mach_init.h>
# include <mach/task.h>
# include <mach/vm_map.h>
#endif

#ifdef __linux__
# include <dirent.h>
#endif

#if defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__)
# include <sys/sysctl.h>
# include <unistd.h>
# if defined(__DragonFly__) || defined(__FreeBSD__)
#  include <sys/user.h>
# endif

# if defined(__NetBSD__)
#  undef KERN_PROC
#  define KERN_PROC KERN_PROC2
#  define kinfo_proc kinfo_proc2
# endif

# if defined(__DragonFly__)
#  define KP_NLWP(kp) (kp.kp_nthreads)
# elif defined(__FreeBSD__)
#  define KP_NLWP(kp) (kp.ki_numthreads)
# elif defined(__NetBSD__)
#  define KP_NLWP(kp) (kp.p_nlwps)
# endif
#endif

LOG_CHANNEL(perf_log, "PERF");

namespace utils
{
#ifdef _WIN32
	fmt::win_error pdh_error(PDH_STATUS status)
	{
		return fmt::win_error{static_cast<unsigned long>(status), LoadLibrary(L"pdh.dll")};
	}
#endif

	cpu_stats::cpu_stats()
	{
#ifdef _WIN32
		FILETIME ftime, fsys, fuser;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&m_last_cpu, &ftime, sizeof(FILETIME));

		GetProcessTimes(GetCurrentProcess(), &ftime, &ftime, &fsys, &fuser);
		memcpy(&m_sys_cpu, &fsys, sizeof(FILETIME));
		memcpy(&m_usr_cpu, &fuser, sizeof(FILETIME));
#else
		struct tms timeSample;

		m_last_cpu = times(&timeSample);
		m_sys_cpu  = timeSample.tms_stime;
		m_usr_cpu  = timeSample.tms_utime;
#endif
	}

	cpu_stats::~cpu_stats()
	{
#ifdef _WIN32
		if (m_cpu_query)
		{
			PDH_STATUS status = PdhCloseQuery(m_cpu_query);
			if (ERROR_SUCCESS != status)
			{
				perf_log.error("Failed to close cpu query of per core cpu usage: %s", pdh_error(status));
			}
		}
#endif
	}

	void cpu_stats::init_cpu_query()
	{
#ifdef _WIN32
		PDH_STATUS status = PdhOpenQuery(NULL, 0, &m_cpu_query);
		if (ERROR_SUCCESS != status)
		{
			perf_log.error("Failed to open cpu query for per core cpu usage: %s", pdh_error(status));
			return;
		}
		status = PdhAddEnglishCounter(m_cpu_query, L"\\Processor(*)\\% Processor Time", 0, &m_cpu_cores);
		if (ERROR_SUCCESS != status)
		{
			perf_log.error("Failed to add processor time counter for per core cpu usage: %s", pdh_error(status));
			return;
		}
		status = PdhCollectQueryData(m_cpu_query);
		if (ERROR_SUCCESS != status)
		{
			perf_log.error("Failed to collect per core cpu usage: %s", pdh_error(status));
			return;
		}
#endif
	}

	void cpu_stats::get_per_core_usage(std::vector<double>& per_core_usage, double& total_usage)
	{
		total_usage = 0.0;

		per_core_usage.resize(utils::get_thread_count());
		std::fill(per_core_usage.begin(), per_core_usage.end(), 0.0);

#ifdef _WIN32
		if (!m_cpu_cores || !m_cpu_query)
		{
			perf_log.warning("Can not collect per core cpu usage: The required API is not initialized.");
			return;
		}

		PDH_STATUS status = PdhCollectQueryData(m_cpu_query);
		if (ERROR_SUCCESS != status)
		{
			perf_log.error("Failed to collect per core cpu usage: %s", pdh_error(status));
			return;
		}

		DWORD dwBufferSize = 0; // Size of the items buffer
		DWORD dwItemCount = 0;  // Number of items in the items buffer

		status = PdhGetFormattedCounterArray(m_cpu_cores, PDH_FMT_DOUBLE, &dwBufferSize, &dwItemCount, nullptr);
		if (static_cast<PDH_STATUS>(PDH_MORE_DATA) == status)
		{
			std::vector<PDH_FMT_COUNTERVALUE_ITEM> items(utils::aligned_div(dwBufferSize, sizeof(PDH_FMT_COUNTERVALUE_ITEM)));
			if (items.size() >= dwItemCount)
			{
				status = PdhGetFormattedCounterArray(m_cpu_cores, PDH_FMT_DOUBLE, &dwBufferSize, &dwItemCount, items.data());
				if (ERROR_SUCCESS == status)
				{
					ensure(dwItemCount == per_core_usage.size() + 1); // Plus one for _Total

					// Loop through the array and get the instance name and percentage.
					for (usz i = 0; i < dwItemCount; i++)
					{
						const PDH_FMT_COUNTERVALUE_ITEM& item = items[i];
						const std::string token = wchar_to_utf8(item.szName);

						if (const std::string lower = fmt::to_lower(token); lower.find("total") != umax)
						{
							total_usage = item.FmtValue.doubleValue;
							continue;
						}

						if (const auto [success, cpu_index] = string_to_number(token); success && cpu_index < dwItemCount)
						{
							per_core_usage[cpu_index] = item.FmtValue.doubleValue;
						}
						else if (!success)
						{
							perf_log.error("Can not convert string to cpu index for per core cpu usage. (token='%s')", token);
						}
						else
						{
							perf_log.error("Invalid cpu index for per core cpu usage. (token='%s', cpu_index=%d, cores=%d)", token, cpu_index, dwItemCount);
						}
					}
				}
				else if (static_cast<PDH_STATUS>(PDH_CALC_NEGATIVE_DENOMINATOR) == status) // Apparently this is a common uncritical error
				{
					perf_log.notice("Failed to get per core cpu usage: %s", pdh_error(status));
				}
				else
				{
					perf_log.error("Failed to get per core cpu usage: %s", pdh_error(status));
				}
			}
			else
			{
				perf_log.error("Failed to allocate buffer for per core cpu usage. (size=%d, dwItemCount=%d)", items.size(), dwItemCount);
			}
		}

#elif __linux__
#ifndef ANDROID
		m_previous_idle_times_per_cpu.resize(utils::get_thread_count(), 0.0);
		m_previous_total_times_per_cpu.resize(utils::get_thread_count(), 0.0);

		if (std::ifstream proc_stat("/proc/stat"); proc_stat.good())
		{
			std::stringstream content;
			content << proc_stat.rdbuf();
			proc_stat.close();

			const std::vector<std::string> lines = fmt::split(content.str(), {"\n"});
			if (lines.empty())
			{
				perf_log.error("/proc/stat is empty");
				return;
			}

			for (const std::string& line : lines)
			{
				const std::vector<std::string> tokens = fmt::split(line, {" "});
				if (tokens.size() < 5)
				{
					return;
				}

				const std::string& token = tokens[0];
				if (!token.starts_with("cpu"))
				{
					return;
				}

				// Get CPU index
				int cpu_index = -1; // -1 for total

				constexpr size_t size_of_cpu = 3;
				if (token.size() > size_of_cpu)
				{
					if (const auto [success, val] = string_to_number(token.substr(size_of_cpu)); success && val < per_core_usage.size())
					{
						cpu_index = val;
					}
					else if (!success)
					{
						perf_log.error("Can not convert string to cpu index for per core cpu usage. (token='%s', line='%s')", token, line);
						continue;
					}
					else
					{
						perf_log.error("Invalid cpu index for per core cpu usage. (cpu_index=%d, cores=%d, token='%s', line='%s')", cpu_index, per_core_usage.size(), token, line);
						continue;
					}
				}

				size_t idle_time = 0;
				size_t total_time = 0;

				for (size_t i = 1; i < tokens.size(); i++)
				{
					if (const auto [success, val] = string_to_number(tokens[i]); success)
					{
						if (i == 4)
						{
							idle_time = val;
						}

						total_time += val;
					}
					else
					{
						perf_log.error("Can not convert string to time for per core cpu usage. (i=%d, token='%s', line='%s')", i, tokens[i], line);
					}
				}

				if (cpu_index < 0)
				{
					const double idle_time_delta = idle_time - std::exchange(m_previous_idle_time_total, idle_time);
					const double total_time_delta = total_time - std::exchange(m_previous_total_time_total, total_time);
					total_usage = 100.0 * (1.0 - idle_time_delta / total_time_delta);
				}
				else
				{
					const double idle_time_delta = idle_time - std::exchange(m_previous_idle_times_per_cpu[cpu_index], idle_time);
					const double total_time_delta = total_time - std::exchange(m_previous_total_times_per_cpu[cpu_index], total_time);
					per_core_usage[cpu_index] = 100.0 * (1.0 - idle_time_delta / total_time_delta);
				}
			}
		}
		else
		{
			perf_log.error("Failed to open /proc/stat (%s)", strerror(errno));
		}
#endif
#else
		total_usage = get_usage();
#endif
	}

	double cpu_stats::get_usage()
	{
#ifdef _WIN32
		FILETIME ftime, fsys, fusr;
		ULARGE_INTEGER now, sys, usr;
		double percent;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));

		GetProcessTimes(GetCurrentProcess(), &ftime, &ftime, &fsys, &fusr);
		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&usr, &fusr, sizeof(FILETIME));

		if (now.QuadPart <= m_last_cpu || sys.QuadPart < m_sys_cpu || usr.QuadPart < m_usr_cpu)
		{
			// Overflow detection. Just skip this value.
			percent = 0.0;
		}
		else
		{
			percent = static_cast<double>((sys.QuadPart - m_sys_cpu) + (usr.QuadPart - m_usr_cpu));
			percent /= (now.QuadPart - m_last_cpu);
			percent /= utils::get_thread_count(); // Let's assume this is at least 1
			percent *= 100;
		}

		m_last_cpu = now.QuadPart;
		m_usr_cpu  = usr.QuadPart;
		m_sys_cpu  = sys.QuadPart;

		return std::clamp(percent, 0.0, 100.0);
#else
		struct tms timeSample;
		clock_t now = times(&timeSample);
		double percent;

		if (now <= static_cast<clock_t>(m_last_cpu) || timeSample.tms_stime < static_cast<clock_t>(m_sys_cpu) || timeSample.tms_utime < static_cast<clock_t>(m_usr_cpu))
		{
			// Overflow detection. Just skip this value.
			percent = 0.0;
		}
		else
		{
			percent = (timeSample.tms_stime - m_sys_cpu) + (timeSample.tms_utime - m_usr_cpu);
			percent /= (now - m_last_cpu);
			percent /= utils::get_thread_count();
			percent *= 100;
		}
		m_last_cpu = now;
		m_sys_cpu  = timeSample.tms_stime;
		m_usr_cpu  = timeSample.tms_utime;

		return std::clamp(percent, 0.0, 100.0);
#endif
	}

	u32 cpu_stats::get_current_thread_count() // static
	{
#ifdef _WIN32
		// first determine the id of the current process
		const DWORD id = GetCurrentProcessId();

		// then get a process list snapshot.
		const HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		// initialize the process entry structure.
		PROCESSENTRY32 entry = {0};
		entry.dwSize         = sizeof(entry);

		// get the first process info.
		BOOL ret = Process32First(snapshot, &entry);
		while (ret && entry.th32ProcessID != id)
		{
			ret = Process32Next(snapshot, &entry);
		}
		CloseHandle(snapshot);
		return ret ? entry.cntThreads : 0;
#elif defined(__APPLE__)
		const task_t task = mach_task_self();
		mach_msg_type_number_t thread_count;
		thread_act_array_t thread_list;
		if (task_threads(task, &thread_list, &thread_count) != KERN_SUCCESS)
		{
			return 0;
		}
		vm_deallocate(task, reinterpret_cast<vm_address_t>(thread_list),
			      sizeof(thread_t) * thread_count);
		return static_cast<u32>(thread_count);
#elif defined(__DragonFly__) || defined(__FreeBSD__) || defined(__NetBSD__)
		int mib[] = {
			CTL_KERN,
			KERN_PROC,
			KERN_PROC_PID,
			getpid(),
#if defined(__NetBSD__)
			sizeof(struct kinfo_proc),
			1,
#endif
		};
		u_int miblen = std::size(mib);
		struct kinfo_proc info;
		usz size = sizeof(info);
		if (sysctl(mib, miblen, &info, &size, NULL, 0))
		{
			return 0;
		}
		return KP_NLWP(info);
#elif defined(__OpenBSD__)
		int mib[] = {
			CTL_KERN,
			KERN_PROC,
			KERN_PROC_PID | KERN_PROC_SHOW_THREADS,
			getpid(),
			sizeof(struct kinfo_proc),
			0,
		};
		u_int miblen = std::size(mib);

		// get number of structs
		usz size;
		if (sysctl(mib, miblen, NULL, &size, NULL, 0))
		{
			return 0;
		}
		mib[5] = size / mib[4];

		// populate array of structs
		struct kinfo_proc info[mib[5]];
		if (sysctl(mib, miblen, &info, &size, NULL, 0))
		{
			return 0;
		}

		// exclude empty members
		u32 thread_count{0};
		for (int i = 0; i < size / mib[4]; i++)
		{
			if (info[i].p_tid != -1)
				++thread_count;
		}
		return thread_count;
#elif defined(__linux__)
		u32 thread_count{0};

		DIR* proc_dir = opendir("/proc/self/task");
		if (proc_dir)
		{
			// proc available, iterate through tasks and count them
			const struct dirent* entry;
			while ((entry = readdir(proc_dir)) != NULL)
			{
				if (entry->d_name[0] == '.')
					continue;

				++thread_count;
			}

			closedir(proc_dir);
		}
		return thread_count;
#else
		// unimplemented
		return 0;
#endif
	}
}
