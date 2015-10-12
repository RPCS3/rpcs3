#include "stdafx.h"

#include "config.h"
#include "events.h"
#include "state.h"

#include "Utilities/Log.h"
#include "Utilities/File.h"
#include "rpcs3/Ini.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/GameInfo.h"
#include "Emu/SysCalls/ModuleManager.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/PPUInstrTable.h"
#include "Emu/FS/vfsFile.h"
#include "Emu/FS/vfsLocalFile.h"
#include "Emu/FS/vfsDeviceLocalFile.h"

#include "Emu/CPU/CPUThreadManager.h"
#include "Emu/SysCalls/Callback.h"
#include "Emu/IdManager.h"
#include "Emu/Io/Pad.h"
#include "Emu/Io/Keyboard.h"
#include "Emu/Io/Mouse.h"
#include "Emu/RSX/GSManager.h"
#include "Emu/Audio/AudioManager.h"
#include "Emu/FS/VFS.h"
#include "Emu/Event.h"

#include "Loader/PSF.h"
#include "Loader/ELF64.h"
#include "Loader/ELF32.h"

#include "../Crypto/unself.h"
#include <fstream>
using namespace PPU_instr;

static const std::string& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;
extern std::atomic<u32> g_thread_count;

extern u64 get_system_time();
extern void finalize_psv_modules();

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_rsx_callback(0)
	, m_thread_manager(new CPUThreadManager())
	, m_pad_manager(new PadManager())
	, m_keyboard_manager(new KeyboardManager())
	, m_mouse_manager(new MouseManager())
	, m_gs_manager(new GSManager())
	, m_audio_manager(new AudioManager())
	, m_callback_manager(new CallbackManager())
	, m_event_manager(new EventManager())
	, m_module_manager(new ModuleManager())
	, m_vfs(new VFS())
{
	m_loader.register_handler(new loader::handlers::elf32);
	m_loader.register_handler(new loader::handlers::elf64);
}

void Emulator::Init()
{
	rpcs3::config.load();
	rpcs3::oninit();
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
		if (fs::is_file(curpath))
		{
			SetPath(curpath);
			Load();

			return true;
		}
	}

	for (int i = 0; i < sizeof(elf_path) / sizeof(*elf_path); i++)
	{
		curpath = path + elf_path[i];
		
		if (fs::is_file(curpath))
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
	m_status = Ready;

	if (!fs::is_file(m_path))
	{
		m_status = Stopped;
		return;
	}

	const std::string elf_dir = m_path.substr(0, m_path.find_last_of("/\\", std::string::npos, 2) + 1);

	if (IsSelf(m_path))
	{
		const std::string full_name = m_path.substr(elf_dir.length());

		const std::string base_name = full_name.substr(0, full_name.find_last_of('.', std::string::npos));

		const std::string ext = full_name.substr(base_name.length());

		if (fmt::toupper(full_name) == "EBOOT.BIN")
		{
			m_path = elf_dir + "BOOT.BIN";
		}
		else if (fmt::toupper(ext) == ".SELF")
		{
			m_path = elf_dir + base_name + ".elf";
		}
		else if (fmt::toupper(ext) == ".SPRX")
		{
			m_path = elf_dir + base_name + ".prx";
		}
		else
		{
			m_path = elf_dir + base_name + ".decrypted" + ext;
		}

		LOG_NOTICE(LOADER, "Decrypting '%s%s'...", elf_dir, full_name);

		if (!DecryptSelf(m_path, elf_dir + full_name))
		{
			m_status = Stopped;
			return;
		}
	}

	//TODO: load custom config if exists
	rpcs3::state.config = rpcs3::config;

	LOG_NOTICE(LOADER, "Loading '%s'...", m_path.c_str());
	ResetInfo();
	GetVFS().Init(elf_dir);

	// /dev_bdvd/ mounting
	vfsFile f("/app_home/../dev_bdvd.path");
	if (f.IsOpened())
	{
		// load specified /dev_bdvd/ directory and mount it
		std::string bdvd;
		bdvd.resize(f.GetSize());
		f.Read(&bdvd[0], bdvd.size());

		Emu.GetVFS().Mount("/dev_bdvd/", bdvd, new vfsDeviceLocalFile());
	}
	else if (fs::is_file(elf_dir + "../../PS3_DISC.SFB")) // guess loading disc game
	{
		const auto dir_list = fmt::split(elf_dir, { "/", "\\" });

		// check latest two directories
		if (dir_list.size() >= 2 && dir_list.back() == "USRDIR" && *(dir_list.end() - 2) == "PS3_GAME")
		{
			// mount detected /dev_bdvd/ directory
			Emu.GetVFS().Mount("/dev_bdvd/", elf_dir.substr(0, elf_dir.length() - 17), new vfsDeviceLocalFile());
		}
	}

	LOG_NOTICE(LOADER, "");
	LOG_NOTICE(LOADER, "Mount info:");
	for (uint i = 0; i < GetVFS().m_devices.size(); ++i)
	{
		LOG_NOTICE(LOADER, "%s -> %s", GetVFS().m_devices[i]->GetPs3Path().c_str(), GetVFS().m_devices[i]->GetLocalPath().c_str());
	}

	LOG_NOTICE(LOADER, "");

	LOG_NOTICE(LOADER, "Settings:");
	LOG_NOTICE(LOADER, "CPU: %s", Ini.CPUIdToString(Ini.CPUDecoderMode.GetValue()));
	LOG_NOTICE(LOADER, "SPU: %s", Ini.SPUIdToString(Ini.SPUDecoderMode.GetValue()));
	LOG_NOTICE(LOADER, "Renderer: %s", Ini.RendererIdToString(Ini.GSRenderMode.GetValue()));

	if (Ini.GSRenderMode.GetValue() == 2)
	{
		LOG_NOTICE(LOADER, "D3D Adapter: %s", Ini.AdapterIdToString(Ini.GSD3DAdaptater.GetValue()));
	}

	LOG_NOTICE(LOADER, "Resolution: %s", Ini.ResolutionIdToString(Ini.GSResolution.GetValue()));
	/*LOG_NOTICE(LOADER, "Write Depth Buffer: %s", Ini.GSDumpDepthBuffer.GetValue() ? "Yes" : "No");
	LOG_NOTICE(LOADER, "Write Color Buffers: %s", Ini.GSDumpColorBuffers.GetValue() ? "Yes" : "No");
	LOG_NOTICE(LOADER, "Read Color Buffers: %s", Ini.GSReadColorBuffers.GetValue() ? "Yes" : "No");
	LOG_NOTICE(LOADER, "Read Depth Buffer: %s", Ini.GSReadDepthBuffer.GetValue() ? "Yes" : "No");*/
	LOG_NOTICE(LOADER, "Audio Out: %s", Ini.AudioOutIdToString(Ini.AudioOutMode.GetValue()));
	LOG_NOTICE(LOADER, "Log Everything: %s", Ini.HLELogging.GetValue() ? "Yes" : "No");
	LOG_NOTICE(LOADER, "RSX Logging: %s", Ini.RSXLogging.GetValue() ? "Yes" : "No");

	LOG_NOTICE(LOADER, "");

	LOG_NOTICE(LOADER, rpcs3::config.to_string().c_str());

	LOG_NOTICE(LOADER, "");
	f.Open("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	std::string title = psf.GetString("TITLE");
	std::string title_id = psf.GetString("TITLE_ID");
	LOG_NOTICE(LOADER, "Title: %s", title.c_str());
	LOG_NOTICE(LOADER, "Serial: %s", title_id.c_str());

	title.length() ? SetTitle(title) : SetTitle(m_path);
	SetTitleID(title_id);

	if (m_elf_path.empty())
	{
		GetVFS().GetDeviceLocal(m_path, m_elf_path);

		LOG_NOTICE(LOADER, "Elf path: %s", m_elf_path);
		LOG_NOTICE(LOADER, "");
	}

	f.Open(m_elf_path);

	if (!f.IsOpened())
	{
		LOG_ERROR(LOADER, "Opening '%s' failed", m_path.c_str());
		m_status = Stopped;
		return;
	}

	if (!m_loader.load(f))
	{
		LOG_ERROR(LOADER, "Loading '%s' failed", m_path.c_str());
		LOG_NOTICE(LOADER, "");
		m_status = Stopped;
		vm::close();
		return;
	}
	
	LoadPoints(BreakPointsDBName);

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

	rpcs3::onstart();

	SendDbgCommand(DID_START_EMU);

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_status = Running;

	GetCPU().Exec();
	SendDbgCommand(DID_STARTED_EMU);
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// try to set Paused status
	if (!sync_bool_compare_and_swap(&m_status, Running, Paused))
	{
		return false;
	}

	rpcs3::onpause();

	// update pause start time
	if (m_pause_start_time.exchange(start))
	{
		LOG_ERROR(GENERAL, "Emulator::Pause() error: concurrent access");
	}

	SendDbgCommand(DID_PAUSE_EMU);

	for (auto& t : GetCPU().GetAllThreads())
	{
		t->sleep(); // trigger status check
	}

	SendDbgCommand(DID_PAUSED_EMU);

	return true;
}

void Emulator::Resume()
{
	// get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}

	// try to resume
	if (!sync_bool_compare_and_swap(&m_status, Paused, Running))
	{
		return;
	}

	if (!time)
	{
		LOG_ERROR(GENERAL, "Emulator::Resume() error: concurrent access");
	}

	SendDbgCommand(DID_RESUME_EMU);

	for (auto& t : GetCPU().GetAllThreads())
	{
		t->awake(); // untrigger status check and signal
	}

	rpcs3::onstart();

	SendDbgCommand(DID_RESUMED_EMU);
}

extern std::map<u32, std::string> g_armv7_dump;

void Emulator::Stop()
{
	LOG_NOTICE(GENERAL, "Stopping emulator...");

	if (sync_lock_test_and_set(&m_status, Stopped) == Stopped)
	{
		return;
	}

	rpcs3::onstop();
	SendDbgCommand(DID_STOP_EMU);

	{
		LV2_LOCK;

		// notify all threads
		for (auto& t : GetCPU().GetAllThreads())
		{
			std::lock_guard<std::mutex> lock(t->mutex);

			t->sleep(); // trigger status check

			t->cv.notify_one(); // signal
		}
	}

	LOG_NOTICE(GENERAL, "All threads signaled...");

	while (g_thread_count)
	{
		m_cb.process_events();

		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All threads stopped...");

	idm::clear();
	fxm::clear();

	LOG_NOTICE(GENERAL, "Objects cleared...");

	finalize_psv_modules();

	for (auto& v : decltype(g_armv7_dump)(std::move(g_armv7_dump)))
	{
		LOG_NOTICE(ARMv7, v.second);
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
	GetPadManager().Close();
	GetKeyboardManager().Close();
	GetMouseManager().Close();
	GetCallbackManager().Clear();
	GetModuleManager().Close();

	CurGameInfo.Reset();
	RSXIOMem.Clear();
	vm::close();

	SendDbgCommand(DID_STOPPED_EMU);
}

void Emulator::SavePoints(const std::string& path)
{
	std::ofstream f(path, std::ios::binary | std::ios::trunc);

	u32 break_count = (u32)m_break_points.size();
	u32 marked_count = (u32)m_marked_points.size();

	f.write((char*)(&bpdb_version), sizeof(bpdb_version));
	f.write((char*)(&break_count), sizeof(break_count));
	f.write((char*)(&marked_count), sizeof(marked_count));
	
	if (break_count)
	{
		f.write((char*)(m_break_points.data()), sizeof(u64) * break_count);
	}

	if (marked_count)
	{
		f.write((char*)(m_marked_points.data()), sizeof(u64) * marked_count);
	}
}

bool Emulator::LoadPoints(const std::string& path)
{
	if (!fs::is_file(path)) return false;
	std::ifstream f(path, std::ios::binary);
	if (!f.is_open())
		return false;
	f.seekg(0, std::ios::end);
	u64 length = (u64)f.tellg();
	f.seekg(0, std::ios::beg);

	u16 version;
	u32 break_count, marked_count;

	u64 expected_length = sizeof(bpdb_version) + sizeof(break_count) + sizeof(marked_count);

	if (length < expected_length)
	{
		LOG_ERROR(LOADER,
			"'%s' breakpoint db is broken (file is too short, length=0x%x)",
			path, length);
		return false;
	}

	f.read((char*)(&version), sizeof(version));

	if (version != bpdb_version)
	{
		LOG_ERROR(LOADER,
			"'%s' breakpoint db version is unsupported (version=0x%x, length=0x%x)",
			path, version, length);
		return false;
	}

	f.read((char*)(&break_count), sizeof(break_count));
	f.read((char*)(&marked_count), sizeof(marked_count));
	expected_length += break_count * sizeof(u64) + marked_count * sizeof(u64);

	if (expected_length != length)
	{
		LOG_ERROR(LOADER,
			"'%s' breakpoint db format is incorrect "
			"(version=0x%x, break_count=0x%x, marked_count=0x%x, length=0x%x)",
			path, version, break_count, marked_count, length);
		return false;
	}

	if (break_count > 0)
	{
		m_break_points.resize(break_count);
		f.read((char*)(m_break_points.data()), sizeof(u64) * break_count);
	}

	if (marked_count > 0)
	{
		m_marked_points.resize(marked_count);
		f.read((char*)(m_marked_points.data()), sizeof(u64) * marked_count);
	}
	return true;
}

Emulator Emu;
