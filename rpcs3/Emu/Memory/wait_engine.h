#pragma once

#include <mutex>
#include <condition_variable>
#include <functional>

class named_thread;

namespace vm
{
	using mutex_t = std::mutex;
	using cond_t = std::condition_variable;

	struct waiter
	{
		u32 addr;
		u32 mask;
		mutex_t* mutex;
		cond_t* cond;

		std::function<bool()> pred;

		~waiter();

		bool try_notify();
	};

	class waiter_lock
	{
		waiter m_waiter;
		std::unique_lock<mutex_t> m_lock;

	public:
		waiter_lock(u32 addr, u32 size);

		waiter* operator ->()
		{
			return &m_waiter;
		}

		void wait();

		~waiter_lock();
	};

	// Wait until pred() returns true, addr must be aligned to size which must be a power of 2, pred() may be called by any thread
	template<typename F, typename... Args>
	auto wait_op(u32 addr, u32 size, F&& pred, Args&&... args) -> decltype(static_cast<void>(pred(args...)))
	{
		// Return immediately if condition passed (optimistic case)
		if (pred(args...)) return;

		waiter_lock lock(addr, size);

		// Initialize predicate
		lock->pred = WRAP_EXPR(pred(args...));

		lock.wait();
	}

	// Notify waiters on specific addr, addr must be aligned to size which must be a power of 2
	void notify_at(u32 addr, u32 size);
}
