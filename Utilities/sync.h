#pragma once

/* For internal use. Don't include. */

#include "types.h"
#include "Atomic.h"
#include "dynamic_library.h"

#ifdef _WIN32
#include <Windows.h>
#include <time.h>
#elif __linux__
#include <errno.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <unistd.h>
#else
#endif
#include <ctime>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#ifdef _WIN32
DYNAMIC_IMPORT("ntdll.dll", NtWaitForKeyedEvent, NTSTATUS(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtReleaseKeyedEvent, NTSTATUS(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtDelayExecution, NTSTATUS(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval));
#endif

#ifndef __linux__
enum
{
	FUTEX_PRIVATE_FLAG = 0,
	FUTEX_WAIT = 0,
	FUTEX_WAIT_PRIVATE = FUTEX_WAIT,
	FUTEX_WAKE = 1,
	FUTEX_WAKE_PRIVATE = FUTEX_WAKE,
	FUTEX_BITSET = 2,
	FUTEX_WAIT_BITSET = FUTEX_WAIT | FUTEX_BITSET,
	FUTEX_WAIT_BITSET_PRIVATE = FUTEX_WAIT_BITSET,
	FUTEX_WAKE_BITSET = FUTEX_WAKE | FUTEX_BITSET,
	FUTEX_WAKE_BITSET_PRIVATE = FUTEX_WAKE_BITSET,
};
#endif

inline int futex(int* uaddr, int futex_op, int val, const timespec* timeout, int* uaddr2, int val3)
{
#ifdef __linux__
	return syscall(SYS_futex, uaddr, futex_op, val, timeout, uaddr, val3);
#else
	static struct futex_map
	{
		struct waiter
		{
			 int val;
			 uint mask;
			 std::condition_variable cv;
		};

		std::mutex mutex;
		std::unordered_multimap<int*, waiter*, pointer_hash<int>> map;

		int operator()(int* uaddr, int futex_op, int val, const timespec* timeout, int*, uint val3)
		{
			std::unique_lock<std::mutex> lock(mutex);

			switch (futex_op)
			{
			case FUTEX_WAIT:
			{
				val3 = -1;
				// Fallthrough
			}
			case FUTEX_WAIT_BITSET:
			{
				if (*(volatile int*)uaddr != val)
				{
					errno = EAGAIN;
					return -1;
				}

				waiter rec;
				rec.val = val;
				rec.mask = val3;
				const auto& ref = *map.emplace(uaddr, &rec);

				int res = 0;

				if (!timeout)
				{
					rec.cv.wait(lock, [&] { return !rec.mask; });
				}
				else if (futex_op == FUTEX_WAIT)
				{
					const auto nsec = std::chrono::nanoseconds(timeout->tv_nsec + timeout->tv_sec * 1000000000ull);

					if (!rec.cv.wait_for(lock, nsec, [&] { return !rec.mask; }))
					{
						res = -1;
						errno = ETIMEDOUT;
					}
				}
				else
				{
					// TODO
				}

				map.erase(std::find(map.find(uaddr), map.end(), ref));
				return res;
			}

			case FUTEX_WAKE:
			{
				val3 = -1;
				// Fallthrough
			}
			case FUTEX_WAKE_BITSET:
			{
				int res = 0;

				for (auto range = map.equal_range(uaddr); val && range.first != range.second; range.first++)
				{
					auto& entry = *range.first->second;

					if (entry.mask & val3)
					{
						entry.cv.notify_one();
						entry.mask = 0;
						res++;
						val--;
					}
				}

				return res;
			}
			}

			errno = EINVAL;
			return -1;
		}
	} g_futex;

	return g_futex(uaddr, futex_op, val, timeout, uaddr2, val3);
#endif
}
