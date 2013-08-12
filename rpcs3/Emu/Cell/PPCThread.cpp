#include "stdafx.h"
#include "PPCThread.h"
#include "Gui/InterpreterDisAsm.h"

PPCThread* GetCurrentPPCThread()
{
	return (PPCThread*)GetCurrentNamedThread();
}

PPCThread::PPCThread(PPCThreadType type)
	: ThreadBase(true, "PPCThread")
	, m_type(type)
	, DisAsmFrame(NULL)
	, m_arg(0)
	, m_dec(NULL)
	, stack_size(0)
	, stack_addr(0)
	, m_prio(0)
	, m_offset(0)
{
}

PPCThread::~PPCThread()
{
	Close();
}

void PPCThread::Close()
{
	Stop();
	if(DisAsmFrame)
	{
		DisAsmFrame->Close();
		DisAsmFrame = nullptr;
	}
}

void PPCThread::Reset()
{
	CloseStack();

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
	ID& thread = Emu.GetIdManager().GetIDData(m_id);
	thread.m_name = GetName();
}

void PPCThread::SetName(const wxString& name)
{
	m_name = name;
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
	if(!Memory.IsGoodAddr(pc))
	{
		ConLog.Error("%s branch error: bad address 0x%llx #pc: 0x%llx", GetFName(), pc, PC);
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
	earr.Clear();
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
	Reset();
	DoStop();
	Emu.CheckStatus();

	ThreadBase::Stop();

	wxGetApp().SendDbgCommand(DID_STOPED_THREAD, this);
}

void PPCThread::Exec()
{
	wxGetApp().SendDbgCommand(DID_EXEC_THREAD, this);
	ThreadBase::Start();
}

void PPCThread::ExecOnce()
{
	DoCode(Memory.Read32(m_offset + PC));
	NextPc();
}

void PPCThread::Task()
{
	ConLog.Write("%s enter", PPCThread::GetFName());

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

		while(!Emu.IsStopped() && !TestDestroy())
		{
			if(Emu.IsPaused())
			{
				Sleep(1);
				continue;
			}

			DoCode(Memory.Read32(m_offset + PC));
			NextPc();

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

	ConLog.Write("%s leave", PPCThread::GetFName());
}
