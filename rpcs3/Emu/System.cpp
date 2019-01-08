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

#include "Utilities/GDBDebugServer.h"

#include "Utilities/sysinfo.h"

#if defined(_WIN32) || defined(HAVE_VULKAN)
#include "Emu/RSX/VK/VulkanAPI.h"
#endif

utils::typemap g_typemap{nullptr};

cfg_root g_cfg;

bool g_use_rtm;

std::string g_cfg_defaults;

extern atomic_t<u32> g_thread_count;

extern void ppu_load_exec(const ppu_exec_object&);
extern void spu_load_exec(const spu_exec_object&);
extern void ppu_initialize(const ppu_module&);
extern void ppu_unload_prx(const lv2_prx&);
extern std::shared_ptr<lv2_prx> ppu_load_prx(const ppu_prx_object&, const std::string&);

extern void network_thread_init();

fs::file g_tty;
atomic_t<s64> g_tty_size{0};
std::array<std::deque<std::string>, 16> g_tty_input;
std::mutex g_tty_mutex;

// Progress display server synchronization variables
atomic_t<const char*> g_progr{nullptr};
atomic_t<u64> g_progr_ftotal{0};
atomic_t<u64> g_progr_fdone{0};
atomic_t<u64> g_progr_ptotal{0};
atomic_t<u64> g_progr_pdone{0};

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
#endif
#ifdef HAVE_ALSA
		case audio_renderer::alsa: return "ALSA";
#endif
#ifdef HAVE_PULSE
		case audio_renderer::pulse: return "PulseAudio";
#endif
		case audio_renderer::openal: return "OpenAL";
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
	if (!g_tty)
	{
		g_tty.open(fs::get_config_dir() + "TTY.log", fs::rewrite + fs::append);
	}

	idm::init();
	fxm::init();
	g_idm->init();

	// Reset defaults, cache them
	g_cfg.from_default();
	g_cfg_defaults = g_cfg.to_string();

	// Reload global configuration
	g_cfg.from_string(fs::file(fs::get_config_dir() + "/config.yml", fs::read + fs::create).to_string());

	// Create directories (can be disabled if necessary)
	const std::string emu_dir = GetEmuDir();
	const std::string dev_hdd0 = GetHddDir();
	const std::string dev_hdd1 = fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir);
	const std::string dev_usb = fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir);

	if (g_cfg.vfs.init_dirs)
	{
	fs::create_path(dev_hdd0);
	fs::create_path(dev_hdd1);
	fs::create_path(dev_usb);
	fs::create_dir(dev_hdd0 + "game/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/");
	fs::create_dir(dev_hdd0 + "game/TEST12345/USRDIR/");
	fs::create_dir(dev_hdd0 + "game/.locks/");
	fs::create_dir(dev_hdd0 + "home/");
	fs::create_dir(dev_hdd0 + "home/" + m_usr + "/");
	fs::create_dir(dev_hdd0 + "home/" + m_usr + "/exdata/");
	fs::create_dir(dev_hdd0 + "home/" + m_usr + "/savedata/");
	fs::create_dir(dev_hdd0 + "home/" + m_usr + "/trophy/");
	fs::write_file(dev_hdd0 + "home/" + m_usr + "/localusername", fs::create + fs::excl + fs::write, "User"s);
	fs::create_dir(dev_hdd0 + "disc/");
	fs::create_dir(dev_hdd0 + "savedata/");
	fs::create_dir(dev_hdd0 + "savedata/vmc/");
	fs::create_dir(dev_hdd1 + "cache/");
	fs::create_dir(dev_hdd1 + "game/");
	}

	fs::create_path(fs::get_config_dir() + "shaderlog/");
	fs::create_path(fs::get_config_dir() + "captures/");

#ifdef WITH_GDB_DEBUGGER
	LOG_SUCCESS(GENERAL, "GDB debug server will be started and listening on %d upon emulator boot", (int) g_cfg.misc.gdb_server_port);
#endif

	// Initialize patch engine
	fxm::make_always<patch_engine>()->append(fs::get_config_dir() + "/patch.yml");

	// Initialize progress dialog server (TODO)
	if (g_progr.exchange("") == nullptr)
	{
		std::thread server([]()
		{
			while (true)
			{
				std::shared_ptr<MsgDialogBase> dlg;

				// Wait for the start condition
				while (!g_progr_ftotal && !g_progr_ptotal)
				{
					std::this_thread::sleep_for(5ms);
				}

				// Initialize message dialog
				dlg = Emu.GetCallbacks().get_msg_dialog();
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

				u64 ftotal = 0;
				u64 fdone = 0;
				u64 ptotal = 0;
				u64 pdone = 0;
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
						const u32 new_value = ((ptotal ? pdone * 1. / ptotal : 0.) + fdone) * 100. / (ftotal ? ftotal : 1);

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

							dlg->SetMsg(+g_progr);
							dlg->ProgressBarSetMsg(0, progr);
							dlg->ProgressBarInc(0, delta);
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

	u32 id;

	try
	{
		id = static_cast<u32>(std::stoul(user));
	}
	catch (const std::exception&)
	{
		id = 0;
	}

	if (id == 0)
	{
		return false;
	}

	m_usrid = id;
	m_usr = user;
	return true;
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
		LOG_ERROR(LOADER, "Invalid rsx capture file!");
		return false;
	}

	if (frame->version != rsx::FRAME_CAPTURE_VERSION)
	{
		LOG_ERROR(LOADER, "Rsx capture file version not supported! Expected %d, found %d", rsx::FRAME_CAPTURE_VERSION, frame->version);
		return false;
	}

	Init();

	vm::init();

	// PS3 'executable'
	m_state = system_state::ready;
	GetCallbacks().on_ready();

	auto gsrender = fxm::import<GSRender>(Emu.GetCallbacks().get_gs_render);
	auto padhandler = fxm::import<pad_thread>(Emu.GetCallbacks().get_pad_handler);

	if (gsrender.get() == nullptr || padhandler.get() == nullptr)
		return false;

	GetCallbacks().on_run();
	m_state = system_state::running;

	fxm::make<named_thread<rsx::rsx_replay_thread>>("RSX Replay", std::move(frame));

	return true;
}

void Emulator::LimitCacheSize()
{
	const std::string cache_location = Emulator::GetHdd1Dir() + "/cache";
	if (!fs::is_dir(cache_location))
	{
		LOG_WARNING(GENERAL, "Cache does not exist (%s)", cache_location);
		return;
	}

	const u64 size = fs::get_dir_size(cache_location);
	const u64 max_size = static_cast<u64>(g_cfg.vfs.cache_max_size) * 1024 * 1024;

	if (max_size == 0) // Everything must go, so no need to do checks
	{
		fs::remove_all(cache_location, false);
		LOG_SUCCESS(GENERAL, "Cleared disk cache");
		return;
	}

	if (size <= max_size)
	{
		LOG_TRACE(GENERAL, "Cache size below limit: %llu/%llu", size, max_size);
		return;
	}

	LOG_SUCCESS(GENERAL, "Cleaning disk cache...");
	std::vector<fs::dir_entry> file_list{};
	fs::dir cache_dir{};
	if (!cache_dir.open(cache_location))
	{
		LOG_ERROR(GENERAL, "Could not open cache directory");
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

	LOG_SUCCESS(GENERAL, "Cleaned disk cache, removed %.2f MB", size / 1024.0 / 1024.0);
}

bool Emulator::BootGame(const std::string& path, bool direct, bool add_only)
{
	if (g_cfg.vfs.limit_cache_size)
		LimitCacheSize();

	static const char* boot_list[] =
	{
		"/PS3_GAME/USRDIR/EBOOT.BIN",
		"/USRDIR/EBOOT.BIN",
		"/EBOOT.BIN",
		"/eboot.bin",
		"/USRDIR/ISO.BIN.EDAT",
	};

	if (direct && fs::exists(path))
	{
		m_path = path;
		Load(add_only);
		return true;
	}

	for (std::string elf : boot_list)
	{
		elf = path + elf;

		if (fs::is_file(elf))
		{
			m_path = elf;
			Load(add_only);
			return true;
		}
	}

	return false;
}

bool Emulator::InstallPkg(const std::string& path)
{
	LOG_SUCCESS(GENERAL, "Installing package: %s", path);

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
				LOG_SUCCESS(GENERAL, "... %u%%", int_progress);
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

std::string Emulator::GetSfoDirFromGamePath(const std::string& game_path, const std::string& user)
{
	if (fs::is_file(game_path + "/PS3_DISC.SFB"))
	{
		// This is a disc game.
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

void Emulator::SetForceBoot(bool force_boot)
{
	m_force_boot = force_boot;
}

void Emulator::Load(bool add_only)
{
	if (!IsStopped())
	{
		Stop();
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

		LOG_NOTICE(LOADER, "Path: %s", m_path);

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
				m_sfo_dir = GetSfoDirFromGamePath(m_path, GetUsr());
			}
			else if (disc.size())
			{
				// Check previously used category before it's overwritten
				if (m_cat == "DG")
				{
					m_sfo_dir = disc + "/PS3_GAME";
				}
				else if (m_cat == "GD")
				{
					m_sfo_dir = GetHddDir() + "game/" + m_title_id;
				}
				else
				{
					m_sfo_dir = GetSfoDirFromGamePath(disc, GetUsr());
				}
			}
			else
			{
				m_sfo_dir = GetSfoDirFromGamePath(elf_dir + "/../", GetUsr());
			}

			_psf = psf::load_object(fs::file(m_sfo_dir + "/PARAM.SFO"));
		}

		m_title = psf::get_string(_psf, "TITLE", m_path.substr(m_path.find_last_of('/') + 1));
		m_title_id = psf::get_string(_psf, "TITLE_ID");
		m_cat = psf::get_string(_psf, "CATEGORY");

		for (auto& c : m_title)
		{
			// Replace newlines with spaces
			if (c == '\n')
				c = ' ';
		}

		if (!_psf.empty() && m_cat.empty())
		{
			LOG_FATAL(LOADER, "Corrupted PARAM.SFO found! Assuming category GD. Try reinstalling the game.");
			m_cat = "GD";
		}

		LOG_NOTICE(LOADER, "Title: %s", GetTitle());
		LOG_NOTICE(LOADER, "Serial: %s", GetTitleID());
		LOG_NOTICE(LOADER, "Category: %s", GetCat());

		// Initialize data/cache directory
		if (fs::is_dir(m_path))
		{
			m_cache_path = fs::get_config_dir() + "data/" + GetTitleID() + '/';
			LOG_NOTICE(LOADER, "Cache: %s", GetCachePath());
		}
		else
		{
			m_cache_path = fs::get_data_dir(m_title_id, m_path);
			LOG_NOTICE(LOADER, "Cache: %s", GetCachePath());
		}

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

#if defined(_WIN32) || defined(HAVE_VULKAN)
		if (g_cfg.video.renderer == video_renderer::vulkan)
		{
			LOG_NOTICE(LOADER, "Vulkan SDK Revision: %d", VK_HEADER_VERSION);
		}
#endif

		LOG_NOTICE(LOADER, "Used configuration:\n%s\n", g_cfg.to_string());

		// Set RTM usage
		g_use_rtm = utils::has_rtm() && ((utils::has_mpx() && g_cfg.core.enable_TSX == tsx_usage::enabled) || g_cfg.core.enable_TSX == tsx_usage::forced);
		if (g_use_rtm && !utils::has_mpx())
		{
			LOG_WARNING(GENERAL, "TSX forced by User");
		}

		// Load patches from different locations
		fxm::check_unlocked<patch_engine>()->append(fs::get_config_dir() + "data/" + m_title_id + "/patch.yml");
		fxm::check_unlocked<patch_engine>()->append(m_cache_path + "/patch.yml");

		// Mount all devices
		const std::string emu_dir = GetEmuDir();
		const std::string home_dir = g_cfg.vfs.app_home;
		std::string bdvd_dir = g_cfg.vfs.dev_bdvd;

		// Mount default relative path to non-existent directory
		vfs::mount("/dev_hdd0", fmt::replace_all(g_cfg.vfs.dev_hdd0, "$(EmulatorDir)", emu_dir));
		vfs::mount("/dev_hdd1", fmt::replace_all(g_cfg.vfs.dev_hdd1, "$(EmulatorDir)", emu_dir));
		vfs::mount("/dev_flash", g_cfg.vfs.get_dev_flash());
		vfs::mount("/dev_usb", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("/dev_usb000", fmt::replace_all(g_cfg.vfs.dev_usb000, "$(EmulatorDir)", emu_dir));
		vfs::mount("/app_home", home_dir.empty() ? elf_dir + '/' : fmt::replace_all(home_dir, "$(EmulatorDir)", emu_dir));

		// Special boot mode (directory scan)
		if (fs::is_dir(m_path))
		{
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			Run();
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

					LOG_NOTICE(LOADER, "Scanning directory: %s", dir_queue[i]);

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

				for (std::size_t i = 0; i < file_queue.size(); i++)
				{
					const auto& path = file_queue[i].first;

					LOG_NOTICE(LOADER, "Trying to load SPRX: %s", path);

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
							while (g_thread_count >= max_threads + 2)
							{
								std::this_thread::sleep_for(10ms);
							}

							thread_queue.emplace("Worker " + std::to_string(thread_queue.size()), [_prx = std::move(prx)]
							{
								ppu_initialize(*_prx);
								ppu_unload_prx(*_prx);
								g_progr_fdone++;
							});

							continue;
						}
					}

					LOG_ERROR(LOADER, "Failed to load SPRX '%s' (%s)", path, obj.get_error());
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
		const std::size_t bdvd_pos = m_cat == "DG" && bdvd_dir.empty() && disc.empty() ? elf_dir.rfind("/PS3_GAME/") + 1 : 0;
		const bool from_hdd0_game = m_path.find(hdd0_game) != -1;

		if (bdvd_pos && from_hdd0_game)
		{
			// Booting disc game from wrong location
			LOG_ERROR(LOADER, "Disc game %s found at invalid location /dev_hdd0/game/", m_title_id);

			// Move and retry from correct location
			if (fs::rename(elf_dir + "/../../", hdd0_disc + elf_dir.substr(hdd0_game.size()) + "/../../", false))
			{
				LOG_SUCCESS(LOADER, "Disc game %s moved to special location /dev_hdd0/disc/", m_title_id);
				return m_path = hdd0_disc + m_path.substr(hdd0_game.size()), Load();
			}
			else
			{
				LOG_ERROR(LOADER, "Failed to move disc game %s to /dev_hdd0/disc/ (%s)", m_title_id, fs::g_tls_error);
				return;
			}
		}

		// Booting disc game
		if (bdvd_pos)
		{
			// Mount /dev_bdvd/ if necessary
			bdvd_dir = elf_dir.substr(0, bdvd_pos);
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
				LOG_FATAL(LOADER, "Disc directory not found. Try to run the game from the actual game disc directory.");
			}
		}

		// Check /dev_bdvd/
		if (disc.empty() && !bdvd_dir.empty() && fs::is_dir(bdvd_dir))
		{
			fs::file sfb_file;

			vfs::mount("/dev_bdvd", bdvd_dir);
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
		else if (m_cat == "1P" && from_hdd0_game)
		{
			//PS1 Classics
			LOG_NOTICE(LOADER, "PS1 Game: %s, %s", m_title_id, m_title);

			std::string gamePath = m_path.substr(m_path.find("/dev_hdd0/game/"), 24);

			LOG_NOTICE(LOADER, "Forcing manual lib loading mode");
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

			m_cache_path = fs::get_data_dir("", vfs::get(argv[0]));
		}
		else if (m_cat != "DG" && m_cat != "GD")
		{
			// Don't need /dev_bdvd
		}
		else if (m_cat == "DG" && from_hdd0_game)
		{
			vfs::mount("/dev_bdvd/PS3_GAME", hdd0_game + m_path.substr(hdd0_game.size(), 10));
			LOG_NOTICE(LOADER, "Game: %s", vfs::get("/dev_bdvd/PS3_GAME"));
		}
		else if (disc.empty())
		{
			LOG_ERROR(LOADER, "Failed to mount disc directory for the disc game %s", m_title_id);
			return;
		}
		else
		{
			bdvd_dir = disc;
			vfs::mount("/dev_bdvd", bdvd_dir);
			LOG_NOTICE(LOADER, "Disk: %s", vfs::get("/dev_bdvd"));
		}

		if (add_only)
		{
			LOG_NOTICE(LOADER, "Finished to add data to games.yml by boot for: %s", m_path);
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
				LOG_NOTICE(LOADER, "Found INSDIR: %s", ins_dir);

				for (auto&& entry : fs::dir{ins_dir})
				{
					if (!entry.is_directory && ends_with(entry.name, ".PKG") && !InstallPkg(ins_dir + entry.name))
					{
						LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/INSDIR/%s", entry.name);
						return;
					}
				}
			}

			if (lock_file && fs::is_dir(pkg_dir))
			{
				LOG_NOTICE(LOADER, "Found PKGDIR: %s", pkg_dir);

				for (auto&& entry : fs::dir{pkg_dir})
				{
					if (entry.is_directory && entry.name.compare(0, 3, "PKG", 3) == 0)
					{
						const std::string pkg_file = pkg_dir + entry.name + "/INSTALL.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/PKGDIR/%s/INSTALL.PKG", entry.name);
							return;
						}
					}
				}
			}

			if (lock_file && fs::is_dir(extra_dir))
			{
				LOG_NOTICE(LOADER, "Found PS3_EXTRA: %s", extra_dir);

				for (auto&& entry : fs::dir{extra_dir})
				{
					if (entry.is_directory && entry.name[0] == 'D')
					{
						const std::string pkg_file = extra_dir + entry.name + "/DATA000.PKG";

						if (fs::is_file(pkg_file) && !InstallPkg(pkg_file))
						{
							LOG_ERROR(LOADER, "Failed to install /dev_bdvd/PS3_GAME/PKGDIR/%s/DATA000.PKG", entry.name);
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
			LOG_SUCCESS(LOADER, "Updates found at /dev_hdd0/game/%s/!", m_title_id);
			return m_path = hdd0_boot, Load();
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
			LOG_ERROR(LOADER, "Failed to open executable: %s", elf_path);
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
			// Decrypt SELF
			else if (elf_file = decrypt_self(std::move(elf_file), klic.empty() ? nullptr : klic.data()))
			{
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

		if (!elf_file)
		{
			LOG_ERROR(LOADER, "Failed to decrypt SELF: %s", elf_path);
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
					LOG_NOTICE(LOADER, "Disc path: %s", m_dir);
				}
				else if (from_hdd0_game)
				{
					argv[0] = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size());
					m_dir = "/dev_hdd0/game/" + m_path.substr(hdd0_game.size(), 10);
					LOG_NOTICE(LOADER, "Boot path: %s", m_dir);
				}
				else if (!bdvd_dir.empty() && fs::is_dir(bdvd_dir))
				{
					// Disc games are on /dev_bdvd/
					const std::size_t pos = m_path.rfind("PS3_GAME");
					argv[0] = "/dev_bdvd/" + m_path.substr(pos);
					m_dir = "/dev_bdvd/PS3_GAME/";
				}
				else
				{
					// For homebrew
					argv[0] = "/host_root/" + m_path;
					m_dir = "/host_root/" + elf_dir + '/';
				}

				LOG_NOTICE(LOADER, "Elf path: %s", argv[0]);
			}

			ppu_load_exec(ppu_exec);

			fxm::import<GSRender>(Emu.GetCallbacks().get_gs_render); // TODO: must be created in appropriate sys_rsx syscall
			fxm::import<pad_thread>(Emu.GetCallbacks().get_pad_handler);
			network_thread_init();
		}
		else if (ppu_prx.open(elf_file) == elf_error::ok)
		{
			// PPU PRX (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			ppu_load_prx(ppu_prx, m_path);
		}
		else if (spu_exec.open(elf_file) == elf_error::ok)
		{
			// SPU executable (experimental)
			m_state = system_state::ready;
			GetCallbacks().on_ready();
			vm::init();
			spu_load_exec(spu_exec);
		}
		else
		{
			LOG_ERROR(LOADER, "Invalid or unsupported file format: %s", elf_path);

			LOG_WARNING(LOADER, "** ppu_exec -> %s", ppu_exec.get_error());
			LOG_WARNING(LOADER, "** ppu_prx  -> %s", ppu_prx.get_error());
			LOG_WARNING(LOADER, "** spu_exec -> %s", spu_exec.get_error());
			return;
		}

		if ((m_force_boot || g_cfg.misc.autostart) && IsReady())
		{
			Run();
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
		cpu.state -= cpu_flag::stop;
		cpu.notify();
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);

#ifdef WITH_GDB_DEBUGGER
	// Initialize debug server at the end of emu run sequence
	fxm::make<GDBDebugServer>();
#endif
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

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);
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
				if (auto& data = *(be_t<u32>*)(vm::g_stat_addr + i))
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

		LOG_NOTICE(PPU, "[RESUME] Dumping instruction stats:%s", dump);
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

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);
	GetCallbacks().on_resume();
}

void Emulator::Stop(bool restart)
{
	if (m_state.exchange(system_state::stopped) == system_state::stopped)
	{
		if (restart)
		{
			return Load();
		}

		m_force_boot = false;
		return;
	}

	const bool do_exit = !restart && !m_force_boot && g_cfg.misc.autoexit;

	LOG_NOTICE(GENERAL, "Stopping emulator...");

	GetCallbacks().on_stop();

#ifdef WITH_GDB_DEBUGGER
	//fxm for some reason doesn't call on_stop
	fxm::get<GDBDebugServer>()->on_stop();
	fxm::remove<GDBDebugServer>();
#endif

	auto on_select = [&](u32, cpu_thread& cpu)
	{
		cpu.state += cpu_flag::dbg_global_stop;
		cpu.notify();
	};

	idm::select<named_thread<ppu_thread>>(on_select);
	idm::select<named_thread<spu_thread>>(on_select);

	LOG_NOTICE(GENERAL, "All threads signaled...");

	while (g_thread_count)
	{
		std::this_thread::sleep_for(10ms);
	}

	LOG_NOTICE(GENERAL, "All threads stopped...");

	lv2_obj::cleanup();
	idm::clear();
	fxm::clear();
	g_idm->init();

	LOG_NOTICE(GENERAL, "Objects cleared...");

	vm::close();

	if (do_exit)
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

	if (restart)
	{
		return Load();
	}

	// Boot arg cleanup (preserved in the case restarting)
	argv.clear();
	envp.clear();
	data.clear();
	disc.clear();
	klic.clear();

	m_force_boot = false;
}

std::string cfg_root::node_vfs::get(const cfg::string& _cfg, const char* _def) const
{
	if (_cfg.get().empty())
	{
		return fs::get_config_dir() + _def;
	}

	return fmt::replace_all(_cfg.get(), "$(EmulatorDir)", emulator_dir.get().empty() ? fs::get_config_dir() : emulator_dir.get());
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
					LOG_ERROR(GENERAL, "Stat: %s [x%u]", pair.first, pair.second);
				}
			}

			g_tls_error_stats.clear();
			return 0;
		}
	}

	logs::channel* channel = &logs::GENERAL;
	const char* func = "Unknown function";

	if (auto thread = get_current_cpu_thread())
	{
		if (thread->id_type() == 1)
		{
			auto& ppu = static_cast<ppu_thread&>(*thread);

			if (ppu.last_function)
			{
				func = ppu.last_function;
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
