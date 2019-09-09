#include "cond.h"
#include "sync.h"
#include "lockless.h"

#include <limits.h>

#ifndef _WIN32
#include <thread>
#endif

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
		value -= c_waiter_mask & -c_waiter_mask;

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

void shared_cond::wait_all() noexcept
{
	// Try to acquire waiting state without locking but only if there are other locks
	const auto [old_, result] = m_cvx32.fetch_op([](u64& cvx32) -> u64
	{
		// Check waiting alone
		if ((cvx32 & 0xffffffff) == 0)
		{
			return 0;
		}

		// Combine used bits and invert to find least significant bit unused
		const u32 slot = utils::cnttz64(~((cvx32 & 0xffffffff) | (cvx32 >> 32)), true);

		// Set waiting bit (does nothing if all slots are used)
		cvx32 |= (1ull << slot) & 0xffffffff;
		return 1ull << slot;
	});

	if (!result)
	{
		return;
	}

	if (result > 0xffffffffu)
	{
		// All slots are used, fallback to spin wait
		while (m_cvx32 & 0xffffffff)
		{
			busy_wait();
		}

		return;
	}

	const u64 wait_bit = result;
	const u64 lock_bit = wait_bit | (wait_bit << 32);

	balanced_wait_until(m_cvx32, -1, [&](u64& cvx32, auto... ret) -> int
	{
		if ((cvx32 & wait_bit) == 0)
		{
			// Remove signal and unlock at once
			cvx32 &= ~lock_bit;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			cvx32 &= ~lock_bit;
			return -1;
		}

		return 0;
	});
}

bool shared_cond::wait_all(shared_cond::shared_lock& lock) noexcept
{
	AUDIT(lock.m_this == this);

	if (lock.m_slot >= 32)
	{
		// Invalid argument, assume notified
		return true;
	}

	const u64 wait_bit = c_wait << lock.m_slot;
	const u64 lock_bit = c_lock << lock.m_slot;

	// Try to acquire waiting state only if there are other locks
	const auto [old_, not_alone] = m_cvx32.fetch_op([&](u64& cvx32)
	{
		// Check locking alone
		if (((cvx32 >> 32) & cvx32) == (lock_bit >> 32))
		{
			return false;
		}

		// c_lock -> c_wait, c_sig -> unlock
		cvx32 &= ~(lock_bit & ~wait_bit);
		return true;
	});

	if (!not_alone)
	{
		return false;
	}
	else
	{
		// Set invalid slot to acknowledge unlocking
		lock.m_slot = 33;
	}

	if ((old_ & wait_bit) == 0)
	{
		// Already signaled, return without waiting
		return true;
	}

	balanced_wait_until(m_cvx32, -1, [&](u64& cvx32, auto... ret) -> int
	{
		if ((cvx32 & wait_bit) == 0)
		{
			// Remove signal and unlock at once
			cvx32 &= ~lock_bit;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			cvx32 &= ~lock_bit;
			return -1;
		}

		return 0;
	});

	return true;
}

bool shared_cond::notify_all(shared_cond::shared_lock& lock) noexcept
{
	AUDIT(lock.m_this == this);

	if (lock.m_slot >= 32)
	{
		// Invalid argument
		return false;
	}

	const u64 slot_mask = c_sig << lock.m_slot;

	auto [old, ok] = m_cvx32.fetch_op([&](u64& cvx32)
	{
		if (((cvx32 << 32) & cvx32) != slot_mask)
		{
			return false;
		}

		if (const u64 sig_mask = cvx32 & 0xffffffff)
		{
			cvx32 &= (0xffffffffull << 32) & ~slot_mask;
			cvx32 |= (sig_mask << 32) & ~slot_mask;
			return true;
		}

		return false;
	});

	if (!ok)
	{
		// Not an exclusive reader
		return false;
	}

	// Set invalid slot to acknowledge unlocking
	lock.m_slot = 34;

	// Determine if some waiters need a syscall notification
	const u64 wait_mask = old & (~old >> 32);

	if (UNLIKELY(!wait_mask))
	{
		return true;
	}

	balanced_awaken<true>(m_cvx32, utils::popcnt32(wait_mask));
	return true;
}
