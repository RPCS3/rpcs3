#pragma once

bool SM_IsAborted();
void SM_Sleep();

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
		if (SM_IsAborted())
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
		if (SM_IsAborted())
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

typedef SMutexBase<u32>
	SMutex;
