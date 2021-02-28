#include "util/types.hpp"
#include "util/cpu_stats.hpp"
#include "util/sysinfo.hpp"
#include <algorithm>

#ifdef _WIN32
#include "windows.h"
#include "tlhelp32.h"
#else
#include "stdlib.h"
#include "sys/times.h"
#include "sys/types.h"
#include "unistd.h"
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

namespace utils
{
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
			percent = (sys.QuadPart - m_sys_cpu) + (usr.QuadPart - m_usr_cpu);
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

	u32 cpu_stats::get_thread_count() // static
	{
#ifdef _WIN32
		// first determine the id of the current process
		DWORD const id = GetCurrentProcessId();

		// then get a process list snapshot.
		HANDLE const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

		// initialize the process entry structure.
		PROCESSENTRY32 entry = {0};
		entry.dwSize         = sizeof(entry);

		// get the first process info.
		BOOL ret = true;
		ret      = Process32First(snapshot, &entry);
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
			struct dirent* entry;
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
