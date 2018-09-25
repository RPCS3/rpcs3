#pragma once

#include "types.h"
#include "Atomic.h"

#include <exception>
#include <string>
#include <memory>
#include <string_view>

#include "mutex.h"
#include "cond.h"
#include "lockless.h"

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

enum class thread_state
{
	created,  // Initial state
	detached, // Set if the thread has been detached successfully (only possible via shared_ptr)
	aborting, // Set if the thread has been joined in destructor (mutually exclusive with detached)
	finished  // Final state, always set at the end of thread execution
};

template <class Context>
class named_thread;

template <typename T>
struct result_storage
{
	alignas(T) std::byte data[sizeof(T)];

	static constexpr bool empty = false;

	using type = T;

	T* get()
	{
		return reinterpret_cast<T*>(&data);
	}

	const T* get() const
	{
		return reinterpret_cast<const T*>(&data);
	}
};

template <>
struct result_storage<void>
{
	static constexpr bool empty = true;

	using type = void;
};

template <class Context, typename... Args>
using result_storage_t = result_storage<std::invoke_result_t<Context, Args...>>;

// Detect on_stop() method (should return void)
template <typename T, typename = void>
struct thread_on_stop : std::bool_constant<false> {};

template <typename T>
struct thread_on_stop<T, decltype(std::declval<named_thread<T>&>().on_stop())> : std::bool_constant<true> {};

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

// Thread base class
class thread_base : public std::enable_shared_from_this<thread_base>
{
	// Native thread entry point function type
#ifdef _WIN32
	using native_entry = uint(__stdcall*)(void* arg);
#else
	using native_entry = void*(*)(void* arg);
#endif

	// Self pointer for detached thread
	std::shared_ptr<thread_base> m_self;

	// Thread handle (platform-specific)
	atomic_t<std::uintptr_t> m_thread{0};

	// Thread mutex
	mutable shared_mutex m_mutex;

	// Thread condition variable
	cond_variable m_cond;

	// Thread flags
	atomic_t<u32> m_signal{0};

	// Thread joining condition variable
	mutable cond_variable m_jcv;

	// Thread state
	atomic_t<thread_state> m_state = thread_state::created;

	// Remotely set or caught exception
	std::exception_ptr m_exception;

	// Thread initial task
	task_stack m_task;

	// Thread name
	lf_value<std::string> m_name;

	// CPU cycles thread has run for
	u64 m_cycles{0};

	// Start thread
	static void start(const std::shared_ptr<thread_base>&, task_stack);

	void start(native_entry);

	// Called at the thread start
	void initialize();

	// Called at the thread end, returns moved m_self (may be null)
	std::shared_ptr<thread_base> finalize(std::exception_ptr) noexcept;

	static void finalize() noexcept;

	// Internal throwing function. Mutex must be locked and will be unlocked.
	[[noreturn]] void _throw();

	// Internal notification function
	void _notify(cond_variable thread_base::*);

	friend class thread_ctrl;

	template <class Context>
	friend class named_thread;

public:
	thread_base(std::string_view name);

	~thread_base();

	// Get thread name
	const std::string& get_name() const
	{
		return m_name;
	}

	// Set thread name (not recommended)
	void set_name(std::string_view name)
	{
		m_name.assign(name);
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

	// Wait for the thread (it does NOT change thread state, and can be called from multiple threads)
	void join() const;

	// Make thread to manage a shared_ptr of itself
	void detach();

	// Notify the thread
	void notify();
};

// Collection of global function for current thread
class thread_ctrl final
{
	// Current thread
	static thread_local thread_base* g_tls_this_thread;

	// Target cpu core layout
	static atomic_t<native_core_arrangement> g_native_core_layout;

	// Internal waiting function, may throw. Infinite value is -1.
	static bool _wait_for(u64 usec);

	friend class thread_base;

public:
	// Read current state
	static inline thread_state state()
	{
		return g_tls_this_thread->m_state;
	}

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
	template <typename F, typename RT = std::invoke_result_t<F>>
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
	static thread_base* get_current()
	{
		return g_tls_this_thread;
	}

	// Create detached named thread
	template <typename N, typename F>
	static inline void spawn(N&& name, F&& func)
	{
		auto out = std::make_shared<thread_base>(std::forward<N>(name));

		thread_base::start(out, std::forward<F>(func));
	}

	// Named thread factory
	template <typename N, typename F>
	static inline void spawn(std::shared_ptr<thread_base>& out, N&& name, F&& func)
	{
		out = std::make_shared<thread_base>(std::forward<N>(name));

		thread_base::start(out, std::forward<F>(func));
	}

	// Detect layout
	static void detect_cpu_layout();

	// Returns a core affinity mask. Set whether to generate the high priority set or not
	static u16 get_affinity_mask(thread_class group);

	// Sets the native thread priority
	static void set_native_priority(int priority);

	// Sets the preferred affinity mask for this thread
	static void set_thread_affinity_mask(u16 mask);

	template <typename F>
	static inline std::shared_ptr<named_thread<F>> make_shared(std::string_view name, F&& lambda)
	{
		return std::make_shared<named_thread<F>>(name, std::forward<F>(lambda));
	}

	template <typename T, typename... Args>
	static inline std::shared_ptr<named_thread<T>> make_shared(std::string_view name, Args&&... args)
	{
		return std::make_shared<named_thread<T>>(name, std::forward<Args>(args)...);
	}
};

// Derived from the callable object Context, possibly a lambda
template <class Context>
class named_thread final : public Context, result_storage_t<Context>, public thread_base
{
	using result = result_storage_t<Context>;
	using thread = thread_base;

	// Type-erased thread entry point
#ifdef _WIN32
	static inline uint __stdcall entry_point(void* arg) try
#else
	static inline void* entry_point(void* arg) try
#endif
	{
		const auto maybe_last_ptr = static_cast<named_thread*>(static_cast<thread*>(arg))->entry_point();
		thread::finalize();
		return 0;
	}
	catch (...)
	{
		catch_all_exceptions();
	}

	std::shared_ptr<thread> entry_point()
	{
		thread::initialize();

		if constexpr (result::empty)
		{
			// No result
			Context::operator()();
		}
		else
		{
			// Construct the result using placement new (copy elision should happen)
			new (result::get()) typename result::type(Context::operator()());
		}

		return thread::finalize(nullptr);
	}

public:

	// Normal forwarding constructor
	template <typename... Args, typename = std::enable_if_t<std::is_constructible_v<Context, Args&&...>>>
	named_thread(std::string_view name, Args&&... args)
		: Context(std::forward<Args>(args)...)
		, thread(name)
	{
		thread::start(&named_thread::entry_point);
	}

	// Lambda constructor, also the implicit deduction guide candidate
	named_thread(std::string_view name, Context&& f)
		: Context(std::forward<Context>(f))
		, thread(name)
	{
		thread::start(&named_thread::entry_point);
	}

	// Wait for the completion and access result (if not void)
	[[nodiscard]] decltype(auto) operator()()
	{
		thread::join();

		if constexpr (!result::empty)
		{
			return *result::get();
		}
	}

	// Wait for the completion and access result (if not void)
	[[nodiscard]] decltype(auto) operator()() const
	{
		thread::join();

		if constexpr (!result::empty)
		{
			return *result::get();
		}
	}

	// Access thread state
	operator thread_state() const
	{
		return thread::m_state.load();
	}

	// Context type doesn't need virtual destructor
	~named_thread()
	{
		// Notify thread if not detached or terminated
		if (thread::m_state.compare_and_swap_test(thread_state::created, thread_state::aborting))
		{
			// Additional notification if on_stop() method exists
			if constexpr (thread_on_stop<Context>())
			{
				Context::on_stop();
			}

			thread::notify();
			thread::join();
		}
	}
};

// Old named_thread
class old_thread
{
	// Pointer to managed resource (shared with actual thread)
	std::shared_ptr<thread_base> m_thread;

public:
	old_thread();

	virtual ~old_thread();

	old_thread(const old_thread&) = delete;

	old_thread& operator=(const old_thread&) = delete;

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

	thread_base* get() const
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
