#pragma once

// Thread control class
class thread_ctrl final
{
	static thread_local thread_ctrl* g_tls_this_thread;

	// Name getter
	std::function<std::string()> m_name;

	// Thread handle (be careful)
	std::thread m_thread;

	// Thread result
	std::future<void> m_future;

	// Functions scheduled at thread exit
	std::deque<std::function<void()>> m_atexit;

	// Called at the thread start
	static void initialize();

	// Called at the thread end
	static void finalize() noexcept;

public:
	template<typename T>
	thread_ctrl(T&& name)
		: m_name(std::forward<T>(name))
	{
	}

	// Disable copy/move constructors and operators
	thread_ctrl(const thread_ctrl&) = delete;

	~thread_ctrl();

	// Get thread name
	std::string get_name() const;

	// Get future result (may throw)
	void join()
	{
		return m_future.get();
	}

	// Get current thread (may be nullptr)
	static const thread_ctrl* get_current()
	{
		return g_tls_this_thread;
	}

	// Register function at thread exit (for the current thread)
	template<typename T>
	static inline void at_exit(T&& func)
	{
		CHECK_ASSERTION(g_tls_this_thread);

		g_tls_this_thread->m_atexit.emplace_front(std::forward<T>(func));
	}

	// Named thread factory
	template<typename N, typename F>
	static inline std::shared_ptr<thread_ctrl> spawn(N&& name, F&& func)
	{
		auto ctrl = std::make_shared<thread_ctrl>(std::forward<N>(name));

		std::promise<void> promise;

		ctrl->m_future = promise.get_future();

		ctrl->m_thread = std::thread([ctrl, task = std::forward<F>(func)](std::promise<void> promise)
		{
			g_tls_this_thread = ctrl.get();

			try
			{
				initialize();
				task();
				finalize();
				promise.set_value();
			}
			catch (...)
			{
				finalize();
				promise.set_exception(std::current_exception());
			}

		}, std::move(promise));

		return ctrl;
	}
};

class named_thread_t : public std::enable_shared_from_this<named_thread_t>
{
	// Pointer to managed resource (shared with actual thread)
	std::shared_ptr<thread_ctrl> m_thread;

public:
	// Thread condition variable for external use (this thread waits on it, other threads may notify)
	std::condition_variable cv;

	// Thread mutex for external use (can be used with `cv`)
	std::mutex mutex;

protected:
	// Thread task (called in the thread)
	virtual void on_task() = 0;

	// Thread finalization (called after on_task)
	virtual void on_exit() {}

	// ID initialization (called through id_aux_initialize)
	virtual void on_id_aux_initialize() { start(); }

	// ID finalization (called through id_aux_finalize)
	virtual void on_id_aux_finalize() { join(); }

public:
	named_thread_t() = default;

	virtual ~named_thread_t() = default;

	// Deleted copy/move constructors + copy/move operators
	named_thread_t(const named_thread_t&) = delete;

	// Get thread name
	virtual std::string get_name() const;

	// Start thread (cannot be called from the constructor: should throw bad_weak_ptr in such case)
	void start();

	// Join thread (get future result)
	void join();

	// Check whether the thread is not in "empty state"
	bool is_started() const { return m_thread.operator bool(); }

	// Compare with the current thread
	bool is_current() const { CHECK_ASSERTION(m_thread); return thread_ctrl::get_current() == m_thread.get(); }

	// Get thread_ctrl
	const thread_ctrl* get_thread_ctrl() const { return m_thread.get(); }

	friend void id_aux_initialize(named_thread_t* ptr) { ptr->on_id_aux_initialize(); }
	friend void id_aux_finalize(named_thread_t* ptr) { ptr->on_id_aux_finalize(); }
};

// Wrapper for named thread, joins automatically in the destructor, can only be used in function scope
class scope_thread_t final
{
	std::shared_ptr<thread_ctrl> m_thread;

public:
	template<typename N, typename F>
	scope_thread_t(N&& name, F&& func)
		: m_thread(thread_ctrl::spawn(std::forward<N>(name), std::forward<F>(func)))
	{
	}

	// Deleted copy/move constructors + copy/move operators
	scope_thread_t(const scope_thread_t&) = delete;

	// Destructor with exceptions allowed
	~scope_thread_t() noexcept(false)
	{
		m_thread->join();
	}
};

extern const std::function<bool()> SQUEUE_ALWAYS_EXIT;
extern const std::function<bool()> SQUEUE_NEVER_EXIT;

bool squeue_test_exit();

template<typename T, u32 sq_size = 256>
class squeue_t
{
	struct squeue_sync_var_t
	{
		struct
		{
			u32 position : 31;
			u32 pop_lock : 1;
		};
		struct
		{
			u32 count : 31;
			u32 push_lock : 1;
		};
	};

	atomic_t<squeue_sync_var_t> m_sync;

	mutable std::mutex m_rcv_mutex;
	mutable std::mutex m_wcv_mutex;
	mutable std::condition_variable m_rcv;
	mutable std::condition_variable m_wcv;

	T m_data[sq_size];

	enum squeue_sync_var_result : u32
	{
		SQSVR_OK = 0,
		SQSVR_LOCKED = 1,
		SQSVR_FAILED = 2,
	};

public:
	squeue_t()
		: m_sync(squeue_sync_var_t{})
	{
	}

	u32 get_max_size() const
	{
		return sq_size;
	}

	bool is_full() const
	{
		return m_sync.load().count == sq_size;
	}

	bool push(const T& data, const std::function<bool()>& test_exit)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);

			if (sync.push_lock)
			{
				return SQSVR_LOCKED;
			}
			if (sync.count == sq_size)
			{
				return SQSVR_FAILED;
			}

			sync.push_lock = 1;
			pos = sync.position + sync.count;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> wcv_lock(m_wcv_mutex);
			m_wcv.wait_for(wcv_lock, std::chrono::milliseconds(1));
		}

		m_data[pos >= sq_size ? pos - sq_size : pos] = data;

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);
			assert(sync.push_lock);
			sync.push_lock = 0;
			sync.count++;
		});

		m_rcv.notify_one();
		m_wcv.notify_one();
		return true;
	}

	bool push(const T& data, const volatile bool* do_exit)
	{
		return push(data, [do_exit](){ return do_exit && *do_exit; });
	}

	force_inline bool push(const T& data)
	{
		return push(data, SQUEUE_NEVER_EXIT);
	}

	force_inline bool try_push(const T& data)
	{
		return push(data, SQUEUE_ALWAYS_EXIT);
	}

	bool pop(T& data, const std::function<bool()>& test_exit)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos](squeue_sync_var_t& sync) -> u32
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);

			if (!sync.count)
			{
				return SQSVR_FAILED;
			}
			if (sync.pop_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			pos = sync.position;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		data = m_data[pos];

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);
			assert(sync.pop_lock);
			sync.pop_lock = 0;
			sync.position++;
			sync.count--;
			if (sync.position == sq_size)
			{
				sync.position = 0;
			}
		});

		m_rcv.notify_one();
		m_wcv.notify_one();
		return true;
	}

	bool pop(T& data, const volatile bool* do_exit)
	{
		return pop(data, [do_exit](){ return do_exit && *do_exit; });
	}

	force_inline bool pop(T& data)
	{
		return pop(data, SQUEUE_NEVER_EXIT);
	}

	force_inline bool try_pop(T& data)
	{
		return pop(data, SQUEUE_ALWAYS_EXIT);
	}

	bool peek(T& data, u32 start_pos, const std::function<bool()>& test_exit)
	{
		assert(start_pos < sq_size);
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op([&pos, start_pos](squeue_sync_var_t& sync) -> u32
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);

			if (sync.count <= start_pos)
			{
				return SQSVR_FAILED;
			}
			if (sync.pop_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			pos = sync.position + start_pos;
			return SQSVR_OK;
		}))
		{
			if (res == SQSVR_FAILED && (test_exit() || squeue_test_exit()))
			{
				return false;
			}

			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		data = m_data[pos >= sq_size ? pos - sq_size : pos];

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);
			assert(sync.pop_lock);
			sync.pop_lock = 0;
		});

		m_rcv.notify_one();
		return true;
	}

	bool peek(T& data, u32 start_pos, const volatile bool* do_exit)
	{
		return peek(data, start_pos, [do_exit](){ return do_exit && *do_exit; });
	}

	force_inline bool peek(T& data, u32 start_pos = 0)
	{
		return peek(data, start_pos, SQUEUE_NEVER_EXIT);
	}

	force_inline bool try_peek(T& data, u32 start_pos = 0)
	{
		return peek(data, start_pos, SQUEUE_ALWAYS_EXIT);
	}

	class squeue_data_t
	{
		T* const m_data;
		const u32 m_pos;
		const u32 m_count;

		squeue_data_t(T* data, u32 pos, u32 count)
			: m_data(data)
			, m_pos(pos)
			, m_count(count)
		{
		}

	public:
		T& operator [] (u32 index)
		{
			assert(index < m_count);
			index += m_pos;
			index = index < sq_size ? index : index - sq_size;
			return m_data[index];
		}
	};

	void process(void(*proc)(squeue_data_t data))
	{
		u32 pos, count;

		while (m_sync.atomic_op([&pos, &count](squeue_sync_var_t& sync) -> u32
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);

			if (sync.pop_lock || sync.push_lock)
			{
				return SQSVR_LOCKED;
			}

			pos = sync.position;
			count = sync.count;
			sync.pop_lock = 1;
			sync.push_lock = 1;
			return SQSVR_OK;
		}))
		{
			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		proc(squeue_data_t(m_data, pos, count));

		m_sync.atomic_op([](squeue_sync_var_t& sync)
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);
			assert(sync.pop_lock && sync.push_lock);
			sync.pop_lock = 0;
			sync.push_lock = 0;
		});

		m_wcv.notify_one();
		m_rcv.notify_one();
	}

	void clear()
	{
		while (m_sync.atomic_op([](squeue_sync_var_t& sync) -> u32
		{
			assert(sync.count <= sq_size);
			assert(sync.position < sq_size);

			if (sync.pop_lock || sync.push_lock)
			{
				return SQSVR_LOCKED;
			}

			sync.pop_lock = 1;
			sync.push_lock = 1;
			return SQSVR_OK;
		}))
		{
			std::unique_lock<std::mutex> rcv_lock(m_rcv_mutex);
			m_rcv.wait_for(rcv_lock, std::chrono::milliseconds(1));
		}

		m_sync.exchange({});
		m_wcv.notify_one();
		m_rcv.notify_one();
	}
};
