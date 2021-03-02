#include "stdafx.h"
#include "VFS.h"
#include "Utilities/bin_patch.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"
#include "Emu/perf_meter.hpp"

#include "Emu/Cell/ErrorCodes.h"
#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/RSX/RSXThread.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_overlay.h"
#include "Emu/Cell/lv2/sys_rsx.h"
#include "Emu/Cell/Modules/cellMsgDialog.h"

#include "Emu/title.h"
#include "Emu/IdManager.h"
#include "Emu/RSX/Capture/rsx_replay.h"

#include "Loader/PSF.h"
#include "Loader/ELF.h"

#include "Utilities/StrUtil.h"

#include "../Crypto/unself.h"
#include "../Crypto/unpkg.h"
#include "util/sysinfo.hpp"
#include "util/yaml.hpp"
#include "util/logs.hpp"
#include "util/cereal.hpp"

#include <thread>
#include <queue>
#include <fstream>
#include <memory>
#include <regex>
#include <charconv>

#include "Utilities/JIT.h"

#include "display_sleep_control.h"

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VulkanAPI.h"
#endif

LOG_CHANNEL(sys_log, "SYS");

// Preallocate 32 MiB
stx::manual_typemap<void, 0x20'00000, 128> g_fixed_typemap;

bool g_use_rtm = false;
u64 g_rtm_tx_limit1 = 0;
u64 g_rtm_tx_limit2 = 0;

std::string g_cfg_defaults;

atomic_t<u64> g_watchdog_hold_ctr{0};

extern bool ppu_load_exec(const ppu_exec_object&);
extern void spu_load_exec(const spu_exec_object&);
extern void ppu_precompile(std::vector<std::string>& dir_queue, std::vector<lv2_prx*>* loaded_prx);
extern bool ppu_initialize(const ppu_module&, bool = false);
extern void ppu_finalize(const ppu_module&);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);
extern std::pair<std::shared_ptr<lv2_overlay>, CellError> ppu_load_overlay(const ppu_exec_object&, const std::string& path);

fs::file g_tty;
atomic_t<s64> g_tty_size{0};
std::array<std::deque<std::string>, 16> g_tty_input;
std::mutex g_tty_mutex;

// Progress display server synchronization variables
atomic_t<const char*> g_progr{""};
atomic_t<u32> g_progr_ftotal{0};
atomic_t<u32> g_progr_fdone{0};
atomic_t<u32> g_progr_ptotal{0};
atomic_t<u32> g_progr_pdone{0};

// Report error and call std::abort(), defined in main.cpp
[[noreturn]] void report_fatal_error(const std::string&);

namespace
{
	struct progress_dialog_server;
}

namespace atomic_wait
{
	extern void parse_hashtable(bool(*cb)(u64 id, u32 refs, u64 ptr, u32 stats));
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
		}
		return unknown;
	});
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

	idm::init();
	g_fxo->reset();
	g_fxo->need<named_thread<progress_dialog_server>>();

	// Reset defaults, cache them
	g_cfg.from_default();

	// Not all renderers are known at compile time, so set a provided default if possible
	if (m_default_renderer == video_renderer::vulkan && !m_default_graphics_adapter.empty())
	{
		g_cfg.video.renderer.set(m_default_renderer);
		g_cfg.video.vk.adapter.from_string(m_default_graphics_adapter);
	}

	g_cfg_defaults = g_cfg.to_string();

	// Reload override configuration set via command line
	if (!m_config_override_path.empty())
	{
		if (const fs::file cfg_file{m_config_override_path, fs::read + fs::create})
		{
			if (!g_cfg.from_string(cfg_file.to_string()))
			{
				sys_log.fatal("Failed to apply config override: %s. Proceeding with regular configuration.", m_config_override_path);
				m_config_override_path.clear();
			}
			else
			{
				sys_log.success("Applied config override: %s", m_config_override_path);
				g_cfg.name = m_config_override_path;
			}
		}
		else
		{
			sys_log.fatal("Failed to access config override: %s (%s). Proceeding with regular configuration.", m_config_override_path, fs::g_tls_error);
			m_config_override_path.clear();
		}
	}

	// Reload global configuration
	if (m_config_override_path.empty())
	{
		const auto cfg_path = fs::get_config_dir() + "/config.yml";

		if (const fs::file cfg_file{cfg_path, fs::read + fs::create})
		{
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
	const std::string emu_dir = GetEmuDir();
	const std::string dev_hdd0 = GetHddDir();
	const std::string dev_hdd1 = fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir);
	const std::string dev_usb = fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir);
	const std::string dev_flsh = g_cfg.vfs.get_dev_flash();

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
		make_path_verbose(dev_usb);
		make_path_verbose(dev_hdd0 + "game/");
		make_path_verbose(dev_hdd0 + "game/TEST12345/");
		make_path_verbose(dev_hdd0 + "game/TEST12345/USRDIR/");
		make_path_verbose(dev_hdd0 + "game/.locks/");
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
	make_path_verbose(fs::get_config_dir() + "captures/");

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
				else
				{
					sys_log.success("Fixed save data: %s", desired);
				}
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
		LimitCacheSize();

	// Initialize patch engine
	g_fxo->init<patch_engine>()->append_global_patches();
}

namespace
{
	struct progress_dialog_server
	{
		void operator()()
		{
			while (thread_ctrl::state() != thread_state::aborting)
			{
				// Wait for the start condition
				while (!g_progr_ftotal && !g_progr_ptotal)
				{
					if (thread_ctrl::state() == thread_state::aborting)
					{
						break;
					}

					std::this_thread::sleep_for(5ms);
				}

				if (thread_ctrl::state() == thread_state::aborting)
				{
					break;
				}

				// Initialize message dialog
				std::shared_ptr<MsgDialogBase> dlg = Emu.GetCallbacks().get_msg_dialog();
				if (dlg)
				{
					dlg->type.se_normal = true;
					dlg->type.bg_invisible = true;
					dlg->type.progress_bar_count = 1;
					dlg->on_close = [](s32 status)
					{
						Emu.CallAfter([]()
						{
							// Abort everything
							Emu.Stop();
						});
					};

					Emu.CallAfter([dlg]()
					{
						dlg->Create(+g_progr, +g_progr);
					});
				}

				u32 ftotal = 0;
				u32 fdone = 0;
				u32 ptotal = 0;
				u32 pdone = 0;

				// Update progress
				while (thread_ctrl::state() != thread_state::aborting)
				{
					const u32 ftotal_new = g_progr_ftotal;
					const u32 fdone_new  = g_progr_fdone;
					const u32 ptotal_new = g_progr_ptotal;
					const u32 pdone_new  = g_progr_pdone;

					if (ftotal != ftotal_new || fdone != fdone_new || ptotal != ptotal_new || pdone != pdone_new)
					{
						ftotal = ftotal_new;
						fdone  = fdone_new;
						ptotal = ptotal_new;
						pdone  = pdone_new;

						// Compute new progress in percents
						// Assume not all programs were found if files were not compiled (as it may contain more)
						const u64 total = std::max<u64>(ptotal, 1) * std::max<u64>(ftotal, 1);
						const u64 done = pdone * std::max<u64>(fdone, 1);
						const double value = std::fmin(done * 100. / total, 100.);

						// Changes detected, send update
						Emu.CallAfter([=]()
						{
							std::string progr = "Progress:";

							if (ftotal)
								fmt::append(progr, " file %u of %u%s", fdone, ftotal, ptotal ? "," : "");
							if (ptotal)
								fmt::append(progr, " module %u of %u", pdone, ptotal);

							if (dlg)
							{
								dlg->SetMsg(+g_progr);
								dlg->ProgressBarSetMsg(0, progr);
								dlg->ProgressBarSetValue(0, std::floor(value));
							}
						});
					}

					if (fdone >= ftotal && pdone >= ptotal)
					{
						// Close dialog
						break;
					}

					std::this_thread::sleep_for(10ms);
				}

				if (thread_ctrl::state() == thread_state::aborting)
				{
					break;
				}

				// Cleanup
				g_progr_ftotal -= ftotal;
				g_progr_fdone  -= fdone;
				g_progr_ptotal -= ptotal;
				g_progr_pdone  -= pdone;

				if (dlg)
				{
					Emu.CallAfter([=]()
					{
						dlg->Close(true);
					});
				}
			}
		}

		~progress_dialog_server()
		{
			g_progr_ftotal.release(0);
			g_progr_fdone.release(0);
			g_progr_ptotal.release(0);
			g_progr_pdone.release(0);
		}

		static auto constexpr thread_name = "Progress Dialog Server"sv;
	};
}

const bool Emulator::SetUsr(const std::string& user)
{
	if (user.empty())
	{
		return false;
	}

	u32 id = 0;
	std::from_chars(&user.front(), &user.back() + 1, id);

	if (id == 0)
	{
		return false;
	}

	m_usrid = id;
	m_usr = user;
	return true;
}

const std::string Emulator::GetBackgroundPicturePath() const
{
	// Try to find a custom icon first
	std::string path = fs::get_config_dir() + "/Icons/game_icons/" + Emu.GetTitleID() + "/PIC1.PNG";

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

std::string Emulator::PPUCache() const
{
	auto& _main = g_fxo->get<ppu_module>();

	if (!g_fxo->is_init<ppu_module>() || _main.cache.empty())
	{
		ppu_log.warning("PPU Cache location not initialized.");
		return {};
	}

	return _main.cache;
}

bool Emulator::BootRsxCapture(const std::string& path)
{
	fs::file in_file(path);

	if (!in_file)
	{
		return false;
	}

	std::unique_ptr<rsx::frame_capture_data> frame = std::make_unique<rsx::frame_capture_data>();
	cereal_deserialize(*frame, in_file.to_string());
	in_file.close();

	if (frame->magic != rsx::FRAME_CAPTURE_MAGIC)
	{
		sys_log.error("Invalid rsx capture file!");
		return false;
	}

	if (frame->version != rsx::FRAME_CAPTURE_VERSION)
	{
		sys_log.error("Rsx capture file version not supported! Expected %d, found %d", rsx::FRAME_CAPTURE_VERSION, frame->version);
		return false;
	}

	Init();
	g_cfg.video.disable_on_disk_shader_cache.set(true);

	vm::init();
	g_fxo->init(false);

	// PS3 'executable'
	m_state = system_state::ready;
	GetCallbacks().on_ready();

	Emu.GetCallbacks().init_gs_render();
	Emu.GetCallbacks().init_pad_handler("");

	GetCallbacks().on_run(false);
	m_state = system_state::running;

	auto replay_thr = g_fxo->init<named_thread<rsx::rsx_replay_thread>>("RSX Replay"sv, std::move(frame));
	replay_thr->state -= cpu_flag::stop;
	replay_thr->state.notify_one(cpu_flag::stop);

	return true;
}

void Emulator::LimitCacheSize()
{
	const std::string cache_location = Emulator::GetHdd1Dir() + "/caches";

	if (!fs::is_dir(cache_location))
	{
		sys_log.warning("Cache does not exist (%s)", cache_location);
		return;
	}

	const u64 size = fs::get_dir_size(cache_location);

	if (size == umax)
	{
		sys_log.error("Could not calculate cache directory '%s' size (%s)", cache_location, fs::g_tls_error);
		return;
	}

	const u64 max_size = static_cast<u64>(g_cfg.vfs.cache_max_size) * 1024 * 1024;

	if (max_size == 0) // Everything must go, so no need to do checks
	{
		fs::remove_all(cache_location, false);
		sys_log.success("Cleared disk cache");
		return;
	}

	if (size <= max_size)
	{
		sys_log.trace("Cache size below limit: %llu/%llu", size, max_size);
		return;
	}

	sys_log.success("Cleaning disk cache...");
	std::vector<fs::dir_entry> file_list{};
	fs::dir cache_dir(cache_location);
	if (!cache_dir)
	{
		sys_log.error("Could not open cache directory '%s' (%s)", cache_location, fs::g_tls_error);
		return;
	}

	// retrieve items to delete
	for (const auto &item : cache_dir)
	{
		if (item.name != "." && item.name != "..")
			file_list.push_back(item);
	}

	cache_dir.close();

	// sort oldest first
	std::sort(file_list.begin(), file_list.end(), [](auto left, auto right)
	{
		return left.mtime < right.mtime;
	});

	// keep removing until cache is empty or enough bytes have been cleared
	// cache is cleared down to 80% of limit to increase interval between clears
	const u64 to_remove = static_cast<u64>(size - max_size * 0.8);
	u64 removed = 0;
	for (const auto &item : file_list)
	{
		const std::string &name = cache_location + "/" + item.name;
		const bool is_dir = fs::is_dir(name);
		const u64 item_size = is_dir ? fs::get_dir_size(name) : item.size;

		if (is_dir && item_size == umax)
		{
			sys_log.error("Failed to calculate '%s' item '%s' size (%s)", cache_location, item.name, fs::g_tls_error);
			break;
		}

		if (is_dir ? !fs::remove_all(name, true) : !fs::remove_file(name))
		{
			sys_log.error("Could not remove cache directory '%s' item '%s' (%s)", cache_location, item.name, fs::g_tls_error);
			break;
		}

		removed += item_size;
		if (removed >= to_remove)
			break;
	}

	sys_log.success("Cleaned disk cache, removed %.2f MB", size / 1024.0 / 1024.0);
}

game_boot_result Emulator::BootGame(const std::string& path, const std::string& title_id, bool direct, bool add_only, bool force_global_config)
{
	if (!fs::exists(path))
	{
		return game_boot_result::invalid_file_or_folder;
	}

	m_path_old = m_path;

	if (direct || fs::is_file(path))
	{
		m_path = path;
		return Load(title_id, add_only, force_global_config);
	}

	game_boot_result result = game_boot_result::nothing_to_boot;

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
			m_path = elf;
			result = Load(title_id, add_only, force_global_config);
			break;
		}
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
					if (const auto err = Load(title_id, add_only, force_global_config); err != game_boot_result::no_errors)
					{
						result = err;
					}
				}
			}
		}
	}
	return result;
}

bool Emulator::InstallPkg(const std::string& path)
{
	sys_log.success("Installing package: %s", path);

	atomic_t<double> progress(0.);
	int int_progress = 0;

	// Run PKG unpacking asynchronously
	named_thread worker("PKG Installer", [&]
	{
		package_reader reader(path);
		return reader.extract_data(progress);
	});

	{
		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker <= thread_state::aborting)
		{
			// TODO: update unified progress dialog
			double pval = progress;
			pval < 0 ? pval += 1. : pval;
			pval *= 100.;

			if (static_cast<int>(pval) > int_progress)
			{
				int_progress = static_cast<int>(pval);
				sys_log.success("... %u%%", int_progress);
			}
		}
	}

	return worker();
}

std::string Emulator::GetEmuDir()
{
	const std::string& emu_dir_ = g_cfg.vfs.emulator_dir;
	return emu_dir_.empty() ? fs::get_config_dir() : emu_dir_;
}

std::string Emulator::GetHddDir()
{
	return fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", GetEmuDir());
}

std::string Emulator::GetHdd1Dir()
{
	return fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", GetEmuDir());
}

std::string Emulator::GetCacheDir()
{
	return fs::get_cache_dir() + "cache/";
}

#ifdef _WIN32
std::string Emulator::GetExeDir()
{
	wchar_t buffer[32767];
	GetModuleFileNameW(nullptr, buffer, sizeof(buffer)/2);

	std::string path_to_exe = wchar_to_utf8(buffer);
	usz last = path_to_exe.find_last_of("\\");
	return last == std::string::npos ? std::string("") : path_to_exe.substr(0, last+1);
}
#endif

std::string Emulator::GetSfoDirFromGamePath(const std::string& game_path, const std::string& user, const std::string& title_id)
{
	if (fs::is_file(game_path + "/PS3_DISC.SFB"))
	{
		// This is a disc game.
		if (!title_id.empty())
		{
			for (auto&& entry : fs::dir{game_path})
			{
				if (entry.name == "." || entry.name == "..")
				{
					continue;
				}

				const std::string sfo_path = game_path + "/" + entry.name + "/PARAM.SFO";

				if (entry.is_directory && fs::is_file(sfo_path))
				{
					const auto psf = psf::load_object(fs::file(sfo_path));
					const auto serial = psf::get_string(psf, "TITLE_ID");
					if (serial == title_id)
					{
						return game_path + "/" + entry.name;
					}
				}
			}
		}

		return game_path + "/PS3_GAME";
	}

	const auto psf = psf::load_object(fs::file(game_path + "/PARAM.SFO"));

	const auto category = psf::get_string(psf, "CATEGORY");
	const auto content_id = std::string(psf::get_string(psf, "CONTENT_ID"));

	if (category == "HG" && !content_id.empty())
	{
		// This is a trial game. Check if the user has a RAP file to unlock it.
		const std::string rap_path = GetHddDir() + "home/" + user + "/exdata/" + content_id + ".rap";
		if (fs::is_file(rap_path) && fs::is_file(game_path + "/C00/PARAM.SFO"))
		{
			// Load full game data.
			return game_path + "/C00";
		}
	}

	return game_path;
}

std::string Emulator::GetCustomConfigDir()
{
#ifdef _WIN32
	return fs::get_config_dir() + "config/custom_configs/";
#else
	return fs::get_config_dir() + "custom_configs/";
#endif
}

std::string Emulator::GetCustomConfigPath(const std::string& title_id, bool get_deprecated_path)
{
	std::string path;

	if (get_deprecated_path)
		path = fs::get_config_dir() + "data/" + title_id + "/config.yml";
	else
		path = GetCustomConfigDir() + "config_" + title_id + ".yml";

	return path;
}

std::string Emulator::GetCustomInputConfigDir(const std::string& title_id)
{
	// Notice: the extra folder for each title id may be removed at a later stage
	// Warning: make sure to change any function that removes this directory as well
#ifdef _WIN32
	return fs::get_config_dir() + "config/custom_input_configs/" + title_id + "/";
#else
	return fs::get_config_dir() + "custom_input_configs/" + title_id + "/";
#endif
}

std::string Emulator::GetCustomInputConfigPath(const std::string& title_id)
{
	return GetCustomInputConfigDir(title_id) + "/config_input_" + title_id + ".yml";
}

void Emulator::SetForceBoot(bool force_boot)
{
	m_force_boot = force_boot;
}

game_boot_result Emulator::Load(const std::string& title_id, bool add_only, bool force_global_config, bool is_disc_patch)
{
	m_force_global_config = force_global_config;

	if (!IsStopped())
	{
		Stop();
	}

	if (!title_id.empty())
	{
		m_title_id = title_id;
	}

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

		sys_log.notice("Path: %s", m_path);

		const std::string elf_dir = fs::get_parent_dir(m_path);

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
				m_sfo_dir = GetSfoDirFromGamePath(m_path, GetUsr(), m_title_id);
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
					m_sfo_dir = GetHddDir() + "game/" + m_title_id;
				}
				else
				{
					m_sfo_dir = GetSfoDirFromGamePath(disc, GetUsr(), m_title_id);
				}
			}
			else
			{
				m_sfo_dir = GetSfoDirFromGamePath(elf_dir + "/../", GetUsr(), m_title_id);
			}

			_psf = psf::load_object(fs::file(m_sfo_dir + "/PARAM.SFO"));
		}

		m_title = std::string(psf::get_string(_psf, "TITLE", std::string_view(m_path).substr(m_path.find_last_of('/') + 1)));
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

		if (!add_only && !force_global_config && m_config_override_path.empty())
		{
			const std::string config_path_new = GetCustomConfigPath(m_title_id);
			const std::string config_path_old = GetCustomConfigPath(m_title_id, true);

			// Load custom config-1
			if (fs::file cfg_file{ config_path_old })
			{
				sys_log.notice("Applying custom config: %s", config_path_old);

				if (!g_cfg.from_string(cfg_file.to_string()))
				{
					sys_log.fatal("Failed to apply custom config: %s", config_path_old);
				}
			}

			// Load custom config-2
			if (fs::file cfg_file{ config_path_new })
			{
				sys_log.notice("Applying custom config: %s", config_path_new);

				if (g_cfg.from_string(cfg_file.to_string()))
				{
					g_cfg.name = config_path_new;
				}
				else
				{
					sys_log.fatal("Failed to apply custom config: %s", config_path_new);
				}
			}

			// Load custom config-3
			if (fs::file cfg_file{ m_path + ".yml" })
			{
				sys_log.notice("Applying custom config: %s.yml", m_path);

				if (!g_cfg.from_string(cfg_file.to_string()))
				{
					sys_log.fatal("Failed to apply custom config: %s.yml", m_path);
				}
			}
		}

		// Set RTM usage
		g_use_rtm = utils::has_rtm() && ((utils::has_mpx() && g_cfg.core.enable_TSX == tsx_usage::enabled) || g_cfg.core.enable_TSX == tsx_usage::forced);

		// Log some extra info in case of boot
		if (!add_only)
		{
#if defined(_WIN32) || defined(HAVE_VULKAN)
			if (g_cfg.video.renderer == video_renderer::vulkan)
			{
				sys_log.notice("Vulkan SDK Revision: %d", VK_HEADER_VERSION);
			}
#endif

			sys_log.notice("Used configuration:\n%s\n", g_cfg.to_string());

			if (g_use_rtm && !utils::has_mpx())
			{
				sys_log.warning("TSX forced by User");
			}
		}

		if (g_use_rtm)
		{
			// Update supplementary settings
			const f64 _1ns = utils::get_tsc_freq() / 1000'000'000.;
			g_rtm_tx_limit1 = g_cfg.core.tx_limit1_ns * _1ns;
			g_rtm_tx_limit2 = g_cfg.core.tx_limit2_ns * _1ns;
		}

		// Load patches from different locations
		g_fxo->get<patch_engine>().append_title_patches(m_title_id);

		// Mount all devices
		const std::string emu_dir = GetEmuDir();
		const std::string home_dir = g_cfg.vfs.app_home;
		std::string bdvd_dir = g_cfg.vfs.dev_bdvd;

		// Mount default relative path to non-existent directory
		vfs::mount("/dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));
		vfs::mount("/dev_flash", g_cfg.vfs.get_dev_flash());
		vfs::mount("/dev_usb", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("/dev_usb000", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("/app_home", home_dir.empty() ? elf_dir + '/' : fmt::replace_all(home_dir, "$(EmulatorDir)", emu_dir));

		if (!hdd1.empty())
		{
			vfs::mount("/dev_hdd1", hdd1);
			sys_log.notice("Hdd1: %s", vfs::get("/dev_hdd1"));
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
				std::vector<std::string> dir_queue;
				dir_queue.emplace_back(m_path + '/');

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

				// Try to add all related directories
				const std::set<std::string> dirs = GetGameDirs();
				dir_queue.insert(std::end(dir_queue), std::begin(dirs), std::end(dirs));

				if (std::string path = m_path + "/USRDIR/EBOOT.BIN"; fs::is_file(path))
				{
					// Compile EBOOT.BIN first
					ppu_log.notice("Trying to load EBOOT.BIN: %s", path);

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
						sys_log.error("Failed to load EBOOT.BIN '%s' (%s)", path, obj.get_error());
					}
				}
				else
				{
					// Workaround for analyser glitches
					ensure(vm::falloc(0x10000, 0xf0000, vm::main));
				}

				if (Emu.IsStopped())
				{
					return;
				}

				ppu_precompile(dir_queue, nullptr);

				// Exit "process"
				Emu.CallAfter([]
				{
					Emu.Stop();
				});
			});

			return game_boot_result::no_errors;
		}

		// Detect boot location
		constexpr usz game_dir_size = 8; // size of PS3_GAME and PS3_GMXX
		const std::string hdd0_game    = vfs::get("/dev_hdd0/game/");
		const std::string hdd0_disc    = vfs::get("/dev_hdd0/disc/");
		const bool from_hdd0_game      = m_path.find(hdd0_game) != umax;

		// Mount /dev_bdvd/ if necessary
		if (bdvd_dir.empty() && disc.empty())
		{
			if (const usz usrdir_pos = elf_dir.rfind("/USRDIR"); usrdir_pos != umax)
			{
				const std::string main_dir = elf_dir.substr(0, usrdir_pos);

				if (const std::string sfb_dir = fs::get_parent_dir(main_dir);
					!sfb_dir.empty() && fs::is_file(sfb_dir + "/PS3_DISC.SFB"))
				{
					if (from_hdd0_game)
					{
						// Booting disc game from wrong location
						sys_log.error("Disc game %s found at invalid location /dev_hdd0/game/", m_title_id);

						// Move and retry from correct location
						if (fs::rename(elf_dir + "/../../", hdd0_disc + elf_dir.substr(hdd0_game.size()) + "/../../", false))
						{
							sys_log.success("Disc game %s moved to special location /dev_hdd0/disc/", m_title_id);
							return m_path = hdd0_disc + m_path.substr(hdd0_game.size()), Load(m_title_id, add_only, force_global_config);
						}
						else
						{
							sys_log.error("Failed to move disc game %s to /dev_hdd0/disc/ (%s)", m_title_id, fs::g_tls_error);
							return game_boot_result::wrong_disc_location;
						}
					}

					bdvd_dir = sfb_dir + "/";

					// Find game dir
					if (const std::string main_dir_name = main_dir.substr(main_dir.find_last_of("/\\") + 1);
						main_dir_name.size() == game_dir_size)
					{
						m_game_dir = main_dir_name;
					}
					else
					{
						// Show an error and reset game dir for now
						m_game_dir = "PS3_GAME";
						sys_log.error("Could not find proper game dir path for %s", elf_dir);
						return game_boot_result::unsupported_disc_type;
					}
				}
			}
		}

		if (bdvd_dir.empty() && !is_disc_patch)
		{
			// Reset original disc game dir if this is neither disc nor disc patch
			m_game_dir = "PS3_GAME";
		}

		// Booting patch data
		if (m_cat == "GD" && bdvd_dir.empty() && disc.empty())
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

			if (bdvd_title_id != m_title_id)
			{
				sys_log.error("Unexpected disc directory for the disc game %s (found %s)", m_title_id, bdvd_title_id);
				return game_boot_result::invalid_file_or_folder;
			}

			// Store /dev_bdvd/ location
			games[m_title_id] = bdvd_dir;
			YAML::Emitter out;
			out << games;

			fs::pending_file temp(fs::get_config_dir() + "/games.yml");
			temp.file.write(out.c_str(), out.size());
			temp.commit();
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
		else if (m_cat == "DG" && from_hdd0_game)
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

		// Install PKGDIR, INSDIR, PS3_EXTRA
		if (!bdvd_dir.empty())
		{
			const std::string ins_dir = vfs::get("/dev_bdvd/PS3_GAME/INSDIR/");
			const std::string pkg_dir = vfs::get("/dev_bdvd/PS3_GAME/PKGDIR/");
			const std::string extra_dir = vfs::get("/dev_bdvd/PS3_GAME/PS3_EXTRA/");
			fs::file lock_file;

			if (fs::is_dir(ins_dir) || fs::is_dir(pkg_dir) || fs::is_dir(extra_dir))
			{
				// Create lock file to prevent double installation
				lock_file.open(hdd0_game + ".locks/" + m_title_id, fs::read + fs::create + fs::excl);
			}

			if (lock_file && fs::is_dir(ins_dir))
			{
				sys_log.notice("Found INSDIR: %s", ins_dir);

				for (auto&& entry : fs::dir{ins_dir})
				{
					const std::string pkg = ins_dir + entry.name;
					if (!entry.is_directory && entry.name.ends_with(".PKG") && !InstallPkg(pkg))
					{
						sys_log.error("Failed to install %s", pkg);
						return game_boot_result::install_failed;
					}
				}
			}

			if (lock_file && fs::is_dir(pkg_dir))
			{
				sys_log.notice("Found PKGDIR: %s", pkg_dir);

				for (auto&& entry : fs::dir{pkg_dir})
				{
					if (entry.is_directory && entry.name.starts_with("PKG"))
					{
						const std::string pkg_file = pkg_dir + entry.name + "/INSTALL.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							sys_log.error("Failed to install %s", pkg_file);
							return game_boot_result::install_failed;
						}
					}
				}
			}

			if (lock_file && fs::is_dir(extra_dir))
			{
				sys_log.notice("Found PS3_EXTRA: %s", extra_dir);

				for (auto&& entry : fs::dir{extra_dir})
				{
					if (entry.is_directory && entry.name[0] == 'D')
					{
						const std::string pkg_file = extra_dir + entry.name + "/DATA000.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							sys_log.error("Failed to install %s", pkg_file);
							return game_boot_result::install_failed;
						}
					}
				}
			}
		}

		// Check game updates
		const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN";

		if (disc.empty() && !bdvd_dir.empty() && m_path != hdd0_boot && fs::is_file(hdd0_boot))
		{
			// Booting game update
			sys_log.success("Updates found at /dev_hdd0/game/%s/", m_title_id);
			return m_path = hdd0_boot, Load(m_title_id, false, force_global_config, true);
		}

		// Set title to actual disc title if necessary
		const std::string disc_sfo_dir = vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO");

		if (!disc_sfo_dir.empty() && fs::is_file(disc_sfo_dir))
		{
			const auto psf_obj = psf::load_object(fs::file{ disc_sfo_dir });
			const auto bdvd_title = psf::get_string(psf_obj, "TITLE");

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

		fs::file elf_file(elf_path);

		if (!elf_file)
		{
			sys_log.error("Failed to open executable: %s", elf_path);
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

		ppu_exec_object ppu_exec;
		ppu_prx_object ppu_prx;
		spu_exec_object spu_exec;

		if (ppu_exec.open(elf_file) == elf_error::ok)
		{
			// PS3 executable
			m_state = system_state::ready;
			GetCallbacks().on_ready();

			vm::init();

			if (argv.empty())
			{
				argv.resize(1);
			}

			if (argv[0].empty())
			{
				if (from_hdd0_game && m_cat == "DG")
				{
					argv[0] = "/dev_bdvd/PS3_GAME/" + m_path.substr(hdd0_game.size() + 10);
					m_dir = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 10);
					sys_log.notice("Disc path: %s", m_dir);
				}
				else if (from_hdd0_game)
				{
					argv[0] = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size());
					m_dir = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 10);
					sys_log.notice("Boot path: %s", m_dir);
				}
				else if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
				{
					// Disc games are on /dev_bdvd/
					const usz pos = m_path.rfind(m_game_dir);
					argv[0] = "/dev_bdvd/PS3_GAME/" + m_path.substr(pos + game_dir_size + 1);
					m_dir = "/dev_bdvd/PS3_GAME/";
				}
				else
				{
					// For homebrew
					argv[0] = "/host_root/" + m_path;
					m_dir = "/host_root/" + elf_dir + '/';
				}

				sys_log.notice("Elf path: %s", argv[0]);
			}

			g_fxo->init<ppu_module>();

			if (ppu_load_exec(ppu_exec))
			{
				ConfigurePPUCache();

				g_fxo->init(false);
				Emu.GetCallbacks().init_gs_render();
				Emu.GetCallbacks().init_pad_handler(m_title_id);
				Emu.GetCallbacks().init_kb_handler();
				Emu.GetCallbacks().init_mouse_handler();
			}
			// Overlay (OVL) executable (only load it)
			else if (vm::map(0x3000'0000, 0x1000'0000, 0x200); !ppu_load_overlay(ppu_exec, m_path).first)
			{
				ppu_exec.set_error(elf_error::header_type);
			}

			if (ppu_exec != elf_error::ok)
			{
				Stop();

				sys_log.error("Invalid or unsupported PPU executable format: %s", elf_path);

				return game_boot_result::invalid_file_or_folder;
			}
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init(false);
			ppu_load_prx(ppu_prx, m_path);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init(false);
			spu_load_exec(spu_exec);
		}
		else
		{
			sys_log.error("Invalid or unsupported file format: %s", elf_path);

			sys_log.warning("** ppu_exec -> %s", ppu_exec.get_error());
			sys_log.warning("** ppu_prx  -> %s", ppu_prx.get_error());
			sys_log.warning("** spu_exec -> %s", spu_exec.get_error());
			return game_boot_result::invalid_file_or_folder;
		}

		if ((m_force_boot || g_cfg.misc.autostart) && IsReady())
		{
			if (ppu_exec == elf_error::ok)
			{
				if (!fs::is_file(g_cfg.vfs.get_dev_flash() + "sys/external/liblv2.sprx"))
				{
					if (!GetCallbacks().on_missing_fw())
					{
						Stop();
						return game_boot_result::firmware_missing;
					}
				}

				Run(true);
			}

			m_force_boot = false;
		}
		else if (IsPaused())
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
		}
		return game_boot_result::no_errors;
	}
}

void Emulator::Run(bool start_playtime)
{
	if (!IsReady())
	{
		// Reload with prior configuration.
		Load(m_title_id, false, m_force_global_config);

		if (!IsReady())
		{
			return;
		}
	}

	if (IsRunning())
	{
		Stop();
	}

	if (IsPaused())
	{
		Resume();
		return;
	}

	GetCallbacks().on_run(start_playtime);

	m_pause_start_time = 0;
	m_pause_amend_time = 0;
	m_state = system_state::running;

	ConfigureLogs();

	// Run main thread
	idm::check<named_thread<ppu_thread>>(ppu_thread::id_base, [](named_thread<ppu_thread>& cpu)
	{
		ensure(cpu.state.test_and_reset(cpu_flag::stop));
		cpu.state.notify_one(cpu_flag::stop);
	});

	if (auto thr = g_fxo->try_get<named_thread<rsx::rsx_replay_thread>>())
	{
		thr->state -= cpu_flag::stop;
		thr->state.notify_one(cpu_flag::stop);
	}

	if (g_cfg.misc.prevent_display_sleep)
	{
		disable_display_sleep();
	}
}

bool Emulator::Pause()
{
	const u64 start = get_system_time();

	// Try to pause
	if (!m_state.compare_and_swap_test(system_state::running, system_state::paused))
	{
		return m_state.compare_and_swap_test(system_state::ready, system_state::paused);
	}

	// Signal profilers to print results (if enabled)
	cpu_thread::flush_profilers();

	GetCallbacks().on_pause();

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
	// Get pause start time
	const u64 time = m_pause_start_time.exchange(0);

	// Try to increment summary pause time
	if (time)
	{
		m_pause_amend_time += get_system_time() - time;
	}

	// Print and reset debug data collected
	if (m_state == system_state::paused && g_cfg.core.ppu_debug)
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

	perf_stat_base::report();

	// Try to resume
	if (!m_state.compare_and_swap_test(system_state::paused, system_state::running))
	{
		return;
	}

	if (!time)
	{
		sys_log.error("Emulator::Resume() error: concurrent access");
	}

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

	if (g_cfg.misc.prevent_display_sleep)
	{
		disable_display_sleep();
	}
}

void Emulator::Stop(bool restart)
{
	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		if (restart)
		{
			// Reload with prior configs.
			if (const auto error = Load(m_title_id, false, m_force_global_config); error != game_boot_result::no_errors)
				sys_log.error("Restart failed: %s", error);
			return;
		}

		m_force_boot = false;
		return;
	}

	named_thread stop_watchdog("Stop Watchdog", [&]()
	{
		for (uint i = 0; thread_ctrl::state() != thread_state::aborting;)
		{
			// We don't need accurate timekeeping, using clocks may interfere with debugging
			if (i >= 1000)
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

	sys_log.notice("Stopping emulator...");

	GetCallbacks().on_stop();

	if (auto rsx = g_fxo->try_get<rsx::thread>())
	{
		// TODO: notify?
		rsx->state += cpu_flag::exit;
	}

	cpu_thread::stop_all();
	g_fxo->reset();

	sys_log.notice("All threads have been stopped.");

	lv2_obj::cleanup();
	idm::clear();

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

	atomic_wait::parse_hashtable([](u64 id, u32 refs, u64 ptr, u32 maxc) -> bool
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

	if (restart)
	{
		// Reload with prior configs.
		if (const auto error = Load(m_title_id, false, m_force_global_config); error != game_boot_result::no_errors)
			sys_log.error("Restart failed: %s", error);
		return;
	}

	// Boot arg cleanup (preserved in the case restarting)
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();
	hdd1.clear();

	// Always Enable display sleep, not only if it was prevented.
	enable_display_sleep();

	if (!m_force_boot)
	{
		if (Quit(g_cfg.misc.autoexit.get()))
		{
			return;
		}
	}

	m_force_boot = false;
}

bool Emulator::Quit(bool force_quit)
{
	m_force_boot = false;

	// The callback is only used if we actually quit RPCS3
	const auto on_exit = []()
	{
		// Deinitialize object manager to prevent any hanging objects at program exit
		g_fxo->clear();
	};
	return GetCallbacks().try_to_quit(force_quit, on_exit);
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

u32 Emulator::GetMaxThreads() const
{
	const u32 max_threads = static_cast<u32>(g_cfg.core.llvm_threads);
	const u32 hw_threads = utils::get_thread_count();
	const u32 thread_count = max_threads > 0 ? std::min(max_threads, hw_threads) : hw_threads;
	return thread_count;
}

s32 error_code::error_report(const fmt_type_info* sup, u64 arg, const fmt_type_info* sup2, u64 arg2)
{
	static thread_local std::unordered_map<std::string, usz> g_tls_error_stats;
	static thread_local std::string g_tls_error_str;

	if (!sup)
	{
		if (!sup2)
		{
			// Report and clean error state
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

	logs::channel* channel = &sys_log;
	const char* func = "Unknown function";

	if (auto thread = get_current_cpu_thread())
	{
		if (thread->id_type() == 1)
		{
			auto& ppu = static_cast<ppu_thread&>(*thread);

			if (ppu.current_function)
			{
				func = ppu.current_function;
			}
		}
	}

	// Format log message (use preallocated buffer)
	g_tls_error_str.clear();
	fmt::append(g_tls_error_str, "'%s' failed with 0x%08x%s%s%s%s", func, arg, sup ? " : " : "", std::make_pair(sup, arg), sup2 ? ", " : "", std::make_pair(sup2, arg2));

	// Update stats and check log threshold
	const auto stat = ++g_tls_error_stats[g_tls_error_str];

	if (stat <= 3)
	{
		channel->error("%s [%u]", g_tls_error_str, stat);
	}

	return static_cast<s32>(arg);
}

void Emulator::ConfigureLogs()
{
	static bool was_silenced = false;

	const bool silenced = g_cfg.misc.silence_all_logs.get();

	if (silenced)
	{
		if (!was_silenced)
		{
			sys_log.notice("Disabling logging...");
		}

		logs::silence();
	}
	else
	{
		logs::reset();
		logs::set_channel_levels(g_cfg.log.get_map());

		if (was_silenced)
		{
			sys_log.notice("Logging enabled");
		}
	}

	was_silenced = silenced;
}

void Emulator::ConfigurePPUCache()
{
	auto& _main = g_fxo->get<ppu_module>();

	_main.cache = GetCacheDir();

	if (!m_title_id.empty() && m_cat != "1P")
	{
		_main.cache += Emu.GetTitleID();
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

Emulator Emu;
