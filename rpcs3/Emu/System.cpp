#include "stdafx.h"
#include "System.h"
#include "Emu/Memory/Memory.h"
#include "Ini.h"

#include "Emu/Cell/PPCThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Gui/CompilerELF.h"

using namespace PPU_opcodes;

//SysCalls SysCallsManager;

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
		//ConLog.Warning("all paused!");
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
		//ConLog.Warning("all stoped!");
		Pause(); //Stop();
	}
}

void Emulator::Load()
{
	if(!wxFileExists(m_path)) return;
	ConLog.Write("loading '%s'...", m_path);
	Memory.Init();
	GetInfo().Reset();

	Memory.Write64(Memory.PRXMem.Alloc(8), 0xDEADBEEFABADCAFE);

	bool is_error;
	vfsLocalFile f(m_path);
	Loader l(f);

	try
	{
		is_error = !l.Load() || l.GetMachine() == MACHINE_Unknown;
	}
	catch(const wxString& e)
	{
		ConLog.Error(e);
		is_error = true;
	}
	catch(...)
	{
		ConLog.Error("Unhandled loader error.");
		is_error = true;
	}

	if(is_error)
	{
		Memory.Close();
		Stop();
		return;
	}

	PPCThread& thread = GetCPU().AddThread(l.GetMachine() == MACHINE_PPC64);

	thread.SetEntry(l.GetEntry());
	thread.SetArg(thread.GetId());
	Memory.StackMem.Alloc(0x1000);
	thread.InitStack();
	thread.AddArgv(m_path);
	//thread.AddArgv("-emu");

	m_rsx_callback = Memory.MainMem.Alloc(4 * 4) + 4;
	Memory.Write32(m_rsx_callback - 4, m_rsx_callback);

	mem32_t callback_data(m_rsx_callback);
	callback_data += ToOpcode(ADDI) | ToRD(11) | ToRA(0) | ToIMM16(0x3ff);
	callback_data += ToOpcode(SC) | ToSYS(2);
	callback_data += ToOpcode(G_13) | SetField(BCLR, 21, 30) | ToBO(0x10 | 0x04) | ToBI(0) | ToLK(0);

	m_ppu_thr_exit = Memory.MainMem.Alloc(4 * 3);
	
	mem32_t ppu_thr_exit_data(m_ppu_thr_exit);
	ppu_thr_exit_data += ToOpcode(ADDI) | ToRD(11) | ToRA(0) | ToIMM16(41);
	ppu_thr_exit_data += ToOpcode(SC) | ToSYS(2);
	ppu_thr_exit_data += ToOpcode(G_13) | SetField(BCLR, 21, 30) | ToBO(0x10 | 0x04) | ToBI(0) | ToLK(0);

	thread.Run();

	wxGetApp().m_MainFrame->UpdateUI();
	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Ready;
}

void Emulator::Run()
{
	if(!IsReady())
	{
		Load();
		if(!IsReady()) return;
	}

	if(IsRunned()) Stop();
	if(IsPaused())
	{
		Resume();
		return;
	}

	wxCriticalSectionLocker lock(m_cs_status);
	//ConLog.Write("run...");
	m_status = Runned;

	m_vfs.Mount("/", vfsDevice::GetRoot(m_path), new vfsLocalFile());
	m_vfs.Mount("/dev_hdd0/", wxGetCwd() + "\\dev_hdd0\\", new vfsLocalFile());
	m_vfs.Mount("/app_home/", vfsDevice::GetRoot(m_path), new vfsLocalFile());
	m_vfs.Mount(vfsDevice::GetRootPs3(m_path), vfsDevice::GetRoot(m_path), new vfsLocalFile());

	for(uint i=0; i<m_vfs.m_devices.GetCount(); ++i) ConLog.Write("%s -> %s", m_vfs.m_devices[i].GetPs3Path(), m_vfs.m_devices[i].GetLocalPath());

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	wxGetApp().SendDbgCommand(DID_START_EMU);
	wxGetApp().m_MainFrame->UpdateUI();

	if(!m_dbg_console) m_dbg_console = new DbgConsole();

	GetGSManager().Init();
	GetCallbackManager().Init();

	GetCPU().Exec();
}

void Emulator::Pause()
{
	if(!IsRunned()) return;
	//ConLog.Write("pause...");

	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Paused;
	wxGetApp().SendDbgCommand(DID_PAUSE_EMU);
	wxGetApp().m_MainFrame->UpdateUI();
}

void Emulator::Resume()
{
	if(!IsPaused()) return;
	//ConLog.Write("resume...");

	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Runned;
	wxGetApp().SendDbgCommand(DID_RESUME_EMU);
	wxGetApp().m_MainFrame->UpdateUI();

	CheckStatus();
	if(IsRunned() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
}

void Emulator::Stop()
{
	if(IsStopped()) return;
	//ConLog.Write("shutdown...");
	{
		wxCriticalSectionLocker lock(m_cs_status);
		m_status = Stopped;
	}

	m_rsx_callback = 0;
	wxGetApp().SendDbgCommand(DID_STOP_EMU);
	wxGetApp().m_MainFrame->UpdateUI();

	GetGSManager().Close();
	GetCPU().Close();
	//SysCallsManager.Close();
	GetIdManager().Clear();
	GetPadManager().Close();
	GetCallbackManager().Clear();
	UnloadModules();

	CurGameInfo.Reset();
	Memory.Close();

	if(m_dbg_console)
	{
		GetDbgCon().Close();
		GetDbgCon().Clear();
	}
	//if(m_memory_viewer && m_memory_viewer->IsShown()) m_memory_viewer->Hide();
}

Emulator Emu;