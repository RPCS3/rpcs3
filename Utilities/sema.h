#pragma once

#include <mutex>
#include "types.h"
#include "util/atomic.hpp"

// Lightweight semaphore helper class
class semaphore_base
{
	// Semaphore value
	atomic_t<s32> m_value;

	void imp_wait();

	void imp_post(s32 _old);

protected:
	explicit constexpr semaphore_base(s32 value)
		: m_value{value}
	{
	}

	void wait()
	{
		// Load value
		const s32 value = m_value.load();

		// Conditional decrement
		if (value <= 0 || !m_value.compare_and_swap_test(value, value - 1)) [[unlikely]]
		{
			imp_wait();
		}
	}

	bool try_wait()
	{
		return m_value.try_dec(0);
	}

	void post(s32 _max)
	{
		// Unconditional increment
		const s32 value = m_value.fetch_add(1);

		if (value < 0 || value >= _max) [[unlikely]]
		{
			imp_post(value);
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
		return value < 0 ? 0 : value;
	}
};

// Lightweight semaphore template (default arguments define binary semaphore and Def == Max)
template <s32 Max = 1, s32 Def = Max>
class semaphore final : public semaphore_base
{
	static_assert(Max >= 0, "semaphore<>: Max is out of bounds");
	static_assert(Def >= 0, "semaphore<>: Def is out of bounds");
	static_assert(Def <= Max, "semaphore<>: Def is too big");

	using base = semaphore_base;

public:
	// Default constructor (recommended)
	constexpr semaphore()
		: base(Def)
	{
	}

	// Explicit value constructor (not recommended)
	explicit constexpr semaphore(s32 value)
		: base(value)
	{
	}

	// Obtain a semaphore
	void lock()
	{
		return base::wait();
	}

	// Try to obtain a semaphore
	bool try_lock()
	{
		return base::try_wait();
	}

	// Return a semaphore
	void unlock()
	{
		return base::post(Max);
	}

	// Try to return a semaphore
	bool try_unlock()
	{
		return base::try_post(Max);
	}

	// Get max semaphore value
	static constexpr s32 size()
	{
		return Max;
	}
};
