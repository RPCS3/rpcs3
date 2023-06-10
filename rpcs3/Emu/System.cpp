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
#include "Emu/Cell/lv2/sys_spu.h"
#include "Emu/Cell/Modules/cellGame.h"
#include "Emu/Cell/Modules/cellSysutil.h"

#include "Emu/title.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Capture/rsx_replay.h"
#include "Emu/RSX/Overlays/overlay_message.h"

#include "Loader/PSF.h"
#include "Loader/TAR.h"
#include "Loader/ELF.h"
#include "Loader/disc.h"

#include "Utilities/StrUtil.h"

#include "../Crypto/unself.h"
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
extern bool is_savestate_version_compatible(const std::vector<std::pair<u16, u16>>& data, bool is_boot_check);
extern std::vector<std::pair<u16, u16>> read_used_savestate_versions();
std::string get_savestate_path(std::string_view title_id, std::string_view boot_path);

extern void send_close_home_menu_cmds();

fs::file g_tty;
atomic_t<s64> g_tty_size{0};
std::array<std::deque<std::string>, 16> g_tty_input;
std::mutex g_tty_mutex;
thread_local std::string_view g_tls_serialize_name;

extern thread_local std::string(*g_tls_log_prefix)();

// Report error and call std::abort(), defined in main.cpp
[[noreturn]] void report_fatal_error(std::string_view text, bool is_html = false, bool include_help_text = true);

void initialize_timebased_time(u64 timebased_init, bool reset = false);

namespace atomic_wait
{
	extern void parse_hashtable(bool(*cb)(u64 id, u32 refs, u64 ptr, u32 max_coll));
}

namespace rsx
{
	void set_native_ui_flip();
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
		case game_boot_result::invalid_bdvd_folder: return "Invalid dev_bdvd folder";
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
		ensure(thread_ctrl::get_current());
		thread_ctrl::wait_on(wake_up, false);
	}
}

// This function ensures constant initialization order between different compilers and builds
void init_fxo_for_exec(utils::serial* ar, bool full = false)
{
	g_fxo->init<main_ppu_module>();

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

	if (ar)
	{
		Emu.ExecDeserializationRemnants();

		[[maybe_unused]] auto flags = (*ar)(Emu.m_savestate_extension_flags1);

		const usz advance = (Emu.m_savestate_extension_flags1 & Emulator::SaveStateExtentionFlags1::SupportsMenuOpenResume ? 32 : 31);

		ar->pos += advance; // Reserved area
	}
}

// Some settings are not allowed in certain PPU decoders
void fixup_ppu_settings()
{
	if (g_cfg.core.ppu_decoder != ppu_decoder_type::_static)
	{
		if (g_cfg.core.ppu_use_nj_bit)
		{
			sys_log.todo("The setting '%s' is currently not supported with PPU decoder type '%s' and will therefore be disabled during emulation.", g_cfg.core.ppu_use_nj_bit.get_name(), g_cfg.core.ppu_decoder.get());
			g_cfg.core.ppu_use_nj_bit.set(false);
		}

		if (g_cfg.core.ppu_set_vnan)
		{
			sys_log.todo("The setting '%s' is currently not supported with PPU decoder type '%s' and will therefore be disabled during emulation.", g_cfg.core.ppu_set_vnan.get_name(), g_cfg.core.ppu_decoder.get());
			g_cfg.core.ppu_set_vnan.set(false);
		}

		if (g_cfg.core.ppu_set_fpcc)
		{
			sys_log.todo("The setting '%s' is currently not supported with PPU decoder type '%s' and will therefore be disabled during emulation.", g_cfg.core.ppu_set_fpcc.get_name(), g_cfg.core.ppu_decoder.get());
			g_cfg.core.ppu_set_fpcc.set(false);
		}
	}
}

void Emulator::Init()
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
	const std::string dev_bdvd = g_cfg_vfs.get(g_cfg_vfs.dev_bdvd, emu_dir); // Only used for make_path
	const std::string dev_hdd0 = g_cfg_vfs.get(g_cfg_vfs.dev_hdd0, emu_dir);
	const std::string dev_hdd1 = g_cfg_vfs.get(g_cfg_vfs.dev_hdd1, emu_dir);
	const std::string dev_flash = g_cfg_vfs.get_dev_flash();
	const std::string dev_flash2 = g_cfg_vfs.get_dev_flash2();
	const std::string dev_flash3 = g_cfg_vfs.get_dev_flash3();

	vfs::mount("/dev_hdd0", dev_hdd0);
	vfs::mount("/dev_flash", dev_flash);
	vfs::mount("/dev_flash2", dev_flash2);
	vfs::mount("/dev_flash3", dev_flash3);
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

		if (fs::is_dir(usb_info.path))
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

	// Disable incompatible settings
	fixup_ppu_settings();

	// Backup config
	g_backup_cfg.from_string(g_cfg.to_string());

	// Create directories (can be disabled if necessary)
	auto make_path_verbose = [&](const std::string& path, bool must_exist_outside_emu_dir)
	{
		if (fs::is_dir(path))
		{
			return true;
		}

		if (must_exist_outside_emu_dir)
		{
			const std::string parent = fs::get_parent_dir(path);
			const std::string emu_dir_no_delim = emu_dir.substr(0, emu_dir.find_last_not_of(fs::delim) + 1);

			if (parent != emu_dir_no_delim && GetCallbacks().resolve_path(parent) != GetCallbacks().resolve_path(emu_dir_no_delim))
			{
				sys_log.fatal("Cannot use '%s' for Virtual File System because it does not exist.\nPlease specify an existing and writable directory path in Toolbar -> Manage -> Virtual File System.", path);
				return false;
			}
		}

		if (!fs::create_path(path))
		{
			sys_log.fatal("Failed to create path: %s (%s)", path, fs::g_tls_error);
			return false;
		}

		return true;
	};

	const std::string save_path = dev_hdd0 + "home/" + m_usr + "/savedata/";
	const std::string user_path = dev_hdd0 + "home/" + m_usr + "/localusername";

	if (g_cfg.vfs.init_dirs)
	{
		make_path_verbose(dev_bdvd, true);
		make_path_verbose(dev_flash, true);
		make_path_verbose(dev_flash2, true);
		make_path_verbose(dev_flash3, true);
		make_path_verbose(dev_usb, true);

		if (make_path_verbose(dev_hdd1, true))
		{
			make_path_verbose(dev_hdd1 + "caches/", false);
		}

		if (make_path_verbose(dev_hdd0, true))
		{
			make_path_verbose(dev_hdd0 + "game/", false);
			make_path_verbose(dev_hdd0 + reinterpret_cast<const char*>(u8"game/＄locks/"), false);
			make_path_verbose(dev_hdd0 + "game/TEST12345/USRDIR/", false); // Some test elfs rely on this
			make_path_verbose(dev_hdd0 + "home/", false);
			make_path_verbose(dev_hdd0 + "home/" + m_usr + "/", false);
			make_path_verbose(dev_hdd0 + "home/" + m_usr + "/exdata/", false);
			make_path_verbose(save_path, false);
			make_path_verbose(dev_hdd0 + "home/" + m_usr + "/trophy/", false);

			if (!fs::write_file(user_path, fs::create + fs::excl + fs::write, "User"s))
			{
				if (fs::g_tls_error != fs::error::exist)
				{
					sys_log.fatal("Failed to create file: %s (%s)", user_path, fs::g_tls_error);
				}
			}

			make_path_verbose(dev_hdd0 + "savedata/", false);
			make_path_verbose(dev_hdd0 + "savedata/vmc/", false);
			make_path_verbose(dev_hdd0 + "photo/", false);
		}

		const std::string games_common_dir = g_cfg_vfs.get(g_cfg_vfs.games_dir, emu_dir);

		if (make_path_verbose(games_common_dir, true))
		{
			fs::write_file(games_common_dir + "/Disc Games Can Be Put Here For Automatic Detection.txt", fs::create + fs::excl + fs::write, ""s);

#ifdef _WIN32
			if (std::string rpcs3_shortcuts = games_common_dir + "/shortcuts"; make_path_verbose(rpcs3_shortcuts, false))
			{
				fs::write_file(rpcs3_shortcuts + "/Copyable Shortcuts For Installed Games Would Be Added Here.txt", fs::create + fs::excl + fs::write, ""s);
			}
#endif
		}
	}

	make_path_verbose(fs::get_cache_dir() + "shaderlog/", false);
	make_path_verbose(fs::get_cache_dir() + "spu_progs/", false);
	make_path_verbose(fs::get_cache_dir() + "/savestates/", false);
	make_path_verbose(fs::get_config_dir() + "captures/", false);
	make_path_verbose(fs::get_config_dir() + "sounds/", false);
	make_path_verbose(patch_engine::get_patches_path(), false);

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
	m_state = system_state::starting;

	ensure(g_fxo->init<named_thread<rsx::rsx_replay_thread>>("RSX Replay", std::move(frame)));

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

game_boot_result Emulator::BootGame(const std::string& path, const std::string& title_id, bool direct, cfg_mode config_mode, const std::string& config_path)
{
	auto save_args = std::make_tuple(m_path, m_path_original, argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_path);

	auto restore_on_no_boot = [&](game_boot_result result)
	{
		if (IsStopped() || result != game_boot_result::no_errors)
		{
			std::tie(m_path, m_path_original, argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_path) = std::move(save_args);
		}

		return result;
	};

	if (m_path_original.empty() || config_mode != cfg_mode::continuous)
	{
		m_path_original = m_path;
	}

	m_path_old = m_path;

	m_config_mode = config_mode;
	m_config_path = config_path;

	// Handle files and special paths inside Load unmodified
	if (direct || !fs::is_dir(path))
	{
		m_path = path;

		return restore_on_no_boot(Load(title_id));
	}

	game_boot_result result = game_boot_result::nothing_to_boot;

	std::string elf;
	if (const game_boot_result res = GetElfPathFromDir(elf, path); res == game_boot_result::no_errors)
	{
		ensure(!elf.empty());
		m_path = elf;
		result = Load(title_id);
	}

	return restore_on_no_boot(result);
}

void Emulator::SetForceBoot(bool force_boot)
{
	m_force_boot = force_boot;
}

game_boot_result Emulator::Load(const std::string& title_id, bool is_disc_patch, usz recursion_count)
{
	m_ar.reset();

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
			if (fs::file save{m_path, fs::isfile + fs::read}; save && save.size() >= 8 && save.read<u64>() == "RPCS3SAV"_u64)
			{
				m_ar = std::make_shared<utils::serial>();
				m_ar->set_reading_state();
				save.seek(0);
				save.read(m_ar->data, save.size());
				m_ar->data.shrink_to_fit();
			}

			m_boot_source_type = CELL_GAME_GAMETYPE_SYS;
		}
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

	struct cleanup_t
	{
		Emulator* _this;
		bool cleanup = true;

		~cleanup_t()
		{
			if (cleanup && _this->IsStopped())
			{
				_this->Kill(false);
			}
		}
	} cleanup{this};

	{
		Init();

		m_state_inspection_savestate = g_cfg.savestate.state_inspection_mode.get();
		m_savestate_extension_flags1 = {};

		bool resolve_path_as_vfs_path = false;

		const bool from_dev_flash  = IsPathInsideDir(m_path, g_cfg_vfs.get_dev_flash());

		if (m_ar)
		{
			struct file_header
			{
				ENABLE_BITWISE_SERIALIZATION;

				nse_t<u64, 1> magic;
				bool LE_format;
				bool state_inspection_support;
				nse_t<u64, 1> offset;
				std::array<u8, 32> reserved;
			};

			const auto header = m_ar->try_read<file_header>().second;

			if (header.magic != "RPCS3SAV"_u64)
			{
				return game_boot_result::savestate_corrupted;
			}

			if (header.LE_format != (std::endian::native == std::endian::little) || header.offset >= m_ar->data.size())
			{
				return game_boot_result::savestate_corrupted;
			}

			g_cfg.savestate.state_inspection_mode.set(header.state_inspection_support);

			// Emulate seek operation (please avoid using in other places)
			m_ar->pos = header.offset;

			if (!is_savestate_version_compatible(m_ar->operator std::vector<std::pair<u16, u16>>(), true))
			{
				return game_boot_result::savestate_version_unsupported;
			}

			m_ar->pos = sizeof(file_header); // Restore position

			argv.clear();
			klic.clear();

			std::string disc_info;
			(*m_ar)(argv.emplace_back(), disc_info, klic.emplace_back(), m_game_dir, hdd1);

			if (!klic[0])
			{
				klic.clear();
			}

			if (!disc_info.empty() && disc_info[0] != '/')
			{
				// Restore disc path for disc games (must exist in games.yml i.e. your game library)
				m_title_id = disc_info;

				// Load /dev_bdvd/ from game list if available
				if (std::string game_path = m_games_config.get_path(m_title_id); !game_path.empty())
				{
					disc = std::move(game_path);
				}
				else if (!g_cfg.savestate.state_inspection_mode)
				{
					sys_log.fatal("Disc directory not found. Savestate cannot be loaded. ('%s')", m_title_id);
					return game_boot_result::invalid_file_or_folder;
				}
			}

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
				const std::string game_data = *m_ar;

				if (game_data.empty())
				{
					break;
				}

				load_tar(hdd0_game + game_data);
			}

			m_ar->pos += 32; // Reserved area

			if (disc_info.starts_with("/"sv))
			{
				// Restore SFO directory for PSN games

				if (disc_info.starts_with("/dev_hdd0"sv))
				{
					disc = rpcs3::utils::get_hdd0_dir();
					disc += std::string_view(disc_info).substr(9);
				}
				else if (disc_info.starts_with("/host_root"sv))
				{
					sys_log.error("Host root has been used in savestates!");
					disc = disc_info.substr(9);
				}
				else
				{
					sys_log.error("Unknown source for game SFO directory: %s", disc_info);
				}

				m_cat.clear();
			}

			m_path_old = m_path;
			resolve_path_as_vfs_path = true;
		}
		else if (m_path.starts_with(vfs_boot_prefix))
		{
			m_path = m_path.substr(vfs_boot_prefix.size());

			if (!m_path.empty() && m_path[0] != '/')
			{
				// Make valid for VFS
				m_path.insert(0, "/");
			}

			if (!argv.empty())
			{
				argv[0] = m_path;
			}
			else
			{
				argv.emplace_back(m_path);
			}

			resolve_path_as_vfs_path =  true;
		}
		else if (m_path.starts_with(game_id_boot_prefix))
		{
			// Try to boot a game through game ID only
			m_title_id = m_path.substr(game_id_boot_prefix.size());
			m_title_id = m_title_id.substr(0, m_title_id.find_first_of(fs::delim));

			if (m_title_id.size() < 3 && m_title_id.find_first_not_of('.') == umax)
			{
				// Do not allow if TITLE_ID result in path redirection
				sys_log.fatal("Game directory not found using GAMEID token. ('%s')", m_title_id);
				return game_boot_result::invalid_file_or_folder;
			}

			std::string tail = m_path.substr(game_id_boot_prefix.size() + m_title_id.size());

			if (tail.find_first_not_of(fs::delim) == umax)
			{
				// Treat slashes-only trail as if game ID only was provided
				tail.clear();
			}

			bool ok = false;
			std::string title_path;

			// const overload does not create new node on failure
			if (std::string game_path = m_games_config.get_path(m_title_id); !game_path.empty())
			{
				title_path = std::move(game_path);
			}

			for (std::string test_path :
			{
				rpcs3::utils::get_hdd0_dir() + "game/" + m_title_id + "/USRDIR/EBOOT.BIN"
				, tail.empty() ? "" : title_path + tail + "/USRDIR/EBOOT.BIN"
				, title_path + "/PS3_GAME/USRDIR/EBOOT.BIN"
				, title_path + "/USRDIR/EBOOT.BIN"
			})
			{
				if (!test_path.empty() && fs::is_file(test_path))
				{
					m_path = std::move(test_path);
					ok = true;
					break;
				}
			}

			if (!ok)
			{
				sys_log.fatal("Game directory not found using GAMEID token. ('%s')", m_title_id + tail);
				return game_boot_result::invalid_file_or_folder;
			}
		}

		if (resolve_path_as_vfs_path)
		{
			if (argv[0].starts_with("/dev_hdd0"sv))
			{
				m_path = rpcs3::utils::get_hdd0_dir();
				m_path += std::string_view(argv[0]).substr(9);

				constexpr auto game0_path = "/dev_hdd0/game/"sv;

				if (argv[0].starts_with(game0_path) && !fs::is_file(vfs::get(argv[0])))
				{
					std::string title_id = argv[0].substr(game0_path.size());
					title_id = title_id.substr(0, title_id.find_first_of('/'));

					// Try to load game directory from list if available
					if (std::string game_path = m_games_config.get_path(m_title_id); !game_path.empty())
					{
						disc = std::move(game_path);
						m_path = disc + argv[0].substr(game0_path.size() + title_id.size());
					}
				}
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
			else if (argv[0].starts_with("/host_root/"sv))
			{
				sys_log.error("Host root has been used in path redirection!");
				m_path = argv[0].substr(11);
			}
			else if (argv[0].starts_with("/dev_hdd1"sv))
			{
				sys_log.error("HDD1 has been used to store executable in path redirection!");
				m_path = rpcs3::utils::get_hdd1_dir();
				m_path += std::string_view(argv[0]).substr(9);
			}
			else
			{
				sys_log.error("Unknown source for path redirection: %s", argv[0]);
			}

			if (argv.size() == 1)
			{
				// Resolve later properly as if booted through host path
				argv.clear();
			}

			sys_log.notice("Restored executable path: \'%s\'", m_path);
		}

		const std::string resolved_path = GetCallbacks().resolve_path(m_path);

		const std::string elf_dir = fs::get_parent_dir(m_path);

		// Mount /app_home again since m_path might have changed due to savestates.
		vfs::mount("/app_home", g_cfg_vfs.app_home.to_string().empty() ? elf_dir + '/' : g_cfg_vfs.get(g_cfg_vfs.app_home, rpcs3::utils::get_emu_dir()));

		// Load PARAM.SFO (TODO)
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
				m_sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(fs::get_parent_dir(elf_dir), m_title_id);
			}
		}

		const psf::registry _psf = psf::load_object(m_sfo_dir + "/PARAM.SFO");

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
				// Load custom configs
				for (std::string config_path :
				{
					m_path + ".yml",
					rpcs3::utils::get_custom_config_path(from_dev_flash ? m_path.substr(m_path.find_last_of(fs::delim) + 1) : m_title_id),
				})
				{
					if (config_path.empty())
					{
						continue;
					}

					if (fs::file cfg_file{config_path})
					{
						sys_log.notice("Applying custom config: %s", config_path);

						if (g_cfg.from_string(cfg_file.to_string()))
						{
							g_cfg.name = config_path;
							m_config_path = config_path;
							break;
						}

						sys_log.fatal("Failed to apply custom config: %s", config_path);
					}
				}
			}

			// Disable incompatible settings
			fixup_ppu_settings();

			// Force audio provider
			if (m_path.ends_with("vsh.self"sv))
			{
				g_cfg.audio.provider.set(audio_provider::rsxaudio);
			}
			else
			{
				g_cfg.audio.provider.set(audio_provider::cell_audio);
			}

			// Backup config
			g_backup_cfg.from_string(g_cfg.to_string());
		}

		// Set RTM usage
		g_use_rtm = utils::has_rtm() && (((utils::has_mpx() && !utils::has_tsx_force_abort()) && g_cfg.core.enable_TSX == tsx_usage::enabled) || g_cfg.core.enable_TSX == tsx_usage::forced);

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
		std::string bdvd_dir = g_cfg_vfs.get(g_cfg_vfs.dev_bdvd, rpcs3::utils::get_emu_dir());
		{
			if (!bdvd_dir.empty())
			{
				if (bdvd_dir.back() != fs::delim[0] && bdvd_dir.back() != fs::delim[1])
				{
					bdvd_dir.push_back('/');
				}

				if (!fs::is_file(bdvd_dir + "PS3_DISC.SFB"))
				{
					if (fs::get_dir_size(bdvd_dir) == 0)
					{
						// Ignore empty dir. We will need it later for disc games in dev_hdd0.
						sys_log.notice("Ignoring empty vfs BDVD directory: '%s'", bdvd_dir);
					}
					else
					{
						// Unuse if invalid
						sys_log.error("Failed to use custom BDVD directory: '%s'", bdvd_dir);
					}

					bdvd_dir.clear();
				}
			}
		}

		// Special boot mode (directory scan)
		if (fs::is_dir(m_path))
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			g_fxo->init<main_ppu_module>();
			vm::init();
			m_force_boot = false;

			// Force LLVM recompiler
			g_cfg.core.ppu_decoder.from_default();

			// Disable incompatible settings
			fixup_ppu_settings();

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

			Run(false);

			std::string path;
			std::vector<std::string> dir_queue;
			dir_queue.emplace_back(m_path + '/');

			{
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

					if (obj == elf_error::ok && ppu_load_exec(obj))
					{
						g_fxo->get<main_ppu_module>().path = path;
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
					GetCallbacks().on_stop(); // Call on_stop to refresh gui
					return game_boot_result::no_errors;
				}
			}

			if (auto& _main = g_fxo->get<main_ppu_module>(); _main.path.empty())
			{
				init_fxo_for_exec(nullptr, false);
			}

			g_fxo->init<named_thread>("SPRX Loader"sv, [this, dir_queue]() mutable
			{
				if (auto& _main = g_fxo->get<main_ppu_module>(); !_main.path.empty())
				{
					ppu_initialize(_main);
				}

				ppu_precompile(dir_queue, nullptr);

				// Exit "process"
				CallFromMainThread([this]
				{
					Emu.Kill(false);
					m_path = m_path_old; // Reset m_path to fix boot from gui
				});
			});

			return game_boot_result::no_errors;
		}

		// Detect boot location
		const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
		const bool from_hdd0_game   = IsPathInsideDir(m_path, hdd0_game);

		if (game_boot_result error = VerifyPathCasing(m_path, hdd0_game, from_hdd0_game); error != game_boot_result::no_errors)
		{
			return error;
		}

		if (game_boot_result error = VerifyPathCasing(m_path, g_cfg_vfs.get_dev_flash(), from_dev_flash); error != game_boot_result::no_errors)
		{
			return error;
		}

		// Mount /dev_bdvd/ if necessary
		if (bdvd_dir.empty() && disc.empty())
		{
			std::string sfb_dir;
			GetBdvdDir(bdvd_dir, sfb_dir, m_game_dir, elf_dir);

			if (!sfb_dir.empty() && from_hdd0_game)
			{
				// Booting disc game from wrong location
				sys_log.error("Disc game %s found at invalid location /dev_hdd0/game/", m_title_id);

				const std::string games_common = g_cfg_vfs.get(g_cfg_vfs.games_dir, rpcs3::utils::get_emu_dir());
				const std::string dst_dir = games_common + sfb_dir.substr(hdd0_game.size());

				// Move and retry from correct location
				if (fs::create_path(fs::get_parent_dir(dst_dir)) && fs::rename(sfb_dir, dst_dir, false))
				{
					sys_log.success("Disc game %s moved to special location '%s'", m_title_id, dst_dir);
					m_path = games_common + m_path.substr(hdd0_game.size());
					return Load(m_title_id);
				}

				sys_log.error("Failed to move disc game %s to '%s' (%s)", m_title_id, dst_dir, fs::g_tls_error);
				return game_boot_result::wrong_disc_location;
			}
		}

		if (bdvd_dir.empty() && disc.empty() && !is_disc_patch)
		{
			// Reset original disc game dir if this is neither disc nor disc patch
			m_game_dir = "PS3_GAME";
		}

		// Booting patch data
		if ((is_disc_patch || m_cat == "GD") && bdvd_dir.empty() && disc.empty())
		{
			// Load /dev_bdvd/ from game list if available
			if (std::string game_path = m_games_config.get_path(m_title_id); !game_path.empty())
			{
				bdvd_dir = std::move(game_path);
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
			vfs::mount("/dev_bdvd", bdvd_dir);
			sys_log.notice("Disc: %s", vfs::get("/dev_bdvd"));

			vfs::mount("/dev_bdvd/PS3_GAME", bdvd_dir + m_game_dir + "/");
			sys_log.notice("Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));

			if (const std::string sfb_path = vfs::get("/dev_bdvd/PS3_DISC.SFB"); !IsValidSfb(sfb_path))
			{
				sys_log.error("Invalid disc directory for the disc game %s. (%s)", m_title_id, sfb_path);
				return game_boot_result::invalid_file_or_folder;
			}

			const auto game_psf = psf::load_object(vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO"));
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
				if (m_games_config.add_game(m_title_id, bdvd_dir))
				{
					sys_log.notice("Registered BDVD game directory for title '%s': %s", m_title_id, bdvd_dir);
				}
				else
				{
					sys_log.error("Failed to save BDVD location of title '%s' (error=%s)", m_title_id, fs::g_tls_error);
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

			// TODO, this seems like it would normally be done by sysutil etc
			// Basically make 2 128KB memory cards 0 filled and let the games handle formatting.

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

			if (!m_title_id.empty() && !from_hdd0_game && m_cat == "HG")
			{
				std::string game_dir = m_sfo_dir;

				// Add HG games not in HDD0 to games.yml
				[[maybe_unused]] const bool res = m_games_config.add_external_hdd_game(m_title_id, game_dir);

				vfs::mount("/dev_hdd0/game/" + m_title_id, game_dir + '/');
			}
		}
		else if (m_cat == "DG" && from_hdd0_game && disc.empty())
		{
			// Disc game located in dev_hdd0/game
			bdvd_dir = g_cfg_vfs.get(g_cfg_vfs.dev_bdvd, rpcs3::utils::get_emu_dir());

			if (fs::get_dir_size(bdvd_dir))
			{
				sys_log.error("Failed to load disc game from dev_hdd0. The virtual bdvd_dir path does not exist or the directory is not empty: '%s'", bdvd_dir);
				return game_boot_result::invalid_bdvd_folder;
			}

			vfs::mount("/dev_bdvd", bdvd_dir);
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
			vfs::mount("/dev_bdvd/PS3_GAME", bdvd_dir + m_game_dir);
			sys_log.notice("Disk: %s, Dir: %s", vfs::get("/dev_bdvd"), m_game_dir);
		}

		// Initialize progress dialog
		g_fxo->init<named_thread<progress_dialog_server>>();

		// Initialize performance monitor
		g_fxo->init<named_thread<perf_monitor>>();

		// Set title to actual disc title if necessary
		const std::string disc_sfo_dir = vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO");

		const auto disc_psf_obj = psf::load_object(disc_sfo_dir);

		// Install PKGDIR, INSDIR, PS3_EXTRA
		if (!bdvd_dir.empty())
		{
			std::string ins_dir = vfs::get("/dev_bdvd/PS3_GAME/INSDIR/");
			std::string pkg_dir = vfs::get("/dev_bdvd/PS3_GAME/PKGDIR/");
			std::string extra_dir = vfs::get("/dev_bdvd/PS3_EXTRA/");
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

			std::vector<std::string> pkgs;

			if (!lock_file && !ins_dir.empty())
			{
				sys_log.notice("Found INSDIR: %s", ins_dir);

				for (auto&& entry : fs::dir{ins_dir})
				{
					const std::string pkg_file = ins_dir + entry.name;

					if (!entry.is_directory && entry.name.ends_with(".PKG"))
					{
						pkgs.push_back(pkg_file);
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

						if (fs::is_file(pkg_file))
						{
							pkgs.push_back(pkg_file);
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

						if (fs::is_file(pkg_file))
						{
							pkgs.push_back(pkg_file);
						}
					}
				}
			}

			if (!pkgs.empty())
			{
				bool install_success = true;
				BlockingCallFromMainThread([this, &pkgs, &install_success]()
				{
					if (!GetCallbacks().on_install_pkgs(pkgs))
					{
						install_success = false;
					}
				});
				if (!install_success)
				{
					sys_log.error("Failed to install packages");
					return game_boot_result::install_failed;
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
		if (const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN"; !m_ar
				&& recursion_count == 0 && disc.empty() && !bdvd_dir.empty() && !m_title_id.empty()
				&& resolved_path == GetCallbacks().resolve_path(vfs::get("/dev_bdvd/PS3_GAME/USRDIR/EBOOT.BIN"))
				&& resolved_path != GetCallbacks().resolve_path(hdd0_boot) && fs::is_file(hdd0_boot))
		{
			if (const psf::registry update_sfo = psf::load(hdd0_game + m_title_id + "/PARAM.SFO").sfo;
				psf::get_string(update_sfo, "TITLE_ID") == m_title_id && psf::get_string(update_sfo, "CATEGORY") == "GD")
			{
				// Booting game update
				sys_log.success("Updates found at /dev_hdd0/game/%s/", m_title_id);
				m_path = hdd0_boot;

				const game_boot_result boot_result = Load(m_title_id, true, recursion_count + 1);
				if (boot_result == game_boot_result::no_errors)
				{
					return game_boot_result::no_errors;
				}

				sys_log.error("Failed to boot update at \"%s\", game update may be corrupted! Consider uninstalling or reinstalling it. (reason: %s)", m_path, boot_result);
				return boot_result;
			}
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
				else if (!m_title_id.empty() && m_cat == "HG")
				{
					std::string game_dir = m_sfo_dir;

					// Remove the C00 suffix
					if (game_dir.ends_with("/C00") || game_dir.ends_with("\\C00"))
					{
						game_dir = game_dir.substr(0, game_dir.size() - 4);
					}

					m_dir = "/dev_hdd0/game/" + m_title_id + '/';
					argv[0] = m_dir + unescape(resolved_path.substr(GetCallbacks().resolve_path(game_dir).size()));
					sys_log.notice("Boot path: %s", m_dir);
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

			g_fxo->init<main_ppu_module>();

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
				// Preserve emulation state for OVL executable
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
			const auto& libs = g_cfg.core.libraries_control.get_set();

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

	bool signalled_thread = false;

	// Run main thread
	idm::select<named_thread<ppu_thread>>([&](u32, named_thread<ppu_thread>& cpu)
	{
		if (std::exchange(cpu.stop_flag_removal_protection, false))
		{
			return;
		}

		ensure(cpu.state.test_and_reset(cpu_flag::stop));
		cpu.state.notify_one(cpu_flag::stop);
		signalled_thread = true;
	});

	if (!signalled_thread)
	{
		FixGuestTime();
		FinalizeRunRequest();
	}

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

		CallFromMainThread([this]
		{
			// Mark a known savestate location and the one we try to boot (in case we boot a moved/copied savestate)
			if (g_cfg.savestate.suspend_emu)
			{
				for (std::string old_path : std::initializer_list<std::string>{m_ar ? m_path_old : "", m_title_id.empty() ? "" : get_savestate_path(m_title_id, m_path_old)})
				{
					if (old_path.empty())
					{
						continue;
					}

					std::string new_path = old_path.substr(0, old_path.find_last_not_of(fs::delim) + 1);
					const usz insert_pos = new_path.find_last_of(fs::delim) + 1;
					const auto prefix = "used_"sv;

					if (new_path.compare(insert_pos, prefix.size(), prefix) != 0)
					{
						new_path.insert(insert_pos, prefix);

						if (fs::rename(old_path, new_path, true))
						{
							sys_log.success("Savestate has been moved (hidden) to path='%s'", new_path);
						}
					}
				}
			}

			m_ar.reset();

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
	auto on_select = [](u32, spu_thread& spu)
	{
		if (spu.group && spu.index == spu.group->waiter_spu_index)
		{
			return;
		}

		if (std::exchange(spu.stop_flag_removal_protection, false))
		{
			return;
		}

		ensure(spu.state.test_and_reset(cpu_flag::stop));
		spu.state.notify_one(cpu_flag::stop);
	};

	if (m_savestate_extension_flags1 & SaveStateExtentionFlags1::ShouldCloseMenu)
	{
		g_fxo->get<SysutilMenuOpenStatus>().active = true;
	}

	idm::select<named_thread<spu_thread>>(on_select);

	lv2_obj::make_scheduler_ready();

	m_state.compare_and_swap_test(system_state::starting, system_state::running);

	if (m_savestate_extension_flags1 & SaveStateExtentionFlags1::ShouldCloseMenu)
	{
		std::thread([this, info = GetEmulationIdentifier()]()
		{
			std::this_thread::sleep_for(2s);

			CallFromMainThread([this]()
			{
				send_close_home_menu_cmds();
			}, info);
		}).detach();
	}
}

bool Emulator::Pause(bool freeze_emulation, bool show_resume_message)
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

		// Perform the side effects of Resume here when transforming paused to frozen state
		BlockingCallFromMainThread([this]()
		{
			for (auto& ref : m_pause_msgs_refs)
			{
				// Delete the message queued on pause
				*ref = 0;
			}

			m_pause_msgs_refs.clear();
		});
	}

	// Signal profilers to print results (if enabled)
	cpu_thread::flush_profilers();

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

	GetCallbacks().on_pause();

	BlockingCallFromMainThread([this, show_resume_message]()
	{
		const auto status = Emu.GetStatus(false);

		if (!show_resume_message || (status != system_state::paused && status != system_state::frozen))
		{
			return;
		}

		auto msg_ref = std::make_shared<atomic_t<u32>>(1);

		// No timeout
		rsx::overlays::queue_message(status == system_state::paused ? localized_string_id::EMULATION_PAUSED_RESUME_WITH_START : localized_string_id::EMULATION_FROZEN, -1, msg_ref);
		m_pause_msgs_refs.emplace_back(msg_ref);

		auto refresh_l = [this, msg_ref, status]()
		{
			while (*msg_ref && GetStatus(false) == status)
			{
				// Refresh Native UI
				rsx::set_native_ui_flip();
				thread_ctrl::wait_for(33'000);
			}

			msg_ref->release(0);
		};

		struct thread_t
		{
			std::unique_ptr<named_thread<decltype(refresh_l)>> m_thread;
		};

		g_fxo->get<thread_t>().m_thread.reset();
		g_fxo->get<thread_t>().m_thread = std::make_unique<named_thread<decltype(refresh_l)>>("Pause Message Thread"sv, std::move(refresh_l));
	});

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
	if (g_cfg.core.ppu_decoder == ppu_decoder_type::_static && g_cfg.core.ppu_debug)
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

		if (!dump.empty())
		{
			ppu_log.notice("[RESUME] Dumping instruction stats:%s", dump);
		}
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

	BlockingCallFromMainThread([this]()
	{
		for (auto& ref : m_pause_msgs_refs)
		{
			// Delete the message queued on pause
			*ref = 0;
		}

		m_pause_msgs_refs.clear();
	});

	if (g_cfg.misc.prevent_display_sleep)
	{
		disable_display_sleep();
	}
}

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

	auto perform_kill = [read_counter, allow_autoexit, this, info = GetEmulationIdentifier()]()
	{
		bool read_sysutil_signal = false;

		for (u32 i = 100; i < 140; i++)
		{
			std::this_thread::sleep_for(50ms);

			// TODO: Prevent pausing by other threads while in this loop
			CallFromMainThread([this]()
			{
				Resume();
			}, nullptr, true, read_counter);

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
extern bool try_lock_spu_threads_in_a_state_compatible_with_savestates(bool revert_lock = false);

std::shared_ptr<utils::serial> Emulator::Kill(bool allow_autoexit, bool savestate)
{
	std::shared_ptr<utils::serial> to_ar;

	if (savestate && !try_lock_spu_threads_in_a_state_compatible_with_savestates())
	{
		sys_log.error("Failed to savestate: failed to lock SPU threads execution.");
	}

	if (savestate && !try_lock_vdec_context_creation())
	{
		try_lock_spu_threads_in_a_state_compatible_with_savestates(true);

		sys_log.error("Failed to savestate: HLE VDEC (video decoder) context(s) exist."
			"\nLLE libvdec.sprx by selecting it in Adavcned tab -> Firmware Libraries."
			"\nYou need to close the game for to take effect."
			"\nIf you cannot close the game due to losing important progress your best chance is to skip the current cutscenes if any are played and retry.");

		return to_ar;
	}

	g_tls_log_prefix = []()
	{
		return std::string();
	};

	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		// Ensure clean state
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
		read_used_savestate_versions();
		m_savestate_extension_flags1 = {};
		return to_ar;
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

	named_thread stop_watchdog("Stop Watchdog", [&]()
	{
		for (int i = 0; thread_ctrl::state() != thread_state::aborting;)
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
		*thr = thread_state::finished;
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
		to_ar = std::make_unique<utils::serial>();

		// Savestate thread
		named_thread emu_state_cap_thread("Emu State Capture Thread", [&]()
		{
			g_tls_log_prefix = []()
			{
				return fmt::format("Emu State Capture Thread: '%s'", g_tls_serialize_name);
			};

			auto& ar = *to_ar;

			read_used_savestate_versions(); // Reset version data
			USING_SERIALIZATION_VERSION(global_version);

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
					save_tar(_path);
				}
			};

			auto save_hdd0 = [&]()
			{
				if (g_cfg.savestate.save_disc_game_data)
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
			ar(std::array<u8, 32>{}); // Reserved for future use
			ar(usz{0}); // Offset of versioning data, to be overwritten at the end of saving

			if (auto dir = vfs::get("/dev_bdvd/PS3_GAME"); fs::is_dir(dir) && !fs::is_file(fs::get_parent_dir(dir) + "/PS3_DISC.SFB"))
			{
				// Fake /dev_bdvd/PS3_GAME detected, use HDD0 for m_path restoration
				ensure(vfs::unmount("/dev_bdvd/PS3_GAME"));
				ar(vfs::retrieve(m_path));
				ar(vfs::retrieve(disc));
				ensure(vfs::mount("/dev_bdvd/PS3_GAME", dir));
			}
			else
			{
				ar(vfs::retrieve(m_path));
				ar(!m_title_id.empty() && !vfs::get("/dev_bdvd").empty() ? m_title_id : vfs::retrieve(disc));
			}

			ar(klic.empty() ? std::array<u8, 16>{} : std::bit_cast<std::array<u8, 16>>(klic[0]));
			ar(m_game_dir);
			save_hdd1();
			save_hdd0();
			ar(std::array<u8, 32>{}); // Reserved for future use
			vm::save(ar);
			g_fxo->save(ar);

			bs_t<SaveStateExtentionFlags1> extension_flags{SaveStateExtentionFlags1::SupportsMenuOpenResume};

			if (g_fxo->get<SysutilMenuOpenStatus>().active)
			{
				extension_flags += SaveStateExtentionFlags1::ShouldCloseMenu;
			}

			ar(extension_flags);

			ar(std::array<u8, 32>{}); // Reserved for future use
			ar(timestamp);
		});

		// Join it
		emu_state_cap_thread();

		if (emu_state_cap_thread == thread_state::errored)
		{
			sys_log.error("Saving savestate failed due to fatal error!");
			to_ar.reset();
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
		const std::string path = get_savestate_path(m_title_id, m_path);

		fs::pending_file file(path);

		// Identifer -> version
		std::vector<std::pair<u16, u16>> used_serial = read_used_savestate_versions();

		auto& ar = *to_ar;
		const usz pos = ar.seek_end();
		std::memcpy(&ar.data[10], &pos, 8);// Set offset
		ar(used_serial);

		if (!file.file || (file.file.write(ar.data), !file.commit()))
		{
			sys_log.error("Failed to write savestate to file! (path='%s', %s)", path, fs::g_tls_error);
			savestate = false;
		}
		else
		{
			std::string old_path = path.substr(0, path.find_last_not_of(fs::delim));
			std::string old_path2 = old_path;

			old_path2.insert(old_path.find_last_of(fs::delim) + 1, "old-"sv);
			old_path.insert(old_path.find_last_of(fs::delim) + 1, "used_"sv);

			if (fs::remove_file(old_path))
			{
				sys_log.success("Old savestate has been removed: path='%s'", old_path);
			}

			// For backwards compatibility - avoid having loose files
			if (fs::remove_file(old_path2))
			{
				sys_log.success("Old savestate has been removed: path='%s'", old_path2);
			}

			sys_log.success("Saved savestate! path='%s'", path);

			if (!g_cfg.savestate.suspend_emu)
			{
				// Allow to reboot from GUI
				m_path = path;
			}
		}

		ar.set_reading_state();
	}

	// Boot arg cleanup (preserved in the case restarting)
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();
	hdd1.clear();
	init_mem_containers = nullptr;
	m_config_path.clear();
	m_config_mode = cfg_mode::custom;
	m_ar.reset();
	read_used_savestate_versions();
	m_savestate_extension_flags1 = {};

	// Always Enable display sleep, not only if it was prevented.
	enable_display_sleep();

	if (allow_autoexit)
	{
		Quit(g_cfg.misc.autoexit.get());
	}

	return to_ar;
}

game_boot_result Emulator::Restart(bool graceful)
{
	if (!IsStopped())
	{
		auto save_args = std::make_tuple(argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_mode);

		if (graceful)
			GracefulShutdown(false, false);
		else
			Kill(false);

		std::tie(argv, envp, data, disc, klic, hdd1, m_config_mode, m_config_mode) = std::move(save_args);
	}

	// Reload with prior configs.
	if (const auto error = Load(m_title_id); error != game_boot_result::no_errors)
	{
		sys_log.error("Restart failed: %s", error);
		return error;
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

s32 error_code::error_report(s32 result, const logs::message* channel, const char* fmt, const fmt_type_info* sup, const u64* args)
{
	static thread_local std::string g_tls_error_str;
	static thread_local std::unordered_map<std::string, usz> g_tls_error_stats;

	if (!channel)
	{
		channel = &sys_log.error;
	}

	if (!sup && !args)
	{
		if (!fmt)
		{
			// Report and clean error state
			for (auto&& pair : g_tls_error_stats)
			{
				if (pair.second > 3)
				{
					channel->operator()("Stat: %s [x%u]", pair.first, pair.second);
				}
			}

			g_tls_error_stats.clear();
			return 0;
		}
	}

	ensure(fmt);

	const char* func = "Unknown function";

	if (auto ppu = get_current_cpu_thread<ppu_thread>())
	{
		if (auto current = ppu->current_function)
		{
			func = current;
		}
	}
	else if (auto spu = get_current_cpu_thread<spu_thread>())
	{
		if (auto current = spu->current_func; current && spu->start_time)
		{
			func = current;
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
		if (!g_tls_error_stats.empty())
		{
			// Report and clean error state
			error_report(0, nullptr, nullptr, nullptr, nullptr);
		}

		channel->operator()("%s", g_tls_error_str);
	}
	else
	{
		const auto stat = ++g_tls_error_stats[g_tls_error_str];

		if (stat <= 3)
		{
			channel->operator()("%s [%u]", g_tls_error_str, stat);
		}
	}

	return result;
}

void Emulator::ConfigurePPUCache() const
{
	auto& _main = g_fxo->get<main_ppu_module>();

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
				const std::string sfo_path = sfo_dir + "PARAM.SFO";
				const fs::file sfo_file(sfo_path);
				if (!sfo_file)
				{
					continue;
				}

				const psf::registry psf = psf::load_object(sfo_file, sfo_path);
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

void Emulator::AddGamesFromDir(const std::string& path)
{
	m_games_config.set_save_on_dirty(false);

	// search dropped path first or else the direct parent to an elf is wrongly skipped
	if (const auto error = AddGame(path); error == game_boot_result::no_errors)
	{
		// Nothing to do
	}

	process_qt_events();

	// search direct subdirectories, that way we can drop one folder containing all games
	for (auto&& dir_entry : fs::dir(path))
	{
		if (!dir_entry.is_directory || dir_entry.name == "." || dir_entry.name == "..")
		{
			continue;
		}

		const std::string dir_path = path + '/' + dir_entry.name;

		if (const auto error = AddGame(dir_path); error == game_boot_result::no_errors)
		{
			// Nothing to do
		}

		process_qt_events();
	}

	m_games_config.set_save_on_dirty(true);

	if (m_games_config.is_dirty() && !m_games_config.save())
	{
		sys_log.error("Failed to save games.yml after adding games");
	}
}

game_boot_result Emulator::AddGame(const std::string& path)
{
	// Handle files directly
	if (!fs::is_dir(path))
	{
		return AddGameToYml(path);
	}

	game_boot_result result = game_boot_result::nothing_to_boot;

	std::string elf;
	if (const game_boot_result res = GetElfPathFromDir(elf, path); res == game_boot_result::no_errors)
	{
		ensure(!elf.empty());
		result = AddGameToYml(elf);
	}

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
				if (const auto err = AddGameToYml(elf); err != game_boot_result::no_errors)
				{
					result = err;
				}
			}
		}
	}

	return result;
}

game_boot_result Emulator::AddGameToYml(const std::string& path)
{
	// Detect boot location
	const auto is_invalid_path = [this](std::string_view path, std::string_view dir) -> game_boot_result
	{
		if (IsPathInsideDir(path, dir))
		{
			sys_log.error("Adding games from dev_flash is not allowed.");
			return game_boot_result::invalid_file_or_folder;
		}

		return VerifyPathCasing(path, dir, false);
	};

	if (game_boot_result error = is_invalid_path(path, rpcs3::utils::get_hdd0_dir()); error != game_boot_result::no_errors)
	{
		sys_log.error("Adding games from dev_hdd0 is not allowed.");
		return error;
	}

	if (game_boot_result error = is_invalid_path(path, g_cfg_vfs.get_dev_flash()); error != game_boot_result::no_errors)
	{
		sys_log.error("Adding games from dev_flash is not allowed.");
		return error;
	}

	// Load PARAM.SFO
	const std::string elf_dir = fs::get_parent_dir(path);
	std::string sfo_dir = rpcs3::utils::get_sfo_dir_from_game_path(fs::get_parent_dir(elf_dir));
	const psf::registry _psf = psf::load_object(sfo_dir + "/PARAM.SFO");

	const std::string title_id = std::string(psf::get_string(_psf, "TITLE_ID"));
	const std::string cat = std::string(psf::get_string(_psf, "CATEGORY"));

	if (!_psf.empty() && cat.empty())
	{
		sys_log.fatal("Corrupted PARAM.SFO found! Try reinstalling the game.");
		return game_boot_result::invalid_file_or_folder;
	}

	if (title_id.empty())
	{
		sys_log.notice("Can not add binary without TITLE_ID to games.yml. (path=%s, category=%s)", path, cat);
		return game_boot_result::invalid_file_or_folder;
	}

	if (cat == "GD")
	{
		sys_log.notice("Can not add game data to games.yml. (path=%s, title_id=%s, category=%s)", path, title_id, cat);
		return game_boot_result::invalid_file_or_folder;
	}

	// Set bdvd_dir
	std::string bdvd_dir;
	std::string game_dir;
	std::string sfb_dir;
	GetBdvdDir(bdvd_dir, sfb_dir, game_dir, elf_dir);

	// Check /dev_bdvd/
	if (bdvd_dir.empty())
	{
		// Add HG games not in HDD0 to games.yml
		if (cat == "HG")
		{
			if (m_games_config.add_external_hdd_game(title_id, sfo_dir))
			{
				return game_boot_result::no_errors;
			}

			return game_boot_result::generic_error;
		}
	}
	else if (fs::is_dir(bdvd_dir))
	{
		if (const std::string sfb_path = bdvd_dir + "/PS3_DISC.SFB"; !IsValidSfb(sfb_path))
		{
			sys_log.error("Invalid disc directory for the disc game %s. (%s)", title_id, sfb_path);
			return game_boot_result::invalid_file_or_folder;
		}

		// Store /dev_bdvd/ location
		if (m_games_config.add_game(title_id, bdvd_dir))
		{
			sys_log.notice("Registered BDVD game directory for title '%s': %s", title_id, bdvd_dir);
			return game_boot_result::no_errors;
		}

		sys_log.error("Failed to save BDVD location of title '%s' (error=%s)", title_id, fs::g_tls_error);
		return game_boot_result::generic_error;
	}

	sys_log.notice("Nothing to add in path %s (title_id=%s, category=%s)", path, title_id, cat);
	return game_boot_result::invalid_file_or_folder;
}

bool Emulator::IsPathInsideDir(std::string_view path, std::string_view dir) const
{
	const std::string dir_path = GetCallbacks().resolve_path(dir);

	return !dir_path.empty() && (GetCallbacks().resolve_path(path) + '/').starts_with((dir_path.back() == '/') ? dir_path : (dir_path + '/'));
}

game_boot_result Emulator::VerifyPathCasing(
	[[maybe_unused]] std::string_view path,
	[[maybe_unused]] std::string_view dir,
	[[maybe_unused]] bool from_dir) const
{
#ifdef _WIN32
	// path might be passed from command line with differences in uppercase/lowercase on windows.
	if (!from_dir && IsPathInsideDir(fmt::to_lower(path), fmt::to_lower(dir)))
	{
		// Let's just abort to prevent errors down the line.
		sys_log.error("The path seems to contain incorrectly cased characters. Please adjust the path and try again.");
		return game_boot_result::invalid_file_or_folder;
	}
#endif
	return game_boot_result::no_errors;
}

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
}

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

		if (IsValidSfb(parent_dir + "/PS3_DISC.SFB"))
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
		const psf::registry _psf = psf::load_object(sfo_dir + "/PARAM.SFO");

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

bool Emulator::IsVsh()
{
	return g_ps3_process_info.self_info.valid && (g_ps3_process_info.self_info.prog_id_hdr.program_authority_id >> 36 == 0x1070000); // Not only VSH but also most CoreOS LV2 SELFs need the special treatment
}

bool Emulator::IsValidSfb(const std::string& path)
{
	fs::file sfb_file{path, fs::read + fs::isfile};
	return sfb_file && sfb_file.size() >= 4 && sfb_file.read<u32>() == ".SFB"_u32;
}

void Emulator::SaveSettings(const std::string& settings, const std::string& title_id)
{
	std::string config_name;

	if (title_id.empty())
	{
		config_name = fs::get_config_dir() + "/config.yml";
	}
	else
	{
		config_name = rpcs3::utils::get_custom_config_path(title_id);
	}

	// Save config atomically
	fs::pending_file temp(config_name);
	if (!temp.file)
	{
		sys_log.error("Could not save config to %s (failed to create temporary file) (error=%s)", config_name, fs::g_tls_error);
	}
	else
	{
		temp.file.write(settings.c_str(), settings.size());
		if (!temp.commit())
		{
			sys_log.error("Could not save config to %s (failed to commit) (error=%s)", config_name, fs::g_tls_error);
		}
	}

	// Check if the running config/title is the same as the edited config/title.
	if (config_name == g_cfg.name || title_id == Emu.GetTitleID())
	{
		// Update current config
		if (!g_cfg.from_string({settings.c_str(), settings.size()}, !Emu.IsStopped()))
		{
			sys_log.fatal("Failed to update configuration");
		}
		else if (!Emu.IsStopped()) // Don't spam the log while emulation is stopped. The config will be logged on boot anyway.
		{
			sys_log.notice("Updated configuration:\n%s\n", g_cfg.to_string());
		}
	}

	// Backup config
	g_backup_cfg.from_string(g_cfg.to_string());
}

Emulator Emu;
