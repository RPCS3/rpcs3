#include "cond.h"
#include "sync.h"
#include "lockless.h"

#include <limits.h>

#ifndef _WIN32
#include <thread>
#endif

bool cond_variable::imp_wait(u32 _old, u64 _timeout) noexcept
{
	verify("cond_variable overflow" HERE), (_old & 0xffff) != 0xffff; // Very unlikely: it requires 65535 distinct threads to wait simultaneously

	return balanced_wait_until(m_value, _timeout, [&](u32& value, auto... ret) -> int
	{
		if (value >> 16)
		{
			// Success
			value -= 0x10001;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			// Retire
			value -= 1;
			return -1;
		}

		return 0;
	});

#ifdef _WIN32
	if (_old >= 0x10000 && !OptWaitOnAddress && m_value)
	{
		// Workaround possibly stolen signal
		imp_wake(1);
	}
#endif
}

void cond_variable::imp_wake(u32 _count) noexcept
{
	// TODO (notify_one)
	balanced_awaken<true>(m_value, m_value.atomic_op([&](u32& value) -> u32
	{
		// Subtract already signaled number from total amount of waiters
		const u32 can_sig = (value & 0xffff) - (value >> 16);
		const u32 num_sig = std::min<u32>(can_sig, _count);

		value += num_sig << 16;
		return num_sig;
	}));
}

bool notifier::imp_try_lock(u32 count)
{
	return m_counter.atomic_op([&](u32& value)
	{
		if ((value % (max_readers + 1)) + count <= max_readers)
		{
			value += count;
			return true;
		}

		return false;
	});
}

void notifier::imp_unlock(u32 count)
{
	const u32 counter = m_counter.sub_fetch(count);

	if (UNLIKELY(counter % (max_readers + 1)))
	{
		return;
	}

	if (counter)
	{
		const u32 _old = m_counter.atomic_op([](u32& value) -> u32
		{
			if (value % (max_readers + 1))
			{
				return 0;
			}

			return std::exchange(value, 0) / (max_readers + 1);
		});

		const u32 wc = m_cond.m_value;

		if (_old && wc)
		{
			m_cond.imp_wake(_old > wc ? wc : _old);
		}
	}
}

u32 notifier::imp_notify(u32 count)
{
	return m_counter.atomic_op([&](u32& value) -> u32
	{
		if (const u32 add = value % (max_readers + 1))
		{
			// Mutex is locked
			const u32 result = add > count ? count : add;
			value += result * (max_readers + 1);
			return result;
		}
		else
		{
			// Mutex is unlocked
			value = 0;
			return count;
		}
	});
}

bool notifier::wait(u64 usec_timeout)
{
	const u32 _old = m_cond.m_value.fetch_add(1);

	if (max_readers < m_counter.fetch_op([](u32& value)
	{
		if (value > max_readers)
		{
			value -= max_readers;
		}

		value -= 1;
	}))
	{
		// Return without waiting
		m_cond.imp_wait(_old, 0);
		return true;
	}

	const bool res = m_cond.imp_wait(_old, usec_timeout);

	while (!try_lock_shared())
	{
		// TODO
		busy_wait();
	}

	return res;
}

bool unique_cond::imp_wait(u64 _timeout) noexcept
{
	// State transition: c_sig -> c_lock \ c_lock -> c_wait
	const u32 _old = m_value.fetch_sub(1);
	if (LIKELY(_old == c_sig))
		return true;

	return balanced_wait_until(m_value, _timeout, [&](u32& value, auto... ret) -> int
	{
		if (value == c_sig)
		{
			value = c_lock;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			value = c_lock;
			return -1;
		}

		return 0;
	});
}

void unique_cond::imp_notify() noexcept
{
	auto [old, ok] = m_value.fetch_op([](u32& v)
	{
		if (UNLIKELY(v > 0 && v < c_sig))
		{
			v = c_sig;
			return true;
		}

		return false;
	});

	verify(HERE), old <= c_sig;

	if (LIKELY(!ok || old == c_lock))
	{
		return;
	}

	balanced_awaken(m_value, 1);
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

bool lf_queue_base::wait(u64 _timeout)
{
	auto _old = m_head.compare_and_swap(0, 1);

	if (_old)
	{
		verify("lf_queue concurrent wait" HERE), _old != 1;
		return true;
	}

	return balanced_wait_until(m_head, _timeout, [](std::uintptr_t& head, auto... ret) -> int
	{
		if (head != 1)
		{
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			head = 0;
			return -1;
		}

		return 0;
	});
}

void lf_queue_base::imp_notify()
{
	balanced_awaken(m_head, 1);
}
