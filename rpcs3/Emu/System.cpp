#include "stdafx.h"
#include "System.h"
#include "Emu/Memory/Memory.h"
#include "Ini.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"

#include "../Crypto/unself.h"
#include <cstdlib>
#include <fstream>
using namespace PPU_instr;

static const std::string& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;

ModuleInitializer::ModuleInitializer()
{
	Emu.AddModuleInit(std::move(std::unique_ptr<ModuleInitializer>(this)));
}

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_dbg_console(nullptr)
	, m_rsx_callback(0)
	, m_ppu_callback_thr(0)
	, m_event_manager(new EventManager())
{
}

void Emulator::Init()
{
	while(m_modules_init.size())
	{
		m_modules_init[0]->Init();
		m_modules_init.erase(m_modules_init.begin());
	}
	//if(m_memory_viewer) m_memory_viewer->Close();
	//m_memory_viewer = new MemoryViewerPanel(wxGetApp().m_MainFrame);
}

void Emulator::SetPath(const std::string& path, const std::string& elf_path)
{
	m_path = path;
	m_elf_path = elf_path;
}

void Emulator::SetTitleID(const std::string& id)
{
	m_title_id = id;
}

void Emulator::CheckStatus()
{
	std::vector<CPUThread *>& threads = GetCPU().GetThreads();
	if(!threads.size())
	{
		Stop();
		return;	
	}

	bool IsAllPaused = true;
	for(u32 i=0; i<threads.size(); ++i)
	{
		if(threads[i]->IsPaused()) continue;
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
	for(u32 i=0; i<threads.size(); ++i)
	{
		if(threads[i]->IsStopped()) continue;
		IsAllStoped = false;
		break;
	}
	if(IsAllStoped)
	{
		//ConLog.Warning("all stoped!");
		Pause(); //Stop();
	}
}

bool Emulator::BootGame(const std::string& path)
{
	static const char* elf_path[6] =
	{
		"/PS3_GAME/USRDIR/BOOT.BIN",
		"/USRDIR/BOOT.BIN",
		"/BOOT.BIN",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
	};

	for(int i=0; i<sizeof(elf_path) / sizeof(*elf_path);i++)
	{
		const std::string& curpath = path + elf_path[i];

		if(wxFile::Access(fmt::FromUTF8(curpath), wxFile::read))
		{
			SetPath(curpath);
			Load();

			return true;
		}
	}

	return false;
}

void Emulator::Load()
{
	if(!wxFileExists(fmt::FromUTF8(m_path))) return;

	if(IsSelf(m_path))
	{
		std::string self_path = m_path;
		std::string elf_path = fmt::ToUTF8(wxFileName(fmt::FromUTF8(m_path)).GetPath());

		if (wxFileName(fmt::FromUTF8(m_path)).GetFullName().CmpNoCase("EBOOT.BIN") == 0)
		{
			elf_path += "/BOOT.BIN";
		}
		else
		{
			elf_path += "/" + fmt::ToUTF8(wxFileName(fmt::FromUTF8(m_path)).GetName()) + ".elf";
		}

		if(!DecryptSelf(elf_path, self_path))
			return;

		m_path = elf_path;
	}

	ConLog.Write("Loading '%s'...", m_path.c_str());
	GetInfo().Reset();
	m_vfs.Init(m_path);

	ConLog.SkipLn();
	ConLog.Write("Mount info:");
	for(uint i=0; i<m_vfs.m_devices.size(); ++i)
	{
		ConLog.Write("%s -> %s", m_vfs.m_devices[i]->GetPs3Path().c_str(), m_vfs.m_devices[i]->GetLocalPath().c_str());
	}
	ConLog.SkipLn();

	if(m_elf_path.empty())
	{
		GetVFS().GetDeviceLocal(m_path, m_elf_path);
	}

	vfsFile f(m_elf_path);

	if(!f.IsOpened())
	{
		ConLog.Error("Elf not found! (%s - %s)", m_path.c_str(), m_elf_path.c_str());
		return;
	}

	bool is_error;
	Loader l(f);

	try
	{
		if(!(is_error = !l.Analyze()) && l.GetMachine() != MACHINE_Unknown)
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
	catch(const std::string& e)
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
		case MACHINE_ARM: thread_type = CPU_THREAD_ARMv7; break;

		default:
			ConLog.Error("Unimplemented thread type for machine.");
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
		Memory.MainMem.AllocFixed(Memory.MainMem.GetStartAddr() + l.GetMaxAddr(), 0xFFFFED - l.GetMaxAddr());
		thread.SetEntry(l.GetEntry() - Memory.MainMem.GetStartAddr());
	break;

	case MACHINE_PPC64:
	{
		m_ppu_callback_thr = &GetCPU().AddThread(CPU_THREAD_PPU);

		thread.SetEntry(l.GetEntry());
		Memory.StackMem.AllocAlign(0x1000);
		thread.InitStack();
		thread.AddArgv(m_elf_path);
		//thread.AddArgv("-emu");

		m_rsx_callback = Memory.MainMem.AllocAlign(4 * 4) + 4;
		Memory.Write32(m_rsx_callback - 4, m_rsx_callback);

		mem32_ptr_t callback_data(m_rsx_callback);
		callback_data += ADDI(11, 0, 0x3ff);
		callback_data += SC(2);
		callback_data += BCLR(0x10 | 0x04, 0, 0, 0);

		m_ppu_thr_exit = Memory.MainMem.AllocAlign(4 * 4);

		mem32_ptr_t ppu_thr_exit_data(m_ppu_thr_exit);
		ppu_thr_exit_data += ADDI(3, 0, 0);
		ppu_thr_exit_data += ADDI(11, 0, 41);
		ppu_thr_exit_data += SC(2);
		ppu_thr_exit_data += BCLR(0x10 | 0x04, 0, 0, 0);

		Memory.Write64(Memory.PRXMem.AllocAlign(0x10000), 0xDEADBEEFABADCAFE);
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
	GetAudioManager().Init();
	GetEventManager().Init();

	thread.Run();

	m_status = Ready;
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_READY_EMU);
#endif
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
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_START_EMU);
#endif

	//ConLog.Write("run...");
	m_status = Running;

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	GetCPU().Exec();
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_STARTED_EMU);
#endif
}

void Emulator::Pause()
{
	if(!IsRunning()) return;
	//ConLog.Write("pause...");
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_PAUSE_EMU);
#endif

	m_status = Paused;
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_PAUSED_EMU);
#endif
}

void Emulator::Resume()
{
	if(!IsPaused()) return;
	//ConLog.Write("resume...");
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_RESUME_EMU);
#endif

	m_status = Running;

	CheckStatus();
	//if(IsRunning() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_RESUMED_EMU);
#endif
}

void Emulator::Stop()
{
	if(IsStopped()) return;
	//ConLog.Write("shutdown...");

#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_STOP_EMU);
#endif
	m_status = Stopped;

	m_rsx_callback = 0;

	SavePoints(BreakPointsDBName);
	m_break_points.clear();
	m_marked_points.clear();

	m_vfs.UnMountAll();

	GetGSManager().Close();
	GetAudioManager().Close();
	GetEventManager().Clear();
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
#ifndef QT_UI
	wxGetApp().SendDbgCommand(DID_STOPPED_EMU);
#endif
}

void Emulator::SavePoints(const std::string& path)
{
	std::ofstream f(path, std::ios::binary | std::ios::trunc);

	u32 break_count = m_break_points.size();
	u32 marked_count = m_marked_points.size();

	f << bpdb_version << break_count << marked_count;
	
	if(break_count)
	{
		f.write(reinterpret_cast<char*>(&m_break_points[0]), sizeof(u64) * break_count);
	}

	if(marked_count)
	{
		f.write(reinterpret_cast<char*>(&m_marked_points[0]), sizeof(u64) * marked_count);
	}
}

void Emulator::LoadPoints(const std::string& path)
{
	struct stat buf;
	if (!stat(path.c_str(), &buf))
		return;
	std::ifstream f(path, std::ios::binary);
	if (!f.is_open())
		return;
	f.seekg(0, std::ios::end);
	int length = f.tellg();
	f.seekg(0, std::ios::beg);
	u32 break_count, marked_count;
	u16 version;
	f >> version >> break_count >> marked_count;

	if(version != bpdb_version ||
		(sizeof(u16) + break_count * sizeof(u64) + sizeof(u32) + marked_count * sizeof(u64) + sizeof(u32)) != length)
	{
		ConLog.Error("'%s' is broken", path.c_str());
		return;
	}

	if(break_count > 0)
	{
		m_break_points.resize(break_count);
		f.read(reinterpret_cast<char*>(&m_break_points[0]), sizeof(u64) * break_count);
	}

	if(marked_count > 0)
	{
		m_marked_points.resize(marked_count);
		f.read(reinterpret_cast<char*>(&m_marked_points[0]), sizeof(u64) * marked_count);
	}
}

Emulator Emu;
