#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/DbgCommand.h"

#include "CPUThreadManager.h"
#include "CPUDecoder.h"
#include "CPUThread.h"

CPUThread::CPUThread(CPUThreadType type, const std::string& name, std::function<std::string()> thread_name)
	: m_state({ CPU_STATE_STOPPED })
	, m_id(Emu.GetIdManager().get_current_id())
	, m_type(type)
	, m_name(name)
{
	start(std::move(thread_name), [this]
	{
		SendDbgCommand(DID_CREATE_THREAD, this);

		std::unique_lock<std::mutex> lock(mutex);

		// check thread status
		while (joinable() && IsActive())
		{
			CHECK_EMU_STATUS;

			// check stop status
			if (!IsStopped())
			{
				if (lock) lock.unlock();

				try
				{
					Task();
				}
				catch (CPUThreadReturn)
				{
				}
				catch (CPUThreadStop)
				{
					m_state |= CPU_STATE_STOPPED;
				}
				catch (CPUThreadExit)
				{
					m_state |= CPU_STATE_DEAD;
					break;
				}

				m_state &= ~CPU_STATE_RETURN;
				cv.notify_one();
				continue;
			}

			if (!lock) lock.lock();

			cv.wait_for(lock, std::chrono::milliseconds(1));
		}

		cv.notify_all();
	});
}

CPUThread::~CPUThread()
{
	if (joinable())
	{
		throw EXCEPTION("Thread not joined");
	}

	SendDbgCommand(DID_REMOVE_THREAD, this);
}

bool CPUThread::IsPaused() const
{
	return (m_state.load() & CPU_STATE_PAUSED) != 0 || Emu.IsPaused();
}

void CPUThread::DumpInformation() const
{
	LOG_WARNING(GENERAL, RegsToString());
}

void CPUThread::Run()
{
	SendDbgCommand(DID_START_THREAD, this);

	InitStack();
	InitRegs();
	DoRun();

	SendDbgCommand(DID_STARTED_THREAD, this);
}

void CPUThread::Resume()
{
	SendDbgCommand(DID_RESUME_THREAD, this);

	m_state &= ~CPU_STATE_PAUSED;

	cv.notify_one();

	SendDbgCommand(DID_RESUMED_THREAD, this);
}

void CPUThread::Pause()
{
	SendDbgCommand(DID_PAUSE_THREAD, this);

	m_state |= CPU_STATE_PAUSED;

	cv.notify_one();

	SendDbgCommand(DID_PAUSED_THREAD, this);
}

void CPUThread::Stop()
{
	SendDbgCommand(DID_STOP_THREAD, this);

	if (is_current())
	{
		throw CPUThreadStop{};
	}
	else
	{
		m_state |= CPU_STATE_STOPPED;

		cv.notify_one();
	}

	SendDbgCommand(DID_STOPED_THREAD, this);
}

void CPUThread::Exec()
{
	SendDbgCommand(DID_EXEC_THREAD, this);

	m_state &= ~CPU_STATE_STOPPED;

	cv.notify_one();
}

void CPUThread::Exit()
{
	if (is_current())
	{
		throw CPUThreadExit{};
	}
	else
	{
		throw EXCEPTION("Unable to exit another thread");
	}
}

void CPUThread::Step()
{
	m_state.atomic_op([](u64& state)
	{
		state |= CPU_STATE_STEP;
		state &= ~CPU_STATE_PAUSED;
	});

	cv.notify_one();
}

void CPUThread::Sleep()
{
	m_state += CPU_STATE_MAX;
	m_state |= CPU_STATE_SLEEP;

	cv.notify_one();
}

void CPUThread::Awake()
{
	// must be called after the corresponding Sleep() call

	m_state.atomic_op([](u64& state)
	{
		if (state < CPU_STATE_MAX)
		{
			throw EXCEPTION("Sleep()/Awake() inconsistency");
		}

		if ((state -= CPU_STATE_MAX) < CPU_STATE_MAX)
		{
			state &= ~CPU_STATE_SLEEP;
		}
	});

	cv.notify_one();
}

bool CPUThread::CheckStatus()
{
	std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

	while (true)
	{
		CHECK_EMU_STATUS; // check at least once

		if (!IsPaused()) break;

		if (!lock) lock.lock();

		cv.wait_for(lock, std::chrono::milliseconds(1));
	}

	if (m_state.load() & CPU_STATE_RETURN || IsStopped())
	{
		return true;
	}

	if (m_state.load() & CPU_STATE_STEP)
	{
		// set PAUSE, but allow to execute once
		m_state |= CPU_STATE_PAUSED;
		m_state &= ~CPU_STATE_STEP;
	}

	return false;
}
