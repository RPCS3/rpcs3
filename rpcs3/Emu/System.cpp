#include "stdafx.h"
#include "Utilities/bin_patch.h"
#include "Emu/Memory/vm.h"
#include "Emu/System.h"

#include "Emu/Cell/PPUThread.h"
#include "Emu/Cell/PPUCallback.h"
#include "Emu/Cell/PPUOpcodes.h"
#include "Emu/Cell/PPUDisAsm.h"
#include "Emu/Cell/PPUAnalyser.h"
#include "Emu/Cell/SPUThread.h"
#include "Emu/Cell/RawSPUThread.h"
#include "Emu/Cell/lv2/sys_process.h"
#include "Emu/Cell/lv2/sys_memory.h"
#include "Emu/Cell/lv2/sys_sync.h"
#include "Emu/Cell/lv2/sys_prx.h"
#include "Emu/Cell/lv2/sys_rsx.h"

#include "Emu/IdManager.h"
#include "Emu/RSX/GSRender.h"
#include "Emu/RSX/Capture/rsx_replay.h"

#include "Loader/PSF.h"
#include "Loader/ELF.h"

#include "Utilities/StrUtil.h"
#include "Utilities/sysinfo.h"

#include "../Crypto/unself.h"
#include "../Crypto/unpkg.h"
#include <yaml-cpp/yaml.h>

#include "cereal/archives/binary.hpp"

#include <thread>
#include <typeinfo>
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

stx::manual_fixed_typemap<void> g_fixed_typemap;

cfg_root g_cfg;

bool g_use_rtm;

std::string g_cfg_defaults;

extern void ppu_load_exec(const ppu_exec_object&);
extern void spu_load_exec(const spu_exec_object&);
extern void ppu_initialize(const ppu_module&);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);

fs::file g_tty;
atomic_t<s64> g_tty_size{0};
std::array<std::deque<std::string>, 16> g_tty_input;
std::mutex g_tty_mutex;

// Progress display server synchronization variables
atomic_t<const char*> g_progr{nullptr};
atomic_t<u32> g_progr_ftotal{0};
atomic_t<u32> g_progr_fdone{0};
atomic_t<u32> g_progr_ptotal{0};
atomic_t<u32> g_progr_pdone{0};

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
		case pad_handler::ds3: return "DualShock 3";
		case pad_handler::ds4: return "DualShock 4";
#ifdef _WIN32
		case pad_handler::xinput: return "XInput";
		case pad_handler::mm: return "MMJoystick";
#endif
#ifdef HAVE_LIBEVDEV
		case pad_handler::evdev: return "Evdev";
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
		case video_aspect::_4_3: return "4:3";
		case video_aspect::_16_9: return "16:9";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<msaa_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](msaa_level value)
	{
		switch (value)
		{
		case msaa_level::none: return "Disabled";
		case msaa_level::_auto: return "Auto";
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
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return "ALSA";
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return "PulseAudio";
#endif
		case audio_renderer::openal: return "OpenAL";
#ifdef HAVE_FAUDIO
		case audio_renderer::faudio: return "FAudio";
#endif
		}

		return unknown;
	});
}

template <>
inline void fmt_class_string<detail_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](detail_level value)
	{
		switch (value)
		{
		case detail_level::minimal: return "Minimal";
		case detail_level::low: return "Low";
		case detail_level::medium: return "Medium";
		case detail_level::high: return "High";
		}

		return unknown;
	});
}

template <>
inline void fmt_class_string<screen_quadrant>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](screen_quadrant value)
	{
		switch (value)
		{
		case screen_quadrant::top_left: return "Top Left";
		case screen_quadrant::top_right: return "Top Right";
		case screen_quadrant::bottom_left: return "Bottom Left";
		case screen_quadrant::bottom_right: return "Bottom Right";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<tsx_usage>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](tsx_usage value)
	{
		switch (value)
		{
		case tsx_usage::disabled: return "Disabled";
		case tsx_usage::enabled: return "Enabled";
		case tsx_usage::forced: return "Forced";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<sleep_timers_accuracy_level>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](sleep_timers_accuracy_level value)
	{
		switch (value)
		{
		case sleep_timers_accuracy_level::_as_host: return "As Host";
		case sleep_timers_accuracy_level::_usleep: return "Usleep Only";
		case sleep_timers_accuracy_level::_all_timers: return "All Timers";
		}

		return unknown;
	});
}

template <>
void fmt_class_string<enter_button_assign>::format(std::string& out, u64 arg)
{
	format_enum(out, arg, [](enter_button_assign value)
	{
		switch (value)
		{
		case enter_button_assign::circle: return "Enter with circle";
		case enter_button_assign::cross: return "Enter with cross";
		}

		return unknown;
	});
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

	idm::init();
	g_fxo->reset();

	// Reset defaults, cache them
	g_cfg.from_default();
	g_cfg_defaults = g_cfg.to_string();

	// Reload global configuration
	const auto cfg_path = fs::get_config_dir() + "/config.yml";

	if (const fs::file cfg_file{cfg_path, fs::read + fs::create})
	{
		g_cfg.from_string(cfg_file.to_string());
		g_cfg.name = cfg_path;
	}
	else
	{
		sys_log.fatal("Failed to access global config: %s (%s)", cfg_path, fs::g_tls_error);
	}

	// Create directories (can be disabled if necessary)
	const std::string emu_dir = GetEmuDir();
	const std::string dev_hdd0 = GetHddDir();
	const std::string dev_hdd1 = fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir);
	const std::string dev_usb = fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir);

	auto make_path_verbose = [](const std::string& path)
	{
		if (!fs::create_path(path))
		{
			sys_log.fatal("Failed to create path: %s (%s)", path, fs::g_tls_error);
		}
	};

	const std::string save_path = dev_hdd0 + "home/" + m_usr + "/savedata/";

	if (g_cfg.vfs.init_dirs)
	{
		make_path_verbose(dev_hdd0);
		make_path_verbose(dev_hdd1);
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

		if (!fs::write_file(dev_hdd0 + "home/" + m_usr + "/localusername", fs::create + fs::excl + fs::write, "User"s))
		{
			if (fs::g_tls_error != fs::error::exist)
			{
				sys_log.fatal("Failed to create file: %shome/%s/localusername (%s)", dev_hdd0, m_usr, fs::g_tls_error);
			}
		}

		make_path_verbose(dev_hdd0 + "disc/");
		make_path_verbose(dev_hdd0 + "savedata/");
		make_path_verbose(dev_hdd0 + "savedata/vmc/");
		make_path_verbose(dev_hdd1 + "caches/");
	}

	// Fixup savedata
	for (const auto& entry : fs::dir(save_path))
	{
		if (entry.is_directory && entry.name.compare(0, 8, ".backup_", 8) == 0)
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

	make_path_verbose(fs::get_cache_dir() + "shaderlog/");
	make_path_verbose(fs::get_config_dir() + "captures/");

	// Initialize patch engine
	g_fxo->init<patch_engine>()->append(fs::get_config_dir() + "/patch.yml");

	// Initialize progress dialog server (TODO)
	if (g_progr.exchange("") == nullptr)
	{
		std::thread server([]()
		{
			while (true)
			{
				// Wait for the start condition
				while (!g_progr_ftotal && !g_progr_ptotal)
				{
					std::this_thread::sleep_for(5ms);
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

					Emu.CallAfter([=]()
					{
						dlg->Create(+g_progr, +g_progr);
					});
				}

				u32 ftotal = 0;
				u32 fdone = 0;
				u32 ptotal = 0;
				u32 pdone = 0;
				u32 value = 0;

				// Update progress
				while (true)
				{
					if (ftotal != g_progr_ftotal || fdone != g_progr_fdone || ptotal != g_progr_ptotal || pdone != g_progr_pdone)
					{
						ftotal = g_progr_ftotal;
						fdone = g_progr_fdone;
						ptotal = g_progr_ptotal;
						pdone = g_progr_pdone;

						// Compute new progress in percents
						const u32 total = ftotal + ptotal;
						const u32 done = fdone + pdone;
						const u32 new_value = static_cast<u32>(double(done) * 100. / double(total ? total : 1));

						// Compute the difference
						const u32 delta = new_value > value ? new_value - value : 0;

						value += delta;

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
								dlg->ProgressBarInc(0, delta);
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

				// Cleanup
				g_progr_ftotal -= fdone;
				g_progr_fdone  -= fdone;
				g_progr_ptotal -= pdone;
				g_progr_pdone  -= pdone;

				if (dlg)
				{
					Emu.CallAfter([=]
					{
						dlg->Close(true);
					});
				}
			}
		});

		server.detach();
	}
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
	std::string path = m_sfo_dir + "/PIC1.PNG";

	if (!fs::exists(path))
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

			if (!fs::exists(path))
			{
				// Fallback to ICON0.PNG in update dir
				path = m_sfo_dir + "/ICON0.PNG";

				if (!fs::exists(path))
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
	const auto _main = g_fxo->get<ppu_module>();

	if (!_main || _main->cache.empty())
	{
		ppu_log.warning("PPU Cache location not initialized.");
		return {};
	}

	return _main->cache;
}

bool Emulator::BootRsxCapture(const std::string& path)
{
	if (!fs::is_file(path))
		return false;

	std::fstream f(path, std::ios::in | std::ios::binary);

	cereal::BinaryInputArchive archive(f);
	std::unique_ptr<rsx::frame_capture_data> frame = std::make_unique<rsx::frame_capture_data>();
	archive(*frame);

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
	g_fxo->init();

	// PS3 'executable'
	m_state = system_state::ready;
	GetCallbacks().on_ready();

	Emu.GetCallbacks().init_gs_render();
	Emu.GetCallbacks().init_pad_handler("");

	GetCallbacks().on_run(false);
	m_state = system_state::running;

	g_fxo->init<named_thread<rsx::rsx_replay_thread>>("RSX Replay"sv, std::move(frame));

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
	fs::dir cache_dir{};
	if (!cache_dir.open(cache_location))
	{
		sys_log.error("Could not open cache directory");
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
		const u64 item_size = fs::is_dir(name) ? fs::get_dir_size(name) : item.size;

		if (fs::is_dir(name))
		{
			fs::remove_all(name, true);
		}
		else
		{
			fs::remove_file(name);
		}

		removed += item_size;
		if (removed >= to_remove)
			break;
	}

	sys_log.success("Cleaned disk cache, removed %.2f MB", size / 1024.0 / 1024.0);
}

bool Emulator::BootGame(const std::string& path, const std::string& title_id, bool direct, bool add_only, bool force_global_config)
{
	if (g_cfg.vfs.limit_cache_size)
		LimitCacheSize();

	static const char* boot_list[] =
	{
		"/eboot.bin",
		"/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/USRDIR/ISO.BIN.EDAT",
		"/PS3_GAME/USRDIR/EBOOT.BIN",
	};

	m_path_old = m_path;

	if (direct && fs::exists(path))
	{
		m_path = path;
		Load(title_id, add_only, force_global_config);
		return true;
	}

	bool success = false;
	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			m_path = elf;
			Load(title_id, add_only, force_global_config);
			success = true;
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
					Load(title_id, add_only, force_global_config);
					success = true;
				}
			}
		}
	}

	return success;
}

bool Emulator::InstallPkg(const std::string& path)
{
	sys_log.success("Installing package: %s", path);

	atomic_t<double> progress(0.);
	int int_progress = 0;

	// Run PKG unpacking asynchronously
	named_thread worker("PKG Installer", [&]
	{
		return pkg_install(path, progress);
	});

	{
		// Wait for the completion
		while (std::this_thread::sleep_for(5ms), worker != thread_state::finished)
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
					const auto psf           = psf::load_object(fs::file(sfo_path));
					const std::string serial = get_string(psf, "TITLE_ID");
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

	const std::string category   = get_string(psf, "CATEGORY");
	const std::string content_id = get_string(psf, "CONTENT_ID");
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

void Emulator::Load(const std::string& title_id, bool add_only, bool force_global_config)
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

	try
	{
		Init();

		// Load game list (maps ABCD12345 IDs to /dev_bdvd/ locations)
		YAML::Node games = YAML::Load(fs::file{fs::get_config_dir() + "/games.yml", fs::read + fs::create}.to_string());

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
			else if (disc.size())
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

		m_title = psf::get_string(_psf, "TITLE", m_path.substr(m_path.find_last_of('/') + 1));
		m_title_id = psf::get_string(_psf, "TITLE_ID");
		m_cat = psf::get_string(_psf, "CATEGORY");

		std::string version_app  = psf::get_string(_psf, "APP_VER", "Unknown");
		std::string version_disc = psf::get_string(_psf, "VERSION", "Unknown");

		if (!_psf.empty() && m_cat.empty())
		{
			sys_log.fatal("Corrupted PARAM.SFO found! Assuming category GD. Try reinstalling the game.");
			m_cat = "GD";
		}

		sys_log.notice("Title: %s", GetTitle());
		sys_log.notice("Serial: %s", GetTitleID());
		sys_log.notice("Category: %s", GetCat());
		sys_log.notice("Version: %s / %s", version_app, version_disc);

		if (!force_global_config)
		{
			const std::string config_path_new = GetCustomConfigPath(m_title_id);
			const std::string config_path_old = GetCustomConfigPath(m_title_id, true);

			// Load custom config-1
			if (fs::file cfg_file{ config_path_old })
			{
				sys_log.notice("Applying custom config: %s", config_path_old);
				g_cfg.from_string(cfg_file.to_string());
			}

			// Load custom config-2
			if (fs::file cfg_file{ config_path_new })
			{
				sys_log.notice("Applying custom config: %s", config_path_new);
				g_cfg.from_string(cfg_file.to_string());
				g_cfg.name = config_path_new;
			}

			// Load custom config-3
			if (fs::file cfg_file{ m_path + ".yml" })
			{
				sys_log.notice("Applying custom config: %s.yml", m_path);
				g_cfg.from_string(cfg_file.to_string());
			}
		}

#if defined(_WIN32) || defined(HAVE_VULKAN)
		if (g_cfg.video.renderer == video_renderer::vulkan)
		{
			sys_log.notice("Vulkan SDK Revision: %d", VK_HEADER_VERSION);
		}
#endif

		sys_log.notice("Used configuration:\n%s\n", g_cfg.to_string());

		// Set RTM usage
		g_use_rtm = utils::has_rtm() && ((utils::has_mpx() && g_cfg.core.enable_TSX == tsx_usage::enabled) || g_cfg.core.enable_TSX == tsx_usage::forced);

		if (g_use_rtm && !utils::has_mpx())
		{
			sys_log.warning("TSX forced by User");
		}

		// Load patches from different locations
		g_fxo->get<patch_engine>()->append(fs::get_config_dir() + "data/" + m_title_id + "/patch.yml");

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
		if (fs::is_dir(m_path))
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init();
			Run(false);
			m_force_boot = false;

			// Force LLVM recompiler
			g_cfg.core.ppu_decoder.from_default();

			// Workaround for analyser glitches
			vm::falloc(0x10000, 0xf0000, vm::main);

			return thread_ctrl::spawn("SPRX Loader", [this]
			{
				std::vector<std::string> dir_queue;
				dir_queue.emplace_back(m_path + '/');

				std::vector<std::pair<std::string, u64>> file_queue;
				file_queue.reserve(2000);

				std::queue<named_thread<std::function<void()>>> thread_queue;
				const uint max_threads = std::thread::hardware_concurrency();

				// Initialize progress dialog
				g_progr = "Scanning directories for SPRX libraries...";

				// Find all .sprx files recursively (TODO: process .mself files)
				for (std::size_t i = 0; i < dir_queue.size(); i++)
				{
					if (Emu.IsStopped())
					{
						break;
					}

					sys_log.notice("Scanning directory: %s", dir_queue[i]);

					for (auto&& entry : fs::dir(dir_queue[i]))
					{
						if (Emu.IsStopped())
						{
							break;
						}

						if (entry.is_directory)
						{
							if (entry.name != "." && entry.name != "..")
							{
								dir_queue.emplace_back(dir_queue[i] + entry.name + '/');
							}

							continue;
						}

						// Check .sprx filename
						if (entry.name.size() >= 5 && fmt::to_upper(entry.name).compare(entry.name.size() - 5, 5, ".SPRX", 5) == 0)
						{
							if (entry.name == "libfs_155.sprx")
							{
								continue;
							}

							// Get full path
							file_queue.emplace_back(dir_queue[i] + entry.name, 0);
							g_progr_ftotal++;
						}
					}
				}

				g_progr = "Compiling PPU modules";

				atomic_t<u32> worker_count = 0;

				for (std::size_t i = 0; i < file_queue.size(); i++)
				{
					const auto& path = file_queue[i].first;

					sys_log.notice("Trying to load SPRX: %s", path);

					// Load MSELF or SPRX
					fs::file src{path};

					if (file_queue[i].second == 0)
					{
						// Some files may fail to decrypt due to the lack of klic
						src = decrypt_self(std::move(src));
					}

					const ppu_prx_object obj = src;

					if (obj == elf_error::ok)
					{
						if (auto prx = ppu_load_prx(obj, path))
						{
							worker_count++;

							while (worker_count > max_threads)
							{
								std::this_thread::sleep_for(10ms);
							}

							thread_queue.emplace("Worker " + std::to_string(thread_queue.size()), [_prx = std::move(prx), &worker_count]
							{
								ppu_initialize(*_prx);
								ppu_unload_prx(*_prx);
								g_progr_fdone++;
								worker_count--;
							});

							continue;
						}
					}

					sys_log.error("Failed to load SPRX '%s' (%s)", path, obj.get_error());
					g_progr_fdone++;
				}

				// Join every thread
				while (!thread_queue.empty())
				{
					thread_queue.pop();
				}

				// Exit "process"
				Emu.CallAfter([]
				{
					Emu.Stop();
				});
			});
		}

		// Detect boot location
		const std::string hdd0_game = vfs::get("/dev_hdd0/game/");
		const std::string hdd0_disc = vfs::get("/dev_hdd0/disc/");
		const std::size_t game_dir_size = 8; // size of PS3_GAME and PS3_GMXX
		const std::size_t bdvd_pos = m_cat == "DG" && bdvd_dir.empty() && disc.empty() ? elf_dir.rfind("/USRDIR") - game_dir_size : 0;
		const bool from_hdd0_game = m_path.find(hdd0_game) != -1;

		if (bdvd_pos && from_hdd0_game)
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
				return;
			}
		}

		// Booting disc game
		if (bdvd_pos)
		{
			// Mount /dev_bdvd/ if necessary
			bdvd_dir = elf_dir.substr(0, bdvd_pos);
			m_game_dir = elf_dir.substr(bdvd_pos, game_dir_size);
		}
		else
		{
			m_game_dir = "PS3_GAME"; // reset
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

			if (!sfb_file.open(vfs::get("/dev_bdvd/PS3_DISC.SFB")) || sfb_file.size() < 4 || sfb_file.read<u32>() != ".SFB"_u32)
			{
				sys_log.error("Invalid disc directory for the disc game %s", m_title_id);
				return;
			}

			const std::string bdvd_title_id = psf::get_string(psf::load_object(fs::file{vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO")}), "TITLE_ID");

			if (bdvd_title_id != m_title_id)
			{
				sys_log.error("Unexpected disc directory for the disc game %s (found %s)", m_title_id, bdvd_title_id);
				return;
			}

			// Store /dev_bdvd/ location
			games[m_title_id] = bdvd_dir;
			YAML::Emitter out;
			out << games;
			fs::file(fs::get_config_dir() + "/games.yml", fs::rewrite).write(out.c_str(), out.size());
		}
		else if (m_cat == "1P" && from_hdd0_game)
		{
			//PS1 Classics
			sys_log.notice("PS1 Game: %s, %s", m_title_id, m_title);

			std::string gamePath = m_path.substr(m_path.find("/dev_hdd0/game/"), 24);

			sys_log.notice("Forcing manual lib loading mode");
			g_cfg.core.lib_loading.from_string(fmt::format("%s", lib_loading_type::manual));
			g_cfg.core.load_libraries.from_list({});

			argv.resize(9);
			argv[0] = "/dev_flash/ps1emu/ps1_newemu.self";
			argv[1] = m_title_id + "_mc1.VM1";    // virtual mc 1 /dev_hdd0/savedata/vmc/%argv[1]%
			argv[2] = m_title_id + "_mc2.VM1";    // virtual mc 2 /dev_hdd0/savedata/vmc/%argv[2]%
			argv[3] = "0082";                     // region target
			argv[4] = "1600";                     // ??? arg4 600 / 1200 / 1600, resolution scale? (purely a guess, the numbers seem to match closely to resolutions tho)
			argv[5] = gamePath;                   // ps1 game folder path (not the game serial)
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
		else if (m_cat != "DG" && m_cat != "GD")
		{
			// Don't need /dev_bdvd
		}
		else if (m_cat == "DG" && from_hdd0_game)
		{
			vfs::mount("/dev_bdvd/PS3_GAME", hdd0_game + m_path.substr(hdd0_game.size(), 10));
			sys_log.notice("Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));
		}
		else if (disc.empty())
		{
			sys_log.error("Failed to mount disc directory for the disc game %s", m_title_id);
			return;
		}
		else
		{
			bdvd_dir = disc;
			vfs::mount("/dev_bdvd", bdvd_dir);
			sys_log.notice("Disk: %s", vfs::get("/dev_bdvd"));
		}

		if (add_only)
		{
			sys_log.notice("Finished to add data to games.yml by boot for: %s", m_path);
			m_path = m_path_old; // Reset m_path to fix boot from gui
			return;
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
					if (!entry.is_directory && ends_with(entry.name, ".PKG") && !InstallPkg(pkg))
					{
						sys_log.error("Failed to install %s", pkg);
						return;
					}
				}
			}

			if (lock_file && fs::is_dir(pkg_dir))
			{
				sys_log.notice("Found PKGDIR: %s", pkg_dir);

				for (auto&& entry : fs::dir{pkg_dir})
				{
					if (entry.is_directory && entry.name.compare(0, 3, "PKG", 3) == 0)
					{
						const std::string pkg_file = pkg_dir + entry.name + "/INSTALL.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							sys_log.error("Failed to install %s", pkg_file);
							return;
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
							return;
						}
					}
				}
			}
		}

		// Check game updates
		const std::string hdd0_boot = hdd0_game + m_title_id + "/USRDIR/EBOOT.BIN";

		if (disc.empty() && m_cat == "DG" && fs::is_file(hdd0_boot))
		{
			// Booting game update
			sys_log.success("Updates found at /dev_hdd0/game/%s/!", m_title_id);
			return m_path = hdd0_boot, Load(m_title_id, false, force_global_config);
		}

		// Set title to actual disc title if necessary
		const std::string disc_sfo_dir = vfs::get("/dev_bdvd/PS3_GAME/PARAM.SFO");

		if (!disc_sfo_dir.empty() && fs::is_file(disc_sfo_dir))
		{
			const std::string bdvd_title = psf::get_string(psf::load_object(fs::file{ disc_sfo_dir }), "TITLE");

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

		if (m_cat == "1P")
		{
			// Use emulator path
			elf_path = vfs::get(argv[0]);
		}

		fs::file elf_file(elf_path);

		if (!elf_file)
		{
			sys_log.error("Failed to open executable: %s", elf_path);
			return;
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
			else if ((elf_file = decrypt_self(std::move(elf_file), klic.empty() ? nullptr : klic.data(), &g_ps3_process_info.self_info)))
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
			return;
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
					const std::size_t pos = m_path.rfind(m_game_dir);
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

			const auto _main = g_fxo->init<ppu_module>();

			ppu_load_exec(ppu_exec);

			_main->cache = fs::get_cache_dir() + "cache/";

			if (!m_title_id.empty() && m_cat != "1P")
			{
				// TODO
				_main->cache += Emu.GetTitleID();
				_main->cache += '/';
			}

			fmt::append(_main->cache, "ppu-%s-%s/", fmt::base57(_main->sha1), _main->path.substr(_main->path.find_last_of('/') + 1));

			if (!fs::create_path(_main->cache))
			{
				fmt::throw_exception("Failed to create cache directory: %s (%s)", _main->cache, fs::g_tls_error);
			}
			else
			{
				sys_log.notice("Cache: %s", _main->cache);
			}

			g_fxo->init();
			Emu.GetCallbacks().init_gs_render();
			Emu.GetCallbacks().init_pad_handler(m_title_id);
			Emu.GetCallbacks().init_kb_handler();
			Emu.GetCallbacks().init_mouse_handler();
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init();
			ppu_load_prx(ppu_prx, m_path);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			g_fxo->init();
			spu_load_exec(spu_exec);
		}
		else
		{
			sys_log.error("Invalid or unsupported file format: %s", elf_path);

			sys_log.warning("** ppu_exec -> %s", ppu_exec.get_error());
			sys_log.warning("** ppu_prx  -> %s", ppu_prx.get_error());
			sys_log.warning("** spu_exec -> %s", spu_exec.get_error());
			return;
		}

		if ((m_force_boot || g_cfg.misc.autostart) && IsReady())
		{
			Run(true);
			m_force_boot = false;
		}
		else if (IsPaused())
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
		}
	}
	catch (const std::exception& e)
	{
		sys_log.fatal("%s thrown: %s", typeid(e).name(), e.what());
		Stop();
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

	if (g_cfg.misc.silence_all_logs)
	{
		sys_log.notice("Now disabling logging...");
		logs::silence();
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_flag::stop;
		cpu.notify();
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);

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

	if (g_cfg.misc.prevent_display_sleep)
	{
		enable_display_sleep();
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

	// Print and reset debug data collected
	if (m_state == system_state::paused && g_cfg.core.ppu_debug)
	{
		PPUDisAsm dis_asm(CPUDisAsm_InterpreterMode);
		dis_asm.offset = vm::g_base_addr;

		std::string dump;

		for (u32 i = 0x10000; i < 0x20000000;)
		{
			if (vm::check_addr(i))
			{
				if (auto& data = *reinterpret_cast<be_t<u32>*>(vm::g_stat_addr + i))
				{
					dis_asm.dump_pc = i;
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

	if (!time)
	{
		sys_log.error("Emulator::Resume() error: concurrent access");
	}

	auto on_select = [](u32, cpu_thread& cpu)
	{
		cpu.state -= cpu_flag::dbg_global_pause;
		cpu.notify();
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);
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
			return Load(m_title_id, false, m_force_global_config);
		}

		m_force_boot = false;
		return;
	}

	const bool full_stop = !restart && !m_force_boot;
	const bool do_exit   = full_stop && g_cfg.misc.autoexit;

	sys_log.notice("Stopping emulator...");

	GetCallbacks().on_stop();

	cpu_thread::stop_all();
	g_fxo->reset();
	lv2_obj::cleanup();
	idm::clear();

	sys_log.notice("Objects cleared...");

	vm::close();

	if (do_exit)
	{
		GetCallbacks().exit(true);
	}
	else
	{
		if (full_stop)
		{
			GetCallbacks().exit(false);
		}
		Init();
	}

#ifdef LLVM_AVAILABLE
	extern void jit_finalize();
	jit_finalize();
#endif
	jit_runtime::finalize();

	if (restart)
	{
		// Reload with prior configs.
		return Load(m_title_id, false, m_force_global_config);
	}

	// Boot arg cleanup (preserved in the case restarting)
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();
	hdd1.clear();

	m_force_boot = false;

	if (g_cfg.misc.prevent_display_sleep)
	{
		enable_display_sleep();
	}
}

std::string cfg_root::node_vfs::get(const cfg::string& _cfg, const char* _def) const
{
	auto [spath, sshared] = _cfg.get();

	if (spath.empty())
	{
		return fs::get_config_dir() + _def;
	}

	auto [semudir, sshared2] = emulator_dir.get();

	return fmt::replace_all(spath, "$(EmulatorDir)", semudir.empty() ? fs::get_config_dir() : semudir);
}

s32 error_code::error_report(const fmt_type_info* sup, u64 arg, const fmt_type_info* sup2, u64 arg2)
{
	static thread_local std::unordered_map<std::string, std::size_t> g_tls_error_stats;
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

Emulator Emu;
