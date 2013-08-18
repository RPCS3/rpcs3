#include "stdafx.h"
#include "System.h"
#include "Emu/Memory/Memory.h"
#include "Ini.h"

#include "Emu/Cell/PPCThreadManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"
using namespace PPU_instr;

static const wxString& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;

ModuleInitializer::ModuleInitializer()
{
	Emu.AddModuleInit(this);
}

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_dbg_console(NULL)
	, m_rsx_callback(0)
{
}

void Emulator::Init()
{
	while(m_modules_init.GetCount())
	{
		m_modules_init[0].Init();
		m_modules_init.RemoveAt(0);
	}
	//if(m_memory_viewer) m_memory_viewer->Close();
	//m_memory_viewer = new MemoryViewerPanel(wxGetApp().m_MainFrame);
}

void Emulator::SetPath(const wxString& path)
{
	m_path = path;
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

	LoadPoints(BreakPointsDBName);
	PPCThread& thread = GetCPU().AddThread(l.GetMachine() == MACHINE_PPC64 ? PPC_THREAD_PPU : PPC_THREAD_SPU);

	if(l.GetMachine() == MACHINE_SPU)
	{
		ConLog.Write("offset = 0x%llx", Memory.MainMem.GetStartAddr());
		ConLog.Write("max addr = 0x%x", l.GetMaxAddr());
		thread.SetOffset(Memory.MainMem.GetStartAddr());
		Memory.MainMem.Alloc(Memory.MainMem.GetStartAddr() + l.GetMaxAddr(), 0xFFFFED - l.GetMaxAddr());
		thread.SetEntry(l.GetEntry() - Memory.MainMem.GetStartAddr());
	}
	else
	{
		thread.SetEntry(l.GetEntry());
		Memory.StackMem.Alloc(0x1000);
		thread.InitStack();
		thread.AddArgv(m_path);
		//thread.AddArgv("-emu");

		m_rsx_callback = Memory.MainMem.Alloc(4 * 4) + 4;
		Memory.Write32(m_rsx_callback - 4, m_rsx_callback);

		mem32_t callback_data(m_rsx_callback);
		callback_data += ADDI(11, 0, 0x3ff);
		callback_data += SC(2);
		callback_data += BCLR(0x10 | 0x04, 0, 0, 0);

		m_ppu_thr_exit = Memory.MainMem.Alloc(4 * 4);

		mem32_t ppu_thr_exit_data(m_ppu_thr_exit);
		ppu_thr_exit_data += ADDI(3, 0, 0);
		ppu_thr_exit_data += ADDI(11, 0, 41);
		ppu_thr_exit_data += SC(2);
		ppu_thr_exit_data += BCLR(0x10 | 0x04, 0, 0, 0);
	}

	thread.Run();

	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Ready;
	wxGetApp().SendDbgCommand(DID_READY_EMU);
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

	wxGetApp().SendDbgCommand(DID_START_EMU);

	wxCriticalSectionLocker lock(m_cs_status);
	//ConLog.Write("run...");
	m_status = Runned;

	m_vfs.Init(m_path);
	//m_vfs.Mount("/", vfsDevice::GetRoot(m_path), new vfsLocalFile());
	//m_vfs.Mount("/dev_hdd0/", wxGetCwd() + "\\dev_hdd0\\", new vfsLocalFile());
	//m_vfs.Mount("/app_home/", vfsDevice::GetRoot(m_path), new vfsLocalFile());
	//m_vfs.Mount(vfsDevice::GetRootPs3(m_path), vfsDevice::GetRoot(m_path), new vfsLocalFile());

	ConLog.SkipLn();
	ConLog.Write("Mount info:");
	for(uint i=0; i<m_vfs.m_devices.GetCount(); ++i)
	{
		ConLog.Write("%s -> %s", m_vfs.m_devices[i].GetPs3Path(), m_vfs.m_devices[i].GetLocalPath());
	}
	ConLog.SkipLn();

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	if(!m_dbg_console)
		m_dbg_console = new DbgConsole();

	GetGSManager().Init();
	GetCallbackManager().Init();

	GetCPU().Exec();
	wxGetApp().SendDbgCommand(DID_STARTED_EMU);
}

void Emulator::Pause()
{
	if(!IsRunned()) return;
	//ConLog.Write("pause...");
	wxGetApp().SendDbgCommand(DID_PAUSE_EMU);

	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Paused;
	wxGetApp().SendDbgCommand(DID_PAUSED_EMU);
}

void Emulator::Resume()
{
	if(!IsPaused()) return;
	//ConLog.Write("resume...");
	wxGetApp().SendDbgCommand(DID_RESUME_EMU);

	wxCriticalSectionLocker lock(m_cs_status);
	m_status = Runned;

	CheckStatus();
	if(IsRunned() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
	wxGetApp().SendDbgCommand(DID_RESUMED_EMU);
}

void Emulator::Stop()
{
	if(IsStopped()) return;
	//ConLog.Write("shutdown...");

	wxGetApp().SendDbgCommand(DID_STOP_EMU);
	{
		wxCriticalSectionLocker lock(m_cs_status);
		m_status = Stopped;
	}

	m_rsx_callback = 0;

	SavePoints(BreakPointsDBName);
	m_break_points.Clear();
	m_marked_points.Clear();

	m_vfs.UnMountAll();

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
	wxGetApp().SendDbgCommand(DID_STOPED_EMU);
}

void Emulator::SavePoints(const wxString& path)
{
	wxFile f(path, wxFile::write);

	u32 break_count = m_break_points.GetCount();
	u32 marked_count = m_marked_points.GetCount();

	f.Write(&bpdb_version, sizeof(u16));
	f.Write(&break_count, sizeof(u32));
	f.Write(&marked_count, sizeof(u32));

	if(break_count)
	{
		f.Write(&m_break_points[0], sizeof(u64) * break_count);
	}

	if(marked_count)
	{
		f.Write(&m_marked_points[0], sizeof(u64) * marked_count);
	}
}

void Emulator::LoadPoints(const wxString& path)
{
	if(!wxFileExists(path)) return;

	wxFile f(path);

	u32 break_count, marked_count;
	u16 version;
	f.Read(&version, sizeof(u16));
	f.Read(&break_count, sizeof(u32));
	f.Read(&marked_count, sizeof(u32));

	if(version != bpdb_version ||
		(sizeof(u16) + break_count * sizeof(u64) + sizeof(u32) + marked_count * sizeof(u64) + sizeof(u32)) != f.Length())
	{
		ConLog.Error("'%s' is borken", path);
		return;
	}

	if(break_count > 0)
	{
		m_break_points.SetCount(break_count);
		f.Read(&m_break_points[0], sizeof(u64) * break_count);
	}

	if(marked_count > 0)
	{
		m_marked_points.SetCount(marked_count);
		f.Read(&m_marked_points[0], sizeof(u64) * marked_count);
	}
}

Emulator Emu;