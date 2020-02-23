#pragma once

#include "types.h"
#include "util/atomic.hpp"

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

enum class thread_state : u32
{
	created,  // Initial state
	detached, // The thread has been detached to destroy its own named_thread object (can be dangerously misused)
	aborting, // The thread has been joined in the destructor or explicitly aborted (mutually exclusive with detached)
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

	void destroy() noexcept
	{
		get()->~T();
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

// Detect on_abort() method (should return void)
template <typename T, typename = void>
struct thread_on_abort : std::bool_constant<false> {};

template <typename T>
struct thread_on_abort<T, decltype(std::declval<named_thread<T>&>().on_abort())> : std::bool_constant<true> {};

// Detect on_cleanup() static member function (should return void) (in C++20 can use destroying delete instead)
template <typename T, typename = void>
struct thread_on_cleanup : std::bool_constant<false> {};

template <typename T>
struct thread_on_cleanup<T, decltype(named_thread<T>::on_cleanup(std::declval<named_thread<T>*>()))> : std::bool_constant<true> {};

// Detect on_wait() method (should return bool)
template <typename T, typename = bool>
struct thread_on_wait : std::bool_constant<false> {};

template <typename T>
struct thread_on_wait<T, decltype(std::declval<named_thread<T>&>().on_wait())> : std::bool_constant<true> {};

template <typename T, typename = void>
struct thread_thread_name : std::bool_constant<false> {};

template <typename T>
struct thread_thread_name<T, std::void_t<decltype(named_thread<T>::thread_name)>> : std::bool_constant<true> {};

// Thread base class
class thread_base
{
	// Native thread entry point function type
#ifdef _WIN32
	using native_entry = uint(__stdcall*)(void* arg);
#else
	using native_entry = void*(*)(void* arg);
#endif

#ifdef __linux__
	// Linux thread timer
	int m_timer = -1;
#endif

	// Thread handle (platform-specific)
	atomic_t<std::uintptr_t> m_thread{0};

	// Thread mutex
	mutable shared_mutex m_mutex;

	// Thread condition variable
	cond_variable m_cond;

	// Thread flags
	atomic_t<u32> m_signal{0};

	// Thread state
	atomic_t<thread_state> m_state = thread_state::created;

	// Thread state notification info
	atomic_t<const void*> m_state_notifier{nullptr};

	// Thread name
	lf_value<std::string> m_name;

	//
	atomic_t<u64> m_cycles = 0;

	// Start thread
	void start(native_entry);

	// Called at the thread start
	void initialize(bool(*wait_cb)(const void*));

	// May be called in destructor
	void notify_abort() noexcept;

	// Called at the thread end, returns true if needs destruction
	bool finalize(int) noexcept;

	// Cleanup after possibly deleting the thread instance
	static void finalize() noexcept;

	friend class thread_ctrl;

	template <class Context>
	friend class named_thread;

protected:
	thread_base(std::string_view name);

	~thread_base();

public:
	// Get CPU cycles since last time this function was called. First call returns 0.
	u64 get_cycles();

	// Wait for the thread (it does NOT change thread state, and can be called from multiple threads)
	void join() const;

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
	static void _wait_for(u64 usec, bool alert);

	friend class thread_base;

public:
	// Get current thread name
	static std::string_view get_name()
	{
		return g_tls_this_thread->m_name.get();
	}

	// Get thread name
	template <typename T>
	static std::string_view get_name(const named_thread<T>& thread)
	{
		return static_cast<const thread_base&>(thread).m_name.get();
	}

	// Set current thread name (not recommended)
	static void set_name(std::string_view name)
	{
		g_tls_this_thread->m_name.assign(name);
	}

	// Set thread name (not recommended)
	template <typename T>
	static void set_name(named_thread<T>& thread, std::string_view name)
	{
		static_cast<thread_base&>(thread).m_name.assign(name);
	}

	template <typename T>
	static u64 get_cycles(named_thread<T>& thread)
	{
		return static_cast<thread_base&>(thread).get_cycles();
	}

	template <typename T>
	static void notify(named_thread<T>& thread)
	{
		static_cast<thread_base&>(thread).notify();
	}

	// Read current state
	static inline thread_state state()
	{
		return g_tls_this_thread->m_state;
	}

	// Wait once with timeout. May spuriously return false.
	static inline void wait_for(u64 usec, bool alert = true)
	{
		_wait_for(usec, alert);
	}

	// Wait.
	static inline void wait()
	{
		_wait_for(-1, true);
	}

	// Wait until pred().
	template <typename F, typename RT = std::invoke_result_t<F>>
	static inline RT wait(F&& pred)
	{
		while (true)
		{
			if (RT result = pred())
			{
				return result;
			}

			_wait_for(-1, true);
		}
	}

	// Get current thread (may be nullptr)
	static thread_base* get_current()
	{
		return g_tls_this_thread;
	}

	// Detect layout
	static void detect_cpu_layout();

	// Returns a core affinity mask. Set whether to generate the high priority set or not
	static u64 get_affinity_mask(thread_class group);

	// Sets the native thread priority
	static void set_native_priority(int priority);

	// Sets the preferred affinity mask for this thread
	static void set_thread_affinity_mask(u64 mask);
};

// Derived from the callable object Context, possibly a lambda
template <class Context>
class named_thread final : public Context, result_storage_t<Context>, thread_base
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
		const auto _this = static_cast<named_thread*>(static_cast<thread*>(arg));

		// Perform self-cleanup if necessary
		if (_this->entry_point())
		{
			// Call on_cleanup() static member function if it's available
			if constexpr (thread_on_cleanup<Context>())
			{
				Context::on_cleanup(_this);
			}
			else
			{
				delete _this;
			}
		}

		thread::finalize();
		return 0;
	}
	catch (...)
	{
		catch_all_exceptions();
	}

	bool entry_point()
	{
		thread::initialize([](const void* data)
		{
			const auto _this = thread_ctrl::get_current();

			if (_this->m_state >= thread_state::aborting)
			{
				return false;
			}

			if constexpr (thread_on_wait<Context>())
			{
				if (!static_cast<named_thread*>(_this)->on_wait())
				{
					return false;
				}
			}

			_this->m_state_notifier.release(data);

			if (!data)
			{
				return true;
			}

			if (_this->m_state >= thread_state::aborting)
			{
				_this->m_state_notifier.release(nullptr);
				return false;
			}

			if constexpr (thread_on_wait<Context>())
			{
				if (!static_cast<named_thread*>(_this)->on_wait())
				{
					_this->m_state_notifier.release(nullptr);
					return false;
				}
			}

			return true;
		});

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

		return thread::finalize(0);
	}

	static decltype(auto) get_default_thread_name()
	{
		if constexpr (thread_thread_name<Context>())
		{
			return Context::thread_name;
		}
		else
		{
			return "Unnamed Thread";
		}
	}

	// Detached thread constructor
	named_thread(thread_state s, std::string_view name, Context&& f)
		: Context(std::forward<Context>(f))
		, thread(name)
	{
		thread::m_state.raw() = s;
		thread::start(&named_thread::entry_point);
	}

	friend class thread_ctrl;

public:
	// Default constructor
	template <bool Valid = std::is_default_constructible_v<Context>, typename = std::enable_if_t<Valid>>
	named_thread()
		: Context()
		, thread(get_default_thread_name())
	{
		thread::start(&named_thread::entry_point);
	}

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

	named_thread(const named_thread&) = delete;

	named_thread& operator=(const named_thread&) = delete;

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

	// Try to abort/detach
	named_thread& operator=(thread_state s)
	{
		if (s < thread_state::finished && thread::m_state.compare_and_swap_test(thread_state::created, s))
		{
			if (s == thread_state::aborting)
			{
				// Call on_abort() method if it's available
				if constexpr (thread_on_abort<Context>())
				{
					Context::on_abort();
				}

				thread::notify_abort();
			}
		}

		return *this;
	}

	// Context type doesn't need virtual destructor
	~named_thread()
	{
		operator=(thread_state::aborting);
		thread::join();

		if constexpr (!result::empty)
		{
			result::destroy();
		}
	}
};
