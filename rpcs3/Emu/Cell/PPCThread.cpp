#include "stdafx.h"
#include "PPCThread.h"
#include "Gui/InterpreterDisAsm.h"

PPCThread* GetCurrentPPCThread()
{
	return (PPCThread*)GetCurrentNamedThread();
}

PPCThread::PPCThread(PPCThreadType type)
	: ThreadBase(false, "PPCThread")
	, m_type(type)
	, DisAsmFrame(nullptr)
	, m_dec(nullptr)
	, stack_size(0)
	, stack_addr(0)
	, m_prio(0)
	, m_offset(0)
	, m_sync_wait(false)
	, m_wait_thread_id(-1)
	, m_free_data(false)
{
}

PPCThread::~PPCThread()
{
	Close();
}

void PPCThread::Close()
{
	if(IsAlive())
	{
		m_free_data = true;
	}

	if(DisAsmFrame)
	{
		DisAsmFrame->Close();
		DisAsmFrame = nullptr;
	}

	Stop();
}

void PPCThread::Reset()
{
	CloseStack();

	m_sync_wait = 0;
	m_wait_thread_id = -1;
	memset(m_args, 0, sizeof(u64) * 4);

	SetPc(0);
	cycle = 0;

	isBranch = false;

	m_status = Stopped;
	m_error = 0;
	
	DoReset();
}

void PPCThread::InitStack()
{
	if(stack_addr) return;
	if(stack_size == 0) stack_size = 0x10000;
	stack_addr = Memory.StackMem.Alloc(Memory.AlignAddr(stack_size, 0x100));

	stack_point = stack_addr + stack_size;
	/*
	stack_point += stack_size - 0x10;
	stack_point &= -0x10;
	Memory.Write64(stack_point, 0);
	stack_point -= 0x60;
	Memory.Write64(stack_point, stack_point + 0x60);
	*/
	if(wxFileExists("stack.dat"))
	{
		ConLog.Warning("loading stack.dat...");
		wxFile stack("stack.dat");		
		stack.Read(Memory.GetMemFromAddr(stack_addr), 0x10000);
		stack.Close();
	}
}

void PPCThread::CloseStack()
{
	Memory.Free(stack_addr);
	stack_addr = 0;
	stack_size = 0;
}

void PPCThread::SetId(const u32 id)
{
	m_id = id;
}

void PPCThread::SetName(const wxString& name)
{
	m_name = name;
}

void PPCThread::Wait(bool wait)
{
	wxCriticalSectionLocker lock(m_cs_sync);
	m_sync_wait = wait;
}

void PPCThread::Wait(const PPCThread& thr)
{
	wxCriticalSectionLocker lock(m_cs_sync);
	m_wait_thread_id = thr.GetId();
	m_sync_wait = true;
}

bool PPCThread::Sync()
{
	wxCriticalSectionLocker lock(m_cs_sync);

	return m_sync_wait;
}

int PPCThread::ThreadStatus()
{
	if(m_is_step)
	{
		return PPCThread_Step;
	}

	if(Emu.IsStopped())
	{
		return PPCThread_Stopped;
	}

	if(TestDestroy())
	{
		return PPCThread_Break;
	}

	if(Emu.IsPaused() || Sync())
	{
		return PPCThread_Sleeping;
	}

	return PPCThread_Running;
}

void PPCThread::NextBranchPc()
{
	SetPc(nPC);
}

void PPCThread::NextPc()
{
	if(isBranch)
	{
		NextBranchPc();
		isBranch = false;
		return;
	}

	SetPc(PC + 4);
}

void PPCThread::PrevPc()
{
	SetPc(PC - 4);
}

void PPCThread::SetPc(const u64 pc)
{
	PC = pc;
	nPC = PC + 4;
}

void PPCThread::SetEntry(const u64 pc)
{
	entry = pc;
}

void PPCThread::SetBranch(const u64 pc)
{
	if(!Memory.IsGoodAddr(m_offset + pc))
	{
		ConLog.Error("%s branch error: bad address 0x%llx #pc: 0x%llx", GetFName(), m_offset+ pc, m_offset + PC);
		Emu.Pause();
	}
	nPC = pc;
	isBranch = true;
}

void PPCThread::SetError(const u32 error)
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

wxArrayString PPCThread::ErrorToString(const u32 error)
{
	wxArrayString earr;

	if(error == 0) return earr;

	earr.Add("Unknown error");

	return earr;
}

void PPCThread::Run()
{
	if(IsRunning()) Stop();
	if(IsPaused())
	{
		Resume();
		return;
	}
	
	wxGetApp().SendDbgCommand(DID_START_THREAD, this);

	m_status = Running;

	SetPc(entry);
	InitStack();
	InitRegs();
	DoRun();
	Emu.CheckStatus();

	wxGetApp().SendDbgCommand(DID_STARTED_THREAD, this);
}

void PPCThread::Resume()
{
	if(!IsPaused()) return;

	wxGetApp().SendDbgCommand(DID_RESUME_THREAD, this);

	m_status = Running;
	DoResume();
	Emu.CheckStatus();

	ThreadBase::Start();

	wxGetApp().SendDbgCommand(DID_RESUMED_THREAD, this);
}

void PPCThread::Pause()
{
	if(!IsRunning()) return;

	wxGetApp().SendDbgCommand(DID_PAUSE_THREAD, this);

	m_status = Paused;
	DoPause();
	Emu.CheckStatus();

	ThreadBase::Stop(false);
	wxGetApp().SendDbgCommand(DID_PAUSED_THREAD, this);
}

void PPCThread::Stop()
{
	if(IsStopped()) return;

	wxGetApp().SendDbgCommand(DID_STOP_THREAD, this);

	m_status = Stopped;
	ThreadBase::Stop(false);
	Reset();
	DoStop();
	Emu.CheckStatus();

	wxGetApp().SendDbgCommand(DID_STOPED_THREAD, this);
}

void PPCThread::Exec()
{
	m_is_step = false;
	wxGetApp().SendDbgCommand(DID_EXEC_THREAD, this);
	ThreadBase::Start();
	//std::thread thr(std::bind(std::mem_fn(&PPCThread::Task), this));
}

void PPCThread::ExecOnce()
{
	m_is_step = true;
	wxGetApp().SendDbgCommand(DID_EXEC_THREAD, this);
	ThreadBase::Start();
	if(!ThreadBase::Wait()) while(m_is_step) Sleep(1);
	wxGetApp().SendDbgCommand(DID_PAUSE_THREAD, this);
	wxGetApp().SendDbgCommand(DID_PAUSED_THREAD, this);
}

void PPCThread::Task()
{
	//ConLog.Write("%s enter", PPCThread::GetFName());

	const Array<u64>& bp = Emu.GetBreakPoints();

	try
	{
		for(uint i=0; i<bp.GetCount(); ++i)
		{
			if(bp[i] == m_offset + PC)
			{
				Emu.Pause();
				break;
			}
		}

		while(true)
		{
			int status = ThreadStatus();

			if(status == PPCThread_Stopped || status == PPCThread_Break)
			{
				break;
			}

			if(status == PPCThread_Sleeping)
			{
				Sleep(1);
				continue;
			}

			DoCode(Memory.Read32(m_offset + PC));
			NextPc();

			if(status == PPCThread_Step)
			{
				m_is_step = false;
				break;
			}

			for(uint i=0; i<bp.GetCount(); ++i)
			{
				if(bp[i] == m_offset + PC)
				{
					Emu.Pause();
					break;
				}
			}
		}
	}
	catch(const wxString& e)
	{
		ConLog.Error("Exception: %s", e);
	}
	catch(const char* e)
	{
		ConLog.Error("Exception: %s", e);
	}

	//ConLog.Write("%s leave", PPCThread::GetFName());

	if(m_free_data)
		free(this);
}
