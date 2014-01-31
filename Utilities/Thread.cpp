#include "stdafx.h"
#include "Thread.h"

static DWORD g_tls_this_thread = 0xFFFFFFFF;

struct __init_tls
{
	//NamedThreadBase m_main_thr;

	__init_tls()
	{
		g_tls_this_thread = ::TlsAlloc();
		//m_main_thr.SetThreadName("Main Thread");
		//::TlsSetValue(g_tls_this_thread, &m_main_thr);
		::TlsSetValue(g_tls_this_thread, nullptr);
	}

	~__init_tls()
	{
		::TlsFree(g_tls_this_thread);
	}
} _init_tls;

NamedThreadBase* GetCurrentNamedThread()
{
	return (NamedThreadBase*)::TlsGetValue(g_tls_this_thread);
}

std::string NamedThreadBase::GetThreadName() const
{
	return m_name;
}

void NamedThreadBase::SetThreadName(const std::string& name)
{
	m_name = name;
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
}

void ThreadBase::Start()
{
	if(m_executor) Stop();

	std::lock_guard<std::mutex> lock(m_main_mutex);

	m_destroy = false;
	m_alive = true;

	m_executor = new std::thread(
		[this]()
		{
			::TlsSetValue(g_tls_this_thread, this);

			Task();

			m_alive = false;
		});
}

void ThreadBase::Stop(bool wait)
{
	std::lock_guard<std::mutex> lock(m_main_mutex);

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
	m_thr = std::thread([this, func]() { NamedThreadBase info(m_name); ::TlsSetValue(g_tls_this_thread, &info); func(); });
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