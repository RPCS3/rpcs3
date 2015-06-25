#pragma once

class NamedThreadBase
{
	std::string m_name;
	std::condition_variable m_signal_cv;
	std::mutex m_signal_mtx;

public:
	std::atomic<bool> m_tls_assigned;

	NamedThreadBase(const std::string& name) : m_name(name), m_tls_assigned(false)
	{
	}

	NamedThreadBase() : m_tls_assigned(false)
	{
	}

	virtual std::string GetThreadName() const;
	virtual void SetThreadName(const std::string& name);

	void WaitForAnySignal(u64 time = 1);
	void Notify();

	virtual void DumpInformation() {}
};

NamedThreadBase* GetCurrentNamedThread();
void SetCurrentNamedThread(NamedThreadBase* value);

class ThreadBase : public NamedThreadBase
{
protected:
	std::atomic<bool> m_destroy;
	std::atomic<bool> m_alive;
	std::thread* m_executor;

	mutable std::mutex m_main_mutex;

	ThreadBase(const std::string& name);
	~ThreadBase();

public:
	void Start();
	void Stop(bool wait = true, bool send_destroy = true);

	bool Join() const;
	bool IsAlive() const;
	bool TestDestroy() const;

	virtual void Task() = 0;
};

class thread_t
{
	enum thread_state_t
	{
		TS_NON_EXISTENT,
		TS_JOINABLE,
	};

	std::atomic<thread_state_t> m_state;
	std::string m_name;
	std::thread m_thr;
	bool m_autojoin;

public:
	thread_t(const std::string& name, bool autojoin, std::function<void()> func);
	thread_t(const std::string& name, std::function<void()> func);
	thread_t(const std::string& name);
	thread_t();
	~thread_t();

	thread_t(const thread_t& right) = delete;
	thread_t(thread_t&& right) = delete;

	thread_t& operator =(const thread_t& right) = delete;
	thread_t& operator =(thread_t&& right) = delete;

public:
	void set_name(const std::string& name);
	void start(std::function<void()> func);
	void detach();
	void join();
	bool joinable() const;
};

class slw_mutex_t
{

};

class slw_recursive_mutex_t
{

};

class slw_shared_mutex_t
{

};

struct waiter_map_t
{
	static const size_t size = 16;

	std::array<std::mutex, size> mutex;
	std::array<std::condition_variable, size> cv;

	const std::string name;

	waiter_map_t(const char* name)
		: name(name)
	{
	}

	// generate simple "hash" for mutex/cv distribution
	u32 get_hash(u32 addr)
	{
		addr ^= addr >> 16;
		addr ^= addr >> 24;
		addr ^= addr >> 28;
		return addr % size;
	}

	// check emu status
	bool is_stopped(u32 addr);

	// wait until waiter_func() returns true, signal_id is an arbitrary number
	template<typename F, typename... Args> safe_buffers auto wait_op(u32 addr, F pred, Args&&... args) -> decltype(static_cast<void>(pred(args...)))
	{
		const u32 hash = get_hash(addr);

		// set mutex locker
		std::unique_lock<std::mutex> lock(mutex[hash], std::defer_lock);

		while (true)
		{
			// check the condition
			if (pred(args...)) return;

			// lock the mutex and initialize waiter (only once)
			if (!lock) lock.lock();
			
			// wait on appropriate condition variable for 1 ms or until signal arrived
			cv[hash].wait_for(lock, std::chrono::milliseconds(1));

			if (is_stopped(addr)) return;
		}
	}

	// signal all threads waiting on waiter_op() with the same signal_id (signaling only hints those threads that corresponding conditions are *probably* met)
	void notify(u32 addr);
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

	atomic<squeue_sync_var_t> m_sync;

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
		: m_sync({})
	{
	}

	u32 get_max_size() const
	{
		return sq_size;
	}

	bool is_full() const volatile
	{
		return m_sync.data.count == sq_size;
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
