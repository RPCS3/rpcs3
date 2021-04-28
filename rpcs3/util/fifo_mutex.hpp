#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Mutex that tries to maintain the order of acquisition
class fifo_mutex
{
	// Low 8 bits are incremented on acquisition, high 8 bits are incremented on release
	atomic_t<u16> m_value{0};

public:
	constexpr fifo_mutex() noexcept = default;

	void lock() noexcept
	{
		const u16 val = m_value.fetch_op([](u16& val)
		{
			val = (val & 0xff00) | ((val + 1) & 0xff);
		});

		if (val >> 8 != (val & 0xff)) [[unlikely]]
		{
			// TODO: implement busy waiting along with moving to cpp file
			m_value.wait<atomic_wait::op_ne>(((val + 1) & 0xff) << 8, 0xff00);
		}
	}

	bool try_lock() noexcept
	{
		const u16 val = m_value.load();

		if (val >> 8 == (val & 0xff))
		{
			if (m_value.compare_and_swap(val, ((val + 1) & 0xff) | (val & 0xff00)))
			{
				return true;
			}
		}

		return false;
	}

	void unlock() noexcept
	{
		const u16 val = m_value.add_fetch(0x100);

		if (val >> 8 != (val & 0xff))
		{
			m_value.notify_one();
		}
	}

	bool is_free() const noexcept
	{
		const u16 val = m_value.load();

		return (val >> 8) == (val & 0xff);
	}

	void lock_unlock() noexcept
	{
		if (!is_free())
		{
			lock();
			unlock();
		}
	}
};
