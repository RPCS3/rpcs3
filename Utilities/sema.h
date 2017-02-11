#pragma once

#include "types.h"
#include "Atomic.h"

// Lightweight semaphore helper class
class semaphore_base
{
	// Semaphore value (shifted; negative values imply 0 with waiters, LSB is used to ping-pong signals between threads)
	atomic_t<s32> m_value;

	void imp_wait(bool lsb);

	void imp_post(s32 _old);

	friend class semaphore_lock;

protected:
	explicit constexpr semaphore_base(s32 value)
		: m_value{value}
	{
	}

	void wait()
	{
		// Unconditional decrement
		if (UNLIKELY(m_value.sub_fetch(1 << 1) < 0))
		{
			imp_wait(false);
		}
	}

	bool try_wait();

	void post(s32 _max)
	{
		// Unconditional increment
		const s32 value = m_value.fetch_add(1 << 1);

		if (UNLIKELY(value < 0 || value >= _max))
		{
			imp_post(value & ~1);
		}
	}

	bool try_post(s32 _max);

public:
	// Get current semaphore value
	s32 get() const
	{
		// Load value
		const s32 value = m_value;

		// Return only positive value
		return value < 0 ? 0 : value >> 1;
	}
};

// Lightweight semaphore template (default arguments define binary semaphore and Def == Max)
template <s32 Max = 1, s32 Def = Max>
class semaphore final : public semaphore_base
{
	static_assert(Max >= 0 && Max < (1 << 30), "semaphore<>: Max is out of bounds");
	static_assert(Def >= 0 && Def < (1 << 30), "semaphore<>: Def is out of bounds");
	static_assert(Def <= Max, "semaphore<>: Def is too big");

	using base = semaphore_base;

public:
	// Default constructor (recommended)
	constexpr semaphore()
		: base{Def << 1}
	{
	}

	// Explicit value constructor (not recommended)
	explicit constexpr semaphore(s32 value)
		: base{value << 1}
	{
	}

	// Obtain a semaphore
	void wait()
	{
		return base::wait();
	}

	// Try to obtain a semaphore
	explicit_bool_t try_wait()
	{
		return base::try_wait();
	}

	// Return a semaphore
	void post()
	{
		return base::post(Max << 1);
	}

	// Try to return a semaphore
	explicit_bool_t try_post()
	{
		return base::try_post(Max << 1);
	}

	// Get max semaphore value
	static constexpr s32 size()
	{
		return Max;
	}
};

class semaphore_lock
{
	semaphore_base& m_base;

	void lock()
	{
		m_base.wait();
	}

	void unlock()
	{
		m_base.post(INT32_MAX);
	}

	friend class cond_variable;

public:
	explicit semaphore_lock(const semaphore_lock&) = delete;

	semaphore_lock(semaphore_base& sema)
		: m_base(sema)
	{
		lock();
	}

	~semaphore_lock()
	{
		unlock();
	}
};
