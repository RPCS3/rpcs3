#include "stdafx.h"
#include "Emu/System.h"
#include "vm.h"
#include "wait_engine.h"

#include "Utilities/Thread.h"
#include "Utilities/SharedMutex.h"

extern std::condition_variable& get_current_thread_cv();
extern std::mutex& get_current_thread_mutex();

namespace vm
{
	static shared_mutex s_mutex;

	static std::unordered_set<waiter*> s_waiters(256);

	bool waiter::try_notify()
	{
		{
			std::lock_guard<mutex_t> lock(*mutex);

			try
			{
				// Test predicate
				if (!pred || !pred())
				{
					return false;
				}

				// Clear predicate
				pred = nullptr;
			}
			catch (...)
			{
				// Capture any exception possibly thrown by predicate
				pred = [exception = std::current_exception()]() -> bool
				{
					// New predicate will throw the captured exception from the original thread
					std::rethrow_exception(exception);
				};
			}

			// Set addr and mask to invalid values to prevent further polling
			addr = 0;
			mask = ~0;
		}
		
		// Signal thread
		cond->notify_one();
		return true;
	}

	waiter::~waiter()
	{
	}

	waiter_lock::waiter_lock(u32 addr, u32 size)
		: m_lock(get_current_thread_mutex(), std::defer_lock)
	{
		Expects(addr && (size & (~size + 1)) == size && (addr & (size - 1)) == 0);

		m_waiter.mutex = m_lock.mutex();
		m_waiter.cond = &get_current_thread_cv();
		m_waiter.addr = addr;
		m_waiter.mask = ~(size - 1);

		{
			writer_lock lock(s_mutex);

			s_waiters.emplace(&m_waiter);
		}
		
		m_lock.lock();
	}

	void waiter_lock::wait()
	{
		// If another thread successfully called pred(), it must be set to null
		while (m_waiter.pred)
		{
			// If pred() called by another thread threw an exception, it'll be rethrown
			if (m_waiter.pred())
			{
				return;
			}

			CHECK_EMU_STATUS;

			m_waiter.cond->wait(m_lock);
		}
	}

	waiter_lock::~waiter_lock()
	{
		if (m_lock) m_lock.unlock();

		writer_lock lock(s_mutex);

		s_waiters.erase(&m_waiter);
	}

	void notify_at(u32 addr, u32 size)
	{
		reader_lock lock(s_mutex);

		for (const auto _w : s_waiters)
		{
			// Check address range overlapping using masks generated from size (power of 2)
			if (((_w->addr ^ addr) & (_w->mask & ~(size - 1))) == 0)
			{
				_w->try_notify();
			}
		}
	}

	static bool notify_all()
	{
		reader_lock lock(s_mutex);

		std::size_t waiters = 0;
		std::size_t signaled = 0;

		for (const auto _w : s_waiters)
		{
			if (_w->addr)
			{
				waiters++;

				if (_w->try_notify())
				{
					signaled++;
				}
			}
		}

		// return true if waiter list is empty or all available waiters were signaled
		return waiters == signaled;
	}

	void start()
	{
		// start notification thread
		thread_ctrl::spawn("vm::start thread", []()
		{
			while (!Emu.IsStopped())
			{
				// poll waiters periodically (TODO)
				while (!notify_all() && !Emu.IsPaused())
				{
					std::this_thread::yield();
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(1));
			}
		});
	}
}
