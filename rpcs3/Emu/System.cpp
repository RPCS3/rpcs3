#include "stdafx.h"
#include "System.h"
#include "Emu/Memory/Memory.h"
#include "Ini.h"

#include "Emu/Cell/PPCThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"

#include "Emu/SysCalls/SysCalls.h"

SysCalls SysCallsManager;

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_dbg_console(NULL)
	, m_rsx_callback(0)
{
}

void Emulator::Init()
{
	//if(m_memory_viewer) m_memory_viewer->Close();
	//m_memory_viewer = new MemoryViewerPanel(wxGetApp().m_MainFrame);
}

void Emulator::SetSelf(const wxString& path)
{
	m_path = path;
	IsSelf = true;
}

void Emulator::SetElf(const wxString& path)
{
	m_path = path;
	IsSelf = false;
}

void Emulator::CheckStatus()
{
	ArrayF<PPCThread>& threads = GetCPU().GetThreads();
	if(!threads.GetCount())
	{
		Stop();
		return;	
	}

	bool IsAllPaused = true;
	for(u32 i=0; i<threads.GetCount(); ++i)
	{
		if(threads[i].IsPaused()) continue;
		IsAllPaused = false;
		break;
	}
	if(IsAllPaused)
	{
		ConLog.Warning("all paused!");
		Pause();
		return;
	}

	bool IsAllStoped = true;
	for(u32 i=0; i<threads.GetCount(); ++i)
	{
		if(threads[i].IsStopped()) continue;
		IsAllStoped = false;
		break;
	}
	if(IsAllStoped)
	{
		ConLog.Warning("all stoped!");
		Pause(); //Stop();
	}
}

void Emulator::Run()
{
	if(IsRunned()) Stop();
	if(IsPaused())
	{
		Resume();
		return;
	}

	//ConLog.Write("run...");
	m_status = Runned;

	Memory.Init();

	GetInfo().Reset();

	//SetTLSData(0, 0, 0);
	//SetMallocPageSize(0x100000);
	
	Loader l(m_path);
	if(!l.Load())
	{
		Memory.Close();
		Stop();
		return;
	}
	
	if(l.GetMachine() == MACHINE_Unknown)
	{
		ConLog.Error("Unknown machine type");
		Memory.Close();
		Stop();
		return;
	}

	PPCThread& thread = GetCPU().AddThread(l.GetMachine() == MACHINE_PPC64);

	Memory.MainMem.Alloc(0x10000000, 0x10010000);
	Memory.PRXMem.Write64(Memory.PRXMem.GetStartAddr(), 0xDEADBEEFABADCAFE);

	thread.SetPc(l.GetEntry());
	thread.SetArg(thread.GetId());
	Memory.StackMem.Alloc(0x1000);
	thread.InitStack();
	thread.AddArgv(m_path);
	//thread.AddArgv("-emu");

	m_rsx_callback = Memory.MainMem.Alloc(0x10) + 4;
	Memory.Write32(m_rsx_callback - 4, m_rsx_callback);

	thread.Run();

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	wxGetApp().m_MainFrame->UpdateUI();

	if(!m_dbg_console) m_dbg_console = new DbgConsole();

	GetGSManager().Init();

	if(Ini.CPUDecoderMode.GetValue() != 1)
	{
		GetCPU().Start();
		GetCPU().Exec();
	}
}

void Emulator::Pause()
{
	if(!IsRunned()) return;
	//ConLog.Write("pause...");

	m_status = Paused;
	wxGetApp().m_MainFrame->UpdateUI();
}

void Emulator::Resume()
{
	if(!IsPaused()) return;
	//ConLog.Write("resume...");

	m_status = Runned;
	wxGetApp().m_MainFrame->UpdateUI();

	CheckStatus();
	if(IsRunned() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
}

void Emulator::Stop()
{
	if(IsStopped()) return;
	//ConLog.Write("shutdown...");

	m_rsx_callback = 0;
	m_status = Stopped;
	wxGetApp().m_MainFrame->UpdateUI();

	GetGSManager().Close();
	GetCPU().Close();
	SysCallsManager.Close();
	GetIdManager().Clear();
	GetPadManager().Close();

	CurGameInfo.Reset();
	Memory.Close();

	if(m_dbg_console)
	{
		GetDbgCon().Close();
		m_dbg_console = NULL;
	}
	//if(m_memory_viewer && m_memory_viewer->IsShown()) m_memory_viewer->Hide();
}

Emulator Emu;