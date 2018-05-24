#pragma once

#include "types.h"
#include "Atomic.h"

#include <exception>
#include <string>
#include <memory>

#include "sema.h"
#include "cond.h"

// Report error and call std::abort(), defined in main.cpp
[[noreturn]] void report_fatal_error(const std::string&);

// Will report exception and call std::abort() if put in catch(...)
[[noreturn]] void catch_all_exceptions();

// Hardware core layout
enum class native_core_arrangement : u32
{
	undefined,
	generic,
	intel_ht,
	amd_ccx
};

enum class thread_class : u32
{
	general,
	rsx,
	spu,
	ppu
};

// Simple list of void() functors
class task_stack
{
	struct task_base
	{
		std::unique_ptr<task_base> next;

		virtual ~task_base();

		virtual void invoke()
		{
			if (next)
			{
				next->invoke();
			}
		}
	};

	template <typename F>
	struct task_type final : task_base
	{
		std::remove_reference_t<F> func;

		task_type(F&& func)
			: func(std::forward<F>(func))
		{
		}

		void invoke() final override
		{
			func();
			task_base::invoke();
		}
	};

	std::unique_ptr<task_base> m_stack;

public:
	task_stack() = default;

	template <typename F>
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

	void invoke() const
	{
		if (m_stack)
		{
			m_stack->invoke();
		}
	}
};

// Thread control class
class thread_ctrl final
{
	// Current thread
	static thread_local thread_ctrl* g_tls_this_thread;

	// Target cpu core layout
	static atomic_t<native_core_arrangement> g_native_core_layout;

	// Self pointer
	std::shared_ptr<thread_ctrl> m_self;

	// Thread handle (platform-specific)
	atomic_t<std::uintptr_t> m_thread{0};

	// Thread mutex
	mutable semaphore<> m_mutex;

	// Thread condition variable
	cond_variable m_cond;

	// Thread flags
	atomic_t<u32> m_signal{0};

	// Thread joining condition variable
	cond_variable m_jcv;

	// Remotely set or caught exception
	std::exception_ptr m_exception;

	// Thread initial task or atexit task
	task_stack m_task;

	// Fixed name
	std::string m_name;

	// CPU cycles thread has run for
	u64 m_cycles{0};

	// Start thread
	static void start(const std::shared_ptr<thread_ctrl>&, task_stack);

	// Called at the thread start
	void initialize();

	// Called at the thread end
	void finalize(std::exception_ptr) noexcept;

	// Add task (atexit)
	static void _push(task_stack);

	// Internal waiting function, may throw. Infinite value is -1.
	static bool _wait_for(u64 usec);

	// Internal throwing function. Mutex must be locked and will be unlocked.
	[[noreturn]] void _throw();

	// Internal notification function
	void _notify(cond_variable thread_ctrl::*);

public:
	thread_ctrl(std::string&& name);

	thread_ctrl(const thread_ctrl&) = delete;

	~thread_ctrl();

	// Get thread name
	const std::string& get_name() const
	{
		return m_name;
	}

	// Get CPU cycles since last time this function was called. First call returns 0.
	u64 get_cycles();

	// Get platform-specific thread handle
	std::uintptr_t get_native_handle() const
	{
		return m_thread.load();
	}

	// Get exception
	std::exception_ptr get_exception() const;

	// Set exception
	void set_exception(std::exception_ptr ptr);

	// Get thread result (may throw, simultaneous joining allowed)
	void join();

	// Notify the thread
	void notify();

	// Wait once with timeout. Abortable, may throw. May spuriously return false.
	static inline bool wait_for(u64 usec)
	{
		return _wait_for(usec);
	}

	// Wait. Abortable, may throw.
	static inline void wait()
	{
		_wait_for(-1);
	}

	// Wait until pred(). Abortable, may throw.
	template<typename F, typename RT = std::result_of_t<F()>>
	static inline RT wait(F&& pred)
	{
		while (true)
		{
			if (RT result = pred())
			{
				return result;
			}

			_wait_for(-1);
		}
	}

	// Wait eternally until aborted.
	[[noreturn]] static inline void eternalize()
	{
		while (true)
		{
			_wait_for(-1);
		}
	}

	// Test exception (may throw).
	static void test();

	// Get current thread (may be nullptr)
	static thread_ctrl* get_current()
	{
		return g_tls_this_thread;
	}

	// Register function at thread exit (for the current thread)
	template<typename F>
	static inline void atexit(F&& func)
	{
		_push(std::forward<F>(func));
	}

	// Create detached named thread
	template<typename N, typename F>
	static inline void spawn(N&& name, F&& func)
	{
		auto out = std::make_shared<thread_ctrl>(std::forward<N>(name));

		thread_ctrl::start(out, std::forward<F>(func));
	}

	// Named thread factory
	template<typename N, typename F>
	static inline void spawn(std::shared_ptr<thread_ctrl>& out, N&& name, F&& func)
	{
		out = std::make_shared<thread_ctrl>(std::forward<N>(name));

		thread_ctrl::start(out, std::forward<F>(func));
	}

	// Detect layout
	static void detect_cpu_layout();

	// Returns a core affinity mask. Set whether to generate the high priority set or not
	static u16 get_affinity_mask(thread_class group);

	// Sets the native thread priority
	static void set_native_priority(int priority);

	// Sets the preferred affinity mask for this thread
	static void set_thread_affinity_mask(u16 mask);
};

class named_thread
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
	// Start thread (cannot be called from the constructor: should throw in such case)
	void start_thread(const std::shared_ptr<void>& _this);

	// Thread task (called in the thread)
	virtual void on_task() = 0;

	// Thread finalization (called after on_task)
	virtual void on_exit() {}

	// Called once upon thread spawn within the thread's own context
	virtual void on_spawn() {}

public:
	// ID initialization
	virtual void on_init(const std::shared_ptr<void>& _this)
	{
		return start_thread(_this);
	}

	// ID finalization
	virtual void on_stop()
	{
		m_thread->join();
	}

	// Access thread_ctrl
	thread_ctrl* get() const
	{
		return m_thread.get();
	}

	void join() const
	{
		return m_thread->join();
	}

	void notify() const
	{
		return m_thread->notify();
	}
};

// Wrapper for named thread, joins automatically in the destructor, can only be used in function scope
class scope_thread final
{
	std::shared_ptr<thread_ctrl> m_thread;

public:
	template<typename N, typename F>
	scope_thread(N&& name, F&& func)
	{
		thread_ctrl::spawn(m_thread, std::forward<N>(name), std::forward<F>(func));
	}

	// Deleted copy/move constructors + copy/move operators
	scope_thread(const scope_thread&) = delete;

	// Destructor with exceptions allowed
	~scope_thread() noexcept(false)
	{
		m_thread->join();
	}

	// Access thread_ctrl
	thread_ctrl* get() const
	{
		return m_thread.get();
	}
};
