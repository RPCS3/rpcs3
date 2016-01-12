#include "stdafx.h"

#include "config.h"
#include "events.h"
#include "state.h"

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

using namespace PPU_instr;

static const std::string& BreakPointsDBName = "BreakPoints.dat";
static const u16 bpdb_version = 0x1000;

// Draft (not used)
struct bpdb_header_t
{
	le_t<u32> magic;
	le_t<u32> version;
	le_t<u32> count;
	le_t<u32> marked;

	// POD
	bpdb_header_t() = default;

	bpdb_header_t(u32 count, u32 marked)
		: magic(*reinterpret_cast<const u32*>("BPDB"))
		, version(0x00010000)
		, count(count)
		, marked(marked)
	{
	}
};

extern std::atomic<u32> g_thread_count;

extern u64 get_system_time();
extern void finalize_psv_modules();

Emulator::Emulator()
	: m_status(Stopped)
	, m_mode(DisAsm)
	, m_rsx_callback(0)
	, m_cpu_thr_stop(0)
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

void Emulator::CreateConfig(const std::string& name)
{
	const std::string& path = fs::get_config_dir() + "data/" + name;
	const std::string& ini_file = path + "/settings.ini";

	if (!fs::is_dir("data"))
		fs::create_dir("data");

	if (!fs::is_dir(path))
		fs::create_dir(path);

	if (!fs::is_file(ini_file))
		rpcs3::config_t{ ini_file }.save();
}

bool Emulator::BootGame(const std::string& path, bool direct)
{
	static const char* boot_list[] =
	{
		"/PS3_GAME/USRDIR/BOOT.BIN",
		"/USRDIR/BOOT.BIN",
		"/BOOT.BIN",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
	};

	if (direct && fs::is_file(path))
	{
		SetPath(path);
		Load();

		return true;
	}

	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			SetPath(elf);
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

	const std::string& elf_dir = fs::get_parent_dir(m_path);

	if (IsSelf(m_path))
	{
		const std::size_t elf_ext_pos = m_path.find_last_of('.');
		const std::string& elf_ext = fmt::toupper(m_path.substr(elf_ext_pos != -1 ? elf_ext_pos : m_path.size()));
		const std::string& elf_name = m_path.substr(elf_dir.size());

		if (elf_name.compare(elf_name.find_last_of("/\\", -1, 2) + 1, 9, "EBOOT.BIN", 9) == 0)
		{
			m_path.erase(m_path.size() - 9, 1); // change EBOOT.BIN to BOOT.BIN
		}
		else if (elf_ext == ".SELF" || elf_ext == ".SPRX")
		{
			m_path.erase(m_path.size() - 4, 1); // change *.self to *.elf, *.sprx to *.prx
		}
		else
		{
			m_path += ".decrypted.elf";
		}

		if (!DecryptSelf(m_path, elf_dir + elf_name))
		{
			m_status = Stopped;
			return;
		}
	}

	ResetInfo();
	GetVFS().Init(elf_dir);

	LOG_NOTICE(LOADER, "Loading '%s'...", m_path.c_str());

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
	else if (fs::is_file(elf_dir + "/../../PS3_DISC.SFB")) // guess loading disc game
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
	f.Open("/app_home/../PARAM.SFO");
	const PSFLoader psf(f);
	std::string title = psf.GetString("TITLE");
	std::string title_id = psf.GetString("TITLE_ID");
	LOG_NOTICE(LOADER, "Title: %s", title.c_str());
	LOG_NOTICE(LOADER, "Serial: %s", title_id.c_str());

	title.length() ? SetTitle(title) : SetTitle(m_path);
	SetTitleID(title_id);

	rpcs3::state.config = rpcs3::config;

	// load custom config
	if (!rpcs3::config.misc.use_default_ini.value())
	{
		if (title_id.size())
		{
			title_id = title_id.substr(0, 4) + "-" + title_id.substr(4, 5);
			CreateConfig(title_id);
			rpcs3::config_t custom_config { fs::get_config_dir() + "data/" + title_id + "/settings.ini" };
			custom_config.load();
			rpcs3::state.config = custom_config;
		}
	}

	LOG_NOTICE(LOADER, "Used configuration: '%s'", rpcs3::state.config.path().c_str());
	LOG_NOTICE(LOADER, "");
	LOG_NOTICE(LOADER, rpcs3::state.config.to_string().c_str());

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
	
	LoadPoints(fs::get_config_dir() + BreakPointsDBName);

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
		LOG_NOTICE(ARMv7, "%s", v.second);
	}

	m_rsx_callback = 0;
	m_cpu_thr_stop = 0;

	// TODO: check finalization order

	SavePoints(fs::get_config_dir() + BreakPointsDBName);
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

	RSXIOMem.Clear();
	vm::close();

	SendDbgCommand(DID_STOPPED_EMU);
}

void Emulator::SavePoints(const std::string& path)
{
	const u32 break_count = size32(m_break_points);
	const u32 marked_count = size32(m_marked_points);

	fs::file(path, fom::rewrite)
		.write(bpdb_version)
		.write(break_count)
		.write(marked_count)
		.write(m_break_points)
		.write(m_marked_points);
}

bool Emulator::LoadPoints(const std::string& path)
{
	if (fs::file f{ path })
	{
		u16 version;
		u32 break_count;
		u32 marked_count;

		if (!f.read(version) || !f.read(break_count) || !f.read(marked_count))
		{
			LOG_ERROR(LOADER, "BP file '%s' is broken (length=0x%llx)", path, f.size());
			return false;
		}

		if (version != bpdb_version)
		{
			LOG_ERROR(LOADER, "BP file '%s' has unsupported version (version=0x%x)", path, version);
			return false;
		}

		m_break_points.resize(break_count);
		m_marked_points.resize(marked_count);

		if (!f.read(m_break_points) || !f.read(m_marked_points))
		{
			LOG_ERROR(LOADER, "'BP file %s' is broken (length=0x%llx, break_count=%u, marked_count=%u)", path, f.size(), break_count, marked_count);
			return false;
		}

		return true;
	}

	return false;
}

Emulator Emu;
