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

bool cond_one::imp_wait(u64 _timeout) noexcept
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

void cond_one::imp_notify() noexcept
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

bool cond_x16::imp_wait(u32 slot, u64 _timeout) noexcept
{
	const u32 wait_bit = c_wait << slot;
	const u32 lock_bit = c_lock << slot;

	// Change state from c_lock to c_wait
	const u32 old_ = m_cvx16.fetch_op([=](u32& cvx16)
	{
		if (cvx16 & wait_bit)
		{
			// c_sig -> c_lock
			cvx16 &= ~wait_bit;
		}
		else
		{
			cvx16 |= wait_bit;
			cvx16 &= ~lock_bit;
		}
	});

	if (old_ & wait_bit)
	{
		// Already signaled, return without waiting
		return true;
	}

	return balanced_wait_until(m_cvx16, _timeout, [&](u32& cvx16, auto... ret) -> int
	{
		if (cvx16 & lock_bit)
		{
			// c_sig -> c_lock
			cvx16 &= ~wait_bit;
			return +1;
		}

		if constexpr (sizeof...(ret))
		{
			// Retire
			cvx16 |= lock_bit;
			cvx16 &= ~wait_bit;
			return -1;
		}

		return 0;
	});
}

void cond_x16::imp_notify() noexcept
{
	auto [old, ok] = m_cvx16.fetch_op([](u32& v)
	{
		const u32 lock_mask = v >> 16;
		const u32 wait_mask = v & 0xffff;

		if (const u32 sig_mask = lock_mask ^ wait_mask)
		{
			v |= sig_mask | sig_mask << 16;
			return true;
		}

		return false;
	});

	// Determine if some waiters need a syscall notification
	const u32 wait_mask = old & (~old >> 16);

	if (UNLIKELY(!ok || !wait_mask))
	{
		return;
	}

	balanced_awaken<true>(m_cvx16, utils::popcnt16(wait_mask));
}

bool lf_queue_base::wait(u64 _timeout)
{
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
