#include "stdafx.h"
#include "VFS.h"
#include "Utilities/bin_patch.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/system_progress.hpp"
#include "Emu/system_utils.hpp"
#include "Emu/perf_meter.hpp"
#include "Emu/perf_monitor.hpp"
#include "Emu/vfs_config.h"
#include "Emu/IPC_config.h"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_overlay.h"
#include "Emu/Cell/Modules/cellGame.h"

#include "Emu/title.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Capture/rsx_replay.h"

#include "Loader/PSF.h"
#include "Loader/TAR.h"
#include "Loader/ELF.h"
#include "Loader/disc.h"

#include "Utilities/StrUtil.h"

#include "../Crypto/unself.h"
#include "util/yaml.hpp"
#include "util/logs.hpp"
#include "util/serialization.hpp"

#include <fstream>
#include <memory>
#include <regex>
#include <optional>

#include "Utilities/JIT.h"

#include "display_sleep_control.h"

#include "Emu/IPC_socket.h"

#if defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VulkanAPI.h"
#endif

LOG_CHANNEL(sys_log, "SYS");

// Preallocate 32 MiB
stx::manual_typemap<void, 0x20'00000, 128> g_fixed_typemap;

bool g_log_all_errors = false;

bool g_use_rtm = false;
u64 g_rtm_tx_limit1 = 0;
u64 g_rtm_tx_limit2 = 0;

struct serial_ver_t
{
	bool used = false;
	s32 current_version = 0;
	std::set<s32> compatible_versions;
};

static std::array<serial_ver_t, 23> s_serial_versions;

#define SERIALIZATION_VER(name, identifier, ...) \
\
	const bool s_##name##_serialization_fill = []() { ::s_serial_versions[identifier].compatible_versions = {__VA_ARGS__}; return true; }();\
\
	extern void using_##name##_serialization()\
	{\
		ensure(Emu.IsStopped());\
		::s_serial_versions[identifier].used = true;\
	}\
\
	extern s32 get_##name##_serialization_version()\
	{\
		return ::s_serial_versions[identifier].current_version;\
	}

SERIALIZATION_VER(global_version, 0,                            11) // For stuff not listed here
SERIALIZATION_VER(ppu, 1,                                       1)
SERIALIZATION_VER(spu, 2,                                       1, 2)
SERIALIZATION_VER(lv2_sync, 3,                                  1)
SERIALIZATION_VER(lv2_vm, 4,                                    1)
SERIALIZATION_VER(lv2_net, 5,                                   1)
SERIALIZATION_VER(lv2_fs, 6,                                    1)
SERIALIZATION_VER(lv2_prx_overlay, 7,                           1)
SERIALIZATION_VER(lv2_memory, 8,                                1)
SERIALIZATION_VER(lv2_config, 9,                                1)

namespace rsx
{
	SERIALIZATION_VER(rsx, 10,                                  1, 2)
}

namespace np
{
	SERIALIZATION_VER(sceNp, 11,                                1)
}

#ifdef _MSC_VER
// Compiler bug, lambda function body does seem to inherit used namespace atleast for function decleration 
SERIALIZATION_VER(rsx, 10,                                      1, 2)
SERIALIZATION_VER(sceNp, 11,                                    1)
#endif

SERIALIZATION_VER(cellVdec, 12,                                 1)
SERIALIZATION_VER(cellAudio, 13,                                1)
SERIALIZATION_VER(cellCamera, 14,                               1)
SERIALIZATION_VER(cellGem, 15,                                  1)
SERIALIZATION_VER(sceNpTrophy, 16,                              1)
SERIALIZATION_VER(cellMusic, 17,                                1)
SERIALIZATION_VER(cellVoice, 18,                                1)
SERIALIZATION_VER(cellGcm, 19,                                  1)
SERIALIZATION_VER(sysPrxForUser, 20,                            1)
SERIALIZATION_VER(cellSaveData, 21,                             1)
SERIALIZATION_VER(cellAudioOut, 22,                             1)

#undef SERIALIZATION_VER

std::string g_cfg_defaults;

atomic_t<u64> g_watchdog_hold_ctr{0};

extern bool ppu_load_exec(const ppu_exec_object&, utils::serial* = nullptr);
extern void spu_load_exec(const spu_exec_object&);
extern void spu_load_rel_exec(const spu_rel_object&);
extern void ppu_precompile(std::vector<std::string>& dir_queue, std::vector<ppu_module*>* loaded_prx);
extern bool ppu_initialize(const ppu_module&, bool = false);
extern void ppu_finalize(const ppu_module&);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&, s64 = 0, utils::serial* = nullptr);
extern std::pair<std::shared_ptr<lv2_overlay>, CellError> ppu_load_overlay(const ppu_exec_object&, const std::string& path, s64 = 0, utils::serial* = nullptr);
extern bool ppu_load_rel_exec(const ppu_rel_object&);

fs::file g_tty;
atomic_t<s64> g_tty_size{0};
std::array<std::deque<std::string>, 16> g_tty_input;
std::mutex g_tty_mutex;
thread_local std::string_view g_tls_serialize_name;

extern thread_local std::string(*g_tls_log_prefix)();

// Report error and call std::abort(), defined in main.cpp
[[noreturn]] void report_fatal_error(std::string_view);

void initialize_timebased_time(u64 timebased_init, bool reset = false);

namespace atomic_wait
{
	extern void parse_hashtable(bool(*cb)(u64 id, u32 refs, u64 ptr, u32 max_coll));
}

template<>
void fmt_class_string<game_boot_result>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](game_boot_result value)
	{
		switch (value)
		{
		case game_boot_result::no_errors: return "No errors";
		case game_boot_result::generic_error: return "Generic error";
		case game_boot_result::nothing_to_boot: return "Nothing to boot";
		case game_boot_result::wrong_disc_location: return "Wrong disc location";
		case game_boot_result::invalid_file_or_folder: return "Invalid file or folder";
		case game_boot_result::install_failed: return "Game install failed";
		case game_boot_result::decryption_error: return "Failed to decrypt content";
		case game_boot_result::file_creation_error: return "Could not create important files";
		case game_boot_result::firmware_missing: return "Firmware is missing";
		case game_boot_result::unsupported_disc_type: return "This disc type is not supported yet";
		case game_boot_result::savestate_corrupted: return "Savestate data is corrupted or it's not an RPCS3 savestate";
		case game_boot_result::savestate_version_unsupported: return "Savestate versioning data differes from your RPCS3 build";
		}
		return unknown;
	});
}

template<>
void fmt_class_string<cfg_mode>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](cfg_mode value)
	{
		switch (value)
		{
		case cfg_mode::custom: return "custom config";
		case cfg_mode::custom_selection: return "custom config selection";
		case cfg_mode::global: return "global config";
		case cfg_mode::config_override: return "config override";
		case cfg_mode::continuous: return "continuous config";
		case cfg_mode::default_config: return "default config";
		}
		return unknown;
	});
}

void Emulator::CallFromMainThread(std::function<void()>&& func, atomic_t<bool>* wake_up, bool track_emu_state, u64 stop_ctr) const
{
	if (!track_emu_state)
	{
		m_cb.call_from_main_thread(std::move(func), wake_up);
		return;
	}

	std::function<void()> final_func = [this, before = IsStopped(), count = (stop_ctr == umax ? +m_stop_ctr : stop_ctr), func = std::move(func)]
	{
		if (count == m_stop_ctr && before == IsStopped())
		{
			func();
		}
	};

	m_cb.call_from_main_thread(std::move(final_func), wake_up);
}

void Emulator::BlockingCallFromMainThread(std::function<void()>&& func) const
{
	atomic_t<bool> wake_up = false;

	CallFromMainThread(std::move(func), &wake_up);

	while (!wake_up && !IsStopped())
	{
		thread_ctrl::wait_on(wake_up, false);
	}
}

// This function ensures constant initialization order between different compilers and builds 
void init_fxo_for_exec(utils::serial* ar, bool full = false)
{
	g_fxo->init<ppu_module>();

	void init_ppu_functions(utils::serial* ar, bool full);

	if (full)
	{
		init_ppu_functions(ar, true);
	}

	Emu.ConfigurePPUCache();

	g_fxo->init(false, ar);

	Emu.GetCallbacks().init_gs_render(ar);
	Emu.GetCallbacks().init_pad_handler(Emu.GetTitleID());
	Emu.GetCallbacks().init_kb_handler();
	Emu.GetCallbacks().init_mouse_handler();
	if (ar) Emu.ExecDeserializationRemnants();
}

void Emulator::Init(bool add_only)
{
	jit_runtime::initialize();

	if (!g_tty)
	{
		const auto tty_path = fs::get_cache_dir() + "TTY.log";
		g_tty.open(tty_path, fs::rewrite + fs::append);

		if (!g_tty)
		{
			sys_log.fatal("Failed to create TTY log: %s (%s)", tty_path, fs::g_tls_error);
		}
	}

	g_fxo->reset();

	// Reset defaults, cache them
	g_cfg_vfs.from_default();
	g_cfg.from_default();
	g_cfg.name.clear();

	// Not all renderers are known at compile time, so set a provided default if possible
	if (m_default_renderer == video_renderer::vulkan && !m_default_graphics_adapter.empty())
	{
		g_cfg.video.renderer.set(m_default_renderer);
		g_cfg.video.vk.adapter.from_string(m_default_graphics_adapter);
	}

	g_cfg_defaults = g_cfg.to_string();

	// Load VFS config
	g_cfg_vfs.load();
	sys_log.notice("Using VFS config:\n%s", g_cfg_vfs.to_string());

	// Mount all devices
	const std::string emu_dir = rpcs3::utils::get_emu_dir();
	const std::string elf_dir = fs::get_parent_dir(m_path);
	const std::string dev_hdd0 = g_cfg_vfs.get(g_cfg_vfs.dev_hdd0, emu_dir);
	const std::string dev_hdd1 = g_cfg_vfs.get(g_cfg_vfs.dev_hdd1, emu_dir);
	const std::string dev_flsh = g_cfg_vfs.get_dev_flash();
	const std::string dev_flsh2 = g_cfg_vfs.get_dev_flash2();
	const std::string dev_flsh3 = g_cfg_vfs.get_dev_flash3();

	vfs::mount("/dev_hdd0", dev_hdd0);
	vfs::mount("/dev_flash", dev_flsh);
	vfs::mount("/dev_flash2", dev_flsh2);
	vfs::mount("/dev_flash3", dev_flsh3);
	vfs::mount("/app_home", g_cfg_vfs.app_home.to_string().empty() ? elf_dir + '/' : g_cfg_vfs.get(g_cfg_vfs.app_home, emu_dir));

	std::string dev_usb;

	for (const auto& [key, value] : g_cfg_vfs.dev_usb.get_map())
	{
		const cfg::device_info usb_info = g_cfg_vfs.get_device(g_cfg_vfs.dev_usb, key, emu_dir);

		if (key.size() != 11 || !key.starts_with("/dev_usb00"sv) || key.back() < '0' || key.back() > '7')
		{
			sys_log.error("Trying to mount unsupported usb device: %s", key);
			continue;
		}

		vfs::mount(key, usb_info.path);

		if (key == "/dev_usb000"sv)
		{
			dev_usb = usb_info.path;
		}
	}

	ensure(!dev_usb.empty());

	if (!hdd1.empty())
	{
		vfs::mount("/dev_hdd1", hdd1);
		sys_log.notice("Hdd1: %s", vfs::get("/dev_hdd1"));
	}

	const bool is_exitspawn = m_config_mode == cfg_mode::continuous;

	// Load config file
	if (m_config_mode == cfg_mode::config_override)
	{
		if (const fs::file cfg_file{m_config_path, fs::read + fs::create})
		{
			sys_log.notice("Applying config override: %s", m_config_path);

			if (!g_cfg.from_string(cfg_file.to_string()))
			{
				sys_log.fatal("Failed to apply config: %s. Proceeding with regular configuration.", m_config_path);
				m_config_path.clear();
				m_config_mode = cfg_mode::custom;
			}
			else
			{
				sys_log.success("Applied config override: %s", m_config_path);
				g_cfg.name = m_config_path;
			}
		}
		else
		{
			sys_log.fatal("Failed to access config: %s (%s). Proceeding with regular configuration.", m_config_path, fs::g_tls_error);
			m_config_path.clear();
			m_config_mode = cfg_mode::custom;
		}
	}

	// Reload global configuration
	if (m_config_mode != cfg_mode::config_override && m_config_mode != cfg_mode::default_config)
	{
		const auto cfg_path = fs::get_config_dir() + "/config.yml";

		if (const fs::file cfg_file{cfg_path, fs::read + fs::create})
		{
			sys_log.notice("Applying global config: %s", cfg_path);

			if (!g_cfg.from_string(cfg_file.to_string()))
			{
				sys_log.fatal("Failed to apply global config: %s", cfg_path);
			}

			g_cfg.name = cfg_path;
		}
		else
		{
			sys_log.fatal("Failed to access global config: %s (%s)", cfg_path, fs::g_tls_error);
		}
	}

	// Create directories (can be disabled if necessary)
	auto make_path_verbose = [](const std::string& path)
	{
		if (!fs::create_path(path))
		{
			sys_log.fatal("Failed to create path: %s (%s)", path, fs::g_tls_error);
		}
	};

	const std::string save_path = dev_hdd0 + "home/" + m_usr + "/savedata/";
	const std::string user_path = dev_hdd0 + "home/" + m_usr + "/localusername";

	if (g_cfg.vfs.init_dirs)
	{
		make_path_verbose(dev_hdd0);
		make_path_verbose(dev_hdd1);
		make_path_verbose(dev_flsh);
		make_path_verbose(dev_flsh2);
		make_path_verbose(dev_flsh3);
		make_path_verbose(dev_usb);
		make_path_verbose(dev_hdd0 + "game/");
		make_path_verbose(dev_hdd0 + reinterpret_cast<const char*>(u8"game/＄locks/"));
		make_path_verbose(dev_hdd0 + "home/");
		make_path_verbose(dev_hdd0 + "home/" + m_usr + "/");
		make_path_verbose(dev_hdd0 + "home/" + m_usr + "/exdata/");
		make_path_verbose(save_path);
		make_path_verbose(dev_hdd0 + "home/" + m_usr + "/trophy/");

		if (!fs::write_file(user_path, fs::create + fs::excl + fs::write, "User"s))
		{
			if (fs::g_tls_error != fs::error::exist)
			{
				sys_log.fatal("Failed to create file: %s (%s)", user_path, fs::g_tls_error);
			}
		}

		make_path_verbose(dev_hdd0 + "disc/");
		make_path_verbose(dev_hdd0 + "savedata/");
		make_path_verbose(dev_hdd0 + "savedata/vmc/");
		make_path_verbose(dev_hdd0 + "photo/");
		make_path_verbose(dev_hdd1 + "caches/");
	}

	make_path_verbose(fs::get_cache_dir() + "shaderlog/");
	make_path_verbose(fs::get_cache_dir() + "spu_progs/");
	make_path_verbose(fs::get_cache_dir() + "/savestates/");
	make_path_verbose(fs::get_config_dir() + "captures/");
	make_path_verbose(fs::get_config_dir() + "sounds/");
	make_path_verbose(patch_engine::get_patches_path());

	if (add_only)
	{
		// We don't need to initialize the rest if we only add games
		return;
	}

	// Log user
	if (m_usr.empty())
	{
		sys_log.fatal("No user configured");
	}
	else
	{
		std::string username;

		if (const fs::file file = fs::file(user_path))
		{
			if (const std::string localusername = file.to_string(); !localusername.empty())
			{
				username = localusername;
			}
			else
			{
				sys_log.warning("Empty username in file: '%s'. Consider setting a username for user '%s' in the user manager.", user_path, m_usr);
			}
		}
		else
		{
			sys_log.error("Could not read file: '%s'", user_path);
		}

		if (username.empty())
		{
			sys_log.notice("Logged in as user '%s'", m_usr);
		}
		else
		{
			sys_log.notice("Logged in as user '%s' with the username '%s'", m_usr, username);
		}
	}

	if (is_exitspawn)
	{
		// Actions not taken during exitspawn
		return;
	}

	// Fixup savedata
	for (const auto& entry : fs::dir(save_path))
	{
		if (entry.is_directory && entry.name.starts_with(".backup_"))
		{
			const std::string desired = entry.name.substr(8);
			const std::string pending = save_path + ".working_" + desired;

			if (fs::is_dir(pending))
			{
				// Finalize interrupted saving
				if (!fs::rename(pending, save_path + desired, false))
				{
					sys_log.fatal("Failed to fix save data: %s (%s)", pending, fs::g_tls_error);
					continue;
				}

				sys_log.success("Fixed save data: %s", desired);
			}

			// Remove pending backup data
			if (!fs::remove_all(save_path + entry.name))
			{
				sys_log.fatal("Failed to remove save data backup: %s%s (%s)", save_path, entry.name, fs::g_tls_error);
			}
			else
			{
				sys_log.success("Removed save data backup: %s%s", save_path, entry.name);
			}
		}
	}

	// Limit cache size
	if (g_cfg.vfs.limit_cache_size)
	{
		rpcs3::cache::limit_cache_size();
	}

	// Wipe clean VSH's temporary directory of choice
	if (g_cfg.vfs.empty_hdd0_tmp && !fs::remove_all(dev_hdd0 + "tmp/", false, true))
	{
		sys_log.error("Could not clean /dev_hdd0/tmp/ (%s)", fs::g_tls_error);
	}

	// Remove temporary game data that would have been removed when cellGame has been properly shut
	for (const auto& entry : fs::dir(dev_hdd0 + "game/"))
	{
		if (entry.name.starts_with("_GDATA_") && fs::is_dir(dev_hdd0 + "game/" + entry.name + "/USRDIR/"))
		{
			const std::string target = dev_hdd0 + "game/" + entry.name;

			if (!fs::remove_all(target, true, true))
			{
				sys_log.error("Could not clean \"%s\" (%s)", target, fs::g_tls_error);
			}
		}
	}

	// Load IPC config
	g_cfg_ipc.load();
	sys_log.notice("Using IPC config:\n%s", g_cfg_ipc.to_string());
	g_fxo->get<IPC_socket::IPC_server_manager>().set_server_enabled(g_cfg_ipc.get_server_enabled());
}

void Emulator::SetUsr(const std::string& user)
{
	sys_log.notice("Setting user ID '%s'", user);

	const u32 id = rpcs3::utils::check_user(user);

	if (id == 0)
	{
		fmt::throw_exception("Failed to set user ID '%s'", user);
	}

	m_usrid = id;
	m_usr = user;
}

std::string Emulator::GetBackgroundPicturePath() const
{
	// Try to find a custom icon first
	std::string path = fs::get_config_dir() + "/Icons/game_icons/" + GetTitleID() + "/PIC1.PNG";

	if (fs::is_file(path))
	{
		return path;
	}

	path = m_sfo_dir + "/PIC1.PNG";

	if (!fs::is_file(path))
	{
		const std::string disc_dir = vfs::get("/dev_bdvd/PS3_GAME");

		if (disc_dir.empty())
		{
			// Fallback to ICON0.PNG
			path = m_sfo_dir + "/ICON0.PNG";
		}
		else
		{
			// Fallback to PIC1.PNG in disc dir
			path = disc_dir + "/PIC1.PNG";

			if (!fs::is_file(path))
			{
				// Fallback to ICON0.PNG in update dir
				path = m_sfo_dir + "/ICON0.PNG";

				if (!fs::is_file(path))
				{
					// Fallback to ICON0.PNG in disc dir
					path = disc_dir + "/ICON0.PNG";
				}
			}
		}
	}

	return path;
}

bool Emulator::BootRsxCapture(const std::string& path)
{
	fs::file in_file(path);

	if (!in_file)
	{
		return false;
	}

	std::unique_ptr<rsx::frame_capture_data> frame = std::make_unique<rsx::frame_capture_data>();
	utils::serial load;
	load.set_reading_state();
	in_file.read(load.data, in_file.size());
	load.data.shrink_to_fit();

	load(*frame);
	in_file.close();

	if (frame->magic != rsx::c_fc_magic)
	{
		sys_log.error("Invalid rsx capture file!");
		return false;
	}

	if (frame->version != rsx::c_fc_version)
	{
		sys_log.error("Rsx capture file version not supported! Expected %d, found %d", +rsx::c_fc_version, frame->version);
		return false;
	}

	if (frame->LE_format != u32{std::endian::little == std::endian::native})
	{
		static constexpr std::string_view machines[2]{"Big-Endian", "Little-Endian"};

		sys_log.error("Rsx capture byte endianness not supported! Expected %s format, found %s format"
			, machines[frame->LE_format ^ 1], machines[frame->LE_format]);

		return false;
	}

	Init();
	g_cfg.video.disable_on_disk_shader_cache.set(true);

	vm::init();
	g_fxo->init(false);

	// Initialize progress dialog
	g_fxo->init<named_thread<progress_dialog_server>>();

	// Initialize performance monitor
	g_fxo->init<named_thread<perf_monitor>>();

	// PS3 'executable'
	m_state = system_state::ready;
	GetCallbacks().on_ready();

	GetCallbacks().init_gs_render(nullptr);
	GetCallbacks().init_pad_handler("");

	GetCallbacks().on_run(false);
	m_state = system_state::running;

	auto replay_thr = g_fxo->init<named_thread<rsx::rsx_replay_thread>>("RSX Replay", std::move(frame));
	replay_thr->state -= cpu_flag::stop;
	replay_thr->state.notify_one(cpu_flag::stop);

	return true;
}

game_boot_result Emulator::GetElfPathFromDir(std::string& elf_path, const std::string& path)
{
	if (!fs::is_dir(path))
	{
		return game_boot_result::invalid_file_or_folder;
	}

	static const char* boot_list[] =
	{
		"/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/USRDIR/ISO.BIN.EDAT",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
	};

	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			elf_path = elf;
			return game_boot_result::no_errors;
		}
	}

	return game_boot_result::invalid_file_or_folder;
}

game_boot_result Emulator::BootGame(const std::string& path, const std::string& title_id, bool direct, bool add_only, cfg_mode config_mode, const std::string& config_path)
{
	if (!fs::exists(path))
	{
		return game_boot_result::invalid_file_or_folder;
	}

	m_path_old = m_path;

	m_config_mode = config_mode;
	m_config_path = config_path;

	if (fs::file save{path, fs::isfile + fs::read}; save && save.size() >= 8 && save.read<u64>() == "RPCS3SAV"_u64)
	{
		m_ar = std::make_shared<utils::serial>();
		m_ar->set_reading_state();
		save.seek(0);
		save.read(m_ar->data, save.size());
		m_ar->data.shrink_to_fit();
	}

	if (direct || m_ar || fs::is_file(path))
	{
		m_path = path;

		auto error = Load(title_id, add_only);

		if (is_error(error))
		{
			m_ar.reset();
		}

		if (g_cfg.savestate.suspend_emu && m_ar)
		{
			fs::remove_file(path);
		}

		return error;
	}

	game_boot_result result = game_boot_result::nothing_to_boot;

	std::string elf;
	if (const game_boot_result res = GetElfPathFromDir(elf, path); res == game_boot_result::no_errors)
	{
		ensure(!elf.empty());
		m_path = elf;
		result = Load(title_id, add_only);
	}

	if (add_only)
	{
		for (auto&& entry : fs::dir{ path })
		{
			if (entry.name == "." || entry.name == "..")
			{
				continue;
			}

			if (entry.is_directory && std::regex_match(entry.name, std::regex("^PS3_GM[[:digit:]]{2}$")))
			{
				const std::string elf = path + "/" + entry.name + "/USRDIR/EBOOT.BIN";

				if (fs::is_file(elf))
				{
					m_path = elf;
					if (const auto err = Load(title_id, add_only); err != game_boot_result::no_errors)
					{
						result = err;
					}
				}
			}
		}
	}
	return result;
}

void Emulator::SetForceBoot(bool force_boot)
{
	m_force_boot = force_boot;
}

game_boot_result Emulator::Load(const std::string& title_id, bool add_only, bool is_disc_patch)
{
	if (m_config_mode == cfg_mode::continuous)
	{
		// The program is being booted from another running program
		// CELL_GAME_GAMETYPE_GAMEDATA is not used as boot type

		if (m_cat == "DG"sv)
		{
			m_boot_source_type = CELL_GAME_GAMETYPE_DISC;
		}
		else if (m_cat == "HM"sv)
		{
			m_boot_source_type = CELL_GAME_GAMETYPE_HOME;
		}
		else
		{
			m_boot_source_type = CELL_GAME_GAMETYPE_HDD;
		}
	}
	else
	{
		m_boot_source_type = CELL_GAME_GAMETYPE_SYS;
	}

	if (!IsStopped())
	{
		Kill();
	}

	if (!title_id.empty())
	{
		m_title_id = title_id;
	}

	sys_log.notice("Selected config: mode=%s, path=\"%s\"", m_config_mode, m_config_path);
	sys_log.notice("Path: %s", m_path);

	{
		Init(add_only);

		// Load game list (maps ABCD12345 IDs to /dev_bdvd/ locations)
		YAML::Node games;

		if (fs::file f{fs::get_config_dir() + "/games.yml", fs::read + fs::create})
		{
			auto [result, error] = yaml_load(f.to_string());

			if (!error.empty())
			{
				sys_log.error("Failed to load games.yml: %s", error);
			}

			games = result;
		}

		if (!games.IsMap())
		{
			games.reset();
		}

		m_state_inspection_savestate = g_cfg.savestate.state_inspection_mode.get();

		if (m_ar)
		{
			struct file_header
			{
				ENABLE_BITWISE_SERIALIZATION;
	
				nse_t<u64, 1> magic;
				bool LE_format;
				bool state_inspection_support;
				nse_t<u64, 1> offset;
			};
	
			const auto header = m_ar->try_read<file_header>().second;
	
			if (header.magic != "RPCS3SAV"_u64)
			{
				return game_boot_result::savestate_corrupted;
			}
	
			if (header.LE_format != (std::endian::native == std::endian::little))
			{
				return game_boot_result::savestate_corrupted;
			}
	
			g_cfg.savestate.state_inspection_mode.set(header.state_inspection_support);
	
			// Emulate seek operation (please avoid using in other places)
			m_ar->pos = header.offset;
			const std::vector<std::pair<u16, u16>> versions_data = *m_ar;
			m_ar->pos = sizeof(file_header); // Restore position
	
			if (versions_data.empty())
			{
				return game_boot_result::savestate_corrupted;
			}
	
			bool ok = true;
	
			for (auto [identifier, version] : versions_data)
			{
				if (identifier >= s_serial_versions.size())
				{
					sys_log.error("Savestate version identider is unknown! (category=%u, version=%u)", identifier, version);
					ok = false; // Log all mismatches
				}
				else if (!s_serial_versions[identifier].compatible_versions.count(version))
				{
					sys_log.error("Savestate version is not supported. (category=%u, version=%u)", identifier, version);
					ok = false;
				}
				else
				{
					s_serial_versions[identifier].current_version = version;
				}
			}
	
			if (!ok)
			{
				return game_boot_result::savestate_version_unsupported;
			}
	
			argv.clear();
	
			std::string bdvd_by_title_id;
			(*m_ar)(argv.emplace_back(), bdvd_by_title_id);
	
			klic.clear();
	
			if (u128 key = m_ar->operator u128())
			{
				klic.emplace_back(key);
			}
	
			if (!bdvd_by_title_id.empty())
			{
				m_title_id = bdvd_by_title_id;
	
				// Load /dev_bdvd/ from game list if available
				if (auto node = games[m_title_id])
				{
					disc = node.Scalar();
				}
				else
				{
					sys_log.fatal("Disc directory not found. Savestate cannot be loaded. ('%s')", m_title_id);
					return game_boot_result::invalid_file_or_folder;
				}
			}
	
			hdd1 = m_ar->operator std::string();
	
			auto load_tar = [&](const std::string& path)
			{
				const usz size = *m_ar;

				if (size)
				{
					fs::remove_all(path, false);
					ensure(tar_object(fs::file(&m_ar->data[m_ar->pos], size)).extract(path));
					m_ar->pos += size;
				}
			};

			if (!hdd1.empty())
			{
				hdd1 = rpcs3::utils::get_hdd1_dir() + "caches/" + hdd1 + "/";
				load_tar(hdd1);
			}
	
			for (const std::string hdd0_game = rpcs3::utils::get_hdd0_dir() + "game/";;)
			{
				const std::string game_data = m_ar->operator std::string();

				if (game_data.empty())
				{
					break;
				}

				load_tar(hdd0_game + game_data);
			}

			if (argv[0].starts_with("/dev_hdd0"sv))
			{
				m_path = rpcs3::utils::get_hdd0_dir();
				m_path += std::string_view(argv[0]).substr(9);
			}
			else if (argv[0].starts_with("/dev_flash"sv))
			{
				m_path = g_cfg_vfs.get_dev_flash();
				m_path += std::string_view(argv[0]).substr(10);
			}
			else if (argv[0].starts_with("/dev_bdvd"sv))
			{
				m_path = disc;
				m_path += std::string_view(argv[0]).substr(9);
			}
			else if (argv[0].starts_with("/host_root"sv))
			{
				sys_log.error("Host root has been used in savestates!");
				m_path = argv[0].substr(9);
			}
			else if (argv[0].starts_with("/dev_hdd1"sv))
			{
				sys_log.error("HDD1 has been used to store executable in savestates!");
				m_path = rpcs3::utils::get_hdd1_dir();
				m_path += std::string_view(argv[0]).substr(9);
			}
			else
			{
				sys_log.error("Unknown source for savestates: %s", argv[0]);
			}
		}

		const std::string resolved_path = GetCallbacks().resolve_path(m_path);

		const std::string elf_dir = fs::get_parent_dir(m_path);

		// Mount /app_home
		vfs::mount("/app_home", g_cfg_vfs.app_home.to_string().empty() ? elf_dir + '/' : g_cfg_vfs.get(g_cfg_vfs.app_home, rpcs3::utils::get_emu_dir()));

		// Load PARAM.SFO (TODO)
		psf::registry _psf;
		if (fs::file sfov{elf_dir + "/sce_sys/param.sfo"})
		{
			m_sfo_dir = elf_dir;
			_psf = psf::load_object(sfov);
		}
		else
		{
			if (fs::is_dir(m_path))
			{
				// Special case (directory scan)
				m_sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(m_path, m_title_id);
			}
			else if (!disc.empty())
			{
				// Check previously used category before it's overwritten
				if (m_cat == "DG")
				{
					m_sfo_dir = disc + "/" + m_game_dir;
				}
				else if (m_cat == "GD")
				{
					m_sfo_dir = rpcs3::utils::get_hdd0_dir() + "game/" + m_title_id;
				}
				else
				{
					m_sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(disc, m_title_id);
				}
			}
			else
			{
				m_sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(elf_dir + "/../", m_title_id);
			}

			_psf = psf::load_object(fs::file(m_sfo_dir + "/PARAM.SFO"));
		}

		m_title = std::string(psf::get_string(_psf, "TITLE", std::string_view(m_path).substr(m_path.find_last_of(fs::delim) + 1)));
		m_title_id = std::string(psf::get_string(_psf, "TITLE_ID"));
		m_cat = std::string(psf::get_string(_psf, "CATEGORY"));

		const auto version_app  = psf::get_string(_psf, "APP_VER", "Unknown");
		const auto version_disc = psf::get_string(_psf, "VERSION", "Unknown");
		m_app_version           = version_app == "Unknown" ? version_disc : version_app;

		if (!_psf.empty() && m_cat.empty())
		{
			sys_log.fatal("Corrupted PARAM.SFO found! Try reinstalling the game.");
			return game_boot_result::invalid_file_or_folder;
		}

		sys_log.notice("Title: %s", GetTitle());
		sys_log.notice("Serial: %s", GetTitleID());
		sys_log.notice("Category: %s", GetCat());
		sys_log.notice("Version: APP_VER=%s VERSION=%s", version_app, version_disc);

		if (!add_only)
		{
			if (m_config_mode == cfg_mode::custom_selection || (m_config_mode == cfg_mode::continuous && !m_config_path.empty()))
			{
				if (fs::file cfg_file{ m_config_path })
				{
					sys_log.notice("Applying %s: %s", m_config_mode, m_config_path);

					if (g_cfg.from_string(cfg_file.to_string()))
					{
						g_cfg.name = m_config_path;
					}
					else
					{
						sys_log.fatal("Failed to apply %s: %s", m_config_mode, m_config_path);
					}
				}
				else
				{
					sys_log.fatal("Failed to access %s: %s", m_config_mode, m_config_path);
				}
			}
			else if (m_config_mode == cfg_mode::custom)
			{
				const std::string config_path = rpcs3::utils::get_custom_config_path(m_title_id);

				// Load custom config-1
				if (fs::file cfg_file{ config_path })
				{
					sys_log.notice("Applying custom config: %s", config_path);

					if (g_cfg.from_string(cfg_file.to_string()))
					{
						g_cfg.name = config_path;
						m_config_path = config_path;
					}
					else
					{
						sys_log.fatal("Failed to apply custom config: %s", config_path);
					}
				}

				// Load custom config-2
				if (fs::file cfg_file{ m_path + ".yml" })
				{
					sys_log.notice("Applying custom config: %s.yml", m_path);

					if (g_cfg.from_string(cfg_file.to_string()))
					{
						g_cfg.name = m_path + ".yml";
						m_config_path = g_cfg.name;
					}
					else
					{
						sys_log.fatal("Failed to apply custom config: %s.yml", m_path);
					}
				}
			}

			// Force audio provider
			if (m_path.ends_with("vsh.self"sv))
			{
				g_cfg.audio.provider.set(audio_provider::rsxaudio);
			}
			else
			{
				g_cfg.audio.provider.set(audio_provider::cell_audio);
			}
		}

		// Set RTM usage
		g_use_rtm = utils::has_rtm() && (((utils::has_mpx() && !utils::has_tsx_force_abort()) && g_cfg.core.enable_TSX == tsx_usage::enabled) || g_cfg.core.enable_TSX == tsx_usage::forced);

		if (!add_only)
		{
			// Log some extra info in case of boot
#if defined(HAVE_VULKAN)
			if (g_cfg.video.renderer == video_renderer::vulkan)
			{
				sys_log.notice("Vulkan SDK Revision: %d", VK_HEADER_VERSION);
			}
#endif
			sys_log.notice("Used configuration:\n%s\n", g_cfg.to_string());

			if (g_use_rtm && (!utils::has_mpx() || utils::has_tsx_force_abort()))
			{
				sys_log.warning("TSX forced by User");
			}

			// Initialize patch engine
			g_fxo->need<patch_engine>();

			// Load patches from different locations
			g_fxo->get<patch_engine>().append_global_patches();
			g_fxo->get<patch_engine>().append_title_patches(m_title_id);
		}

		if (g_use_rtm)
		{
			// Update supplementary settings
			const f64 _1ns = utils::get_tsc_freq() / 1000'000'000.;
			g_rtm_tx_limit1 = static_cast<u64>(g_cfg.core.tx_limit1_ns * _1ns);
			g_rtm_tx_limit2 = static_cast<u64>(g_cfg.core.tx_limit2_ns * _1ns);
		}

		// Set bdvd_dir
		std::string bdvd_dir;

		if (!add_only)
		{
			bdvd_dir = g_cfg_vfs.dev_bdvd;

			if (!bdvd_dir.empty() && bdvd_dir.back() != fs::delim[0] && bdvd_dir.back() != fs::delim[1])
			{
				bdvd_dir.push_back('/');
			}

			if (!bdvd_dir.empty() && !fs::is_file(bdvd_dir + "PS3_DISC.SFB"))
			{
				// Unuse if invalid
				sys_log.error("Failed to use custom BDVD directory: '%s'", bdvd_dir);
				bdvd_dir.clear();
			}
		}

		// Special boot mode (directory scan)
		if (!add_only && fs::is_dir(m_path))
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init(false);
			Run(false);
			m_force_boot = false;

			// Force LLVM recompiler
			g_cfg.core.ppu_decoder.from_default();

			// Force LLE lib loading mode
			g_cfg.core.libraries_control.set_set([]()
			{
				std::set<std::string> set;

				extern const std::map<std::string_view, int> g_prx_list;

				for (const auto& lib : g_prx_list)
				{
					set.emplace(std::string(lib.first) + ":lle");
				}

				return set;
			}());

			// Fake arg (workaround)
			argv.resize(1);
			argv[0] = "/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN";
			m_dir = "/dev_bdvd/PS3_GAME";

			g_fxo->init<named_thread>("SPRX Loader"sv, [this]
			{
				std::string path;
				std::vector<std::string> dir_queue;
				dir_queue.emplace_back(m_path + '/');

				if (m_title_id.empty())
				{
					// Check if we are trying to scan vsh/module
					const std::string vsh_path = g_cfg_vfs.get_dev_flash() + "vsh/module";

					if (IsPathInsideDir(m_path, vsh_path))
					{
						// Memorize path to vsh.self
						path = vsh_path + "/vsh.self";
					}
				}
				else
				{
					// Find game update to use EBOOT.BIN from it, also add its directory to scan
					if (m_cat == "DG")
					{
						const std::string hdd0_path = vfs::get("/dev_hdd0/game/") + m_title_id;

						if (fs::is_file(hdd0_path + "/USRDIR/EBOOT.BIN"))
						{
							m_path = hdd0_path;
							dir_queue.emplace_back(m_path + '/');
						}
					}

					// Memorize path to EBOOT.BIN
					path = m_path + "/USRDIR/EBOOT.BIN";

					// Try to add all related directories
					const std::set<std::string> dirs = GetGameDirs();
					dir_queue.insert(std::end(dir_queue), std::begin(dirs), std::end(dirs));
				}

				if (fs::is_file(path))
				{
					// Compile binary first
					ppu_log.notice("Trying to load binary: %s", path);

					fs::file src{path};

					src = decrypt_self(std::move(src));

					const ppu_exec_object obj = src;

					if (obj == elf_error::ok)
					{
						auto& _main = g_fxo->get<ppu_module>();

						ppu_load_exec(obj);

						_main.path = path;

						ConfigurePPUCache();

						ppu_initialize(_main);
					}
					else
					{
						sys_log.error("Failed to load binary '%s' (%s)", path, obj.get_error());
					}
				}
				else
				{
					// Workaround for analyser glitches
					ensure(vm::falloc(0x10000, 0xf0000, vm::main));
				}

				if (IsStopped())
				{
					m_path = m_path_old; // Reset m_path to fix boot from gui
					GetCallbacks().on_stop(); // Call on_stop to refresh gui
					return;
				}

				ppu_precompile(dir_queue, nullptr);

				// Exit "process"
				CallFromMainThread([]
				{
					Emu.Kill(false);
				});

				m_path = m_path_old; // Reset m_path to fix boot from gui
				GetCallbacks().on_stop(); // Call on_stop to refresh gui
			});

			return game_boot_result::no_errors;
		}

		// Detect boot location
		const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
		const std::string hdd0_disc = vfs::get("/dev_hdd0/disc/");
		const bool from_hdd0_game   = IsPathInsideDir(m_path, hdd0_game);
		const bool from_dev_flash   = IsPathInsideDir(m_path, g_cfg_vfs.get_dev_flash());

#ifdef _WIN32
		// m_path might be passed from command line with differences in uppercase/lowercase on windows.
		if ((!from_hdd0_game && IsPathInsideDir(fmt::to_lower(m_path), fmt::to_lower(hdd0_game))) ||
			(!from_dev_flash && IsPathInsideDir(fmt::to_lower(m_path), fmt::to_lower(g_cfg_vfs.get_dev_flash()))))
		{
			// Let's just abort to prevent errors down the line.
			sys_log.error("The boot path seems to contain incorrectly cased characters. Please adjust the path and try again.");
			return game_boot_result::invalid_file_or_folder;
		}
#endif

		// Mount /dev_bdvd/ if necessary
		if (bdvd_dir.empty() && disc.empty())
		{
			std::string sfb_dir;
			GetBdvdDir(bdvd_dir, sfb_dir, m_game_dir, elf_dir);

			if (!sfb_dir.empty() && from_hdd0_game)
			{
				// Booting disc game from wrong location
				sys_log.error("Disc game %s found at invalid location /dev_hdd0/game/", m_title_id);

				const std::string dst_dir = hdd0_disc + sfb_dir.substr(hdd0_game.size());

				// Move and retry from correct location
				if (fs::create_path(fs::get_parent_dir(dst_dir)) && fs::rename(sfb_dir, dst_dir, false))
				{
					sys_log.success("Disc game %s moved to special location /dev_hdd0/disc/", m_title_id);
					m_path = hdd0_disc + m_path.substr(hdd0_game.size());
					return Load(m_title_id, add_only);
				}

				sys_log.error("Failed to move disc game %s to /dev_hdd0/disc/ (%s)", m_title_id, fs::g_tls_error);
				return game_boot_result::wrong_disc_location;
			}
		}

		if (bdvd_dir.empty() && !is_disc_patch)
		{
			// Reset original disc game dir if this is neither disc nor disc patch
			m_game_dir = "PS3_GAME";
		}

		// Booting patch data
		if ((is_disc_patch || m_cat == "GD") && bdvd_dir.empty() && disc.empty())
		{
			// Load /dev_bdvd/ from game list if available
			if (auto node = games[m_title_id])
			{
				bdvd_dir = node.Scalar();
			}
			else
			{
				sys_log.fatal("Disc directory not found. Try to run the game from the actual game disc directory.");
				return game_boot_result::invalid_file_or_folder;
			}
		}

		// Check /dev_bdvd/
		if (disc.empty() && !bdvd_dir.empty() && fs::is_dir(bdvd_dir))
		{
			fs::file sfb_file;

			vfs::mount("/dev_bdvd", bdvd_dir);
			sys_log.notice("Disc: %s", vfs::get("/dev_bdvd"));

			vfs::mount("/dev_bdvd/PS3_GAME", bdvd_dir + m_game_dir + "/");
			sys_log.notice("Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));

			const auto sfb_path = vfs::get("/dev_bdvd/PS3_DISC.SFB");

			if (!sfb_file.open(sfb_path) || sfb_file.size() < 4 || sfb_file.read<u32>() != ".SFB"_u32)
			{
				sys_log.error("Invalid disc directory for the disc game %s. (%s)", m_title_id, sfb_path);
				return game_boot_result::invalid_file_or_folder;
			}

			const auto game_psf = psf::load_object(fs::file{vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO")});
			const auto bdvd_title_id = psf::get_string(game_psf, "TITLE_ID");

			if (m_title_id.empty())
			{
				// We are most likely booting a binary inside a disc directory
				sys_log.error("Booting binary without TITLE_ID inside disc dir of '%s'", bdvd_title_id);
			}
			else
			{
				if (bdvd_title_id != m_title_id)
				{
					// Not really an error just an odd situation
					sys_log.error("Unexpected PARAM.SFO found in disc directory '%s' (found '%s')", m_title_id, bdvd_title_id);
				}

				// Store /dev_bdvd/ location
				games[m_title_id] = bdvd_dir;
				YAML::Emitter out;
				out << games;

				fs::pending_file temp(fs::get_config_dir() + "/games.yml");

				// Do not update games.yml when TITLE_ID is empty
				if (!temp.file || temp.file.write(out.c_str(), out.size()), !temp.commit())
				{
					sys_log.error("Failed to save BDVD location of title '%s' (%s)", m_title_id, fs::g_tls_error);
				}
			}
		}
		else if (m_cat == "1P" && from_hdd0_game)
		{
			// PS1 Classic located in dev_hdd0/game
			sys_log.notice("PS1 Game: %s, %s", m_title_id, m_title);

			const std::string game_path = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 9);

			argv.resize(9);
			argv[0] = "/dev_flash/ps1emu/ps1_newemu.self";
			argv[1] = m_title_id + "_mc1.VM1";    // virtual mc 1 /dev_hdd0/savedata/vmc/%argv[1]%
			argv[2] = m_title_id + "_mc2.VM1";    // virtual mc 2 /dev_hdd0/savedata/vmc/%argv[2]%
			argv[3] = "0082";                     // region target
			argv[4] = "1600";                     // ??? arg4 600 / 1200 / 1600, resolution scale? (purely a guess, the numbers seem to match closely to resolutions tho)
			argv[5] = game_path;                  // ps1 game folder path (not the game serial)
			argv[6] = "1";                        // ??? arg6 1 ?
			argv[7] = "2";                        // ??? arg7 2 -- full screen on/off 2/1 ?
			argv[8] = "1";                        // ??? arg8 2 -- smoothing	on/off	= 1/0 ?

			//TODO, this seems like it would normally be done by sysutil etc
			//Basically make 2 128KB memory cards 0 filled and let the games handle formatting.

			fs::file card_1_file(vfs::get("/dev_hdd0/savedata/vmc/" + argv[1]), fs::write + fs::create);
			card_1_file.trunc(128 * 1024);
			fs::file card_2_file(vfs::get("/dev_hdd0/savedata/vmc/" + argv[2]), fs::write + fs::create);
			card_2_file.trunc(128 * 1024);
		}
		else if (m_cat == "PE" && from_hdd0_game)
		{
			// PSP Remaster located in dev_hdd0/game
			sys_log.notice("PSP Remaster Game: %s, %s", m_title_id, m_title);

			const std::string game_path = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 9);

			argv.resize(2);
			argv[0] = "/dev_flash/pspemu/psp_emulator.self";
			argv[1] = game_path;
		}
		else if (m_cat != "DG" && m_cat != "GD")
		{
			// Don't need /dev_bdvd
		}
		else if (m_cat == "DG" && from_hdd0_game && disc.empty())
		{
			// Disc game located in dev_hdd0/game
			vfs::mount("/dev_bdvd/PS3_GAME", hdd0_game + m_path.substr(hdd0_game.size(), 10));
			sys_log.notice("Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));
		}
		else if (disc.empty())
		{
			sys_log.error("Failed to mount disc directory for the disc game %s", m_title_id);
			return game_boot_result::invalid_file_or_folder;
		}
		else
		{
			// Disc game
			bdvd_dir = disc;
			vfs::mount("/dev_bdvd", bdvd_dir);
			sys_log.notice("Disk: %s", vfs::get("/dev_bdvd"));
		}

		if (add_only)
		{
			sys_log.notice("Finished to add data to games.yml by boot for: %s", m_path);
			m_path = m_path_old; // Reset m_path to fix boot from gui
			return game_boot_result::no_errors;
		}

		// Initialize progress dialog
		g_fxo->init<named_thread<progress_dialog_server>>();

		// Initialize performance monitor
		g_fxo->init<named_thread<perf_monitor>>();

		// Set title to actual disc title if necessary
		const std::string disc_sfo_dir = vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO");

		const auto disc_psf_obj = psf::load_object(fs::file{ disc_sfo_dir });

		// Install PKGDIR, INSDIR, PS3_EXTRA
		if (!bdvd_dir.empty())
		{
			std::string ins_dir = vfs::get("/dev_bdvd/PS3_GAME/INSDIR/");
			std::string pkg_dir = vfs::get("/dev_bdvd/PS3_GAME/PKGDIR/");
			std::string extra_dir = vfs::get("/dev_bdvd/PS3_GAME/PS3_EXTRA/");
			fs::file lock_file;

			for (const auto path_ptr : {&ins_dir, &pkg_dir, &extra_dir})
			{
				if (!fs::is_dir(*path_ptr))
				{
					path_ptr->clear();
				}
			}

			const std::string lock_file_path = fmt::format("%s%s%s_v%s", hdd0_game, u8"＄locks/", m_title_id, psf::get_string(disc_psf_obj, "APP_VER"));

			if (!ins_dir.empty() || !pkg_dir.empty() || !extra_dir.empty())
			{
				// For backwards compatibility
				if (!lock_file.open(hdd0_game + ".locks/" + m_title_id))
				{
					// Check if already installed
					lock_file.open(lock_file_path);
				}
			}

			if (!lock_file && !ins_dir.empty())
			{
				sys_log.notice("Found INSDIR: %s", ins_dir);

				for (auto&& entry : fs::dir{ins_dir})
				{
					const std::string pkg = ins_dir + entry.name;
					if (!entry.is_directory && entry.name.ends_with(".PKG") && !rpcs3::utils::install_pkg(pkg))
					{
						sys_log.error("Failed to install %s", pkg);
						return game_boot_result::install_failed;
					}
				}
			}

			if (!lock_file && !pkg_dir.empty())
			{
				sys_log.notice("Found PKGDIR: %s", pkg_dir);

				for (auto&& entry : fs::dir{pkg_dir})
				{
					if (entry.is_directory && entry.name.starts_with("PKG"))
					{
						const std::string pkg_file = pkg_dir + entry.name + "/INSTALL.PKG";

						if (fs::is_file(pkg_file) && !rpcs3::utils::install_pkg(pkg_file))
						{
							sys_log.error("Failed to install %s", pkg_file);
							return game_boot_result::install_failed;
						}
					}
				}
			}

			if (!lock_file && !extra_dir.empty())
			{
				sys_log.notice("Found PS3_EXTRA: %s", extra_dir);

				for (auto&& entry : fs::dir{extra_dir})
				{
					if (entry.is_directory && entry.name[0] == 'D')
					{
						const std::string pkg_file = extra_dir + entry.name + "/DATA000.PKG";

						if (fs::is_file(pkg_file) && !rpcs3::utils::install_pkg(pkg_file))
						{
							sys_log.error("Failed to install %s", pkg_file);
							return game_boot_result::install_failed;
						}
					}
				}
			}

			if (!lock_file)
			{
				// Create lock file to prevent double installation
				// Do it after installation to prevent false positives when RPCS3 closed in the middle of the operation
				lock_file.open(lock_file_path, fs::read + fs::create + fs::excl);
			}
		}

		// Check game updates
		const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN";

		if (!m_ar && disc.empty() && !bdvd_dir.empty() && GetCallbacks().resolve_path(m_path) != GetCallbacks().resolve_path(hdd0_boot) && fs::is_file(hdd0_boot))
		{
			// Booting game update
			sys_log.success("Updates found at /dev_hdd0/game/%s/", m_title_id);
			m_path = hdd0_boot;
			return Load(m_title_id, false, true);
		}

		if (!disc_psf_obj.empty())
		{
			const auto bdvd_title = psf::get_string(disc_psf_obj, "TITLE");

			if (!bdvd_title.empty() && bdvd_title != m_title)
			{
				sys_log.notice("Title was set from %s to %s", m_title, bdvd_title);
				m_title = bdvd_title;
			}
		}

		for (auto& c : m_title)
		{
			// Replace newlines with spaces
			if (c == '\n')
				c = ' ';
		}

		// Mount /host_root/ if necessary (special value)
		if (g_cfg.vfs.host_root)
		{
			vfs::mount("/host_root", "/");
		}

		// Open SELF or ELF
		std::string elf_path = m_path;

		if (m_cat == "1P" || m_cat == "PE")
		{
			// Use emulator path
			elf_path = vfs::get(argv[0]);
		}

		if (m_ar)
		{
			g_tls_log_prefix = []()
			{
				return fmt::format("Emu State Load Thread: '%s'", g_tls_serialize_name);
			};
		}

		fs::file elf_file(elf_path);

		if (!elf_file)
		{
			sys_log.error("Failed to open executable: %s", elf_path);

			if (m_ar)
			{
				sys_log.warning("State Inspection Savestate Mode!");

				vm::init();
				vm::load(*m_ar);

				if (!hdd1.empty())
				{
					vfs::mount("/dev_hdd1", hdd1);
					sys_log.notice("Hdd1: %s", hdd1);
				}

				init_fxo_for_exec(DeserialManager(), true);

				return game_boot_result::no_errors;
			}

			return game_boot_result::invalid_file_or_folder;
		}

		// Check SELF header
		if (elf_file.size() >= 4 && elf_file.read<u32>() == "SCE\0"_u32)
		{
			const std::string decrypted_path = "boot.elf";

			fs::stat_t encrypted_stat = elf_file.stat();
			fs::stat_t decrypted_stat;

			// Check modification time and try to load decrypted ELF
			if (false && fs::stat(decrypted_path, decrypted_stat) && decrypted_stat.mtime == encrypted_stat.mtime)
			{
				elf_file.open(decrypted_path);
			}
			// Decrypt SELF
			else if ((elf_file = decrypt_self(std::move(elf_file), klic.empty() ? nullptr : reinterpret_cast<u8*>(&klic[0]), &g_ps3_process_info.self_info)))
			{
				if (true)
				{
				}
				else if (fs::file elf_out{decrypted_path, fs::rewrite})
				{
					elf_out.write(elf_file.to_vector<u8>());
					elf_out.close();
					fs::utime(decrypted_path, encrypted_stat.atime, encrypted_stat.mtime);
				}
				else
				{
					sys_log.error("Failed to create boot.elf");
				}
			}
		}
		else
		{
			g_ps3_process_info.self_info.valid = false;
		}

		if (!elf_file)
		{
			sys_log.error("Failed to decrypt SELF: %s", elf_path);
			return game_boot_result::decryption_error;
		}

		m_state = system_state::ready;

		ppu_exec_object ppu_exec;
		ppu_prx_object ppu_prx;
		ppu_rel_object ppu_rel;
		spu_exec_object spu_exec;
		spu_rel_object spu_rel;

		vm::init();

		if (m_ar)
		{
			vm::load(*m_ar);
		}

		if (!hdd1.empty())
		{
			vfs::mount("/dev_hdd1", hdd1);
			sys_log.notice("Hdd1: %s", vfs::get("/dev_hdd1"));
		}

		if (ppu_exec.open(elf_file) == elf_error::ok)
		{
			// PS3 executable
			GetCallbacks().on_ready();

			if (argv.empty())
			{
				argv.resize(1);
			}

			if (argv[0].empty())
			{
				auto unescape = [](std::string_view path)
				{
					// Unescape from host FS
					std::vector<std::string> escaped = fmt::split(path, {std::string_view{&fs::delim[0], 1}, std::string_view{&fs::delim[1], 1}});
					std::vector<std::string> result;
					for (auto& sv : escaped)
						result.emplace_back(vfs::unescape(sv));

					return fmt::merge(result, "/");
				};

				const std::string resolved_hdd0 = GetCallbacks().resolve_path(hdd0_game) + '/';

				if (from_hdd0_game && m_cat == "DG")
				{
					argv[0] = "/dev_bdvd/PS3_GAME/" + unescape(resolved_path.substr(resolved_hdd0.size() + 10));
					m_dir = "/dev_hdd0/game/" + resolved_path.substr(resolved_hdd0.size(), 10);
					sys_log.notice("Disc path: %s", m_dir);
				}
				else if (from_hdd0_game)
				{
					argv[0] = "/dev_hdd0/game/" + unescape(resolved_path.substr(resolved_hdd0.size()));
					m_dir = "/dev_hdd0/game/" + resolved_path.substr(resolved_hdd0.size(), 10);
					sys_log.notice("Boot path: %s", m_dir);
				}
				else if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
				{
					// Disc games are on /dev_bdvd/
					const usz pos = resolved_path.rfind(m_game_dir);
					argv[0] = "/dev_bdvd/PS3_GAME/" + unescape(resolved_path.substr(pos + m_game_dir.size() + 1));
					m_dir = "/dev_bdvd/PS3_GAME/";
				}
				else if (from_dev_flash)
				{
					// Firmware executables
					argv[0] = "/dev_flash" + resolved_path.substr(GetCallbacks().resolve_path(g_cfg_vfs.get_dev_flash()).size());
					m_dir = fs::get_parent_dir(argv[0]) + '/';
				}
				else if (g_cfg.vfs.host_root)
				{
					// For homebrew
					argv[0] = "/host_root/" + resolved_path;
					m_dir = "/host_root/" + elf_dir + '/';
				}
				else
				{
					// Use /app_home if /host_root is disabled
					argv[0] = "/app_home/" + resolved_path.substr(resolved_path.find_last_of(fs::delim) + 1);
					m_dir = "/app_home/";
				}

				sys_log.notice("Elf path: %s", argv[0]);
			}

			if (!argv[0].starts_with("/dev_hdd0/game"sv) && m_cat == "HG"sv)
			{
				sys_log.error("Booting HG category outside of HDD0!");
			}

			g_fxo->init<ppu_module>();

			if (ppu_load_exec(ppu_exec, DeserialManager()))
			{
			}
			// Overlay (OVL) executable (only load it)
			else if (vm::map(0x3000'0000, 0x1000'0000, 0x200); !ppu_load_overlay(ppu_exec, m_path).first)
			{
				ppu_exec.set_error(elf_error::header_type);
			}
			else
			{
				// Preserve emulation state for OVL excutable
				Pause(true);
			}

			if (ppu_exec != elf_error::ok)
			{
				Kill(false);

				sys_log.error("Invalid or unsupported PPU executable format: %s", elf_path);

				return game_boot_result::invalid_file_or_folder;
			}
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX
			GetCallbacks().on_ready();
			g_fxo->init(false);
			ppu_load_prx(ppu_prx, m_path);
			Pause(true);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable
			GetCallbacks().on_ready();
			g_fxo->init(false);
			spu_load_exec(spu_exec);
			Pause(true);
		}
		else if (spu_rel.open(elf_file) == elf_error::ok)
		{
			// SPU linker file
			GetCallbacks().on_ready();
			g_fxo->init(false);
			spu_load_rel_exec(spu_rel);
			Pause(true);
		}
		else if (ppu_rel.open(elf_file) == elf_error::ok)
		{
			// PPU linker file
			GetCallbacks().on_ready();
			g_fxo->init(false);
			ppu_load_rel_exec(ppu_rel);
			Pause(true);
		}
		else
		{
			sys_log.error("Invalid or unsupported file format: %s", elf_path);

			sys_log.warning("** ppu_exec -> %s", ppu_exec.get_error());
			sys_log.warning("** ppu_prx  -> %s", ppu_prx.get_error());
			sys_log.warning("** spu_exec -> %s", spu_exec.get_error());
			sys_log.warning("** spu_rel -> %s", spu_rel.get_error());
			sys_log.warning("** ppu_rel -> %s", ppu_rel.get_error());

			Kill(false);
			return game_boot_result::invalid_file_or_folder;
		}

		if (ppu_exec == elf_error::ok && !fs::is_file(g_cfg_vfs.get_dev_flash() + "sys/external/liblv2.sprx"))
		{
			const auto libs = g_cfg.core.libraries_control.get_set();

			extern const std::map<std::string_view, int> g_prx_list;

			// Check if there are any firmware SPRX which may be LLEd during emulation
			// Don't prompt GUI confirmation if there aren't any
			if (std::any_of(g_prx_list.begin(), g_prx_list.end(), [&libs](auto& lib)
			{
				return libs.count(std::string(lib.first) + ":lle") || (!lib.second && !libs.count(std::string(lib.first) + ":hle"));
			}))
			{
				Kill(false);

				CallFromMainThread([this]()
				{
					GetCallbacks().on_missing_fw();
				});

				return game_boot_result::firmware_missing;
			}
		}

		const bool autostart = (std::exchange(m_force_boot, false) || g_cfg.misc.autostart);

		if (IsReady())
		{
			if (autostart)
			{
				Run(true);
			}
		}

		return game_boot_result::no_errors;
	}
}

void Emulator::Run(bool start_playtime)
{
	ensure(IsReady());

	GetCallbacks().on_run(start_playtime);

	m_pause_start_time = 0;
	m_pause_amend_time = 0;

	rpcs3::utils::configure_logs();

	m_state = system_state::starting;

	if (g_cfg.misc.prevent_display_sleep)
	{
		disable_display_sleep();
	}
}

void Emulator::RunPPU()
{
	ensure(IsStarting());

	// Run main thread
	idm::select<named_thread<ppu_thread>>([](u32, named_thread<ppu_thread>& cpu)
	{
		if (cpu.stop_flag_removal_protection) return;
		ensure(cpu.state.test_and_reset(cpu_flag::stop));
		cpu.state.notify_one(cpu_flag::stop);
	});

	if (auto thr = g_fxo->try_get<named_thread<rsx::rsx_replay_thread>>())
	{
		thr->state -= cpu_flag::stop;
		thr->state.notify_one(cpu_flag::stop);
	}
}

void Emulator::FixGuestTime()
{
	if (m_ar)
	{
		initialize_timebased_time(m_ar->operator u64());

		g_cfg.savestate.state_inspection_mode.set(m_state_inspection_savestate);
		m_ar.reset();

		CallFromMainThread([this]
		{
			g_tls_log_prefix = []()
			{
				return std::string();
			};
		});
	}
	else
	{
		initialize_timebased_time(0);
	}
}
void Emulator::FinalizeRunRequest()
{
	auto on_select = [] (u32, spu_thread& cpu)
	{
		if (cpu.stop_flag_removal_protection) return;
		ensure(cpu.state.test_and_reset(cpu_flag::stop));
		cpu.state.notify_one(cpu_flag::stop);
	};

	idm::select<named_thread<spu_thread>>(on_select);

	lv2_obj::awake_all();

	m_state.compare_and_swap_test(system_state::starting, system_state::running);
}

bool Emulator::Pause(bool freeze_emulation)
{
	const u64 start = get_system_time();

	const system_state pause_state = freeze_emulation ? system_state::frozen : system_state::paused;

	// Try to pause
	if (!m_state.compare_and_swap_test(system_state::running, pause_state))
	{
		if (!freeze_emulation)
		{
			return false;
		}

		if (!m_state.compare_and_swap_test(system_state::ready, pause_state))
		{
			if (!m_state.compare_and_swap_test(system_state::paused, pause_state))
			{
				return false;
			}
		}
	}

	// Signal profilers to print results (if enabled)
	cpu_thread::flush_profilers();

	GetCallbacks().on_pause();

	static atomic_t<u32> pause_mark = 0;

	if (freeze_emulation)
	{
		sys_log.warning("Emulation has been frozen! You can either use debugger tools to inspect current emulation state or terminate it.");
	}
	else
	{
		sys_log.success("Emulation is being paused... (mark=%u)", pause_mark++);
	}

	// Update pause start time
	if (m_pause_start_time.exchange(start))
	{
		sys_log.error("Emulator::Pause() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_pause;
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);

	if (auto rsx = g_fxo->try_get<rsx::thread>())
	{
		rsx->state += cpu_flag::dbg_global_pause;
	}

	// Always Enable display sleep, not only if it was prevented.
	enable_display_sleep();

	return true;
}

void Emulator::Resume()
{
	if (m_state != system_state::paused)
	{
		return;
	}

	// Print and reset debug data collected
	if (g_cfg.core.ppu_debug)
	{
		PPUDisAsm dis_asm(cpu_disasm_mode::dump, vm::g_sudo_addr);

		std::string dump;

		for (u32 i = 0x10000; i < 0xE0000000;)
		{
			if (vm::check_addr(i, vm::page_executable))
			{
				if (auto& data = *reinterpret_cast<be_t<u32>*>(vm::g_stat_addr + i))
				{
					dis_asm.disasm(i);
					fmt::append(dump, "\n\t'%08X' %s", data, dis_asm.last_opcode);
					data = 0;
				}

				i += sizeof(u32);
			}
			else
			{
				i += 4096;
			}
		}

		ppu_log.notice("[RESUME] Dumping instruction stats:%s", dump);
	}

	// Try to resume
	if (!m_state.compare_and_swap_test(system_state::paused, system_state::running))
	{
		return;
	}

	// Get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// Try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}
	else
	{
		sys_log.error("Emulator::Resume() error: concurrent access");
	}

	perf_stat_base::report();

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_flag::dbg_global_pause;
		cpu.state.notify_one(cpu_flag::dbg_global_pause);
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);

	if (auto rsx = g_fxo->try_get<rsx::thread>())
	{
		// TODO: notify?
		rsx->state -= cpu_flag::dbg_global_pause;
	}

	GetCallbacks().on_resume();

	sys_log.success("Emulation has been resumed!");

	if (g_cfg.misc.prevent_display_sleep)
	{
		disable_display_sleep();
	}
}

s32 sysutil_send_system_cmd(u64 status, u64 param);
u64 get_sysutil_cb_manager_read_count();

void process_qt_events();

void Emulator::GracefulShutdown(bool allow_autoexit, bool async_op, bool savestate)
{
	const auto old_state = m_state.load();

	if (old_state == system_state::stopped)
	{
		return;
	}

	if (old_state == system_state::paused)
	{
		Resume();
	}

	const u64 read_counter = get_sysutil_cb_manager_read_count();

	if (old_state == system_state::frozen || savestate || !sysutil_send_system_cmd(0x0101 /* CELL_SYSUTIL_REQUEST_EXITGAME */, 0))
	{
		// The callback has been rudely ignored, we have no other option but to force termination
		Kill(allow_autoexit && !savestate, savestate);
		return;
	}

	auto perform_kill = [read_counter, allow_autoexit, this, info = ProcureCurrentEmulationCourseInformation()]()
	{
		bool read_sysutil_signal = false;

		for (u32 i = 100; i < 140; i++)
		{
			std::this_thread::sleep_for(50ms);
			Resume(); // TODO: Prevent pausing by other threads while in this loop
			process_qt_events(); // Is nullified when performed on non-main thread

			if (!read_sysutil_signal && read_counter != get_sysutil_cb_manager_read_count())
			{
				i -= 100; // Grant 5 seconds (if signal is not read force kill after two second)
				read_sysutil_signal = true;
			}

			if (static_cast<u64>(info) != m_stop_ctr)
			{
				return;
			}
		}

		// An inevitable attempt to terminate the *current* emulation course will be issued after 7s
		CallFromMainThread([allow_autoexit, this]()
		{
			Kill(allow_autoexit);
		}, info);
	};

	if (async_op)
	{
		std::thread{perform_kill}.detach();
	}
	else
	{
		perform_kill();
	}
}

extern bool try_lock_vdec_context_creation();

void Emulator::Kill(bool allow_autoexit, bool savestate)
{
	if (savestate && !try_lock_vdec_context_creation())
	{
		sys_log.error("Failed to savestate: HLE VDEC (video decoder) context(s) exist."
			"\nLLE libvdec.sprx by selecting it in Adavcned tab -> Firmware Libraries."
			"\nYou need to close the game for to take effect."
			"\nIf you cannot close the game due to losing important progress your best chance is to skip the current cutscenes if any are played and retry.");

		return;
	}

	g_tls_log_prefix = []()
	{
		return std::string();
	};

	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		// Ensure clean state
		argv.clear();
		envp.clear();
		data.clear();
		disc.clear();
		klic.clear();
		hdd1.clear();
		init_mem_containers = nullptr;
		m_config_path.clear();
		m_config_mode = cfg_mode::custom;
		return;
	}

	sys_log.notice("Stopping emulator...");

	{
		// Show visual feedback to the user in case that stopping takes a while.
		// This needs to be done before actually stopping, because otherwise the necessary threads will be terminated before we can show an image.
		if (auto progress_dialog = g_fxo->try_get<named_thread<progress_dialog_server>>(); progress_dialog && +g_progr)
		{
			// We are currently showing a progress dialog. Notify it that we are going to stop emulation.
			g_system_progress_stopping = true;
			std::this_thread::sleep_for(20ms); // Enough for one frame to be rendered
		}
	}

	m_ar.reset();

	if (savestate)
	{
		m_ar = std::make_unique<utils::serial>();
	}

	named_thread stop_watchdog("Stop Watchdog", [&]()
	{
		for (uint i = 0; thread_ctrl::state() != thread_state::aborting;)
		{
			// We don't need accurate timekeeping, using clocks may interfere with debugging
			if (i >= (savestate ? 2000 : 1000))
			{
				// Total amount of waiting: about 5s
				report_fatal_error("Stopping emulator took too long."
					"\nSome thread has probably deadlocked. Aborting.");
			}

			thread_ctrl::wait_for(5'000);

			if (!g_watchdog_hold_ctr)
			{
				// Don't count if there are still uninterruptable threads like PPU LLVM workers
				i++;
			}
		}
	});

	// Signal threads

	// Stop the replay thread "game" first
	if (auto thr = g_fxo->try_get<named_thread<rsx::rsx_replay_thread>>())
	{
		sys_log.notice("Stopping RSX replay thread...");
		thr->state += cpu_flag::stop;

		// Wait for a couple of seconds
		for (int i = 0; *thr <= thread_state::aborting && i < 300; i++)
		{
			std::this_thread::sleep_for(10ms);
			process_qt_events();
		}

		if (*thr <= thread_state::aborting)
		{
			sys_log.error("Failed to stop RSX replay thread in time.");
		}
		else
		{
			sys_log.notice("RSX replay thread stopped");
		}
	}

	if (auto rsx = g_fxo->try_get<rsx::thread>())
	{
		*static_cast<cpu_thread*>(rsx) = thread_state::aborting;
	}

	for (const auto& [type, data] : *g_fxo)
	{
		if (type.stop)
		{
			type.stop(data, thread_state::aborting);
		}
	}

	sys_log.notice("All emulation threads have been signaled.");

	// Wait fot newly created cpu_thread to see that emulation has been stopped
	id_manager::g_mutex.lock_unlock();

	GetCallbacks().on_stop();

	// Join threads
	for (const auto& [type, data] : *g_fxo)
	{
		if (type.stop)
		{
			type.stop(data, thread_state::finished);
		}
	}

	// Save it first for maximum timing accuracy
	const u64 timestamp = get_timebased_time();

	stop_watchdog = thread_state::aborting;

	sys_log.notice("All threads have been stopped.");

	if (savestate)
	{
		// Savestate thread
		named_thread emu_state_cap_thread("Emu State Capture Thread", [&]()
		{
			g_tls_log_prefix = []()
			{
				return fmt::format("Emu State Capture Thread: '%s'", g_tls_serialize_name);
			};

			auto& ar = *m_ar;

			for (serial_ver_t& ver : s_serial_versions)
			{
				ver.used = false;
			}

			using_global_version_serialization();
			using_ppu_serialization();

			// Avoid duplicating TAR object memory because it can be very large
			auto save_tar = [&](const std::string& path)
			{
				ar(usz{}); // Reserve memory to be patched later with correct size
				const usz old_size = ar.data.size();
				ar.data = tar_object::save_directory(path, std::move(ar.data));
				ar.seek_end();
				const usz tar_size = ar.data.size() - old_size;
				std::memcpy(ar.data.data() + old_size - sizeof(usz), &tar_size, sizeof(usz));
				sys_log.success("Saved the contents of directory '%s' (size=0x%x)", path, tar_size);
			};

			auto save_hdd1 = [&]()
			{
				const std::string _path = vfs::get("/dev_hdd1");
				std::string_view path = _path;

				path = path.substr(0, path.find_last_not_of(fs::delim) + 1);

				ar(std::string(path.substr(path.find_last_of(fs::delim) + 1)));

				if (!_path.empty())
				{
					if (!g_cfg.savestate.suspend_emu)
					{
						save_tar(_path);
					}
					else
					{
						ar(usz{});
					}
				}
			};

			auto save_hdd0 = [&]()
			{
				if (!g_cfg.savestate.suspend_emu && g_cfg.savestate.save_disc_game_data)
				{
					const std::string path = vfs::get("/dev_hdd0/game/");

					for (auto& entry : fs::dir(path))
					{
						if (entry.is_directory && entry.name != "." && entry.name != "..")
						{
							if (auto res = psf::load(path + entry.name + "/PARAM.SFO"); res && /*!m_title_id.empty() &&*/ psf::get_string(res.sfo, "TITLE_ID") == m_title_id && psf::get_string(res.sfo, "CATEGORY") == "GD")
							{
								ar(entry.name);
								save_tar(path + entry.name);
							}
						}
					}
				}

				ar(std::string{});
			};

			ar("RPCS3SAV"_u64);
			ar(std::endian::native == std::endian::little);
			ar(g_cfg.savestate.state_inspection_mode.get());
			ar(usz{0}); // Offset of versioning data, to be overwritten at the end of saving
			ar(argv[0]);
			ar(!m_title_id.empty() && !vfs::get("/dev_bdvd").empty() ? m_title_id : std::string());
			ar(klic.empty() ? std::array<u8, 16>{} : std::bit_cast<std::array<u8, 16>>(klic[0]));
			save_hdd1();
			save_hdd0();
			vm::save(ar);
			g_fxo->save(ar);
			ar(timestamp);
		});

		// Join it
		emu_state_cap_thread();

		if (emu_state_cap_thread == thread_state::errored)
		{
			sys_log.error("Saving savestate failed due to fatal error!");
			m_ar.reset();
			savestate = false;
		}
	}

	cpu_thread::cleanup();

	initialize_timebased_time(0, true);

	lv2_obj::cleanup();

	g_fxo->reset();

	sys_log.notice("Objects cleared...");

	vm::close();

	jit_runtime::finalize();

	perf_stat_base::report();

	static u64 aw_refs = 0;
	static u64 aw_colm = 0;
	static u64 aw_colc = 0;
	static u64 aw_used = 0;

	aw_refs = 0;
	aw_colm = 0;
	aw_colc = 0;
	aw_used = 0;

	atomic_wait::parse_hashtable([](u64 /*id*/, u32 refs, u64 ptr, u32 maxc) -> bool
	{
		aw_refs += refs != 0;
		aw_used += ptr != 0;

		aw_colm = std::max<u64>(aw_colm, maxc);
		aw_colc += maxc != 0;

		return false;
	});

	sys_log.notice("Atomic wait hashtable stats: [in_use=%u, used=%u, max_collision_weight=%u, total_collisions=%u]", aw_refs, aw_used, aw_colm, aw_colc);

	m_stop_ctr++;
	m_stop_ctr.notify_all();

	if (savestate)
	{
		const std::string path = fs::get_cache_dir() + "/savestates/" + (m_title_id.empty() ? m_path.substr(m_path.find_last_of(fs::delim) + 1) : m_title_id) + ".SAVESTAT";

		fs::pending_file file(path);

		// Identifer -> version
		std::vector<std::pair<u16, u16>> used_serial;
		used_serial.reserve(s_serial_versions.size());

		for (const serial_ver_t& ver : s_serial_versions)
		{
			if (ver.used)
			{
				used_serial.emplace_back(&ver - s_serial_versions.data(), *ver.compatible_versions.rbegin());
			}
		}

		auto& ar = *m_ar;
		const usz pos = ar.seek_end();
		std::memcpy(&ar.data[10], &pos, 8);// Set offset
		ar(used_serial);

		if (!file.file || (file.file.write(ar.data), !file.commit()))
		{
			sys_log.error("Failed to write savestate to file! (path='%s', %s)", path, fs::g_tls_error);
		}
		else
		{
			sys_log.success("Saved savestate! path='%s'", path);
		}

		ar.pos = 0;
	}

	// Boot arg cleanup (preserved in the case restarting)
	m_ar.reset();
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();
	hdd1.clear();
	init_mem_containers = nullptr;
	m_config_path.clear();
	m_config_mode = cfg_mode::custom;

	// Always Enable display sleep, not only if it was prevented.
	enable_display_sleep();

	if (allow_autoexit)
	{
		if (Quit(g_cfg.misc.autoexit.get()))
		{
			return;
		}
	}
}

game_boot_result Emulator::Restart(bool savestate)
{
	if (m_state == system_state::stopped)
	{
		return game_boot_result::generic_error;
	}

	auto save_args = std::make_tuple(argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_mode);

	GracefulShutdown(false, false, savestate);

	std::tie(argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_mode) = std::move(save_args);

	if (savestate)
	{
		if (!m_ar)
		{
			return game_boot_result::generic_error;
		}

		if (g_cfg.savestate.suspend_emu)
		{
			return game_boot_result::no_errors;
		}
	}

	// Reload with prior configs.
	if (const auto error = Load(m_title_id); error != game_boot_result::no_errors)
	{
		sys_log.error("Restart failed: %s", error);
		return error;
	}
	else
	{
		m_ar.reset();
	}

	return game_boot_result::no_errors;
}

bool Emulator::Quit(bool force_quit)
{
	m_force_boot = false;

	// The callback is only used if we actually quit RPCS3
	const auto on_exit = []()
	{
		Emu.CleanUp();
	};

	if (GetCallbacks().try_to_quit)
	{
		return GetCallbacks().try_to_quit(force_quit, on_exit);
	}

	on_exit();
	return true;
}

void Emulator::CleanUp()
{
	// Deinitialize object manager to prevent any hanging objects at program exit
	g_fxo->clear();
}

std::string Emulator::GetFormattedTitle(double fps) const
{
	rpcs3::title_format_data title_data;
	title_data.format = g_cfg.misc.title_format.to_string();
	title_data.title = GetTitle();
	title_data.title_id = GetTitleID();
	title_data.renderer = g_cfg.video.renderer.to_string();
	title_data.vulkan_adapter = g_cfg.video.vk.adapter.to_string();
	title_data.fps = fps;

	return rpcs3::get_formatted_title(title_data);
}

s32 error_code::error_report(s32 result, const char* fmt, const fmt_type_info* sup, const u64* args)
{
	static thread_local std::unordered_map<std::string, usz> g_tls_error_stats;
	static thread_local std::string g_tls_error_str;

	if (!sup && !args)
	{
		if (!fmt)
		{
			// Report and clean error state

			if (g_log_all_errors) [[unlikely]]
			{
				g_tls_error_stats.clear();
				return 0;
			}

			for (auto&& pair : g_tls_error_stats)
			{
				if (pair.second > 3)
				{
					sys_log.error("Stat: %s [x%u]", pair.first, pair.second);
				}
			}

			g_tls_error_stats.clear();
			return 0;
		}
	}

	ensure(fmt);

	logs::channel* channel = &sys_log;
	const char* func = "Unknown function";

	if (auto ppu = get_current_cpu_thread<ppu_thread>())
	{
		if (ppu->current_function)
		{
			func = ppu->current_function;
		}
	}

	// Format log message (use preallocated buffer)
	g_tls_error_str.clear();
	fmt::append(g_tls_error_str, "'%s' failed with 0x%08x", func, result);

	// Add spacer between error and fmt if necessary
	if (fmt[0] != ' ')
		g_tls_error_str += " : ";

	fmt::raw_append(g_tls_error_str, fmt, sup, args);

	// Update stats and check log threshold

	if (g_log_all_errors) [[unlikely]]
	{
		channel->error("%s", g_tls_error_str);
	}
	else
	{
		const auto stat = ++g_tls_error_stats[g_tls_error_str];

		if (stat <= 3)
		{
			channel->error("%s [%u]", g_tls_error_str, stat);
		}
	}

	return result;
}

void Emulator::ConfigurePPUCache() const
{
	auto& _main = g_fxo->get<ppu_module>();

	_main.cache = rpcs3::utils::get_cache_dir();

	if (!m_title_id.empty() && m_cat != "1P")
	{
		_main.cache += GetTitleID();
		_main.cache += '/';
	}

	fmt::append(_main.cache, "ppu-%s-%s/", fmt::base57(_main.sha1), _main.path.substr(_main.path.find_last_of('/') + 1));

	if (!fs::create_path(_main.cache))
	{
		sys_log.error("Failed to create cache directory: %s (%s)", _main.cache, fs::g_tls_error);
	}
	else
	{
		sys_log.notice("Cache: %s", _main.cache);
	}
}

std::set<std::string> Emulator::GetGameDirs() const
{
	std::set<std::string> dirs;

	// Add boot directory.
	// For installed titles and disc titles with updates this is usually /dev_hdd0/game/<title_id>/
	// For disc titles without updates this is /dev_bdvd/PS3_GAME/
	if (const std::string dir = vfs::get(GetDir()); !dir.empty())
	{
		dirs.insert(dir + '/');
	}

	// Add more paths for disc titles.
	if (const std::string dev_bdvd = vfs::get("/dev_bdvd/PS3_GAME");
		!dev_bdvd.empty() && !GetTitleID().empty())
	{
		// Add the dev_bdvd dir if available. This is necessary for disc titles with installed updates.
		dirs.insert(dev_bdvd + '/');

		// Naive search for all matching game data dirs.
		const std::string game_dir = vfs::get("/dev_hdd0/game/");
		for (auto&& entry : fs::dir(game_dir))
		{
			if (entry.is_directory && entry.name.starts_with(GetTitleID()))
			{
				const std::string sfo_dir = game_dir + entry.name + '/';
				const fs::file sfo_file(sfo_dir + "PARAM.SFO");
				if (!sfo_file)
				{
					continue;
				}

				const psf::registry psf    = psf::load_object(sfo_file);
				const std::string title_id = std::string(psf::get_string(psf, "TITLE_ID", ""));

				if (title_id == GetTitleID())
				{
					dirs.insert(sfo_dir);
				}
			}
		}
	}

	return dirs;
}

bool Emulator::IsPathInsideDir(std::string_view path, std::string_view dir) const
{
	const std::string dir_path = GetCallbacks().resolve_path(dir);

	return !dir_path.empty() && (GetCallbacks().resolve_path(path) + '/').starts_with(dir_path + '/');
};

const std::string& Emulator::GetFakeCat() const
{
	if (m_cat == "DG")
	{
		const std::string mount_point = vfs::get("/dev_bdvd");

		if (mount_point.empty() || !IsPathInsideDir(m_path, mount_point))
		{
			static const std::string s_hg = "HG";
			return s_hg;
		}
	}

	return m_cat;
};

const std::string Emulator::GetSfoDir(bool prefer_disc_sfo) const
{
	if (prefer_disc_sfo)
	{
		const std::string sfo_dir = vfs::get("/dev_bdvd/PS3_GAME");

		if (!sfo_dir.empty())
		{
			return sfo_dir;
		}
	}

	return m_sfo_dir;
}

void Emulator::GetBdvdDir(std::string& bdvd_dir, std::string& sfb_dir, std::string& game_dir, const std::string& elf_dir)
{
	// Find disc directory by searching a valid PS3_DISC.SFB closest to root directory
	std::string main_dir;
	std::string_view main_dir_name;

	for (std::string search_dir = elf_dir;;)
	{
		std::string parent_dir = fs::get_parent_dir(search_dir);

		if (parent_dir.size() == search_dir.size())
		{
			// Keep looking until root directory is reached
			break;
		}

		if (fs::file sfb_file{parent_dir + "/PS3_DISC.SFB", fs::read + fs::isfile}; sfb_file && sfb_file.size() >= 4 && sfb_file.read<u32>() == ".SFB"_u32)
		{
			main_dir_name = std::string_view{search_dir}.substr(search_dir.find_last_of(fs::delim) + 1);

			if (main_dir_name == "PS3_GAME" || std::regex_match(main_dir_name.begin(), main_dir_name.end(), std::regex("^PS3_GM[[:digit:]]{2}$")))
			{
				// Remember valid disc directory
				main_dir = search_dir;
				sfb_dir = parent_dir;
				main_dir_name = std::string_view{main_dir}.substr(main_dir.find_last_of(fs::delim) + 1);
			}
		}

		search_dir = std::move(parent_dir);
	}

	if (!sfb_dir.empty())
	{
		bdvd_dir = sfb_dir + "/";
		game_dir = std::string{main_dir_name};
	}
}

void Emulator::EjectDisc()
{
	if (!Emu.IsRunning())
	{
		sys_log.error("Can not eject disc if the Emulator is not running!");
		return;
	}

	if (vfs::get("/dev_bdvd").empty() && vfs::get("/dev_ps2disc").empty())
	{
		sys_log.error("Can not eject disc if both dev_bdvd and dev_ps2disc are not mounted!");
		return;
	}

	sys_log.notice("Ejecting disc...");

	m_sfo_dir.clear();

	if (g_fxo->is_init<disc_change_manager>())
	{
		g_fxo->get<disc_change_manager>().eject_disc();
	}
}

game_boot_result Emulator::InsertDisc(const std::string& path)
{
	if (!Emu.IsRunning())
	{
		sys_log.error("Can not insert disc if the Emulator is not running!");
		return game_boot_result::generic_error;
	}

	sys_log.notice("Inserting disc... (path='%s')", path);

	const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
	const bool from_hdd0_game = IsPathInsideDir(path, hdd0_game);

	if (from_hdd0_game)
	{
		sys_log.error("Inserting disc failed: Can not mount discs from '/dev_hdd0/game/'. (path='%s')", path);
		return game_boot_result::wrong_disc_location;
	}

	std::string disc_root;
	std::string ps3_game_dir;
	const disc::disc_type disc_type = disc::get_disc_type(path, disc_root, ps3_game_dir);

	if (disc_type == disc::disc_type::invalid)
	{
		sys_log.error("Inserting disc failed: not a disc (path='%s')", path);
		return game_boot_result::wrong_disc_location;
	}

	ensure(!disc_root.empty());

	u32 type = CELL_GAME_DISCTYPE_OTHER;
	std::string title_id;

	if (disc_type == disc::disc_type::ps3)
	{
		type = CELL_GAME_DISCTYPE_PS3;

		// Double check PARAM.SFO
		const std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(disc_root);
		const psf::registry _psf = psf::load_object(fs::file(sfo_dir + "/PARAM.SFO"));

		if (_psf.empty())
		{
			sys_log.error("Inserting disc failed: Corrupted PARAM.SFO found! (path='%s/PARAM.SFO')", sfo_dir);
			return game_boot_result::invalid_file_or_folder;
		}

		title_id = std::string(psf::get_string(_psf, "TITLE_ID"));

		if (title_id.empty())
		{
			sys_log.error("Inserting disc failed: Corrupted PARAM.SFO found! TITLE_ID empty (path='%s/PARAM.SFO')", sfo_dir);
			return game_boot_result::invalid_file_or_folder;
		}

		m_sfo_dir = sfo_dir;
		m_game_dir = ps3_game_dir;

		sys_log.notice("New sfo dir: %s", m_sfo_dir);
		sys_log.notice("New game dir: %s", m_game_dir);

		ensure(vfs::mount("/dev_bdvd", disc_root));
		ensure(vfs::mount("/dev_bdvd/PS3_GAME", disc_root + m_game_dir + "/"));
	}
	else if (disc_type == disc::disc_type::ps2)
	{
		type = CELL_GAME_DISCTYPE_PS2;

		ensure(vfs::mount("/dev_ps2disc", disc_root));
	}
	else
	{
		// TODO: find out where other discs are mounted
		sys_log.todo("Mounting non-ps2/ps3 disc in dev_bdvd. Is this correct? (path='%s')", disc_root);
		ensure(vfs::mount("/dev_bdvd", disc_root));
	}

	if (g_fxo->is_init<disc_change_manager>())
	{
		g_fxo->get<disc_change_manager>().insert_disc(type, std::move(title_id));
	}

	return game_boot_result::no_errors;
}

utils::serial* Emulator::DeserialManager() const
{
	ensure(!m_ar || !m_ar->is_writing());
	return m_ar.get();
}

Emulator Emu;
