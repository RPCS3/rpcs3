#pragma once

#include <Utilities/types.h>

#ifdef _WIN32
#include "windows.h"
#include "tlhelp32.h"
#else
#include "dirent.h"
#include "stdlib.h"
#include "stdio.h"
#include "string.h"
#include "sys/times.h"
#include "sys/types.h"
#endif

class CPUStats
{
#ifdef _WIN32
	HANDLE m_self;
	using time_type = ULARGE_INTEGER;
#else
	using time_type = clock_t;
#endif

private:
	s32 m_num_processors;
	time_type m_last_cpu, m_sys_cpu, m_usr_cpu;

public:
	CPUStats()
	{
#ifdef _WIN32
		SYSTEM_INFO sysInfo;
		FILETIME ftime, fsys, fuser;

		GetSystemInfo(&sysInfo);
		m_num_processors = sysInfo.dwNumberOfProcessors;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&m_last_cpu, &ftime, sizeof(FILETIME));

		m_self = GetCurrentProcess();
		GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fuser);
		memcpy(&m_sys_cpu, &fsys, sizeof(FILETIME));
		memcpy(&m_usr_cpu, &fuser, sizeof(FILETIME));
#else
		FILE* file;
		struct tms timeSample;
		char line[128];

		m_last_cpu = times(&timeSample);
		m_sys_cpu  = timeSample.tms_stime;
		m_usr_cpu  = timeSample.tms_utime;

		file             = fopen("/proc/cpuinfo", "r");
		m_num_processors = 0;
		while (fgets(line, 128, file) != NULL)
		{
			if (strncmp(line, "processor", 9) == 0)
				m_num_processors++;
		}
		fclose(file);
#endif
	}

	double get_usage()
	{
#ifdef _WIN32
		FILETIME ftime, fsys, fusr;
		ULARGE_INTEGER now, sys, usr;

		GetSystemTimeAsFileTime(&ftime);
		memcpy(&now, &ftime, sizeof(FILETIME));

		GetProcessTimes(m_self, &ftime, &ftime, &fsys, &fusr);
		memcpy(&sys, &fsys, sizeof(FILETIME));
		memcpy(&usr, &fusr, sizeof(FILETIME));
		double percent = (sys.QuadPart - m_sys_cpu.QuadPart) + (usr.QuadPart - m_usr_cpu.QuadPart);
		percent /= (now.QuadPart - m_last_cpu.QuadPart);
		percent /= m_num_processors;

		m_last_cpu = now;
		m_usr_cpu  = usr;
		m_sys_cpu  = sys;

		return std::clamp(percent * 100, 0.0, 100.0);
#else
		struct tms timeSample;
		clock_t now;
		double percent;

		now = times(&timeSample);
		if (now <= m_last_cpu || timeSample.tms_stime < m_sys_cpu || timeSample.tms_utime < m_usr_cpu)
		{
			// Overflow detection. Just skip this value.
			percent = -1.0;
		}
		else
		{
			percent = (timeSample.tms_stime - m_sys_cpu) + (timeSample.tms_utime - m_usr_cpu);
			percent /= (now - m_last_cpu);
			percent /= m_num_processors;
			percent *= 100;
		}
		m_last_cpu = now;
		m_sys_cpu  = timeSample.tms_stime;
		m_usr_cpu  = timeSample.tms_utime;

		return percent;
#endif
	}

	static u32 get_thread_count()
	{
#ifdef _WIN32
		// first determine the id of the current process
		DWORD const id = GetCurrentProcessId();

		// then get a process list snapshot.
		HANDLE const snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPALL, 0);

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
#else
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
#endif
	}
};
