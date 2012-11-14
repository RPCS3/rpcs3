#include "stdafx.h"
#include "Thread.h"

void ThreadBase::Start()
{
	if(m_executor) return;

	m_executor = new ThreadExec(m_detached, this);
}

void ThreadBase::Stop(bool wait)
{
	if(!m_executor) return;
	ThreadExec* exec = m_executor;
	m_executor = nullptr;
	exec->Stop(wait);
}

bool ThreadBase::IsAlive()
{
	return m_executor != nullptr;
}

bool ThreadBase::TestDestroy()
{
	if(!m_executor) return true;

	return m_executor->TestDestroy();
}