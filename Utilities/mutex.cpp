#include "mutex.h"
#include "sync.h"

#ifdef _WIN32
thread_local const u32 owned_mutex::g_tid = GetCurrentThreadId();
#elif __linux__
#include <sys/types.h>
thread_local const u32 owned_mutex::g_tid = syscall(SYS_gettid) + 1;
static_assert(sizeof(pid_t) == sizeof(u32), "Unexpected sizeof(pid_t)");
#else

#include <vector>

thread_local const u32 owned_mutex::g_tid = []() -> u32
{
	static std::mutex g_tid_mutex;
	static std::vector<bool> g_tid_map(1);

	thread_local const struct tid_alloc
	{
		u32 id = 0;

		tid_alloc()
		{
			std::lock_guard<std::mutex> lock(g_tid_mutex);

			// Allocate
			while (++id < g_tid_map.size())
			{
				if (!g_tid_map[id])
				{
					g_tid_map[id] = true;
					return;
				}
			}

			g_tid_map.push_back(true);
		}

		~tid_alloc()
		{
			std::lock_guard<std::mutex> lock(g_tid_mutex);

			// Erase
			g_tid_map[id] = false;
		}
	} g_tid;

	return g_tid.id;
}();
#endif

void shared_mutex::imp_lock_shared(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old <= c_max;

	// 1) Wait as a writer, notify the next writer
	// 2) Wait as a reader, until the value > 0
	lock();
	_old = m_value.fetch_add(c_one - c_min);

	if (_old)
	{
		imp_unlock(_old);
	}

#ifdef _WIN32
	if (_old + c_one - c_min < 0)
	{
		NtWaitForKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
	}
#else
	for (s64 value = m_value; value < 0; value = m_value)
	{
		if (futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAIT_PRIVATE, value >> 32, nullptr, nullptr, 0) == -1)
		{
			verify(HERE), errno == EAGAIN;
		}
	}
#endif
}

void shared_mutex::imp_unlock_shared(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old + c_min <= c_max;

	// Check reader count, notify the writer if necessary (set c_sig)
	if ((_old + c_min) % c_one == 0) // TODO
	{
		verify(HERE), !atomic_storage<s64>::bts(m_value.raw(), 0);
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
		verify(HERE), futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) >= 0;
#endif
	}
}

void shared_mutex::imp_lock(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old <= c_max;

#ifdef _WIN32
	NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
	verify(HERE), atomic_storage<s64>::btr(m_value.raw(), 0);
#else
	for (s64 value = m_value; (m_value & c_sig) == 0 || !atomic_storage<s64>::btr(m_value.raw(), 0); value = m_value)
	{
		if (futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAIT_PRIVATE, value, nullptr, nullptr, 0) == -1)
		{
			verify(HERE), errno == EAGAIN;
		}
	}
#endif
}

void shared_mutex::imp_unlock(s64 _old)
{
	verify("shared_mutex overflow" HERE), _old + c_one <= c_max;

	// 1) Notify the next writer if necessary (set c_sig)
	// 2) Notify all readers otherwise if necessary
	if (_old + c_one <= 0)
	{
		verify(HERE), !atomic_storage<s64>::bts(m_value.raw(), 0);
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
		verify(HERE), futex((int*)&m_value.raw() + IS_BE_MACHINE, FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0) >= 0;
#endif
	}
	else if (s64 count = -_old / c_min)
	{
#ifdef _WIN32
		while (count--)
		{
			NtReleaseKeyedEvent(nullptr, (int*)&m_value + 1, false, nullptr);
		}
#else
		verify(HERE), futex((int*)&m_value.raw() + IS_LE_MACHINE, FUTEX_WAKE_PRIVATE, INT_MAX, nullptr, nullptr, 0) >= 0;
#endif
	}
}

void shared_mutex::imp_lock_upgrade()
{
	unlock_shared();
	lock();
}

void shared_mutex::imp_lock_degrade()
{
	unlock();
	lock_shared();
}

bool shared_mutex::try_lock_shared()
{
	// Conditional decrement
	return m_value.fetch_op([](s64& value) { if (value >= c_min) value -= c_min; }) >= c_min;
}

bool shared_mutex::try_lock()
{
	// Conditional decrement (TODO: obtain c_sig)
	return m_value.compare_and_swap_test(c_one, 0);
}

bool shared_mutex::try_lock_upgrade()
{
	// TODO
	return m_value.compare_and_swap_test(c_one - c_min, 0);
}

bool shared_mutex::try_lock_degrade()
{
	// TODO
	return m_value.compare_and_swap_test(0, c_one - c_min);
}

bool owned_mutex::lock() noexcept
{
	if (m_value && m_owner == g_tid)
	{
		return false;
	}

#ifdef _WIN32
	if (m_value++)
	{
		NtWaitForKeyedEvent(nullptr, &m_value, false, nullptr);
	}

	m_owner.store(g_tid);
#else
	u32 _last = ++m_value;

	if (_last == 1 && m_owner.compare_and_swap_test(0, g_tid))
	{
		return true;
	}

	while (!m_owner.compare_and_swap_test(0, g_tid))
	{
		if (futex((int*)&m_value.raw(), FUTEX_WAIT_PRIVATE, _last, nullptr, nullptr, 0))
		{
			_last = m_value.load();
		}
	}
#endif

	return true;
}

bool owned_mutex::try_lock() noexcept
{
	if (m_value || !m_value.compare_and_swap_test(0, 1))
	{
		return false;
	}

	m_owner.store(g_tid);
	return true;
}

bool owned_mutex::unlock() noexcept
{
	if (UNLIKELY(m_owner != g_tid))
	{
		return false;
	}

	m_owner.store(0);

	if (--m_value)
	{
#ifdef _WIN32
		NtReleaseKeyedEvent(nullptr, &m_value, false, nullptr);
#else
		futex((int*)&m_value.raw(), FUTEX_WAKE_PRIVATE, 1, nullptr, nullptr, 0);
#endif
	}

	return true;
}
