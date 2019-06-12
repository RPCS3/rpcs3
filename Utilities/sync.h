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
#include <algorithm>
#include <ctime>
#include <chrono>
#include <mutex>
#include <condition_variable>
#include <unordered_map>

#ifdef _WIN32
DYNAMIC_IMPORT("ntdll.dll", NtWaitForKeyedEvent, NTSTATUS(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtReleaseKeyedEvent, NTSTATUS(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout));
DYNAMIC_IMPORT("ntdll.dll", NtDelayExecution, NTSTATUS(BOOLEAN Alertable, PLARGE_INTEGER DelayInterval));
inline utils::dynamic_import<BOOL(volatile VOID* Address, PVOID CompareAddress, SIZE_T AddressSize, DWORD dwMilliseconds)> OptWaitOnAddress("kernel32.dll", "WaitOnAddress");
inline utils::dynamic_import<VOID(PVOID Address)> OptWakeByAddressSingle("kernel32.dll", "WakeByAddressSingle");
inline utils::dynamic_import<VOID(PVOID Address)> OptWakeByAddressAll("kernel32.dll", "WakeByAddressAll");
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

template <typename T, typename Pred>
bool balanced_wait_until(atomic_t<T>& var, u64 usec_timeout, Pred&& pred)
{
	static_assert(sizeof(T) == 4 || sizeof(T) == 8);

	const bool is_inf = usec_timeout > u64{UINT32_MAX / 1000} * 1000000;

	// Optional second argument indicates that the predicate should try to retire
	auto test_pred = [&](T& _new, auto... args)
	{
		T old = var.load();

		while (true)
		{
			_new = old;

			// Zero indicates failure without modifying the value
			// Negative indicates failure but modifies the value
			auto ret = std::invoke(std::forward<Pred>(pred), _new, args...);

			if (LIKELY(!ret || var.compare_exchange(old, _new)))
			{
				return ret > 0;
			}
		}
	};

	T value;

#ifdef _WIN32
	if (OptWaitOnAddress)
	{
		while (!test_pred(value))
		{
			if (OptWaitOnAddress(&var, &value, sizeof(T), is_inf ? INFINITE : usec_timeout / 1000))
			{
				if (!test_pred(value, nullptr))
				{
					return false;
				}

				break;
			}

			if (GetLastError() == ERROR_TIMEOUT)
			{
				// Retire
				return test_pred(value, nullptr);
			}
		}

		return true;
	}

	LARGE_INTEGER timeout;
	timeout.QuadPart = usec_timeout * -10;

	if (!usec_timeout || NtWaitForKeyedEvent(nullptr, &var, false, is_inf ? nullptr : &timeout))
	{
		// Timed out: retire
		if (!test_pred(value, nullptr))
		{
			return false;
		}

		// Signaled in the last moment: restore balance
		NtWaitForKeyedEvent(nullptr, &var, false, nullptr);
		return true;
	}

	if (!test_pred(value, nullptr))
	{
		// Stolen notification: restore balance
		NtReleaseKeyedEvent(nullptr, &var, false, nullptr);
		return false;
	}

	return true;
#else
	struct timespec timeout;
	timeout.tv_sec  = usec_timeout / 1000000;
	timeout.tv_nsec = (usec_timeout % 1000000) * 1000;

	char* ptr = reinterpret_cast<char*>(&var);

	if constexpr (sizeof(T) == 8)
	{
		ptr += 4 * IS_BE_MACHINE;
	}

	while (!test_pred(value))
	{
		if (futex(ptr, FUTEX_WAIT_PRIVATE, static_cast<u32>(value), is_inf ? nullptr : &timeout) == 0)
		{
			if (!test_pred(value, nullptr))
			{
				return false;
			}

			break;
		}

		switch (errno)
		{
		case EAGAIN: break;
		case ETIMEDOUT: return test_pred(value, nullptr);
		default: verify("Unknown futex error" HERE), 0;
		}
	}

	return true;
#endif
}

template <bool All = false, typename T>
void balanced_awaken(atomic_t<T>& var, u32 weight)
{
	static_assert(sizeof(T) == 4 || sizeof(T) == 8);

#ifdef _WIN32
	if (OptWaitOnAddress)
	{
		if (All || weight > 3)
		{
			OptWakeByAddressAll(&var);
			return;
		}

		for (u32 i = 0; i < weight; i++)
		{
			OptWakeByAddressSingle(&var);
		}

		return;
	}

	for (u32 i = 0; i < weight; i++)
	{
		NtReleaseKeyedEvent(nullptr, &var, false, nullptr);
	}
#else
	char* ptr = reinterpret_cast<char*>(&var);

	if constexpr (sizeof(T) == 8)
	{
		ptr += 4 * IS_BE_MACHINE;
	}

	if (All || weight)
	{
		futex(ptr, FUTEX_WAKE_PRIVATE, All ? INT_MAX : std::min<u32>(INT_MAX, weight));
	}

	return;
#endif
}
