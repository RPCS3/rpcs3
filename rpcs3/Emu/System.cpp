#include "stdafx.h"
#include "Utilities/Config.h"
#include "Utilities/AutoPause.h"
#include "Utilities/event.h"
#include "Emu/Memory/Memory.h"
#include "Emu/System.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/PSP2/ARMv7Thread.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"

#include "Loader/PSF.h"
#include "Loader/ELF.h"

#include "Utilities/StrUtil.h"

#include "../Crypto/unself.h"

#include <thread>

cfg::bool_entry g_cfg_autostart(cfg::root.misc, "Always start after boot", true);
cfg::bool_entry g_cfg_autoexit(cfg::root.misc, "Exit RPCS3 when process finishes");

std::string g_cfg_defaults;

extern cfg::string_entry g_cfg_vfs_dev_bdvd;
extern cfg::string_entry g_cfg_vfs_app_home;

extern atomic_t<u32> g_thread_count;

extern atomic_t<u32> g_ppu_core[2];

extern u64 get_system_time();

fs::file g_tty;

namespace rpcs3
{
	event<void>& on_run() { static event<void> on_run; return on_run; }
	event<void>& on_stop() { static event<void> on_stop; return on_stop; }
	event<void>& on_pause() { static event<void> on_pause; return on_pause; }
	event<void>& on_resume() { static event<void> on_resume; return on_resume; }
}

Emulator::Emulator()
	: m_status(Stopped)
	, m_cpu_thr_stop(0)
	, m_callback_manager(new CallbackManager())
{
}

void Emulator::Init()
{
	if (!g_tty)
	{
		g_tty.open(fs::get_config_dir() + "TTY.log", fs::rewrite + fs::append);
	}
	
	idm::init();
	fxm::init();

	g_ppu_core[0] = 0;
	g_ppu_core[1] = 0;

	// Reset defaults, cache them
	cfg::root.from_default();
	g_cfg_defaults = cfg::root.to_string();

	// Reload global configuration
	cfg::root.from_string(fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::create).to_string());
}

void Emulator::SetPath(const std::string& path, const std::string& elf_path)
{
	m_path = path;
	m_elf_path = elf_path;
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
	Stop();

	try
	{
		Init();

		if (!fs::is_file(m_path))
		{
			LOG_ERROR(LOADER, "File not found: %s", m_path);
			return;
		}

		const std::string& elf_dir = fs::get_parent_dir(m_path);

		if (IsSelf(m_path))
		{
			const std::size_t elf_ext_pos = m_path.find_last_of('.');
			const std::string& elf_ext = fmt::to_upper(m_path.substr(elf_ext_pos != -1 ? elf_ext_pos : m_path.size()));
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
				LOG_ERROR(LOADER, "Failed to decrypt %s", elf_dir + elf_name);
				return;
			}
		}

		ResetInfo();

		LOG_NOTICE(LOADER, "Path: %s", m_path);

		// Load custom config
		if (fs::file cfg_file{ m_path + ".yml" })
		{
			LOG_NOTICE(LOADER, "Custom config: %s.yml", m_path);
			cfg::root.from_string(cfg_file.to_string());
		}

		const fs::file elf_file(m_path);
		ppu_exec_loader ppu_exec;
		ppu_prx_loader ppu_prx;
		spu_exec_loader spu_exec;
		arm_exec_loader arm_exec;

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to open %s", m_path);
			return;
		}
		else if (ppu_exec.open(elf_file) == elf_error::ok)
		{
			// PS3 executable
			m_status = Ready;
			vm::ps3::init();

			if (m_elf_path.empty())
			{
				m_elf_path = "/host_root/" + m_path;
				LOG_NOTICE(LOADER, "Elf path: %s", m_elf_path);
			}

			// Load PARAM.SFO
			const auto _psf = psf::load_object(fs::file(elf_dir + "/../PARAM.SFO"));
			m_title = psf::get_string(_psf, "TITLE", m_path);
			m_title_id = psf::get_string(_psf, "TITLE_ID");

			LOG_NOTICE(LOADER, "Title: %s", GetTitle());
			LOG_NOTICE(LOADER, "Serial: %s", GetTitleID());
			LOG_NOTICE(LOADER, "");

			LOG_NOTICE(LOADER, "Used configuration:\n%s\n", cfg::root.to_string());

			// Mount /dev_bdvd/
			if (g_cfg_vfs_dev_bdvd.size() == 0 && fs::is_file(elf_dir + "/../../PS3_DISC.SFB"))
			{
				const auto dir_list = fmt::split(elf_dir, { "/", "\\" });

				// Check latest two directories
				if (dir_list.size() >= 2 && dir_list.back() == "USRDIR" && *(dir_list.end() - 2) == "PS3_GAME")
				{
					g_cfg_vfs_dev_bdvd = elf_dir.substr(0, elf_dir.length() - 15);
				}
				else
				{
					g_cfg_vfs_dev_bdvd = elf_dir + "/../../";
				}
			}

			// Mount /app_home/
			if (g_cfg_vfs_app_home.size() == 0)
			{
				g_cfg_vfs_app_home = elf_dir + '/';
			}

			vfs::dump();

			ppu_exec.load();

			Emu.GetCallbackManager().Init();
			fxm::import<GSRender>(PURE_EXPR(Emu.GetCallbacks().get_gs_render())); // TODO: must be created in appropriate sys_rsx syscall
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			m_status = Ready;
			vm::ps3::init();
			ppu_prx.load();
			GetCallbackManager().Init();
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			m_status = Ready;
			vm::ps3::init();
			spu_exec.load();
		}
		else if (arm_exec.open(elf_file) == elf_error::ok)
		{
			// ARMv7 executable
			m_status = Ready;
			vm::psv::init();
			arm_exec.load();
		}
		else
		{
			LOG_ERROR(LOADER, "Invalid or unsupported file format: %s", m_path);

			LOG_WARNING(LOADER, "** ppu_exec_loader -> %s", ppu_exec.get_error());
			LOG_WARNING(LOADER, "** ppu_prx_loader -> %s", ppu_prx.get_error());
			LOG_WARNING(LOADER, "** spu_exec_loader -> %s", spu_exec.get_error());
			LOG_WARNING(LOADER, "** arm_exec_loader -> %s", arm_exec.get_error());
			return;
		}

		debug::autopause::reload();
		SendDbgCommand(DID_READY_EMU);
		if (g_cfg_autostart) Run();
	}
	catch (const std::exception& e)
	{
		LOG_FATAL(LOADER, "%s thrown: %s", typeid(e).name(), e.what());
		Stop();
	}
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

	rpcs3::on_run()();

	SendDbgCommand(DID_START_EMU);

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_status = Running;

	idm::select<PPUThread, SPUThread, RawSPUThread, ARMv7Thread>([](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_state::stop;
		cpu->lock_notify();
	});

	SendDbgCommand(DID_STARTED_EMU);
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// Try to pause
	if (!m_status.compare_and_swap_test(Running, Paused))
	{
		return false;
	}

	rpcs3::on_pause()();

	// Update pause start time
	if (m_pause_start_time.exchange(start))
	{
		LOG_ERROR(GENERAL, "Emulator::Pause() error: concurrent access");
	}

	SendDbgCommand(DID_PAUSE_EMU);

	idm::select<PPUThread, SPUThread, RawSPUThread, ARMv7Thread>([](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_state::dbg_global_pause;
	});

	SendDbgCommand(DID_PAUSED_EMU);

	return true;
}

void Emulator::Resume()
{
	// Get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// Try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}

	// Try to resume
	if (!m_status.compare_and_swap_test(Paused, Running))
	{
		return;
	}

	if (!time)
	{
		LOG_ERROR(GENERAL, "Emulator::Resume() error: concurrent access");
	}

	SendDbgCommand(DID_RESUME_EMU);

	idm::select<PPUThread, SPUThread, RawSPUThread, ARMv7Thread>([](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_state::dbg_global_pause;
		cpu->lock_notify();
	});

	rpcs3::on_resume()();

	SendDbgCommand(DID_RESUMED_EMU);
}

void Emulator::Stop()
{
	if (m_status.exchange(Stopped) == Stopped)
	{
		return;
	}

	LOG_NOTICE(GENERAL, "Stopping emulator...");

	rpcs3::on_stop()();
	SendDbgCommand(DID_STOP_EMU);

	{
		LV2_LOCK;

		idm::select<PPUThread, SPUThread, RawSPUThread, ARMv7Thread>([](u32, cpu_thread& cpu)
		{
			cpu.state += cpu_state::dbg_global_stop;
			cpu->lock();
			cpu->set_exception(std::make_exception_ptr(EmulationStopped()));
			cpu->unlock();
			cpu->notify();
		});
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

	GetCallbackManager().Clear();

	RSXIOMem.Clear();
	vm::close();

	SendDbgCommand(DID_STOPPED_EMU);

	if (g_cfg_autoexit)
	{
		GetCallbacks().exit();
	}
	else
	{
		Init();
	}
}

Emulator Emu;
