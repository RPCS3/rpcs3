#pragma once

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

class s_mutex_t
{

};

class s_shared_mutex_t
{

};

class s_cond_var_t
{
	
//public:
//	s_cond_var_t();
//	~s_cond_var_t();
//
//	s_cond_var_t(s_cond_var_t& right) = delete;
//	s_cond_var_t& operator = (s_cond_var_t& right) = delete;
//
//	void wait();
//	void wait_for();
//
//	void notify();
//	void notify_all();
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

		// check condition or if emulator is stopped
		while (!waiter_func() && !is_stopped(signal_id))
		{
			// initialize waiter (only first time)
			waiter.init();
			// wait for 1 ms or until signal arrived
			waiter.thread->WaitForAnySignal(1);
		}
	}

	// signal all threads waiting on waiter_op() with the same signal_id (signaling only hints those threads that corresponding conditions are *probably* met)
	void notify(u64 signal_id);
};
