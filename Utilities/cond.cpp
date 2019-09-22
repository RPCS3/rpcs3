#include "cond.h"
#include "sync.h"
#include "lockless.h"

#include <climits>

// use constants, increase signal space

void cond_variable::imp_wait(u32 _old, u64 _timeout) noexcept
{
	// Not supposed to fail
	verify(HERE), _old;

	// Wait with timeout
	m_value.wait(_old, atomic_wait_timeout{_timeout > max_timeout ? UINT64_MAX : _timeout * 1000});

	// Cleanup
	m_value.atomic_op([](u32& value)
	{
		// Remove waiter (c_waiter_mask)
		value -= 1;

		if ((value & c_waiter_mask) == 0)
		{
			// Last waiter removed, clean signals
			value = 0;
		}
	});
}

void cond_variable::imp_wake(u32 _count) noexcept
{
	const auto [_old, ok] = m_value.fetch_op([](u32& value)
	{
		if (!value || (value & c_signal_mask) == c_signal_mask)
		{
			return false;
		}

		// Add signal
		value += c_signal_mask & -c_signal_mask;
		return true;
	});

	if (!ok || !_count)
	{
		return;
	}

	if (_count > 1 || ((_old + (c_signal_mask & -c_signal_mask)) & c_signal_mask) == c_signal_mask)
	{
		// Resort to notify_all if signal count reached max
		m_value.notify_all();
	}
	else
	{
		m_value.notify_one();
	}
}

bool shared_cond::imp_wait(u32 slot, u64 _timeout) noexcept
{
	if (slot >= 32)
	{
		// Invalid argument, assume notified
		return true;
	}

	const u64 wait_bit = c_wait << slot;
	const u64 lock_bit = c_lock << slot;

	// Change state from c_lock to c_wait
	const u64 old_ = m_cvx32.fetch_op([=](u64& cvx32)
	{
		if (cvx32 & wait_bit)
		{
			// c_lock -> c_wait
			cvx32 &= ~(lock_bit & ~wait_bit);
		}
		else
		{
			// c_sig -> c_lock
			cvx32 |= lock_bit;
		}
	});

	if ((old_ & wait_bit) == 0)
	{
		// Already signaled, return without waiting
		return true;
	}

	return balanced_wait_until(m_cvx32, _timeout, [&](u64& cvx32, auto... ret) -> int
	{
		if ((cvx32 & wait_bit) == 0)
		{
			// c_sig -> c_lock
			cvx32 |= lock_bit;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			// Retire
			cvx32 |= lock_bit;
			return -1;
		}

		return 0;
	});
}

void shared_cond::imp_notify() noexcept
{
	auto [old, ok] = m_cvx32.fetch_op([](u64& cvx32)
	{
		if (const u64 sig_mask = cvx32 & 0xffffffff)
		{
			cvx32 &= 0xffffffffull << 32;
			cvx32 |= sig_mask << 32;
			return true;
		}

		return false;
	});

	// Determine if some waiters need a syscall notification
	const u64 wait_mask = old & (~old >> 32);

	if (UNLIKELY(!ok || !wait_mask))
	{
		return;
	}

	balanced_awaken<true>(m_cvx32, utils::popcnt32(wait_mask));
}
