#include "stdafx.h"
#include "Log.h"
#include "Thread.h"

thread_local NamedThreadBase* g_tls_this_thread = nullptr;
std::atomic<u32> g_thread_count(0);

NamedThreadBase* GetCurrentNamedThread()
{
	return g_tls_this_thread;
}

void SetCurrentNamedThread(NamedThreadBase* value)
{
	auto old_value = g_tls_this_thread;

	if (old_value == value)
	{
		return;
	}

	if (value && value->m_tls_assigned.exchange(true))
	{
		LOG_ERROR(GENERAL, "Thread '%s' was already assigned to g_tls_this_thread of another thread", value->GetThreadName().c_str());
		g_tls_this_thread = nullptr;
	}
	else
	{
		g_tls_this_thread = value;
	}

	if (old_value)
	{
		old_value->m_tls_assigned = false;
	}
}

std::string NamedThreadBase::GetThreadName() const
{
	return m_name;
}

void NamedThreadBase::SetThreadName(const std::string& name)
{
	m_name = name;
}

void NamedThreadBase::WaitForAnySignal(u64 time) // wait for Notify() signal or sleep
{
	std::unique_lock<std::mutex> lock(m_signal_mtx);
	m_signal_cv.wait_for(lock, std::chrono::milliseconds(time));
}

void NamedThreadBase::Notify() // wake up waiting thread or nothing
{
	m_signal_cv.notify_one();
}

ThreadBase::ThreadBase(const std::string& name)
	: NamedThreadBase(name)
	, m_executor(nullptr)
	, m_destroy(false)
	, m_alive(false)
{
}

ThreadBase::~ThreadBase()
{
	if(IsAlive())
		Stop(false);

	delete m_executor;
	m_executor = nullptr;
}

void ThreadBase::Start()
{
	if(m_executor) Stop();

	std::lock_guard<std::mutex> lock(m_main_mutex);

	m_destroy = false;
	m_alive = true;

	m_executor = new std::thread([this]()
	{
		SetCurrentNamedThread(this);
		g_thread_count++;

		try
		{
			Task();
		}
		catch (const char* e)
		{
			LOG_ERROR(GENERAL, "%s: %s", GetThreadName().c_str(), e);
		}
		catch (const std::string& e)
		{
			LOG_ERROR(GENERAL, "%s: %s", GetThreadName().c_str(), e.c_str());
		}

		m_alive = false;
		SetCurrentNamedThread(nullptr);
		g_thread_count--;
	});
}

void ThreadBase::Stop(bool wait, bool send_destroy)
{
	std::lock_guard<std::mutex> lock(m_main_mutex);

	if (send_destroy)
		m_destroy = true;

	if(!m_executor)
		return;

	if(wait && m_executor->joinable() && m_alive)
	{
		m_executor->join();
	}
	else
	{
		m_executor->detach();
	}

	delete m_executor;
	m_executor = nullptr;
}

bool ThreadBase::Join() const
{
	std::lock_guard<std::mutex> lock(m_main_mutex);
	if(m_executor->joinable() && m_alive && m_executor != nullptr)
	{
		m_executor->join();
		return true;
	}

	return false;
}

bool ThreadBase::IsAlive() const
{
	std::lock_guard<std::mutex> lock(m_main_mutex);
	return m_alive;
}

bool ThreadBase::TestDestroy() const
{
	return m_destroy;
}

thread::thread(const std::string& name, std::function<void()> func) : m_name(name)
{
	start(func);
}

thread::thread(const std::string& name) : m_name(name)
{
}

thread::thread()
{
}

void thread::start(std::function<void()> func)
{
	std::string name = m_name;

	m_thr = std::thread([func, name]()
	{
		NamedThreadBase info(name);
		SetCurrentNamedThread(&info);
		g_thread_count++;

		try
		{
			func();
		}
		catch (const char* e)
		{
			LOG_ERROR(GENERAL, "%s: %s", name.c_str(), e);
		}
		catch (const std::string& e)
		{
			LOG_ERROR(GENERAL, "%s: %s", name.c_str(), e.c_str());
		}

		SetCurrentNamedThread(nullptr);
		g_thread_count--;
	});
}

void thread::detach()
{
	m_thr.detach();
}

void thread::join()
{
	m_thr.join();
}

bool thread::joinable() const
{
	return m_thr.joinable();
}
