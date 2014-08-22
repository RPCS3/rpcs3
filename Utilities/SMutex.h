#pragma once
#include "BEType.h"
#include "Emu/System.h"

extern void SM_Sleep();
extern size_t SM_GetCurrentThreadId();
extern u32 SM_GetCurrentCPUThreadId();
extern be_t<u32> SM_GetCurrentCPUThreadIdBE();

enum SMutexResult
{
	SMR_OK = 0, // succeeded (lock, trylock, unlock)
	SMR_FAILED, // failed (trylock, unlock)
	SMR_DEADLOCK, // mutex reached deadlock (lock, trylock)
	SMR_SIGNAL = SMR_DEADLOCK, // unlock can be used for signaling specific thread
	SMR_PERMITTED, // not owner of the mutex (unlock)
	SMR_ABORT, // emulator has been stopped (lock, trylock, unlock)
	SMR_DESTROYED, // mutex has been destroyed (lock, trylock, unlock)
	SMR_TIMEOUT, // timed out (lock)
};

template
<
	typename T,
	const u64 free_value = 0,
	const u64 dead_value = 0xffffffffffffffffull,
	void (*wait)() = SM_Sleep
>
class SMutexBase
{
	static_assert(sizeof(T) == sizeof(std::atomic<T>), "Invalid SMutexBase type");
	std::atomic<T> owner;

public:
	static const T GetFreeValue()
	{
		static const u64 value = free_value;
		return (const T&)value;
	}

	static const T GetDeadValue()
	{
		static const u64 value = dead_value;
		return (const T&)value;
	}

	void initialize()
	{
		owner = GetFreeValue();
	}

	SMutexBase()
	{
		initialize();
	}

	void finalize()
	{
		owner = GetDeadValue();
	}

	__forceinline T GetOwner() const
	{
		return (T&)owner;
	}

	SMutexResult trylock(T tid)
	{
		if (Emu.IsStopped())
		{
			return SMR_ABORT;
		}
		T old = GetFreeValue();

		if (!owner.compare_exchange_strong(old, tid))
		{
			if (old == tid)
			{
				return SMR_DEADLOCK;
			}
			if (old == GetDeadValue())
			{
				return SMR_DESTROYED;
			}
			return SMR_FAILED;
		}

		return SMR_OK;
	}

	SMutexResult unlock(T tid, T to = GetFreeValue())
	{
		if (Emu.IsStopped())
		{
			return SMR_ABORT;
		}
		T old = tid;

		if (!owner.compare_exchange_strong(old, to))
		{
			if (old == GetFreeValue())
			{
				return SMR_FAILED;
			}
			if (old == GetDeadValue())
			{
				return SMR_DESTROYED;
			}

			return SMR_PERMITTED;
		}

		return SMR_OK;
	}

	SMutexResult lock(T tid, u64 timeout = 0)
	{
		u64 counter = 0;

		while (true)
		{
			switch (SMutexResult res = trylock(tid))
			{
				case SMR_FAILED: break;
				default: return res;
			}

			if (wait != nullptr) wait();

			if (timeout && counter++ > timeout)
			{
				return SMR_TIMEOUT;
			}
		}
	}
};

template<typename T, typename Tid, Tid (get_tid)()>
class SMutexLockerBase
{
	T& sm;
public:
	const Tid tid;

	__forceinline SMutexLockerBase(T& _sm)
		: sm(_sm)
		, tid(get_tid())
	{
		if (!tid)
		{
			if (!Emu.IsStopped())
			{
				assert(!"SMutexLockerBase: thread id == 0");
			}
			return;
		}
		sm.lock(tid);
	}

	__forceinline ~SMutexLockerBase()
	{
		if (tid) sm.unlock(tid);
	}
};

typedef SMutexBase<size_t>
	SMutexGeneral;
typedef SMutexBase<u32>
	SMutex;
typedef SMutexBase<be_t<u32>>
	SMutexBE;

typedef SMutexLockerBase<SMutexGeneral, size_t, SM_GetCurrentThreadId>
	SMutexGeneralLocker;
typedef SMutexLockerBase<SMutex, u32, SM_GetCurrentCPUThreadId>
	SMutexLocker;
typedef SMutexLockerBase<SMutexBE, be_t<u32>, SM_GetCurrentCPUThreadIdBE>
	SMutexBELocker;
