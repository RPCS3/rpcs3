#pragma once
#include "Emu/Memory/atomic_type.h"

static std::thread::id main_thread;

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

class thread
{
	std::string m_name;
	std::thread m_thr;

public:
	thread(const std::string& name, std::function<void()> func);
	thread(const std::string& name);
	thread();


public:
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

class waiter_map_t
{
	// TODO: optimize (use custom lightweight readers-writer lock)
	std::mutex m_mutex;

	struct waiter_t
	{
		u64 signal_id;
		NamedThreadBase* thread;
	};

	std::vector<waiter_t> m_waiters;

	std::string m_name;

	struct waiter_reg_t
	{
		NamedThreadBase* thread;
		const u64 signal_id;
		waiter_map_t& map;

		waiter_reg_t(waiter_map_t& map, u64 signal_id)
			: thread(nullptr)
			, signal_id(signal_id)
			, map(map)
		{
		}

		~waiter_reg_t();

		void init();
	};

	bool is_stopped(u64 signal_id);

public:
	waiter_map_t(const char* name)
		: m_name(name)
	{
	}

	// wait until waiter_func() returns true, signal_id is an arbitrary number
	template<typename WT> __forceinline void wait_op(u64 signal_id, const WT waiter_func)
	{
		// register waiter
		waiter_reg_t waiter(*this, signal_id);

		// check the condition or if the emulator is stopped
		while (!waiter_func() && !is_stopped(signal_id))
		{
			// initialize waiter (only once)
			waiter.init();
			// wait for 1 ms or until signal arrived
			waiter.thread->WaitForAnySignal(1);
		}
	}

	// signal all threads waiting on waiter_op() with the same signal_id (signaling only hints those threads that corresponding conditions are *probably* met)
	void notify(u64 signal_id);
};

bool squeue_test_exit(const volatile bool* do_exit);

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

	atomic_le_t<squeue_sync_var_t> m_sync;

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
	{
		m_sync.write_relaxed({});
	}

	u32 get_max_size() const
	{
		return sq_size;
	}

	bool is_full() const volatile
	{
		return m_sync.read_relaxed().count == sq_size;
	}

	bool push(const T& data, const volatile bool* do_exit = nullptr)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op_sync(SQSVR_OK, [&pos](squeue_sync_var_t& sync) -> u32
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
			if (res == SQSVR_FAILED && squeue_test_exit(do_exit))
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

	bool try_push(const T& data)
	{
		static const volatile bool no_wait = true;

		return push(data, &no_wait);
	}

	bool pop(T& data, const volatile bool* do_exit = nullptr)
	{
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op_sync(SQSVR_OK, [&pos](squeue_sync_var_t& sync) -> u32
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
			if (res == SQSVR_FAILED && squeue_test_exit(do_exit))
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

	bool try_pop(T& data)
	{
		static const volatile bool no_wait = true;

		return pop(data, &no_wait);
	}

	void clear()
	{
		while (m_sync.atomic_op_sync(SQSVR_OK, [](squeue_sync_var_t& sync) -> u32
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

	bool peek(T& data, u32 start_pos = 0, const volatile bool* do_exit = nullptr)
	{
		assert(start_pos < sq_size);
		u32 pos = 0;

		while (u32 res = m_sync.atomic_op_sync(SQSVR_OK, [&pos, start_pos](squeue_sync_var_t& sync) -> u32
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
			if (res == SQSVR_FAILED && squeue_test_exit(do_exit))
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

	bool try_peek(T& data, u32 start_pos = 0)
	{
		static const volatile bool no_wait = true;

		return peek(data, start_pos, &no_wait);
	}
};
