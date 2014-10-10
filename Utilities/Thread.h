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

// for internal use (checks if Emu is stopped)
bool waiter_is_stopped(const char* func_name, u64 signal_id);
// for internal use
void waiter_register(u64 signal_id, NamedThreadBase* thread);
// for internal use
void waiter_unregister(u64 signal_id, NamedThreadBase* thread);

// wait until waiter_func() returns true, signal_id is arbitrary number
template<typename WT> static __forceinline void waiter_op(const char* func_name, u64 signal_id, const WT waiter_func)
{
	if (waiter_func()) return;

	NamedThreadBase* thread = GetCurrentNamedThread();
	waiter_register(signal_id, thread);

	while (true)
	{
		thread->WaitForAnySignal(1);
		if (waiter_is_stopped(func_name, signal_id)) break;
		if (waiter_func()) break;
	}

	waiter_unregister(signal_id, thread);
}

// signal all threads waiting on waiter_op() with the same signal_id (signaling only hints those threads that corresponding conditions are *probably* met)
void waiter_signal(u64 signal_id);
