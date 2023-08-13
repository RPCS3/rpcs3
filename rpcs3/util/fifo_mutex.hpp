#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"

// Mutex that tries to maintain the order of acquisition
class fifo_mutex
{
	// Low 16 bits are incremented on acquisition, high 16 bits are incremented on release
	atomic_t<u32> m_value{0};

public:
	constexpr fifo_mutex() noexcept = default;

	void lock() noexcept
	{
		// clang-format off
		const u32 val = m_value.fetch_op([](u32& val)
		{
			val = (val & 0xffff0000) | ((val + 1) & 0xffff);
		});
		// clang-format on

		if (val >> 16 != (val & 0xffff)) [[unlikely]]
		{
			// TODO: implement busy waiting along with moving to cpp file
			m_value.wait((val & 0xffff0000) | ((val + 1) & 0xffff));
		}
	}

	bool try_lock() noexcept
	{
		const u32 val = m_value.load();

		if (val >> 16 == (val & 0xffff))
		{
			if (m_value.compare_and_swap(val, ((val + 1) & 0xffff) | (val & 0xffff0000)))
			{
				return true;
			}
		}

		return false;
	}

	void unlock() noexcept
	{
		const u32 val = m_value.add_fetch(0x10000);

		if (val >> 16 != (val & 0xffff))
		{
			m_value.notify_one();
		}
	}

	bool is_free() const noexcept
	{
		const u32 val = m_value.load();

		return (val >> 16) == (val & 0xffff);
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
