#include "stdafx.h"
#include "Utilities/Log.h"
#include "Utilities/rFile.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/GameInfo.h"
#include "Emu/ARMv7/PSVFuncList.h"
#include "Emu/ARMv7/PSVObjectList.h"
#include "Emu/SysCalls/Static.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsLocalFile.h"
#include "Emu/FS/vfsDeviceLocalFile.h"
#include "Emu/DbgCommand.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/IdManager.h"
#include "Emu/Io/Pad.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Mouse.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/FS/VFS.h"
#include "Emu/SysCalls/SyncPrimitivesManager.h"

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
	, m_sync_prim_manager(new SyncPrimManager())
	, m_vfs(new VFS())
{
	m_loader.register_handler(new loader::handlers::elf32);
	m_loader.register_handler(new loader::handlers::elf64);
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
	delete m_sync_prim_manager;
	delete m_vfs;
}

void Emulator::Init()
{
	while(m_modules_init.size())
	{
		m_modules_init[0]->Init();
		m_modules_init.erase(m_modules_init.begin());
	}
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

void Emulator::SetTitle(const std::string& title)
{
	m_title = title;
}

void Emulator::CheckStatus()
{
	//auto& threads = GetCPU().GetThreads();
	//if (!threads.size())
	//{
	//	Stop();
	//	return;	
	//}

	//bool IsAllPaused = true;
	//for (u32 i = 0; i < threads.size(); ++i)
	//{
	//	if (threads[i]->IsPaused()) continue;
	//	IsAllPaused = false;
	//	break;
	//}

	//if(IsAllPaused)
	//{
	//	//ConLog.Warning("all paused!");
	//	Pause();
	//	return;
	//}

	//bool IsAllStoped = true;
	//for (u32 i = 0; i < threads.size(); ++i)
	//{
	//	if (threads[i]->IsStopped()) continue;
	//	IsAllStoped = false;
	//	break;
	//}

	//if (IsAllStoped)
	//{
	//	//LOG_WARNING(GENERAL, "all stoped!");
	//	Pause(); //Stop();
	//}
}

bool Emulator::BootGame(const std::string& path, bool direct)
{
	static const char* elf_path[6] =
	{
		"/PS3_GAME/USRDIR/BOOT.BIN",
		"/USRDIR/BOOT.BIN",
		"/BOOT.BIN",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN"
	};
	auto curpath = path;

	if (direct)
	{
		if (rFile::Access(curpath, rFile::read))
		{
			SetPath(curpath);
			Load();

			return true;
		}
	}

	for (int i = 0; i < sizeof(elf_path) / sizeof(*elf_path); i++)
	{
		curpath = path + elf_path[i];
		
		if (rFile::Access(curpath, rFile::read))
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

	if (!rExists(m_path)) return;

	if (IsSelf(m_path))
	{
		std::string elf_path = rFileName(m_path).GetPath();

		if (fmt::CmpNoCase(rFileName(m_path).GetFullName(),"EBOOT.BIN") == 0)
		{
			elf_path += "/BOOT.BIN";
		}
		else
		{
			elf_path += "/" + rFileName(m_path).GetName() + ".elf";
		}

		if (!DecryptSelf(elf_path, m_path))
			return;

		m_path = elf_path;
	}

	LOG_NOTICE(LOADER, "Loading '%s'...", m_path.c_str());
	GetInfo().Reset();
	GetVFS().Init(rFileName(m_path).GetPath());

	LOG_NOTICE(LOADER, " "); //used to be skip_line
	LOG_NOTICE(LOADER, "Mount info:");
	for (uint i = 0; i < GetVFS().m_devices.size(); ++i)
	{
		LOG_NOTICE(LOADER, "%s -> %s", GetVFS().m_devices[i]->GetPs3Path().c_str(), GetVFS().m_devices[i]->GetLocalPath().c_str());
	}

	LOG_NOTICE(LOADER, " "); //used to be skip_line
	vfsFile sfo("/app_home/../PARAM.SFO");
	PSFLoader psf(sfo);
	psf.Load(false);
	std::string title = psf.GetString("TITLE");
	std::string title_id = psf.GetString("TITLE_ID");
	LOG_NOTICE(LOADER, "Title: %s", title.c_str());
	LOG_NOTICE(LOADER, "Serial: %s", title_id.c_str());

	title.length() ? SetTitle(title) : SetTitle(m_path);
	SetTitleID(title_id);

	// bdvd inserting imitation
	vfsFile f1("/app_home/../dev_bdvd.path");
	if (f1.IsOpened())
	{
		std::string bdvd;
		bdvd.resize(f1.GetSize());
		f1.Read(&bdvd[0], bdvd.size());

		// load desired /dev_bdvd/ real directory and remount
		Emu.GetVFS().Mount("/dev_bdvd/", bdvd, new vfsDeviceLocalFile());
		LOG_NOTICE(LOADER, "/dev_bdvd/ remounted into %s", bdvd.c_str());
	}
	LOG_NOTICE(LOADER, " "); //used to be skip_line

	if (m_elf_path.empty())
	{
		GetVFS().GetDeviceLocal(m_path, m_elf_path);
	}

	vfsFile f(m_elf_path);

	if (!f.IsOpened())
	{
		LOG_ERROR(LOADER, "Elf not found! (%s - %s)", m_path.c_str(), m_elf_path.c_str());
		return;
	}

	if (!m_loader.load(f))
	{
		LOG_ERROR(LOADER, "Loading '%s' failed", m_path.c_str());
		vm::close();
		return;
	}

	// trying to load some info from PARAM.SFO
	vfsFile f2("/app_home/../PARAM.SFO");
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

	m_status = Ready;

	GetGSManager().Init();
	GetCallbackManager().Init();
	GetAudioManager().Init();
	GetEventManager().Init();

	SendDbgCommand(DID_READY_EMU);
}

void Emulator::Run()
{
	if (!IsReady())
	{
		Load();
		if(!IsReady()) return;
	}

	if (IsRunning()) Stop();

	if (IsPaused())
	{
		Resume();
		return;
	}

	SendDbgCommand(DID_START_EMU);

	m_status = Running;

	GetCPU().Exec();
	SendDbgCommand(DID_STARTED_EMU);
}

void Emulator::Pause()
{
	if (!IsRunning()) return;
	SendDbgCommand(DID_PAUSE_EMU);

	if (InterlockedCompareExchange((volatile u32*)&m_status, Paused, Running) == Running)
	{
		SendDbgCommand(DID_PAUSED_EMU);

		GetCallbackManager().RunPauseCallbacks(true);
	}
}

void Emulator::Resume()
{
	if (!IsPaused()) return;
	SendDbgCommand(DID_RESUME_EMU);

	m_status = Running;

	CheckStatus();

	SendDbgCommand(DID_RESUMED_EMU);

	GetCallbackManager().RunPauseCallbacks(false);
}

extern std::map<u32, std::string> g_armv7_dump;

void Emulator::Stop()
{
	if(IsStopped()) return;

	SendDbgCommand(DID_STOP_EMU);
	m_status = Stopped;

	while (g_thread_count)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}

	LOG_NOTICE(HLE, "All threads stopped...");

	finalize_psv_modules();
	clear_all_psv_objects();

	for (auto& v : g_armv7_dump)
	{
		LOG_NOTICE(ARMv7, v.second);
	}

	g_armv7_dump.clear();

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
	GetSyncPrimManager().Close();

	CurGameInfo.Reset();
	Memory.Close();

	SendDbgCommand(DID_STOPPED_EMU);
}

void Emulator::SavePoints(const std::string& path)
{
	std::ofstream f(path, std::ios::binary | std::ios::trunc);

	u32 break_count = (u32)m_break_points.size();
	u32 marked_count = (u32)m_marked_points.size();

	f << bpdb_version << break_count << marked_count;
	
	if (break_count)
	{
		f.write(reinterpret_cast<char*>(&m_break_points[0]), sizeof(u64) * break_count);
	}

	if (marked_count)
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

	if (version != bpdb_version || (sizeof(u16) + break_count * sizeof(u64) + sizeof(u32) + marked_count * sizeof(u64) + sizeof(u32)) != length)
	{
		LOG_ERROR(LOADER, "'%s' is broken", path.c_str());
		return;
	}

	if (break_count > 0)
	{
		m_break_points.resize(break_count);
		f.read(reinterpret_cast<char*>(&m_break_points[0]), sizeof(u64) * break_count);
	}

	if (marked_count > 0)
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
