#pragma once
#include <atomic>

extern void SM_Sleep();
extern DWORD SM_GetCurrentThreadId();
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
	u32 free_value = 0,
	u32 dead_value = ~0,
	void (wait)() = SM_Sleep
>
class SMutexBase
{
	static_assert(sizeof(T) == 4, "Invalid SMutexBase typename");
	std::atomic<T> owner;

public:
	SMutexBase()
		: owner((T)free_value)
	{
	}

	~SMutexBase()
	{
		lock((T)dead_value);
		owner = (T)dead_value;
	}

	__forceinline T GetOwner() const
	{
		return (T&)owner;
	}

	SMutexResult trylock(T tid)
	{
		T old = (T)free_value;

		if (!owner.compare_exchange_strong(old, tid))
		{
			if (old == tid)
			{
				return SMR_DEADLOCK;
			}
			if (Emu.IsStopped())
			{
				return SMR_ABORT;
			}
			if (old == (T)dead_value)
			{
				return SMR_DESTROYED;
			}

			return SMR_FAILED;
		}

		return SMR_OK;
	}

	SMutexResult unlock(T tid, T to = (T)free_value)
	{
		T old = tid;

		if (!owner.compare_exchange_strong(old, to))
		{
			if (old == (T)free_value)
			{
				return SMR_FAILED;
			}
			if (old == (T)dead_value)
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

			wait();

			if (timeout && counter++ > timeout)
			{
				return SMR_TIMEOUT;
			}
		}
	}
};

template<typename T, T (get_tid)()>
class SMutexLockerBase
{
	SMutexBase<T>& sm;
public:
	const T tid;

	SMutexLockerBase(SMutexBase<T>& _sm)
		: sm(_sm)
		, tid(get_tid())
	{
		if (!tid) throw "SMutexLockerBase: invalid thread id";
		sm.lock(tid);
	}

	~SMutexLockerBase()
	{
		sm.unlock(tid);
	}
};

typedef SMutexBase<DWORD>
	SMutexGeneral;
typedef SMutexBase<u32>
	SMutex;
typedef SMutexBase<be_t<u32>>
	SMutexBE;

typedef SMutexLockerBase<DWORD, SM_GetCurrentThreadId>
	SMutexGeneralLocker;
typedef SMutexLockerBase<u32, SM_GetCurrentCPUThreadId>
	SMutexLocker;
typedef SMutexLockerBase<be_t<u32>, SM_GetCurrentCPUThreadIdBE>
	SMutexBELocker;