#include "stdafx.h"
#include "System.h"
#include "Emu/Memory/Memory.h"
#include "Ini.h"

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

void Emulator::SetPath(const wxString& path, const wxString& elf_path)
{
	m_path = path;
	m_elf_path = elf_path;
}

void Emulator::CheckStatus()
{
	ArrayF<CPUThread>& threads = GetCPU().GetThreads();
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
	ConLog.Write("Loading '%s'...", m_path);
	GetInfo().Reset();
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

	if(m_elf_path.IsEmpty())
	{
		GetVFS().GetDeviceLocal(m_path, m_elf_path);
	}

	vfsFile f(m_elf_path);

	if(!f.IsOpened())
	{
		ConLog.Error("Elf not found! (%s - %s)", m_path, m_elf_path);
		return;
	}

	bool is_error;
	Loader l(f);

	try
	{
		if(!(is_error = !l.Analyze() || l.GetMachine() == MACHINE_Unknown))
		{
			switch(l.GetMachine())
			{
			case MACHINE_SPU:
			case MACHINE_PPC64:
				Memory.Init(Memory_PS3);
			break;

			case MACHINE_MIPS:
				Memory.Init(Memory_PSP);
			break;

			case MACHINE_ARM:
				Memory.Init(Memory_PSV);
			break;
			}

			is_error = !l.Load();
		}
		
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

	CPUThreadType thread_type;

	if(!is_error)
	{
		switch(l.GetMachine())
		{
		case MACHINE_PPC64: thread_type = CPU_THREAD_PPU; break;
		case MACHINE_SPU: thread_type = CPU_THREAD_SPU; break;
		case MACHINE_ARM: thread_type = CPU_THREAD_ARM9; break;

		default:
			is_error = true;
		break;
		}
	}

	if(is_error)
	{
		Memory.Close();
		Stop();
		return;
	}

	LoadPoints(BreakPointsDBName);

	CPUThread& thread = GetCPU().AddThread(thread_type);

	switch(l.GetMachine())
	{
	case MACHINE_SPU:
		ConLog.Write("offset = 0x%llx", Memory.MainMem.GetStartAddr());
		ConLog.Write("max addr = 0x%x", l.GetMaxAddr());
		thread.SetOffset(Memory.MainMem.GetStartAddr());
		Memory.MainMem.Alloc(Memory.MainMem.GetStartAddr() + l.GetMaxAddr(), 0xFFFFED - l.GetMaxAddr());
		thread.SetEntry(l.GetEntry() - Memory.MainMem.GetStartAddr());
	break;

	case MACHINE_PPC64:
	{
		thread.SetEntry(l.GetEntry());
		Memory.StackMem.Alloc(0x1000);
		thread.InitStack();
		thread.AddArgv(m_path);
		//thread.AddArgv("-emu");

		m_rsx_callback = Memory.MainMem.Alloc(4 * 4) + 4;
		Memory.Write32(m_rsx_callback - 4, m_rsx_callback);

		mem32_ptr_t callback_data(m_rsx_callback);
		callback_data += ADDI(11, 0, 0x3ff);
		callback_data += SC(2);
		callback_data += BCLR(0x10 | 0x04, 0, 0, 0);

		m_ppu_thr_exit = Memory.MainMem.Alloc(4 * 4);

		mem32_ptr_t ppu_thr_exit_data(m_ppu_thr_exit);
		ppu_thr_exit_data += ADDI(3, 0, 0);
		ppu_thr_exit_data += ADDI(11, 0, 41);
		ppu_thr_exit_data += SC(2);
		ppu_thr_exit_data += BCLR(0x10 | 0x04, 0, 0, 0);
	}
	break;

	default:
		thread.SetEntry(l.GetEntry());
	break;
	}

	if(!m_dbg_console)
	{
		m_dbg_console = new DbgConsole();
	}
	else
	{
		GetDbgCon().Close();
		GetDbgCon().Clear();
	}

	GetGSManager().Init();
	GetCallbackManager().Init();

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

	if(IsRunning()) Stop();
	if(IsPaused())
	{
		Resume();
		return;
	}

	wxGetApp().SendDbgCommand(DID_START_EMU);

	wxCriticalSectionLocker lock(m_cs_status);
	//ConLog.Write("run...");
	m_status = Running;

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	GetCPU().Exec();
	wxGetApp().SendDbgCommand(DID_STARTED_EMU);
}

void Emulator::Pause()
{
	if(!IsRunning()) return;
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
	m_status = Running;

	CheckStatus();
	if(IsRunning() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
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
	GetKeyboardManager().Close();
	GetMouseManager().Close();
	GetCallbackManager().Clear();
	UnloadModules();

	CurGameInfo.Reset();
	Memory.Close();

	//if(m_memory_viewer && m_memory_viewer->IsShown()) m_memory_viewer->Hide();
	wxGetApp().SendDbgCommand(DID_STOPPED_EMU);
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
		ConLog.Error("'%s' is broken", path);
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
