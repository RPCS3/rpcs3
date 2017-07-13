#include "stdafx.h"
#include "Utilities/event.h"
#include "Utilities/bin_patch.h"
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
#include "yaml-cpp/yaml.h"

#include <thread>

#include "Utilities/GDBDebugServer.h"

cfg_root g_cfg;

system_type g_system;

std::string g_cfg_defaults;

extern atomic_t<u32> g_thread_count;

extern u64 get_system_time();

extern void ppu_load_exec(const ppu_exec_object&);
extern void spu_load_exec(const spu_exec_object&);
extern void arm_load_exec(const arm_exec_object&);
extern std::shared_ptr<struct lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);

fs::file g_tty;

template <>
void fmt_class_string<mouse_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](mouse_handler value)
	{
		switch (value)
		{
		case mouse_handler::null: return "Null";
		case mouse_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<pad_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](pad_handler value)
	{
		switch (value)
		{
		case pad_handler::null: return "Null";
		case pad_handler::keyboard: return "Keyboard";
		case pad_handler::ds4: return "DualShock 4";
#ifdef _MSC_VER
		case pad_handler::xinput: return "XInput";
#endif
#ifdef _WIN32
		case pad_handler::mm: return "MMJoystick";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_renderer value)
	{
		switch (value)
		{
		case video_renderer::null: return "Null";
		case video_renderer::opengl: return "OpenGL";
		case video_renderer::vulkan: return "Vulkan";
#ifdef _MSC_VER
		case video_renderer::dx12: return "D3D12";
#endif
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_resolution>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_resolution value)
	{
		switch (value)
		{
		case video_resolution::_1080: return "1920x1080";
		case video_resolution::_720: return "1280x720";
		case video_resolution::_480: return "720x480";
		case video_resolution::_576: return "720x576";
		case video_resolution::_1600x1080: return "1600x1080";
		case video_resolution::_1440x1080: return "1440x1080";
		case video_resolution::_1280x1080: return "1280x1080";
		case video_resolution::_960x1080: return "960x1080";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<video_aspect>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](video_aspect value)
	{
		switch (value)
		{
		case video_aspect::_auto: return "Auto";
		case video_aspect::_4_3: return "4:3";
		case video_aspect::_16_9: return "16:9";
		}

		return unknown;
	});
}


template <>
void fmt_class_string<keyboard_handler>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](keyboard_handler value)
	{
		switch (value)
		{
		case keyboard_handler::null: return "Null";
		case keyboard_handler::basic: return "Basic";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<audio_renderer>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](audio_renderer value)
	{
		switch (value)
		{
		case audio_renderer::null: return "Null";
#ifdef _WIN32
		case audio_renderer::xaudio: return "XAudio2";
#elif __linux__
		case audio_renderer::alsa: return "ALSA";
#endif
		case audio_renderer::openal: return "OpenAL";
		}

		return unknown;
	});
}

void Emulator::Init()
{
	if (!g_tty)
	{
		g_tty.open(fs::get_config_dir() + "TTY.log", fs::rewrite + fs::append);
	}
	
	idm::init();
	fxm::init();

	// Reset defaults, cache them
	g_cfg.from_default();
	g_cfg_defaults = g_cfg.to_string();

	// Reload global configuration
	g_cfg.from_string(fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::create).to_string());

	// Create directories
	const std::string emu_dir_ = g_cfg.vfs.emulator_dir;
	const std::string emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
	const std::string dev_hdd0 = fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir);
	const std::string dev_hdd1 = fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir);
	const std::string dev_usb = fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir);

	fs::create_path(dev_hdd0);
	fs::create_dir(dev_hdd0 + "game/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/USRDIR/");
	fs::create_dir(dev_hdd0 + "home/");
	fs::create_dir(dev_hdd0 + "home/00000001/");
	fs::create_dir(dev_hdd0 + "home/00000001/exdata/");
	fs::create_dir(dev_hdd0 + "home/00000001/savedata/");
	fs::create_dir(dev_hdd0 + "home/00000001/trophy/");
	fs::write_file(dev_hdd0 + "home/00000001/localusername", fs::create + fs::excl + fs::write, "User"s);
	fs::create_dir(dev_hdd0 + "disc/");
	fs::create_dir(dev_hdd1 + "cache/");
	fs::create_dir(dev_hdd1 + "game/");
	fs::create_path(dev_hdd1);
	fs::create_path(dev_usb);
  
#ifdef WITH_GDB_DEBUGGER
	fxm::make<GDBDebugServer>();
#endif
	// Initialize patch engine
	fxm::make_always<patch_engine>()->append(fs::get_config_dir() + "/patch.yml");
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
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
		"/eboot.bin",
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

std::string Emulator::GetHddDir()
{
	const std::string& emu_dir_ = g_cfg.vfs.emulator_dir;
	const std::string& emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;

	return fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir);
}

std::string Emulator::GetLibDir()
{
	const std::string& emu_dir_ = g_cfg.vfs.emulator_dir;
	const std::string& emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;

	return fmt::replace_all(g_cfg.vfs.dev_flash, "$(EmulatorDir)", emu_dir) + "sys/external/";
}

void Emulator::Load()
{
	Stop();

	try
	{
		Init();

		// Load game list (maps ABCD12345 IDs to /dev_bdvd/ locations) 
		YAML::Node games = YAML::Load(fs::file{fs::get_config_dir() + "/games.yml", fs::read + fs::create}.to_string());

		if (!games.IsMap())
		{
			games.reset();
		}

		LOG_NOTICE(LOADER, "Path: %s", m_path);

		const std::string elf_dir = fs::get_parent_dir(m_path);

		// Load PARAM.SFO (TODO)
		const auto _psf = psf::load_object([&]
		{
			if (fs::file sfov{elf_dir + "/sce_sys/param.sfo"})
			{
				return sfov;
			}
			else
			{
				return fs::file(elf_dir + "/../PARAM.SFO");
			}
		}());
		m_title = psf::get_string(_psf, "TITLE", m_path);
		m_title_id = psf::get_string(_psf, "TITLE_ID");
		const auto _cat = psf::get_string(_psf, "CATEGORY");

		LOG_NOTICE(LOADER, "Title: %s", GetTitle());
		LOG_NOTICE(LOADER, "Serial: %s", GetTitleID());

		// Initialize data/cache directory
		m_cache_path = fs::get_data_dir(m_title_id, m_path);
		LOG_NOTICE(LOADER, "Cache: %s", GetCachePath());

		// Load custom config-0
		if (fs::file cfg_file{m_cache_path + "/config.yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: %s/config.yml", m_cache_path);
			g_cfg.from_string(cfg_file.to_string());
		}

		// Load custom config-1
		if (fs::file cfg_file{fs::get_config_dir() + "data/" + m_title_id + "/config.yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: data/%s/config.yml", m_title_id);
			g_cfg.from_string(cfg_file.to_string());
		}

		// Load custom config-2
		if (fs::file cfg_file{m_path + ".yml"})
		{
			LOG_NOTICE(LOADER, "Applying custom config: %s.yml", m_path);
			g_cfg.from_string(cfg_file.to_string());
		}

		LOG_NOTICE(LOADER, "Used configuration:\n%s\n", g_cfg.to_string());

		// Load patches from different locations
		fxm::check_unlocked<patch_engine>()->append(fs::get_config_dir() + "data/" + m_title_id + "/patch.yml");
		fxm::check_unlocked<patch_engine>()->append(m_cache_path + "/patch.yml");

		// Mount all devices
		const std::string emu_dir_ = g_cfg.vfs.emulator_dir;
		const std::string emu_dir = emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
		const std::string home_dir = g_cfg.vfs.app_home;
		std::string bdvd_dir = g_cfg.vfs.dev_bdvd;

		vfs::mount("dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_hdd1", fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_flash", fmt::replace_all(g_cfg.vfs.dev_flash, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_usb", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("dev_usb000", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("app_home", home_dir.empty() ? elf_dir + '/' : fmt::replace_all(home_dir, "$(EmulatorDir)", emu_dir));

		// Detect boot location
		const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
		const std::string hdd0_disc = vfs::get("/dev_hdd0/disc/");

		if (_cat == "DG" && m_path.find(hdd0_game + m_title_id + '/') != -1)
		{
			// Booting disc game from wrong location
			LOG_ERROR(LOADER, "Disc game found at invalid location: /dev_hdd0/game/%s/", m_title_id);

			// Move and retry from correct location
			if (fs::rename(hdd0_game + m_title_id, hdd0_disc + m_title_id))
			{
				LOG_SUCCESS(LOADER, "Disc game moved to special location: /dev_hdd0/disc/%s/", m_title_id);
				return SetPath(hdd0_disc + m_path.substr(hdd0_game.size())), Load();
			}
			else
			{
				LOG_ERROR(LOADER, "Failed to move disc game to /dev_hdd0/disc/%s/ (%s)", m_title_id, fs::g_tls_error);
				return;
			}
		}

		// Booting disc game
		if (_cat == "DG" && bdvd_dir.empty())
		{
			// Mount /dev_bdvd/ if necessary
			if (auto pos = elf_dir.rfind("/PS3_GAME") + 1)
			{
				bdvd_dir = elf_dir.substr(0, pos);
			}
		}

		// Booting patch data
		if (_cat == "GD" && bdvd_dir.empty())
		{
			// Load /dev_bdvd/ from game list if available
			if (auto node = games[m_title_id])
			{
				bdvd_dir = node.Scalar();
			}
			else
			{
				LOG_FATAL(LOADER, "Disc directory not found. Try to run the game from the actual game disc directory.");
			}
		}

		// Check /dev_bdvd/
		if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
		{
			fs::file sfb_file;

			vfs::mount("dev_bdvd", bdvd_dir);
			LOG_NOTICE(LOADER, "Disc: %s", vfs::get("/dev_bdvd"));

			if (!sfb_file.open(vfs::get("/dev_bdvd/PS3_DISC.SFB")) || sfb_file.size() < 4 || sfb_file.read<u32>() != ".SFB"_u32)
			{
				LOG_ERROR(LOADER, "Invalid disc directory for the disc game %s", m_title_id);
				return;
			}

			const std::string bdvd_title_id = psf::get_string(psf::load_object(fs::file{vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO")}), "TITLE_ID");

			if (bdvd_title_id != m_title_id)
			{
				LOG_ERROR(LOADER, "Unexpected disc directory for the disc game %s (found %s)", m_title_id, bdvd_title_id);
				return;
			}

			// Store /dev_bdvd/ location
			games[m_title_id] = bdvd_dir;
			YAML::Emitter out;
			out << games;
			fs::file(fs::get_config_dir() + "/games.yml", fs::rewrite).write(out.c_str(), out.size());
		}
		else if (_cat == "DG" || _cat == "GD")
		{
			LOG_ERROR(LOADER, "Failed to mount disc directory for the disc game %s", m_title_id);
			return;
		}

		// Check game updates
		const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN";

		if (_cat == "DG" && fs::is_file(hdd0_boot))
		{
			// Booting game update
			LOG_SUCCESS(LOADER, "Updates found at /dev_hdd0/game/%s/!", m_title_id);
			return SetPath(hdd0_boot), Load();
		}

		// Mount /host_root/ if necessary
		if (g_cfg.vfs.host_root)
		{
			vfs::mount("host_root", {});
		}

		// Open SELF or ELF
		fs::file elf_file(m_path);

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to open executable: %s", m_path);
			return;
		}

		// Check SELF header
		if (elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::string decrypted_path = m_cache_path + "boot.elf";

			fs::stat_t encrypted_stat = elf_file.stat();
			fs::stat_t decrypted_stat;

			// Check modification time and try to load decrypted ELF
			if (fs::stat(decrypted_path, decrypted_stat) && decrypted_stat.mtime == encrypted_stat.mtime)
			{
				elf_file.open(decrypted_path);
			}
			else
			{
				// Decrypt SELF
				elf_file = decrypt_self(std::move(elf_file));

				if (fs::file elf_out{decrypted_path, fs::rewrite})
				{
					elf_out.write(elf_file.to_vector<u8>());
					elf_out.close();
					fs::utime(decrypted_path, encrypted_stat.atime, encrypted_stat.mtime);
				}
				else
				{
					LOG_ERROR(LOADER, "Failed to create boot.elf");
				}
			}
		}

		ppu_exec_object ppu_exec;
		ppu_prx_object ppu_prx;
		spu_exec_object spu_exec;
		arm_exec_object arm_exec;

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to decrypt SELF: %s", m_path);
			return;
		}
		else if (ppu_exec.open(elf_file) == elf_error::ok)
		{
			// PS3 executable
			g_system = system_type::ps3;
			m_state = system_state::ready;
			GetCallbacks().on_ready();

			vm::ps3::init();

			if (m_elf_path.empty())
			{
				if (m_path.find(hdd0_game) != -1)
				{
					m_elf_path = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size());
				}
				else if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
				{
					//Disc games are on /dev_bdvd/
					size_t pos = m_path.rfind("PS3_GAME");
					m_elf_path = "/dev_bdvd/" + m_path.substr(pos);
				}
				else
				{
					//For homebrew
					m_elf_path = "/host_root/" + m_path;
				}

				LOG_NOTICE(LOADER, "Elf path: %s", m_elf_path);
			}

			ppu_load_exec(ppu_exec);

			fxm::import<GSRender>(Emu.GetCallbacks().get_gs_render); // TODO: must be created in appropriate sys_rsx syscall
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			g_system = system_type::ps3;
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::ps3::init();
			ppu_load_prx(ppu_prx, m_path);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			g_system = system_type::ps3;
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::ps3::init();
			spu_load_exec(spu_exec);
		}
		else if (arm_exec.open(elf_file) == elf_error::ok)
		{
			// ARMv7 executable
			g_system = system_type::psv;
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::psv::init();

			if (m_elf_path.empty())
			{
				m_elf_path = "host_root:" + m_path;
				LOG_NOTICE(LOADER, "Elf path: %s", m_elf_path);
			}

			arm_load_exec(arm_exec);
		}
		else
		{
			LOG_ERROR(LOADER, "Invalid or unsupported file format: %s", m_path);

			LOG_WARNING(LOADER, "** ppu_exec -> %s", ppu_exec.get_error());
			LOG_WARNING(LOADER, "** ppu_prx  -> %s", ppu_prx.get_error());
			LOG_WARNING(LOADER, "** spu_exec -> %s", spu_exec.get_error());
			LOG_WARNING(LOADER, "** arm_exec -> %s", arm_exec.get_error());
			return;
		}

		if (g_cfg.misc.autostart && IsReady())
		{
			Run();
		}
		else if (IsPaused())
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
		}
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

	
	GetCallbacks().on_run();

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_state = system_state::running;

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.run();
	};

	idm::select<ppu_thread>(on_select);
	idm::select<ARMv7Thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// Try to pause
	if (!m_state.compare_and_swap_test(system_state::running, system_state::paused))
	{
		return m_state.compare_and_swap_test(system_state::ready, system_state::paused);
	}

	GetCallbacks().on_pause();

	// Update pause start time
	if (m_pause_start_time.exchange(start))
	{
		LOG_ERROR(GENERAL, "Emulator::Pause() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_pause;
	};

	idm::select<ppu_thread>(on_select);
	idm::select<ARMv7Thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);

	if (auto mfc = fxm::check<mfc_thread>())
	{
		on_select(0, *mfc);
	}

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
	if (!m_state.compare_and_swap_test(system_state::paused, system_state::running))
	{
		return;
	}

	if (!time)
	{
		LOG_ERROR(GENERAL, "Emulator::Resume() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_flag::dbg_global_pause;
		cpu.notify();
	};

	idm::select<ppu_thread>(on_select);
	idm::select<ARMv7Thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);

	if (auto mfc = fxm::check<mfc_thread>())
	{
		on_select(0, *mfc);
	}

	GetCallbacks().on_resume();
}

void Emulator::Stop()
{
	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		return;
	}

	LOG_NOTICE(GENERAL, "Stopping emulator...");

	GetCallbacks().on_stop();

#ifdef WITH_GDB_DEBUGGER
	//fxm for some reason doesn't call on_stop
	fxm::get<GDBDebugServer>()->on_stop();
	fxm::remove<GDBDebugServer>();
#endif

	auto e_stop = std::make_exception_ptr(cpu_flag::dbg_global_stop);

	auto on_select = [&](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_stop;
		cpu.get()->set_exception(e_stop);
	};

	idm::select<ppu_thread>(on_select);
	idm::select<ARMv7Thread>(on_select);
	idm::select<RawSPUThread>(on_select);
	idm::select<SPUThread>(on_select);

	if (auto mfc = fxm::check<mfc_thread>())
	{
		on_select(0, *mfc);
	}

	LOG_NOTICE(GENERAL, "All threads signaled...");

	while (g_thread_count)
	{
		m_cb.process_events();

		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All threads stopped...");

	lv2_obj::cleanup();
	idm::clear();
	fxm::clear();

	LOG_NOTICE(GENERAL, "Objects cleared...");

	RSXIOMem.Clear();
	vm::close();

	if (g_cfg.misc.autoexit)
	{
		GetCallbacks().exit();
	}
	else
	{
		Init();
	}

#ifdef LLVM_AVAILABLE
	extern void jit_finalize();
	jit_finalize();
#endif
}

s32 error_code::error_report(const fmt_type_info* sup, u64 arg)
{
	logs::channel* channel = &logs::GENERAL;
	logs::level level = logs::level::error;
	const char* func = "Unknown function";

	if (auto thread = get_current_cpu_thread())
	{
		if (g_system == system_type::ps3 && thread->id_type() == 1)
		{
			auto& ppu = static_cast<ppu_thread&>(*thread);

			// Filter some annoying reports
			switch (arg)
			{
			case CELL_ESRCH:
			case CELL_EDEADLK:
			{
				if (ppu.m_name == "_cellsurMixerMain" || ppu.m_name == "_sys_MixerChStripMain")
				{
					if (std::memcmp(ppu.last_function, "sys_mutex_lock", 15) == 0 ||
						std::memcmp(ppu.last_function, "sys_lwmutex_lock", 17) == 0 ||
						std::memcmp(ppu.last_function, "_sys_mutex_lock", 16) == 0 ||
						std::memcmp(ppu.last_function, "_sys_lwmutex_lock", 18) == 0)
					{
						level = logs::level::trace;
					}
				}

				break;
			}
			}

			if (ppu.last_function)
			{
				func = ppu.last_function;
			}
		}

		if (g_system == system_type::psv)
		{
			if (auto _func = static_cast<ARMv7Thread*>(thread)->last_function)
			{
				func = _func;
			}
		}
	}

	channel->format(level, "'%s' failed with 0x%08x%s%s", func, arg, sup ? " : " : "", std::make_pair(sup, arg));
	return static_cast<s32>(arg);
}

Emulator Emu;
