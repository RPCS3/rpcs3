#pragma once

/* For internal use. Don't include. */

#include "types.h"
#include "util/atomic.hpp"
#include "dynamic_library.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#include <time.h>
#elif __linux__
#include <errno.h>
#include <sys/syscall.h>
#include <linux/futex.h>
#include <sys/time.h>
#include <unistd.h>
#include <fcntl.h>
#endif
#include <algorithm>
#include <ctime>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#ifdef _WIN32
DYNAMIC_IMPORT("ntdll.dll", NtWaitForKeyedEvent, NTSTATUS(HANDLE, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtReleaseKeyedEvent, NTSTATUS(HANDLE, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtWaitForSingleObject, NTSTATUS(HANDLE Handle, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtDelayExecution, NTSTATUS(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval));
DYNAMIC_IMPORT("ntdll.dll", NtWaitForAlertByThreadId, NTSTATUS(PVOID Address, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtAlertThreadByThreadId, NTSTATUS(DWORD_PTR ThreadId));

constexpr NTSTATUS NTSTATUS_SUCCESS = 0;
constexpr NTSTATUS NTSTATUS_ALERTED = 0x101;
constexpr NTSTATUS NTSTATUS_TIMEOUT = 0x102;
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

inline int futex(volatile void* uaddr, int futex_op, uint val, const timespec* timeout = nullptr, uint mask = 0)
{
#ifdef __linux__
	return syscall(SYS_futex, uaddr, futex_op, static_cast<int>(val), timeout, nullptr, static_cast<int>(mask));
#else
	static struct futex_manager
	{
		struct waiter
		{
			 uint val;
			 uint mask;
			 std::condition_variable cv;
		};

		std::mutex mutex;
		std::unordered_multimap<volatile void*, waiter*, pointer_hash<volatile void, alignof(int)>> map;

		int operator()(volatile void* uaddr, int futex_op, uint val, const timespec* timeout, uint mask)
		{
			std::unique_lock lock(mutex);

			switch (futex_op)
			{
			case FUTEX_WAIT_PRIVATE:
			{
				mask = -1;
				[[fallthrough]];
			}
			case FUTEX_WAIT_BITSET_PRIVATE:
			{
				if (*reinterpret_cast<volatile uint*>(uaddr) != val)
				{
					errno = EAGAIN;
					return -1;
				}

				waiter rec;
				rec.val = val;
				rec.mask = mask;
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

			case FUTEX_WAKE_PRIVATE:
			{
				mask = -1;
				[[fallthrough]];
			}
			case FUTEX_WAKE_BITSET_PRIVATE:
			{
				int res = 0;

				for (auto range = map.equal_range(uaddr); val && range.first != range.second; range.first++)
				{
					auto& entry = *range.first->second;

					if (entry.mask & mask)
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

	return g_futex(uaddr, futex_op, val, timeout, mask);
#endif
}
