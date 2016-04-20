#pragma once

#include <string>
#include <memory>
#include <thread>

#include "Platform.h"
#include "Atomic.h"

// Will report exception and call std::abort() if put in catch(...)
[[noreturn]] void catch_all_exceptions();

// Simple list of void() functors
class task_stack
{
	struct task_base
	{
		std::unique_ptr<task_base> next;

		virtual ~task_base() = default;

		virtual void exec()
		{
			if (next)
			{
				next->exec();
			}
		}
	};

	std::unique_ptr<task_base> m_stack;

public:
	template<typename F>
	void push(F&& func)
	{
		struct task_t : task_base
		{
			std::remove_reference_t<F> func;

			task_t(F&& func)
				: func(std::forward<F>(func))
			{
			}

			void exec() override
			{
				func();
				task_base::exec();
			}
		};

		auto _top = new task_t(std::forward<F>(func));
		auto _next = m_stack.release();
		m_stack.reset(_top);
#ifndef _MSC_VER
		_top->next.reset(_next);
#else
		auto& next = _top->next;
		next.release();
		next.reset(_next);
#endif
	}

	void reset()
	{
		m_stack.reset();
	}

	void exec() const
	{
		if (m_stack)
		{
			m_stack->exec();
		}
	}
};

// Thread control class
class thread_ctrl final
{
	struct internal;

	static thread_local thread_ctrl* g_tls_this_thread;

	// Thread handle
	std::thread m_thread;

	// Thread join contention counter
	atomic_t<uint> m_joining{};

	// Fixed name
	std::string m_name;

	// Thread internals
	mutable atomic_t<internal*> m_data{};

	// Called at the thread start
	void initialize();

	// Set std::current_exception
	void set_exception() noexcept;

	// Called at the thread end
	void finalize() noexcept;

	// Get atexit function
	task_stack& get_atexit() const;

public:
	template<typename N>
	thread_ctrl(N&& name)
		: m_name(std::forward<N>(name))
	{
	}

	// Disable copy/move constructors and operators
	thread_ctrl(const thread_ctrl&) = delete;

	~thread_ctrl();

	// Get thread name
	const std::string& get_name() const
	{
		return m_name;
	}

	// Initialize internal data
	void initialize_once() const;

	// Get thread result (may throw, simultaneous joining allowed)
	void join();

	// Lock, unlock, notify the thread (required if the condition changed locklessly)
	void lock_notify() const;

	// Notify the thread, beware the condition change
	void notify() const;

	//
	internal* get_data() const;

	// Get current thread (may be nullptr)
	static const thread_ctrl* get_current()
	{
		return g_tls_this_thread;
	}

	// Register function at thread exit (for the current thread)
	template<typename F>
	static inline void at_exit(F&& func)
	{
		return g_tls_this_thread->get_atexit().push(std::forward<F>(func));
	}

	// Named thread factory
	template<typename N, typename F, typename... Args>
	static inline std::shared_ptr<thread_ctrl> spawn(N&& name, F&& func, Args&&... args)
	{
		auto ctrl = std::make_shared<thread_ctrl>(std::forward<N>(name));

		ctrl->m_thread = std::thread([ctrl, task = std::forward<F>(func)](Args&&... args)
		{
			try
			{
				ctrl->initialize();
				task(std::forward<Args>(args)...);
			}
			catch (...)
			{
				ctrl->set_exception();
			}

			ctrl->finalize();

		}, std::forward<Args>(args)...);

		return ctrl;
	}
};

class named_thread : public std::enable_shared_from_this<named_thread>
{
	// Pointer to managed resource (shared with actual thread)
	std::shared_ptr<thread_ctrl> m_thread;

public:
	named_thread();

	virtual ~named_thread();

	// Deleted copy/move constructors + copy/move operators
	named_thread(const named_thread&) = delete;

	// Get thread name
	virtual std::string get_name() const;

protected:
	// Start thread (cannot be called from the constructor: should throw bad_weak_ptr in such case)
	void start();

	// Thread task (called in the thread)
	virtual void on_task() = 0;

	// Thread finalization (called after on_task)
	virtual void on_exit() {}

public:
	// ID initialization
	virtual void on_init()
	{
		start();
	}

	// ID finalization
	virtual void on_stop()
	{
		m_thread->join();
	}

	// Get thread_ctrl
	const thread_ctrl* operator->() const
	{
		return m_thread.get();
	}

	// Lock mutex, notify condition variable
	void lock_notify() const
	{
		m_thread->lock_notify();
	}

	// Notify condition variable
	void notify() const
	{
		m_thread->notify();
	}
};

// Wrapper for named thread, joins automatically in the destructor, can only be used in function scope
class scope_thread final
{
	std::shared_ptr<thread_ctrl> m_thread;

public:
	template<typename N, typename F>
	scope_thread(N&& name, F&& func)
		: m_thread(thread_ctrl::spawn(std::forward<N>(name), std::forward<F>(func)))
	{
	}

	// Deleted copy/move constructors + copy/move operators
	scope_thread(const scope_thread&) = delete;

	// Destructor with exceptions allowed
	~scope_thread() noexcept(false)
	{
		m_thread->join();
	}
};
