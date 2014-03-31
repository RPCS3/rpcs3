#pragma once

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
	u64 free_value = 0,
	u64 dead_value = 0xffffffff,
	void (*wait)() = SM_Sleep
>
class SMutexBase
{
	static_assert(sizeof(T) == sizeof(std::atomic<T>), "Invalid SMutexBase type");
	std::atomic<T> owner;

public:
	SMutexBase()
		: owner((T)free_value)
	{
	}

	void initialize()
	{
		(T&)owner = free_value;
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

	__forceinline T GetFreeValue() const
	{
		return (T)free_value;
	}

	__forceinline T GetDeadValue() const
	{
		return (T)dead_value;
	}

	SMutexResult trylock(T tid)
	{
		if (Emu.IsStopped())
		{
			return SMR_ABORT;
		}
		T old = (T)free_value;

		if (!owner.compare_exchange_strong(old, tid))
		{
			if (old == tid)
			{
				return SMR_DEADLOCK;
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
		if (Emu.IsStopped())
		{
			return SMR_ABORT;
		}
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

	__forceinline SMutexLockerBase(SMutexBase<T>& _sm)
		: sm(_sm)
		, tid(get_tid())
	{
		if (!tid)
		{
			if (!Emu.IsStopped())
			{
				ConLog.Error("SMutexLockerBase: thread id == 0");
				Emu.Pause();
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

typedef SMutexLockerBase<size_t, SM_GetCurrentThreadId>
	SMutexGeneralLocker;
typedef SMutexLockerBase<u32, SM_GetCurrentCPUThreadId>
	SMutexLocker;
typedef SMutexLockerBase<be_t<u32>, SM_GetCurrentCPUThreadIdBE>
	SMutexBELocker;
