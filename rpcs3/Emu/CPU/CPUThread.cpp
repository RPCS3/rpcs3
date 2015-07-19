#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/IdManager.h"
#include "Emu/DbgCommand.h"

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
		while (joinable() && is_alive())
		{
			CHECK_EMU_STATUS;

			// check stop status
			if (!is_stopped())
			{
				if (lock) lock.unlock();

				try
				{
					task();
				}
				catch (CPUThreadReturn)
				{
					;
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
				catch (const fmt::exception&)
				{
					dump_info();
					throw;
				}

				m_state &= ~CPU_STATE_RETURN;
				continue;
			}

			if (!lock)
			{
				lock.lock();
				continue;
			}

			cv.wait(lock);
		}
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

bool CPUThread::is_paused() const
{
	return (m_state.load() & CPU_STATE_PAUSED) != 0 || Emu.IsPaused();
}

void CPUThread::dump_info() const
{
	if (!Emu.IsStopped())
	{
		LOG_NOTICE(GENERAL, RegsToString());
	}
}

void CPUThread::run()
{
	SendDbgCommand(DID_START_THREAD, this);

	init_stack();
	init_regs();
	do_run();

	SendDbgCommand(DID_STARTED_THREAD, this);
}

void CPUThread::pause()
{
	SendDbgCommand(DID_PAUSE_THREAD, this);

	m_state |= CPU_STATE_PAUSED;

	SendDbgCommand(DID_PAUSED_THREAD, this);
}

void CPUThread::resume()
{
	SendDbgCommand(DID_RESUME_THREAD, this);

	{
		// lock for reliable notification
		std::lock_guard<std::mutex> lock(mutex);

		m_state &= ~CPU_STATE_PAUSED;

		cv.notify_one();
	}

	SendDbgCommand(DID_RESUMED_THREAD, this);
}

void CPUThread::stop()
{
	SendDbgCommand(DID_STOP_THREAD, this);

	if (is_current())
	{
		throw CPUThreadStop{};
	}
	else
	{
		// lock for reliable notification
		std::lock_guard<std::mutex> lock(mutex);

		m_state |= CPU_STATE_STOPPED;

		cv.notify_one();
	}

	SendDbgCommand(DID_STOPED_THREAD, this);
}

void CPUThread::exec()
{
	SendDbgCommand(DID_EXEC_THREAD, this);

	{
		// lock for reliable notification
		std::lock_guard<std::mutex> lock(mutex);

		m_state &= ~CPU_STATE_STOPPED;

		cv.notify_one();
	}
}

void CPUThread::exit()
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

void CPUThread::step()
{
	if (m_state.atomic_op([](u64& state) -> bool
	{
		const bool was_paused = (state & CPU_STATE_PAUSED) != 0;

		state |= CPU_STATE_STEP;
		state &= ~CPU_STATE_PAUSED;

		return was_paused;
	}))
	{
		if (is_current()) return;

		// lock for reliable notification (only if PAUSE was removed)
		std::lock_guard<std::mutex> lock(mutex);

		cv.notify_one();
	}
}

void CPUThread::sleep()
{
	m_state += CPU_STATE_MAX;
	m_state |= CPU_STATE_SLEEP;
}

void CPUThread::awake()
{
	// must be called after the balanced Sleep() call

	if (m_state.atomic_op([](u64& state) -> bool
	{
		if (state < CPU_STATE_MAX)
		{
			throw EXCEPTION("Sleep()/Awake() inconsistency");
		}

		if ((state -= CPU_STATE_MAX) < CPU_STATE_MAX)
		{
			state &= ~CPU_STATE_SLEEP;

			// notify the condition variable as well
			return true;
		}

		return false;
	}))
	{
		if (is_current()) return;

		// lock for reliable notification; the condition being checked is probably externally set
		std::lock_guard<std::mutex> lock(mutex);

		cv.notify_one();
	}
}

bool CPUThread::signal()
{
	// try to set SIGNAL
	if (m_state._or(CPU_STATE_SIGNAL) & CPU_STATE_SIGNAL)
	{
		return false;
	}
	else
	{
		// not truly responsible for signal delivery, requires additional measures like LV2_LOCK
		cv.notify_one();

		return true;
	}
}

bool CPUThread::unsignal()
{
	// remove SIGNAL and return its old value
	return (m_state._and_not(CPU_STATE_SIGNAL) & CPU_STATE_SIGNAL) != 0;
}

bool CPUThread::check_status()
{
	std::unique_lock<std::mutex> lock(mutex, std::defer_lock);

	while (true)
	{
		CHECK_EMU_STATUS; // check at least once

		if (!is_paused()) break;

		if (!lock)
		{
			lock.lock();
			continue;
		}

		cv.wait(lock);
	}

	if (m_state.load() & CPU_STATE_RETURN || is_stopped())
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
