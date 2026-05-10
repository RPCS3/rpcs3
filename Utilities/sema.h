#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Lightweight semaphore helper class
class semaphore_base
{
	// Semaphore value
	atomic_t<u32> m_value;

	enum : u32
	{
		c_value = 1u << 0,
		c_value_mask = +c_value * 0xffff,
		c_waiter = 1u << 16,
		c_waiter_mask = +c_waiter * 0xffff,
	};

	void imp_wait();

	void imp_post(u32 _old);

protected:
	explicit constexpr semaphore_base(u32 value) noexcept
		: m_value{value}
	{
	}

	void wait()
	{
		// Load value
		const u32 value = m_value.load();

		// Conditional decrement
		if ((value & c_value_mask) == 0 || !m_value.compare_and_swap_test(value, value - c_value)) [[unlikely]]
		{
			imp_wait();
		}
	}

	bool try_wait()
	{
		return m_value.fetch_op([](u32& value)
		{
			if (value & c_value_mask)
			{
				value -= c_value;
				return true;
			}

			return false;
		}).second;
	}

	void post(u32 _max)
	{
		// Unconditional increment
		const u32 value = m_value.fetch_add(c_value);

		if (value & c_waiter_mask || (value & c_value_mask) >= std::min<u32>(c_value_mask, _max)) [[unlikely]]
		{
			imp_post(value);
		}
	}

	bool try_post(u32 _max);

public:
	// Get current semaphore value
	s32 get() const
	{
		// Load value
		const u32 raw_value = m_value;
		const u32 waiters = (raw_value & c_waiter_mask) / c_waiter;
		const u32 value = (raw_value & c_value_mask) / c_value;

		// Return only positive value
		return static_cast<s32>(waiters >= value ? 0 : value - waiters);
	}
};

// Lightweight semaphore template (default arguments define binary semaphore and Def == Max)
template <s16 Max = 1, s16 Def = Max>
class semaphore final : public semaphore_base
{
	static_assert(Max >= 0, "semaphore<>: Max is out of bounds");
	static_assert(Def >= 0, "semaphore<>: Def is out of bounds");
	static_assert(Def <= Max, "semaphore<>: Def is too big");

	using base = semaphore_base;

public:
	// Default constructor (recommended)
	constexpr semaphore() noexcept
		: base(Def)
	{
	}

	// Explicit value constructor (not recommended)
	explicit constexpr semaphore(s16 value) noexcept
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
