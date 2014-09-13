#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/GameInfo.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsDeviceLocalFile.h"
#include "Emu/DbgCommand.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/IdManager.h"
#include "Emu/Io/Pad.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Mouse.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/FS/VFS.h"

#include "Loader/PSF.h"

#include "../Crypto/unself.h"
#include <cstdlib>
#include <fstream>
using namespace PPU_instr;

static const std::string& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;
extern std::atomic<u32> g_thread_count;

ModuleInitializer::ModuleInitializer()
{
	Emu.AddModuleInit(std::move(std::unique_ptr<ModuleInitializer>(this)));
}

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_rsx_callback(0)
	, m_thread_manager(new CPUThreadManager())
	, m_pad_manager(new PadManager())
	, m_keyboard_manager(new KeyboardManager())
	, m_mouse_manager(new MouseManager())
	, m_id_manager(new IdManager())
	, m_gs_manager(new GSManager())
	, m_audio_manager(new AudioManager())
	, m_callback_manager(new CallbackManager())
	, m_event_manager(new EventManager())
	, m_sfunc_manager(new StaticFuncManager())
	, m_module_manager(new ModuleManager())
	, m_vfs(new VFS())
{
}

Emulator::~Emulator()
{
	delete m_thread_manager;
	delete m_pad_manager;
	delete m_keyboard_manager;
	delete m_mouse_manager;
	delete m_id_manager;
	delete m_gs_manager;
	delete m_audio_manager;
	delete m_callback_manager;
	delete m_event_manager;
	delete m_sfunc_manager;
	delete m_module_manager;
	delete m_vfs;
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

		if(rFile::Access(curpath, rFile::read))
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
	GetModuleManager().init();

	if(!rExists(m_path)) return;

	if(IsSelf(m_path))
	{
		std::string self_path = m_path;
		std::string elf_path = rFileName(m_path).GetPath();

		if (fmt::CmpNoCase(rFileName(m_path).GetFullName(),"EBOOT.BIN") == 0)
		{
			elf_path += "/BOOT.BIN";
		}
		else
		{
			elf_path += "/" + rFileName(m_path).GetName() + ".elf";
		}

		if(!DecryptSelf(elf_path, self_path))
			return;

		m_path = elf_path;
	}

	LOG_NOTICE(LOADER, "Loading '%s'...", m_path.c_str());
	GetInfo().Reset();
	GetVFS().Init(m_path);

	LOG_NOTICE(LOADER, " "); //used to be skip_line
	LOG_NOTICE(LOADER, "Mount info:");
	for(uint i=0; i<GetVFS().m_devices.size(); ++i)
	{
		LOG_NOTICE(LOADER, "%s -> %s", GetVFS().m_devices[i]->GetPs3Path().c_str(), GetVFS().m_devices[i]->GetLocalPath().c_str());
	}

	LOG_NOTICE(LOADER, " ");//used to be skip_line
	vfsFile sfo("/app_home/PARAM.SFO");
	PSFLoader psf(sfo);
	psf.Load(false);
	std::string title = psf.GetString("TITLE");
	std::string title_id = psf.GetString("TITLE_ID");
	LOG_NOTICE(LOADER, "Title: %s", title.c_str());
	LOG_NOTICE(LOADER, "Serial: %s", title_id.c_str());

	// bdvd inserting imitation
	vfsFile f1("/app_home/dev_bdvd.path");
	if (f1.IsOpened())
	{
		std::string bdvd;
		bdvd.resize(f1.GetSize());
		f1.Read(&bdvd[0], bdvd.size());

		// load desired /dev_bdvd/ real directory and remount
		Emu.GetVFS().Mount("/dev_bdvd/", bdvd, new vfsDeviceLocalFile());
		LOG_NOTICE(LOADER, "/dev_bdvd/ remounted into %s", bdvd.c_str());
	}
	LOG_NOTICE(LOADER, " ");//used to be skip_line

	if(m_elf_path.empty())
	{
		GetVFS().GetDeviceLocal(m_path, m_elf_path);
	}

	vfsFile f(m_elf_path);

	if(!f.IsOpened())
	{
		LOG_ERROR(LOADER, "Elf not found! (%s - %s)", m_path.c_str(), m_elf_path.c_str());
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
				Memory.Init(Memory_PS3);
				Memory.MainMem.AllocFixed(Memory.MainMem.GetStartAddr(), 0x40000);
			break;

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
		LOG_ERROR(LOADER, e);
		is_error = true;
	}
	catch(...)
	{
		LOG_ERROR(LOADER, "Unhandled loader error.");
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
			LOG_ERROR(LOADER, "Unimplemented thread type for machine.");
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

	// setting default values
	Emu.m_sdk_version = -1; // possibly "unknown" value

	// trying to load some info from PARAM.SFO
	vfsFile f2("/app_home/PARAM.SFO");
	if (f2.IsOpened())
	{
		PSFLoader psf(f2);
		if (psf.Load(false))
		{
			std::string version = psf.GetString("PS3_SYSTEM_VER");

			const size_t dot = version.find('.');
			if (dot != std::string::npos)
			{
				Emu.m_sdk_version = (std::stoi(version, nullptr, 16) << 20) | ((std::stoi(version.substr(dot + 1), nullptr, 16) & 0xffff) << 4) | 1;
			}
		}
	}

	LoadPoints(BreakPointsDBName);

	CPUThread& thread = GetCPU().AddThread(thread_type);

	switch(l.GetMachine())
	{
	case MACHINE_SPU:
		LOG_NOTICE(LOADER, "offset = 0x%llx", Memory.MainMem.GetStartAddr());
		LOG_NOTICE(LOADER, "max addr = 0x%x", l.GetMaxAddr());
		thread.SetOffset(Memory.MainMem.GetStartAddr());
		thread.SetEntry(l.GetEntry() - Memory.MainMem.GetStartAddr());
	break;

	case MACHINE_PPC64:
	{
		thread.SetEntry(l.GetEntry());
		Memory.StackMem.AllocAlign(0x1000);
		thread.InitStack();
		thread.AddArgv(m_elf_path); // it doesn't work
		//thread.AddArgv("-emu");

		m_rsx_callback = (u32)Memory.MainMem.AllocAlign(4 * 4) + 4;
		vm::write32(m_rsx_callback - 4, m_rsx_callback);

		auto callback_data = vm::ptr<be_t<u32>>::make(m_rsx_callback);
		callback_data[0] = ADDI(11, 0, 0x3ff);
		callback_data[1] = SC(2);
		callback_data[2] = BCLR(0x10 | 0x04, 0, 0, 0);

		m_ppu_thr_exit = (u32)Memory.MainMem.AllocAlign(4 * 4);

		auto ppu_thr_exit_data = vm::ptr<be_t<u32>>::make(m_ppu_thr_exit);
		//ppu_thr_exit_data += ADDI(3, 0, 0); // why it kills return value (GPR[3]) ?
		ppu_thr_exit_data[0] = ADDI(11, 0, 41);
		ppu_thr_exit_data[1] = SC(2);
		ppu_thr_exit_data[2] = BCLR(0x10 | 0x04, 0, 0, 0);

		m_ppu_thr_stop = (u32)Memory.MainMem.AllocAlign(2 * 4);

		auto ppu_thr_stop_data = vm::ptr<be_t<u32>>::make(m_ppu_thr_stop);
		ppu_thr_stop_data[0] = SC(4);
		ppu_thr_stop_data[1] = BCLR(0x10 | 0x04, 0, 0, 0);

		vm::write64(Memory.PRXMem.AllocAlign(0x10000), 0xDEADBEEFABADCAFE);
	}
	break;

	default:
		thread.SetEntry(l.GetEntry());
	break;
	}

	m_status = Ready;

	GetGSManager().Init();
	GetCallbackManager().Init();
	GetAudioManager().Init();
	GetEventManager().Init();

	thread.Run();

	SendDbgCommand(DID_READY_EMU);
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
	SendDbgCommand(DID_START_EMU);

	//ConLog.Write("run...");
	m_status = Running;

	//if(m_memory_viewer && m_memory_viewer->exit) safe_delete(m_memory_viewer);

	//m_memory_viewer->SetPC(loader.GetEntry());
	//m_memory_viewer->Show();
	//m_memory_viewer->ShowPC();

	GetCPU().Exec();
	SendDbgCommand(DID_STARTED_EMU);
}

void Emulator::Pause()
{
	if(!IsRunning()) return;
	//ConLog.Write("pause...");
	SendDbgCommand(DID_PAUSE_EMU);

	if (InterlockedCompareExchange((volatile u32*)&m_status, Paused, Running) == Running)
	{
		SendDbgCommand(DID_PAUSED_EMU);
	}
}

void Emulator::Resume()
{
	if(!IsPaused()) return;
	//ConLog.Write("resume...");
	SendDbgCommand(DID_RESUME_EMU);

	m_status = Running;

	CheckStatus();
	//if(IsRunning() && Ini.CPUDecoderMode.GetValue() != 1) GetCPU().Exec();
	SendDbgCommand(DID_RESUMED_EMU);
}

void Emulator::Stop()
{
	if(IsStopped()) return;
	//ConLog.Write("shutdown...");

	SendDbgCommand(DID_STOP_EMU);
	m_status = Stopped;

	u32 uncounted = 0;
	u32 counter = 0;
	while (true)
	{
		if (g_thread_count <= uncounted)
		{
			LOG_NOTICE(HLE, "All threads stopped...");
			break;
		}
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	m_rsx_callback = 0;

	// TODO: check finalization order

	SavePoints(BreakPointsDBName);
	m_break_points.clear();
	m_marked_points.clear();

	GetVFS().UnMountAll();

	GetGSManager().Close();
	GetAudioManager().Close();
	GetEventManager().Clear();
	GetCPU().Close();
	GetIdManager().Clear();
	GetPadManager().Close();
	GetKeyboardManager().Close();
	GetMouseManager().Close();
	GetCallbackManager().Clear();
	GetModuleManager().UnloadModules();
	GetSFuncManager().StaticFinalize();

	CurGameInfo.Reset();
	Memory.Close();

	//if(m_memory_viewer && m_memory_viewer->IsShown()) m_memory_viewer->Hide();
	SendDbgCommand(DID_STOPPED_EMU);
}

void Emulator::SavePoints(const std::string& path)
{
	std::ofstream f(path, std::ios::binary | std::ios::trunc);

	u32 break_count = (u32)m_break_points.size();
	u32 marked_count = (u32)m_marked_points.size();

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
	int length = (int)f.tellg();
	f.seekg(0, std::ios::beg);
	u32 break_count, marked_count;
	u16 version;
	f >> version >> break_count >> marked_count;

	if(version != bpdb_version ||
		(sizeof(u16) + break_count * sizeof(u64) + sizeof(u32) + marked_count * sizeof(u64) + sizeof(u32)) != length)
	{
		LOG_ERROR(LOADER, "'%s' is broken", path.c_str());
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

CallAfterCbType CallAfterCallback = nullptr;

void CallAfter(std::function<void()> func)
{
	CallAfterCallback(func);
}

void SetCallAfterCallback(CallAfterCbType cb)
{
	CallAfterCallback = cb;
}