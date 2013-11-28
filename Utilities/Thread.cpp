#include "stdafx.h"
#include "Thread.h"

ThreadBase* GetCurrentNamedThread()
{
	ThreadExec* thr = (ThreadExec*)::wxThread::This();
	return thr ? thr->m_parent : nullptr;
}

ThreadBase::ThreadBase(bool detached, const std::string& name)
	: m_detached(detached)
	, m_name(name)
	, m_executor(nullptr)
{
}

void ThreadBase::Start()
{
	if(m_executor) return;

	m_executor = new ThreadExec(m_detached, this);
}

void ThreadBase::Resume()
{
	if(m_executor)
	{
		m_executor->Resume();
	}
}

void ThreadBase::Pause()
{
	if(m_executor)
	{
		m_executor->Pause();
	}
}

void ThreadBase::Stop(bool wait)
{
	if(!m_executor) return;
	ThreadExec* exec = m_executor;
	m_executor = nullptr;

	if(!m_detached)
	{
		if(wait)
		{
			exec->Wait();
		}

		exec->Stop(false);
		delete exec;
	}
	else
	{
		exec->Stop(wait);
	}
}

bool ThreadBase::Wait() const
{
	return m_executor != nullptr && m_executor->Wait() != (wxThread::ExitCode)-1;
}

bool ThreadBase::IsRunning() const
{
	return m_executor != nullptr && m_executor->IsRunning();
}

bool ThreadBase::IsPaused() const
{
	return m_executor != nullptr && m_executor->IsPaused();
}

bool ThreadBase::IsAlive() const
{
	return m_executor != nullptr;
}

bool ThreadBase::TestDestroy() const
{
	if(!m_executor || !m_executor->m_parent) return true;

	return m_executor->TestDestroy();
}

std::string ThreadBase::GetThreadName() const
{
	return m_name;
}

void ThreadBase::SetThreadName(const std::string& name)
{
	m_name = name;
}