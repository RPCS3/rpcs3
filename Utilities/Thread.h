#pragma once

#include <exception>
#include <string>
#include <memory>

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

	template<typename F>
	struct task_type : task_base
	{
		std::remove_reference_t<F> func;

		task_type(F&& func)
			: func(std::forward<F>(func))
		{
		}

		void exec() override
		{
			func();
			task_base::exec();
		}
	};

	std::unique_ptr<task_base> m_stack;

public:
	task_stack() = default;

	template<typename F>
	task_stack(F&& func)
		: m_stack(new task_type<F>(std::forward<F>(func)))
	{
	}

	void push(task_stack stack)
	{
		auto _top = stack.m_stack.release();
		auto _next = m_stack.release();
		m_stack.reset(_top);
		while (UNLIKELY(_top->next)) _top = _top->next.get();
		_top->next.reset(_next);
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
public: // TODO
	struct internal;

private:
	static thread_local thread_ctrl* g_tls_this_thread;

	// Thread handle storage
	std::aligned_storage_t<16> m_thread;

	// Thread join contention counter
	atomic_t<u32> m_joining{};

	// Thread internals
	atomic_t<internal*> m_data{};

	// Fixed name
	std::string m_name;

	// Start thread
	static void start(const std::shared_ptr<thread_ctrl>&, task_stack);

	// Called at the thread start
	void initialize();

	// Called at the thread end
	void finalize() noexcept;

	// Get atexit function
	void push_atexit(task_stack);

	// Start waiting
	void wait_start(u64 timeout);

	// Proceed waiting
	bool wait_wait(u64 timeout);

	// Check exception
	void test();

public:
	thread_ctrl(std::string&& name);

	thread_ctrl(const thread_ctrl&) = delete;

	~thread_ctrl();

	// Get thread name
	const std::string& get_name() const
	{
		return m_name;
	}

	// Initialize internal data
	void initialize_once();

	// Get thread result (may throw, simultaneous joining allowed)
	void join();

	// Lock thread mutex
	void lock();

	// Lock conditionally (double-checked)
	template<typename F>
	bool lock_if(F&& pred)
	{
		if (pred())
		{
			lock();

			try
			{
				if (LIKELY(pred()))
				{
					return true;
				}
				else
				{
					unlock();
					return false;
				}
			}
			catch (...)
			{
				unlock();
				throw;
			}
		}
		else
		{
			return false;
		}
	}

	// Unlock thread mutex (internal data must be initialized)
	void unlock();

	// Lock, unlock, notify the thread (required if the condition changed locklessly)
	void lock_notify();

	// Notify the thread (internal data must be initialized)
	void notify();

	// Set exception (internal data must be initialized, thread mutex must be locked)
	void set_exception(std::exception_ptr);

	// Current thread sleeps for specified amount of microseconds.
	// Wrapper for std::this_thread::sleep, doesn't require valid thread_ctrl.
	[[deprecated]] static void sleep(u64 useconds);

	// Wait until pred(). Abortable, may throw. Thread must be locked.
	// Timeout in microseconds (zero means infinite).
	template<typename F>
	static inline auto wait(u64 useconds, F&& pred)
	{
		g_tls_this_thread->wait_start(useconds);

		while (true)
		{
			g_tls_this_thread->test();

			if (auto&& result = pred())
			{
				return result;
			}
			else if (!g_tls_this_thread->wait_wait(useconds) && useconds)
			{
				return result;
			}
		}
	}

	// Wait until pred(). Abortable, may throw. Thread must be locked.
	template<typename F>
	static inline auto wait(F&& pred)
	{
		while (true)
		{
			g_tls_this_thread->test();

			if (auto&& result = pred())
			{
				return result;
			}

			g_tls_this_thread->wait_wait(0);
		}
	}

	// Wait once. Thread must be locked.
	static inline void wait()
	{
		g_tls_this_thread->test();
		g_tls_this_thread->wait_wait(0);
		g_tls_this_thread->test();
	}

	// Wait unconditionally until aborted. Thread must be locked.
	[[noreturn]] static inline void eternalize()
	{
		while (true)
		{
			g_tls_this_thread->test();
			g_tls_this_thread->wait_wait(0);
		}
	}

	// Get current thread (may be nullptr)
	static thread_ctrl* get_current()
	{
		return g_tls_this_thread;
	}

	// Register function at thread exit (for the current thread)
	template<typename F>
	static inline void atexit(F&& func)
	{
		return g_tls_this_thread->push_atexit(std::forward<F>(func));
	}

	// Named thread factory
	template<typename N, typename F>
	static inline std::shared_ptr<thread_ctrl> spawn(N&& name, F&& func)
	{
		auto ctrl = std::make_shared<thread_ctrl>(std::forward<N>(name));

		thread_ctrl::start(ctrl, std::forward<F>(func));

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

	// Access thread_ctrl
	thread_ctrl* operator->() const
	{
		return m_thread.get();
	}
};

// Simple thread mutex locker
class thread_lock final
{
	thread_ctrl* m_thread;

public:
	thread_lock(const thread_lock&) = delete;

	// Lock specified thread
	thread_lock(thread_ctrl* thread)
		: m_thread(thread)
	{
		m_thread->lock();
	}

	// Lock specified named_thread
	thread_lock(named_thread& thread)
		: thread_lock(thread.operator->())
	{
	}

	// Lock current thread
	thread_lock()
		: thread_lock(thread_ctrl::get_current())
	{
	}

	~thread_lock()
	{
		m_thread->unlock();
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
