#pragma once

#include "util/types.hpp"
#include "util/atomic.hpp"
#include "util/shared_ptr.hpp"

#include <string>

#include "mutex.h"
#include "lockless.h"

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
	created = 0,  // Initial state
	aborting = 1, // The thread has been joined in the destructor or explicitly aborted
	errored = 2, // Set after the emergency_exit call
	finished = 3,  // Final state, always set at the end of thread execution
	mask = 3
};

template <class Context>
class named_thread;

class thread_base;

template <typename Ctx, typename... Args>
struct result_storage
{
	static constexpr bool empty = true;

	using type = void;
};

template <typename Ctx, typename... Args> requires (!std::is_void_v<std::invoke_result_t<Ctx, Args&&...>>)
struct result_storage<Ctx, Args...>
{
	using T = std::invoke_result_t<Ctx, Args&&...>;

	static_assert(std::is_default_constructible_v<T>);

	alignas(T) std::byte data[sizeof(T)];

	static constexpr bool empty = false;

	using type = T;

	T* _get()
	{
		return reinterpret_cast<T*>(&data);
	}

	const T* _get() const
	{
		return reinterpret_cast<const T*>(&data);
	}

	void init() noexcept
	{
		new (data) T();
	}

	void destroy() noexcept
	{
		_get()->~T();
	}
};

template <typename T>
concept NamedThreadName = requires (const T& t)
{
	std::string(t.thread_name);
};

// Base class for task queue (linked list)
class thread_future
{
	friend class thread_base;

	shared_ptr<thread_future> next{};

	shared_ptr<thread_future>* prev{};

protected:
	atomic_t<void(*)(thread_base*, thread_future*)> exec{};

public:
	// Get reference to the atomic variable for inspection and waiting for
	const auto& get_wait() const
	{
		return exec;
	}

	// Wait (preset)
	void wait() const
	{
		exec.wait<atomic_wait::op_ne>(nullptr);
	}
};

// Thread base class
class thread_base
{
public:
	// Native thread entry point function type
#ifdef _WIN32
	using native_entry = uint(__stdcall*)(void* arg);
#else
	using native_entry = void*(*)(void* arg);
#endif

	const native_entry entry_point;

private:
	// Thread handle (platform-specific)
	atomic_t<u64> m_thread{0};

	// Thread state and cycles
	atomic_t<u64> m_sync{0};

	// Thread name
	atomic_ptr<std::string> m_tname;

	// Thread task queue (reversed linked list)
	atomic_ptr<thread_future> m_taskq{};

	// Start thread
	void start();

	// Called at the thread start
	void initialize(void (*error_cb)());

	// Called at the thread end, returns self handle
	u64 finalize(thread_state result) noexcept;

	// Cleanup after possibly deleting the thread instance
	static native_entry finalize(u64 _self) noexcept;

	// Set name for debugger
	static void set_name(std::string);

	// Make entry point
	static native_entry make_trampoline(u64(*)(thread_base*));

	friend class thread_ctrl;

	template <class Context>
	friend class named_thread;

protected:
	thread_base(native_entry, std::string name);

	~thread_base();

public:
	// Get CPU cycles since last time this function was called. First call returns 0.
	u64 get_cycles();

	// Wait for the thread (it does NOT change thread state, and can be called from multiple threads)
	bool join(bool dtor = false) const;

	// Notify the thread
	void notify();

	// Get thread id
	u64 get_native_id() const;

	// Add work to the queue
	void push(shared_ptr<thread_future>);

private:
	// Clear task queue (execute unless aborting)
	void exec();
};

// Collection of global function for current thread
class thread_ctrl final
{
	// Current thread
	static thread_local thread_base* g_tls_this_thread;

	// Error handling details
	static thread_local void(*g_tls_error_callback)();

	// Target cpu core layout
	static atomic_t<native_core_arrangement> g_native_core_layout;

	// Internal waiting function, may throw. Infinite value is -1.
	static void _wait_for(u64 usec, bool alert);

	friend class thread_base;

	// Optimized get_name() for logging
	static std::string get_name_cached();

public:
	// Get current thread name
	static std::string get_name()
	{
		return *g_tls_this_thread->m_tname.load();
	}

	// Get thread name
	template <typename T>
	static std::string get_name(const named_thread<T>& thread)
	{
		return *static_cast<const thread_base&>(thread).m_tname.load();
	}

	// Set current thread name (not recommended)
	static void set_name(std::string name)
	{
		g_tls_this_thread->m_tname.store(make_single<std::string>(name));
		g_tls_this_thread->set_name(std::move(name));
	}

	// Set thread name (not recommended)
	template <typename T>
	static void set_name(named_thread<T>& thread, std::string name)
	{
		static_cast<thread_base&>(thread).m_tname.store(make_single<std::string>(name));

		if (g_tls_this_thread == std::addressof(static_cast<thread_base&>(thread)))
		{
			g_tls_this_thread->set_name(std::move(name));
		}
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

	template <typename T>
	static u64 get_native_id(named_thread<T>& thread)
	{
		return static_cast<thread_base&>(thread).get_native_id();
	}

	// Read current state, possibly executing some tasks
	static thread_state state();

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

	// Wait for both thread sync var and provided atomic var
	template <atomic_wait::op Op = atomic_wait::op::eq, typename T, typename U>
	static inline void wait_on(T& wait, U old, u64 usec = -1)
	{
		auto _this = g_tls_this_thread;

		if (_this->m_sync.bit_test_reset(2) || _this->m_taskq)
		{
			return;
		}

		atomic_wait::list<3> list{};
		list.set<0, Op>(wait, old);
		list.set<1>(_this->m_sync, 0, 4 + 1);
		list.set<2>(_this->m_taskq, nullptr);
		list.wait(atomic_wait_timeout{usec <= 0xffff'ffff'ffff'ffff / 1000 ? usec * 1000 : 0xffff'ffff'ffff'ffff});
	}

	// Exit.
	[[noreturn]] static void emergency_exit(std::string_view reason);

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

	// Get process affinity mask
	static u64 get_process_affinity_mask();

	// Miscellaneous
	static u64 get_thread_affinity_mask();

	// Get current thread stack addr and size
	static std::pair<void*, usz> get_thread_stack();

	// Sets the native thread priority and returns it to zero at destructor
	struct scoped_priority
	{
		explicit scoped_priority(int prio)
		{
			set_native_priority(prio);
		}

		scoped_priority(const scoped_priority&) = delete;

		scoped_priority& operator=(const scoped_priority&) = delete;

		~scoped_priority()
		{
			set_native_priority(0);
		}
	};

	// Get thread ID (works for all threads)
	static u64 get_tid();

	// Check whether current thread is main thread (usually Qt GUI)
	static bool is_main();

private:
	// Miscellaneous
	static const u64 process_affinity_mask;
};

// Used internally
template <bool Discard, typename Ctx, typename... Args>
class thread_future_t : public thread_future, result_storage<Ctx, std::conditional_t<Discard, int, void>, Args...>
{
	[[no_unique_address]] decltype(std::make_tuple(std::forward<Args>(std::declval<Args>())...)) m_args;

	[[no_unique_address]] Ctx m_func;

	using future = thread_future_t;

public:
	thread_future_t(Ctx&& func, Args&&... args)
		: m_args(std::forward<Args>(args)...)
		, m_func(std::forward<Ctx>(func))
	{
		thread_future::exec.raw() = +[](thread_base* tb, thread_future* tf)
		{
			const auto _this = static_cast<future*>(tf);

			if (!tb) [[unlikely]]
			{
				if constexpr (!future::empty && !Discard)
				{
					_this->init();
				}

				return;
			}

			if constexpr (future::empty || Discard)
			{
				std::apply(_this->m_func, std::move(_this->m_args));
			}
			else
			{
				new (_this->_get()) decltype(auto)(std::apply(_this->m_func, std::move(_this->m_args)));
			}
		};
	}

	~thread_future_t()
	{
		if constexpr (!future::empty && !Discard)
		{
			if (!this->exec)
			{
				this->destroy();
			}
		}
	}

	decltype(auto) get()
	{
		while (this->exec)
		{
			this->wait();
		}

		if constexpr (!future::empty && !Discard)
		{
			return *this->_get();
		}
	}

	decltype(auto) get() const
	{
		while (this->exec)
		{
			this->wait();
		}

		if constexpr (!future::empty && !Discard)
		{
			return *this->_get();
		}
	}
};

// Derived from the callable object Context, possibly a lambda
template <class Context>
class named_thread final : public Context, result_storage<Context>, thread_base
{
	using result = result_storage<Context>;
	using thread = thread_base;

	static u64 entry_point(thread_base* _base)
	{
		return static_cast<named_thread*>(_base)->entry_point2();
	}

	u64 entry_point2()
	{
		thread::initialize([]()
		{
			if constexpr (!result::empty)
			{
				// Construct using default constructor in the case of failure
				static_cast<result*>(static_cast<named_thread*>(thread_ctrl::get_current()))->init();
			}
		});

		if constexpr (result::empty)
		{
			// No result
			if constexpr (std::is_invocable_v<Context>)
			{
				Context::operator()();
			}
			else
			{
				// Default event loop
				while (thread_ctrl::state() != thread_state::aborting)
				{
					thread_ctrl::wait();
				}
			}
		}
		else
		{
			// Construct the result using placement new (copy elision should happen)
			new (result::_get()) decltype(auto)(Context::operator()());
		}

		return thread::finalize(thread_state::finished);
	}

	static inline thread::native_entry trampoline = thread::make_trampoline(entry_point);

	friend class thread_ctrl;

public:
	// Forwarding constructor with default name (also potentially the default constructor)
	template <typename... Args> requires (std::is_constructible_v<Context, Args&&...>) && (NamedThreadName<Context>)
	named_thread(Args&&... args)
		: Context(std::forward<Args>(args)...)
		, thread(trampoline, std::string(Context::thread_name))
	{
		thread::start();
	}

	// Normal forwarding constructor
	template <typename... Args> requires (std::is_constructible_v<Context, Args&&...>)
	named_thread(std::string name, Args&&... args)
		: Context(std::forward<Args>(args)...)
		, thread(trampoline, std::move(name))
	{
		thread::start();
	}

	// Lambda constructor, also the implicit deduction guide candidate
	named_thread(std::string_view name, Context&& f)
		: Context(std::forward<Context>(f))
		, thread(trampoline, std::string(name))
	{
		thread::start();
	}

	named_thread(const named_thread&) = delete;

	named_thread& operator=(const named_thread&) = delete;

	// Wait for the completion and access result (if not void)
	[[nodiscard]] decltype(auto) operator()()
	{
		thread::join();

		if constexpr (!result::empty)
		{
			return *result::_get();
		}
	}

	// Wait for the completion and access result (if not void)
	[[nodiscard]] decltype(auto) operator()() const
	{
		thread::join();

		if constexpr (!result::empty)
		{
			return *result::_get();
		}
	}

	// Send command to the thread to invoke directly (references should be passed via std::ref())
	template <bool Discard = true, typename Arg, typename... Args>
	auto operator()(Arg&& arg, Args&&... args)
	{
		// Overloaded operator() of the Context.
		constexpr bool v1 = std::is_invocable_v<Context, Arg&&, Args&&...>;

		// Anything invocable, not necessarily involving the Context.
		constexpr bool v2 = std::is_invocable_v<Arg&&, Args&&...>;

		// Could be pointer to a non-static member function (or data member) of the Context.
		constexpr bool v3 = std::is_member_pointer_v<std::decay_t<Arg>> && std::is_invocable_v<Arg, Context&, Args&&...>;

		// Only one invocation type shall be valid, otherwise we don't know.
		static_assert((v1 + v2 + v3) == 1, "Ambiguous or invalid named_thread call.");

		if constexpr (v1)
		{
			using future = thread_future_t<Discard, Context&, Arg, Args...>;

			single_ptr<future> target = make_single<future>(*static_cast<Context*>(this), std::forward<Arg>(arg), std::forward<Args>(args)...);

			if constexpr (!Discard)
			{
				shared_ptr<future> result = std::move(target);

				// Copy result
				thread::push(result);
				return result;
			}
			else
			{
				// Move target
				thread::push(std::move(target));
				return;
			}
		}
		else if constexpr (v2)
		{
			using future = thread_future_t<Discard, Arg, Args...>;

			single_ptr<future> target = make_single<future>(std::forward<Arg>(arg), std::forward<Args>(args)...);

			if constexpr (!Discard)
			{
				shared_ptr<future> result = std::move(target);
				thread::push(result);
				return result;
			}
			else
			{
				thread::push(std::move(target));
				return;
			}
		}
		else if constexpr (v3)
		{
			using future = thread_future_t<Discard, Arg, Context&, Args...>;

			single_ptr<future> target = make_single<future>(std::forward<Arg>(arg), std::ref(*static_cast<Context*>(this)), std::forward<Args>(args)...);

			if constexpr (!Discard)
			{
				shared_ptr<future> result = std::move(target);
				thread::push(result);
				return result;
			}
			else
			{
				thread::push(std::move(target));
				return;
			}
		}
	}

	// Access thread state
	operator thread_state() const
	{
		return static_cast<thread_state>(thread::m_sync.load() & 3);
	}

	// Try to abort by assigning thread_state::aborting/finished
	// Join thread by thread_state::finished
	named_thread& operator=(thread_state s)
	{
		if constexpr (std::is_assignable_v<Context&, thread_state>)
		{
			static_cast<Context&>(*this) = s;
		}

		if (s >= thread_state::aborting && thread::m_sync.fetch_op([](u64& v){ return !(v & 3) && (v |= 1); }).second)
		{
			thread::m_sync.notify_one(1);
		}

		if (s == thread_state::finished)
		{
			// This participates in emulation stopping, use destruction-alike semantics
			thread::join(true);
		}

		return *this;
	}

	// Context type doesn't need virtual destructor
	~named_thread()
	{
		// Assign aborting state forcefully and join thread
		operator=(thread_state::finished);

		if constexpr (!result::empty)
		{
			result::destroy();
		}
	}
};

// Group of named threads, similar to named_thread
template <class Context>
class named_thread_group final
{
	using Thread = named_thread<Context>;

	const u32 m_count;

	Thread* m_threads;

	void init_threads()
	{
		m_threads = static_cast<Thread*>(::operator new(sizeof(Thread) * m_count, std::align_val_t{alignof(Thread)}));
	}

public:
	// Lambda constructor, also the implicit deduction guide candidate
	named_thread_group(std::string_view name, u32 count, const Context& f)
		: m_count(count)
		, m_threads(nullptr)
	{
		if (count == 0)
		{
			return;
		}

		init_threads();

		// Create all threads
		for (u32 i = 0; i < m_count; i++)
		{
			new (static_cast<void*>(m_threads + i)) Thread(std::string(name) + std::to_string(i + 1), f);
		}
	}

	// Default constructor
	named_thread_group(std::string_view name, u32 count)
		: m_count(count)
		, m_threads(nullptr)
	{
		if (count == 0)
		{
			return;
		}

		init_threads();

		// Create all threads
		for (u32 i = 0; i < m_count; i++)
		{
			new (static_cast<void*>(m_threads + i)) Thread(std::string(name) + std::to_string(i + 1));
		}
	}

	named_thread_group(const named_thread_group&) = delete;

	named_thread_group& operator=(const named_thread_group&) = delete;

	// Wait for completion
	bool join() const
	{
		bool result = true;

		for (u32 i = 0; i < m_count; i++)
		{
			std::as_const(*std::launder(m_threads + i))();

			if (std::as_const(*std::launder(m_threads + i)) != thread_state::finished)
				result = false;
		}

		return result;
	}

	// Join and access specific thread
	auto operator[](u32 index) const
	{
		return std::as_const(*std::launder(m_threads + index))();
	}

	// Join and access specific thread
	auto operator[](u32 index)
	{
		return (*std::launder(m_threads + index))();
	}

	// Dumb iterator
	auto begin()
	{
		return std::launder(m_threads);
	}

	// Dumb iterator
	auto end()
	{
		return m_threads + m_count;
	}

	u32 size() const
	{
		return m_count;
	}

	~named_thread_group()
	{
		// Destroy all threads (it should join them)
		for (u32 i = 0; i < m_count; i++)
		{
			std::launder(m_threads + i)->~Thread();
		}

		::operator delete(static_cast<void*>(m_threads), std::align_val_t{alignof(Thread)});
	}
};
