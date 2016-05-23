#pragma once

/* For internal use. Don't include. */

#include "types.h"
#include "Macro.h"
#include "Atomic.h"

#ifdef _WIN32

#include <Windows.h>

#define DYNAMIC_IMPORT(handle, name) do { name = reinterpret_cast<decltype(name)>(GetProcAddress(handle, #name)); } while (0)

static NTSTATUS(*NtSetTimerResolution)(ULONG DesiredResolution, BOOLEAN SetResolution, PULONG CurrentResolution);
static NTSTATUS(*NtWaitForKeyedEvent)(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout);
static NTSTATUS(*NtReleaseKeyedEvent)(HANDLE Handle, PVOID Key, BOOLEAN Alertable, PLARGE_INTEGER Timeout);

namespace util
{
	static const bool keyed_init = []
	{
		const auto handle = LoadLibraryA("ntdll.dll");
		DYNAMIC_IMPORT(handle, NtSetTimerResolution);
		DYNAMIC_IMPORT(handle, NtWaitForKeyedEvent);
		DYNAMIC_IMPORT(handle, NtReleaseKeyedEvent);
		FreeLibrary(handle);

		ULONG res = 100;
		NtSetTimerResolution(100, TRUE, &res);

		return NtWaitForKeyedEvent && NtReleaseKeyedEvent;
	}();

	// Wait for specified condition. func() acknowledges success by value modification.
	template<typename F>
	inline void keyed_wait(atomic_t<u32>& key, F&& func)
	{
		while (true)
		{
			u32 read = key.load();
			u32 copy = read;

			while (func(read), read != copy)
			{
				read = key.compare_and_swap(copy, read);
				
				if (copy == read)
				{
					return;
				}
				
				copy = read;
			}

			NtWaitForKeyedEvent(NULL, &key, FALSE, NULL);
		}
	}

	// Try to wake up a thread.
	inline bool keyed_post(atomic_t<u32>& key, u32 acknowledged_value)
	{
		LARGE_INTEGER timeout;
		timeout.QuadPart = -50;

		while (UNLIKELY(NtReleaseKeyedEvent(NULL, &key, FALSE, &timeout) != ERROR_SUCCESS))
		{
			if (key.load() != acknowledged_value)
				return false;
		}

		return true;
	}

	struct native_rwlock
	{
		SRWLOCK rwlock = SRWLOCK_INIT;

		constexpr native_rwlock() = default;

		native_rwlock(const native_rwlock&) = delete;

		void lock()
		{
			AcquireSRWLockExclusive(&rwlock);
		}

		bool try_lock()
		{
			return TryAcquireSRWLockExclusive(&rwlock) != 0;
		}

		void unlock()
		{
			ReleaseSRWLockExclusive(&rwlock);
		}

		void lock_shared()
		{
			AcquireSRWLockShared(&rwlock);
		}

		bool try_lock_shared()
		{
			return TryAcquireSRWLockShared(&rwlock) != 0;
		}

		void unlock_shared()
		{
			ReleaseSRWLockShared(&rwlock);
		}
	};

	struct native_cond
	{
		CONDITION_VARIABLE cond = CONDITION_VARIABLE_INIT;

		constexpr native_cond() = default;

		native_cond(const native_cond&) = delete;

		void notify_one()
		{
			WakeConditionVariable(&cond);
		}

		void notify_all()
		{
			WakeAllConditionVariable(&cond);
		}

		void wait(native_rwlock& rwlock)
		{
			SleepConditionVariableSRW(&cond, &rwlock.rwlock, INFINITE, 0);
		}

		void wait_shared(native_rwlock& rwlock)
		{
			SleepConditionVariableSRW(&cond, &rwlock.rwlock, INFINITE, CONDITION_VARIABLE_LOCKMODE_SHARED);
		}
	};

	class exclusive_lock
	{
		native_rwlock& m_rwlock;

	public:
		exclusive_lock(native_rwlock& rwlock)
			: m_rwlock(rwlock)
		{
			m_rwlock.lock();
		}

		~exclusive_lock()
		{
			m_rwlock.unlock();
		}
	};

	class shared_lock
	{
		native_rwlock& m_rwlock;

	public:
		shared_lock(native_rwlock& rwlock)
			: m_rwlock(rwlock)
		{
			m_rwlock.lock_shared();
		}

		~shared_lock()
		{
			m_rwlock.unlock_shared();
		}
	};
}

#else

namespace util
{
	struct native_rwlock;
	struct native_cond;
}

#endif

CHECK_SIZE_ALIGN(util::native_rwlock, sizeof(void*), alignof(void*));
CHECK_SIZE_ALIGN(util::native_cond, sizeof(void*), alignof(void*));
