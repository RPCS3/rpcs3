#include "stdafx.h"
#include "SharedMutex.h"

static const u32 MAX_READERS = 0x7fffffff; // 2^31-1

inline bool shared_mutex_t::try_lock_shared()
{
	return m_info.atomic_op([](ownership_info_t& info) -> bool
	{
		if (info.readers < MAX_READERS && !info.writers && !info.waiting_readers && !info.waiting_writers)
		{
			info.readers++;
			return true;
		}

		return false;
	});
}

void shared_mutex_t::lock_shared()
{
	if (!try_lock_shared())
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_wrcv.wait(lock, WRAP_EXPR(m_info.atomic_op([](ownership_info_t& info) -> bool
		{
			if (info.waiting_readers < UINT16_MAX)
			{
				info.waiting_readers++;
				return true;
			}

			return false;
		})));

		m_rcv.wait(lock, WRAP_EXPR(m_info.atomic_op([](ownership_info_t& info) -> bool
		{
			if (!info.writers && !info.waiting_writers && info.readers < MAX_READERS)
			{
				info.readers++;
				return true;
			}

			return false;
		})));

		const auto info = m_info.atomic_op([](ownership_info_t& info)
		{
			if (!info.waiting_readers--)
			{
				throw EXCEPTION("Invalid value");
			}
		});

		if (info.waiting_readers == UINT16_MAX)
		{
			m_wrcv.notify_one();
		}
	}
}

void shared_mutex_t::unlock_shared()
{
	const auto info = m_info.atomic_op([](ownership_info_t& info)
	{
		if (!info.readers--)
		{
			throw EXCEPTION("Not locked");
		}
	});

	const bool notify_writers = info.readers == 1 && info.writers;
	const bool notify_readers = info.readers == UINT32_MAX && info.waiting_readers;

	if (notify_writers || notify_readers)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (notify_writers) m_wcv.notify_one();
		if (notify_readers) m_rcv.notify_one();
	}
}

inline bool shared_mutex_t::try_lock()
{
	return m_info.compare_and_swap_test({ 0, 0, 0, 0 }, { 0, 1, 0, 0 });
}

void shared_mutex_t::lock()
{
	if (!try_lock())
	{
		std::unique_lock<std::mutex> lock(m_mutex);

		m_wwcv.wait(lock, WRAP_EXPR(m_info.atomic_op([](ownership_info_t& info) -> bool
		{
			if (info.waiting_writers < UINT16_MAX)
			{
				info.waiting_writers++;
				return true;
			}

			return false;
		})));

		m_wcv.wait(lock, WRAP_EXPR(m_info.atomic_op([](ownership_info_t& info) -> bool
		{
			if (!info.writers)
			{
				info.writers++;
				return true;
			}

			return false;
		})));

		m_wcv.wait(lock, WRAP_EXPR(m_info.load().readers == 0));

		const auto info = m_info.atomic_op([](ownership_info_t& info)
		{
			if (!info.waiting_writers--)
			{
				throw EXCEPTION("Invalid value");
			}
		});

		if (info.waiting_writers == UINT16_MAX)
		{
			m_wwcv.notify_one();
		}
	}
}

void shared_mutex_t::unlock()
{
	const auto info = m_info.atomic_op([](ownership_info_t& info)
	{
		if (!info.writers--)
		{
			throw EXCEPTION("Not locked");
		}
	});

	if (info.waiting_writers || info.waiting_readers)
	{
		std::lock_guard<std::mutex> lock(m_mutex);

		if (info.waiting_writers) m_wcv.notify_one();
		else if (info.waiting_readers) m_rcv.notify_all();
	}
}
