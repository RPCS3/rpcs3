#pragma once

#include <exception>
#include <string>
#include <memory>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "Platform.h"

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

	never_inline void push(std::unique_ptr<task_base> task)
	{
		m_stack.swap(task->next);
		m_stack.swap(task);
	}

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

		return push(std::unique_ptr<task_base>{ new task_t(std::forward<F>(func)) });
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
	static thread_local thread_ctrl* g_tls_this_thread;

	// Fixed name
	std::string m_name;

	// Thread handle (be careful)
	std::thread m_thread;

	// Thread result (exception)
	std::exception_ptr m_exception;

	// Functions scheduled at thread exit
	task_stack m_atexit;

	// Called at the thread start
	static void initialize();

	// Called at the thread end
	static void finalize() noexcept;

public:
	template<typename N>
	thread_ctrl(N&& name)
		: m_name(std::forward<N>(name))
	{
	}

	// Disable copy/move constructors and operators
	thread_ctrl(const thread_ctrl&) = delete;

	~thread_ctrl()
	{
		if (m_thread.joinable())
		{
			m_thread.detach();
		}
	}

	// Get thread name
	const std::string& get_name() const
	{
		return m_name;
	}

	// Get thread result (may throw)
	void join()
	{
		if (m_thread.joinable())
		{
			m_thread.join();
		}

		if (auto&& e = std::move(m_exception))
		{
			std::rethrow_exception(e);
		}
	}

	// Get current thread (may be nullptr)
	static const thread_ctrl* get_current()
	{
		return g_tls_this_thread;
	}

	// Register function at thread exit (for the current thread)
	template<typename F>
	static inline void at_exit(F&& func)
	{
		return g_tls_this_thread->m_atexit.push(std::forward<F>(func));
	}

	// Named thread factory
	template<typename N, typename F>
	static inline std::shared_ptr<thread_ctrl> spawn(N&& name, F&& func)
	{
		auto ctrl = std::make_shared<thread_ctrl>(std::forward<N>(name));

		ctrl->m_thread = std::thread([ctrl, task = std::forward<F>(func)]()
		{
			// Initialize TLS variable
			g_tls_this_thread = ctrl.get();

			try
			{
				initialize();
				task();
				finalize();
			}
			catch (...)
			{
				finalize();

				// Set exception
				ctrl->m_exception = std::current_exception();
			}
		});

		return ctrl;
	}
};

class named_thread : public std::enable_shared_from_this<named_thread>
{
	// Pointer to managed resource (shared with actual thread)
	std::shared_ptr<thread_ctrl> m_thread;

public:
	// Thread condition variable for external use (this thread waits on it, other threads may notify)
	std::condition_variable cv;

	// Thread mutex for external use (can be used with `cv`)
	std::mutex mutex;

	// Lock mutex, notify condition variable
	void safe_notify()
	{
		// Lock for reliable notification, condition is assumed to be changed externally
		std::unique_lock<std::mutex> lock(mutex);

		cv.notify_one();
	}

	// ID initialization
	virtual void on_init()
	{
		start();
	}

	// ID finalization
	virtual void on_stop()
	{
		join();
	}

protected:
	// Thread task (called in the thread)
	virtual void on_task() = 0;

	// Thread finalization (called after on_task)
	virtual void on_exit() {}

public:
	named_thread() = default;

	virtual ~named_thread() = default;

	// Deleted copy/move constructors + copy/move operators
	named_thread(const named_thread&) = delete;

	// Get thread name
	virtual std::string get_name() const;

	// Start thread (cannot be called from the constructor: should throw bad_weak_ptr in such case)
	void start();

	// Join thread (get thread result)
	void join();

	// Get thread_ctrl
	const thread_ctrl* get_thread_ctrl() const
	{
		return m_thread.get();
	}

	// Compare with the current thread
	bool is_current() const
	{
		return m_thread && thread_ctrl::get_current() == m_thread.get();
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
