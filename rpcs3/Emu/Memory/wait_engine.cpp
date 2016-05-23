#include "stdafx.h"
#include "Emu/System.h"
#include "vm.h"
#include "wait_engine.h"

#include "Utilities/Thread.h"
#include "Utilities/SharedMutex.h"

#include <unordered_set>

namespace vm
{
	static shared_mutex s_mutex;

	static std::unordered_set<waiter_base*, pointer_hash<waiter_base>> s_waiters(256);

	void waiter_base::initialize(u32 addr, u32 size)
	{
		EXPECTS(addr && (size & (~size + 1)) == size && (addr & (size - 1)) == 0);

		this->addr = addr;
		this->mask = ~(size - 1);
		this->thread = thread_ctrl::get_current();

		struct waiter final
		{
			waiter_base* m_ptr;
			thread_ctrl* m_thread;

			waiter(waiter_base* ptr)
				: m_ptr(ptr)
				, m_thread(ptr->thread)
			{
				// Initialize waiter
				writer_lock{s_mutex}, s_waiters.emplace(m_ptr);

				m_thread->lock();
			}

			~waiter()
			{
				// Reset thread
				atomic_storage<thread_ctrl*>::store(m_ptr->thread, nullptr);
				m_thread->unlock();

				// Remove waiter
				writer_lock{s_mutex}, s_waiters.erase(m_ptr);
			}
		};

		// Wait until thread == nullptr
		waiter{this}, thread_ctrl::wait(WRAP_EXPR(!thread || test()));
	}

	bool waiter_base::try_notify()
	{
		const auto _t = atomic_storage<thread_ctrl*>::load(thread);

		if (UNLIKELY(!_t))
		{
			// Return if thread not found
			return false;
		}

		// Lock the thread
		_t->lock();

		try
		{
			// Test predicate
			if (UNLIKELY(!thread || !test()))
			{
				_t->unlock();
				return false;
			}
		}
		catch (...)
		{
			// Capture any exception thrown by the predicate
			_t->set_exception(std::current_exception());
		}

		// Signal the thread with nullptr
		atomic_storage<thread_ctrl*>::store(thread, nullptr);
		_t->unlock();
		_t->notify();
		return true;
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

	// Return amount of threads which are not notified
	static std::size_t notify_all()
	{
		reader_lock lock(s_mutex);

		std::size_t signaled = 0;

		for (const auto _w : s_waiters)
		{
			if (_w->try_notify())
			{
				signaled++;
			}
		}

		return s_waiters.size() - signaled;
	}

	void start()
	{
		thread_ctrl::spawn("vm::wait", []()
		{
			while (!Emu.IsStopped())
			{
				// Poll waiters periodically (TODO)
				while (notify_all() && !Emu.IsPaused() && !Emu.IsStopped())
				{
					thread_ctrl::sleep(50);
				}

				thread_ctrl::sleep(1000);
			}
		});
	}
}
