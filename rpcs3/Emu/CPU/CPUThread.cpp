#include "stdafx.h"
#include "rpcs3/Ini.h"
#include "Emu/SysCalls/SysCalls.h"
#include "Utilities/Log.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"
#include "Emu/DbgCommand.h"

#include "CPUDecoder.h"
#include "CPUThread.h"

CPUThread* GetCurrentCPUThread()
{
	return (CPUThread*)GetCurrentNamedThread();
}

CPUThread::CPUThread(CPUThreadType type)
	: ThreadBase("CPUThread")
	, m_type(type)
	, m_stack_size(0)
	, m_stack_addr(0)
	, m_offset(0)
	, m_prio(0)
	, m_sync_wait(false)
	, m_wait_thread_id(-1)
	, m_dec(nullptr)
	, m_is_step(false)
	, m_is_branch(false)
	, m_status(Stopped)
	, m_last_syscall(0)
{
}

CPUThread::~CPUThread()
{
	safe_delete(m_dec);
}

bool CPUThread::IsRunning() const { return m_status == Running; }
bool CPUThread::IsPaused() const { return m_status == Paused; }
bool CPUThread::IsStopped() const { return m_status == Stopped; }

void CPUThread::Close()
{
	ThreadBase::Stop(m_sync_wait);
	DoStop();

	delete m_dec;
	m_dec = nullptr;
}

void CPUThread::Reset()
{
	CloseStack();

	m_sync_wait = 0;
	m_wait_thread_id = -1;

	SetPc(0);
	cycle = 0;
	m_is_branch = false;

	m_status = Stopped;
	m_error = 0;
	
	DoReset();
}

void CPUThread::CloseStack()
{
	if(m_stack_addr)
	{
		Memory.StackMem.Free(m_stack_addr);
		m_stack_addr = 0;
	}

	m_stack_size = 0;
	m_stack_point = 0;
}

void CPUThread::SetId(const u32 id)
{
	m_id = id;
}

void CPUThread::SetName(const std::string& name)
{
	NamedThreadBase::SetThreadName(name);
}

void CPUThread::Wait(bool wait)
{
	std::lock_guard<std::mutex> lock(m_cs_sync);
	m_sync_wait = wait;
}

void CPUThread::Wait(const CPUThread& thr)
{
	std::lock_guard<std::mutex> lock(m_cs_sync);
	m_wait_thread_id = thr.GetId();
	m_sync_wait = true;
}

bool CPUThread::Sync()
{
	return m_sync_wait;
}

int CPUThread::ThreadStatus()
{
	if(Emu.IsStopped() || IsStopped() || IsPaused())
	{
		return CPUThread_Stopped;
	}

	if(TestDestroy())
	{
		return CPUThread_Break;
	}

	if(m_is_step)
	{
		return CPUThread_Step;
	}

	if (Emu.IsPaused() || Sync())
	{
		return CPUThread_Sleeping;
	}

	return CPUThread_Running;
}

void CPUThread::SetEntry(const u64 pc)
{
	entry = pc;
}

void CPUThread::NextPc(u8 instr_size)
{
	if(m_is_branch)
	{
		m_is_branch = false;

		SetPc(nPC);
	}
	else
	{
		PC += instr_size;
	}
}

void CPUThread::SetBranch(const u64 pc, bool record_branch)
{
	m_is_branch = true;
	nPC = pc;

	if(record_branch)
		CallStackBranch(pc);
}

void CPUThread::SetPc(const u64 pc)
{
	PC = pc;
}

void CPUThread::SetError(const u32 error)
{
	if(error == 0)
	{
		m_error = 0;
	}
	else
	{
		m_error |= error;
	}
}

std::vector<std::string> CPUThread::ErrorToString(const u32 error)
{
	std::vector<std::string> earr;

	if(error == 0) return earr;

	earr.push_back("Unknown error");

	return earr;
}

void CPUThread::Run()
{
	if(!IsStopped())
		Stop();

	Reset();
	
	SendDbgCommand(DID_START_THREAD, this);

	m_status = Running;

	SetPc(entry);
	InitStack();
	InitRegs();
	DoRun();
	Emu.CheckStatus();

	SendDbgCommand(DID_STARTED_THREAD, this);
}

void CPUThread::Resume()
{
	if(!IsPaused()) return;

	SendDbgCommand(DID_RESUME_THREAD, this);

	m_status = Running;
	DoResume();
	Emu.CheckStatus();

	ThreadBase::Start();

	SendDbgCommand(DID_RESUMED_THREAD, this);
}

void CPUThread::Pause()
{
	if(!IsRunning()) return;

	SendDbgCommand(DID_PAUSE_THREAD, this);

	m_status = Paused;
	DoPause();
	Emu.CheckStatus();

	// ThreadBase::Stop(); // "Abort() called" exception
	SendDbgCommand(DID_PAUSED_THREAD, this);
}

void CPUThread::Stop()
{
	if(IsStopped()) return;

	SendDbgCommand(DID_STOP_THREAD, this);

	m_status = Stopped;

	if(CPUThread* thr = GetCurrentCPUThread())
	{
		if(thr->GetId() != GetId())
			ThreadBase::Stop();
	}
	else
		ThreadBase::Stop();

	Emu.CheckStatus();

	SendDbgCommand(DID_STOPED_THREAD, this);
}

void CPUThread::Exec()
{
	m_is_step = false;
	SendDbgCommand(DID_EXEC_THREAD, this);

	if(IsRunning())
		ThreadBase::Start();
}

void CPUThread::ExecOnce()
{
	m_is_step = true;
	SendDbgCommand(DID_EXEC_THREAD, this);

	m_status = Running;
	ThreadBase::Start();
	ThreadBase::Stop(true,false);
	m_status = Paused;
	SendDbgCommand(DID_PAUSE_THREAD, this);
	SendDbgCommand(DID_PAUSED_THREAD, this);
}

#ifdef _WIN32
void _se_translator(unsigned int u, EXCEPTION_POINTERS* pExp)
{
	const u64 addr = (u64)pExp->ExceptionRecord->ExceptionInformation[1] - (u64)Memory.GetBaseAddr();
	if (u == EXCEPTION_ACCESS_VIOLATION && addr < 0x100000000)
	{
		// TODO: allow recovering from a page fault
		throw fmt::Format("Access violation: addr = 0x%x (last_syscall=0x%llx (%s))",
			(u32)addr, (u64)GetCurrentCPUThread()->m_last_syscall, SysCalls::GetHLEFuncName((u64)GetCurrentCPUThread()->m_last_syscall).c_str());
	}
	else
	{
		// some fatal error (should crash)
		return;
	}
}
#else
// TODO: linux version
#endif

void CPUThread::Task()
{
	if (Ini.HLELogging.GetValue()) LOG_NOTICE(PPU, "%s enter", CPUThread::GetFName().c_str());

	const std::vector<u64>& bp = Emu.GetBreakPoints();

	for (uint i = 0; i<bp.size(); ++i)
	{
		if (bp[i] == m_offset + PC)
		{
			Emu.Pause();
			break;
		}
	}

	std::vector<u64> trace;

#ifdef _WIN32
	_set_se_translator(_se_translator);
#else
	// TODO: linux version
#endif

	try
	{
		while (true)
		{
			int status = ThreadStatus();

			if (status == CPUThread_Stopped || status == CPUThread_Break)
			{
				break;
			}

			if (status == CPUThread_Sleeping)
			{
				std::this_thread::sleep_for(std::chrono::milliseconds(1));
				continue;
			}

			Step();
			//if (PC - 0x13ED4 < 0x288) trace.push_back(PC);
			NextPc(m_dec->DecodeMemory(PC + m_offset));

			if (status == CPUThread_Step)
			{
				m_is_step = false;
				break;
			}

			for (uint i = 0; i < bp.size(); ++i)
			{
				if (bp[i] == PC)
				{
					Emu.Pause();
					break;
				}
			}
		}
	}
	catch (const std::string& e)
	{
		LOG_ERROR(GENERAL, "Exception: %s", e.c_str());
		Emu.Pause();
	}
	catch (const char* e)
	{
		LOG_ERROR(GENERAL, "Exception: %s", e);
		Emu.Pause();
	}

	for (auto& v : trace) LOG_NOTICE(PPU, "PC = 0x%llx", v);

	if (Ini.HLELogging.GetValue()) LOG_NOTICE(PPU, "%s leave", CPUThread::GetFName().c_str());
}

s64 CPUThread::ExecAsCallback(u64 pc, bool wait, u64 a1, u64 a2, u64 a3, u64 a4) // not multithread-safe
{
	while (m_alive)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(PPU, "ExecAsCallback() aborted");
			return CELL_ECANCELED; // doesn't mean anything
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	Stop();
	Reset();

	SetEntry(pc);
	SetPrio(1001);
	SetStackSize(0x10000);
	SetExitStatus(CELL_OK);

	SetArg(0, a1);
	SetArg(1, a2);
	SetArg(2, a3);
	SetArg(3, a4);
	Run();

	Exec();

	while (wait && m_alive)
	{
		if (Emu.IsStopped())
		{
			LOG_WARNING(PPU, "ExecAsCallback(wait=%s) aborted", wait ? "true" : "false");
			return CELL_EABORT; // doesn't mean anything
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	return wait * m_exit_status;
}
